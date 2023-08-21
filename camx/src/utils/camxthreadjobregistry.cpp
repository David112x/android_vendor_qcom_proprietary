////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxthreadjobregistry.cpp
///
/// @brief Provides job registration and associated utilities
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxincs.h"
#include "camxthreadjobregistry.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobRegistry::~JobRegistry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JobRegistry::~JobRegistry()
{
    // Assert if Job Family is still registered
    CAMX_ASSERT(FALSE == CheckJobsStillRegistered());

    if (NULL != m_pRegistryLock)
    {
        m_pRegistryLock->Destroy();
        m_pRegistryLock = NULL;
    }
    if (NULL != m_pFlushJobSubmitLock)
    {
        m_pFlushJobSubmitLock->Destroy();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobRegistry::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JobRegistry::Initialize()
{
    CamxResult result = CamxResultSuccess;

    m_pRegistryLock = Mutex::Create("JobRegistry");
    if (NULL == m_pRegistryLock)
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Couldn't create lock resources");
        result = CamxResultEFailed;
    }

    m_pFlushJobSubmitLock = Mutex::Create("FlushJobSubmitLock");
    if (NULL == m_pFlushJobSubmitLock)
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Couldn't create lock resources");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobRegistry::RegisterNewJob
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JobRegistry::RegisterNewJob(
    JobFunc     jobFuncAddr,
    const CHAR* pJobFuncName,
    JobCb       flushDoneCb,
    JobPriority priority,
    BOOL        isSerialize,
    JobHandle*  phJob)
{
    CamxResult      result          = CamxResultSuccess;
    UINT32          slot            = 0;
    UINT32          counter         = 0;
    RegisteredJob*  pRegisteredJob  = NULL;

    BOOL jobAlreadyRegistered = IsJobAlreadyRegistered(phJob);
    BOOL freeSlotsAvailable   = IsFreeSlotAvailable(&slot, &counter);

    if ((TRUE  == jobAlreadyRegistered) ||
        (FALSE == freeSlotsAvailable))
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils,
                       "No more jobs can be registered, jobAlreadyRegistered: %d and has freeSlotsAvail: %d",
                       jobAlreadyRegistered,
                       freeSlotsAvailable);
        result = CamxResultEFailed;
    }

    // A job registered will live on in the registry till the lifetime of the camera session
    if (CamxResultSuccess == result)
    {
        pRegisteredJob                  = &m_registeredJobs[slot];
        Utils::Memset(pRegisteredJob, 0x0, sizeof(RegisteredJob));
        pRegisteredJob->funcAddr        = jobFuncAddr;
        pRegisteredJob->flushDoneCb     = flushDoneCb;
        pRegisteredJob->priority        = priority;
        pRegisteredJob->isSerial        = isSerialize;
        pRegisteredJob->slot            = slot;
        pRegisteredJob->uniqueCounter   = counter;

        // Creating new flush semaphore for each job family flush
        pRegisteredJob->pFlushSemaphore = Semaphore::Create();
        if (NULL == pRegisteredJob->pFlushSemaphore)
        {
            CAMX_LOG_ERROR(CamxLogGroupUtils, "Couldn't create lock resources");
            result = CamxResultEFailed;
        }

        if (CamxResultSuccess == result)
        {
            OsUtils::StrLCpy(pRegisteredJob->name, pJobFuncName, MaxNameLength);

            // Call Pack only after filling all the info except pRegisteredJob->hRegister
            // Fill pRegisteredJob->hRegister after packing with the job handle returned
            *phJob = PackJobHandleFromRegisteredJob(pRegisteredJob);

            pRegisteredJob->hRegister = *phJob;
            SetFlushStatus(*phJob, Noflush);

            CAMX_LOG_INFO(CamxLogGroupUtils,
                   "Registering a new job at slot %d- addr: %p, name: %s, handle: %llu",
                   slot,
                   jobFuncAddr,
                   pJobFuncName,
                   *phJob);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobRegistry::UnregisterJob
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JobRegistry::UnregisterJob(
    JobFunc     jobFuncAddr,
    const CHAR* pJobFuncName,
    JobHandle   hJob)
{
    m_pRegistryLock->Lock();

    RegisteredJob* pRegisteredJob = GetJobByHandle(hJob);

    if (TRUE != IsJobAlreadyRegistered(&hJob))
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils,
                       "Error: UnregisterJob called on a job that is not registered: addr: %p, name: %s",
                       jobFuncAddr, pJobFuncName);
    }
    else if ((NULL != pRegisteredJob) && (TRUE == ValidateJob(hJob)))
    {

        CAMX_LOG_VERBOSE(CamxLogGroupUtils,
                         "UnregisterJob called on job: addr: %p, name: %s, handle: %llu",
                         jobFuncAddr,
                         pJobFuncName,
                         hJob);

        pRegisteredJob->pFlushSemaphore->Destroy();
        pRegisteredJob->pFlushSemaphore = NULL;
        /// @note Please do not forget that pRegisteredJob is pointing to m_registeredJobs[slot]
        ///       Indeed memset could be done on pRegisteredJob variable, but to match the slot setting flow
        ///       and for readability, we are following this pattern.
        m_registeredSlots[pRegisteredJob->slot] = 0;
        Utils::Memset(&m_registeredJobs[pRegisteredJob->slot], 0x0, sizeof(RegisteredJob));
    }

    m_pRegistryLock->Unlock();

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobRegistry::CheckJobsStillRegistered
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL JobRegistry::CheckJobsStillRegistered()
{
    BOOL    result = FALSE;
    UINT32  i       = 0;

    for (i = 0; i < MaxRegisteredJobs; i++)
    {
        if (NULL != m_registeredJobs[i].funcAddr)
        {
            CAMX_LOG_ERROR(CamxLogGroupUtils, "%s While closing camera, this job is not Unregistered",
                m_registeredJobs[i].name);
            result = TRUE;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobRegistry::ValidateJob
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL JobRegistry::ValidateJob(
    JobHandle  hJob)
{
    BOOL           bResult          = FALSE;
    RegisteredJob* pRegisteredJob   = GetJobByHandle(hJob);

    // We are validating the handle coming in to the thread library, as we don't want unregistered jobs to run
    if ((NULL != pRegisteredJob)                   &&
        (pRegisteredJob->slot < MaxRegisteredJobs) &&
        (pRegisteredJob->hRegister == m_registeredJobs[pRegisteredJob->slot].hRegister))
    {
        bResult = TRUE;

        UINT32 counter = GetCounterByHandle(hJob);
        if (counter != m_registeredJobs[pRegisteredJob->slot].uniqueCounter)
        {
            CAMX_LOG_WARN(CamxLogGroupUtils,
                           "Unique counters are different, incoming=%d, actualInSlot=%d, actualName=%s, jobHandle=0x%llx",
                           counter, m_registeredJobs[pRegisteredJob->slot].uniqueCounter,
                           m_registeredJobs[pRegisteredJob->slot].name,
                           m_registeredJobs[pRegisteredJob->slot].funcAddr,
                           hJob);

            bResult = FALSE;
        }
    }

    return bResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobRegistry::StartFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JobRegistry::StartFlush(
    JobHandle   hJob,
    VOID*       pUserData,
    BOOL        isBlocking)
{
    RegisteredJob* pRegisteredJob = GetJobByHandle(hJob);

    /// hold lock when setting flush to sync with job submission
    this->FlushJobSubmitLock();
    SetFlushStatus(hJob, FlushRequested);
    this->FlushJobSubmitUnLock();

    pRegisteredJob->pFlushUserData = pUserData;

    if (TRUE == isBlocking)
    {
        SetFlushBlockStatus(pRegisteredJob, TRUE);
    }
    else
    {
        SetFlushBlockStatus(pRegisteredJob, FALSE);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobRegistry::WaitOutFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JobRegistry::WaitOutFlush(
    JobHandle hJob)
{
    RegisteredJob* pRegisteredJob = GetJobByHandle(hJob);

    if (FlushRequested == GetFlushStatus(hJob))
    {
        CAMX_LOG_INFO(CamxLogGroupUtils, "Entering Wait for flush for jobfamily %s", GetJobName(hJob));
        pRegisteredJob->pFlushSemaphore->Wait();
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobRegistry::ResumeJobAfterFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JobRegistry::ResumeJobAfterFlush(
    JobHandle hJob)
{
    CamxResult result = CamxResultSuccess;

    // if it is in flush processing, wait for flush done
    if (FlushRequested == GetFlushStatus(hJob))
    {
        WaitOutFlush(hJob);
    }

    // set NoFlush status after flush
    if (Flushed == GetFlushStatus(hJob))
    {
        this->FlushJobSubmitLock();
        SetFlushStatus(hJob, Noflush);
        this->FlushJobSubmitUnLock();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobRegistry::CheckFlushDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL JobRegistry::CheckFlushDone(
    JobHandle hJob)
{
    BOOL           result           = FALSE;
    RegisteredJob* pRegisteredJob   = GetJobByHandle(hJob);

    // Atomics are used in lieu of a lock/unlock for single access variables, to increase performance
    // If code is added later on which needs to secure larger sections, lock/unlock will be needed
    if ((0             != hJob) &&
        (FlushRequested == GetFlushStatus(hJob)) &&
        (0              == GetJobCount(hJob)))
    {
        SetFlushStatus(hJob, Flushed);
        if (NULL != pRegisteredJob->flushDoneCb)
        {
            pRegisteredJob->flushDoneCb(pRegisteredJob->pFlushUserData);
        }

        result = TRUE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobRegistry::SignalFlushDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JobRegistry::SignalFlushDone(
    JobHandle hJob)
{
    m_pRegistryLock->Lock();
    RegisteredJob* pRegisteredJob = GetJobByHandle(hJob);

    if (FALSE == ValidateJob(hJob))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupUtils,
                         "Unregistered job is being signaled. "
                         "Check for the null fence will block the flow saving locking mechanism");
    }

    // Unblock caller
    if ((TRUE == GetFlushBlockStatus(pRegisteredJob)) &&
        (NULL != pRegisteredJob->pFlushSemaphore))
    {
        pRegisteredJob->pFlushSemaphore->Signal();
    }
    m_pRegistryLock->Unlock();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobRegistry::CheckAllFlushDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JobRegistry::CheckAllFlushDone()
{
    UINT32      i    = 0;
    JobHandle   hJob = 0;

    for (i = 0; i < MaxRegisteredJobs; i++)
    {
        hJob = m_registeredJobs[i].hRegister;
        if (0 != hJob)
        {
            CheckFlushDone(hJob);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobRegistry::CheckAllFlushRequested
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL JobRegistry::CheckAllFlushRequested()
{
    UINT32      i       = 0;
    BOOL        result  = FALSE;
    JobHandle   hJob    = 0;

    for (i = 0; i < MaxRegisteredJobs; i++)
    {
        hJob = m_registeredJobs[i].hRegister;
        if (0 != hJob)
        {
            if ((FlushRequested == GetFlushStatus(hJob)) &&
                (0 != GetJobCount(hJob)))
            {
                result = TRUE;
                break;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobRegistry::CheckJobsOnHold
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL JobRegistry::CheckJobsOnHold()
{
    UINT32      i           = 0;
    JobHandle   hJob        = 0;
    BOOL        jobsOnHold  = FALSE;

    CAMX_ASSERT(m_pRegistryLock != NULL);

    m_pRegistryLock->Lock();

    for (i = 0; i < MaxRegisteredJobs; i++)
    {
        hJob = m_registeredJobs[i].hRegister;
        if (0 != hJob)
        {
            // Atomics are used in lieu of a lock/unlock for single access variables, to increase performance
            // If code is added later on which needs to secure larger sections, lock/unlock will be needed
            if ((TRUE == AreJobsOnHold(hJob)) && (0 == GetInflightCount(hJob)))
            {
                jobsOnHold = TRUE;
                break;
            }
        }

    }

    m_pRegistryLock->Unlock();

    return jobsOnHold;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobRegistry::IsJobAlreadyRegistered
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL JobRegistry::IsJobAlreadyRegistered(
    JobHandle* phJob)
{
    BOOL    result = FALSE;
    UINT32  i       = 0;

    /// @note If we allow one single thread manager instance across multiple camera sessions, then the following
    // logic won't work. Multiple camera sessions will try to register a job family with the same job name and
    // address we will then need to incorporate a session id concept
    if (*phJob != InvalidJobHandle)
    {
        for (i = 0; i < MaxRegisteredJobs; i++)
        {
            // A job is simply identified by the combination of its job function address and job name
            if ((TRUE == m_registeredSlots[i]) && (*phJob == m_registeredJobs[i].hRegister))
            {
                result = TRUE;
                break;
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JobRegistry::IsFreeSlotAvailable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL JobRegistry::IsFreeSlotAvailable(
    UINT32* pSlot,
    UINT32* pCounter)
{
    BOOL    result = FALSE;
    UINT32  i       = 0;

    CAMX_ASSERT(m_pRegistryLock != NULL);

    m_pRegistryLock->Lock();

    // Simple search as this is one time setup, and not per-frame
    for (i = 0; i < MaxRegisteredJobs; i++)
    {
        if (FALSE == m_registeredSlots[i])
        {
            *pSlot = i;
            m_registeredSlots[i] = TRUE;

            m_counter++;
            if (0 == m_counter)
            {
                // Do not allow 0 as unique value, so increment by 1. Needed when m_counter wraps around after reaching max
                m_counter++;
            }
            *pCounter = m_counter;

            result = TRUE;
            break;
        }
    }

    m_pRegistryLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JobRegistry::DumpStateToFile
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JobRegistry::DumpStateToFile(
    INT     fd,
    UINT32  indent)
{
    CAMX_ASSERT(m_pRegistryLock != NULL);

    if (CamxResultSuccess == m_pRegistryLock->TryLock())
    {
        CAMX_LOG_TO_FILE(fd, indent, "Job Registry %p", this);

        RegisteredJob* pJobFam = NULL;
        for (UINT32 i = 0; i < MaxRegisteredJobs; i++)
        {
            if (TRUE == m_registeredSlots[i])
            {
                pJobFam = &m_registeredJobs[i];

                CAMX_LOG_TO_FILE(fd, indent + 2, "Job Family %s - Total Job Count: %u, In Flight: %u, On Hold: %u",
                    pJobFam->name,
                    pJobFam->jobCount,
                    pJobFam->inflightCount,
                    pJobFam->holdCount);
            }
        }

        m_pRegistryLock->Unlock();
    }
    else
    {
        CAMX_LOG_TO_FILE(fd, indent + 2, "No Dump for Registry %p - Failed to acquire lock", this);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JobRegistry::DumpStateToLog
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JobRegistry::DumpStateToLog()
{
    CAMX_ASSERT(m_pRegistryLock != NULL);
    if (CamxResultSuccess == m_pRegistryLock->TryLock())
    {
        CAMX_LOG_DUMP(CamxLogGroupCore, "Job Registry %p", this);

        RegisteredJob* pJobFam = NULL;
        for (UINT32 i = 0; i < MaxRegisteredJobs; i++)
        {
            if (TRUE == m_registeredSlots[i])
            {
                pJobFam = &m_registeredJobs[i];
                CAMX_LOG_DUMP(CamxLogGroupCore, "\t Job Family %s - Total Job Count: %u, In Flight: %u, On Hold: %u",
                    pJobFam->name,
                    pJobFam->jobCount,
                    pJobFam->inflightCount,
                    pJobFam->holdCount);
            }
        }

        m_pRegistryLock->Unlock();
    }
    else
    {
        CAMX_LOG_DUMP(CamxLogGroupCore, "No Dump for Registry %p - Failed to acquire lock", this);
    }
}

CAMX_NAMESPACE_END
