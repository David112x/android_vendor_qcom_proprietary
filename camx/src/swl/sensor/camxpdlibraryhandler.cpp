////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxpdlibraryhandler.cpp
/// @brief This class handle creation  of PD Lib
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxhal3module.h"
#include "camxosutils.h"
#include "camxpdlibraryhandler.h"

const CHAR* pDefaultPDWrapperName        = "libcom.qti.stats.pdlibwapper";
const CHAR* pPDWrapperPath               = "";
const CHAR* pPDWrapperEntryFunctionName  = "CreatePDLibWrapper";

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CPDLibHandler::LoadPDLib
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* CPDLibHandler::LoadPDLib(
    CamX::OSLIBRARYHANDLE*  hHandle,
    const CHAR*             pLibPath,
    const CHAR*             pLibName,
    const CHAR*             pFuncName)
{
    VOID*  pAddr            = NULL;
    INT    numCharWritten   = 0;
    CHAR   libFilename[FILENAME_MAX];

    numCharWritten = OsUtils::SNPrintF(libFilename,
                                       FILENAME_MAX,
                                       "%s%s.%s",
                                       pLibPath,
                                       pLibName,
                                       SharedLibraryExtension);

    CAMX_ASSERT((numCharWritten < FILENAME_MAX) && (numCharWritten != -1));

    *hHandle = CamX::OsUtils::LibMap(libFilename);

    if (NULL == *hHandle)
    {
        numCharWritten = OsUtils::SNPrintF(libFilename,
                                           FILENAME_MAX,
                                           "%s%s%s.%s",
                                           VendorLibPath,
                                           PathSeparator,
                                           pLibName,
                                           SharedLibraryExtension);

        CAMX_ASSERT((numCharWritten < FILENAME_MAX) && (numCharWritten != -1));

        *hHandle = CamX::OsUtils::LibMap(libFilename);
    }

    if (NULL != (*hHandle))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAF, "Loaded Algorithm Lib: %s", libFilename);
        pAddr = CamX::OsUtils::LibGetAddr(*hHandle, pFuncName);
    }

    return pAddr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CPDLibHandler::CreateLib
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CPDLibHandler::CreateLib(
    CREATEPD    pfnCreate,
    CHIPDLib**  ppPDLibPtr,
    CHAR*       pPDAFLibName,
    UINT32      cameraId)
{
    CamxResult              result          = CamxResultSuccess;
    const StaticSettings*   pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();
    const CHAR*             pLibName        = NULL;
    const CHAR*             pLibPath        = NULL;
    VOID*                   pEntryFunction  = NULL;

    CAMX_ASSERT(NULL != pStaticSettings);

    if (TRUE == pStaticSettings->disablePDLibCHIOverload)
    {
        if (FALSE == pStaticSettings->enableCustomPDLib)
        {
            pLibName = pDefaultPDWrapperName;
            pLibPath = pPDWrapperPath;
        }
        else
        {
            pLibName = pStaticSettings->customPDLibName;
            pLibPath = pStaticSettings->customPDLibPath;
        }

        pEntryFunction = LoadPDLib(&m_hHandle, pLibPath, pLibName, pPDWrapperEntryFunctionName);
    }
    else
    {
        pEntryFunction = reinterpret_cast<VOID*>(pfnCreate);
    }

    // Create an instance of the core algorithm
    CAMX_ASSERT(NULL != pEntryFunction);

    if (NULL == pEntryFunction)
    {
        result = CamxResultEUnableToLoad;

        CAMX_ASSERT_ALWAYS_MESSAGE ("Unable to load the algo Lib! (%s) Path:%s", pLibName, pLibPath);

        if (NULL != m_hHandle)
        {
            OsUtils::LibUnmap(m_hHandle);
            m_hHandle = NULL;
        }
    }
    else
    {
        PDLibCreateParamList    createParamList = { 0 };
        PDLibCreateParam        createParams[PDLibCreateParamTypeCount];

        UINT* pOverridePDLibOpen                                                = (UINT *)&pStaticSettings->overridePDLibOpen;
        createParams[PDLibCreateParamTypeCameraOpenIndicator].createParamType   = PDLibCreateParamTypeCameraOpenIndicator;
        createParams[PDLibCreateParamTypeCameraOpenIndicator].pParam            = static_cast<VOID*>(pOverridePDLibOpen);
        createParams[PDLibCreateParamTypeCameraOpenIndicator].sizeOfParam       = sizeof(UINT);

        createParams[PDLibCreateParamTypeLibName].createParamType = PDLibCreateParamTypeLibName;
        createParams[PDLibCreateParamTypeLibName].pParam          = static_cast<VOID*>(pPDAFLibName);
        createParams[PDLibCreateParamTypeLibName].sizeOfParam     = sizeof(UINT);

        StatsCameraInfo cameraInfo;
        cameraInfo.cameraId = cameraId;
        createParams[PDLibCreateParamTypeCameraInfo].createParamType = PDLibCreateParamTypeCameraInfo;
        createParams[PDLibCreateParamTypeCameraInfo].pParam          = static_cast<VOID*>(&cameraInfo);
        createParams[PDLibCreateParamTypeCameraInfo].sizeOfParam     = sizeof(StatsCameraInfo);

        createParamList.paramCount = PDLibCreateParamTypeCount;
        createParamList.pParamList = &createParams[0];
        CREATEPD pCreatePDFucntion = reinterpret_cast<CREATEPD>(pEntryFunction);
        result = (*pCreatePDFucntion)(ppPDLibPtr, &createParamList);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CPDLibHandler::CPDLibHandler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CPDLibHandler::CPDLibHandler()
    : m_hHandle(NULL)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CPDLibHandler::~CPDLibHandler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CPDLibHandler::~CPDLibHandler()
{
    if (NULL != m_hHandle)
    {
        OsUtils::LibUnmap(m_hHandle);
        m_hHandle = NULL;
    }
}

CAMX_NAMESPACE_END
