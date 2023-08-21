////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxthreadcore.cpp
///
/// @brief Provides core worker thread functionality
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxincs.h"
#include "camxthreadcore.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadCore::ThreadCore
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ThreadCore::ThreadCore(
    JobRegistry* pJobRegistry,
    JobList*     pJobList,
    const CHAR*  pName,
    UINT32       numThreads)
    : m_pJobregistry(pJobRegistry)
    , m_pJobList(pJobList)
    , m_numThreads(numThreads)
    , m_stopped(FALSE)
    , m_jobPending(FALSE)
    , m_status(Stopped)
{
    if (NULL == pName)
    {
        pName = "CamXWorker";
    }
    OsUtils::StrLCpy(m_name, pName, sizeof(m_name));

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadCore::~ThreadCore
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ThreadCore::~ThreadCore()
{
    StopThreads();

    // If successful, the Core will be left in Stopped state
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadCore::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ThreadCore::Initialize()
{
    return StartThreads();

    // If successful, the Core will be left in Stopped state
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadCore::AcceptNewJob
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ThreadCore::AcceptNewJob(
    JobHandle   hJob,
    JobCb       stoppedCb,
    VOID**      ppData,
    BOOL        isSplitable,
    BOOL        isBlocking)
{
    CamxResult result = CamxResultSuccess;

    // For the moment, allowing only registered jobs to run. So validate the
    // handle to check if it was previously registered or not.

    if ((Initialized != GetStatus()) ||
        (FALSE == m_pJobregistry->ValidateJob(hJob)))
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Job cannot be accepted at this time");
        return CamxResultEFailed;
    }

    RuntimeJob* pRuntimeJob = m_pJobList->AcquireJobEntry(hJob);

    if (NULL != pRuntimeJob)
    {
        pRuntimeJob->hJob         = hJob;
        pRuntimeJob->stoppedCb    = stoppedCb;
        pRuntimeJob->isBlocking   = isBlocking;
        pRuntimeJob->isSplitable  = isSplitable;

        if (FALSE == isSplitable)
        {
            pRuntimeJob->pData[0]       = ppData[0];
            pRuntimeJob->numPartitions  = 1;
        }
        else
        {
            UINT32 j = 0;
            while (ppData[j] != NULL)
            {
                if (j < MaxDataPartition)
                {
                    pRuntimeJob->pData[j] = ppData[j];
                }
                j++;
            }

            if (j > MaxDataPartition)
            {
                CAMX_LOG_ERROR(CamxLogGroupUtils, "Job exceeded max data partition limit");
                result = CamxResultEFailed;
            }
            else
            {
                pRuntimeJob->numPartitions = j;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Job couldn't be added to job list");
        result = CamxResultEFailed;
    }

    if (CamxResultSuccess == result)
    {
        // check flush status and submission in critical section
        m_pJobregistry->FlushJobSubmitLock();
        if (Noflush == m_pJobregistry->GetFlushStatus(hJob))
        {
            if (TRUE == pRuntimeJob->isBlocking)
            {
                pRuntimeJob->pJobSemaphore = Semaphore::Create();
                // If we cannot create the blocking semaphore, we do not proceed
                /// @todo (CAMX-69) - Handle catastrophic errors
                CAMX_ASSERT(pRuntimeJob->pJobSemaphore != NULL);
            }

            result = AddToPriorityQueue(pRuntimeJob);
            m_pJobregistry->FlushJobSubmitUnLock();
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupUtils, "Couldn't add job to Priority Queue");
            }
            else
            {
                Trigger();

                if (TRUE == pRuntimeJob->isBlocking)
                {
                    CAMX_NOT_TESTED();

                    pRuntimeJob->pJobSemaphore->Wait();
                    pRuntimeJob->pJobSemaphore->Destroy();
                    pRuntimeJob->pJobSemaphore = NULL;
                }
            }
        }
        else
        {
            m_pJobregistry->FlushJobSubmitUnLock();
            result = CamxResultEFailed;
        }
    }

    if ((CamxResultSuccess != result) && (pRuntimeJob != NULL))
    {
        m_pJobList->ReleaseJobEntry(pRuntimeJob);
        Trigger();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadCore::ResumeJob
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ThreadCore::ResumeJob(
    JobHandle   hJob)
{
    CamxResult result = CamxResultSuccess;

    m_pThreadLock->Lock();
    if (FALSE == m_stopped)
    {
        m_pJobregistry->ResumeJobAfterFlush(hJob);
    }
    m_pThreadLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadCore::FlushJob
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ThreadCore::FlushJob(
    JobHandle   hJob,
    VOID*       pUserData,
    BOOL        isBlocking)
{
    CamxResult result = CamxResultSuccess;

    // For the moment, allowing only registered jobs to run

    if ((Initialized != GetStatus()) ||
        (FALSE == m_pJobregistry->ValidateJob(hJob)))
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Job cannot be flushed at this time");
        return CamxResultEFailed;
    }

    if (Noflush == m_pJobregistry->GetFlushStatus(hJob))
    {
        m_pJobregistry->StartFlush(hJob, pUserData, isBlocking);

        Trigger();

        if (TRUE == isBlocking)
        {
            m_pJobregistry->WaitOutFlush(hJob);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadCore::AddToPriorityQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ThreadCore::AddToPriorityQueue(
    RuntimeJob* pJob)
{
    CamxResult  result    = CamxResultEFailed;
    JobPriority priority  = m_pJobregistry->GetJobPriority(pJob->hJob);
    JobQueue*   pQueue    = GetQueue(priority);

    if (pQueue != NULL)
    {
        // For the moment, we are not taking care of data partitioned jobs
        /// @todo (CAMX-38) - Add support for DP Jobs

        pJob->status = JobStatus::Submitted;

        result = pQueue->Enqueue(pJob, m_pJobregistry);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadCore::GetQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JobQueue* ThreadCore::GetQueue(
    JobPriority priority)
{
    UINT jobPriority = static_cast<UINT>(priority);

    if (jobPriority >= MaxNumQueues)
    {
        jobPriority = static_cast<UINT>(JobPriority::Normal);
    }

    return &m_jobQueues[jobPriority];
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadCore::Trigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ThreadCore::Trigger()
{
    CAMX_ASSERT(m_pReadOK != NULL);
    CAMX_ASSERT(m_pThreadLock != NULL);

    // Wake at least one thread up

    m_pThreadLock->Lock();
    m_jobPending = TRUE;
    m_pThreadLock->Unlock();

    m_pReadOK->Signal();

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadCore::DispatchJob
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ThreadCore::DispatchJob(
    RuntimeJob* pJob)
{
    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupUtils, "ThreadCore::DispatchJob %s", m_pJobregistry->GetJobName(pJob->hJob));

    JobFunc func = m_pJobregistry->GetJobFunc(pJob->hJob);

    // Catering to only non data partitioned jobs now
    /// @todo (CAMX-38) - Add support for DP Jobs
    // Ignore the return from func

    if (NULL != func)
    {
        func(pJob->pData[0]);
    }

    m_pJobregistry->DecrementInflightCount(pJob->hJob);

    CAMX_TRACE_SYNC_END(CamxLogGroupUtils);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadCore::OnJobStopped
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ThreadCore::OnJobStopped(
    RuntimeJob* pJob)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupUtils, SCOPEEventThreadCoreOnJobStopped);

    if (NULL != pJob->stoppedCb)
    {
        pJob->stoppedCb(pJob->pData[0]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadCore::WorkerThreadBody
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ThreadCore::WorkerThreadBody(
    VOID* pArg)
{
    ThreadConfig*   pWorker = NULL;

    pWorker = reinterpret_cast<ThreadConfig*>(pArg);

    CAMX_ASSERT(NULL != pWorker);

    ThreadCore* pThreadCore = reinterpret_cast<ThreadCore*>(pWorker->pContext);
    pThreadCore->DoWork();

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadCore::DoWork
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ThreadCore::DoWork()
{
    CamxResult result = CamxResultEFailed;

    JobHandle   hSignalList[MaxRegisteredJobs];
    UINT        signalListCount = 0;

    CAMX_ASSERT(m_pReadOK != NULL);
    CAMX_ASSERT(m_pThreadLock != NULL);

    m_pThreadLock->Lock();

    while (FALSE == m_stopped)
    {
        while ((FALSE == m_stopped) && (FALSE == m_jobPending))
        {
            m_pReadOK->Wait(m_pThreadLock->GetNativeHandle());
        }

        if (FALSE == m_stopped)
        {
            // Release lock before going to Q read to give other threads a look in
            m_pThreadLock->Unlock();

            // result is ignored for now
            result = ProcessJobQueue();

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupUtils, "ProcessJobQueue failed with result %s", Utils::CamxResultToString(result));
            }

            // Lock it back before spinning on the condition
            m_pThreadLock->Lock();

            signalListCount = 0;
            // Record all jobHandles need to be signal
            for (UINT i = 0; i < MaxRegisteredJobs; i++)
            {
                JobHandle hJob = m_pJobregistry->GetRegisteredJob(i);
                if (0 != hJob)
                {
                    BOOL isFlushDone = m_pJobregistry->CheckFlushDone(hJob);
                    if (TRUE == isFlushDone)
                    {
                        hSignalList[signalListCount] = hJob;
                        signalListCount++;
                    }
                }
            }

            // Check to see if any job was pushed to Onhold and is now ready to run
            BOOL jobsOnHold = m_pJobregistry->CheckJobsOnHold();

            // Check to see if a flush was requested on any job and is now ready to be stopped
            BOOL jobsFlushRequested = m_pJobregistry->CheckAllFlushRequested();

            BOOL hasAnyJobs = FALSE;

            for (UINT i = 0; i < MaxNumQueues; i++)
            {
                m_jobQueues[i].LockQueue();
            }

            for (UINT i = 0; i < MaxNumQueues; i++)
            {
                hasAnyJobs |= m_jobQueues[i].HasJobs();
            }

            for (UINT i = 0; i < MaxNumQueues; i++)
            {
                m_jobQueues[i].UnlockQueue();
            }

            if ((FALSE == jobsOnHold) && (FALSE == jobsFlushRequested) && (FALSE == hasAnyJobs))
            {
                m_jobPending = FALSE;
            }

            // signal
            for (UINT i = 0; i < signalListCount; i++)
            {
                m_pJobregistry->SignalFlushDone(hSignalList[i]);
            }
        }
    }

    m_pThreadLock->Unlock();

    return NULL;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadCore::ProcessJobQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ThreadCore::ProcessJobQueue()
{
    CamxResult  result              = CamxResultSuccess;
    JobStatus   status              = JobStatus::Submitted;
    UINT32      i                   = 0;
    RuntimeJob* pJob                = NULL;
    JobQueue*   pQueue              = NULL;

    CAMX_ASSERT(m_pThreadLock != NULL);

    for (i = 0; i < MaxNumQueues; i++)
    {
        pQueue = &m_jobQueues[i];
        do
        {
            status = pQueue->CheckAndDequeue(&pJob, m_pJobregistry);

            if (NULL != pJob)
            {
                if (JobStatus::Ready == status)
                {
                    DispatchJob(pJob);
                }
                else if (JobStatus::Stopped == status)
                {
                    OnJobStopped(pJob);
                }

                if (pJob->isBlocking)
                {
                    // Unblock caller, if it was blocking
                    pJob->pJobSemaphore->Signal();
                }

                m_pJobList->ReleaseJobEntry(pJob);
            }
        } while (NULL != pJob);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadCore::FlushAllJobsInternal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ThreadCore::FlushAllJobsInternal()
{
    UINT32      i    = 0;
    JobHandle   hJob = 0;

    for (i = 0; i < MaxRegisteredJobs; i++)
    {
        hJob = m_pJobregistry->GetRegisteredJob(i);
        if (0 != hJob)
        {
            CamxResult result = FlushJob(hJob, NULL, FALSE);
            CAMX_ASSERT(CamxResultSuccess == result);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadCore::StartThreads
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ThreadCore::StartThreads()
{
    UINT32      i       = 0;
    CamxResult  result  = CamxResultEFailed;

    m_pReadOK       = Condition::Create("ThreadCoreReadOk");
    m_pThreadLock   = Mutex::Create(m_name);

    if ((NULL == m_pReadOK) || (NULL == m_pThreadLock))
    {
        SetStatus(Error);
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Couldn't create lock resources");
        return CamxResultEFailed;
    }

    for (i = 0; i < m_numThreads; i++)
    {
        m_workers[i].threadId       = i;
        m_workers[i].workThreadFunc = WorkerThreadBody;
        m_workers[i].pContext       = reinterpret_cast<VOID*>(this);

        result = OsUtils::ThreadCreate(m_workers[i].workThreadFunc, &m_workers[i], &m_workers[i].hWorkThread);
        if (CamxResultEFailed == result)
        {
            CAMX_LOG_ERROR(CamxLogGroupUtils, "Couldn't create worker thread");
            SetStatus(Stopped);
            break;
        }

        CHAR name[ThreadNameLength] = { 0 };
        INT  numCharWritten = OsUtils::SNPrintF(name, sizeof(name), "%s_%d", m_name, i);
        CAMX_ASSERT((numCharWritten < ThreadNameLength) && (numCharWritten != -1));
        OsUtils::ThreadSetName(m_workers[i].hWorkThread, name);
    }

    if (CamxResultSuccess == result)
    {
        SetStatus(Initialized);
    }
    else
    {
        m_stopped = TRUE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadCore::StopThreads
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ThreadCore::StopThreads()
{
    UINT32      i       = 0;
    CamxResult  result  = CamxResultSuccess;

    CAMX_ASSERT(m_pReadOK != NULL);
    CAMX_ASSERT(m_pThreadLock != NULL);

    // First flush all jobs if they are still pending
    FlushAllJobsInternal();
    // Then call the flush dones
    m_pJobregistry->CheckAllFlushDone();

    m_pThreadLock->Lock();
    m_stopped = TRUE;
    m_pReadOK->Broadcast();
    m_pThreadLock->Unlock();

    for (i = 0; i < m_numThreads; i++)
    {
        OsUtils::ThreadWait(m_workers[i].hWorkThread);
    }

    SetStatus(Stopped);

    m_pReadOK->Destroy();
    m_pReadOK = NULL;

    m_pThreadLock->Destroy();
    m_pThreadLock = NULL;

    return result;
}

CAMX_NAMESPACE_END
