////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2007-2010, 2015-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2test.cpp
/// @brief Implementation of the test framework.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <list>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdexcept>
#include <stdarg.h>
#include <regex>
#include "chifeature2log.h"
#include "chifeature2test.h"

// Forward Declarations
VOID ShutDown();
bool ComparePriorities(ChiFeature2Test *first, ChiFeature2Test *second);
VOID ReportResults(ChiFeature2Test *funcObj);
VOID ReportAllResults(int totalPassed, int totalFailed);
VOID RunTest(ChiFeature2Test *funcObj, int *totalPassedOut, int *totalFailedOut, bool *passedOut);

// Initialize static members
ChiFeature2Test* ChiFeature2Test::m_pTestObj;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Test framework related class/structure implementations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct ChiFeature2TestContainer
{
public:
    ChiFeature2TestContainer()
        : m_pTestListInitialize(NULL)
        , m_pTestListShutDown(NULL)
    {
        (VOID)m_TestListDefault.empty();
        (VOID)m_TestListOrder.empty();
    }

    ~ChiFeature2TestContainer()
    {
        m_pTestListInitialize = NULL;
        m_pTestListShutDown = NULL;
        (VOID)m_TestListDefault.empty();
        (VOID)m_TestListOrder.empty();
    }

    std::list<ChiFeature2Test *> m_TestListDefault;
    std::list<ChiFeature2Test *> m_TestListOrder;

    ChiFeature2Test *m_pTestListInitialize;
    ChiFeature2Test *m_pTestListShutDown;

}    ChiFeature2TestContainer;

// Globals
static ChiFeature2TestContainer *g_testFuncContainer = NULL;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Test helper functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID signalHandler(int signum)
{
    static char szException[cStringSize];
    (VOID)snprintf(szException,
        cStringSize,
        "Signal: %d", signum);
    throw new std::runtime_error(szException);
}

bool Check(ChiFeature2Test *funcObj, bool passed, const char *fileName, int lineNumber, const char* condition, const char* errorFmt, ...)
{
    // Get error format and args
    va_list errorArgs;
    va_start(errorArgs, errorFmt);
    char errOutput[cStringSize] = "\0";
    (VOID)vsnprintf(errOutput, cStringSize, errorFmt, errorArgs);
    va_end(errorArgs);

    if (!funcObj)
    {
        CF2_LOG_ERROR("ChiFeature2Test aborted: Invalid Class Object");
        exit(-1);
    }
    else
    {
        if (!passed)
        {
            funcObj->SetFailed();
            CF2_LOG_ERROR("CONDITION FAILED! %s:%d \"%s: %s\"\n  %s\n",
                fileName, lineNumber, funcObj->GetTestFullName(), condition, errOutput);
        }
    }

    return (passed);
}

bool ComparePriorities(ChiFeature2Test *first, ChiFeature2Test *second)
{
    if (first->GetOrder() < second->GetOrder())
    {
        return (true);
    }
    else
    {
        return (false);
    }
}

VOID RegisterFunction(ChiFeature2Test *funcObj)
{
    if (g_testFuncContainer == 0)
    {
        g_testFuncContainer = new ChiFeature2TestContainer();
    }

    if (funcObj->GetOrder() == cChiFeature2TestOrderDefault)
    {
        g_testFuncContainer->m_TestListDefault.push_back(funcObj);
    }
    else if ((funcObj->GetOrder() >= cChiFeature2TestOrderLowest) && (funcObj->GetOrder() <= cChiFeature2TestOrderHighest))
    {
        g_testFuncContainer->m_TestListOrder.push_back(funcObj);
        g_testFuncContainer->m_TestListOrder.sort(ComparePriorities);
    }
    else if (funcObj->GetOrder() == cChiFeature2TestOrderInitialize)
    {
        g_testFuncContainer->m_pTestListInitialize = funcObj;

    }
    else if (funcObj->GetOrder() == cChiFeature2TestOrderShutDown)
    {
        g_testFuncContainer->m_pTestListShutDown = funcObj;
    }
    else
    {
        CF2_LOG_ERROR("ChiFeature2Test aborted: \"%s\" was assigned to an invalid order number = %d",
            funcObj->GetTestFullName(), funcObj->GetOrder());
        exit(-1);
    }
}

