////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  feature2testcase.cpp
/// @brief Implementation of the test suite parent class.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "feature2testcase.h"
#include "chimetadatautil.h"
#include "chxextensionmodule.h"
#include "spectraconfigparser.h"
#include "chistatsproperty.h"
#include "metaconfigparser.h"
#include "streamconfigparser.h"

#include <string>

// Initialize static variables
UINT32 Feature2TestCase::m_frameNumber;

VOID Feature2TestCase::Setup()
{
    CF2_LOG_ENTRY();

    // Initialize member variables
    m_pChiModule = NULL;
    m_pFeature2RequestStateMutex = NULL;
    m_pFeature2RequestStateComplete = NULL;
    m_pMetadataUtil = NULL;
    m_pFeature2RequestObject = NULL;
    m_ppBufferManagers = NULL;
    m_streamBufferMap.clear();
    m_numStreams = 0;
    m_frameNumber = 0;

    // Setup HAL and CHI helpers
    CF2_ASSERT(CDKResultSuccess == SetupCamera(), "Failed to setup camera!");
    CF2_ASSERT(CDKResultSuccess == LoadChiOps(), "Failed to load CHI ops!");

    if (NULL != m_pChiModule)
    {
        // Setup buffer manager library
        CF2_EXPECT(0 == Feature2BufferManager::LoadBufferLibs(m_pChiModule->GetLibrary()),
            "Failed to load symbols for buffer manager!");
    }

    // Get metadata manager
    CDKResult result = CreateMetadataManager();
    if (CDKResultSuccess != result)
    {
        CF2_LOG_ERROR("Metadata manager creation failed!");
    }

    // Get thread manager
    if (CDKResultSuccess == result)
    {
        result = CHIThreadManager::Create(&m_pThreadManager, "FeatureThreadManager");
        if (CDKResultSuccess != result)
        {
            CF2_LOG_ERROR("Thread manager creation failed!");
        }
    }

    // Get metadata util
    //m_pMetadataUtil = ChiMetadataUtil::GetInstance();
    //CF2_ASSERT(NULL != m_pMetadataUtil, "Could not get instance of metadata util!");

    // Setup mutex and conditions
    m_pFeature2RequestStateMutex = Mutex::Create();
    m_pFeature2RequestStateComplete = Condition::Create();

    CF2_LOG_EXIT();
}

VOID Feature2TestCase::Teardown()
{
    CF2_LOG_ENTRY();

    m_pMetadataUtil->DestroyInstance();
    m_pMetadataUtil = NULL;

    m_pChiModule->DestroyInstance();
    m_pChiModule = NULL;

    if (NULL != m_pMetadataManager)
    {
        m_pMetadataManager->Destroy();
        m_pMetadataManager = NULL;
    }

    if (NULL != m_pThreadManager)
    {
        m_pThreadManager->Destroy();
        m_pThreadManager = NULL;
    }

    if (NULL != m_pFeature2RequestObject)
    {
        m_pFeature2RequestObject->Destroy();
        m_pFeature2RequestObject = NULL;
    }

    // Destroy buffer managers
    std::map<NativeChiStream*, GenericBufferManager*>::iterator iter;
    for (iter = m_streamBufferMap.begin(); iter != m_streamBufferMap.end(); iter++)
    {
        NativeChiStream* pStream = iter->first;
        GenericBufferManager* pManager = iter->second;
        if (NULL != pManager)
        {
            pManager->DestroyBuffers();
        }
    }
    m_streamBufferMap.clear();

    // Destroy mutex and conditions
    if (NULL != m_pFeature2RequestStateMutex)
    {
        m_pFeature2RequestStateMutex->Destroy();
        m_pFeature2RequestStateMutex = NULL;
    }

    if (NULL != m_pFeature2RequestStateComplete)
    {
        m_pFeature2RequestStateComplete->Destroy();
        m_pFeature2RequestStateComplete = NULL;
    }

    ChiModule::DestroyInstance();
    m_pChiModule = NULL;
    CF2_LOG_EXIT();
}

CDKResult Feature2TestCase::LoadChiOps()
{
    CF2_LOG_ENTRY();
    m_pChiModule = ChiModule::GetInstance();
    if (NULL == m_pChiModule)
    {
        CF2_LOG_ERROR("Could not initialize ChiModule!");
        CF2_LOG_EXIT();
        return CDKResultEFailed;
    }

    CF2_LOG_EXIT();
    return CDKResultSuccess;
}

