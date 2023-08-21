////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxhwenvironment.cpp
/// @brief HwEnvironment class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcustomization.h"
#include "camxdefs.h"
#include "camxhwcontext.h"
#include "camximagesensormoduledatamanager.h"
#include "camximagesensormoduledata.h"
#include "camximagesensordata.h"
#include "camxhwfactory.h"
#include "camxcsljumptable.h"
#include "camxsettingsmanager.h"
#include "camxtuningdatamanager.h"
#include "camxchicomponent.h"
#include "camxcslsensordefs.h"
#include "camxdefs.h"
#include "camxsettingsmanager.h"
#include "camxstaticcaps.h"
#include "camxtypes.h"
#include "camxvendortags.h"
#include "camxhwenvironment.h"
#include "camxmoduleconfig.h"


CAMX_NAMESPACE_BEGIN
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Prototypes for the HWL init functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xGetStaticEntryMethods
///
/// @brief  External function to be supplied by Titan 17X hardware layer for initialization
///
/// @param  pStaticEntry    Static entry methods.
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xGetStaticEntryMethods(
    HwContextStaticEntry* pStaticEntry);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Class methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::GetInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HwEnvironment* HwEnvironment::GetInstance()
{
    static HwEnvironment s_HwEnvironmentSingleton;

    /// @todo (CAMX-2684) Workaround a chicken-and-egg problem in HwEnvironment initialization...clean up later
    // By calling InitCaps here, the call it triggers back into GetInstance will not cause HwEnvironment to be recreated.
    // Branch prediction should make this essentially free
    if (InitCapsInitialize == s_HwEnvironmentSingleton.m_initCapsStatus)
    {
        s_HwEnvironmentSingleton.InitCaps();
    }

    return &s_HwEnvironmentSingleton;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::GetNCSObject
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* HwEnvironment::GetNCSObject()
{
    return m_pNCSObject;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::SetNCSObject
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HwEnvironment::SetNCSObject(
    VOID* pNCSObject)
{
    m_pNCSObject = pNCSObject;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::HwEnvironment
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HwEnvironment::HwEnvironment()
    : m_initCapsStatus(InitCapsInvalid)
    , m_pNCSObject(NULL)
{
    Initialize();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::~HwEnvironment
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HwEnvironment::~HwEnvironment()
{
    CamxResult  result            = CamxResultEInvalidState;
    RegisterSetting* pRegSettings = NULL;

    if (NULL != m_pOEMInterface->pReleaseExtendedHWEnvironmentStaticCaps)
    {
        m_pOEMInterface->pReleaseExtendedHWEnvironmentStaticCaps(&m_caps[0], m_numberSensors);
    }

    if (NULL != m_pOEMInterface->pReleaseExtendedPlatformStaticCaps)
    {
        m_pOEMInterface->pReleaseExtendedPlatformStaticCaps(&m_platformCaps[0], m_numberSensors);
    }

    if (NULL != m_pImageSensorModuleDataManager)
    {
        m_pImageSensorModuleDataManager->Destroy();
        m_pImageSensorModuleDataManager = NULL;
    }

    if (NULL != m_pSettingsManager)
    {
        m_pSettingsManager->Destroy();
        m_pSettingsManager = NULL;
    }

    if (NULL != m_pHwFactory)
    {
        m_pHwFactory->Destroy();
        m_pHwFactory = NULL;
    }

    for (UINT i = 0; i < MaxNumImageSensors; ++i)
    {
        if (NULL != m_pTuningManager[i])
        {
            m_pTuningManager[i]->Destroy();
            m_pTuningManager[i] = NULL;
        }
    }

    for (UINT i = 0; i < m_numberSensors; i++)
    {
        CAMX_FREE(m_sensorInfoTable[i].moduleCaps.OTPData.EEPROMInfo.rawOTPData.pRawData);
        m_sensorInfoTable[i].moduleCaps.OTPData.EEPROMInfo.rawOTPData.pRawData = NULL;

        pRegSettings = m_sensorInfoTable[i].moduleCaps.OTPData.SPCCalibration.settings.regSetting;
        if (NULL != pRegSettings)
        {
            // Free the SPC data allocated in the eeprom calibration FormatSPCData()
            // Free the memory in hwEnvironment for possible memory leak
            CAMX_FREE(pRegSettings->registerData);
            pRegSettings->registerData = NULL;

            CAMX_FREE(pRegSettings);
            pRegSettings = NULL;
            m_sensorInfoTable[i].moduleCaps.OTPData.SPCCalibration.settings.regSetting = NULL;
        }

        pRegSettings = m_sensorInfoTable[i].moduleCaps.OTPData.QSCCalibration.settings.regSetting;
        if (NULL != pRegSettings)
        {
            // Free the QSC data allocated in the eeprom calibration FormatQSCData()
            // Free the memory in hwEnvironment for possible memory leak
            CAMX_FREE(pRegSettings->registerData);
            pRegSettings->registerData = NULL;

            CAMX_FREE(pRegSettings);
            pRegSettings = NULL;
            m_sensorInfoTable[i].moduleCaps.OTPData.QSCCalibration.settings.regSetting = NULL;
        }

        pRegSettings = m_sensorInfoTable[i].moduleCaps.OTPData.OISCalibration.settings.regSetting;
        if (NULL != pRegSettings)
        {
            // Free the OIS data allocated in the eeprom calibration FormatOISData()
            // Free the memory in hwEnvironment for possible memory leak
            CAMX_FREE(pRegSettings->registerData);
            pRegSettings->registerData = NULL;

            CAMX_FREE(pRegSettings);
            pRegSettings = NULL;
            m_sensorInfoTable[i].moduleCaps.OTPData.OISCalibration.settings.regSetting = NULL;
        }

        pRegSettings = m_sensorInfoTable[i].moduleCaps.OTPData.WBCalibration[0].settings.regSetting;
        if (NULL != pRegSettings)
        {
            // Free the WB data allocated in the eeprom calibration FormatWBData()
            // Free the memory in hwEnvironment for possible memory leak
            CAMX_FREE(pRegSettings->registerData);
            pRegSettings->registerData = NULL;

            CAMX_FREE(pRegSettings);
            pRegSettings = NULL;
            m_sensorInfoTable[i].moduleCaps.OTPData.WBCalibration[0].settings.regSetting = NULL;
        }

        pRegSettings = m_sensorInfoTable[i].moduleCaps.OTPData.LSCCalibration[0].settings.regSetting;
        if (NULL != pRegSettings)
        {
            // Free the LSC data allocated in the eeprom calibration FormatLSCData()
            // Free the memory in hwEnvironment for possible memory leak
            CAMX_FREE(pRegSettings->registerData);
            pRegSettings->registerData = NULL;

            CAMX_FREE(pRegSettings);
            pRegSettings = NULL;
            m_sensorInfoTable[i].moduleCaps.OTPData.LSCCalibration[0].settings.regSetting = NULL;
        }
    }

    for (UINT i = 0; i < MaxExternalComponents; i++)
    {
        if (NULL != m_externalComponent[i].pComponentName)
        {
            CAMX_FREE(m_externalComponent[i].pComponentName);
            m_externalComponent[i].pComponentName = NULL;
        }
    }

    if (NULL != m_pHWEnvLock)
    {
        m_pHWEnvLock->Destroy();
    }

    result = CSLUninitialize();
    m_initCapsStatus = InitCapsInvalid;
    CAMX_ASSERT(CamxResultSuccess == result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HwEnvironment::Initialize()
{
    CamxResult              result                  = CamxResultSuccess;
    CSLInitializeParams     params                  = { 0 };
    SettingsManager*        pStaticSettingsManager  = SettingsManager::Create(NULL);
    ExternalComponentInfo*  pExternalComponent      = GetExternalComponent();

    m_pHWEnvLock = Mutex::Create("HwEnvLock");
    CAMX_ASSERT(NULL != m_pHWEnvLock);

    CAMX_ASSERT(NULL != pStaticSettingsManager);

    if (NULL != pStaticSettingsManager)
    {
        const StaticSettings* pStaticSettings = pStaticSettingsManager->GetStaticSettings();

        CAMX_ASSERT(NULL != pStaticSettings);

        if (NULL != pStaticSettings)
        {
            params.mode                                           = pStaticSettings->CSLMode;
            params.emulatedSensorParams.enableSensorSimulation    = pStaticSettings->enableSensorEmulation;
            params.emulatedSensorParams.dumpSensorEmulationOutput = pStaticSettings->dumpSensorEmulationOutput;

            OsUtils::StrLCpy(params.emulatedSensorParams.sensorEmulatorPath,
                             pStaticSettings->sensorEmulatorPath,
                             sizeof(pStaticSettings->sensorEmulatorPath));

            OsUtils::StrLCpy(params.emulatedSensorParams.sensorEmulator,
                             pStaticSettings->sensorEmulator,
                             sizeof(pStaticSettings->sensorEmulator));

            result = CSLInitialize(&params);

            if (CamxResultSuccess == result)
            {
                // Query the camera platform
                result = QueryHwContextStaticEntryMethods();
            }

            if (CamxResultSuccess == result)
            {
                m_pHwFactory = m_staticEntryMethods.CreateHwFactory();

                if (NULL == m_pHwFactory)
                {
                    CAMX_ASSERT_ALWAYS_MESSAGE("Failed to create the HW factory");
                    result = CamxResultEFailed;
                }
            }

            if (CamxResultSuccess == result)
            {
                m_pSettingsManager = m_pHwFactory->CreateSettingsManager();

                if (NULL == m_pSettingsManager)
                {
                    CAMX_ASSERT_ALWAYS_MESSAGE("Failed to create the HW settings manager");
                    result = CamxResultEFailed;
                }
            }

            if (CamxResultSuccess == result)
            {
                m_staticEntryMethods.GetHWBugWorkarounds(&m_workarounds);
            }
        }

        pStaticSettingsManager->Destroy();
        pStaticSettingsManager = NULL;
    }

    CAMX_ASSERT(NULL != pExternalComponent);
    if ((CamxResultSuccess == result) && (NULL != pExternalComponent))
    {
        result = ProbeChiComponents(pExternalComponent, &m_numExternalComponent);
    }

    if (CamxResultSuccess == result)
    {
        // Load the OEM sensor capacity customization functions
        CAMXCustomizeCAMXInterface camxInterface;
        camxInterface.pGetHWEnvironment = HwEnvironment::GetInstance;
        CAMXCustomizeEntry(&m_pOEMInterface, &camxInterface);
    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "FATAL ERROR: Raise SigAbort. HwEnvironment initialization failed");
        m_numberSensors = 0;
        OsUtils::RaiseSignalAbort();
    }
    else
    {
        m_initCapsStatus = InitCapsInitialize;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::InitCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HwEnvironment::InitCaps()
{
    CamxResult    result = CamxResultSuccess;

    m_pHWEnvLock->Lock();

    if (InitCapsRunning == m_initCapsStatus ||
        InitCapsDone == m_initCapsStatus)
    {
        m_pHWEnvLock->Unlock();
        return;
    }

    m_initCapsStatus = InitCapsRunning;

    if (CamxResultSuccess == result)
    {
        EnumerateDevices();
        ProbeImageSensorModules();
        EnumerateSensorDevices();
        InitializeSensorSubModules();
        InitializeSensorStaticCaps();

        result = m_staticEntryMethods.GetStaticCaps(&m_platformCaps[0]);
        // copy the static capacity to remaining sensor's
        for (UINT index = 1; index < m_numberSensors; index++)
        {
            Utils::Memcpy(&m_platformCaps[index], &m_platformCaps[0], sizeof(m_platformCaps[0]));
        }
        if (NULL != m_pOEMInterface->pInitializeExtendedPlatformStaticCaps)
        {
            m_pOEMInterface->pInitializeExtendedPlatformStaticCaps(&m_platformCaps[0], m_numberSensors);
        }
    }

    CAMX_ASSERT(CamxResultSuccess == result);

    if (CamxResultSuccess == result)
    {
        InitializeHwEnvironmentStaticCaps();
    }

    m_initCapsStatus = InitCapsDone;

    m_pHWEnvLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::InitializeSensorHwDeviceCache
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HwEnvironment::InitializeSensorHwDeviceCache(
    UINT32                 cameraID,
    HwContext*             pHwContext,
    CSLHandle              hCSLSession,
    UINT32                 hCSLSessionIdx,
    VOID*                  pSensorDeviceHandles,
    VOID*                  pSensorDeviceCache)
{
    m_sensorInfoTable[cameraID].moduleCaps.pHwContext = pHwContext;

    if (CSLInvalidHandle == hCSLSession)
    {
        m_sensorInfoTable[cameraID].moduleCaps.hCSLSession[0] = CSLInvalidHandle;
        m_sensorInfoTable[cameraID].moduleCaps.hCSLSession[1] = CSLInvalidHandle;
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupCore, "storing session handle %p", hCSLSession);
        m_sensorInfoTable[cameraID].moduleCaps.hCSLSession[hCSLSessionIdx] = hCSLSession;
    }
    if (0 == hCSLSessionIdx)
    {
        m_sensorInfoTable[cameraID].moduleCaps.pSensorDeviceHandles = pSensorDeviceHandles;
        m_sensorInfoTable[cameraID].moduleCaps.pSensorDeviceCache   = pSensorDeviceCache;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::GetCameraInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HwEnvironment::GetCameraInfo(
    UINT32          cameraID,
    HwCameraInfo*   pCameraInfo
    ) const
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL     != pCameraInfo);
    CAMX_ASSERT(cameraID < m_numberSensors);
    CAMX_ASSERT(NULL     != m_sensorInfoTable[cameraID].pData);

    if ((NULL != pCameraInfo) && (cameraID < m_numberSensors))
    {
        UINT32 roll         = m_sensorInfoTable[cameraID].CSLCapability.roll;
        UINT32 pitch        = m_sensorInfoTable[cameraID].CSLCapability.pitch;
        UINT32 yaw          = m_sensorInfoTable[cameraID].CSLCapability.yaw;
        BOOL   hasOverride  = m_sensorInfoTable[cameraID].pData->GetOverrideSensorMountAvailable();

        if (TRUE == hasOverride)
        {
            result = m_sensorInfoTable[cameraID].pData->GetOverrideSensorMount(&roll, &pitch, &yaw);
        }

        if (CamxResultSuccess == result)
        {
            pCameraInfo->imageSensorFacing = ImageSensorFacingExternal;

            if (CSLSensorPitchLevel == pitch)
            {
                if (CSLSensorYawFront == yaw)
                {
                    pCameraInfo->imageSensorFacing = ImageSensorFacingFront;
                }
                else if (CSLSensorYawRear == yaw)
                {
                    pCameraInfo->imageSensorFacing = ImageSensorFacingBack;
                }
            }

            switch (roll)
            {
                case CSLSensorRoll0:
                    pCameraInfo->imageOrientation = ImageOrientationCW0;
                    break;

                case CSLSensorRoll90:
                    pCameraInfo->imageOrientation = ImageOrientationCW90;
                    break;

                case CSLSensorRoll180:
                    pCameraInfo->imageOrientation = ImageOrientationCW180;
                    break;

                case CSLSensorRoll270:
                    pCameraInfo->imageOrientation = ImageOrientationCW270;
                    break;

                default:
                    pCameraInfo->imageSensorFacing = ImageSensorFacingExternal;
                    pCameraInfo->imageOrientation  = ImageOrientationCW0;
                    break;
            }

            pCameraInfo->mountAngle               = pitch;
            pCameraInfo->pPlatformCaps            = &m_platformCaps[cameraID];
            pCameraInfo->pSensorCaps              = &(m_sensorInfoTable[cameraID].moduleCaps);
            pCameraInfo->pHwEnvironmentCaps       = &(m_caps[cameraID]);

            /// @todo (CAMX-541) Populate resource costing and conflicting devices.
            pCameraInfo->conflictingDevicesLength = 0;
            pCameraInfo->resourceCost             = 50;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::GetSettingsManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const SettingsManager* HwEnvironment::GetSettingsManager() const
{
    return m_pSettingsManager;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::GetStaticSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const StaticSettings* HwEnvironment::GetStaticSettings() const
{
    return m_pSettingsManager->GetStaticSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::QueryHwContextStaticEntryMethods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HwEnvironment::QueryHwContextStaticEntryMethods()
{
    CamxResult          result = CamxResultSuccess;
    CSLCameraPlatform   CSLPlatform;

    Utils::Memset(&CSLPlatform, 0, sizeof(CSLPlatform));

    result = CSLQueryCameraPlatform(&CSLPlatform);

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupHWL, "HWL Query Camera Platform = %d MajorV=%d MinorV=%d",
            CSLPlatform.family,
            CSLPlatform.platformVersion.majorVersion,
            CSLPlatform.platformVersion.minorVersion);

        switch (CSLPlatform.family)
        {
            case CSLCameraFamilyTitan:
            {
                // Titan 17X
                BOOL isTitan17x = ((1 == CSLPlatform.platformVersion.majorVersion) &&
                                    ((7 == CSLPlatform.platformVersion.minorVersion) ||
                                    (6 == CSLPlatform.platformVersion.minorVersion) ||
                                    (5 == CSLPlatform.platformVersion.minorVersion)));

                // Spectra 480
                BOOL isTitan480 = ((4 == CSLPlatform.platformVersion.majorVersion) &&
                                    (8 == CSLPlatform.platformVersion.minorVersion));

                if (TRUE == isTitan17x || TRUE == isTitan480)
                {
                    result = Titan17xGetStaticEntryMethods(&m_staticEntryMethods);

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupHWL, "HWL static entry methods not completely defined");
                        result = CamxResultEUnableToLoad;
                    }
                    else
                    {
                        CAMX_ASSERT(NULL != m_staticEntryMethods.Create);
                        CAMX_ASSERT(NULL != m_staticEntryMethods.CreateHwFactory);
                        CAMX_ASSERT(NULL != m_staticEntryMethods.GetStaticCaps);
                        CAMX_ASSERT(NULL != m_staticEntryMethods.GetStaticMetadataKeysInfo);
                        CAMX_ASSERT(NULL != m_staticEntryMethods.QueryVendorTagsInfo);
                        CAMX_ASSERT(NULL != m_staticEntryMethods.GetHWBugWorkarounds);
                        CAMX_ASSERT(NULL != m_staticEntryMethods.QueryExternalComponentVendorTagsInfo);
                    }
                }
                break;
            }

            default:
                // Unsupported camera family
                CAMX_ASSERT_ALWAYS_MESSAGE("Unsupported camera family: %d", CSLPlatform.family);
                result = CamxResultEUnsupported;
                break;
        }

        if ((NULL == m_staticEntryMethods.Create)                    ||
            (NULL == m_staticEntryMethods.CreateHwFactory)           ||
            (NULL == m_staticEntryMethods.GetStaticCaps)             ||
            (NULL == m_staticEntryMethods.GetStaticMetadataKeysInfo) ||
            (NULL == m_staticEntryMethods.QueryVendorTagsInfo)       ||
            (NULL == m_staticEntryMethods.GetHWBugWorkarounds)       ||
            (NULL == m_staticEntryMethods.QueryExternalComponentVendorTagsInfo))
        {
            CAMX_LOG_ERROR(CamxLogGroupHWL, "HWL static entry methods not completely defined");
            result = CamxResultEUnableToLoad;
        }
    }

    if (CamxResultEUnsupported == result)
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Unsupported camera platform family: %d, Version %d.%d.%d, cpas version %d.%d.%d",
                                   CSLPlatform.family,
                                   CSLPlatform.platformVersion.majorVersion,
                                   CSLPlatform.platformVersion.minorVersion,
                                   CSLPlatform.platformVersion.revVersion,
                                   CSLPlatform.CPASVersion.majorVersion,
                                   CSLPlatform.CPASVersion.minorVersion,
                                   CSLPlatform.CPASVersion.revVersion);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::ProbeImageSensorModules
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HwEnvironment::ProbeImageSensorModules()
{
    CamxResult                      result         = CamxResultSuccess;
    ImageSensorModuleDataManager*   pSensorManager = NULL;

    result = ImageSensorModuleDataManager::Create(&pSensorManager, this);

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupHWL, "ImageSensorModuleDataManager created");
        CAMX_ASSERT(NULL != pSensorManager);

        m_pImageSensorModuleDataManager = pSensorManager;

        const UINT moduleCount = m_pImageSensorModuleDataManager->GetNumberOfImageSensorModuleData();

        for (UINT i = 0; i < moduleCount; i++)
        {
            ImageSensorModuleData* pData = m_pImageSensorModuleDataManager->GetImageSensorModuleData(i);

            if (NULL != pData)
            {
                BOOL  detected       = FALSE;
                INT32 deviceIndex    = 0;
                UINT  cameraPosition = 0;

                pData->GetCameraPosition(&cameraPosition);

                if ((CameraPosition::FRONT_AUX == static_cast<CameraPosition>(cameraPosition)) &&
                    (FALSE == GetSettingsManager()->GetStaticSettings()->exportSecureCamera))
                {
                    CAMX_LOG_INFO(CamxLogGroupHWL, "Hide the secure camera due to the overridesettings.");
                    continue;
                }

                if (TRUE == pData->IsExternalSensor())
                {
                    // Since this is an external sensor we should not be doing any probe
                    // We should assume that the probe will be done by external and will be successful
                    result      = CamxResultSuccess;
                    detected    = TRUE;
                    // Since the deviceIndex is not used, assign invalid value.
                    deviceIndex = -1;
                    CAMX_LOG_INFO(CamxLogGroupHWL, "External Sensor detected");
                }
                else
                {
                    result = pData->Probe(&detected, &deviceIndex);
                }

                if ((CamxResultSuccess == result) && (TRUE == detected))
                {
                    CAMX_ASSERT(m_numberSensors < CamxMaxDeviceIndex);

                    m_sensorInfoTable[m_numberSensors].deviceIndex = deviceIndex;
                    m_sensorInfoTable[m_numberSensors].pData       = pData;

                    if (FALSE == pData->IsExternalSensor())
                    {
                        result = CSLQueryDeviceCapabilities(deviceIndex,
                            static_cast<VOID*>(&(m_sensorInfoTable[m_numberSensors].CSLCapability)),
                            sizeof(m_sensorInfoTable[m_numberSensors].CSLCapability));

                        if (CamxResultSuccess == result)
                        {
                            CAMX_LOG_VERBOSE(CamxLogGroupHWL,
                                "QueryCap results for Sensor %d (deviceIndex: %d) - "
                                "slotInfo: %u, secureCamera: %u, pitch: %u "
                                "roll: %u, yaw: %u, actuatorSlotId: %u "
                                "EEPROMSlotId: %u, OISSlotId: %u, flashSlotId: %u "
                                "CSIPHYSlotId: %u",
                                m_numberSensors,
                                deviceIndex,
                                m_sensorInfoTable[m_numberSensors].CSLCapability.slotInfo,
                                m_sensorInfoTable[m_numberSensors].CSLCapability.secureCamera,
                                m_sensorInfoTable[m_numberSensors].CSLCapability.pitch,
                                m_sensorInfoTable[m_numberSensors].CSLCapability.roll,
                                m_sensorInfoTable[m_numberSensors].CSLCapability.yaw,
                                m_sensorInfoTable[m_numberSensors].CSLCapability.actuatorSlotId,
                                m_sensorInfoTable[m_numberSensors].CSLCapability.EEPROMSlotId,
                                m_sensorInfoTable[m_numberSensors].CSLCapability.OISSlotId,
                                m_sensorInfoTable[m_numberSensors].CSLCapability.flashSlotId,
                                m_sensorInfoTable[m_numberSensors].CSLCapability.CSIPHYSlotId);
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupHWL, "CSLQueryCap failed for Sensor (deviceindex: %d)", deviceIndex);
                        }
                    }

                    /// @todo (CAMX-1215) - Do not create tuning data manager for YUV sensor.
                    result = CreateTuningDataManager(pData, m_numberSensors);

                    if (CamxResultSuccess == result)
                    {
                        m_numberSensors++;
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupHWL, "failed: Create tuning data manager %d ", result);
                    }
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupHWL, "Sensor not detected for deviceIndex = %d", deviceIndex);
                }

            }
        }

        result = m_pImageSensorModuleDataManager->DeleteEEBinFiles();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::EnumerateSensorDevices
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HwEnvironment::EnumerateSensorDevices()
{
    CamxResult          result = CamxResultSuccess;
    INT32               deviceIndex = 0;
    CSLDeviceDescriptor device;
    HwDeviceTypeInfo* pDeviceInfo;

    do
    {
        device.deviceIndex = deviceIndex++;
        result = CSLEnumerateDevices(&device);

        if (CamxResultSuccess == result)
        {
            if (CSLDeviceTypeImageSensor == device.deviceType)
            {
                pDeviceInfo = &(m_cslDeviceTable[device.deviceType]);

                pDeviceInfo->deviceIndex[pDeviceInfo->deviceIndexCount++] = device.deviceIndex;
                pDeviceInfo->hwVersion = device.hwVersion;
                pDeviceInfo->driverVersion = device.driverVersion;
            }
        }

    } while (CamxResultSuccess == result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::GetEEbinDeviceIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HwEnvironment::GetEEbinDeviceIndex(
    INT32           deviceIndex,
    CSLDeviceType   deviceType)
{
    CamxResult              result              = CamxResultSuccess;
    CSLEEPROMCapability     EEPROMCSLCapability = { 0 };

    if (CSLDeviceTypeEEPROM == deviceType)
    {
        result = CSLQueryDeviceCapabilities(deviceIndex,
            static_cast<VOID*>(&(EEPROMCSLCapability)),
            sizeof(EEPROMCSLCapability));
        if (CamxResultSuccess == result)
        {
            if (1 == EEPROMCSLCapability.multiModule)
            {
                CAMX_LOG_INFO(CamxLogGroupHWL, "Multi module supported");
                m_eebinDeviceIndex = deviceIndex;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::EnumerateDevices
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HwEnvironment::EnumerateDevices()
{
    CamxResult              result              = CamxResultSuccess;
    INT32                   deviceIndex         = 0;
    CSLDeviceDescriptor     device;

    m_eebinDeviceIndex = -1;    /// Initialization
    do
    {
        device.deviceIndex = deviceIndex++;
        result             = CSLEnumerateDevices(&device);

        if (CamxResultSuccess == result)
        {
            CAMX_ASSERT(device.deviceType >= 0 && device.deviceType < CSLDeviceTypeMaxDevice);

            HwDeviceTypeInfo* pDeviceInfo = &(m_cslDeviceTable[device.deviceType]);

            pDeviceInfo->deviceIndex[pDeviceInfo->deviceIndexCount++] = device.deviceIndex;
            pDeviceInfo->hwVersion                                    = device.hwVersion;
            pDeviceInfo->driverVersion                                = device.driverVersion;
            // check for eebin device index
            GetEEbinDeviceIndex(device.deviceIndex, device.deviceType);
        }

    } while (CamxResultSuccess == result);

    if (m_eebinDeviceIndex == -1)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupHWL, "multi module not supported");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::InitializeSensorSubModules
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HwEnvironment::InitializeSensorSubModules()
{
    UINT    index   = 0;

    for (index = 0; index < m_numberSensors ; index++)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupHWL, "Create Sensor sub modules for sensor index %d", index);
        m_sensorInfoTable[index].pData->CreateSensorSubModules(&m_sensorInfoTable[index], &m_cslDeviceTable[0]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::InitializeSensorStaticCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HwEnvironment::InitializeSensorStaticCaps()
{
    UINT    index   = 0;

    for (index = 0; index < m_numberSensors; index++)
    {
        m_sensorInfoTable[index].pData->GetSensorDataObject()->ConfigureImageSensorData(GetSensorVersion(index));

        m_sensorInfoTable[index].pData->GetStaticCaps(&(m_sensorInfoTable[index].moduleCaps),
                                                      m_pTuningManager[index], index);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::InitializeHwEnvironmentStaticCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HwEnvironment::InitializeHwEnvironmentStaticCaps()
{
    Utils::Memset(&m_caps, 0, sizeof(m_caps));

    InitializeAETargetFPSRangesStaticCaps();
    InitializeScalerStaticCaps();
    InitializeJPEGMaxSizeStaticCaps();
    if (NULL != m_pOEMInterface->pInitializeExtendedHWEnvironmentStaticCaps)
    {
        m_pOEMInterface->pInitializeExtendedHWEnvironmentStaticCaps(&m_caps[0], m_numberSensors);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::GetHWResourceInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HwEnvironment::GetHWResourceInfo(
    const SensorModuleStaticCaps*   pSensorCaps,
    const INT                       sensorIndex,
    INT32*                          pMaxIFEsRequired)
{
    FLOAT       maxPPS                  =  0.0f;
    INT         maxSensorWidth          =     0;
    const INT   hbiCount                =   140; // 64 pixel path + 76 HVX
    const INT   vbiCount                =    32;
    const FLOAT defaultOverlap          =  1.2f;
    const FLOAT minFPSForPPSCalculation = 60.0f;

    // Indices of the sensor configuration with maximum resource cost
    INT sensorConfigIndex       = 0;
    INT sensorStreamConfigIndex = 0;

    for (UINT configIndex = 0; configIndex < pSensorCaps->numSensorConfigs; ++configIndex)
    {
        const SensorConfiguration* pSensorConfig = &pSensorCaps->sensorConfigs[configIndex];
        FLOAT                      maxFPS        = static_cast<FLOAT>(pSensorConfig->maxFPS);

        if (minFPSForPPSCalculation >= maxFPS)
        {
            for (UINT streamIndex = 0; streamIndex < pSensorConfig->numStreamConfig; ++streamIndex)
            {
                const SensorStreamConfiguration* pSensorStreamConfig = &pSensorConfig->streamConfigs[streamIndex];

                if (StreamType::IMAGE == pSensorStreamConfig->streamType)
                {
                    INT32  sensorWidth  = pSensorStreamConfig->dimension.width;
                    INT32  sensorHeight = pSensorStreamConfig->dimension.height;

                    if ((TRUE == pSensorCaps->isQuadCFASensor) &&
                        (FALSE ==  GetSettingsManager()->GetStaticSettings()->exposeFullSizeForQCFA))
                    {
                        if ((sensorWidth  > (pSensorCaps->QuadCFADim.width>>1)) ||
                            (sensorHeight > (pSensorCaps->QuadCFADim.height>>1)))
                        {
                            continue; // this resolutions is not exposed. so skip the PPS computation
                        }
                    }

                    maxSensorWidth = Utils::MaxINT32(maxSensorWidth, sensorWidth);

                    FLOAT pixelsPerSec = (sensorWidth + hbiCount) * (sensorHeight + vbiCount) * maxFPS;

                    if (pixelsPerSec > maxPPS)
                    {
                        maxPPS                  = pixelsPerSec;
                        sensorConfigIndex       = configIndex;
                        sensorStreamConfigIndex = streamIndex;
                    }

                }
            }
        }
    }

    FLOAT maxIFEClockRateRequired = maxPPS * defaultOverlap / static_cast<FLOAT>(GetPlatformStaticCaps()->IFEPixelsPerClock);
    FLOAT defaultIFEClockRate     = static_cast<FLOAT>(GetPlatformStaticCaps()->minIFEHWClkRate);
    INT32 maxIFELineWidth         = GetPlatformStaticCaps()->IFEMaxLineWidth;
    INT32 maxIFELineWidthRequired = static_cast<INT32>(maxSensorWidth);

    if ((maxIFELineWidthRequired > maxIFELineWidth) || (maxIFEClockRateRequired > defaultIFEClockRate))
    {
        *pMaxIFEsRequired = 2;
    }
    else
    {
        *pMaxIFEsRequired = 1;
    }

    CAMX_LOG_CONFIG(CamxLogGroupHWL, "HW resource cost for sensor[%d] %s numIFEs needed %d maxPPS = %f"
        " clockRate threshold %f required %f"
        " Stream combination fps %f Dimension %dx%d maxLineWidthRequired %d lineWidth %d",
        sensorIndex,
        pSensorCaps->pSensorName,
        *pMaxIFEsRequired,
        maxPPS,
        defaultIFEClockRate,
        maxIFEClockRateRequired,
        pSensorCaps->sensorConfigs[sensorConfigIndex].maxFPS,
        pSensorCaps->sensorConfigs[sensorConfigIndex].streamConfigs[sensorStreamConfigIndex].dimension.width,
        pSensorCaps->sensorConfigs[sensorConfigIndex].streamConfigs[sensorStreamConfigIndex].dimension.height,
        maxIFELineWidthRequired,
        maxIFELineWidth);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::InitializeScalerStaticCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HwEnvironment::InitializeScalerStaticCaps()
{
    const SensorModuleStaticCaps*   pSensorCaps                     = NULL;
    const StaticSettings*           pStaticSettings                 = GetSettingsManager()->GetStaticSettings();
    const PlatformStaticCaps*       pPlatformCaps                   = NULL;
    UINT                            numStreamConfigs                = 0;
    UINT                            numHFRRanges                    = 0;
    UINT                            numMinFrameDurations            = 0;
    UINT                            numStallDurations               = 0;
    UINT                            numScalerFormats                = 0;
    INT32                           maxWidth                        = 0;
    INT32                           maxHeight                       = 0;

    Hashmap*                        pRawSizeMap;
    DimensionCap                    rawSize                         = { 0 };
    HashmapParams                   hashMapParams                   = { 0 };
    UINT32                          rawIndex                        = 0;        // dummy index for hash map only

    CSLCameraPlatform                CSLPlatform                    = {};
    CamxResult                       result                         = CamxResultSuccess;
    result                                                          = CSLQueryCameraPlatform(&CSLPlatform);

    if (CamxResultSuccess == result)
    {
        m_socId = CSLPlatform.socId;
    }

    hashMapParams.keySize       = sizeof(DimensionCap);
    hashMapParams.valSize       = sizeof(UINT32);
    hashMapParams.maxNumBuckets = MaxResolutions;

    pRawSizeMap                 = Hashmap::Create(&hashMapParams);

    CAMX_ASSERT(NULL != pRawSizeMap);
    CAMX_ASSERT(pStaticSettings != NULL);

    if (NULL == pRawSizeMap)
    {
        CAMX_LOG_INFO(CamxLogGroupHAL, "Out of memory, failed to create HashMap");
        return;
    }

    if (NULL == pStaticSettings)
    {
        CAMX_LOG_INFO(CamxLogGroupHAL, "Invalid static settings");
        return;
    }

    for (UINT sensorIndex = 0; sensorIndex < m_numberSensors; sensorIndex++)
    {
        UINT numCustomHFRParams      = 0;
        UINT numPreviewVideoParams   = 0;
        UINT numVideoMitigations     = 0;

        pPlatformCaps           = &m_platformCaps[sensorIndex];
        numScalerFormats        = pPlatformCaps->numScalerFormats;
        maxWidth                = 0;
        maxHeight               = 0;

        numStreamConfigs        = 0;
        numHFRRanges            = 0;
        numMinFrameDurations    = 0;
        numStallDurations       = 0;

        pSensorCaps             = &(m_sensorInfoTable[sensorIndex].moduleCaps);

        const ResolutionInformation* pResInfo =
            GetImageSensorModuleData(sensorIndex)->GetSensorDataObject()->GetResolutionInfo();

        CAMX_ASSERT(NULL != pSensorCaps);

        if (pSensorCaps->isDepthSensor)
        {
            // Assume depth sensor just supports one dimension
            INT32 width  = pSensorCaps->sensorConfigs->streamConfigs[0].dimension.width;
            INT32 height = pSensorCaps->sensorConfigs->streamConfigs[0].dimension.height;
            INT32 maxFPS = static_cast<INT32>(pSensorCaps->sensorConfigs[0].maxFPS);

            CAMX_LOG_INFO(CamxLogGroupHWL, "Found depth sensor, wxh = %dx%d", width, height);
            for (UINT format = 0; format < pPlatformCaps->numDepthFormats; format++)
            {
                if ((DepthAvailableFormatsY16  == pPlatformCaps->depthFormats[format]) ||
                    (DepthAvailableFormatsRawDepth  == pPlatformCaps->depthFormats[format]) ||
                    (DepthAvailableFormatsBlob == pPlatformCaps->depthFormats[format]) ||
                    (DepthAvailableFormatsImplementationDefined == pPlatformCaps->depthFormats[format]))
                {
                    SetAvailableStreamConfig(&(m_caps[sensorIndex].streamConfigs[numStreamConfigs]),
                        pPlatformCaps->depthFormats[format],
                        width,
                        height,
                        DepthAvailableDepthStreamConfigurationsOutput,
                        &numStreamConfigs);

                    SetAvailableMinFrameDuration(&(m_caps[sensorIndex].minFrameDurations[numMinFrameDurations]),
                        pPlatformCaps->depthFormats[format],
                        width,
                        height,
                        static_cast<INT64>(NanoSecondsPerSecond / maxFPS),
                        &numMinFrameDurations);

                    SetAvailableMinStallDuration(&(m_caps[sensorIndex].minStallDurations[numStallDurations]),
                        pPlatformCaps->depthFormats[format],
                        width,
                        height,
                        static_cast<INT64>(NanoSecondsPerSecond / maxFPS),
                        &numStallDurations);
                }
            }

            m_caps[sensorIndex].numStreamConfigs        = numStreamConfigs;
            m_caps[sensorIndex].numMinFrameDurations    = numMinFrameDurations;
            m_caps[sensorIndex].numStallDurations       = numStallDurations;

            pRawSizeMap->Clear();
            continue;
        }


        /////////////////////////////
        /// Adding HFR video sizes
        /////////////////////////////

        for (UINT imageIndex = 0; imageIndex < pPlatformCaps->numDefaultHFRVideoSizes; imageIndex++)
        {
            BOOL    bKeepImageSize = FALSE;
            INT32   width           = pPlatformCaps->defaultHFRVideoSizes[imageIndex].width;
            INT32   height          = pPlatformCaps->defaultHFRVideoSizes[imageIndex].height;
            INT32   maxFPS          = pPlatformCaps->defaultHFRVideoSizes[imageIndex].maxFPS;
            INT32   minFPS          = pPlatformCaps->defaultHFRVideoSizes[imageIndex].minFPS;

            for (UINT configIndex = 0; configIndex < pSensorCaps->numSensorConfigs; configIndex++)
            {
                if (static_cast<INT32>(pSensorCaps->sensorConfigs[configIndex].maxFPS) >= 60)
                {
                    for (UINT i = 0; i < pSensorCaps->sensorConfigs[configIndex].numStreamConfig; i++)
                    {
                        if (StreamType::IMAGE == pSensorCaps->sensorConfigs[configIndex].streamConfigs[i].streamType)
                        {
                            if ((width  <= pSensorCaps->sensorConfigs[configIndex].streamConfigs[i].dimension.width)  &&
                                (height <= pSensorCaps->sensorConfigs[configIndex].streamConfigs[i].dimension.height) &&
                                (maxFPS <=  pSensorCaps->sensorConfigs[configIndex].maxFPS))
                            {
                                bKeepImageSize  = TRUE;
                            }
                            break;
                        }
                    }
                }
            }
            if (TRUE == bKeepImageSize)
            {
                m_caps[sensorIndex].HFRVideoSizes[numHFRRanges].width = width;
                m_caps[sensorIndex].HFRVideoSizes[numHFRRanges].height = height;
                m_caps[sensorIndex].HFRVideoSizes[numHFRRanges].maxFPS = maxFPS;
                m_caps[sensorIndex].HFRVideoSizes[numHFRRanges].minFPS = minFPS;
                m_caps[sensorIndex].HFRVideoSizes[numHFRRanges].batchSizeMax =
                        pPlatformCaps->defaultHFRVideoSizes[imageIndex].batchSizeMax;
                numHFRRanges++;
            }
        }
        m_caps[sensorIndex].numHFRRanges        = numHFRRanges;


        if ((TRUE == pSensorCaps->isQuadCFASensor) && (FALSE == pStaticSettings->exposeFullSizeForQCFA))
        {
            maxWidth  = pSensorCaps->QuadCFADim.width  >> 1;
            maxHeight = pSensorCaps->QuadCFADim.height >> 1;
        }
        else
        {
            // Find out the max sensor output width*height.
            // We should use sensor output size, instead of platform defaultImageSizes, for downscale limitation calculation.
            for (UINT configIndex = 0; configIndex < pSensorCaps->numSensorConfigs; configIndex++)
            {
                for (UINT i = 0; i < pSensorCaps->sensorConfigs[configIndex].numStreamConfig; i++)
                {
                    if (StreamType::IMAGE == pSensorCaps->sensorConfigs[configIndex].streamConfigs[i].streamType)
                    {
                        INT32 sensorWidth  = pSensorCaps->sensorConfigs[configIndex].streamConfigs[i].dimension.width;
                        INT32 sensorHeight = pSensorCaps->sensorConfigs[configIndex].streamConfigs[i].dimension.height;

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
        CAMX_LOG_INFO(CamxLogGroupHWL, "Max sensor output dimension: %dx%d", maxWidth, maxHeight);
        // bounding supported resolutions between min and max MP
        // minInOutResolution and  maxInOutResolution are min and max values of resolutions of input
        // and output streams which could be supported
        INT32 minInOutResolution = static_cast<INT32>
                                   (GetSettingsManager()->GetStaticSettings()->minInOutResolution);
        INT32 maxInOutResolution = static_cast<INT32>
                                   (GetSettingsManager()->GetStaticSettings()->maxInOutResolution);
        CAMX_LOG_INFO(CamxLogGroupHWL, "Max input output stream resolution: %dMP "
                                        "Min output stream resolution: %dMP",
                                        maxInOutResolution, minInOutResolution);
        minInOutResolution = minInOutResolution * 1024 * 1024;
        maxInOutResolution = maxInOutResolution * 1024 * 1024;

        // Add non-Raw format scaler infomation
        for (UINT imageIndex = 0; imageIndex < pPlatformCaps->numDefaultImageSizes; imageIndex++)
        {
            BOOL    bKeepImageSize  = FALSE;
            INT32   width           = pPlatformCaps->defaultImageSizes[imageIndex].width;
            INT32   height          = pPlatformCaps->defaultImageSizes[imageIndex].height;
            INT32   maxFPS          = 0;
            INT32   highspeedFPS    = 0;
            // Cap to supported single IFE/TFE resolution if the setting is turn on
            if (TRUE == pStaticSettings->capResolutionForSingleIFE)
            {
                if ((width > pStaticSettings->singleIFESupportedWidth) ||
                    (height > pStaticSettings->singleIFESupportedHeight))
                {
                    continue;
                }
            }

            // For a given image size, find out the maximum fps sensor mode.
            for (UINT configIndex = 0; configIndex < pSensorCaps->numSensorConfigs; configIndex++)
            {
                if (static_cast<INT32>(pSensorCaps->sensorConfigs[configIndex].maxFPS) <= 60)
                {
                    for (UINT i = 0; i < pSensorCaps->sensorConfigs[configIndex].numStreamConfig; i++)
                    {
                        if (StreamType::IMAGE == pSensorCaps->sensorConfigs[configIndex].streamConfigs[i].streamType)
                        {
                            // Add 1920x1080 since cts tests need atleast one preview resolution in <= 1080p
                            if ((width  <= pSensorCaps->sensorConfigs[configIndex].streamConfigs[i].dimension.width)  &&
                                (height <= pSensorCaps->sensorConfigs[configIndex].streamConfigs[i].dimension.height) &&
                                (maxFPS <  pSensorCaps->sensorConfigs[configIndex].maxFPS) &&
                                ((((width * height) >= minInOutResolution) && ((width * height) <= maxInOutResolution)) ||
                                (width == 1920 && height == 1080)))
                            {
                                bKeepImageSize  = TRUE;
                                maxFPS          = static_cast<INT32>(pSensorCaps->sensorConfigs[configIndex].maxFPS);
                            }
                            break;
                        }
                    }
                }
            }

            // For a given image size, find out the maximum fps for HFR60 and HFR90 modes.
            for (UINT configIndex = 0; configIndex < pSensorCaps->numSensorConfigs; configIndex++)
            {
                if ((static_cast<INT32>(pSensorCaps->sensorConfigs[configIndex].maxFPS) > 30) &&
                    (static_cast<INT32>(pSensorCaps->sensorConfigs[configIndex].maxFPS) < 120))
                {
                    for (UINT i = 0; i < pSensorCaps->sensorConfigs[configIndex].numStreamConfig;
                        i++)
                    {
                        if (StreamType::IMAGE ==
                            pSensorCaps->sensorConfigs[configIndex].streamConfigs[i].streamType)
                        {
                            if ((width <=pSensorCaps->sensorConfigs[configIndex].
                                streamConfigs[i].dimension.width) &&
                                (height <= pSensorCaps->sensorConfigs[configIndex].
                                streamConfigs[i].dimension.height) &&
                                (highspeedFPS <  pSensorCaps->sensorConfigs[configIndex].maxFPS))
                            {
                                highspeedFPS =static_cast<INT32>
                                    (pSensorCaps->sensorConfigs[configIndex].maxFPS);
                            }
                            break;
                        }
                    }
                }
            }

            if ((IsSupportedVideoResolution(pPlatformCaps, width, height)) &&
                ((CSLCameraTitanSocSM6350 == CSLPlatform.socId) || (CSLCameraTitanSocSM7225 == CSLPlatform.socId)))
            {
                INT32 fps = 30;
                while (highspeedFPS - fps >= 0)
                {
                    UINT32 isEISSupported      = 1;
                    UINT32 isLiveShotSupported = 1;

                    // EIS not supported for resolutions < 720p in Bitra
                    if (height < 720)
                    {
                        isEISSupported = 0;
                    }

                    SetVideoEISLiveshotParams
                                    (&(m_caps[sensorIndex].videoMitigationsTable[numVideoMitigations]),
                                    width,
                                    height,
                                    pPlatformCaps->maxPreviewFPS,
                                    fps,
                                    isLiveShotSupported,
                                    isEISSupported,
                                    &numVideoMitigations);
                    fps += 30;
                }
            }

            if (CustomHFR60Fps == highspeedFPS)
            {
                // For SDM670/710 disable 4KUHD HFR/HSR 60 option
                if ((CamxResultSuccess == result) && (!(((CSLPlatform.socId == CSLCameraTitanSocSDM670)              ||
                    (CSLPlatform.socId == CSLCameraTitanSocSDM710) || (CSLPlatform.socId == CSLCameraTitanSocSDM712)) &&
                    (width*height >= 3840*2160))))
                {
                    SetAvailableCustomHFRConfig
                    (&(m_caps[sensorIndex].customHFRParams[numCustomHFRParams]),
                    width,
                    height,
                    highspeedFPS,
                    &numCustomHFRParams);

                    INT32 previewfps = CustomHFR30Fps;
                    INT32 previewvideofpsdelta = 0;
                    // For SM7250, previewFPS 60 is not supported. Need to do the same for
                    // kamorta as well after SOCID support is added.
                    if ((CSLPlatform.socId == CSLCameraTitanSocSM7250) || (CSLPlatform.socId == CSLCameraTitanSocQSM7250))
                    {
                        previewvideofpsdelta = 30;
                    }
                    while ((highspeedFPS - previewfps) >= previewvideofpsdelta)
                    {
                        if (CamxResultSuccess == result)
                        {
                            SetAvailableCustomHFRPreviewVideoConfig
                            (&(m_caps[sensorIndex].supportedHFRPreviewVideoFPS[numPreviewVideoParams]),
                            width,
                            height,
                            previewfps,
                            highspeedFPS,
                            &numPreviewVideoParams);
                        }
                        previewfps += 30;
                    }
                }
            }

            if ((TRUE == pSensorCaps->isQuadCFASensor) && (FALSE == pStaticSettings->exposeFullSizeForQCFA))
            {
                // only expose quarter size for Quad CFA senosr.
                if ((width > pSensorCaps->QuadCFADim.width >> 1) || (height > pSensorCaps->QuadCFADim.height >> 1))
                {
                    CAMX_LOG_INFO(CamxLogGroupHWL, "Quad CFA quarter size:%dx%d, skip size:%dx%d",
                        pSensorCaps->QuadCFADim.width  >> 1,
                        pSensorCaps->QuadCFADim.height >> 1,
                        width,
                        height);

                    continue;
                }
            }

            if (TRUE == bKeepImageSize)
            {
                // StreamConfigurations
                for (UINT format = 0; format < numScalerFormats; format++)
                {
                    if (ScalerAvailableFormatsBlob == pPlatformCaps->scalerFormats[format])
                    {
                        // Limit blob resolution to downscale capability.
                        if (((maxWidth / width) < pPlatformCaps->maxDownscaleRatio )&&
                            ((maxHeight / height) < pPlatformCaps->maxDownscaleRatio ))
                        {
                            SetAvailableStreamConfig(&(m_caps[sensorIndex].streamConfigs[numStreamConfigs]),
                                                     pPlatformCaps->scalerFormats[format],
                                                     width,
                                                     height,
                                                     ScalerAvailableStreamConfigurationsOutput,
                                                     &numStreamConfigs);
                        }
                    }

                    if ((ScalerAvailableFormatsImplementationDefined == pPlatformCaps->scalerFormats[format]) ||
                        (ScalerAvailableFormatsYCbCr420888           == pPlatformCaps->scalerFormats[format]))
                    {
                        SetAvailableStreamConfig(&(m_caps[sensorIndex].streamConfigs[numStreamConfigs]),
                                                 pPlatformCaps->scalerFormats[format],
                                                 width,
                                                 height,
                                                 ScalerAvailableStreamConfigurationsOutput,
                                                 &numStreamConfigs);
                    }

                    if ((ScalerAvailableFormatsYCbCr420888           == pPlatformCaps->scalerFormats[format]) ||
                        (ScalerAvailableFormatsImplementationDefined == pPlatformCaps->scalerFormats[format]))
                    {
                        INT32 minReprocessInputWidth  = static_cast<INT32>
                                                        (GetSettingsManager()->GetStaticSettings()->minReprocessInputWidth);
                        INT32 minReprocessInputHeight = static_cast<INT32>
                                                        (GetSettingsManager()->GetStaticSettings()->minReprocessInputHeight);

                        /* Check for Sensor Capability is more than 5MP(2592x1944)*/
                        if ((maxWidth * maxHeight) > (minReprocessInputWidth * minReprocessInputHeight))
                        {
                            /* Cap minimum reprocess width of input dimension supported by HAL.
                            By default all sizes upto 5MP(2592x1944) will be advertised.*/
                            if ((width * height)  >= (minReprocessInputWidth * minReprocessInputHeight))
                            {
                                SetAvailableStreamConfig(&(m_caps[sensorIndex].streamConfigs[numStreamConfigs]),
                                                         pPlatformCaps->scalerFormats[format],
                                                         width,
                                                         height,
                                                         ScalerAvailableStreamConfigurationsInput,
                                                         &numStreamConfigs);
                            }
                        }
                        else
                        {
                            SetAvailableStreamConfig(&(m_caps[sensorIndex].streamConfigs[numStreamConfigs]),
                                                     pPlatformCaps->scalerFormats[format],
                                                     width,
                                                     height,
                                                     ScalerAvailableStreamConfigurationsInput,
                                                     &numStreamConfigs);
                        }
                    }
                }

                // MinFrameDurations
                for (UINT format = 0; format < numScalerFormats; format++)
                {
                    if ((ScalerAvailableFormatsYCbCr420888           == pPlatformCaps->scalerFormats[format]) ||
                        (ScalerAvailableFormatsImplementationDefined == pPlatformCaps->scalerFormats[format]) ||
                        (ScalerAvailableFormatsBlob                  == pPlatformCaps->scalerFormats[format]))
                    {
                        INT supportedFPS             = 0;
                        INT maxBPSProcessingFPS      = 0;
                        INT maxIPEProcessingFPS       = 0;
                        INT maxNonHfrFps             =
                            static_cast<INT32>(GetSettingsManager()->GetStaticSettings()->maxNonHfrFps);

                        // 1. Limit minframeduration based upon maxfps published.
                        maxFPS = Utils::MinUINT32(maxFPS, maxNonHfrFps);

                        // 2. calculate the FPS throughput from IPE/BPS depending on the format
                        if ((ScalerAvailableFormatsYCbCr420888           == pPlatformCaps->scalerFormats[format]) ||
                            (ScalerAvailableFormatsBlob                  == pPlatformCaps->scalerFormats[format]))
                        {

                            // For snapshot (blob, and yuv snapshot stream) consider IPE running at Nominal clock
                            // Also, consider 1080P preview processing parallelly in IPE. Add this as well for processing.
                            maxIPEProcessingFPS = static_cast<UINT>(((pPlatformCaps->maxIPEProcessingClockOffline) /
                                                       ((width * height * pPlatformCaps->offlineIPEOverhead /
                                                           pPlatformCaps->offlineIPEEfficiency) +
                                                       (1920 * 1080 * pPlatformCaps->realtimeIPEOverhead /
                                                           pPlatformCaps->realtimeIPEEfficiency))));

                            maxBPSProcessingFPS      = ((pPlatformCaps->maxBPSProcessingClockNom) / (width * height));
                        }
                        else
                        {
                            // For video/preview (implementation defined format), consider IPE clock as SVS_L1
                            maxIPEProcessingFPS = static_cast<UINT>(((pPlatformCaps->maxIPEProcessingClockRealtime *
                                                    pPlatformCaps->realtimeIPEEfficiency) /
                                                    (width * height * pPlatformCaps->realtimeIPEOverhead)));

                            // For video/preview, we don't use BPS so just assign the maxFPS
                            maxBPSProcessingFPS      = maxFPS;
                        }

                        // 3. Supported FPS will be min of maxFPS, BPS FPS and IPE FPS.
                        supportedFPS = maxFPS;
                        supportedFPS = Utils::MinUINT32(supportedFPS, maxBPSProcessingFPS);
                        supportedFPS = Utils::MinUINT32(supportedFPS, maxIPEProcessingFPS);


                        supportedFPS = ClosestMaxFps(supportedFPS, sensorIndex);

                        CAMX_LOG_VERBOSE(CamxLogGroupHWL, "supportedFPS = %d BPS = %d, IPE = %d for width x height = %d x %d",
                                 supportedFPS, maxBPSProcessingFPS, maxIPEProcessingFPS, width, height);

                        SetAvailableMinFrameDuration(&(m_caps[sensorIndex].minFrameDurations[numMinFrameDurations]),
                                                     pPlatformCaps->scalerFormats[format],
                                                     width,
                                                     height,
                                                     static_cast<INT64>(NanoSecondsPerSecond / supportedFPS),
                                                     &numMinFrameDurations);
                    }
                }

                // StallDurations
                SetAvailableMinStallDuration(&(m_caps[sensorIndex].minStallDurations[numStallDurations]),
                                             ScalerAvailableFormatsBlob,
                                             width,
                                             height,
                                             static_cast<INT64>(width * height * pPlatformCaps->JPEGFormatStallFactorPerPixel),
                                             &numStallDurations);
            }
        }

        // Add Raw format scaler infomation
        /// @todo (CAMX-2268) Get supported raw size for presil test
        if ((CSLPresilEnabled     == GetCSLMode()) ||
            (CSLPresilRUMIEnabled == GetCSLMode()))
        {
            rawSize.width  = 640;
            rawSize.height = 480;

            for (UINT format = 0; format < numScalerFormats; format++)
            {
                if ((ScalerAvailableFormatsRaw16     == pPlatformCaps->scalerFormats[format]) ||
                    (ScalerAvailableFormatsRawOpaque == pPlatformCaps->scalerFormats[format]) ||
                    (ScalerAvailableFormatsRaw10     == pPlatformCaps->scalerFormats[format]))
                {
                    SetAvailableStreamConfig(&(m_caps[sensorIndex].streamConfigs[numStreamConfigs]),
                                             pPlatformCaps->scalerFormats[format],
                                             rawSize.width,
                                             rawSize.height,
                                             ScalerAvailableStreamConfigurationsOutput,
                                             &numStreamConfigs);

                    SetAvailableStreamConfig(&(m_caps[sensorIndex].streamConfigs[numStreamConfigs]),
                                             pPlatformCaps->scalerFormats[format],
                                             rawSize.width,
                                             rawSize.height,
                                             ScalerAvailableStreamConfigurationsInput,
                                             &numStreamConfigs);
                }
            }
        }
        else
        {
            // Check all the sensor supported raw size and expose as per maxRAWSizes setting
            UINT rawSizes = pStaticSettings->maxRAWSizes;
            for (UINT configIndex = 0; (configIndex < rawSizes) && (configIndex < pSensorCaps->numSensorConfigs);
                configIndex++)
            {
                BOOL isNormal  = FALSE;
                BOOL isQuadCFA = FALSE;

                if (static_cast<INT32>(pSensorCaps->sensorConfigs[configIndex].maxFPS) <= 60)
                {
                    for (UINT i = 0; i < pSensorCaps->sensorConfigs[configIndex].numStreamConfig; i++)
                    {
                        if (StreamType::IMAGE == pSensorCaps->sensorConfigs[configIndex].streamConfigs[i].streamType)
                        {
                            rawSize.width = pSensorCaps->sensorConfigs[configIndex].streamConfigs[i].dimension.width;
                            rawSize.height = pSensorCaps->sensorConfigs[configIndex].streamConfigs[i].dimension.height;

                            UINT numCaps = pResInfo->resolutionData[configIndex].capabilityCount;

                            for (UINT j = 0; j < numCaps; j++)
                            {
                                if (pResInfo->resolutionData[configIndex].capability[j] == SensorCapability::NORMAL)
                                {
                                    isNormal = TRUE;
                                }
                                else if (SensorCapability::QUADCFA == pResInfo->resolutionData[configIndex].capability[j])
                                {
                                    isQuadCFA = TRUE;
                                }
                            }

                            if ((TRUE == isQuadCFA) && (FALSE == pStaticSettings->exposeFullSizeForQCFA))
                            {
                                CAMX_LOG_INFO(CamxLogGroupHWL, "Quad CFA raw dim: %dx%d, check next for normal bayer raw",
                                    rawSize.width, rawSize.height);
                                rawSizes += 1;
                                break;
                            }

                            // Cap to supoported single IFE resolution if the setting is turn on
                            if (TRUE == pStaticSettings->capResolutionForSingleIFE)
                            {
                                if ((rawSize.width > pStaticSettings->singleIFESupportedWidth) ||
                                    (rawSize.height > pStaticSettings->singleIFESupportedHeight))
                                {
                                    rawSizes += 1;
                                    break;
                                }
                            }

                            if ((TRUE == isNormal) || (TRUE == isQuadCFA))
                            {
                                // Check if the raw size was added already. Avoid expose the same value twice.
                                if (CamxResultSuccess != pRawSizeMap->Get(&rawSize, &rawIndex))
                                {
                                    pRawSizeMap->Put(&rawSize, &rawIndex);
                                    for (UINT format = 0; format < numScalerFormats; format++)
                                    {
                                        if ((ScalerAvailableFormatsRaw16     == pPlatformCaps->scalerFormats[format]) ||
                                            (ScalerAvailableFormatsRawOpaque == pPlatformCaps->scalerFormats[format]) ||
                                            (ScalerAvailableFormatsRaw10     == pPlatformCaps->scalerFormats[format]))
                                        {
                                            SetAvailableStreamConfig(&(m_caps[sensorIndex].streamConfigs[numStreamConfigs]),
                                                pPlatformCaps->scalerFormats[format],
                                                rawSize.width,
                                                rawSize.height,
                                                ScalerAvailableStreamConfigurationsOutput,
                                                &numStreamConfigs);
                                            SetAvailableMinFrameDuration(
                                                &(m_caps[sensorIndex].minFrameDurations[numMinFrameDurations]),
                                                pPlatformCaps->scalerFormats[format],
                                                rawSize.width,
                                                rawSize.height,
                                                static_cast<INT64>(NanoSecondsPerSecond /
                                                    pSensorCaps->sensorConfigs[configIndex].maxFPS),
                                                &numMinFrameDurations);
                                            if ((ScalerAvailableFormatsRawOpaque ==
                                                pPlatformCaps->scalerFormats[format]) &&
                                                (m_caps[sensorIndex].opaqueRawSizesCount < MaxOpaqueRawSizes))
                                            {
                                                // Calculation of RAW size as MIPI RAW10
                                                UINT count = m_caps[sensorIndex].opaqueRawSizesCount;
                                                m_caps[sensorIndex].opaqueRawSizes[count].width  = rawSize.width;
                                                m_caps[sensorIndex].opaqueRawSizes[count].height = rawSize.height;
                                                m_caps[sensorIndex].opaqueRawSizes[count].size   = CalculateRawSize(
                                                            pSensorCaps->sensorConfigs[configIndex].streamConfigs[0].bitWidth,
                                                            rawSize.width, rawSize.height);
                                                m_caps[sensorIndex].opaqueRawSizesCount++;
                                            }

                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        GetHWResourceInfo(pSensorCaps, sensorIndex, &m_platformCaps[sensorIndex].maxNumberOfIFEsRequired);

        m_caps[sensorIndex].numStreamConfigs            = numStreamConfigs;
        m_caps[sensorIndex].numMinFrameDurations        = numMinFrameDurations;
        m_caps[sensorIndex].numStallDurations           = numStallDurations;
        m_caps[sensorIndex].numCustomHFRParams          = numCustomHFRParams;
        m_caps[sensorIndex].numSupportedPreviewVideoFPS = numPreviewVideoParams;
        m_caps[sensorIndex].numVideoMitigations         = numVideoMitigations;

        pRawSizeMap->Clear();
    }

    pRawSizeMap->Destroy();
    pRawSizeMap = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::InitializeJPEGMaxSizeStaticCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HwEnvironment::InitializeJPEGMaxSizeStaticCaps()
{
    for (UINT sensorIndex = 0; sensorIndex < m_numberSensors; sensorIndex++)
    {
        INT32 maxSizeInBytes                    = 0;
        m_caps[sensorIndex].JPEGMaxSizeInBytes  = 0;

        for (UINT i = 0; i < m_caps[sensorIndex].numStallDurations; i++)
        {
            // Max JPEG buffer size is max JPEG image buffer size + JPEG header + Max Exif size + DebugDataSize + Padding

            maxSizeInBytes = static_cast<INT32>((m_caps[sensorIndex].minStallDurations[i].width *
                m_caps[sensorIndex].minStallDurations[i].height * JpegMult) + sizeof(Camera3JPEGBlob) + EXIFSize +
                JpegPadding + HAL3MetadataUtil::DebugDataSize(DebugDataType::AllTypes));

            if (m_caps[sensorIndex].JPEGMaxSizeInBytes < maxSizeInBytes)
            {
                m_caps[sensorIndex].JPEGMaxSizeInBytes = maxSizeInBytes;
            }
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FPSSizeMaxSort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT FPSSizeMaxSort(
    const VOID* pArg0,
    const VOID* pArg1)
{
    const RangeINT32* pFirst     = static_cast<const RangeINT32*>(pArg0);
    const RangeINT32* pSecond    = static_cast<const RangeINT32*>(pArg1);
    INT               comparison = 0;

    if (pFirst->max < pSecond->max)
    {
        comparison = -1;
    }
    else
    {
        comparison = 1;
    }

    return comparison;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FPSSizeMinSort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT FPSSizeMinSort(
    const VOID* pArg0,
    const VOID* pArg1)
{
    const RangeINT32* pFirst     = static_cast<const RangeINT32*>(pArg0);
    const RangeINT32* pSecond    = static_cast<const RangeINT32*>(pArg1);
    INT               comparison = 0;

    if (pFirst->min > pSecond->min)
    {
        comparison = -1;
    }
    else
    {
        comparison = 1;
    }

    return comparison;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::InitializeAETargetFPSRangesStaticCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HwEnvironment::InitializeAETargetFPSRangesStaticCaps()
{
    UINT                            numFPSRanges    = 0;
    const SensorModuleStaticCaps*   pSensorCaps     = NULL;
    const PlatformStaticCaps*       pPlatformCaps   = NULL;

    for (UINT sensorIndex = 0; sensorIndex < m_numberSensors; sensorIndex++)
    {
        pPlatformCaps   = &m_platformCaps[sensorIndex];
        pSensorCaps     = &(m_sensorInfoTable[sensorIndex].moduleCaps);
        numFPSRanges    = 0;

        CAMX_ASSERT(NULL != pSensorCaps);

        // AE available target FPS ranges
        for (UINT configIndex = 0; configIndex < pSensorCaps->numSensorConfigs; configIndex++)
        {
            if (static_cast<UINT>(pSensorCaps->sensorConfigs[configIndex].maxFPS) <=
                GetSettingsManager()->GetStaticSettings()->maxNonHfrFps)
            {
                for (UINT i = 0; i < pSensorCaps->sensorConfigs[configIndex].numStreamConfig; i++)
                {
                    BOOL bAddFPSRange   = TRUE;
                    BOOL bAddMaxFPS     = TRUE;

                    if (StreamType::IMAGE == pSensorCaps->sensorConfigs[configIndex].streamConfigs[i].streamType)
                    {
                        for (UINT rangeIndex = 0; rangeIndex < numFPSRanges; rangeIndex++)
                        {
                            if ((m_caps[sensorIndex].AETargetFPSRanges[rangeIndex].min ==
                                 static_cast<INT32>(pSensorCaps->sensorConfigs[configIndex].minFPS)) &&
                                (m_caps[sensorIndex].AETargetFPSRanges[rangeIndex].max ==
                                 static_cast<INT32>(pSensorCaps->sensorConfigs[configIndex].maxFPS)))
                            {
                                bAddFPSRange = FALSE;
                                break;
                            }
                        }

                        for (UINT rangeIndex = 0; rangeIndex < numFPSRanges; rangeIndex++)
                        {
                            if ((m_caps[sensorIndex].AETargetFPSRanges[rangeIndex].min ==
                                 static_cast<INT32>(pSensorCaps->sensorConfigs[configIndex].maxFPS)) &&
                                (m_caps[sensorIndex].AETargetFPSRanges[rangeIndex].max ==
                                 static_cast<INT32>(pSensorCaps->sensorConfigs[configIndex].maxFPS)))
                            {
                                bAddMaxFPS = FALSE;
                                break;
                            }
                        }

                        if (TRUE == bAddFPSRange)
                        {
                            // Add Range[min, max]
                            m_caps[sensorIndex].AETargetFPSRanges[numFPSRanges].min = static_cast<INT32>
                                (pSensorCaps->sensorConfigs[configIndex].minFPS);
                            m_caps[sensorIndex].AETargetFPSRanges[numFPSRanges].max = static_cast<INT32>
                                (pSensorCaps->sensorConfigs[configIndex].maxFPS);
                            numFPSRanges++;
                        }

                        if (TRUE == bAddMaxFPS)
                        {
                            m_caps[sensorIndex].AETargetFPSRanges[numFPSRanges].min = static_cast<INT32>
                                (pSensorCaps->sensorConfigs[configIndex].maxFPS);
                            m_caps[sensorIndex].AETargetFPSRanges[numFPSRanges].max = static_cast<INT32>
                                (pSensorCaps->sensorConfigs[configIndex].maxFPS);
                            numFPSRanges++;
                        }
                    }
                }
            }
        }
        UINT sensorNumFPSRanges  = numFPSRanges;
        UINT defFpsRangeIndex    = 0;
        BOOL bAddDefaultFPSRange = FALSE;
        for (UINT defIndex = 0; defIndex < pPlatformCaps->numDefaultFpsRanges; defIndex++)
        {
            bAddDefaultFPSRange = TRUE;
            for (UINT rangeIndex = 0; rangeIndex < sensorNumFPSRanges; rangeIndex++)
            {
                if ((m_caps[sensorIndex].AETargetFPSRanges[rangeIndex].min ==
                    pPlatformCaps->defaultTargetFpsRanges[defIndex].min) &&
                    (m_caps[sensorIndex].AETargetFPSRanges[rangeIndex].max ==
                    pPlatformCaps->defaultTargetFpsRanges[defIndex].max))
                {
                    bAddDefaultFPSRange = FALSE;
                    break;
                }
            }
            if (bAddDefaultFPSRange == TRUE)
            {
                for (UINT rangeIndex = 0; rangeIndex < sensorNumFPSRanges; rangeIndex++)
                {
                    if ((m_caps[sensorIndex].AETargetFPSRanges[rangeIndex].min <=
                        pPlatformCaps->defaultTargetFpsRanges[defIndex].min) &&
                        (m_caps[sensorIndex].AETargetFPSRanges[rangeIndex].max >=
                            pPlatformCaps->defaultTargetFpsRanges[defIndex].max))
                    {
                        m_caps[sensorIndex].AETargetFPSRanges[numFPSRanges].min =
                            pPlatformCaps->defaultTargetFpsRanges[defIndex].min;
                        m_caps[sensorIndex].AETargetFPSRanges[numFPSRanges].max =
                            pPlatformCaps->defaultTargetFpsRanges[defIndex].max;
                        numFPSRanges++;
                        break;
                    }
                }
            }
        }
        m_caps[sensorIndex].numAETargetFPSRanges = numFPSRanges;
        // CTS expects the list to be sorted
        Utils::Qsort(&m_caps[sensorIndex].AETargetFPSRanges[0], numFPSRanges, sizeof(m_caps[sensorIndex].AETargetFPSRanges[0]),
                    FPSSizeMinSort);
        Utils::Qsort(&m_caps[sensorIndex].AETargetFPSRanges[0], numFPSRanges, sizeof(m_caps[sensorIndex].AETargetFPSRanges[0]),
                    FPSSizeMaxSort);

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::CreateTuningDataManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HwEnvironment::CreateTuningDataManager(
    ImageSensorModuleData* pData,
    UINT                   sensorIndex)
{
    CamxResult result = CamxResultEFailed;
    m_pTuningManager[sensorIndex] = TuningDataManager::Create();

    if (NULL != m_pTuningManager[sensorIndex])
    {
        UINT16            fileCount                                = 0;
        const CHAR*       pSensorName                              = pData->GetSensorDataObject()->GetSensorName();
        const CHAR*       pChromatixName                           = pData->GetChromatixName();
        CHAR              pChromatixName_SM7150[FILENAME_MAX]      = {};
        CHAR              pChromatixName_SM6350[FILENAME_MAX]      = {};
        CHAR              binFiles[MaxSensorModules][FILENAME_MAX] = { "" };
        CSLCameraPlatform CSLPlatform                              = {};
        BOOL              is_SM6350                                = FALSE;

        result = CSLQueryCameraPlatform(&CSLPlatform);

        BOOL is_SM7150 = (CSLPlatform.socId == CSLCameraTitanSocSM7150 || CSLPlatform.socId == CSLCameraTitanSocSM7150P);
        if (is_SM7150 == FALSE)
        {
            is_SM6350 = (CSLPlatform.socId == CSLCameraTitanSocSM6350 || CSLPlatform.socId == CSLCameraTitanSocSM7225);
        }

        OsUtils::SNPrintF(pChromatixName_SM7150, FILENAME_MAX, "%s%s", "SM7150_", pChromatixName);
        OsUtils::SNPrintF(pChromatixName_SM6350, FILENAME_MAX, "%s%s", "bitra_", pChromatixName);

        CAMX_ASSERT(NULL != pSensorName);
        CAMX_ASSERT(NULL != pChromatixName);
        CAMX_ASSERT(NULL != pChromatixName_SM7150);
        CAMX_ASSERT(NULL != pChromatixName_SM6350);

        if (is_SM7150 == TRUE && '\0' != *pChromatixName_SM7150)
        {
            fileCount = OsUtils::GetFilesFromPath(SensorModulesPath,
                                                  FILENAME_MAX,
                                                  &binFiles[0][0],
                                                  "*",
                                                  "tuned",
                                                  pChromatixName_SM7150,
                                                  "*",
                                                  "bin");

            CAMX_LOG_INFO(CamxLogGroupHWL, "Searching SM7150 Specific tuning file");
        }

        if (is_SM6350 == TRUE && '\0' != *pChromatixName_SM6350)
        {
            fileCount = OsUtils::GetFilesFromPath(SensorModulesPath,
                                                  FILENAME_MAX,
                                                  &binFiles[0][0],
                                                  "*",
                                                  "tuned",
                                                  pChromatixName_SM6350,
                                                  "*",
                                                  "bin");

            CAMX_LOG_INFO(CamxLogGroupHWL, "Searching SM6350 Specific tuning file");
        }

        if (fileCount == 0 && (NULL != pChromatixName && '\0' != *pChromatixName))
        {
            fileCount = OsUtils::GetFilesFromPath(MmSensorModulesPath,
                                                  FILENAME_MAX,
                                                  &binFiles[0][0],
                                                  "*",
                                                  "tuned",
                                                  pChromatixName,
                                                  "*",
                                                  "bin");

            if (0 == fileCount)
            {
                CAMX_LOG_INFO(CamxLogGroupHWL, "Searching regular tuning file");
                fileCount = OsUtils::GetFilesFromPath(SensorModulesPath,
                                                      FILENAME_MAX,
                                                      &binFiles[0][0],
                                                      "*",
                                                      "tuned",
                                                      pChromatixName,
                                                      "*",
                                                      "bin");
            }
        }

        if (0 == fileCount)
        {
            CAMX_LOG_INFO(CamxLogGroupHWL, "No tuning data file for sensor: %s, assigning default", pSensorName);
            fileCount = OsUtils::GetFilesFromPath(SensorModulesPath,
                                                  FILENAME_MAX,
                                                  &binFiles[0][0],
                                                  "*",
                                                  "tuned",
                                                  "default",
                                                  "*",
                                                  "bin");
        }

        if (0 != fileCount && NULL != pSensorName)
        {
            result = m_pTuningManager[sensorIndex]->LoadTunedDataFile(binFiles[0]);
        }

        if (CamxResultSuccess == result)
        {
            result = m_pTuningManager[sensorIndex]->CreateTunedModeTree();
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupHWL, "Failed to load tuning data for sensor index: %d", sensorIndex);
            m_pTuningManager[sensorIndex]->Destroy();
            m_pTuningManager[sensorIndex] = NULL;
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupHWL, "Loaded tuning file: %s for sensor index: %d", &binFiles[0][0], sensorIndex);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHWL, "Couldn't create Tuning manager for sensor index: %d", sensorIndex);
        result = CamxResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::GetImageSensorModuleData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const ImageSensorModuleData* HwEnvironment::GetImageSensorModuleData(
    UINT32          cameraID
    ) const
{
    CAMX_ASSERT(cameraID < m_numberSensors);

    return m_sensorInfoTable[cameraID].pData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::IsHWBugWorkaroundEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HwEnvironment::IsHWBugWorkaroundEnabled(
    UINT32 workaroundId
    ) const
{
    CAMX_ASSERT(workaroundId < m_workarounds.numWorkarounds);

    BOOL enabled = FALSE;

    if (CSLPresilEnabled == GetCSLMode())
    {
        enabled = m_workarounds.pWorkarounds[workaroundId].presilCSIMEnabled;
    }
    else if (CSLPresilRUMIEnabled == GetCSLMode())
    {
        enabled = m_workarounds.pWorkarounds[workaroundId].presilRUMIEnabled;
    }
    else
    {
        enabled = m_workarounds.pWorkarounds[workaroundId].enabled;
    }

    return enabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::GetDeviceVersion
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HwEnvironment::GetDeviceVersion(
    CSLDeviceType   deviceType,
    CSLVersion*     pVersion
    ) const
{
    CamxResult  result      = CamxResultSuccess;
    INT         typeIndex   = static_cast<INT>(deviceType);

    if ((NULL != pVersion) && (typeIndex >= 0) && (typeIndex < static_cast<INT>(CSLDeviceTypeMaxDevice)))
    {
        *pVersion = m_cslDeviceTable[typeIndex].hwVersion;
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid arguments");
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::GetDriverVersion
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HwEnvironment::GetDriverVersion(
    CSLDeviceType   deviceType,
    CSLVersion*     pVersion
    ) const
{
    CamxResult  result      = CamxResultSuccess;
    INT         typeIndex   = static_cast<INT>(deviceType);

    if ((NULL != pVersion) && (typeIndex >= 0) && (typeIndex < static_cast<INT>(CSLDeviceTypeMaxDevice)))
    {
        *pVersion = m_cslDeviceTable[typeIndex].driverVersion;
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid arguments");
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::GetSensorVersion
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 HwEnvironment::GetSensorVersion(
    UINT32   cameraId
    ) const
{
    UINT32 sensorVersion = 0;

    if (cameraId < m_numberSensors)
    {
        const EEPROMInformation* pEEPROMInfo = NULL;

        pEEPROMInfo = &m_sensorInfoTable[cameraId].moduleCaps.OTPData.EEPROMInfo;

        for (UINT count = 0; count < pEEPROMInfo->customInfoCount; count++)
        {
            if (0 == OsUtils::StrCmp("SensorVersionOffset", pEEPROMInfo->customInfo[count].name))
            {
                INT32 offset = pEEPROMInfo->customInfo[count].value;

                if (offset >= 0)
                {
                    sensorVersion = *(reinterpret_cast<UINT32*>(pEEPROMInfo->rawOTPData.pRawData + offset));
                }
                break;
            }
        }
    }

    return sensorVersion;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::GetDeviceIndices
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HwEnvironment::GetDeviceIndices(
    CSLDeviceType   deviceType,
    INT32*          pDeviceIndices,
    UINT            deviceIndicesLength,
    UINT*           pDeviceIndicesLengthRequired
    ) const
{
    CamxResult  result = CamxResultSuccess;
    INT         typeIndex = static_cast<INT>(deviceType);

    if ((NULL == pDeviceIndicesLengthRequired)                   || // pDeviceIndicesLengthRequired must not be NULL
        ((NULL == pDeviceIndices) && (deviceIndicesLength != 0)) || // pDeviceIndices can be NULL if pDeviceIndicesLength is 0
        ((typeIndex < 0) || (typeIndex >= CSLDeviceTypeMaxDevice))) // device type index must be in range
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid arguments index: %d, DeviceIndices: %p, DeviceIndicesLength: %p",
                                   typeIndex, pDeviceIndices, pDeviceIndicesLengthRequired);
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        const HwDeviceTypeInfo* pDeviceInfo = &(m_cslDeviceTable[typeIndex]);

        *pDeviceIndicesLengthRequired = pDeviceInfo->deviceIndexCount;

        for (UINT i = 0; (i < pDeviceInfo->deviceIndexCount) && (i < deviceIndicesLength); i++)
        {
            pDeviceIndices[i] = pDeviceInfo->deviceIndex[i];
        }
        result = CamxResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HwEnvironment::GetStaticMetadataKeysInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HwEnvironment::GetStaticMetadataKeysInfo(
    StaticMetadataKeysInfo* pKeysInfo,
    CameraMetadataTag       tag
    ) const
{
    CAMX_ASSERT(NULL != m_staticEntryMethods.GetStaticMetadataKeysInfo);

    return m_staticEntryMethods.GetStaticMetadataKeysInfo(pKeysInfo, tag);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HwEnvironment::SearchExternalComponent
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HwEnvironment::SearchExternalComponent(
    ExternalComponentInfo* pNodeComponentInfo,
    UINT                       nodeExternalComponentCount
    ) const
{
    UINT32                       index                  = 0;
    CamxResult                   result                 = CamxResultEFailed;
    const ExternalComponentInfo* pExternalComponentInfo = &m_externalComponent[0];

    for (UINT i = 0; i < nodeExternalComponentCount ; i++)
    {
        index = 0;
        while (pExternalComponentInfo[index].inUse != 0)
        {
            if (OsUtils::StrStr(pExternalComponentInfo[index].pComponentName, pNodeComponentInfo[i].pComponentName) != NULL)
            {
                if (pExternalComponentInfo[index].nodeAlgoType == ExternalComponentNodeAlgo::COMPONENTALGORITHM)
                {
                    pNodeComponentInfo[i].nodeAlgoType = ExternalComponentNodeAlgo::COMPONENTALGORITHM;
                    if (pExternalComponentInfo[index].statsAlgo == ExternalComponentStatsAlgo::ALGOAEC)
                    {
                        pNodeComponentInfo[i].AECAlgoCallbacks = pExternalComponentInfo[index].AECAlgoCallbacks;
                        pNodeComponentInfo[i].statsAlgo        = ExternalComponentStatsAlgo::ALGOAEC;
                    }
                    else if (pExternalComponentInfo[index].statsAlgo == ExternalComponentStatsAlgo::ALGOAF)
                    {
                        pNodeComponentInfo[i].AFAlgoCallbacks = pExternalComponentInfo[index].AFAlgoCallbacks;
                        pNodeComponentInfo[i].statsAlgo       = ExternalComponentStatsAlgo::ALGOAF;
                    }
                    else if (pExternalComponentInfo[index].statsAlgo == ExternalComponentStatsAlgo::ALGOAWB)
                    {
                        pNodeComponentInfo[i].AWBAlgoCallbacks = pExternalComponentInfo[index].AWBAlgoCallbacks;
                        pNodeComponentInfo[i].statsAlgo        = ExternalComponentStatsAlgo::ALGOAWB;
                    }
                    else if (pExternalComponentInfo[index].statsAlgo == ExternalComponentStatsAlgo::ALGOAFD)
                    {
                        pNodeComponentInfo[i].AFDAlgoCallbacks = pExternalComponentInfo[index].AFDAlgoCallbacks;
                        pNodeComponentInfo[i].statsAlgo        = ExternalComponentStatsAlgo::ALGOAFD;
                    }
                    else if (pExternalComponentInfo[index].statsAlgo == ExternalComponentStatsAlgo::ALGOASD)
                    {
                        pNodeComponentInfo[i].ASDAlgoCallbacks = pExternalComponentInfo[index].ASDAlgoCallbacks;
                        pNodeComponentInfo[i].statsAlgo        = ExternalComponentStatsAlgo::ALGOASD;
                    }
                    else if (pExternalComponentInfo[index].statsAlgo == ExternalComponentStatsAlgo::ALGOTRACK)
                    {
                        pNodeComponentInfo[i].trackerAlgoCallbacks = pExternalComponentInfo[index].trackerAlgoCallbacks;
                        pNodeComponentInfo[i].statsAlgo = ExternalComponentStatsAlgo::ALGOTRACK;
                    }
                }
                else
                {
                    pNodeComponentInfo[i].nodeAlgoType  = pExternalComponentInfo[index].nodeAlgoType;
                    pNodeComponentInfo[i].nodeCallbacks = pExternalComponentInfo[index].nodeCallbacks;
                }
                result = CamxResultSuccess;
                break;
            }
            index = index + 1;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::ReadEEPROMData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HwEnvironment::ReadEEPROMData(
    UINT32      cameraID,
    CSLHandle   hCSL)
{
    return m_sensorInfoTable[cameraID].pData->CreateAndReadEEPROMData(&m_sensorInfoTable[cameraID],
                                                                      &m_cslDeviceTable[CSLDeviceTypeEEPROM],
                                                                      hCSL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::ClosestMaxFps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT HwEnvironment::ClosestMaxFps(
    INT      fps,
    INT      sensorIndex)
{
    INT minBoundFps = static_cast<INT32>(GetSettingsManager()->GetStaticSettings()->minFrameRateBound);
    INT maxFps      = minBoundFps;

    // Find the closest fps among supported fps range which is less than calculated fps
    // calculated fps for particular resolution by keeping each node's clock in mind.
    // As CTS expects 8MP to support atleast 10 fps, hence keeping default minfpsbound as 10.

    for (UINT i = 0; i < m_caps[sensorIndex].numAETargetFPSRanges; i++)
    {
        if (fps == m_caps[sensorIndex].AETargetFPSRanges[i].max)
        {
            maxFps = m_caps[sensorIndex].AETargetFPSRanges[i].max;
            break;
        }
        else if (fps == m_caps[sensorIndex].AETargetFPSRanges[i].min)
        {
            maxFps = m_caps[sensorIndex].AETargetFPSRanges[i].min;
            break;
        }
        else
        {
            if (m_caps[sensorIndex].AETargetFPSRanges[i].max < fps &&
                m_caps[sensorIndex].AETargetFPSRanges[i].max >= minBoundFps)
            {
                if (abs(maxFps - fps) > abs(fps - m_caps[sensorIndex].AETargetFPSRanges[i].max))
                {
                    maxFps = m_caps[sensorIndex].AETargetFPSRanges[i].max;
                }
            }
            if (m_caps[sensorIndex].AETargetFPSRanges[i].min < fps &&
                m_caps[sensorIndex].AETargetFPSRanges[i].min >= minBoundFps)
            {
                if (abs(maxFps - fps) > abs(fps - m_caps[sensorIndex].AETargetFPSRanges[i].min))
                {
                    maxFps = m_caps[sensorIndex].AETargetFPSRanges[i].min;
                }
            }
        }
    }
    return maxFps;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::DumpEEPROMData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HwEnvironment::DumpEEPROMData(
    UINT32      cameraId)
{

    m_sensorInfoTable[cameraId].pData->DumpEEPROMData(&m_sensorInfoTable[cameraId]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HwEnvironment::CalculateRawSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 HwEnvironment::CalculateRawSize(
    UINT32 bitWidth,
    UINT32 width,
    UINT32 height)
{
    UINT32 stride  = width;
    UINT32 size    = width * height;

    // Calculate MIPI raw size as per bitwidth
    if (8 == bitWidth)
    {
        // 1 byte per 1 pixel
        stride = width;
        size   = CAMX_ALIGN_TO((stride * height), 16);
    }
    else if (12 == bitWidth)
    {
        // 3 bytes per 2 pixels
        stride = Utils::AlignGeneric32((width * 3 / 2), 16);
        size   = CAMX_ALIGN_TO((stride * height), 16);
    }
    else if (14 == bitWidth)
    {
        // 7 bytes per 4 pixels
        stride = Utils::AlignGeneric32((width * 7 / 4), 16);
        size   = stride * height * 7 / 4;
    }
    else
    {
         // Default to 10bpp
        // 5 bytes per 4 pixels
        stride = Utils::AlignGeneric32((width * 5 / 4), 16);
        size   = CAMX_ALIGN_TO((stride * height), 16);
    }
    return size;
}


CAMX_NAMESPACE_END
