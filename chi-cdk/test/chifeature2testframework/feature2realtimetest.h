////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  feature2realtimetest.h
/// @brief Declarations for realtime feature testing.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef FEATURE2REALTIMETEST_H
#define FEATURE2REALTIMETEST_H

#include "feature2testcase.h"

class Feature2RealTimeTest : public Feature2TestCase
{
public:

    enum TestId
    {
        RealTime
    };

    VOID Setup() override;
    VOID Teardown() override;

    VOID RealTimeFeatureTest(TestId testId);

    // Interface functions
    static VOID InitializeFeature2Test();
    static VOID GetRealTimeFeature2Descriptor(ChiFeature2CreateInputInfo* pFeature2CreateInputInfoOut);
    static VOID GetInputFeature2RequestObject(ChiFeature2Base* pFeature2Base, ChiMetadata* pMetadata,
        ChiFeature2RequestObject** ppFeature2RequestObjectOut,
        VOID*                      pPrivateData);
    static CDKResult ProcessMessage(ChiFeature2RequestObject* pFeatureRequestObj, ChiFeature2Messages* pMessages);
    static VOID GetInputForPort(ChiFeature2Base* pFeature2Base, ChiFeature2RequestObject* pFeature2ResultObject);
    static VOID UpdateInputMetadata(ChiFeature2Base* pFeature2Base, ChiFeature2RequestObject* pFeature2ResultObject);
    static ChiFeature2Base* CreateFeature2(ChiFeature2CreateInputInfo* pFeature2CreateInputInfo);

private:

    VOID SetFeature2Interface() override;
    static CDKResult ProcessSubmitRequestMessage(
        ChiFeature2Messages* pMessages);

    static TestId m_testId;
    static LogicalCameraInfo m_logicalCameraInfo;

};

#endif // FEATURE2REALTIMETEST_H