////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  feature2offlinetest.cpp
/// @brief Implementations for offline feature testing.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "feature2offlinetest.h"
#include "feature2buffermanager.h"
#include "chifeature2generic.h"
#include "bayer2yuvinputdata.h"
#include "bpsinputdata.h"
#include "ipeinputdata.h"
#include "yuv2jpeginputdata.h"
#include "spectraconfigparser.h"

#ifndef _LINUX
#include <direct.h> // _mkdir
#endif

extern const ChiFeature2Descriptor  MultiStageFeatureDescriptor;
extern const ChiFeature2Descriptor  JPEGFeatureDescriptor;
extern const ChiFeature2Descriptor  Bayer2YuvFeatureDescriptor;
extern const ChiFeature2Descriptor  BPSFeatureDescriptor;
extern const ChiFeature2Descriptor  IPEFeatureDescriptor;
// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

// Initialize static variables
Feature2OfflineTest::TestId Feature2OfflineTest::m_testId;
ChiFeature2InstanceProps    Feature2OfflineTest::m_instanceProps;
Feature2OfflineTest*        Feature2OfflineTest::m_pTestObj;
CHISTREAM*                  Feature2OfflineTest::m_pInputStream;
CHISTREAM*                  Feature2OfflineTest::m_pOutputStream[MaxNumOutputStreams]      = { NULL };
CHISTREAMCONFIGINFO*        Feature2OfflineTest::m_pStreamConfig;
ChiMetadata*                Feature2OfflineTest::m_pMetadata;
CHITARGETBUFFERINFOHANDLE   Feature2OfflineTest::m_hInputImageBuffer;
CHITARGETBUFFERINFOHANDLE   Feature2OfflineTest::m_hOutputImageBuffer[MaxNumOutputStreams] = { NULL };
CHITARGETBUFFERINFOHANDLE   Feature2OfflineTest::m_hMetaBuffer;
const CHAR*                 Feature2OfflineTest::m_pOutputPortName[MaxNumOutputStreams]    = { NULL };
LogicalCameraInfo           Feature2OfflineTest::m_logicalCameraInfo;
CHITargetBufferManager*     Feature2OfflineTest::m_pInputImageTBM;
CHITargetBufferManager*     Feature2OfflineTest::m_pOutputImageTBM[MaxNumOutputStreams]    = { NULL };
CHITargetBufferManager*     Feature2OfflineTest::m_pMetadataTBM;
const CHAR*                 Feature2OfflineTest::m_testSuiteName;
const CHAR*                 Feature2OfflineTest::m_testCaseName;
const CHAR*                 Feature2OfflineTest::m_testFullName;

VOID Feature2OfflineTest::Setup()
{
    CF2_LOG_ENTRY();

    // Save instance so static functions can access later
    m_pTestObj = this;

    // Set interface in parent class
    SetFeature2Interface();

    // Parent class setup
    Feature2TestCase::Setup();

    CF2_LOG_EXIT();
}

VOID Feature2OfflineTest::Teardown()
{
    CF2_LOG_ENTRY();

    // Parent class teardown
    Feature2TestCase::Teardown();

    CF2_LOG_EXIT();
}