VOID ReportResults(ChiFeature2Test *funcObj)
{
    int failed = funcObj->GetFailed();
    CF2_LOG_INFO("\"%s\" report -> %s\n",
        funcObj->GetTestFullName(),
        (failed == 0) ? "[PASSED]" : "[FAILED]");
}

VOID ReportAllResults(int totalPassed, int totalFailed)
{
    CF2_LOG_INFO("Final Report -> [%d PASSED] and [%d FAILED]\n", totalPassed, totalFailed)
}

VOID RunTest(ChiFeature2Test *funcObj, int *totalPassedOut, int *totalFailedOut, bool *passedOut)
{
    if (funcObj)
    {
        funcObj->SetTestObject(funcObj);
        CF2_LOG_INFO("Running test: %s", funcObj->GetTestFullName());

#ifdef CF2_TRYCATCH
        try
        {
#endif
        funcObj->Setup();
        // Only run if setup has not failed
        if (funcObj->GetFailed() == 0)
        {
            funcObj->Run();
        }
        funcObj->Teardown();
#ifdef CF2_TRYCATCH
        }
        catch (std::runtime_error *ex)
        {
            CF2_LOG_INFO("===============================================================");
            CF2_LOG_ERROR("ChiFeature2Test aborted: Test \"%s\" crashed. Error: %s",
                    funcObj->GetTestFullName(), ex->what());
            CF2_LOG_INFO("===============================================================");
            exit(1);
        }
#endif

        ReportResults(funcObj);

        *totalPassedOut += funcObj->GetPassed();
        *totalFailedOut += funcObj->GetFailed();

        if (funcObj->GetFailed() != 0)
        {
            *passedOut = false;
        }
    }
}

