////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxhistalgorithmhandler.cpp
/// @brief This class handle creation  of HDR algorithm
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxhal3module.h"
#include "camxosutils.h"
#include "camxhistalgorithmhandler.h"

const CHAR* pDefaultHistAlgorithmLibraryName = "com.qti.stats.localhistogram";
const CHAR* pHDRFunctionName = "CreateHistAlgorithm";

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HistAlgorithmHandler::CreateHistAlgorithm
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HistAlgorithmHandler::CreateHistAlgorithm(
    const HistAlgoProcessCreateParamList*  pCreateParams,
    ChiHistAlgoProcess**                   ppHDRAlgorithm,
    CREATEHISTOGRAMALGOPROCESS             pfnCreate)
{
    CAMX_UNREFERENCED_PARAM(pfnCreate); // In case we add histogram node in usecase.xml We can get rid of this file

    CamxResult result = CamxResultSuccess;
    VOID*      pAddr  =
        StatsUtil::LoadAlgorithmLib(&m_hHandle, DefaultAlgorithmPath, pDefaultHistAlgorithmLibraryName, pHDRFunctionName);

    // Create an instance of the core HDR algorithm
    if (NULL == pAddr)
    {
        result = CamxResultEUnableToLoad;

        CAMX_ASSERT_ALWAYS_MESSAGE("Unable to load the algo library! (%s) Path:%s",
            pDefaultHistAlgorithmLibraryName,
            DefaultAlgorithmPath);

        if (NULL != m_hHandle)
        {
            OsUtils::LibUnmap(m_hHandle);
        }
    }
    else
    {
        CREATEHISTOGRAMALGOPROCESS pHDR = reinterpret_cast<CREATEHISTOGRAMALGOPROCESS>(pAddr);
        result = (*pHDR)(pCreateParams, ppHDRAlgorithm);
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HistAlgorithmHandler::HistAlgorithmHandler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HistAlgorithmHandler::HistAlgorithmHandler()
    : m_hHandle(NULL)
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HistAlgorithmHandler::~HistAlgorithmHandler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HistAlgorithmHandler::~HistAlgorithmHandler()
{
    if (NULL != m_hHandle)
    {
        OsUtils::LibUnmap(m_hHandle);
        m_hHandle = NULL;
    }
}


CAMX_NAMESPACE_END
