////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Copyright (c) 2007-2010, 2015-2019 Qualcomm Technologies, Inc.
/// All Rights Reserved.
/// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2testmain.cpp
/// @brief Implementation for test framework
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2test.h"
#include "feature2offlinetest.h"
#include "feature2realtimetest.h"
#include "feature2mfxrtest.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// main
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // print the title with app version and platform info
#ifdef ENVIRONMENT64 // 64-bit
    CF2_LOG_INFO("===== ChiFeature2Test %s (64-bit %s %s) =====", CF2_VERSION,
        CmdLineParser::GetTargetProduct(), CmdLineParser::GetPlatformVersion());
#else // 32-bit
    CF2_LOG_INFO("===== ChiFeature2Test %s (32-bit %s %s) =====", CF2_VERSION,
        CmdLineParser::GetTargetProduct(), CmdLineParser::GetPlatformVersion());
#endif

    // parse all command line options, print help menu if illegal option provided
    if (-1 == CmdLineParser::ParseCommandLine(argc, argv))
    {
        CmdLineParser::PrintCommandLineUsage();
        return 0;
    }

    // set the verbose level
    verboseSeverity eSev = static_cast<verboseSeverity>(CmdLineParser::GetLogLevel());
    if (-1 == CF2Log.SetLogLevel(eSev))
    {
        CF2_LOG_ERROR("Invalid log level %d", CmdLineParser::GetLogLevel());
        return 1;
    }

    // Run tests
    int result = RunTests();
    return (result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Register tests
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIFEATURE2TEST_TEST(Feature2OfflineTest, TestBayerToYUV)
{
    OfflineFeatureTest(Feature2OfflineTest::TestBayerToYUV);
}

CHIFEATURE2TEST_TEST(Feature2OfflineTest, TestYUVToJpeg)
{
    OfflineFeatureTest(Feature2OfflineTest::TestYUVToJpeg);
}

CHIFEATURE2TEST_TEST(Feature2OfflineTest, TestMultiStage)
{
    OfflineFeatureTest(Feature2OfflineTest::TestMultiStage);
}

CHIFEATURE2TEST_TEST(Feature2RealTimeTest, RealTime)
{
    RealTimeFeatureTest(Feature2RealTimeTest::RealTime);
}

CHIFEATURE2TEST_TEST(Feature2MFXRTest, TestMFXR)
{
    MFXRFeatureTest(Feature2MFXRTest::TestMFXR);
}

CHIFEATURE2TEST_TEST(Feature2OfflineTest, TestBPS)
{
    OfflineFeatureTest(Feature2OfflineTest::TestBPS);
}

CHIFEATURE2TEST_TEST(Feature2OfflineTest, TestIPE)
{
    OfflineFeatureTest(Feature2OfflineTest::TestIPE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//CHIFEATURE2TEST_TEST_INITIALIZE(ChiFeature2TestBase, Initialize)
//{
//
//}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global Shutdown
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//CHIFEATURE2TEST_TEST_SHUTDOWN(ChiFeature2TestBase, Shutdown)
//{
//}
