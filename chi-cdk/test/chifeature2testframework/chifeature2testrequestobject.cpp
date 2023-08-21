////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Copyright (c) 2019 Qualcomm Technologies, Inc.
/// All Rights Reserved.
/// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2testrequestobject.cpp
/// @brief Test for Feature2 Request Object Class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "chifeature2testrequestobject.h"
#include "chifeature2requestobject.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2TestRequestObject::ChiFeature2TestRequestObject
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2TestRequestObject::ChiFeature2TestRequestObject(ChiFeature2Test* pTest):
    m_pTest(pTest)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiFeature2TestRequestObject::RunInitAndDestroyTests
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ChiFeature2TestRequestObject::RunInitAndDestroyTests()
{

    ChiFeature2RequestObject* pRequestInfo = NULL;

    pRequestInfo =  ChiFeature2RequestObject::Create(NULL);
    CHIFEATURE2TEST_CHECK2(m_pTest, NULL == pRequestInfo);

    ChiFeature2RequestObjectCreateInputInfo inputInfo;
    pRequestInfo = ChiFeature2RequestObject::Create(&inputInfo);
    CHIFEATURE2TEST_CHECK2(m_pTest, NULL != pRequestInfo);
    CHIFEATURE2TEST_CHECK2(m_pTest, ChiFeature2RequestState::Initialized == pRequestInfo->GetCurRequestState());

    INT32 processSequenceId = 0;
    processSequenceId = pRequestInfo->GetProcessSequenceId(ChiFeature2SequenceOrder::Current);
    CHIFEATURE2TEST_CHECK2(m_pTest, 0 == processSequenceId);
    processSequenceId = pRequestInfo->GetProcessSequenceId(ChiFeature2SequenceOrder::Next);
    CHIFEATURE2TEST_CHECK2(m_pTest, InvalidProcessSequenceId == processSequenceId);
    CHIFEATURE2TEST_CHECK2(m_pTest, NULL == pRequestInfo->GetPrivContext());
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess != pRequestInfo->MoveToNextProcessSequenceInfo());
    pRequestInfo->SetMaxSequenceInfo(3);
#if 0
    ChiFeature2PortDescriptor portDescriptor;
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess != pRequestInfo->SetDescriptor(ChiFeature2SequenceOrder::Current,
        ChiFeature2RequestObjectOpsType::InputConfiguration,
        &portDescriptor,
        1));
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess != pRequestInfo->SetDescriptor(ChiFeature2SequenceOrder::Next,
        ChiFeature2RequestObjectOpsType::InputConfiguration,
        &portDescriptor,
        1));

        const ChiFeature2PortDescriptor*    pPortDescriptor = NULL;
        UINT32                              numDescriptor   = 0;
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess != pRequestInfo->GetDescriptor(ChiFeature2SequenceOrder::Next,
        ChiFeature2RequestObjectOpsType::InputConfiguration,
        &pPortDescriptor,
        &numDescriptor));

    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess != pRequestInfo->GetDescriptor(ChiFeature2SequenceOrder::Current,
        ChiFeature2RequestObjectOpsType::InputConfiguration,
        &pPortDescriptor,
        &numDescriptor));

    ChiFeature2BufferMetadataInfo bufferMetadataInfo;
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess != pRequestInfo->SetBufferMetadataInfo(ChiFeature2RequestObjectOpsType::InputDependency,
        1,
        &bufferMetadataInfo));

    pRequestInfo->Destroy();
    CHIUNITTEST_CHECK2(m_pUnitTest, ChiFeature2RequestState::InvalidMax == pRequestInfo->GetCurRequestState());
