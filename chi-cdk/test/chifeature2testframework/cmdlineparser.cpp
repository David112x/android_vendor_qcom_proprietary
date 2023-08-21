////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  cmdlineparser.cpp
/// @brief Implementation of command line parser.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>
#include "cmdlineparser.h"
#include "xmlparser.h"

// static Definitions
std::string CmdLineParser::m_suiteName           = "";
std::string CmdLineParser::m_caseName            = "";
std::string CmdLineParser::m_fullName            = "";
std::string CmdLineParser::m_SpectraCamSimConfig = "";
int    CmdLineParser::m_nLog        = 3;
bool   CmdLineParser::m_bHelp       = false;
bool   CmdLineParser::m_bListTests  = false;
bool   CmdLineParser::m_bReport     = false;
int    CmdLineParser::m_nFrames     = 20;
int    CmdLineParser::m_nCamera     = -1;   // -1 means --camera flag not set; select all cameras for testing

/**************************************************************************************************
*   ParseCommandLine
*
*   @brief
*       Parse the command line using GNU Getopt feature
*   @return
*       0 on success, -1 on failure
**************************************************************************************************/
int CmdLineParser::ParseCommandLine(
    int argc,       ///< [in] the total number of arguments
    char** argv)    ///< [in] array of argument values
{
    int getOptReturn;

    // Parse all command line flag options with '-' and '--'
    while (true)
    {
        int optionIndex = 0;

        static struct option cmdLineOptions[] =
        {
            // option,      has_arg,            flag,   value/alias
            { "help",       no_argument,        0,      'h' },
            { "list",       no_argument,        0,      'l' },
            { "test",       required_argument,  0,      't' },
            { "verbose",    required_argument,  0,      'v' },
            { "report",     no_argument,        0,      'r' }, // TODO: Link it to reports printing
            { "frames",     required_argument,  0,      'f' }, // TODO: Link to tests logic
            { "camera",     required_argument,  0,      'c' }, // TODO: Link to tests logic
            { "xml",        required_argument,  0,      'x' },
            { 0,            0,        0,      0  } //last element is 0 to mark ending
        };

        getOptReturn = getopt_long(argc, argv, "hlt:v:rf:c:x:0", cmdLineOptions, &optionIndex);

        if (getOptReturn == -1)
            break; //TODO: Add a better exit statement, preferably CAMX_ASSERT

        switch (getOptReturn)
        {
            // run test
            case 't':
            {
                std::string delimiter = ".";
                std::string testArg(optarg); // convert to string type for further parsing
                m_suiteName = testArg.substr(0, testArg.find(delimiter));  //extract suite name
                m_caseName = testArg.erase(0, m_suiteName.size() + 1);     //extract case  name
                m_fullName = m_suiteName + delimiter + m_caseName;         //create valid test full name [unused]
                break;
            }
            case 'h':
            case '?':
                m_bHelp = true;
                break;
            case 'l':
                m_bListTests = true;
                break;
            case 'v':
                m_nLog = atoi(optarg);
                break;
            case 'r':
                m_bReport = true;
                break;
            case 'f':
                m_nFrames = atoi(optarg);
                break;
            case 'c':
                m_nCamera = atoi(optarg);
                break;
            case 'x':
            {
                m_SpectraCamSimConfig = optarg;
                CF2_LOG_INFO("XML file path: %s", m_SpectraCamSimConfig.c_str());
            }
                break;
            default:
                m_bHelp = true;
                break;
        }
    }

    if (m_bHelp)
    {
        return -1;
    }

    return 0;
}
/**************************************************************************************************
*   PrintCommandLineUsage
*
*   @brief
*       Print the usage instructions to stdout
*       Usage instructions are printed when a commandline error is detected or --help (-h) is used
**************************************************************************************************/
void CmdLineParser::PrintCommandLineUsage()
{
    printf("\n************************************* USAGE *************************************\n");
    printf("\n");
    printf(" --test    (-t) : Run the given testsuite / testcases(s)\n");
    printf("                  ex: --run TestSuite.TestCase or TestSuite.*\n");
    printf(" --verbose (-v) : Logging levels. Insert number in range [0,7]\n");
    printf("                  [0-entry & exit, 2-debug, 3-info, 4-performance, 5-unsupported\n");
    printf("                  6-warning, 7-error]\n");
    printf(" --list    (-l) : List registered tests\n");
    printf(" --help    (-h) : Print the usage instructions\n");
    //printf(" --report (-r)  : Generate a summary report for the test\n");         // TODO not yet implemented
    //printf(" --frames (-f)  : Override default number of frames to be dumped\n"); // TODO not yet implemented
    //printf(" --camera (-c)  : Override for camera selection to run test on\n");   // TODO not yet implemented
    printf("\n");
    printf("*********************************************************************************\n");
}

/**************************************************************************************************
*   CmdLineParser::GetTargetProduct
*
*   @brief
*       Return string describing the target product
**************************************************************************************************/
const char* CmdLineParser::GetTargetProduct()
{
#ifdef TARGET_KONA
    return "Kona";
#elif TARGET_HANA
    return "Hana";
#elif TARGET_TALOSMOOREA
    return "Talos/Moorea";
#elif TARGET_NAPALI
    return "Napali";
#endif
    return "?";
}

/**************************************************************************************************
*   CmdLineParser::GetPlatformVersion
*
*   @brief
*       Return string describing the Android platform version
**************************************************************************************************/
const char* CmdLineParser::GetPlatformVersion()
{
#ifdef PLATFORM_VERSION_Q
    return "Q";
#elif PLATFORM_VERSION_P
    return "P";
#endif
    return "?";
}
