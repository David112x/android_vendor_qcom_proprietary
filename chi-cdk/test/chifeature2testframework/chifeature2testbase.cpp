////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Copyright (c) 2019 Qualcomm Technologies, Inc.
/// All Rights Reserved.
/// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2testbase.cpp
/// @brief Test for Feature2 Base Class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "chifeature2testbase.h"
#include "chxextensionmodule.h"
#include "chifeature2generic.cpp"
#include "chxutils.h"

extern const ChiFeature2InstanceProps Bayer2YUVInstanceProps;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2TestBase::ChiFeature2TestBase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2TestBase::ChiFeature2TestBase(ChiFeature2Test* pTest):
	m_pTest(pTest),
	m_pMetadataManager(NULL)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2TestBase::~ChiFeature2TestBase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2TestBase::~ChiFeature2TestBase()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2TestBase::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2TestBase::Initialize(
    Feature2Type feature2Type)
{
    CHX_LOG_INFO("Result: Initialize in");
    CDKResult result = CDKResultSuccess;

    m_feature2Type = feature2Type;

    result = CreateMetadataManager();

    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess == result);

    //result = createOutputTargetBufferManager(); //TODO: placeholder to replace when proper definition is available.
    //CHIFEATURE2TEST_CHECK2(m_pTest, CamxResultSuccess == result);
    m_pFeature2RequestStateMutex    = Mutex::Create();
    m_pFeature2RequestStateComplete = Condition::Create();
    CHX_LOG_INFO("Result: Initialize out");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2TestBase::CreateMetadataManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2TestBase::CreateMetadataManager()
{
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
                DefaultNumMetadataBuffers,
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

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiFeature2TestBase::Uninitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2TestBase::Uninitialize()
{
    if (NULL != m_pMetadataManager)
    {
        m_pMetadataManager->Destroy();
        m_pMetadataManager = NULL;
    }

    if (NULL != m_pFeature2RequestStateMutex)
    {
        m_pFeature2RequestStateMutex->Destroy();
    }

    if (NULL != m_pFeature2RequestStateComplete)
    {
        m_pFeature2RequestStateComplete->Destroy();
    }
    //TODO: Destroy buffer manager
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiFeature2TestBase::GetFeature2Descriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2TestBase::GetFeature2Descriptor(
     ChiFeature2CreateInputInfo* pFeature2CreateInputInfoOut)
{
    /*
     pCameraInfo;
     pStreamConfig;
     pFeatureDescriptor;

     pUsecaseDescriptor;
    */
    (void)pFeature2CreateInputInfoOut;
    switch (m_feature2Type)
    {
        case Bayer2Yuv:
#if 0
            CHX_LOG_INFO("Result: GetFeature2Descriptor in");
            //TODO: either make one file of multiple input tables or include input tables for different features
            pFeature2CreateInputInfoOut->pFeatureDescriptor = const_cast<ChiFeature2Descriptor*>(&Bayer2YuvUnitTestFeatureDescriptor);
            pFeature2CreateInputInfoOut->pInstanceProps     = &Bayer2YUVInstanceProps;
            pFeature2CreateInputInfoOut->pCameraInfo        = const_cast<LogicalCameraInfo*> (ExtensionModule::GetInstance()->GetPhysicalCameraInfo(0));
            pFeature2CreateInputInfoOut->pUsecaseDescriptor = g_pUsecaseSAT;
            pFeature2CreateInputInfoOut->pStreamConfig      = &Bayer2YuvStreamConfigInfo;
            CHX_LOG_INFO("Result: GetFeature2Descriptor out");
#endif
            break;
        case MFNR:
            break;
        case HDR:
            break;
        default:
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiFeature2TestBase::GetInputFeature2RequestObject
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2TestBase::GetInputFeature2RequestObject(
    ChiFeature2Base*            pFeature2Base,
    ChiFeature2RequestObject**  ppFeature2RequestObjectOut,
    VOID*                       pPrivateData)
{
    ChiFeature2RequestObjectCreateInputInfo feature2RequestObjInputInfo = {0};
    ChiFeature2RequestOutputInfo            featureRequestOutputInfo    = {0};

    (void)pFeature2Base;
    (void)ppFeature2RequestObjectOut;
    switch (m_feature2Type)
    {
        case Bayer2Yuv:
#if 0
            featurePortDescriptorInfo.bufferMetadataInfo.hMetadata              =
                m_pMetadataManager->Get(m_genericMetadataClientId, 0)->GetHandle();
            featurePortDescriptorInfo.bufferMetadataInfo.numTargetBufferHandle  = 1;
            featurePortDescriptorInfo.bufferMetadataInfo.pTargetBufferHandle    = NULL; //TODO: Update it when proper definition is available
            featurePortDescriptorInfo.pPortDescriptor                      = &Bayer2YuvOutputPortDescriptors[0];

            feature2RequestObjInputInfo.pUsecaseReqObj                      = NULL; //TODO: Not required in first phase
            feature2RequestObjInputInfo.pGraph                              = NULL; //TODO: Not required in first phase
            feature2RequestObjInputInfo.pFeatureBase                        = pFeature2Base;
            feature2RequestObjInputInfo.numRequestOutput                    = 1;
            feature2RequestObjInputInfo.pRequestOutput                      = &featureRequestOutputInfo;
#endif
            break;
        case MFNR:
            break;
        case HDR:
            break;
        default:
            break;
    }

    CHIFEATURE2TEST_CHECK2(m_pTest, (NULL != ppFeature2RequestObjectOut));

    // TODO: Placeholder until properly defined.
    *ppFeature2RequestObjectOut = ChiFeature2RequestObject::Create(&feature2RequestObjInputInfo);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiFeature2TestBase::RunTestFeature
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2TestBase::RunTestFeature2(
    Feature2Type feature2Type)
{
    ChiFeature2CreateInputInfo feature2CreateInputInfo;
    ChiFeature2Base *          pFeature2Base;
    ChiFeature2RequestObject*  pFeature2RequestObject;

    ChiFeature2RequestState requestState = ChiFeature2RequestState::Initialized;

    Initialize(feature2Type);

    GetFeature2Descriptor(&feature2CreateInputInfo);

    // Fail test if feature descriptor is NULL
    CHIFEATURE2TEST_CHECK2(m_pTest,
                       (NULL != feature2CreateInputInfo.pFeatureDescriptor));

    pFeature2Base = ChiFeature2Generic::Create(&feature2CreateInputInfo);

    // Fail test if feature instance is NULL
    CHIFEATURE2TEST_CHECK2(m_pTest,
                       NULL != pFeature2Base);
    Uninitialize();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiFeature2TestBase::ChiFeature2TestCbInputRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2TestBase::ChiFeature2TestCbInputRequest(
    ChiFeature2RequestObject*   pFeature2RequestObject,
    VOID*                       pPrivateCallbackData)
{
    ChiFeature2RequestState requestState    = pFeature2RequestObject->GetCurRequestState();
    ///TODO: update it correctly when full information is available
    ChiFeature2TestBase*    pTest       = static_cast<ChiFeature2TestBase*> (pPrivateCallbackData);

    switch (requestState)
    {
    case ChiFeature2RequestState::InputResourcePending:
        //TODO: read RAW buffer file included in this project, create RAW buffer (TDev team can help if required)
        // Create some metadata and update feature object
        break;
    case ChiFeature2RequestState::Complete:
        pTest->m_pFeature2RequestStateMutex->Lock();
        pTest->m_pFeature2RequestStateComplete->Signal();
        pTest->m_pFeature2RequestStateMutex->Unlock();
        break;
    default:
        break;
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Register test
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIFEATURE2TEST_TEST_ORDERED(TestFeatureBayer2Yuv, 1)
{
    ChiFeature2TestBase feature2TestBase(this);

//    featureUnitTest.RunTestFeature2(featureUnitTest.Bayer2Yuv);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Register test
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIFEATURE2TEST_TEST_ORDERED(TestFeatureMFNR, 2)
{
    ChiFeature2TestBase feature2TestBase(this);

    feature2TestBase.RunTestFeature2(feature2TestBase.MFNR);
}
