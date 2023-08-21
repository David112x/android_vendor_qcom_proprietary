////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchisession.cpp
/// @brief Definitions for CHISession class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxatomic.h"
#include "camxchisession.h"
#include "camxhaldevice.h"
#include "camxhwcontext.h"
#include "camxmemspy.h"

CAMX_NAMESPACE_BEGIN

extern CHICALLBACKS  g_callbacks;

/// @todo (CAMX-1797) Not sure if we need anything fancy, quickly rethink
volatile UINT CHISession::s_numInstances = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHISession::~CHISession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHISession::~CHISession()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHISession::FlushThreadJobCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CHISession::FlushThreadJobCallback()
{
    if (InvalidJobHandle != m_hJobFamilyHandle)
    {
        m_pThreadManager->FlushJobFamily(m_hJobFamilyHandle, m_pThreadManager, TRUE);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHISession::UnregisterThreadJobCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CHISession::UnregisterThreadJobCallback()
{
    if (InvalidJobHandle != m_hJobFamilyHandle)
    {
        CHAR wrapperName[FILENAME_MAX];
        OsUtils::SNPrintF(&wrapperName[0], sizeof(wrapperName), "CHISessionWrapper%p", this);
        m_pThreadManager->UnregisterJobFamily(ThreadJobCallback, wrapperName, m_hJobFamilyHandle);
        m_hJobFamilyHandle = InvalidJobHandle;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHISession::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CHISession::Destroy()
{
    Session::Destroy();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHISession::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHISession* CHISession::Create(
    CHISessionCreateData* pCreateData)
{
    CamxResult  result      = CamxResultSuccess;
    CHISession* pCHISession = CAMX_NEW CHISession;

    if (NULL != pCHISession)
    {
        result = pCHISession->Initialize(pCreateData);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL, "CHISession Initialize failed!");
            pCHISession->Destroy();
            pCHISession = NULL;
        }
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("CHISession: Couldn't create CHISession object - out of memory");
        result = CamxResultENoMemory;
    }

    return pCHISession;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHISession::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CHISession::Initialize(
    CHISessionCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;
    CHAR wrapperName[FILENAME_MAX];

    CAMX_ASSERT(NULL != pCreateData);
    CAMX_ASSERT(NULL != pCreateData->sessionCreateData.pThreadManager);
    CAMX_ASSERT(NULL != pCreateData->sessionCreateData.pChiContext);

    result = Session::Initialize(&pCreateData->sessionCreateData);

    if (CamxResultSuccess == result)
    {
        m_localInstance = CamxAtomicIncU(&s_numInstances);
        OsUtils::SNPrintF(&wrapperName[0], sizeof(wrapperName), "CHISessionWrapper%p", this);

        result = m_pThreadManager->RegisterJobFamily(ThreadJobCallback,
                                                     wrapperName,
                                                     NULL,
                                                     JobPriority::Normal,
                                                     TRUE,
                                                     &m_hJobFamilyHandle);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHISession::ThreadJobCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* CHISession::ThreadJobCallback(
    VOID* pData)
{
    CamxResult  result   = CamxResultEFailed;
    CHISession* pSession = reinterpret_cast<CHISession*>(pData);

    if (NULL != pSession)
    {
        result = pSession->ThreadJobExecute();

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Chi workerCore failed with result %s", Utils::CamxResultToString(result));
        }
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHISession::ThreadJobExecute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CHISession::ThreadJobExecute()
{
    CamxResult result = CamxResultSuccess;

    // First process the Results
    while (TRUE == static_cast<BOOL>(CamxAtomicLoad32(&m_aCheckResults)))
    {
        CamxAtomicStore32(&m_aCheckResults, FALSE);
        result = ProcessResults();
    }

    if (CamxResultSuccess == result)
    {
        // If nothing catastrophic occurred, process the Requests
        result = ProcessRequest();

        if (CamxResultECancelledRequest == result)
        {
            CAMX_LOG_INFO(CamxLogGroupCore, "Session is in flush state, so, request is cancelled");
            // returning success as it is not an actual failure
            // Session flush call should take care of cleaning up the resources for cancelled requests
            result = CamxResultSuccess;
        }

        if (CamxResultSuccess != result)
        {
            // A serious error happened (couldn't process or cancel request)
            CAMX_LOG_ERROR(CamxLogGroupCore, "Request was not successful or cancelled (Result = %s), something went wrong",
                Utils::CamxResultToString(result));
        }
    }
    else
    {
        // Otherwise flush everything, as it is a device error
        Flush();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHISession::InitializeOutResults
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CHISession::InitializeOutResults(
    const Camera3CaptureResult* pCaptureResults,
    Camera3CaptureResult*       pOutResults)
{
    pOutResults->frameworkFrameNum  = pCaptureResults->frameworkFrameNum;
    pOutResults->pResultMetadata    = pCaptureResults->pResultMetadata;
    pOutResults->numOutputBuffers   = 0;
    pOutResults->pInputBuffer       = pCaptureResults->pInputBuffer;
    pOutResults->numPartialMetadata = pCaptureResults->numPartialMetadata;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHISession::DispatchResults
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CHISession::DispatchResults(
    ChiCaptureResult* pCaptureResults,
    UINT32            numCaptureResults)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(pCaptureResults);

    for (UINT32 index = 0; index < numCaptureResults; index++)
    {
        CAMX_LOG_INFO(CamxLogGroupChi,
                      "Sending Chi override client result for frame: %d, numOutputBuffers: %d, Metadata:%p,"
                      "numPartial:%d, priv = %p",
                      pCaptureResults[index].frameworkFrameNum,
                      pCaptureResults[index].numOutputBuffers,
                      pCaptureResults[index].pResultMetadata,
                      pCaptureResults[index].numPartialMetadata,
                      pCaptureResults[index].pPrivData);

        m_chiCallBacks.ChiProcessCaptureResult(&pCaptureResults[index], m_pPrivateCbData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHISession::DispatchNotify
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CHISession::DispatchNotify(
    ChiMessageDescriptor* pNotifyMessage)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(pNotifyMessage);

    m_chiCallBacks.ChiNotify(pNotifyMessage, m_pPrivateCbData);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHISession::DispatchPartialMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CHISession::DispatchPartialMetadata(
    ChiPartialCaptureResult* pPartialCaptureResult)
{

    CamxResult result = CamxResultSuccess;

    m_chiCallBacks.ChiProcessPartialCaptureResult(pPartialCaptureResult, m_pPrivateCbData);

    return result;
}

CAMX_NAMESPACE_END
