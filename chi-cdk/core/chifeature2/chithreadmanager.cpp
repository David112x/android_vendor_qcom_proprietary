////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chithreadmanager.cpp
/// @brief CHX CHIThreadManager class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE CP006: Need vector to pass filtered port information

#include "chithreadmanager.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHIThreadManager::Create(
    CHIThreadManager ** ppInstance,
    const CHAR*         pName)
{
    CDKResult          result         = CDKResultEFailed;
    CHIThreadManager*  pLocalInstance = NULL;

    pLocalInstance = CHX_NEW CHIThreadManager();
    if (NULL != pLocalInstance)
    {
        result = pLocalInstance->Initialize(pName);
        if (CDKResultSuccess != result)
        {
            CHX_DELETE pLocalInstance;
            pLocalInstance = NULL;

            CHX_LOG_ERROR("Failed to create Chi threadmanager");
        }
    }

    if (CDKResultSuccess == result)
    {
        *ppInstance = pLocalInstance;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CHIThreadManager::Destroy()
{
    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::RegisterJobFamily
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHIThreadManager::RegisterJobFamily(
    JobFunc     jobFuncAddr,
    const CHAR* pJobFuncName,
    JobHandle*  phJob)
{
    CDKResult       result         = CDKResultSuccess;
    UINT32          slot           = 0;
    UINT32          count          = 0;
    RegisteredJob*  pRegisteredJob = NULL;

    // get an available slot to be used by both registeredJob and thredConfig
    BOOL jobAlreadyRegistered = IsJobAlreadyRegistered(phJob);
    BOOL freeSlotsAvailable   = IsFreeSlotAvailable(&slot, &count);

    if ((TRUE == jobAlreadyRegistered) ||
        (FALSE == freeSlotsAvailable))
    {
        CHX_LOG_ERROR("Failed to registerJobFramily, jobAlreadyRegistered: %d freeSlotsAvailable %d",
            jobAlreadyRegistered,
            freeSlotsAvailable);

        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        pRegisteredJob                = &m_registeredJobs[slot];
        ChxUtils::Memset(pRegisteredJob, 0x0, sizeof(RegisteredJob));
        pRegisteredJob->funcAddr      = jobFuncAddr;
        pRegisteredJob->slot          = slot;
        pRegisteredJob->uniqueCounter = count;
        pRegisteredJob->isUsed        = TRUE;

        CdkUtils::StrLCpy(pRegisteredJob->name, pJobFuncName, MaxNameLength);

        *phJob                    = PackJobHandleFromRegisteredJob(pRegisteredJob);
        pRegisteredJob->hRegister = *phJob;

        // create a thread dedicated to a client
        result = StartThreads(slot, phJob);

        if (CDKResultSuccess == result)
        {
            if (0 != pRegisteredJob->hRegisteredThread)
            {
                SetFlushStatus(*phJob, Noflush);
            }
            else
            {
                CHX_LOG_ERROR("Failed to get thread handle");
                result = CDKResultEFailed;
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::UnregisterJobFamily
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHIThreadManager::UnregisterJobFamily(
    JobFunc     jobFuncAddr,
    const CHAR* pJobFuncName,
    JobHandle   hJob)
{
    CDKResult      result         = CDKResultEFailed;
    RegisteredJob* pRegisteredJob = NULL;

    m_pRegisteredJobLock->Lock();

    pRegisteredJob = GetJobByHandle(hJob);

    if (TRUE != IsJobAlreadyRegistered(&hJob))
    {
        CHX_LOG_ERROR("Failed to unregister job %s FuncAddr %p result %d",
            pJobFuncName,
            jobFuncAddr,
            result);
    }
    else if (NULL != pRegisteredJob)
    {
        UINT32 slot = pRegisteredJob->slot;

        result = StopThreads(hJob);

        if (CDKResultSuccess == result)
        {
            m_threadWorker[slot].data.pq.clear();
            m_threadWorker[slot].data.pq.shrink_to_fit();

            ChxUtils::Memset(&m_registeredJobs[slot], 0x0, sizeof(RegisteredJob));
            ChxUtils::Memset(&m_threadWorker[slot], 0x0, sizeof(ThreadConfig));

            m_totalNumOfRegisteredJob--;

            CHX_LOG_INFO("Feature name %s remaining jobfamily %d", pJobFuncName, m_totalNumOfRegisteredJob);
        }
        else
        {
            CHX_LOG_ERROR("Failed to stop thread");
        }
    }

    m_pRegisteredJobLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::StartThreads
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHIThreadManager::StartThreads(
    UINT32      slot,
    JobHandle*  phJob)
{
    RegisteredJob* pRegisteredJob = GetJobByHandle(*phJob);
    CDKResult      result         = CDKResultSuccess;
    ThreadConfig*  pCfg           = NULL;

    pCfg = &m_threadWorker[slot];

    // check if slot is available.
    // since registeredJob and threadConfig are 1:1 mapped, same slot should be used for threadConfig as well.
    if ((NULL == pRegisteredJob) || (pCfg->isUsed == TRUE))
    {
        CHX_LOG_ERROR("Slot %d is already occupied. totalNumOfRegisteredJob %d",
            slot,
            m_totalNumOfRegisteredJob);

        result = CDKResultEFailed;
    }
    else
    {
        // create a thread dedicated to a client
        pCfg->threadId                 = pRegisteredJob->uniqueCounter;
        pCfg->workThreadFunc           = WorkerThreadBody;
        pCfg->ctrl.pReadOK             = Condition::Create();
        pCfg->ctrl.pThreadLock         = Mutex::Create();
        pCfg->ctrl.pFlushJobSubmitLock = Mutex::Create();
        pCfg->ctrl.pQueueLock          = Mutex::Create();
        pCfg->ctrl.pFlushOK            = Condition::Create();
        pCfg->ctrl.pFlushLock          = Mutex::Create();
        pCfg->pContext                 = reinterpret_cast<VOID*>(this); // threadManager
        pCfg->isUsed                   = TRUE;

        if ((NULL == pCfg->ctrl.pReadOK) ||
            (NULL == pCfg->ctrl.pThreadLock) ||
            (NULL == pCfg->ctrl.pFlushJobSubmitLock) ||
            (NULL == pCfg->ctrl.pQueueLock))
        {
            CHX_LOG_ERROR("Couldn't create lock resources. slot %d m_totalNumOfRegisteredJob %d",
                slot,
                m_totalNumOfRegisteredJob);

            pCfg->isUsed = FALSE;

            result = CDKResultEFailed;
        }

        if (CDKResultSuccess == result)
        {
            result = ChxUtils::ThreadCreate(
                pCfg->workThreadFunc,
                pCfg,
                &pCfg->hWorkThread);

            if (CDKResultSuccess == result)
            {
                pRegisteredJob->hRegisteredThread = pCfg->hWorkThread;
                pCfg->ctrl.pThreadLock->Lock();
                SetStatus(&pCfg->ctrl, Initialized);
                pCfg->ctrl.pThreadLock->Unlock();
            }
            else
            {
                CHX_LOG_ERROR("Couldn't create worker thread, logical threadId %d", pCfg->threadId);

                pCfg->isUsed = FALSE;

                result = CDKResultEFailed;
            }
        }

        CHX_LOG_INFO("ThreadId %d workThreadFunc %p hWorkThread %ld ReadOk %p, threadLock %p"
            "flushjobsubmit lock %p queuelock %p context %p isUsed %d, slot %d NumOfRegisteredJob %d",
            pCfg->threadId,
            pCfg->workThreadFunc,
            pCfg->hWorkThread,
            pCfg->ctrl.pReadOK,
            pCfg->ctrl.pThreadLock,
            pCfg->ctrl.pFlushJobSubmitLock,
            pCfg->ctrl.pQueueLock,
            pCfg->pContext,
            pCfg->isUsed,
            slot,
            m_totalNumOfRegisteredJob);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::AddToPriorityQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHIThreadManager::AddToPriorityQueue(
    RuntimeJob * pJob)
{
    CDKResult        result  = CDKResultSuccess;
    JobHandle        hJob    = 0;
    ThreadData*      pData   = NULL;
    ThreadControl*   pCtrl   = NULL;

    hJob    = pJob->hJob;
    pData   = GetThreadDataByHandle(hJob);
    pCtrl   = GetThreadControlByHandle(hJob);

    if ((NULL == pData) || (NULL == pCtrl))
    {
        CHX_LOG_ERROR("Failed to get threaddata %p or threadctrl %p",
            pData,
            pCtrl);

        result = CamxResultEFailed;
    }
    else
    {
        pCtrl->pQueueLock->Lock();
        pJob->status = JobStatus::Submitted;
        pData->pq.push_back(pJob);
        pCtrl->pQueueLock->Unlock();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::WorkerThreadBody
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* CHIThreadManager::WorkerThreadBody(
    VOID * pArg)
{
    ThreadConfig*   pConfig = NULL;

    pConfig = reinterpret_cast<ThreadConfig*>(pArg);

    if (NULL != pConfig)
    {
        CHIThreadManager* pThreadManager = reinterpret_cast<CHIThreadManager*>(pConfig->pContext);
        pThreadManager->DoWork(pArg);
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::DoWork
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID * CHIThreadManager::DoWork(
    VOID * pArg)
{
    CamxResult     result  = CamxResultEFailed;
    ThreadConfig*  pConfig = NULL;
    ThreadControl* pCtrl   = NULL;

    // per thread
    pConfig = reinterpret_cast<ThreadConfig*>(pArg);
    pCtrl   = &pConfig->ctrl;

    if (NULL == pCtrl)
    {
        CHX_LOG_ERROR("Failed to start");
    }
    else
    {
        pCtrl->pThreadLock->Lock();

        while (Stopped != GetStatus(pCtrl))
        {
            while ((Stopped != GetStatus(pCtrl)) && (FALSE == pCtrl->jobPending))
            {
                pCtrl->pReadOK->Wait(pCtrl->pThreadLock->GetNativeHandle());
            }

            if (Stopped != GetStatus(pCtrl))
            {
                pCtrl->jobPending = FALSE; // Set to False immediately after wait, so that next trigger won't be blocked
                pCtrl->pThreadLock->Unlock();
            }

            result = ProcessJobQueue(pConfig);

            // To guarantee flush is done
            if (TRUE == GetFlushBlockStatus(pCtrl))
            {
                pCtrl->pFlushLock->Lock();
                SetFlushBlockStatus(pCtrl, FALSE);
                pCtrl->pFlushOK->Signal();
                pCtrl->pFlushLock->Unlock();
            }

            if (Stopped != GetStatus(pCtrl))
            {
                pCtrl->pThreadLock->Lock();
            }

            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("ProcessJobQueue failed with result");
            }
        }
        pCtrl->pThreadLock->Unlock();
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::Trigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHIThreadManager::Trigger(
    JobHandle hJob)
{
    CDKResult      result = CDKResultSuccess;
    ThreadControl* pCtrl  = NULL;

    pCtrl = GetThreadControlByHandle(hJob);

    if (NULL == pCtrl)
    {
        CHX_LOG_ERROR("Failed to get threadctrl %p", pCtrl);

        result = CDKResultEFailed;
    }
    else
    {
        pCtrl->pThreadLock->Lock();
        pCtrl->jobPending = TRUE;
        pCtrl->pReadOK->Signal();
        pCtrl->pThreadLock->Unlock();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::ProcessJobQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CHIThreadManager::ProcessJobQueue(
    VOID* pCfg)
{
    UINT32         result    = CDKResultSuccess;
    ThreadConfig*  pConfig   = NULL;
    ThreadControl* pCtrl     = NULL;
    ThreadData*    pData     = NULL;
    RuntimeJob*    pJob      = NULL;
    BOOL           isQueued  = FALSE;

    pConfig = reinterpret_cast<ThreadConfig*>(pCfg);
    pCtrl = &pConfig->ctrl;
    pData = &pConfig->data;

    pCtrl->pQueueLock->Lock();
    if (FALSE == pData->pq.empty())
    {
        isQueued = TRUE;
    }
    pCtrl->pQueueLock->Unlock();

    while (TRUE == isQueued)
    {
        pCtrl->pQueueLock->Lock();
        pJob = pData->pq.front();

        if (NULL != pJob)
        {
            switch(pJob->status)
            {
                case JobStatus::Submitted:
                    if (FlushRequested == GetFlushStatus(pJob->hJob))
                    {
                        pJob->status = JobStatus::Stopped;
                    }
                    else if (Noflush == GetFlushStatus(pJob->hJob))
                    {
                        pJob->status = JobStatus::Ready;
                    }

                    break;
                case JobStatus::Stopped:
                    break;
                default:
                    break;
            }

            pData->pq.pop_front();
            pCtrl->pQueueLock->Unlock();

            if (pJob->status == JobStatus::Ready)
            {
                DispatchJob(pJob);
            }

            CHX_FREE(pJob);
            pJob = NULL;
        }

        pCtrl->pQueueLock->Lock();
        if (TRUE == pData->pq.empty())
        {
            isQueued = FALSE;
        }
        pCtrl->pQueueLock->Unlock();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::DispatchJob
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CHIThreadManager::DispatchJob(
    RuntimeJob * pJob)
{
    JobFunc func = GetJobFunc(pJob->hJob);

    if (NULL != func)
    {
        func(pJob->pData);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::FlushJob
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHIThreadManager::FlushJob(
    JobHandle hJob,
    BOOL      forceFlush)
{
    CDKResult result = CDKResultSuccess;

    ThreadControl* pCtrl = GetThreadControlByHandle(hJob);

    if (NULL == pCtrl)
    {
        CHX_LOG_ERROR("pCtrl is null");
        result = CDKResultEFailed;
    }
    else
    {
        if (Initialized != GetStatus(pCtrl))
        {
            CHX_LOG_ERROR("Thread is not initialized");

            result = CDKResultEFailed;
        }
        else
        {
            if (Noflush == GetFlushStatus(hJob))
            {
                pCtrl->pFlushJobSubmitLock->Lock();
                SetFlushStatus(hJob, FlushRequested);
                pCtrl->pFlushJobSubmitLock->Unlock();

                SetFlushBlockStatus(pCtrl, TRUE);

                result = Trigger(hJob);

                if (CDKResultSuccess == result)
                {
                    // it would not wait when blocking singal comes first than wait()
                    pCtrl->pFlushLock->Lock();
                    if (TRUE == GetFlushBlockStatus(pCtrl))
                    {
                        pCtrl->pFlushOK->Wait(pCtrl->pFlushLock->GetNativeHandle());
                    }
                    pCtrl->pFlushLock->Unlock();
                }
                else
                {
                    CHX_LOG_ERROR("Failed to trigger");
                }

                if (TRUE == forceFlush)
                {
                    pCtrl->pFlushJobSubmitLock->Lock();
                    SetFlushStatus(hJob, Noflush);
                    pCtrl->pFlushJobSubmitLock->Unlock();
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::StopThreads
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CHIThreadManager::StopThreads(
    JobHandle hJob)
{
    CamxResult     result = CamxResultSuccess;
    ThreadConfig*  pCfg   = GetThreadConfigByHandle(hJob);
    ThreadData*    pData  = GetThreadDataByHandle(hJob);
    ThreadControl* pCtrl  = GetThreadControlByHandle(hJob);

    if ((NULL == pCfg) || (NULL == pData) || (NULL == pCtrl))
    {
        CHX_LOG_ERROR("Failed cfg %p data %p ctrl %p",
            pCfg,
            pData,
            pCtrl);

        result = CamxResultEFailed;
    }
    else
    {
        result = FlushJob(hJob, FALSE);

        pCtrl->pFlushJobSubmitLock->Lock();
        SetFlushStatus(hJob, Noflush);
        pCtrl->pFlushJobSubmitLock->Unlock();

        pCtrl->pThreadLock->Lock();
        SetStatus(pCtrl, Stopped);
        pCtrl->pReadOK->Signal();
        pCtrl->pThreadLock->Unlock();

        ChxUtils::ThreadTerminate(pCfg->hWorkThread);

        pCtrl->pReadOK->Destroy();
        pCtrl->pReadOK = NULL;

        pCtrl->pThreadLock->Destroy();
        pCtrl->pThreadLock = NULL;

        pCtrl->pFlushJobSubmitLock->Destroy();
        pCtrl->pFlushJobSubmitLock = NULL;

        pCtrl->pQueueLock->Destroy();
        pCtrl->pQueueLock = NULL;

        pCtrl->pFlushLock->Destroy();
        pCtrl->pFlushLock = NULL;

        pCtrl->pFlushOK->Destroy();
        pCtrl->pFlushOK = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::PostJob
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHIThreadManager::PostJob(
    JobHandle hJob,
    VOID*     pData,
    UINT64    requestId)
{
    RuntimeJob*    pRuntimeJob = NULL;
    CDKResult      result      = CDKResultSuccess;
    ThreadControl* pCtrl       = NULL;

    pCtrl = GetThreadControlByHandle(hJob);

    if (NULL == pCtrl)
    {
        CHX_LOG_ERROR("pCtrl is null");
        result = CDKResultEFailed;
    }
    else
    {
        if (Initialized != GetStatus(pCtrl))
        {
            CHX_LOG_ERROR("Thread is not initialized threadId");

            result = CDKResultEFailed;
        }
        else
        {
            pRuntimeJob = reinterpret_cast<RuntimeJob*>(CHX_CALLOC(sizeof(RuntimeJob)));

            if (NULL != pRuntimeJob)
            {
                pRuntimeJob->hJob      = hJob;
                pRuntimeJob->requestId = requestId;
                pRuntimeJob->pData     = pData;

                pCtrl->pFlushJobSubmitLock->Lock();
                result = AddToPriorityQueue(pRuntimeJob);
                pCtrl->pFlushJobSubmitLock->Unlock();

                if (CamxResultSuccess != result)
                {
                    CHX_LOG_ERROR("Couldn't add job to Priority Queue");
                }
                else
                {
                    result = Trigger(hJob);

                    if (CDKResultSuccess != result)
                    {
                        CHX_LOG_ERROR("Failed to trigger");
                    }
                }
            }
            else
            {
                CHX_LOG_ERROR("Failed to create runtimejob out of memory");
                result = CDKResultENoMemory;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::RemoveJob
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHIThreadManager::RemoveJob(
    JobHandle hJob,
    VOID * pRemoveJobData)
{
    CDKResult        result = CDKResultSuccess;
    ThreadData*      pData = NULL;
    ThreadControl*   pCtrl = NULL;

    pData = GetThreadDataByHandle(hJob);
    pCtrl = GetThreadControlByHandle(hJob);

    if ((NULL == pData) || (NULL == pCtrl))
    {
        CHX_LOG_ERROR("Failed to get threaddata %p or threadctrl %p",
            pData,
            pCtrl);

        result = CamxResultEFailed;
    }
    else
    {
        pCtrl->pQueueLock->Lock();

        for (UINT32 i = 0; i < pData->pq.size(); ++i)
        {
            RuntimeJob* pJob = pData->pq[i];
            if (pRemoveJobData == pJob->pData)
            {
                pData->pq.erase(pData->pq.begin() + i);
            }
        }
        pCtrl->pQueueLock->Unlock();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::GetAllPostedJobs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHIThreadManager::GetAllPostedJobs(JobHandle hJob, std::vector<VOID*>& rPostedJobs)
{
    CDKResult        result = CDKResultSuccess;
    ThreadData*      pData = NULL;
    ThreadControl*   pCtrl = NULL;

    pData = GetThreadDataByHandle(hJob);
    pCtrl = GetThreadControlByHandle(hJob);

    if ((NULL == pData) || (NULL == pCtrl))
    {
        CHX_LOG_ERROR("Failed to get threaddata %p or threadctrl %p",
            pData,
            pCtrl);

        result = CamxResultEFailed;
    }
    else
    {
        pCtrl->pQueueLock->Lock();

        for (UINT32 i = 0; i < pData->pq.size(); ++i)
        {
            RuntimeJob* pJob = pData->pq[i];
            if (JobStatus::Submitted == pJob->status)
            {
                rPostedJobs.push_back(pJob->pData);
            }
        }
        pCtrl->pQueueLock->Unlock();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::CHIThreadManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIThreadManager::CHIThreadManager()
{
    m_totalNumOfRegisteredJob = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::~CHIThreadManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIThreadManager::~CHIThreadManager()
{
    CDKResult result = CDKResultSuccess;

    // finishing all the remaining jobs and threads
    if (m_totalNumOfRegisteredJob > 0)
    {
        RegisteredJob* pRegisteredJob = NULL;

        for (UINT32 i = 0; i < MaxRegisteredJobs; i++)
        {
            pRegisteredJob = &m_registeredJobs[i];

            if (0 != pRegisteredJob->hRegister)
            {
                result = UnregisterJobFamily(NULL, pRegisteredJob->name, pRegisteredJob->hRegister);

                if (CDKResultSuccess != result)
                {
                    CHX_LOG_ERROR("Failed to process UnregisterJob slot %d", pRegisteredJob->slot);
                }
            }
        }
    }

    m_pRegisteredJobLock->Destroy();
    m_pRegisteredJobLock = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHIThreadManager::Initialize(
    const CHAR * pName)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pName)
    {
        pName = "ChiThreadmanager";
    }
    CdkUtils::StrLCpy(m_name, pName, sizeof(m_name));

    m_pRegisteredJobLock  = Mutex::Create();

    if (NULL == m_pRegisteredJobLock)
    {
        CHX_LOG_ERROR("Failed to initialize %s", pName);
        result = CDKResultEFailed;
    }
    else
    {
        ChxUtils::Memset(&m_registeredJobs, 0, sizeof(m_registeredJobs));
        ChxUtils::Memset(&m_threadWorker, 0, sizeof(m_threadWorker));

        CHX_LOG_INFO("initialized %s", pName);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::IsJobAlreadyRegistered
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CHIThreadManager::IsJobAlreadyRegistered(
    JobHandle * phJob)
{
    BOOL    isExist = FALSE;

    if (*phJob != InvalidJobHandle)
    {
        for (UINT32  i = 0; i < MaxRegisteredJobs; i++)
        {
            if ((TRUE == m_registeredJobs[i].isUsed) && (*phJob == m_registeredJobs[i].hRegister))
            {
                isExist = TRUE;
                break;
            }
        }
    }

    return isExist;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::IsFreeSlotAvailable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CHIThreadManager::IsFreeSlotAvailable(
    UINT32* pSlot,
    UINT32* pCount)
{
    BOOL    result = FALSE;

    m_pRegisteredJobLock->Lock();

    for (UINT32 i = 0; i < MaxRegisteredJobs; i++)
    {
        if ((FALSE == m_registeredJobs[i].isUsed) && (FALSE == m_threadWorker[i].isUsed))
        {
            *pSlot = i;
            m_totalNumOfRegisteredJob++;

            if (0 == m_totalNumOfRegisteredJob)
            {
                m_totalNumOfRegisteredJob++;
            }

            if (m_totalNumOfRegisteredJob < MaxRegisteredJobs)
            {
                *pCount = m_totalNumOfRegisteredJob;
                result = TRUE;
            }
            else
            {
                CHX_LOG_ERROR("Exceed MaxRegisteredJobs %d", m_totalNumOfRegisteredJob);
            }

            break;
        }
    }

    m_pRegisteredJobLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIThreadManager::Dump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CHIThreadManager::Dump()
{
    CHX_LOG_VERBOSE("Remaining jobfamily and threads");

    RegisteredJob* pRegisteredJob = NULL;

    for (UINT32 i = 0; i < MaxRegisteredJobs; i++)
    {
        pRegisteredJob = &m_registeredJobs[i];

        if (0 != pRegisteredJob->isUsed)
        {
            CHX_LOG_VERBOSE("job name %s flushStatus %d",
                pRegisteredJob->name,
                pRegisteredJob->flushStatus);
        }
    }
}
