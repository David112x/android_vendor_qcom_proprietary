////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Copyright (c) 2019 Qualcomm Technologies, Inc.
/// All Rights Reserved.
/// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  feature2replayer.cpp
/// @brief Implementation for feature2 replayer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2test.h"
#include "feature2mfxrtest.h"
#include "feature2offlinetest.h"
#include "spectraconfigparser.h"
#include "metaconfigparser.h"
#include "streamconfigparser.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// main
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
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

    std::string camsim_config = CmdLineParser::GetSimConfigFilePath();
    if (TRUE == camsim_config.empty())
    {
        CF2_LOG_ERROR("Can not load camsim config xml file!");
        return 1;
    }

    if (0 != SpectraConfigParser::ParseSpectraConfigXml(camsim_config.c_str()))
    {
        return 1;
    }

    std::string metaConfigName = SpectraConfigParser::GetMetaConfigName();
    if (TRUE == metaConfigName.empty())
    {
        CF2_LOG_ERROR("Meta config xml not specified!");
    }
    else
    {
        if (std::string::npos == metaConfigName.find("/"))
        {
            metaConfigName = inputImagePath + metaConfigName;
        }

        if (0 != MetaConfigParser::ParseMetaConfigXml(metaConfigName.c_str()))
        {
            CF2_LOG_ERROR("Can not parse Meta config xml!");
        }
    }

    std::string streamConfigName = SpectraConfigParser::GetStreamConfigName();
    if (TRUE == streamConfigName.empty())
    {
        CF2_LOG_ERROR("Stream config xml not specified!");
    }
    else
    {
        if (std::string::npos == streamConfigName.find("/"))
        {
            streamConfigName = inputImagePath + streamConfigName;
        }

        if (0 != StreamConfigParser::ParseStreamConfigXml(streamConfigName.c_str()))
        {
            CF2_LOG_ERROR("Can not parse Stream config xml!");
        }
    }

    std::string featureName = SpectraConfigParser::GetFeatureName();

    ChiFeature2Test* funcObj = NULL;
    UINT             playId  = 0;
    if (0 == featureName.compare("MFXR"))
    {
        funcObj = CF2_NEW Feature2MFXRTest;
        playId  = Feature2MFXRTest::TestId::TestMFXR;
    }
    else if (0 == featureName.compare("BayerToYUV"))
    {
        funcObj = CF2_NEW Feature2OfflineTest;
        playId  = Feature2OfflineTest::TestId::TestBayerToYUV;
    }
    else if (0 == featureName.compare("BPS"))
    {
        funcObj = CF2_NEW Feature2OfflineTest;
        playId  = Feature2OfflineTest::TestId::TestBPS;
    }
    else if (0 == featureName.compare("IPE"))
    {
        funcObj = CF2_NEW Feature2OfflineTest;
        playId = Feature2OfflineTest::TestId::TestIPE;
    }
    else if (0 == featureName.compare("YUV2Jpeg"))
    {
        funcObj = CF2_NEW Feature2OfflineTest;
        playId = Feature2OfflineTest::TestId::TestYUVToJpeg;
    }

    int result = 0;
    if (NULL != funcObj)
    {
        result = RunPlay(funcObj, playId);
        delete funcObj;
    }

    return (result);
}
