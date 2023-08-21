////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  feature2realtimetest.cpp
/// @brief Implementations for realtime feature testing.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "feature2realtimetest.h"
#include "chifeature2realtime.h"
#include "chifeature2descriptors.h"

#ifndef _LINUX
#include <direct.h> // _mkdir
#endif

// Initialize static variables
Feature2RealTimeTest::TestId Feature2RealTimeTest::m_testId;
LogicalCameraInfo            Feature2RealTimeTest::m_logicalCameraInfo;

static CHISTREAMPARAMS StreamParams1
{
    0,
    0
};

static CHISTREAM RealTimeStreamsOutput1
{
    ChiStreamTypeOutput,
    1920,
    1440,
    ChiStreamFormatImplDefined,
    0,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
#if (CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // Android-P or better
    NULL,
#endif // CAMX_ANDROID_API && CAMX_ANDROID_API >= 28
    NULL,

    StreamParams1,
    { NULL, NULL, NULL },
};
static CHISTREAM RealTimeStreamsOutput2
{
    ChiStreamTypeOutput,
    4096,
    3072,
    ChiStreamFormatRaw16,
    0,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
#if (CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // Android-P or better
    NULL,
#endif // CAMX_ANDROID_API && CAMX_ANDROID_API >= 28
    NULL,
    StreamParams1,
    { NULL, NULL, NULL },
};
static CHISTREAM RealTimeStreamsOutput3
{
    ChiStreamTypeOutput,
    640,
    480,
    ChiStreamFormatYCbCr420_888,
    0,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
#if (CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // Android-P or better
    NULL,
#endif // CAMX_ANDROID_API && CAMX_ANDROID_API >= 28
    NULL,
    StreamParams1,
    { NULL, NULL, NULL },
};

static CHISTREAM RealTimeStreamsOutput4
{
    ChiStreamTypeOutput,
    1920,
    1080,
    ChiStreamFormatImplDefined,
    GRALLOC_USAGE_HW_VIDEO_ENCODER,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
#if (CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // Android-P or better
    NULL,
#endif // CAMX_ANDROID_API && CAMX_ANDROID_API >= 28
    NULL,
    StreamParams1,
    { NULL, NULL, NULL },
};

static CHISTREAM* chistreamArray[]
{
    &RealTimeStreamsOutput1,
    &RealTimeStreamsOutput2,
    &RealTimeStreamsOutput3,
    &RealTimeStreamsOutput4,
};

static CHISTREAMCONFIGINFO RealTimeStreamConfigInfo
{
    4,
    &chistreamArray[0],
    0,
    NULL
};

VOID Feature2RealTimeTest::Setup()
{
    CF2_LOG_ENTRY();

    // Parent class setup
    Feature2TestCase::Setup();

    CF2_LOG_EXIT();
}

VOID Feature2RealTimeTest::Teardown()
{
    CF2_LOG_ENTRY();

    // Parent class teardown
    Feature2TestCase::Teardown();

    CF2_LOG_EXIT();
}

