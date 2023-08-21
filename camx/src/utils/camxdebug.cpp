////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxdebug.cpp
/// @brief Debug implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdarg.h>
#include "camxincs.h"

static UINT       g_runtimeAssertMask = 0x0;
static const UINT MaxAssertLogLength = 1024;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CamxLogAssert
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CamxLogAssert(
    UINT        assertType,
    const CHAR* pFileName,
    UINT        line,
    const CHAR* pFunction,
    const CHAR* pCondition,
    ...)
{
    if ((TRUE == CamX::g_logInfo.systemLogEnable) || (NULL != CamX::g_logInfo.pDebugLogFile))
    {
        // Generate output string
        CHAR        condition[MaxAssertLogLength];
        CHAR        assertText[MaxAssertLogLength];
        const CHAR* pAssertTypeLabel = CamxAssertTypeToString(static_cast<CamxAssert>(assertType));

        if (pFunction == NULL)
        {
            pFunction = "?function?";
        }

        if (pFileName == NULL)
        {
            pFileName = "?file?";
        }

        if (pCondition != NULL)
        {
            va_list argptr;
            INT     result;

            // write formatted data from variable argument list to sized buffer
            va_start(argptr, pCondition);
            result = CamX::OsUtils::VSNPrintF(condition, sizeof(condition), pCondition, argptr);
            va_end(argptr);
            if (result > 0)
            {
                pCondition = condition;
            }
        }

        // Write final Assert text. This will be used by several different embodiments of the assert
        if (pCondition != NULL)
        {
            CamX::OsUtils::SNPrintF(assertText, sizeof(assertText), "[ERROR][%s ] %s:%lu %s(): %s",
                                    pAssertTypeLabel,
                                    CamX::OsUtils::GetFileName(pFileName),
                                    line,
                                    pFunction,
                                    pCondition);
        }
        else
        {
            CamX::OsUtils::SNPrintF(assertText, sizeof(assertText), "[ERROR][%s ] %s:%lu %s(): %s",
                                    pAssertTypeLabel,
                                    CamX::OsUtils::GetFileName(pFileName),
                                    line,
                                    pFunction);

        }

        if (TRUE == CamX::g_logInfo.systemLogEnable)
        {
            CamX::OsUtils::LogSystem(assertType, assertText);
        }

        if (NULL != CamX::g_logInfo.pDebugLogFile)
        {
            CamX::OsUtils::FPrintF(CamX::g_logInfo.pDebugLogFile, "%s\n", assertText);
        }
    }

    // Now that logging is done, generate runtime assert.
    CamxFireAssert(static_cast<CamxAssert>(assertType));

    /// @todo (CAMX-6) Debug print to xlog
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CamxAssertTypeToString
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const CHAR* CamxAssertTypeToString(
    CamxAssert assertType)
{
    const CHAR* pString = NULL;
    switch (assertType)
    {
        case CamxAssertConditional:           pString = "Assert";         break;
        case CamxAssertAlways:                pString = "Always";         break;
        case CamxAssertNotImplemented:        pString = "NotImplemented"; break;
        case CamxAssertNotTested:             pString = "NotTested";      break;
        default:                              pString = "Unknown";        break;
    }
    return pString;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CamxUpdateAssertMask
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CamxUpdateAssertMask(
    CamxAssert assertMask)
{
    g_runtimeAssertMask = assertMask;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CamxFireAssert
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CamxFireAssert(
    CamxAssert assertType)
{
    if (assertType & g_runtimeAssertMask)
    {
        CamX::OsUtils::PerformSoftwareBreakpoint();
    }
}