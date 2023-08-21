////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxsensornode.cpp
/// @brief SensorNode class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcsiphysubmodule.h"
#include "camxhwcontext.h"
#include "camxpdlibraryhandler.h"
#include "camxpipeline.h"
#include "camxsensorpickmode.h"
#include "camxtrace.h"
#include "camxvendortags.h"
#include "chi.h"
#include "chipdlibinterface.h"
#include "camxpdafconfig.h"
#include "camxpdafdata.h"
#include "camxsensornode.h"
#include "camxtuningdatamanager.h"
#include "camximagesensorutils.h"
#include "camxosutils.h"

CAMX_NAMESPACE_BEGIN

static const UINT   SensorMaxCmdBufferManagerCount = 30;    ///< Number of max command buffer managers
                                                            ///< Counted 26 CreateCmdBufferManager Called in this SensorNode
static const UINT32 MaxCommandBuffers              = 10;    ///< Number of command for CreateAndSubmitCommand
static const UINT   InitialConfigCmdCount          = 1;     ///< Number of command buffers in config command
static const UINT   I2CInfoCmdCount                = 1;     ///< Number of command buffers in I2C command

static const UINT BitWidth10                       = 10;    ///< Number of bits perpixel for Mipi10 format
static const UINT BitWidth8                        = 8;     ///< Number of bits perpixel for Mipi8 format


// @brief list of tags published by sensor node
static const UINT32 SensorOutputMetadataTags[] =
{
    SensorSensitivity,
    SensorTestPatternMode,
    SensorFrameDuration,
    SensorRollingShutterSkew,
    SensorExposureTime,
    SensorGreenSplit,
    SensorNoiseProfile,
    LensAperture,
    LensFilterDensity,
    LensFocalLength,
    LensFocusRange,
    LensState,
    LensOpticalStabilizationMode,
    FlashState,
    FlashMode,
    PropertyIDSensorExposureStartTime,
    PropertyIDSensorMetaData,
    PropertyIDSensorResolutionInfo,
    PropertyIDPostSensorGainId,
    PropertyIDSensorCurrentMode,
    PropertyIDSensorPDAFInfo,
    PropertyIDSensorProperties,
    PropertyIDUsecaseSensorISO100Gain,
    PropertyIDUsecaseCameraModuleInfo,
    PropertyIDUsecaseHWPDConfig,
    PropertyIDUsecaseLensInfo,
    PropertyIDUsecasePDLibInfo,
    PropertyIDSensorNumberOfLEDs,
};

