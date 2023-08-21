////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchicontext.cpp
/// @brief Definitions for ChiContext class containing CHI API specific information
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcommontypes.h"
#include "camxchi.h"
#include "camxchisession.h"
#include "camxhwcontext.h"
#include "camxhwenvironment.h"
#include "camximagesensormoduledata.h"
#include "camximagesensordata.h"
#include "camxpipeline.h"
#include "camxsession.h"
#include "camxsettingsmanager.h"
#include "camxvendortags.h"
#include "g_camxsettings.h"
#include "camxchicontext.h"
#include "camxhal3defaultrequest.h"

CAMX_NAMESPACE_BEGIN

// These macros match the definitions found in hardware/camera_common.h and hardware/hardware.h
#define CAMERA_DEVICE_HALAPI_VERSION(major, minor) ((((major) & 0xFF) << 8) | ((minor) & 0xFF))
#if !defined(CAMERA_DEVICE_API_VERSION_3_3)
#define CAMERA_DEVICE_API_VERSION_3_3           CAMERA_DEVICE_HALAPI_VERSION(3, 3)
#endif // CAMERA_DEVICE_API_VERSION_3_3
#if !defined(CAMERA_DEVICE_API_VERSION_3_5)
#define CAMERA_DEVICE_API_VERSION_3_5           CAMERA_DEVICE_HALAPI_VERSION(3, 5)
#endif // CAMERA_DEVICE_API_VERSION_3_3

static const UINT MaxTrackedSessions = 128;
static const UINT MaxTrackedPipelines = 512;
static const UINT StreamDimensionCount = 2;

// We can allocate max of 8*8 = 64 buffers for 240fps HFR case
// but capping to 48 only because frameworks allows max of 64 buffer
// which includes camera HAL buffers and video buffers (of count 9)
static const UINT MaxNumberOfBuffersAllowed = 48;

static const UINT MaxNumberOfOpenFiles      = 2048;

static const UINT32 UBWCMaxSupportedWidth  = 8000;
static const UINT32 UBWCMaxSupportedHeight = 6000;