// Queries the camera info to generate a context in the chioverride
CDKResult Feature2TestCase::SetupCamera()
{
    CF2_LOG_ENTRY();

    // Get number of cameras
    UINT32 numFwCameras;
    UINT32 numLogicalCameras;
    ExtensionModule::GetInstance()->GetNumCameras(&numFwCameras, &numLogicalCameras);

    LogicalCameraInfo logicalCameraInfo = { 0 };
    ChxUtils::Memcpy(&logicalCameraInfo,
                     ExtensionModule::GetInstance()->GetPhysicalCameraInfo(SpectraConfigParser::GetSensorID()),
                     sizeof(LogicalCameraInfo));
    m_selectCam = logicalCameraInfo.cameraId;

    // Get camera info
    if (numLogicalCameras > 0)
    {
        ExtensionModule::GetInstance()->GetCameraInfo(m_selectCam, &m_cameraInfo);

        // Initialize extension module settings
        const UINT32 numExtendSettings = static_cast<UINT32>(ChxSettingsToken::CHX_SETTINGS_TOKEN_COUNT);
        CHIEXTENDSETTINGS* extendSettings = NULL;
        CHIMODIFYSETTINGS* modifySettings = NULL;
        ChiModule::GetInstance()->GetChiOps()->pGetSettings(&extendSettings, &modifySettings);

        // Open camera
        ExtensionModule::GetInstance()->ExtendOpen(m_selectCam, extendSettings);

        // Modify settings for each override setting
        for (UINT32 i = 0; i < numExtendSettings; i++)
        {
            ExtensionModule::GetInstance()->ModifySettings(&modifySettings[i]);
        }
    }

    CF2_LOG_EXIT();
    return CDKResultSuccess;
}

// Called by derived class to initialize input metadata buffer pool
CDKResult Feature2TestCase::InitializeInputMetaBufferPool(int cameraId, CHISTREAMCONFIGINFO* pStreamConfig, const char* inputMetaName, bool multiFrame)
{
    CDKResult result = CDKResultSuccess;
    CDK_UNUSED_PARAM(cameraId);

    UINT32 poolSize = 8;
    if (NULL != pStreamConfig)
    {
        poolSize = pStreamConfig->pChiStreams[0]->maxNumBuffers;
    }

    ChiMetadataUtil* pInstance = ChiMetadataUtil::GetInstance();
    if (NULL != pInstance)
    {
        result = pInstance->CreateInputMetabufferPool(poolSize, inputMetaName, multiFrame);
    }
    else
    {
        CF2_LOG_ERROR("Fail to get ChiMetadataUtil instance!");
        result = CDKResultENoSuch;
    }

    return result;
}

// Called by derived class to initialize buffer managers for each stream
CDKResult Feature2TestCase::InitializeBufferManagers(int cameraId, CHISTREAMCONFIGINFO* pStreamConfig, const char* inputImage, bool multiFrame)
{
    CDKResult result     = CDKResultSuccess;
    int numOutputStreams = 0;

    m_numStreams = pStreamConfig->numStreams;

    if (m_numStreams <= 0)
    {
        CF2_LOG_ERROR("Cannot create buffermanagers with stream count: %d", m_numStreams);
        return CDKResultENeedMore;
    }

    m_ppBufferManagers = CF2_NEW GenericBufferManager*[m_numStreams];

    for (int streamIndex = 0; streamIndex < m_numStreams; streamIndex++)
    {
        NativeChiStream* pCurrentStream = pStreamConfig->pChiStreams[streamIndex];
        if ((ChiStreamTypeOutput        == pCurrentStream->streamType) ||
            (ChiStreamTypeBidirectional == pCurrentStream->streamType))
        {
            ++numOutputStreams;
        }
        GenericStream genericStream(pCurrentStream);

        GenericBufferManager* pManager = CF2_NEW Feature2BufferManager();
        m_ppBufferManagers[streamIndex] = pManager->Initialize(
            cameraId,
            &genericStream,
            streamIndex,
            GetTestCaseName(),
            inputImage,
            multiFrame);

        if (NULL == m_ppBufferManagers[streamIndex])
        {
            CF2_LOG_ERROR("Failed to allocate memory for buffer manager for stream index: %d", streamIndex);
            return CDKResultENoMemory;
        }

        m_streamBufferMap[pCurrentStream] = m_ppBufferManagers[streamIndex];
    }
    m_numOutputStreams = numOutputStreams;
    return CDKResultSuccess;
}