// @brief list of vendor tags published by sensor node
static const struct NodeVendorTag SensorOutputVendorTags[] =
{
    { "com.qti.sensorbps"                       , "mode_index"              },
    { "com.qti.sensorbps"                       , "gain"                    },
    { "org.codeaurora.qcamera3.sensor_meta_data", "integration_information" },
    { "org.codeaurora.qcamera3.sensor_meta_data", "EEPROMInformation"       },
    { "org.codeaurora.qcamera3.sensor_meta_data", "current_mode"            },
    { "org.quic.camera.AECDataPublisherPresent" , "AECDataPublisherPresent" }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorSubDevicesCache::SensorSubDevicesCache
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SensorSubDevicesCache::SensorSubDevicesCache()
{
    m_pCacheLock = Mutex::Create("SubDeviceCache");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorSubDevicesCache::~SensorSubDevicesCache
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SensorSubDevicesCache::~SensorSubDevicesCache()
{
    m_pCacheLock->Destroy();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorSubDevicesCache::SetSubDeviceHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorSubDevicesCache::SetSubDeviceHandle(
    CSLHandle       hCSLSession,
    UINT32          cameraId,
    CSLDeviceHandle handle,
    SubDevice       type)
{
    CamxResult result = CamxResultSuccess;

    if ((MaxNumImageSensors > cameraId) && (MaxSubDevices > type))
    {
        m_pCacheLock->Lock();

        SubDeviceProperty* pSubDevice = &m_sensorSubDevices[cameraId].subDevices[type];

        if (0 == m_sensorSubDevices[cameraId].refCount)
        {
            CAMX_ASSERT(CSLInvalidHandle != handle);

            m_sensorSubDevices[cameraId].refCount++;
            m_sensorSubDevices[cameraId].hCSLSession[0] = hCSLSession;
            pSubDevice->isAcquired                      = TRUE;
            pSubDevice->hDevice                         = handle;

            CAMX_LOG_INFO(CamxLogGroupSensor,
                          "CameraId=%u: Cached subdevice=%u refCount=%u, CSLAddReference(0x%x) hDevice=0x%x",
                          cameraId, type, m_sensorSubDevices[cameraId].refCount, hCSLSession, handle);

            // This ref count will get cleared when we call CSLClose() in ReleaseAllSubDevices().
            // Anyone using ClearCache() directly (instead of ReleaseAllSubDevices() ) must ensure that they
            // call CSLClose() explicitly.
            CSLAddReference(hCSLSession);
        }
        else
        {
            CAMX_ASSERT(m_sensorSubDevices[cameraId].hCSLSession[0] == hCSLSession);

            if (m_sensorSubDevices[cameraId].hCSLSession[0] == hCSLSession)
            {
                if (FALSE == pSubDevice->isAcquired)
                {
                    m_sensorSubDevices[cameraId].refCount++;
                    pSubDevice->isAcquired = TRUE;
                    pSubDevice->hDevice    = handle;

                    CAMX_LOG_INFO(CamxLogGroupSensor, "CameraId=%u: Cached subdevice=%u refCount=%u",
                                  cameraId, type, m_sensorSubDevices[cameraId].refCount);
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupSensor, "CameraId=%u: Subdevice=%u already acquired", cameraId, type);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor,
                               "Potential leak! Subdevice CSLhandle=0x%x is not same as main CSLhandle0x%x",
                               hCSLSession, m_sensorSubDevices[cameraId].hCSLSession[0]);
                result = CamxResultEFailed;
            }
        }
        m_pCacheLock->Unlock();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid params: cameraId=%u Subdevice=%u", cameraId, type);
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorSubDevicesCache::MustRelease
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SensorSubDevicesCache::MustRelease(
    UINT32    cameraId,
    SubDevice type)
{
    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();
    CAMX_ASSERT(pStaticSettings != NULL);
    BOOL bReleaseDevice = FALSE;

    if (FALSE == pStaticSettings->enableSensorCaching)
    {
        bReleaseDevice = TRUE;
    }
    else
    {
        BOOL isCameraClosing = Utils::IsBitMaskSet(pStaticSettings->overrideCameraClose, (1 << cameraId));

        if ((TRUE == isCameraClosing) &&
            (TRUE == m_sensorSubDevices[cameraId].subDevices[type].isAcquired))
        {
            bReleaseDevice = TRUE;
        }
    }

    CAMX_LOG_INFO(CamxLogGroupSensor, "cameraId=%u subDevice=%u: bReleaseDevice=%u",
                  cameraId, type, bReleaseDevice);

    return bReleaseDevice;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorSubDevicesCache::ReleaseOneSubDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorSubDevicesCache::ReleaseOneSubDevice(
    UINT32    cameraId,
    SubDevice type)
{
    CamxResult result = CamxResultEFailed;

    if ((MaxNumImageSensors > cameraId) && (MaxSubDevices > type))
    {
        m_pCacheLock->Lock();

        if (0 < m_sensorSubDevices[cameraId].refCount)
        {
            SubDeviceProperty* pSubDevice = &m_sensorSubDevices[cameraId].subDevices[type];

            if (TRUE == pSubDevice->isAcquired)
            {
                CSLReleaseDevice(m_sensorSubDevices[cameraId].hCSLSession[0], pSubDevice->hDevice);
                pSubDevice->isAcquired = FALSE;
                m_sensorSubDevices[cameraId].refCount--;

                CAMX_LOG_INFO(CamxLogGroupSensor,
                              "Released CameraId=%u CSLSession=0x%x refCount=%u SubdeviceType=%u subDevHandle=0x%x",
                              cameraId, m_sensorSubDevices[cameraId].hCSLSession[0], m_sensorSubDevices[cameraId].refCount,
                              type, pSubDevice->hDevice);

                if (0 == m_sensorSubDevices[cameraId].refCount)
                {
                    CAMX_LOG_INFO(CamxLogGroupSensor, "CameraId=%u CSLClose(%p)",
                                  cameraId, m_sensorSubDevices[cameraId].hCSLSession[0]);
                    CSLClose(m_sensorSubDevices[cameraId].hCSLSession[0]);
                    HwEnvironment::GetInstance()->InitializeSensorHwDeviceCache(cameraId,
                        m_pHwContext, CSLInvalidHandle, 0, GetSensorSubDeviceHandles(cameraId), this);
                    if (CSLInvalidHandle != m_sensorSubDevices[cameraId].hCSLSession[1])
                    {
                        CSLClose(m_sensorSubDevices[cameraId].hCSLSession[1]);
                        HwEnvironment::GetInstance()->InitializeSensorHwDeviceCache(cameraId,
                            m_pHwContext, CSLInvalidHandle, 1, GetSensorSubDeviceHandles(cameraId), this);
                    }
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_INFO(CamxLogGroupSensor, "CameraId=%u Subdevice=%u handle %d already released!",
                              cameraId, type, pSubDevice->hDevice);
            }
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "CameraID=%u Subdevice=%u refCount already 0!", cameraId, type);
        }

        m_pCacheLock->Unlock();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid params: cameraId=%u Subdevice=%u", cameraId, type);
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorSubDevicesCache::ReleaseAllSubDevices
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorSubDevicesCache::ReleaseAllSubDevices(
    UINT32 cameraID)
{
    CamxResult result = CamxResultSuccess;

    if (MaxNumImageSensors > cameraID)
    {
        for (UINT32 subDevType = 0; subDevType < static_cast<UINT32>(MaxSubDevices); subDevType++)
        {
            ReleaseOneSubDevice(cameraID, static_cast<CamX::SubDevice>(subDevType));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorSubDevicesCache::SetSubDeviceIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorSubDevicesCache::SetSubDeviceIndex(
    UINT32    cameraId,
    INT32     deviceIndex,
    SubDevice type)
{
    CamxResult result = CamxResultSuccess;

    if ((MaxNumImageSensors > cameraId) && (MaxSubDevices > type))
    {
        m_pCacheLock->Lock();

        m_sensorSubDevices[cameraId].subDevices[type].deviceIndex = deviceIndex;

        m_pCacheLock->Unlock();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid params: cameraId=%u Subdevice=%u", cameraId, type);
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorSubDevicesCache::SetSubDeviceData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorSubDevicesCache::SetSubDeviceData(
    UINT32    cameraId,
    UINT32    data,
    SubDevice type)
{
    CamxResult result = CamxResultSuccess;

    if ((MaxNumImageSensors > cameraId) && (MaxSubDevices > type))
    {
        m_pCacheLock->Lock();

        m_sensorSubDevices[cameraId].subDevices[type].dataMask = data;

        m_pCacheLock->Unlock();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid params: cameraId=%u Subdevice=%u", cameraId, type);
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorSubDevicesCache::GetSubDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SubDeviceProperty SensorSubDevicesCache::GetSubDevice(
    UINT32    cameraId,
    SubDevice type
    ) const
{
    SubDeviceProperty subDeviceProperty;

    if ((MaxNumImageSensors > cameraId) && (MaxSubDevices > type))
    {
        m_pCacheLock->Lock();

        subDeviceProperty = m_sensorSubDevices[cameraId].subDevices[type];

        m_pCacheLock->Unlock();
    }
    else
    {
        Utils::Memset(&subDeviceProperty, 0, sizeof(SubDeviceProperty));
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid params: cameraId=%u Subdevice=%u", cameraId, type);
    }

    return subDeviceProperty;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::SensorNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SensorNode::SensorNode()
    : m_hSensorDevice(CSLInvalidHandle)
    , m_currentResolutionIndex(1)
    , m_currentSyncMode(NoSync)
    , m_hJobFamilyHandle(InvalidJobHandle)
    , m_sensorConfigStatus(SensorConfigurationStatus::SensorConfigurationStateUninitialized)
    , m_hJobFamilySubModuleOISHandle(InvalidJobHandle)
    , m_OISConfigStatus(SensorOISConfigurationStatus::SensorOISConfigurationStateUninitialized)
    , m_initialConfigPending(TRUE)
    , m_isFSModeEnabled(FALSE)
    , m_isMultiCameraUsecase(FALSE)
    , m_isMasterCamera(TRUE)
    , m_isPdafUpdated(FALSE)
    , m_isVendorAECDataAvailable(FALSE)
    , m_isWBGainUpdated(FALSE)
    , m_seamlessInSensorState(SeamlessInSensorState::None)
    , m_hJobFamilyReadHandle(InvalidJobHandle)
    , m_initialRequestId(FirstValidRequestId)
{
    m_pNodeName              = "Sensor";
    m_derivedNodeHandlesMetaDone = TRUE;
    m_pSensorSubDevicesCache = SensorSubDevicesCache::GetInstance();
    m_pPDAFType                  = PDAFType::PDTypeUnknown;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::~SensorNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SensorNode::~SensorNode()
{

    // Unregister job family first, we depend on the members we will be destroying to handle callbacks.
    if (InvalidJobHandle != m_hJobFamilyHandle)
    {
        const CHAR* pSensorName = m_pSensorModuleData->GetSensorDataObject()->GetSensorName();
        m_pThreadManager->UnregisterJobFamily(SensorThreadJobCallback, pSensorName, m_hJobFamilyHandle);
        m_hJobFamilyHandle = InvalidJobHandle;
    }

    // Unregister job family first, we depend on the members we will be destroying to handle callbacks.
    if (InvalidJobHandle != m_hJobFamilySubModuleOISHandle)
    {
        CHAR OISJobName[256];
        OsUtils::SNPrintF(OISJobName, sizeof(OISJobName), "%s_%s",
            m_pSensorModuleData->GetSensorDataObject()->GetSensorName(), "OIS");

        m_pThreadManager->UnregisterJobFamily(SensorThreadJobCallback, OISJobName, m_hJobFamilySubModuleOISHandle);
        m_hJobFamilySubModuleOISHandle = InvalidJobHandle;
    }

    // Unregister job family first, we depend on the members we will be destroying to handle callbacks.
    if (InvalidJobHandle != m_hJobFamilyReadHandle)
    {
        CHAR readName[256];
        OsUtils::SNPrintF(readName, sizeof(readName), "%s_%s",
            m_pSensorModuleData->GetSensorDataObject()->GetSensorName(), "readName");

        m_pThreadManager->UnregisterJobFamily(SensorThreadJobCallback, readName, m_hJobFamilyReadHandle);
        m_hJobFamilyReadHandle = InvalidJobHandle;
    }

    // These resources are managed manually, hence recycled manually.
    if ((NULL != m_pConfigPacketManager) && (NULL != m_pConfigPacket))
    {
        m_pConfigPacketManager->Recycle(m_pConfigPacket);
        m_pConfigPacket = NULL;
    }

    if ((NULL != m_pConfigCmdBufferManager) && (NULL != m_pConfigCmdBuffer))
    {
        m_pConfigCmdBufferManager->Recycle(m_pConfigCmdBuffer);
        m_pConfigCmdBuffer = NULL;
    }

    if (NULL != m_pExposureInfo)
    {
        CAMX_FREE(m_pExposureInfo);
        m_pExposureInfo = NULL;
    }

    if (NULL != m_pRegSettings)
    {
        CAMX_FREE(m_pRegSettings);
        m_pRegSettings = NULL;
    }

    if (NULL != m_pPDAFSettings)
    {
        CAMX_FREE(m_pPDAFSettings);
        m_pPDAFSettings = NULL;
    }

    if (NULL != m_pWBGainSettings)
    {
        CAMX_FREE(m_pWBGainSettings);
        m_pWBGainSettings = NULL;
    }

    if (NULL != m_pLTCRatioSettings)
    {
        CAMX_FREE(m_pLTCRatioSettings);
        m_pLTCRatioSettings = NULL;
    }

    if (NULL != m_pExposureRegAddressInfo)
    {
        CAMX_FREE(m_pExposureRegAddressInfo);
        m_pExposureRegAddressInfo = NULL;
    }

    if (NULL != m_pSensorParamQueue)
    {
        CAMX_FREE(m_pSensorParamQueue);
        m_pSensorParamQueue = NULL;
    }

    // Free all sub modules
    if (NULL != m_pActuator)
    {
        m_pActuator->Destroy();
        m_pActuator = NULL;
    }

    if (NULL != m_pOis)
    {
        m_pOis->Destroy();
        m_pOis = NULL;
    }

    if (NULL != m_pCSIPHY)
    {
        m_pCSIPHY->Destroy();
        m_pCSIPHY = NULL;
    }

    if (NULL != m_pFlash)
    {
        m_pFlash->Destroy();
        m_pFlash = NULL;
    }

    if (CSLInvalidHandle != m_hSensorDevice)
    {
        if ((TRUE == SensorSubDevicesCache::GetInstance()->MustRelease(m_cameraId, SensorHandle)) ||
            (TRUE == IsFullRecoveryFlagSet()))
        {
            SensorSubDevicesCache::GetInstance()->ReleaseOneSubDevice(m_cameraId, SensorHandle);
            SetDeviceAcquired(FALSE);
        }
    }

    const StaticSettings* pStaticSettings = GetStaticSettings();

    // Destroy all the created objects.
    if ((NULL != m_pPDLib) && (NULL != pStaticSettings) && (NULL != m_pPDLib->PDLibDestroy))
    {
        PDLibDestroyParamList destroyParamList = { 0 };
        PDLibDestroyParam     destroyParams[PDLibDestroyParamTypeCount] = {};

        UINT* pOverrideCameraClose                                                  =
            (UINT *)&pStaticSettings->overridePDLibClose;
        destroyParams[PDLibDestroyParamTypeCameraCloseIndicator].destroyParamType   = PDLibDestroyParamTypeCameraCloseIndicator;
        destroyParams[PDLibDestroyParamTypeCameraCloseIndicator].pParam             = static_cast<VOID*>(pOverrideCameraClose);
        destroyParams[PDLibDestroyParamTypeCameraCloseIndicator].sizeOfParam        = sizeof(UINT);

        StatsCameraInfo cameraInfo;
        cameraInfo.cameraId = m_cameraId;
        destroyParams[PDLibDestroyParamTypeCameraInfo].destroyParamType   = PDLibDestroyParamTypeCameraInfo;
        destroyParams[PDLibDestroyParamTypeCameraInfo].pParam             = static_cast<VOID*>(&cameraInfo);
        destroyParams[PDLibDestroyParamTypeCameraInfo].sizeOfParam        = sizeof(StatsCameraInfo);

        destroyParamList.paramCount = PDLibDestroyParamTypeCount;
        destroyParamList.pParamList = &destroyParams[0];
        m_pPDLib->PDLibDestroy(m_pPDLib, &destroyParamList);
        m_pPDLib = NULL;
    }

    if (NULL != m_pPDLibHandler)
    {
        CAMX_DELETE m_pPDLibHandler;
        m_pPDLibHandler = NULL;
    }

    if (NULL != m_signalSensorInit.pMutex)
    {
        m_signalSensorInit.pMutex->Destroy();
        m_signalSensorInit.pMutex = NULL;
    }

    if (NULL != m_signalSensorInit.pWaitCondition)
    {
        m_signalSensorInit.pWaitCondition->Destroy();
        m_signalSensorInit.pWaitCondition = NULL;
    }

    if (NULL != m_signalSensorConfig.pMutex)
    {
        m_signalSensorConfig.pMutex->Destroy();
        m_signalSensorConfig.pMutex = NULL;
    }

    if (NULL != m_signalSensorConfig.pWaitCondition)
    {
        m_signalSensorConfig.pWaitCondition->Destroy();
        m_signalSensorConfig.pWaitCondition = NULL;
    }

    if (NULL != m_signalSensorSubModules.pMutex)
    {
        m_signalSensorSubModules.pMutex->Destroy();
        m_signalSensorSubModules.pMutex = NULL;
    }

    if (NULL != m_signalSensorSubModules.pWaitCondition)
    {
        m_signalSensorSubModules.pWaitCondition->Destroy();
        m_signalSensorSubModules.pWaitCondition = NULL;
    }

    if (NULL != m_signalOISInit.pMutex)
    {
        m_signalOISInit.pMutex->Destroy();
        m_signalOISInit.pMutex = NULL;
    }

    if (NULL != m_signalOISInit.pWaitCondition)
    {
        m_signalOISInit.pWaitCondition->Destroy();
        m_signalOISInit.pWaitCondition = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SensorNode* SensorNode::Create(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    SensorNode* pSensorNode = CAMX_NEW SensorNode;

    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    if (NULL != pSensorNode)
    {
        if (NULL != pCreateInputData->pPDLibCallbacks)
        {
            pSensorNode->m_pPDCallback = pCreateInputData->pPDLibCallbacks;
        }
        else
        {
            pSensorNode->m_pPDCallback = NULL;
            CAMX_LOG_VERBOSE(CamxLogGroupAF, "pPDLibCallbacks is NULL.");
        }
    }

    return pSensorNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SensorNode::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::ProcessingNodeInitialize(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{

    CAMX_UNREFERENCED_PARAM(pCreateInputData);

    CamxResult result = CamxResultSuccess;

    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupSensor, SCOPEEventSensorNodeProcessingNodeInitialize);

    pCreateOutputData->pNodeName = m_pNodeName;

    // register to update config done for initial PCR
    pCreateOutputData->createFlags.willNotifyConfigDone = TRUE;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::CreateSubModules
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::CreateSubModules()
{
    CamxResult result              = CamxResultSuccess;
    SubDeviceProperty CSIPHYDevice = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, CSIPHYHandle);
    result = CSIPHYSubmodule::Create(m_pHwContext, GetCSLSession(), &m_pCSIPHY, CSIPHYDevice.deviceIndex, m_cameraId,
                                     NodeIdentifierString());

    if (result != CamxResultSuccess)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "CSIPhy Submodule creation failed");
        return result;
    }

    if (NULL == m_pCSIPHY)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "CSIPhy Pointer is NULL");
        return CamxResultEInvalidPointer;
    }

    AddCSLDeviceHandle(m_pCSIPHY->GetDeviceHandle());
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::PostPipelineCreate()
{
    CamxResult       result           = CamxResultEFailed;
    SensorPostJob*   pSensorPostJob   = CAMX_NEW SensorPostJob;
    const DelayInfo* pDelayInfo       = GetSensorDataObject()->GetSensorPipelineDelay(DelayInfoType::EXPOSURECONTROL);

    if (NULL != pSensorPostJob)
    {
        pSensorPostJob->pSensor          = this;
        pSensorPostJob->sensorJobCommand = SensorPostJobCommand::ConfigureSensor;
        pSensorPostJob->pData            = NULL;
        VOID* pData[]                    = { pSensorPostJob, NULL };
        result                           = m_pThreadManager->PostJob(m_hJobFamilyHandle,
                                                                     NULL,
                                                                     &pData[0],
                                                                     FALSE,
                                                                     FALSE);
    }

    // Setting default values for tuning file struture
    ChiTuningModeParameter  chiTuningModeParameter = { 0 };
    UINT32                  metaTag                = 0;
    chiTuningModeParameter.TuningMode[0].mode      = ChiModeType::Default;
    chiTuningModeParameter.TuningMode[1].mode      = ChiModeType::Sensor;
    chiTuningModeParameter.TuningMode[2].mode      = ChiModeType::Usecase;
    chiTuningModeParameter.TuningMode[3].mode      = ChiModeType::Feature1;
    chiTuningModeParameter.TuningMode[4].mode      = ChiModeType::Feature2;
    chiTuningModeParameter.TuningMode[5].mode      = ChiModeType::Scene;
    chiTuningModeParameter.TuningMode[6].mode      = ChiModeType::Effect;

    // Read tuning mode from usecase pool to pass usecase info to PDLib
    // Pass default values if vendor tag not available
    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.tuning.mode",
                                                      "TuningMode",
                                                      &metaTag);

    UINT              GetProps[]             = { metaTag | UsecaseMetadataSectionMask };
    static const UINT GetPropsLength         = CAMX_ARRAY_SIZE(GetProps);
    VOID*             pData[GetPropsLength]  = { 0 };
    UINT64            offset[GetPropsLength] = { 0 };

    if (CamxResultSuccess == result)
    {
        GetDataList(GetProps, pData, offset, GetPropsLength);
        if (NULL != pData[0])
        {
            chiTuningModeParameter = *reinterpret_cast<ChiTuningModeParameter*>(pData[0]);
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupSensor, "pData[0] NULL for tuning mode vendor tag");
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupSensor, "Query tuning mode vendor tag failed");
    }

    // Assign sensor mode from m_currentResolutionIndex
    chiTuningModeParameter.TuningMode[1].subMode.value = static_cast<UINT16>(m_currentResolutionIndex);

    if ((CamxResultSuccess == result) && (FALSE == GetStaticSettings()->disablePDAF) && (TRUE == m_isPDAFEnabled))
    {
        // No Need to check for failed PDAF init since this is an optional feature
        PDAFData*               pPDAFData               = m_pSensorModuleData->GetPDAFDataObj();
        VOID*                   pTuningManager          = GetTuningDataManager()->GetChromatix();
        StatsCameraInfo         cameraInfo              = {};
        PDHWEnableConditions*   pPDHWEnableConditions   = NULL;

        // This property is just for PDLib input, it considers all conditions except tuning. Tuning will be consider later when
        // getting config from PDLib.
        const UINT tags[]              = { PropertyIDUsecasePDLibInputPDHWEnableConditions };
        const UINT length              = CAMX_ARRAY_SIZE(tags);
        VOID*      pPropData[length]   = { 0 };
        UINT64     offsets[length]     = { 0 };

        GetDataList(tags, pPropData, offsets, length);
        if (NULL != pPropData[0])
        {
            pPDHWEnableConditions = reinterpret_cast<PDHWEnableConditions*>(pPropData[0]);
        }

        result = GetInitialCameraInfo(&cameraInfo);
        if ((CamxResultSuccess== result) && (NULL != pPDAFData))
        {
            PDLibDataBufferInfo CAMIFT3DataPattern;
            Utils::Memset(&CAMIFT3DataPattern, 0, sizeof(PDLibDataBufferInfo));
            GetPDAFT3CAMIFPattern(&CAMIFT3DataPattern, pPDAFData);

            PDLibBufferFormat bufferFormatLCR = PDLibBufferFormatMipi10;
            GetRDIBufferFormat(&bufferFormatLCR);

            pPDAFData->PDAFInit(m_pPDLib,
                                m_currentResolutionIndex,
                                m_pSensorModuleData->GetActuatorDataObject(),
                                m_pSensorModuleData->GetSensorDataObject(),
                                m_pOTPData,
                                &cameraInfo,
                                pTuningManager,
                                &CAMIFT3DataPattern,
                                bufferFormatLCR,
                                GetHwContext(),
                                pPDHWEnableConditions,
                                &m_sensorCrop,
                                &chiTuningModeParameter);
        }
    }

    IsFSModeEnabled(&m_isFSModeEnabled);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Fastshutter feature : %s ",
                           m_isFSModeEnabled == TRUE ? "enabled" : "disabled");

    if (pDelayInfo->maxPipeline > MaxSensorPipelineDelay)
    {
        m_maxSensorPipelineDelay = MaxSensorPipelineDelay;
    }
    else
    {
        m_maxSensorPipelineDelay = pDelayInfo->maxPipeline;
    }

    m_pSensorParamQueue = static_cast<SensorParam*>(CAMX_CALLOC(sizeof(SensorParam) * m_maxSensorPipelineDelay));
    if (NULL == m_pSensorParamQueue)
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to malloc memory for param queue.");
    }

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_INFO(CamxLogGroupSensor, "PostPipelineCreate on Sensor successful!");
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "PostPipelineCreate on Sensor failed: %s", Utils::CamxResultToString(result));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::PublishISO100GainInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SensorNode::PublishISO100GainInformation()
{

    FLOAT        ISO100Gain         = GetSensorDataObject()->GetISO100Gain(m_cameraId);
    const UINT   SensorOutputTags[] = { PropertyIDUsecaseSensorISO100Gain };
    const VOID*  pOutputData[1]     = { 0 };
    UINT         pDataCount[1]      = { 0 };
    pDataCount[0]                   = sizeof(FLOAT);
    pOutputData[0]                  = &ISO100Gain;
    WriteDataList(SensorOutputTags, pOutputData, pDataCount, 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::PublishSensorProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SensorNode::PublishSensorProperties()
{
    SensorProperties  sensorProperties = { 0 };
    LensInfo   lensInfo                = { 0 };

    UpdateLensInformation(&lensInfo);
    sensorProperties.sensingMethod          = GetSensorDataObject()->GetSensorSensingMethod();
    sensorProperties.focalLengthIn35mm      = GetSensorDataObject()->GetSensorCropFactor() * lensInfo.focalLength;
    UINT              SensorPropertiesTag[] = { PropertyIDSensorProperties};
    static const UINT TagSize               = CAMX_ARRAY_SIZE(SensorPropertiesTag);
    const VOID*       pData[TagSize]        = { 0 };
    UINT              pDataCount[TagSize]   = { 0 };
    UINT              dataIndex             = 0;
    pData[dataIndex]                        = &sensorProperties;
    pDataCount[dataIndex]                   = sizeof(SensorProperties);
    WriteDataList(SensorPropertiesTag, pData, pDataCount, TagSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::PopulatePDAFInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SensorNode::PopulatePDAFInformation()
{
    PDAFData*                    pPDAFData         = m_pSensorModuleData->GetPDAFDataObj();
    const PDAFConfigurationData* pPDAFConfigData   = pPDAFData->GetPDAFConfigData();
    UINT32                       index             = 0;
    CamxResult                   result            = CamxResultSuccess;
    PDAFModeInformation*         pPDAFModeInfo     = NULL;
    UINT32                       PDAFModeIdx       = 0;

    result = pPDAFData->GetCurrentPDAFModeIndex(m_currentResolutionIndex, &PDAFModeIdx);

    if (CamxResultSuccess == result)
    {
        result = pPDAFData->GetPDAFModeInformation(PDAFModeIdx, &pPDAFModeInfo);
    }

    if ((NULL != pPDAFModeInfo) && (CamxResultSuccess == result))
    {
        CAMX_LOG_INFO(CamxLogGroupSensor,
                    "pdtype %d, PDAFname: %s",
                    pPDAFModeInfo->PDType,
                    pPDAFConfigData->PDCommonInfo.PDAFName);

        /* Type1 PDAF sensors do not have native pattern info or block pattern info.
           TYPE3 PDAF sensors do not have block pattern info. */
        if ((PDAFType::PDType2   == pPDAFModeInfo->PDType) ||
            (PDAFType::PDType2PD == pPDAFModeInfo->PDType))
        {
            m_sensorPDAFInfo.PDAFBufferFormat = static_cast<PDBufferFormat>(
                pPDAFModeInfo->PDBufferBlockPatternInfo.PDBufferFormat);

            m_sensorPDAFInfo.PDAFNativeBufferFormat = static_cast<PDBufferFormat>(
                pPDAFModeInfo->PDSensorNativePatternInfo.PDNativeBufferFormat);
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "PDAFBufferFormat %d, PDAFNativeBufferFormat %d",
                          m_sensorPDAFInfo.PDAFBufferFormat,
                          m_sensorPDAFInfo.PDAFNativeBufferFormat);
        }

        /* For Type1 PDAF sensors, output buffer format is indicated in native buffer format */
        if (PDAFType::PDType1 == pPDAFModeInfo->PDType)
        {
            if (TRUE == pPDAFModeInfo->PDSensorOutputFormatExists)
            {
                m_sensorPDAFInfo.PDAFNativeBufferFormat = static_cast<PDBufferFormat>(
                    pPDAFModeInfo->PDSensorOutputFormat);
            }
            else
            {
                m_sensorPDAFInfo.PDAFNativeBufferFormat = PDBufferFormat::UNPACKED16;
            }
        }

        m_sensorPDAFInfo.PDAFSensorType = pPDAFModeInfo->PDType;

        if ((TRUE == pPDAFModeInfo->PDSensorNativePatternInfoExists) &&
           (TRUE == pPDAFModeInfo->PDSensorNativePatternInfo.PDBlockPatternExists))
        {
            m_sensorPDAFInfo.PDAFBlockHeight   =
                pPDAFModeInfo->PDSensorNativePatternInfo.PDBlockPattern.PDBlockDimensions.height;
            m_sensorPDAFInfo.PDAFBlockWidth    =
                pPDAFModeInfo->PDSensorNativePatternInfo.PDBlockPattern.PDBlockDimensions.width;
            m_sensorPDAFInfo.PDAFGlobaloffsetX =
                pPDAFModeInfo->PDSensorNativePatternInfo.PDBlockPattern.PDOffsetHorizontal;
            m_sensorPDAFInfo.PDAFGlobaloffsetY =
                pPDAFModeInfo->PDSensorNativePatternInfo.PDBlockPattern.PDOffsetVertical;
            m_sensorPDAFInfo.PDAFPixelCount    =
                pPDAFModeInfo->PDSensorNativePatternInfo.PDBlockPattern.PDPixelCount;

            for (index = 0; index < m_sensorPDAFInfo.PDAFPixelCount; index++)
            {
                m_sensorPDAFInfo.PDAFPixelCoords[index].PDXCoordinate =
                    pPDAFModeInfo->PDSensorNativePatternInfo.PDBlockPattern.PDPixelCoordinates[index].PDXCoordinate;

                m_sensorPDAFInfo.PDAFPixelCoords[index].PDYCoordinate =
                    pPDAFModeInfo->PDSensorNativePatternInfo.PDBlockPattern.PDPixelCoordinates[index].PDYCoordinate;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::PublishPDAFInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SensorNode::PublishPDAFInformation()
{
    PDAFData* pPDAFData = m_pSensorModuleData->GetPDAFDataObj();

    if (TRUE == m_isPDAFEnabled)
    {
        // Publish SensorPDAFInfo to result pool for IFE/BPS modules
        const UINT  SensorOutputTags[]= { PropertyIDSensorPDAFInfo };
        const VOID* pOutputData[1]    = { 0 };
        UINT        pDataCount[1]     = { 0 };
        pDataCount[0]                 = sizeof(SensorPDAFInfo);
        pOutputData[0]                = &m_sensorPDAFInfo;
        WriteDataList(SensorOutputTags, pOutputData, pDataCount, 1);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::PublishCameraModuleInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SensorNode::PublishCameraModuleInformation()
{
    CamxResult                       result           = CamxResultSuccess;
    CameraConfigurationInformation   cameraModuleInfo = { 0 };
    HwCameraInfo                     cameraInfo;

    result = HwEnvironment::GetInstance()->GetCameraInfo(m_cameraId, &cameraInfo);

    if (CamxResultSuccess == result)
    {
        cameraModuleInfo.mountAngle       = cameraInfo.mountAngle;
        cameraModuleInfo.imageOrientation = cameraInfo.imageOrientation;
        result = m_pSensorModuleData->GetCameraPosition(&cameraModuleInfo.position);
    }

    if (CamxResultSuccess == result)
    {
        const UINT SensorOutputTags[] = { PropertyIDUsecaseCameraModuleInfo };
        const VOID* pOutputData[1]    = { 0 };
        UINT pDataCount[1]            = { 0 };
        pDataCount[0]                 = sizeof(CameraConfigurationInformation);
        pOutputData[0]                = &cameraModuleInfo;

        WriteDataList(SensorOutputTags, pOutputData, pDataCount, 1);
    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Camera Module Information could not be published");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::PublishLensInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::PublishLensInformation()
{
    CamxResult result   = CamxResultSuccess;
    LensInfo   lensInfo = { 0 };

    result = UpdateLensInformation(&lensInfo);
    CAMX_ASSERT(CamxResultSuccess == result);

    const UINT SensorOutputTags[] = { PropertyIDUsecaseLensInfo };
    const VOID* pOutputData[1] = { 0 };
    UINT pDataCount[1] = { 0 };
    pDataCount[0] = sizeof(LensInfo);
    pOutputData[0] = &lensInfo;

    WriteDataList(SensorOutputTags, pOutputData, pDataCount, 1);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::PrepareStreamOn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::PrepareStreamOn()
{
    CamxResult  result              = CamxResultSuccess;
    BOOL        isEarlyPCREnabled   = FALSE;

    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupSensor, SCOPEEventSensorNodePrepareStreamOn);

    m_signalSensorConfig.pMutex->Lock();
    if ((SensorConfigurationStatus::SensorConfigurationComplete != m_sensorConfigStatus) &&
        (SensorConfigurationStatus::SensorConfigurationFailed != m_sensorConfigStatus))
    {
        m_signalSensorConfig.pWaitCondition->Wait(m_signalSensorConfig.pMutex->GetNativeHandle());
    }
    m_signalSensorConfig.pMutex->Unlock();

    if (SensorConfigurationStatus::SensorConfigurationComplete == m_sensorConfigStatus)
    {
        IsEarlyPCREnabled(&isEarlyPCREnabled);
        if (FALSE == isEarlyPCREnabled)
        {
            BOOL statsNodeEnabled = IsStatsNodeEnabled();

            if ((TRUE == statsNodeEnabled) || (TRUE == m_isVendorAECDataAvailable))
            {
                result = UpdateStartupExposureSettings(statsNodeEnabled);
                if (CamxResultSuccess == result)
                {
                    UINT regSettingIdx = 0;
                    UINT AECCmdSize = GetSensorDataObject()->GetI2CCmdSize(I2CRegSettingType::AEC,
                                                                           m_pRegSettings,
                                                                           0,
                                                                           &regSettingIdx);
                    CAMX_ASSERT(0 == (AECCmdSize % sizeof(UINT32)));
                    result = CreateAndSubmitCommand(AECCmdSize,
                                                    I2CRegSettingType::AEC,
                                                    CSLPacketOpcodesSensorConfig,
                                                    m_currentResolutionIndex,
                                                    regSettingIdx);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to submit startup exposure settings");
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to update startup exposure settings");
                }
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupSensor, "Stats Node not available/enabled");
            }
        }
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Sensor not configured, cannot setup sensor");
    }

    CAMX_LOG_CONFIG(CamxLogGroupSensor, "sensorConfigStatus:%d isEarlyPCREnabled:%d",
        m_sensorConfigStatus, isEarlyPCREnabled);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::OnStreamOn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::OnStreamOn()
{
    CamxResult result = CamxResultSuccess;

    if (m_initialRequestId < FirstValidRequestId)
    {
        m_initialRequestId = FirstValidRequestId;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "m_initialRequestId=%lld", m_initialRequestId);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::OnStreamOff
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::OnStreamOff(
    CHIDEACTIVATEPIPELINEMODE modeBitmask)
{
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Prepare stream off ");

    if (NULL != m_pFlash)
    {
        m_pFlash->OnStreamOff(modeBitmask);
    }

    if (NULL != m_pActuator)
    {
        m_pActuator->OnStreamOff(modeBitmask);
    }

    m_initialConfigPending = TRUE;

    m_initialRequestId = GetPipeline()->GetLastSubmittedRequestId() + 1;
    Utils::Memset(m_pSensorParamQueue,   0, sizeof(SensorParam) * m_maxSensorPipelineDelay);
    Utils::Memset(&m_appliedSensorParam, 0, sizeof(SensorParam));
    Utils::Memset(&m_sensorParam,        0, sizeof(SensorParam));

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "m_initialRequestId=%lld", m_initialRequestId);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::PublishPerFrameSensorMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SensorNode::PublishPerFrameSensorMetaData(
    UINT64 requestId)
{
    CamxResult             result                 = CamxResultSuccess;
    SensorMetaData         metaData               = {0};
    SensorResolutionInfo   resolutionInfo         = {0};
    ResolutionData*        pResolutionData        =
        &(GetSensorDataObject()->GetResolutionInfo())->resolutionData[m_currentResolutionIndex];
    FLOAT                  ISPDigitalGain         = 0.0f;
    FLOAT                  sensorAnalogRealGain   = 0.0f;
    FLOAT                  sensorDigitalRealGain  = 0.0f;
    FLOAT                  lensFilterDensity      = 0.0f;
    FLOAT                  lensAperture           = 0.0f;
    FLOAT                  lensFocalLength        = 0.0f;
    RangeFLOAT             lensFocusRange         = {0};
    FLOAT                  ISO100Gain             = 0.0f;
    VOID*                  pRegControlInfo        = NULL;
    static const UINT      NumChannels            = 4;
    ColorFilterArrangement colorFilterArrangement = GetSensorDataObject()->GetColorFilterPattern(m_currentResolutionIndex);
    SensorSeamlessType     currentSeamlessType    = SensorSeamlessType::None;

    if ((NULL == m_pExposureInfo))
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid m_pExposureInfo: %p", m_pExposureInfo);
        return;
    }

    ISPDigitalGain        = GetSensorDataObject()->GetISPDigitalGain(m_pExposureInfo);
    sensorAnalogRealGain  = GetSensorDataObject()->GetAnalogRealGain(m_pExposureInfo);
    sensorDigitalRealGain = GetSensorDataObject()->GetDigitalRealGain(m_pExposureInfo);

    static const UINT PropertiesISO100Gain[] =
    {
        PropertyIDUsecaseSensorISO100Gain
    };
    VOID*  pISO100Gain[1]           = { 0 };
    UINT   length                   = CAMX_ARRAY_SIZE(PropertiesISO100Gain);
    UINT64 propertyDataOffset[1]    = { 0 };

    GetDataList(PropertiesISO100Gain, pISO100Gain, propertyDataOffset, length);
    if (NULL != pISO100Gain[0])
    {
        ISO100Gain  = *reinterpret_cast<FLOAT*>(pISO100Gain[0]);
    }

    for (UINT32 index = 0; index < pResolutionData->streamInfo.streamConfigurationCount; index++)
    {
        if (StreamType::IMAGE == pResolutionData->streamInfo.streamConfiguration[index].type)
        {
            metaData.width  = pResolutionData->streamInfo.streamConfiguration[index].frameDimension.width -
                              pResolutionData->cropInfo.left -
                              pResolutionData->cropInfo.right;
            metaData.height = pResolutionData->streamInfo.streamConfiguration[index].frameDimension.height -
                              pResolutionData->cropInfo.top -
                              pResolutionData->cropInfo.bottom;
        }
    }

    if (NULL != m_sensorParam.pRegControlData)
    {
        pRegControlInfo = reinterpret_cast<VOID *>(m_sensorParam.pRegControlData->sensorControl.registerControl);
    }

    metaData.exposureTime       = GetSensorDataObject()->GetExposureTime(m_sensorParam.currentExposure, pRegControlInfo);
    metaData.sensorGain         = sensorAnalogRealGain * sensorDigitalRealGain;

    metaData.frameLengthLines   = m_sensorParam.currentFrameLengthLines;
    metaData.sensitivity        = GetSensorDataObject()->GainToSensitivity(metaData.sensorGain, ISO100Gain, pRegControlInfo);

    metaData.filterArrangement  = static_cast<UINT32>(colorFilterArrangement);
    metaData.rollingShutterSkew = GetSensorDataObject()->GetRollingShutterSkew(metaData.height,
                                                                               m_currentResolutionIndex);
    metaData.shortExposureTime  = GetSensorDataObject()->GetExposureTime(m_sensorParam.currentShortExposure, pRegControlInfo);
    metaData.shortSensorGain    = m_sensorParam.currentShortGain;

    /// @todo (CAMX-2766) - Support for test pattern modes.
    metaData.testPatternMode    = static_cast<INT32>(TestPatternMode::OFF);

    metaData.frameDuration      = GetSensorDataObject()->GetFrameReadoutTime(m_sensorParam.currentFrameLengthLines,
                                                                             m_currentResolutionIndex);

    // Sanity check: Frame Duration should not be smaller than duration for max fps for the resolution.
    UINT16 maxFps               = static_cast<UINT16>(GetSensorDataObject()->GetMaxFPS(m_currentResolutionIndex));
    if (metaData.frameDuration < NanoSecondsPerSecond/maxFps)
    {
        metaData.frameDuration = static_cast<UINT64>(NanoSecondsPerSecond/maxFps);
    }

    // Channels * 2 to sensor amplification (S) and sensor readout noise (O) on each channel
    DOUBLE noiseProfile[NumChannels * 2] = { 0 };
    GetSensorDataObject()->GetNoiseProfile(noiseProfile, colorFilterArrangement, metaData.sensitivity, ISO100Gain);

    CAMX_LOG_INFO(CamxLogGroupSensor,
                     "Sensor[%d]: ReqId: %llu resID %d rollingShutterSkew: %llu , FilterArrangement: %d, "
                     "frameDuration: %llu, exposureTime:%llu, sensitivity: %d, sensorGain[analog:%f, digital:%f total: %f],"
                     "ispDigitalGain: %f",
                     m_cameraId,
                     requestId,
                     m_currentResolutionIndex,
                     metaData.rollingShutterSkew,
                     metaData.filterArrangement,
                     metaData.frameDuration,
                     metaData.exposureTime,
                     metaData.sensitivity,
                     sensorAnalogRealGain,
                     sensorDigitalRealGain,
                     metaData.sensorGain,
                     ISPDigitalGain);

    // Due to there's no need to publish seamless in-sensor's lineLengthPixelClk and frameLengthLine to meta,
    // so we pass default value (SensorSeamlessType::None) to function GetLineLengthPixelClk and GetFrameLengthLines here.
    resolutionInfo.vtPixelClk          = GetSensorDataObject()->GetVTPixelClk(m_currentResolutionIndex);
    resolutionInfo.lineLengthPixelClk  = GetSensorDataObject()->GetLineLengthPixelClk(m_currentResolutionIndex,
                                                                                      currentSeamlessType);
    resolutionInfo.frameRate           = static_cast<FLOAT>(GetSensorDataObject()->GetMaxFPS(m_currentResolutionIndex));
    resolutionInfo.frameLengthLine     = GetSensorDataObject()->GetFrameLengthLines(m_currentResolutionIndex,
                                                                                    currentSeamlessType);
    resolutionInfo.lineReadoutTime     = GetSensorDataObject()->GetLineReadoutTime(m_currentResolutionIndex,
                                                                                   currentSeamlessType);
    UINT SensorMetaDataTags[] =
    {
        PropertyIDSensorMetaData,
        PropertyIDSensorResolutionInfo,
        PropertyIDPostSensorGainId,
        PropertyIDSensorCurrentMode,
        SensorSensitivity,
        SensorTestPatternMode,
        SensorFrameDuration,
        SensorRollingShutterSkew,
        SensorExposureTime,
        SensorGreenSplit,
        SensorNoiseProfile,
        0,
        0,
    };

    static const UINT NumSensorMetaDataTags             = CAMX_ARRAY_SIZE(SensorMetaDataTags);
    const VOID*       pData[NumSensorMetaDataTags]      = { 0 };
    UINT              pDataCount[NumSensorMetaDataTags] = { 0 };
    UINT              dataIndex                         = 0;
    UINT              metaTag                           = 0;

    pData[dataIndex]        = &metaData;
    pDataCount[dataIndex++] = sizeof(SensorMetaData);

    pData[dataIndex]        = &resolutionInfo;
    pDataCount[dataIndex++] = sizeof(resolutionInfo);

    pData[dataIndex]        = &ISPDigitalGain;
    pDataCount[dataIndex++] = sizeof(FLOAT);

    pData[dataIndex]        = &m_currentResolutionIndex;
    pDataCount[dataIndex++] = sizeof(m_currentResolutionIndex);

    pData[dataIndex]        = &(metaData.sensitivity);
    pDataCount[dataIndex++] = 1;

    pData[dataIndex]        = &(metaData.testPatternMode);
    pDataCount[dataIndex++] = 1;

    pData[dataIndex]        = &(metaData.frameDuration);
    pDataCount[dataIndex++] = 1;

    pData[dataIndex]        = &(metaData.rollingShutterSkew);
    pDataCount[dataIndex++] = 1;

    pData[dataIndex]        = &(metaData.exposureTime);
    pDataCount[dataIndex++] = 1;

    pData[dataIndex]        = &(m_opt_wb_grgb);
    pDataCount[dataIndex++] = 1;

    pData[dataIndex]        = &(noiseProfile);
    pDataCount[dataIndex++] = CAMX_ARRAY_SIZE(noiseProfile);

    result = VendorTagManager::QueryVendorTagLocation("com.qti.sensorbps", "mode_index", &metaTag);
    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to get vendor tag: mode_index");

    SensorMetaDataTags[dataIndex] = metaTag;
    pData[dataIndex]              = &m_currentResolutionIndex;
    pDataCount[dataIndex++]       = 1;

    result = VendorTagManager::QueryVendorTagLocation("com.qti.sensorbps", "gain", &metaTag);
    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to get vendor tag: gain");

    SensorMetaDataTags[dataIndex] = metaTag;
    pData[dataIndex]              = &ISPDigitalGain;
    pDataCount[dataIndex++]       = 1;

    WriteDataList(SensorMetaDataTags, pData, pDataCount, NumSensorMetaDataTags);
    const LensInformation* pLensInformation = m_pSensorModuleData->GetLensInfo();
    if (NULL == pLensInformation)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Lens Information is NULL");
        return;
    }

    // LensAperture metadata is represented as float, not double
    lensAperture       = static_cast<FLOAT>(pLensInformation->fNumber);

    // LensFocalLength metadata is represented as float, not double
    lensFocalLength    = static_cast<FLOAT>(pLensInformation->focalLength);

    lensFocusRange.max = static_cast<FLOAT>(pLensInformation->maxFocusDistance);
    lensFocusRange.min = static_cast<FLOAT>(pLensInformation->minFocusDistance);

    UINT LensInfoMetaDataTags[] =
    {
        LensAperture,
        LensFilterDensity,
        LensFocalLength,
        LensFocusRange,
    };

    static const UINT NumLensInfoMetaDataTags = CAMX_ARRAY_SIZE(LensInfoMetaDataTags);
    const VOID*       pLensData[NumLensInfoMetaDataTags] = { 0 };
    UINT              pLensDataCount[NumLensInfoMetaDataTags] = { 0 };
    dataIndex = 0;

    pLensData[dataIndex] = &lensAperture;
    pLensDataCount[dataIndex++] = 1;

    pLensData[dataIndex]        = &lensFilterDensity;
    pLensDataCount[dataIndex++] = 1;

    pLensData[dataIndex]        = &lensFocalLength;
    pLensDataCount[dataIndex++] = 1;

    pLensData[dataIndex]        = &(lensFocusRange);
    pLensDataCount[dataIndex++] = 2;

    WriteDataList(LensInfoMetaDataTags, pLensData, pLensDataCount, NumLensInfoMetaDataTags);

    // If Fixed focus sensor
    // Update the lens state as stationary perframe
    if (NULL == m_pSensorModuleData->GetActuatorDataObject())
    {
        static const UINT MetadataLens[] =
        {
            LensState
        };

        LensStateValues lensState      = LensStateStationary;
        const void * pLensStateData[1] = { &lensState};
        UINT pLensStateDataCount[1]    = {1};

        WriteDataList(MetadataLens, pLensStateData, pLensStateDataCount, 1);
    }

    LensOpticalStabilizationModeValues oisEnable = LensOpticalStabilizationModeOff;
    static const UINT MetadataOIS[] =
    {
        InputLensOpticalStabilizationMode
    };

    VOID* pOisGetData[1]            = { 0 };
    UINT64 propertyDataOisOffset[1] = { 0 };

    GetDataList(MetadataOIS, pOisGetData, propertyDataOisOffset, 1);
    if (NULL != pOisGetData[0])
    {
        oisEnable = *(static_cast<LensOpticalStabilizationModeValues*>(pOisGetData[0]));
    }

    static const UINT OISResultMetaDataTag[] =
    {
        LensOpticalStabilizationMode
    };
    const VOID* pOisData[1]      = { 0 };
    UINT        pOisDataCount[1] = { 0 };

    pOisData[0]      = &oisEnable;
    pOisDataCount[0] = 1;

    WriteDataList(OISResultMetaDataTag, pOisData, pOisDataCount, 1);

    if (TRUE == pResolutionData->integrationInfoExists)
    {
        UINT        IntegrationInfoTags[] = { 0 };
        const VOID* pIntegrationData[1]   = { 0 };
        UINT        pIntegrationDataCount[1]         = { 0 };

        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.sensor_meta_data",
                                                          "integration_information",
                                                          &metaTag);
        CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to get vendor tag: integration_information");

        IntegrationInfoTags[0]      = metaTag;
        pIntegrationData[0]         = &pResolutionData->integrationInfo;
        pIntegrationDataCount[0]    = sizeof(IntegrationInformation) / sizeof(UINT32);
        WriteDataList(IntegrationInfoTags, pIntegrationData, pIntegrationDataCount, 1);
    }

    PublishPDAFInformation();

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::UpdateDebugExposureSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SensorNode::UpdateDebugExposureSettings(
    BOOL         applyShortExposure,
    BOOL         applyMiddleExposure,
    SensorParam* pExposureParameters,
    UINT64       requestID)
{
    const StaticSettings* pSettings       = m_pHwContext->GetStaticSettings();
    UINT                  exposureTime[3] = {pSettings->sensorExposureTime, pSettings->sensorShortExposureTime,
    pSettings->sensorMiddleExposureTime};
    FLOAT                 gain[3]         = {pSettings->gain, pSettings->shortGain, pSettings->middleGain};

    if (0 != exposureTime[0])
    {
        pExposureParameters->currentExposure = exposureTime[0];
    }

    if ((0 != exposureTime[1]) && (TRUE == applyShortExposure))
    {
        pExposureParameters->currentShortExposure = exposureTime[1];
    }

    if ((0 != exposureTime[2]) && (TRUE == applyMiddleExposure))
    {
        pExposureParameters->currentMiddleExposure = exposureTime[2];
    }

    if (0.0 != gain[0])
    {
        pExposureParameters->currentGain = gain[0];
    }

    if ((0 != gain[1]) && (TRUE == applyShortExposure))
    {
        pExposureParameters->currentShortGain = gain[1];
    }

    if ((0 != gain[2]) && (TRUE == applyMiddleExposure))
    {
        pExposureParameters->currentMiddleGain = gain[2];
    }

    CAMX_LOG_CONFIG(CamxLogGroupSensor,
                    "DEBUG:[sensor:%d] reqID:%llu, [gain, exposureTime]:[long:%ld, %f], [mid:%ld, %f],"
                    "[short:%ld, %f]: applyShort:%d, applyMiddle:%d",
                    m_cameraId,
                    requestID,
                    pExposureParameters->currentGain,
                    pExposureParameters->currentExposure,
                    pExposureParameters->currentMiddleGain,
                    pExposureParameters->currentMiddleExposure,
                    pExposureParameters->currentShortGain,
                    pExposureParameters->currentShortExposure,
                    applyShortExposure,
                    applyMiddleExposure);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::UpdateSensorExposure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SensorNode::UpdateSensorExposure(
    BOOL              applyShortExposure,
    BOOL              applyMiddleExposure,
    SensorParam*      pExposureParameters,
    AECFrameControl*  pAECOutput,
    UINT64            requestID)
{
    if ((NULL != pAECOutput) && (NULL != pExposureParameters))
    {
        if (TRUE == applyShortExposure && TRUE == applyMiddleExposure)
        {
            pExposureParameters->currentGain           = pAECOutput->exposureInfo[ExposureIndexLong].linearGain;
            pExposureParameters->currentExposure       = pAECOutput->exposureInfo[ExposureIndexLong].exposureTime;
            pExposureParameters->currentMiddleGain     = pAECOutput->exposureInfo[ExposureIndexSafe].linearGain;
            pExposureParameters->currentMiddleExposure = pAECOutput->exposureInfo[ExposureIndexSafe].exposureTime;
            pExposureParameters->currentShortGain      = pAECOutput->exposureInfo[ExposureIndexShort].linearGain;
            pExposureParameters->currentShortExposure  = pAECOutput->exposureInfo[ExposureIndexShort].exposureTime;
        }
        else if (TRUE == applyShortExposure)
        {
            pExposureParameters->currentGain           = pAECOutput->exposureInfo[ExposureIndexLong].linearGain;
            pExposureParameters->currentExposure       = pAECOutput->exposureInfo[ExposureIndexLong].exposureTime;
            pExposureParameters->currentMiddleGain     = 0;
            pExposureParameters->currentMiddleExposure = 0;
            pExposureParameters->currentShortGain      = pAECOutput->exposureInfo[ExposureIndexShort].linearGain;
            pExposureParameters->currentShortExposure  = pAECOutput->exposureInfo[ExposureIndexShort].exposureTime;
        }
        else
        {
            pExposureParameters->currentGain           = pAECOutput->exposureInfo[ExposureIndexShort].linearGain;
            pExposureParameters->currentExposure       = pAECOutput->exposureInfo[ExposureIndexShort].exposureTime;
            pExposureParameters->currentMiddleGain     = 0;
            pExposureParameters->currentMiddleExposure = 0;
            pExposureParameters->currentShortGain      = 0;
            pExposureParameters->currentShortExposure  = 0;
        }

        // Check for debug exposure settings and if its enabled get the values and use as exposure settings
        if (TRUE == m_pHwContext->GetStaticSettings()->enableDebugSensorExposure)
        {
            CAMX_LOG_CONFIG(CamxLogGroupSensor,
                            "AEC:[sensor:%d] reqID:%llu, [gain, exposureTime]:[long:%ld, %f], [mid:%ld, %f], [short:%ld, %f]:"
                            " applyShort:%d, applyMiddle:%d",
                            m_cameraId,
                            requestID,
                            pExposureParameters->currentGain,
                            pExposureParameters->currentExposure,
                            pExposureParameters->currentMiddleGain,
                            pExposureParameters->currentMiddleExposure,
                            pExposureParameters->currentShortGain,
                            pExposureParameters->currentShortExposure,
                            applyShortExposure,
                            applyMiddleExposure);
            UpdateDebugExposureSettings(applyShortExposure, applyMiddleExposure, pExposureParameters, requestID);
        }

        pExposureParameters->currentFrameLengthLines = GetSensorDataObject()->GetFrameLengthLines(m_currentResolutionIndex,
                                                                                GetSensorSeamlessType(m_seamlessInSensorState));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "pExposureParameters or pAECOutput is pointing to NULL");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::NotifyCSLMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::NotifyCSLMessage(
    CSLMessage* pCSLMessage)
{
    CamxResult result = CamxResultSuccess;

    if (CSLMessageType::CSLMessageTypeFrame == pCSLMessage->type)
    {
        UINT32 getProps[]                         = {SensorExposureTime};
        UINT64 offsets[CAMX_ARRAY_SIZE(getProps)] = {0};
        VOID*  pData[CAMX_ARRAY_SIZE(getProps)]   = {0};
        UINT64 exposureStartTime                  = 0;

        result = GetDataList(getProps, pData, offsets, CAMX_ARRAY_SIZE(getProps));
        if ((CamxResultSuccess == result) && (NULL != pData[0]))
        {
            exposureStartTime = GetSensorDataObject()->GetExposureStartTime(pCSLMessage->message.frameMessage.timestamp,
                                                                            *reinterpret_cast<UINT64*>(pData[0]),
                                                                            m_currentResolutionIndex);

            UINT32      writeProps[]                            = {PropertyIDSensorExposureStartTime};
            UINT32      count[CAMX_ARRAY_SIZE(writeProps)]      = {sizeof(UINT64)};
            const VOID* pWriteData[CAMX_ARRAY_SIZE(writeProps)] = {&exposureStartTime};

            result = WriteDataList(writeProps, pWriteData, count, CAMX_ARRAY_SIZE(writeProps));

            CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                         "requestID: %llu, sofTimestamp: %lld, exposure start time: %lld",
                         pCSLMessage->message.frameMessage.requestID,
                         pCSLMessage->message.frameMessage.timestamp,
                         exposureStartTime);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::SetDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::SetDependencies(
    NodeProcessRequestData* pNodeProcessRequestData,
    BOOL                    hasExplicitDependencies,
    UINT64                  requestIdOffsetFromLastFlush)
{
    CamxResult      result      = CamxResultSuccess;
    DependencyUnit* pDependency = &pNodeProcessRequestData->dependencyInfo[0];
    UINT64          requestId   = pNodeProcessRequestData->pCaptureRequest->requestId;
    UINT            count       = 0;

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Sensor: ProcessRequest: Setting dependency for Req#%llu", requestId);

    // Set a dependency on the completion of the previous ExecuteProcessRequest() call
    // so that we can guarantee serialization of all ExecuteProcessRequest() calls for this node.
    // Needed since the ExecuteProcessRequest() implementation is not reentrant.
    // Remove when requirement CAMX-3030 is implemented.
    // Skip setting dependency for first request
    if (FirstValidRequestId < requestIdOffsetFromLastFlush)
    {
        pDependency->dependencyFlags.hasPropertyDependency = TRUE;
        pDependency->propertyDependency.properties[count]  = GetNodeCompleteProperty();
        // Always point to the previous request. Should NOT be tied to the pipeline delay!
        pDependency->propertyDependency.offsets[count]     = 1;
        pDependency->processSequenceId                     = 1;
        count++;
    }

    if ((TRUE == hasExplicitDependencies) && (TRUE == IsStatsNodeEnabled()))
    {
        if (TRUE == IsTagPresentInPublishList(PropertyIDAECFrameControl))
        {
            pDependency->propertyDependency.properties[count++] = PropertyIDAECFrameControl;
            pDependency->processSequenceId                      = 1;
            pDependency->propertyDependency.count               = count;
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "pipeline cannot publish property: %08x ", PropertyIDAECFrameControl);
        }

        if ((TRUE == IsTagPresentInPublishList(PropertyIDAFFrameControl)) && (PDAFType::PDType1 == m_pPDAFType))
        {
            pDependency->propertyDependency.properties[count++] = PropertyIDAFFrameControl;
            pDependency->processSequenceId                      = 1;
            pDependency->propertyDependency.count               = count;
        }
        else if (PDAFType::PDType1 == m_pPDAFType)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "pipeline cannot publish property: %08x ", PropertyIDAFFrameControl);
        }

        if (FALSE == IsTagPresentInPublishList(PropertyIDAFFrameInfo))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "pipeline cannot publish property: %08x ", PropertyIDAFFrameInfo);
            if (NULL != m_pActuator)
            {
                // Since actuator is a realtime device need to send NOP packet as AF property is not published
                m_pActuator->SendNOP(requestId);
            }
        }

        if (TRUE == IsTagPresentInPublishList(PropertyIDAWBFrameControl))
        {
            BOOL isHWRemosaic       = FALSE;
            BOOL is3ExposureHDRMode = GetSensorDataObject()->Is3ExposureHDRMode(m_currentResolutionIndex);
            BOOL isQuadCFAMode      = GetSensorDataObject()->IsQuadCFAMode(m_currentResolutionIndex, &isHWRemosaic);
            if ((TRUE == is3ExposureHDRMode) || ((TRUE == isQuadCFAMode) && (TRUE == isHWRemosaic)))
            {
                /* Request frame N AWB framcontrol dependency to fill WB gain back to sensor every frame.
                   Optionally, change to request N-1 if AWB is heavy and delay long, image qualtiy may has
                   chance to become worse because of not apply correct AWB gain to sensor for corresponding
                   frame*/
                pDependency->propertyDependency.offsets[count]                          = 0;
                pDependency->propertyDependency.properties[count++]                     = PropertyIDAWBFrameControl;
                pDependency->processSequenceId                                          = 1;
                pDependency->propertyDependency.count                                   = count;
            }
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "pipeline cannot publish property: %08x ", PropertyIDAWBFrameControl);
        }

        UINT32          regControlMetaTag = 0;
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.sensor_register_control",
                                                          "sensor_register_control",
                                                          &regControlMetaTag);
        if (0 != regControlMetaTag && (TRUE == IsTagPresentInPublishList(regControlMetaTag)))
        {
            pDependency->propertyDependency.properties[count++]                 = regControlMetaTag;
            pDependency->processSequenceId                                      = 1;
            pDependency->propertyDependency.count                               = count;
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "pipeline cannot publish property: %08x ", regControlMetaTag);
        }

        if ((TRUE == GetStaticSettings()->enableSensorFpsMatch) && (TRUE == m_isMultiCameraUsecase) &&
            (TRUE != m_isMasterCamera))
        {
            MultiRequestSyncData*  pMultiRequestData = pNodeProcessRequestData->pCaptureRequest->pMultiRequestData;
            INT64                  lpmRequestDelta   = requestId - pMultiRequestData->currReq.requestID[m_peerPipelineId];
            BOOL                   negate            = static_cast<BOOL>(lpmRequestDelta <= 0);

            if (TRUE == IsTagPresentInPublishList(PropertyIDSensorMetaData))
            {
                pDependency->propertyDependency.properties[count]  = PropertyIDSensorMetaData;
                pDependency->propertyDependency.negate[count]      = negate;
                pDependency->propertyDependency.pipelineIds[count] = m_peerPipelineId;
                pDependency->propertyDependency.offsets[count++]   = static_cast<UINT64>(abs(lpmRequestDelta));
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "pipeline cannot publish property: %08x ", PropertyIDSensorMetaData);
            }

            if (TRUE == IsTagPresentInPublishList(PropertyIDSensorResolutionInfo))
            {
                pDependency->propertyDependency.properties[count]  = PropertyIDSensorResolutionInfo;
                pDependency->propertyDependency.negate[count]      = negate;
                pDependency->propertyDependency.pipelineIds[count] = m_peerPipelineId;
                pDependency->propertyDependency.offsets[count++]   = static_cast<UINT64>(abs(lpmRequestDelta));
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "pipeline cannot publish property: %08x ", PropertyIDSensorResolutionInfo);
            }

            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Sensor: Setting dependency for fps match, req#%llu", requestId);
        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Skip Setting Dependency for ReqID:%llu", requestId);
    }

    if (count > 0)
    {
        pDependency->propertyDependency.count              = count;
        pDependency->dependencyFlags.hasPropertyDependency = TRUE;
        pNodeProcessRequestData->numDependencyLists        = 1;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::ExecuteProcessRequest(
    ExecuteProcessRequestData* pExecuteProcessRequestData)
{
    CamxResult              result              = CamxResultSuccess;
    TuningDataManager*      pTuningManager      = GetTuningDataManager();
    const StaticSettings*   pStaticSettings     = GetStaticSettings();

    if ((NULL != pStaticSettings) && (NULL != pExecuteProcessRequestData) &&
        (NULL != pExecuteProcessRequestData->pNodeProcessRequestData) &&
        (NULL != pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest))
    {

        CAMX_ENTRYEXIT_SCOPE_ID(CamxLogGroupSensor, SCOPEEventSensorNodeExecuteProcessRequest,
            pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId);

        UINT32          regControlMetaTag       = 0;

        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.sensor_register_control",
                                                          "sensor_register_control",
                                                          &regControlMetaTag);

        UINT64                requestId           =
            pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId;
        DependencyUnit*       pDependency         = &pExecuteProcessRequestData->pNodeProcessRequestData->dependencyInfo[0];
        BOOL                  applyShortExposure  = FALSE;
        BOOL                  applyMiddleExposure = FALSE;

        if ((FirstValidRequestId == requestId) && (0 == pExecuteProcessRequestData->pNodeProcessRequestData->processSequenceId))
        {
            result = InitializeMultiCameraInfo();
        }

        if ((CamxResultSuccess == result) && (TRUE == m_isMultiCameraUsecase) &&
            (TRUE == pStaticSettings->enableSensorFpsMatch))
        {
            /// Need to update the multi camera info per frame as we don't know which two cameras are running
            /// on multi camera usecase.
            result = UpdateMultiCameraInfo(pExecuteProcessRequestData);
        }

        if ((CamxResultSuccess == result) && (TRUE == pStaticSettings->perFrameSensorMode))
        {
            // Update the sensor mode per frame
            static const UINT PropertiesForSensorInputs[] =
            {
                PropertyIDSensorCurrentMode,
            };

            static const UINT Length                    = CAMX_ARRAY_SIZE(PropertiesForSensorInputs);
            VOID*             pSensorData[Length]       = { 0 };
            UINT64            sensorDataOffset[Length]  = { 0 };

            GetDataList(PropertiesForSensorInputs, pSensorData, sensorDataOffset, Length);

            if (NULL != pSensorData[0])
            {
                m_currentResolutionIndex = *(static_cast<UINT32*>(pSensorData[0]));
            }
            else
            {
                result = CamxResultEInvalidPointer;
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Current Sensor Mode pointer is NULL");
            }
        }

        applyShortExposure  = GetSensorDataObject()->IsHDRMode(m_currentResolutionIndex);
        applyMiddleExposure = GetSensorDataObject()->Is3ExposureHDRMode(m_currentResolutionIndex);

        BOOL             bShouldSetDependency           = FALSE;
        AECFlashInfoType flashType                      = FlashInfoTypeOff;
        UINT32           LEDCurrents[LEDSettingCount]   = { 0 };
        UINT64           requestIdOffsetFromLastFlush   = GetRequestIdOffsetFromLastFlush(requestId);
        BOOL             isFSCaptureRequest             = IsFSCaptureRequest();
        BOOL             hasExplicitDependencies        = TRUE;

        if ((CamxResultSuccess == result) && (0 == pExecuteProcessRequestData->pNodeProcessRequestData->processSequenceId))
        {
            // Publish flash metadata ASAP if flash is unavailable to meet the early metadata.
            if (NULL == m_pFlash)
            {
                UINT                    numberOfLED              = 0;
                static FlashStateValues flashState               = FlashStateUnavailable;
                static FlashModeValues  flashMode                = FlashModeOff;
                static UINT             flashTags[]              = { FlashState, FlashMode, PropertyIDSensorNumberOfLEDs};
                static const UINT       NumFlashTags             = CAMX_ARRAY_SIZE(flashTags);
                const VOID*             pData[NumFlashTags]      = { &flashState, &flashMode , &numberOfLED};
                UINT                    pDataCount[NumFlashTags] = { 1, 1, sizeof(UINT)};

                WriteDataList(flashTags, pData, pDataCount, NumFlashTags);
            }

            if (TRUE == isFSCaptureRequest)
            {
                hasExplicitDependencies = FALSE;
            }

            if ((FirstValidRequestId < requestIdOffsetFromLastFlush) ||
                ((TRUE == IsStatsNodeEnabled()) && (TRUE == hasExplicitDependencies)))
            {
                bShouldSetDependency = TRUE;
            }
        }

        if (TRUE == bShouldSetDependency)
        {
            result = SetDependencies(pExecuteProcessRequestData->pNodeProcessRequestData,
                                     hasExplicitDependencies,
                                     requestIdOffsetFromLastFlush);
        }
        else if (CamxResultSuccess == result)
        {
            if ((FALSE == IsStatsNodeEnabled()) || (TRUE == isFSCaptureRequest))
            {
                if (NULL != m_pActuator)
                {
                    // Since actuator is a realtime device need to send NOP packet when stats node is not enabled
                    m_pActuator->SendNOP(requestId);
                }

                FLOAT               ISO100Gain          = GetSensorDataObject()->GetISO100Gain(m_cameraId);
                SensorSeamlessType  currentSeamlessType = SensorSeamlessType::None;
                SensorParam updateParam     = { 0 };
                updateParam.pRegControlData = NULL;
                updateParam.isFSModeCapture = FALSE;

                if (TRUE == m_isVendorAECDataAvailable)
                {
                    UINT32      AECDataMetaTag = 0;
                    ChiAECData* pChiAECData;

                    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.AECData",
                                                                      "AECData",
                                                                      &AECDataMetaTag);
                    UINT AECData[] =
                    {
                        AECDataMetaTag | UsecaseMetadataSectionMask,
                    };

                    const UINT length                = CAMX_ARRAY_SIZE(AECData);
                    VOID*      pAECData[length]      = {0};
                    UINT64     pAECDataCount[length] = {0};

                    GetDataList(AECData, pAECData, pAECDataCount, length);
                    pChiAECData = reinterpret_cast<ChiAECData*>(pAECData[0]);
                    if (NULL != pChiAECData)
                    {
                        updateParam.currentExposure      = pChiAECData->exposureTime;
                        updateParam.currentShortExposure = pChiAECData->exposureTime;
                        updateParam.currentGain          =
                            GetSensorDataObject()->SensitivityToGain(pChiAECData->sensitivity, ISO100Gain);
                        updateParam.currentShortGain     = updateParam.currentGain;
                    }
                }
                else if (TRUE == isFSCaptureRequest)
                {
                    updateParam.currentExposure      = m_sensorParam.previousExposure;
                    updateParam.currentShortExposure = m_sensorParam.previousShortExposure;
                    updateParam.currentGain          = m_sensorParam.previousGain;
                    updateParam.currentShortGain     = m_sensorParam.previousShortGain;
                    updateParam.isFSModeCapture      = TRUE;

                    // Seamless in-sensor won't be triggered in this mode.
                    // So we pass default value (SensorSeamlessType::None) into function GetFrameLengthLines.
                    updateParam.currentFrameLengthLines = GetSensorDataObject()->GetFrameLengthLines(m_currentResolutionIndex,
                                                                                                     currentSeamlessType);
                }
                else
                {
                    UINT InputAECData[] =
                    {
                        InputSensorExposureTime,                        // 0
                        InputSensorSensitivity,                         // 1
                        InputSensorFrameDuration,                       // 2
                    };

                    static const UINT   InputLength                  = CAMX_ARRAY_SIZE(InputAECData);
                    VOID*               pInputData[InputLength]      = { 0 };
                    UINT64              pInputDataCount[InputLength] = { 0 };

                    GetDataList(InputAECData, pInputData, pInputDataCount, InputLength);
                    updateParam.currentExposure      = ((NULL != pInputData[0]) ? *(static_cast<UINT64*>(pInputData[0])) : 1);
                    updateParam.currentShortExposure = updateParam.currentExposure;
                    INT32 sensorSensitivity          = ((NULL != pInputData[1]) ? *(static_cast<INT32*>(pInputData[1])) : 100);
                    updateParam.currentGain          =  GetSensorDataObject()->SensitivityToGain(sensorSensitivity, ISO100Gain);
                    updateParam.currentShortGain     = updateParam.currentGain;

                    // Apply HAL override of frameLengthLines (aka frame duration) if provided
                    UINT64 inputFrameDuration = (NULL != pInputData[2] ? *(static_cast<UINT64*>(pInputData[2])) : 0);
                    if ((0 != inputFrameDuration) && (FALSE == isFSCaptureRequest))
                    {
                        // Seamless in-sensor won't be triggered in this mode
                        updateParam.currentFrameLengthLines = GetSensorDataObject()->ExposureToLineCount(
                            inputFrameDuration,
                            m_currentResolutionIndex,
                            currentSeamlessType);
                        updateParam.currentFrameLengthLines = Utils::MaxUINT32(updateParam.currentFrameLengthLines,
                            GetSensorDataObject()->GetFrameLengthLines(m_currentResolutionIndex, currentSeamlessType));
                        updateParam.currentFrameLengthLines = Utils::MinUINT32(updateParam.currentFrameLengthLines,
                            GetSensorDataObject()->GetMaxFrameLengthLines(m_currentResolutionIndex, m_cameraId));

                        CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                                "Hal Override: frameLengthLines %u (min/max %u %u)",
                                updateParam.currentFrameLengthLines,
                                GetSensorDataObject()->GetFrameLengthLines(m_currentResolutionIndex, currentSeamlessType),
                                GetSensorDataObject()->GetMaxFrameLengthLines(m_currentResolutionIndex, m_cameraId));
                    }
                    else
                    {
                        updateParam.currentFrameLengthLines =
                                    GetSensorDataObject()->GetFrameLengthLines(m_currentResolutionIndex, currentSeamlessType);
                    }
                }

                PrepareSensorUpdate(&updateParam, pExecuteProcessRequestData->pNodeProcessRequestData);

                CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                                "Sensor: Update Exposure from input pool: Gain:%f ExpTime:%ld, "
                                "short Gain:%f, short ExpTime:%ld FLL:%u",
                                updateParam.currentGain,
                                updateParam.currentExposure,
                                updateParam.currentShortGain,
                                updateParam.currentShortExposure,
                                updateParam.currentFrameLengthLines);

                m_sensorParam.currentGain             = updateParam.currentGain;
                m_sensorParam.currentExposure         = updateParam.currentExposure;
                m_sensorParam.currentLineCount        = updateParam.currentLineCount;
                m_sensorParam.currentFrameLengthLines = updateParam.currentFrameLengthLines;
                m_sensorParam.currentShortGain        = updateParam.currentShortGain;
                m_sensorParam.currentShortExposure    = updateParam.currentShortExposure;
                m_sensorParam.currentShortLineCount   = updateParam.currentShortLineCount;

                result = ApplySensorUpdate(requestId);

                if (CamxResultSuccess == result)
                {
                    m_sensorParam.previousGain             = m_sensorParam.currentGain;
                    m_sensorParam.previousExposure         = m_sensorParam.currentExposure;
                    m_sensorParam.previousLineCount        = m_sensorParam.currentLineCount;
                    m_sensorParam.previousFrameLengthLines = m_sensorParam.currentFrameLengthLines;
                    m_sensorParam.previousMiddleGain       = m_sensorParam.currentMiddleGain;
                    m_sensorParam.previousMiddleExposure   = m_sensorParam.currentMiddleExposure;
                    m_sensorParam.previousMiddleLineCount  = m_sensorParam.currentMiddleLineCount;
                    m_sensorParam.previousShortGain        = m_sensorParam.currentShortGain;
                    m_sensorParam.previousShortExposure    = m_sensorParam.currentShortExposure;
                    m_sensorParam.previousShortLineCount   = m_sensorParam.currentShortLineCount;

                    m_appliedSensorParam.previousGain             = m_appliedSensorParam.currentGain;
                    m_appliedSensorParam.previousExposure         = m_appliedSensorParam.currentExposure;
                    m_appliedSensorParam.previousLineCount        = m_appliedSensorParam.currentLineCount;
                    m_appliedSensorParam.previousFrameLengthLines = m_appliedSensorParam.currentFrameLengthLines;
                    m_appliedSensorParam.previousMiddleGain       = m_appliedSensorParam.currentMiddleGain;
                    m_appliedSensorParam.previousMiddleExposure   = m_appliedSensorParam.currentMiddleExposure;
                    m_appliedSensorParam.previousMiddleLineCount  = m_appliedSensorParam.currentMiddleLineCount;
                    m_appliedSensorParam.previousShortGain        = m_appliedSensorParam.currentShortGain;
                    m_appliedSensorParam.previousShortExposure    = m_appliedSensorParam.currentShortExposure;
                    m_appliedSensorParam.previousShortLineCount   = m_appliedSensorParam.currentShortLineCount;

                    m_pSensorParamQueue[requestId % m_maxSensorPipelineDelay] = m_sensorParam;
                }
            }
            else
            {
                UINT32 sensorLTCRatioTag = 0;
                result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.statsconfigs",
                                                                  "HistNodeLTCRatioIndex",
                                                                  &sensorLTCRatioTag);

                UINT32 seamlessInsensorStateTag = 0;
                result = VendorTagManager::QueryVendorTagLocation("com.qti.insensor_control",
                                                                  "seamless_insensor_state",
                                                                  &seamlessInsensorStateTag);

                // We previously queued to DRQ with a prop, it must be available now
                static const UINT Properties[]               =
                {
                    PropertyIDAECFrameControl,
                    regControlMetaTag,
                    PropertyIDAFFrameControl,
                    PropertyIDAWBFrameControl,
                    seamlessInsensorStateTag | InputMetadataSectionMask,
                    sensorLTCRatioTag
                };
                const UINT  propLength                          = CAMX_ARRAY_SIZE(Properties);
                VOID*       pData[propLength]                   = { 0 };
                UINT64      propertyDataOffset[propLength]      = { 0, 0, 0, 0, 0, 1 };
                SensorParam updateParam                         = { 0 };

                updateParam.isFSModeCapture                     = FALSE;
                GetDataList(Properties, pData, propertyDataOffset, propLength);

                CAMX_ASSERT(pData[0] != NULL);

                AECFrameControl*   pAECOutput            = reinterpret_cast<AECFrameControl*>(pData[0]);
                updateParam.pRegControlData              = reinterpret_cast<AECAlgoAdditionalInfo*>(pData[1]);
                AFFrameControl*    pFrameControl         = NULL;
                PDLibWindowConfig* pPDAFWindowConfig     = NULL;
                BOOL               applyPdafData         = FALSE;
                AWBFrameControl*   pAWBFrameControl      = NULL;
                AWBGainParams*     pWBGainConfig         = NULL;

                if ((NULL != pData[2]) && (PDAFType::PDType1 == m_pPDAFType))
                {
                    pFrameControl       = reinterpret_cast<AFFrameControl*>(pData[2]);
                    pPDAFWindowConfig   = &(pFrameControl->PDLibROI);
                }
                else
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Failed to get PropertyIDAFFrameControl/PDAFT1 is false");
                }

                if (NULL != pPDAFWindowConfig)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "PDLibROI type %d, [%f %f %f %f], hnum %d vnum %d residx = %d",
                                    pPDAFWindowConfig->roiType,
                                    pPDAFWindowConfig->fixedAFWindow.startX,
                                    pPDAFWindowConfig->fixedAFWindow.startY,
                                    pPDAFWindowConfig->fixedAFWindow.endX,
                                    pPDAFWindowConfig->fixedAFWindow.endY,
                                    pPDAFWindowConfig->horizontalWindowCount,
                                    pPDAFWindowConfig->verticalWindowCount,
                                    m_currentResolutionIndex);

                    if ((0.0 == pPDAFWindowConfig->fixedAFWindow.startX) &&
                       (0.0 == pPDAFWindowConfig->fixedAFWindow.startY) &&
                       (0.0 == pPDAFWindowConfig->fixedAFWindow.endX) &&
                       (0.0 == pPDAFWindowConfig->fixedAFWindow.endY))
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "PDLibROI are all Zeros");
                    }
                    else
                    {
                        PrepareSensorPDAFUpdate(pPDAFWindowConfig);
                    }
                }

                if (NULL != pData[3])
                {
                    pAWBFrameControl    = reinterpret_cast<AWBFrameControl*>(pData[3]);
                    pWBGainConfig       = &pAWBFrameControl->AWBGains;
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupSensor, "Failed to get PropertyIDAWBFrameControl");
                }

                if (NULL != pWBGainConfig)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "AWB Rgain=%f, Ggain=%f, Bgain=%f, residx = %d",
                                    pWBGainConfig->rGain,
                                    pWBGainConfig->gGain,
                                    pWBGainConfig->bGain,
                                    m_currentResolutionIndex);

                    if ((0.0 == pWBGainConfig->rGain) &&
                        (0.0 == pWBGainConfig->gGain) &&
                        (0.0 == pWBGainConfig->bGain))
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "WB gain are all Zeros");
                    }
                    else
                    {
                        PrepareSensorWBGain(pWBGainConfig);
                    }
                }

                // Update seamless in-sensor control state from meta
                if (NULL != pData[4])
                {
                    m_seamlessInSensorState = *reinterpret_cast<SeamlessInSensorState*>(pData[4]);

                    if ((SensorSeamlessType::SeamlessInSensorHDR3Exp == GetSensorSeamlessType(m_seamlessInSensorState)) &&
                        (TRUE == GetSensorDataObject()->IsSeamlessInSensorHDR3ExpAvailable(m_currentResolutionIndex)))
                    {
                        applyShortExposure    = TRUE;
                        applyMiddleExposure   = TRUE;
                        GetSensorDataObject()->SetMaxAnalogGain(m_currentResolutionIndex,
                                                                SensorSeamlessType::SeamlessInSensorHDR3Exp);
                        CAMX_LOG_INFO(CamxLogGroupSensor, "Seamless in-sensor HDR 3Exp is enabled, set apply short/mid exposure"
                                                          "to true and change max analog gain to sensor driver");
                    }
                    else if ((SeamlessInSensorState::InSensorHDR3ExpStop == m_seamlessInSensorState) &&
                             (TRUE == GetSensorDataObject()->IsSeamlessInSensorHDR3ExpAvailable(m_currentResolutionIndex)))
                    {
                        // Reset the max analog gain to default for sensor driver when seamless state is InSensorHDR3ExpStop
                        GetSensorDataObject()->SetMaxAnalogGain(m_currentResolutionIndex, SensorSeamlessType::None);
                    }
                    else
                    {
                        // Not in any in-sensor control state or the corresponding sensor settings aren't available
                        m_seamlessInSensorState = SeamlessInSensorState::None;
                    }

                    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Update m_seamlessInSensorState = %u", m_seamlessInSensorState);
                }

                //  Update LTC ratio index from meta
                if (NULL != pData[5])
                {
                    if (NULL != m_pLTCRatioSettings)
                    {
                        //  Update LTC ratio from meta during preview and apply the latest LTC ratio index of preview
                        //  while taking in-sensor HDR 3 exposure snapshot
                        if (SeamlessInSensorState::None == m_seamlessInSensorState)
                        {
                            m_latestLTCRatioData = *reinterpret_cast<UINT32*>(pData[5]);
                            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Update LTC ratio to 0x%02x", m_latestLTCRatioData);
                        }

                        GetSensorDataObject()->FillLTCRatioArray(m_currentResolutionIndex,
                                                                 &m_latestLTCRatioData,
                                                                 m_pLTCRatioSettings);
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupSensor, "m_pLTCRatioSettings is not Available");
                    }
                }

                if (NULL != pAECOutput)
                {
                    flashType       = pAECOutput->flashInfo;
                    LEDCurrents[0]  = pAECOutput->LEDCurrents[0];
                    LEDCurrents[1]  = pAECOutput->LEDCurrents[1];

                    UpdateSensorExposure(applyShortExposure, applyMiddleExposure, &updateParam, pAECOutput, requestId);
                    CAMX_LOG_INFO(CamxLogGroupSensor,
                                "Sensor[%d]: ProcessRequest: ApplyGains: RequestID=%llu Gain:ExpTime =%f : %llu "
                                "Middle Gain:ExpTime =%f : %llu, Short Gain:ExpTime =%f : %llu "
                                "LuxIdx = %f, LED Currents = %d : %d",
                                m_cameraId,
                                requestId,
                                updateParam.currentGain,
                                updateParam.currentExposure,
                                updateParam.currentMiddleGain,
                                updateParam.currentMiddleExposure,
                                updateParam.currentShortGain,
                                updateParam.currentShortExposure,
                                pAECOutput->luxIndex,
                                pAECOutput->LEDCurrents[0],
                                pAECOutput->LEDCurrents[1]);
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupSensor, "Failed to get PropertyIDAECFrameControl");
                }

                if (NULL != pExecuteProcessRequestData->pTuningModeData)
                {
                    UpdateSensitivityCorrectionFactor(pExecuteProcessRequestData->pTuningModeData);
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupSensor, "Can't obtain sensitivity correction factor (NULL TuningModedata)!");
                }

                PrepareSensorUpdate(&updateParam, pExecuteProcessRequestData->pNodeProcessRequestData);

                if (TRUE == GetSensorDataObject()->Is3ExposureHDRMode(m_currentResolutionIndex))
                {
                    if (NULL != pExecuteProcessRequestData->pTuningModeData)
                    {
                        SensorExposureInfo*    pExposureInfo = reinterpret_cast<SensorExposureInfo*>(m_pExposureInfo);
                        result                               = EVCompensationISPGainAdjust(pExposureInfo,
                                                                 pExecuteProcessRequestData->pTuningModeData);
                        if (CamxResultEFailed == result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupSensor, "3-exposure HDR: Adjust ISP gain failed!");
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupSensor, "3-exposure HDR: Adjust ISP gain failed(NULL TuningModedata)");
                    }
                }

                m_sensorParam.currentGain             = updateParam.currentGain;
                m_sensorParam.currentExposure         = updateParam.currentExposure;
                m_sensorParam.currentLineCount        = updateParam.currentLineCount;
                m_sensorParam.currentFrameLengthLines = updateParam.currentFrameLengthLines;
                m_sensorParam.currentShortGain        = updateParam.currentShortGain;
                m_sensorParam.currentShortExposure    = updateParam.currentShortExposure;
                m_sensorParam.currentShortLineCount   = updateParam.currentShortLineCount;
                m_sensorParam.currentMiddleGain       = updateParam.currentMiddleGain;
                m_sensorParam.currentMiddleExposure   = updateParam.currentMiddleExposure;
                m_sensorParam.currentMiddleLineCount  = updateParam.currentMiddleLineCount;
                m_sensorParam.pRegControlData         = updateParam.pRegControlData;

                result = ApplySensorUpdate(requestId);

                if (CamxResultSuccess == result)
                {
                    m_sensorParam.previousGain             = m_sensorParam.currentGain;
                    m_sensorParam.previousExposure         = m_sensorParam.currentExposure;
                    m_sensorParam.previousLineCount        = m_sensorParam.currentLineCount;
                    m_sensorParam.previousFrameLengthLines = m_sensorParam.currentFrameLengthLines;
                    m_sensorParam.previousMiddleGain       = m_sensorParam.currentMiddleGain;
                    m_sensorParam.previousMiddleExposure   = m_sensorParam.currentMiddleExposure;
                    m_sensorParam.previousMiddleLineCount  = m_sensorParam.currentMiddleLineCount;
                    m_sensorParam.previousShortGain        = m_sensorParam.currentShortGain;
                    m_sensorParam.previousShortExposure    = m_sensorParam.currentShortExposure;
                    m_sensorParam.previousShortLineCount   = m_sensorParam.currentShortLineCount;

                    m_prevSensorPdafData.PDAFroiType           = m_sensorPdafData.PDAFroiType;
                    m_prevSensorPdafData.PDAFstartX            = m_sensorPdafData.PDAFstartX;
                    m_prevSensorPdafData.PDAFstartY            = m_sensorPdafData.PDAFstartY;
                    m_prevSensorPdafData.PDAFendX              = m_sensorPdafData.PDAFendX;
                    m_prevSensorPdafData.PDAFendY              = m_sensorPdafData.PDAFendY;
                    m_prevSensorPdafData.horizontalWindowCount = m_sensorPdafData.horizontalWindowCount;
                    m_prevSensorPdafData.verticalWindowCount   = m_sensorPdafData.verticalWindowCount;

                    m_prevSensorWBGainData.RGain               = m_sensorWBGainData.RGain;
                    m_prevSensorWBGainData.GGain               = m_sensorWBGainData.GGain;
                    m_prevSensorWBGainData.BGain               = m_sensorWBGainData.BGain;

                    m_appliedSensorParam.previousGain             = m_appliedSensorParam.currentGain;
                    m_appliedSensorParam.previousExposure         = m_appliedSensorParam.currentExposure;
                    m_appliedSensorParam.previousLineCount        = m_appliedSensorParam.currentLineCount;
                    m_appliedSensorParam.previousFrameLengthLines = m_appliedSensorParam.currentFrameLengthLines;
                    m_appliedSensorParam.previousMiddleGain       = m_appliedSensorParam.currentMiddleGain;
                    m_appliedSensorParam.previousMiddleExposure   = m_appliedSensorParam.currentMiddleExposure;
                    m_appliedSensorParam.previousMiddleLineCount  = m_appliedSensorParam.currentMiddleLineCount;
                    m_appliedSensorParam.previousShortGain        = m_appliedSensorParam.currentShortGain;
                    m_appliedSensorParam.previousShortExposure    = m_appliedSensorParam.currentShortExposure;
                    m_appliedSensorParam.previousShortLineCount   = m_appliedSensorParam.currentShortLineCount;

                    m_pSensorParamQueue[requestId % m_maxSensorPipelineDelay] = m_sensorParam;
                }

                pDependency->dependencyFlags.dependencyFlagsMask = 0;
            }

            PublishPerFrameSensorMetaData(requestId);

            if (NULL != m_pFlash)
            {
                result = m_pFlash->ExecuteProcessRequest(pExecuteProcessRequestData, flashType, LEDCurrents, pTuningManager);
            }

            if (NULL != m_pOis)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "OIS execute process request called");
                m_pOis->ExecuteProcessRequest(pExecuteProcessRequestData);
            }
        }

        if (CamxResultSuccess == result)
        {
            PublishSensorProperties();

            if (0 == pDependency->dependencyFlags.dependencyFlagsMask)
            {
                ProcessPartialMetadataDone(requestId);
                ProcessMetadataDone(requestId);
                ProcessRequestIdDone(requestId);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "ProcessRequest on Sensor failed: %s", Utils::CamxResultToString(result));
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupSensor,
                       "ProcessRequest on Sensor failed: result:%s, pStaticSettings:%p, pEPRData:%p",
                       Utils::CamxResultToString(result),
                       pStaticSettings,
                       pExecuteProcessRequestData);
        if (NULL != pExecuteProcessRequestData)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor,
                           "ProcessRequest on Sensor failed: NodePRData:%p",
                           pExecuteProcessRequestData->pNodeProcessRequestData);

            if (NULL != pExecuteProcessRequestData->pNodeProcessRequestData)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor,
                               "ProcessRequest on Sensor failed: pCaptureRequest:%p",
                               pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::InitializeMultiCameraInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::InitializeMultiCameraInfo()
{
    CamxResult result = CamxResultSuccess;

    UINT32     numberOfCamerasRunning;
    UINT32     currentCameraId;
    BOOL       isMasterCamera;

    result = GetMultiCameraInfo(&m_isMultiCameraUsecase, &numberOfCamerasRunning, &currentCameraId, &isMasterCamera);

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_INFO(CamxLogGroupSensor, "m_isMultiCameraUsecase=%d, isMasterCamera=%d, currentCameraId=%d",
            m_isMultiCameraUsecase, isMasterCamera, currentCameraId);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Get multi camera info failed.");
        m_isMultiCameraUsecase = FALSE;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::UpdateMultiCameraInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::UpdateMultiCameraInfo(
    ExecuteProcessRequestData*  pNodeRequestData)
{
    CamxResult            result            = CamxResultSuccess;
    Pipeline*             pPipeline         = GetPipeline();
    MultiRequestSyncData* pMultiRequestData = pNodeRequestData->pNodeProcessRequestData->pCaptureRequest->pMultiRequestData;

    if ((NULL != pMultiRequestData) && (NULL != pPipeline))
    {
        m_isMasterCamera = IsMasterCamera();

        if (FALSE == m_isMasterCamera)
        {
            result = pPipeline->GetPeerRealtimePipelineId(pMultiRequestData,
                                                          GetPipelineId(),
                                                          &m_peerPipelineId);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Get peer pipeline id failed, rc:%d", result);
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "isMasterCamera=%d, peerPipelineId=%d",
                    m_isMasterCamera, m_peerPipelineId);
            }
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "isMasterCamera=%d", m_isMasterCamera);
        }
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupSensor, "pMultiRequestData=%p, pPipeline=%p",
            pMultiRequestData, pPipeline);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::IsMasterCamera
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SensorNode::IsMasterCamera()
{
    CamxResult result                  = CamxResultSuccess;
    BOOL       isMasterCamera          = TRUE;
    UINT32*    pTmp                    = NULL;
    UINT32     masterCameraTag         = 0;

    result = VendorTagManager::QueryVendorTagLocation("com.qti.chi.multicamerainfo", "MasterCamera", &masterCameraTag);

    if (CamxResultSuccess == result)
    {
        masterCameraTag |= InputMetadataSectionMask;

        const UINT propertyTag[]     = { masterCameraTag };
        VOID*      pData[1]          = { 0 };
        UINT64     propertyOffset[1] = { 0 };
        UINT       length            = CAMX_ARRAY_SIZE(propertyTag);

        result = GetDataList(propertyTag, pData, propertyOffset, length);

        if (NULL != pData[0])
        {
            pTmp           = static_cast<UINT32 *>(pData[0]);
            isMasterCamera = static_cast<BOOL>(*pTmp);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Get MasterCamera vendor tag data failed");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor,
            "MasterCamera vendor tags location not available %d",
            masterCameraTag);
    }

    return isMasterCamera;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::AdjustSlaveCameraExposureInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SensorNode::AdjustSlaveCameraExposureInfo(
    SensorParam* pSensorParam,
    NodeProcessRequestData* pNodeProcessRequestData)
{
    CamxResult            result                = CamxResultSuccess;
    UINT64                requestId             = pNodeProcessRequestData->pCaptureRequest->requestId;
    MultiRequestSyncData* pMultiRequestData     = pNodeProcessRequestData->pCaptureRequest->pMultiRequestData;
    INT64                 lpmRequestDelta       = requestId - pMultiRequestData->currReq.requestID[m_peerPipelineId];
    SensorSeamlessType    currentSeamlessType   = SensorSeamlessType::None;
    UINT                  SensorTags[]          =
    {
        PropertyIDSensorMetaData,
        PropertyIDSensorResolutionInfo
    };
    static const UINT     NumSensorTags     = CAMX_ARRAY_SIZE(SensorTags);
    VOID*                 pData[NumSensorTags];
    BOOL                  negate[NumSensorTags];
    UINT64                offsets[NumSensorTags];

    for (UINT i = 0; i < NumSensorTags; i++)
    {
        pData[i]   = NULL;
        negate[i]  = static_cast<BOOL>(lpmRequestDelta <= 0);
        offsets[i] = static_cast<UINT64>(abs(lpmRequestDelta));
    }

    result = GetDataListFromPipeline(SensorTags, pData, offsets, NumSensorTags, negate, m_peerPipelineId);

    if ((CamxResultSuccess == result) && (NULL != pData[0]) && (NULL != pData[1]))
    {
        SensorMetaData*       pMasterMetaData       = NULL;
        SensorResolutionInfo* pMasterResoultionInfo = NULL;

        pMasterMetaData       = static_cast<SensorMetaData*>(pData[0]);
        pMasterResoultionInfo = static_cast<SensorResolutionInfo*>(pData[1]);

        if ((NULL != pMasterMetaData) && (NULL != pMasterResoultionInfo))
        {
            UINT   lineCount        = pSensorParam->currentLineCount;
            UINT64 exposureTime     = pSensorParam->currentExposure;
            FLOAT  gain             = pSensorParam->currentGain;
            DOUBLE lineTime         = GetSensorDataObject()->GetLineReadoutTime(m_currentResolutionIndex, currentSeamlessType);
            UINT32 defaultFLL       = GetSensorDataObject()->GetFrameLengthLines(m_currentResolutionIndex, currentSeamlessType);
            UINT64 lineDelta        = GetSensorDataObject()->GetMultiCameraLineDelta();
            UINT32 frameLengthLines = pSensorParam->currentFrameLengthLines;
            DOUBLE maxFPS           = GetSensorDataObject()->GetMaxFPS(m_currentResolutionIndex);
            UINT32 verticalOffset   = GetSensorDataObject()->GetExposureControlInfo()->verticalOffset;
            FLOAT  masterFps        = (static_cast<FLOAT>(pMasterResoultionInfo->frameLengthLine) /
                                       static_cast<FLOAT>(pMasterMetaData->frameLengthLines)) *
                                      pMasterResoultionInfo->frameRate;
            FLOAT  slaveFps         = (static_cast<FLOAT>(defaultFLL) / (pSensorParam->currentFrameLengthLines)) * (maxFPS);

            CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                             "Sensor[%d]:current: requestID:%llu, Linecount:%d, currFLL:%d,"
                             " defaultFLL:%d, FPS:%f, exp_time:%llu, gain:%f",
                             m_cameraId,
                             requestId,
                             pSensorParam->currentLineCount,
                             pSensorParam->currentFrameLengthLines,
                             defaultFLL,
                             slaveFps,
                             pSensorParam->currentExposure,
                             pSensorParam->currentGain);

            // if: HW master and slave should synchronize the coarse integration time as pre-shutter time
            // else: for the normal operation FPS should match for master and slave
            if ((FALSE == IsPipelineStreamedOn()) && (TRUE == m_initialConfigPending))
            {
                exposureTime = (pMasterMetaData->exposureTime) + (lineDelta * pMasterResoultionInfo->lineReadoutTime);
                lineCount = GetSensorDataObject()->ExposureToLineCount(exposureTime,
                                                                       m_currentResolutionIndex,
                                                                       currentSeamlessType);
                CAMX_LOG_INFO(CamxLogGroupSensor, "adjusted pre-shutter time, lineDelta: %d", lineDelta);
            }
            else
            {
                UINT64 frameDuration = (pMasterMetaData->frameDuration) + (lineDelta * pMasterResoultionInfo->lineReadoutTime);
                frameLengthLines = GetSensorDataObject()->FrameDurationToFrameLengthLines(frameDuration,
                                                                                          m_currentResolutionIndex);
            }

            if ((frameLengthLines) < (lineCount + verticalOffset))
            {
                if ((FALSE == IsPipelineStreamedOn()) && (TRUE == m_initialConfigPending))
                {
                    frameLengthLines = lineCount + verticalOffset;
                }
                else
                {
                    lineCount = frameLengthLines - verticalOffset;
                }
            }

            if (frameLengthLines < defaultFLL)
            {
                CAMX_LOG_WARN(CamxLogGroupSensor, "adjusted FLL is smaller than min FLL, so setting to min FLL");
                frameLengthLines = defaultFLL;
            }

            if (lineCount != pSensorParam->currentLineCount)
            {
                exposureTime = GetSensorDataObject()->LineCountToExposure(lineCount,
                                                                          m_currentResolutionIndex,
                                                                          currentSeamlessType);
                gain = gain * (static_cast<FLOAT>(pSensorParam->currentExposure) / static_cast<FLOAT>(exposureTime));

                if ((gain < 1.0) && ((FALSE == m_initialConfigPending) || (TRUE == IsPipelineStreamedOn())))
                {
                    lineCount = static_cast<UINT>(static_cast<FLOAT>(lineCount) * gain);
                    gain = 1.0;
                }

                if (0 == lineCount)
                {
                    lineCount = 1;
                }
            }

            pSensorParam->currentExposure         = exposureTime;
            pSensorParam->currentGain             = gain;
            pSensorParam->currentLineCount        = lineCount;
            pSensorParam->currentFrameLengthLines = frameLengthLines;

            slaveFps = (static_cast<FLOAT>(defaultFLL) / (pSensorParam->currentFrameLengthLines)) * (maxFPS);

            CAMX_LOG_INFO(CamxLogGroupSensor,
                          "Sensor[%d]:Updated requestID:%llu, (SW master):FPS:%f, Exp_Time:%llu, frameduration: %llu"
                          "(SW Slave):FPS:%f, exp_time:%llu, lineCount:%d, FLL:%d, gain:%f",
                          m_cameraId,
                          requestId,
                          masterFps,
                          pMasterMetaData->exposureTime,
                          pMasterMetaData->frameDuration,
                          slaveFps,
                          pSensorParam->currentExposure,
                          pSensorParam->currentLineCount,
                          pSensorParam->currentFrameLengthLines,
                          pSensorParam->currentGain);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Get data from peer pipeline:%u failed", m_peerPipelineId);
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::GetOTPData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const EEPROMOTPData* SensorNode::GetOTPData()
{
    CamxResult            result     = CamxResultSuccess;
    const EEPROMOTPData*  pOTPData   = NULL;
    HwCameraInfo          cameraInfo;

    result = HwEnvironment::GetInstance()->GetCameraInfo(m_cameraId, &cameraInfo);

    if (CamxResultSuccess == result)
    {
        pOTPData = &(cameraInfo.pSensorCaps->OTPData);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to obtain EEPROM OTP data, result: %d", result);
    }

    return pOTPData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::LoadPDlibrary
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::LoadPDlibrary()
{
    CamxResult result       = CamxResultSuccess;
    CHAR*      pPDAFLibName = NULL;

    PDAFData* pPDAFData = m_pSensorModuleData->GetPDAFDataObj();
    if (NULL != pPDAFData)
    {
        pPDAFLibName = pPDAFData->GetPDAFLibraryName(m_currentResolutionIndex);
    }

    if (NULL != pPDAFLibName)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "PD library to load %s", pPDAFLibName);

        m_pPDLibHandler = CAMX_NEW CPDLibHandler();

        if (NULL == m_pPDLibHandler)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "m_pPDLibHandler is NULL.");
            return CamxResultENoMemory;
        }

        if (NULL != m_pPDCallback && NULL != m_pPDCallback->pfnSetAlgoInterface)
        {

            /// @todo (CAMX-2931): Add vendor tag support to Pdlib
#if PDLIB_VENDOR_TAG_SUPPORT
            CHIALGORITHMINTERFACE chiAlgoInterface;

            StatsVendorTagCallbacks::GetInstance()->SetMetatadataPool(pFinalizeInitializationData->pMainPool);
            chiAlgoInterface.pGetVendorTagBase = StatsVendorTagCallbacks::GetVendorTagBase;
            chiAlgoInterface.pGetMetadata      = StatsVendorTagCallbacks::GetVendorTag;
            chiAlgoInterface.pSetMetaData      = StatsVendorTagCallbacks::SetVendorTag;
            m_pPDCallback->pfnSetAlgoInterface(&chiAlgoInterface);
#endif // PDLIB_VENDOR_TAG_SUPPORT

            // Create an instance of the PD library
            result = m_pPDLibHandler->CreateLib(m_pPDCallback->pfnCreate, &m_pPDLib, pPDAFLibName, m_cameraId);
            if ((m_pPDLib != NULL) ||(CamxResultSuccess == result))
            {
                CAMX_LOG_INFO(CamxLogGroupSensor, "PD library CreateLib result=%d, m_pPDLib=%p",
                    result, m_pPDLib);
            }
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupSensor, "NULL pointer m_pPDCallback=%p ", m_pPDCallback);
        }

        UINTPTR_T   PDLibAddr             = reinterpret_cast<UINTPTR_T>(m_pPDLib);
        const UINT  SensorPDOutputTags[]  = { PropertyIDUsecasePDLibInfo };
        const VOID* pPDOutputData[1]      = { 0 };
        UINT        pPDDataCount[1]       = { 0 };
        pPDDataCount[0]                   = sizeof(PDLibAddr);
        pPDOutputData[0]                  = &PDLibAddr;
        WriteDataList(SensorPDOutputTags, pPDOutputData, pPDDataCount, 1);
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupSensor, "No PDAF Library to load.");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::ProcessingNodeFinalizeInitialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::ProcessingNodeFinalizeInitialization(
    FinalizeInitializationData* pFinalizeInitializationData)
{
    CamxResult              result           = CamxResultEFailed;
    CSLCSIPHYCapability     CSIPHYCapability = {0};
    CSLFlashQueryCapability flashCapability  = {0};
    CSLActuatorCapability   actuatorCap      = {0};
    CSLOISCapability        oisCap           = {0};
    ResolutionData*         pResolutionData  = NULL;

    ///@ note 0 is the type for Sensor
    CAMX_ASSERT_MESSAGE(Sensor == Type(), "Invalid type: %d", Type());
    CAMX_ASSERT(NULL != GetHwContext());

    m_pHwContext        = GetHwContext();
    m_cameraId          = GetPipeline()->GetCameraId();

    m_pSensorSubDevicesCache->SetHWContextPointer(m_pHwContext);
    HwEnvironment::GetInstance()->InitializeSensorHwDeviceCache(m_cameraId,
        GetHwContext(), GetCSLSession(), 0, m_pSensorSubDevicesCache->GetSensorSubDeviceHandles(m_cameraId),
        m_pSensorSubDevicesCache);

    m_currentSyncMode   = GetPipeline()->GetSensorSyncMode();
    m_pThreadManager    = pFinalizeInitializationData->pThreadManager;

    CAMX_ASSERT_MESSAGE(m_cameraId < MaxNumImageSensors, "Invalid camera ID: %d", m_cameraId);

    if ((NULL != m_pHwContext) && (m_cameraId < MaxNumImageSensors))
    {
        // NOWHINE CP036a: Need exception here
        m_pSensorModuleData = const_cast<ImageSensorModuleData*>(m_pHwContext->GetImageSensorModuleData(m_cameraId));
        CAMX_ASSERT(NULL != m_pSensorModuleData);
    }

    if (NULL != m_pSensorModuleData)
    {
        CAMX_LOG_CONFIG(CamxLogGroupSensor, "CameraID:%d SensorName:%s SensorMode:%d SyncMode:%d",
                                            m_cameraId, m_pSensorModuleData->GetSensorDataObject()->GetSensorName(),
                                            pFinalizeInitializationData->pSensorModeInfo->modeIndex, m_currentSyncMode);
        result = GetSensorModuleIndexes(&CSIPHYCapability, &flashCapability, &actuatorCap, &oisCap);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to obtain sensor module indexs. result: %d", result);
            return result;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to get sensor module data:m_pSensorModuleData %p", m_pSensorModuleData);
        return result;
    }

    // Reload EEPROM data before publishing when the below property is set to 1
    if (TRUE == OsUtils::GetPropertyBool("persist.vendor.camera.eeprom.reload", FALSE))
    {
        CAMX_LOG_INFO(CamxLogGroupSensor, "Reloading EEPROM data");
        HwEnvironment::GetInstance()->ReadEEPROMData(m_cameraId, GetCSLSession());
    }

    if (TRUE == GetStaticSettings()->dumpSensorEEPROMData)
    {
        HwEnvironment::GetInstance()->DumpEEPROMData(m_cameraId);
    }

    // Update the OTP data member variable and here on this can be used wherever OTP data info is needed.
    // Do not move this to earlier as this should get the updated data when reload is enabled
    m_pOTPData = GetOTPData();

    if (NULL == m_pOTPData)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "OTP data is NULL");
        return CamxResultEInvalidPointer;
    }

    m_opt_wb_grgb = m_pOTPData->WBCalibration[0].grOverGB;

    // Determine current sensor mode before published to usecase property pool
    m_pCurrentSensorModeInfo = pFinalizeInitializationData->pSensorModeInfo;
    m_currentResolutionIndex = pFinalizeInitializationData->pSensorModeInfo->modeIndex;

    const CSIInformation*   pCSIInfo        = NULL;
    UsecaseSensorModes      sensorModeData  = { { {0} } };
    BOOL                    isComboMode     = FALSE;
    UINT32                  laneAssign      = 0;

    pCSIInfo = m_pSensorModuleData->GetCSIInfo();
    if (NULL != pCSIInfo)
    {
        isComboMode = pCSIInfo->isComboMode;
        laneAssign  = pCSIInfo->laneAssign;
    }

    GetSensorDataObject()->PopulateSensorModeData(&sensorModeData, CSIPHYCapability.slotInfo, laneAssign, isComboMode);

    pResolutionData  = &(GetSensorDataObject()->GetResolutionInfo())->resolutionData[m_currentResolutionIndex];

    // Setting sensor specific crop info to be used during PD Lib initialization.
    m_sensorCrop = sensorModeData.allModes[m_currentResolutionIndex].cropInfo;

    // Setup what we are going to publish since we know it in advance
    UINT SensorOutputTags[] =
    {
        PropertyIDUsecaseSensorModes,
        PropertyIDUsecaseSensorCurrentMode,
        0,
        0
    };

    static const UINT NumSensorMetaDataTags             = CAMX_ARRAY_SIZE(SensorOutputTags);
    const VOID*       pData[NumSensorMetaDataTags]      = { 0 };
    UINT              pDataCount[NumSensorMetaDataTags] = { 0 };
    UINT              dataIndex                         = 0;
    UINT              metaTag                           = 0;

    pData[dataIndex]        = &sensorModeData;
    pDataCount[dataIndex++] = sizeof(UsecaseSensorModes);

    pData[dataIndex]        = &m_currentResolutionIndex;
    pDataCount[dataIndex++] = sizeof(m_currentResolutionIndex);

    result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.sensor_meta_data",
                                                      "EEPROMInformation",
                                                      &metaTag);
    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to get vendor tag for EEPROMInfo");

    SensorOutputTags[dataIndex] = (metaTag | StaticMetadataSectionMask);
    pData[dataIndex]            = &(m_pOTPData->EEPROMInfo);
    pDataCount[dataIndex++]     = sizeof(EEPROMInformation);

    result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.sensor_meta_data",
                                                      "current_mode",
                                                      &metaTag);
    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to get vendor tag for current_mode");

    SensorOutputTags[dataIndex] = (metaTag | UsecaseMetadataSectionMask);
    pData[dataIndex]            = &m_currentResolutionIndex;
    pDataCount[dataIndex++]     = sizeof(m_currentResolutionIndex);

    WriteDataList(SensorOutputTags, pData, pDataCount, NumSensorMetaDataTags);

    if (TRUE == pResolutionData->integrationInfoExists)
    {
        UINT        IntegrationInfoTags[]           = { 0 };
        const VOID* pIntegrationInfoData[1]         = { 0 };
        UINT        pIntegrationInfoDataCount[1]    = { 0 };
        UINT        integrationInfoMetaTag          = 0;

        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.sensor_meta_data",
                                                          "integration_information",
                                                          &integrationInfoMetaTag);
        CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to get vendor tag: integration_information");

        IntegrationInfoTags[0]                 = (integrationInfoMetaTag | UsecaseMetadataSectionMask);
        pIntegrationInfoData[0]                = &pResolutionData->integrationInfo;
        pIntegrationInfoDataCount[0]           = sizeof(IntegrationInformation) / sizeof(UINT32);
        WriteDataList(IntegrationInfoTags, pIntegrationInfoData, pIntegrationInfoDataCount, 1);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "IntegrationInfo from sensor bin is not available");
    }

    if ((CamxResultSuccess == result) && (FALSE == GetStaticSettings()->disablePDAF))
    {
        m_isPDAFEnabled = IsPDAFEnabled();

        // ok to fail in cases where pdlibrary does not exist.
        // We log error in the function
        if (TRUE == m_isPDAFEnabled)
        {
            PDAFData* pPDAFData = m_pSensorModuleData->GetPDAFDataObj();
            if (NULL != pPDAFData)
            {
                PopulatePDAFInformation();
                PublishPDAFInformation();

                LoadPDlibrary();
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid PDAF data object: pPDAFData %p", pPDAFData);
            }
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupSensor, "PDAF not enabled: m_isPDAFEnabled %d", m_isPDAFEnabled);
        }
    }

    if (CamxResultSuccess == result)
    {
        /// @todo (CAMX-4059) Mechanism for nodes to know the vendor tags that will get published
        UINT32 AECDataPublisherPresentTag = 0;

        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.AECDataPublisherPresent",
                                                          "AECDataPublisherPresent",
                                                          &AECDataPublisherPresentTag);
        if (CamxResultSuccess == result)
        {
            const UINT tags[]               = { AECDataPublisherPresentTag | UsecaseMetadataSectionMask };
            const UINT length               = CAMX_ARRAY_SIZE(tags);
            VOID*      pPropData[length]    = { 0 };
            UINT64     offsets[length]      = { 0 };

            GetDataList(tags, pPropData, offsets, length);
            if (NULL != pPropData[0])
            {
                BOOL* pAECDataPublisherPresent = reinterpret_cast<BOOL*>(pPropData[0]);

                m_isVendorAECDataAvailable = *pAECDataPublisherPresent;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to get AECDataPublisherPresent Location");
        }
    }

    if ((CamxResultSuccess == result) && (InvalidJobHandle == m_hJobFamilyHandle))
    {
        const CHAR* pSensorName = m_pSensorModuleData->GetSensorDataObject()->GetSensorName();
        result = m_pThreadManager->RegisterJobFamily(SensorThreadJobCallback,
                                                     pSensorName,
                                                     NULL,
                                                     JobPriority::High,
                                                     TRUE,
                                                     &m_hJobFamilyHandle);
    }

    if ((CamxResultSuccess == result) && (InvalidJobHandle == m_hJobFamilySubModuleOISHandle))
    {
        CHAR OISJobName[256];
        OsUtils::SNPrintF(OISJobName, sizeof(OISJobName), "%s_%s",
                          m_pSensorModuleData->GetSensorDataObject()->GetSensorName(), "OIS");
        result = m_pThreadManager->RegisterJobFamily(SensorThreadJobCallback,
                                                     OISJobName,
                                                     NULL,
                                                     JobPriority::High,
                                                     TRUE,
                                                     &m_hJobFamilySubModuleOISHandle);
    }

    if ((CamxResultSuccess == result) && (InvalidJobHandle == m_hJobFamilyReadHandle))
    {
        CHAR readName[256];
        OsUtils::SNPrintF(readName, sizeof(readName), "%s_%s",
                          m_pSensorModuleData->GetSensorDataObject()->GetSensorName(), "readName");
        result = m_pThreadManager->RegisterJobFamily(SensorThreadJobCallback,
                                                     readName,
                                                     NULL,
                                                     JobPriority::High,
                                                     TRUE,
                                                     &m_hJobFamilyReadHandle);
    }

    if (CamxResultSuccess == result)
    {
        CreateSensorStateSignals();
    }

    if (CamxResultSuccess == result)
    {
        // Must initialize before creating command buffers
        result = InitializeCmdBufferManagerList(SensorMaxCmdBufferManagerCount);
    }

    if (CamxResultSuccess == result)
    {
        result = CreatePerFrameUpdateResources();
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to CreatePerFrameUpdateResources");
        }
    }

    if (CamxResultSuccess == result)
    {
        GetMaxCmdBufSize();
        result = CreateCmdBuffers();
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to Create Command Buffers");
        }
    }

    const StaticSettings* pStaticSettings = GetStaticSettings();
    CAMX_ASSERT(pStaticSettings != NULL);

    if ((CamxResultSuccess == result)                    &&
        (TRUE == pStaticSettings->enableOISOptimization) &&
        (NULL != m_pSensorModuleData->GetOisDataObject()))
    {
        // OIS initialization:
        // This runs in an offloaded thread parallel to sensor initialization since this is
        // not dependent on the sensor configuration status.
        CAMX_LOG_INFO(CamxLogGroupSensor, "OIS Optimization Enabled");
        SensorPostJob* pSensorPostJob    = CAMX_NEW SensorPostJob;

        if (NULL != pSensorPostJob)
        {
            pSensorPostJob->pSensor          = this;
            pSensorPostJob->sensorJobCommand = SensorPostJobCommand::InitializeOIS;
            pSensorPostJob->pData            = NULL;
            VOID* pSensorPostJobData[]       = { pSensorPostJob, NULL };
            result                           = m_pThreadManager->PostJob(m_hJobFamilySubModuleOISHandle,
                                                                         NULL,
                                                                         &pSensorPostJobData[0],
                                                                         FALSE,
                                                                         FALSE);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Out of memory SensorPostJob create failed");
            result = CamxResultENoMemory;
        }
    }

    // Acquire sensor device now if the optimization is disabled
    if ((CamxResultSuccess == result) &&
        (FALSE == GetStaticSettings()->enableSensorAcquireOptimization))
    {
        result = AcquireDevice();
    }

    if (CamxResultSuccess == result)
    {
        SensorPostJob* pSensorPostJob    = CAMX_NEW SensorPostJob;

        // Check pSensorPostJob before using it.
        if (NULL != pSensorPostJob)
        {
            pSensorPostJob->pSensor          = this;
            pSensorPostJob->sensorJobCommand = SensorPostJobCommand::InitializeSensor;
            pSensorPostJob->pData            = NULL;
            VOID* pSensorPostJobData[]       = { pSensorPostJob, NULL };
            result                           = m_pThreadManager->PostJob(m_hJobFamilyHandle,
                                                                         NULL,
                                                                         &pSensorPostJobData[0],
                                                                         FALSE,
                                                                         FALSE);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Out of memory SensorPostJob create failed");
            result = CamxResultENoMemory;
        }
    }

    if (CamxResultSuccess == result)
    {
        SensorPostJob* pSensorPostJobCreateSubModules = CAMX_NEW SensorPostJob;
        if (NULL != pSensorPostJobCreateSubModules)
        {
            pSensorPostJobCreateSubModules->pSensor          = this;
            pSensorPostJobCreateSubModules->sensorJobCommand = SensorPostJobCommand::SubModulesCreate;
            pSensorPostJobCreateSubModules->pData            = NULL;
            VOID* pSubModuleCreateData[]                     = { pSensorPostJobCreateSubModules, NULL };
            result                                           = m_pThreadManager->PostJob(m_hJobFamilyHandle,
                                                                                         NULL,
                                                                                         &pSubModuleCreateData[0],
                                                                                         FALSE,
                                                                                         FALSE);
        }
    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "ProcessingNodeFinalize on Sensor failed: %d", Utils::CamxResultToString(result));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::CreateSensorStateSignals
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::CreateSensorStateSignals()
{
    CamxResult result = CamxResultSuccess;

    m_signalSensorInit.pWaitCondition       = Condition::Create("SensorInitDone");
    m_signalSensorConfig.pWaitCondition     = Condition::Create("SensorConfigDone");
    m_signalSensorSubModules.pWaitCondition = Condition::Create("SensorSubModuleCreated");
    m_signalSensorInit.pMutex               = Mutex::Create("SensorInitDone");
    m_signalSensorConfig.pMutex             = Mutex::Create("SensorConfigDone");
    m_signalSensorSubModules.pMutex         = Mutex::Create("SensorSubModuleCreated");
    m_signalOISInit.pWaitCondition          = Condition::Create("OISInitDone");
    m_signalOISInit.pMutex                  = Mutex::Create("OISInitDone");

    if ((NULL == m_signalSensorInit.pWaitCondition)       || (NULL == m_signalSensorConfig.pWaitCondition) ||
        (NULL == m_signalSensorSubModules.pWaitCondition) || (NULL == m_signalSensorInit.pMutex) ||
        (NULL == m_signalSensorConfig.pMutex)             || (NULL == m_signalSensorSubModules.pMutex) ||
        (NULL == m_signalOISInit.pWaitCondition)          || (NULL == m_signalOISInit.pMutex))
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create sensor state signal");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::IsPDAFEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SensorNode::IsPDAFEnabled()
{
    BOOL     isSupported = FALSE;
    PDAFType PDType      = PDAFType::PDTypeUnknown;

    m_pSensorModuleData->GetPDAFInformation(m_currentResolutionIndex, &isSupported, &PDType);
    m_pPDAFType = PDType;

    CAMX_LOG_INFO(CamxLogGroupSensor, "PDAFType = %d: isPDAFEnabled %d", m_pPDAFType, isSupported);

    return isSupported;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::UpdateStartupExposureSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::UpdateStartupExposureSettings(
    BOOL isStatsNodeEnabled)
{
    CamxResult result              = CamxResultSuccess;
    BOOL       applyShortExposure  = FALSE;
    BOOL       applyMiddleExposure = FALSE;

    static const UINT UsecaseProperties[] =
    {
        PropertyIDUsecaseAECFrameControl,
    };
    VOID*  pUsecaseData[1]      = { 0 };
    UINT64 usecaseDataOffset[1] = { 0 };
    UINT   length;

    if (TRUE == isStatsNodeEnabled)
    {
        length = CAMX_ARRAY_SIZE(UsecaseProperties);
        result = GetDataList(UsecaseProperties, pUsecaseData, usecaseDataOffset, length);
    }
    else
    {
        UINT32 AECDataMetaTag = 0;

        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.AECData",
                                                          "AECData",
                                                          &AECDataMetaTag);
        if (CamxResultSuccess == result)
        {
            UINT AECData[] =
            {
                AECDataMetaTag | UsecaseMetadataSectionMask,
            };
            length = CAMX_ARRAY_SIZE(AECData);
            result = GetDataList(AECData, pUsecaseData, usecaseDataOffset, length);
        }
    }

    if ((CamxResultSuccess == result) && (NULL != pUsecaseData[0]))
    {
        SensorParam updateParam     = { 0 };

        updateParam.isFSModeCapture = FALSE;
        applyShortExposure = GetSensorDataObject()->IsHDRMode(m_currentResolutionIndex);
        applyMiddleExposure = GetSensorDataObject()->Is3ExposureHDRMode(m_currentResolutionIndex);
        if (TRUE == isStatsNodeEnabled)
        {
            AECFrameControl* pControl = reinterpret_cast<AECFrameControl*>(pUsecaseData[0]);
            UpdateSensorExposure(applyShortExposure, applyMiddleExposure, &updateParam, pControl, 0);
        }
        else
        {
            ChiAECData* pChiAECData = reinterpret_cast<ChiAECData*>(pUsecaseData[0]);

            updateParam.currentExposure      = pChiAECData->exposureTime;
            updateParam.currentShortExposure = pChiAECData->exposureTime;
            updateParam.currentGain          = GetSensorDataObject()->SensitivityToGain(pChiAECData->sensitivity, 1.0f);
            updateParam.currentShortGain     = updateParam.currentGain;
        }
        updateParam.pRegControlData = NULL;

        // Second argument can be NULL because we do not need to update slave exposure settings based on the master
        // sensor in the first frame
        PrepareSensorUpdate(&updateParam, NULL);

        CAMX_LOG_INFO(CamxLogGroupSensor,
                      "Sensor[%d]: Startup Exposure: [Gain, ExpTime] [long: %f, %ld], [Middle: %f, %ld, "
                      "[Short: %f, %ld] FLL = %d",
                      m_cameraId,
                      updateParam.currentGain,
                      updateParam.currentExposure,
                      updateParam.currentMiddleGain,
                      updateParam.currentMiddleExposure,
                      updateParam.currentShortGain,
                      updateParam.currentShortExposure,
                      updateParam.currentFrameLengthLines);

        m_sensorParam.currentGain             = updateParam.currentGain;
        m_sensorParam.currentExposure         = updateParam.currentExposure;
        m_sensorParam.currentLineCount        = updateParam.currentLineCount;
        m_sensorParam.currentMiddleGain       = updateParam.currentMiddleGain;
        m_sensorParam.currentMiddleExposure   = updateParam.currentMiddleExposure;
        m_sensorParam.currentMiddleLineCount  = updateParam.currentMiddleLineCount;
        m_sensorParam.currentShortGain        = updateParam.currentShortGain;
        m_sensorParam.currentShortExposure    = updateParam.currentShortExposure;
        m_sensorParam.currentShortLineCount   = updateParam.currentShortLineCount;
        m_sensorParam.currentFrameLengthLines = updateParam.currentFrameLengthLines;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::UpdateLensInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::UpdateLensInformation(
    LensInfo* pLensInfo)
{
    CamxResult result                       = CamxResultSuccess;

    const LensInformation* pLensInformation = m_pSensorModuleData->GetLensInfo();

    if (NULL == pLensInformation)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Lens Information is NULL");
        return CamxResultEInvalidPointer;
    }

    pLensInfo->fNumber             = static_cast <FLOAT> (pLensInformation->fNumber);
    pLensInfo->totalFDistance      = static_cast <FLOAT> (pLensInformation->maxFocusDistance);
    pLensInfo->focalLength         = static_cast <FLOAT> (pLensInformation->focalLength);
    pLensInfo->actuatorSensitivity = m_pSensorModuleData->GetActuatorSensitivity();
    pLensInfo->pixelSize           = GetSensorDataObject()->GetPixelSize();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::FinalizeBufferProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SensorNode::FinalizeBufferProperties(
    BufferNegotiationData* pBufferNegotiationData)
{
    CAMX_UNREFERENCED_PARAM(pBufferNegotiationData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::CreateAndSubmitCommand
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::CreateAndSubmitCommand(
    UINT cmdSize,
    I2CRegSettingType cmdType,
    CSLPacketOpcodesSensor opCode,
    UINT32 currentResolutionIndex,
    UINT regSettingIdx)

{
    CamxResult result = CamxResultSuccess;

    m_pConfigCmdBuffer  = GetCmdBuffer(m_pConfigCmdBufferManager);
    m_pConfigPacket     = GetPacket(m_pConfigPacketManager);

    if ((NULL == m_pConfigPacket) || (NULL == m_pConfigCmdBuffer))
    {
        result = CamxResultENoMemory;
    }
    else
    {
        ImageSensorData* pSensorData = GetSensorDataObject();
        CAMX_ASSERT(NULL != pSensorData);

        VOID* pCmd = m_pConfigCmdBuffer->BeginCommands(cmdSize / sizeof(UINT32));

        if (cmdSize > m_maxCmdBufSize)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "MaxCommandBuffers needs to be updated with %d for type %d", cmdSize, cmdType);
            result = CamxResultEOutOfBounds;
        }

        if ((NULL != pCmd) && (CamxResultSuccess == result))
        {
            if (I2CRegSettingType::AEC == cmdType)
            {
                result = pSensorData->CreateI2CCmd(cmdType,
                    pCmd,
                    m_pRegSettings,
                    currentResolutionIndex,
                    regSettingIdx);
            }
            else if (I2CRegSettingType::SPC == cmdType)
            {
                result = pSensorData->CreateI2CCmd(cmdType,
                    pCmd,
                    static_cast<const VOID*>(&m_pOTPData->SPCCalibration.settings),
                    currentResolutionIndex,
                    regSettingIdx);
            }
            else if (I2CRegSettingType::QSC == cmdType)
            {
                result = pSensorData->CreateI2CCmd(cmdType,
                    pCmd,
                    static_cast<const VOID*>(&m_pOTPData->QSCCalibration.settings),
                    currentResolutionIndex,
                    regSettingIdx);
            }
            else if (I2CRegSettingType::AWBOTP == cmdType)
            {
                result = pSensorData->CreateI2CCmd(cmdType,
                    pCmd,
                    static_cast<const VOID*>(&m_pOTPData->WBCalibration[0].settings),
                    currentResolutionIndex,
                    regSettingIdx);
            }
            else if (I2CRegSettingType::LSC == cmdType)
            {
                result = pSensorData->CreateI2CCmd(cmdType,
                    pCmd,
                    static_cast<const VOID*>(&m_pOTPData->LSCCalibration[0].settings),
                    currentResolutionIndex,
                    regSettingIdx);
            }
            else
            {
                result = pSensorData->CreateI2CCmd(cmdType, pCmd, NULL, currentResolutionIndex, regSettingIdx);
            }
            if (CamxResultSuccess == result)
            {
                result = m_pConfigCmdBuffer->CommitCommands();
            }
        }
        if (CamxResultSuccess == result)
        {
            // Not associated with any request. Won't be recycled.
            m_pConfigCmdBuffer->SetRequestId(CamxInvalidRequestId);

            // Not associated with any request. Won't be recycled.
            m_pConfigPacket->SetRequestId(CamxInvalidRequestId);

            m_pConfigPacket->SetOpcode(CSLDeviceTypeInvalidDevice, opCode);

            result = m_pConfigPacket->AddCmdBufferReference(m_pConfigCmdBuffer, NULL);
        }

        if (CamxResultSuccess == result)
        {
            result = m_pConfigPacket->CommitPacket();
        }
        if (CamxResultSuccess == result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "IN Submit!");
            // Send Sensor  configuration
            result = m_pHwContext->Submit(GetCSLSession(), m_hSensorDevice, m_pConfigPacket);
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "out Submit! result %d", result);
        }
        if (NULL != m_pConfigPacketManager)
        {
            m_pConfigPacketManager->Recycle(m_pConfigPacket);
            m_pConfigPacket = NULL;
        }

        if (NULL != m_pConfigCmdBufferManager)
        {
            m_pConfigCmdBufferManager->Recycle(m_pConfigCmdBuffer);
            m_pConfigCmdBuffer = NULL;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::LoadSensorConfigCmds
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::LoadSensorConfigCmds()
{
    CamxResult       result      = CamxResultEFailed;
    ImageSensorData* pSensorData = GetSensorDataObject();

    CAMX_ASSERT(NULL != pSensorData);

    if (SensorConfigurationStatus::SensorSubModuleCreateComplete == m_sensorConfigStatus)
    {
        m_sensorConfigStatus = SensorConfigurationStatus::SensorConfigurationInProgress;
        UINT regSettingIdx = 0; // using common index as only init reg setting uses this param
        UINT resCmdSize   = pSensorData->GetI2CCmdSize(I2CRegSettingType::Res, NULL, m_currentResolutionIndex, &regSettingIdx);
        UINT startCmdSize = pSensorData->GetI2CCmdSize(I2CRegSettingType::Start, NULL, 0, &regSettingIdx);
        UINT stopCmdSize  = pSensorData->GetI2CCmdSize(I2CRegSettingType::Stop, NULL, 0, &regSettingIdx);

        UINT SPCCmdSize                  = 0;
        UINT QSCCmdSize                  = 0;
        UINT syncCmdSize                 = 0;

        I2CRegSettingType syncI2CCmdType = I2CRegSettingType::Master;

        if (MasterMode == m_currentSyncMode)
        {
            syncCmdSize    = pSensorData->GetI2CCmdSize(I2CRegSettingType::Master, NULL, 0, &regSettingIdx);
            syncI2CCmdType = I2CRegSettingType::Master;
        }
        else if (SlaveMode == m_currentSyncMode)
        {
            syncCmdSize    = pSensorData->GetI2CCmdSize(I2CRegSettingType::Slave, NULL, 0, &regSettingIdx);
            syncI2CCmdType = I2CRegSettingType::Slave;
        }

        if ((NULL != m_pOTPData) && (TRUE == m_pOTPData->SPCCalibration.isAvailable))
        {
            SPCCmdSize = pSensorData->GetI2CCmdSize(I2CRegSettingType::SPC,
                                                    static_cast<const VOID*>(&m_pOTPData->SPCCalibration.settings),
                                                    0, &regSettingIdx);
            CAMX_ASSERT(0 == (SPCCmdSize % sizeof(UINT32)));
        }

        if ((NULL != m_pOTPData) && (TRUE == m_pOTPData->QSCCalibration.isAvailable) &&
            (TRUE == pSensorData->IsQSCCalibrationEnabledMode(m_currentResolutionIndex)))
        {
            QSCCmdSize = pSensorData->GetI2CCmdSize(I2CRegSettingType::QSC,
                                                    static_cast<const VOID*>(&m_pOTPData->QSCCalibration.settings),
                                                    0, &regSettingIdx);
            if ((0 != QSCCmdSize) && (0 != (QSCCmdSize % sizeof(UINT32))))
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "QSCCmdSize: %d is invalid", QSCCmdSize);
            }
        }

        CAMX_ASSERT_MESSAGE((resCmdSize != 0) && (startCmdSize != 0) && (stopCmdSize != 0),
            "Command size(res: %d, start: %d stop: %d)", resCmdSize, startCmdSize, stopCmdSize);

        // All sizes should be a multiple of dword
        CAMX_ASSERT(0 == (resCmdSize % sizeof(UINT32)));
        CAMX_ASSERT(0 == (startCmdSize % sizeof(UINT32)));
        CAMX_ASSERT(0 == (stopCmdSize % sizeof(UINT32)));

        CAMX_LOG_INFO(CamxLogGroupSensor,
            "resCmdSize: %d, startCmdSize: %d, stopCmdSize:= %d, syncCmdSize: %d, SPCCmdSize: %d, QSCCmdSize: %d",
            resCmdSize, startCmdSize, stopCmdSize, syncCmdSize, SPCCmdSize, QSCCmdSize);

        result = CreateAndSubmitCommand(
            resCmdSize, I2CRegSettingType::Res, CSLPacketOpcodesSensorConfig,
            m_currentResolutionIndex, regSettingIdx);

        if (CamxResultSuccess == result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Res configuration successful!");

            if (NoSync != m_currentSyncMode)
            {
                if (0 == syncCmdSize)
                {
                    CAMX_LOG_WARN(CamxLogGroupSensor,
                        "Seems no sensor sync setting in sensor driver, can't apply sync setting!");
                }
                else
                {
                    result = CreateAndSubmitCommand(syncCmdSize, syncI2CCmdType,
                             CSLPacketOpcodesSensorConfig, 0, regSettingIdx);

                    if (CamxResultSuccess == result)
                    {
                        CAMX_LOG_INFO(CamxLogGroupSensor, "Sensor hardware sync configure successful!");
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupSensor, "Sensor hardware sync configure failed!");
                    }
                }
            }

            if ((CamxResultSuccess == result) && (0 != SPCCmdSize))
            {
                result = CreateAndSubmitCommand(SPCCmdSize, I2CRegSettingType::SPC,
                         CSLPacketOpcodesSensorConfig, 0, regSettingIdx);
                if (CamxResultSuccess == result)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "SpcSetting configuration successful!");
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "SpcSetting configuration failed!");
                }
            }

            if ((CamxResultSuccess == result) && (0 != QSCCmdSize))
            {
                result = CreateAndSubmitCommand(QSCCmdSize, I2CRegSettingType::QSC,
                         CSLPacketOpcodesSensorConfig, 0, regSettingIdx);
                if (CamxResultSuccess == result)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "QscSetting configuration successful!");
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "QscSetting configuration failed!");
                }
            }

            if (CamxResultSuccess == result)
            {
                result = CreateAndSubmitCommand(startCmdSize, I2CRegSettingType::Start,
                         CSLPacketOpcodesSensorStreamOn, 0, regSettingIdx);

                if (CamxResultSuccess == result)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Streamon configuration successful!");

                    result = CreateAndSubmitCommand(stopCmdSize, I2CRegSettingType::Stop,
                             CSLPacketOpcodesSensorStreamOff, 0, regSettingIdx);

                    if (CamxResultSuccess == result)
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "StreamOFF configuration successful!");
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupSensor, "Sensor Streamoff configuration failed!");
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Sensor StreamOn configuration failed!");
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Sensor Res configuration failed!");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Sensor Initialization failed, cannot configure sensor!");
    }

    if (CamxResultSuccess == result)
    {
        m_sensorConfigStatus = SensorConfigurationStatus::SensorConfigurationComplete;
        CAMX_LOG_INFO(CamxLogGroupSensor, "Sensor configuration successful");
    }
    else
    {
        m_sensorConfigStatus = SensorConfigurationStatus::SensorConfigurationFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::CreateActuatorResources()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::CreateActuatorResources()
{
    CamxResult     result               = CamxResultSuccess;
    ResourceParams packetResourceParams = {0};
    ResourceParams cmdResourceParams    = {0};
    UINT           I2CInfoCmdSize       = sizeof(CSLSensorI2CInfo);
    UINT           powerUpCmdSize       = m_pSensorModuleData->GetActuatorDataObject()->GetPowerSequenceCmdSize(TRUE);
    UINT           powerDownCmdSize     = m_pSensorModuleData->GetActuatorDataObject()->GetPowerSequenceCmdSize(FALSE);
    UINT           powerSequenceSize    = (powerUpCmdSize + powerDownCmdSize);
    UINT           initializeCmdSize    = m_pSensorModuleData->GetActuatorDataObject()->GetInitializeCmdSize();
    UINT           moveFocusCmdSize     = m_pSensorModuleData->GetActuatorDataObject()->GetMaxMoveFocusCmdSize();
    UINT           maxRequestQueueDepth = GetPipeline()->GetRequestQueueDepth() + 2;

    if (CamxResultSuccess == result)
    {
        packetResourceParams.usageFlags.packet             = 1;
        // one for power sequence, one for I2C Info, one for init settings - Total 3 cmd buffers
        packetResourceParams.packetParams.maxNumCmdBuffers = 3;
        packetResourceParams.packetParams.maxNumIOConfigs  = 0;
        packetResourceParams.resourceSize                  =
            Packet::CalculatePacketSize(&packetResourceParams.packetParams);
        packetResourceParams.poolSize                      =
            (maxRequestQueueDepth * packetResourceParams.resourceSize);
        packetResourceParams.alignment                     = CamxPacketAlignmentInBytes;
        packetResourceParams.pDeviceIndices                = NULL;
        packetResourceParams.numDevices                    = 0;
        packetResourceParams.memFlags                      = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("ActuatorPacketManager", &packetResourceParams, &m_pActuatorPacketManager);
    }

    if (CamxResultSuccess == result)
    {
        cmdResourceParams.resourceSize         = I2CInfoCmdSize;
        cmdResourceParams.poolSize             = I2CInfoCmdCount * I2CInfoCmdSize;
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::Generic;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("ActuatorI2CInfoCmdManager", &cmdResourceParams, &m_pActuatorI2CInfoCmdManager);
    }

    if (CamxResultSuccess == result)
    {
        cmdResourceParams.resourceSize         = powerSequenceSize;
        cmdResourceParams.poolSize             = cmdResourceParams.resourceSize;
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::Generic;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("ActuatorPowerCmdManager", &cmdResourceParams, &m_pActuatorPowerCmdManager);
    }

    if (CamxResultSuccess == result)
    {
        cmdResourceParams.resourceSize         = initializeCmdSize;
        cmdResourceParams.poolSize             = InitialConfigCmdCount * initializeCmdSize;
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::I2C;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("ActuatorInitializeCmdManager", &cmdResourceParams, &m_pActuatorInitializeCmdManager);
    }

    if (CamxResultSuccess == result)
    {
        cmdResourceParams.resourceSize         = moveFocusCmdSize;
        cmdResourceParams.poolSize             = (maxRequestQueueDepth * cmdResourceParams.resourceSize);;
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::I2C;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("ActuatorMoveFocusCmdManager", &cmdResourceParams, &m_pActuatorMoveFocusCmdManager);
    }

    if (CamxResultSuccess == result)
    {
        ActuatorCreateData createData = {0};

        createData.pHwContext            = m_pHwContext;
        createData.pMainPool             = GetPerFramePool(PoolType::PerFrameResult);
        createData.pPerUsecasePool       = GetPerFramePool(PoolType::PerUsecase);
        createData.pPacketManager        = m_pActuatorPacketManager;
        createData.pInitCmdManager       = m_pActuatorInitializeCmdManager;
        createData.pI2CInfoCmdManager    = m_pActuatorI2CInfoCmdManager;
        createData.pPowerCmdManager      = m_pActuatorPowerCmdManager;
        createData.pMoveFocusCmdManager  = m_pActuatorMoveFocusCmdManager;
        createData.cameraId              = m_cameraId;
        createData.pParentNode           = this;
        createData.pOTPData              = m_pOTPData;
        createData.requestQueueDepth     = GetPipeline()->GetRequestQueueDepth();
        SubDeviceProperty actuatorDevice = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, ActuatorHandle);

        result = Actuator::Create(&createData, actuatorDevice.deviceIndex);

        if (CamxResultSuccess == result)
        {
            m_pActuator = createData.pActuator;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::CreateFlashResources()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::CreateFlashResources()
{
    CamxResult        result                     = CamxResultSuccess;
    CmdBufferManager* pFlashPacketManager        = NULL;
    CmdBufferManager* pFlashInitializeCmdManager = NULL;
    CmdBufferManager* pFlashPowerCmdManager      = NULL;
    CmdBufferManager* pFlashI2CCmdManager        = NULL;
    CmdBufferManager* pFlashI2CInitCmdManager    = NULL;
    CmdBufferManager* pFlashFireCmdManager       = NULL;
    CmdBufferManager* pFlashI2CFireCmdManager    = NULL;
    CmdBufferManager* pFlashRERCmdManager        = NULL;
    CmdBufferManager* pFlashQueryCmdManager      = NULL;
    UINT              maxRequestQueueDepth       = GetPipeline()->GetRequestQueueDepth() + 2;

    ResourceParams packetResourceParams = {0};

    packetResourceParams.usageFlags.packet             = 1;
    packetResourceParams.packetParams.maxNumCmdBuffers = FlashCmdCount;
    packetResourceParams.packetParams.maxNumIOConfigs  = 0;
    packetResourceParams.packetParams.maxNumPatches    = 0;
    packetResourceParams.resourceSize                  = Packet::CalculatePacketSize(&packetResourceParams.packetParams);
    packetResourceParams.poolSize                      =
        (FlashCmdCount * (maxRequestQueueDepth * packetResourceParams.resourceSize));
    packetResourceParams.alignment                     = CamxPacketAlignmentInBytes;
    packetResourceParams.pDeviceIndices                = NULL;
    packetResourceParams.numDevices                    = 0;
    packetResourceParams.memFlags                      = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

    result = CreateCmdBufferManager("FlashPacketManager", &packetResourceParams, &pFlashPacketManager);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Fail to create FlashPacketManager, result %d", result);
    }

    if (CamxResultSuccess == result)
    {
        ResourceParams cmdResourceParams = {0};

        cmdResourceParams.resourceSize         = sizeof(CSLFlashInfoCmd);
        cmdResourceParams.poolSize             = (maxRequestQueueDepth * cmdResourceParams.resourceSize);
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::Generic;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("FlashInitializeCmdManager", &cmdResourceParams, &pFlashInitializeCmdManager);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Fail to create FlashInitializeCmdManager, result %d", result);
        }
    }

    FlashDriverType flashType =   m_pSensorModuleData->GetFlashDataObject()->GetFlashType();

    if ((CamxResultSuccess == result) && (FlashDriverType::I2C == flashType))
    {
        UINT i2cInfoCmdSize                    = sizeof(CSLSensorI2CInfo);
        ResourceParams cmdResourceParams       = {0};

        cmdResourceParams.resourceSize         = i2cInfoCmdSize;
        cmdResourceParams.poolSize             = I2CInfoCmdCount * i2cInfoCmdSize;
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::Generic;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("FlashI2CInfoCmdManager", &cmdResourceParams, &pFlashI2CCmdManager);
    }

    if ((CamxResultSuccess == result) && (FlashDriverType::I2C == flashType))
    {
        UINT powerUpCmdSize                    = m_pSensorModuleData->GetFlashDataObject()->GetPowerSequenceCmdSize(TRUE);
        UINT powerDownCmdSize                  = m_pSensorModuleData->GetFlashDataObject()->GetPowerSequenceCmdSize(FALSE);
        UINT powerSequenceSize                 = (powerUpCmdSize + powerDownCmdSize);
        ResourceParams cmdResourceParams       = { 0 };

        cmdResourceParams.resourceSize         = powerSequenceSize;
        cmdResourceParams.poolSize             = cmdResourceParams.resourceSize;
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::Generic;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("FlashPowerCmdManager", &cmdResourceParams, &pFlashPowerCmdManager);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Fail to create FlashPowerCmdManager, result %d", result);
        }
    }

    if ((CamxResultSuccess == result) && (FlashDriverType::I2C == flashType))
    {
        UINT i2cInitializeCmdSize              = m_pSensorModuleData->GetFlashDataObject()->GetI2CInitializeCmdSize();
        ResourceParams cmdResourceParams       = {0};

        cmdResourceParams.resourceSize         = i2cInitializeCmdSize;
        cmdResourceParams.poolSize             = InitialConfigCmdCount * i2cInitializeCmdSize;
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::I2C;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("FlashI2CInitializeCmdManager", &cmdResourceParams, &pFlashI2CInitCmdManager);
    }

    if (CamxResultSuccess == result)
    {
        ResourceParams cmdResourceParams = {0};

        cmdResourceParams.resourceSize         = sizeof(CSLFlashFireCmd);
        cmdResourceParams.poolSize             = (maxRequestQueueDepth * cmdResourceParams.resourceSize);
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::Generic;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("FlashFireCmdManager", &cmdResourceParams, &pFlashFireCmdManager);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Fail to create FlashFireCmdManager, result %d", result);
        }
    }

    if ((CamxResultSuccess == result) && (FlashDriverType::I2C == flashType))
    {

        UINT i2cFireCmdSize                    = m_pSensorModuleData->GetFlashDataObject()->GetI2CFireMaxCmdSize();
        ResourceParams cmdResourceParams       = {0};

        cmdResourceParams.resourceSize         = i2cFireCmdSize;
        cmdResourceParams.poolSize             = InitialConfigCmdCount * i2cFireCmdSize;
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::I2C;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("FlashI2CFireCmdManager", &cmdResourceParams, &pFlashI2CFireCmdManager);
    }

    if (CamxResultSuccess == result)
    {
        ResourceParams cmdResourceParams = {0};

        cmdResourceParams.resourceSize         = sizeof(CSLFlashQueryCurrentCmd);;
        cmdResourceParams.poolSize             = (maxRequestQueueDepth * cmdResourceParams.resourceSize);
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::Generic;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("FlashQueryCmdManager", &cmdResourceParams, &pFlashQueryCmdManager);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Fail to create FlashQueryCmdManager, result %d", result);
        }
    }

    if (CamxResultSuccess == result)
    {
        ResourceParams cmdResourceParams = {0};

        cmdResourceParams.resourceSize         = sizeof(CSLFlashRERCmd);
        cmdResourceParams.poolSize             = (maxRequestQueueDepth * cmdResourceParams.resourceSize);
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::Generic;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       =  NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("FlashRERCmdManager", &cmdResourceParams, &pFlashRERCmdManager);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Fail to create FlashRERCmdManager")
        }
    }

    if (CamxResultSuccess == result)
    {
        FlashCreateData createData = {0};

        createData.pHwContext            = m_pHwContext;
        createData.pPacketManager        = pFlashPacketManager;
        createData.pI2CCmdManager        = pFlashI2CCmdManager;
        createData.pInitializeCmdManager = pFlashInitializeCmdManager;
        createData.pFlashPowerCmdManager = pFlashPowerCmdManager;
        createData.pI2CInitCmdManager    = pFlashI2CInitCmdManager;
        createData.pFireCmdManager       = pFlashFireCmdManager;
        createData.pI2CFireCmdManager    = pFlashI2CFireCmdManager;
        createData.pRERCmdManager        = pFlashRERCmdManager;
        createData.pQueryCmdManager      = pFlashQueryCmdManager;
        createData.pParentNode           = this;
        createData.cameraId              = m_cameraId;
        createData.operationMode         = CSLRealtimeOperation;

        result = Flash::Create(&createData);
        if (CamxResultSuccess == result)
        {
            m_pFlash = createData.pFlash;
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Flash resources initialized");
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Flash resources initialization failed");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::CreateOisResources()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::CreateOisResources()
{
    CamxResult        result            = CamxResultSuccess;
    UINT              I2CInfoCmdSize    = sizeof(CSLOISI2CInfo);
    UINT              powerUpCmdSize    = m_pSensorModuleData->GetOisDataObject()->GetPowerSequenceCmdSize(TRUE);
    UINT              powerDownCmdSize  = m_pSensorModuleData->GetOisDataObject()->GetPowerSequenceCmdSize(FALSE);
    UINT              powerSequenceSize = (powerUpCmdSize + powerDownCmdSize);
    UINT              initializeCmdSize = m_pSensorModuleData->GetOisDataObject()->GetInitializeCmdSize();
    UINT              modeCmdSize       = m_pSensorModuleData->GetOisDataObject()->GetOISModeMaxCmdSize();
    SubDeviceProperty oisDevice         = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, OISHandle);

    if (CamxResultSuccess == result)
    {
        ResourceParams packetResourceParams = {0};

        // Creating 4 command buffers for OIS i2ccmdbuffer, powerSettings, initsettings, calibsettings
        packetResourceParams.usageFlags.packet             = 1;
        packetResourceParams.packetParams.maxNumCmdBuffers = 4;
        packetResourceParams.packetParams.maxNumIOConfigs  = 0;
        packetResourceParams.resourceSize                  =
            Packet::CalculatePacketSize(&packetResourceParams.packetParams);
        packetResourceParams.poolSize                      = packetResourceParams.resourceSize;
        packetResourceParams.alignment                     = CamxPacketAlignmentInBytes;
        packetResourceParams.pDeviceIndices                = NULL;
        packetResourceParams.numDevices                    = 0;
        packetResourceParams.memFlags                      = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("OISInitializePacketManager", &packetResourceParams, &m_pOISInitializePacketManager);
    }

    if (CamxResultSuccess == result)
    {
        ResourceParams cmdResourceParams    = {0};

        cmdResourceParams.resourceSize         = I2CInfoCmdSize;
        cmdResourceParams.poolSize             = I2CInfoCmdCount * (I2CInfoCmdSize);
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::Generic;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("OISI2CInfoCmdManager", &cmdResourceParams, &m_pOISI2CInfoCmdManager);
    }

    if (CamxResultSuccess == result)
    {
        ResourceParams cmdResourceParams    = {0};

        cmdResourceParams.resourceSize         = powerSequenceSize;
        cmdResourceParams.poolSize             = cmdResourceParams.resourceSize;
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::Generic;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("OISPowerCmdManager", &cmdResourceParams, &m_pOISPowerCmdManager);
    }

    if (CamxResultSuccess == result)
    {
        ResourceParams cmdResourceParams    = {0};

        cmdResourceParams.resourceSize         = initializeCmdSize;
        cmdResourceParams.poolSize             = InitialConfigCmdCount* (initializeCmdSize);
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::I2C;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("OISInitializeCmdManager", &cmdResourceParams, &m_pOISInitializeCmdManager);
    }

    if (CamxResultSuccess == result)
    {
        ResourceParams packetResourceParams = {0};

        packetResourceParams.usageFlags.packet             = 1;
        packetResourceParams.packetParams.maxNumCmdBuffers = 1;
        packetResourceParams.packetParams.maxNumIOConfigs  = 0;
        packetResourceParams.resourceSize                  =
            Packet::CalculatePacketSize(&packetResourceParams.packetParams);
        packetResourceParams.poolSize                      = packetResourceParams.resourceSize;
        packetResourceParams.alignment                     = CamxPacketAlignmentInBytes;
        packetResourceParams.pDeviceIndices                = NULL;
        packetResourceParams.numDevices                    = 0;
        packetResourceParams.memFlags                      = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("pOISModePacketManager", &packetResourceParams, &m_pOISModePacketManager);
    }

    if (CamxResultSuccess == result)
    {
        ResourceParams cmdResourceParams    = {0};

        cmdResourceParams.resourceSize         = modeCmdSize;
        cmdResourceParams.poolSize             = InitialConfigCmdCount* (modeCmdSize);
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::I2C;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("OISModeCmdManager", &cmdResourceParams, &m_pOISModeCmdManager);
    }

    if (CamxResultSuccess == result)
    {
        OisCreateData createData = {0};

        createData.pHwContext         = m_pHwContext;
        createData.pInitPacketManager = m_pOISInitializePacketManager;
        createData.pInitCmdManager    = m_pOISInitializeCmdManager;
        createData.pI2CInfoCmdManager = m_pOISI2CInfoCmdManager;
        createData.pPowerCmdManager   = m_pOISPowerCmdManager;
        createData.pModeCmdManager    = m_pOISModeCmdManager;
        createData.pModePacketManager = m_pOISModePacketManager;
        createData.cameraId           = m_cameraId;
        createData.pOTPData           = m_pOTPData;
        createData.pParentNode        = this;

        result = OIS::Create(&createData, oisDevice.deviceIndex);

        if (CamxResultSuccess == result)
        {
            m_pOis = createData.pOis;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::UpdateSensitivityCorrectionFactor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SensorNode::UpdateSensitivityCorrectionFactor(
    ChiTuningModeParameter* pTuningModeData)
{
    TuningSetManager*      pTuningManager             = GetTuningDataManager()->GetChromatix();
    TuningMode *           pSelectors                 = reinterpret_cast<TuningMode*>(&pTuningModeData->TuningMode[0]);
    UINT                   numSelectors               = pTuningModeData->noOfSelectionParameter;
    aecArbitration::AECCoreArbitrationType*   pModArb = pTuningManager->GetModule_Arbitration(pSelectors, numSelectors);
    SensorExposureInfo*    pExposureInfo              = reinterpret_cast<SensorExposureInfo*>(m_pExposureInfo);

    pExposureInfo->sensitivityCorrectionFactor        = pModArb->expTables[0].sensitivityCorrectionFactor;

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "UpdateSensitivityCorrectionFactor=%f", pExposureInfo->sensitivityCorrectionFactor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::EVCompensationISPGainAdjust
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::EVCompensationISPGainAdjust(
    SensorExposureInfo* pExposureInfo,
    ChiTuningModeParameter* pTuningModeData)
{
    CamxResult             result                    = CamxResultSuccess;

    static const UINT      metaDataTag[]             = { InputControlAEExposureCompensation};
    const UINT             length                    = CAMX_ARRAY_SIZE(metaDataTag);
    VOID*                  pData[length]             = { 0 };
    UINT64                 metaDataOffset[length]    = { 0 };
    INT32                  evCompVal                 = 0;

    TuningSetManager*      pTuningManager            = GetTuningDataManager()->GetChromatix();
    TuningMode *           pSelectors                = reinterpret_cast<TuningMode*>(&pTuningModeData->TuningMode[0]);
    UINT                   numSelectors              = pTuningModeData->noOfSelectionParameter;
    aecMetering::AECCoreMeteringType*   pModMtr      = pTuningManager->GetModule_Metering(pSelectors, numSelectors);
    INT32                  stepPerEV                 = 0;

    GetDataList(metaDataTag, pData, metaDataOffset, length);
    if (NULL != pData[0])
    {
        evCompVal = *static_cast<INT32*>(pData[0]);
        if (evCompVal >= 0)
        {
            stepPerEV                     = pModMtr->mtrLumaTarget.EVLumaTarget.stepsPerEV;
            pExposureInfo->ISPDigitalGain = static_cast<FLOAT>(evCompVal) / stepPerEV + 1.0f;

            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "use ISP digital gain to apply EV+: "
                                                 "EVCompVal = %d, stepPerEV = %d, ISPDigitalGain=%f",
                                                 evCompVal, stepPerEV, pExposureInfo->ISPDigitalGain);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "set ISP digital gain to defaut value 1.0 when EV-");
        }
    }
    else
    {
        result                       = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::CreatePerFrameUpdateResources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::CreatePerFrameUpdateResources()
{
    CamxResult result = CamxResultSuccess;

    if (NULL == m_pExposureInfo)
    {
        m_pExposureInfo = static_cast<VOID*>(GetSensorDataObject()->Allocate(SensorAllocateType::ExposureInfo));
    }

    if (NULL == m_pRegSettings)
    {
        m_pRegSettings = static_cast<VOID*>(GetSensorDataObject()->Allocate(SensorAllocateType::I2CRegSetting));
    }

    if (NULL == m_pPDAFSettings)
    {
        m_pPDAFSettings = static_cast<VOID*>(GetSensorDataObject()->Allocate(SensorAllocateType::I2CRegSetting));
    }

    if (NULL == m_pWBGainSettings)
    {
        m_pWBGainSettings = static_cast<VOID*>(GetSensorDataObject()->Allocate(SensorAllocateType::I2CRegSetting));
    }

    if (NULL == m_pLTCRatioSettings)
    {
        m_pLTCRatioSettings = static_cast<VOID*>(GetSensorDataObject()->Allocate(SensorAllocateType::I2CRegSetting));
    }

    if (NULL == m_pExposureRegAddressInfo)
    {
        m_pExposureRegAddressInfo =
            static_cast<VOID*>(GetSensorDataObject()->Allocate(SensorAllocateType::ExposureRegAddressInfo));

        if (NULL != m_pExposureRegAddressInfo)
        {
            GetSensorDataObject()->UpdateExposureRegAddressInfo(m_pExposureRegAddressInfo);
        }
    }

    if ((NULL == m_pExposureInfo) || (NULL == m_pRegSettings)    || (NULL == m_pExposureRegAddressInfo) ||
        (NULL == m_pPDAFSettings) || (NULL == m_pWBGainSettings) || (NULL == m_pLTCRatioSettings))
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor,
                       "Failed to allocate buffer ExposureInfo: %p, RegSettings: %p, RegAddressInfo: %p PDAFsettings =%p",
                       "m_pWBGainSettings=%p, m_pLTCRatioSettings=%p",
                       m_pExposureInfo,
                       m_pRegSettings,
                       m_pExposureRegAddressInfo,
                       m_pPDAFSettings,
                       m_pWBGainSettings,
                       m_pLTCRatioSettings);

        return CamxResultEInvalidPointer;
    }

    UINT maxUpdateCmdSize        = GetSensorDataObject()->GetI2CCmdMaxSize(I2CRegSettingType::AEC, m_pRegSettings);
    UINT maxPDAFSettingSize      = GetSensorDataObject()->GetI2CCmdMaxSize(I2CRegSettingType::PDAF, m_pPDAFSettings);
    UINT maxWBGainSettingSize    = GetSensorDataObject()->GetI2CCmdMaxSize(I2CRegSettingType::AWB, m_pWBGainSettings);
    UINT maxLTCRatioSettingSize  = GetSensorDataObject()->GetI2CCmdMaxSize(I2CRegSettingType::LTC, m_pLTCRatioSettings);
    UINT maxMasterSettingSize    = GetSensorDataObject()->GetI2CCmdMaxSize(I2CRegSettingType::Master, NULL);
    UINT maxSlaveSettingSize     = GetSensorDataObject()->GetI2CCmdMaxSize(I2CRegSettingType::Slave, NULL);

    maxUpdateCmdSize            += maxPDAFSettingSize;
    maxUpdateCmdSize            += maxWBGainSettingSize;
    maxUpdateCmdSize            += maxLTCRatioSettingSize;
    maxUpdateCmdSize            += maxMasterSettingSize > maxSlaveSettingSize ? maxMasterSettingSize : maxSlaveSettingSize;
    // Update the pdaf setting

    CAMX_LOG_INFO(CamxLogGroupSensor, "maxUpdateCmdSize: %d", maxUpdateCmdSize);

    CAMX_ASSERT(maxUpdateCmdSize != 0);
    CAMX_ASSERT(0 == (maxUpdateCmdSize % sizeof(UINT32)));

    // Create the update packet manager, command manager, packet and commands

    ResourceParams resourceParams                   = { 0 };
    resourceParams.usageFlags.packet                = 1;
    resourceParams.packetParams.maxNumCmdBuffers    = 1;
    resourceParams.packetParams.maxNumIOConfigs     = 0;
    resourceParams.resourceSize                     = Packet::CalculatePacketSize(&resourceParams.packetParams);
    resourceParams.poolSize                         = MaxCommandBuffers * resourceParams.resourceSize;
    resourceParams.alignment                        = CamxPacketAlignmentInBytes;
    resourceParams.pDeviceIndices                   = NULL;
    resourceParams.numDevices                       = 0;
    resourceParams.memFlags                         = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

    if (CamxResultSuccess == CreateCmdBufferManager("UpdatePacketManager", &resourceParams, &m_pUpdatePacketManager))
    {
        ResourceParams params       = { 0 };
        params.resourceSize         = maxUpdateCmdSize;
        params.poolSize             = MaxCommandBuffers * maxUpdateCmdSize;
        params.usageFlags.cmdBuffer = 1;
        params.cmdParams.type       = CmdType::I2C;
        params.alignment            = CamxCommandBufferAlignmentInBytes;
        params.pDeviceIndices       = NULL;
        params.numDevices           = 0;
        params.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("UpdateCmdBufferManager", &params, &m_pUpdateCmdBufferManager);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::HandleDelayInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SensorNode::HandleDelayInfo(
    SensorParam* pAppliedSensorParam,
    NodeProcessRequestData* pNodeProcessRequestData)
{
    UINT64           requestId  = 0;
    BOOL             isDelayed  = FALSE;
    const DelayInfo* pDelayInfo = NULL;

    if (SensorSyncMode::NoSync == GetPipeline()->GetSensorSyncMode())
    {
        pDelayInfo = GetSensorDataObject()->GetSensorPipelineDelay(DelayInfoType::EXPOSURECONTROL);
    }
    else
    {
        pDelayInfo = GetSensorDataObject()->GetSensorPipelineDelay(DelayInfoType::FRAMESYNC);
    }

    if ((NULL != pNodeProcessRequestData) && (NULL != pDelayInfo) &&
        (pDelayInfo->maxPipeline > 0))
    {
        requestId = pNodeProcessRequestData->pCaptureRequest->requestId;

        if ((TRUE == pDelayInfo->frameLengthLinesExists) && (0 != pDelayInfo->frameLengthLines) &&
             (pDelayInfo->frameLengthLines < pDelayInfo->maxPipeline) &&
            ((requestId - m_initialRequestId) >= (pDelayInfo->maxPipeline - pDelayInfo->frameLengthLines)))
        {
            UINT32 appliedIndex = (requestId - (pDelayInfo->maxPipeline - pDelayInfo->frameLengthLines)) %
                m_maxSensorPipelineDelay;
            pAppliedSensorParam->currentFrameLengthLines = m_pSensorParamQueue[appliedIndex].currentFrameLengthLines;
            isDelayed = TRUE;
        }

        if ((TRUE == pDelayInfo->linecountExists) && (0 != pDelayInfo->linecount) &&
            (pDelayInfo->linecount < pDelayInfo->maxPipeline) &&
            ((requestId - m_initialRequestId) >= (pDelayInfo->maxPipeline - pDelayInfo->linecount)))
        {
            UINT32 appliedIndex                         = (requestId - (pDelayInfo->maxPipeline - pDelayInfo->linecount)) %
                m_maxSensorPipelineDelay;
            pAppliedSensorParam->currentLineCount       = m_pSensorParamQueue[appliedIndex].currentLineCount;
            pAppliedSensorParam->currentMiddleLineCount = m_pSensorParamQueue[appliedIndex].currentMiddleLineCount;
            pAppliedSensorParam->currentShortLineCount  = m_pSensorParamQueue[appliedIndex].currentShortLineCount;
            isDelayed = TRUE;
        }

        if ((TRUE == pDelayInfo->gainExists) && (0 != pDelayInfo->gain) &&
            (pDelayInfo->gain < pDelayInfo->maxPipeline) &&
            ((requestId - m_initialRequestId) >= (pDelayInfo->maxPipeline - pDelayInfo->gain)))
        {
            UINT32 appliedIndex                    = (requestId - (pDelayInfo->maxPipeline - pDelayInfo->gain)) %
                m_maxSensorPipelineDelay;
            pAppliedSensorParam->currentGain       = m_pSensorParamQueue[appliedIndex].currentGain;
            pAppliedSensorParam->currentMiddleGain = m_pSensorParamQueue[appliedIndex].currentMiddleGain;
            pAppliedSensorParam->currentShortGain  = m_pSensorParamQueue[appliedIndex].currentShortGain;
            isDelayed = TRUE;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Applied FLL:%d, linecount:%d, gain:%f for req#%llu",
            pAppliedSensorParam->currentFrameLengthLines, pAppliedSensorParam->currentLineCount,
            pAppliedSensorParam->currentGain, requestId);
    }

    return isDelayed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::PrepareSensorUpdate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SensorNode::PrepareSensorUpdate(
    SensorParam* pSensorParam,
    NodeProcessRequestData* pNodeProcessRequestData)
{
    BOOL                isDelayed           = FALSE;
    VOID*               pRegControlInfo     = NULL;
    UINT32              minLineCount        = GetSensorDataObject()->GetMinLineCount();
    SensorSeamlessType  currentSeamlessType = GetSensorSeamlessType(m_seamlessInSensorState);
    UINT64              lineReadoutTime     =
        static_cast<UINT64>(GetSensorDataObject()->GetLineReadoutTime(m_currentResolutionIndex,
                                                                      currentSeamlessType));
    UINT64             minExposureTime      = static_cast<UINT64>(minLineCount) * lineReadoutTime;
    SensorExposureInfo appliedExposureInfo;

    const ExposureContorlInformation* pExposureControlInfo = GetSensorDataObject()->GetExposureControlInfo();

    CAMX_ASSERT(NULL != pExposureControlInfo);

    if ((NULL == m_pExposureInfo) || (NULL == m_pRegSettings) || (NULL == m_pExposureRegAddressInfo))
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor,
                       "Invalid inputs: m_pExposureInfo: %p, m_pRegSettings: %p, RegAddressInfo: %p",
                       m_pExposureInfo,
                       m_pRegSettings,
                       m_pExposureRegAddressInfo);

        return;
    }

    if (pSensorParam->currentExposure < minExposureTime)
    {
        pSensorParam->currentExposure = minExposureTime;
    }
    if (pSensorParam->currentMiddleExposure < minExposureTime)
    {
        pSensorParam->currentMiddleExposure = minExposureTime;
    }

    if (pSensorParam->currentShortExposure < minExposureTime)
    {
        pSensorParam->currentShortExposure = minExposureTime;
    }

    pSensorParam->currentLineCount = GetSensorDataObject()->ExposureToLineCount(pSensorParam->currentExposure,
                                                                                m_currentResolutionIndex,
                                                                                currentSeamlessType);

    pSensorParam->currentMiddleLineCount = GetSensorDataObject()->ExposureToLineCount(pSensorParam->currentMiddleExposure,
                                                                                      m_currentResolutionIndex,
                                                                                      currentSeamlessType);

    pSensorParam->currentShortLineCount = GetSensorDataObject()->ExposureToLineCount(pSensorParam->currentShortExposure,
                                                                                     m_currentResolutionIndex,
                                                                                     currentSeamlessType);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "AppliedExpTime=%llu", pSensorParam->currentExposure);

    if (pSensorParam->currentLineCount > pExposureControlInfo->maxLineCount)
    {
        pSensorParam->currentLineCount = pExposureControlInfo->maxLineCount;
        /// Updating the actual exposure in case if line count is capped to maxLineCount.
        pSensorParam->currentExposure = GetSensorDataObject()->LineCountToExposure(pSensorParam->currentLineCount,
                                                                                   m_currentResolutionIndex,
                                                                                   currentSeamlessType);
    }

    if (pSensorParam->currentMiddleLineCount > pExposureControlInfo->maxLineCount)
    {
        pSensorParam->currentMiddleLineCount = pExposureControlInfo->maxLineCount;
        pSensorParam->currentMiddleExposure = GetSensorDataObject()->LineCountToExposure(pSensorParam->currentMiddleLineCount,
                                                                                         m_currentResolutionIndex,
                                                                                         currentSeamlessType);
    }

    if (pSensorParam->currentShortLineCount > pExposureControlInfo->maxLineCount)
    {
        pSensorParam->currentShortLineCount = pExposureControlInfo->maxLineCount;
        pSensorParam->currentShortExposure = GetSensorDataObject()->LineCountToExposure(pSensorParam->currentShortLineCount,
                                                                                        m_currentResolutionIndex,
                                                                                        currentSeamlessType);
    }

    UINT32 frameLengthLines = pSensorParam->currentFrameLengthLines;
    frameLengthLines = static_cast<UINT32>(Utils::RoundDOUBLE(frameLengthLines * GetFpsDiv(pSensorParam->isFSModeCapture)));

    if ((TRUE == GetStaticSettings()->enableSensorFpsMatch) && (TRUE == m_isMultiCameraUsecase) &&
        (FALSE == m_isMasterCamera) && (NULL != pNodeProcessRequestData))
    {
        pSensorParam->currentFrameLengthLines = frameLengthLines;
        AdjustSlaveCameraExposureInfo(pSensorParam, pNodeProcessRequestData);
        frameLengthLines = pSensorParam->currentFrameLengthLines;
    }

    /// Make sure that minumum of vertical offset is maintained between line count and frame length lines
    if (pSensorParam->currentLineCount > (frameLengthLines - pExposureControlInfo->verticalOffset))
    {
        frameLengthLines = pSensorParam->currentLineCount + pExposureControlInfo->verticalOffset;
    }
    pSensorParam->currentFrameLengthLines = frameLengthLines;

    if (pSensorParam->currentFrameLengthLines <
        GetSensorDataObject()->GetFrameLengthLines(m_currentResolutionIndex, currentSeamlessType))
    {
        /// Don't return error, just report it to the log and ask developer to check if it is expected.
        /// The issue may result in the adjusted fps is bigger than slave's max supported fps on multi
        /// camera fps match feature.
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Serious problems, please check it ASAP, current FLL:%d default FLL:%d",
            pSensorParam->currentFrameLengthLines,
            GetSensorDataObject()->GetFrameLengthLines(m_currentResolutionIndex, currentSeamlessType));
        pSensorParam->currentFrameLengthLines =
            GetSensorDataObject()->GetFrameLengthLines(m_currentResolutionIndex, currentSeamlessType);
    }

    // Apply HAL override of frame duration if provided
    UINT32 metaTagDisableFPSLimits            = 0;
    UINT32 metaTagOverrideSensorFrameDuration = 0;
    VendorTagManager::QueryVendorTagLocation("org.quic.camera.manualExposure",
                                             "disableFPSLimits",
                                             &metaTagDisableFPSLimits);
    VendorTagManager::QueryVendorTagLocation("org.quic.camera.manualExposure",
                                             "overrideSensorFrameDuration",
                                             &metaTagOverrideSensorFrameDuration);

    UINT metaTagFpsOverrideData[] =
    {
        metaTagDisableFPSLimits            | InputMetadataSectionMask,  // 0
        metaTagOverrideSensorFrameDuration | InputMetadataSectionMask   // 1
    };
    const UINT32        pInputDataSize                   = CAMX_ARRAY_SIZE(metaTagFpsOverrideData);
    VOID*               pInputData[pInputDataSize]       = { 0 };
    UINT64              pInputDataOffset[pInputDataSize] = { 0 };

    GetDataList(metaTagFpsOverrideData, pInputData, pInputDataOffset, pInputDataSize);
    BOOL   disableFPSLimits        = (NULL != pInputData[0] ? *(static_cast<BYTE*>(pInputData[0]))   : FALSE);
    UINT64 overrideFrameDurationNs = (NULL != pInputData[1] ? *(static_cast<UINT64*>(pInputData[1])) : 0);

    if ((TRUE == disableFPSLimits) && (0 != overrideFrameDurationNs))
    {
        // override frameLengthLines (aka frame-duration)
        pSensorParam->currentFrameLengthLines = GetSensorDataObject()->ExposureToLineCount(overrideFrameDurationNs,
                                                                                           m_currentResolutionIndex,
                                                                                           currentSeamlessType);
        CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                "Hal Override: frameLengthLines %u (min/max %u %u)",
                pSensorParam->currentFrameLengthLines,
                GetSensorDataObject()->GetFrameLengthLines(m_currentResolutionIndex, currentSeamlessType),
                GetSensorDataObject()->GetMaxFrameLengthLines(m_currentResolutionIndex, m_cameraId));

        // override lineCount (aka exposure) if needed to avoid invalid exposure length
        if (pSensorParam->currentLineCount > (pSensorParam->currentFrameLengthLines - pExposureControlInfo->verticalOffset))
        {
            pSensorParam->currentLineCount = pSensorParam->currentFrameLengthLines - pExposureControlInfo->verticalOffset;
            CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                "Hal Override: currentLineCount %u (ffl %u vertOffset %u)",
                pSensorParam->currentLineCount,
                pSensorParam->currentFrameLengthLines,
                pExposureControlInfo->verticalOffset);
        }
    }

    // The result is used to publish to other nodes
    GetSensorDataObject()->CalculateExposure(pSensorParam->currentGain,
                                             pSensorParam->currentLineCount,
                                             pSensorParam->currentMiddleGain,
                                             pSensorParam->currentMiddleLineCount,
                                             pSensorParam->currentShortGain,
                                             pSensorParam->currentShortLineCount,
                                             m_currentResolutionIndex,
                                             m_pExposureInfo);

    m_appliedSensorParam.currentGain             = pSensorParam->currentGain;
    m_appliedSensorParam.currentLineCount        = pSensorParam->currentLineCount;
    m_appliedSensorParam.currentShortGain        = pSensorParam->currentShortGain;
    m_appliedSensorParam.currentFrameLengthLines = pSensorParam->currentFrameLengthLines;
    m_appliedSensorParam.currentShortLineCount   = pSensorParam->currentShortLineCount;
    m_appliedSensorParam.currentMiddleGain       = pSensorParam->currentMiddleGain;
    m_appliedSensorParam.currentMiddleLineCount  = pSensorParam->currentMiddleLineCount;

    isDelayed = HandleDelayInfo(&m_appliedSensorParam, pNodeProcessRequestData);

    if (TRUE == isDelayed)
    {
        // The result is used to apply for current request
        GetSensorDataObject()->CalculateExposure(m_appliedSensorParam.currentGain,
                                                 m_appliedSensorParam.currentLineCount,
                                                 m_appliedSensorParam.currentMiddleGain,
                                                 m_appliedSensorParam.currentMiddleLineCount,
                                                 m_appliedSensorParam.currentShortGain,
                                                 m_appliedSensorParam.currentShortLineCount,
                                                 m_currentResolutionIndex,
                                                 reinterpret_cast<VOID*>(&appliedExposureInfo));
    }
    else
    {
        appliedExposureInfo = *reinterpret_cast<SensorExposureInfo*>(m_pExposureInfo);
    }

    if (NULL != pSensorParam->pRegControlData)
    {
        pRegControlInfo = reinterpret_cast<VOID*>(pSensorParam->pRegControlData->sensorControl.registerControl);
    }

    BOOL applyShortExposure  = ((GetSensorDataObject()->IsHDRMode(m_currentResolutionIndex)) ||
                                (TRUE == IsApplyShortExposureNeeded(currentSeamlessType)));
    BOOL applyMiddleExposure = ((GetSensorDataObject()->Is3ExposureHDRMode(m_currentResolutionIndex)) ||
                                (TRUE == IsApplyMiddleExposureNeeded(currentSeamlessType)));

    GetSensorDataObject()->FillExposureArray(reinterpret_cast<VOID*>(&appliedExposureInfo),
                                             m_appliedSensorParam.currentFrameLengthLines,
                                             pRegControlInfo,
                                             m_pExposureRegAddressInfo,
                                             m_pRegSettings,
                                             applyShortExposure,
                                             applyMiddleExposure,
                                             m_currentResolutionIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::PrepareSensorPDAFUpdate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SensorNode::PrepareSensorPDAFUpdate(
    PDLibWindowConfig* pPDAFWindowConfig)
{
    ResolutionData* pResolutionData = &(GetSensorDataObject()->GetResolutionInfo())->resolutionData[m_currentResolutionIndex];
    UINT sensorWidth                = pResolutionData->streamInfo.streamConfiguration[0].frameDimension.width;
    UINT sensorHeight               = pResolutionData->streamInfo.streamConfiguration[0].frameDimension.height;

    CAMX_ASSERT(NULL != m_pPDAFSettings);

    // PDAF window mode fixed, default, floating support to be added later
    // now configuring only to fixed window
    m_sensorPdafData.PDAFWindowMode        = 0;
    m_sensorPdafData.PDAFroiType           = pPDAFWindowConfig->roiType;
    m_sensorPdafData.PDAFstartX            = static_cast<UINT>((pPDAFWindowConfig->fixedAFWindow.startX) * sensorWidth);
    m_sensorPdafData.PDAFstartY            = static_cast<UINT>((pPDAFWindowConfig->fixedAFWindow.startY) * sensorHeight);
    m_sensorPdafData.PDAFendX              = static_cast<UINT>((pPDAFWindowConfig->fixedAFWindow.endX) * sensorWidth);
    m_sensorPdafData.PDAFendY              = static_cast<UINT>((pPDAFWindowConfig->fixedAFWindow.endY) *sensorHeight);
    m_sensorPdafData.horizontalWindowCount = pPDAFWindowConfig->horizontalWindowCount;
    m_sensorPdafData.verticalWindowCount   = pPDAFWindowConfig->verticalWindowCount;

    if (NULL != m_pPDAFSettings)
    {
        m_isPdafUpdated = GetSensorDataObject()->FillPDAFArray(&m_sensorPdafData, m_pPDAFSettings);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "m_pPDAFSettings not Available");
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::PrepareSensorWBGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SensorNode::PrepareSensorWBGain(
    AWBGainParams* pWBGainConfig)
{
    CAMX_ASSERT(NULL != m_pWBGainSettings);

    if (NULL != pWBGainConfig)
    {
        m_sensorWBGainData.RGain      = pWBGainConfig->rGain;
        m_sensorWBGainData.GGain      = pWBGainConfig->gGain;
        m_sensorWBGainData.BGain      = pWBGainConfig->bGain;

        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "rgain=%f, ggain=%f, bgain=%f",
            m_sensorWBGainData.RGain, m_sensorWBGainData.GGain, m_sensorWBGainData.BGain);
    }

    if (NULL != m_pWBGainSettings)
    {
        m_isWBGainUpdated = GetSensorDataObject()->FillWBGainArray(&m_sensorWBGainData, m_pWBGainSettings);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "m_pWBGainSettings not Available");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::ApplySensorUpdate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::ApplySensorUpdate(
    UINT64  requestId)
{
    CamxResult              result                  = CamxResultSuccess;
    BOOL                    isNeedUpdateAEC         = FALSE;
    BOOL                    isNeedUpdateSyncMode    = FALSE;
    BOOL                    isNeedUpdatePDAF        = FALSE;
    BOOL                    isNeedUpdateWBGain      = FALSE;
    BOOL                    isNeedUpdateLTCRatio    = FALSE;
    VOID*                   pSyncMode               = NULL;
    SensorSyncMode          syncMode                = NoSync;
    CSLPacketOpcodesSensor  opcode;

    if ((NULL == m_pUpdatePacketManager) || (NULL == m_pUpdateCmdBufferManager) || (NULL == m_pRegSettings))
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor,
                       "Invalid input(s): packetManager: %p, cmdbufferManager: %p, RegSettings: %p",
                       m_pUpdatePacketManager,
                       m_pUpdateCmdBufferManager,
                       m_pRegSettings);

        return CamxResultEInvalidPointer;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                    "ReqId=%llu gain: %f, lineCount: %d, middleGain: %f, middleLinecount: %d, "
                    "shortGain: %f, shortLinecount: %d, FLL: %d",
                    requestId,
                    m_sensorParam.currentGain,
                    m_sensorParam.currentLineCount,
                    m_sensorParam.currentMiddleGain,
                    m_sensorParam.currentMiddleLineCount,
                    m_sensorParam.currentShortGain,
                    m_sensorParam.currentShortLineCount,
                    m_sensorParam.currentFrameLengthLines);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor,
        "ReqId=%llu previousGain: %f, previousLineCount: %u, previousFLL:%u",
        requestId,
        m_sensorParam.previousGain,
        m_sensorParam.previousLineCount,
        m_sensorParam.previousFrameLengthLines);

    m_pUpdatePacket = GetPacketForRequest(requestId, m_pUpdatePacketManager);
    m_pUpdateCmds   = GetCmdBufferForRequest(requestId, m_pUpdateCmdBufferManager);

    CAMX_ASSERT(NULL != m_pUpdatePacket);
    CAMX_ASSERT(NULL != m_pUpdateCmds);

    if ((NULL == m_pUpdatePacket) || (NULL == m_pUpdateCmds))
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "NULL input packet or command buffer!");
        return CamxResultENoMemory;
    }

    syncMode = m_currentSyncMode;

    if (TRUE == m_isMultiCameraUsecase)
    {
        syncMode = GetPipeline()->GetSensorSyncMode();
    }

    if (syncMode != m_currentSyncMode)
    {
        UINT dualCmdSize = 0;
        I2CRegSettingType i2cSettingType = I2CRegSettingType::Master;
        UINT regSettingIdx = 0;

        if (syncMode == MasterMode)
        {
            i2cSettingType = I2CRegSettingType::Master;
            dualCmdSize = GetSensorDataObject()->GetI2CCmdSize(I2CRegSettingType::Master, NULL, 0, &regSettingIdx);
        }
        else if (syncMode == SlaveMode)
        {
            i2cSettingType = I2CRegSettingType::Slave;
            dualCmdSize = GetSensorDataObject()->GetI2CCmdSize(I2CRegSettingType::Slave, NULL, 0, &regSettingIdx);
        }
        else if (syncMode == NoSync && m_currentSyncMode == SlaveMode)
        {
            i2cSettingType = I2CRegSettingType::Master;
            dualCmdSize = GetSensorDataObject()->GetI2CCmdSize(I2CRegSettingType::Master, NULL, 0, &regSettingIdx);
        }

        CAMX_LOG_INFO(CamxLogGroupSensor, "requestId: %llu, dualMode: %d,dualcmdSize:%d",
            requestId, syncMode, dualCmdSize);
        if (dualCmdSize != 0)
        {
            VOID* pDualUpdate = m_pUpdateCmds->BeginCommands(dualCmdSize / sizeof(UINT32));
            if (pDualUpdate != NULL)
            {
                result = GetSensorDataObject()->CreateI2CCmd(i2cSettingType, pDualUpdate, NULL, 0, regSettingIdx);
                if (CamxResultSuccess == result)
                {
                    result = m_pUpdateCmds->CommitCommands();
                    isNeedUpdateSyncMode = TRUE;
                }
            }
        }
    }

    if ((SeamlessInSensorState::InSensorHDR3ExpStart == m_seamlessInSensorState) ||
        (SeamlessInSensorState::InSensorHDR3ExpStop  == m_seamlessInSensorState))
    {
        UINT                inSensorHDR3ExpCmdSize = 0;
        UINT                regSettingIdx          = 0;
        I2CRegSettingType   i2cSettingType         = I2CRegSettingType::InSensorHDR3ExpOff;

        if (SeamlessInSensorState::InSensorHDR3ExpStart == m_seamlessInSensorState)
        {
            i2cSettingType = I2CRegSettingType::InSensorHDR3ExpOn;
        }
        else
        {
            i2cSettingType = I2CRegSettingType::InSensorHDR3ExpOff;
        }

        inSensorHDR3ExpCmdSize = GetSensorDataObject()->GetI2CCmdSize(i2cSettingType,
                                                                      NULL,
                                                                      m_currentResolutionIndex,
                                                                      &regSettingIdx);

        if (0 != inSensorHDR3ExpCmdSize)
        {
            CAMX_LOG_INFO(CamxLogGroupSensor, "Seamless in-sensor HDR 3 Exposure, requestId: %llu, cameraId %d,"
                          "inSensorHDR3ExpCmdSize:%d, m_seamlessInSensorState=%u",
                          requestId, m_cameraId, inSensorHDR3ExpCmdSize, m_seamlessInSensorState);
            VOID* pInSensorHDR3ExpUpdate = m_pUpdateCmds->BeginCommands(inSensorHDR3ExpCmdSize / sizeof(UINT32));
            if (pInSensorHDR3ExpUpdate != NULL)
            {
                result = GetSensorDataObject()->CreateI2CCmd(i2cSettingType,
                                                             pInSensorHDR3ExpUpdate,
                                                             NULL,
                                                             m_currentResolutionIndex,
                                                             regSettingIdx);
                if (CamxResultSuccess == result)
                {
                    result = m_pUpdateCmds->CommitCommands();
                }
            }
        }
    }

    if ((((m_appliedSensorParam.previousGain != m_appliedSensorParam.currentGain) ||
         (m_appliedSensorParam.previousLineCount != m_appliedSensorParam.currentLineCount) ||
         (m_appliedSensorParam.previousShortGain != m_appliedSensorParam.currentShortGain) ||
         (m_appliedSensorParam.previousFrameLengthLines != m_appliedSensorParam.currentFrameLengthLines) ||
         (m_appliedSensorParam.previousShortLineCount != m_appliedSensorParam.currentShortLineCount) ||
         (m_appliedSensorParam.previousMiddleGain != m_appliedSensorParam.currentMiddleGain) ||
         (m_appliedSensorParam.previousMiddleLineCount != m_appliedSensorParam.currentMiddleLineCount)) &&
         (m_appliedSensorParam.currentLineCount != 0)) ||
        (SeamlessInSensorState::InSensorHDR3ExpStart == m_seamlessInSensorState))
    {
        UINT regSettingIdx = 0;
        UINT exposureUpdateCmdSize = GetSensorDataObject()->GetI2CCmdSize(I2CRegSettingType::AEC,
                                     m_pRegSettings, 0, &regSettingIdx);

        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "requestId: %llu, exposureUpdateCmdSize: %d", requestId, exposureUpdateCmdSize);

        CAMX_ASSERT(0 != exposureUpdateCmdSize);
        CAMX_ASSERT(0 == (exposureUpdateCmdSize % sizeof(UINT32)));

        if (0 != exposureUpdateCmdSize)
        {
            VOID* pCmdBegin = m_pUpdateCmds->BeginCommands(exposureUpdateCmdSize / sizeof(UINT32));
            if (NULL != pCmdBegin)
            {
                CSLSensorI2CRandomWriteCmd* pCmdExpUpdate = reinterpret_cast<CSLSensorI2CRandomWriteCmd*>(pCmdBegin);

                result = GetSensorDataObject()->CreateI2CCmd(I2CRegSettingType::AEC, pCmdExpUpdate,
                                            m_pRegSettings, 0, regSettingIdx);
                if (CamxResultSuccess == result)
                {
                    result = m_pUpdateCmds->CommitCommands();
                    isNeedUpdateAEC = TRUE;
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to reserve exposure update command buffer");
            }
        }
    }

    if ((m_prevSensorPdafData.PDAFroiType != m_sensorPdafData.PDAFroiType) ||
        (m_prevSensorPdafData.PDAFstartX != m_sensorPdafData.PDAFstartX) ||
        (m_prevSensorPdafData.PDAFstartY != m_sensorPdafData.PDAFstartY) ||
        (m_prevSensorPdafData.PDAFendX != m_sensorPdafData.PDAFendX) ||
        (m_prevSensorPdafData.PDAFendY != m_sensorPdafData.PDAFendY) ||
        (m_prevSensorPdafData.horizontalWindowCount != m_sensorPdafData.horizontalWindowCount) ||
        (m_prevSensorPdafData.verticalWindowCount != m_sensorPdafData.verticalWindowCount))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Prev [%d %d %d %d] Present [%d %d %d %d]",
                                            m_prevSensorPdafData.PDAFstartX,
                                            m_prevSensorPdafData.PDAFstartY,
                                            m_prevSensorPdafData.PDAFendX,
                                            m_prevSensorPdafData.PDAFendY,
                                            m_sensorPdafData.PDAFstartX,
                                            m_sensorPdafData.PDAFstartY,
                                            m_sensorPdafData.PDAFendX,
                                            m_sensorPdafData.PDAFendY);


        UINT regSettingIdx = 0;
        UINT pdafUpdateCmdSize = GetSensorDataObject()->GetI2CCmdSize(I2CRegSettingType::PDAF,
                                m_pPDAFSettings, 0, &regSettingIdx);

        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "requestId: %llu, pdafUpdateCmdSize: %d", requestId, pdafUpdateCmdSize);

        CAMX_ASSERT(0 == (pdafUpdateCmdSize % sizeof(UINT32)));

        if (0 != pdafUpdateCmdSize)
        {
            VOID* pCmdBegin = m_pUpdateCmds->BeginCommands(pdafUpdateCmdSize / sizeof(UINT32));
            if (NULL != pCmdBegin)
            {
                CSLSensorI2CRandomWriteCmd* pCmdPDAFUpdate = reinterpret_cast<CSLSensorI2CRandomWriteCmd*>(pCmdBegin);

                result = GetSensorDataObject()->CreateI2CCmd(I2CRegSettingType::PDAF,
                                  pCmdPDAFUpdate, m_pPDAFSettings, 0, regSettingIdx);

                if (CamxResultSuccess == result)
                {
                    result = m_pUpdateCmds->CommitCommands();
                    isNeedUpdatePDAF = TRUE;
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to reserve PDAF update command buffer");
            }
        }
    }

    if ((m_prevSensorWBGainData.RGain != m_sensorWBGainData.RGain) ||
        (m_prevSensorWBGainData.GGain != m_sensorWBGainData.GGain) ||
        (m_prevSensorWBGainData.BGain != m_sensorWBGainData.BGain))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Prev [%f %f %f] Present [%f %f %f]",
                                            m_prevSensorWBGainData.RGain,
                                            m_prevSensorWBGainData.GGain,
                                            m_prevSensorWBGainData.BGain,
                                            m_sensorWBGainData.RGain,
                                            m_sensorWBGainData.GGain,
                                            m_sensorWBGainData.BGain);

        UINT regSettingIdx = 0;

        UINT awbGainUpdateCmdSize = GetSensorDataObject()->GetI2CCmdSize(I2CRegSettingType::AWB,
                                   m_pWBGainSettings, 0, &regSettingIdx);

        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "requestId: %llu, awbGainUpdateCmdSize: %d", requestId, awbGainUpdateCmdSize);

        CAMX_ASSERT(0 == (awbGainUpdateCmdSize % sizeof(UINT32)));

        if (0 != awbGainUpdateCmdSize)
        {
            VOID* pCmdBegin = m_pUpdateCmds->BeginCommands(awbGainUpdateCmdSize / sizeof(UINT32));
            if (NULL != pCmdBegin)
            {
                CSLSensorI2CRandomWriteCmd* pCmdWBGainUpdate = reinterpret_cast<CSLSensorI2CRandomWriteCmd*>(pCmdBegin);

                result = GetSensorDataObject()->CreateI2CCmd(I2CRegSettingType::AWB,
                                  pCmdWBGainUpdate, m_pWBGainSettings, 0, regSettingIdx);

                if (CamxResultSuccess == result)
                {
                    result = m_pUpdateCmds->CommitCommands();
                    isNeedUpdateWBGain = TRUE;
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to reserve AWB update command buffer");
            }
        }
    }

    // LTC ratio index
    if (SeamlessInSensorState::InSensorHDR3ExpStart == m_seamlessInSensorState)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "LTC Ratio: The latest LTC ratio index = 0x%02x", m_latestLTCRatioData);

        UINT regSettingIdx         = 0;
        UINT ltcRatioUpdateCmdSize = GetSensorDataObject()->GetI2CCmdSize(I2CRegSettingType::LTC,
                                                                          m_pLTCRatioSettings,
                                                                          0,
                                                                          &regSettingIdx);

        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "requestId: %llu, ltcRatioUpdateCmdSize: %d", requestId, ltcRatioUpdateCmdSize);

        if (0 != ltcRatioUpdateCmdSize)
        {
            VOID* pCmdBegin = m_pUpdateCmds->BeginCommands(ltcRatioUpdateCmdSize / sizeof(UINT32));
            if (NULL != pCmdBegin)
            {
                CSLSensorI2CRandomWriteCmd* pCmdLTCRatioUpdate = reinterpret_cast<CSLSensorI2CRandomWriteCmd*>(pCmdBegin);

                result = GetSensorDataObject()->CreateI2CCmd(I2CRegSettingType::LTC,
                                                             pCmdLTCRatioUpdate,
                                                             m_pLTCRatioSettings,
                                                             0,
                                                             regSettingIdx);

                if (CamxResultSuccess == result)
                {
                    result = m_pUpdateCmds->CommitCommands();
                    isNeedUpdateLTCRatio = TRUE;
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to reserve LTC update command buffer");
            }
        }
    }

    if ((CamxResultSuccess == result) &&
        (isNeedUpdateSyncMode || isNeedUpdateAEC || isNeedUpdatePDAF || isNeedUpdateWBGain || isNeedUpdateLTCRatio))
    {
        m_pUpdateCmds->SetRequestId(GetCSLSyncId(requestId));

        opcode = CSLPacketOpcodesSensorUpdate;
        if (FALSE == IsPipelineStreamedOn())
        {
            if (TRUE == m_initialConfigPending)
            {
                opcode                 = CSLPacketOpcodesSensorConfig;
                m_initialConfigPending = FALSE;
            }
        }
        else
        {
            m_initialConfigPending = TRUE;
        }

        m_pUpdatePacket->SetOpcode(CSLDeviceTypeInvalidDevice, opcode);

        result = m_pUpdatePacket->AddCmdBufferReference(m_pUpdateCmds, NULL);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Sensor sending NOP packet");

        m_pUpdatePacket->SetOpcode(CSLDeviceTypeInvalidDevice, CSLPacketOpcodesNop);
        if (FALSE == IsPipelineStreamedOn())
        {
            if (TRUE == m_initialConfigPending)
            {
                m_initialConfigPending = FALSE;
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        m_pUpdatePacket->SetRequestId(GetCSLSyncId(requestId));

        result = m_pUpdatePacket->CommitPacket();

        if (CamxResultSuccess == result)
        {
            CAMX_LOG_INFO(CamxLogGroupSensor, "submit exp update: requestID: %llu", requestId);
            result = m_pHwContext->Submit(GetCSLSession(), m_hSensorDevice, m_pUpdatePacket);
        }
    }

    if (CamxResultSuccess == result)
    {
        m_currentSyncMode = syncMode;
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Sensor per frame exposure update successful!");
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Sensor per frame exposure update failed: %d", Utils::CamxResultToString(result));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::GetCurrentFPS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DOUBLE SensorNode::GetCurrentFPS()
{
    DOUBLE currentFPS       = GetSensorDataObject()->GetMaxFPS(m_currentResolutionIndex);
    UINT32 frameLengthLines = GetSensorDataObject()->GetFrameLengthLines(m_currentResolutionIndex, SensorSeamlessType::None);

    if (m_sensorParam.currentLineCount > frameLengthLines)
    {
        currentFPS = (currentFPS * frameLengthLines / m_sensorParam.currentLineCount);
    }

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Configured current FPS = %lf", currentFPS);

    return currentFPS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::IsFSModeEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::IsFSModeEnabled(
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

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::IsFSCaptureRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SensorNode::IsFSCaptureRequest()
{
    BOOL isFSCapture = FALSE;
    UINT32 i         = 0;

    UINT   HALInputCaptureIntent[] =
    {
        InputControlCaptureIntent
    };
    static const UINT Length                 = CAMX_ARRAY_SIZE(HALInputCaptureIntent);
    VOID* pData[Length]                      = { 0 };
    UINT64 pDataCount[Length]                = { 0 };
    ControlCaptureIntentValues captureIntent = ControlCaptureIntentEnd;

    if (TRUE == m_isFSModeEnabled)
    {
        GetDataList(HALInputCaptureIntent, pData, pDataCount, 1);

        if (NULL != pData[0])
        {
            captureIntent = *(static_cast<ControlCaptureIntentValues*>(pData[0]));
        }

        if (ControlCaptureIntentStillCapture == captureIntent)
        {
            isFSCapture = TRUE;
        }
    }

    return isFSCapture;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::GetFpsDiv
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DOUBLE SensorNode::GetFpsDiv(
    BOOL isFSCapture)
{
    INT32  HALFpsRange[2] = { 0 };
    DOUBLE HALmaxFPS      = 0;
    DOUBLE fps_div        = 1.0;
    DOUBLE maxFPS         = static_cast<DOUBLE>(Utils::RoundDOUBLE(GetSensorDataObject()->GetMaxFPS(m_currentResolutionIndex)));
    UINT   HALInputParams[] =
    {
        InputControlAETargetFpsRange,
    };
    static const UINT Length                 = CAMX_ARRAY_SIZE(HALInputParams);
    VOID* pData[Length]                      = { 0 };
    UINT64 pDataCount[Length]                = { 0 };
    CamxResult result                        = CamxResultEFailed;

    GetDataList(HALInputParams, pData, pDataCount, Length);

    if (NULL != pData[0])
    {
        Utils::Memcpy(&HALFpsRange, pData[0], sizeof(HALFpsRange));
    }
    HALmaxFPS = HALFpsRange[1];

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "HAL FPS min:max = %d:%d currentFPS = %lf", HALFpsRange[0], HALFpsRange[1], maxFPS);

    if (HALmaxFPS > maxFPS)
    {
        HALmaxFPS = maxFPS;
    }

    if (HALmaxFPS != 0)
    {
        if (TRUE == isFSCapture)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                "Still capture in Fast shutter mode. Don't alter max FPS");
        }
        else
        {
            fps_div = maxFPS / HALmaxFPS;
        }
    }

    return fps_div;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::GetInitialCameraInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::GetInitialCameraInfo(
    StatsCameraInfo* pCameraInfo)
{
    CamxResult result   = CamxResultSuccess;
    UINT cameraPosition = 0;

    pCameraInfo->cameraId = m_cameraId;
    result                = m_pSensorModuleData->GetCameraPosition(&cameraPosition);

    // Get initial camera information
    if (CamxResultSuccess == result && (TRUE == GetStaticSettings()->multiCameraEnable) &&
        (MultiCamera3ASyncDisabled != GetStaticSettings()->multiCamera3ASync))
    {
        // Convert CameraPosition to CHISENSORPOSITIONTYPE
        cameraPosition += 1;
        if (REAR == cameraPosition)
        {
            pCameraInfo->algoRole   = StatsAlgoRoleMaster;
        }
        else if (REAR_AUX == cameraPosition)
        {
            pCameraInfo->algoRole   = StatsAlgoRoleSlave;
        }
        else
        {
            pCameraInfo->algoRole   = StatsAlgoRoleDefault;
        }
    }
    else
    {
        pCameraInfo->algoRole   = StatsAlgoRoleDefault;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupStats, "Sensor initial camera info: ID:%d role:%d",
        pCameraInfo->cameraId,
        pCameraInfo->algoRole);

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::AcquireDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::AcquireDevice()
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupSensor, SCOPEEventSensorAcquire);

    CamxResult  result             = CamxResultEFailed;
    SubDeviceProperty sensorDevice = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, SensorHandle);
    SubDeviceProperty csiphyDevice = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, CSIPHYHandle);

    CAMX_LOG_INFO(CamxLogGroupSensor, "m_sensorDeviceIndex:%d, m_CSIPHYDeviceIndex: %d",
                                       sensorDevice.deviceIndex, csiphyDevice.deviceIndex);

    if (FALSE == sensorDevice.isAcquired)
    {
        CAMX_LOG_INFO(CamxLogGroupSensor, "Acquiring sensor device handle");

        result = CSLAcquireDevice(GetCSLSession(),
                                  &m_hSensorDevice,
                                  sensorDevice.deviceIndex,
                                  NULL,
                                  0,
                                  NULL,
                                  0,
                                  NodeIdentifierString());

        if (CamxResultSuccess == result)
        {
            m_pSensorSubDevicesCache->SetSubDeviceHandle(GetCSLSession(), m_cameraId, m_hSensorDevice, SensorHandle);
        }
    }
    else
    {
        CAMX_ASSERT(sensorDevice.hDevice != CSLInvalidHandle);
        m_hSensorDevice = sensorDevice.hDevice;
        CAMX_LOG_INFO(CamxLogGroupSensor, "Reusing sensor device handle: %p for camerId: %d",
                                           m_hSensorDevice, m_cameraId);
        m_sensorConfigStatus = SensorConfigurationStatus::SensorInitializationComplete;
        result = CamxResultSuccess;
    }


    if (CamxResultSuccess == result)
    {
        SetDeviceAcquired(TRUE);
        AddCSLDeviceHandle(m_hSensorDevice);
        CAMX_LOG_INFO(CamxLogGroupSensor, "AcquireDevice on Sensor successful: handle=0x%x", m_hSensorDevice);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "AcquireDevice on Sensor failed: %d", Utils::CamxResultToString(result));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::IsStatsNodeEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SensorNode::IsStatsNodeEnabled()
{
    BOOL isEnabled = TRUE;
    const StaticSettings* pStaticSettings = GetStaticSettings();

    if ( FALSE == GetPipeline()->HasStatsNode() )
    {
        isEnabled = FALSE;
    }

    return isEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::SensorThreadJobCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* SensorNode::SensorThreadJobCallback(
    VOID* pData)
{
    CamxResult  result            = CamxResultEFailed;
    SensorPostJob* pSensorPostJob = reinterpret_cast<SensorPostJob*>(pData);

    if ((NULL != pSensorPostJob) && (NULL != pSensorPostJob->pSensor))
    {
        SensorNode* pSensorNode          = static_cast<SensorNode*>(pSensorPostJob->pSensor);
        SensorPostJobCommand condition   = pSensorPostJob->sensorJobCommand;

        switch (condition)
        {
            case SensorPostJobCommand::InitializeSensor:
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "InitializeSensor: start");
                pSensorNode->m_signalSensorInit.pMutex->Lock();
                result = pSensorNode->LoadSensorInitConfigCmd();
                pSensorNode->m_signalSensorInit.pWaitCondition->Broadcast();
                pSensorNode->m_signalSensorInit.pMutex->Unlock();

                UINT32* pUserData = reinterpret_cast<UINT32*>(pSensorPostJob->pData);
                if (pUserData != NULL)
                {
                    CAMX_DELETE(pUserData);
                    pUserData = NULL;
                }

                if (pSensorPostJob != NULL)
                {
                    CAMX_DELETE(pSensorPostJob);
                    pSensorPostJob = NULL;
                }

                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "InitializeSensor: done");
                break;
            }
            case SensorPostJobCommand::SubModulesCreate:
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "SubModulesCreate: start");
                pSensorNode->m_signalSensorInit.pMutex->Lock();
                if ((SensorConfigurationStatus::SensorInitializationFailed != pSensorNode->m_sensorConfigStatus) &&
                    (SensorConfigurationStatus::SensorInitializationComplete != pSensorNode->m_sensorConfigStatus))
                {
                    CAMX_LOG_INFO(CamxLogGroupSensor, "SubModulesCreate: Waiting for sensor init to complete");
                    pSensorNode->m_signalSensorInit.pWaitCondition->Wait(
                        pSensorNode->m_signalSensorInit.pMutex->GetNativeHandle());
                }
                pSensorNode->m_signalSensorInit.pMutex->Unlock();

                pSensorNode->m_signalSensorSubModules.pMutex->Lock();
                result = pSensorNode->CreateSensorSubmodules();
                pSensorNode->m_signalSensorSubModules.pWaitCondition->Broadcast();
                pSensorNode->m_signalSensorSubModules.pMutex->Unlock();

                UINT32* pUserData = reinterpret_cast<UINT32*>(pSensorPostJob->pData);
                if (pUserData != NULL)
                {
                    CAMX_DELETE(pUserData);
                    pUserData = NULL;
                }

                if (pSensorPostJob != NULL)
                {
                    CAMX_DELETE(pSensorPostJob);
                    pSensorPostJob = NULL;
                }

                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "SubModulesCreate: done");
                break;
            }
            case SensorPostJobCommand::ConfigureSensor:
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "ConfigureSensor: start");
                pSensorNode->m_signalSensorSubModules.pMutex->Lock();
                if ((SensorConfigurationStatus::SensorSubModuleCreateFailed != pSensorNode->m_sensorConfigStatus) &&
                    (SensorConfigurationStatus::SensorSubModuleCreateComplete != pSensorNode->m_sensorConfigStatus))
                {
                    CAMX_LOG_INFO(CamxLogGroupSensor, "ConfigureSensor: Waiting for sub module create to complete");
                    pSensorNode->m_signalSensorSubModules.pWaitCondition->Wait(
                        pSensorNode->m_signalSensorSubModules.pMutex->GetNativeHandle());
                }
                pSensorNode->m_signalSensorSubModules.pMutex->Unlock();

                pSensorNode->m_signalSensorConfig.pMutex->Lock();
                result = pSensorNode->LoadSensorConfigCmds();
                pSensorNode->m_signalSensorConfig.pWaitCondition->Broadcast();
                pSensorNode->m_signalSensorConfig.pMutex->Unlock();

                UINT32* pUserData = reinterpret_cast<UINT32*>(pSensorPostJob->pData);
                if (pUserData != NULL)
                {
                    CAMX_DELETE(pUserData);
                    pUserData = NULL;
                }

                if (pSensorPostJob != NULL)
                {
                    CAMX_DELETE(pSensorPostJob);
                    pSensorPostJob = NULL;
                }
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "ConfigureSensor: done");

                break;
            }
            case SensorPostJobCommand::InitializeOIS:
            {
                pSensorNode->m_signalOISInit.pMutex->Lock();
                result = pSensorNode->CreateOISSubmodule();
                pSensorNode->m_signalOISInit.pWaitCondition->Broadcast();
                pSensorNode->m_signalOISInit.pMutex->Unlock();

                UINT32* pUserData = reinterpret_cast<UINT32*>(pSensorPostJob->pData);
                if (pUserData != NULL)
                {
                    CAMX_DELETE(pUserData);
                    pUserData = NULL;
                }

                if (pSensorPostJob != NULL)
                {
                    CAMX_DELETE(pSensorPostJob);
                    pSensorPostJob = NULL;
                }

                break;
            }
            case SensorPostJobCommand::ReadRequest:
            {
                result = pSensorNode->ProcessReadRequest(pSensorPostJob->pData);

                SensorReadRequestFormat* pUserData = reinterpret_cast<SensorReadRequestFormat*>(pSensorPostJob->pData);
                if (pUserData != NULL)
                {
                    CAMX_DELETE(pUserData);
                    pUserData = NULL;
                }

                if (pSensorPostJob != NULL)
                {
                    CAMX_DELETE(pSensorPostJob);
                    pSensorPostJob = NULL;
                }
                break;
            }
            default:
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Sensor CB execution failed");
                break;
            }
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Sensor workerCore failed with result %s", Utils::CamxResultToString(result));
        }
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::LoadSensorInitConfigCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::LoadSensorInitConfigCmd()
{
    CamxResult  result = CamxResultSuccess;

    // Acquire sensor device now if the optimization is enabled
    if (TRUE == GetStaticSettings()->enableSensorAcquireOptimization)
    {
        result = AcquireDevice();
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Sensor Acquire Failed");
            m_sensorConfigStatus = SensorConfigurationStatus::SensorInitializationFailed;
            return result;
        }
    }

    if (SensorConfigurationStatus::SensorConfigurationStateUninitialized == m_sensorConfigStatus)
    {
        m_sensorConfigStatus = SensorConfigurationStatus::SensorInitializationInProgress;
        UINT initializeCmdSize = 0;
        UINT WBCmdSize = 0;
        UINT LSCCmdSize = 0;
        UINT regSettingIdx = 0;
        do
        {
            UINT curRegSettingIdx = regSettingIdx;
            // NOWHINE NC011: Asking for exception, sensor commands are well known as - init, res, AEC, start, stop etc
            initializeCmdSize = GetSensorDataObject()->GetI2CCmdSize(I2CRegSettingType::Init,
            NULL, 0, &regSettingIdx);

            // All sizes should be a multiple of dword
            CAMX_ASSERT(0 == (initializeCmdSize % sizeof(UINT32)));

            CAMX_LOG_INFO(CamxLogGroupSensor, "Sensor initializeCmdSize: %d", initializeCmdSize);

            // NOWHINE NC011: Asking for exception, sensor commands are well known as - init
            result = CreateAndSubmitCommand(initializeCmdSize, I2CRegSettingType::Init,
                CSLPacketOpcodesSensorInitialConfig, 0, curRegSettingIdx);

            if (CamxResultSuccess == result)
            {
                if ((NULL != m_pOTPData) &&
                    (0 != m_pOTPData->WBCalibration[0].settings.regSettingCount))
                {
                    WBCmdSize = GetSensorDataObject()->GetI2CCmdSize(
                        I2CRegSettingType::AWBOTP,
                        static_cast<const VOID*>(&m_pOTPData->WBCalibration[0].settings),
                        0,
                        &regSettingIdx);

                    if (0 != WBCmdSize)
                    {
                        CAMX_LOG_INFO(CamxLogGroupSensor, "Sensor WBCmdSize: %d", WBCmdSize);
                        CAMX_ASSERT(0 == (WBCmdSize % sizeof(UINT32)));
                        result = CreateAndSubmitCommand(WBCmdSize,
                                                        I2CRegSettingType::AWBOTP,
                                                        CSLPacketOpcodesSensorInitialConfig,
                                                        0,
                                                        curRegSettingIdx);
                        if (CamxResultSuccess == result)
                        {
                            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "AWBSetting initialized successful!");
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupSensor, "AWBSetting initialized failed!");
                        }
                    }
                }
                if (CamxResultSuccess == result)
                {
                    if ((NULL != m_pOTPData) &&
                        (0 != m_pOTPData->LSCCalibration[0].settings.regSettingCount))
                    {
                        LSCCmdSize = GetSensorDataObject()->GetI2CCmdSize(
                            I2CRegSettingType::LSC,
                            static_cast<const VOID*>(&m_pOTPData->LSCCalibration[0].settings),
                            0,
                            &regSettingIdx);

                        if (0 != LSCCmdSize)
                        {
                            CAMX_LOG_INFO(CamxLogGroupSensor, "Sensor lSCCmdSize: %d", LSCCmdSize);
                            CAMX_ASSERT(0 == (LSCCmdSize % sizeof(UINT32)));
                            result = CreateAndSubmitCommand(LSCCmdSize,
                                                            I2CRegSettingType::LSC,
                                                            CSLPacketOpcodesSensorInitialConfig,
                                                            0,
                                                            curRegSettingIdx);
                            if (CamxResultSuccess == result)
                            {
                                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "LSCSetting initialized successful!");
                            }
                            else
                            {
                                CAMX_LOG_ERROR(CamxLogGroupSensor, "LSCSetting initialized failed!");
                            }
                        }
                    }
                }
                m_sensorConfigStatus = SensorConfigurationStatus::SensorInitializationComplete;
                CAMX_LOG_INFO(CamxLogGroupSensor, "Sensor initialized successfully");
            }
            else
            {
                m_sensorConfigStatus = SensorConfigurationStatus::SensorInitializationFailed;
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Sensor initialization failed");
            }
        } while (regSettingIdx != 0);
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupSensor, "Sensor initialization sequence already done");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::GetSensorModuleIndexes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::GetSensorModuleIndexes(
    CSLCSIPHYCapability*     pCSIPHYCapability,
    CSLFlashQueryCapability* pFlashCapability,
    CSLActuatorCapability*   pActuatorCap,
    CSLOISCapability*        pOisCap)
{
    CDKResult result = CDKResultEFailed;
    // Get the slot info for this camera
    CSLSensorCapability sensorCap    = { 0 };
    SubDeviceProperty sensorDevice   = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, SensorHandle);
    SubDeviceProperty CSIPHYDevice   = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, CSIPHYHandle);
    SubDeviceProperty OISDevice      = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, OISHandle);
    SubDeviceProperty actuatorDevice = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, ActuatorHandle);
    SubDeviceProperty flashDevice    = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, FlashHandle);


    INT32 deviceIndices[MaxNumImageSensors] = { 0 };
    UINT  actualNumIndices = 0;

    if (FALSE == sensorDevice.isAcquired)
    {
        result = HwEnvironment::GetInstance()->GetDeviceIndices(CSLDeviceTypeImageSensor,
                                                                &deviceIndices[0],
                                                                MaxNumImageSensors,
                                                                &actualNumIndices);

        if (CamxResultSuccess == result)
        {
            CAMX_ASSERT(m_cameraId < actualNumIndices);
            m_pSensorSubDevicesCache->SetSubDeviceIndex(m_cameraId, deviceIndices[m_cameraId], SensorHandle);
            sensorDevice = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, SensorHandle); // to get the updated value
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to get device index for Sensor");
            return result;
        }
    }

    if (CamxResultSuccess == AddDeviceIndex(sensorDevice.deviceIndex))
    {

        result = CSLQueryDeviceCapabilities(sensorDevice.deviceIndex,
                                            &sensorCap,
                                            sizeof(CSLSensorCapability));

        if (CamxResultSuccess == result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                "SensorDeviceIndex[%d] Cap={slotInfo:%d,CSIPHY:%d,actuator:%d,EEPROM:%d,flash:%d OIS:%d}",
                sensorDevice.deviceIndex,
                sensorCap.slotInfo,
                sensorCap.CSIPHYSlotId,
                sensorCap.actuatorSlotId,
                sensorCap.EEPROMSlotId,
                sensorCap.flashSlotId,
                sensorCap.OISSlotId);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to query sensor capabilites");
            return result;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor,
            "Failed to add device result: %d, sensor index: %d, actual indices: %d",
            Utils::CamxResultToString(result),
            sensorDevice.deviceIndex,
            actualNumIndices);

        return result;
    }

    if (FALSE == CSIPHYDevice.isAcquired)
    {
        result = HwEnvironment::GetInstance()->GetDeviceIndices(CSLDeviceTypeCSIPHY,
                                                                &deviceIndices[0],
                                                                MaxNumImageSensors,
                                                                &actualNumIndices);

        if (CamxResultSuccess == result)
        {
            // Find CSIPHY index match sensor slot info
            for (UINT i = 0; i < actualNumIndices; i++)
            {
                if ((CamxResultSuccess == CSLQueryDeviceCapabilities(deviceIndices[i],
                                                                     pCSIPHYCapability,
                                                                     sizeof(CSLCSIPHYCapability))) &&
                    (pCSIPHYCapability->slotInfo == sensorCap.CSIPHYSlotId))
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                        "Found matched CSIPHY[%d] deviceIndices %d slotInfo %d",
                        i,
                        deviceIndices[i],
                        pCSIPHYCapability->slotInfo);

                    m_pSensorSubDevicesCache->SetSubDeviceIndex(m_cameraId, deviceIndices[i], CSIPHYHandle);
                    CSIPHYDevice = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, CSIPHYHandle); // to get the updated value
                    break;
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to get device indices for CSIPHY");
            return result;
        }
    }
    else
    {
        if ((CamxResultSuccess != CSLQueryDeviceCapabilities(CSIPHYDevice.deviceIndex,
                                                             pCSIPHYCapability,
                                                             sizeof(CSLCSIPHYCapability))) ||
            (pCSIPHYCapability->slotInfo != sensorCap.CSIPHYSlotId))
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Cached CSIPHY device index: %d is not valid for given sensor",
                CSIPHYDevice.deviceIndex);
            return CamxResultEFailed;
        }
    }

    result = AddDeviceIndex(CSIPHYDevice.deviceIndex);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor,
            "Failed to add device result: %d, CSIPHY index: %d, actual indices: %d",
            Utils::CamxResultToString(result),
            CSIPHYDevice.deviceIndex,
            actualNumIndices);

        return result;
    }

    if (FALSE == actuatorDevice.isAcquired)
    {
        result = HwEnvironment::GetInstance()->GetDeviceIndices(CSLDeviceTypeLensActuator,
                                                                &deviceIndices[0],
                                                                MaxNumImageSensors,
                                                                &actualNumIndices);
        if (CamxResultSuccess == result)
        {
            // Find Actuator index match sensor slot info
            for (UINT i = 0; i < actualNumIndices; i++)
            {
                if ((CamxResultSuccess == CSLQueryDeviceCapabilities(deviceIndices[i],
                                                                     pActuatorCap,
                                                                     sizeof(CSLActuatorCapability))) &&
                    (pActuatorCap->slotInfo == sensorCap.actuatorSlotId))
                {
                    CAMX_LOG_INFO(CamxLogGroupSensor,
                        "Found matched Actautor[%d] deviceIndices %d slotInfo %d",
                        i,
                        deviceIndices[i],
                        pActuatorCap->slotInfo);

                    m_pSensorSubDevicesCache->SetSubDeviceIndex(m_cameraId, deviceIndices[i], ActuatorHandle);
                    // To get the updated value
                    actuatorDevice = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, ActuatorHandle);
                    break;
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to get device indices for actuator");
        }
    }
    else
    {
        if ((CamxResultSuccess != CSLQueryDeviceCapabilities(actuatorDevice.deviceIndex,
                                                             pActuatorCap,
                                                             sizeof(CSLActuatorCapability))) ||
            (pActuatorCap->slotInfo != sensorCap.actuatorSlotId))
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Cached actuator device index: %d is not valid for given sensor",
                actuatorDevice.deviceIndex);
        }
    }

    if (CamxResultSuccess == result)
    {
        result = AddDeviceIndex(actuatorDevice.deviceIndex);
    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor,
            "Failed to add device result: %d, actuator index: %d, actual indices: %d",
            Utils::CamxResultToString(result),
            actuatorDevice.deviceIndex,
            actualNumIndices);
    }

    if (FALSE == OISDevice.isAcquired)
    {
        result = HwEnvironment::GetInstance()->GetDeviceIndices(CSLDeviceTypeOIS,
                                                                &deviceIndices[0],
                                                                MaxNumImageSensors,
                                                                &actualNumIndices);
        if (CamxResultSuccess == result)
        {
            // Find Ois index match sensor slot info
            for (UINT i = 0; i < actualNumIndices; i++)
            {
                if ((CamxResultSuccess == CSLQueryDeviceCapabilities(deviceIndices[i],
                    pOisCap,
                    sizeof(CSLOISCapability))) &&
                    (pOisCap->slotInfo == sensorCap.OISSlotId))
                {
                    CAMX_LOG_INFO(CamxLogGroupSensor,
                        "Found matched OIS[%d] deviceIndices %d slotInfo %d",
                        i,
                        deviceIndices[i],
                        pOisCap->slotInfo);

                    m_pSensorSubDevicesCache->SetSubDeviceIndex(m_cameraId, deviceIndices[i], OISHandle);
                    OISDevice = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, OISHandle); // to get the updated value
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to get device indices for OIS");
        }
    }
    else
    {
        if ((CamxResultSuccess != CSLQueryDeviceCapabilities(OISDevice.deviceIndex,
                                                             pOisCap,
                                                             sizeof(CSLOISCapability))) &&
            (pOisCap->slotInfo != sensorCap.OISSlotId))
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Cached OIS device index: %d is not valid for given sensor",
                OISDevice.deviceIndex);
        }
    }

    if (CamxResultSuccess == result)
    {
        result = AddDeviceIndex(OISDevice.deviceIndex);
    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor,
            "Failed to add device result: %d, ois index: %d, actual indices: %d",
            Utils::CamxResultToString(result),
            OISDevice.deviceIndex,
            actualNumIndices);
    }

    if (FALSE == flashDevice.isAcquired)
    {
        // Query capability for flash device
        result = HwEnvironment::GetInstance()->GetDeviceIndices(CSLDeviceTypeFlash,
                                                                &deviceIndices[0],
                                                                MaxNumImageSensors,
                                                                &actualNumIndices);

        if ((CamxResultSuccess == result) && (actualNumIndices > 0))
        {
            // Find flash index match sensor slot info
            for (UINT i = 0; i < actualNumIndices; i++)
            {
                if (CamxResultSuccess == CSLQueryDeviceCapabilities(deviceIndices[i],
                                                                    pFlashCapability,
                                                                    sizeof(CSLFlashQueryCapability)))
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                        "Index %d deviceIndices %d flash slotInfo %d sensorCap SlotInfo %d",
                        i,
                        deviceIndices[i],
                        pFlashCapability->slotInfo,
                        sensorCap.slotInfo);

                    for (UINT j = 0; j < CSLFlashMaxLEDTrigger; j++)
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                            "Flash %d maxCurrent %d maxDuration %d maxTorchCurrent %d",
                            pFlashCapability->maxFlashCurrent[j],
                            pFlashCapability->maxFlashDuration[j],
                            pFlashCapability->maxTorchCurrent[j])
                    }

                    if (pFlashCapability->slotInfo == sensorCap.flashSlotId)
                    {
                        m_pSensorSubDevicesCache->SetSubDeviceIndex(m_cameraId, deviceIndices[i], FlashHandle);
                        // To get the updated value
                        flashDevice = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, FlashHandle);
                        break;
                    }
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "GetDeviceIndex failure for Flash, no flash hardware found");
        }
    }
    else
    {
        if ((CamxResultSuccess != CSLQueryDeviceCapabilities(flashDevice.deviceIndex,
                                                             pFlashCapability,
                                                             sizeof(CSLFlashQueryCapability))) ||
            (pFlashCapability->slotInfo != sensorCap.flashSlotId))
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Cached flash device index: %d is not valid for given sensor",
                flashDevice.deviceIndex);
        }
    }

    if (CamxResultSuccess == result)
    {
        result = AddDeviceIndex(flashDevice.deviceIndex);
    }


    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor,
            "Failed to add device result: %d, flash index: %d, actual indices: %d",
            Utils::CamxResultToString(result),
            flashDevice.deviceIndex,
            actualNumIndices);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::CreateSensorSubmodules
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::CreateSensorSubmodules()
{
    CamxResult      result = CamxResultSuccess;

    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupSensor, SCOPEEventSubModuleAcquire);

    if (SensorConfigurationStatus::SensorInitializationComplete == m_sensorConfigStatus)
    {
        m_sensorConfigStatus = SensorConfigurationStatus::SensorSubModuleCreateInProgress;

        result = CreateSubModules();

        if ((CamxResultSuccess == result) && (NULL != m_pSensorModuleData->GetActuatorDataObject()))
        {
            result = CreateActuatorResources();
        }

        const StaticSettings* pStaticSettings = GetStaticSettings();
        CAMX_ASSERT(pStaticSettings != NULL);

        if ((CamxResultSuccess == result) &&
            (FALSE == pStaticSettings->enableOISOptimization) &&
            (NULL != m_pSensorModuleData->GetOisDataObject()))
        {
            // OIS Optimization is disabled, OIS needs to be created serially now
            result = CreateOISSubmodule();
        }

        if ((CamxResultSuccess == result) &&
            (FALSE == pStaticSettings->disableFlash) &&
            (NULL != m_pSensorModuleData->GetFlashDataObject()))
        {
            result = CreateFlashResources();
        }

        if (CamxResultSuccess == result)
        {
            const CSIInformation*   pCSIInfo        = m_pSensorModuleData->GetCSIInfo();
            CSLSensorCSIPHYInfo     cmdCSIPHYConfig = {0};
            if (NULL != pCSIInfo)
            {
                GetSensorDataObject()->CreateCSIPHYConfig(&cmdCSIPHYConfig, pCSIInfo->laneAssign, pCSIInfo->isComboMode,
                    m_currentResolutionIndex);
                cmdCSIPHYConfig.secureMode = static_cast<UINT8>(IsSecureMode());
                result                     = m_pCSIPHY->Configure(GetCSLSession(), &cmdCSIPHYConfig);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "CSI Configuration failed");
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            PublishLensInformation();
            PublishCameraModuleInformation();
            PublishISO100GainInformation();

            m_sensorConfigStatus = SensorConfigurationStatus::SensorSubModuleCreateComplete;
            CAMX_LOG_INFO(CamxLogGroupSensor, "Sensor Submodule creation successful");
        }
        else
        {
            m_sensorConfigStatus = SensorConfigurationStatus::SensorSubModuleCreateFailed;
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Sensor Submodule creation failed");
        }
    }
    else
    {
        result = CamxResultEFailed;
        m_sensorConfigStatus = SensorConfigurationStatus::SensorSubModuleCreateFailed;
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Creating sensor submodules failed due to failed sensor configuration");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::CreateOISSubmodule
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::CreateOISSubmodule()
{
    CamxResult      result = CamxResultSuccess;

    if (SensorOISConfigurationStatus::SensorOISConfigurationStateUninitialized == m_OISConfigStatus)
    {
        m_OISConfigStatus = SensorOISConfigurationStatus::SensorOISInitializationInProgress;

        CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupSensor, "CreateOisResources");

        result = CreateOisResources();

        CAMX_TRACE_SYNC_END(CamxLogGroupSensor);

        if (CamxResultSuccess == result)
        {
            m_OISConfigStatus = SensorOISConfigurationStatus::SensorOISInitializationComplete;
            CAMX_LOG_INFO(CamxLogGroupSensor, "OIS Submodule creation successful");
        }
        else
        {
            m_OISConfigStatus = SensorOISConfigurationStatus::SensorOISInitializationFailed;
            CAMX_LOG_ERROR(CamxLogGroupSensor, "OIS submodule creation failed");
        }
    }
    else
    {
        result = CamxResultEFailed;
        m_OISConfigStatus = SensorOISConfigurationStatus::SensorOISInitializationFailed;
        CAMX_LOG_ERROR(CamxLogGroupSensor, "OIS submodule creation failed");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::GetRDIBufferFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SensorNode::GetRDIBufferFormat(
    PDLibBufferFormat* pBufferFormat)
{

    CamxResult      result            = CamxResultEFailed;
    UINT            tagID[1]          = { 0 };
    CamX::Format    pipelineRawFormat = CamX::Format::RawMIPI;
    ResolutionData* pResolutionData   = &(GetSensorDataObject()->GetResolutionInfo())->resolutionData[m_currentResolutionIndex];

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, " m_currentResolutionIndex %d, bitWidth %d ", m_currentResolutionIndex,
                     pResolutionData->streamInfo.streamConfiguration[0].bitWidth);

    result = VendorTagManager::QueryVendorTagLocation(
            "org.quic.camera.LCRRawformat",
            "LCRRawformat",
            &tagID[0]);

    tagID[0] |= UsecaseMetadataSectionMask;
    VOID*  pData[1]   = {0};
    UINT64 offsets[1] = {0};

    GetDataList(tagID, reinterpret_cast<VOID **>(pData), offsets, 1);

    if (NULL != pData[0])
    {
        CHIRAWFORMATPORT* p_rawports     = static_cast<CHIRAWFORMATPORT*>(pData[0]);
        pipelineRawFormat                = static_cast<CamX::Format>(p_rawports->format);
        CAMX_LOG_INFO(CamxLogGroupSensor, "Rawformat %d IFEPortID %d AFPortID %d",
                      p_rawports->format, p_rawports->inputPortId, p_rawports->sinkPortId);
    }

    switch (pipelineRawFormat)
    {
        case CamX::Format::RawMIPI:
        {
            if (BitWidth10 == pResolutionData->streamInfo.streamConfiguration[0].bitWidth)
            {
                *pBufferFormat = PDLibBufferFormatMipi10;
            }
            else if (BitWidth8 == pResolutionData->streamInfo.streamConfiguration[0].bitWidth)
            {
                *pBufferFormat = PDLibBufferFormatMipi8;
            }
            break;
        }
        case CamX::Format::RawMIPI8:
        {
            *pBufferFormat = PDLibBufferFormatMipi8;
            break;
        }
        case CamX::Format::RawPlain16:
        {
            *pBufferFormat = PDLibBufferFormatUnpacked16;
            break;
        }
        default:
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Unsupported RDI format %d ", pipelineRawFormat);
            break;
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::GetPDAFT3CAMIFPattern
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::GetPDAFT3CAMIFPattern(
    PDLibDataBufferInfo* pCAMIFT3DataPattern,
    PDAFData* pPDAFData)
{
    PDLibDataBufferInfo*         pInputDataPatternFromIFE = NULL;
    CamxResult                   result                   = CamxResultSuccess;
    PDAFType                     PDType                   = PDAFType::PDTypeUnknown;
    PDAFModeInformation*         pPDAFModeInfo            = NULL;
    UINT32                       PDAFModeIdx              = 0;

    if (NULL != pPDAFData)
    {
        result = pPDAFData->GetCurrentPDAFModeIndex(m_currentResolutionIndex, &PDAFModeIdx);

        if (CamxResultSuccess == result)
        {
            result = pPDAFData->GetPDAFModeInformation(PDAFModeIdx, &pPDAFModeInfo);
        }

        if ((NULL != pPDAFModeInfo) && (CamxResultSuccess == result))
        {
            PDType = pPDAFModeInfo->PDType;
        }
    }

    if (CamxResultSuccess == result)
    {
        if (PDAFType::PDType3 == PDType)
        {
            static const UINT T3DataPattern[] =
            {
                PropertyIDUsecaseIFEPDAFInfo
            };
            VOID*  pT3dataPattern[1]        = { 0 };
            UINT   length                   = CAMX_ARRAY_SIZE(T3DataPattern);
            UINT64 propertyDataOffset[1]    = { 0 };
            GetDataList(T3DataPattern, pT3dataPattern, propertyDataOffset, length);
            pInputDataPatternFromIFE  = reinterpret_cast<PDLibDataBufferInfo*>(pT3dataPattern[0]);

            if ((NULL != pInputDataPatternFromIFE) && (NULL != pCAMIFT3DataPattern))
            {
                pCAMIFT3DataPattern->bufferFormat     = pInputDataPatternFromIFE->bufferFormat;
                pCAMIFT3DataPattern->imageOverlap     = pInputDataPatternFromIFE->imageOverlap;
                pCAMIFT3DataPattern->isp1BufferStride = pInputDataPatternFromIFE->isp1BufferStride;
                pCAMIFT3DataPattern->isp1BufferWidth  = pInputDataPatternFromIFE->isp1BufferWidth;
                pCAMIFT3DataPattern->isp2BufferStride = pInputDataPatternFromIFE->isp2BufferStride;
                pCAMIFT3DataPattern->isp2BufferWidth  = pInputDataPatternFromIFE->isp2BufferWidth;
                pCAMIFT3DataPattern->isp2ImageOffset  = pInputDataPatternFromIFE->isp2ImageOffset;
                pCAMIFT3DataPattern->ispBufferHeight  = pInputDataPatternFromIFE->ispBufferHeight;
                pCAMIFT3DataPattern->sensorType       = pInputDataPatternFromIFE->sensorType;

                pCAMIFT3DataPattern->isp1BlockPattern.pixelCount            =
                    pInputDataPatternFromIFE->isp1BlockPattern.pixelCount;
                pCAMIFT3DataPattern->isp1BlockPattern.verticalPDOffset      =
                    pInputDataPatternFromIFE->isp1BlockPattern.verticalPDOffset;
                pCAMIFT3DataPattern->isp1BlockPattern.horizontalPDOffset    =
                    pInputDataPatternFromIFE->isp1BlockPattern.horizontalPDOffset;
                pCAMIFT3DataPattern->isp1BlockPattern.blockDimension.width  =
                    pInputDataPatternFromIFE->isp1BlockPattern.blockDimension.width;
                pCAMIFT3DataPattern->isp1BlockPattern.blockDimension.height =
                    pInputDataPatternFromIFE->isp1BlockPattern.blockDimension.height;
                for (UINT index = 0; index < pCAMIFT3DataPattern->isp1BlockPattern.pixelCount; index++)
                {
                    pCAMIFT3DataPattern->isp1BlockPattern.pixelCoordinate[index].x    =
                        pInputDataPatternFromIFE->isp1BlockPattern.pixelCoordinate[index].x;

                    pCAMIFT3DataPattern->isp1BlockPattern.pixelCoordinate[index].y    =
                        pInputDataPatternFromIFE->isp1BlockPattern.pixelCoordinate[index].y;

                    pCAMIFT3DataPattern->isp1BlockPattern.pixelCoordinate[index].type =
                        pInputDataPatternFromIFE->isp1BlockPattern.pixelCoordinate[index].type;
                }

                pCAMIFT3DataPattern->isp2BlockPattern.pixelCount            =
                    pInputDataPatternFromIFE->isp2BlockPattern.pixelCount;
                pCAMIFT3DataPattern->isp2BlockPattern.verticalPDOffset      =
                    pInputDataPatternFromIFE->isp2BlockPattern.verticalPDOffset;
                pCAMIFT3DataPattern->isp2BlockPattern.horizontalPDOffset    =
                    pInputDataPatternFromIFE->isp2BlockPattern.horizontalPDOffset;
                pCAMIFT3DataPattern->isp2BlockPattern.blockDimension.width  =
                    pInputDataPatternFromIFE->isp2BlockPattern.blockDimension.width;
                pCAMIFT3DataPattern->isp2BlockPattern.blockDimension.height =
                    pInputDataPatternFromIFE->isp2BlockPattern.blockDimension.height;
                for (UINT index = 0; index < pCAMIFT3DataPattern->isp2BlockPattern.pixelCount; index++)
                {
                    pCAMIFT3DataPattern->isp2BlockPattern.pixelCoordinate[index].x    =
                        pInputDataPatternFromIFE->isp2BlockPattern.pixelCoordinate[index].x;

                    pCAMIFT3DataPattern->isp2BlockPattern.pixelCoordinate[index].y    =
                        pInputDataPatternFromIFE->isp2BlockPattern.pixelCoordinate[index].y;

                    pCAMIFT3DataPattern->isp2BlockPattern.pixelCoordinate[index].type =
                        pInputDataPatternFromIFE->isp2BlockPattern.pixelCoordinate[index].type;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Cannot get IFE CAMIF T3 Extracted pattern");
                result = CamxResultEFailed;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Not able to get PDAF Type for current sensor mode %d", m_currentResolutionIndex);
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::QueryMetadataPublishList(
    NodeMetadataList* pPublistTagList)
{
    CamxResult result   = CamxResultSuccess;
    UINT32     tagCount = 0;

    for (UINT32 tagIndex = 0; tagIndex < CAMX_ARRAY_SIZE(SensorOutputMetadataTags); ++tagIndex)
    {
        pPublistTagList->tagArray[tagCount++] = SensorOutputMetadataTags[tagIndex];
    }

    for (UINT32 tagIndex = 0; tagIndex < CAMX_ARRAY_SIZE(SensorOutputVendorTags); ++tagIndex)
    {
        UINT32 tagID;

        result = VendorTagManager::QueryVendorTagLocation(
            SensorOutputVendorTags[tagIndex].pSectionName,
            SensorOutputVendorTags[tagIndex].pTagName,
            &tagID);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupMeta, "Cannot query vendor tag %s %s",
                SensorOutputVendorTags[tagIndex].pSectionName,
                SensorOutputVendorTags[tagIndex].pTagName);
            break;
        }

        pPublistTagList->tagArray[tagCount++] = tagID;
    }

    if (CamxResultSuccess == result)
    {
        pPublistTagList->tagCount = tagCount;
        CAMX_LOG_VERBOSE(CamxLogGroupMeta, "%d tags will be published", tagCount);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::ReadSensorDeviceData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::ReadSensorDeviceData(
    UINT32 cameraID,
    CHAR*  pDeviceName,
    UINT64 readDataAddr,
    UINT   readAddr,
    UINT   readAddrType,
    UINT16 numOfBytes)
{
    CamxResult  result           = CamxResultSuccess;
    INT32       hCSLDeviceHandle = 0;

    if ((0 == readDataAddr) || (NULL == pDeviceName) || (0 == numOfBytes))
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Read request cannot be completed");
        result = CamxResultEFailed;
    }

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Read request for camera ID %d", cameraID);

        // Get the CSL handles for the device to be read
        SubDeviceProperty sensorDevice   = m_pSensorSubDevicesCache->GetSubDevice(cameraID, SensorHandle);
        SubDeviceProperty actuatorDevice = m_pSensorSubDevicesCache->GetSubDevice(cameraID, ActuatorHandle);
        SubDeviceProperty OISDevice      = m_pSensorSubDevicesCache->GetSubDevice(cameraID, OISHandle);

        // NOWHINE CP036a: exception
        if (0 == OsUtils::StrNICmp(const_cast<CHAR*>(pDeviceName), "Sensor", sizeof("Sensor")))
        {
            hCSLDeviceHandle = sensorDevice.hDevice;
        }
        // NOWHINE CP036a: exception
        else if (0 == OsUtils::StrNICmp(const_cast<CHAR*>(pDeviceName), "OIS", sizeof("OIS")))
        {
            hCSLDeviceHandle = OISDevice.hDevice;
        }
        // NOWHINE CP036a: exception
        else if (0 == OsUtils::StrNICmp(const_cast<CHAR*>(pDeviceName), "Actuator", sizeof("Actuator")))
        {
            hCSLDeviceHandle = actuatorDevice.hDevice;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid device to read from");
            result = CamxResultEFailed;
        }

        if (CamxResultSuccess == result)
        {
            SensorPostJob*           pSensorPostJob     = CAMX_NEW SensorPostJob;
            SensorReadRequestFormat* pSensorReadRequest = CAMX_NEW SensorReadRequestFormat;

            if ((NULL != pSensorPostJob) && (NULL != pSensorReadRequest))
            {
                pSensorReadRequest->hCSLDeviceHandle = hCSLDeviceHandle;
                pSensorReadRequest->numOfBytes       = numOfBytes;
                pSensorReadRequest->pReadData        = readDataAddr;
                pSensorReadRequest->regAddr          = readAddr;
                pSensorReadRequest->regAddrType      = readAddrType;
                pSensorReadRequest->cameraID         = cameraID;

                pSensorPostJob->pSensor          = this;
                pSensorPostJob->sensorJobCommand = SensorPostJobCommand::ReadRequest;
                pSensorPostJob->pData            = static_cast<VOID*>(pSensorReadRequest);
                VOID* pSensorPostJobData[]       = { pSensorPostJob, NULL };
                result                           = m_pThreadManager->PostJob(m_hJobFamilyReadHandle,
                                                                             NULL,
                                                                             &pSensorPostJobData[0],
                                                                             FALSE,
                                                                             FALSE);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::ProcessReadRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::ProcessReadRequest(
    VOID* pData)
{
    CamxResult             result                = CamxResultSuccess;

    Packet*                pSensorReadPacket     = NULL; // Packet that will carry the read request
    CmdBufferManager*      pReadPacketManager    = NULL; // Cmd buffer manager for this packet
    ResourceParams         packetResourceParams  = {0};  // Resources to create this packet

    CmdBuffer*             pReadCmd              = NULL; // Cmd buffer for read request, used for casting and code readability
    CmdBufferManager*      pReadCmdBufferManager = NULL; // Cmd buffer manager for this resource
    ResourceParams         resourceParams        = {0};  // Resources to create read command

    UINT                   readCmdSize           = 0;
    UINT32                 cmdBufferIndex        = 0;

    I2CRegSettingType      i2cSettingType        = I2CRegSettingType::Read;
    UINT                   regSettingIdx         = 0;
    UINT                   resolutionIdx         = 0;
    RegSettingsInfo        regSettingInfo;
    UINT32                 cameraID              = 0;

    CSLDeviceType          device                = CSLDeviceTypeInvalidDevice;
    INT32                  hCSLDeviceHandle      = 0;
    UINT32                 opcode                = 0;
    INT32                  hCSLSessionHandle     = 0;
    INT32                  deviceID              = 0;

    if (NULL == pData)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Null read request");
        result = CamxResultEFailed;
    }

    if (CamxResultSuccess == result)
    {
        SensorReadRequestFormat* pSensorReadRequest = static_cast<SensorReadRequestFormat*>(pData);
        cameraID                                    = pSensorReadRequest->cameraID;
        hCSLSessionHandle                           = GetCSLSession();
        hCSLDeviceHandle                            = pSensorReadRequest->hCSLDeviceHandle;

        if (0 == pSensorReadRequest->pReadData)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "No memory supplied to the sensor, cannot complete read request.");
            result = CamxResultEFailed;
        }

        if (CamxResultSuccess == result)
        {
            // Get the CSL handles for the device to be read
            SubDeviceProperty sensorDevice   = m_pSensorSubDevicesCache->GetSubDevice(cameraID, SensorHandle);
            SubDeviceProperty actuatorDevice = m_pSensorSubDevicesCache->GetSubDevice(cameraID, ActuatorHandle);
            SubDeviceProperty OISDevice      = m_pSensorSubDevicesCache->GetSubDevice(cameraID, OISHandle);

            // Select the device to read from and update device/opcode accordingly
            if (hCSLDeviceHandle == sensorDevice.hDevice)
            {
                device   = CSLDeviceTypeImageSensor;
                deviceID = sensorDevice.deviceIndex;
                opcode   = CSLPacketOpcodesSensorRead;
            }
            else if (hCSLDeviceHandle == actuatorDevice.hDevice)
            {
                device   = CSLDeviceTypeLensActuator;
                deviceID = actuatorDevice.deviceIndex;
                opcode   = CSLPacketOpcodesActuatorRead;
            }
            else if (hCSLDeviceHandle == OISDevice.hDevice)
            {
                device   = CSLDeviceTypeOIS;
                deviceID = OISDevice.deviceIndex;
                opcode   = CSLPacketOpcodesOisRead;
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid device to read from =%d", deviceID);
            }

            if (CamxResultSuccess == result)
            {
                // Create READ command to be submitted
                regSettingInfo.regSettingCount               = 1;
                regSettingInfo.regSetting[0].registerAddr    = pSensorReadRequest->regAddr;
                regSettingInfo.regSetting[0].regAddrType     =
                    static_cast<I2CRegAddressDataType>(pSensorReadRequest->regAddrType);
                regSettingInfo.regSetting[0].regDataType     = I2CRegAddressDataTypeByte;
                regSettingInfo.regSetting[0].registerData    = pSensorReadRequest->numOfBytes;
                regSettingInfo.regSetting[0].operation       = IOOperationTypeRead;
                regSettingInfo.regSetting[0].delayUs         = 0;

                // Create packet manager
                packetResourceParams.usageFlags.packet                = 1;
                packetResourceParams.packetParams.maxNumCmdBuffers    = 1;
                packetResourceParams.packetParams.maxNumIOConfigs     = 1;
                packetResourceParams.resourceSize                     =
                    Packet::CalculatePacketSize(&packetResourceParams.packetParams);
                packetResourceParams.poolSize                         = packetResourceParams.resourceSize;
                packetResourceParams.alignment                        = CamxPacketAlignmentInBytes;
                packetResourceParams.pDeviceIndices                   = NULL;
                packetResourceParams.numDevices                       = 0;
                packetResourceParams.memFlags                         = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

                result = CreateCmdBufferManager("ReadPacketMgr", &packetResourceParams, &pReadPacketManager);

                if (CamxResultSuccess == result)
                {
                    pSensorReadPacket = GetPacket(pReadPacketManager);
                }

                // Create cmd buffer manager for sensor read request
                if (CamxResultSuccess == result)
                {
                    resourceParams.usageFlags.cmdBuffer = 1;
                    resourceParams.resourceSize         = sizeof(CSLSensorI2CContinuousReadCmd);
                    resourceParams.poolSize             = resourceParams.resourceSize;
                    resourceParams.cmdParams.type       = CmdType::Generic;
                    resourceParams.alignment            = CamxPacketAlignmentInBytes;
                    resourceParams.pDeviceIndices       = NULL;
                    resourceParams.numDevices           = 0;
                    resourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

                    result = CreateCmdBufferManager("ReadCmdBufferMgr", &resourceParams, &pReadCmdBufferManager);

                }

                if (CamxResultSuccess == result)
                {
                    pReadCmd = GetCmdBuffer(pReadCmdBufferManager);

                    if (pReadCmd != NULL)
                    {
                        VOID* pReadSettingsInfo = static_cast<VOID*>(&regSettingInfo);

                        readCmdSize = GetSensorDataObject()->GetI2CCmdSize(I2CRegSettingType::Read,
                                                                            pReadSettingsInfo,
                                                                            resolutionIdx,
                                                                            &regSettingIdx);

                        if (0!= readCmdSize)
                        {
                            VOID* pReadRequest = pReadCmd->BeginCommands(readCmdSize / sizeof(UINT32));
                            if (pReadRequest != NULL)
                            {
                                CSLSensorI2CContinuousReadCmd* pCmdReadUpdate =
                                    reinterpret_cast<CSLSensorI2CContinuousReadCmd*>(pReadRequest);
                                result = GetSensorDataObject()->CreateI2CCmd(i2cSettingType,
                                                                            pReadRequest,
                                                                            pReadSettingsInfo,
                                                                            0,
                                                                            regSettingIdx);


                                // Commit the read request cmd to this packet
                                if (CamxResultSuccess == result)
                                {
                                    result = pReadCmd->CommitCommands();
                                }
                                else
                                {
                                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create I2C command");
                                    result = CamxResultEFailed;
                                }

                                // Ad-hoc read request is not associated with any request ID
                                // This needs to be recycled manually at the end of read
                                // This adds the read command buffer to the packet
                                if (CamxResultSuccess == result)
                                {
                                    pSensorReadPacket->SetOpcode(device, opcode);
                                    pSensorReadPacket->SetRequestId(CamxInvalidRequestId);
                                    result = pSensorReadPacket->AddCmdBufferReference(pReadCmd, &cmdBufferIndex);
                                }

                                // Read the data and copy to the memory supplied from the application
                                if (CamxResultSuccess == result)
                                {
                                    result = m_pSensorModuleData->GetSensorDataObject()->ReadDeviceData(deviceID,
                                                                                    pSensorReadRequest->numOfBytes,
                                                                                    reinterpret_cast<UINT8*>(
                                                                                    pSensorReadRequest->pReadData),
                                                                                    opcode,
                                                                                    hCSLDeviceHandle,
                                                                                    device,
                                                                                    hCSLSessionHandle,
                                                                                    pSensorReadPacket);
                                }
                            }
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupSensor, "Read request command size is 0");
                            result = CamxResultEFailed;
                        }

                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupSensor, "Read command is NULL");
                        result = CamxResultEFailed;
                    }

                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to obtain command buffer to process read request");
                    result = CamxResultEFailed;
                }

                // Recycle packet and buffer managers
                if (NULL != pReadPacketManager)
                {
                    if (NULL != pSensorReadPacket)
                    {
                        pReadPacketManager->Recycle(pSensorReadPacket);
                        pSensorReadPacket = NULL;
                    }
                    DestroyCmdBufferManager(&pReadPacketManager);
                }

                if (NULL != pReadCmdBufferManager)
                {
                    if (NULL != pReadCmd)
                    {
                        pReadCmdBufferManager->Recycle(pReadCmd);
                        pReadCmd = NULL;
                    }
                    DestroyCmdBufferManager(&pReadCmdBufferManager);
                }
            }
        }
    }
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::AcquireResources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::AcquireResources()
{
    CamxResult       result      = CamxResultSuccess;
    ImageSensorData* pSensorData = GetSensorDataObject();

    CAMX_LOG_CONFIG(CamxLogGroupSensor, "m_currentResolutionIndex:%d, m_reconfigResSetting:%d",
        m_currentResolutionIndex, m_reconfigResSetting);

    if (TRUE == m_reconfigResSetting)
    {
        m_sensorConfigStatus = SensorConfigurationStatus::SensorConfigurationInProgress;

        // reconfigure sensor res setting
        UINT regSettingIdx = 0; // using common index as only init reg setting uses this param
        UINT resCmdSize    = pSensorData->GetI2CCmdSize(I2CRegSettingType::Res, NULL, m_currentResolutionIndex, &regSettingIdx);

        if ((0 == resCmdSize) || (0 != (resCmdSize % sizeof(UINT32))))
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid resCmdSize: %d", resCmdSize);
            result = CDKResultEInvalidArg;
        }

        if (CamxResultSuccess == result)
        {
            result = CreateAndSubmitCommand(
                resCmdSize, I2CRegSettingType::Res, CSLPacketOpcodesSensorConfig,
                m_currentResolutionIndex, regSettingIdx);

            if (CamxResultSuccess == result)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Res configuration successful!");
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Sensor Res configuration failed!");
            }
        }

        // configure csiphy parameters
        if (CamxResultSuccess == result)
        {
            const CSIInformation*   pCSIInfo        = m_pSensorModuleData->GetCSIInfo();
            CSLSensorCSIPHYInfo     cmdCSIPHYConfig ={ 0 };
            if (NULL != pCSIInfo)
            {
                GetSensorDataObject()->CreateCSIPHYConfig(&cmdCSIPHYConfig, pCSIInfo->laneAssign, pCSIInfo->isComboMode,
                    m_currentResolutionIndex);
                cmdCSIPHYConfig.secureMode = static_cast<UINT8>(IsSecureMode());
                result                     = m_pCSIPHY->Configure(GetCSLSession(), &cmdCSIPHYConfig);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "CSI Configuration failed");
                }
            }
        }

        if (CDKResultSuccess == result)
        {
            m_sensorConfigStatus = SensorConfigurationStatus::SensorConfigurationComplete;
            m_reconfigResSetting = FALSE;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorNode::ReleaseResources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::ReleaseResources(
    CHIDEACTIVATEPIPELINEMODE modeBitmask)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != m_pOis)
    {
        m_pOis->ReleaseResources(modeBitmask);
    }

    if (CHIDeactivateModeUnlinkPipeline == modeBitmask)
    {
        CAMX_LOG_CONFIG(CamxLogGroupSensor, "mode: CHIDeactivateModeUnlinkPipeline");
        m_reconfigResSetting = TRUE;
    }

    if (CHIDeactivateModeSensorStandby & modeBitmask)
    {
        CAMX_LOG_INFO(CamxLogGroupSensor, "Skipping recycling of command buffers");
    }
    else
    {
        RecycleAllCmdBuffers();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::GetMaxCmdBufSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SensorNode::GetMaxCmdBufSize()
{
    ImageSensorData* pSensorData   = GetSensorDataObject();
    UINT             regSettingIdx = 0;
    UINT             maxResolution = pSensorData->GetResolutionInfo()->resolutionDataCount;
    UINT             maxCmdSize    = 0;
    UINT             cmdSize       = 0;

    for (UINT i = 0; i < static_cast<UINT>(I2CRegSettingType::Max); i++)
    {
        I2CRegSettingType type = static_cast<I2CRegSettingType>(i);
        switch (type)
        {
            case I2CRegSettingType::Res:
                for (UINT resolutionIdx = 0; resolutionIdx < maxResolution; resolutionIdx++)
                {
                    cmdSize = pSensorData->GetI2CCmdSize(I2CRegSettingType::Res, NULL, resolutionIdx, &regSettingIdx);
                    if (cmdSize > maxCmdSize)
                    {
                        maxCmdSize = cmdSize;
                    }
                }
                break;
            case I2CRegSettingType::Start:
            case I2CRegSettingType::Stop:
            // NOWHINE NC011: Asking for exception, sensor commands are well known as - init, res, start, stop etc
            case I2CRegSettingType::Init:
            case I2CRegSettingType::AEC:
            case I2CRegSettingType::Master:
            case I2CRegSettingType::Slave:
                cmdSize = pSensorData->GetI2CCmdSize(type, NULL, 0, &regSettingIdx);
                if (cmdSize > maxCmdSize)
                {
                    maxCmdSize = cmdSize;
                }
                break;
            case I2CRegSettingType::SPC:
                if ((NULL != m_pOTPData) && (TRUE == m_pOTPData->SPCCalibration.isAvailable))
                {
                    cmdSize = pSensorData->GetI2CCmdSize(I2CRegSettingType::SPC,
                                                         static_cast<const VOID*>(&m_pOTPData->SPCCalibration.settings),
                                                         0, &regSettingIdx);
                    if (cmdSize > maxCmdSize)
                    {
                        maxCmdSize = cmdSize;
                    }
                }
                break;
            case I2CRegSettingType::QSC:
                if ((NULL != m_pOTPData) && (TRUE == m_pOTPData->QSCCalibration.isAvailable))
                {
                    cmdSize = pSensorData->GetI2CCmdSize(I2CRegSettingType::QSC,
                                                         static_cast<const VOID*>(&m_pOTPData->QSCCalibration.settings),
                                                         0, &regSettingIdx);
                    if (cmdSize > maxCmdSize)
                    {
                        maxCmdSize = cmdSize;
                    }
                }
                break;
            case I2CRegSettingType::AWBOTP:
                if ((NULL != m_pOTPData) && (0 != m_pOTPData->WBCalibration[0].settings.regSettingCount))
                {
                    cmdSize = pSensorData->GetI2CCmdSize(I2CRegSettingType::AWBOTP,
                                                         static_cast<const VOID*>(&m_pOTPData->WBCalibration[0].settings),
                                                         0, &regSettingIdx);
                    if (cmdSize > maxCmdSize)
                    {
                        maxCmdSize = cmdSize;
                    }
                }
                break;
            default:
                break;
        }
    }
    m_maxCmdBufSize = maxCmdSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SensorNode::CreateCmdBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorNode::CreateCmdBuffers()
{
    CamxResult       result        = CamxResultSuccess;

    // Create configuration packet/command buffers
    ResourceParams resourceParams                   = { 0 };
    resourceParams.usageFlags.packet                = 1;
    resourceParams.packetParams.maxNumCmdBuffers    = MaxCommandBuffers;
    resourceParams.packetParams.maxNumIOConfigs     = 0;
    resourceParams.resourceSize                     = Packet::CalculatePacketSize(&resourceParams.packetParams);
    resourceParams.poolSize                         = MaxCommandBuffers * resourceParams.resourceSize;
    resourceParams.alignment                        = CamxPacketAlignmentInBytes;
    resourceParams.pDeviceIndices                   = NULL;
    resourceParams.numDevices                       = 0;
    resourceParams.memFlags                         = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

    result = CreateCmdBufferManager("PacketManager", &resourceParams, &m_pConfigPacketManager);
    if ((CamxResultSuccess != result) || (NULL == m_pConfigPacketManager))
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "PacketManager not created rc=%d", result);
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupSensor, "PacketManager created successfully");
        ResourceParams params       = { 0 };
        params.resourceSize         = m_maxCmdBufSize;
        params.poolSize             = MaxCommandBuffers * params.resourceSize;
        params.usageFlags.cmdBuffer = 1;
        params.cmdParams.type       = CmdType::I2C;
        params.alignment            = CamxCommandBufferAlignmentInBytes;
        params.pDeviceIndices       = NULL;
        params.numDevices           = 0;
        params.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("CmdBufferManager", &params, &m_pConfigCmdBufferManager);
        if ((CamxResultSuccess == result) && (NULL != m_pConfigPacketManager))
        {
            CAMX_LOG_INFO(CamxLogGroupSensor, "CmdBufferManager created successfully");
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "CmdBufferManager not created rc=%d", result);
        }
    }
    return result;
}

CAMX_NAMESPACE_END