struct ChiFenceCallbackData
{
    ChiFence*           pChiFence;  ///< Chi fence
    PFNCHIFENCECALLBACK pCallback;  ///< Callback pointer called when fence signaled
    VOID*               pUserData;  ///< User data
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiContext* ChiContext::Create()
{
    ChiContext* pChiContext = CAMX_NEW ChiContext;

    if (NULL != pChiContext)
    {
        CamxResult result = CamxResultSuccess;

        result = pChiContext->Initialize();

        if (CamxResultSuccess != result)
        {
            pChiContext->Destroy();
            pChiContext = NULL;
        }
    }

    return pChiContext;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::Initialize()
{
    CamxResult result                = CamxResultSuccess;
    BOOL       isTOFEnabled          = FALSE;
    UINT32     numThreads            = 0;
    BOOL       enableResourceManager = FALSE;

    m_pHwEnvironment = HwEnvironment::GetInstance();

    if (NULL != m_pHwEnvironment)
    {
        isTOFEnabled          = GetStaticSettings()->enableTOFInterface;
        numThreads            = GetStaticSettings()->numberOfCHIThreads;
        enableResourceManager = GetStaticSettings()->enableResourceManager;

        if (TRUE == isTOFEnabled)
        {
            /// Increase number of threads by 1 since TOF sensor needs a dedicated thread.
            numThreads += 1;
        }

        result = ThreadManager::Create(&m_pThreadManager, "SoloThreadManager", numThreads);
    }
    else
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Out of memory");
    }

    if (CamxResultSuccess == result)
    {
        DeferredRequestQueueCreateData deferredCreateData;
        deferredCreateData.numPipelines      = 0;
        deferredCreateData.pThreadManager    = m_pThreadManager;
        deferredCreateData.requestQueueDepth = DefaultRequestQueueDepth;

        m_pDeferredRequestQueue = DeferredRequestQueue::Create(&deferredCreateData);
        if (NULL == m_pDeferredRequestQueue)
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupHAL, "Out of memory");
        }
    }

    if (CamxResultSuccess == result)
    {
        HwContextCreateData createData = { 0 };

        createData.pHwEnvironment = m_pHwEnvironment;

        result = HwContext::Create(&createData);

        if (CamxResultSuccess == result)
        {
            CAMX_ASSERT(NULL != createData.pHwContext);

            m_pHwContext = createData.pHwContext;
        }
    }

    if (CamxResultSuccess == result)
    {
        /// default max num of open files is 1024, increase it to MaxNumberOfOpenFiles(2048)
        CAMX_LOG_INFO(CamxLogGroupHAL, "Set max open files to %d", MaxNumberOfOpenFiles);
        result = OsUtils::SetFDLimit(MaxNumberOfOpenFiles);
    }

    if ((CamxResultSuccess == result) && (TRUE == enableResourceManager))
    {
        ResourceMgrCreateInfo createInfo    = { 0 };
        createInfo.pResourceMgrName         = "CamXRealtimePipelineResourceManager";
        m_pResourceManager                  = ResourceManager::Create(&createInfo);
    }

    if (CamxResultSuccess == result)
    {
        m_pReleaseChiFence = Mutex::Create("ReleaseChiFence");
        if (NULL == m_pReleaseChiFence)
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL, "failed to create ReleaseChiFence mutex lock");
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::~ChiContext
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiContext::~ChiContext()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiContext::Destroy()
{
    for (UINT32 cameraId = 0; cameraId < MaxNumCameras; cameraId++)
    {
        MetadataPool* pStaticMetadataPool = m_perCameraInfo[cameraId].pStaticMetadataPool;

        if (NULL != pStaticMetadataPool)
        {
            pStaticMetadataPool->Destroy();
            pStaticMetadataPool = NULL;
            m_perCameraInfo[cameraId].pStaticMetadataPool = NULL;
        }
    }

    if (NULL != m_pHwContext)
    {
        m_pHwContext->Destroy();
        m_pHwContext = NULL;
    }

    if (NULL != m_pDeferredRequestQueue)
    {
        m_pDeferredRequestQueue->Destroy();
        m_pDeferredRequestQueue = NULL;
    }

    /// @todo (CAMX-2491) Is there a need to wait for all threads to retire or is it good to assume so
    if (NULL != m_pThreadManager)
    {
        m_pThreadManager->Destroy();
        m_pThreadManager = NULL;
    }

    if (NULL != m_pResourceManager)
    {
        m_pResourceManager->Destroy();
        m_pResourceManager = NULL;
    }

    if (NULL != m_pReleaseChiFence)
    {
        m_pReleaseChiFence->Destroy();
        m_pReleaseChiFence = NULL;
    }

    CAMX_DELETE this;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::GetNumCameras
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiContext::GetNumCameras()
{
    return m_pHwEnvironment->GetNumCameras();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::EnumerateSensorModes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::EnumerateSensorModes(
    UINT32             cameraId,
    UINT32             numSensorModes,
    ChiSensorModeInfo* pSensorModeInfo)
{
    ImageSensorModuleData*       pImageSensorModuleData =
    // NOWHINE CP036a: Since the function is const, had to add the const_cast
        const_cast<ImageSensorModuleData*>(m_pHwEnvironment->GetImageSensorModuleData(cameraId));
    ImageSensorData*             pImageSensorData         = pImageSensorModuleData->GetSensorDataObject();
    const ResolutionInformation* pResolutionInfo          = pImageSensorData->GetResolutionInfo();
    ResolutionData*              pResolutionData          = pResolutionInfo->resolutionData;

    const CSIInformation*        pCSIInformation          = pImageSensorModuleData->GetCSIInfo();

    INT32               deviceIndices[MaxNumImageSensors] = { 0 };
    UINT                actualNumIndices                  = 0;
    CSLSensorCapability sensorCap                         = { 0 };
    CamxResult          result                            = CamxResultSuccess;

    HwEnvironment::GetInstance()->GetDeviceIndices(CSLDeviceTypeImageSensor,
        &deviceIndices[0],
        MaxNumImageSensors,
        &actualNumIndices);

    result = CSLQueryDeviceCapabilities(deviceIndices[cameraId],
                                        &sensorCap,
                                        sizeof(CSLSensorCapability));

    if ((numSensorModes != pResolutionInfo->resolutionDataCount) ||
        (result != CamxResultSuccess)                            ||
        (NULL == pSensorModeInfo)                                ||
        (NULL == pCSIInformation))
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "numSensorModes: %d != resolutionDataCount: %d?, "
                                        "Query Device Cap Result: %s, "
                                        "pSensorModeInfo: %p",
            numSensorModes, pResolutionInfo->resolutionDataCount, CamxResultStrings[result], pSensorModeInfo);

        result = CamxResultEInvalidArg;
    }
    else
    {
        for (UINT i = 0; i < pResolutionInfo->resolutionDataCount; i++)
        {
            PDAFType PDType          = PDAFType::PDTypeUnknown;;
            BOOL     isPDAFSupported = FALSE;

            pSensorModeInfo[i].streamConfigCount = pResolutionData[i].streamInfo.streamConfigurationCount;

            for (UINT streamIndex = 0; streamIndex < pSensorModeInfo[i].streamConfigCount; streamIndex++)
            {
                StreamConfiguration* pStreamConfiguration = &pResolutionData[i].streamInfo.streamConfiguration[streamIndex];

                // sensor mode is selected based on IMAGE type only
                if (StreamType::IMAGE == pStreamConfiguration->type)
                {
                    pSensorModeInfo[i].modeIndex             = i;
                    pSensorModeInfo[i].frameDimension.left   = pStreamConfiguration->frameDimension.xStart;
                    pSensorModeInfo[i].frameDimension.top    = pStreamConfiguration->frameDimension.yStart;
                    pSensorModeInfo[i].frameDimension.width  = pStreamConfiguration->frameDimension.width;
                    pSensorModeInfo[i].frameDimension.height = pStreamConfiguration->frameDimension.height;
                    pSensorModeInfo[i].streamtype            = static_cast<CHISENSORSTREAMTYPE>(pStreamConfiguration->type);
                    pSensorModeInfo[i].frameRate             = static_cast<UINT32>(pResolutionData[i].frameRate);
                    pSensorModeInfo[i].bpp                   = pStreamConfiguration->bitWidth;
                    pSensorModeInfo[i].horizontalBinning     = pResolutionData[i].horizontalBinning;
                    pSensorModeInfo[i].verticalBinning       = pResolutionData[i].verticalBinning;

                    for (UINT j = 0; j < pResolutionData[i].capabilityCount; j++)
                    {
                        pSensorModeInfo[i].sensorModeCaps.value = 0;

                        switch (pResolutionData[i].capability[j])
                        {
                            case SensorCapability::NORMAL:
                                pSensorModeInfo[i].sensorModeCaps.u.Normal   = 1;
                                break;
                            case SensorCapability::HFR:
                                pSensorModeInfo[i].sensorModeCaps.u.HFR      = 1;
                                break;
                            case SensorCapability::IHDR:
                                pSensorModeInfo[i].sensorModeCaps.u.IHDR     = 1;
                                break;
                            case SensorCapability::PDAF:
                                pSensorModeInfo[i].sensorModeCaps.u.PDAF     = 1;
                                break;
                            case SensorCapability::QUADCFA:
                                pSensorModeInfo[i].sensorModeCaps.u.QuadCFA  = 1;
                                break;
                            case SensorCapability::ZZHDR:
                                pSensorModeInfo[i].sensorModeCaps.u.ZZHDR    = 1;
                                break;
                            case SensorCapability::DEPTH:
                                pSensorModeInfo[i].sensorModeCaps.u.DEPTH    = 1;
                                break;
                            case SensorCapability::FS:
                                pSensorModeInfo[i].sensorModeCaps.u.FS       = 1;
                                break;
                            case SensorCapability::INTERNAL:
                                pSensorModeInfo[i].sensorModeCaps.u.Internal = 1;
                                break;
                            default:
                                CAMX_LOG_ERROR(CamxLogGroupHAL, "Unsupported capability");
                                break;
                        }
                    }
                    pSensorModeInfo[i].cropInfo.left    = pResolutionData[i].cropInfo.left;
                    pSensorModeInfo[i].cropInfo.top     = pResolutionData[i].cropInfo.top;
                    pSensorModeInfo[i].cropInfo.width   =
                        pStreamConfiguration->frameDimension.width - pResolutionData[i].cropInfo.right - 1;
                    pSensorModeInfo[i].cropInfo.height  =
                        pStreamConfiguration->frameDimension.height - pResolutionData[i].cropInfo.bottom - 1;

                    if (0 != pResolutionData[i].RemosaicTypeInfoExists)
                    {
                        pSensorModeInfo[i].remosaictype = static_cast<CHIREMOSAICTYPE>(pResolutionData[i].RemosaicTypeInfo);
                    }
                    else
                    {
                        pSensorModeInfo[i].remosaictype = CHIREMOSAICTYPE::UnKnown;
                    }
                }

                pSensorModeInfo[i].streamConfig[streamIndex].vc                   = pStreamConfiguration->vc;
                pSensorModeInfo[i].streamConfig[streamIndex].dt                   = pStreamConfiguration->dt;
                pSensorModeInfo[i].streamConfig[streamIndex].frameDimension.left  = pStreamConfiguration->frameDimension.xStart;
                pSensorModeInfo[i].streamConfig[streamIndex].frameDimension.top   = pStreamConfiguration->frameDimension.yStart;
                pSensorModeInfo[i].streamConfig[streamIndex].frameDimension.width = pStreamConfiguration->frameDimension.width;
                pSensorModeInfo[i].streamConfig[streamIndex].frameDimension.height= pStreamConfiguration->frameDimension.height;
                pSensorModeInfo[i].streamConfig[streamIndex].bpp                  = pStreamConfiguration->bitWidth;
                pSensorModeInfo[i].streamConfig[streamIndex].streamtype           =
                    static_cast<CHISENSORSTREAMTYPE>(pStreamConfiguration->type);
            }

            pImageSensorModuleData->GetPDAFInformation(i, &isPDAFSupported, &PDType);
            pSensorModeInfo[i].sensorModeCaps.u.PDAF = isPDAFSupported;
            switch (PDType)
            {
                case PDAFType::PDTypeUnknown:
                    pSensorModeInfo[i].PDAFType = CHIPDAFTYPE::PDTypeUnknown;
                    break;
                case PDAFType::PDType1:
                    pSensorModeInfo[i].PDAFType = CHIPDAFTYPE::PDType1;
                    break;
                case PDAFType::PDType2:
                    pSensorModeInfo[i].PDAFType = CHIPDAFTYPE::PDType2;
                    break;
                case PDAFType::PDType3:
                    pSensorModeInfo[i].PDAFType = CHIPDAFTYPE::PDType3;
                    break;
                case PDAFType::PDType2PD:
                    pSensorModeInfo[i].PDAFType = CHIPDAFTYPE::PDType2PD;
                    break;
                default:
                    pSensorModeInfo[i].PDAFType = CHIPDAFTYPE::PDTypeUnknown;
                    CAMX_LOG_WARN(CamxLogGroupHAL, "Unsupported PD type: %d", PDType);
                    break;
            }

            pSensorModeInfo[i].CSIPHYId      = sensorCap.CSIPHYSlotId;
            pSensorModeInfo[i].is3Phase      = pResolutionData[i].is3Phase;
            pSensorModeInfo[i].laneCount     = pResolutionData[i].laneCount;
            pSensorModeInfo[i].outPixelClock = pResolutionData[i].outputPixelClock;

            if (pCSIInformation->laneAssignExists)
            {
                pSensorModeInfo[i].laneCfg   = pCSIInformation->laneAssign;
            }
            else
            {
                pSensorModeInfo[i].laneCfg   = 0;
            }

            pImageSensorData->CalculateSensorCrop(&(pSensorModeInfo[i].activeArrayCropWindow), i);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiContext::ProcessCameraOpen
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::ProcessCameraOpen(
    UINT32 cameraId)
{
    CamxResult result = CamxResultSuccess;

    if (cameraId >= GetNumCameras())
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid camera id: %d", cameraId);
        result = CamxResultEInvalidArg;
    }
    else if (TRUE == IsCameraOpened(cameraId))
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Camera id already in use: %d", cameraId);
        result = CamxResultEBusy;
    }
    else if ((m_numCamerasOpened + 1) > MaxConcurrentDevices)
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Too many concurrent devices to open camera id: %d", cameraId);
        result = CamxResultETooManyUsers;
    }
    else
    {
        m_perCameraInfo[cameraId].isCameraOpened = TRUE;
        m_numCamerasOpened++;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiContext::ProcessCameraClose
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::ProcessCameraClose(
    UINT32 cameraId)
{
    CamxResult result = CamxResultSuccess;

    if (GetNumCameras() <= cameraId)
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid camera id: %d", cameraId);
        result = CamxResultEInvalidArg;
    }
    else if (FALSE == IsCameraOpened(cameraId))
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Camera id is closed already: %d", cameraId);
        result = CamxResultEBusy;
    }
    else
    {
        m_perCameraInfo[cameraId].isCameraOpened = FALSE;
        m_numCamerasOpened--;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiContext::OverrideMitigation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::OverrideMitigation(
    UINT32 cameraId)
{
    CamxResult    result     = CamxResultSuccess;
    HwCameraInfo  cameraInfo = {};

    result = m_pHwEnvironment->GetCameraInfo(cameraId, &cameraInfo);
    UINT32 maxPreviewSizeVendorTag;
    UINT32 maxBurstShotFPSVendorTag;
    UINT32 burstShotFPSTag;
    UINT32 count = 0;
    UINT32   isLiveshotSizeSameAsVideoSizeVendorTag = 0;
    UINT32   isFDRenderingInVideoUISupportedVendorTag;

    HwCameraInfo* pCameraInfo = &cameraInfo;
    CamxResult resultTag = CamxResultSuccess;
    MetadataSlot* pStaticMetadataSlot = m_perCameraInfo[cameraId].pStaticMetadataPool->GetSlot(0);

    /// @note This result is needed only for setting metadata
    resultTag = VendorTagManager::QueryVendorTagLocation("org.quic.camera.MaxPreviewSize",
                                                         "MaxPreviewSize",
                                                         &maxPreviewSizeVendorTag);

    INT32  maxPreviewSizeDimension[2] = { 0 };

    maxPreviewSizeDimension[0] = pCameraInfo->pPlatformCaps->maxPreviewDimension.width;
    maxPreviewSizeDimension[1] = pCameraInfo->pPlatformCaps->maxPreviewDimension.height;
    /// @note This can be ignored because all standard tag set properly
    if (CamxResultSuccess == resultTag)
    {
        resultTag = pStaticMetadataSlot->SetMetadataByTag(maxPreviewSizeVendorTag,
                                                          static_cast<VOID*>(&maxPreviewSizeDimension),
                                                          2,
                                                          "camxContext");
    }

    BOOL   isBurstShotSupported = TRUE;
    resultTag = VendorTagManager::QueryVendorTagLocation(
        "org.quic.camera.BurstFPS", "isBurstShotSupported", &burstShotFPSTag);
    if (CamxResultSuccess == resultTag)
    {
        /// @note This can be ignored because all standard tag set properly
        resultTag = pStaticMetadataSlot->SetMetadataByTag(burstShotFPSTag,
                                                          static_cast<VOID*>(&isBurstShotSupported),
                                                          1,
                                                          "camxContext");
    }

    /// @note This result is needed only for setting metadata
    resultTag = VendorTagManager::QueryVendorTagLocation("org.quic.camera.BurstFPS",
                                                         "MaxBurstShotFPS",
                                                         &maxBurstShotFPSVendorTag);

    FLOAT  maxBurstshotFPS = 0.0;
    if (TRUE == isBurstShotSupported)
    {
        INT32  maxWidth        = 1;
        INT32  maxHeight       = 1;

        // Get sensor maxwidth and maxHeight
        if ((TRUE == pCameraInfo->pSensorCaps->isQuadCFASensor) && (FALSE ==  GetStaticSettings()->exposeFullSizeForQCFA))
        {
            maxWidth  = pCameraInfo->pSensorCaps->QuadCFADim.width  >> 1;
            maxHeight = pCameraInfo->pSensorCaps->QuadCFADim.height >> 1;
        }
        else
        {
            for (UINT configIndex = 0; configIndex < pCameraInfo->pSensorCaps->numSensorConfigs; configIndex++)
            {
                for (UINT i = 0; i < pCameraInfo->pSensorCaps->sensorConfigs[configIndex].numStreamConfig; i++)
                {
                    if (StreamType::IMAGE ==
                                 pCameraInfo->pSensorCaps->sensorConfigs[configIndex].streamConfigs[i].streamType)
                    {
                        INT32 sensorWidth  =
                            pCameraInfo->pSensorCaps->sensorConfigs[configIndex].streamConfigs[i].dimension.width;
                        INT32 sensorHeight =
                            pCameraInfo->pSensorCaps->sensorConfigs[configIndex].streamConfigs[i].dimension.height;

                        if ((sensorWidth * sensorHeight) > (maxWidth * maxHeight))
                        {
                            maxWidth  = sensorWidth;
                            maxHeight = sensorHeight;
                        }
                        break;
                    }
                }
            }
        }
        // update the max burstFPS based on current sensor resolution
        maxBurstshotFPS = static_cast<FLOAT>(pCameraInfo->pPlatformCaps->maxSnapshotBw) /
                          static_cast<FLOAT>((maxWidth * maxHeight));
        // Cap the burstFPS to 30 max
        maxBurstshotFPS  = (maxBurstshotFPS > 30) ? 30 : maxBurstshotFPS;
    }

    /// @note This can be ignored because all standard tag set properly
    if (CamxResultSuccess == resultTag)
    {
        resultTag = pStaticMetadataSlot->SetMetadataByTag(maxBurstShotFPSVendorTag,
                                                          static_cast<VOID*>(&maxBurstshotFPS),
                                                          1,
                                                          "camxContext");
    }

    BOOL isLiveshotSizeSameAsVideoSize;
    if (FALSE == GetStaticSettings()->overrideForceFullSizeLiveshot)
    {
        isLiveshotSizeSameAsVideoSize = pCameraInfo->pPlatformCaps->isLiveshotSizeSameAsVideoSize;
    }
    else
    {
        isLiveshotSizeSameAsVideoSize = TRUE;
    }

    resultTag = VendorTagManager::QueryVendorTagLocation(
                 "org.quic.camera.LiveshotSize", "isLiveshotSizeSameAsVideoSize", &isLiveshotSizeSameAsVideoSizeVendorTag);

    if (CamxResultSuccess == resultTag)
    {
        resultTag = pStaticMetadataSlot->SetMetadataByTag(isLiveshotSizeSameAsVideoSizeVendorTag,
                                                          static_cast<VOID*>(&isLiveshotSizeSameAsVideoSize),
                                                          1,
                                                          "camxContext");
    }

    BOOL isFDRenderingInVideoUISupported;
    if (FALSE == GetStaticSettings()->overrideForceFDRendering)
    {
        isFDRenderingInVideoUISupported = pCameraInfo->pPlatformCaps->isFDRenderingInVideoUISupported;
    }
    else
    {
        isFDRenderingInVideoUISupported = TRUE;
    }
    resultTag = VendorTagManager::QueryVendorTagLocation(
        "org.quic.camera.FDRendering", "isFDRenderingInVideoUISupported", &isFDRenderingInVideoUISupportedVendorTag);
    if (CamxResultSuccess == resultTag)
    {
        /// @note This can be ignored because all standard tag set properly
        resultTag = pStaticMetadataSlot->SetMetadataByTag(isFDRenderingInVideoUISupportedVendorTag,
                                                          static_cast<VOID*>(&isFDRenderingInVideoUISupported),
                                                          1,
                                                          "camxContext");
    }

    UINT32                 videoMitigationsTableTag;
    UINT32                 videoMitigationsTableSize = pCameraInfo->pHwEnvironmentCaps->numVideoMitigations;
    VideoMitigationsParams videoMitigationsTable[videoMitigationsTableSize];

    count = pCameraInfo->pHwEnvironmentCaps->numVideoMitigations *
            (sizeof(VideoMitigationsParams) / sizeof(INT32));
    for (UINT i = 0; i < videoMitigationsTableSize; i++)
    {
        videoMitigationsTable[i] = pCameraInfo->pHwEnvironmentCaps->videoMitigationsTable[i];
    }
    resultTag = VendorTagManager::QueryVendorTagLocation(
            "org.quic.camera2.VideoConfigurations.info", "VideoConfigurationsTable",
                                                                     &videoMitigationsTableTag);
    if (CamxResultSuccess == resultTag)
    {
        /// @note This can be ignored because all standard tag set properly
        resultTag = pStaticMetadataSlot->SetMetadataByTag(videoMitigationsTableTag,
                                                          static_cast<VOID*>(videoMitigationsTable),
                                                          count,
                                                          "camxContext");
    }

    UINT32 videoMitigationsTableSizeTag;

    resultTag = VendorTagManager::QueryVendorTagLocation(
        "org.quic.camera2.VideoConfigurations.info", "VideoConfigurationsTableSize",
                                                                 &videoMitigationsTableSizeTag);
    if (CamxResultSuccess == resultTag)
    {
        /// @note This can be ignored because all standard tag set properly
        resultTag = pStaticMetadataSlot->SetMetadataByTag(videoMitigationsTableSizeTag,
                                                          static_cast<VOID*>(&videoMitigationsTableSize),
                                                          1,
                                                          "camxContext");
    }

    return resultTag;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::InitializeStaticMetadataPool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::InitializeStaticMetadataPool(
    UINT32  cameraId)
{
    CamxResult    result     = CamxResultSuccess;
    HwCameraInfo  cameraInfo = {};

    CAMX_ASSERT(cameraId <  GetNumCameras());
    CAMX_ASSERT(NULL     != m_pHwEnvironment);

    result = m_pHwEnvironment->GetCameraInfo(cameraId, &cameraInfo);

    // Populate static metadata from platform capabilities.
    if (CamxResultSuccess == result)
    {
        HwCameraInfo* pCameraInfo = &cameraInfo;

        // Static Pool has only one slot ever
        MetadataSlot* pStaticMetadataSlot = m_perCameraInfo[cameraId].pStaticMetadataPool->GetSlot(0);

        pStaticMetadataSlot->SetSlotRequestId(1);

        CAMX_ASSERT(NULL != pStaticMetadataSlot);

        /// @todo (CAMX-961) Add check to see if valid static metadata is already filled.

        // Fill in all supported characteristics keys
        UINT numCharacteristicsKeys = cameraInfo.pPlatformCaps->numCharacteristicsKeys;

        // NOWHINE GR004 {} Suspress the switch case indent problem with curly brackets
        for (UINT key = 0; key < numCharacteristicsKeys; key++)
        {
            UINT count = 0;
            /// @todo (CAMX-79) - Populate static capabilities from sensor module.
            /// @todo (CAMX-961)- Fill in the remaining metadata.
            // NOWHINE CP036a {} Google API requires const type
            switch (pCameraInfo->pPlatformCaps->characteristicsKeys[key])
            {
                // Color correction
                case ColorCorrectionAvailableAberrationModes:
                {
                    UINT8 availableAbberationsModes[ColorCorrectionAberrationModeEnd] = { 0 };
                    count = pCameraInfo->pPlatformCaps->numAbberationsModes;

                    for (UINT i = 0; i < count; i++)
                    {
                        availableAbberationsModes[i] = static_cast<UINT8>(pCameraInfo->pPlatformCaps->abberationModes[i]);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(ColorCorrectionAvailableAberrationModes,
                                                                   static_cast<VOID*>(availableAbberationsModes),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                // Control
                case ControlAEAvailableAntibandingModes:
                {
                    UINT8 availableAntibandingModes[ControlAEAntibandingModeEnd] = { 0 };
                    count = pCameraInfo->pPlatformCaps->numAntibandingModes;

                    for (UINT i = 0; i < count; i++)
                    {
                        availableAntibandingModes[i] = static_cast<UINT8>(pCameraInfo->pPlatformCaps->antibandingModes[i]);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(ControlAEAvailableAntibandingModes,
                                                                   static_cast<VOID*>(availableAntibandingModes),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case ControlAEAvailableModes:
                {
                    UINT8 availableAEModes[ControlAEModeEnd] = { 0 };
                    count = pCameraInfo->pPlatformCaps->numAEModes;

                    UINT capsCount = 0;

                    for (UINT i = 0; i < count; i++)
                    {
                        if ((ControlAEModeOnAutoFlash       == pCameraInfo->pPlatformCaps->AEModes[i]) ||
                            (ControlAEModeOnAlwaysFlash     == pCameraInfo->pPlatformCaps->AEModes[i]) ||
                            (ControlAEModeOnAutoFlashRedeye == pCameraInfo->pPlatformCaps->AEModes[i]))
                        {
                            if (TRUE == pCameraInfo->pSensorCaps->hasFlash)
                            {
                                availableAEModes[capsCount++] = static_cast<UINT8>(pCameraInfo->pPlatformCaps->AEModes[i]);
                            }
                        }
                        else
                        {
                            availableAEModes[capsCount++] = static_cast<UINT8>(pCameraInfo->pPlatformCaps->AEModes[i]);
                        }
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(ControlAEAvailableModes,
                                                                   static_cast<VOID*>(availableAEModes),
                                                                   capsCount,
                                                                   "camxContext");
                    break;

                }

                case ControlAEAvailableTargetFPSRanges:
                {
                    count = pCameraInfo->pHwEnvironmentCaps->numAETargetFPSRanges * (sizeof(RangeINT32) / sizeof(INT32));

                    result = pStaticMetadataSlot->SetMetadataByTag(ControlAEAvailableTargetFPSRanges,
                                                                   static_cast<VOID*>(const_cast<RangeINT32*>(
                                                                   pCameraInfo->pHwEnvironmentCaps->AETargetFPSRanges)),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case ControlAECompensationRange:
                {
                    // Update.AE compensation range. min and max values
                    INT32 availableAECompensationRange[] =
                    {
                        pCameraInfo->pPlatformCaps->minAECompensationValue,
                        pCameraInfo->pPlatformCaps->maxAECompensationValue
                    };
                    count = sizeof(availableAECompensationRange) / sizeof(availableAECompensationRange[0]);

                    result = pStaticMetadataSlot->SetMetadataByTag(ControlAECompensationRange,
                                                                   static_cast<VOID*>(availableAECompensationRange),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case ControlAECompensationStep:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(ControlAECompensationStep,
                                                                   static_cast<VOID*>(const_cast<Rational*>
                                                                   (&(pCameraInfo->pPlatformCaps->AECompensationSteps))),
                                                                   1,
                                                                   "camxContext");
                    break;

                }

                case ControlAFAvailableModes:
                {
                    UINT8 availableAFModes[ControlAFModeEnd] = { 0 };
                    count = pCameraInfo->pPlatformCaps->numAFModes;

                    if (FALSE == pCameraInfo->pSensorCaps->isFixedFocus)
                    {
                        for (UINT i = 0; i < count; i++)
                        {
                            availableAFModes[i] = static_cast<UINT8>(pCameraInfo->pPlatformCaps->AFModes[i]);
                        }
                    }
                    else
                    {
                        count = 1;
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(ControlAFAvailableModes,
                                                                   static_cast<VOID*>(availableAFModes),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case ControlAvailableEffects:
                {
                    UINT8 availableEffectModes[ControlEffectModeEnd] = { 0 };
                    count = pCameraInfo->pPlatformCaps->numEffectModes;

                    for (UINT i = 0; i < count; i++)
                    {
                        availableEffectModes[i] = static_cast<UINT8>(pCameraInfo->pPlatformCaps->effectModes[i]);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(ControlAvailableEffects,
                                                                   static_cast<VOID*>(availableEffectModes),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case ControlAvailableSceneModes:
                {
                    UINT8 availableSceneModes[ControlSceneModeEnd] = { 0 };
                    count = pCameraInfo->pPlatformCaps->numSceneModes;

                    for (UINT i = 0; i < count; i++)
                    {
                        availableSceneModes[i] = static_cast<UINT8>(pCameraInfo->pPlatformCaps->sceneModes[i]);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(ControlAvailableSceneModes,
                                                                   static_cast<VOID*>(availableSceneModes),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case ControlAvailableVideoStabilizationModes:
                {
                    UINT8 availableVideoStabilizationModes[ControlVideoStabilizationModeEnd] = { 0 };
                    count = 0;

                    for (UINT i = 0; i < pCameraInfo->pPlatformCaps->numVideoStabilizationModes; i++)
                    {
                        if ((ImageSensorFacingExternal       != pCameraInfo->imageSensorFacing) ||
                            (ControlVideoStabilizationModeOn != pCameraInfo->pPlatformCaps->videoStabilizationsModes[i]))
                        {
                            availableVideoStabilizationModes[i] = static_cast<UINT8>(
                                pCameraInfo->pPlatformCaps->videoStabilizationsModes[i]);
                            count++;
                        }
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(ControlAvailableVideoStabilizationModes,
                                                                   static_cast<VOID*>(availableVideoStabilizationModes),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case ControlAWBAvailableModes:
                {
                    UINT8 availableAWBModes[ControlAWBModeEnd] = { 0 };
                    count = pCameraInfo->pPlatformCaps->numAWBModes;

                    for (UINT i = 0; i < count; i++)
                    {
                        availableAWBModes[i] = static_cast<UINT8>(pCameraInfo->pPlatformCaps->AWBModes[i]);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(ControlAWBAvailableModes,
                                                                   static_cast<VOID*>(availableAWBModes),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case ControlMaxRegions:
                {
                    // Update.Available Max Regions for AE, AWB and AF respectively.
                    INT32 availableMaxRegions[] =
                    {
                        pCameraInfo->pPlatformCaps->maxRegionsAE,
                        pCameraInfo->pPlatformCaps->maxRegionsAWB,
                        pCameraInfo->pPlatformCaps->maxRegionsAF,
                    };

                    if (TRUE == pCameraInfo->pSensorCaps->isFixedFocus)
                    {
                        availableMaxRegions[2] = 0;
                    }

                    count = sizeof(availableMaxRegions) / sizeof(availableMaxRegions[0]);

                    result = pStaticMetadataSlot->SetMetadataByTag(ControlMaxRegions,
                                                                   static_cast<VOID*>(availableMaxRegions),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case ControlSceneModeOverrides:
                {
                    // Update Available Scene Mode overrides for AE, AWB and AF.
                    UINT8 availableSceneModeOverrides[ControlSceneModeEnd][3];
                    count = pCameraInfo->pPlatformCaps->numSceneModes * 3;

                    for (UINT i = 0; i < pCameraInfo->pPlatformCaps->numSceneModes; i++)
                    {
                        availableSceneModeOverrides[i][0] =
                            static_cast<UINT8>(pCameraInfo->pPlatformCaps->sceneModeOverride[i].AEModeOverride);
                        availableSceneModeOverrides[i][1] =
                            static_cast<UINT8>(pCameraInfo->pPlatformCaps->sceneModeOverride[i].AWBModeOverride);
                        availableSceneModeOverrides[i][2] =
                            static_cast<UINT8>(pCameraInfo->pPlatformCaps->sceneModeOverride[i].AFModeOverride);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(ControlSceneModeOverrides,
                                                                   static_cast<VOID*>(availableSceneModeOverrides),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case ControlAvailableHighSpeedVideoConfigurations:
                {
                    if (pCameraInfo->pHwEnvironmentCaps->numHFRRanges > 0)
                    {
                        count = pCameraInfo->pHwEnvironmentCaps->numHFRRanges *
                                    (sizeof(HFRConfigurationParams) / sizeof(INT32));

                        result = pStaticMetadataSlot->SetMetadataByTag(ControlAvailableHighSpeedVideoConfigurations,
                                                                       static_cast<VOID*>(const_cast
                                                                       <HFRConfigurationParams*>(
                                                                       pCameraInfo->pHwEnvironmentCaps->HFRVideoSizes)),
                                                                       count,
                                                                       "camxContext");
                    }
                    break;
                }

                case ControlAELockAvailable:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(ControlAELockAvailable,
                                                                   static_cast<VOID*>(const_cast<BOOL*>(
                                                                   &(pCameraInfo->pPlatformCaps->lockAEAvailable))),
                                                                   1,
                                                                   "camxContext");
                    break;

                }

                case ControlAWBLockAvailable:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(ControlAWBLockAvailable,
                                                                   static_cast<VOID*>(const_cast<BOOL*>(
                                                                   &(pCameraInfo->pPlatformCaps->lockAWBAvailable))),
                                                                   1,
                                                                  "camxContext");
                    break;

                }

                case ControlAvailableModes:
                {
                    UINT8 availableModes[ControlModeEnd] = { 0 };
                    count = pCameraInfo->pPlatformCaps->numAvailableModes;

                    for (UINT i = 0; i < count; i++)
                    {
                        availableModes[i] = static_cast<UINT8>(pCameraInfo->pPlatformCaps->availableModes[i]);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(ControlAvailableModes,
                                                                   static_cast<VOID*>(availableModes),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case ControlPostRawSensitivityBoostRange:
                {
                    RangeINT32 sensitivityRange;

                    sensitivityRange.min = pCameraInfo->pPlatformCaps->minPostRawSensitivityBoost;
                    sensitivityRange.max = pCameraInfo->pPlatformCaps->maxPostRawSensitivityBoost;

                    result = pStaticMetadataSlot->SetMetadataByTag(ControlPostRawSensitivityBoostRange,
                                                                   static_cast<VOID*>(&sensitivityRange),
                                                                   sizeof(RangeINT32) / sizeof(INT32),
                                                                   "camxContext");
                    break;

                }

                // Edge
                case EdgeAvailableEdgeModes:
                {
                    UINT8 availableEdgeModes[EdgeModeEnd] = { 0 };
                    count = pCameraInfo->pPlatformCaps->numEdgeModes;

                    /// @todo (CAMX-1015)- check efficient way to eliminate array copy.
                    /// do the same thing for other enum array in this file.
                    for (UINT i = 0; i < count; i++)
                    {
                        availableEdgeModes[i] = static_cast<UINT8>(pCameraInfo->pPlatformCaps->edgeModes[i]);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(EdgeAvailableEdgeModes,
                                                                   static_cast<VOID*>(availableEdgeModes),
                                                                   count,
                                                                   "camxContext");
                    break;
                }

                // Flash
                case FlashInfoAvailable:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(FlashInfoAvailable,
                                                                   static_cast<VOID*>(const_cast<BOOL*>(
                                                                   &(pCameraInfo->pSensorCaps->hasFlash))),
                                                                   1,
                                                                   "camxContext");
                    break;
                }
                case FlashInfoChargeDuration:
                    CAMX_LOG_ERROR(CamxLogGroupHAL, "No static metadata value for FlashInfoChargeDuration");
                    break;

                case FlashColorTemperature:
                    CAMX_LOG_ERROR(CamxLogGroupHAL, "No static metadata value for FlashColorTemperature");
                    break;

                case FlashMaxEnergy:
                    CAMX_LOG_ERROR(CamxLogGroupHAL, "No static metadata value for FlashMaxEnergy");
                    break;

                // Hot pixel
                case HotPixelAvailableHotPixelModes:
                {
                    UINT8 availableHotPixelModes[HotPixelModeEnd] = { 0 };
                    count = pCameraInfo->pPlatformCaps->numHotPixelModes;

                    for (UINT i = 0; i < count; i++)
                    {
                        availableHotPixelModes[i] = static_cast<UINT8>(pCameraInfo->pPlatformCaps->hotPixelModes[i]);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(HotPixelAvailableHotPixelModes,
                                                                   static_cast<VOID*>(availableHotPixelModes),
                                                                   count,
                                                                   "camxContext");

                    break;
                }

                // JPEG
                case JPEGAvailableThumbnailSizes:
                {
                    count = pCameraInfo->pPlatformCaps->numJPEGThumbnailSizes * (sizeof(DimensionCap) / sizeof(INT32));

                    result = pStaticMetadataSlot->SetMetadataByTag(JPEGAvailableThumbnailSizes,
                                                                   static_cast<VOID*>(const_cast<DimensionCap*>(
                                                                   pCameraInfo->pPlatformCaps->JPEGThumbnailSizes)),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case JPEGMaxSize:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(JPEGMaxSize,
                                                                   static_cast<VOID*>(const_cast<INT32*>(
                                                                   &(pCameraInfo->pHwEnvironmentCaps->JPEGMaxSizeInBytes))),
                                                                   1,
                                                                   "camxContext");
                    break;

                }

                // Lens
                case LensInfoAvailableApertures:
                {
                    count   = pCameraInfo->pSensorCaps->numAperatures;
                    CAMX_ASSERT(count <= MaxTagValues);

                    result = pStaticMetadataSlot->SetMetadataByTag(LensInfoAvailableApertures,
                                                                   static_cast<VOID*>(const_cast<FLOAT*>(
                                                                   &(pCameraInfo->pSensorCaps->aperatures[0]))),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case LensInfoAvailableFilterDensities:
                {
                    count   = pCameraInfo->pSensorCaps->numNDFs;
                    CAMX_ASSERT(count <= MaxTagValues);

                    result = pStaticMetadataSlot->SetMetadataByTag(LensInfoAvailableFilterDensities,
                                                                   static_cast<VOID*>(const_cast<FLOAT*>(
                                                                   &(pCameraInfo->pSensorCaps->NDFs[0]))),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case LensInfoAvailableFocalLengths:
                {
                    count   = pCameraInfo->pSensorCaps->numFocalLengths;
                    if (count > 0)
                    {
                        CAMX_ASSERT(count <= MaxTagValues);

                        result = pStaticMetadataSlot->SetMetadataByTag(LensInfoAvailableFocalLengths,
                                                                       static_cast<VOID*>(const_cast<FLOAT*>(
                                                                       &(pCameraInfo->pSensorCaps->focalLengths[0]))),
                                                                       count,
                                                                       "camxContext");
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupHAL, "No static metadata value for LensInfoAvailableFocalLengths");
                    }

                    break;
                }

                case LensInfoAvailableOpticalStabilization:
                {
                    UINT8 availableOisModes[LensOpticalStabilizationModeEnd] =
                    {
                        LensOpticalStabilizationModeOff,
                        LensOpticalStabilizationModeOn
                    };

                    // HAL must support LensOpticalStabilizationModeOff, and check if lens support optical stabilization
                    count = (TRUE == pCameraInfo->pSensorCaps->hasOIS) ? 2 : 1;
                    CAMX_ASSERT(count <= LensOpticalStabilizationModeEnd);

                    result = pStaticMetadataSlot->SetMetadataByTag(LensInfoAvailableOpticalStabilization,
                                                                   static_cast<VOID*>(availableOisModes),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case LensInfoHyperfocalDistance:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(LensInfoHyperfocalDistance,
                                                                   static_cast<VOID*>(const_cast<FLOAT*>(
                                                                   &(pCameraInfo->pSensorCaps->hyperfocalDistance))),
                                                                   1,
                                                                   "camxContext");
                    break;
                }

                case LensInfoMinimumFocusDistance:
                {
                    FLOAT   minimumFocusDistance = 0.0;
                    if (FALSE == pCameraInfo->pSensorCaps->isFixedFocus)
                    {
                        minimumFocusDistance = pCameraInfo->pSensorCaps->minimumFocusDistance;
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(LensInfoMinimumFocusDistance,
                                                                   static_cast<VOID*>(const_cast<FLOAT*>(
                                                                   &(minimumFocusDistance))),
                                                                   1,
                                                                   "camxContext");
                    break;
                }

                case LensInfoShadingMapSize:
                {
                    count = sizeof(DimensionCap) / sizeof(INT32);

                    result = pStaticMetadataSlot->SetMetadataByTag(LensInfoShadingMapSize,
                                                                   static_cast<VOID*>(const_cast<DimensionCap*>(
                                                                   &(pCameraInfo->pSensorCaps->lensShadingMapSize))),
                                                                   count,
                                                                   "camxContext");
                    break;
                }

                case LensInfoFocusDistanceCalibration:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(LensInfoFocusDistanceCalibration,
                                                                   static_cast<VOID*>
                                                                   (const_cast<LensInfoFocusDistanceCalibrationValues*>
                                                                   (&(pCameraInfo->pSensorCaps->focusDistanceCalibration))),
                                                                   1,
                                                                   "camxContext");
                    break;
                }
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
                case LogicalMultiCameraPhysicalIDs:
                {
                    CAMX_LOG_INFO(CamxLogGroupHAL, "Static Metadata value for Logical Multi Camera Physical Ids ");
                    break;
                }
#endif // Android-P or better
                case LensFacing:
                {
                    LensFacingValues lensFacing = LensFacingInvalid;

                    // NOWHINE CF007 {} Ignore warning as whiner expect 8 spaces indent between switch and case below
                    switch (pCameraInfo->imageSensorFacing)
                    {
                        case ImageSensorFacingBack:
                            lensFacing = LensFacingBack;
                            break;
                        case ImageSensorFacingFront:
                            lensFacing = LensFacingFront;
                            break;
                        case ImageSensorFacingExternal:
                            lensFacing = LensFacingExternal;
                            break;
                        default:
                            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Lens facing info: %d", pCameraInfo->imageSensorFacing);
                            break;
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(LensFacing,
                                                                   static_cast<VOID*>(&lensFacing),
                                                                   1,
                                                                   "camxContext");
                    break;

                }

                case LensPoseRotation:
                {
                    FLOAT availableLensPoseRotation[LensPoseRotationSize] = { 0 };
                    count = pCameraInfo->pSensorCaps->lensPoseRotationCount;

                    for (UINT i = 0; i < count; i++)
                    {
                        availableLensPoseRotation[i] =
                            static_cast<FLOAT>(pCameraInfo->pSensorCaps->lensPoseRotationDC[i]);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(LensPoseRotation,
                                                                   static_cast<VOID*>(availableLensPoseRotation),
                                                                   count,
                                                                   "camxContext");
                    break;
                }

                case LensPoseTranslation:
                {
                    FLOAT availableLensPoseTranslation[LensPoseTranslationSize] = { 0 };
                    count = pCameraInfo->pSensorCaps->lensPoseTranslationCount;

                    for (UINT i = 0; i < count; i++)
                    {
                        availableLensPoseTranslation[i] =
                            static_cast<FLOAT>(pCameraInfo->pSensorCaps->lensPoseTranslationDC[i]);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(LensPoseTranslation,
                                                                   static_cast<VOID*>(availableLensPoseTranslation),
                                                                   count,
                                                                   "camxContext");
                    break;
                }

                case LensIntrinsicCalibration:
                {
                    FLOAT availableLensIntrinsicCalibration[LensIntrinsicCalibrationSize] = { 0 };
                    count = pCameraInfo->pSensorCaps->lensIntrinsicCalibrationCount;

                    for (UINT i = 0; i < count; i++)
                    {
                        availableLensIntrinsicCalibration[i] =
                            static_cast<FLOAT>(pCameraInfo->pSensorCaps->lensIntrinsicCalibrationDC[i]);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(LensIntrinsicCalibration,
                                                                   static_cast<VOID*>(availableLensIntrinsicCalibration),
                                                                   count,
                                                                   "camxContext");
                    break;
                }

#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
                case LensPoseReference:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(LensPoseReference,
                                                                   static_cast<VOID*>(const_cast<UINT8*>(
                                                                   &(pCameraInfo->pSensorCaps->lensPoseReferenceDC))),
                                                                   1,
                                                                   "camxContext");
                    break;
                }

                case LensDistortion:
                {
                    FLOAT availableLensDistortion[LensDistortionSize] = { 0 };
                    count = pCameraInfo->pSensorCaps->lensDistortionCount;

                    for (UINT i = 0; i < count; i++)
                    {
                        availableLensDistortion[i] =
                            static_cast<FLOAT>(pCameraInfo->pSensorCaps->lensDistortionDC[i]);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(LensDistortion,
                                                                   static_cast<VOID*>(availableLensDistortion),
                                                                   count,
                                                                   "camxContext");
                    break;
                }
#endif // Android-P or better
                case LensRadialDistortion:
                    CAMX_LOG_ERROR(CamxLogGroupHAL, "No static metadata value for LensRadialDistortion");
                    break;

                // Noise reduction
                case NoiseReductionAvailableNoiseReductionModes:
                {
                    UINT8 availableNoiseReductionMode[NoiseReductionEnd] = { 0 };
                    count = pCameraInfo->pPlatformCaps->numNoiseReductionModes;

                    for (UINT i = 0; i < count; i++)
                    {
                        availableNoiseReductionMode[i] = static_cast<UINT8>(pCameraInfo->pPlatformCaps->noiseReductionModes[i]);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(NoiseReductionAvailableNoiseReductionModes,
                                                                   static_cast<VOID*>(availableNoiseReductionMode),
                                                                   count,
                                                                   "camxContext");
                    break;
                }

                // Request
                case RequestMaxNumOutputStreams:
                {
                    // Update Available Max output streams for Raw formats,
                    // non stalling Processed streams and stalling Processed streams respectively.
                    INT32 availableMaxNumOutputStreams[] =
                    {
                        pCameraInfo->pPlatformCaps->maxRawStreams,
                        pCameraInfo->pPlatformCaps->maxProcessedStreams,
                        pCameraInfo->pPlatformCaps->maxProcessedStallingStreams,
                    };

                    count = sizeof(availableMaxNumOutputStreams) / sizeof(availableMaxNumOutputStreams[0]);

                    result = pStaticMetadataSlot->SetMetadataByTag(RequestMaxNumOutputStreams,
                                                                   static_cast<VOID*>(availableMaxNumOutputStreams),
                                                                   count,
                                                                   "camxContext");
                    break;
                }

                case RequestMaxNumReprocessStreams:
                    CAMX_LOG_ERROR(CamxLogGroupHAL, "No static metadata value for RequestMaxNumReprocessStreams");
                    break;

                case RequestMaxNumInputStreams:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(RequestMaxNumInputStreams,
                                                                   static_cast<VOID*>(const_cast<INT32*>(
                                                                   &(pCameraInfo->pPlatformCaps->maxInputStreams))),
                                                                   1,
                                                                   "camxContext");
                    break;
                }

                case RequestPipelineMaxDepth:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(RequestPipelineMaxDepth,
                                                                   static_cast<VOID*>(const_cast<UINT8*>(
                                                                   &(pCameraInfo->pPlatformCaps->maxPipelineDepth))),
                                                                   1,
                                                                   "camxContext");
                    break;
                }

                case RequestPartialResultCount:
                {
                    const StaticSettings* pStaticSettings = GetStaticSettings();
                    UINT32 numPartialResult =
                        Utils::MinUINT32(pStaticSettings->numMetadataResults, pCameraInfo->pPlatformCaps->partialResultCount);

                    /// @todo (CAMX-4119) Have CHI do modification of settings
                    /* We will increment the Partial result count by 1 if CHI also has its own implementation */
                    if (CHIPartialDataSeparate == pStaticSettings->enableCHIPartialData)
                    {
                        CAMX_LOG_INFO(CamxLogGroupHAL, "CHI has enabled Partial Data Handling");
                        numPartialResult++;
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(RequestPartialResultCount,
                                                                   static_cast<VOID*>(const_cast<UINT32*>(
                                                                   &(numPartialResult))),
                                                                   1,
                                                                   "camxContext");
                    break;
                }

                case RequestAvailableCapabilities:
                {
                    UINT8 availableRequestCaps[RequestAvailableCapabilitiesEnd] = { 0 };
                    const StaticSettings* pStaticSettings = GetStaticSettings();
                    UINT numRequestCaps = pCameraInfo->pPlatformCaps->numRequestCaps;
                    UINT capsCount = 0;
                    BOOL isHighSpeedVideoSupported = FALSE;

                    for (UINT configIndex = 0; configIndex < pCameraInfo->pSensorCaps->numSensorConfigs; configIndex++)
                    {
                        if ((static_cast<UINT>(pCameraInfo->pSensorCaps->sensorConfigs[configIndex].maxFPS) > 30) &&
                            (pCameraInfo->pHwEnvironmentCaps->numHFRRanges > 0))
                        {
                            isHighSpeedVideoSupported = TRUE;
                            break;
                        }
                    }

                    for (UINT i = 0; i < numRequestCaps; i++)
                    {
                        UINT8 requestCap = static_cast<UINT8>(pCameraInfo->pPlatformCaps->requestCaps[i]);

                        if ((FALSE == pStaticSettings->enableRAWProcessing) &&
                            (requestCap == RequestAvailableCapabilitiesRaw))
                        {
                            continue;
                        }

                        if (!pCameraInfo->pSensorCaps->isDepthSensor &&
                            (requestCap == RequestAvailableCapabilitiesDepthOutput))
                        {
                            continue;
                        }

                        if ((FALSE == isHighSpeedVideoSupported) &&
                            (RequestAvailableCapabilitiesConstrainedHighSpeedVideo == requestCap))
                        {
                            // skip HFR supported if max fps is <=30
                            continue;
                        }

                        availableRequestCaps[capsCount++] = requestCap;
                    }
                    result = pStaticMetadataSlot->SetMetadataByTag(RequestAvailableCapabilities,
                                                                   static_cast<VOID*>(availableRequestCaps),
                                                                   capsCount,
                                                                   "camxContext");
                    break;
                }

                case RequestAvailableRequestKeys:
                {
                    INT32 availableRequestKeys[MaxMetadataTagCount] = { 0 };
                    count = pCameraInfo->pPlatformCaps->numRequestKeys;

                    UINT capsCount = 0;

                    for (UINT i = 0; i < count; i++)
                    {
                        if ((TRUE == pCameraInfo->pSensorCaps->isFixedFocus) &&
                            (ControlAFRegions == pCameraInfo->pPlatformCaps->requestKeys[i]))
                        {
                            // skip adding AF regions if camera is a fixed-focus
                            continue;
                        }
                        else
                        {
                            availableRequestKeys[capsCount++] =
                                static_cast<INT32>(pCameraInfo->pPlatformCaps->requestKeys[i]);
                        }
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(RequestAvailableRequestKeys,
                                                                   static_cast<VOID*>(availableRequestKeys),
                                                                   capsCount,
                                                                   "camxContext");
                    break;
                }

                case RequestAvailableResultKeys:
                {
                    INT32 availableResultKeys[MaxMetadataTagCount] = { 0 };
                    count = pCameraInfo->pPlatformCaps->numResultKeys;

                    UINT capsCount = 0;
                    for (UINT i = 0; i < count; i++)
                    {

                        availableResultKeys[capsCount++] = static_cast<INT32>(pCameraInfo->pPlatformCaps->resultKeys[i]);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(RequestAvailableResultKeys,
                                                                   static_cast<VOID*>(availableResultKeys),
                                                                   capsCount,
                                                                   "camxContext");
                    break;
                }

                case RequestAvailableCharacteristicsKeys:
                {
                    INT32 availableCharacteristicsKeys[MaxMetadataTagCount] = { 0 };
                    count = pCameraInfo->pPlatformCaps->numCharacteristicsKeys;

                    for (UINT i = 0; i < count; i++)
                    {
                        availableCharacteristicsKeys[i] = static_cast<INT32>(pCameraInfo->pPlatformCaps->
                                                                                characteristicsKeys[i]);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(RequestAvailableCharacteristicsKeys,
                                                                   static_cast<VOID*>(availableCharacteristicsKeys),
                                                                   count,
                                                                   "camxContext");
                    break;

                }
#if ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
                case RequestAvailableSessionKeys:
                {
                    INT32 availableSessionKeys[MaxMetadataTagCount] = { 0 };
                    count = pCameraInfo->pPlatformCaps->numSessionKeys;

                    for (UINT i = 0; i < count; i++)
                    {
                        availableSessionKeys[i] = static_cast<INT32>(pCameraInfo->pPlatformCaps->
                                                                                sessionKeys[i]);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(RequestAvailableSessionKeys,
                                                                   static_cast<VOID*>(availableSessionKeys),
                                                                   count,
                                                                   "camxContext");
                    break;

                }
#endif // ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28))
                // Scaler
                case ScalerAvailableJPEGSizes:
                {
                    /// @todo (CAMX-961)- Check if this tag should be deprecated.
                    DimensionCap  JPEGSizes[MaxResolutions];
                    count = pCameraInfo->pPlatformCaps->numDefaultImageSizes * (sizeof(DimensionCap) / sizeof(INT32));

                    for (UINT i = 0; i < pCameraInfo->pPlatformCaps->numDefaultImageSizes; i++)
                    {
                        JPEGSizes[i].width  = pCameraInfo->pPlatformCaps->defaultImageSizes[i].width;
                        JPEGSizes[i].height = pCameraInfo->pPlatformCaps->defaultImageSizes[i].height;
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(ScalerAvailableJPEGSizes,
                                                                   static_cast<VOID*>(JPEGSizes),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case ScalerAvailableFormats:
                {
                    /// @todo (CAMX-961)- Check if this tag should be deprecated.
                    INT32 availablescalerFormats[MaxScalerFormats] = { 0 };
                    count = pCameraInfo->pPlatformCaps->numScalerFormats;

                    for (UINT i = 0; i < count; i++)
                    {
                        availablescalerFormats[i] = static_cast<INT32>(pCameraInfo->pPlatformCaps->scalerFormats[i]);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(ScalerAvailableFormats,
                                                                   static_cast<VOID*>(availablescalerFormats),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case ScalerAvailableMaxDigitalZoom:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(ScalerAvailableMaxDigitalZoom,
                                                                   static_cast<VOID*>(const_cast<FLOAT*>(
                                                                   &(pCameraInfo->pPlatformCaps->maxDigitalZoom))),
                                                                   1,
                                                                   "camxContext");
                    break;


                }

                case ScalerAvailableInputOutputFormatsMap:
                {
                    count = pCameraInfo->pPlatformCaps->numInputOutputFormatMaps;

                    result = pStaticMetadataSlot->SetMetadataByTag(ScalerAvailableInputOutputFormatsMap,
                                                                   static_cast<VOID*>(const_cast<INT32*>(
                                                                   (pCameraInfo->pPlatformCaps->inputOutputFormatMap))),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case ScalerAvailableRawSizes:
                {
                    INT32 availableRawSizes[MaxSensorStreamConfigurations * MaxResolutions * StreamDimensionCount] = { 0 };
                    UINT i;
                    UINT j = 0;

                    for (count = 0; count < pCameraInfo->pSensorCaps->numSensorConfigs; count++)
                    {
                        for (i = 0; i < pCameraInfo->pSensorCaps->sensorConfigs[count].numStreamConfig; i++)
                           {
                               availableRawSizes[j] =
                                    pCameraInfo->pSensorCaps->sensorConfigs[count].streamConfigs[i].dimension.width;
                               availableRawSizes[j+1] =
                                    pCameraInfo->pSensorCaps->sensorConfigs[count].streamConfigs[i].dimension.height;
                               j += 2;
                            }
                     }

                     result = pStaticMetadataSlot->SetMetadataByTag(ScalerAvailableRawSizes,
                                    static_cast<VOID*>(availableRawSizes),
                                    j,
                                    "camxContext");
                     break;
                }

                case ScalerAvailableStreamConfigurations:
                {
                    if (!pCameraInfo->pSensorCaps->isDepthSensor)
                    {
                        count = pCameraInfo->pHwEnvironmentCaps->numStreamConfigs *
                                (sizeof(ScalerStreamConfig) / sizeof(INT32));

                        result = pStaticMetadataSlot->SetMetadataByTag(ScalerAvailableStreamConfigurations,
                                                                       static_cast<VOID*>(const_cast<
                                                                       ScalerStreamConfig*>(pCameraInfo->
                                                                       pHwEnvironmentCaps->streamConfigs)),
                                                                       count,
                                                                       "camxContext");
                    }
                    break;

                }

                case SensorOpaqueRawSize:
                {
                    count  = pCameraInfo->pHwEnvironmentCaps->opaqueRawSizesCount *
                             (sizeof(SensorRawOpaqueConfig) / sizeof(INT32));

                    if (0 < count)
                    {
                        result = pStaticMetadataSlot->SetMetadataByTag(SensorOpaqueRawSize,
                            static_cast<VOID*>(const_cast<SensorRawOpaqueConfig*>(pCameraInfo->pHwEnvironmentCaps->
                            opaqueRawSizes)),
                            count,
                            "camxContext");
                    }
                }
                break;

                case ScalerAvailableMinFrameDurations:
                {
                    count = pCameraInfo->pHwEnvironmentCaps->numMinFrameDurations *
                            (sizeof(ScalerFrameDurationINT64) / sizeof(INT64));

                    result = pStaticMetadataSlot->SetMetadataByTag(ScalerAvailableMinFrameDurations,
                                                                   static_cast<VOID*>(const_cast<
                                                                   ScalerFrameDurationINT64*>(pCameraInfo->
                                                                   pHwEnvironmentCaps->minFrameDurations)),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case ScalerAvailableStallDurations:
                {
                    count = pCameraInfo->pHwEnvironmentCaps->numStallDurations *
                            (sizeof(ScalerStallDurationINT64) / sizeof(INT64));

                    result = pStaticMetadataSlot->SetMetadataByTag(ScalerAvailableStallDurations,
                                                                   static_cast<VOID*>(const_cast<
                                                                   ScalerStallDurationINT64*>(pCameraInfo->
                                                                   pHwEnvironmentCaps->minStallDurations)),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case ScalerCroppingType:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(ScalerCroppingType,
                                                                   static_cast<VOID*>
                                                                   (const_cast<ScalerCroppingTypeValues*>
                                                                   (&(pCameraInfo->pPlatformCaps->croppingSupport))),
                                                                   1,
                                                                   "camxContext");
                    break;
                }

                // Sensor
                case SensorInfoActiveArraySize:
                {
                    Region                activeArraySize = pCameraInfo->pSensorCaps->activeArraySize;
                    const StaticSettings* pStaticSettings = GetStaticSettings();

                    if ((TRUE  == pCameraInfo->pSensorCaps->isQuadCFASensor) &&
                        (FALSE == pStaticSettings->exposeFullSizeForQCFA))
                    {
                            activeArraySize = pCameraInfo->pSensorCaps->QuadCFAActiveArraySize;
                            CAMX_LOG_INFO(CamxLogGroupHAL, "activeArraySize for quadcfa sensor: %dx%d",
                                          activeArraySize.width, activeArraySize.height);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(SensorInfoActiveArraySize,
                                                                   static_cast<VOID*>(&activeArraySize),
                                                                   sizeof(Region) / sizeof(INT32),
                                                                   "camxContext");
                    break;
                }

                case SensorInfoSensitivityRange:
                {
                    RangeINT32 sensitivityRange;

                    sensitivityRange.min = pCameraInfo->pSensorCaps->minISOSensitivity;
                    sensitivityRange.max = pCameraInfo->pSensorCaps->maxISOSensitivity;

                    result = pStaticMetadataSlot->SetMetadataByTag(SensorInfoSensitivityRange,
                                                                   static_cast<VOID*>(&sensitivityRange),
                                                                   sizeof(RangeINT32) / sizeof(INT32),
                                                                   "camxContext");
                    break;

                }

                case SensorInfoColorFilterArrangement:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(SensorInfoColorFilterArrangement,
                                                                   static_cast<VOID*>(
                                                                   const_cast<SensorInfoColorFilterArrangementValues*>(
                                                                   &(pCameraInfo->pSensorCaps->colorFilterArrangement))),
                                                                   1,
                                                                   "camxContext");
                    break;
                }

                case SensorInfoExposureTimeRange:
                {
                    if ((InfoSupportedHardwareLevelFull == (pCameraInfo->pPlatformCaps->supportedHwLevel)) ||
                        (InfoSupportedHardwareLevel3 == (pCameraInfo->pPlatformCaps->supportedHwLevel)))
                    {
                        // For FULL capability devices (android.info.supportedHardwareLevel == FULL),
                        // the maximum of the range SHOULD be at least 1 second (1e9), MUST be at least 100ms.
                        CAMX_ASSERT(pCameraInfo->pSensorCaps->maxExposureTime >= 100000000);
                    }

                    RangeINT64 timeRange;

                    timeRange.min = pCameraInfo->pSensorCaps->minExposureTime;
                    timeRange.max = pCameraInfo->pSensorCaps->maxExposureTime;

                    result = pStaticMetadataSlot->SetMetadataByTag(SensorInfoExposureTimeRange,
                                                                   static_cast<VOID*>(&timeRange),
                                                                   sizeof(RangeINT64) / sizeof(INT64),
                                                                   "camxContext");
                    break;

                }

                case SensorInfoMaxFrameDuration:
                {
                    if ((InfoSupportedHardwareLevelFull == (pCameraInfo->pPlatformCaps->supportedHwLevel)) ||
                        (InfoSupportedHardwareLevel3 == (pCameraInfo->pPlatformCaps->supportedHwLevel)))
                    {
                        // For FULL capability devices(android.info.supportedHardwareLevel == FULL),
                        // the maximum of the range SHOULD be at least 1 second(1e9), MUST be at least 100ms(100e6).
                        CAMX_ASSERT(pCameraInfo->pSensorCaps->maxFrameDuration >= 100000000);
                    }

                    // android.sensor.info.maxFrameDuration must be greater or equal to the
                    // android.sensor.info.exposureTimeRange max value (since exposure time overrides frame duration).
                    CAMX_ASSERT(pCameraInfo->pSensorCaps->maxFrameDuration >= pCameraInfo->pSensorCaps->maxExposureTime);

                    result = pStaticMetadataSlot->SetMetadataByTag(SensorInfoMaxFrameDuration,
                                                                   static_cast<VOID*>(const_cast<UINT64*>(
                                                                   &(pCameraInfo->pSensorCaps->maxFrameDuration))),
                                                                   1,
                                                                   "camxContext");
                    break;

                }

                case SensorInfoPhysicalSize:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(SensorInfoPhysicalSize,
                                                                   static_cast<VOID*>(const_cast<DimensionCapFloat*>(
                                                                   &(pCameraInfo->pSensorCaps->physicalSensorSize))),
                                                                   sizeof(DimensionCapFloat) / sizeof(FLOAT),
                                                                   "camxContext");
                    break;

                }

                case SensorInfoPixelArraySize:
                {
                    DimensionCap          pixelArraySize  = pCameraInfo->pSensorCaps->pixelArraySize;
                    const StaticSettings* pStaticSettings = GetStaticSettings();

                    if ((TRUE  == pCameraInfo->pSensorCaps->isQuadCFASensor) &&
                        (FALSE == pStaticSettings->exposeFullSizeForQCFA))
                    {
                            pixelArraySize = pCameraInfo->pSensorCaps->QuadCFAPixelArraySize;
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(SensorInfoPixelArraySize,
                                                                   static_cast<VOID*>(&pixelArraySize),
                                                                   sizeof(DimensionCap) / sizeof(INT32),
                                                                   "camxContext");
                    break;
                }

                case SensorInfoWhiteLevel:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(SensorInfoWhiteLevel,
                                                                   static_cast<VOID*>(const_cast<INT32*>(
                                                                   &(pCameraInfo->pSensorCaps->whiteLevel))),
                                                                   1,
                                                                   "camxContext");
                    break;

                }

                case SensorInfoTimestampSource:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(SensorInfoTimestampSource,
                                                                   static_cast<VOID*>(
                                                                   const_cast<SensorInfoTimestampSourceValues*>(
                                                                   &(pCameraInfo->pPlatformCaps->timestampSource))),
                                                                   1,
                                                                   "camxContext");
                    break;

                }

                case SensorInfoLensShadingApplied:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(SensorInfoLensShadingApplied,
                                                                   static_cast<VOID*>(const_cast<BOOL*>(
                                                                   &(pCameraInfo->pSensorCaps->lensShadingAppliedInSensor))),
                                                                   1,
                                                                   "camxContext");
                    break;

                }

                case SensorInfoPreCorrectionActiveArraySize:
                {
                    CAMX_ASSERT(pCameraInfo->pSensorCaps->preCorrectionActiveArraySize.xMin >= 0);
                    CAMX_ASSERT(pCameraInfo->pSensorCaps->preCorrectionActiveArraySize.yMin >= 0);
                    CAMX_ASSERT(pCameraInfo->pSensorCaps->preCorrectionActiveArraySize.width <=
                                pCameraInfo->pSensorCaps->pixelArraySize.width);
                    CAMX_ASSERT(pCameraInfo->pSensorCaps->preCorrectionActiveArraySize.height <=
                                pCameraInfo->pSensorCaps->pixelArraySize.height);

                    Region                preCorrectionArraySize = pCameraInfo->pSensorCaps->preCorrectionActiveArraySize;
                    const StaticSettings* pStaticSettings        = GetStaticSettings();

                    if ((TRUE  == pCameraInfo->pSensorCaps->isQuadCFASensor) &&
                        (FALSE == pStaticSettings->exposeFullSizeForQCFA))
                    {
                        preCorrectionArraySize = pCameraInfo->pSensorCaps->QuadCFAPreCorrectionActiveArraySize;
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(SensorInfoPreCorrectionActiveArraySize,
                                                                   static_cast<VOID*>(const_cast<Region*>(
                                                                   &(preCorrectionArraySize))),
                                                                   sizeof(Region) / sizeof(INT32),
                                                                   "camxContext");
                    break;
                }

                case SensorReferenceIlluminant1:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(SensorReferenceIlluminant1,
                                                                   static_cast<VOID*>(
                                                                   const_cast<SensorReferenceIlluminantValues*>(
                                                                   &(pCameraInfo->pSensorCaps->referenceIlluminant1))),
                                                                   1,
                                                                   "camxContext");
                    break;

                }

                case SensorReferenceIlluminant2:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(SensorReferenceIlluminant2,
                                                                   static_cast<VOID*>(
                                                                   const_cast<SensorReferenceIlluminantValues*>(
                                                                   &(pCameraInfo->pSensorCaps->referenceIlluminant2))),
                                                                   1,
                                                                   "camxContext");
                    break;

                }

                case SensorCalibrationTransform1:
                {
                    count = sizeof(pCameraInfo->pSensorCaps->calibrationTransform1) / sizeof(Rational);

                    result = pStaticMetadataSlot->SetMetadataByTag(SensorCalibrationTransform1,
                                                                   static_cast<VOID*>(const_cast<Rational*>(
                                                                   &(pCameraInfo->pSensorCaps->
                                                                   calibrationTransform1[0][0]))),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case SensorCalibrationTransform2:
                {
                    count = sizeof(pCameraInfo->pSensorCaps->calibrationTransform2) / sizeof(Rational);

                    result = pStaticMetadataSlot->SetMetadataByTag(SensorCalibrationTransform2,
                                                                   static_cast<VOID*>(const_cast<Rational*>(
                                                                   &(pCameraInfo->pSensorCaps->
                                                                   calibrationTransform2[0][0]))),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case SensorColorTransform1:
                {
                    count = sizeof(pCameraInfo->pSensorCaps->colorTransform1) / sizeof(Rational);

                    result = pStaticMetadataSlot->SetMetadataByTag(SensorColorTransform1,
                                                                   static_cast<VOID*>(const_cast<Rational*>(
                                                                   &(pCameraInfo->pSensorCaps->colorTransform1[0][0]))),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case SensorColorTransform2:
                {
                    count = sizeof(pCameraInfo->pSensorCaps->colorTransform2) / sizeof(Rational);

                    result = pStaticMetadataSlot->SetMetadataByTag(SensorColorTransform2,
                                                                   static_cast<VOID*>(const_cast<Rational*>(
                                                                   &(pCameraInfo->pSensorCaps->colorTransform2[0][0]))),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case SensorForwardMatrix1:
                {
                    count = sizeof(pCameraInfo->pSensorCaps->forwardMatrix1) / sizeof(Rational);

                    result = pStaticMetadataSlot->SetMetadataByTag(SensorForwardMatrix1,
                                                                   static_cast<VOID*>(const_cast<Rational*>(
                                                                   &(pCameraInfo->pSensorCaps->forwardMatrix1[0][0]))),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case SensorForwardMatrix2:
                {
                    count = sizeof(pCameraInfo->pSensorCaps->forwardMatrix2) / sizeof(Rational);

                    result = pStaticMetadataSlot->SetMetadataByTag(SensorForwardMatrix2,
                                                                   static_cast<VOID*>(const_cast<Rational*>(
                                                                   &(pCameraInfo->pSensorCaps->forwardMatrix2[0][0]))),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case SensorBaseGainFactor:
                    CAMX_LOG_ERROR(CamxLogGroupHAL, "No static metadata value for SensorBaseGainFactor");
                    break;

                case SensorBlackLevelPattern:
                {
                    count = sizeof(pCameraInfo->pSensorCaps->blackLevelPattern) / sizeof(INT32);

                    result = pStaticMetadataSlot->SetMetadataByTag(SensorBlackLevelPattern,
                                                                   static_cast<VOID*>(const_cast<INT32*>(
                                                                   &(pCameraInfo->pSensorCaps->blackLevelPattern[0]))),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case SensorMaxAnalogSensitivity:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(SensorMaxAnalogSensitivity,
                                                                   static_cast<VOID*>(const_cast<INT32*>(
                                                                   &(pCameraInfo->pSensorCaps->maxAnalogSensitivity))),
                                                                   1,
                                                                   "camxContext");
                    break;

                }

                case SensorOrientation:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(SensorOrientation,
                                                                   static_cast<VOID*>(&(pCameraInfo->imageOrientation)),
                                                                   1,
                                                                   "camxContext");
                    break;

                }

                case SensorProfileHueSaturationMapDimensions:
                {
                    count = sizeof(ProfileHueSaturationMapDimensions) / sizeof(INT32);

                    result = pStaticMetadataSlot->SetMetadataByTag(SensorProfileHueSaturationMapDimensions,
                                                                   static_cast<VOID*>(
                                                                   const_cast<ProfileHueSaturationMapDimensions*>(
                                                                   &(pCameraInfo->pSensorCaps->
                                                                   profileHueSaturationMapDimensions))),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                case SensorAvailableTestPatternModes:
                {
                    count = pCameraInfo->pSensorCaps->numTestPatterns;

                    result = pStaticMetadataSlot->SetMetadataByTag(SensorAvailableTestPatternModes,
                                                                   static_cast<VOID*>(const_cast<INT32*>(
                                                                   &(pCameraInfo->pSensorCaps->testPatterns[0]))),
                                                                   count,
                                                                   "camxContext");
                    break;

                }

                // Shading
                case ShadingAvailableModes:
                {
                    UINT8 availableShadingModes[HotPixelModeEnd] = { 0 };
                    count = pCameraInfo->pPlatformCaps->numShadingModes;

                    for (UINT i = 0; i < count; i++)
                    {
                        availableShadingModes[i] = static_cast<UINT8>(pCameraInfo->pPlatformCaps->shadingModes[i]);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(ShadingAvailableModes,
                                                                   static_cast<VOID*>(availableShadingModes),
                                                                   count,
                                                                   "camxContext");

                    break;
                }

                // Statistics
                case StatisticsInfoAvailableFaceDetectModes:
                {
                    UINT8 availableFaceDetectionModes[StatisticsFaceDetectModeEnd] = { 0 };
                    count = pCameraInfo->pPlatformCaps->numFaceDetectModes;

                    for (UINT i = 0; i < count; i++)
                    {
                        availableFaceDetectionModes[i] = static_cast<UINT8>(pCameraInfo->pPlatformCaps->faceDetectModes[i]);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(StatisticsInfoAvailableFaceDetectModes,
                                                                   static_cast<VOID*>(availableFaceDetectionModes),
                                                                   count,
                                                                   "camxContext");

                    break;
                }

                case StatisticsInfoHistogramBucketCount:
                    CAMX_LOG_ERROR(CamxLogGroupHAL, "No static metadata value for StatisticsInfoHistogramBucketCount");
                    break;

                case StatisticsInfoMaxFaceCount:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(StatisticsInfoMaxFaceCount,
                                                                   static_cast<VOID*>(const_cast<INT32*>(
                                                                   &(pCameraInfo->pPlatformCaps->maxFaceCount))),
                                                                   1,
                                                                   "camxContext");

                    break;
                }

                case StatisticsInfoMaxHistogramCount:
                    CAMX_LOG_ERROR(CamxLogGroupHAL, "No static metadata value for StatisticsInfoMaxHistogramCount");
                    break;

                case StatisticsInfoMaxSharpnessMapValue:
                    CAMX_LOG_ERROR(CamxLogGroupHAL, "No static metadata value for StatisticsInfoMaxSharpnessMapValue");
                    break;

                case StatisticsInfoSharpnessMapSize:
                    CAMX_LOG_ERROR(CamxLogGroupHAL, "No static metadata value for StatisticsInfoSharpnessMapSize");
                    break;

                case StatisticsInfoAvailableHotPixelMapModes:
                {
                    UINT8 availableHotPixelMapModes[StatisticsHotPixelMapModeEnd] =
                    {
                        StatisticsHotPixelMapModeOff,
                        StatisticsHotPixelMapModeOn
                    };

                    // HAL must support StatisticsHotPixelMapModeOff, and check if lens support hot pixel map
                    count = (TRUE == pCameraInfo->pSensorCaps->hotPixelMapAvailable) ? 2 : 1;
                    CAMX_ASSERT(count <= StatisticsHotPixelMapModeEnd);

                    result = pStaticMetadataSlot->SetMetadataByTag(StatisticsInfoAvailableHotPixelMapModes,
                                                                   static_cast<VOID*>(availableHotPixelMapModes),
                                                                   count,
                                                                   "camxContext");
                    break;
                }

                case StatisticsInfoAvailableLensShadingMapModes:
                {
                    UINT8 availableLensShadingMapModes[StatisticsLensShadingMapModeEnd] =
                    {
                        StatisticsLensShadingMapModeOff,
                        StatisticsLensShadingMapModeOn
                    };

                    // HAL must support StatisticsLensShadingMapModeOff, and check if lens support shading
                    count = (TRUE == pCameraInfo->pSensorCaps->lensShadingAppliedInSensor) ? 2 : 1;
                    CAMX_ASSERT(count <= StatisticsLensShadingMapModeEnd);

                    result = pStaticMetadataSlot->SetMetadataByTag(StatisticsInfoAvailableLensShadingMapModes,
                                                                   static_cast<VOID*>(availableLensShadingMapModes),
                                                                   count,
                                                                   "camxContext");
                    break;
                }

                // Tonemap
                case TonemapMaxCurvePoints:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(TonemapMaxCurvePoints,
                                                                   static_cast<VOID*>(const_cast<UINT*>(
                                                                   &(pCameraInfo->pPlatformCaps->maxTonemapCurvePoints))),
                                                                   1,
                                                                   "camxContext");

                    break;
                }

                case TonemapAvailableToneMapModes:
                {
                    UINT8 availableToneMapModes[TonemapModeEnd] = { 0 };
                    count = pCameraInfo->pPlatformCaps->numTonemapModes;

                    for (UINT i = 0; i < count; i++)
                    {
                        availableToneMapModes[i] = static_cast<UINT8>(pCameraInfo->pPlatformCaps->tonemapModes[i]);
                    }

                    result = pStaticMetadataSlot->SetMetadataByTag(TonemapAvailableToneMapModes,
                                                                   static_cast<VOID*>(availableToneMapModes),
                                                                   count,
                                                                   "camxContext");

                    break;
                }

                // LED
                case LedAvailableLeds:
                    CAMX_LOG_ERROR(CamxLogGroupHAL, "No static metadata value for LedAvailableLeds");
                    break;

                // Info
                case InfoSupportedHardwareLevel:
                {
                    UINT8 pSupportedHwLevel = static_cast<UINT8>(pCameraInfo->pPlatformCaps->supportedHwLevel);

                    result = pStaticMetadataSlot->SetMetadataByTag(InfoSupportedHardwareLevel,
                                                                   static_cast<VOID*>(&pSupportedHwLevel),
                                                                   1,
                                                                   "camxContext");
                    break;

                }

                // Sync
                case SyncMaxLatency:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(SyncMaxLatency,
                                                                   static_cast<VOID*>
                                                                   (const_cast<SyncMaxLatencyValues*>
                                                                   (&(pCameraInfo->pPlatformCaps->syncMaxLatency))),
                                                                   1,
                                                                   "camxContext");
                    break;

                }

                // Reprocess
                case ReprocessMaxCaptureStall:
                {
                    result = pStaticMetadataSlot->SetMetadataByTag(ReprocessMaxCaptureStall,
                                                                   static_cast<VOID*>(const_cast<INT32*>(
                                                                   &(pCameraInfo->pPlatformCaps->maxCaptureStall))),
                                                                   1,
                                                                   "camxContext");
                    break;
                }

                // Depth
                case DepthMaxDepthSamples:
                    CAMX_LOG_ERROR(CamxLogGroupHAL, "No static metadata value for DepthMaxDepthSamples");
                    break;

                case DepthAvailableDepthStreamConfigurations:
                {
                    if (pCameraInfo->pSensorCaps->isDepthSensor)
                    {
                        count = pCameraInfo->pHwEnvironmentCaps->numStreamConfigs *
                            (sizeof(ScalerStreamConfig) / sizeof(INT32));

                        result = pStaticMetadataSlot->SetMetadataByTag(DepthAvailableDepthStreamConfigurations,
                                                                       static_cast<VOID*>(const_cast<
                                                                       ScalerStreamConfig*>(pCameraInfo->
                                                                       pHwEnvironmentCaps->streamConfigs)),
                                                                       count,
                                                                       "camxContext");
                    }
                    break;
                }

                case DepthAvailableDepthMinFrameDurations:
                {
                    if (pCameraInfo->pSensorCaps->isDepthSensor)
                    {
                        count = pCameraInfo->pHwEnvironmentCaps->numMinFrameDurations *
                            (sizeof(ScalerFrameDurationINT64) / sizeof(INT64));

                        result = pStaticMetadataSlot->SetMetadataByTag(DepthAvailableDepthMinFrameDurations,
                                                                       static_cast<VOID*>(const_cast<
                                                                       ScalerFrameDurationINT64*>(pCameraInfo->
                                                                       pHwEnvironmentCaps->minFrameDurations)),
                                                                       count,
                                                                       "camxContext");
                    }
                    break;
                }

                case DepthAvailableDepthStallDurations:
                {
                    if (pCameraInfo->pSensorCaps->isDepthSensor)
                    {
                        count = pCameraInfo->pHwEnvironmentCaps->numStallDurations *
                            (sizeof(ScalerStallDurationINT64) / sizeof(INT64));

                        result = pStaticMetadataSlot->SetMetadataByTag(DepthAvailableDepthStallDurations,
                                                                       static_cast<VOID*>(const_cast<
                                                                       ScalerStallDurationINT64*>(pCameraInfo->
                                                                       pHwEnvironmentCaps->minStallDurations)),
                                                                       count,
                                                                       "camxContext");
                    }

                    break;
                }

                case DepthDepthIsExclusive:
                    CAMX_LOG_ERROR(CamxLogGroupHAL, "No static metadata value for DepthDepthIsExclusive");
                    break;
#if (CAMERA_MODULE_API_VERSION_CURRENT > CAMERA_MODULE_API_VERSION_2_4) // Check Camera Module Version
                case HEICInfoSupported:
                {
                    if (TRUE == GetStaticSettings()->enableNativeHEIF)
                    {
                        result = pStaticMetadataSlot->SetMetadataByTag(HEICInfoSupported,
                                                           static_cast<VOID*>(const_cast<UINT8*>(
                                                           &(pCameraInfo->pPlatformCaps->heicInfoSupported))),
                                                           1,
                                                           "camxContext");
                    }
                    break;

                }
                case HEICInfoMaxJpegAppSegmentsCount:
                {
                    if (TRUE == GetStaticSettings()->enableNativeHEIF)
                    {
                        result = pStaticMetadataSlot->SetMetadataByTag(HEICInfoMaxJpegAppSegmentsCount,
                                                           static_cast<VOID*>(const_cast<UINT8*>(
                                                           &(pCameraInfo->pPlatformCaps->heicInfoMaxJpegAppSegmentsCount))),
                                                           1,
                                                           "camxContext");
                    }
                    break;
                }
#endif // Check Camera Module Version
                default:
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupHAL, "Invalid static metadata key: %x",
                                     pCameraInfo->pPlatformCaps->characteristicsKeys[key]);
                    break;
                }
            }

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupHAL, "Initialize StaticMetadata key: %x failed with error %s",
                                pCameraInfo->pPlatformCaps->characteristicsKeys[key],
                                Utils::CamxResultToString(result));
                break;
            }
        }

        UINT32      exposureMeteringVendorTag;
        UINT32      saturationVendorTag;
        UINT32      isoVendorTag;
        UINT32      EEPROMInfoTag;
        CamxResult  resultTag                   = CamxResultSuccess;
        UINT        count                       = 0;
        UINT32      sharpnessVendorTag;
        UINT32      histogramBucketsVendorTag;
        UINT32      histogramCountVendorTag;
        UINT32      instantAecVendorTag;
        UINT32      ltmContrastVendorTag;
        UINT32      ltmDarkBoostStrengthVendorTag;
        UINT32      ltmBrightSupressStrengthTag;

        // Saturation Range
        INT32 availableSaturationRange[4] = { 0 };
        availableSaturationRange[0]       = pCameraInfo->pPlatformCaps->saturationRange.minValue;
        availableSaturationRange[1]       = pCameraInfo->pPlatformCaps->saturationRange.maxValue;
        availableSaturationRange[2]       = pCameraInfo->pPlatformCaps->saturationRange.defaultValue;
        availableSaturationRange[3]       = pCameraInfo->pPlatformCaps->saturationRange.step;
        /// @note This result is needed only for setting metadata
        resultTag = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.saturation", "range",
                                                             &saturationVendorTag);
        if (CamxResultSuccess == resultTag)
        {
            /// @note This can be ignored because all standard tag set properly
            resultTag = pStaticMetadataSlot->SetMetadataByTag(saturationVendorTag,
                                                              static_cast<VOID*>(availableSaturationRange),
                                                              4,
                                                              "camxContext");
        }

        // Exposure Metering Modes
        INT32   availableExposureMetering[ExposureMeteringEnd]  = { 0 };
        count = pCameraInfo->pPlatformCaps->numExposureMeteringModes;
        CAMX_ASSERT(count < ExposureMeteringEnd);
        for (UINT i = 0; i < count; i++)
        {
            availableExposureMetering[i] = static_cast<INT32>(pCameraInfo->pPlatformCaps->exposureMeteringModes[i]);
        }
        /// @note This result is needed only for setting metadata
        resultTag = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.exposure_metering", "available_modes",
                                                             &exposureMeteringVendorTag);
        if (CamxResultSuccess == resultTag)
        {
            /// @note This can be ignored because all standard tag set properly
            resultTag = pStaticMetadataSlot->SetMetadataByTag(exposureMeteringVendorTag,
                                                              static_cast<VOID*>(availableExposureMetering),
                                                              count,
                                                              "camxContext");
        }

        // ISO Modes
        INT32   availableISOModes[ISOModeEnd] = { 0 };
        count = pCameraInfo->pPlatformCaps->numISOAvailableModes;
        for (UINT i = 0; i < count; i++)
        {
            availableISOModes[i] = static_cast<INT32>(pCameraInfo->pPlatformCaps->ISOAvailableModes[i]);
        }
        /// @note This result is needed only for setting metadata
        resultTag = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.iso_exp_priority",
                                                             "iso_available_modes",
                                                             &isoVendorTag);
        if (CamxResultSuccess == resultTag)
        {
            /// @note This can be ignored because all standard tag set properly
            resultTag = pStaticMetadataSlot->SetMetadataByTag(isoVendorTag,
                                                              static_cast<VOID*>(availableISOModes),
                                                              count,
                                                              "camxContext");
        }
        // Exposure Time
        INT64   availExposureTimeRange[2] = { 0 };
        UINT32  exposureTimeVendorTag;
        availExposureTimeRange[0] = pCameraInfo->pSensorCaps->minExposureTime;
        availExposureTimeRange[1] = pCameraInfo->pSensorCaps->maxExposureTime;

        /// @note This result is needed only for setting metadata
        resultTag = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.iso_exp_priority",
                                                             "exposure_time_range",
                                                             &exposureTimeVendorTag);
        if (CamxResultSuccess == resultTag)
        {
            /// @note This can be ignored because all standard tag set properly
            resultTag = pStaticMetadataSlot->SetMetadataByTag(exposureTimeVendorTag,
                                                              static_cast<VOID*>(availExposureTimeRange),
                                                              2,
                                                              "camxContext");
        }

        EEPROMInformation   EEPROMInfo = { { 0 } };

        resultTag = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.sensor_meta_data",
                                                             "EEPROMInformation",
                                                             &EEPROMInfoTag);

        Utils::Memcpy(&EEPROMInfo, &pCameraInfo->pSensorCaps->OTPData.EEPROMInfo, sizeof(EEPROMInformation));

        if (CamxResultSuccess == resultTag)
        {
            resultTag = pStaticMetadataSlot->SetMetadataByTag(EEPROMInfoTag,
                                                              static_cast<VOID*>(&EEPROMInfo), sizeof(EEPROMInformation),
                                                              "camxContext");
        }

        // Sharpness Range
        INT32 availableSharpnessRange[2] = { 0 };
        availableSharpnessRange[0]       = pCameraInfo->pPlatformCaps->sharpnessRange.minValue;
        availableSharpnessRange[1]       = pCameraInfo->pPlatformCaps->sharpnessRange.maxValue;
        /// @note This result is needed only for setting metadata
        resultTag = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.sharpness", "range",
                                                             &sharpnessVendorTag);
        if (CamxResultSuccess == resultTag)
        {
            /// @note This can be ignored because all standard tag set properly
            resultTag = pStaticMetadataSlot->SetMetadataByTag(sharpnessVendorTag,
                                                              static_cast<VOID*>(availableSharpnessRange),
                                                              2,
                                                              "camxContext");
        }
        // Histogram Buckets
        INT32 histBuckets = pCameraInfo->pPlatformCaps->histogramBuckets;
        /// @note This result is needed only for setting metadata
        resultTag = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.histogram", "buckets",
                                                             &histogramBucketsVendorTag);
        if (CamxResultSuccess == resultTag)
        {
            /// @note This can be ignored because all standard tag set properly
            resultTag = pStaticMetadataSlot->SetMetadataByTag(histogramBucketsVendorTag,
                                                              static_cast<VOID*>(&histBuckets),
                                                              1,
                                                              "camxContext");
            /// @note This failure means the app has failed to call get_vendor_tag_ops (HAL or CHI)
        }
        // Histogram Counts
        INT32 histCount = pCameraInfo->pPlatformCaps->histogramCount;
        /// @note This result is needed only for setting metadata
        resultTag = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.histogram", "max_count",
                                                             &histogramCountVendorTag);
        if (CamxResultSuccess == resultTag)
        {
            /// @note This can be ignored because all standard tag set properly
            resultTag = pStaticMetadataSlot->SetMetadataByTag(histogramCountVendorTag,
                                                              static_cast<VOID*>(&histCount),
                                                              1,
                                                              "camxContext");
        }

        // Instant AEC Modes
        INT32   availableInstantAec[InstantAecEnd]  = { 0 };
        count = pCameraInfo->pPlatformCaps->numInstantAecModes;
        CAMX_ASSERT(InstantAecEnd >= count);
        for (UINT i = 0; i < count; i++)
        {
            availableInstantAec[i] = static_cast<INT32>(pCameraInfo->pPlatformCaps->instantAecAvailableModes[i]);
        }
        /// @note This result is needed only for setting metadata
        resultTag = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.instant_aec",
                                                             "instant_aec_available_modes",
                                                             &instantAecVendorTag);
        if (CamxResultSuccess == resultTag)
        {
            /// @note This can be ignored because all standard tag set properly
            resultTag = pStaticMetadataSlot->SetMetadataByTag(instantAecVendorTag,
                                                              static_cast<VOID*>(availableInstantAec),
                                                              count,
                                                              "camxContext");
        }

        // Publish mount angle in the vendor tags
        UINT32 mountAngleTag;
        UINT32 mountAngle;
        resultTag = VendorTagManager::QueryVendorTagLocation(
            "org.codeaurora.qcamera3.sensor_meta_data", "mountAngle", &mountAngleTag);
        mountAngle = pCameraInfo->mountAngle;
        resultTag = pStaticMetadataSlot->SetMetadataByTag(
            mountAngleTag, static_cast<VOID*>(&mountAngle), 1,
            "camxContext");
        // Publish camera position in the vendor tags
        UINT32 cameraPositionTag;
        UINT   cameraPosition;
        resultTag = VendorTagManager::QueryVendorTagLocation(
            "org.codeaurora.qcamera3.sensor_meta_data", "cameraPosition", &cameraPositionTag);
        cameraPosition = pCameraInfo->pSensorCaps->position;
        resultTag = pStaticMetadataSlot->SetMetadataByTag(
            cameraPositionTag, static_cast<VOID*>(&cameraPosition), 1,
            "camxContext");

        resultTag = OverrideMitigation(cameraId);

        UINT32 sensorModeFastShutterVendorTag;
        UINT8  isFastShutterModeSupportedondevice = pCameraInfo->pPlatformCaps->sensorModeFastShutter;
        UINT8  isFastShutterSensor                = pCameraInfo->pSensorCaps->isFSSensor;
        BOOL   isFastShutterModeSupported         = FALSE;

        if (isFastShutterModeSupportedondevice && isFastShutterSensor)
        {
            // FastShutter mode is supported only if sensor and device support the same
            isFastShutterModeSupported         = TRUE;
        }

        /// @note This result is needed only for setting metadata
        resultTag = VendorTagManager::QueryVendorTagLocation("org.quic.camera.SensorModeFS",
                                                             "isFastShutterModeSupported",
                                                             &sensorModeFastShutterVendorTag);

        if (CamxResultSuccess == resultTag)
        {
            /// @note This can be ignored because all standard tag set properly
            resultTag = pStaticMetadataSlot->SetMetadataByTag(sensorModeFastShutterVendorTag,
                                                              static_cast<VOID*>(&isFastShutterModeSupported),
                                                              1,
                                                              "camxContext");

        }

        if (pCameraInfo->pSensorCaps->isQuadCFASensor)
        {
            CAMX_LOG_INFO(CamxLogGroupHAL, "Add Quad CFA flag and Quad CFA dim (%dx%d) to vendor tag.",
                pCameraInfo->pSensorCaps->QuadCFADim.width,
                pCameraInfo->pSensorCaps->QuadCFADim.height);

            UINT32 QuadCFASensorVendorTag;
            /// @note This result is needed only for setting metadata
            resultTag = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.quadra_cfa",
                                                                 "is_qcfa_sensor",
                                                                 &QuadCFASensorVendorTag);
            if (CamxResultSuccess == resultTag)
            {
                UINT8  isQuadCFASensor = TRUE;
                /// @note This can be ignored because all standard tag set properly
                resultTag = pStaticMetadataSlot->SetMetadataByTag(QuadCFASensorVendorTag,
                                                                  static_cast<VOID*>(&isQuadCFASensor),
                                                                  1,
                                                                  "camxContext");
            }

            UINT32 QuadCFADimVendorTag;
            /// @note This result is needed only for setting metadata
            resultTag = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.quadra_cfa",
                                                                 "qcfa_dimension",
                                                                 &QuadCFADimVendorTag);
            if (CamxResultSuccess == resultTag)
            {
                INT32 dim[2] = { 0 };
                dim[0]       = pCameraInfo->pSensorCaps->QuadCFADim.width;
                dim[1]       = pCameraInfo->pSensorCaps->QuadCFADim.height;

                /// @note This can be ignored because all standard tag set properly
                resultTag = pStaticMetadataSlot->SetMetadataByTag(QuadCFADimVendorTag,
                                                                  static_cast<VOID*>(dim),
                                                                  2,
                                                                  "camxContext");
            }
        }
        if (pCameraInfo->pSensorCaps->isZZHDRSupported || pCameraInfo->pSensorCaps->isSHDRSupported)
        {
            UINT32 videoHdrVendorTag;
            /// @note This result is needed only for setting metadata
            resultTag = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.available_video_hdr_modes",
                                                                 "video_hdr_modes",
                                                                 &videoHdrVendorTag);
            if (CamxResultSuccess == resultTag)
            {
                INT32 modes[3] = {0};
                modes[0] = VideoHdrOff;
                modes[1] = VideoHdrOn;
                modes[2] = VideoHdrEnd;
                resultTag = pStaticMetadataSlot->SetMetadataByTag(videoHdrVendorTag,
                                                                  static_cast<VOID*>(modes),
                                                                  2,
                                                                  "camxContext");
            }
        }
        // IFE MAX Capabilities
        UINT32 singleIFEResolutionTag = 0;
        INT32  singleIFEMaxResolution = pCameraInfo->pPlatformCaps->IFEMaxLineWidth;
        resultTag = VendorTagManager::QueryVendorTagLocation("org.quic.camera.HWResourceInfo",
                                                             "IFEMaxLineWidth",
                                                             &singleIFEResolutionTag);
        if (CamxResultSuccess == resultTag)
        {
            resultTag = pStaticMetadataSlot->SetMetadataByTag(singleIFEResolutionTag,
                                                              static_cast<VOID*>(&singleIFEMaxResolution),
                                                              1,
                                                              "camxContext");
            if (CamxResultSuccess != resultTag)
            {
                CAMX_LOG_ERROR(CamxLogGroupHAL, "Failed to write vendor tag singleIFEResolution");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL, "Failed to get vendor tag location for SingleIFEResolution ");
        }

        // Number of IFE for a given Target
        UINT32 numIFEsforGivenTargetTag = 0;
        INT32  numIFEsforGivenTarget    = pCameraInfo->pPlatformCaps->numIFEsforGivenTarget;
        /// @note This result is needed only for setting metadata
        resultTag = VendorTagManager::QueryVendorTagLocation("org.quic.camera.HWResourceInfo",
                                                             "numIFEsforGivenTarget",
                                                             &numIFEsforGivenTargetTag);
        if (CamxResultSuccess == resultTag)
        {
            resultTag = pStaticMetadataSlot->SetMetadataByTag(numIFEsforGivenTargetTag,
                                                              static_cast<VOID*>(&numIFEsforGivenTarget),
                                                              1,
                                                              "camxContext");
            if (CamxResultSuccess != resultTag)
            {
                CAMX_LOG_ERROR(CamxLogGroupHAL, "Failed to write vendor tag numIFEsforGivenTarget");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL, "Failed to get vendor tag location for numIFEsforGivenTarget ");
        }

        // Number of IFEs used by the sensor in the worstcase scenario
        INT32  maxNumberOfIFEsRequired    = pCameraInfo->pPlatformCaps->maxNumberOfIFEsRequired;
        UINT32 maxNumberOfIFEsRequiredTag = 0;

        resultTag = VendorTagManager::QueryVendorTagLocation("org.quic.camera.HWResourceInfo",
                                                             "maxIFEsRequired",
                                                             &maxNumberOfIFEsRequiredTag);
        if (CamxResultSuccess == resultTag)
        {
            resultTag = pStaticMetadataSlot->SetMetadataByTag(maxNumberOfIFEsRequiredTag,
                                                              static_cast<VOID*>(&maxNumberOfIFEsRequired),
                                                              1,
                                                              "camxContext");
            if (CamxResultSuccess != resultTag)
            {
                CAMX_LOG_ERROR(CamxLogGroupHAL, "Failed to write vendor tag maxNumberOfIFEsRequiredTag for %s",
                    pCameraInfo->pSensorCaps->pSensorName);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL, "Failed to query vendor tag maxNumberOfIFEsRequiredTag for %s",
                pCameraInfo->pSensorCaps->pSensorName);
        }

        // Color Temperature Range
        UINT32 colorTemperatureVendorTag;
        INT32 availableColorTemperatureRange[2] = { 0 };
        availableColorTemperatureRange[0] = pCameraInfo->pPlatformCaps->colorTemperatureRange.minValue;
        availableColorTemperatureRange[1] = pCameraInfo->pPlatformCaps->colorTemperatureRange.maxValue;

        /// @note This result is needed only for setting metadata
        resultTag = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.manualWB",
                                                             "color_temperature_range",
                                                             &colorTemperatureVendorTag);
        if (CamxResultSuccess == resultTag)
        {
            /// @note This can be ignored because all standard tag set properly
            resultTag = pStaticMetadataSlot->SetMetadataByTag(colorTemperatureVendorTag,
                                                              static_cast<VOID*>(availableColorTemperatureRange),
                                                              2,
                                                              "camxContext");
        }
        // White Balance Gains Range
        UINT32 whiteBalanceGainVendorTag;
        FLOAT availableWBGainRange[2] = { 0.0 };
        availableWBGainRange[0] = pCameraInfo->pPlatformCaps->whiteBalanceGainsRange.minValue;
        availableWBGainRange[1] = pCameraInfo->pPlatformCaps->whiteBalanceGainsRange.maxValue;

        /// @note This result is needed only for setting metadata
        resultTag = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.manualWB",
                                                             "gains_range",
                                                             &whiteBalanceGainVendorTag);
        if (CamxResultSuccess == resultTag)
        {
            /// @note This can be ignored because all standard tag set properly
            resultTag = pStaticMetadataSlot->SetMetadataByTag(whiteBalanceGainVendorTag,
                                                              static_cast<VOID*>(availableWBGainRange),
                                                              2,
                                                              "camxContext");
        }

        // ICACapabilities
        UINT32 ICATransformTypeVendorTag;
        INT32 ICACapabilities[2] = { 0 };
        ICACapabilities[0] = pCameraInfo->pPlatformCaps->IPEICACapability.supportedIPEICATransformType;
        ICACapabilities[1] = pCameraInfo->pPlatformCaps->IPEICACapability.IPEICATransformGridSize;

        /// @note This result is needed only for setting metadata
        resultTag = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.platformCapabilities",
                                                             "IPEICACapabilities",
                                                             &ICATransformTypeVendorTag);
        if (CamxResultSuccess == resultTag)
        {
            /// @note This can be ignored because all standard tag set properly
            resultTag = pStaticMetadataSlot->SetMetadataByTag(ICATransformTypeVendorTag,
                                                              static_cast<VOID*>(ICACapabilities),
                                                              sizeof(IPEICACapability),
                                                              "camxContext");

        }

        // Publish Sensor Modes in Vendor Tags
        UINT32 sensorModeTag;
        // Table Contains sensor modes (width, height, fps)
        // first 2 elements of table are Number of sensor modes and size of each entries
        // corresponding to sensor mode each mode can be accessed by i * SensorModeTableEntrySize + 2
        INT32 availableCustomVideoFpsVals[MaxResolutions * SensorModeTableEntrySize + 2];

        ImageSensorModuleData*       pImageSensorModuleData =
            // NOWHINE CP036a: Since the function is const, had to add the const_cast
            const_cast<ImageSensorModuleData*>
            (m_pHwEnvironment->GetImageSensorModuleData(cameraId));
        ImageSensorData*             pImageSensorData       =
            pImageSensorModuleData->GetSensorDataObject();
        const ResolutionInformation* pResolutionInfo        = pImageSensorData->GetResolutionInfo();
        UINT32 numSensorModes = pResolutionInfo->resolutionDataCount;
        availableCustomVideoFpsVals[0] = numSensorModes;
        availableCustomVideoFpsVals[1] = SensorModeTableEntrySize;
        ChiSensorModeInfo* pSensorModeInfo;
        pSensorModeInfo = static_cast<CHISENSORMODEINFO*>
            (CAMX_CALLOC(sizeof(CHISENSORMODEINFO) * numSensorModes));  // Release the buffer
        EnumerateSensorModes(cameraId, numSensorModes, pSensorModeInfo);
        INT j = 2;
        for (UINT i = 0; i < numSensorModes ; i++, j++)
        {
            availableCustomVideoFpsVals[j++] =
                static_cast<INT32>(pSensorModeInfo[i].frameDimension.width);
            availableCustomVideoFpsVals[j++] =
                static_cast<INT32>(pSensorModeInfo[i].frameDimension.height);
            availableCustomVideoFpsVals[j] = static_cast<INT32>(pSensorModeInfo[i].frameRate);
        }
        resultTag = VendorTagManager::QueryVendorTagLocation(
            "org.quic.camera2.sensormode.info", "SensorModeTable", &sensorModeTag);
        if (CamxResultSuccess == resultTag)
        {
            /// @note This can be ignored because all standard tag set properly
            resultTag = pStaticMetadataSlot->SetMetadataByTag(sensorModeTag,
                static_cast<VOID*>(availableCustomVideoFpsVals),
                (numSensorModes * SensorModeTableEntrySize + 2),
                 "camxContext");
        }
        CAMX_FREE(pSensorModeInfo);

        // Publish Supported 60, 90 Fps resolution in Vendor Tags
        UINT32 customHFRFpsTag;
        HFRCustomParams customHFRfps[MaxCustomHFRSizes];
        count = pCameraInfo->pHwEnvironmentCaps->numCustomHFRParams *
            (sizeof(HFRCustomParams) / sizeof(INT32));
        for (UINT i = 0; i < pCameraInfo->pHwEnvironmentCaps->numCustomHFRParams; i++)
        {
            customHFRfps[i] = pCameraInfo->pHwEnvironmentCaps->customHFRParams[i];
        }
        // publish table has each entry <width, height, fps>
        resultTag = VendorTagManager::QueryVendorTagLocation(
            "org.quic.camera2.customhfrfps.info", "CustomHFRFpsTable", &customHFRFpsTag);
        if (CamxResultSuccess == resultTag)
        {
            /// @note This can be ignored because all standard tag set properly
            resultTag = pStaticMetadataSlot->SetMetadataByTag(customHFRFpsTag,
                                                              static_cast<VOID*>(customHFRfps),
                                                              count,
                                                              "camxContext");
        }

        // Publish Supported preview fps for 60 fps video-recording
        UINT32 streamBasedFPSTag;
        HFRCustomPreviewVideoParams supportedPreviewVideoFPS[MaxPreviewVideoFPS];
        // StreamBasedSupportedFPS streamBasedPreviewVideoFPS[16];
        // FLOAT supportedPreviewFPS[] = {60, 30, 15, 7.5};
        // UINT32 numSupportedPreviewFPS = sizeof(supportedPreviewFPS) / sizeof(FLOAT);
        // count = numSupportedPreviewFPS * (sizeof(StreamBasedSupportedFPS) / sizeof(INT32));
        count = pCameraInfo->pHwEnvironmentCaps->numSupportedPreviewVideoFPS *
            (sizeof(HFRCustomPreviewVideoParams) / sizeof(INT32));
        for (UINT i = 0; i < pCameraInfo->pHwEnvironmentCaps->numSupportedPreviewVideoFPS; i++)
        {
            supportedPreviewVideoFPS[i] = pCameraInfo->pHwEnvironmentCaps->supportedHFRPreviewVideoFPS[i];
        }
        // publish table has each entry <PreviewFPS, VideoFPS>
        resultTag = VendorTagManager::QueryVendorTagLocation(
            "org.quic.camera2.streamBasedFPS.info", "StreamBasedFPSTable", &streamBasedFPSTag);
        if (CamxResultSuccess == resultTag)
        {
            /// @note This can be ignored because all standard tag set properly
            resultTag = pStaticMetadataSlot->SetMetadataByTag(streamBasedFPSTag,
                                                              static_cast<VOID*>(supportedPreviewVideoFPS),
                                                              count,
                                                              "camxContext");
        }

        // LTM Dynamic Contrast Range
        FLOAT availableltmContastRange[2] = { 0 };
        FLOAT availableltmDarkBoostStrength[2] = { 0 };
        FLOAT availableltmBrightSupressStrength[2] = { 0 };
        availableltmContastRange[0] = availableltmDarkBoostStrength[0] = availableltmBrightSupressStrength[0] =
            pCameraInfo->pPlatformCaps->ltmContrastRange.min;
        availableltmContastRange[1] = availableltmDarkBoostStrength[1] = availableltmBrightSupressStrength[1] =
            pCameraInfo->pPlatformCaps->ltmContrastRange.max;

        /// @note This result is needed only for setting metadata
        resultTag = VendorTagManager::QueryVendorTagLocation("org.quic.camera.ltmDynamicContrast",
                                                             "ltmDynamicContrastStrengthRange",
                                                             &ltmContrastVendorTag);

        if (CamxResultSuccess == resultTag)
        {
            /// @note This can be ignored because all standard tag set properly
            resultTag = pStaticMetadataSlot->SetMetadataByTag(ltmContrastVendorTag,
                                                              static_cast<VOID*>(availableltmContastRange),
                                                              2,
                                                              "camxContext");

        }

        resultTag = VendorTagManager::QueryVendorTagLocation("org.quic.camera.ltmDynamicContrast",
                                                             "ltmDarkBoostStrengthRange",
                                                             &ltmDarkBoostStrengthVendorTag);

        if (CamxResultSuccess == resultTag)
        {
            /// @note This can be ignored because all standard tag set properly
            resultTag = pStaticMetadataSlot->SetMetadataByTag(ltmDarkBoostStrengthVendorTag,
                                                              static_cast<VOID*>(availableltmDarkBoostStrength),
                                                              2,
                                                              "camxContext");

        }

        resultTag = VendorTagManager::QueryVendorTagLocation("org.quic.camera.ltmDynamicContrast",
                                                             "ltmBrightSupressStrengthRange",
                                                             &ltmBrightSupressStrengthTag);

        if (CamxResultSuccess == resultTag)
        {
            /// @note This can be ignored because all standard tag set properly
            resultTag = pStaticMetadataSlot->SetMetadataByTag(ltmBrightSupressStrengthTag,
                                                              static_cast<VOID*>(availableltmBrightSupressStrength),
                                                              2,
                                                              "camxContext");

        }


        /// Facial attribute detection capability
        UINT32 bsgcAvailableTag;

        resultTag = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.stats",
                                                             "bsgc_available",
                                                              &bsgcAvailableTag);
        if (CamxResultSuccess == resultTag)
        {
            CamxResult          resultQuery;
            UINT8               bsgcAvailable = 0;

            if ((TRUE == GetStaticSettings()->enablePTDetection) || (TRUE == GetStaticSettings()->enableSMDetection) ||
                (TRUE == GetStaticSettings()->enableGBDetection) || (TRUE == GetStaticSettings()->enableCTDetection))
            {
                bsgcAvailable = 1;
            }

            resultTag = pStaticMetadataSlot->SetMetadataByTag(bsgcAvailableTag,
                                                              static_cast<VOID*>(&bsgcAvailable),
                                                              1,
                                                              "camxContext");
        }

        // HEICSupport
        UINT32 heicSupportTag;

        resultTag = VendorTagManager::QueryVendorTagLocation("org.quic.camera.HEICSupport",
                                                             "HEICEnabled",
                                                             &heicSupportTag);
        if (CamxResultSuccess == resultTag)
        {
            BOOL heicSupportCapability = TRUE;
            resultTag = pStaticMetadataSlot->SetMetadataByTag(heicSupportTag,
                                                              static_cast<VOID*>(&heicSupportCapability),
                                                              1,
                                                              "camxContext");
        }

        // SSMSupport
        UINT32 ssmSupportTag;

        resultTag = VendorTagManager::QueryVendorTagLocation("org.quic.camera.SSMSupport",
                                                             "SSMEnabled",
                                                             &ssmSupportTag);
        if (CamxResultSuccess == resultTag)
        {
            BOOL ssmSupportCapability = pCameraInfo->pPlatformCaps->ssmEnable;
            resultTag = pStaticMetadataSlot->SetMetadataByTag(ssmSupportTag,
                                                              static_cast<VOID*>(&ssmSupportCapability),
                                                              1,
                                                              "camxContext");
        }
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Failed to initialize static metadata pool!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiContext::GetStaticMetadataPool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MetadataPool* ChiContext::GetStaticMetadataPool(
    UINT32 cameraId)
{
    CAMX_ASSERT(cameraId < MaxNumCameras);

    CamxResult result = CamxResultSuccess;

    if (NULL == m_perCameraInfo[cameraId].pStaticMetadataPool)
    {
        // Static MetadataPool not belong to Pipeline, so give pipeline index as -1, and not parallelize the creation
        m_perCameraInfo[cameraId].pStaticMetadataPool = MetadataPool::Create(PoolType::Static,
                                                                             UINT32_MAX,
                                                                             NULL,
                                                                             1,
                                                                             "camxChiContext",
                                                                             0);

        CAMX_ASSERT(NULL != m_perCameraInfo[cameraId].pStaticMetadataPool);

        if (NULL != m_perCameraInfo[cameraId].pStaticMetadataPool)
        {
            result = InitializeStaticMetadataPool(cameraId);
        }

        if ((CamxResultSuccess == result) && (FALSE == HAL3MetadataUtil::IsMetadataTableInitialized()))
        {
            result = HAL3MetadataUtil::InitializeMetadataTable();
        }
    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Unable to Initialize Static Metadata Pool and metadata table!");
    }

    return m_perCameraInfo[cameraId].pStaticMetadataPool;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiContext::GetStaticMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const Metadata* ChiContext::GetStaticMetadata(
    UINT32 cameraId)
{
    const Metadata* pStaticMetadata     = NULL;
    MetadataPool*   pStaticMetadataPool = GetStaticMetadataPool(cameraId);

    if (NULL != pStaticMetadataPool)
    {
        pStaticMetadata = pStaticMetadataPool->GetSlot(0)->GetMetadata();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Unable to get Static Metadata Pool!");
    }

    return pStaticMetadata;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::InitializeStaticMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::InitializeStaticMetadata(
    UINT32         cameraId,
    ChiCameraInfo* pChiCameraInfo)
{
    CamxResult   result     = CamxResultSuccess;
    HwCameraInfo cameraInfo = {};

    if ((NULL != pChiCameraInfo) && (cameraId < GetNumCameras()))
    {
        result = m_pHwEnvironment->GetCameraInfo(cameraId, &cameraInfo);
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupHAL,
                       "Invalid arguments cameraId = %d, m_numImageSensors = %d, pCameraInfo = %p",
                       cameraId,
                       GetNumCameras(),
                       pChiCameraInfo);

        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        CameraInfo* pCameraInfo = static_cast<CameraInfo*>(pChiCameraInfo->pLegacy);
        pChiCameraInfo->sensorCaps.sensorId   = cameraInfo.pSensorCaps->sensorId;
        pCameraInfo->imageSensorFacing        = cameraInfo.imageSensorFacing;
        pCameraInfo->imageOrientation         = cameraInfo.imageOrientation;
#if ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
        pCameraInfo->deviceVersion            = CAMERA_DEVICE_API_VERSION_3_5;    /// Only support CHI
#else
        pCameraInfo->deviceVersion            = CAMERA_DEVICE_API_VERSION_3_3;    /// Only support CHI
#endif // ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28))
        /// @todo (CAMX-79) - Build static metadata from ImageSensorData and assign to pCameraInfo->pStaticCameraInfo.
        ///                   Satic metadata must stay valid and not change for the life of the CHIModule object.
        pCameraInfo->pStaticCameraInfo        = NULL;                             /// Use static metadata from above
        /// @todo (CAMX-541) Populate resource costing and conflicting devices.
        pCameraInfo->resourceCost             = 100;
        pCameraInfo->ppConflictingDevices     = NULL;
        pCameraInfo->conflictingDevicesLength = 0;

        // Finally update the static metadata info pointer to report it to frameworks.
        const Metadata* pMetadata = GetStaticMetadata(cameraId);

        pCameraInfo->pStaticCameraInfo = pMetadata;

        pChiCameraInfo->sensorCaps.positionType = static_cast<CHISENSORPOSITIONTYPE>(
            cameraInfo.pSensorCaps->position + 1);

        // This setting enable multi camera support.
        // two physical and one logical camera will be visiable.
        if (GetStaticSettings()->multiCameraEnable)
        {
            CAMX_LOG_INFO(CamxLogGroupHAL, "Enabled multi camera %d Logical with front %d",
                             GetStaticSettings()->multiCameraEnable,
                             GetStaticSettings()->multiCameraEnableFront);
            if ((TRUE == GetStaticSettings()->multiCameraEnableFront) &&
                (FRONT == pChiCameraInfo->sensorCaps.positionType))
            {
                // Add +1 to make position type as REAR AUX for forming a logical camera with front
                pChiCameraInfo->sensorCaps.positionType = REAR_AUX;
            }
        }
        else
        {
            if (FRONT_AUX == pChiCameraInfo->sensorCaps.positionType)
            {
                pChiCameraInfo->sensorCaps.positionType = FRONT;
            }
            else if (REAR_AUX == pChiCameraInfo->sensorCaps.positionType)
            {
                pChiCameraInfo->sensorCaps.positionType = REAR;
            }
        }

        CAMX_LOG_VERBOSE(CamxLogGroupHAL, "CameraId:%d has position:%d",
            cameraId,
            pChiCameraInfo->sensorCaps.positionType);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::GetCameraInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::GetCameraInfo(
    UINT32         cameraId,
    ChiCameraInfo* pCameraInfo)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCameraInfo) && (cameraId < GetNumCameras()))
    {
        result = InitializeStaticMetadata(cameraId, pCameraInfo);

        ImageSensorModuleData*       pImageSensorModuleData =
            // NOWHINE CP036a: Since the function is const, had to add the const_cast
            const_cast<ImageSensorModuleData*>(m_pHwEnvironment->GetImageSensorModuleData(cameraId));
        ImageSensorData*             pImageSensorData       = pImageSensorModuleData->GetSensorDataObject();
        const ResolutionInformation* pResolutionInfo        = pImageSensorData->GetResolutionInfo();

        pCameraInfo->numSensorModes = pResolutionInfo->resolutionDataCount;

        SensorModuleStaticCaps staticCaps = { 0 };
        pImageSensorData->GetSensorStaticCapability(&staticCaps, cameraId);

        pCameraInfo->sensorCaps.pixelSize          = staticCaps.pixelSize;
        pCameraInfo->sensorCaps.activeArray.left   = staticCaps.activeArraySize.xMin;
        pCameraInfo->sensorCaps.activeArray.top    = staticCaps.activeArraySize.yMin;
        pCameraInfo->sensorCaps.activeArray.width  = staticCaps.activeArraySize.width;
        pCameraInfo->sensorCaps.activeArray.height = staticCaps.activeArraySize.height;
        pCameraInfo->sensorCaps.pRawOTPData        = staticCaps.OTPData.EEPROMInfo.rawOTPData.pRawData;
        pCameraInfo->sensorCaps.rawOTPDataSize     = staticCaps.OTPData.EEPROMInfo.rawOTPData.rawDataSize;
        pCameraInfo->sensorCaps.size               = sizeof(CHISENSORCAPS);
        pCameraInfo->sensorCaps.pSensorName        = staticCaps.pSensorName;

        if (NULL != pImageSensorModuleData->GetLensInfo())
        {
            pCameraInfo->lensCaps.focalLength  = static_cast<FLOAT>(pImageSensorModuleData->GetLensInfo()->focalLength);
            pCameraInfo->lensCaps.horViewAngle = static_cast<FLOAT>(pImageSensorModuleData->GetLensInfo()->horizontalViewAngle);
        }
        pCameraInfo->lensCaps.size          = sizeof(CHILENSCAPS);
        pCameraInfo->lensCaps.isFixedFocus  = (pImageSensorModuleData->GetActuatorDataObject() == NULL) ? TRUE : FALSE;

        pCameraInfo->size = sizeof(CHICAMERAINFO);
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupHAL,
                       "Invalid arguments cameraId = %d, m_numImageSensors = %d, pCameraInfo = %p",
                       cameraId,
                       GetNumCameras(),
                       pCameraInfo);

        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::SetPipelineDescriptorOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiContext::SetPipelineDescriptorOutput(
    PipelineDescriptor*      pPipelineDescriptor,
    UINT                     numOutputs,
    ChiPortBufferDescriptor* pOutputBufferDescriptor)
{
    pPipelineDescriptor->numOutputs = numOutputs;

    for (UINT i = 0; i < numOutputs; i++)
    {
        Utils::Memcpy(&pPipelineDescriptor->outputData[i].nodePort,
                      &pOutputBufferDescriptor[i].pNodePort[0],
                      sizeof(ChiLinkNodeDescriptor));

        pPipelineDescriptor->outputData[i].pOutputStreamWrapper =
            reinterpret_cast<ChiStreamWrapper*>(pOutputBufferDescriptor[i].pStream->pPrivateInfo);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::SetPipelineDescriptorInputOptions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiContext::SetPipelineDescriptorInputOptions(
    PipelineDescriptor*      pPipelineDescriptor,
    UINT32                   numInputs,
    ChiPipelineInputOptions* pChiPipelineInputOptions)
{
    pPipelineDescriptor->numInputs = numInputs;

    for (UINT i = 0; i < numInputs; i++)
    {
        Utils::Memcpy(&(pPipelineDescriptor->inputData[i].nodePort),
                      &(pChiPipelineInputOptions[i].nodePort),
                      sizeof(ChiLinkNodeDescriptor));

        Utils::Memcpy(&(pPipelineDescriptor->inputData[i].bufferOptions),
                      &(pChiPipelineInputOptions[i].bufferOptions),
                      sizeof(ChiBufferOptions));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::SetPipelineDescriptorInputStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::SetPipelineDescriptorInputStream(
    PipelineDescriptor*            pPipelineDescriptor,
    const ChiPortBufferDescriptor* pBufferDescriptor,
    BOOL                           isWrapperOwner)
{
    CamxResult result = CamxResultSuccess;

    BOOL matchFound = FALSE;
    for (UINT input = 0; input < pPipelineDescriptor->numInputs; input++)
    {
        PipelineInputData* pPipelineInputData = &pPipelineDescriptor->inputData[input];

        if (NULL != pPipelineInputData->pInputStreamWrapper)
        {
            continue;
        }

        for (UINT portIndex = 0; portIndex < pBufferDescriptor->numNodePorts; portIndex++)
        {
            /// @todo (CAMX-1015) Add a function IsSameNodePort since its used in other places as well
            if ((pPipelineInputData->nodePort.nodeId         == pBufferDescriptor->pNodePort[portIndex].nodeId)         &&
                (pPipelineInputData->nodePort.nodeInstanceId == pBufferDescriptor->pNodePort[portIndex].nodeInstanceId) &&
                (pPipelineInputData->nodePort.nodePortId     == pBufferDescriptor->pNodePort[portIndex].nodePortId))
            {
                matchFound = TRUE;
                pPipelineInputData->pInputStreamWrapper =
                    reinterpret_cast<ChiStreamWrapper*>(pBufferDescriptor->pStream->pPrivateInfo);
                /// @todo (CAMX-1512) Session can contain all the created Wrappers that it can clean up when it is destroyed

                // In case of shared target source buffer by 2 input ports, the ports share the same input stream wrapper,
                // so we make only first input stream wrapper the owner to avoid freeing the same wrapper twice during destroy
                if (0 == portIndex)
                {
                    pPipelineInputData->isWrapperOwner      = isWrapperOwner;
                    pPipelineDescriptor->inputData[input].isWrapperOwner = isWrapperOwner;
                }
                break;
            }
        }
    }

    // if no match found
    if (FALSE == matchFound)
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "No Matching buffer descriptor found for one of it's input port of pipeline: %s, "
            "numInputs: %d",
            pPipelineDescriptor->pipelineName,
            pPipelineDescriptor->numInputs);

        for (UINT input = 0; input < pPipelineDescriptor->numInputs; input++)
        {
            PipelineInputData* pPipelineInputData = &pPipelineDescriptor->inputData[input];

            CAMX_LOG_CONFIG(CamxLogGroupChi, "%s: pipeline descriptor -- input PortId: %d, NodeId: %d, InstanceId: %d, "
                "InputStreamWrapper: %p",
                pPipelineDescriptor->pipelineName,
                pPipelineInputData->nodePort.nodePortId,
                pPipelineInputData->nodePort.nodeId,
                pPipelineInputData->nodePort.nodeInstanceId,
                pPipelineInputData->pInputStreamWrapper);

            for (UINT portIndex = 0; portIndex < pBufferDescriptor->numNodePorts; portIndex++)
            {
                CAMX_LOG_CONFIG(CamxLogGroupChi, "%s: Buffer descriptor -- input PortId: %d, NodeId: %d, InstanceId: %d",
                    pPipelineDescriptor->pipelineName,
                    pBufferDescriptor->pNodePort[portIndex].nodePortId,
                    pBufferDescriptor->pNodePort[portIndex].nodeId,
                    pBufferDescriptor->pNodePort[portIndex].nodeInstanceId);
            }
        }
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::CreatePipelineDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PipelineDescriptor* ChiContext::CreatePipelineDescriptor(
    const CHAR*                        pPipelineName,
    const ChiPipelineCreateDescriptor* pPipelineCreateDescriptor,
    UINT32                             numOutputs,
    ChiPortBufferDescriptor*           pOutputBufferDescriptor,
    UINT32                             numInputs,
    CHIPIPELINEINPUTOPTIONS*           pPipelineInputOptions)
{
    CamxResult               result                    = CamxResultSuccess;
    PipelineCreateInputData  pipelineCreateInputData   = { 0 };
    PipelineCreateOutputData pipelineCreateOutputData  = { 0 };
    PipelineDescriptor*      pPipelineDescriptor       = NULL;

    if ((NULL == pPipelineName)                                     ||
        (NULL == pPipelineCreateDescriptor)                         ||
        ((0   != numOutputs) && (NULL == pOutputBufferDescriptor)))
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid input arg pPipelineName=%s pPipelineCreateDescriptor=%p",
                       pPipelineName, pPipelineCreateDescriptor);
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        pPipelineDescriptor = static_cast<PipelineDescriptor*>(CAMX_CALLOC(sizeof(PipelineDescriptor)));
    }

    if (NULL != pPipelineDescriptor)
    {
        pPipelineDescriptor->flags.isRealTime = pPipelineCreateDescriptor->isRealTime;

        UINT                  numBatchedFrames           = pPipelineCreateDescriptor->numBatchedFrames;
        BOOL                  HALOutputBufferCombined    = pPipelineCreateDescriptor->HALOutputBufferCombined;
        UINT                  maxFPSValue                = pPipelineCreateDescriptor->maxFPSValue;
        OverrideOutputFormat  overrideImpDefinedFormat   = { {0} };

        CAMX_LOG_INFO(CamxLogGroupHAL, "numBatchedFrames:%d HALOutputBufferCombined:%d maxFPSValue:%d",
                      numBatchedFrames, HALOutputBufferCombined, maxFPSValue);

        pPipelineDescriptor->numBatchedFrames        = numBatchedFrames;
        pPipelineDescriptor->HALOutputBufferCombined = HALOutputBufferCombined;

        pPipelineDescriptor->maxFPSValue      = maxFPSValue;
        pPipelineDescriptor->cameraId         = pPipelineCreateDescriptor->cameraId;
        pPipelineDescriptor->pPrivData        = NULL;
        pPipelineDescriptor->pSessionMetadata = reinterpret_cast<MetaBuffer*>(pPipelineCreateDescriptor->hPipelineMetadata);

        OsUtils::StrLCpy(pPipelineDescriptor->pipelineName, pPipelineName, MaxStringLength256);

        for (UINT streamId = 0; streamId < numOutputs; streamId++)
        {
            ChiStream*          pChiStream          = pOutputBufferDescriptor[streamId].pStream;
            if (NULL != pChiStream)
            {
                pChiStream->pHalStream = NULL;
            }
            GrallocUsage64      grallocUsage        = GetGrallocUsage(pChiStream);
            BOOL                isVideoHwEncoder    = (GrallocUsageHwVideoEncoder ==
                                                      (GrallocUsageHwVideoEncoder & grallocUsage));

            // Override preview output format to UBWCTP10 if the session has video HDR
            if (TRUE == isVideoHwEncoder)
            {
                BOOL isFormatUBWCTP10 = ((grallocUsage & GrallocUsageUBWC) == GrallocUsageUBWC) &&
                    ((grallocUsage & GrallocUsage10Bit) == GrallocUsage10Bit);

                if (TRUE == isFormatUBWCTP10)
                {
                    overrideImpDefinedFormat.isHDR = 1;
                }
            }
        }

        for (UINT streamId = 0; streamId < numOutputs; streamId++)
        {
            if (NULL == pOutputBufferDescriptor[streamId].pStream)
            {
                CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid input pStream for streamId=%d", streamId);
                result = CamxResultEInvalidArg;
                break;
            }

            /// @todo (CAMX-1797) Need to fix the reinterpret_cast
            ChiStream*          pChiStream          = pOutputBufferDescriptor[streamId].pStream;
            Camera3Stream*      pHAL3Stream         = reinterpret_cast<Camera3Stream*>(pChiStream);
            ChiStreamWrapper*   pChiStreamWrapper   = NULL;
            GrallocProperties   grallocProperties;
            Format              selectedFormat;

            overrideImpDefinedFormat.isRaw = pOutputBufferDescriptor[streamId].bIsOverrideImplDefinedWithRaw;

            if (0 != (GetGrallocUsage(pChiStream) & GrallocUsageProtected))
            {
                pPipelineDescriptor->flags.isSecureMode = TRUE;
            }

            if (numBatchedFrames > 1)
            {
                pPipelineDescriptor->flags.isHFRMode = TRUE;
            }

            // override preview dataspace if the session has video hdr10
            if ((GrallocUsageHwComposer == (GetGrallocUsage(pChiStream) & GrallocUsageHwComposer)) &&
                (TRUE == overrideImpDefinedFormat.isHDR))
            {
                pChiStream->dataspace = DataspaceStandardBT2020_PQ;
            }

            grallocProperties.colorSpace         = static_cast<ColorSpace>(pChiStream->dataspace);
            grallocProperties.pixelFormat        = pChiStream->format;
            grallocProperties.grallocUsage       = GetGrallocUsage(pChiStream);
            grallocProperties.isInternalBuffer   = TRUE;
            grallocProperties.isRawFormat        = pOutputBufferDescriptor[streamId].bIsOverrideImplDefinedWithRaw;
            grallocProperties.staticFormat       = HwEnvironment::GetInstance()->GetStaticSettings()->outputFormat;
            grallocProperties.isMultiLayerFormat = ((TRUE == HALOutputBufferCombined) &&
                                                    (GrallocUsageHwVideoEncoder ==
                                                     (GrallocUsageHwVideoEncoder & GetGrallocUsage(pChiStream))));

            result = ImageFormatUtils::GetFormat(grallocProperties, selectedFormat);

            if (CamxResultSuccess == result)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCore,
                    "GetFormat: pixelFormat %d, outputFormat %d, rawformat %d, selectedFormat %d usage 0x%llx",
                    grallocProperties.pixelFormat, grallocProperties.staticFormat,
                    grallocProperties.isRawFormat, selectedFormat, grallocProperties.grallocUsage);
                pChiStreamWrapper = CAMX_NEW ChiStreamWrapper(pHAL3Stream, streamId, selectedFormat);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore,
                    "GetFormat failed, pixelFormat %d, outputFormat %d, rawformat %d usage %llu",
                    grallocProperties.pixelFormat, grallocProperties.staticFormat,
                    grallocProperties.isRawFormat, grallocProperties.grallocUsage);
            }

            CAMX_ASSERT(NULL != pChiStreamWrapper);

            if (NULL != pChiStreamWrapper)
            {
                auto* pOutputDesc = &pOutputBufferDescriptor[streamId];
                if (TRUE == pOutputDesc->hasValidBufferNegotiationOptions)
                {
                    pChiStreamWrapper->SetBufferNegotiationOptions(pOutputDesc->pBufferNegotiationsOptions);
                }

                UINT32 maxBuffer;
                if (pPipelineCreateDescriptor->HALOutputBufferCombined == TRUE)
                {
                    maxBuffer = 1;
                }
                else
                {
                    maxBuffer = numBatchedFrames;
                }

                if (ChiExternalNode == pOutputBufferDescriptor[streamId].pNodePort[0].nodeId)
                {

                    CAMX_LOG_INFO(CamxLogGroupChi, "StreamId=%d is associated with external Node %d and portID:%d",
                        streamId,
                        pOutputBufferDescriptor[streamId].pNodePort[0].nodeId,
                        pOutputBufferDescriptor[streamId].pNodePort[0].nodePortId);

                    SetChiStreamInfo(pChiStreamWrapper, maxBuffer, TRUE);
                }
                else
                {
                    SetChiStreamInfo(pChiStreamWrapper, maxBuffer, FALSE);
                }

                pChiStream->pPrivateInfo = pChiStreamWrapper;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Can't allocate StreamWrapper");
                result = CamxResultENoMemory;
                break;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "Out of memory");
        result = CamxResultENoMemory;
    }

    if (CamxResultSuccess == result)
    {
        // Unfortunately, we don't know the lifetime of the objects being pointed to, so we have to assume they will not
        // exist after this function call, and certainly not by the call to CamX::Session::Initialize, so we might as well
        // perform the conversion here, and keep the data in the format we expect
        result = ProcessPipelineCreateDesc(pPipelineCreateDescriptor,
                                           numOutputs,
                                           pOutputBufferDescriptor,
                                           pPipelineDescriptor);
    }

    if (result == CamxResultSuccess)
    {
        SetPipelineDescriptorOutput(pPipelineDescriptor, numOutputs, pOutputBufferDescriptor);

        pipelineCreateInputData.pPipelineDescriptor    = pPipelineDescriptor;
        pipelineCreateInputData.pChiContext            = this;
        pipelineCreateInputData.isSecureMode           = pPipelineDescriptor->flags.isSecureMode;
        pipelineCreateOutputData.pPipelineInputOptions = pPipelineInputOptions;

        result = Pipeline::Create(&pipelineCreateInputData, &pipelineCreateOutputData);

        if (CamxResultSuccess != result)
        {
            if (NULL != pipelineCreateOutputData.pPipeline)
            {
                pipelineCreateOutputData.pPipeline->Destroy();
            }
        }
        else
        {
            pPipelineDescriptor->pPrivData = pipelineCreateOutputData.pPipeline;
        }
    }

    if (CamxResultSuccess == result)
    {
        if ((FALSE == pPipelineCreateDescriptor->isRealTime) && (numInputs < pipelineCreateOutputData.numInputs))
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL, "Number inputs %d are not matching per pipeline descriptor", numInputs);
        }
        SetPipelineDescriptorInputOptions(pPipelineDescriptor, pipelineCreateOutputData.numInputs, pPipelineInputOptions);
    }
    else
    {
        if (NULL != pPipelineDescriptor)
        {
            DestroyPipelineDescriptor(pPipelineDescriptor);
            pPipelineDescriptor = NULL;
        }
        CAMX_LOG_ERROR(CamxLogGroupChi, "Pipeline descriptor creation failed");
    }

    return pPipelineDescriptor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::DestroyPipelineDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiContext::DestroyPipelineDescriptor(
    PipelineDescriptor* pPipelineDescriptor)
{
    if (NULL != pPipelineDescriptor)
    {
        if (NULL != pPipelineDescriptor->pipelineInfo.pNodeInfo)
        {
            for (UINT i = 0; i < pPipelineDescriptor->pipelineInfo.numNodes; i++)
            {
                PerNodeInfo* pPerNodeInfo = &pPipelineDescriptor->pipelineInfo.pNodeInfo[i];

                if (NULL != pPerNodeInfo->pNodeProperties)
                {
                    for (UINT j = 0; j < pPerNodeInfo->nodePropertyCount; j++)
                    {
                        CAMX_FREE(pPerNodeInfo->pNodeProperties[j].pValue);
                        pPerNodeInfo->pNodeProperties[j].pValue = NULL;
                    }
                    CAMX_FREE(pPerNodeInfo->pNodeProperties);
                    pPerNodeInfo->pNodeProperties = NULL;
                }

                if (NULL != pPerNodeInfo->inputPorts.pPortInfo)
                {
                    CAMX_FREE(pPerNodeInfo->inputPorts.pPortInfo);
                    pPerNodeInfo->inputPorts.pPortInfo = NULL;
                }

                if (NULL != pPerNodeInfo->outputPorts.pPortInfo)
                {
                    CAMX_FREE(pPerNodeInfo->outputPorts.pPortInfo);
                    pPerNodeInfo->outputPorts.pPortInfo = NULL;
                }
            }

            CAMX_FREE(pPipelineDescriptor->pipelineInfo.pNodeInfo);
            pPipelineDescriptor->pipelineInfo.pNodeInfo = NULL;
        }

        for (UINT i = 0; i < pPipelineDescriptor->numOutputs; i++)
        {
            if (NULL != pPipelineDescriptor->outputData[i].pOutputStreamWrapper)
            {
                CAMX_DELETE pPipelineDescriptor->outputData[i].pOutputStreamWrapper;
                pPipelineDescriptor->outputData[i].pOutputStreamWrapper = NULL;
            }
        }

        for (UINT i = 0; i < pPipelineDescriptor->numInputs; i++)
        {
            PipelineInputData* pPipelineInputData = &pPipelineDescriptor->inputData[i];

            if ((NULL != pPipelineInputData->pInputStreamWrapper) && (TRUE == pPipelineInputData->isWrapperOwner))
            {
                CAMX_DELETE pPipelineDescriptor->inputData[i].pInputStreamWrapper;
                pPipelineInputData->pInputStreamWrapper = NULL;
                pPipelineInputData->isWrapperOwner      = FALSE;
            }
        }

        if (pPipelineDescriptor->pPrivData != NULL)
        {
            Pipeline* pPipeline = static_cast<Pipeline*>(pPipelineDescriptor->pPrivData);
            pPipeline->Destroy();
            pPipelineDescriptor->pPrivData = NULL;
        }

        CAMX_FREE(pPipelineDescriptor);
        pPipelineDescriptor = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::CreatePipelineFromDesc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Pipeline* ChiContext::CreatePipelineFromDesc(
    PipelineDescriptor* pPipelineDescriptor,
    UINT                pipelineIndex)
{
    CamxResult result    = CamxResultSuccess;
    Pipeline*  pPipeline = NULL;

    if (NULL != pPipelineDescriptor)
    {
        if (NULL == pPipelineDescriptor->pPrivData)
        {
            PipelineCreateInputData  pipelineCreateInputData  = { 0 };
            PipelineCreateOutputData pipelineCreateOutputData = { 0 };
            ChiPipelineInputOptions  pipelineInputOptions[MaxPipelineInputs];

            /// @todo (CAMX-1015) Duplicated code
            pipelineCreateInputData.pChiContext            = this;
            pipelineCreateInputData.pPipelineDescriptor    = pPipelineDescriptor;
            pipelineCreateInputData.pipelineIndex          = pipelineIndex;
            pipelineCreateInputData.isRealTime             = pPipelineDescriptor->flags.isRealTime;
            pipelineCreateInputData.isSecureMode           = pPipelineDescriptor->flags.isSecureMode;

            // MaxPipelineInputs is the number of elements in the array below. Pipeline::Create call will fill in only those
            // many entries based on the actual number of pipeline buffer inputs or 1 entry for sensor input
            pipelineCreateOutputData.numInputs             = MaxPipelineInputs;
            pipelineCreateOutputData.pPipelineInputOptions = &pipelineInputOptions[0];

            result = Pipeline::Create(&pipelineCreateInputData, &pipelineCreateOutputData);

            if (CamxResultSuccess == result)
            {
                pPipelineDescriptor->pPrivData = pipelineCreateOutputData.pPipeline;
                pPipeline                      = pipelineCreateOutputData.pPipeline;
            }
        }
        else
        {
            pPipeline = static_cast<Pipeline*>(pPipelineDescriptor->pPrivData);

            CAMX_ASSERT((pPipeline->IsRealTime()   == pPipelineDescriptor->flags.isRealTime) &&
                        (pPipeline->IsSecureMode() == pPipelineDescriptor->flags.isSecureMode));

            pPipeline->ReusePipelineFromDescriptor();
            pPipeline->SetParams(pipelineIndex);
        }
    }

    return pPipeline;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::CreateChiFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::CreateChiFence(
    CHIFENCECREATEPARAMS* pInfo,
    CHIFENCEHANDLE*       phChiFence)
{
    CamxResult result    = CamxResultSuccess;
    ChiFence*  pChiFence = NULL;

    CAMX_ASSERT(NULL != pInfo);
    CAMX_ASSERT(NULL != phChiFence);

    pChiFence = static_cast<ChiFence*>(CAMX_CALLOC(sizeof(ChiFence)));
    if (NULL == pChiFence)
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupChi, "Out of memory");
    }
    else
    {
        pChiFence->hChiFence   = static_cast<CHIFENCEHANDLE>(pChiFence);
        pChiFence->type        = pInfo->type;
        pChiFence->aRefCount   = 1;
        pChiFence->resultState = ChiFenceInit;
    }

    if (CamxResultSuccess == result)
    {
        if (ChiFenceTypeInternal == pInfo->type)
        {
            result = CSLCreatePrivateFence(pInfo->pName, &pChiFence->hFence);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupChi, "Failed to create CSL fence: %s: %d", pInfo->pName, result);
                CAMX_FREE(pChiFence);
                pChiFence = NULL;
            }
        }
        else if (ChiFenceTypeEGL == pInfo->type)
        {
            pChiFence->eglSync = pInfo->eglSync;
        }
        else if (ChiFenceTypeNative == pInfo->type)
        {
            pChiFence->nativeFenceFD = pInfo->nativeFenceFD;
        }
        else
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid Chi fence type requested: %d", pInfo->type);
            CAMX_FREE(pChiFence);
            pChiFence = NULL;
        }
    }

    *phChiFence = static_cast<CHIFENCEHANDLE>(pChiFence);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::AttachChiFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::AttachChiFence(
    CHIFENCEHANDLE hChiFence)
{
    CamxResult result    = CamxResultSuccess;
    ChiFence*  pChiFence = static_cast<ChiFence*>(hChiFence);

    if (NULL == pChiFence)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupChi, "hChiFence is invalid");
    }
    else
    {
        CamxAtomicInc(&pChiFence->aRefCount);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::ReleaseChiFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::ReleaseChiFence(
    CHIFENCEHANDLE hChiFence)
{
    CamxResult result    = CamxResultSuccess;
    ChiFence*  pChiFence = static_cast<ChiFence*>(hChiFence);

    m_pReleaseChiFence->Lock();

    if (NULL == pChiFence)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupChi, "hChiFence is invalid");
    }
    else
    {
        if (0 == CamxAtomicDec(&pChiFence->aRefCount))
        {
            if (ChiFenceTypeInternal == pChiFence->type)
            {
                if (CSLInvalidHandle == pChiFence->hFence)
                {
                    result = CamxResultEInvalidArg;
                    CAMX_LOG_ERROR(CamxLogGroupChi, "hChiFence is invalid");
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupChi, "release Chi fence %d", pChiFence->hFence);
                    result = CSLReleaseFence(pChiFence->hFence);
                    if (CamxResultSuccess == result)
                    {
                        pChiFence->hFence = CSLInvalidFence;
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupChi, "Failed to release CSL fence %d", pChiFence->hFence);
                    }
                }

                CAMX_FREE(pChiFence);
                pChiFence = NULL;
            }
        }
        else
        {
            CAMX_ASSERT(pChiFence->aRefCount > 0);
        }
    }

    m_pReleaseChiFence->Unlock();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::SignalChiFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::SignalChiFence(
    CHIFENCEHANDLE hChiFence,
    CamxResult     resultStatus)
{
    CamxResult result    = CamxResultSuccess;
    ChiFence*  pChiFence = static_cast<ChiFence*>(hChiFence);

    if (NULL == pChiFence)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupChi, "hChiFence is invalid");
    }
    else if (ChiFenceTypeInternal != pChiFence->type)
    {
        result = CamxResultEUnsupported;
        CAMX_LOG_ERROR(CamxLogGroupChi, "Signal not supported for this type of fence: %p", hChiFence);
    }
    else if (CSLInvalidHandle == pChiFence->hFence)
    {
        result = CamxResultEInvalidState;
        CAMX_LOG_ERROR(CamxLogGroupChi, "Internal Chi fence %p has invalid state", hChiFence);
    }
    else
    {
        // Set the CHI fence error status and based on which node(s) waiting for the signal
        // is/are expected to retrigger processing or take some other appropriate action
        // NOTE: Fence signal is treated successful, even if CHI fence error status is not
        //       in order for the DRQ to continue to function properly

        result = SetChiFenceResult(hChiFence, resultStatus);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupChi,
                "Failed to set chi fence (%d) status (%d) in order to notify waiting the node(s)",
                pChiFence->hFence,
                resultStatus);
        }

        result = CSLFenceSignal(pChiFence->hFence, CSLFenceResultSuccess);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::SetChiFenceResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::SetChiFenceResult(
    CHIFENCEHANDLE hChiFence,
    CamxResult     lResult)
{
    CamxResult result    = CamxResultSuccess;
    ChiFence*  pChiFence = static_cast<ChiFence*>(hChiFence);

    if (NULL == pChiFence)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupChi, "pChiFence is NULL");
    }

    if (CamxResultSuccess == result)
    {
        // Set default value of the result to invalid state
        pChiFence->resultState = ChiFenceInvalid;

        if (ChiFenceTypeInternal != pChiFence->type)
        {
            result = CamxResultEUnsupported;
            CAMX_LOG_ERROR(CamxLogGroupChi, "Signal not supported for this type of fence: %p", hChiFence);
        }
        else if (CSLInvalidHandle == pChiFence->hFence)
        {
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupChi, "Internal Chi fence %p has invalid state", hChiFence);
        }

        if (CamxResultSuccess == result)
        {
            pChiFence->resultState = (CamxResultSuccess == lResult) ? ChiFenceSuccess : ChiFenceFailed;
            CAMX_LOG_INFO(CamxLogGroupChi, "Internal Chi fence %d has state %d", pChiFence->hFence, pChiFence->resultState);

        }
        else
        {
            pChiFence->resultState = ChiFenceInvalid;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::GetChiFenceResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::GetChiFenceResult(
    CHIFENCEHANDLE hChiFence,
    CDKResult*     pResult)
{
    CamxResult result    = CamxResultSuccess;
    ChiFence*  pChiFence = static_cast<ChiFence*>(hChiFence);

    if ((NULL == pChiFence) || (NULL == pResult))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid inputs pChiFence=%p, pResult=%p", pChiFence, pResult);
    }
    else if (ChiFenceTypeInternal != pChiFence->type)
    {
        result = CamxResultEUnsupported;
        CAMX_LOG_ERROR(CamxLogGroupChi, "Signal not supported for this type of fence: %p", hChiFence);
    }
    else if (CSLInvalidHandle == pChiFence->hFence)
    {
        result = CamxResultEInvalidState;
        CAMX_LOG_ERROR(CamxLogGroupChi, "Internal Chi fence %p has invalid state", hChiFence);
    }

    if (NULL != pResult)
    {
        *pResult = CDKResultEInvalidState;

        if (CamxResultSuccess == result)
        {
            if (ChiFenceSuccess == pChiFence->resultState)
            {
                *pResult = CDKResultSuccess;
            }
            else if (ChiFenceFailed == pChiFence->resultState)
            {
                *pResult = CDKResultEFailed;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::WaitChiFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::WaitChiFence(
    CHIFENCEHANDLE      hChiFence,
    PFNCHIFENCECALLBACK pCallback,
    VOID*               pUserData,
    UINT64              waitTime)
{
    CamxResult result    = CamxResultSuccess;
    ChiFence*  pChiFence = static_cast<ChiFence*>(hChiFence);

    if (NULL == pChiFence)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupChi, "hChiFence is invalid");
    }
    else if (ChiFenceTypeInternal == pChiFence->type)
    {
        if (CSLInvalidHandle == pChiFence->hFence)
        {
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupChi, "Internal Chi fence %p has invalid state", hChiFence);
        }
        else
        {
            if (NULL == pCallback)
            {
                result = CSLFenceWait(pChiFence->hFence, waitTime);
            }
            else
            {
                m_pDeferredRequestQueue->WaitForFenceDependency(&pChiFence, 1, pCallback, pUserData);
            }

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupChi, "Failed to wait on Chi fence %p", hChiFence);
            }
        }
    }
    else if ((ChiFenceTypeEGL == pChiFence->type) || (ChiFenceTypeNative == pChiFence->type))
    {
        result = m_pDeferredRequestQueue->WaitForFenceDependency(&pChiFence, 1, pCallback, pUserData);
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid Chi fence (%p) type: %d", pChiFence, pChiFence->type);
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::CreateSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHISession* ChiContext::CreateSession(
    UINT             numPipelines,
    ChiPipelineInfo* pPipelineInfo,
    ChiCallBacks*    pCallbacks,
    VOID*            pPrivateCallbackData,
    CHISESSIONFLAGS  sessionCreateflags)
{
    CHISession*          pChiSession = NULL;
    CHISessionCreateData createData  = {};

    /// @todo (CAMX-1512) HFR support for CHI
    createData.sessionCreateData.pChiContext             = this;
    createData.sessionCreateData.pHwContext              = m_pHwContext;
    createData.sessionCreateData.pThreadManager          = GetThreadManager();

    if (TRUE == pPipelineInfo->pipelineInputInfo.isInputSensor)
    {
        createData.sessionCreateData.usecaseNumBatchedFrames =
            pPipelineInfo->pipelineInputInfo.sensorInfo.pSensorModeInfo->batchedFrames;
        createData.sessionCreateData.HALOutputBufferCombined =
            pPipelineInfo->pipelineInputInfo.sensorInfo.pSensorModeInfo->HALOutputBufferCombined;
    }
    else
    {
        createData.sessionCreateData.usecaseNumBatchedFrames = 1;
        createData.sessionCreateData.HALOutputBufferCombined = FALSE;
    }

    createData.sessionCreateData.pChiAppCallBacks        = pCallbacks;
    createData.sessionCreateData.numPipelines            = numPipelines;
    createData.sessionCreateData.pPipelineInfo           = pPipelineInfo;
    createData.sessionCreateData.pPrivateCbData          = pPrivateCallbackData;
    createData.sessionCreateData.isNativeChi             = sessionCreateflags.u.isNativeChi;

    pChiSession = CHISession::Create(&createData);

    if (NULL == pChiSession)
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "Unable to create session");
    }

    return pChiSession;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::GetGrallocUsage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
GrallocUsage64 ChiContext::GetGrallocUsage(
    const ChiStream* pStream)
{
    GrallocUsage64 grallocUsage = 0;
    if (pStream != NULL)
    {
        if (pStream->pHalStream != NULL)
        {
            if ((ChiStreamTypeOutput == pStream->streamType) ||
                    (ChiStreamTypeBidirectional == pStream->streamType))
            {
                grallocUsage = pStream->pHalStream->producerUsage;
            }
            else if (ChiStreamTypeInput == pStream->streamType)
            {
                grallocUsage = pStream->pHalStream->consumerUsage;
            }
            else
            {
                grallocUsage = pStream->grallocUsage;
            }
        }
        else
        {
            grallocUsage = pStream->grallocUsage;
            CAMX_LOG_VERBOSE(CamxLogGroupChi, "Gralloc Usage is 32 bit here: 0x%llx", grallocUsage);
        }
        if (ChiStreamTypeInput != pStream->streamType)
        {
            grallocUsage |= GrallocUsageHwCameraWrite;
        }
        if (ChiStreamTypeOutput != pStream->streamType)
        {
            grallocUsage |= GrallocUsageHwCameraRead;
        }
    }
    return grallocUsage;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::SelectFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Format ChiContext::SelectFormat(
    const ChiStream*      pStream,
    OverrideOutputFormat  overrideImpDefinedFormat)
{
    /// @todo (CAMX-1512) Fix the format selection
    Format          format = Format::Blob;
    ChiStreamFormat pixelFormat;
    GrallocUsage64    streamGrallocUsage = 0;
    CHISTREAMTYPE   streamType;

    if (NULL != pStream)
    {
        pixelFormat = pStream->format;
        streamGrallocUsage = GetGrallocUsage(pStream);
        streamType = pStream->streamType;

        if (ChiStreamFormatYCrCb420_SP == pixelFormat)
        {
            format = Format::YUV420NV21;
        }
        else if (ChiStreamFormatYCbCr420_888 == pixelFormat)
        {
            // Determine if the stream's pixel format should be NV21
            if ((ChiStreamTypeOutput == pStream->streamType) &&
                (GrallocUsageSwReadOften == (GrallocUsageSwReadOften & streamGrallocUsage)))
            {
                format = Format::YUV420NV21;
            }
            else
            {
                format = Format::YUV420NV12;
            }
        }
        else if (ChiStreamFormatNV12HEIF == pixelFormat)
        {
            format = Format::YUV420NV12;
        }
        else if (ChiStreamFormatP010 == pixelFormat)
        {
            format = Format::P010;
        }
        else if (ChiStreamFormatImplDefined == pixelFormat)
        {
            if ((ChiStreamTypeBidirectional == streamType) || (ChiStreamTypeInput == streamType) ||
                ((streamGrallocUsage & GrallocUsageHwCameraZSL) == GrallocUsageHwCameraZSL))
            {
                if (TRUE == overrideImpDefinedFormat.isRaw)
                {
                    format = Format::RawMIPI;
                }
                else
                {
                    const StaticSettings* pStaticSettings = m_pHwEnvironment->GetSettingsManager()->GetStaticSettings();

                    if (OutputFormatYUV420NV21 == pStaticSettings->outputFormat)
                    {
                        format = Format::YUV420NV21;
                    }
                    else if ((OutputFormatUBWCNV12 == pStaticSettings->outputFormat) &&
                             (TRUE == pStaticSettings->multiCameraVREnable))
                    {
                        // VR Camera stitch node currently supports UBWC format for preview
                        // but other Dual Camera nodes such as SAT and RTB doesnt suppport UBWC format.
                        format = Format::UBWCNV12;
                    }
                    else
                    {
                        format = Format::YUV420NV12;
                    }
                }
            }
            else
            {
                format = static_cast<Format>(GetImplDefinedFormat(pStream, overrideImpDefinedFormat));
            }
            CAMX_LOG_INFO(CamxLogGroupChi, "select ChiStreamFormatImplDefined format = %d", format);
        }
        /// @todo (CAMX-1797) Fix format selection
        else if (ChiStreamFormatRaw16 == pixelFormat)
        {
            format = Format::RawPlain16;
        }
        else if (ChiStreamFormatRaw64 == pixelFormat)
        {
            format = Format::RawPlain64;
        }
        else if ((ChiStreamFormatRawOpaque == pixelFormat) ||
            (ChiStreamFormatRaw10 == pixelFormat) ||
            (ChiStreamFormatY8 == pixelFormat))
        {
            format = Format::RawMIPI;
        }
        else if (ChiStreamFormatBlob == pixelFormat)
        {
            if ((DataspaceJFIF == pStream->dataspace) ||
                (DataspaceV0JFIF == pStream->dataspace))
            {
                format = Format::Jpeg;
            }
            else
            {
                format = Format::Blob;
            }
        }
        else if (ChiStreamFormatPD10 == pixelFormat)
        {
            format = Format::PD10;
            CAMX_LOG_INFO(CamxLogGroupChi, "Select PD10 format = %d", format);
        }
        else if (ChiStreamFormatUBWCNV124R == pixelFormat)
        {
            format = Format::UBWCNV124R;
            CAMX_LOG_INFO(CamxLogGroupChi, "Select UBWCNV124R format = %d", format);
        }
        else if (ChiStreamFormatUBWCTP10 == pixelFormat)
        {
            format = Format::UBWCTP10;
            CAMX_LOG_INFO(CamxLogGroupChi, "Select UBWCTP10 format = %d", format);
        }
        else if (ChiStreamFormatY16 == pixelFormat)
        {
            format = Format::Y16;
        }
        else if (ChiStreamFormatUBWCNV12 == pixelFormat)
        {
            format = Format::UBWCNV12;
        }
        else
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("ERROR: CHI failed to pick the correct output format %u in CreatePipeline!",
                                       pixelFormat);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "pStream is NULL");
    }

    return format;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::GetImplDefinedFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Format ChiContext::GetImplDefinedFormat(
    const ChiStream*    pStream,
    OverrideOutputFormat  overrideImplDefinedFormat)
{
    Format       chosenImplDefinedFormat = Format::YUV420NV12;
    GrallocUsage64 grallocUsage            = GetGrallocUsage(pStream);

    if (NULL != pStream)
    {
        switch (m_pHwEnvironment->GetSettingsManager()->GetStaticSettings()->outputFormat)
        {
            case OutputFormatYUV420NV12:
                // Determine if the stream's format should be NV21
                if ((ChiStreamTypeInput == pStream->streamType)          ||
                    (ChiStreamTypeBidirectional == pStream->streamType)  ||
                    (GrallocUsageHwCameraZSL == (GrallocUsageHwCameraZSL & grallocUsage)))
                {
                    if (TRUE == overrideImplDefinedFormat.isRaw)
                    {
                        chosenImplDefinedFormat = Format::RawMIPI;
                    }
                    else
                    {
                        chosenImplDefinedFormat = Format::YUV420NV21;
                    }
                }
                else
                {
                    chosenImplDefinedFormat = Format::YUV420NV12;
                }
                break;

            case OutputFormatYUV420NV21:
                chosenImplDefinedFormat = Format::YUV420NV21;
                break;

            case OutputFormatUBWCNV12:
                chosenImplDefinedFormat = Format::UBWCNV12;
                break;

            case OutputFormatUBWCTP10:
                chosenImplDefinedFormat = Format::UBWCTP10;
                break;

            case OutputFormatRAWPLAIN16:
                chosenImplDefinedFormat = Format::RawPlain16;
                break;

            case OutputFormatRAWPLAIN64:
                chosenImplDefinedFormat = Format::RawPlain64;
                break;

            case OutputFormatPD10:
                chosenImplDefinedFormat = Format::PD10;
                break;

            case OutputFormatP010:
                chosenImplDefinedFormat = Format::P010;
                break;
            default:
                chosenImplDefinedFormat = Format::YUV420NV12;
                break;
        }

        if (TRUE == overrideImplDefinedFormat.isHDR)
        {
            chosenImplDefinedFormat = Format::UBWCTP10;
        }

        // If SW encoder gralloc flag is set, output must not be given in UBWC format
        if (((chosenImplDefinedFormat == Format::UBWCNV12)     ||
             (chosenImplDefinedFormat == Format::UBWCTP10)     ||
             (chosenImplDefinedFormat == Format::UBWCNV124R))  &&
            (GrallocUsageSwReadOften == (GrallocUsageSwReadOften & grallocUsage)))
        {
            chosenImplDefinedFormat = Format::YUV420NV12;
        }
    }
    // if encoder is set and grallocUsagePrivate is not set, output should not be UBWC
    // suggested by video-encoder team
    // There are 3 condition for selecting format in video usecase, as confirmed by video-enc team:
    // 1) Encoder + Private0 ---> UBWC
    // 2) Encoder + Private0 + TP10 ---> UBWCTP10
    // 3) Encoder (with no flags) ---> YUV420NV12
    BOOL isVideoHwEncoder = (GrallocUsageHwVideoEncoder == (GrallocUsageHwVideoEncoder & grallocUsage));

    if (TRUE == isVideoHwEncoder)
    {
        BOOL isFormatUBWCTP10 = ((grallocUsage & GrallocUsageUBWC) == GrallocUsageUBWC) &&
            ((grallocUsage & GrallocUsage10Bit) == GrallocUsage10Bit);
        BOOL isFormatUBWCNV12 = ((grallocUsage & GrallocUsageUBWC) == GrallocUsageUBWC) &&
            !((grallocUsage & GrallocUsage10Bit) == GrallocUsage10Bit);
        BOOL isFormatP010 = !((grallocUsage & GrallocUsagePrivate0) == GrallocUsagePrivate0) &&
            ((grallocUsage & GrallocUsage10Bit) == GrallocUsage10Bit);

        // overwrite the format based on the gralloc flag
        if (TRUE == isFormatUBWCTP10)
        {
            chosenImplDefinedFormat = Format::UBWCTP10;
        }
        else if (TRUE == isFormatUBWCNV12)
        {
            chosenImplDefinedFormat = Format::UBWCNV12;
        }
        else if (TRUE == isFormatP010)
        {
            chosenImplDefinedFormat = Format::P010;
        }
        else
        {
            chosenImplDefinedFormat = Format::YUV420NV12;
        }
        CAMX_LOG_INFO(CamxLogGroupChi, "Overwrite chosen format to %d", chosenImplDefinedFormat);

        // Overwrite with override settings format. For video other than NV12/NV21, dont
        // expect any other formats to be specified to be over written
        switch (m_pHwEnvironment->GetSettingsManager()->GetStaticSettings()->outputFormat)
        {
            case OutputFormatYUV420NV12:
                chosenImplDefinedFormat = Format::YUV420NV12;
                CAMX_LOG_INFO(CamxLogGroupChi, "Overwrite with override settings format %d", chosenImplDefinedFormat);
                break;
            case OutputFormatYUV420NV21:
                chosenImplDefinedFormat = Format::YUV420NV21;
                CAMX_LOG_INFO(CamxLogGroupChi, "Overwrite with override settings format %d", chosenImplDefinedFormat);
                break;
            default:
                // Don't do anything. Retain existing format
                CAMX_LOG_INFO(CamxLogGroupChi, "Not a valid override settings format %d for video",
                    m_pHwEnvironment->GetSettingsManager()->GetStaticSettings()->outputFormat);
                break;
        }
    }

    // check for platform capability if UBWCTP10 is supported as IPE output.
    if ((FALSE == m_pHwEnvironment->GetPlatformStaticCaps()->ubwctp10PreviewVideoIPESupported) &&
        (chosenImplDefinedFormat == Format::UBWCTP10))
    {
        CAMX_LOG_INFO(CamxLogGroupChi, "UBWCTP10 is not supported. Falling back to YUV420NV12");
        chosenImplDefinedFormat = Format::YUV420NV12;
    }

    CAMX_LOG_INFO(CamxLogGroupChi, "Setting format type %d", chosenImplDefinedFormat);
    return chosenImplDefinedFormat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::SetChiStreamInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiContext::SetChiStreamInfo(
    ChiStreamWrapper* pChiStreamWrapper,
    UINT              numBatchFrames,
    BOOL              bExternalNode)
{
    /// @todo (CAMX-1512) Fix the stream - NativeStream returns Camera3Stream
    ChiStream* pStream = reinterpret_cast<ChiStream*>(pChiStreamWrapper->GetNativeStream());
    GrallocUsage64 grallocUsage = 0;
    GrallocUsage grallocUsage32 = 0;

    if (NULL != pStream)
    {
        CHISTREAMFORMAT overrideFormat = pStream->format;
        grallocUsage32 = pStream->grallocUsage;
        grallocUsage = GetGrallocUsage(pStream);

        switch (pStream->streamType)
        {
            case ChiStreamTypeOutput:

                if (grallocUsage & GrallocUsageHwVideoEncoder)
                {
                    grallocUsage |= (GrallocUsageSwReadRarely | GrallocUsageSwWriteRarely | GrallocUsageHwCameraWrite);
                }
                else
                {
                    grallocUsage |= GrallocUsageHwCameraWrite;
                }

                // We assume if ChiNode is the sink node in a Protected use case, it is running on CDSP.
                // In such cases we want Gralloc to allocate buffers from secure DSP carve out.
                // App sets GrallocUsagePrivateCDSP in usage bits while requesting Gralloc buffers.
                // But, Gralloc would give preference to GrallocUsageHwCameraWrite than GrallocUsagePrivateCDSP, so remove
                // GrallocUsageHwCameraWrite so that Gralloc allocates from secure DSP carve out instead of camera/display
                // carve out..
                if ((0    != (grallocUsage & GrallocUsageProtected))   &&
                    (TRUE == bExternalNode))
                {
                    grallocUsage &= ~(GrallocUsageHwCameraWrite);
                }

                if (ChiStreamFormatImplDefined == pStream->format)
                {
                    overrideFormat = ChiStreamFormatImplDefined;
                    // setting gralloc usage flag as private0 for UBWC NV12 or TP10
                    CAMX_LOG_INFO(CamxLogGroupChi, "Internal format %d ",
                        pChiStreamWrapper->GetInternalFormat());
                    switch (pChiStreamWrapper->GetInternalFormat())
                    {
                        case Format::UBWCNV12FLEX:
                            overrideFormat = ChiStreamFormatNV12UBWCFLEX;
                            break;
                        case Format::UBWCNV12:
                            grallocUsage |= GrallocUsagePrivate0;
                            overrideFormat = ChiStreamFormatUBWCNV12;
                            break;

                        case Format::UBWCTP10:
                            grallocUsage |= (GrallocUsageUBWC | GrallocUsage10Bit);
                            overrideFormat = ChiStreamFormatUBWCTP10;
                            break;

                        case Format::P010:
                            grallocUsage |= GrallocUsage10Bit;
                            overrideFormat = ChiStreamFormatP010;
                            break;

                        case Format::YUV420NV21:
                            if (0 != (GrallocUsageHwVideoEncoder & grallocUsage))
                            {
                                grallocUsage |= GrallocUsageProducerVideoNV21Encoder;
                            }
                            else
                            {
                                grallocUsage |= GrallocUsageProducerCamera;
                            }
                            break;
                        case Format::YUV420NV12:
                            if ((grallocUsage & GrallocUsagePrivate0) &&
                                (grallocUsage & GrallocUsageHwVideoEncoder))
                            {
                                overrideFormat = ChiStreamFormatYCbCr420_888;
                            }
                            // Check if ImageEncoder is enabled for HEIF
                            if ((GrallocUsageHwImageEncoder == (GrallocUsageHwImageEncoder & grallocUsage)) ||
                                (DataspaceHEIF == pStream->dataspace))
                            {
                                overrideFormat = ChiStreamFormatNV12HEIF;
                            }
                            grallocUsage &= ~(GrallocUsagePrivate0 | GrallocUsagePrivate3);
                            break;

                        default:
                            grallocUsage &= ~(GrallocUsagePrivate0 | GrallocUsagePrivate3);
                            break;
                    }
                }

                break;

            case ChiStreamTypeInput:
            case ChiStreamTypeBidirectional:
                grallocUsage |= (GrallocUsageHwCameraRead | GrallocUsageHwCameraWrite);
                break;

            default:
                break;
        }

        if (pStream->pHalStream)
        {
            if (ChiStreamFormatImplDefined == pStream->format)
            {
                pStream->pHalStream->overrideFormat = overrideFormat;
                if (((overrideFormat == ChiStreamFormatUBWCTP10) ||
                        (overrideFormat == ChiStreamFormatUBWCNV12)) &&
                    (m_pHwEnvironment->GetPlatformStaticCaps()->ubwcVersion > UBWCVersion2))
                {
                    // Check if resolution is greater than UHD and UBWC version is atleast 3.0
                    // to support Perceptually Indistinguishable UBWC format
                    CAMX_LOG_INFO(CamxLogGroupChi,
                        "overrideFormat: 0x%x, w x h : %d x %d  ubwc version: %d",
                        overrideFormat, pStream->width, pStream->height,
                        m_pHwEnvironment->GetPlatformStaticCaps()->ubwcVersion);

                    if (TRUE == IsUBWCLossySupported(pStream, pChiStreamWrapper))
                    {
                        grallocUsage |= GrallocUsagePrivateAllocUBWCPI;
                        CAMX_LOG_INFO(CamxLogGroupChi,
                            "overrideFormat: 0x%x, grallocUsage: %llx",
                            overrideFormat, grallocUsage);
                    }
                }
            }
            if ((pStream->streamType == ChiStreamTypeOutput) ||
                   (pStream->streamType == ChiStreamTypeBidirectional) )
            {
                pStream->pHalStream->consumerUsage = 0;
                pStream->pHalStream->producerUsage = grallocUsage;
                pChiStreamWrapper->SetNativeProducerGrallocUsage(pStream->pHalStream->producerUsage);
            }
            else if (pStream->streamType == ChiStreamTypeInput)
            {
                pStream->pHalStream->consumerUsage = grallocUsage;
                pStream->pHalStream->producerUsage = 0;
                pChiStreamWrapper->SetNativeConsumerGrallocUsage(pStream->pHalStream->consumerUsage);
            }

            HALPixelFormat format = static_cast<HALPixelFormat>(pStream->pHalStream->overrideFormat);
            pChiStreamWrapper->SetNativeOverrideFormat(format);

            CAMX_LOG_INFO(CamxLogGroupCore,
                "overrideformat: 0x%x consumerUsage: 0x%llx producerUsage: 0x%llx",
                pStream->pHalStream->overrideFormat,
                pStream->pHalStream->consumerUsage,
                pStream->pHalStream->producerUsage);
        }
        grallocUsage32 = static_cast<HALPixelFormat>(grallocUsage & 0xFFFFFFFF);
        pChiStreamWrapper->SetNativeGrallocUsage(grallocUsage32);
    }
    UINT32 maxNumBuffers = DefaultRequestQueueDepth;

    // For HFR usecase, only increase video buffers, don't increase preview buffers.
    // This is to fix green frame issue when switching between normal and HFR video mode
    // Check pStream before dereferencing it.
    if ((numBatchFrames > 1) && (NULL != pStream) &&
        (GrallocUsageHwVideoEncoder == (GrallocUsageHwVideoEncoder & pStream->grallocUsage)))
    {
        maxNumBuffers = RequestQueueDepth * numBatchFrames;

        if (maxNumBuffers > MaxNumberOfBuffersAllowed)
        {
            maxNumBuffers = MaxNumberOfBuffersAllowed;
        }
    }
    if ((NULL != pStream) && (0 != (grallocUsage & GrallocUsageProtected)))
    {
        maxNumBuffers = GetStaticSettings()->maxBuffersSecureCamera;
    }
    pChiStreamWrapper->SetNativeMaxNumBuffers(maxNumBuffers);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiContext::CloneNodeProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiContext::CloneNodeProperties(
    ChiNode*      pChiNode,
    PerNodeInfo*  pPerNodeInfo)
{
    SIZE_T length = 0;

    pPerNodeInfo->nodePropertyCount = pChiNode->numProperties;

    if (0 == pPerNodeInfo->nodePropertyCount)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupHAL, "Nothing to clone");
        pPerNodeInfo->pNodeProperties = NULL;
        return;
    }

    pPerNodeInfo->pNodeProperties =
        static_cast<PerNodeProperty*>(CAMX_CALLOC(sizeof(PerNodeProperty) * pChiNode->numProperties));

    if (NULL == pPerNodeInfo->pNodeProperties)
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "No memory allocated for pNodeProperties");
        return;
    }

    for (UINT i = 0; i < pChiNode->numProperties; i++)
    {
        pPerNodeInfo->pNodeProperties[i].id = pChiNode->pNodeProperties[i].id;

        switch (pChiNode->pNodeProperties[i].id)
        {
            case NodePropertyCustomLib:
            case NodePropertyProfileId:
            case NodePropertyStabilizationType:
            case NodePropertyProcessingType:
            case NodePropertyIPEDownscale:
            case NodePropertyIPEDownscaleWidth:
            case NodePropertyIPEDownscaleHeight:
            case NodePropertyIFECSIDHeight:
            case NodePropertyIFECSIDWidth:
            case NodePropertyIFECSIDTop:
            case NodePropertyIFECSIDLeft:
            case NodePropertyNodeClass:
            case NodePropertyGPUCapsMaskType:
            case NodePropertyForceSingleIFEOn:
            case NodePropertyEnbaleIPECHICropDependency:
                length = OsUtils::StrLen(static_cast<const CHAR*>(pChiNode->pNodeProperties[i].pValue)) + 1;
                pPerNodeInfo->pNodeProperties[i].pValue = CAMX_CALLOC(length);
                Utils::Memcpy(pPerNodeInfo->pNodeProperties[i].pValue, pChiNode->pNodeProperties[i].pValue, length);
                break;
            case NodePropertyStatsSkipPattern:
            case NodePropertySkipUpdatingBufferProperty:
                length = sizeof(UINT);
                pPerNodeInfo->pNodeProperties[i].pValue = CAMX_CALLOC(length);
                Utils::Memcpy(pPerNodeInfo->pNodeProperties[i].pValue, pChiNode->pNodeProperties[i].pValue, length);
                break;
            case NodePropertyEnableFOVC:
                length = sizeof(UINT);
                pPerNodeInfo->pNodeProperties[i].pValue = CAMX_CALLOC(length);
                Utils::Memcpy(pPerNodeInfo->pNodeProperties[i].pValue, pChiNode->pNodeProperties[i].pValue, length);
                break;
            case NodePropertyStitchMaxJpegSize:
                length = OsUtils::StrLen(static_cast<const CHAR*>(pChiNode->pNodeProperties[i].pValue)) + 1;
                pPerNodeInfo->pNodeProperties[i].pValue = CAMX_CALLOC(length);
                Utils::Memcpy(pPerNodeInfo->pNodeProperties[i].pValue, pChiNode->pNodeProperties[i].pValue, length);
                break;

            default:
                break;
        }

        if (NodePropertyVendorStart <= pChiNode->pNodeProperties[i].id)
        {
            length = OsUtils::StrLen(static_cast<const CHAR*>(pChiNode->pNodeProperties[i].pValue)) + 1;
            pPerNodeInfo->pNodeProperties[i].pValue = CAMX_CALLOC(length);
            Utils::Memcpy(pPerNodeInfo->pNodeProperties[i].pValue, pChiNode->pNodeProperties[i].pValue, length);
        }
    }
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiContext::ProcessPipelineCreateDesc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::ProcessPipelineCreateDesc(
    const ChiPipelineCreateDescriptor* pPipelineCreateDescriptor,
    UINT                               numOutputs,
    ChiPortBufferDescriptor*           pOutputBufferDescriptor,
    PipelineDescriptor*                pPipelineDescriptor)
{
    CAMX_UNREFERENCED_PARAM(numOutputs);
    CAMX_UNREFERENCED_PARAM(pOutputBufferDescriptor);

    CamxResult result = CamxResultSuccess;

    PerPipelineInfo* pPipelineInfo = &pPipelineDescriptor->pipelineInfo;

    Utils::Memset(pPipelineInfo, 0, sizeof(PerPipelineInfo));

    pPipelineInfo->numNodes  = pPipelineCreateDescriptor->numNodes;
    if (0 < pPipelineInfo->numNodes)
    {
        pPipelineInfo->pNodeInfo = static_cast<PerNodeInfo*>(CAMX_CALLOC(sizeof(PerNodeInfo) * pPipelineInfo->numNodes));
    }

    if (NULL != pPipelineInfo->pNodeInfo)
    {
        const ChiNodeLink* pChiNodeLinkages = pPipelineCreateDescriptor->pLinks;

        for (UINT node = 0; node < pPipelineInfo->numNodes; node++)
        {
            UINT32 numInputPorts  = pPipelineCreateDescriptor->pNodes[node].nodeAllPorts.numInputPorts;
            UINT32 numOutputPorts = pPipelineCreateDescriptor->pNodes[node].nodeAllPorts.numOutputPorts;

            pPipelineInfo->pNodeInfo[node].inputPorts.numPorts  = numInputPorts;
            pPipelineInfo->pNodeInfo[node].outputPorts.numPorts = numOutputPorts;

            if (0 < numInputPorts)
            {
                pPipelineInfo->pNodeInfo[node].inputPorts.pPortInfo =
                    static_cast<InputPortInfo*>(CAMX_CALLOC(sizeof(InputPortInfo) * numInputPorts));
                if (NULL == pPipelineInfo->pNodeInfo[node].inputPorts.pPortInfo)
                {
                    result = CamxResultENoMemory;
                    CAMX_LOG_ERROR(CamxLogGroupHAL,
                                   "Input ports info mem calloc failed for nodeId %d at index %d",
                                   pPipelineCreateDescriptor->pNodes[node].nodeId,
                                   node);
                    break;
                }
            }

            if (0 < numOutputPorts)
            {
                pPipelineInfo->pNodeInfo[node].outputPorts.pPortInfo =
                    static_cast<OutputPortInfo*>(CAMX_CALLOC(sizeof(OutputPortInfo) * numOutputPorts));
                if (NULL == pPipelineInfo->pNodeInfo[node].outputPorts.pPortInfo)
                {
                    result = CamxResultENoMemory;
                    CAMX_LOG_ERROR(CamxLogGroupHAL, "Output ports info mem calloc failed for nodeId %d at index %d",
                                   pPipelineCreateDescriptor->pNodes[node].nodeId,
                                   node);
                    break;
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            for (UINT node = 0; node < pPipelineInfo->numNodes; node++)
            {
                PerNodeInfo*  pPerNodeInfo   = &pPipelineInfo->pNodeInfo[node];
                ChiNode*      pChiNode       = &pPipelineCreateDescriptor->pNodes[node];
                ChiNodePorts* pChiPorts      = &pChiNode->nodeAllPorts;

                pPerNodeInfo->nodeId                = pChiNode->nodeId;
                pPerNodeInfo->instanceId            = pChiNode->nodeInstanceId;

                CloneNodeProperties(pChiNode, pPerNodeInfo);

                // IFE is always the first HW Node (thereby checking on HwNodeIDStart). Any topology that has sensor node
                // would be a real time and with same analogy, any topology that has IFE is also real time as sensor is
                // hooked directly to IFE. Since there would be some usecases where there is only IFE that is hooked with
                // external sensor then it makes sense to check for IFE
                if (Sensor == pPerNodeInfo->nodeId)
                {
                    pPipelineDescriptor->flags.isSensorInput = TRUE;
                    pPipelineDescriptor->flags.isRealTime    = TRUE;
                }

                /// @todo (CAMX-3119) remove Torch check below and handle this in generic way.
                if (Torch == pPerNodeInfo->nodeId)
                {
                    pPipelineDescriptor->flags.isTorchWidget = TRUE;
                }

                pPerNodeInfo->nodeClass = NodeClass::Default;
                for (UINT index = 0; index < pPerNodeInfo->nodePropertyCount; index++)
                {
                    if ((NodePropertyNodeClass == pPerNodeInfo->pNodeProperties[index].id))
                    {
                        const CHAR* pValue    = static_cast<const CHAR*>(pPerNodeInfo->pNodeProperties[index].pValue);
                        const UINT  nodeClass = OsUtils::StrToUL(pValue, NULL, 10);
                        switch (nodeClass)
                        {
                            case 2: // Inplace Node
                            case 1: // Bypass Node
                            case 0: // Default Node
                                pPerNodeInfo->nodeClass = static_cast<NodeClass>(nodeClass);
                                break;
                            default:
                                CAMX_LOG_ERROR(CamxLogGroupCore, "Unknown node type: %u", nodeClass);
                                break;
                        }
                        break;
                    }
                }

                // Go through the output ports
                for (UINT outputPort = 0; outputPort < pPerNodeInfo->outputPorts.numPorts; outputPort++)
                {
                    ChiOutputPortDescriptor* pChiOutputPortDescriptor = &pChiPorts->pOutputPorts[outputPort];
                    OutputPortInfo*          pOutputPortInfo          = &pPerNodeInfo->outputPorts.pPortInfo[outputPort];

                    pOutputPortInfo->portId                 = pChiOutputPortDescriptor->portId;
                    pOutputPortInfo->portSourceTypeId       = pChiOutputPortDescriptor->portSourceTypeId;
                    pOutputPortInfo->numSourceIdsMapped     = pChiOutputPortDescriptor->numSourceIdsMapped;
                    pOutputPortInfo->pMappedSourcePortIds   = pChiOutputPortDescriptor->pMappedSourceIds;

                    if (FALSE != pChiOutputPortDescriptor->isOutputStreamBuffer)
                    {
                        pOutputPortInfo->flags.isSinkBuffer = TRUE;
                    }
                    else if (FALSE != pChiOutputPortDescriptor->isSinkPort)
                    {
                        pOutputPortInfo->flags.isSinkNoBuffer = TRUE;
                    }

                    for (UINT link = 0; link < pPipelineCreateDescriptor->numLinks; link++)
                    {
                        ///@ todo (CAMX-1797) Disable setting
                        if (TRUE == GetStaticSettings()->forceDisableUBWCOnIfeIpeLink)
                        {
                            for (UINT dst = 0; dst < pChiNodeLinkages[link].numDestNodes; dst++)
                            {
                                if ((pChiNodeLinkages[link].srcNode.nodeId == 65536) ||
                                    (pChiNodeLinkages[link].srcNode.nodeId == 255))
                                {
                                    if (Format::UBWCTP10 ==
                                        static_cast<Format>(pChiNodeLinkages[link].bufferProperties.bufferFormat))
                                    {
                                        UINT32* pOverrideFormat =
                                            // NOWHINE CP036a: Since the function is const, had to add the const_cast
                                            const_cast<UINT32*>(&(pChiNodeLinkages[link].bufferProperties.bufferFormat));

                                        *pOverrideFormat = static_cast<UINT32>(Format::YUV420NV12);
                                        break;
                                    }
                                }
                            }
                        }

                        if (TRUE == GetStaticSettings()->enableUBWCNV124ROnIFEFullOutIPELink)
                        {
                            for (UINT dst = 0; dst < pChiNodeLinkages[link].numDestNodes; dst++)
                            {
                                if ((pChiNodeLinkages[link].srcNode.nodeId == 65536) &&
                                    (pChiNodeLinkages[link].pDestNodes[dst].nodeId == 65538))
                                {
                                    if (Format::UBWCTP10 ==
                                        static_cast<Format>(pChiNodeLinkages[link].bufferProperties.bufferFormat))
                                    {
                                        UINT32* pOverrideFormat =
                                            // NOWHINE CP036a: Since the function is const, had to add the const_cast
                                            const_cast<UINT32*>(&(pChiNodeLinkages[link].bufferProperties.bufferFormat));

                                        *pOverrideFormat = static_cast<UINT32>(Format::UBWCNV124R);
                                        break;
                                    }
                                }
                            }
                        }

                        // SM6350 or SM7225 does not support UBWC width > 8000 and height > 6000,
                        // so overriding the format to NV12
                        if ((CSLCameraTitanSocSM6350 == m_pHwEnvironment->GetSocId()) ||
                            (CSLCameraTitanSocSM7225 == m_pHwEnvironment->GetSocId()))
                        {
                            for (UINT dst = 0; dst < pChiNodeLinkages[link].numDestNodes; dst++)
                            {
                                ChiStream* pChiStream = pOutputBufferDescriptor[dst].pStream;

                                if (NULL != pChiStream)
                                {
                                    if ((UBWCMaxSupportedWidth  < pChiStream->width) ||
                                        (UBWCMaxSupportedHeight < pChiStream->height))
                                    {
                                        if ((pChiNodeLinkages[link].srcNode.nodeId == 65539) &&
                                            (pChiNodeLinkages[link].pDestNodes[dst].nodeId == 65538))
                                        {
                                            if (Format::UBWCTP10 ==
                                                static_cast<Format>(pChiNodeLinkages[link].bufferProperties.bufferFormat))
                                            {
                                                UINT32* pOverrideFormat =
                                                   // NOWHINE CP036a: Since the function is const, had to add the const_cast
                                                   const_cast<UINT32*>(&(pChiNodeLinkages[link].bufferProperties.bufferFormat));

                                                *pOverrideFormat = static_cast<UINT32>(Format::YUV420NV12);
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        if ((pOutputPortInfo->portId  == pChiNodeLinkages[link].srcNode.nodePortId) &&
                            (pPerNodeInfo->nodeId     == pChiNodeLinkages[link].srcNode.nodeId) &&
                            (pPerNodeInfo->instanceId == pChiNodeLinkages[link].srcNode.nodeInstanceId))
                        {
                            const ChiLinkBufferProperties* pChiLinkBufferProperties = &pChiNodeLinkages[link].bufferProperties;
                            const ChiLinkProperties*       pChiLinkProperties       = &pChiNodeLinkages[link].linkProperties;

                            pOutputPortInfo->portLink.numInputPortsConnected = pChiNodeLinkages[link].numDestNodes;

                            if (FALSE != pChiLinkProperties->isBatchedMode)
                            {
                                pOutputPortInfo->portLink.linkProperties.isBatchMode = TRUE;
                            }

                            UINT linkFlags     = pChiLinkProperties->linkFlags;
                            UINT numLinkFlags  = 0;

                            if (0 != (linkFlags & LinkFlagDisableLateBinding))
                            {
                                pOutputPortInfo->portLink.linkProperties.LinkFlags[numLinkFlags] =
                                    LinkPropFlags::DisableLateBinding;
                                numLinkFlags++;
                                linkFlags = (linkFlags & (~LinkFlagDisableLateBinding));
                            }

                            if (0 != (linkFlags & LinkFlagDisableSelfShrinking))
                            {
                                pOutputPortInfo->portLink.linkProperties.LinkFlags[numLinkFlags] =
                                    LinkPropFlags::DisableSelfShrinking;
                                numLinkFlags++;
                                linkFlags = (linkFlags & (~LinkFlagDisableSelfShrinking));
                            }

                            if (0 != (linkFlags & LinkFlagSinkInplaceBuffer))
                            {
                                pOutputPortInfo->portLink.linkProperties.LinkFlags[numLinkFlags] =
                                    LinkPropFlags::SinkInplaceBuffer;
                                numLinkFlags++;
                                linkFlags = (linkFlags & (~LinkFlagSinkInplaceBuffer));
                            }

                            pOutputPortInfo->portLink.linkProperties.numLinkFlags = numLinkFlags;

                            pOutputPortInfo->portLink.linkBufferProperties.format =
                                static_cast<Format>(pChiLinkBufferProperties->bufferFormat);
                            pOutputPortInfo->portLink.linkBufferProperties.immediateAllocCount =
                                pChiLinkBufferProperties->bufferImmediateAllocCount;

                            pOutputPortInfo->portLink.linkBufferProperties.sizeBytes  = pChiLinkBufferProperties->bufferSize;
                            pOutputPortInfo->portLink.linkBufferProperties.queueDepth =
                                pChiLinkBufferProperties->bufferQueueDepth;
                            pOutputPortInfo->portLink.linkBufferProperties.heap       = pChiLinkBufferProperties->bufferHeap;

                            UINT memFlags    = pChiLinkBufferProperties->bufferFlags;
                            UINT numMemFlags = 0;

                            /// @todo (CAMX-1015) Optimize this
                            if (0 != (memFlags & BufferMemFlagHw))
                            {
                                pOutputPortInfo->portLink.linkBufferProperties.memFlags[numMemFlags] = BufferMemFlag::Hw;
                                numMemFlags++;
                                memFlags = (memFlags & (~BufferMemFlagHw));
                            }

                            if (0 != (memFlags & BufferMemFlagProtected))
                            {
                                pOutputPortInfo->portLink.linkBufferProperties.memFlags[numMemFlags] = BufferMemFlag::Protected;
                                numMemFlags++;
                                memFlags = (memFlags & (~BufferMemFlagProtected));
                            }

                            if (0 != (memFlags & BufferMemFlagCache))
                            {
                                pOutputPortInfo->portLink.linkBufferProperties.memFlags[numMemFlags] = BufferMemFlag::Cache;
                                numMemFlags++;
                                memFlags = (memFlags & (~BufferMemFlagCache));
                            }

                            if (0 != (memFlags & BufferMemFlagLockable))
                            {
                                pOutputPortInfo->portLink.linkBufferProperties.memFlags[numMemFlags] = BufferMemFlag::UMDAccess;
                                numMemFlags++;
                                memFlags = (memFlags & (~BufferMemFlagLockable));
                            }

                            CAMX_ASSERT(0 == memFlags);

                            pOutputPortInfo->portLink.linkBufferProperties.numMemFlags = numMemFlags;
                            break;
                        }
                    }
                }
            }

            /// @todo (CAMX-1797) Remove hardcoding to pipeline index 0 - pPerUsecase->pipelineInfo[0]
            for (UINT nodeIndex = 0; nodeIndex < pPipelineInfo->numNodes; nodeIndex++)
            {
                PerNodeInfo*  pPerNodeInfo  = &pPipelineInfo->pNodeInfo[nodeIndex];
                ChiNode*      pChiNode      = &pPipelineCreateDescriptor->pNodes[nodeIndex];
                ChiNodePorts* pChiPorts     = &pChiNode->nodeAllPorts;
                UINT32        numInputPorts = pChiPorts->numInputPorts;

                // Go through the input ports
                for (UINT inputPort = 0; inputPort < numInputPorts; inputPort++)
                {
                    ChiInputPortDescriptor* pChiInputPortDescriptor = &pChiPorts->pInputPorts[inputPort];
                    UINT                    inputPortId             = pChiInputPortDescriptor->portId;
                    InputPortInfo*          pInputPortInfo          = &pPerNodeInfo->inputPorts.pPortInfo[inputPort];

                    pInputPortInfo->portId = inputPortId;
                    pInputPortInfo->portSourceTypeId = pChiInputPortDescriptor->portSourceTypeId;

                    if (FALSE != pChiInputPortDescriptor->isInputStreamBuffer)
                    {
                        pInputPortInfo->flags.isSourceBuffer = TRUE;
                    }
                    else
                    {
                        BOOL matchFound = FALSE;

                        for (UINT link = 0; ((link < pPipelineCreateDescriptor->numLinks) && (FALSE == matchFound)); link++)
                        {
                            for (UINT dest = 0; (dest < pChiNodeLinkages[link].numDestNodes) && (FALSE == matchFound); dest++)
                            {
                                if ((inputPortId              == pChiNodeLinkages[link].pDestNodes[dest].nodePortId) &&
                                    (pPerNodeInfo->nodeId     == pChiNodeLinkages[link].pDestNodes[dest].nodeId) &&
                                    (pPerNodeInfo->instanceId == pChiNodeLinkages[link].pDestNodes[dest].nodeInstanceId))
                                {
                                    for (UINT index = 0; index < pPipelineInfo->numNodes; index++)
                                    {
                                        UINT32 nodeId         = pPipelineInfo->pNodeInfo[index].nodeId;
                                        UINT32 nodeInstanceId = pPipelineInfo->pNodeInfo[index].instanceId;

                                        if ((nodeId         == pChiNodeLinkages[link].srcNode.nodeId) &&
                                            (nodeInstanceId == pChiNodeLinkages[link].srcNode.nodeInstanceId))
                                        {
                                            pInputPortInfo->parentNodeIndex     = index;
                                            pInputPortInfo->parentOutputPortId  = pChiNodeLinkages[link].srcNode.nodePortId;

                                            matchFound = TRUE;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Node info mem calloc failed %p, num nodes %d",
                       pPipelineInfo->pNodeInfo,
                       pPipelineInfo->numNodes);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::SubmitRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::SubmitRequest(
    CHISession*         pSession,
    ChiPipelineRequest* pRequest)
{
    CamxResult result = CamxResultSuccess;

    // Validate requests
    for (UINT i = 0; i < pRequest->numRequests; i++)
    {
        if (0 != pRequest->pCaptureRequests[i].hPipelineHandle)
        {
            result = pSession->CheckValidInputRequest(&pRequest->pCaptureRequests[i]);

            if (CamxResultSuccess == result)
            {
                // Fall back to non optimized stream on before PCR logic if 0 or for non real time pipelines
                if ((FALSE == pSession->IsPipelineRealTime(pRequest->pCaptureRequests[i].hPipelineHandle)))
                {
                    result = pSession->StreamOn(pRequest->pCaptureRequests[i].hPipelineHandle);
                }
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Request batch index %u is not valid.", i);
                break;
            }
        }
    }

    // Submit requests to session.
    // For multi-camera, batch of requests for different pipeline are sent to session together.
    if (CamxResultSuccess == result)
    {
        result = pSession->ProcessCaptureRequest(pRequest);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "Request %llu failed when submitting requests.",
                pRequest->pCaptureRequests[0].frameNumber);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::ActivatePipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::ActivatePipeline(
    CHISession*         pChiSession,
    CHIPIPELINEHANDLE   hPipelineDescriptor)
{
    CAMX_ASSERT(NULL != pChiSession);
    CAMX_ASSERT(NULL != hPipelineDescriptor);

    CamxResult result = CamxResultSuccess;

    if (TRUE == pChiSession->UsingResourceManager(0))
    {
        ResourceID resourceId = static_cast<ResourceID>(ResourceType::RealtimePipeline);
        GetResourceManager()->CheckAndAcquireResource(resourceId, static_cast<VOID*>(hPipelineDescriptor), 0);
    }

    result = pChiSession->StreamOn(hPipelineDescriptor);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::DeactivatePipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::DeactivatePipeline(
    CHISession*                 pChiSession,
    CHIPIPELINEHANDLE           hPipelineDescriptor,
    CHIDEACTIVATEPIPELINEMODE   modeBitmask)
{
    CAMX_ASSERT(NULL != pChiSession);
    CAMX_ASSERT(NULL != hPipelineDescriptor);

    CamxResult result = CamxResultSuccess;

    result = pChiSession->StreamOff(hPipelineDescriptor, modeBitmask);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::DestroySession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiContext::DestroySession(
    CHISession* pChiSession)
{
    if (NULL != pChiSession)
    {
        pChiSession->Destroy();
        pChiSession = NULL;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid input arguments");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::FlushSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::FlushSession(
    CHISession*         pChiSession,
    CHISESSIONFLUSHINFO hSessionFlushInfo)
{
    CamxResult result;

    CAMX_ASSERT(pChiSession);

    CAMX_LOG_INFO(CamxLogGroupChi, "Processing Flush Request");
    if (NULL != hSessionFlushInfo.pPipelineFlushInfo)
    {
        result = pChiSession->Flush(&hSessionFlushInfo);
    }
    else
    {
        result = pChiSession->Flush();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::GetThreadManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ThreadManager* ChiContext::GetThreadManager()
{
    return m_pThreadManager;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::GetHwContext
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HwContext* ChiContext::GetHwContext() const
{
    return m_pHwContext;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::GetStaticSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const StaticSettings* ChiContext::GetStaticSettings() const
{
    return m_pHwEnvironment->GetSettingsManager()->GetStaticSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::DumpState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiContext::DumpState(
    INT fd)
{
    static const UINT32 Indent = 2;

    // Dump the context's DRQ
    CAMX_LOG_TO_FILE(fd, Indent, "+------------------------------------------------------------------+");
    CAMX_LOG_TO_FILE(fd, Indent, "+          ChiContext DRQ                                          +");
    CAMX_LOG_TO_FILE(fd, Indent, "+------------------------------------------------------------------+");
    m_pDeferredRequestQueue->DumpState(fd, Indent + 2);

    // Dump the threadpool state
    // Dump current MemSpy state
    // Dump command buffers...
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::QueryMetadataInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiContext::QueryMetadataInfo(
    CHISession*             pChiSession,
    const CHIPIPELINEHANDLE hPipelineDescriptor,
    const UINT32            maxPublishTagArraySize,
    UINT32*                 pPublishTagArray,
    UINT32*                 pPublishTagCount,
    UINT32*                 pPartialPublishTagCount,
    UINT32*                 pMaxNumMetaBuffers)
{
    CAMX_ASSERT(NULL != pChiSession);
    CAMX_ASSERT(NULL != hPipelineDescriptor);

    CamxResult result = pChiSession->QueryMetadataInfo(
        hPipelineDescriptor,
        maxPublishTagArraySize,
        pPublishTagArray,
        pPublishTagCount,
        pPartialPublishTagCount,
        pMaxNumMetaBuffers);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiContext::IsUBWCLossySupported
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiContext::IsUBWCLossySupported(
    const ChiStream*        pStream,
    const ChiStreamWrapper* pChiStreamWrapper)
{
    BOOL result                           = FALSE;
    BOOL is10BitFormat                    = ImageFormatUtils::Is10BitUBWCFormat(pChiStreamWrapper->GetInternalFormat());
    const PlatformStaticCaps* pStaticCaps = m_pHwEnvironment->GetPlatformStaticCaps();

    // Video stream
    if ((TRUE                       == Utils::IsBitMaskSet64(GetGrallocUsage(pStream), GrallocUsageHwVideoEncoder))    &&
        // Lossy supported on Video stream
        (TRUE                       == pStaticCaps->ubwcLossyVideoSupported)                                           &&
        // 10 bit limitation  for lossy format dimension limit
        (((TRUE                     == is10BitFormat)                                                                  &&
        (pStream->width             >= pStaticCaps->ubwcLossyVideo10BitMinWidth)                                       &&
        (pStream->height            >= pStaticCaps->ubwcLossyVideo10BitMinHeight))                                     ||
        ((pStream->width            >= pStaticCaps->ubwcLossyVideoMinWidth)                                            &&
        (pStream->height            >= pStaticCaps->ubwcLossyVideoMinHeight))))
    {
        result = TRUE;
    }
    else if (((TRUE                   == Utils::IsBitMaskSet64(GetGrallocUsage(pStream), GrallocUsageHwRender))        ||
             (TRUE                    == Utils::IsBitMaskSet64(GetGrallocUsage(pStream), GrallocUsageHwComposer)))     &&
             // Lossy supported on preview path
             (TRUE                    == pStaticCaps->ubwcLossyPreviewSupported)                                       &&
             //  10 bit format and supported dimension cap
             (((TRUE                  == is10BitFormat)                                                                &&
             (pStream->width          >= pStaticCaps->ubwcLossyPreview10BitMinWidth)                                   &&
             // 8 bit format and supported dimension cap
             (pStream->height         >= pStaticCaps->ubwcLossyPreview10BitMinHeight))                                 ||
             ((pStream->width         >= pStaticCaps->ubwcLossyPreviewMinWidth)                                        &&
             (pStream->height         >= pStaticCaps->ubwcLossyPreviewMinHeight))))
    {
        result = TRUE;
    }

    return result;
}
CAMX_NAMESPACE_END