// Called by derived class to patch input metadata buffers from file
CDKResult Feature2TestCase::PatchingMetadata(const CHAR* pFileName, bool multiFrame)
{
    CDKResult result = CDKResultSuccess;

    ChiMetadataUtil* pInstance = ChiMetadataUtil::GetInstance();
    if (NULL != pInstance)
    {
        result = pInstance->PatchingMetabufferPool(pFileName, multiFrame);
    }
    else
    {
        CF2_LOG_ERROR("Failed to get ChiMetadataUtil instance!");
        result = CDKResultENoSuch;
    }

    return result;
}

// Called by derived class to patch input metadata with stats
CDKResult Feature2TestCase::PatchingStats(const CHAR* pStatsVendorTagSection)
{
    CDKResult result = CDKResultSuccess;

    ChiMetadataUtil* pInstance = ChiMetadataUtil::GetInstance();
    if (NULL != pInstance)
    {
        result = pInstance->PatchingMetabufferPoolStats(pStatsVendorTagSection);
    }
    else
    {
        CF2_LOG_ERROR("Failed to get ChiMetadataUtil instance!");
        result = CDKResultENoSuch;
    }

    return result;
}

// Called by derived class to delete tag from metadata buffers
CDKResult Feature2TestCase::DeleteTag(const CHAR* pSection, const CHAR* pTag)
{
    CDKResult        result    = CDKResultSuccess;
    ChiMetadataUtil* pInstance = ChiMetadataUtil::GetInstance();

    if (NULL != pInstance)
    {
        result = pInstance->DeleteTagFromMetabufferPool(pSection, pTag);
    }
    else
    {
        CF2_LOG_ERROR("Failed to get ChiMetadataUtil instance!");
        result = CDKResultENoSuch;
    }

    return result;
}

// Called by derived class to patch input metadata buffers
CDKResult Feature2TestCase::PatchingMetadata(const CHAR* pSection, const CHAR* pTag, const VOID* pData, UINT32 count)
{
    CDKResult result = CDKResultSuccess;

    ChiMetadataUtil* pInstance = ChiMetadataUtil::GetInstance();
    if (NULL != pInstance)
    {
        result = pInstance->PatchingMetabufferPool(pSection, pTag, pData, count);
    }
    else
    {
        CF2_LOG_ERROR("Failed to get ChiMetadataUtil instance!");
        result = CDKResultENoSuch;
    }

    return result;
}

// Called by derived class to patch input metadata buffers with tuning mode
CDKResult Feature2TestCase::PatchingTuningMode()
{
    CDKResult result = CDKResultSuccess;

    ChiMetadataUtil* pInstance = ChiMetadataUtil::GetInstance();
    if (NULL != pInstance)
    {
        UINT feature1SubModeType = SpectraConfigParser::GetModeFeature1SubModeType();
        result = pInstance->PatchingMetabufferPool("org.quic.camera2.tuning.feature", "Feature1Mode", &feature1SubModeType, 1);

        UINT feature2SubModeType = SpectraConfigParser::GetModeFeature2SubModeType();
        result |= pInstance->PatchingMetabufferPool("org.quic.camera2.tuning.feature", "Feature2Mode", &feature2SubModeType, 1);

        UINT controlModeValue = ANDROID_CONTROL_MODE_AUTO;
        // result |= pInstance->PatchingMetabufferPool(ANDROID_CONTROL_MODE, &controlModeValue, 1);

        UINT sceneSubModeType = SpectraConfigParser::GetModeSceneSubModeType();
        result |= pInstance->PatchingMetabufferPool(ANDROID_CONTROL_SCENE_MODE, &sceneSubModeType, 1);

        UINT effectSubModeType = SpectraConfigParser::GetModeEffectSubModeType();
        result |= pInstance->PatchingMetabufferPool(ANDROID_CONTROL_EFFECT_MODE, &effectSubModeType, 1);
    }
    else
    {
        CF2_LOG_ERROR("Failed to get ChiMetadataUtil instance!");
        result = CDKResultENoSuch;
    }

    return result;
}