VOID Feature2RealTimeTest::SetFeature2Interface()
{
    CHX_LOG_ERROR("E");
    m_feature2Interface.pInitializeFeature2Test         = Feature2RealTimeTest::InitializeFeature2Test;
    m_feature2Interface.pGetFeature2Descriptor          = Feature2RealTimeTest::GetRealTimeFeature2Descriptor;
    m_feature2Interface.pGetInputFeature2RequestObject  = Feature2RealTimeTest::GetInputFeature2RequestObject;
    m_feature2Interface.pProcessMessage                 = Feature2RealTimeTest::ProcessMessage;
    m_feature2Interface.pGetInputForPort                = Feature2RealTimeTest::GetInputForPort;
    m_feature2Interface.pUpdateInputMetadata            = Feature2RealTimeTest::UpdateInputMetadata;
    m_feature2Interface.pCreateFeature2                 = Feature2RealTimeTest::CreateFeature2;
    CHX_LOG_ERROR("X");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Feature2RealTimeTest::RealTimeFeatureTest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2RealTimeTest::RealTimeFeatureTest(TestId testId)
{
    CF2_LOG_ENTRY();
    CHX_LOG_ERROR("E");
    m_testId = testId;

    // Set interface in parent class
    SetFeature2Interface();

    // Run the test
    RunFeature2Test();
    CHX_LOG_ERROR("X");
    CF2_LOG_EXIT();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Feature2RealTimeTest::InitializeFeature2Test
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2RealTimeTest::InitializeFeature2Test()
{
    CF2_LOG_ENTRY();
    CHX_LOG_ERROR("E");

    CHX_LOG_ERROR("X");
    CF2_LOG_EXIT();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Feature2RealTimeTest::GetGenericFeature2Descriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2RealTimeTest::GetRealTimeFeature2Descriptor(
    ChiFeature2CreateInputInfo* pFeature2CreateInputInfoOut)
{
    CF2_LOG_ENTRY();
    m_logicalCameraInfo = { 0 };

    CHX_LOG_ERROR("Result: GetFeature2Descriptor in");
    m_logicalCameraInfo.cameraId           = 0;
    m_logicalCameraInfo.numPhysicalCameras = 1;
    //TODO get it from here :CHXExtensionModule::GetPhysicalCameraInfo(0);


    //TODO: either make one file of multiple input tables or include input tables for different features
    pFeature2CreateInputInfoOut->pFeatureDescriptor = const_cast<ChiFeature2Descriptor*>(&RealTimeFeatureDescriptor);
    pFeature2CreateInputInfoOut->pCameraInfo = &m_logicalCameraInfo; /// TODO check when extension module dependancy is added const_cast<LogicalCameraInfo*> (ExtensionModule::GetInstance()->GetPhysicalCameraInfo(0));
    CHX_LOG_ERROR("Result: GetFeature2Descriptor out numvam %d", pFeature2CreateInputInfoOut->pCameraInfo->numPhysicalCameras);
    pFeature2CreateInputInfoOut->pUsecaseDescriptor = g_pUsecaseZSL;//g_pUsecaseSAT;
    pFeature2CreateInputInfoOut->pStreamConfig = &RealTimeStreamConfigInfo;
    pFeature2CreateInputInfoOut->bFrameworkVideoStream = TRUE;
    CHX_LOG_ERROR("Result: GetFeature2Descriptor out");

    CF2_LOG_EXIT();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Feature2RealTimeTest::GetInputFeature2RequestObject
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2RealTimeTest::GetInputFeature2RequestObject(
    ChiFeature2Base*            pFeature2Base,
    ChiMetadata*                pMetadata,
    ChiFeature2RequestObject**  ppFeature2RequestObjectOut,
    VOID*                       pPrivateData)
{
    CF2_LOG_ENTRY();
    CHX_LOG_ERROR("E");

    ChiFeature2RequestObjectCreateInputInfo feature2RequestObjInputInfo = { 0 };
    ChiFeature2RequestOutputInfo            featureRequestOutputInfo[3] = { {0} };
    UINT8                                   numExternalOutputPorts      = 0;

    // Create usecaseRequestObject with frame number
    camera3_capture_request_t halRequest = { 0 };
    halRequest.frame_number = ++m_frameNumber;
    ChiFeature2UsecaseRequestObjectCreateInputInfo usecaseRequestObjectCreateInputInfo = { 0 };
    usecaseRequestObjectCreateInputInfo.pRequest = &halRequest;
    usecaseRequestObjectCreateInputInfo.pAppSettings = pMetadata;
    ChiFeature2UsecaseRequestObject* pUsecaseRequestObj = ChiFeature2UsecaseRequestObject::Create(
        &usecaseRequestObjectCreateInputInfo);

    feature2RequestObjInputInfo.pUsecaseRequestObj      = pUsecaseRequestObj;
    feature2RequestObjInputInfo.pGraphPrivateData       = pPrivateData; //TODO: Not required in first phase
    feature2RequestObjInputInfo.pFeatureBase            = pFeature2Base;

    std::vector<ChiFeature2Identifier> identifiers = pFeature2Base->GetExternalGlobalPortIdList();

    for (UINT8 portIndex = 0; portIndex < identifiers.size(); portIndex++)
    {
        if (ChiFeature2PortDirectionType::ExternalOutput == identifiers[portIndex].portDirectionType)
        {
            featureRequestOutputInfo[numExternalOutputPorts].pPortDescriptor =
                pFeature2Base->GetPortDescriptorFromPortId(&identifiers[portIndex]);
            numExternalOutputPorts++;
        }
    }

    // New Request Output information Format Dummy implementation
    // We are hardcoding number of Request as 1
    ChiFeature2RequestMap requestTable[1]       = {{0}};
    feature2RequestObjInputInfo.numRequests     = 1;
    feature2RequestObjInputInfo.pRequestTable   = requestTable;

    // Since there is only 1 request we are directly indexing as 0
    requestTable[0].requestIndex                        = 0;
    requestTable[0].numRequestOutputs                   = numExternalOutputPorts;
    ChiFeature2RequestOutputInfo *pRequestOutputs   = static_cast<ChiFeature2RequestOutputInfo*>(
        CHX_CALLOC(sizeof(ChiFeature2RequestOutputInfo) * numExternalOutputPorts));
    if (NULL != pRequestOutputs)
    {
        for (UINT portIndex = 0; portIndex < numExternalOutputPorts; portIndex++)
        {
            pRequestOutputs[portIndex] = featureRequestOutputInfo[portIndex];
        }
    }

    requestTable[0].pRequestOutputs = pRequestOutputs;

    *ppFeature2RequestObjectOut = ChiFeature2RequestObject::Create(&feature2RequestObjInputInfo);
    CHX_LOG_ERROR("X");
    CF2_LOG_EXIT();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Feature2RealTimeTest::ProcessMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Feature2RealTimeTest::ProcessMessage(
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
        VOID* pGraphPrivateData = NULL;
        pFeatureRequestObj->GetGraphPrivateData(&pGraphPrivateData);
        Feature2TestCase* pFeature2TestCase = reinterpret_cast<Feature2TestCase*>(pGraphPrivateData);
        if (NULL != pMessages->pFeatureMessages)
        {
            switch (pMessages->pFeatureMessages->messageType)
            {
            case ChiFeature2MessageType::GetInputDependency:
            {
                break;
            }

            case ChiFeature2MessageType::ResultNotification:
            {
                int native_handle = 0;
                void *pHostptr = NULL;
                // calculate appropriately for different type image formats. For Ex: yuv: w*h*1.5
                int bufferLength = 0;
                FILE* fd = NULL;
                CHAR dumpFilename[256];

                ChiFeature2BufferMetadataInfo* pOutputBufferMetaInfo = NULL;
                for (UINT i = 0; i < pMessages->pFeatureMessages->message.result.numPorts; ++i)
                {
                    ChiFeature2Identifier portidentifier =
                        pMessages->pFeatureMessages->message.result.pPorts[i];

                    pFeatureRequestObj->SetOutputNotifiedForPort(portidentifier, 0);

                    pFeatureRequestObj->GetFinalBufferMetadataInfo(portidentifier, &pOutputBufferMetaInfo, 0);

                    CHISTREAMBUFFER outputBuffer;

                    CHISTREAMBUFFER* pTargetBuffer = NULL;
                    CHITargetBufferManager* pTargetBufferManager = CHITargetBufferManager::GetTargetBufferManager(
                        pOutputBufferMetaInfo->hBuffer);

                    if (NULL != pTargetBufferManager)
                    {
                        pTargetBuffer = reinterpret_cast<CHISTREAMBUFFER*>(pTargetBufferManager->GetTarget(
                            pOutputBufferMetaInfo->hBuffer, pOutputBufferMetaInfo->key));

                        if (pTargetBuffer != NULL)
                        {
                            ChxUtils::Memcpy(&outputBuffer, pTargetBuffer, sizeof(CHISTREAMBUFFER));
                        }
                        else
                        {
                            CHX_LOG_ERROR("Unable to get buffer info for handle %p", pOutputBufferMetaInfo->hBuffer);
                            result = CDKResultENoSuch;
                        }
                    }

                    const CHISTREAMBUFFER* pBuffer = &outputBuffer;
                    native_handle = CHIBufferManager::GetFileDescriptor(&pBuffer->bufferInfo);
                    if (ChiStreamFormatYCbCr420_888 == pBuffer->pStream->format)
                    {
                        bufferLength = pBuffer->pStream->width * pBuffer->pStream->height * 1.5;
                    }
                    else if (ChiStreamFormatRaw16 == pBuffer->pStream->format)
                    {
                        bufferLength = pBuffer->pStream->width * pBuffer->pStream->height * 2;
                    }
                    if (0 != native_handle)
                    {
                        pHostptr = ChxUtils::MemMap(native_handle, bufferLength, 0);
                        if ((pHostptr != NULL))
                        {
                            // Ensure output capture folder gets created or exists
#ifdef _LINUX
                            if (mkdir(outputImagePath.c_str(), 0777) != 0 && EEXIST != errno)
#else // _WINDOWS
                            if (_mkdir(outputImagePath.c_str()) != 0 && EEXIST != errno)
#endif
                            {
                                CF2_EXPECT(EEXIST == errno, "Failed to create capture folder! Error: %d", errno);
                                return CDKResultEFailed;
                            }
                            UINT frameworkFrameNum = pFeatureRequestObj->GetProcessSequenceId(
                                ChiFeature2SequenceOrder::Current, 0);

                            if (ChiStreamFormatYCbCr420_888 == pBuffer->pStream->format)
                            {
                                CdkUtils::SNPrintF(dumpFilename, sizeof(dumpFilename), "%syuv_dump_at_chi_%d_%d_%d.yuv",
                                    outputImagePath.c_str(),
                                    pBuffer->pStream->width,
                                    pBuffer->pStream->height,
                                    frameworkFrameNum);
                            }
                            else if (ChiStreamFormatRaw16 == pBuffer->pStream->format)
                            {
                                CdkUtils::SNPrintF(dumpFilename, sizeof(dumpFilename), "%sraw_dump_at_chi_%d_%d_%d.raw",
                                    outputImagePath.c_str(),
                                    pBuffer->pStream->width,
                                    pBuffer->pStream->height,
                                    frameworkFrameNum);
                            }

                            fd = CdkUtils::FOpen(dumpFilename, "w");
                            if (NULL == fd)
                            {
                                CF2_LOG_ERROR("fd is NULL!");
                                break;
                            }
                            CdkUtils::FWrite(pHostptr, sizeof(char), bufferLength, fd);
                            ChxUtils::MemUnmap(pHostptr, bufferLength);
                            CdkUtils::FClose(fd);

                            CF2_LOG_INFO("Saved result image at %s", dumpFilename);
                        }
                    }
                }
                //TODO if we receive output, change FeatureRequestObject's state to output notified and signal the thread for next process request
                break;
            }

            case ChiFeature2MessageType::ReleaseInputDependency:
            {
                // Clean up input buffer
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
                break;
            }
        }
    }

    CF2_LOG_EXIT();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Feature2RealTimeTest::ProcessSubmitRequestMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Feature2RealTimeTest::ProcessSubmitRequestMessage(
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
//  Feature2RealTimeTest::GetInputForPort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2RealTimeTest::GetInputForPort(
    ChiFeature2Base*            pFeature2Base,
    ChiFeature2RequestObject*   pFeature2ResultObject)
{
    CF2_LOG_ENTRY();
    CHX_LOG_ERROR("E");
    UNUSED_PARAM(pFeature2Base);
    UNUSED_PARAM(pFeature2ResultObject);
    CHX_LOG_ERROR("X");
    CF2_LOG_EXIT();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Feature2RealTimeTest::UpdateInputMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2RealTimeTest::UpdateInputMetadata(
    ChiFeature2Base*            pFeature2Base,
    ChiFeature2RequestObject*   pFeature2ResultObject)
{
    CF2_LOG_ENTRY();
    CHX_LOG_ERROR("E");
    UNUSED_PARAM(pFeature2Base);
    UNUSED_PARAM(pFeature2ResultObject);
    CHX_LOG_ERROR("X");
    CF2_LOG_EXIT();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Feature2RealTimeTest::CreateFeature2
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2Base* Feature2RealTimeTest::CreateFeature2(
    ChiFeature2CreateInputInfo* pFeature2CreateInputInfo)
{
    CF2_LOG_ENTRY();
    CHX_LOG_ERROR("E");
    ChiFeature2Base* pFeature2Base = NULL;
    pFeature2Base = ChiFeature2RealTime::Create(pFeature2CreateInputInfo);
    CHX_LOG_ERROR("X");
    CF2_LOG_EXIT();
    return pFeature2Base;
}
