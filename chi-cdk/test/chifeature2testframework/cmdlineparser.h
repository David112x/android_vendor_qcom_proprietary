////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  cmdlineparser.h
/// @brief Declarations of command line parser.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2_CMDPARSER_H
#define CHIFEATURE2_CMDPARSER_H

#include <stdint.h>
#include <string>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

//=================================================================================================
// Class Definition
//=================================================================================================
class CmdLineParser
{
    public:

         CmdLineParser() = default;
        ~CmdLineParser() = default;

        /// Do not allow the copy constructor or assignment operator
        CmdLineParser(const CmdLineParser&) = delete;
        CmdLineParser& operator= (const CmdLineParser&) = delete;

        static int  ParseCommandLine(int argc, char** argv);
        static void PrintCommandLineUsage();

        static const char* GetTargetProduct();
        static const char* GetPlatformVersion();

        static int GetLogLevel()
        {
            return m_nLog;
        }
        static bool GenerateReport()
        {
            return m_bReport;
        }
        static int GetFrameCount()
        {
            return m_nFrames;
        }
        static bool PrintHelp()
        {
            return m_bHelp;
        }
        static bool ListTests()
        {
            return m_bListTests;
        }
        static int GetCamera()
        {
            return m_nCamera;
        }
        static std::string GetTestSuiteName()
        {
            return m_suiteName;
        }
        static std::string GetTestCaseName()
        {
            return m_caseName;
        }
        static std::string GetTestFullName()
        {
            return m_fullName;
        }
        static std::string GetSimConfigFilePath()
        {
            return m_SpectraCamSimConfig;
        }

    private:
        static int   m_nLog;
        static bool  m_bHelp;
        static bool  m_bListTests;
        static bool  m_bReport;
        static int   m_nFrames;
        static int   m_nCamera;
        static std::string m_suiteName;
        static std::string m_caseName;
        static std::string m_fullName;
        static std::string m_SpectraCamSimConfig;
};

#endif // CHIFEATURE2_CMDPARSER_H