// Called by derived class to update stream with data from stream config xml
CDKResult Feature2TestCase::PatchingStreamConfig(CHISTREAMCONFIGINFO* pStreamConfigInfo)
{
    CDKResult result = CDKResultSuccess;

    CHISTREAM* pInputStreamsConfig      = StreamConfigParser::GetInputStreams();
    CHISTREAM* pOutputStreamsConfig     = StreamConfigParser::GetOutputStreams();
    UINT       inputStreamConfigNum     = StreamConfigParser::GetNumInputStreams();
    UINT       outputStreamConfigNum    = StreamConfigParser::GetNumOutputStreams();
    UINT       inputStreamsNum          = 0;
    UINT       outputStreamsNum         = 0;
    UINT       bidirectionStreamsNum    = 0;

    if ((NULL != pStreamConfigInfo) && (TRUE == StreamConfigParser::IsInitialized()))
    {
        for (UINT i = 0; i < pStreamConfigInfo->numStreams; i++)
        {
            CHISTREAM*    pStream    = pStreamConfigInfo->pChiStreams[i];
            CHISTREAMTYPE streamType = pStream->streamType;

            switch (streamType)
            {
            case ChiStreamTypeInput:
                inputStreamsNum++;
                break;
            case ChiStreamTypeOutput:
                outputStreamsNum++;
                break;
            case ChiStreamTypeBidirectional:
                bidirectionStreamsNum++;
                break;
            default:
                CF2_LOG_WARN("Unsupported stream type %d", streamType);
                break;
            }
        }

        UINT inputStreamCount  = (inputStreamsNum <= inputStreamConfigNum) ? inputStreamsNum : inputStreamConfigNum;
        UINT outputStreamCount = (outputStreamsNum <= outputStreamConfigNum) ? outputStreamsNum : outputStreamConfigNum;
        UINT inputStreamIndx   = 0;
        UINT outputStreamIndx  = 0;

        for (UINT i = 0; i < pStreamConfigInfo->numStreams; i++)
        {
            CHISTREAM*    pStream    = pStreamConfigInfo->pChiStreams[i];
            CHISTREAMTYPE streamType = pStream->streamType;

            switch (streamType)
            {
            case ChiStreamTypeInput:
                if (inputStreamCount > inputStreamIndx)
                {
                    pStream->width                    = pInputStreamsConfig[inputStreamIndx].width;
                    pStream->height                   = pInputStreamsConfig[inputStreamIndx].height;
                    pStream->format                   = pInputStreamsConfig[inputStreamIndx].format;
                    pStream->streamParams.planeStride = pInputStreamsConfig[inputStreamIndx].streamParams.planeStride;
                    pStream->streamParams.sliceHeight = pInputStreamsConfig[inputStreamIndx].streamParams.sliceHeight;
                    inputStreamIndx++;
                }
                break;
            case ChiStreamTypeOutput:
                if (outputStreamCount > outputStreamIndx)
                {
                    pStream->width                    = pOutputStreamsConfig[outputStreamIndx].width;
                    pStream->height                   = pOutputStreamsConfig[outputStreamIndx].height;
                    pStream->format                   = pOutputStreamsConfig[outputStreamIndx].format;
                    pStream->streamParams.planeStride = pOutputStreamsConfig[outputStreamIndx].streamParams.planeStride;
                    pStream->streamParams.sliceHeight = pOutputStreamsConfig[outputStreamIndx].streamParams.sliceHeight;
                    outputStreamIndx++;
                }
                break;
            case ChiStreamTypeBidirectional:
                break;
            default:
                CF2_LOG_WARN("Unsupported stream type %d", streamType);
                break;
            }
        }
    }
    else
    {
        CF2_LOG_DEBUG("stream config info = %pK, stream config initialized = %d",
                      pStreamConfigInfo, StreamConfigParser::IsInitialized());
    }

    return result;
}