VOID Feature2OfflineTest::SetFeature2Interface()
{
    m_feature2Interface.pInitializeFeature2Test         = Feature2OfflineTest::InitializeFeature2Test;
    m_feature2Interface.pGetFeature2Descriptor          = Feature2OfflineTest::GetGenericFeature2Descriptor;
    m_feature2Interface.pGetInputFeature2RequestObject  = Feature2OfflineTest::GetInputFeature2RequestObject;
    m_feature2Interface.pProcessMessage                 = Feature2OfflineTest::ProcessMessage;
    m_feature2Interface.pGetInputForPort                = Feature2OfflineTest::GetInputForPort;
    m_feature2Interface.pUpdateInputMetadata            = Feature2OfflineTest::UpdateInputMetadata;
    m_feature2Interface.pCreateFeature2                 = Feature2OfflineTest::CreateFeature2;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Feature2OfflineTest::OfflineFeatureTest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2OfflineTest::OfflineFeatureTest(TestId testId)
{
    CF2_LOG_ENTRY();

    m_testSuiteName = "Feature2OfflineTest";

    m_testId = testId;
    switch (m_testId)
    {
    case TestId::TestBayerToYUV:
        {
            m_testCaseName = "BayerToYUV";
            m_testFullName = "Feature2OfflineTest.BayerToYUV";
        }
        break;
    case TestId::TestYUVToJpeg:
        {
            m_testCaseName = "YUVToJpeg";
            m_testFullName = "Feature2OfflineTest.YUVToJpeg";
        }
        break;
    case TestId::TestMultiStage:
        {
            m_testCaseName = "MultiStage";
            m_testFullName = "Feature2OfflineTest.MultiStage";
        }
        break;
    case TestId::TestBPS:
        {
            m_testCaseName = "BPS";
            m_testFullName = "Feature2OfflineTest.BPS";
        }
        break;
    case TestId::TestIPE:
        {
            m_testCaseName = "IPE";
            m_testFullName = "Feature2OfflineTest.IPE";
        }
        break;
    default:
        CF2_LOG_ERROR("Unknown player Id!");
        break;
    }

    // Run the test
    RunFeature2Test();

    CF2_LOG_EXIT();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Feature2OfflineTest::InitializeFeature2Test
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2OfflineTest::InitializeFeature2Test()
{
    CF2_LOG_ENTRY();

    CDKResult                        result                                        = CDKResultSuccess;
    ChiTargetBufferManagerCreateData inputImageTBMCreateData                       = { 0 };
    ChiTargetBufferManagerCreateData outputImageTBMCreateData[MaxNumOutputStreams] = { { 0 } };
    ChiTargetBufferManagerCreateData metadataTBMCreateData                         = { 0 };
    std::string                      spectraSimConfigImageName                     = SpectraConfigParser::GetImageFileName();
    SIZE_T                           pos                                           = spectraSimConfigImageName.find('.');

    BOOL bIsCustomFileName = (false == spectraSimConfigImageName.empty());

    const int   size                                             = 256;
    char        inputMetaName[size]                              = { 0 };
    char        parsedTintlessBGStats[size]                      = { 0 };
    char        parsedAWBBGStats[size]                           = { 0 };
    char        parsedBHISTStats[size]                           = { 0 };
    const char* inputImage                                       = spectraSimConfigImageName.c_str();
    const char* inputImageNoExt;
    const char* inputImageTargetBufferName                       = NULL;
    const char* outputImageTargetBufferName[MaxNumOutputStreams] = { NULL };

    // Case where a custom file name is specified for the input images
    if (true == bIsCustomFileName)
    {
        // Get the portion of the file name before the extension
        inputImageNoExt = std::string(spectraSimConfigImageName).erase(pos).c_str();

        // The metadata and stats dumps have the same name as the input image followed by their respective extensions
        CdkUtils::StrLCpy(inputMetaName, inputImageNoExt, size);
        CdkUtils::StrLCat(inputMetaName, ".bin", size);
        CdkUtils::StrLCpy(parsedTintlessBGStats, inputImageNoExt, size);
        CdkUtils::StrLCat(parsedTintlessBGStats, ".tint", size);
        CdkUtils::StrLCpy(parsedAWBBGStats, inputImageNoExt, size);
        CdkUtils::StrLCat(parsedAWBBGStats, ".awbbg", size);
        CdkUtils::StrLCpy(parsedBHISTStats, inputImageNoExt, size);
        CdkUtils::StrLCat(parsedBHISTStats, ".bhist", size);
    }
    // If no custom file name is specified, then the default names are used for the various inputs
    else
    {
        CdkUtils::StrLCpy(inputMetaName, "B2Y_Metadata_0.bin", size);
        CdkUtils::StrLCpy(parsedTintlessBGStats, "ParsedTintlessBGStats_0.tint", size);
        CdkUtils::StrLCpy(parsedAWBBGStats, "ParsedAWBBGStats_0.awbbg", size);
        CdkUtils::StrLCpy(parsedBHISTStats, "ParsedBHISTStats_0.bhist", size);
    }

    switch (m_testId)
    {
    case TestBPS:
        m_pInputStream                          = &tests::BPSStreamsInput;
        m_pOutputStream[FULL_INDEX]             = &tests::BPSStreamsOutput;
        m_pOutputStream[DS4_INDEX]              = &tests::BPSStreamsOutputDS4;
        m_pOutputStream[DS16_INDEX]             = &tests::BPSStreamsOutputDS16;
        m_pOutputStream[DS64_INDEX]             = &tests::BPSStreamsOutputDS64;
        m_pStreamConfig                         = &tests::BPSStreamConfigInfo;
        m_pOutputPortName[FULL_INDEX]           = "YUV_Out";
        m_pOutputPortName[DS4_INDEX]            = "DS4_Out";
        m_pOutputPortName[DS16_INDEX]           = "DS16_Out";
        m_pOutputPortName[DS64_INDEX]           = "DS64_Out";
        inputImage                              = (TRUE == bIsCustomFileName) ?
                                                  (spectraSimConfigImageName.c_str()):
                                                  (tests::BPS_TEST_IMAGE);
        inputImageTargetBufferName              = "inRAWTarget";
        outputImageTargetBufferName[FULL_INDEX] = "outYUVTarget";
        outputImageTargetBufferName[DS4_INDEX]  = "outDS4Target";
        outputImageTargetBufferName[DS16_INDEX] = "outDS16Target";
        outputImageTargetBufferName[DS64_INDEX] = "outDS64Target";
        break;
    case TestIPE:
        m_pInputStream                 = &tests::IPEStreamsInput;
        m_pOutputStream[0]             = &tests::IPEStreamsOutput;
        m_pStreamConfig                = &tests::IPEStreamConfigInfo;
        m_pOutputPortName[0]           = "YUV_Out";
        inputImage                     = (TRUE == bIsCustomFileName) ?
                                         (spectraSimConfigImageName.c_str()):
                                         (tests::IPE_TEST_IMAGE);
        inputImageTargetBufferName     = "inYUVTarget";
        outputImageTargetBufferName[0] = "outYUVTarget";
        break;
    case TestBayerToYUV:
        m_pInputStream                 = &tests::Bayer2YuvStreamsInput;
        m_pOutputStream[0]             = &tests::Bayer2YuvStreamsOutput;
        m_pStreamConfig                = &tests::Bayer2YuvStreamConfigInfo;
        m_pOutputPortName[0]           = "YUV_Out";
        inputImage                     = (TRUE == bIsCustomFileName) ?
                                         (spectraSimConfigImageName.c_str()):
                                         (tests::BPS_IDEALRAW_UHD);
        inputImageTargetBufferName     = "inRAWTarget";
        outputImageTargetBufferName[0] = "outYUVTarget";
        break;
    case TestMultiStage:
        m_pInputStream                 = &tests::Bayer2YuvStreamsInput;
        m_pOutputStream[0]             = &tests::Bayer2YuvStreamsOutput;
        m_pStreamConfig                = &tests::Bayer2YuvStreamConfigInfo;
        inputImage                     = (TRUE == bIsCustomFileName) ?
                                         (spectraSimConfigImageName.c_str()):
                                         (tests::BPS_IDEALRAW_UHD);
        inputImageTargetBufferName     = "inRAWTarget";
        outputImageTargetBufferName[0] = "outYUVTarget";
        break;
    case TestYUVToJpeg:
        m_pInputStream                 = &tests::YUV2JPEGStreamsInput;
        m_pOutputStream[0]             = &tests::YUV2JPEGStreamsOutput;
        m_pStreamConfig                = &tests::YUV2JPEGStreamConfigInfo;
        m_pOutputPortName[0]           = "JPEG_Out";
        inputImage                     = (TRUE == bIsCustomFileName) ?
                                         (spectraSimConfigImageName.c_str()):
                                         (tests::IPE_COLORBAR_VGA);
        inputImageTargetBufferName     = "inYUVTarget";
        outputImageTargetBufferName[0] = "outJpegTarget";
        break;
    default:
        CF2_FAIL2(m_pTestObj, "Invalid test ID! (%d)", m_testId);
        break;
    }

    m_pTestObj->PatchingStreamConfig(m_pStreamConfig);

    int cameraId = SpectraConfigParser::GetSensorID();

    // Create a buffer manager for each stream
    CF2_ASSERT(CDKResultSuccess == m_pTestObj->InitializeBufferManagers(cameraId, m_pStreamConfig, inputImage, FALSE),
        "Failed to initialize buffer managers!");

    // Create input image target buffer manager
    inputImageTBMCreateData.pTargetBufferName          = inputImageTargetBufferName;
    inputImageTBMCreateData.numOfMetadataBuffers       = 0;
    inputImageTBMCreateData.numOfInternalStreamBuffers = 0;
    inputImageTBMCreateData.numOfExternalStreamBuffers = 1;
    inputImageTBMCreateData.externalStreamIds[0]       = (UINT64)m_pInputStream;
    m_pInputImageTBM = CHITargetBufferManager::Create(&inputImageTBMCreateData);

    // Create output image target buffer manager
    for (UINT32 streamIndex = 0; streamIndex < m_pTestObj->GetNumOutputStreams(); streamIndex++)
    {
        outputImageTBMCreateData[streamIndex].pTargetBufferName          = outputImageTargetBufferName[streamIndex];
        outputImageTBMCreateData[streamIndex].numOfMetadataBuffers       = 0;
        outputImageTBMCreateData[streamIndex].numOfInternalStreamBuffers = 0;
        outputImageTBMCreateData[streamIndex].numOfExternalStreamBuffers = 1;
        outputImageTBMCreateData[streamIndex].externalStreamIds[0]       = (UINT64)m_pOutputStream[streamIndex];

        m_pOutputImageTBM[streamIndex] = CHITargetBufferManager::Create(&outputImageTBMCreateData[streamIndex]);
    }

    // Create input metadata pool and preload metadata from files
    CF2_ASSERT(CDKResultSuccess == m_pTestObj->InitializeInputMetaBufferPool(cameraId, m_pStreamConfig, &inputMetaName[0], FALSE),
               "Failed to initialize input metadata pool!");

    // Patching Parsed Tintless BG stats to metadata
    m_pTestObj->PatchingStats("org.codeaurora.qcamera3.tintless");

    // Patching Parsed AWB BG stats to metadata
    m_pTestObj->PatchingStats("org.codeaurora.qcamera3.awbbg");

    // Patching Parsed BHIST stats to metadata
    m_pTestObj->PatchingStats("org.codeaurora.qcamera3.bhist");

    // Disable debug data
    result = m_pTestObj->DeleteTag("org.quic.camera.debugdata", "DebugDataAll");

    // Set tuning mode
    m_pTestObj->PatchingTuningMode();

    m_pTestObj->PatchingMetaConfig();
    // Create metadata target buffer manager
    metadataTBMCreateData.pTargetBufferName                  = "MetadataTarget";
    metadataTBMCreateData.numOfMetadataBuffers               = 1;
    metadataTBMCreateData.numOfInternalStreamBuffers         = 0;
    metadataTBMCreateData.numOfExternalStreamBuffers         = 0;
    metadataTBMCreateData.pChiMetadataManager                = m_pTestObj->GetMetadataManager();
    metadataTBMCreateData.metadataIds[0]                     = m_pTestObj->GetMetadataClientId();
    m_pMetadataTBM = CHITargetBufferManager::Create(&metadataTBMCreateData);

    if (NULL == m_pInputImageTBM || NULL == m_pMetadataTBM)
    {
        result = CDKResultEFailed;
        CF2_LOG_ERROR("TBM creation failed!");
    }

    CF2_LOG_EXIT();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Feature2OfflineTest::GetGenericFeature2Descriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2OfflineTest::GetGenericFeature2Descriptor(
    ChiFeature2CreateInputInfo* pFeature2CreateInputInfoOut)
{
    CF2_LOG_ENTRY();

    const ChiFeature2Descriptor* pFeatureDescriptor = NULL;

    switch (m_testId)
    {
    case TestBayerToYUV:
        pFeatureDescriptor = &Bayer2YuvFeatureDescriptor;
        break;
    case TestBPS:
        pFeatureDescriptor = &BPSFeatureDescriptor;
        break;
    case TestIPE:
        pFeatureDescriptor = &IPEFeatureDescriptor;
        break;
    case TestYUVToJpeg:
        pFeatureDescriptor = &JPEGFeatureDescriptor;
        break;
    case TestMultiStage:
        pFeatureDescriptor = &MultiStageFeatureDescriptor;
        break;
    default:
        CF2_FAIL2(m_pTestObj, "Invalid test ID! (%d)", m_testId);
        break;
    }

    m_instanceProps                        = { 0 };
    m_instanceProps.cameraId               = 0;
    ChxUtils::Memcpy(&m_logicalCameraInfo,
                     ExtensionModule::GetInstance()->GetPhysicalCameraInfo(SpectraConfigParser::GetSensorID()),
                     sizeof(LogicalCameraInfo));

    //TODO: either make one file of multiple input tables or include input tables for different features
    pFeature2CreateInputInfoOut->pInstanceProps     = &m_instanceProps;
    pFeature2CreateInputInfoOut->pFeatureDescriptor = const_cast<ChiFeature2Descriptor*>(pFeatureDescriptor);
    pFeature2CreateInputInfoOut->pCameraInfo        = &m_logicalCameraInfo;
    CHX_LOG_ERROR("Result: GetFeature2Descriptor out numcam %d", pFeature2CreateInputInfoOut->pCameraInfo->numPhysicalCameras);
    pFeature2CreateInputInfoOut->pUsecaseDescriptor = g_pUsecaseZSL;//g_pUsecaseSAT;
    pFeature2CreateInputInfoOut->pStreamConfig      = m_pStreamConfig;
    CHX_LOG_ERROR("Result: GetFeature2Descriptor out");

    CF2_LOG_EXIT();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Feature2OfflineTest::GetInputFeature2RequestObject
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2OfflineTest::GetInputFeature2RequestObject(
    ChiFeature2Base*            pFeature2Base,
    ChiMetadata*                pMetadata,
    ChiFeature2RequestObject**  ppFeature2RequestObjectOut,
    VOID*                       pPrivateData)
{
    CF2_LOG_ENTRY();
    UINT32 halFrameIndex = SpectraConfigParser::GetStartFrameIndex() + m_frameNumber;
    m_pMetadata = pMetadata;
    m_frameNumber++;

    ChiFeature2RequestObjectCreateInputInfo feature2RequestObjInputInfo                   = { 0 };
    ChiFeature2RequestOutputInfo            featureRequestOutputInfo[MaxNumOutputStreams] = { { 0 } };
    UINT8                                   numExternalOutputPorts                        = 0;

    // Create usecaseRequestObject with frame number
    camera3_stream_buffer_t   halBuffers[MaxNumOutputStreams] = { { 0 } };
    camera3_capture_request_t halRequest                      = { 0 };

    halRequest.frame_number              = halFrameIndex;
    halRequest.num_output_buffers        = m_pTestObj->GetNumOutputStreams();
    for (UINT32 i = 0; i < m_pTestObj->GetNumOutputStreams(); i++)
    {
        halBuffers[i].stream = reinterpret_cast<camera3_stream_t*>(m_pOutputStream[i]);
    }
    halRequest.output_buffers            = halBuffers;

    ChiFeature2UsecaseRequestObjectCreateInputInfo usecaseRequestObjectCreateInputInfo = { 0 };
    usecaseRequestObjectCreateInputInfo.pRequest        = &halRequest;
    usecaseRequestObjectCreateInputInfo.pAppSettings    = pMetadata;
    ChiFeature2UsecaseRequestObject* pUsecaseRequestObj =
        ChiFeature2UsecaseRequestObject::Create(&usecaseRequestObjectCreateInputInfo);

    feature2RequestObjInputInfo.usecaseMode        = static_cast<ChiModeUsecaseSubModeType>(SpectraConfigParser::GetUsecase());
    feature2RequestObjInputInfo.pUsecaseRequestObj = pUsecaseRequestObj;
    feature2RequestObjInputInfo.pGraphPrivateData  = pPrivateData; //TODO: Not required in first phase
    feature2RequestObjInputInfo.pFeatureBase       = pFeature2Base;

    // Count external ports and setup port info
    std::vector<ChiFeature2Identifier> identifiers = pFeature2Base->GetExternalGlobalPortIdList();
    for (UINT8 portIndex = 0; portIndex < identifiers.size(); portIndex++)
    {
        if (ChiFeature2PortDirectionType::ExternalOutput == identifiers[portIndex].portDirectionType)
        {
            numExternalOutputPorts++;
        }
    }

    int externalIndex = 0;
    std::shared_ptr<ChiFeature2RequestOutputInfo> pRequestOutputs(
        CF2_NEW ChiFeature2RequestOutputInfo[numExternalOutputPorts]);

    // Setup output port info
    for (UINT8 portIndex = 0; portIndex < identifiers.size(); portIndex++)
    {
        BOOL validEntry        = FALSE;
        INT  outputStreamIndex = -1;

        if (ChiFeature2PortDirectionType::ExternalOutput == identifiers[portIndex].portDirectionType)
        {
            const ChiFeature2PortDescriptor* pPortDescriptor =
                pFeature2Base->GetPortDescriptorFromPortId((&identifiers[portIndex]));

            // Setup output image buffers
            if (ChiFeature2PortType::ImageBuffer == identifiers[portIndex].portType)
            {
                for (UINT i = 0; i < MaxNumOutputStreams; i++)
                {
                    const CHAR* pPortName = m_pOutputPortName[i];

                    if ((NULL != pPortName)       &&
                        (NULL != pPortDescriptor) &&
                        (0 == CdkUtils::StrCmp(pPortDescriptor->pPortName, pPortName)))
                    {
                        validEntry        = TRUE;
                        outputStreamIndex = i;
                        break;
                    }
                }
            }

            if (ChiFeature2PortType::MetaData == identifiers[portIndex].portType)
            {
                validEntry                                              = TRUE;
                featureRequestOutputInfo[externalIndex].pPortDescriptor = pPortDescriptor;
            }

            if (-1 != outputStreamIndex)
            {
                ChiFeature2BufferMetadataInfo bufferMetaInfo = { 0 };
                UINT64 targetId                              = m_frameNumber;

                featureRequestOutputInfo[externalIndex].pPortDescriptor = pPortDescriptor;

                // Get output buffer for request
                GenericBufferManager* pManager = m_pTestObj->m_streamBufferMap[m_pOutputStream[outputStreamIndex]];
                NativeChiBuffer* pOutputBuffer = static_cast<NativeChiBuffer*>(pManager->GetOutputBufferForRequest());

                // Import output image buffer
                CDKResult result = m_pOutputImageTBM[outputStreamIndex]->ImportExternalTargetBuffer(targetId,
                    (UINT64)(m_pOutputStream[outputStreamIndex]), pOutputBuffer);
                if (CDKResultSuccess == result)
                {
                    m_hOutputImageBuffer[outputStreamIndex] =
                        m_pOutputImageTBM[outputStreamIndex]->SetupTargetBuffer(targetId);
                }
                else
                {
                    m_hOutputImageBuffer[outputStreamIndex] = NULL;
                    CF2_LOG_ERROR("Output image buffer import failed!");
                }

                bufferMetaInfo.hBuffer                                     = m_hOutputImageBuffer[outputStreamIndex];
                bufferMetaInfo.key                                         = (UINT64)(m_pOutputStream[outputStreamIndex]);
                featureRequestOutputInfo[externalIndex].bufferMetadataInfo = bufferMetaInfo;
            }

            if (TRUE == validEntry)
            {
                pRequestOutputs.get()[externalIndex] = featureRequestOutputInfo[externalIndex];
                externalIndex++;
            }
        }
    }

    // New Request Output information Format Dummy implementation
    // We are hardcoding number of Request as 1
    std::shared_ptr<ChiFeature2RequestMap> pRequestTable(CF2_NEW ChiFeature2RequestMap);
    feature2RequestObjInputInfo.numRequests     = 1;
    feature2RequestObjInputInfo.pRequestTable   = pRequestTable.get();

    // Since there is only 1 request we are directly indexing as 0
    pRequestTable->requestIndex         = 0;
    pRequestTable->numRequestOutputs    = externalIndex;
    pRequestTable->pRequestOutputs      = pRequestOutputs.get();

    // Example Code for set offline feature settings
    /* ChiMetadata*     pFeaturesetting                        = ChiMetadata::Create(NULL, 0, true);

    if (NULL != pFeaturesetting)
    {
        feature2RequestObjInputInfo.phPortRequestSettings[0]   = pFeaturesetting->GetHandle();
        feature2RequestObjInputInfo.numPortSettings            = 1;
    }
    else
    {
        CHX_LOG_ERROR("Create feature setting failed");
    }
    */
    // Example Code End

    // Set ChiFeature2Hint
    ChiFeature2Hint* pFeatureHint = static_cast<ChiFeature2Hint*>(CHX_CALLOC(sizeof(ChiFeature2Hint)));
    feature2RequestObjInputInfo.pFeatureHint = pFeatureHint;
    if(pFeatureHint != NULL)
    {
	pFeatureHint->numFrames = SpectraConfigParser::GetFrameCount();
    }
    else
    {
	CHX_LOG_ERROR("pFeatureHint is NULL");
    }

    *ppFeature2RequestObjectOut = ChiFeature2RequestObject::Create(&feature2RequestObjInputInfo);
    CF2_LOG_EXIT();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Feature2OfflineTest::ProcessMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Feature2OfflineTest::ProcessMessage(
    ChiFeature2RequestObject*   pFeatureRequestObj,
    ChiFeature2Messages*        pMessages)
{
    CF2_LOG_ENTRY();

    CDKResult result = CDKResultSuccess;

    if (NULL == pFeatureRequestObj)
    {
        CF2_LOG_ERROR("Invalid argument: pFeatureRequestObj is NULL");
        result = CDKResultEInvalidArg;
    }

    if (NULL == pMessages)
    {
        CF2_LOG_ERROR("Invalid argument: pMessages is NULL");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        if (NULL != pMessages->pFeatureMessages)
        {
            switch (pMessages->pFeatureMessages->messageType)
            {
                case ChiFeature2MessageType::GetInputDependency:
                {
                    result = ProcessGetInputDependencyMessage(pFeatureRequestObj, pMessages);
                    if (CDKResultSuccess != result)
                    {
                        CF2_LOG_ERROR("Failed to process get input dependency message!");
                    }
                    break;
                }
                case ChiFeature2MessageType::ResultNotification:
                {
                    result = ProcessResultNotificationMessage(pFeatureRequestObj, pMessages);
                    if (CDKResultSuccess != result)
                    {
                        CF2_LOG_ERROR("Failed to process result notification message!");
                    }
                    break;
                }
                case ChiFeature2MessageType::ReleaseInputDependency:
                {
                    result = ProcessReleaseInputDependencyMessage(pFeatureRequestObj, pMessages);
                    if (CDKResultSuccess != result)
                    {
                        CF2_LOG_ERROR("Failed to process release input dependency message!");
                    }
                    break;
                }
                case ChiFeature2MessageType::MetadataNotification:
                {
                    // MetadataNotification not handled in phase 1
                    break;
                }
                case ChiFeature2MessageType::SubmitRequestNotification:
                {
                    result = ProcessSubmitRequestMessage(pMessages);
                    if (CDKResultSuccess != result)
                    {
                        CF2_LOG_ERROR("Failed to submit request with result:%d!", result);
                    }
                    break;
                }
                default:
                    CF2_LOG_WARN("Unknown ChiFeature2MessageType %d", pMessages->pFeatureMessages->messageType);
                    break;
            }
        }
    }

    CF2_LOG_EXIT();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Feature2OfflineTest::ProcessGetInputDependencyMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Feature2OfflineTest::ProcessGetInputDependencyMessage(
    ChiFeature2RequestObject* pFeatureRequestObj,
    ChiFeature2Messages* pMessages)
{
    CDKResult result = CDKResultSuccess;

    // Iterate through all ports, satisfying their dependencies
    UINT numDependencies = pMessages->pFeatureMessages->message.getDependencyData.pBatches[0].numDependencies;

    for (UINT8 dependencyIndex = 0; dependencyIndex < numDependencies; ++dependencyIndex)
    {
        ChiFeature2PortIdList       portIds =
        {
            pMessages->pFeatureMessages->message.getDependencyData.pBatches[0].pDependencies[dependencyIndex].numPorts,
            pMessages->pFeatureMessages->message.getDependencyData.pBatches[0].pDependencies[dependencyIndex].pPorts
        };

        for (UINT32 portIndex = 0; portIndex < portIds.numPorts; portIndex++)
        {
            CF2_LOG_DEBUG("portId session %d, pipeline %d, port %d", portIds.pPorts[portIndex].session,
                portIds.pPorts[portIndex].pipeline, portIds.pPorts[portIndex].port);
            ChiFeature2Identifier portIdentifier = portIds.pPorts[portIndex];

            ChiFeature2BufferMetadataInfo bufferMetaInfo = { 0 };
            UINT64 seqId = m_frameNumber * numDependencies;

            switch (portIdentifier.portType)
            {
            case ChiFeature2PortType::ImageBuffer:
            {
                // Get input buffer for request
                GenericBufferManager* pManager = m_pTestObj->m_streamBufferMap[m_pInputStream];
                NativeChiBuffer* pInputBuffer = static_cast<NativeChiBuffer*>(pManager->GetInputBufferForRequest());

                // Import input image buffer
                result = m_pInputImageTBM->ImportExternalTargetBuffer(seqId + dependencyIndex, (UINT64)(m_pInputStream), pInputBuffer);
                if (CDKResultSuccess == result)
                {
                    m_hInputImageBuffer = m_pInputImageTBM->SetupTargetBuffer(seqId + dependencyIndex);
                }
                else
                {
                    m_hInputImageBuffer = NULL;
                    CF2_LOG_ERROR("Input image buffer import failed!");
                }
                bufferMetaInfo.hBuffer = m_hInputImageBuffer;
                bufferMetaInfo.key = (UINT64)(m_pInputStream);
                break;
            }
            case ChiFeature2PortType::MetaData:
            {
                // Import input metadata buffer
                UINT32 index = (m_frameNumber - 1)* numDependencies + dependencyIndex;
                ChiMetadataUtil* pInstance = ChiMetadataUtil::GetInstance();
                ChiMetadata*     pMetadata = NULL;
                if (NULL != pInstance)
                {
                    pMetadata = pInstance->GetInputMetabufferFromPool(index);
                    if (NULL != pMetadata)
                    {
                        result = m_pMetadata->Copy(*pMetadata);
                    }
                    else
                    {
                        CF2_LOG_ERROR("Fail to get ChiMetadata buffer from pool!");
                        result = CDKResultENoSuch;
                    }
                }
                else
                {
                    CF2_LOG_ERROR("Fail to get ChiMetadataUtil instance!");
                    result = CDKResultENoSuch;
                }

                if (CDKResultSuccess == result)
                {
                    result = m_pMetadataTBM->ImportExternalTargetBuffer(seqId + dependencyIndex,
                        (UINT64)(m_pTestObj->GetMetadataClientId()), pMetadata);
                }
                if (CDKResultSuccess == result)
                {
                    m_hMetaBuffer = m_pMetadataTBM->SetupTargetBuffer(seqId + dependencyIndex);
                }
                else
                {
                    m_hMetaBuffer = NULL;
                    CF2_LOG_ERROR("Metadata buffer import failed!");
                }
                bufferMetaInfo.hBuffer = m_hMetaBuffer;
                bufferMetaInfo.key = (UINT64)(m_pTestObj->GetMetadataClientId());
                break;
            }
            default:
                CF2_LOG_ERROR("Invalid port type! (%d)", portIdentifier.portType);
                break;
            }

            result = pFeatureRequestObj->SetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency, &portIdentifier,
                bufferMetaInfo.hBuffer, bufferMetaInfo.key, FALSE, 0, dependencyIndex);
            if (CDKResultSuccess != result)
            {
                CF2_LOG_ERROR("Failed to set buffer info!");
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Feature2OfflineTest::ProcessResultNotificationMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Feature2OfflineTest::ProcessResultNotificationMessage(
    ChiFeature2RequestObject*   pFeatureRequestObj,
    ChiFeature2Messages*        pMessages)
{
    int native_handle = 0;
    void *pHostptr = NULL;
    int bufferLength = 0;
    FILE* fd = NULL;
    CHAR dumpFilename[256];

    CDKResult result = CDKResultSuccess;
    ChiFeature2BufferMetadataInfo* pOutputBufferMetaInfo = NULL;

    // Iterate through all ports
    for (UINT i = 0; i < pMessages->pFeatureMessages->message.result.numPorts; ++i)
    {
        ChiFeature2Identifier portIdentifier = pMessages->pFeatureMessages->message.result.pPorts[i];

        const ChiFeature2PortDescriptor* pPortDescriptor =
            pFeatureRequestObj->GetFeature()->GetPortDescriptorFromPortId(&portIdentifier);

        // Process image buffer
        if (ChiFeature2PortType::ImageBuffer == portIdentifier.portType)
        {
            pFeatureRequestObj->GetFinalBufferMetadataInfo(portIdentifier, &pOutputBufferMetaInfo, 0);

            NativeChiBuffer*        pTargetBuffer        = NULL;
            CHITargetBufferManager* pTargetBufferManager =
                CHITargetBufferManager::GetTargetBufferManager(pOutputBufferMetaInfo->hBuffer);

            if (NULL != pTargetBufferManager)
            {
                pTargetBuffer = static_cast<NativeChiBuffer*>(pTargetBufferManager->GetTarget(
                    pOutputBufferMetaInfo->hBuffer, pOutputBufferMetaInfo->key));

                if (pTargetBuffer == NULL)
                {
                    CHX_LOG_ERROR("Unable to get buffer info for handle %pK", pOutputBufferMetaInfo->hBuffer);
                    result = CDKResultENoSuch;
                }
                else
                {
                    INT streamIndex = -1;
                    for (UINT i = 0; i < MaxNumOutputStreams; i++)
                    {
                        // skip image target which is not configured by us
                        const CHAR* pPortName = m_pOutputPortName[i];

                        if ((NULL != pPortName)       &&
                            (NULL != pPortDescriptor) &&
                            (0 == CdkUtils::StrCmp(pPortDescriptor->pPortName, pPortName)))
                        {
                            streamIndex = i;
                            break;
                        }
                    }
                    // Process result buffer
                    if (-1 != streamIndex)
                    {
                        GenericBuffer         genericBuffer(pTargetBuffer);
                        GenericBufferManager* pManager = m_pTestObj->m_streamBufferMap[m_pOutputStream[streamIndex]];
                        pManager->ProcessBufferFromResult(m_frameNumber, &genericBuffer, true);
                    }

                    // Release the target buffer
                    pTargetBufferManager->ReleaseTargetBuffer(pOutputBufferMetaInfo->hBuffer);
                }
            }
            else
            {
                CF2_LOG_ERROR("Failed to get TargetBufferManager for ImageBuffer!");
                result = CDKResultENoSuch;
            }
        }

        // Need to send ProcessRequest so base can release the buffers associated with this port
        pFeatureRequestObj->SetOutputNotifiedForPort(portIdentifier, 0);

    }
    // TODO if we receive output, change FeatureRequestObjects's state to output notified and signal the thread for next request

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Feature2OfflineTest::ProcessReleaseInputDependencyMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Feature2OfflineTest::ProcessReleaseInputDependencyMessage(
    ChiFeature2RequestObject*   pFeatureRequestObj,
    ChiFeature2Messages*        pMessages)
{
    CDKResult result = CDKResultSuccess;

    // Iterate through all dependencies and all input ports in message to mark the input resources for release
    UINT numDependencies = pMessages->pFeatureMessages->message.releaseDependencyData.numDependencies;
    ChiFeature2Dependency* pDependencies =
        pMessages->pFeatureMessages->message.releaseDependencyData.pDependencies;

    CF2_LOG_DEBUG("ReleaseInputDependency numDependencies: %u", numDependencies);

    for (UINT8 dependencyIndex = 0; dependencyIndex < numDependencies; ++dependencyIndex)
    {
        UINT numInputPorts = pDependencies[dependencyIndex].numPorts;
        const ChiFeature2Identifier* pInputPorts = pDependencies[dependencyIndex].pPorts;

        // Iterate through all input ports in the message
        for (UINT inputPortIndex = 0; inputPortIndex < numInputPorts; inputPortIndex++)
        {
            switch (pInputPorts[inputPortIndex].portType)
            {
            case ChiFeature2PortType::ImageBuffer:
                // Release the target input buffer
                m_pInputImageTBM->ReleaseTargetBuffer(m_hInputImageBuffer);
                break;
            case ChiFeature2PortType::MetaData:
                // Release the target metadata buffer
                m_pMetadataTBM->ReleaseTargetBuffer(m_hMetaBuffer);
                break;
            default:
                CF2_LOG_ERROR("Invalid port type! (%d)", pInputPorts[inputPortIndex].portType);
                break;
            }
            pFeatureRequestObj->SetOutputNotifiedForPort(pInputPorts[inputPortIndex], 0);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Feature2OfflineTest::ProcessSubmitRequestMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Feature2OfflineTest::ProcessSubmitRequestMessage(
    ChiFeature2Messages*        pMessages)
{
    CDKResult result = CDKResultSuccess;

    result = ExtensionModule::GetInstance()->SubmitRequest(const_cast<CHIPIPELINEREQUEST*>
        (&(pMessages->pFeatureMessages->message.submitRequest)));

    if (CDKResultECancelledRequest == result)
    {
        CHX_LOG_WARN("Session returned that flush was in progress. Rewriting result as success.");
        result = CDKResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Feature2OfflineTest::GetInputForPort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2OfflineTest::GetInputForPort(
    ChiFeature2Base*            pFeature2Base,
    ChiFeature2RequestObject*   pFeature2ResultObject)
{
    CF2_LOG_ENTRY();

    UNUSED_PARAM(pFeature2Base);
    UNUSED_PARAM(pFeature2ResultObject);

    CF2_LOG_EXIT();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Feature2OfflineTest::UpdateInputMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2OfflineTest::UpdateInputMetadata(
    ChiFeature2Base*            pFeature2Base,
    ChiFeature2RequestObject*   pFeature2ResultObject)
{
    CF2_LOG_ENTRY();

    UNUSED_PARAM(pFeature2Base);
    UNUSED_PARAM(pFeature2ResultObject);

    CF2_LOG_EXIT();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Feature2OfflineTest::CreateFeature2
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2Base* Feature2OfflineTest::CreateFeature2(
    ChiFeature2CreateInputInfo* pFeature2CreateInputInfo)
{
    CF2_LOG_ENTRY();
    ChiFeature2Base* pFeature2Base = NULL;
    pFeature2Base = ChiFeature2Generic::Create(pFeature2CreateInputInfo);
    CF2_LOG_EXIT();
    return pFeature2Base;
}
