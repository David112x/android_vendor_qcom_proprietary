////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxthreadqueue.cpp
/// @brief Implements queue utilities for runtime jobs in thread pool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxincs.h"
#include "camxthreadqueue.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobQueue::JobQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JobQueue::JobQueue()
{
    m_pQueueLock = Mutex::Create("JobQueue");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobQueue::~JobQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JobQueue::~JobQueue()
{
    if (NULL != m_pQueueLock)
    {
        m_pQueueLock->Destroy();
        m_pQueueLock = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobQueue::Enqueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JobQueue::Enqueue(
    RuntimeJob*  pJob,
    JobRegistry* pJobRegistry)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(m_pQueueLock != NULL);

    m_pQueueLock->Lock();

    if (((m_tail + 1) & (MaxRuntimeJobs - 1)) == m_head)
    {
        result = CamxResultEFailed;
    }

    if (CamxResultSuccess == result)
    {
        if (TRUE == pJobRegistry->IsSerial(pJob->hJob))
        {
            pJobRegistry->OnJobAdded(pJob);
        }

        m_pJobs[m_tail] = pJob;
        m_tail          = (m_tail + 1) & (MaxRuntimeJobs - 1);
    }

    m_pQueueLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobQueue::LockQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JobQueue::LockQueue()
{
    m_pQueueLock->Lock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobQueue::UnlockQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JobQueue::UnlockQueue()
{
    m_pQueueLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobQueue::HasJobs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL JobQueue::HasJobs()
{
    BOOL hasJobs = FALSE;

    // Only jobs in Submit state is reported, here. The jobs in onHold state and
    // ready to run is checked through JobRegistry::CheckJobsOnHold()
    if ((m_head != m_tail) && (JobStatus::Submitted == m_pJobs[m_head]->status))
    {
        hasJobs = TRUE;
    }

    return hasJobs;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobQueue::CheckAndDequeue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JobStatus JobQueue::CheckAndDequeue(
    RuntimeJob**    ppJob,
    JobRegistry*    pJobRegistry)
{
    JobStatus   status      = JobStatus::Invalid;
    RuntimeJob* pCurrentJob = NULL;
    BOOL        moveHead    = FALSE;

    CAMX_ASSERT(m_pQueueLock != NULL);

    *ppJob = NULL;

    m_pQueueLock->Lock();

    // If we woke up, and there was no job to process, it means some other
    // worker thread(s) may have beat me to it.It's OK to return and the caller
    // will not check status if job (ppJob) returned is NULL
    if (m_head == m_tail)
    {
        m_pQueueLock->Unlock();
        return status;
    }

    UINT32 localHead = m_head;

    pCurrentJob = m_pJobs[localHead];

    while (NULL != pCurrentJob)
    {
        switch (pCurrentJob->status)
        {
            case JobStatus::Submitted:

                if (FlushRequested == pJobRegistry->GetFlushStatus(pCurrentJob->hJob))
                {
                    pCurrentJob->status = JobStatus::Stopped;
                    *ppJob              = pCurrentJob;

                    if (m_head == localHead)
                    {
                        moveHead = TRUE;
                    }
                }
                else
                {
                    if (FALSE == pJobRegistry->IsSerial(pCurrentJob->hJob))
                    {
                        pCurrentJob->status = JobStatus::Ready;
                        *ppJob              = pCurrentJob;
                    }
                    else if (0 == pJobRegistry->GetInflightCount(pCurrentJob->hJob))
                    {
                        // At this sweep point, we have come across a job which can move from Submitted to Ready.
                        // But there may be earlier job(s) of the same family also eligible. We need to make the earliest
                        // such job Ready

                        RuntimeJob* pScanJob = GetFirstEligibleJob(pJobRegistry, pCurrentJob->hJob);
                        if (pScanJob != NULL)
                        {
                            pScanJob->status     = JobStatus::Ready;
                            *ppJob               = pScanJob;

                            if (pScanJob != pCurrentJob)
                            {
                                CAMX_ASSERT(pJobRegistry->GetHoldCount(pScanJob->hJob) > 0);

                                pJobRegistry->DecrementHoldCount(pScanJob->hJob);
                            }
                        }

                        if (m_head == localHead)
                        {
                            moveHead = TRUE;
                        }
                    }
                    else
                    {
                        // Once we enter here, we will not be able to move the Queue head
                        pCurrentJob->status = JobStatus::OnHold;
                        pJobRegistry->IncrementHoldCount(pCurrentJob->hJob);
                    }
                }

                break;

            case JobStatus::OnHold:

                if (FlushRequested == pJobRegistry->GetFlushStatus(pCurrentJob->hJob))
                {
                    pCurrentJob->status = JobStatus::Stopped;
                    *ppJob              = pCurrentJob;

                    if (m_head == localHead)
                    {
                        moveHead = TRUE;
                    }
                }
                else if (0 == pJobRegistry->GetInflightCount(pCurrentJob->hJob))
                {
                    // At this sweep point, we have come across a job which can move from OnHold to Ready.
                    // But there may be earlier job(s) of the same family also eligible. We need to make the earliest
                    // such job Ready

                    RuntimeJob* pScanJob = GetFirstEligibleJob(pJobRegistry, pCurrentJob->hJob);
                    if (pScanJob != NULL)
                    {
                        pScanJob->status     = JobStatus::Ready;
                        *ppJob               = pScanJob;

                        pJobRegistry->DecrementHoldCount(pCurrentJob->hJob);
                    }
                    if (m_head == localHead)
                    {
                        moveHead = TRUE;
                    }
                }

                break;

            case JobStatus::Stopped:

                if (m_head == localHead)
                {
                    moveHead = TRUE;
                }

                break;

            case JobStatus::Ready:
            case JobStatus::Invalid:
            default:
                break;
        }

        // If head job is ineligible, search for the next eligible job in queue
        if (NULL == *ppJob)
        {
            localHead = (localHead + 1) & (MaxRuntimeJobs - 1);
            if (localHead != m_tail)
            {
                pCurrentJob = m_pJobs[localHead];
            }
            else
            {
                pCurrentJob = NULL;
            }
        }
        else
        {
            pCurrentJob = NULL;
        }
    }

    if (TRUE == moveHead)
    {
        m_head = (m_head + 1) & (MaxRuntimeJobs - 1);
    }
    else
    {
        // The job(s) at head may have become Ready from OnHold in this sweep
        // We need to move the head as far as possible in that case

        pCurrentJob = m_pJobs[m_head];

        while ((m_head != m_tail) && (NULL != pCurrentJob) && (JobStatus::Ready == pCurrentJob->status))
        {
            m_head      = (m_head + 1) & (MaxRuntimeJobs - 1);
            pCurrentJob = m_pJobs[m_head];
        }
    }

    if (NULL != *ppJob)
    {
        status = (*ppJob)->status;
        if (JobStatus::Ready == status)
        {
            pJobRegistry->IncrementInflightCount((*ppJob)->hJob);
        }
    }

    m_pQueueLock->Unlock();

    return status;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobQueue::GetFirstEligibleJob
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
RuntimeJob* JobQueue::GetFirstEligibleJob(
    JobRegistry* pJobRegistry,
    JobHandle    hJob)
{
    RuntimeJob* pFirstJob = pJobRegistry->OnGetFirstJob(hJob);

    CAMX_ASSERT(NULL != pFirstJob);

    return pFirstJob;
}

CAMX_NAMESPACE_END