// Called by derived class to patch input metadata buffers with data from meta config xml
CDKResult Feature2TestCase::PatchingMetaConfig()
{
    CDKResult result = CDKResultSuccess;

    ChiMetadataUtil* pInstance = ChiMetadataUtil::GetInstance();
    if ((NULL != pInstance) && (TRUE == MetaConfigParser::IsInitialized()))
    {
        // AEC meta - Frame Control
        AECFrameControl aecFrameCtl = { { { 0 } } };

        aecFrameCtl.exposureInfo[ExposureIndexShort].exposureTime      = MetaConfigParser::GetAecExpTimeShort();
        aecFrameCtl.exposureInfo[ExposureIndexLong].exposureTime       = MetaConfigParser::GetAecExpTimeLong();
        aecFrameCtl.exposureInfo[ExposureIndexSafe].exposureTime       = MetaConfigParser::GetAecExpTimeSafe();

        aecFrameCtl.exposureInfo[ExposureIndexShort].linearGain        = MetaConfigParser::GetShortGain() /
                                                                         MetaConfigParser::GetDigitalGain();
        aecFrameCtl.exposureInfo[ExposureIndexLong].linearGain         = MetaConfigParser::GetGain() /
                                                                         MetaConfigParser::GetDigitalGain();
        aecFrameCtl.exposureInfo[ExposureIndexSafe].linearGain         = MetaConfigParser::GetGain() /
                                                                         MetaConfigParser::GetDigitalGain();

        aecFrameCtl.exposureInfo[ExposureIndexShort].sensitivity       = MetaConfigParser::GetAecExpTimeShort() *
                                                                         MetaConfigParser::GetShortGain();
        aecFrameCtl.exposureInfo[ExposureIndexLong].sensitivity        = MetaConfigParser::GetAecExpTimeLong() *
                                                                         MetaConfigParser::GetGain();
        aecFrameCtl.exposureInfo[ExposureIndexSafe].sensitivity        = MetaConfigParser::GetAecExpTimeSafe() *
                                                                         MetaConfigParser::GetGain();

        aecFrameCtl.exposureInfo[ExposureIndexShort].deltaEVFromTarget = 0.0;
        aecFrameCtl.exposureInfo[ExposureIndexLong].deltaEVFromTarget  = 0.0;
        aecFrameCtl.exposureInfo[ExposureIndexSafe].deltaEVFromTarget  = 0.0;

        aecFrameCtl.LEDFirstEntryRatio                                 = 1.0;
        aecFrameCtl.LEDLastEntryRatio                                  = 1.0;
        aecFrameCtl.LEDInfluenceRatio                                  = 1.0;
        aecFrameCtl.predictiveGain                                     = 1.0;

        aecFrameCtl.luxIndex                                           = MetaConfigParser::GetLuxIdx();
        aecFrameCtl.flashInfo                                          = AECFlashInfoType::FlashInfoTypeOff;
        aecFrameCtl.calibFlashState                                    = CalibrationFlashState::CalibrationFlashReady;
        aecFrameCtl.preFlashState                                      = PreFlashState::PreFlashStateInactive;
        aecFrameCtl.compenADRCGain                                     = MetaConfigParser::GetDrcGain();
        aecFrameCtl.isInSensorHDR3ExpSnapshot                          = MetaConfigParser::GetHDRMode();
        aecFrameCtl.digitalGainForSimulation                           = MetaConfigParser::GetDigitalGain();

        result = pInstance->PatchingMetabufferPool("org.quic.camera2.statsconfigs",
                                                   "AECFrameControl",
                                                   &aecFrameCtl,
                                                   sizeof(AECFrameControl));

        static const UINT PropertyIDAECFrameControl = 0x30000001;
        result = pInstance->PatchingMetabufferPool(PropertyIDAECFrameControl, &aecFrameCtl, sizeof(AECFrameControl));

        // AWB meta - Frame Control
        AWBFrameControl awbFrameCtl = { { 0 } };

        awbFrameCtl.AWBGains.rGain   = MetaConfigParser::GetGain_r();
        awbFrameCtl.AWBGains.gGain   = MetaConfigParser::GetGain_g();
        awbFrameCtl.AWBGains.bGain   = MetaConfigParser::GetGain_b();
        awbFrameCtl.colorTemperature = MetaConfigParser::GetCCT();

        awbFrameCtl.AWBCCM[0].isCCMOverrideEnabled = MetaConfigParser::GetCCMIFEFlag();
        ChxUtils::Memcpy(awbFrameCtl.AWBCCM[0].CCM, MetaConfigParser::GetCCMIFEMatrix(), sizeof(awbFrameCtl.AWBCCM[0].CCM));
        ChxUtils::Memcpy(awbFrameCtl.AWBCCM[0].CCMOffset,
                         MetaConfigParser::GetCCMIFEOffset(),
                         sizeof(awbFrameCtl.AWBCCM[0].CCMOffset));

        awbFrameCtl.AWBCCM[1].isCCMOverrideEnabled = MetaConfigParser::GetCCMBPSFlag();
        ChxUtils::Memcpy(awbFrameCtl.AWBCCM[1].CCM, MetaConfigParser::GetCCMBPSMatrix(), sizeof(awbFrameCtl.AWBCCM[1].CCM));
        ChxUtils::Memcpy(awbFrameCtl.AWBCCM[1].CCMOffset,
                         MetaConfigParser::GetCCMBPSOffset(),
                         sizeof(awbFrameCtl.AWBCCM[1].CCMOffset));

        awbFrameCtl.AWBCCM[2].isCCMOverrideEnabled = MetaConfigParser::GetCCMIPEFlag();
        ChxUtils::Memcpy(awbFrameCtl.AWBCCM[2].CCM, MetaConfigParser::GetCCMIPEMatrix(), sizeof(awbFrameCtl.AWBCCM[2].CCM));
        ChxUtils::Memcpy(awbFrameCtl.AWBCCM[2].CCMOffset,
                         MetaConfigParser::GetCCMIPEOffset(),
                         sizeof(awbFrameCtl.AWBCCM[2].CCMOffset));

        // TODO - Make the below based on use case ? Snapshot will use 2 CCMs (BPS/IPE), Video 2 (IFE / IPE)
        awbFrameCtl.numValidCCMs = 2;

        result = pInstance->PatchingMetabufferPool("org.quic.camera2.statsconfigs",
                                                   "AWBFrameControl",
                                                   &awbFrameCtl,
                                                   sizeof(AWBFrameControl));

        static const UINT PropertyIDAWBFrameControl = 0x30000002;
        result = pInstance->PatchingMetabufferPool(PropertyIDAWBFrameControl, &awbFrameCtl, sizeof(AWBFrameControl));
    }
    else
    {
        CF2_LOG_DEBUG("ChiMetaUtil Instance = %pK, meta config initialized = %d",
                      pInstance, MetaConfigParser::IsInitialized());
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHIUnitTestFeature2::CreateMetadataManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Feature2TestCase::CreateMetadataManager()
{
    CF2_LOG_ENTRY();
    CDKResult result = CDKResultSuccess;

    if (NULL == m_pMetadataManager)
    {
        m_pMetadataManager = ChiMetadataManager::Create();
        if (NULL != m_pMetadataManager)
        {
            m_genericMetadataClientId = m_pMetadataManager->RegisterClient(
                TRUE,
                NULL,
                0,
                0,
                m_DefaultNumMetadataBuffers,
                ChiMetadataUsage::Generic);
        }
        else
        {
            result = CDKResultENoMemory;
        }
    }
    else
    {
        result = CDKResultEFailed;
    }

    CF2_LOG_EXIT();
    return result;
}

CDKResult Feature2TestCase::VerifyFeature2Interface()
{
    CF2_EXPECT(NULL != m_feature2Interface.pInitializeFeature2Test, "pInitializeFeature2Test is NULL!");
    CF2_EXPECT(NULL != m_feature2Interface.pGetFeature2Descriptor, "pGetFeature2Descriptor is NULL!");
    CF2_EXPECT(NULL != m_feature2Interface.pGetInputFeature2RequestObject, "pGetInputFeature2RequestObject is NULL!");
    CF2_EXPECT(NULL != m_feature2Interface.pProcessMessage, "pProcessMessage is NULL!");
    CF2_EXPECT(NULL != m_feature2Interface.pGetInputForPort, "pGetInputForPort is NULL!");
    CF2_EXPECT(NULL != m_feature2Interface.pUpdateInputMetadata, "pUpdateInputMetadata is NULL!");
    CF2_EXPECT(NULL != m_feature2Interface.pCreateFeature2, "pCreateFeature2 is NULL!");

    if (NULL == m_feature2Interface.pInitializeFeature2Test ||
        NULL == m_feature2Interface.pGetFeature2Descriptor ||
        NULL == m_feature2Interface.pGetInputFeature2RequestObject ||
        NULL == m_feature2Interface.pProcessMessage ||
        NULL == m_feature2Interface.pGetInputForPort ||
        NULL == m_feature2Interface.pUpdateInputMetadata ||
        NULL == m_feature2Interface.pCreateFeature2)
    {
        return CDKResultENotImplemented;
    }
    else
    {
        return CDKResultSuccess;
    }
}

ChiFeature2Interface* Feature2TestCase::GetFeature2Interface()
{
    return &m_feature2Interface;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  CHIUnitTestFeature2::Feature2CbNotifyMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Feature2TestCase::ProcessMessage(
    ChiFeature2RequestObject*   pFeatureRequestObj,
    ChiFeature2Messages*        pMessages)
{
    CF2_LOG_ENTRY();

    CDKResult result = CDKResultSuccess;

    if (NULL == pFeatureRequestObj)
    {
        CF2_LOG_DEBUG("Invalid argument: pFeatureRequestObj is NULL");
        result = CDKResultEInvalidArg;
    }

    if (NULL == pMessages)
    {
        CF2_LOG_ERROR("Invalid argument: pMessages is NULL");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        VOID* pGraphPrivateData = NULL;
        pFeatureRequestObj->GetGraphPrivateData(&pGraphPrivateData);
        Feature2TestCase* pFeature2TestCase = reinterpret_cast<Feature2TestCase*>(pGraphPrivateData);
        ChiFeature2Interface* pFeature2Interface = pFeature2TestCase->GetFeature2Interface();

        if ((NULL != pFeature2Interface) &&
            (NULL != pFeature2Interface->pProcessMessage))
        {
            pFeature2Interface->pProcessMessage(pFeatureRequestObj, pMessages);
        }
    }

    CF2_LOG_EXIT();

    return result;
}

VOID Feature2TestCase::FillDefaultMetadata(ChiMetadata* pMetaData)
{
    if (pMetaData != NULL)
    {
        ChiTuningModeParameter chiTuningModeParameter = { 0 };
        chiTuningModeParameter.noOfSelectionParameter = MaxTuningMode;

        chiTuningModeParameter.TuningMode[0].mode = ChiModeType::Default;
        chiTuningModeParameter.TuningMode[0].subMode.value = 0;
        chiTuningModeParameter.TuningMode[1].mode = ChiModeType::Sensor;
        chiTuningModeParameter.TuningMode[1].subMode.value = 0;
        chiTuningModeParameter.TuningMode[2].mode = ChiModeType::Usecase;
        chiTuningModeParameter.TuningMode[2].subMode.usecase = ChiModeUsecaseSubModeType::Preview;
        chiTuningModeParameter.TuningMode[3].mode = ChiModeType::Feature1;
        chiTuningModeParameter.TuningMode[3].subMode.feature1 = ChiModeFeature1SubModeType::None;
        chiTuningModeParameter.TuningMode[4].mode = ChiModeType::Feature2;
        chiTuningModeParameter.TuningMode[4].subMode.feature2 = ChiModeFeature2SubModeType::None;
        chiTuningModeParameter.TuningMode[5].mode = ChiModeType::Scene;
        chiTuningModeParameter.TuningMode[5].subMode.scene = ChiModeSceneSubModeType::None;
        chiTuningModeParameter.TuningMode[6].mode = ChiModeType::Effect;
        chiTuningModeParameter.TuningMode[6].subMode.effect = ChiModeEffectSubModeType::None;

        ChxUtils::SetVendorTagValue(pMetaData, VendorTag::TuningMode, sizeof(ChiTuningModeParameter), &chiTuningModeParameter);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  CHIUnitTestFeature2::RunFeature2Test
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2TestCase::RunFeature2Test()
{
    CF2_LOG_ENTRY();
    ChiFeature2CreateInputInfo feature2CreateInputInfo = { 0 };
    ChiFeature2Base*           pFeature2Base = NULL;
    //ChiFeature2RequestObject*  m_pFeature2RequestObject = NULL;
    // Verify interface and initialize
    CF2_ASSERT(CDKResultSuccess == VerifyFeature2Interface(), "One or more functions in the feature2 interface were NULL!");
    m_feature2Interface.pInitializeFeature2Test();
    ChiFeature2RequestState requestState = ChiFeature2RequestState::Initialized;

    // Get feature descriptor
    m_feature2Interface.pGetFeature2Descriptor(&feature2CreateInputInfo);

    //Add metadata manager
    feature2CreateInputInfo.pMetadataManager = m_pMetadataManager;
    CF2_LOG_INFO("Feature2Descriptor number of cameras %d", feature2CreateInputInfo.pCameraInfo->numPhysicalCameras);

    //Add thread manager
    feature2CreateInputInfo.pThreadManager = m_pThreadManager;

    // Per feature2 test? or parent test will handle callback?
    ChiFeature2GraphCallbacks graphCallbacks    = {0};
    graphCallbacks.ChiFeature2ProcessMessage    = ProcessMessage;
    feature2CreateInputInfo.pClientCallbacks    = &graphCallbacks;
    // TODO Fail test if feature descriptor is NULL

    // Create feature 2 base
    pFeature2Base = m_feature2Interface.pCreateFeature2(&feature2CreateInputInfo);


    if (NULL != pFeature2Base && NULL != m_feature2Interface.pGetInputFeature2RequestObject)
    {
        int frameCount = CmdLineParser::GetFrameCount();
        for (int i = 0; i < frameCount; i++)
        {
            // Get input metadata
            ChiMetadata* pMetadata = m_pMetadataManager->Get(m_genericMetadataClientId, 0);
            FillDefaultMetadata(pMetadata);

            m_feature2Interface.pGetInputFeature2RequestObject(pFeature2Base, pMetadata, &m_pFeature2RequestObject, this);

            if (NULL != m_pFeature2RequestObject)
            {
                //TODO : remove when callbacks are properly impemented
                do
                {
                    switch (m_pFeature2RequestObject->GetCurRequestState(0))
                    {
                    case ChiFeature2RequestState::Initialized:
                        pFeature2Base->ProcessRequest(m_pFeature2RequestObject);
                        break;
                    case ChiFeature2RequestState::InputResourcePending:
                        // Assume dependency has been published from notify callback
                        pFeature2Base->ProcessRequest(m_pFeature2RequestObject);
                        break;
                    case ChiFeature2RequestState::OutputNotificationPending:
                        pFeature2Base->ProcessRequest(m_pFeature2RequestObject);
                        break;
                    case ChiFeature2RequestState::OutputResourcePending:
                    case ChiFeature2RequestState::Executing:
                    case ChiFeature2RequestState::ReadyToExecute:
                        m_pFeature2RequestStateMutex->Lock();
                        m_pFeature2RequestStateComplete->TimedWait(m_pFeature2RequestStateMutex->GetNativeHandle(), 500);
                        m_pFeature2RequestStateMutex->Unlock();
                        break;
                    default:
                        CHX_LOG_INFO("Not waiting for %d", m_pFeature2RequestObject->GetCurRequestState(0));
                        break;
                    }
                } while (ChiFeature2RequestState::Complete != m_pFeature2RequestObject->GetCurRequestState(0));
            }
        }
    }


    //TODO: uncomment when available
    //m_feature2Interface.pGetInputFeature2RequestObject(pFeature2Base, &m_pFeature2RequestObject);

    //pFeature2Base->ProcessRequest(m_pFeature2RequestObject);

    // Fail test if feature instance is NULL
    //CF2_ASSERT(NULL != m_pFeature2RequestObject, "m_pFeature2RequestObject is NULL!");

    //m_pFeature2RequestStateMutex->Lock();
    //m_pFeature2RequestStateComplete->Wait(m_pFeature2RequestStateMutex->GetNativeHandle());
    //m_pFeature2RequestStateMutex->Unlock();

    //process output from the feature (check if it needs anything for next stage)
    //ValidateResult(pFeatureRequestObject); //TODO: validate result

    CF2_LOG_EXIT();
}

UINT32 Feature2TestCase::GetMetadataClientId() const
{
    return m_genericMetadataClientId;
}

UINT32 Feature2TestCase::GetNumOutputStreams() const
{
    return m_numOutputStreams;
}

ChiMetadataManager* Feature2TestCase::GetMetadataManager() const
{
    return m_pMetadataManager;
}

CHIThreadManager* Feature2TestCase::GetThreadManager() const
{
    return m_pThreadManager;
}
