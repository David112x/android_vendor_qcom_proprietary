////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  feature2mfsrtest.h
/// @brief Declarations for MFXR feature testing.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef FEATURE2MFXRTEST_H
#define FEATURE2MFXRTEST_H

#include "feature2testcase.h"
#include "chibufferutils.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

class Feature2MFXRTest : public Feature2TestCase
{
public:

    enum TestId
    {
        TestMFXR
    };

    VOID Setup() override;
    VOID Teardown() override;

    CHAR* GetTestSuiteName() { return(const_cast<CHAR*>(m_testSuiteName));}
    CHAR* GetTestCaseName() { return(const_cast<CHAR*>(m_testCaseName)); }
    CHAR* GetTestFullName() { return(const_cast<CHAR*>(m_testFullName)); }

    VOID MFXRFeatureTest(TestId testId);
    VOID play(UINT id) { TestId testId = static_cast<TestId>(id);  MFXRFeatureTest(testId); };

    // Interface functions
    static VOID InitializeFeature2Test();
    static VOID GetMFXRFeature2Descriptor(ChiFeature2CreateInputInfo* pFeature2CreateInputInfoOut);
    static VOID GetInputFeature2RequestObject(ChiFeature2Base* pFeature2Base, ChiMetadata* pMetadata,
        ChiFeature2RequestObject** ppFeature2RequestObjectOut,
        VOID*                      pPrivateData);
    static CDKResult ProcessMessage(ChiFeature2RequestObject* pFeatureRequestObj, ChiFeature2Messages* pMessages);
    static VOID GetInputForPort(ChiFeature2Base* pFeature2Base, ChiFeature2RequestObject* pFeature2ResultObject);
    static VOID UpdateInputMetadata(ChiFeature2Base* pFeature2Base, ChiFeature2RequestObject* pFeature2ResultObject);
    static ChiFeature2Base* CreateFeature2(ChiFeature2CreateInputInfo* pFeature2CreateInputInfo);

    static const UINT32 MaxNumOutputStreams = 16;

private:

    VOID SetFeature2Interface() override;
    static CDKResult ProcessGetInputDependencyMessage(
        ChiFeature2RequestObject* pFeatureRequestObj,
        ChiFeature2Messages* pMessages);
    static CDKResult ProcessResultNotificationMessage(
        ChiFeature2RequestObject* pFeatureRequestObj,
        ChiFeature2Messages* pMessages);
    static CDKResult ProcessReleaseInputDependencyMessage(
        ChiFeature2RequestObject* pFeatureRequestObj,
        ChiFeature2Messages* pMessages);
    static CDKResult ProcessSubmitRequestMessage(
        ChiFeature2Messages* pMessages);

    static TestId                     m_testId;
    static Feature2MFXRTest*          m_pTestObj;
    static ChiFeature2InstanceProps   m_instanceProps;
    static CHISTREAM*                 m_pInputStream;
    static CHISTREAM*                 m_pOutputStream[];
    static CHISTREAMCONFIGINFO*       m_pStreamConfig;
    static ChiMetadata*               m_pMetadata;
    static CHITARGETBUFFERINFOHANDLE  m_hInputImageBuffer;
    static CHITARGETBUFFERINFOHANDLE  m_hOutputImageBuffer[];
    static CHITARGETBUFFERINFOHANDLE  m_hMetaBuffer;
    static const CHAR*                m_pOutputPortName[];
    static LogicalCameraInfo          m_logicalCameraInfo;
    static CHITargetBufferManager*    m_pInputImageTBM;
    static CHITargetBufferManager*    m_pOutputImageTBM[];
    static CHITargetBufferManager*    m_pMetadataTBM;
    static UINT                       m_inputDepSequence;
    static const CHAR*                m_testSuiteName;
    static const CHAR*                m_testCaseName;
    static const CHAR*                m_testFullName;

};

#endif // FEATURE2MFXRTEST_H
