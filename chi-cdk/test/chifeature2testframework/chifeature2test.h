////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2test.h
/// @brief Declarations for the test framework.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2TEST_H
#define CHIFEATURE2TEST_H

#include "chifeature2log.h"
#include "chxutils.h"

#include <stdio.h>
#include <string>

#ifndef NULL
#define NULL 0
#endif

// Determine the compiler's environment
#if ((ULONG_MAX) == (UINT_MAX))
#define ENVIRONMENT32
#else
#define ENVIRONMENT64
#endif

// General macros
#define STRINGIFY(x) #x
#define PASTE(x, y) PASTE_(x, y)
#define PASTE_(x, y) x##y
#define CF2_NEW new
#define UNUSED_PARAM(expr) (VOID)(expr)

// Registering test macros
#define CHIFEATURE2TEST_TEST_INITIALIZE(_testsuite, _testcase)\
    CHIFEATURE2TEST_TEST_ORDERED(_testsuite, _testcase, cChiFeature2TestOrderInitialize)

#define CHIFEATURE2TEST_TEST_SHUTDOWN(_testsuite, _testcase)\
    CHIFEATURE2TEST_TEST_ORDERED(_testsuite, _testcase, cChiFeature2TestOrderShutDown)

#define CHIFEATURE2TEST_TEST(_testsuite, _testcase)\
    CHIFEATURE2TEST_TEST_ORDERED(_testsuite, _testcase, cChiFeature2TestOrderDefault)

#define CHIFEATURE2TEST_CLASS_NAME(_testsuite, _testcase)\
    _testsuite_##_testcase##Name

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHIFEATURE2TEST_TEST_ORDERED
///
/// @brief  Main macro to construct test class.
///         Inheritance order (base to derived): ChiFeature2Test -> Feature2TestCase -> _testsuite -> macro_generated_class
///
/// @param  _testsuite  Class that holds the test case
/// @param  _testcase   Name of the test case
/// @param  _testorder  Priority of this test case
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CHIFEATURE2TEST_TEST_ORDERED(_testsuite, _testcase, _testorder)\
class CHIFEATURE2TEST_CLASS_NAME(_testsuite, _testcase) : public _testsuite\
{\
public:\
    CHIFEATURE2TEST_CLASS_NAME(_testsuite, _testcase)()\
    {\
        static char suiteName[] = #_testsuite;\
        static char caseName[] = #_testcase;\
        static char fullName[] = #_testsuite "." #_testcase;\
        this->testSuiteName = suiteName;\
        this->testCaseName = caseName;\
        this->testFullName = fullName;\
        result = 0;\
        order = (int)_testorder;\
        RegisterFunction(this);\
    };\
    ~CHIFEATURE2TEST_CLASS_NAME(_testsuite, _testcase)(){};\
private:\
    char *testSuiteName;\
    char *testCaseName;\
    char *testFullName;\
    int result;\
    int order;\
    _testsuite testSuiteInstance;\
public:\
    VOID Run();\
    int GetPassed()\
    {\
        return(result == 0? 1 : 0);\
    }\
    int GetFailed()\
    {\
        return(result > 0? 1 : 0);\
    }\
    VOID SetFailed()\
    {\
        result = 1;\
    }\
    char *GetTestSuiteName()\
    {\
        return(testSuiteName);\
    }\
    char *GetTestCaseName()\
    {\
        return(testCaseName);\
    }\
    char *GetTestFullName()\
    {\
        return(testFullName);\
    }\
    int GetOrder()\
    {\
        return(order);\
    }\
}    PASTE(CHIFEATURE2TEST_CLASS_NAME(_testsuite, _testcase), Instance);\
VOID CHIFEATURE2TEST_CLASS_NAME(_testsuite, _testcase)::Run()

// Define assert and expect statements
#define CF2_ASSERT(_condition, _errorfmt, ...)\
    if (false == Check(GetTestObject(), _condition, __FILENAME__, __LINE__, #_condition, _errorfmt, ##__VA_ARGS__))\
        return

#define CF2_ASSERT2(_obj, _condition, _errorfmt, ...)\
    if (false == Check(_obj, _condition, __FILENAME__, __LINE__, #_condition, _errorfmt, ##__VA_ARGS__))\
        return

#define CF2_EXPECT(_condition, _errorfmt, ...)\
    Check(GetTestObject(), _condition, __FILENAME__, __LINE__, #_condition, _errorfmt, ##__VA_ARGS__)

#define CF2_EXPECT2(_obj, _condition, _errorfmt, ...)\
    Check(_obj, _condition, __FILENAME__, __LINE__, #_condition, _errorfmt, ##__VA_ARGS__)

#define CF2_FAIL(_errorfmt, ...)\
    if (false == Check(GetTestObject(), false, __FILENAME__, __LINE__, "FAIL", _errorfmt, ##__VA_ARGS__))\
        return

#define CF2_FAIL2(_obj, _errorfmt, ...)\
    if (false == Check(_obj, false, __FILENAME__, __LINE__, "FAIL", _errorfmt, ##__VA_ARGS__))\
        return

// Constants
const int cChiFeature2TestOrderLowest     = 0;
const int cChiFeature2TestOrderHighest    = 0x7FFFFFFE;
const int cChiFeature2TestOrderDefault    = -1;
const int cChiFeature2TestOrderInitialize = -2;
const int cChiFeature2TestOrderShutDown   = -3;
static const size_t cStringSize           = 1024;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Test related class/structure implementations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiFeature2Test
{
public:
    ChiFeature2Test() {};
    virtual ~ChiFeature2Test() {};
    static ChiFeature2Test* GetTestObject() { return m_pTestObj; };
    VOID SetTestObject(ChiFeature2Test* testObj) { m_pTestObj = testObj; };

    /* Feature2Test and all derived will implement */
    virtual VOID  Setup() = 0;
    virtual VOID  Teardown() = 0;

    /* Will be overridden by macro-created-class */
    virtual VOID  play(UINT id) { UNUSED_PARAM(id); };
    virtual VOID  Run() {};
    virtual int   GetPassed() { return 0; };
    virtual int   GetFailed() { return 0; };
    virtual VOID  SetFailed() {};
    virtual char* GetTestSuiteName() { return NULL; };
    virtual char* GetTestCaseName() { return NULL; };
    virtual char* GetTestFullName() { return NULL; };
    virtual int   GetOrder() { return 0; };

private:
    static ChiFeature2Test* m_pTestObj;
};

struct Size
{
    uint32_t width;
    uint32_t height;
    Size(uint32_t w = 0, uint32_t h = 0) : width(w), height(h) {}
};

// Extern definitions
extern VOID RegisterFunction(ChiFeature2Test *funcObj);
extern int  RunTests();
extern int  RunPlay(ChiFeature2Test *funcObj, UINT playId);
extern bool Check(ChiFeature2Test *funcObj, bool passed, const char* fileName,
    int lineNumber, const char* condition, const char* errorFmt, ...);

#endif // CHIFEATURE2TEST_H