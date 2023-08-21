////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcaecstatsprocessor.cpp
/// @brief The class that implements IStatsProcessor for AEC.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcaecstatsprocessor.h"
#include "camxcsljumptable.h"
#include "camxhal3metadatautil.h"
#include "camxhal3module.h"
#include "camxmem.h"
#include "camxtrace.h"
#include "camximagesensormoduledata.h"
#include "camximagesensordata.h"
#include "camxpropertyblob.h"
#include "camxtitan17xdefs.h"

CAMX_NAMESPACE_BEGIN

/// @todo (CAMX-1259): Adding IFE HW bit width in metadata. Query IFE HW bit width from metadata instead of hardcoding.
static const UINT8  BGStatsMaximumBitWidth      = 14;      ///< Bandwidth consumed by BG stats
static const UINT8  CompensationStepNumerator   = 1;       ///< EV compensation Step Numerator
class TuningDataManager;

// @brief Number of Partial Meta Data Tag from the Publish Tag list
static const UINT NumberOfPartialMetadataTags = 10;

// @brief list of tags published
static const UINT32 AECOutputMetadataTags[] =
{
    ControlAEExposureCompensation,
    ControlAELock,
    ControlAEMode,
    ControlAERegions,
    ControlAEState,
    ControlAEPrecaptureTrigger,
    ControlMode,
    ControlAECompensationStep,
    ControlAEAntibandingMode,
    ControlAETargetFpsRange,
    StatisticsSceneFlicker,
    PropertyIDUsecaseAECFrameControl,
    PropertyIDUsecaseAECStatsControl,
    PropertyIDCrossAECStats,
    PropertyIDAECInternal,
    PropertyIDAECFrameControl,
    PropertyIDAECStatsControl,
    PropertyIDAECPeerInfo,
    PropertyIDAECFrameInfo,
};