#endif
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiFeature2TestRequestObject::RunBasicCallFlow
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ChiFeature2TestRequestObject::RunBasicCallFlow()
{
#if 0
    VOID* pRequestMemory = static_cast<VOID*>(CHX_CALLOC(50));

    ChiFeature2RequestObject*               pRequestInfo = NULL;
    ChiFeature2RequestObjectCreateInputInfo inputInfo;

    pRequestInfo = ChiFeature2RequestObject::Create(&inputInfo);

    // Verify that our State is Initialized
    CHIFEATURE2TEST_CHECK2(m_pTest, ChiFeature2RequestState::Initialized == pRequestInfo->GetCurRequestState());

    // Set Private Context
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess == pRequestInfo->SetPrivContext(pRequestMemory));

    //Get Private Context
    CHIFEATURE2TEST_CHECK2(m_pTest, pRequestMemory == pRequestInfo->GetPrivContext());

    // Verify State Transition
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess == pRequestInfo->SetCurRequestState(ChiFeature2RequestState::ReadyToExecute));

    // Verify State Transition
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess == pRequestInfo->SetCurRequestState(ChiFeature2RequestState::Executing));

    // Verify State Transition
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess == pRequestInfo->SetCurRequestState(ChiFeature2RequestState::InputResourcePending));

    // Verify State Transition
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess == pRequestInfo->SetCurRequestState(ChiFeature2RequestState::ReadyToExecute));

    // Verify State Transition
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess == pRequestInfo->SetCurRequestState(ChiFeature2RequestState::Executing));

    // Verify State Transition
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess == pRequestInfo->SetCurRequestState(ChiFeature2RequestState::OutputResourcePending));

    // Verify State Transition
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess == pRequestInfo->SetCurRequestState(ChiFeature2RequestState::ReadyToExecute));

    // Verify State Transition
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess == pRequestInfo->SetCurRequestState(ChiFeature2RequestState::Executing));

    //Set Stage Info for next Sequence
    ChiFeature2StageInfo stageInfo;
    stageInfo.stageId           = 1;
    stageInfo.stageSequenceId   = 9;

    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess != pRequestInfo->SetStageInfo(ChiFeature2SequenceOrder::Current, &stageInfo));
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess == pRequestInfo->SetStageInfo(ChiFeature2SequenceOrder::Next, &stageInfo));

    CHIFEATURE2TEST_CHECK2(m_pTest, InvalidProcessSequenceId != pRequestInfo->GetProcessSequenceId(ChiFeature2SequenceOrder::Next));
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess == pRequestInfo->MoveToNextProcessSequenceInfo());
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess == pRequestInfo->GetStageInfo(ChiFeature2SequenceOrder::Current,
        &stageInfo));
    CHIFEATURE2TEST_CHECK2(m_pTest, 1 == stageInfo.stageId);
    CHIFEATURE2TEST_CHECK2(m_pTest, 9 == stageInfo.stageSequenceId);

    stageInfo.stageId           = 1;
    stageInfo.stageSequenceId   = 10;
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess == pRequestInfo->SetStageInfo(ChiFeature2SequenceOrder::Next, &stageInfo));
    CHIFEATURE2TEST_CHECK2(m_pTest, InvalidProcessSequenceId != pRequestInfo->GetProcessSequenceId(ChiFeature2SequenceOrder::Next));
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess == pRequestInfo->MoveToNextProcessSequenceInfo());

    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess == pRequestInfo->GetStageInfo(ChiFeature2SequenceOrder::Current,
        &stageInfo));

    CHIFEATURE2TEST_CHECK2(m_pTest, 1 == stageInfo.stageId);
    CHIFEATURE2TEST_CHECK2(m_pTest, 10 == stageInfo.stageSequenceId);

    ChiFeature2PortDescriptor portDescriptor;
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess == pRequestInfo->SetDescriptor(ChiFeature2SequenceOrder::Current,
        ChiFeature2RequestObjectOpsType::InputConfiguration,
        &portDescriptor,
        1));

    // Verify State Transition
    CHIFEATURE2TEST_CHECK2(m_pTest, CDKResultSuccess == pRequestInfo->SetCurRequestState(ChiFeature2RequestState::Complete));
#endif
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Register test
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIFEATURE2TEST_TEST_ORDERED(TestFeature2RequestObject, 3)
{

    ChiFeature2TestRequestObject chiFeature2TestRequestObject(this);

    chiFeature2TestRequestObject.RunInitAndDestroyTests();
//    chiFeature2TestRequestObject.RunBasicCallFlow();
}