int RunTests()
{
    bool passed = true;
    int totalPassed = 0;
    int totalFailed = 0;
    int totalRegistered = 0;
    int registeredTests = 0;
    int matchingTestCaseCount = 0;
    char szOutput[cStringSize];

    std::list<ChiFeature2Test *>::iterator i;
    std::list<ChiFeature2Test *>::iterator end;
    ChiFeature2Test* pTest = NULL;

    // Flush stdout and stderr by default
    setbuf(stdout, NULL);

    // Get test to run from user
    std::string testSuiteName = CmdLineParser::GetTestSuiteName();
    std::string testCaseName  = CmdLineParser::GetTestCaseName();
    std::string testFullName  = CmdLineParser::GetTestFullName();

    // Treat wildcard input (*) as empty string
    if (testSuiteName == "*") testSuiteName = "";
    if (testCaseName == "*") testCaseName = "";

    // Print out registered tests and find number of matching tests to run
    CF2_LOG_INFO("Registered Tests:")

    // Priority tests
    end = g_testFuncContainer->m_TestListOrder.end();
    for (i = g_testFuncContainer->m_TestListOrder.begin();
         i != end;
         ++i)
    {
        totalRegistered++;
        pTest = reinterpret_cast<ChiFeature2Test *>(*i);

        std::string regTestSuiteName(pTest->GetTestSuiteName());
        std::string regTestCaseName(pTest->GetTestCaseName());

        CF2_LOG_INFO(" %d) %s.%s", ++registeredTests, regTestSuiteName.c_str(), regTestCaseName.c_str())

        // Suite name must match completely or be "*" or empty, no partial match allowed
        if (0 == testSuiteName.compare(regTestSuiteName) || testSuiteName == "")
        {
            // Case name can be a partial match
            std::string searchCase = testCaseName + "(.*)";
            std::regex searchCaseObj(searchCase);
            if (regex_match(regTestCaseName, searchCaseObj) || testCaseName == "")
            {
                matchingTestCaseCount++;
            }
        }
    }

    // Unordered / Default tests
    end = g_testFuncContainer->m_TestListDefault.end();
    for (i = g_testFuncContainer->m_TestListDefault.begin();
         i != end;
         ++i)
    {
        totalRegistered++;
        pTest = reinterpret_cast<ChiFeature2Test *>(*i);
        std::string regTestSuiteName(pTest->GetTestSuiteName());
        std::string regTestCaseName(pTest->GetTestCaseName());

        CF2_LOG_INFO(" %d) %s.%s", ++registeredTests, regTestSuiteName.c_str(), regTestCaseName.c_str())

        if (0 == testSuiteName.compare(regTestSuiteName) || testSuiteName=="")
        {
            std::string searchCase = testCaseName + "(.*)";
            std::regex searchCaseObj(searchCase);
            if (regex_match(regTestCaseName, searchCaseObj) || testCaseName == "")
            {
                matchingTestCaseCount++;
            }
        }
    }

    // Return if user only wanted to list registered tests
    if (CmdLineParser::ListTests())
    {
        return 0;
    }

    CF2_LOG_INFO("Running %d of %d tests...\n", matchingTestCaseCount, totalRegistered)

    // Register signal handlers to trap test case crashes
    signal(SIGSEGV, signalHandler);
    signal(SIGFPE, signalHandler);
    signal(SIGILL, signalHandler);

    // Run initialize
    RunTest(g_testFuncContainer->m_pTestListInitialize, &totalPassed, &totalFailed, &passed);

    // Run order list
    end = g_testFuncContainer->m_TestListOrder.end();
    for (i = g_testFuncContainer->m_TestListOrder.begin();
         i != end;
         ++i)
    {
        pTest = reinterpret_cast<ChiFeature2Test *>(*i);
        std::string regTestSuiteName(pTest->GetTestSuiteName());
        std::string regTestCaseName(pTest->GetTestCaseName());

        if (0 == testSuiteName.compare(regTestSuiteName) || testSuiteName == "")
        {
            std::string searchCase = testCaseName + "(.*)";
            std::regex searchCaseObj(searchCase);
            if (regex_match(regTestCaseName, searchCaseObj) || testCaseName == "")
            {
                RunTest(pTest, &totalPassed, &totalFailed, &passed);
            }
        }
        if(end==i)
        {
            CF2_LOG_ERROR("No tests match the input. Aborting.");
            break;
        }
    }
    // Run Unordered / Default list
    end = g_testFuncContainer->m_TestListDefault.end();
    for (i = g_testFuncContainer->m_TestListDefault.begin();
         i != end;
         ++i)
    {
        pTest = reinterpret_cast<ChiFeature2Test *>(*i);
        std::string regTestSuiteName(pTest->GetTestSuiteName());
        std::string regTestCaseName(pTest->GetTestCaseName());

        if (0 == testSuiteName.compare(regTestSuiteName) || testSuiteName == "")
        {
            std::string searchCase = testCaseName + "(.*)";
            std::regex searchCaseObj(searchCase);
            if (regex_match(regTestCaseName, searchCaseObj) || testCaseName == "")
            {
                RunTest(pTest, &totalPassed, &totalFailed, &passed);
            }
        }
        if (end == i)
        {
            CF2_LOG_ERROR("No tests match the input. Aborting.");
            break;
        }
    }

    // Run shutdown
    RunTest(g_testFuncContainer->m_pTestListShutDown, &totalPassed, &totalFailed, &passed);

    ShutDown();

    // Report
    ReportAllResults(totalPassed, totalFailed);

    if (passed)
    {
        return (0);
    }
    else
    {
        return (1);
    }
}

VOID ShutDown()
{
    if (g_testFuncContainer)
    {
        delete g_testFuncContainer;
        g_testFuncContainer = NULL;
    }
}

CDKResult RunPlay(ChiFeature2Test *funcObj, UINT playId)
{
    CDKResult result = CDKResultSuccess;

    if (funcObj)
    {
        funcObj->SetTestObject(funcObj);

#ifdef CF2_TRYCATCH
        try
        {
#endif
            funcObj->Setup();
            // Only run if setup has not failed
            if (funcObj->GetFailed() == 0)
            {
                funcObj->play(playId);
            }
            funcObj->Teardown();
#ifdef CF2_TRYCATCH
        }
        catch (std::runtime_error *ex)
        {
            CF2_LOG_INFO("===============================================================");
            CF2_LOG_ERROR("f2player aborted: f2Play \"%s\" crashed. Error: %s",
                funcObj->GetTestFullName(), ex->what());
            CF2_LOG_INFO("===============================================================");
            exit(1);
        }
#endif

        ReportResults(funcObj);

        if (funcObj->GetFailed() != 0)
        {
            result = CDKResultEFailed;
        }
    }

    return result;
}