// @brief list of vendor tags published
static const struct NodeVendorTag AECOutputVendorTags[] =
{
    { "org.quic.camera2.statsconfigs"           , "AECFrameControl"         },
    { "com.qti.stats_control"                   , "is_flash_snapshot"       },
    { "com.qti.stats_control"                   , "is_lls_needed"           },
    { "com.qti.chi.statsaec"                    , "AecLux"                  },
    { "org.quic.camera2.sensor_register_control", "sensor_register_control" },
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::Create(
    IStatsProcessor** ppAECStatsProcessor)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);

    CamxResult          result              = CamxResultSuccess;
    CAECStatsProcessor* pAECStatsProcessor  = NULL;

    if (NULL == ppAECStatsProcessor)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "Invalid arguments");
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        pAECStatsProcessor = CAMX_NEW CAECStatsProcessor();
        if (NULL == pAECStatsProcessor)
        {
            CAMX_LOG_ERROR(CamxLogGroupAEC, "Create failed - out of memory");
            result = CamxResultENoMemory;
        }
        else
        {
            *ppAECStatsProcessor = pAECStatsProcessor;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::CAECStatsProcessor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAECStatsProcessor::CAECStatsProcessor()
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);
    m_isFixedFocus = FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::Initialize(
    const StatsInitializeData* pInitializeData)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);
    CamxResult result = CamxResultSuccess;
    BOOL isEarlyPCREnabled = FALSE;

    if (NULL == pInitializeData)
    {
        return CamxResultEInvalidPointer;
    }

    m_pStaticSettings = pInitializeData->pHwContext->GetStaticSettings();
    if (NULL == m_pStaticSettings)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "m_pStaticSettings: NULL, Static settings read error");
        return CamxResultEFailed;
    }

    m_pNode                 = pInitializeData->pNode;
    m_pDebugDataPool        = pInitializeData->pDebugDataPool;
    m_lastSettledState      = AECSettleUnknown;
    m_cameraId              = pInitializeData->pPipeline->GetCameraId();
    m_pStatsInitializeData  = pInitializeData;

    m_pNode->IsEarlyPCREnabled(&isEarlyPCREnabled);

    m_algoInput.statsSession.Initialize(pInitializeData);

    // If AEC stats processing is disabled we publish some hardcoded values to the metadata pool
    if (NULL != pInitializeData->initializecallback.pAECCallback)
    {
        m_pfnCreate = pInitializeData->initializecallback.pAECCallback->pfnCreate;
    }

    m_algoInput.cameraInfo = pInitializeData->cameraInfo;

    // Initialize CAECGyro
    if ((CamxResultSuccess == result) && (TRUE == m_pStaticSettings->enableNCSService))
    {
        result = m_AECGyro.Initialize();
        if (CamxResultEFailed == result)
        {
            CAMX_LOG_WARN(CamxLogGroupAEC, "Failed to Initialize Gyro");
        }

        //  Set result to CamxResultSuccess due to we don't want to stop here even if gyro failed at initialization.
        result = CamxResultSuccess;
    }

    // Create an instance of the core algorithm
    m_pAECEngine = CAECEngine::Create(m_pfnCreate, &m_algoInput.statsSession, &m_algoInput.cameraInfo);

    if (NULL == m_pAECEngine)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "Failed to create the AEC Engine - out of memory");
        result = CamxResultEUnableToLoad;
    }

    if (CamxResultSuccess == result)
    {
        // Start the driver
        AECCommandInputParam  inputParam  = { 0 };
        AECCommandOutputParam outputParam = { 0 };
        result = m_pAECEngine->HandleCommand(AECCommand::StartDriver, &inputParam, &outputParam);
    }

    if (CamxResultSuccess == result)
    {
        SetInitialTuning(pInitializeData);
    }

    if (CamxResultSuccess == result)
    {
        m_pStatsAECPropertyReadWrite = CAMX_NEW StatsPropertyReadAndWrite(pInitializeData);
    }
    if (NULL == m_pStatsAECPropertyReadWrite)
    {
        result = CamxResultENoMemory;
    }

    if (CamxResultSuccess == result)
    {
        InitializeReadProperties();
        InitializeWriteProperties();
        InitializeVendorTagPublishList();
    }

    if (CamxResultSuccess == result)
    {
        AECCommandInputParam  inputParam  = { 0 };
        AECCommandOutputParam outputParam = { 0 };

        inputParam.systemLatency = static_cast<UINT8>(m_pNode->GetMaximumPipelineDelay());
        result                   = m_pAECEngine->HandleCommand(AECCommand::SetPipelineDelay, &inputParam, &outputParam);
    }

    if (CamxResultSuccess == result)
    {
        HwCameraInfo cameraInfo;

        // Retrieve the static capabilities for this camera
        result = HwEnvironment::GetInstance()->GetCameraInfo(pInitializeData->pPipeline->GetCameraId(), &cameraInfo);

        // AEC has dependency on AF for depth information (except when sensor is fixed focus),
        // AF node does not publish depth info when disableAFStatsProcessing is True,
        // hence m_isFixedFocus is set to TRUE when AF processing is disabled
        // so that AEC does not add dependencies on AF
        if ((TRUE == cameraInfo.pSensorCaps->isFixedFocus) || (TRUE == m_pStaticSettings->disableAFStatsProcessing))
        {
            m_isFixedFocus = TRUE;
        }

        m_isZZHDRSupported = cameraInfo.pSensorCaps->isZZHDRSupported;
        m_isSHDRSupported  = cameraInfo.pSensorCaps->isSHDRSupported;

        if (TRUE == cameraInfo.pSensorCaps->OTPData.dualCameraCalibration.isAvailable)
        {
            AECCommandInputParam  inputParam  = { 0 };
            AECCommandOutputParam outputParam = { 0 };

            const UINT                             oldFormatVersion       = 0;
            const DualCameraSystemCalibrationdata* pSystemCalibrationData =
                &cameraInfo.pSensorCaps->OTPData.dualCameraCalibration.systemCalibrationData;
            if (NULL != pSystemCalibrationData)
            {
                UINT version = pSystemCalibrationData->absoluteMethodAECSyncData.version;
                inputParam.dcCalibrationInfo.version = version;


                if (version > oldFormatVersion)
                {
                    ///> New OTP format
                    inputParam.dcCalibrationInfo.absolute.averageLuma    = pSystemCalibrationData->
                        absoluteMethodAECSyncData.averageLuma;
                    inputParam.dcCalibrationInfo.absolute.gain           = pSystemCalibrationData->
                        absoluteMethodAECSyncData.gain;
                    inputParam.dcCalibrationInfo.absolute.exposureTimeUs = pSystemCalibrationData->
                        absoluteMethodAECSyncData.exposureTimeUs;
                    inputParam.dcCalibrationInfo.absolute.CCT            = pSystemCalibrationData->
                        referenceMasterColorTemperature;
                    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Multicamera:CalibData: Ver:%d AvgLuma:%d Gain:%f ET:%d cct:%d",
                        inputParam.dcCalibrationInfo.version,
                        inputParam.dcCalibrationInfo.absolute.averageLuma,
                        inputParam.dcCalibrationInfo.absolute.gain,
                        inputParam.dcCalibrationInfo.absolute.exposureTimeUs,
                        inputParam.dcCalibrationInfo.absolute.CCT);
                }
                else
                {

                    ///> Old OTP format
                    inputParam.dcCalibrationInfo.relative.brightnessRatio = pSystemCalibrationData->brightnessRatio;
                    inputParam.dcCalibrationInfo.relative.slaveGain       = pSystemCalibrationData->referenceSlaveGain;
                    inputParam.dcCalibrationInfo.relative.slaveTimeUnit   = pSystemCalibrationData->referenceSlaveExpTime;
                    inputParam.dcCalibrationInfo.relative.masterGain      = pSystemCalibrationData->referenceMasterGain;
                    inputParam.dcCalibrationInfo.relative.masterTimeUnit  = pSystemCalibrationData->referenceMasterExpTime;

                    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Multicamera:CalibData: Ver:%d BrtRatio:%f slave[G:%f ET:%f] "
                        "master[G:%f ET:%f] cct:%d",
                        inputParam.dcCalibrationInfo.version,
                        inputParam.dcCalibrationInfo.relative.brightnessRatio,
                        inputParam.dcCalibrationInfo.relative.slaveGain,
                        inputParam.dcCalibrationInfo.relative.slaveTimeUnit,
                        inputParam.dcCalibrationInfo.relative.masterGain,
                        inputParam.dcCalibrationInfo.relative.masterTimeUnit,
                        inputParam.dcCalibrationInfo.relative.CCT);
                }
                m_dcCalibrationInfo = inputParam.dcCalibrationInfo;

                if (FALSE == isEarlyPCREnabled)
                {
                    result = m_pAECEngine->HandleCommand(AECCommand::SetDCCalibrationData, &inputParam, &outputParam);
                }
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Multicamera :systemCalibrationData is not available");
            }

        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Multicamera :CalibData is not available");
        }

        if (CamxResultSuccess == result)
        {
            AECCommandInputParam    inputParam  = { 0 };
            AECCommandOutputParam   outputParam = { 0 };
            StatsRectangle          cropWindow  = { 0 };

            GetCropWindow(&cropWindow);
            inputParam.pCropWindow = &cropWindow;

            result = m_pAECEngine->HandleCommand(AECCommand::ProcessCropWindow, &inputParam, &outputParam);
        }

        if (CamxResultSuccess == result)
        {
            AECCommandInputParam    inputParam   = { 0 };
            AECCommandOutputParam   outputParam  = { 0 };

            GetHardwareInfo(
                        &m_hardwareInfo,
                        pInitializeData->pHwContext,
                        pInitializeData->pPipeline->GetCameraId());
            m_hardwareInfo.isDual = pInitializeData->pPipeline->IsMultiCamera();
            m_hardwareInfo.isFixedFocus = m_isFixedFocus;
            inputParam.pHardwareInfo = &m_hardwareInfo;
            result = m_pAECEngine->HandleCommand(AECCommand::ProcessHardwareInfo, &inputParam, &outputParam);
        }

        if (CamxResultSuccess == result)
        {
            DetermineStabilizationMargins();
        }

        if (CamxResultSuccess == result)
        {
            static const UINT ReadParameter[] =
            {
                ControlAETargetFpsRange | UsecaseMetadataSectionMask,
            };
            static const UINT GetDataLength         = CAMX_ARRAY_SIZE(ReadParameter);
            VOID*             pData[GetDataLength]  = { 0 };
            UINT64            offset[GetDataLength] = { 0 };
            m_pNode->GetDataList(ReadParameter, pData, offset, GetDataLength);

            if (NULL == pData[0])
            {
                // No need to return error.Proper value will be read at EPR.
                CAMX_LOG_WARN(CamxLogGroupAEC, "UseCasePool FPS Range: pData[0]:%p not published", pData[0]);
            }
            else
            {
                Utils::Memcpy(&m_HALParam.FPSRange, pData[0], sizeof(m_HALParam.FPSRange));
                AECCommandInputParam    inputParam     = { 0 };
                AECCommandOutputParam   outputParam    = { 0 };
                CAMX_LOG_VERBOSE(CamxLogGroupAEC, "FPS Range: min: %d max: %d",
                    m_HALParam.FPSRange.min, m_HALParam.FPSRange.max);

                inputParam.pHALParam                   = &m_HALParam;
                result  = m_pAECEngine->HandleCommand(AECCommand::SetFPSRange, &inputParam, &outputParam);
            }
        }

        if (CamxResultSuccess == result)
        {
            result = VendorTagListAllocation();
        }

        if (CamxResultSuccess == result)
        {
            if (FALSE == isEarlyPCREnabled)
            {
                result = PublishPreRequestOutput(pInitializeData->pPipeline->GetPerFramePool(PoolType::PerUsecase));
            }
        }
    }
    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "zzHDR:%d SHDR:%d FF:%d",
        m_isZZHDRSupported, m_isSHDRSupported, m_isFixedFocus);

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::InitializeReadProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECStatsProcessor::InitializeReadProperties()
{
    m_AECReadProperties[AECReadTypePropertyIDISPHDRBEConfig]                =
    {
        PropertyIDISPHDRBEConfig,
        m_pNode->GetMaximumPipelineDelay(),
        NULL
    };
    m_AECReadProperties[AECReadTypePropertyIDParsedHDRBEStatsOutput]        =
    {
        PropertyIDParsedHDRBEStatsOutput,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypePropertyIDParsedHDRBHistStatsOutput]     =
    {
        PropertyIDParsedHDRBHistStatsOutput,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypePropertyIDParsedBHistStatsOutput]        =
    {
        PropertyIDParsedBHistStatsOutput,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypeFlashMode]                               =
    {
        FlashMode,
        1,
        NULL
    };
    m_AECReadProperties[AECReadTypePropertyIDDebugDataAll]                  =
    {
        PropertyIDDebugDataAll,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypeInputControlAEExposureCompensation]      =
    {
        InputControlAEExposureCompensation,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypeInputControlAELock]                      =
    {
        InputControlAELock,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypeInputControlAEMode]                      =
    {
        InputControlAEMode,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypeInputControlAEPrecaptureTrigger]         =
    {
        InputControlAEPrecaptureTrigger,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypeInputControlAFTrigger]                   =
    {
        InputControlAFTrigger,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypeInputControlCaptureIntent]               =
    {
        InputControlCaptureIntent,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypeInputControlMode]                        =
    {
        InputControlMode,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypeInputControlSceneMode]                   =
    {
        InputControlSceneMode,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypeInputFlashMode]                          =
    {
        InputFlashMode,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypeInputSensorExposureTime]                 =
    {
        InputSensorExposureTime,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypeInputSensorSensitivity]                  =
    {
        InputSensorSensitivity,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypeInputControlAEAntibandingMode]           =
    {
        InputControlAEAntibandingMode,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypeInputControlAERegions]                   =
    {
        InputControlAERegions,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypeFlashState]                              =
    {
        FlashState,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypeInputControlAETargetFpsRange]            =
    {
        InputControlAETargetFpsRange,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypeInputControlZslEnable]                   =
    {
        InputControlZslEnable,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypeSensorFrameDuration]                     =
    {
        SensorFrameDuration,
        m_pNode->GetMaximumPipelineDelay(),
        NULL
    };
    m_AECReadProperties[AECReadTypeInputControlPostRawSensitivityBoost]     =
    {
        InputControlPostRawSensitivityBoost,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypeInputScalerCropRegion]                   =
    {
        InputScalerCropRegion,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypePropertyIDAFDFrameInfo]                  =
    {
        PropertyIDAFDFrameInfo,
        1,
        NULL
    };
    m_AECReadProperties[AECReadTypePropertyIDAFFrameInfo]                   =
    {
        PropertyIDAFFrameInfo,
        1,
        NULL
    };
    m_AECReadProperties[AECReadTypePropertyIDAWBInternal]                   =
    {
        PropertyIDAWBInternal,
        1,
        NULL
    };
    m_AECReadProperties[AECReadTypePropertyIDAWBFrameInfo]                  =
    {
        PropertyIDAWBFrameInfo,
        1,
        NULL
    };
    m_AECReadProperties[AECReadTypeInputControlAWBMode]                     =
    {
        InputControlAWBMode,
        1,
        NULL
    };
    m_AECReadProperties[AECReadTypePropertyIDRERCompleted]                  =
    {
        PropertyIDRERCompleted,
        1,
        NULL
    };
    m_AECReadProperties[AECReadTypePropertyIDLensInfo]                      =
    {
        PropertyIDUsecaseLensInfo,
        0,
        NULL
    };
    m_AECReadProperties[AECReadTypePropertyIDParsedIHistStatsOutput]        =
    {
        PropertyIDParsedIHistStatsOutput,
        0,
        NULL
    };

    m_pStatsAECPropertyReadWrite->SetReadProperties(m_AECReadProperties);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::InitializeWriteProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECStatsProcessor::InitializeWriteProperties()
{
    m_AECWriteProperties[0] =
    {
        PropertyIDAECFrameControl,
        sizeof(AECFrameControl),
        &m_frameControl
    };
    m_AECWriteProperties[1] =
    {
        PropertyIDAECStatsControl,
        sizeof(AECStatsControl),
        &m_statsControl,
    };
    m_AECWriteProperties[2] =
    {
        PropertyIDAECPeerInfo,
        sizeof(VOID*),
        &m_pPeerInfo,
    };
    m_AECWriteProperties[3] =
    {
        PropertyIDAECFrameInfo,
        sizeof(AECFrameInformation),
        &m_frameInfo,
    };
    m_AECWriteProperties[4] =
    {
        ControlAEExposureCompensation,
        sizeof(m_aeHALData.aeCompensation) / sizeof(INT32),
        &m_aeHALData.aeCompensation,
    };
    m_AECWriteProperties[5] =
    {
        ControlAELock,
        sizeof(m_aeHALData.aeLockFlag) / sizeof(BOOL),
        &m_aeHALData.aeLockFlag,
    };
    m_AECWriteProperties[6] =
    {
        ControlAEMode,
        sizeof(ControlAEModeValues) / sizeof(INT32),
        &m_aeHALData.mode,
    };
    m_AECWriteProperties[7] =
    {
        ControlAERegions,
        AERegionSize,
        &m_aeHALData.aeRegion,
    };
    m_AECWriteProperties[8] =
    {
        ControlAEState,
        sizeof(ControlAEStateValues) / sizeof(INT32),
        &m_aeHALData.aeState,
    };
    m_AECWriteProperties[9] =
    {
        ControlAEPrecaptureTrigger,
        sizeof(ControlAEPrecaptureTriggerValues) / sizeof(INT32),
        &m_aeHALData.triggerValue,
    };
    m_AECWriteProperties[10] =
    {
        ControlMode,
        sizeof(ControlModeValues) / sizeof(INT32),
        &m_aeHALData.controlMode,
    };
    m_AECWriteProperties[11] =
    {
        ControlAECompensationStep,
        1,
        &m_aeHALData.exposureCompensationStep,
    };
    m_AECWriteProperties[12] =
    {
        StatisticsSceneFlicker,
        sizeof(m_aeHALData.flickerMode) / sizeof(UINT8),
        &m_aeHALData.flickerMode,
    };
    m_AECWriteProperties[13] =
    {
        ControlAEAntibandingMode,
        sizeof(m_aeHALData.aeAntiBandingMode) / sizeof(UINT8),
        &m_aeHALData.aeAntiBandingMode,
    };
    m_AECWriteProperties[14] =
    {
        ControlAETargetFpsRange,
        sizeof(m_engineStatsOutput.currentFPSRange) / sizeof(INT32),
        &m_engineStatsOutput.currentFPSRange,
    };
    m_AECWriteProperties[15] =
    {
        PropertyIDAECInternal,
        sizeof(AECOutputInternal),
        &m_outputInternal,
    };
    m_AECWriteProperties[16] =
    {
        PropertyIDCrossAECStats,
        sizeof(UINT64),
        &m_currProcessingRequestId,
    };

    m_pStatsAECPropertyReadWrite->SetWriteProperties(m_AECWriteProperties);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::InitializeVendorTagPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECStatsProcessor::InitializeVendorTagPublishList()
{
    m_aecVendorTagsPublish[0] =
    {
        "org.quic.camera2.statsconfigs",
        "AECSensitivity",
        &m_aecSensitivity,
        sizeof(m_aecSensitivity) / sizeof(FLOAT)
    };
    m_aecVendorTagsPublish[1] =
    {
        "org.quic.camera2.statsconfigs",
        "AECExposureTime",
        &m_aecExposureTime,
        sizeof(m_aecExposureTime) / sizeof(UINT64)
    };
    m_aecVendorTagsPublish[2] =
    {
        "org.quic.camera2.statsconfigs",
        "AECLinearGain",
        &m_aecLinearGain,
        sizeof(m_aecLinearGain) / sizeof(FLOAT)
    };
    m_aecVendorTagsPublish[3] =
    {
        "org.quic.camera2.statsconfigs",
        "AECLuxIndex",
        &m_aecLuxIndex,
        sizeof(FLOAT)
    };
    m_aecVendorTagsPublish[4] =
    {
        "org.quic.camera2.statsconfigs",
        "AECFrameControl",
        &m_frameControl,
        sizeof(AECFrameControl)
    };
    m_aecVendorTagsPublish[5] =
    {
        "org.quic.camera2.sensor_register_control" ,
        "sensor_register_control",
        &m_sensorControl,
        sizeof(SensorRegisterControl) / sizeof(INT32)
    };
    m_aecVendorTagsPublish[6] =
    {
        "org.quic.camera.debugdata",
        "DebugDataAll",
        &m_debugData,
        sizeof(DebugData)
    };
    m_aecVendorTagsPublish[7] =
    {
        "org.codeaurora.qcamera3.bayer_exposure",
        "roi_be_x",
        &m_aecProcessedROI.x,
        sizeof(m_aecProcessedROI.x) / sizeof(FLOAT)
    };
    m_aecVendorTagsPublish[8] =
    {
        "org.codeaurora.qcamera3.bayer_exposure",
        "roi_be_y",
        &m_aecProcessedROI.y,
        sizeof(m_aecProcessedROI.y) / sizeof(FLOAT)
    };
    m_aecVendorTagsPublish[9] =
    {
        "org.codeaurora.qcamera3.bayer_exposure",
        "roi_be_width",
        &m_aecProcessedROI.dx,
        sizeof(m_aecProcessedROI.dx) / sizeof(FLOAT)
    };
    m_aecVendorTagsPublish[10] =
    {
        "org.codeaurora.qcamera3.bayer_exposure",
        "roi_be_height",
        &m_aecProcessedROI.dy,
        sizeof(m_aecProcessedROI.dy) / sizeof(FLOAT)
    };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::SetInitialTuning
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECStatsProcessor::SetInitialTuning(
    const StatsInitializeData* pInitializeData)
{
    CamxResult result = CamxResultSuccess;

    // Set the chromatix to driver
    AECCommandInputParam  inputParam  = { 0 };
    AECCommandOutputParam outputParam = { 0 };
    BOOL isEarlyPCREnabled            = FALSE;
    m_pNode->IsEarlyPCREnabled(&isEarlyPCREnabled);

    m_tuningData.pTuningSetManager = static_cast<VOID*>(pInitializeData->pTuningDataManager->GetChromatix());

    if (FALSE == isEarlyPCREnabled && NULL != m_tuningData.pTuningSetManager)
    {
        ChiTuningMode tuningSelectors[MaxTuningMode];
        Utils::Memset(&tuningSelectors[0], 0, sizeof(tuningSelectors));

        for (UINT modeIndex = 0; modeIndex < MaxTuningMode; modeIndex++)
        {
            tuningSelectors[modeIndex].mode = static_cast<ChiModeType>(modeIndex);
        }

        // Default
        m_tuningData.pTuningModeSelectors = &tuningSelectors[0];
        m_tuningData.numSelectors         = MaxTuningMode;
        inputParam.pTuningData            = &m_tuningData;
        result                            = m_pAECEngine->HandleCommand(AECCommand::SetChromatix, &inputParam, &outputParam);
        m_tuningData.pTuningModeSelectors = NULL; /// Removing Local address

        // when early PCR disabled,load dual led calibration data once here.
        if ((CamxResultSuccess == result) && (DualLEDCalibrationDisabled == m_pStaticSettings->dualLEDCalibrationMode))
        {
            result = m_pAECEngine->HandleCommand(AECCommand::LoadLEDCalibrationData, &inputParam, &m_outputParam);
        }

        // when early PCR disabled, load Dynamic inline LED Calibration Data. should only load once from bin file at camera open
        if ((CamxResultSuccess == result) &&
            (TRUE == m_pStaticSettings->dynamicInlineCalibration))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "loading led inline calibration data");
            result = m_pAECEngine->HandleCommand(AECCommand::LoadLEDInlineCalibrationData, &inputParam, &m_outputParam);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupAEC, "unable to load bin file. result: %d", result);
            }
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupAEC, "pTuningSetManager may be NULL");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::ExecuteProcessRequest(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo)
{

    if (NULL == pStatsProcessRequestDataInfo)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "pStatsProcessRequestDataInfo pointer is NULL");
        return CamxResultEInvalidPointer;
    }

    CAMX_ENTRYEXIT_SCOPE_ID(CamxLogGroupAEC, SCOPEEventCAECStatsProcessorExecuteProcessRequest,
        pStatsProcessRequestDataInfo->requestId);

    CamxResult                  result          = CamxResultSuccess;
    AECCommandInputParam        inputParam      = { 0 };
    UINT64 requestIdOffsetFromLastFlush         =
        m_pNode->GetRequestIdOffsetFromLastFlush(pStatsProcessRequestDataInfo->requestId);

    if (m_currProcessingRequestId != pStatsProcessRequestDataInfo->requestId-1)
    {
        CAMX_LOG_WARN(CamxLogGroupAEC,
                       "AEC: Execute Process Request: Id=%llu LastProcessed Id = %llu requestIdOffsetFromLastFlush %llu",
                       pStatsProcessRequestDataInfo->requestId, m_currProcessingRequestId, requestIdOffsetFromLastFlush);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAEC,
                        "AEC: Execute Process Request: Id=%llu LastProcessed Id = %llu  requestIdOffsetFromLastFlush %llu",
                         pStatsProcessRequestDataInfo->requestId, m_currProcessingRequestId, requestIdOffsetFromLastFlush);
    }

    m_currProcessingRequestId = pStatsProcessRequestDataInfo->requestId;
    m_skipProcessing = pStatsProcessRequestDataInfo->skipProcessing;

    m_pStatsAECPropertyReadWrite->GetReadProperties(AECReadTypePropertyIDCount, requestIdOffsetFromLastFlush);

    if (CanSkipAEProcessing(pStatsProcessRequestDataInfo))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAEC,
                         "Skipping AEC Algo processing for RequestId=%llu",
                         pStatsProcessRequestDataInfo->requestId);
    }
    else
    {
        if (NULL == m_pAECEngine)
        {
            CAMX_LOG_ERROR(CamxLogGroupAEC, "m_pAECEngine pointer is NULL");
            result = CamxResultEInvalidPointer;
        }

        // Read all input parameters which are set to the algorithm
        if (CamxResultSuccess == result)
        {
            // Set the HAL parameters
            result = ReadHALAECParam(&m_HALParam, pStatsProcessRequestDataInfo);
        }

        if (CamxResultSuccess == result)
        {
            result = SetCameraInformation(pStatsProcessRequestDataInfo, &m_algoInput);
            if (CamxResultSuccess == result)
            {
                result = SetAlgoChromatix(pStatsProcessRequestDataInfo->pTuningModeData);
            }
        }

        // Set the Tuning data to the algorithm
        if (CamxResultSuccess == result)
        {
            BOOL isEarlyPCREnabled = FALSE;
            m_pNode->IsEarlyPCREnabled(&isEarlyPCREnabled);

            if ((FirstValidRequestId == requestIdOffsetFromLastFlush) &&
                (TRUE == isEarlyPCREnabled))
            {
                CAMX_LOG_INFO(CamxLogGroupAEC, "Early PCR is enabled.");
                inputParam.dcCalibrationInfo = m_dcCalibrationInfo;
                result                       =
                    m_pAECEngine->HandleCommand(AECCommand::SetDCCalibrationData, &inputParam, &m_outputParam);

                if (CamxResultSuccess == result)
                {
                    result = PublishPreRequestOutput(m_pStatsInitializeData->pPipeline->GetPerFramePool(PoolType::PerUsecase));
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupAEC, "SetDCCalibrationData returned error with:%d", result);
                }

                // when early PCR enabled, load Dynamic inline LED Calibration Data.
                // Should only load once from bin file at first valid request
                if ((CamxResultSuccess == result) &&
                    (TRUE == m_pStaticSettings->dynamicInlineCalibration))
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "loading led inline calibration data");
                    result = m_pAECEngine->HandleCommand(AECCommand::LoadLEDInlineCalibrationData, &inputParam, &m_outputParam);

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_WARN(CamxLogGroupAEC, "unable to load bin file. result: %d", result);
                    }
                }

                // when early pcr is enabled , Load LED calibration data
                if ((CamxResultSuccess == result) && (DualLEDCalibrationDisabled == m_pStaticSettings->dualLEDCalibrationMode))
                {
                    result = m_pAECEngine->HandleCommand(AECCommand::LoadLEDCalibrationData, &inputParam, &m_outputParam);
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            // Set the AE Sync Lock parameters
            result = ConfigureAESyncLockParam(&m_HALParam,
                pStatsProcessRequestDataInfo->requestId,
                pStatsProcessRequestDataInfo->cameraInfo.algoRole);
        }

        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "ReqId:%llu AESyncLockStatus:%d Role:%d",
            pStatsProcessRequestDataInfo->requestId,
            m_HALParam.AELock,
            pStatsProcessRequestDataInfo->cameraInfo.algoRole);

        // Get LED Calibration config
        if ((CamxResultSuccess == result) &&
            ((DualLEDCalibrationDisabled != m_pStaticSettings->dualLEDCalibrationMode) ||
            (TRUE == m_pStaticSettings->dynamicInlineCalibration)))
        {
            result = m_pAECEngine->HandleCommand(AECCommand::GetLEDCalibrationConfig, &inputParam, &m_outputParam);
        }

        // Get Gyro data
        if ((CamxResultSuccess == result) && (TRUE == m_pStaticSettings->enableNCSService))
        {
            result = GetGyroDataFromNCS(&inputParam);
            if (CamxResultEFailed == result)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Failed to populate Gyro data");
            }
            else if (CamxResultSuccess == result)
            {
                result = m_pAECEngine->HandleCommand(AECCommand::ProcessGYROStats, &inputParam, &m_outputParam);
                if (CamxResultEFailed == result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupAEC, "Failed to process Gyro Stats");
                }
            }

            //  Set result to CamxResultSuccess due to we don't want to stop here even if gyro failed at getting data.
            result = CamxResultSuccess;
        }

        // Set the input parameters to the algorithm
        if (CamxResultSuccess == result)
        {
            result = SetAlgoSetParams(&m_HALParam);
        }

        if (CamxResultSuccess == result)
        {
            result = GetAlgoInput(pStatsProcessRequestDataInfo, &m_algoInput);
        }

        if (CamxResultSuccess == result)
        {
            result = PrepareAlgorithmOutput(&m_engineStatsOutput, requestIdOffsetFromLastFlush);
        }

        // Process the input
        if (CamxResultSuccess == result)
        {
            m_outputParam.pProcessStatsOutput = &m_engineStatsOutput;
            inputParam.pAlgorithmInput      = &m_algoInput;

            result = m_pAECEngine->HandleCommand(AECCommand::ProcessStats, &inputParam, &m_outputParam);
        }

        if (CamxResultSuccess == result)
        {
            if (TRUE == pStatsProcessRequestDataInfo->reportConverged)
            {
                if (ControlAELockOn == inputParam.pHALParam->AELock)
                {
                    m_outputParam.pProcessStatsOutput->AEState = ControlAEStateLocked;
                }
                else
                {
                    m_outputParam.pProcessStatsOutput->AEState = ControlAEStateConverged;
                }
            }
        }

        // Update all of AEC Trace Events required
        if (CamxResultSuccess == result)
        {
            UpdateTraceEvents(m_outputParam.pProcessStatsOutput);
        }
    }

    if (CamxResultSuccess == result)
    {
        result = PublishMetadata(m_outputParam.pProcessStatsOutput, &m_HALParam);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::VendorTagListAllocation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::VendorTagListAllocation()
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);

    CamxResult              result              = CamxResultSuccess;
    StatsVendorTagInfoList* pVendorTagInfoList  = NULL;

    AECCommandInputParam  inputParam    = { 0 };
    AECCommandOutputParam outputParam   = { 0 };

    outputParam.pVendorTagInfoList = &m_vendorTagInfoOutputputList;
    inputParam.pAlgorithmInput = &m_algoInput;

    result = m_pAECEngine->HandleCommand(AECCommand::GetPubVendorTagFromAlgo, &inputParam, &outputParam);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Algorithm library does not support Vendor Tag");
        result = CamxResultSuccess;
    }
    else
    {
        pVendorTagInfoList = static_cast<StatsVendorTagInfoList*>(outputParam.pVendorTagInfoList);
        // allocate memory to hold each vendor tag's data
        result = AllocateMemoryVendorTag(pVendorTagInfoList);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::AddOEMDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::AddOEMDependencies(
    const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
    StatsDependency*                pStatsDependency)
{
    CamxResult              result              = CamxResultSuccess;
    StatsVendorTagInfoList* pVendorTagInfoList  = NULL;

    AECCommandInputParam  inputParam    = { 0 };
    AECCommandOutputParam outputParam   = { 0 };

    outputParam.pVendorTagInfoList = &m_vendorTagInputList;
    inputParam.pAlgorithmInput = &m_algoInput;

    result = m_pAECEngine->HandleCommand(AECCommand::GetVendorTagFromAlgo, &inputParam, &outputParam);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Algorithm library does not support Vendor Tag");
        result = CamxResultSuccess;
    }
    else
    {
        pVendorTagInfoList = static_cast<StatsVendorTagInfoList*>(outputParam.pVendorTagInfoList);
        if (NULL != pVendorTagInfoList)
        {
            UINT32  propertyCount = Utils::MinUINT32(pVendorTagInfoList->vendorTagCount,
                                                     (MaxStatsProperties - pStatsDependency->propertyCount));

            // We can have only MaxStatsProperties number of properties that Stats node can be dependent on
            // Adding vendor tag dependencies
            for (UINT32 i = 0; i < propertyCount; i++)
            {
                pStatsDependency->properties[pStatsDependency->propertyCount + i].property =
                    pVendorTagInfoList->vendorTag[i].vendorTagId;
                pStatsDependency->properties[pStatsDependency->propertyCount + i].offset =
                    pVendorTagInfoList->vendorTag[i].appliedDelay;

                CAMX_LOG_VERBOSE(CamxLogGroupAEC,
                    "Added Vendor Tag(%u) to the dependent list with offset(%u). requestID(%llu)",
                    pVendorTagInfoList->vendorTag[i].vendorTagId,
                    pVendorTagInfoList->vendorTag[i].appliedDelay,
                    pStatsProcessRequestDataInfo->requestId);
            }

            pStatsDependency->propertyCount += propertyCount;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::GetDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::GetDependencies(
    const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
    StatsDependency*                pStatsDependency)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);

    CamxResult result                   = CamxResultSuccess;
    UINT64 requestIdOffsetFromLastFlush =
        m_pNode->GetRequestIdOffsetFromLastFlush(pStatsProcessRequestDataInfo->requestId);

    // Ensure ordering
    if (FirstValidRequestId < requestIdOffsetFromLastFlush)
    {
        pStatsDependency->properties[pStatsDependency->propertyCount++] = { PropertyIDAECStatsControl, 1 };
        pStatsDependency->properties[pStatsDependency->propertyCount++] = { PropertyIDAWBFrameInfo, 1 };
    }

    if (TRUE == pStatsProcessRequestDataInfo->skipProcessing)
    {
        // We need to publish previous frame data to property pool when skipProcessing flag is set.
        // Add previous frame(offset = 1) AEC Frame/Stats Control properties as dependency.
        pStatsDependency->properties[pStatsDependency->propertyCount++] = { PropertyIDAECFrameControl, 1};
    }
    else
    {
        result = AddOEMDependencies(pStatsProcessRequestDataInfo, pStatsDependency);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::~CAECStatsProcessor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAECStatsProcessor::~CAECStatsProcessor()
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);

    if (NULL != m_pAECEngine)
    {
        m_pAECEngine->Destroy();
        m_pAECEngine = NULL;
    }

    if (NULL != m_pStatsAECPropertyReadWrite)
    {
        CAMX_DELETE m_pStatsAECPropertyReadWrite;
        m_pStatsAECPropertyReadWrite = NULL;
    }

    if (NULL != m_pRDIStatsDataBuffer)
    {
        for (UINT32 channelCount = 0; channelCount < StatisticsBayerChannelsCount; channelCount++)
        {
            if (NULL != m_pRDIStatsDataBuffer->splitBayerHist.pHistData[channelCount])
            {
                CAMX_FREE(m_pRDIStatsDataBuffer->splitBayerHist.pHistData[channelCount]);
                m_pRDIStatsDataBuffer->splitBayerHist.pHistData[channelCount] = NULL;
            }

            if (NULL != m_pRDIStatsDataBuffer->mergedBayerHist.pHistData[channelCount])
            {
                CAMX_FREE(m_pRDIStatsDataBuffer->mergedBayerHist.pHistData[channelCount]);
                m_pRDIStatsDataBuffer->mergedBayerHist.pHistData[channelCount] = NULL;
            }
        }

        if (HDR3ExposureType::HDR3ExposureType2 == m_HDR3ExposureType)
        {
            for (UINT32 channelCount = 0; channelCount < StatisticsBayerChannelsCount; channelCount++)
            {
                if (NULL != m_pRDIStatsDataBuffer->gridBayerStats.pGridData[channelCount])
                {
                    CAMX_FREE(m_pRDIStatsDataBuffer->gridBayerStats.pGridData[channelCount]);
                    m_pRDIStatsDataBuffer->gridBayerStats.pGridData[channelCount] = NULL;
                }
            }
        }
        CAMX_FREE(m_pRDIStatsDataBuffer);
        m_pRDIStatsDataBuffer = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::GetVendorTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::GetVendorTags(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AECEngineAlgorithmInput*       pInput)
{
    CamxResult      result = CamxResultSuccess;
    UINT64          requestId     = pStatsProcessRequestDataInfo->requestId;

    if (NULL == pInput)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "pInput is NULL");
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        Utils::Memset(&pInput->vendorTagInputList, 0, sizeof(pInput->vendorTagInputList));
        for (UINT32 i = 0; i < m_vendorTagInputList.vendorTagCount; i++)
        {
            UINT   pVendorTag[1] = { m_vendorTagInputList.vendorTag[i].vendorTagId };
            UINT64 pVendorTagOffset[1] = { 0 };

            m_pNode->GetDataList(pVendorTag,
                static_cast<VOID** const>(&pInput->vendorTagInputList.vendorTag[i].pData), pVendorTagOffset, 1);

            pInput->vendorTagInputList.vendorTag[i].vendorTagId = pVendorTag[0];
            pInput->vendorTagInputList.vendorTag[i].dataSize =
                static_cast<UINT32>(HAL3MetadataUtil::GetMaxSizeByTag(pVendorTag[0]));

            // Currently DRQ handles property/metadata dependency only on current request.
            pInput->vendorTagInputList.vendorTag[i].appliedDelay = 0;

            pInput->vendorTagInputList.vendorTag[i].sizeOfWrittenData = 0;
            if ((NULL != pInput->vendorTagInputList.vendorTag[i].pData) &&
                (pInput->vendorTagInputList.vendorTag[i].dataSize > 0))
            {
                pInput->vendorTagInputList.vendorTagCount++;
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupAEC,
                    "Failed to retrieve input vendor tag data [%d]. Id: %u pData:%p dataSize:%u reqID: %llu",
                    i, pVendorTag[0],
                    pInput->vendorTagInputList.vendorTag[i].pData,
                    pInput->vendorTagInputList.vendorTag[i].dataSize,
                    requestId);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::GetChiStatsSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::GetChiStatsSession(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AECEngineAlgorithmInput*       pInput)
{
    pInput->statsSession.SetStatsProcessorRequestData(pStatsProcessRequestDataInfo);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::GetAlgoInput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::GetAlgoInput(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AECEngineAlgorithmInput*       pInput)
{

    CamxResult result = CamxResultSuccess;

    CAMX_ENTRYEXIT(CamxLogGroupAEC);

    VOID*           pRDIStatsBuffer    = NULL;
    INT             RDIStatsBufferFd   = -1;
    SIZE_T          RDIStatsBufferSize = 0;

    if (NULL == pInput || NULL == pStatsProcessRequestDataInfo)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC,
            "Invalid Argument pInput:%p pStatsProcessRequestDataInfo:%p",
            pInput,
            pStatsProcessRequestDataInfo);
        return CamxResultEInvalidPointer;
    }

    // Populate the frame number for core to use
    pInput->requestId                       = pStatsProcessRequestDataInfo->requestId;
    pInput->requestIdOffsetFromLastFlush    =
        m_pNode->GetRequestIdOffsetFromLastFlush(pStatsProcessRequestDataInfo->requestId);

    PropertyISPHDRBEStats* pHdrBeStatsConfig = reinterpret_cast<PropertyISPHDRBEStats*>
        (m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypePropertyIDISPHDRBEConfig));
    ParsedHDRBEStatsOutput** ppHdrBeStats = reinterpret_cast<ParsedHDRBEStatsOutput**>
        (m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypePropertyIDParsedHDRBEStatsOutput));
    ParsedHDRBHistStatsOutput** ppHdrBhistStats = reinterpret_cast<ParsedHDRBHistStatsOutput**>
        (m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypePropertyIDParsedHDRBHistStatsOutput));
    ParsedBHistStatsOutput** ppBhistStats = reinterpret_cast<ParsedBHistStatsOutput**>
        (m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypePropertyIDParsedBHistStatsOutput));
    FlashModeValues* pFlashMode = reinterpret_cast<FlashModeValues*>
        (m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeFlashMode));
    ParsedIHistStatsOutput** ppIHistStats = reinterpret_cast<ParsedIHistStatsOutput**>
        (m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypePropertyIDParsedIHistStatsOutput));

    if (m_pNode->GetMaximumPipelineDelay() >= pInput->requestIdOffsetFromLastFlush ||
        pStatsProcessRequestDataInfo->skipProcessing)
    {
        pInput->bayerGrid.startupMode = StatisticsStartUpInvalid;
        pInput->bayerHist.startupMode = StatisticsStartUpInvalid;
        pInput->hdrBHist.startupMode  = StatisticsStartUpInvalid;
        pInput->imgHist.startupMode   = StatisticsStartUpInvalid;
    }
    else
    {
        // Check for presence of BE stats
        if (NULL == ppHdrBeStats || NULL == pHdrBeStatsConfig)
        {
            CAMX_LOG_ERROR(CamxLogGroupAEC, "ReqId:%lld Invalid Stats hdrBeStatsConfig:%p hdrBeStats:%p",
                pStatsProcessRequestDataInfo->requestId,
                pHdrBeStatsConfig, ppHdrBeStats);
            pInput->bayerGrid.startupMode = StatisticsStartUpInvalid;
        }
        else
        {
            // Save the BE stats inside core's input structure
            SetAlgoBayerGridValue(pHdrBeStatsConfig, *ppHdrBeStats, &pInput->bayerGrid, &pInput->bayerGridROI);
        }

        if (NULL != ppHdrBhistStats)
        {
            SetAlgoBayerHDRBHistValue(*ppHdrBhistStats,
                &pInput->bayerHist);
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupAEC,
                "ReqId:%lld PropertyID %d (ISP HDR-BHist Stats) has not been published! ",
                pStatsProcessRequestDataInfo->requestId,
                PropertyIDParsedHDRBHistStatsOutput);
            pInput->bayerHist.startupMode = StatisticsStartUpInvalid;
        }

        // Bayer and image hists are not published for BPS based camera
        if (FALSE == m_pNode->IsCameraRunningOnBPS())
        {
            if (NULL != ppBhistStats)
            {
                SetAlgoBayerHistValue(*ppBhistStats, &pInput->hdrBHist);
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupAEC,
                    "ReqId:%lld PropertyID %d (ISP BHist Stats) has not been published! ",
                    pStatsProcessRequestDataInfo->requestId,
                    PropertyIDParsedBHistStatsOutput);
                pInput->hdrBHist.startupMode = StatisticsStartUpInvalid;
            }
            if (NULL != ppIHistStats)
            {
                SetAlgoImageHistValue(*ppIHistStats, &pInput->imgHist);
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupAEC,
                    "ReqId:%lld PropertyID %d (ISP IHist Stats) has not been published! ",
                    pStatsProcessRequestDataInfo->requestId,
                    PropertyIDParsedIHistStatsOutput);
                pInput->imgHist.startupMode = StatisticsStartUpInvalid;
            }
        }
        else
        {
            pInput->hdrBHist.startupMode = StatisticsStartUpInvalid;
            pInput->imgHist.startupMode  = StatisticsStartUpInvalid;
        }

        if (HDRTypeValues::SensorHDR == m_HDRType)
        {
            for (INT32 i = 0; i < pStatsProcessRequestDataInfo->bufferCount; i++)
            {
                if (ISPStatsTypeRDIStats == pStatsProcessRequestDataInfo->bufferInfo[i].statsType)
                {
                    pRDIStatsBuffer    = pStatsProcessRequestDataInfo->bufferInfo[i].pBuffer->GetPlaneVirtualAddr(0, 0);
                    RDIStatsBufferFd   = pStatsProcessRequestDataInfo->bufferInfo[i].pBuffer->GetFileDescriptor();
                    RDIStatsBufferSize = pStatsProcessRequestDataInfo->bufferInfo[i].pBuffer->GetPlaneSize(0);
                    // Get the RDI HDR stats buffer
                    if (NULL != pRDIStatsBuffer)
                    {
                        SetAlgoRDIStatsValue(pRDIStatsBuffer, RDIStatsBufferSize, &pInput->HDR3ExposureStatsData);
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupAEC, "Invalid RDI HDR buffer to parse");
                        result = CamxResultEInvalidPointer;
                    }
                    break;
                }
            }
        }
    }

    UINT64 requestIdOffsetFromLastFlush =
        m_pNode->GetRequestIdOffsetFromLastFlush(pStatsProcessRequestDataInfo->requestId);

    if (FirstValidRequestId != requestIdOffsetFromLastFlush)
    {
        if (NULL != pFlashMode)
        {
            pInput->bIsTorchAEC = (FlashModeTorch == *pFlashMode) ? TRUE : FALSE;
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "flashMode %d bIsTorchAEC %d", *pFlashMode, pInput->bIsTorchAEC);
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupAEC, "ReqId:%lld PropertyID %d (FlashMode) has not been published! ",
                pStatsProcessRequestDataInfo->requestId,
                FlashMode);
        }
    }

    result = GetVendorTags(pStatsProcessRequestDataInfo, pInput);

    if (CamxResultSuccess == result)
    {
        result = GetChiStatsSession(pStatsProcessRequestDataInfo, pInput);
    }

    if (CamxResultSuccess == result)
    {
        SetDebugDataPointer(pStatsProcessRequestDataInfo, pInput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::SetVideoHDRInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::SetVideoHDRInformation(
    BOOL             isVideoHDREnabled,
    HDR3ExposureType HDR3ExposureType)
{
    m_HDRType          = isVideoHDREnabled ? HDRTypeValues::SensorHDR : HDRTypeValues::HDRDisabled;
    m_HDR3ExposureType = HDR3ExposureType;

    if (HDRTypeValues::SensorHDR == m_HDRType)
    {
        if (NULL == m_pRDIStatsDataBuffer)
        {
            m_pRDIStatsDataBuffer = static_cast<StatsHDR3ExposureDataType* >(CAMX_CALLOC(sizeof(StatsHDR3ExposureDataType)));

            if (NULL == m_pRDIStatsDataBuffer) return CamxResultEInvalidPointer;

            for (UINT32 channelCount = 0; channelCount < StatisticsBayerChannelsCount; channelCount++)
            {
                m_pRDIStatsDataBuffer->splitBayerHist.pHistData[channelCount] =
                    static_cast<UINT32 *>(CAMX_CALLOC(sizeof(UINT32) * StatisticsBHistMaxBinSize));

                m_pRDIStatsDataBuffer->mergedBayerHist.pHistData[channelCount] =
                    static_cast<UINT32 *>(CAMX_CALLOC(sizeof(UINT32) * StatisticsBHistMaxBinSize));
            }

            if (HDR3ExposureType::HDR3ExposureType2 == m_HDR3ExposureType)
            {
                for (UINT32 channelCount = 0; channelCount < StatisticsBayerChannelsCount; channelCount++)
                {
                    m_pRDIStatsDataBuffer->gridBayerStats.pGridData[channelCount] =
                        static_cast<UINT16 *>(CAMX_CALLOC(sizeof(UINT16) * StatisticsBGStatsNum));
                }
            }
        }
    }
    else
    {
        if (NULL != m_pRDIStatsDataBuffer)
        {

            for (UINT32 channelCount = 0; channelCount < StatisticsBayerChannelsCount; channelCount++)
            {
                if (NULL != m_pRDIStatsDataBuffer->splitBayerHist.pHistData[channelCount])
                {
                    CAMX_FREE(m_pRDIStatsDataBuffer->splitBayerHist.pHistData[channelCount]);
                    m_pRDIStatsDataBuffer->splitBayerHist.pHistData[channelCount] = NULL;
                }

                if (NULL != m_pRDIStatsDataBuffer->mergedBayerHist.pHistData[channelCount])
                {
                    CAMX_FREE(m_pRDIStatsDataBuffer->mergedBayerHist.pHistData[channelCount]);
                    m_pRDIStatsDataBuffer->mergedBayerHist.pHistData[channelCount] = NULL;
                }
            }

            if (HDR3ExposureType::HDR3ExposureType2 == m_HDR3ExposureType)
            {
                for (UINT32 channelCount = 0; channelCount < StatisticsBayerChannelsCount; channelCount++)
                {
                    if (NULL != m_pRDIStatsDataBuffer->gridBayerStats.pGridData[channelCount])
                    {
                        CAMX_FREE(m_pRDIStatsDataBuffer->gridBayerStats.pGridData[channelCount]);
                        m_pRDIStatsDataBuffer->gridBayerStats.pGridData[channelCount] = NULL;
                    }
                }
            }
            CAMX_FREE(m_pRDIStatsDataBuffer);
            m_pRDIStatsDataBuffer = NULL;
        }
    }
    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::SetDebugDataPointer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECStatsProcessor::SetDebugDataPointer(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AECEngineAlgorithmInput*       pInput)
{
    DebugData*  pDebugData  = NULL;
    UINT64      requestId   = pStatsProcessRequestDataInfo->requestId;

    // Only use debug data on the master camera
    if ((StatsAlgoRoleDefault   == pStatsProcessRequestDataInfo->cameraInfo.algoRole)   ||
        (StatsAlgoRoleMaster    == pStatsProcessRequestDataInfo->cameraInfo.algoRole))
    {
        (void)StatsUtil::GetDebugDataBuffer(m_pDebugDataPool, requestId, PropertyIDDebugDataAEC, &pDebugData);
    }

    if (NULL != pDebugData)
    {
        pInput->debugData.dataSize  = static_cast<UINT32>(pDebugData->size);
        pInput->debugData.pData     = pDebugData->pData;
    }
    else
    {
        pInput->debugData.dataSize  = 0;
        pInput->debugData.pData     = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::SetCameraInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::SetCameraInformation(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AECEngineAlgorithmInput*       pInput)
{
    CamxResult result = CamxResultSuccess;
    AECCommandInputParam    inputParam = { 0 };
    AECCommandOutputParam   outputParam = { 0 };

    if (NULL == pStatsProcessRequestDataInfo || NULL == pInput)
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupAEC, "Setting camera information failed pStatsProcessRequestDataInfo: %p pInput: %p",
            pStatsProcessRequestDataInfo, pInput);
    }

    if (CamxResultSuccess == result)
    {
        pInput->cameraInfo.algoRole = pStatsProcessRequestDataInfo->cameraInfo.algoRole;
        pInput->cameraInfo.cameraId = m_cameraId;

        inputParam.pAlgorithmInput = pInput;
        result = m_pAECEngine->HandleCommand(AECCommand::SetCameraInformation, &inputParam, &outputParam);

        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Setting camera information cameraId:%d role:%d",
            pInput->cameraInfo.cameraId, pInput->cameraInfo.algoRole);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::SetAlgoChromatix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::SetAlgoChromatix(
    ChiTuningModeParameter* pInputTuningModeData)
{
    CamxResult              result      = CamxResultSuccess;
    AECCommandInputParam    inputParam  = { 0 };
    AECCommandOutputParam   outputParam = { 0 };

    CAMX_ASSERT(m_tuningData.pTuningSetManager != NULL );

    if (NULL != pInputTuningModeData)
    {
        m_tuningData.pTuningModeSelectors   = reinterpret_cast<TuningMode*>(&pInputTuningModeData->TuningMode[0]);
        m_tuningData.numSelectors           = static_cast<UINT>(pInputTuningModeData->noOfSelectionParameter);
        inputParam.pTuningData              = &m_tuningData;
        result = m_pAECEngine->HandleCommand(AECCommand::SetChromatix, &inputParam, &outputParam);
    }

    if (NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypePropertyIDLensInfo))
    {
        m_hardwareInfo.sensorInfo.fNumber =
            reinterpret_cast<LensInfo*>(m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypePropertyIDLensInfo))->fNumber;

        inputParam.pHardwareInfo = &m_hardwareInfo;
        result                   = m_pAECEngine->HandleCommand(AECCommand::ProcessHardwareInfo, &inputParam, &outputParam);
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupAEC, "AECReadTypePropertyIDLensInfo property data is NULL");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::SetAlgoSetParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::SetAlgoSetParams(
    AECEngineHALParam* pHALParam)
{
    CamxResult              result         = CamxResultSuccess;
    AECCommandInputParam    inputParam     = { 0 };
    AECCommandOutputParam   outputParam    = { 0 };
    StatsRectangle          cropWindow     = { 0 };
    AECEngineNodesUpdate    AECNodesUpdate = { 0 };
    AFOutput                AFOutput       = {};
    AWBOutputInternal       AWBoutput      = {};
    AWBFrameInfo            AWBInfo        = {};

    inputParam.pHALParam                   = pHALParam;
    result  = m_pAECEngine->HandleCommand(AECCommand::SetPerFrameControlParam, &inputParam, &outputParam);

    // Set updates from other Stats Nodes: AF, AWB, AFD, ASD
    AFOutput.status.status            = AFStatusInitialized;
    AWBoutput.flashEstimationStatus   = AWBFlashEstimationState::AWBEstimationInactive;
    AECNodesUpdate.pAFOutput          = &AFOutput;
    AECNodesUpdate.pAWBOutputInternal = &AWBoutput;
    AECNodesUpdate.pAWBFrameInfo      = &AWBInfo;
    ReadStatsNodesUpdates(&AECNodesUpdate);
    inputParam.pNodesUpdate = &AECNodesUpdate;
    result = m_pAECEngine->HandleCommand(AECCommand::SetNodesUpdate, &inputParam, &outputParam);
    // Set the stats window, which could change with zoom
    if (CamxResultSuccess == result)
    {
        GetCropWindow(&cropWindow);
        inputParam.pCropWindow = &cropWindow;

        result = m_pAECEngine->HandleCommand(AECCommand::ProcessCropWindow, &inputParam, &outputParam);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECStatsProcessor::PublishVendorTagMetadata()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::PublishVendorTagMetadata(
    StatsVendorTagList*             pVendorTagOutput)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pVendorTagOutput)
    {
        for (UINT32 i = 0; i < pVendorTagOutput->vendorTagCount; i++)
        {
            if (0 == pVendorTagOutput->vendorTag[i].sizeOfWrittenData)
            {
                continue;
            }

            static const UINT   WriteProps[]                             = { pVendorTagOutput->vendorTag[i].vendorTagId };
            const VOID*         pOutputData[CAMX_ARRAY_SIZE(WriteProps)] = { pVendorTagOutput->vendorTag[i].pData };
            UINT                pDataCount[CAMX_ARRAY_SIZE(WriteProps)]  = { 1 };

            result = m_pNode->WriteDataList(WriteProps, static_cast<const VOID**>(pOutputData),
                pDataCount, CAMX_ARRAY_SIZE(WriteProps));

            CAMX_LOG_VERBOSE(CamxLogGroupAEC,
                "Published VendorTag(%u) size(%d) pData(%p). Result = %s",
                pVendorTagOutput->vendorTag[i].vendorTagId,
                pVendorTagOutput->vendorTag[i].dataSize,
                pVendorTagOutput->vendorTag[i].pData,
                Utils::CamxResultToString(result));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupAEC, "Failed to publish vendor tag %u. Result = %s",
                    WriteProps[0], Utils::CamxResultToString(result));
            }

        }
    }

    for (UINT i = 0; i < NumAECVendorTagsPublish; i++)
    {
        StatsUtil::WriteVendorTag(m_pNode,
                                  m_aecVendorTagsPublish[i].pTagName,
                                  m_aecVendorTagsPublish[i].pSectionName,
                                  m_aecVendorTagsPublish[i].pData,
                                  m_aecVendorTagsPublish[i].dataSize);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::AllocateMemoryVendorTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::AllocateMemoryVendorTag(
    StatsVendorTagInfoList* pVendorTagInfoList)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);

    CamxResult result = CamxResultSuccess;

    if (NULL == pVendorTagInfoList)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "pVendorTagInfoList pointer is NULL");
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        Utils::Memset(&m_algoVendorTagOutputList, 0, sizeof(m_algoVendorTagOutputList));

        for (UINT32 j = 0; j < pVendorTagInfoList->vendorTagCount; j++)
        {
            SIZE_T size = HAL3MetadataUtil::GetMaxSizeByTag(pVendorTagInfoList->vendorTag[j].vendorTagId);
            if (0 != size)
            {
                m_algoVendorTagOutputList.vendorTag[j].vendorTagId = pVendorTagInfoList->vendorTag[j].vendorTagId;
                m_algoVendorTagOutputList.vendorTag[j].appliedDelay = pVendorTagInfoList->vendorTag[j].appliedDelay;
                m_algoVendorTagOutputList.vendorTag[j].pData = CAMX_CALLOC(size);

                if (NULL == m_algoVendorTagOutputList.vendorTag[j].pData)
                {
                    CAMX_LOG_ERROR(CamxLogGroupAEC,
                        "Allocating memory for vendor tagID[%d]: %d failed.",
                        j,
                        pVendorTagInfoList->vendorTag[j].vendorTagId);
                    result = CamxResultENoMemory;
                }
                else
                {
                    m_algoVendorTagOutputList.vendorTag[j].dataSize = static_cast<UINT32>(size);
                    m_algoVendorTagOutputList.vendorTagCount++;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::PublishMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::PublishMetadata(
    AECEngineProcessStatsOutput*   pOutput,
    AECEngineHALParam*             pHALParam)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);

    CamxResult result = CamxResultSuccess;

    result = PublishPropertyPoolFrameControl(pOutput, &m_frameControl, &m_statsControl, &m_pPeerInfo);
    DumpAECStats();


    if (CamxResultSuccess == result)
    {
        result = PublishPropertyPoolFrameInfo(pOutput, pHALParam, &m_frameInfo);
        DumpFrameInfo();
    }
    if (CamxResultSuccess == result)
    {
        result = PublishExternalCameraMetadata(pOutput, &m_aeHALData);
        DumpHALData();
    }

    if (CamxResultSuccess == result)
    {
        result = PublishPropertyPoolInternal(pOutput, &m_outputInternal);
    }

    if (CamxResultSuccess == result)
    {
        result = PublishPropertyDebugData();
    }

    if (CamxResultSuccess == result)
    {
        result = PublishVendorTagMetadata(&pOutput->vendorTagList);
    }

    m_pStatsAECPropertyReadWrite->WriteProperties(NumAECPropertyWriteTags);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::OverwriteAECOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::OverwriteAECOutput(
    AECEngineProcessStatsOutput* pOutput
    ) const
{
    if (NULL == pOutput)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "pOutput pointer is NULL");
        return CamxResultEInvalidPointer;
    }

    AECAlgoFrameInfo*     pAlgoFrameInfo     = NULL;
    AECAlgoFrameControl*  pAlgoFrameControl  = NULL;
    AECAlgoAPEXData*      pAPEXData          = NULL;
    UINT32*               pLEDCurrents       = NULL;
    AECAlgoExposureData*  pExposureData      = NULL;

    pAlgoFrameInfo      = &pOutput->algorithmOutput.frameInfo;
    pAlgoFrameControl   = &pOutput->algorithmOutput.engineFrameControl.frameControl;
    pAPEXData           = &pOutput->algorithmOutput.engineFrameControl.apexData;
    pLEDCurrents        = &pOutput->algorithmOutput.engineFrameControl.LEDCurrents[0];
    pExposureData       = &pOutput->algorithmOutput.engineFrameControl.exposureData[0];

    // Populate the defaults for Frame Info
    pAlgoFrameInfo->brightnessSettled       = TRUE;
    pAlgoFrameInfo->aecSettled              = TRUE;
    pAlgoFrameInfo->asdExtremeBlueRatio     = 1.0f;
    pAlgoFrameInfo->asdExtremeGreenRatio    = 1.0f;
    pAlgoFrameInfo->snapshotIndicator       = AECAlgoSnapshotNormal;
    pAlgoFrameInfo->touchEVIndicator        = AECAlgoTouchEVInactive;
    pAlgoFrameInfo->LEDAFRequired           = FALSE;
    CamX::Utils::Memset(&pAlgoFrameInfo->legacyYStats[0], 0, sizeof(pAlgoFrameInfo->legacyYStats));

    // Populate the defaults for Frame Control
    pAlgoFrameControl->luxIndex             = m_pStaticSettings->luxIndex;
    pAlgoFrameControl->flashState           = AECAlgoFlashStateOff;
    pAlgoFrameControl->ISOValue             = 100;
    pAlgoFrameControl->LEDBGRatio           = 1.0f;
    pAlgoFrameControl->LEDRGRatio           = 1.0f;
    pAlgoFrameControl->LEDInfluenceRatio    = 1.0f;
    pAlgoFrameControl->LEDFirstEntryRatio   = 1.0f;
    pAlgoFrameControl->LEDLastEntryRatio    = 0.0f;

    // Populate the defaults for APEX Data
    pAPEXData->brightnessValue = 0.0f;
    pAPEXData->apertureValue   = 0.0f;
    pAPEXData->speedValue      = 0.0f;
    pAPEXData->timeValue       = 0.0f;
    pAPEXData->exposureValue   = 0.0f;

    // Populate the defaults for LED Currents
    pLEDCurrents[LEDSetting1]  = 0;
    pLEDCurrents[LEDSetting2]  = 0;

    // Populate the defaults for Exposure Data
    for (UINT32 i = 0; i < ExposureIndexCount; i++)
    {
        pExposureData[i].linearGain   = m_pStaticSettings->exposureGain;
        pExposureData[i].exposureTime = m_pStaticSettings->exposureTime;
        pExposureData[i].sensitivity  = m_pStaticSettings->exposureTime * m_pStaticSettings->exposureGain;
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::PublishPropertyPoolFrameControl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::PublishPropertyPoolFrameControl(
    AECEngineProcessStatsOutput*    pEngineOutput,
    AECFrameControl*                pFrameControl,
    AECStatsControl*                pStatsControl,
    VOID**                          ppPeerInfo)
{
    CamxResult result = CamxResultSuccess;
    // Populate AEC update for normal/preview frame control
    result = PopulatePropPoolFrameControl(pEngineOutput, pFrameControl);
    // Populate AEC update for stats control

    if (result == CamxResultSuccess)
    {
        PopulatePropPoolStatsControl(pEngineOutput, pStatsControl);

        PopulatePropPoolPeerControl(&m_engineStatsOutput, ppPeerInfo);

        CAMX_LOG_VERBOSE(CamxLogGroupAEC,
            "AEC: Publish FrameControl for ReqId=%llu camID:%d G ET (0: %f %llu) (1: %f %llu) (2: %f %llu)  LuxIndex=%f"
                         " LEDInfluenceRatio=%f LEDFirstEntryRatio=%f LEDLastEntryRatio=%f "
                         " FlashType=%d PreFlashState=%d PredictiveGain=%f LEDCurrents=%d:%d HxV = %dx%di pInfo:%p",
                         m_currProcessingRequestId,
                         m_cameraId,
                         pFrameControl->exposureInfo[0].linearGain,
                         pFrameControl->exposureInfo[0].exposureTime,
                         pFrameControl->exposureInfo[1].linearGain,
                         pFrameControl->exposureInfo[1].exposureTime,
                         pFrameControl->exposureInfo[2].linearGain,
                         pFrameControl->exposureInfo[2].exposureTime,
                         pFrameControl->luxIndex,
                         pFrameControl->LEDInfluenceRatio,
                         pFrameControl->LEDFirstEntryRatio,
                         pFrameControl->LEDLastEntryRatio,
                         pFrameControl->flashInfo,
                         pFrameControl->preFlashState,
                         pFrameControl->predictiveGain,
                         pFrameControl->LEDCurrents[0],
                         pFrameControl->LEDCurrents[1],
                         pStatsControl->statsConfig.BEConfig.horizontalNum,
                         pStatsControl->statsConfig.BEConfig.verticalNum,
                         ppPeerInfo);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::PublishPropertyPoolAdditionalFrameControl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::PublishPropertyPoolAdditionalFrameControl(
    AECEngineProcessStatsOutput*   pOutput)
{
    m_sensorControl = pOutput->algorithmOutput.engineFrameControl.engineAdditionalControl.sensorControl;
    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::PublishPropertyPoolFrameInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::PublishPropertyPoolFrameInfo(
    AECEngineProcessStatsOutput*   pOutput,
    AECEngineHALParam*             pHALParam,
    AECFrameInformation*           pFrameInfo)
{
    if (NULL == pOutput || NULL == pHALParam || NULL == pFrameInfo)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "pOutput: %p or pHALParam: %p or pFrameInfo: %p is NULL",
            pOutput, pHALParam, pFrameInfo);
        return CamxResultEInvalidPointer;
    }

    CamxResult result       = CamxResultSuccess;

    PopulatePropPoolFrameInfo(pOutput, pFrameInfo, pHALParam);

    CAMX_LOG_VERBOSE(CamxLogGroupAEC,
                     "AEC: Publish FrameInfo for ReqId=%llu Settled=%d Lux=%f Preflash=%d SnapshotInd=%d BrightnessValue=%f "
                     "FL:%f ",
                     m_currProcessingRequestId,
                     pFrameInfo->AECSettled,
                     pFrameInfo->luxIndex,
                     pFrameInfo->AECPreFlashState,
                     pFrameInfo->snapshotIndicator,
                     pFrameInfo->brightnessValue,
                     pFrameInfo->frameLuma);

    INT32              isFlashRequired       = (SnapshotTypeFlash == pFrameInfo->snapshotIndicator) ? 1 : 0;
    INT32              isLLSNedded           = (SnapshotTypeLLS == pFrameInfo->snapshotIndicator) ? 1 : 0;

    result = StatsUtil::WriteVendorTag(m_pNode,
                                       "com.qti.stats_control",
                                       "is_flash_snapshot",
                                       reinterpret_cast<VOID*>(&isFlashRequired),
                                       1);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "cannot write to is_flash_snapshot control vendor tag");
    }
    result = StatsUtil::WriteVendorTag(m_pNode,
                                       "com.qti.stats_control",
                                       "is_lls_needed",
                                       reinterpret_cast<VOID*>(&isLLSNedded),
                                       1);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "cannot write to is_lls_needed control vendor tag");
    }
    result = StatsUtil::WriteVendorTag(m_pNode,
                                       "com.qti.chi.statsaec",
                                       "AecLux",
                                       reinterpret_cast<VOID*>(&m_frameInfo.luxIndex),
                                       1);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "cannot write to AecLux vendor tag");
    }
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::PublishExternalCameraMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::PublishExternalCameraMetadata(
    AECEngineProcessStatsOutput*    pOutput,
    AECHALData*                     pAECHALData)
{
    CamxResult            result          = CamxResultSuccess;

    if (NULL == pOutput || NULL == pAECHALData)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "pOutput: %p or pAECHALData: %p pointer is NULL",
            pOutput, pAECHALData);
        return CamxResultEInvalidPointer;
    }

    UINT8                 mode            = static_cast<UINT8>(pOutput->AEMode);
    BOOL                  AELockFlag      = (pOutput->AEState == ControlAEStateLocked) ? TRUE : FALSE;
    UINT8                 state           = static_cast<UINT8>(pOutput->AEState);
    // For AE state, the app monitors the transition of the AE state from converged to
    // locked state so hardcode to lock will not work.However, we can make ae state to
    // NULL which will cause APP to ignore the AE state.So, please, put AE state to NULL
    // until ae algo is available.
    UINT32                activeWidth  = m_hardwareInfo.sensorInfo.sensorActiveResWidth;
    UINT32                activeHeight = m_hardwareInfo.sensorInfo.sensorActiveResHeight;
    UINT32                resWidth     = m_hardwareInfo.sensorInfo.sensorResWidth;
    UINT32                resHeight    = m_hardwareInfo.sensorInfo.sensorResHeight;
    FLOAT                 widthRatio   = (static_cast<FLOAT>(activeWidth) / resWidth);
    FLOAT                 heightRatio  = (static_cast<FLOAT>(activeHeight) / resHeight);
    StatsRectangle        statsROI     = { 0 };
    UINT8                 triggerValue;
    Rational              exposureCompensationStep;

    // Set Pre-capture Trigger
    ControlAEPrecaptureTriggerValues precaptureTrigger;
    switch (pOutput->AEPrecapTrigger)
    {
        case PrecapTriggerStart:
            precaptureTrigger = ControlAEPrecaptureTriggerStart;
            break;

        case PrecapTriggerCancel:
            precaptureTrigger = ControlAEPrecaptureTriggerCancel;
            break;

        case PrecapTriggerIdle:
        default:
            precaptureTrigger = ControlAEPrecaptureTriggerIdle;
            break;
    }

    triggerValue                         = static_cast<UINT8>(precaptureTrigger);
    pAECHALData->triggerValue            = triggerValue;

    exposureCompensationStep.numerator    = CompensationStepNumerator;
    exposureCompensationStep.denominator  = pOutput->AECEVCapabilities.stepsPerEV;
    pAECHALData->exposureCompensationStep = exposureCompensationStep;


    // Get crop window in CAMIF size
    GetCropWindow(&statsROI);
    // Set AE Regions based on crop window
    pOutput->AERegion.xMin   = Utils::RoundFLOAT((m_pAECEngine->m_HALParam.touchROI.x *
        statsROI.width * widthRatio) + (statsROI.left * widthRatio));
    pOutput->AERegion.yMin   = Utils::RoundFLOAT((m_pAECEngine->m_HALParam.touchROI.y *
        statsROI.height * heightRatio) + (statsROI.top * heightRatio));
    pOutput->AERegion.xMax   = pOutput->AERegion.xMin +
        Utils::RoundFLOAT(m_pAECEngine->m_HALParam.touchROI.dx * statsROI.width * widthRatio);
    pOutput->AERegion.yMax   = pOutput->AERegion.yMin +
        Utils::RoundFLOAT(m_pAECEngine->m_HALParam.touchROI.dy * statsROI.height * heightRatio);
    pOutput->AERegion.weight = static_cast<INT32>(m_pAECEngine->m_HALParam.touchROI.weight);

    if ((pOutput->AERegion.xMax - pOutput->AERegion.xMin) < 0 ||
        (pOutput->AERegion.yMax - pOutput->AERegion.yMin) < 0)
    {
        CAMX_LOG_INFO(CamxLogGroupAEC, "Negative CropWindow, reset window");
        pOutput->AERegion.xMin  = 0;
        pOutput->AERegion.yMin  = 0;
        pOutput->AERegion.xMax  = resWidth;
        pOutput->AERegion.yMax  = resHeight;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Publish CropWindow xMin, yMin, width, height = %d, %d, %d, %d",
        pOutput->AERegion.xMin,
        pOutput->AERegion.yMin,
        (pOutput->AERegion.xMax - pOutput->AERegion.xMin),
        (pOutput->AERegion.yMax - pOutput->AERegion.yMin));

    pAECHALData->aeAntiBandingMode = static_cast<UINT8>(m_pAECEngine->m_HALParam.AEAntibandingModeValue);
    pAECHALData->flickerMode       = static_cast<UINT8>(m_pAECEngine->m_HALParam.flickerMode);
    pAECHALData->aeCompensation    = pOutput->AECompensation;
    pAECHALData->aeLockFlag        = AELockFlag;
    pAECHALData->mode              = mode;
    pAECHALData->aeRegion          = pOutput->AERegion;
    pAECHALData->aeState           = state;
    pAECHALData->controlMode       = pOutput->controlMode;
    pAECHALData->currentFPSRange   = pOutput->currentFPSRange;

    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Published AEC Meta:camId:%d ReqId=%llu AECompensation:%d AELock:%d AEMode:%d,"
        "AE State:%d AE Precapture trigger:%d, ExposureCompStep:%d InputAntiBandingMode:%d, DetectedAntiBandingMode:%d, "
        "FPS Range(min:%d max:%d)",
        m_cameraId,
        m_currProcessingRequestId, pOutput->AECompensation, AELockFlag, mode, state, triggerValue,
        exposureCompensationStep.denominator, pAECHALData->aeAntiBandingMode,
        pAECHALData->flickerMode, pOutput->currentFPSRange.min,
        pOutput->currentFPSRange.max);
    // Publishing Additional frame control output from the algorithm to the future metadata slot
    result = PublishPropertyPoolAdditionalFrameControl(pOutput);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::PublishPropertyPoolInternal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::PublishPropertyPoolInternal(
    AECEngineProcessStatsOutput*    pOutput,
    AECOutputInternal*              pOutputInternal)
{
    if (NULL == pOutput || NULL == pOutputInternal)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "pOutput: %p or pOutputInternal: %p pointer is NULL",
            pOutput, pOutputInternal);
        return CamxResultEInvalidPointer;
    }

    PopulatePropPoolInternal(pOutput, pOutputInternal);
    CAMX_LOG_VERBOSE(CamxLogGroupAEC,
                     "AEC: Publish Internal MetaData for ReqId=%llu FlashInfo=%d PreFlashState=%d "
                     "LEDInfluenceRatio=%f RG:BG Ratio=%f:%f",
                     m_currProcessingRequestId,
                     pOutputInternal->flashInfo,
                     pOutput->preFlashState,
                     pOutputInternal->LEDInfluenceRatio,
                     pOutputInternal->LEDRGRatio,
                     pOutputInternal->LEDBGRatio);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::PublishPropertyDebugData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::PublishPropertyDebugData()
{
    CamxResult  result = CamxResultSuccess;
    m_debugData        = *reinterpret_cast<DebugData*>(
    m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypePropertyIDDebugDataAll));
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::PublishPreRequestOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::PublishPreRequestOutput(
    MetadataPool* pUsecasePool)
{
    CamxResult                  result                = CamxResultSuccess;
    AECCommandInputParam        inputParam            = {0};
    AECCommandOutputParam       outputParam           = {0};
    AECEngineFrameControl       engineFrameControl    = {};
    AECFrameControl             usecaseFrameControl   = {{{0}}};
    AECStatsControl             usecaseStatsControl   = {{{0}}};
    AECStatsControl             statsControl          = {{{0}}};
    AECFrameControl             frameControl          = {{{0}}};

    // Query the algorithm for the start streaming parameter
    // Read from use case pool to check if we have results from Fast AEC
    m_pAECEngine->m_statsStreamInitConfig.operationMode = StatsOperationModeNormal;
    result = StatsUtil::GetStatsStreamInitConfig(pUsecasePool, &m_pAECEngine->m_statsStreamInitConfig);

    inputParam.pInitStreamConfig = &m_pAECEngine->m_statsStreamInitConfig;
    outputParam.pFrameControl    = &engineFrameControl;

    result = m_pAECEngine->HandleCommand(AECCommand::StartStreaming, &inputParam, &outputParam);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "Failed to query start exposure parameters. result: %s",
                       Utils::CamxResultToString(result));
        return result;
    }

    // Populate PropertyIDUsecaseAECFrameControl from the engine command output
    usecaseFrameControl.luxIndex = engineFrameControl.frameControl.luxIndex;
    CAMX_STATIC_ASSERT(AECAlgoExposureCount == ExposureIndexCount);
    for (UINT32 i = 0; i < ExposureIndexCount; i++)
    {
        usecaseFrameControl.exposureInfo[i].deltaEVFromTarget = engineFrameControl.exposureData[i].deltaEVFromTarget;
        usecaseFrameControl.exposureInfo[i].exposureTime      = engineFrameControl.exposureData[i].exposureTime;
        usecaseFrameControl.exposureInfo[i].linearGain        = engineFrameControl.exposureData[i].linearGain;
        usecaseFrameControl.exposureInfo[i].sensitivity       = engineFrameControl.exposureData[i].sensitivity;
    }

    // Populate PropertyIDUsecaseAECStatsControl from the engine command output
    SetStatsConfigFromAlgoConfig(&(engineFrameControl.statsConfig),
                                 &(engineFrameControl.statsBHISTConfig),
                                 &(usecaseStatsControl.statsConfig));

    CAMX_LOG_INFO(CamxLogGroupAEC,
                  "AEC: Publish startup exp in UseCase Pool: Gain=%f ExpTime=%llu StatsConfig HxV = %dx%d",
                  engineFrameControl.exposureData[0].linearGain,
                  engineFrameControl.exposureData[0].exposureTime,
                  usecaseStatsControl.statsConfig.BEConfig.horizontalNum,
                  usecaseStatsControl.statsConfig.BEConfig.verticalNum);

    m_engineStatsOutput.algorithmOutput.engineFrameControl = engineFrameControl;

    // Populate AEC update for frame control and stats control
    PopulatePropPoolFrameControl(&m_engineStatsOutput, &frameControl);
    PopulatePropPoolStatsControl(&m_engineStatsOutput, &statsControl);

    static const UINT WriteProps[] =
    {
        PropertyIDUsecaseAECFrameControl,
        PropertyIDUsecaseAECStatsControl
    };
    const VOID* pOutputData[CAMX_ARRAY_SIZE(WriteProps)] =
    {
        &usecaseFrameControl,
        &usecaseStatsControl
    };
    UINT pDataCount[CAMX_ARRAY_SIZE(WriteProps)] =
    {
        sizeof(usecaseFrameControl),
        sizeof(usecaseStatsControl)
    };

    // Writing only usecase, so can write outside EPR
    m_pNode->WriteDataList(WriteProps, static_cast<const VOID**>(pOutputData), pDataCount, CAMX_ARRAY_SIZE(WriteProps));

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECStatsProcessor::SetAlgoBayerGridValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::SetAlgoBayerGridValue(
    PropertyISPHDRBEStats*  pISPHDRStats,
    ParsedHDRBEStatsOutput* pHDRBEOutput,
    StatsBayerGrid*         pBayerGrid,
    StatsRectangle*         pBayerGridROI)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);

    CamxResult  result       = CamxResultSuccess;
    BGBEConfig* pHDRBEConfig = &pISPHDRStats->statsConfig.HDRBEConfig;

    pBayerGrid->horizontalRegionCount       = pHDRBEConfig->horizontalNum;
    pBayerGrid->verticalRegionCount         = pHDRBEConfig->verticalNum;
    pBayerGrid->totalRegionCount            = (pHDRBEConfig->horizontalNum * pHDRBEConfig->verticalNum);
    pBayerGrid->regionWidth                 = pISPHDRStats->statsConfig.regionWidth;
    pBayerGrid->regionHeight                = pISPHDRStats->statsConfig.regionHeight;
    pBayerGrid->bitDepth                    = static_cast<UINT16>(pHDRBEConfig->outputBitDepth);

    CAMX_STATIC_ASSERT(sizeof(pBayerGrid->satThreshold) == sizeof(pHDRBEConfig->channelGainThreshold));

    CamX::Utils::Memcpy(pBayerGrid->satThreshold,
                        pHDRBEConfig->channelGainThreshold,
                        sizeof(pBayerGrid->satThreshold));

    pBayerGrid->flags.hasSatInfo            = pHDRBEOutput->flags.hasSatInfo;
    pBayerGrid->flags.usesY                 = pHDRBEOutput->flags.usesY;
    pBayerGrid->numValidRegions             = pHDRBEOutput->numROIs;

    pBayerGrid->SetChannelDataArray(reinterpret_cast<StatsBayerGridChannelInfo*>(pHDRBEOutput->GetChannelDataArray()));

    pBayerGridROI->left                     = pHDRBEConfig->ROI.left;
    pBayerGridROI->top                      = pHDRBEConfig->ROI.top;
    pBayerGridROI->width                    = pHDRBEConfig->ROI.width;
    pBayerGridROI->height                   = pHDRBEConfig->ROI.height;

    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "BE: algo input numValidRegions:%d, totalRegionCount:%d, horizontalRegionCount:%d,"
                     "verticalRegionCount:%d, ROI:(%d, %d, %d, %d), regionWidth:%d, regionHeight:%d, bitDepth:%d, "
                     "hasSatInfo:%d, usesY:%d",
                     pBayerGrid->numValidRegions,
                     pBayerGrid->totalRegionCount,
                     pBayerGrid->horizontalRegionCount,
                     pBayerGrid->verticalRegionCount,
                     pBayerGridROI->left,
                     pBayerGridROI->top,
                     pBayerGridROI->width,
                     pBayerGridROI->height,
                     pBayerGrid->regionWidth,
                     pBayerGrid->regionHeight,
                     pBayerGrid->bitDepth,
                     pBayerGrid->flags.hasSatInfo,
                     pBayerGrid->flags.usesY);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::SetAlgoBayerHDRBHistValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::SetAlgoBayerHDRBHistValue(
    ParsedHDRBHistStatsOutput*  pHDRBHistStatsOutput,
    StatsBayerHist*             pBayerHistogram)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);

    CamxResult result = CamxResultSuccess;

    pBayerHistogram->channelCount = 3;
    pBayerHistogram->uniform      = TRUE;
    pBayerHistogram->binCount     = pHDRBHistStatsOutput->numBins;

    if (pHDRBHistStatsOutput->numBins == 0)
    {
        CAMX_LOG_WARN(CamxLogGroupAEC, "0 BIN COUNT!");
    }

    pBayerHistogram->histDataType[0] = StatsColorChannelR;
    pBayerHistogram->pHistData[0]    = pHDRBHistStatsOutput->HDRBHistStats.redHistogram;

    pBayerHistogram->histDataType[1] = StatsColorChannelG;
    pBayerHistogram->pHistData[1]    = pHDRBHistStatsOutput->HDRBHistStats.greenHistogram;

    pBayerHistogram->histDataType[2] = StatsColorChannelB;
    pBayerHistogram->pHistData[2]    = pHDRBHistStatsOutput->HDRBHistStats.blueHistogram;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::SetAlgoBayerHistValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::SetAlgoBayerHistValue(
    ParsedBHistStatsOutput* pBHistStatsOutput,
    StatsBayerHist*         pBayerHistogram)
{
    CamxResult result = CamxResultSuccess;

    pBayerHistogram->binCount   = pBHistStatsOutput->numBins;
    pBayerHistogram->uniform    = pBHistStatsOutput->uniform;

    // For Bhist at a time only channel is set always.
    pBayerHistogram->channelCount = 1;

    for (UINT i = 1; i < StatisticsBayerChannelsCount; i++)
    {
        pBayerHistogram->histDataType[i] = StatsColorChannelInvalid;
    }

    UINT8 channel = static_cast<UINT8>(pBHistStatsOutput->channelType);
    switch (channel)
    {
        case ColorChannelR:
            pBayerHistogram->histDataType[0] = StatsColorChannelR;
            break;
        case ColorChannelB:
            pBayerHistogram->histDataType[0] = StatsColorChannelB;
            break;
        case ColorChannelGR:
            pBayerHistogram->histDataType[0] = StatsColorChannelGR;
            break;
        case ColorChannelGB:
            pBayerHistogram->histDataType[0] = StatsColorChannelGB;
            break;
        case ColorChannelG:
            pBayerHistogram->histDataType[0] = StatsColorChannelG;
            break;
        case ColorChannelY:
            pBayerHistogram->histDataType[0] = StatsColorChannelY;
            break;
        default:
            CAMX_LOG_WARN(CamxLogGroupAEC, "Unsupported bayer hist channel!");
            result = CamxResultEUnsupported;
            break;
    }

    if (CamxResultSuccess == result)
    {
        pBayerHistogram->pHistData[0] = pBHistStatsOutput->BHistogramStats;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::SetAlgoImageHistValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::SetAlgoImageHistValue(
    ParsedIHistStatsOutput*  pIHistStatsOutput,
    StatsIHist*              pImageHistogram)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);
    CamxResult result = CamxResultSuccess;

    pImageHistogram->numBins = pIHistStatsOutput->numBins;

    if (pImageHistogram->numBins == 0)
    {
        CAMX_LOG_WARN(CamxLogGroupAEC, "0 BIN COUNT!");
    }

    pImageHistogram->channelData[IHistChannelYCC]   = pIHistStatsOutput->imageHistogram.YCCHistogram;
    pImageHistogram->validChannelsMask              |= (1 << IHistChannelYCC);

    pImageHistogram->channelData[IHistChannelG]     = pIHistStatsOutput->imageHistogram.greenHistogram;
    pImageHistogram->validChannelsMask              |= (1 << IHistChannelG);

    pImageHistogram->channelData[IHistChannelB]     = pIHistStatsOutput->imageHistogram.blueHistogram;
    pImageHistogram->validChannelsMask              |= (1 << IHistChannelB);

    pImageHistogram->channelData[IHistChannelR]     = pIHistStatsOutput->imageHistogram.redHistogram;
    pImageHistogram->validChannelsMask              |= (1 << IHistChannelR);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::SetStatsParseFuncPtr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::SetStatsParseFuncPtr(
    VOID* pStatsParseFuncPtr)
{
    CamxResult result = CamxResultSuccess;

    m_pfnStatsParse = reinterpret_cast<StatsParseFuncPtr_t>(pStatsParseFuncPtr);
    CAMX_LOG_INFO(CamxLogGroupAEC, "Set sensor stats parse function pointer successfully");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::SetAlgoRDIStatsValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::SetAlgoRDIStatsValue(
    VOID*              pRDIStatsOutput,
    SIZE_T             RDIStatsBufferSize,
    HDR3ExposureStats* pRDIStatsData)
{
    CAMX_UNREFERENCED_PARAM(RDIStatsBufferSize);

    CamxResult result = CamxResultSuccess;

    UINT8* pSrcBuf = static_cast<UINT8* >(pRDIStatsOutput);

    if (NULL != m_pfnStatsParse)
    {
        m_pfnStatsParse(pSrcBuf, m_pRDIStatsDataBuffer, m_HDR3ExposureType, 0);
    }else
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "Stats parser function pointer is NULL, need to be implemented in sensor driver");
    }

    pRDIStatsData->pHDR3ExposureStats = m_pRDIStatsDataBuffer;
    pRDIStatsData->HDR3ExposureType   = m_HDR3ExposureType;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::PrepareAlgorithmOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::PrepareAlgorithmOutput(
    AECEngineProcessStatsOutput*  pOutput,
    UINT64                        requestId
    ) const
{
    if (NULL == pOutput)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "pOutput pointer is NULL");
        return CamxResultEInvalidPointer;
    }

    if (m_pNode->GetMaximumPipelineDelay() < requestId)
    {
        CamX::Utils::Memset(pOutput, 0, sizeof(AECEngineProcessStatsOutput));
    }
    pOutput->vendorTagList = m_algoVendorTagOutputList;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::PopulatePropPoolFrameControl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::PopulatePropPoolFrameControl(
    AECEngineProcessStatsOutput* pOutput,
    AECFrameControl*             pFrameControlOut)
{
    CamxResult             result               = CamxResultSuccess;
    AECEngineFrameControl* pEngineFrameControl  = NULL;
    AECAlgoExposureData*   pExposureData        = NULL;
    static FLOAT           prevLuxIndex;

    if (NULL == pOutput || NULL == pFrameControlOut)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "pOutput: %p or pFrameControlOut: %p pointer is NULL",
            pOutput, pFrameControlOut);
        return CamxResultEInvalidPointer;
    }

    pEngineFrameControl = &pOutput->algorithmOutput.engineFrameControl;
    pExposureData       = &pOutput->algorithmOutput.engineFrameControl.exposureData[0];

    CAMX_STATIC_ASSERT(ExposureIndexCount == AECAlgoExposureCount);

    pFrameControlOut->LEDInfluenceRatio            = pEngineFrameControl->frameControl.LEDInfluenceRatio;
    pFrameControlOut->luxIndex                     = pEngineFrameControl->frameControl.luxIndex;
    m_aecLuxIndex                                  = pEngineFrameControl->frameControl.luxIndex;
    pFrameControlOut->prevLuxIndex                 = prevLuxIndex;
    pFrameControlOut->preFlashState                = pOutput->preFlashState;
    pFrameControlOut->calibFlashState              = pOutput->calibFlashState;
    pFrameControlOut->flashInfo                    = GetFlashInfoType(pEngineFrameControl->frameControl.flashState);
    pFrameControlOut->LEDCurrents[LEDSetting1]     = pEngineFrameControl->LEDCurrents[LEDSetting1];
    pFrameControlOut->LEDCurrents[LEDSetting2]     = pEngineFrameControl->LEDCurrents[LEDSetting2];
    pFrameControlOut->LEDFirstEntryRatio           = pEngineFrameControl->frameControl.LEDFirstEntryRatio;
    pFrameControlOut->LEDLastEntryRatio            = pEngineFrameControl->frameControl.LEDLastEntryRatio;
    pFrameControlOut->predictiveGain               = pEngineFrameControl->frameControl.predictiveGain;
    pFrameControlOut->digitalGainForSimulation     = pEngineFrameControl->frameControl.digitalGainForSimulation;
    pFrameControlOut->compenADRCGain               = pEngineFrameControl->frameControl.compenADRCGain;
    pFrameControlOut->isInSensorHDR3ExpSnapshot    = pEngineFrameControl->frameControl.isInSensorHDR3ExpSnapshot;
    pFrameControlOut->inSensorHDR3ExpTriggerOutput = pEngineFrameControl->frameControl.inSensorHDR3ExpTriggerOutput;

    prevLuxIndex = pEngineFrameControl->frameControl.luxIndex;

    for (UINT32 i = 0; i < ExposureIndexCount; i++)
    {
        if ((0.0f == pExposureData[i].linearGain) || (0 == pExposureData[i].exposureTime))
        {
            CAMX_LOG_ERROR(CamxLogGroupAEC, "Invalid exposure parameters: gain: %f, exposureTime: %llu",
                           pExposureData[i].linearGain,
                           pExposureData[i].exposureTime);
            result = CamxResultEFailed;
            break;
        }

        pFrameControlOut->exposureInfo[i].deltaEVFromTarget = pExposureData[i].deltaEVFromTarget;
        pFrameControlOut->exposureInfo[i].linearGain        = pExposureData[i].linearGain;
        pFrameControlOut->exposureInfo[i].sensitivity       = pExposureData[i].sensitivity;
        pFrameControlOut->exposureInfo[i].exposureTime      = pExposureData[i].exposureTime;
        m_aecSensitivity[i]                                 = pExposureData[i].sensitivity;
        m_aecExposureTime[i]                                = pExposureData[i].exposureTime;
        m_aecLinearGain[i]                                  = pExposureData[i].linearGain;
    }

    // Hooking Up AEC Algo Parameters to be consumed by ISP pipeline
    pFrameControlOut->stretchControl.enable  = pEngineFrameControl->frameControl.stretchControl.enable;
    pFrameControlOut->stretchControl.clamp   = pEngineFrameControl->frameControl.stretchControl.clamp;
    pFrameControlOut->stretchControl.scaling = pEngineFrameControl->frameControl.stretchControl.scaling;

    OverrideInSensorHDR3ExpOutputMeta(pFrameControlOut);

    FLOAT realGain = pFrameControlOut->exposureInfo[ExposureIndexSafe].linearGain;
    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Stats Processor: AEC Gain published = %f", realGain);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::PopulatePropPoolPeerControl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECStatsProcessor::PopulatePropPoolPeerControl(
    AECEngineProcessStatsOutput* pOutput,
    VOID**             ppPeerInfo)
{
    *ppPeerInfo = pOutput->algorithmOutput.pPeerInfo;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::PopulatePropPoolStatsControl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECStatsProcessor::PopulatePropPoolStatsControl(
    AECEngineProcessStatsOutput* pOutput,
    AECStatsControl*             pStatsControl)
{
    SetStatsConfigFromAlgoConfig(&(pOutput->algorithmOutput.engineFrameControl.statsConfig),
                                 &(pOutput->algorithmOutput.engineFrameControl.statsBHISTConfig),
                                 &(pStatsControl->statsConfig));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::PopulatePropPoolFrameInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::PopulatePropPoolFrameInfo(
    AECEngineProcessStatsOutput* pOutput,
    AECFrameInformation*         pFrameInfo,
    AECEngineHALParam*           pHALParam)
{
    CamxResult           result            = CamxResultSuccess;
    AECAlgoFrameInfo*    pAlgoFrameInfo    = NULL;
    AECAlgoFrameControl* pAlgoFrameControl = NULL;
    UINT32*              pLEDCurrents      = NULL;
    AECAlgoExposureData* pExposureData     = NULL;

    pAlgoFrameInfo      = &pOutput->algorithmOutput.frameInfo;
    pAlgoFrameControl   = &pOutput->algorithmOutput.engineFrameControl.frameControl;
    pLEDCurrents        = &pOutput->algorithmOutput.engineFrameControl.LEDCurrents[0];
    pExposureData       = &pOutput->algorithmOutput.engineFrameControl.exposureData[0];

    /// @todo (CAMX-1171): Implement Pre-capture Trigger feature
    pFrameInfo->AECPrecaptureTrigger    = PrecapTriggerIdle;
    pFrameInfo->AECPreFlashState        = pOutput->preFlashState;
    pFrameInfo->AECSettled              = pAlgoFrameInfo->aecSettled;
    pFrameInfo->pCustomData             = NULL;
    /// @todo (CAMX-649): Implement debug data support
    pFrameInfo->pDebugData              = NULL;
    pFrameInfo->ISOValue                = pAlgoFrameControl->ISOValue;
    pFrameInfo->LEDAFRequired           = pAlgoFrameInfo->LEDAFRequired;
    pFrameInfo->luxIndex                = pAlgoFrameControl->luxIndex;
    pFrameInfo->frameLuma               = pAlgoFrameInfo->frameLuma;
    m_aecProcessedROI.x                 = pAlgoFrameInfo->processedROI.x;
    m_aecProcessedROI.y                 = pAlgoFrameInfo->processedROI.y;
    m_aecProcessedROI.dx                = pAlgoFrameInfo->processedROI.dx;
    m_aecProcessedROI.dy                = pAlgoFrameInfo->processedROI.dy;

    CAMX_STATIC_ASSERT(LEDSettingCount == StatisticsMaxNumOfLED);

    pFrameInfo->LEDCurrents[LEDSetting1]    = pLEDCurrents[LEDSetting1];
    pFrameInfo->LEDCurrents[LEDSetting2]    = pLEDCurrents[LEDSetting2];
    pFrameInfo->frameDuration               = pHALParam->frameDuration;

    CAMX_STATIC_ASSERT( ExposureIndexCount == AECAlgoExposureCount );

    for (UINT32 i = 0; i < ExposureIndexCount; i++)
    {
        if (pExposureData[i].linearGain == 0.0f || pExposureData[i].exposureTime == 0)
        {
            CAMX_LOG_ERROR(CamxLogGroupAEC, "Invalid exposure parameters: gain: %f, exposureTime: %llu",
                           pExposureData[i].linearGain, pExposureData[i].exposureTime);
            result = CamxResultEFailed;
            break;
        }

        pFrameInfo->exposureInfo[i].deltaEVFromTarget = pExposureData[i].deltaEVFromTarget;
        pFrameInfo->exposureInfo[i].linearGain        = pExposureData[i].linearGain;
        pFrameInfo->exposureInfo[i].sensitivity       = pExposureData[i].sensitivity;
        pFrameInfo->exposureInfo[i].exposureTime      = pExposureData[i].exposureTime;
    }

    if (CamxResultSuccess == result)
    {

        switch (pAlgoFrameInfo->snapshotIndicator)
        {
            case AECAlgoSnapshotNormal:
                pFrameInfo->snapshotIndicator = SnapshotTypeNormal;
                break;

            case AECAlgoSnapshotFlash:
                pFrameInfo->snapshotIndicator = SnapshotTypeFlash;
                break;

            case AECAlgoSnapshotLLS:
                pFrameInfo->snapshotIndicator = SnapshotTypeLLS;
                break;

            default:
                CAMX_LOG_ERROR(CamxLogGroupAEC, "Invalid snapshot indicator: %d", pAlgoFrameInfo->snapshotIndicator);
                pFrameInfo->snapshotIndicator = SnapshotTypeNormal;
                break;
        }

        switch (pAlgoFrameInfo->touchEVIndicator)
        {
            case AECAlgoTouchEVInactive:
                pFrameInfo->touchEVIndicator = AECTouchEVTypeInactive;
                break;

            case AECAlgoTouchEVConverging:
                pFrameInfo->touchEVIndicator = AECTouchEVTypeConverging;
                break;

            case AECAlgoTouchEVSettled:
                pFrameInfo->touchEVIndicator = AECTouchEVTypeSettled;
                break;

            case AECAlgoTouchEVSceneChange:
                pFrameInfo->touchEVIndicator = AECTouchEVTypeSceneChange;
                break;

            default:
                CAMX_LOG_ERROR(CamxLogGroupAEC, "Invalid EV indicator type: %d",
                               pAlgoFrameInfo->touchEVIndicator);
                pFrameInfo->touchEVIndicator = AECTouchEVTypeInactive;
                break;
        }
    }

    if (CamxResultSuccess == result)
    {
        /* Update the fields required for EXIF update */
        pFrameInfo->meteringMode        = static_cast<UINT16>(pHALParam->AEMeteringMode);
        pFrameInfo->exposureProgram     = (AECState::Manual == m_pAECEngine->GetAECState()) ?
                                          AECExposureProgramManual : AECExposureProgramNormal;
        pFrameInfo->brightnessValue     = pOutput->algorithmOutput.engineFrameControl.apexData.brightnessValue;
        pFrameInfo->sceneType           = 1; // If a DSC recorded the image, this tag value shall always be set to 1
        if (ControlAEModeOff == pHALParam->AEMode)
        {
            pFrameInfo->exposureMode = 1; // 1 = Manual exposure
        }
        else
        {
            pFrameInfo->exposureMode = 0; // 0 = Auto exposure
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::PopulatePropPoolInternal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::PopulatePropPoolInternal(
    AECEngineProcessStatsOutput* pOutput,
    AECOutputInternal*           pInternalOutput)
{
    AECAlgoFrameInfo*    pAlgoFrameInfo         = &pOutput->algorithmOutput.frameInfo;
    AECAlgoFrameControl* pAlgoFrameControl      = &pOutput->algorithmOutput.engineFrameControl.frameControl;
    AECAlgoAPEXData*     pAPEXData              = &pOutput->algorithmOutput.engineFrameControl.apexData;

    pInternalOutput->APEXValues.brightness      = pAPEXData->brightnessValue;
    pInternalOutput->APEXValues.aperture        = pAPEXData->apertureValue;
    pInternalOutput->APEXValues.speed           = pAPEXData->speedValue;
    pInternalOutput->APEXValues.time            = pAPEXData->timeValue;
    pInternalOutput->APEXValues.exposure        = pAPEXData->exposureValue;
    pInternalOutput->asdExtremeGreenRatio       = pAlgoFrameInfo->asdExtremeGreenRatio;
    pInternalOutput->asdExtremeBlueRatio        = pAlgoFrameInfo->asdExtremeBlueRatio;
    pInternalOutput->brightnessSettled          = pAlgoFrameInfo->brightnessSettled;

    /* AWB Needs Main flash gains at start of AWB, rather than exactly at Main flash time frame */
    if (PreFlashStateTriggerAWB == pOutput->preFlashState)
    {
        pAlgoFrameControl                   = &pOutput->algorithmOutput.engineFrameControl.mainFlashFrameControl;
    }

    pInternalOutput->flashInfo              = GetFlashInfoType(pAlgoFrameControl->flashState);;
    pInternalOutput->LEDInfluenceRatio      = pAlgoFrameControl->LEDInfluenceRatio;
    pInternalOutput->LEDRGRatio             = pAlgoFrameControl->LEDRGRatio;
    pInternalOutput->LEDBGRatio             = pAlgoFrameControl->LEDBGRatio;

    CAMX_STATIC_ASSERT(sizeof(pInternalOutput->legacyYStats) == sizeof(pAlgoFrameInfo->legacyYStats));
    CamX::Utils::Memcpy(&pInternalOutput->legacyYStats[0],
                        &pAlgoFrameInfo->legacyYStats[0],
                        sizeof(pInternalOutput->legacyYStats));

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::ReadInputVendorTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECStatsProcessor::ReadInputVendorTag(
    AECEngineHALParam* pHALParam
    ) const
{
    CamxResult result;
    VOID* pData;
#define READ(sec, tag, val) \
    result = StatsUtil::ReadVendorTag(m_pNode, sec, tag, &pData);\
    if (CamxResultSuccess == result  && NULL != pData)\
    {\
        Utils::Memcpy(&val, pData, sizeof(val));\
    }\

    READ("org.codeaurora.qcamera3.exposure_metering", "exposure_metering_mode", pHALParam->AEMeteringMode);
    READ("org.codeaurora.qcamera3.iso_exp_priority", "select_priority", pHALParam->ISOExposureTimePriortyMode);
    READ("org.codeaurora.qcamera3.iso_exp_priority", "use_iso_exp_priority", pHALParam->ISOorExposureTimePriorityValue);
    READ("org.codeaurora.qcamera3.iso_exp_priority", "use_iso_value", pHALParam->ISOValue);
    READ("org.codeaurora.qcamera3.iso_exp_priority", "use_gain_value", pHALParam->gain);
    READ("org.codeaurora.qcamera3.ae_bracket", "mode", pHALParam->AEBracketMode);
    READ("org.codeaurora.qcamera3.adrc", "disable", pHALParam->disableADRC);

    READ("org.codeaurora.qcamera3.exposuretable", "isValid", pHALParam->customExposureTable.isValid);
    if (TRUE == pHALParam->customExposureTable.isValid)
    {
        READ("org.codeaurora.qcamera3.exposuretable", "sensitivityCorrectionFactor",
            pHALParam->customExposureTable.sensitivityCorrectionFactor);
        READ("org.codeaurora.qcamera3.exposuretable", "kneeCount", pHALParam->customExposureTable.kneeCount);
        READ("org.codeaurora.qcamera3.exposuretable", "gainKneeEntries", pHALParam->customExposureTable.gain);
        READ("org.codeaurora.qcamera3.exposuretable", "expTimeKneeEntries", pHALParam->customExposureTable.expTime);
        READ("org.codeaurora.qcamera3.exposuretable", "incrementPriorityKneeEntries",
            pHALParam->customExposureTable.incrementPriority);
        READ("org.codeaurora.qcamera3.exposuretable", "expIndexKneeEntries", pHALParam->customExposureTable.expIndex);
        READ("org.codeaurora.qcamera3.exposuretable", "thresAntiBandingMinExpTimePct",
            pHALParam->customExposureTable.antiBandingMinimumTimePercentageThreshold);
    }

    READ("org.codeaurora.qcamera3.meteringtable", "meteringTableSize", pHALParam->customMeteringTable.meteringTableSize);
    if (0 < pHALParam->customMeteringTable.meteringTableSize)
    {
        READ("org.codeaurora.qcamera3.meteringtable", "meteringTable", pHALParam->customMeteringTable.meteringTable);
    }
    READ("org.codeaurora.qcamera3.aec_convergence_speed", "aec_speed", pHALParam->convergenceSpeed);

    if (FALSE == m_pAECEngine->m_warmStartDone)
    {
        READ("org.quic.camera2.statsconfigs", "AECStartUpSensitivity", pHALParam->warmStartSensitivity);

        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Warm Start Sensitivities[short:%f long:%f safe:%f]",
            pHALParam->warmStartSensitivity[AECAlgoExposureShort],
            pHALParam->warmStartSensitivity[AECAlgoExposureLong],
            pHALParam->warmStartSensitivity[AECAlgoExposureSafe]);
    }

    READ("org.codeaurora.qcamera3.instant_aec", "instant_aec_mode", pHALParam->instantAECMode);

    // Get the seamless in-sensor control state from vendor tags
    READ("com.qti.insensor_control", "seamless_insensor_state", pHALParam->seamlessInSensorState);

#undef READ
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::ConfigureAESyncLockParam
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::ConfigureAESyncLockParam(
    AECEngineHALParam*             pHALParam,
    UINT64                         requestID,
    StatsAlgoRole                  role)
{
    UINT64*         pRequestID;
    UINT64          aeLockStartRequestID    = 0;
    UINT64          aeLockStopRequestID     = 0;
    MetadataPool*   pPerUsecasePool         = m_pStatsInitializeData->pPipeline->GetPerFramePool(PoolType::PerUsecase);
    MetadataSlot*   pUsecasePoolSlot        = pPerUsecasePool->GetSlot(0);

    pRequestID = static_cast<UINT64*>(pUsecasePoolSlot->GetMetadataByTag(PropertyIDUsecaseAESyncStartLockTagID));

    if (NULL != pRequestID)
    {
        aeLockStartRequestID = *pRequestID;
    }

    pRequestID = static_cast<UINT64*>(pUsecasePoolSlot->GetMetadataByTag(PropertyIDUsecaseAESyncStopLockTagID));

    if (NULL != pRequestID)
    {
        aeLockStopRequestID = *pRequestID;
    }

    if (aeLockStartRequestID != 0 && aeLockStopRequestID != 0)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "AE lock on request %llu to %llu and current requestId=%llu",
            aeLockStartRequestID,
            aeLockStopRequestID,
            requestID);
        if ((requestID >= aeLockStartRequestID) && (requestID <= aeLockStopRequestID))
        {
            if (StatsAlgoRoleMaster == role)
            {
                CAMX_LOG_CONFIG(CamxLogGroupAEC, "AE lock on request %llu applied", requestID);
                pHALParam->AELock = ControlAELockOn;
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupAEC, "AE lock on request %llu not applicable as we are not master:%d",
                    requestID,
                    role);
            }
        }
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::RetrieveFlashStatsInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::RetrieveFlashStatsInfo(
    const StatsProcessRequestData*          pStatsProcessRequestDataInfo
    ) const
{
    CAMX_UNREFERENCED_PARAM(pStatsProcessRequestDataInfo);
    CamxResult              result = CamxResultSuccess;

    static const UINT GetData[] =
    {
        InputControlCaptureIntent,
        PropertyIDAECFrameControl
    };

    static const UINT GetDataLength                 = CAMX_ARRAY_SIZE(GetData);
    VOID*             pData[GetDataLength]          = { 0, 0 };

    // due to pipeline delay in getting stats about the capture frame,we need to get AEC frame data with
    // offset(MaxPipeLineDelay)
    UINT64            offset[GetDataLength]         =
    { m_pNode->GetMaximumPipelineDelay(), m_pNode->GetMaximumPipelineDelay() };

    m_pNode->GetDataList(GetData, pData, offset, GetDataLength);

    if (NULL == pData[0] || NULL == pData[1])
    {
        CAMX_LOG_WARN(CamxLogGroupAEC,
            "reqId: %llu PropertyIDAECFrameControl:%p InputControlCaptureIntent:%p not published",
            pStatsProcessRequestDataInfo->requestId,
            pData[1],
            pData[0]);
        result = CamxResultEFailed;
    }
    else
    {
        ControlCaptureIntentValues* pCaptureIntent = reinterpret_cast<ControlCaptureIntentValues*>(pData[0]);
        AECFrameControl*  pAECFrameControl = reinterpret_cast<AECFrameControl*>(pData[1]);

        // check for capture intent, flash state and led currents to find the snapshot frame influenced by flash
        if (ControlCaptureIntentStillCapture == *pCaptureIntent &&
            pAECFrameControl->flashInfo == FlashInfoTypeMain &&
            ((pAECFrameControl->LEDCurrents[LEDSetting1] > 0) || (pAECFrameControl->LEDCurrents[LEDSetting2] > 0)))
        {
            m_pAECEngine->SetFlashInfluencedStats(TRUE);
            CAMX_LOG_INFO(CamxLogGroupAEC, "reqID: %llu, found main flash snapshot frame based on stats",
                pStatsProcessRequestDataInfo->requestId);
        }
        else
        {
            m_pAECEngine->SetFlashInfluencedStats(FALSE);
            CAMX_LOG_INFO(CamxLogGroupAEC, "reqID: %llu, not a snapshot frame with main flash based on stats",
                pStatsProcessRequestDataInfo->requestId);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::ReadHALAECParam
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::ReadHALAECParam(
    AECEngineHALParam* pHALParam,
    const StatsProcessRequestData* pStatsProcessRequestDataInfo
    ) const
{
    CamxResult            result          = CamxResultSuccess;

    if (NULL == pHALParam)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "pHALParam pointer is NULL");
        return CamxResultEInvalidPointer;
    }

    UINT32      metaTagFDROI;
    CamxResult  resultFDROI;

    if (TRUE == m_pNode->GetStaticSettings()->useFDUseCasePool)
    {
        metaTagFDROI = PropertyIDUsecaseFDResults;
    }
    else
    {
        resultFDROI = VendorTagManager::QueryVendorTagLocation(VendorTagSectionOEMFDResults,
            VendorTagNameOEMFDResults,
            &metaTagFDROI);
    }

    UINT   metaTagFDROIData[]                                        = { metaTagFDROI };
    VOID*  pData[CAMX_ARRAY_SIZE(metaTagFDROIData)]                  = { 0 };
    UINT64 metaTagFDROIDataOffset[CAMX_ARRAY_SIZE(metaTagFDROIData)] = { m_pNode->GetMaximumPipelineDelay() };

    m_pNode->GetDataList(metaTagFDROIData, static_cast<VOID** const>(pData),
        metaTagFDROIDataOffset, CAMX_ARRAY_SIZE(metaTagFDROIData));

    UINT32 metaTagTrackerROI                                                   = PropertyIDUsecaseTrackerResults;
    UINT   metaTagTrackerROIData[]                                             = { metaTagTrackerROI };
    VOID*  pTrackerData[CAMX_ARRAY_SIZE(metaTagTrackerROIData)]                = { 0 };
    UINT64 metaTagTrackerROIDataOffset[CAMX_ARRAY_SIZE(metaTagTrackerROIData)] = { m_pNode->GetMaximumPipelineDelay() };

    m_pNode->GetDataList(metaTagTrackerROIData, static_cast<VOID** const>(pTrackerData),
        metaTagTrackerROIDataOffset, CAMX_ARRAY_SIZE(metaTagTrackerROIData));

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

    m_pNode->GetDataList(metaTagFpsOverrideData, pInputData, pInputDataOffset, pInputDataSize);
    BOOL   disableFPSLimits        = (NULL != pInputData[0] ? *(static_cast<BYTE*>(pInputData[0]))   : FALSE);
    UINT64 overrideFrameDurationNs = (NULL != pInputData[1] ? *(static_cast<UINT64*>(pInputData[1])) : 0);

    if (NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputControlAEExposureCompensation))
    {
        pHALParam->AECompensation = *(reinterpret_cast<INT32*>(m_pStatsAECPropertyReadWrite->GetPropertyData(
                                    AECReadTypeInputControlAEExposureCompensation)));
    }
    if (NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputControlAELock))
    {
        pHALParam->AELock = *(reinterpret_cast<ControlAELockValues*>(m_pStatsAECPropertyReadWrite->GetPropertyData(
                            AECReadTypeInputControlAELock)));
    }
    if (NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputControlAEMode))
    {
        pHALParam->AEMode = *(reinterpret_cast<ControlAEModeValues*>(m_pStatsAECPropertyReadWrite->GetPropertyData(
                            AECReadTypeInputControlAEMode)));
    }
    if ((StatsAlgoRoleDefault == pStatsProcessRequestDataInfo->cameraInfo.algoRole) ||
        (StatsAlgoRoleMaster == pStatsProcessRequestDataInfo->cameraInfo.algoRole))
    {
        if (NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputControlAEPrecaptureTrigger))
        {
            pHALParam->AETrigger = *(reinterpret_cast<ControlAEPrecaptureTriggerValues*>(
                                   m_pStatsAECPropertyReadWrite->GetPropertyData(
                                   AECReadTypeInputControlAEPrecaptureTrigger)));
        }
    }
    else
    {
        pHALParam->AETrigger = ControlAEPrecaptureTriggerIdle;
    }

    if (NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputControlAFTrigger))
    {
        pHALParam->AFTrigger = *(reinterpret_cast<ControlAFTriggerValues*>(m_pStatsAECPropertyReadWrite->GetPropertyData(
                               AECReadTypeInputControlAFTrigger)));
    }
    if ( NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputControlCaptureIntent))
    {
        pHALParam->captureIntent = *(reinterpret_cast<ControlCaptureIntentValues*>(
                                   m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputControlCaptureIntent)));
    }
    if (NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputControlMode))
    {
        pHALParam->controlMode = *(reinterpret_cast<ControlModeValues*>(m_pStatsAECPropertyReadWrite->GetPropertyData(
                                 AECReadTypeInputControlMode)));
    }
    if (NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputControlSceneMode))
    {
        pHALParam->controlSceneMode = *(reinterpret_cast<ControlSceneModeValues*>(
                                      m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputControlSceneMode)));
    }
    if (NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputFlashMode))
    {
        pHALParam->flashMode = *(reinterpret_cast<FlashModeValues*>(m_pStatsAECPropertyReadWrite->GetPropertyData(
                               AECReadTypeInputFlashMode)));
    }
    if (NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputSensorExposureTime))
    {
        pHALParam->exposureTime = *(reinterpret_cast<INT64*>(m_pStatsAECPropertyReadWrite->GetPropertyData(
                                  AECReadTypeInputSensorExposureTime)));
    }

    if (NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputSensorSensitivity))
    {
        pHALParam->sensitivity = *(reinterpret_cast<INT32*>(m_pStatsAECPropertyReadWrite->GetPropertyData(
                                 AECReadTypeInputSensorSensitivity)));
    }
    if (NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputControlAEAntibandingMode))
    {
        pHALParam->AEAntibandingModeValue = *(reinterpret_cast<ControlAEAntibandingModeValues*>(
                                            m_pStatsAECPropertyReadWrite->GetPropertyData(
                                            AECReadTypeInputControlAEAntibandingMode)));
    }
    if (NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputControlAERegions) && NULL == pTrackerData[0])
    {
        SetTouchROISettings(pHALParam, reinterpret_cast<WeightedRectangle*>(
            m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputControlAERegions)));
    }
    // For metaTagFDROI tag
    if (NULL != pData[0])
    {
        SetFaceROISettings(pHALParam, reinterpret_cast<FaceROIInformation*>(pData[0]));
    }

    // For metaTagTrackerROI tag
    if (NULL != pTrackerData[0])
    {
        SetTrackerROISettings(pHALParam, reinterpret_cast<TrackerROIInformation*>(pTrackerData[0]));
    }


    // Read the Antibanding mode published by AFD if the Antibanding Mode is set to AUTO.
    ReadAFDMode(pHALParam);
    pHALParam->sensorFlashState = FlashStateUnavailable;

    if (NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeFlashState))
    {
        pHALParam->sensorFlashState = *(reinterpret_cast<FlashStateValues*>(m_pStatsAECPropertyReadWrite->GetPropertyData(
                                      AECReadTypeFlashState)));
    }

    // if frame-duration-override enabled, override AEC FPS-range to limit exposure-time
    if ((TRUE == disableFPSLimits) && (0 != overrideFrameDurationNs))
    {
        pHALParam->FPSRange.min = 1 + static_cast<INT32>(NanoSecondsPerSecond / overrideFrameDurationNs);
        pHALParam->FPSRange.max = pHALParam->FPSRange.min;
    }
    else if (NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputControlAETargetFpsRange))
    {
        pHALParam->FPSRange = *(reinterpret_cast<RangeINT32*>(m_pStatsAECPropertyReadWrite->GetPropertyData(
                              AECReadTypeInputControlAETargetFpsRange)));
    }

    if (NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputControlZslEnable))
    {
        pHALParam->controlZslEnable = *(reinterpret_cast<INT32*>(m_pStatsAECPropertyReadWrite->GetPropertyData(
                                      AECReadTypeInputControlZslEnable)));
    }

    if (NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeSensorFrameDuration))
    {
        pHALParam->frameDuration = *(reinterpret_cast<UINT64*>(m_pStatsAECPropertyReadWrite->GetPropertyData(
                                   AECReadTypeSensorFrameDuration)));
    }

    if (NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputControlPostRawSensitivityBoost))
    {
        pHALParam->controlPostRawSensitivityBoost = *(reinterpret_cast<INT32*>(m_pStatsAECPropertyReadWrite->GetPropertyData(
                                                    AECReadTypeInputControlPostRawSensitivityBoost)));
    }

    ReadInputVendorTag(pHALParam);

    SetISOExpPriorityValue(pHALParam);
    // This needs to be called at last as it overrides AEC metering, compensation and ISO values based on scene mode
    SetBestShotModeSettings(pHALParam);

    // Set videoHDRType from camxsettings.xml or vendor tag
    // Here we assumed the seamless in-sensor HDR 3 exposure snapshot has the highest priority,
    // then we pass its state to AEC with the variable pHALParam->videoHDRType = HDRTypeValues::SensorHDR
    if ((TRUE == IsSeamlessInSensorHDR3ExpSnapshot()) || (HDRTypeValues::SensorHDR == m_HDRType))
    {
        pHALParam->videoHDRType = HDRTypeValues::SensorHDR;
    }
    else if (1 == m_pStaticSettings->feature1)
    {
        pHALParam->videoHDRType = HDRTypeValues::ISPHDR;
    }
    else
    {
        pHALParam->videoHDRType = HDRTypeValues::HDRDisabled;
    }

    if (StatsAlgoRoleDefault != m_algoInput.cameraInfo.algoRole &&
        (MultiCamera3ASyncDisabled != m_pStaticSettings->multiCamera3ASync) &&
        (TRUE == m_pNode->IsMultiCamera()))
    {
        GetPeerInfo(pStatsProcessRequestDataInfo, &pHALParam->pPeerInfo);
    }

    if (m_pNode->GetMaximumPipelineDelay() >= m_currProcessingRequestId)
    {
        pHALParam->skipPreflashTriggers = TRUE;
    }
    else
    {
        pHALParam->skipPreflashTriggers = FALSE;
    }

    if (TRUE == m_pStaticSettings->disableADRC)
    {
        pHALParam->disableADRC = TRUE;
    }
    pHALParam->frameId = m_currProcessingRequestId;

    RetrieveFlashStatsInfo(pStatsProcessRequestDataInfo);

    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "ReqId:%llu, camID:%d, AECompensation:%d, AELock:%d, AEMode:%d, AEMeteringMode:%d, "
                     "AETrigger:%d, AFTrigger:%d, captureIntent:%d, controlMode:%d, controlSceneMode:%d, flashMode:%d, "
                     "exposureTime:%lld, sensitivity:%d, AEAntibandingModeValue:%d, ISOExpTimePrioritySet:%d, "
                     "ISOorExposureTimePriorityValue:%lld, Gain:%f ISOValue:%d, FPS Range:(min:%d max:%d), AEBracketMode:%d, "
                     "videoHDRType:%d, ZSLEnable %d, FrameDuration:%lld Face (Cnt:%d x:%d y:%d dx:%d dy:%d), "
                     "Touch (x:%f y:%f dx:%f dy:%f), controlPostRawSensitivityBoost: %d, disableADRC: %d, "
                     "DynamicConvergenceSpeed: %f, Tracker (Cnt:%d x:%d y:%d dx:%d dy:%d), InstantAecMode: %d",
                     m_currProcessingRequestId,
                     m_cameraId,
                     pHALParam->AECompensation,
                     pHALParam->AELock,
                     pHALParam->AEMode,
                     pHALParam->AEMeteringMode,
                     pHALParam->AETrigger,
                     pHALParam->AFTrigger,
                     pHALParam->captureIntent,
                     pHALParam->controlMode,
                     pHALParam->controlSceneMode,
                     pHALParam->flashMode,
                     pHALParam->exposureTime,
                     pHALParam->sensitivity,
                     pHALParam->AEAntibandingModeValue,
                     pHALParam->ISOExposureTimePriortyMode,
                     pHALParam->ISOorExposureTimePriorityValue,
                     pHALParam->gain,
                     pHALParam->ISOValue,
                     pHALParam->FPSRange.min,
                     pHALParam->FPSRange.max,
                     pHALParam->AEBracketMode,
                     pHALParam->videoHDRType,
                     pHALParam->controlZslEnable,
                     pHALParam->frameDuration,
                     pHALParam->faceROI.ROICount,
                     pHALParam->faceROI.stabilizedROI[0].faceRect.left,
                     pHALParam->faceROI.stabilizedROI[0].faceRect.top,
                     pHALParam->faceROI.stabilizedROI[0].faceRect.width,
                     pHALParam->faceROI.stabilizedROI[0].faceRect.height,
                     pHALParam->touchROI.x,
                     pHALParam->touchROI.y,
                     pHALParam->touchROI.dx,
                     pHALParam->touchROI.dy,
                     pHALParam->controlPostRawSensitivityBoost,
                     pHALParam->disableADRC,
                     pHALParam->convergenceSpeed,
                     pHALParam->trackerROI.ROICount,
                     pHALParam->trackerROI.ROI[0].rect.left,
                     pHALParam->trackerROI.ROI[0].rect.top,
                     pHALParam->trackerROI.ROI[0].rect.width,
                     pHALParam->trackerROI.ROI[0].rect.height,
                     pHALParam->instantAECMode);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::GetHardwareInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::GetHardwareInfo(
    AECEngineHWInfo*    pHardwareInfo,
    HwContext*          pHardwareContext,
    INT32               cameraID)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);
    if (NULL == pHardwareInfo)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "pHardwareInfo pointer is NULL");
        return CamxResultEInvalidPointer;
    }

    CamxResult                      result = CamxResultSuccess;
    StatsSensorInfo*                pSensorInfo        = &pHardwareInfo->sensorInfo;
    StatsCapability*                pStatsCapabilities = &pHardwareInfo->statsCapabilities;
    StatsRectangle                  activeWindow       = { 0 };
    SensorModuleStaticCaps          sensorStaticCaps   = { 0 };
    const ImageSensorModuleData*    pSensorModuleData  = pHardwareContext->GetImageSensorModuleData(cameraID);

    if ((NULL != pSensorModuleData) && (NULL != pSensorModuleData->GetSensorDataObject()))
    {
        pSensorModuleData->GetSensorDataObject()->GetSensorStaticCapability(&sensorStaticCaps, cameraID);

        pSensorInfo->sensorActiveResWidth   = static_cast<UINT32>(sensorStaticCaps.activeArraySize.width);
        pSensorInfo->sensorActiveResHeight  = static_cast<UINT32>(sensorStaticCaps.activeArraySize.height);
    }
    else
    {
        result = CamxResultEFailed;
    }

    if (CamxResultSuccess == result)
    {
        if ((CSLPresilEnabled == GetCSLMode()) || (CSLPresilRUMIEnabled == GetCSLMode()))
        {
            pSensorInfo->currentFPS             = Utils::FloatToQNumber(30, 256);
            pSensorInfo->maxFPS                 = Utils::FloatToQNumber(30, 256);
            pSensorInfo->currentLinesPerFrame   = 1;
            pSensorInfo->currentMaxLineCount    = 1;
            pSensorInfo->maxGain                = 1.0f;
            pSensorInfo->pixelClock             = 1;
            pSensorInfo->pixelClockPerLine      = 1;
            pSensorInfo->sensorCapabilities     = 0;
            pSensorInfo->sensorResWidth         = m_pStaticSettings->IFETestImageSizeWidth;
            pSensorInfo->sensorResHeight        = m_pStaticSettings->IFETestImageSizeHeight;
            pSensorInfo->pixelSumFactor         = 1;
            pSensorInfo->fNumber                = 1.0f;

            activeWindow.top    = 0;
            activeWindow.left   = 0;
            activeWindow.width  = static_cast<INT32>(pSensorInfo->sensorResWidth);
            activeWindow.height = static_cast<INT32>(pSensorInfo->sensorResHeight);
        }
        else
        {
            const SensorMode* pSensorMode = NULL;

            result = m_pNode->GetSensorModeData(&pSensorMode);
            if (CamxResultSuccess == result)
            {
                DOUBLE minAnalogGain = pSensorModuleData->GetSensorDataObject()->GetMinAnalogGain();
                /// @todo  (CAMX-1245): Sensor to publish current FPS to the property pool.
                ///                     Below information on current FPS is incorrect
                pSensorInfo->currentFPS             = Utils::FloatToQNumber(30, 256);
                pSensorInfo->maxFPS                 = Utils::DoubleToQNumber(pSensorMode->maxFPS, 256);
                pSensorInfo->currentLinesPerFrame   = static_cast<UINT32>(pSensorMode->numLinesPerFrame);
                pSensorInfo->currentMaxLineCount    = static_cast<UINT32>(pSensorMode->maxLineCount);
                pSensorInfo->maxGain                = static_cast<FLOAT>(pSensorMode->maxGain);
                pSensorInfo->minGain                = static_cast<FLOAT>(minAnalogGain);
                pSensorInfo->pixelClock             = static_cast<UINT32>(pSensorMode->vtPixelClock);
                pSensorInfo->pixelClockPerLine      = static_cast<UINT32>(pSensorMode->numPixelsPerLine);
                /// @todo (CAMX-2264): Sensor capabilities missing
                pSensorInfo->sensorCapabilities     = 0;

                /// @todo (CAMX-2184): use binningTypeH vs binnignTypeV
                pSensorInfo->pixelSumFactor         = static_cast<UINT16>(pSensorMode->binningTypeH);
                /// @todo (CAMX-2264): Sensor F-number missing
                pSensorInfo->fNumber                = 1.0f;

                // Active sensor region for which stats is collected is the cropped sensor output
                const IFEInputResolution* pIFEInput = NULL;

                result = m_pNode->GetIFEInputResolution(&pIFEInput);
                if ((CamxResultSuccess == result) && (NULL != pIFEInput))
                {
                    pSensorInfo->sensorResWidth     = static_cast<UINT32>(pIFEInput->resolution.width);
                    pSensorInfo->sensorResHeight    = static_cast<UINT32>(pIFEInput->resolution.height);
                    activeWindow.top                = static_cast<INT32>(pIFEInput->CAMIFWindow.top);
                    activeWindow.left               = static_cast<INT32>(pIFEInput->CAMIFWindow.left);
                    activeWindow.width              = static_cast<INT32>(pIFEInput->CAMIFWindow.width);
                    activeWindow.height             = static_cast<INT32>(pIFEInput->CAMIFWindow.height);
                }
                else
                {
                    pSensorInfo->sensorResWidth     = static_cast<UINT32>(pSensorMode->resolution.outputWidth);
                    pSensorInfo->sensorResHeight    = static_cast<UINT32>(pSensorMode->resolution.outputHeight);
                    activeWindow.top                = static_cast<INT32>(pSensorMode->cropInfo.firstLine);
                    activeWindow.left               = static_cast<INT32>(pSensorMode->cropInfo.firstPixel);
                    activeWindow.width              = static_cast<INT32>(pSensorMode->cropInfo.lastPixel -
                        pSensorMode->cropInfo.firstPixel + 1);
                    activeWindow.height             = static_cast<INT32>(pSensorMode->cropInfo.lastLine -
                        pSensorMode->cropInfo.firstLine + 1);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupAEC, "Failed to get the sensor mode!");
            }
        }
    }

    pStatsCapabilities->activePixelWindow           = activeWindow;
    pStatsCapabilities->maxBitDepth                 = BGStatsMaximumBitWidth;
    // Currently none of the additional HW capabilities are supported
    pStatsCapabilities->HWStatsCapabilities         = 0;
    pStatsCapabilities->maxHWStatsSkippingFactor    = 0;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::GetPeerInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::GetPeerInfo(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    VOID** ppPeerInfo
    ) const
{
    CamxResult  result          = CamxResultSuccess;
    BOOL        needSync        = pStatsProcessRequestDataInfo->peerSyncInfo.needSync;
    INT64       requestDelta    = pStatsProcessRequestDataInfo->peerSyncInfo.requestDelta;
    UINT        peerPipelineId  = pStatsProcessRequestDataInfo->peerSyncInfo.peerPipelineID;

    if (TRUE == needSync)
    {
        // Get property from peer pipeline
        static const UINT GetProps[]              = { PropertyIDAECPeerInfo };
        static const UINT GetPropsLength          = CAMX_ARRAY_SIZE(GetProps);
        VOID*             pData[GetPropsLength]   = { 0 };
        UINT64            offsets[GetPropsLength] = { static_cast<UINT64>(abs(requestDelta)) };
        BOOL              negates[GetPropsLength] = { (requestDelta < 0) ? TRUE : FALSE };

        result = m_pNode->GetDataListFromPipeline(GetProps, static_cast<VOID** const>(pData),
            offsets, GetPropsLength, negates, peerPipelineId);

        if (NULL != pData[0])
        {
            *ppPeerInfo = *(reinterpret_cast<VOID**>(pData[0]));
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "MultiCamera Pdata: PeerInfo:%010p peer_pipeline_id:%d "
                "ReqId:%04llu CamId:%d negate:%d delta:%d algoAction:%d isMultiRequest:%d",
                *ppPeerInfo, peerPipelineId, m_currProcessingRequestId,
                m_cameraId, negates[0], requestDelta,
                pStatsProcessRequestDataInfo->algoAction,
                pStatsProcessRequestDataInfo->pMultiRequestSync->currReq.isMultiRequest);
        }
        else
        {
            *ppPeerInfo = NULL;
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "MultiCamera Pdata:NULL PeerInfo:%010p peer_pipeline_id:%d "
                "ReqId:%04llu CamId:%d negate:%d delta:%d algoAction:%d isMultiRequest:%d",
                *ppPeerInfo, peerPipelineId, m_currProcessingRequestId,
                m_cameraId, negates[0], requestDelta,
                pStatsProcessRequestDataInfo->algoAction,
                pStatsProcessRequestDataInfo->pMultiRequestSync->currReq.isMultiRequest);
        }
    }
    else
    {
        *ppPeerInfo = NULL;
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "No need to sync for Req:%llu on pipeline:%d",
                         m_currProcessingRequestId, m_pNode->GetPipelineId());
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::GetCropWindow
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::GetCropWindow(
    StatsRectangle* pCropWindow
    ) const
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);

    CropWindow            HALCropWindow           = { 0 };
    CamxResult            result                  = CamxResultSuccess;
    FLOAT                 widthRatio              = static_cast<FLOAT>(m_hardwareInfo.sensorInfo.sensorResWidth) /
                                                                      (m_hardwareInfo.sensorInfo.sensorActiveResWidth);
    FLOAT                 heightRatio             = static_cast<FLOAT>(m_hardwareInfo.sensorInfo.sensorResHeight) /
                                                                      (m_hardwareInfo.sensorInfo.sensorActiveResHeight);
    if (NULL == pCropWindow)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "pCropWindow pointer is NULL");
        return CamxResultEInvalidPointer;
    }

    if (NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputScalerCropRegion))
    {
        HALCropWindow = (*reinterpret_cast<CropWindow*>(m_pStatsAECPropertyReadWrite->GetPropertyData(
            AECReadTypeInputScalerCropRegion)));
    }

    if (HALCropWindow.width > static_cast<INT>(m_hardwareInfo.sensorInfo.sensorActiveResWidth))
    {
        CAMX_LOG_WARN(CamxLogGroupAEC, "Wrong input: HALCropWindow width(%d) > Active Array width(%d)",
            HALCropWindow.width, m_hardwareInfo.sensorInfo.sensorActiveResWidth);
        HALCropWindow.width = m_hardwareInfo.sensorInfo.sensorActiveResWidth;
    }
    if (HALCropWindow.height > static_cast<INT>(m_hardwareInfo.sensorInfo.sensorActiveResHeight))
    {
        CAMX_LOG_WARN(CamxLogGroupAEC, "Wrong input: HALCropWindow height(%d) > Active Array height(%d)",
            HALCropWindow.height, m_hardwareInfo.sensorInfo.sensorActiveResHeight);
        HALCropWindow.height = m_hardwareInfo.sensorInfo.sensorActiveResHeight;
    }

    if (HALCropWindow.width  != 0 &&
        HALCropWindow.height != 0)
    {
        // mapping crop window to CAMIF size from Sensor Active pixel size
        pCropWindow->width  = Utils::RoundFLOAT(HALCropWindow.width * widthRatio);
        pCropWindow->height = Utils::RoundFLOAT(HALCropWindow.height * heightRatio);
        pCropWindow->left   = Utils::RoundFLOAT(HALCropWindow.left * widthRatio);
        pCropWindow->top    = Utils::RoundFLOAT(HALCropWindow.top * heightRatio);
    }
    else
    {
        // If the crop window has not been set, set it to the entire sensor crop window
        (*pCropWindow) = m_hardwareInfo.statsCapabilities.activePixelWindow;
        CAMX_LOG_INFO(CamxLogGroupAEC, "Wrong input: HALCropWindow width,height = %dx%d",
            HALCropWindow.width, HALCropWindow.height);
    }

    // Boundary check for Width
    if (m_hardwareInfo.sensorInfo.sensorResWidth < static_cast<UINT32>(pCropWindow->width + pCropWindow->left))
    {
        pCropWindow->width = m_hardwareInfo.sensorInfo.sensorResWidth - pCropWindow->left;
    }

    // Boundary check for Height
    if (m_hardwareInfo.sensorInfo.sensorResHeight < static_cast<UINT32>(pCropWindow->height + pCropWindow->top))
    {
        pCropWindow->height = m_hardwareInfo.sensorInfo.sensorResHeight - pCropWindow->top;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupAEC,
        "Stats crop window:CamId:%d HALCrop %d %d %d %d, AdjustCrop %d %d %d %d, SensorRes %d %d, "
        "ActiveArray %d %d, w h ratio %f %f",
        m_cameraId,
        HALCropWindow.left,
        HALCropWindow.top,
        HALCropWindow.width,
        HALCropWindow.height,
        pCropWindow->left,
        pCropWindow->top,
        pCropWindow->width,
        pCropWindow->height,
        m_hardwareInfo.sensorInfo.sensorResWidth,
        m_hardwareInfo.sensorInfo.sensorResHeight,
        m_hardwareInfo.sensorInfo.sensorActiveResWidth,
        m_hardwareInfo.sensorInfo.sensorActiveResHeight,
        widthRatio,
        heightRatio);

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::UpdateTraceEvents
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECStatsProcessor::UpdateTraceEvents(
    AECEngineProcessStatsOutput* pProcessStatsOutput)
{
    // Convergence starts always for first frame or another other stream resets
    if (AECSettleUnknown == m_lastSettledState)
    {
        CAMX_TRACE_ASYNC_BEGIN_F(CamxLogGroupAEC, 0, "AEC: Convergence");
        m_lastSettledState = AECSettleFalse;
    }
    else
    {
        if (m_lastSettledState != pProcessStatsOutput->algorithmOutput.frameInfo.aecSettled)
        {
            if (FALSE == pProcessStatsOutput->algorithmOutput.frameInfo.aecSettled)
            {
                CAMX_TRACE_ASYNC_BEGIN_F(CamxLogGroupAEC, 0, "AEC: Convergence");
                m_lastSettledState = AECSettleFalse;
            }
            else
            {
                CAMX_TRACE_ASYNC_END_F(CamxLogGroupAEC, 0, "AEC: Convergence");
                m_lastSettledState = AECSettled;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::SetStatsConfigFromAlgoConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECStatsProcessor::SetStatsConfigFromAlgoConfig(
    StatsBayerGridBayerExposureConfig*  pAlgoStatsConfig,
    StatsBayerHistogramConfig*          pAlgoBHISTStatsConfig,
    AECConfig*                          pStatsConfig
    ) const
{
    if (NULL == pAlgoStatsConfig || NULL == pStatsConfig)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "pAlgoStatsConfig: %p or pStatsConfig: %p pointer is NULL",
            pAlgoStatsConfig, pStatsConfig);
        return;
    }

    StatsRectangle statsROI = { 0 };

    // BG stats configuration
    pStatsConfig->BGConfig.channelGainThreshold[ChannelIndexGR] = pAlgoStatsConfig->channelGainThreshold[StatsColorChannelGR];
    pStatsConfig->BGConfig.channelGainThreshold[ChannelIndexGB] = pAlgoStatsConfig->channelGainThreshold[StatsColorChannelGB];
    pStatsConfig->BGConfig.channelGainThreshold[ChannelIndexR]  = pAlgoStatsConfig->channelGainThreshold[StatsColorChannelR];
    pStatsConfig->BGConfig.channelGainThreshold[ChannelIndexB]  = pAlgoStatsConfig->channelGainThreshold[StatsColorChannelB];
    pStatsConfig->BGConfig.horizontalNum                        = pAlgoStatsConfig->horizontalRegionCount;
    pStatsConfig->BGConfig.verticalNum                          = pAlgoStatsConfig->verticalRegionCount;
    pStatsConfig->BGConfig.outputBitDepth                       = pAlgoStatsConfig->outputBitDepth;
    CAMX_STATIC_ASSERT( sizeof(pAlgoStatsConfig->YStatsWeights) == sizeof(pStatsConfig->BGConfig.YStatsWeights));
    CAMX_ASSERT_MESSAGE(MaxAWBBGStatsNum >= (pStatsConfig->BGConfig.horizontalNum * pStatsConfig->BGConfig.verticalNum),
                        "AWBBGStatsNum out of bound");
    for (UINT32 i = 0; i < CAMX_ARRAY_SIZE(pAlgoStatsConfig->YStatsWeights); i++)
    {
        pStatsConfig->BGConfig.YStatsWeights[i] = pAlgoStatsConfig->YStatsWeights[i];
    }

    // BE stats configuration
    pStatsConfig->BEConfig.channelGainThreshold[ChannelIndexGR] = pAlgoStatsConfig->channelGainThreshold[StatsColorChannelGR];
    pStatsConfig->BEConfig.channelGainThreshold[ChannelIndexGB] = pAlgoStatsConfig->channelGainThreshold[StatsColorChannelGB];
    pStatsConfig->BEConfig.channelGainThreshold[ChannelIndexR]  = pAlgoStatsConfig->channelGainThreshold[StatsColorChannelR];
    pStatsConfig->BEConfig.channelGainThreshold[ChannelIndexB]  = pAlgoStatsConfig->channelGainThreshold[StatsColorChannelB];
    pStatsConfig->BEConfig.horizontalNum                        = pAlgoStatsConfig->horizontalRegionCount;
    pStatsConfig->BEConfig.verticalNum                          = pAlgoStatsConfig->verticalRegionCount;
    pStatsConfig->BEConfig.outputBitDepth                       = pAlgoStatsConfig->outputBitDepth;
    CAMX_STATIC_ASSERT( sizeof(pAlgoStatsConfig->YStatsWeights) == sizeof(pStatsConfig->BEConfig.YStatsWeights));
    for (UINT32 i = 0; i < CAMX_ARRAY_SIZE(pAlgoStatsConfig->YStatsWeights); i++)
    {
        pStatsConfig->BEConfig.YStatsWeights[i] = pAlgoStatsConfig->YStatsWeights[i];
    }

    if (pAlgoStatsConfig->enableSaturationStats)
    {
        pStatsConfig->BGConfig.outputMode                       = BGBESaturationEnabled;
        pStatsConfig->BEConfig.outputMode                       = BGBESaturationEnabled;
    }
    else if (pAlgoStatsConfig->enableYStatsComputation)
    {
        pStatsConfig->BGConfig.outputMode                       = BGBEYStatsEnabled;
        pStatsConfig->BGConfig.greenType                        = GAverage;
        pStatsConfig->BEConfig.outputMode                       = BGBEYStatsEnabled;
        pStatsConfig->BEConfig.greenType                        = GAverage;
    }
    else
    {
        pStatsConfig->BGConfig.outputMode                       = BGBERegular;
        pStatsConfig->BEConfig.outputMode                       = BGBERegular;
    }

    // BHist stats configuration
    pStatsConfig->BHistConfig.channel                           = pAlgoBHISTStatsConfig->channel;
    pStatsConfig->BHistConfig.uniform                           = pAlgoBHISTStatsConfig->uniform;
    pStatsConfig->HDRBHistConfig.greenChannelInput              = HDRBHistSelectGR;

    // Configure the ROI selection
    switch (pAlgoStatsConfig->ROISelection)
    {
        case StatsROISelectionCustom:
            statsROI = pAlgoStatsConfig->ROI;
            break;

        case StatsROISelectionCroppedFOV:
        {
            GetCropWindow(&statsROI);
            UpdateWindowForStabilization(&statsROI);
            break;
        }

        case StatsROISelectionFullFOV:
        default:
            statsROI = m_hardwareInfo.statsCapabilities.activePixelWindow;
            break;
    }

    // Set the stats ROI
    pStatsConfig->BGConfig.ROI.left         = statsROI.left;
    pStatsConfig->BGConfig.ROI.top          = statsROI.top;
    pStatsConfig->BGConfig.ROI.width        = statsROI.width;
    pStatsConfig->BGConfig.ROI.height       = statsROI.height;

    pStatsConfig->BEConfig.ROI.left         = statsROI.left;
    pStatsConfig->BEConfig.ROI.top          = statsROI.top;
    pStatsConfig->BEConfig.ROI.width        = statsROI.width;
    pStatsConfig->BEConfig.ROI.height       = statsROI.height;

    // Update ROI if Algo gave custom BHIST
    if (pAlgoBHISTStatsConfig->ROISelection == StatsROISelectionCustom)
    {
        statsROI = pAlgoBHISTStatsConfig->ROI;
    }
    pStatsConfig->BHistConfig.ROI.left      = statsROI.left;
    pStatsConfig->BHistConfig.ROI.top       = statsROI.top;
    pStatsConfig->BHistConfig.ROI.width     = statsROI.width;
    pStatsConfig->BHistConfig.ROI.height    = statsROI.height;

    pStatsConfig->HDRBHistConfig.ROI.left   = statsROI.left;
    pStatsConfig->HDRBHistConfig.ROI.top    = statsROI.top;
    pStatsConfig->HDRBHistConfig.ROI.width  = statsROI.width;
    pStatsConfig->HDRBHistConfig.ROI.height = statsROI.height;

    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "BG: hnum: %d vnum: %d, BE: hnum: %d, vnum: %d "
        "ROISel:%d "
        "ROI (%d %d %d %d)"
        "Ywt (%f %f %f)"
        "CGTh (l:%d t:%d w:%d h:%d)"
        "BHIST: chnl:%d uniform:%d ROISel:%d ROI(l:%d t:%d w:%d h:%d)",
        pStatsConfig->BGConfig.horizontalNum, pStatsConfig->BGConfig.verticalNum,
        pStatsConfig->BEConfig.horizontalNum, pStatsConfig->BEConfig.verticalNum,
        pAlgoStatsConfig->ROISelection,
        pStatsConfig->BGConfig.ROI.left, pStatsConfig->BGConfig.ROI.top,
        pStatsConfig->BGConfig.ROI.width, pStatsConfig->BGConfig.ROI.height,
        pStatsConfig->BEConfig.YStatsWeights[0],
        pStatsConfig->BEConfig.YStatsWeights[1],
        pStatsConfig->BEConfig.YStatsWeights[2],
        pStatsConfig->BGConfig.channelGainThreshold[ChannelIndexGR],
        pStatsConfig->BGConfig.channelGainThreshold[ChannelIndexGB],
        pStatsConfig->BGConfig.channelGainThreshold[ChannelIndexR],
        pStatsConfig->BGConfig.channelGainThreshold[ChannelIndexB],
        pStatsConfig->BHistConfig.channel,
        pStatsConfig->BHistConfig.uniform,
        pAlgoBHISTStatsConfig->ROISelection,
        pStatsConfig->HDRBHistConfig.ROI.left, pStatsConfig->HDRBHistConfig.ROI.top,
        pStatsConfig->HDRBHistConfig.ROI.width, pStatsConfig->HDRBHistConfig.ROI.height);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::SetTouchROISettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECStatsProcessor::SetTouchROISettings(
    AECEngineHALParam* pHALParam,
    WeightedRectangle* pTouchROIInfo
    ) const
{
    // Taking local variable to save multiple derefrence and type cast.
    FLOAT          xMax         = static_cast<FLOAT>(pTouchROIInfo->xMax);
    FLOAT          yMax         = static_cast<FLOAT>(pTouchROIInfo->yMax);
    FLOAT          xMin         = static_cast<FLOAT>(pTouchROIInfo->xMin);
    FLOAT          yMin         = static_cast<FLOAT>(pTouchROIInfo->yMin);
    FLOAT          activeWidth  = static_cast<FLOAT>(m_hardwareInfo.sensorInfo.sensorActiveResWidth);
    FLOAT          activeHeight = static_cast<FLOAT>(m_hardwareInfo.sensorInfo.sensorActiveResHeight);
    FLOAT          resWidth     = static_cast<FLOAT>(m_hardwareInfo.sensorInfo.sensorResWidth);
    FLOAT          resHeight    = static_cast<FLOAT>(m_hardwareInfo.sensorInfo.sensorResHeight);
    StatsRectangle statsROI     = { 0 };
    AECAlgoROI     touchROI;

    if (0 != pTouchROIInfo->weight)
    {
        GetCropWindow(&statsROI);

        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Input CamId:%d Max(x:%d y:%d) Min(x:%d y%d)",
            m_cameraId,
            pTouchROIInfo->xMax,
            pTouchROIInfo->yMax,
            pTouchROIInfo->xMin,
            pTouchROIInfo->yMin);


        // Convert input AE regions to CAMIF size from Sensor Active pixel size
        touchROI.x  = xMin * resWidth / activeWidth;
        touchROI.y  = yMin * resHeight / activeHeight;
        touchROI.dx = (xMax - xMin) * resWidth / activeWidth;
        touchROI.dy = (yMax - yMin) * resHeight / activeHeight;

        // Input Touch region can be 0 and less then statsROI.left
        // tested using cts.CaptureRequestTest#testDigitalZoom
        // tested using cts.CaptureRequestTest#testZoomRatio
        if (Utils::RoundFLOAT(touchROI.x) >= static_cast<INT32>(statsROI.left))
        {
            touchROI.x -= statsROI.left;
        }
        if (Utils::RoundFLOAT(touchROI.y) >= static_cast<INT32>(statsROI.top))
        {
            touchROI.y -= statsROI.top;
        }
        if (Utils::RoundFLOAT(touchROI.dx + touchROI.x) > static_cast<INT32>(statsROI.width))
        {
            touchROI.dx = statsROI.width - touchROI.x;
        }
        if (Utils::RoundFLOAT(touchROI.dy + touchROI.y) > static_cast<INT32>(statsROI.height))
        {
            touchROI.dy = statsROI.height - touchROI.y;
        }

        // Calculate ratio of AE region to Crop window and send to Algo
        pHALParam->touchROI.x  = touchROI.x / statsROI.width;
        pHALParam->touchROI.y  = touchROI.y / statsROI.height;
        pHALParam->touchROI.dx = touchROI.dx / statsROI.width;
        pHALParam->touchROI.dy = touchROI.dy / statsROI.height;
        pHALParam->touchROI.weight = static_cast<FLOAT>(pTouchROIInfo->weight);

        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Output x:%f y:%f dx:%f dy %f weight:%f",
            pHALParam->touchROI.x,
            pHALParam->touchROI.y,
            pHALParam->touchROI.dx,
            pHALParam->touchROI.dy,
            pHALParam->touchROI.weight);
    }
    else
    {
        pHALParam->touchROI.weight = 0.0f;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::SetFaceROISettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECStatsProcessor::SetFaceROISettings(
    AECEngineHALParam*  pHALParam,
    FaceROIInformation* pFaceROIInfo
    ) const
{
    pHALParam->faceROI = *pFaceROIInfo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::SetTrackerROISettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECStatsProcessor::SetTrackerROISettings(
    AECEngineHALParam*  pHALParam,
    TrackerROIInformation* pTrackerROIInfo
    ) const
{
    pHALParam->trackerROI = *pTrackerROIInfo;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::SetBestShotModeSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::SetBestShotModeSettings(
    AECEngineHALParam* pHALParam
    ) const
{
    CamxResult result = CamxResultSuccess;
    UINT32     bestShotmode;

    if (NULL != pHALParam)
    {
        bestShotmode = pHALParam->controlSceneMode;
        switch (bestShotmode)
        {
            case ControlSceneModeAction:
            case ControlSceneModeSteadyphoto:
            case ControlSceneModeSports:
                pHALParam->AEMeteringMode                 = AECAlgoMeteringModeCenterWeighted;          // Center_Weighted
                pHALParam->AECompensation                 = AECompensation0;
                pHALParam->ISOExposureTimePriortyMode     = ISOExposureTimePriorityMode::ISOPriority;
                pHALParam->ISOorExposureTimePriorityValue = static_cast<INT64>(ISOMode::ISOMode400);
                pHALParam->sensitivity                    = AESensitivity_Sports;   // High Gain to achieve Low Exposure time
                break;
            case ControlSceneModeLandscape:
                pHALParam->AEMeteringMode = AECAlgoMeteringModeFrameAverage;                            // Frame_Average
                pHALParam->AECompensation = AECompensation0;
                break;
            case ControlSceneModeBeach:
            case ControlSceneModeSnow:
                pHALParam->AEMeteringMode = AECAlgoMeteringModeFrameAverage;                            // Frame_Average
                pHALParam->AECompensation = AECompensation_Beach_Snow;
                break;
            case ControlSceneModeSunset:
                pHALParam->AEMeteringMode = AECAlgoMeteringModeFrameAverage;                            // Frame_Average
                pHALParam->AECompensation = AECompensation_Sunset_CandleLight;
                break;
            case ControlSceneModeCandlelight:
                pHALParam->AEMeteringMode = AECAlgoMeteringModeCenterWeighted;                          // Center_Weighted
                pHALParam->AECompensation = AECompensation_Sunset_CandleLight;
                break;
            case ControlSceneModePortrait:
            case ControlSceneModeNight:
            case ControlSceneModeNightPortrait:
            case ControlSceneModeTheatre:
            case ControlSceneModeFireworks:
            case ControlSceneModeParty:
                pHALParam->AEMeteringMode = AECAlgoMeteringModeCenterWeighted;                          // Center_Weighted
                pHALParam->AECompensation = AECompensation0;
                break;
            case ControlSceneModeDisabled:
            default:
                break;
        }
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Scene Mode: %d AE Metering Mode:%d AE Compensation:%d Sensitivity:%d",
                         bestShotmode, pHALParam->AEMeteringMode, pHALParam->AECompensation, pHALParam->sensitivity);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "Input HAL Param is NULL");
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::GetFlashInfoType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AECFlashInfoType CAECStatsProcessor::GetFlashInfoType(
    AECAlgoFlashStateType flashState
    ) const
{
    AECFlashInfoType flashInfo = FlashInfoTypeOff;
    switch (flashState)
    {
        case AECAlgoFlashStatePre:
            flashInfo = FlashInfoTypePre;
            break;
        case AECAlgoFlashStateMain:
            flashInfo = FlashInfoTypeMain;
            break;
        case AECAlgoFlashStateCalibaration:
            flashInfo = FlashInfoTypeCalibration;
            break;
        case AECAlgoFlashStateOff:
            flashInfo = FlashInfoTypeOff;
            break;
        default:
            break;
    }
    return flashInfo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::SetISOExpPriorityValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECStatsProcessor::SetISOExpPriorityValue(
    AECEngineHALParam* pHALParam
    ) const
{
    ISOExposureTimePriorityMode priorityMode =
                    static_cast<ISOExposureTimePriorityMode>(pHALParam->ISOExposureTimePriortyMode);

    switch (priorityMode)
    {
        case ISOExposureTimePriorityMode::ISOPriority:
        {
            ISOMode isoMode = static_cast<ISOMode>(pHALParam->ISOorExposureTimePriorityValue);

            switch (isoMode)
            {
                case ISOMode::ISOModeAuto:
                    break;
                case ISOMode::ISOModeDeblur:
                    break;
                case ISOMode::ISOMode100:
                    pHALParam->sensitivity = 100;
                    break;
                case ISOMode::ISOMode200:
                    pHALParam->sensitivity = 200;
                    break;
                case ISOMode::ISOMode400:
                    pHALParam->sensitivity = 400;
                    break;
                case ISOMode::ISOMode800:
                    pHALParam->sensitivity = 800;
                    break;
                case ISOMode::ISOMode1600:
                    pHALParam->sensitivity = 1600;
                    break;
                case ISOMode::ISOMode3200:
                    pHALParam->sensitivity = 3200;
                    break;
                case ISOMode::ISOModeAbsolute:
                    pHALParam->sensitivity = pHALParam->ISOValue;
                    break;
                default:
                    CAMX_LOG_ERROR(CamxLogGroupAEC, "Unsupported ISO mode %d", isoMode);
                    break;
            }
            pHALParam->gain = 0;
            break;
        }
        case ISOExposureTimePriorityMode::ExposureTimePriority:
        {
            pHALParam->exposureTime = pHALParam->ISOorExposureTimePriorityValue;
            pHALParam->gain         = 0;
            break;
        }
        case ISOExposureTimePriorityMode::GainPriority:
            // Do Nothing Already saved in pHALParam->gain
        case ISOExposureTimePriorityMode::DisablePriority:
            break;
        default:
        {
            CAMX_LOG_ERROR(CamxLogGroupAEC, "Unsupported priority mode %d", priorityMode);
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::ReadAFDMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::ReadAFDMode(
    AECEngineHALParam* pHALParam
    ) const
{
    CamxResult        result                  = CamxResultSuccess;
    AFDFrameInfo*     pAFDFrameInfo           = { 0 };

    pHALParam->flickerMode                    = pHALParam->AEAntibandingModeValue;

    if (ControlAEAntibandingModeAuto  == pHALParam->AEAntibandingModeValue  ||
        ControlAEAntibandingAuto_50HZ == pHALParam->AEAntibandingModeValue  ||
        ControlAEAntibandingAuto_60HZ == pHALParam->AEAntibandingModeValue)
    {
        if (NULL != m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypePropertyIDAFDFrameInfo))
        {
            pAFDFrameInfo =
                static_cast<AFDFrameInfo*>(m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypePropertyIDAFDFrameInfo));

            pHALParam->flickerMode = static_cast<ControlAEAntibandingModeValues>(pAFDFrameInfo->detectedAFDMode);

            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "AFD mode at AEC side %d", pHALParam->flickerMode);
        }
    }
    else
    {
        // Nothing to do here.
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECStatsProcessor::ReadStatsNodesUpdates
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECStatsProcessor::ReadStatsNodesUpdates(
    AECEngineNodesUpdate* pAECNodesUpdate)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);

    if (NULL == pAECNodesUpdate)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "pAECNodesUpdate pointer is NULL");
        return;
    }

    static const UINT StatsData[] =
    {
        PropertyIDAFFrameInfo,
        PropertyIDAWBInternal,
        PropertyIDAWBFrameInfo,
        InputControlAWBMode,
        PropertyIDRERCompleted
    };

    static const UINT StatsLength = CAMX_ARRAY_SIZE(StatsData);
    VOID*             pData[StatsLength] = { 0 };
    pData[0] = m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypePropertyIDAFFrameInfo);
    pData[1] = m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypePropertyIDAWBInternal);
    pData[2] = m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypePropertyIDAWBFrameInfo);
    pData[3] = m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputControlAWBMode);
    pData[4] = m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypePropertyIDRERCompleted);

    if (NULL != pData[0])
    {
        AFFrameInformation* pAfFrameInfo = reinterpret_cast<AFFrameInformation*>(pData[0]);
        pAECNodesUpdate->pAFOutput->status.status = pAfFrameInfo->focusStatus.status;
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "AF state received: %d", pAfFrameInfo->focusStatus.status);
    }

    if (NULL != pData[1])
    {
        AWBOutputInternal* pAWBOutputInternal = reinterpret_cast<AWBOutputInternal*>(pData[1]);
        pAECNodesUpdate->pAWBOutputInternal->flashEstimationStatus =
            pAWBOutputInternal->flashEstimationStatus;
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "AWB Flash Estimation state Received: %d",
            pAECNodesUpdate->pAWBOutputInternal->flashEstimationStatus);
    }

    if (NULL != pData[2])
    {
        *pAECNodesUpdate->pAWBFrameInfo = *reinterpret_cast<AWBFrameInfo*>(pData[2]);
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "AWB Frame Info: R,G,B gains: %f, %f, %f",
            pAECNodesUpdate->pAWBFrameInfo->AWBGains.rGain,
            pAECNodesUpdate->pAWBFrameInfo->AWBGains.gGain,
            pAECNodesUpdate->pAWBFrameInfo->AWBGains.bGain);
    }

    if (NULL != pData[3])
    {
        ControlAWBModeValues AWBMode = *reinterpret_cast<ControlAWBModeValues*>(pData[3]);
        pAECNodesUpdate->isAWBModeAuto = (AWBMode == ControlAWBModeAuto);
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "AWB Control Mode: Auto %d",
            pAECNodesUpdate->isAWBModeAuto);
    }

    if (NULL != pData[4])
    {
        pAECNodesUpdate->isRERDone = *reinterpret_cast<BOOL*>(pData[4]);
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "RER completion Status: %d", pAECNodesUpdate->isRERDone);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECStatsProcessor::GetPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::GetPublishList(
    const UINT32    maxTagArraySize,
    UINT32*         pTagArray,
    UINT32*         pTagCount,
    UINT32*         pPartialTagCount)
{
    CamxResult result           = CamxResultSuccess;
    UINT32     tagCount         = 0;
    UINT32     numMetadataTags  = CAMX_ARRAY_SIZE(AECOutputMetadataTags);
    UINT32     numVendorTags    = CAMX_ARRAY_SIZE(AECOutputVendorTags);
    UINT32     tagID;

    if (numMetadataTags + numVendorTags + m_vendorTagInfoOutputputList.vendorTagCount <= maxTagArraySize)
    {
        for (UINT32 tagIndex = 0; tagIndex < numMetadataTags; ++tagIndex)
        {
            pTagArray[tagCount++] = AECOutputMetadataTags[tagIndex];
        }

        for (UINT32 tagIndex = 0; tagIndex < m_vendorTagInfoOutputputList.vendorTagCount; ++tagIndex)
        {
            pTagArray[tagCount++] = m_vendorTagInfoOutputputList.vendorTag[tagIndex].vendorTagId;
        }

        for (UINT32 tagIndex = 0; tagIndex < numVendorTags; ++tagIndex)
        {
            result = VendorTagManager::QueryVendorTagLocation(
                AECOutputVendorTags[tagIndex].pSectionName,
                AECOutputVendorTags[tagIndex].pTagName,
                &tagID);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupAEC, "Cannot query vendor tag %s %s",
                    AECOutputVendorTags[tagIndex].pSectionName,
                    AECOutputVendorTags[tagIndex].pTagName);
                continue;
            }
            pTagArray[tagCount++] = tagID;
        }
    }
    else
    {
        result = CamxResultEOutOfBounds;
        CAMX_LOG_ERROR(CamxLogGroupAEC, "ERROR cannot publish tag count (%d %d %d)",
            m_vendorTagInfoOutputputList.vendorTagCount,
            numVendorTags,
            numMetadataTags);
    }

    if (CamxResultSuccess == result)
    {
        *pTagCount        = tagCount;
        *pPartialTagCount = NumberOfPartialMetadataTags;

        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "%d tags will be published. tag count (%d %d %d)",
            tagCount,
            m_vendorTagInfoOutputputList.vendorTagCount,
            numVendorTags,
            numMetadataTags);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxResult  CAECStatsProcessor::GetGyroDataFromNCS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::GetGyroDataFromNCS(
    AECCommandInputParam* pInput)
{
    CamxResult result = CamxResultEFailed;

    /// Get gyro data from NCS interface
    if (NULL != pInput)
    {
        result = m_AECGyro.PopulateGyroData(pInput, m_aeHALData.currentFPSRange.min);
        if (result == CamxResultEFailed)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Failed to populate Gyro data");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "AECCommandInputParam* pInput is NULL");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxResult CAECGyro::Initialize()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECGyro::Initialize()
{
    CamxResult result = CamxResultSuccess;

    result = SetupNCSLink();

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupAEC, "CAECGyro: Failed setting up NCS link");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxResult CAECGyro::SetupNCSLink()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECGyro::SetupNCSLink()
{
    CamxResult result = CamxResultEFailed;
    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "CAECGyro: Setting up an NCS link");

    // Get the NCS service object handle
    m_pNCSServiceObject = reinterpret_cast<NCSService *>(HwEnvironment::GetInstance()->GetNCSObject());
    if (NULL != m_pNCSServiceObject)
    {
        result = SetupNCSLinkForSensor(NCSGyroType);
        if (result != CamxResultSuccess)
        {
            CAMX_LOG_WARN(CamxLogGroupAEC, "CAECGyro: Unable to Setup NCS Link For Gyro Sensor");
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupAEC, "CAECGyro: Unable to get NCS service object");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxResult CAECGyro::SetupNCSLinkForSensor();
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECGyro::SetupNCSLinkForSensor(
    NCSSensorType  sensorType)
{
    CamxResult      result = CamxResultEFailed;
    NCSSensorConfig sensorConfig = { 0 };
    NCSSensorCaps   sensorCaps;

    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "CAECGyro: Setting up an NCS link for sensor %d", sensorType);

    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

    result = m_pNCSServiceObject->QueryCapabilites(&sensorCaps, sensorType);
    if (result == CamxResultSuccess)
    {
        switch (sensorType)
        {
            case NCSGyroType:
                // Client's responsibility to set supported config
                sensorConfig.samplingRate  = pStaticSettings->gyroSensorSamplingRate;
                sensorConfig.reportRate    = pStaticSettings->gyroDataReportRate;
                sensorConfig.operationMode = 0;
                sensorConfig.sensorType    = sensorType;

                CAMX_LOG_VERBOSE(CamxLogGroupAEC, "CAECGyro: SR = %f. RR = %d sensorType: %d",
                    pStaticSettings->gyroSensorSamplingRate,
                    pStaticSettings->gyroDataReportRate,
                    sensorConfig.sensorType);

                m_pNCSSensorHandleGyro = m_pNCSServiceObject->RegisterService(sensorType, &sensorConfig);
                if (NULL == m_pNCSSensorHandleGyro)
                {
                    CAMX_LOG_ERROR(CamxLogGroupAEC, "CAECGyro: Unable to register %d with the NCS !!", sensorType);
                    result = CamxResultEFailed;
                }
                break;
            default:
                CAMX_ASSERT_ALWAYS_MESSAGE("CAECGyro: Unexpected sensor ID: %d", sensorType);
                break;
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupAEC, "CAECGyro: Unable to Query caps sensor type %d error %s", sensorType,
            Utils::CamxResultToString(result));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxResult CAECGyro::PopulateGyroData();
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECGyro::PopulateGyroData(
    AECCommandInputParam* pInput,
    INT32                 minFPS)
{
    CamxResult result = CamxResultEFailed;
    StatsGyroInfo* pGyroInfo = &(pInput->gyroInfo);

    if (NULL != m_pNCSSensorHandleGyro && NULL != m_pNCSSensorHandleGyro->GetSensorConfig())
    {
        NCSSensorData*   pDataObj        = NULL;
        NCSDataGyro*     pGyroData       = NULL;

        UINT16 sampleTimeinMilliSec      =
            static_cast<UINT16>(m_pNCSSensorHandleGyro->GetSensorConfig()->reportRate / 1000.0);

        // Setting gyro sampling time to be used in AEC algo
        pGyroInfo->samplingTimeMillisec = sampleTimeinMilliSec;

        UINT numberOfSamples = 0;
        if (minFPS == 0.0)
        {
            numberOfSamples = 1;
        }
        else
        {
            numberOfSamples = static_cast<UINT>((1000.0 / minFPS) / sampleTimeinMilliSec);
            if (numberOfSamples == 0)
            {
                numberOfSamples = 1;
            }
            else if (numberOfSamples > MaxAECGyroSampleSize)
            {
                numberOfSamples = MaxAECGyroSampleSize;
            }
        }
        // Number of gyro samples needed per frame is 1000/FPS/GyroSampleTimeinMilliSeconds
        pDataObj = m_pNCSSensorHandleGyro->GetLastNSamples(numberOfSamples);

        if (NULL != pDataObj)
        {
            UINT32 index;
            pGyroInfo->enabled = TRUE;
            pGyroInfo->sampleCount = static_cast<UINT16>(pDataObj->GetNumSamples());
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "CAECGyro: sampleCount %d ", pGyroInfo->sampleCount);

            for (index = 0; index < pGyroInfo->sampleCount && index < MaxAECGyroSampleSize; index++)
            {
                pDataObj->SetIndex(index);
                pGyroData = reinterpret_cast<NCSDataGyro*>(pDataObj->GetCurrent());

                if (NULL != pGyroData)
                {
                    pGyroInfo->pAngularVelocityX[index] = pGyroData->x;
                    pGyroInfo->pAngularVelocityY[index] = pGyroData->y;
                    pGyroInfo->pAngularVelocityZ[index] = pGyroData->z;
                    pGyroInfo->timeStamp[index] = pGyroData->timestamp;

                    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "CAECGyro: xyz timestamp  %f %f %f %llu",
                        pGyroInfo->pAngularVelocityX[index],
                        pGyroInfo->pAngularVelocityY[index],
                        pGyroInfo->pAngularVelocityZ[index],
                        pGyroInfo->timeStamp[index]);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupAEC, "CAECGyro: GetCurrrent returned null");
                    break;
                }
            }

            m_pNCSSensorHandleGyro->PutBackDataObj(pDataObj);

            if (index == pGyroInfo->sampleCount)
            {
                result = CamxResultSuccess;
            }
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupAEC, "CAECGyro: Gyro samples not available");
            pGyroInfo->enabled = FALSE;
            pGyroInfo->timeStamp[0] = 0;
            pGyroInfo->pAngularVelocityX[0] = 0.0f;
            pGyroInfo->pAngularVelocityY[0] = 0.0f;
            pGyroInfo->pAngularVelocityZ[0] = 0.0f;
            pGyroInfo->sampleCount = 1;
        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "CAECGyro: NCS Gyro handle is NULL");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECGyro::CAECGyro()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAECGyro::CAECGyro()
    : m_pNCSSensorHandleGyro(NULL)
    , m_pNCSServiceObject(NULL)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECGyro::~CAECGyro();
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAECGyro::~CAECGyro()
{
    if (NULL != m_pNCSSensorHandleGyro)
    {
        CamxResult result = m_pNCSServiceObject->UnregisterService(m_pNCSSensorHandleGyro);

        CAMX_ASSERT_MESSAGE((CamxResultSuccess == result),
                            "CAECGyro: Unable to unregister from the NCS %s", Utils::CamxResultToString(result));

        m_pNCSSensorHandleGyro = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECStatsProcessor::DumpAECStats()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECStatsProcessor::DumpAECStats(){
    UINT32 outputMask = HwEnvironment::GetInstance()->GetStaticSettings()->dumpAEC;

    if (outputMask & AECStatsControlDump)
    {
        CAMX_LOG_VERBOSE(
            CamxLogGroupAEC,
            "[AEC STATS] BG config: horizontalNum: %d, verticalNum: %d, ROI:(L:%d, T: %d, W: %d, H: %d)"
            " O/P bitDepth: %d, O/P mode: %d, GreenDataType: %d, Subgrid Region (h: %d, w: %d) "
            " CH_GainThr(GR: %d GB: %d R: %d B: %d)",
            m_statsControl.statsConfig.BGConfig.horizontalNum,
            m_statsControl.statsConfig.BGConfig.verticalNum,
            m_statsControl.statsConfig.BGConfig.ROI.left,
            m_statsControl.statsConfig.BGConfig.ROI.top,
            m_statsControl.statsConfig.BGConfig.ROI.width,
            m_statsControl.statsConfig.BGConfig.ROI.height,
            m_statsControl.statsConfig.BGConfig.outputBitDepth,
            m_statsControl.statsConfig.BGConfig.outputMode,
            m_statsControl.statsConfig.BGConfig.greenType,
            m_statsControl.statsConfig.BGConfig.regionHeight,
            m_statsControl.statsConfig.BGConfig.regionWidth,
            m_statsControl.statsConfig.BGConfig.channelGainThreshold[ChannelIndexGR],
            m_statsControl.statsConfig.BGConfig.channelGainThreshold[ChannelIndexGB],
            m_statsControl.statsConfig.BGConfig.channelGainThreshold[ChannelIndexR],
            m_statsControl.statsConfig.BGConfig.channelGainThreshold[ChannelIndexB]);

        CAMX_LOG_VERBOSE(
            CamxLogGroupAEC,
            "[AEC STATS] BE config: horizontalNum: %d, verticalNum: %d, ROI:(L:%d, T: %d, W: %d, H: %d)"
            " O/P bitDepth: %d, O/P mode: %d, GreenDataType: %d, Subgrid Region (h: %d, w: %d) "
            "Ywt (%f %f %f)",
            m_statsControl.statsConfig.BEConfig.horizontalNum,
            m_statsControl.statsConfig.BEConfig.verticalNum,
            m_statsControl.statsConfig.BEConfig.ROI.left,
            m_statsControl.statsConfig.BEConfig.ROI.top,
            m_statsControl.statsConfig.BEConfig.ROI.width,
            m_statsControl.statsConfig.BEConfig.ROI.height,
            m_statsControl.statsConfig.BEConfig.outputBitDepth,
            m_statsControl.statsConfig.BEConfig.outputMode,
            m_statsControl.statsConfig.BEConfig.greenType,
            m_statsControl.statsConfig.BEConfig.regionHeight,
            m_statsControl.statsConfig.BEConfig.regionWidth,
            m_statsControl.statsConfig.BEConfig.YStatsWeights[0],
            m_statsControl.statsConfig.BEConfig.YStatsWeights[1],
            m_statsControl.statsConfig.BEConfig.YStatsWeights[2]);

        CAMX_LOG_VERBOSE(
            CamxLogGroupAEC,
            "[AEC STATS] BHist config: ROI:(l:%d, t:%d, w:%d, h:%d), colorChannel: %d uniform: %d "
            "HDRBHist config: ROI:(l:%d, t:%d, w:%d, h:%d), Green Channel Selection: %d, "
            "input field selection: %d",
            m_statsControl.statsConfig.BHistConfig.ROI.left,
            m_statsControl.statsConfig.BHistConfig.ROI.top,
            m_statsControl.statsConfig.BHistConfig.ROI.width,
            m_statsControl.statsConfig.BHistConfig.ROI.height,
            m_statsControl.statsConfig.BHistConfig.channel,
            m_statsControl.statsConfig.BHistConfig.uniform,
            m_statsControl.statsConfig.HDRBHistConfig.ROI.left,
            m_statsControl.statsConfig.HDRBHistConfig.ROI.top,
            m_statsControl.statsConfig.HDRBHistConfig.ROI.width,
            m_statsControl.statsConfig.HDRBHistConfig.ROI.height,
            m_statsControl.statsConfig.HDRBHistConfig.greenChannelInput,
            m_statsControl.statsConfig.HDRBHistConfig.inputFieldSelect);

        CAMX_LOG_VERBOSE(
            CamxLogGroupAEC,
            "[AEC STATS] Tintless BE config: horizontalNum: %d, verticalNum: %d, ROI:(L:%d, T: %d, W: %d, H: %d)"
            " O/P bitDepth: %d, O/P mode: %d, GreenDataType: %d, Subgrid Region (h: %d, w: %d)",
            m_statsControl.statsConfig.TintlessBGConfig.horizontalNum,
            m_statsControl.statsConfig.TintlessBGConfig.verticalNum,
            m_statsControl.statsConfig.TintlessBGConfig.ROI.left,
            m_statsControl.statsConfig.TintlessBGConfig.ROI.top,
            m_statsControl.statsConfig.TintlessBGConfig.ROI.width,
            m_statsControl.statsConfig.TintlessBGConfig.ROI.height,
            m_statsControl.statsConfig.TintlessBGConfig.outputBitDepth,
            m_statsControl.statsConfig.TintlessBGConfig.outputMode,
            m_statsControl.statsConfig.TintlessBGConfig.greenType,
            m_statsControl.statsConfig.TintlessBGConfig.regionHeight,
            m_statsControl.statsConfig.TintlessBGConfig.regionWidth);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECStatsProcessor::DumpFrameInfo()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECStatsProcessor::DumpFrameInfo() {
    UINT32 outputMask = HwEnvironment::GetInstance()->GetStaticSettings()->dumpAEC;

    if (outputMask & AECFrameInfoDump)
    {
        CAMX_LOG_VERBOSE(
            CamxLogGroupAEC,
            "[AEC FRAME INFO] PrecapTrigger: %d, PreFlashState: %d, AECSettled: %d, ISOValue: %d, LEDAFRequired: %d, "
            "luxIndex: %f snapShotIndicator: %d, touchEVIndicator: %d, frameDuration: %lld, meteringMode: %d, "
            "exposureProgram: %d, brightnessValue: %f, sceneType: %d, exposureMode: %d, frameLuma: %f",
            m_frameInfo.AECPrecaptureTrigger,
            m_frameInfo.AECPreFlashState,
            m_frameInfo.AECSettled,
            m_frameInfo.ISOValue,
            m_frameInfo.luxIndex,
            m_frameInfo.snapshotIndicator,
            m_frameInfo.touchEVIndicator,
            m_frameInfo.frameDuration,
            m_frameInfo.meteringMode,
            m_frameInfo.exposureProgram,
            m_frameInfo.brightnessValue,
            m_frameInfo.sceneType,
            m_frameInfo.exposureMode,
            m_frameInfo.frameLuma);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECStatsProcessor::DumpHALData()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECStatsProcessor::DumpHALData() {
    UINT32 outputMask = HwEnvironment::GetInstance()->GetStaticSettings()->dumpAEC;

    if (outputMask & AECHALDump)
    {
        CAMX_LOG_VERBOSE(
            CamxLogGroupAEC,
            "[AEC HAL] aeCompensation: %d, aeLockFlag: %d, mode: %d, aeRegion [xMin: %d, yMin: %d, xMax: %d, "
            " yMax: %d, weight: %d]",
            m_aeHALData.aeCompensation,
            m_aeHALData.aeLockFlag,
            m_aeHALData.mode,
            m_aeHALData.aeRegion.xMin,
            m_aeHALData.aeRegion.yMin,
            m_aeHALData.aeRegion.xMax,
            m_aeHALData.aeRegion.yMax,
            m_aeHALData.aeRegion.weight);

        CAMX_LOG_VERBOSE(
            CamxLogGroupAEC,
            "[AEC HAL] aeState: %d, triggerValue: %d, controlMode: %d, exposure compensation step [num: %d, denom: %d] "
            "FPS range [min: %d, max: %d]",
            m_aeHALData.aeState,
            m_aeHALData.triggerValue,
            m_aeHALData.controlMode,
            m_aeHALData.exposureCompensationStep.numerator,
            m_aeHALData.exposureCompensationStep.denominator,
            m_aeHALData.currentFPSRange.min,
            m_aeHALData.currentFPSRange.max);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECStatsProcessor::OverrideInSensorHDR3ExpOutputMeta()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECStatsProcessor::OverrideInSensorHDR3ExpOutputMeta(
    AECFrameControl*    pFrameControlOut)
{
    if (TRUE == IsSeamlessInSensorHDR3ExpSnapshot())
    {
        pFrameControlOut->luxIndex                      = m_prevLuxIndex;
        pFrameControlOut->compenADRCGain                = m_compenADRCGain;
        pFrameControlOut->isInSensorHDR3ExpSnapshot     = m_isInSensorHDR3ExpSnapshot;
        pFrameControlOut->inSensorHDR3ExpTriggerOutput  = m_prevInSensorHDR3ExpTriggerInfo;
    }
    else
    {
        m_prevLuxIndex                   = pFrameControlOut->luxIndex;
        m_compenADRCGain                 = pFrameControlOut->compenADRCGain;
        m_isInSensorHDR3ExpSnapshot      = pFrameControlOut->isInSensorHDR3ExpSnapshot;
        m_prevInSensorHDR3ExpTriggerInfo = pFrameControlOut->inSensorHDR3ExpTriggerOutput;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Seamless in-sensor HDR 3 exposure snapshot: %u, change luxindex from %f to %f",
                     IsSeamlessInSensorHDR3ExpSnapshot(),
                     pFrameControlOut->luxIndex,
                     m_prevLuxIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECStatsProcessor::CanSkipAEProcessing()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::CanSkipAEProcessing(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo)
{
    BOOL    bSkip               = FALSE;
    INT32*  pCurrAECompensation = reinterpret_cast<INT32*>(
        m_pStatsAECPropertyReadWrite->GetPropertyData(AECReadTypeInputControlAEExposureCompensation));

    // we cannot skip algo if EV value is set
    if ((TRUE == pStatsProcessRequestDataInfo->skipProcessing) &&
        ((NULL == pCurrAECompensation) || (*pCurrAECompensation == m_HALParam.AECompensation)))
    {
        bSkip =  TRUE;
    }

    return bSkip;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECStatsProcessor::DetermineStabilizationMargins()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::DetermineStabilizationMargins()
{
    CamxResult              result = CamxResultSuccess;
    UINT32                  videoMetaTag        = 0;
    UINT32                  previewMetaTag      = 0;
    UINT32                  EISEnabledTag       = 0;
    UINT32                  minTotalMarginTag   = 0;
    ChiBufferDimension*     pVideoDimension     = NULL;
    ChiBufferDimension*     pPreviewDimension   = NULL;
    IFEOutputResolution*    pIFEResolution      = NULL;
    BOOL                    EISEnabled          = FALSE;
    MarginRequest*          pMinTotalMargin     = NULL;

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.streamDimension",
                                                      "video",
                                                      &videoMetaTag);
    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to get vendor tag: org.quic.camera.streamDimension.video");

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.streamDimension",
                                                      "preview",
                                                      &previewMetaTag);
    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to get vendor tag: org.quic.camera.streamDimension.preview");

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.eisrealtime",
                                                      "Enabled",
                                                      &EISEnabledTag);
    CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Fail to query: eisrealtime Enabled");

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.eisrealtime",
                                                      "MinimalTotalMargins",
                                                      &minTotalMarginTag);
    CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Fail to query: eisrealtime MinimalTotalMargins");

    const UINT marginTags[] =
    {
        videoMetaTag | UsecaseMetadataSectionMask,
        previewMetaTag | UsecaseMetadataSectionMask,
        PropertyIDUsecaseIFEOutputResolution,
        EISEnabledTag | UsecaseMetadataSectionMask,
        minTotalMarginTag | UsecaseMetadataSectionMask
    };

    const SIZE_T length         = CAMX_ARRAY_SIZE(marginTags);
    VOID*        pData[length]  = { 0 };
    UINT64       offset[length] = { 0, 0, m_pNode->GetMaximumPipelineDelay(), 0, 0 };

    result = m_pNode->GetDataList(marginTags, pData, offset, length);

    if (CamxResultSuccess == result)
    {
        if (NULL != pData[0])
        {
            pVideoDimension = static_cast<ChiBufferDimension*>(pData[0]);
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "videoResolution(%ux%u)", pVideoDimension->width, pVideoDimension->height);
        }

        if (NULL != pData[1])
        {
            pPreviewDimension = static_cast<ChiBufferDimension*>(pData[1]);
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "previewResolution(%ux%u)", pPreviewDimension->width, pPreviewDimension->height);
        }

        if (NULL != pData[2])
        {
            pIFEResolution = static_cast<IFEOutputResolution*>(pData[2]);
            CAMX_LOG_VERBOSE(CamxLogGroupAEC,
                        "IFE FullPath(Enabled: %d Dimension(%dx%d)) DisplayFullPath(Enabled: %d Dimension(%dx%d))",
                        pIFEResolution->fullPortEnable,
                        pIFEResolution->fullPortDimension.width,
                        pIFEResolution->fullPortDimension.height,
                        pIFEResolution->displayPortEnable,
                        pIFEResolution->displayPortDimension.width,
                        pIFEResolution->displayPortDimension.height);
        }

        if (NULL != pData[3])
        {
            EISEnabled = *static_cast<BOOL*>(pData[3]);
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "EISEnabled: %d", EISEnabled);
        }

        if (NULL != pData[4])
        {
            pMinTotalMargin = static_cast<MarginRequest*>(pData[4]);
            CAMX_LOG_VERBOSE(CamxLogGroupAEC,
                            "%s EIS Minimum Total Margin: (%f x %f)",
                            m_pNode->GetPipelineName(),
                            pMinTotalMargin->widthMargin, pMinTotalMargin->heightMargin);
        }
    }

    if ((TRUE == EISEnabled) && (NULL != pVideoDimension) && (NULL != pIFEResolution))
    {
        // calculate EIS margin
        if ((NULL != pPreviewDimension) && (pPreviewDimension->width > pVideoDimension->width) &&
            (pIFEResolution->fullPortDimension.width >= pPreviewDimension->width) &&
            (pIFEResolution->fullPortDimension.height >= pPreviewDimension->height))
        {
            m_stabilizationMargin.widthPixels = pIFEResolution->fullPortDimension.width - pPreviewDimension->width;
            m_stabilizationMargin.heightLines = pIFEResolution->fullPortDimension.height - pPreviewDimension->height;
        }
        else if ((pIFEResolution->fullPortDimension.width >= pVideoDimension->width) &&
                 (pIFEResolution->fullPortDimension.height >= pVideoDimension->height))
        {
            m_stabilizationMargin.widthPixels = pIFEResolution->fullPortDimension.width - pVideoDimension->width;
            m_stabilizationMargin.heightLines = pIFEResolution->fullPortDimension.height - pVideoDimension->height;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupAEC,
                    "%s: CameraId(%d) EIS margin(%ux%u)",
                    m_pNode->GetPipelineName(),
                    m_cameraId,
                    m_stabilizationMargin.widthPixels,
                    m_stabilizationMargin.heightLines);

        StabilizationMargin minMargin = { 0 };
        if ((NULL != pMinTotalMargin) && (pMinTotalMargin->widthMargin > 0.0f) && (pMinTotalMargin->heightMargin > 0.0f))
        {
            minMargin.widthPixels = static_cast<UINT32>(pIFEResolution->fullPortDimension.width * pMinTotalMargin->widthMargin);
            minMargin.heightLines =
                static_cast<UINT32>(pIFEResolution->fullPortDimension.height * pMinTotalMargin->heightMargin);
        }

        if ((minMargin.widthPixels > m_stabilizationMargin.widthPixels) ||
            (minMargin.heightLines > m_stabilizationMargin.heightLines))
        {
            m_stabilizationMargin = minMargin;

            CAMX_LOG_VERBOSE(CamxLogGroupAEC,
                "%s: CameraId(%d) After applying minimum total margin, EIS margin(%ux%u)",
                m_pNode->GetPipelineName(),
                m_cameraId,
                m_stabilizationMargin.widthPixels,
                m_stabilizationMargin.heightLines);
        }

        // Above margin is calculated based on IFE output dimension.
        // We need to calculate based on CAMIF because Stats are based on CAMIF
        // CAMIF_Margin_width = (CAMIF_width/IFE_output_width) * Margin_width
        FLOAT widthRatio =
            static_cast<FLOAT>(m_hardwareInfo.sensorInfo.sensorResWidth) / pIFEResolution->fullPortDimension.width;
        FLOAT heightRatio =
            static_cast<FLOAT>(m_hardwareInfo.sensorInfo.sensorResHeight) / pIFEResolution->fullPortDimension.height;
        m_stabilizationMargin.widthPixels = static_cast<UINT32>(m_stabilizationMargin.widthPixels * widthRatio);
        m_stabilizationMargin.heightLines = static_cast<UINT32>(m_stabilizationMargin.heightLines * heightRatio);

        CAMX_LOG_VERBOSE(CamxLogGroupAEC,
            "CAMIF EIS margins(%ux%u) CAMIF(%ux%u) Ratio(%fx%f)",
            m_stabilizationMargin.widthPixels,
            m_stabilizationMargin.heightLines,
            m_hardwareInfo.sensorInfo.sensorResWidth,
            m_hardwareInfo.sensorInfo.sensorResHeight,
            widthRatio,
            heightRatio);

    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAEC,
                   "EISEnabled(%d) pVideoDimension(%p) pPreviewDimension(%p) pIFEResolution(%p)",
                   EISEnabled,
                   pVideoDimension,
                   pPreviewDimension,
            pIFEResolution);
    }


    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECStatsProcessor::UpdateWindowForStabilization()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECStatsProcessor::UpdateWindowForStabilization(
    StatsRectangle* pStatsROI
    ) const
{
    CamxResult result = CamxResultSuccess;

    if (NULL == pStatsROI)
    {
        return CamxResultEInvalidArg;
    }

    // Fixed center crop based on stabilization margin
    if ((0 != m_stabilizationMargin.widthPixels) || (0 != m_stabilizationMargin.heightLines))
    {
        // Calculate margin based on zoom
        FLOAT widthRatio        = static_cast<FLOAT>(pStatsROI->width) / m_hardwareInfo.sensorInfo.sensorResWidth;
        FLOAT heightRatio       = static_cast<FLOAT>(pStatsROI->height) / m_hardwareInfo.sensorInfo.sensorResHeight;
        UINT32 adjustedMarginW  = static_cast<UINT32>(widthRatio * m_stabilizationMargin.widthPixels);
        UINT32 adjustedMarginH  = static_cast<UINT32>(heightRatio * m_stabilizationMargin.heightLines);

        if ((pStatsROI->width > adjustedMarginW) && (pStatsROI->height > adjustedMarginH))
        {
            pStatsROI->width -= adjustedMarginW;
            pStatsROI->height -= adjustedMarginH;
            pStatsROI->left += adjustedMarginW / 2;
            pStatsROI->top += adjustedMarginH / 2;

            CAMX_LOG_VERBOSE(CamxLogGroupAEC,
                "After applying EIS margin Stats crop window:CamId:%d %d %d %d %d EIS Margin(%dx%d) zoomRatio(%fx%f)",
                m_cameraId,
                pStatsROI->left, pStatsROI->top, pStatsROI->width, pStatsROI->height,
                adjustedMarginW, adjustedMarginH, widthRatio, heightRatio);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupAEC,
                "Invalid stabilization margin. CamId:%d HALCrop %d %d %d %d EIS Margin(%dx%d) zoomRatio(%fx%f)",
                m_cameraId,
                pStatsROI->left, pStatsROI->top, pStatsROI->width, pStatsROI->height,
                adjustedMarginW, adjustedMarginH, widthRatio, heightRatio);
            result = CamxResultEFailed;
        }
    }

    return result;
}
CAMX_NAMESPACE_END
