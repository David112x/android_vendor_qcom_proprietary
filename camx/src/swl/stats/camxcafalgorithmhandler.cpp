////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcafalgorithmhandler.cpp
/// @brief This class handle creation  of AF algorithm
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxhal3module.h"
#include "camxosutils.h"
#include "camxcafalgorithmhandler.h"

const CHAR* pDefaultAlgorithmLibraryName = "com.qti.stats.af";
const CHAR* pFunctionName                = "CreateAFAlgorithm";

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFAlgorithmHandler::CreateAlgorithm
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFAlgorithmHandler::CreateAlgorithm(
    const AFAlgoCreateParamList* pCreateParams,
    CHIAFAlgorithm**             ppAfAlgorithm,
    CREATEAF                     pfnCreate)
{
    CamxResult result                     = CamxResultSuccess;
    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();
    VOID* pAddr                           = NULL;

    if (NULL != pStaticSettings)
    {

        const CHAR* pLibraryName = NULL;
        const CHAR* pLibraryPath = NULL;

        if (TRUE == pStaticSettings->disableAFAlgoCHIOverload)
        {
            if (FALSE == pStaticSettings->enableCustomAlgoAF)
            {
                pLibraryName = pDefaultAlgorithmLibraryName;
                pLibraryPath = DefaultAlgorithmPath;
            }
            else
            {
                pLibraryName = pStaticSettings->customAlgoAFName;
                pLibraryPath = pStaticSettings->customAlgoAFPath;
            }

            pAddr = StatsUtil::LoadAlgorithmLib(&m_hHandle, pLibraryPath, pLibraryName, pFunctionName);

            if (NULL == pAddr)
            {
                result = CamxResultEUnableToLoad;

                CAMX_ASSERT_ALWAYS_MESSAGE("Unable to load the algo library! (%s) Path:%s", pLibraryName, pLibraryPath);

                if (NULL != m_hHandle)
                {
                    OsUtils::LibUnmap(m_hHandle);
                }
            }
            else
            {
                CREATEAF pAF = reinterpret_cast<CREATEAF>(pAddr);
                result = (*pAF)(pCreateParams, ppAfAlgorithm);
            }
        }
        else if (NULL != pfnCreate)
        {
            result = (*pfnCreate)(pCreateParams, ppAfAlgorithm);

            if ((CamxResultSuccess != result) || (NULL == ppAfAlgorithm))
            {
                CAMX_LOG_ERROR(CamxLogGroupAF,
                    "Failed to initialize result: %s, CHI Overload enabled: %d, ppAfAlgorithm: %p, Create pointer: %p",
                    Utils::CamxResultToString(result),
                    pStaticSettings->disableAFAlgoCHIOverload,
                    ppAfAlgorithm,
                    pStaticSettings->disableAFAlgoCHIOverload ? pAddr : reinterpret_cast<VOID*>(pfnCreate));
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAF, "pStaticSettings is a NULL pointer");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFAlgorithmHandler::CAFAlgorithmHandler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAFAlgorithmHandler::CAFAlgorithmHandler()
    : m_hHandle(NULL)
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFAlgorithmHandler::~CAFAlgorithmHandler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAFAlgorithmHandler::~CAFAlgorithmHandler()
{
    if (NULL != m_hHandle)
    {
        OsUtils::LibUnmap(m_hHandle);
        m_hHandle = NULL;
    }
}
CAMX_NAMESPACE_END
