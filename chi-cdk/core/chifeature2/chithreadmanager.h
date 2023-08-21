////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chithreadmanager.h
/// @brief CHI CHIThreadManager class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE NC004c: CHI files wont be having Camx
// NOWHINE FILE CP006: Need whiner update: std::priority queue allowed in exceptional cases
// NOWHINE FILE CP008: Need whiner update: operator overloading allowed in exceptional cases
// NOWHINE FILE PR007b: Whiner incorrectly concludes as non-library files

#ifndef CHITHREADMANAGER_H
#define CHITHREADMANAGER_H

#include <queue>

#include "cdkutils.h"
#include "chxdefs.h"
#include "chxusecaseutils.h"

/// @brief Opaque handle to a job
typedef UINT64 JobHandle;
/// @brief Function pointer type for a job function
typedef VOID* (*JobFunc)(VOID* pArg);

// Invalid thread handle
static const JobHandle InvalidJobHandle = 0;

// Max length of job family name
static const UINT32 MaxNameLength = 255;
// Max registered jobs in a pool
static const UINT32 MaxRegisteredJobs = 50;
// Max Number of threads to be created
static const UINT32 MaxNumThread = 50;
/// @brief Core status type
typedef UINT32 CoreStatus;
/// @brief Core status type
typedef UINT32 JobFlushStatus;

 // Thread core status
static const CoreStatus Error       = 0;       ///< Error state
static const CoreStatus Initialized = 1;       ///< Initialized and running state
static const CoreStatus Stopped     = 2;       ///< Stopped state

// Flush status
static const JobFlushStatus Noflush        = 0;    ///< Default state
static const JobFlushStatus FlushRequested = 1;    ///< A flush is requested
static const JobFlushStatus Flushed        = 2;    ///< Flush request has been fulfilled

/// @brief Status of a runtime job in the threadpool
enum class JobStatus
{
    Submitted,  ///< A job is being added to one of the job Queues
    Ready,      ///< A submitted job is being dispatched
    OnHold,     ///< A submitted job is being put on hold since one of the previous jobs in family hasn't finished
    Stopped,    ///< A flush is issued on the job family and the job is being stopped
    Invalid     ///< Invalid status
};

/// @brief Structure describing the properties of a registered Job family
struct RegisteredJob
{
    JobFunc         funcAddr;                   ///< Address of job function for job family
    CHAR            name[MaxNameLength];        ///< Text name of job function for job family
    BOOL            isUsed;                     ///< flag to check if slot is occupied
    UINT64          hRegister;                  ///< Opaque handle to the registered job
    UINT32          slot;                       ///< The slot in the job registry in which a job family is registered
    UINT32          uniqueCounter;              ///< Unique value for this Register
    OSThreadHandle  hRegisteredThread;          ///< thread handle to find corresponding threadConfig
    JobFlushStatus  flushStatus;                ///< Current flush status of the registered job
};

/// @brief Structure describing the properties of a runtime Job
struct RuntimeJob
{
    UINT64      hJob;                       ///< Opaque handle to the registered job
    VOID*       pData;                      ///< data pointer for a job
    UINT64      requestId;                  ///< requestId corresponding to this job
    JobStatus   status;                     ///< Current status of the runtime job
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Comparator for threadqueue
///
/// Implements
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Compare
{
public:
    BOOL operator() (RuntimeJob* pJob1, RuntimeJob* pJob2)
    {
        BOOL isPriority = false;

        if (pJob1->requestId > pJob2->requestId)
        {
            isPriority = true;
        }

        return isPriority;
    }
};

struct ThreadControl
{
    Mutex*          pThreadLock;            ///< lock for each thread, covers m_pReadOK, m_stopped, m_jobPending
    Mutex*          pFlushJobSubmitLock;    ///< Job submit and flush sync lock
    Mutex*          pQueueLock;             ///< Priority Queue lock
    Mutex*          pFlushLock;             ///< flush blockinglock
    Condition*      pReadOK;                ///< condition for each thread
    Condition*      pFlushOK;               ///< condition for blocking flush
    CoreStatus      status;                 ///< Indicates whether the threads should stop running
    volatile BOOL   jobPending;             ///< Indicates if a new job is submitted
    volatile UINT32 blockingStatus;         ///< blocking status while flushing
};

/// @brief Structure describing the data used per thread
struct ThreadData
{
    std::deque<RuntimeJob*> pq;
};

/// @brief Structure describing a Worker Thread per feature
struct ThreadConfig
{
    UINT32          threadId;               ///< Logical thread number
    OSThreadHandle  hWorkThread;            ///< Platform thread id
    JobFunc         workThreadFunc;         ///< Thread entry function
    VOID*           pContext;               ///< Opaque configuration handle
    ThreadData      data;                   ///< data per thread
    ThreadControl   ctrl;                   ///< thread information
    BOOL            isUsed;                 ///< to check if slot is occupied
};

/// @brief Forward declartion of class
class CHIThreadManager;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief CHIThreadManager Class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDK_VISIBILITY_PUBLIC CHIThreadManager
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static method to create an instance of ThreadManager
    ///
    /// @param  ppInstance Instance pointer to be returned
    /// @param  pName      Name of the pool
    ///
    /// @return Success or EFailed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult Create(
        CHIThreadManager** ppInstance,
        const CHAR*     pName);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Method to delete an instance of ThreadManager
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RegisterJobFamily
    ///
    /// @brief  Register a job family to the thread library. A job family is the set of jobs with the same function name and
    ///         address, and priority
    ///
    /// @param  jobFuncAddr   Job function address
    /// @param  pJobFuncName  Job function name
    /// @param  phJob         Handle to the job family, returned from the library
    ///
    /// @return Success or EFailed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult RegisterJobFamily(
        JobFunc     jobFuncAddr,
        const CHAR* pJobFuncName,
        JobHandle*  phJob);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UnregisterJobFamily
    ///
    /// @brief  Un-register a job family from the thread library.
    ///
    /// @param  jobFuncAddr  Job function address
    /// @param  pJobFuncName Job function name
    /// @param  hJob         Handle to the job family
    ///
    /// @return Success or EFailed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult UnregisterJobFamily(
        JobFunc     jobFuncAddr,
        const CHAR* pJobFuncName,
        JobHandle   hJob);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostJob
    ///
    /// @brief  Post a job, which may belong to a previously registered family
    ///
    /// @param  hJob         Handle to the job returned from RegisterJobFamily
    /// @param  pData        Array or 1 or more partitioned data, terminated by NULL pointer
    /// @param  requestId    RequestId corresponding to this job
    ///
    /// @return Success or EFailed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult PostJob(
        JobHandle   hJob,
        VOID*       pData,
        UINT64      requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RemoveJob
    ///
    /// @brief  Remove a job from list of pending jobs
    ///
    /// @param  hJob         Handle to the job returned from RegisterJobFamily
    /// @param  pData        Array or 1 or more partitioned data, terminated by NULL pointer
    ///
    /// @return Success or EFailed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult RemoveJob(
        JobHandle   hJob,
        VOID*       pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAllPostedJobs
    ///
    /// @brief  Get List of jobs currently in submitted state
    ///
    /// @param  hJob         Handle to the job returned from RegisterJobFamily
    /// @param  rPostedJobs  Reference to vector containing all posted jobs
    ///
    /// @return Success or EFailed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult   GetAllPostedJobs(
        JobHandle           hJob,
        std::vector<VOID*>  &rPostedJobs);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FlushJob
    ///
    /// @brief  Flush a registered family of job
    ///
    /// @param  hJob         Handle to the job returned from RegisterJobFamily
    /// @param  forceFlush   Toggles modified codeflow when we only want to flush the job
    ///
    /// @return Success or EFailed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult FlushJob(
        JobHandle   hJob,
        BOOL        forceFlush);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CHIThreadManager
    ///
    /// @brief  Default constructor for ThreadManager object.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHIThreadManager();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~CHIThreadManager
    ///
    /// @brief  Destructor for ThreadManager object.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~CHIThreadManager();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize a newly created CHIThreadManager object
    ///
    /// @param  pName      Name of the pool
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Initialize(
        const CHAR* pName);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsJobAlreadyRegistered
    ///
    /// @brief  Check to see if an incoming job is already registered or not
    ///
    /// @param  phJob  Job Handle
    ///
    /// @return TRUE or FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsJobAlreadyRegistered(
        JobHandle* phJob);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsFreeSlotAvailable
    ///
    /// @brief  Check if a free slot is available in the job registry
    ///
    /// @param  pSlot       Slot returned
    /// @param  pCount      Count returned
    ///
    /// @return TRUE or FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsFreeSlotAvailable(
        UINT32* pSlot,
        UINT32* pCount);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCounterByHandle
    ///
    /// @brief  Utility function to return unique counter to the registered job given a job handle
    ///
    /// @param  hJob Job handle
    ///
    /// @return Unique counter value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CHX_INLINE UINT32 GetCounterByHandle(
        JobHandle hJob)
    {
        return static_cast<UINT32>(hJob >> 32);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSlotByHandle
    ///
    /// @brief  Utility function to return slot index to the registered job given a job handle
    ///
    /// @param  hJob Job handle
    ///
    /// @return Slot index corresponds to this Job handle
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CHX_INLINE UINT32 GetSlotByHandle(
        JobHandle hJob)
    {
        UINT32 last32Bits = 0xFFFFFFFF;
        UINT32 slot = (hJob & last32Bits);

        return slot;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetJobByHandle
    ///
    /// @brief  Utility function to return pointer to the registered job given a job handle
    ///
    /// @param  hJob Job handle
    ///
    /// @return Pointer to registered job
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE RegisteredJob* GetJobByHandle(
        JobHandle hJob)
    {
        UINT32 slot = GetSlotByHandle(hJob);

        return &m_registeredJobs[slot];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetThreadConfigByHandle
    ///
    /// @brief  Utility function to return pointer to the thread config given a job handle
    ///
    /// @param  hJob Job handle
    ///
    /// @return Pointer to ThreadConfig
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE ThreadConfig* GetThreadConfigByHandle(
        JobHandle hJob)
    {
        OSThreadHandle hThreadConfig  = 0;
        OSThreadHandle hRegisteredJob = 0;
        UINT32         slot           = GetSlotByHandle(hJob);

        hRegisteredJob = m_registeredJobs[slot].hRegisteredThread;
        hThreadConfig  = m_threadWorker[slot].hWorkThread;

        if ((0 != hRegisteredJob) && (hRegisteredJob == hThreadConfig))
        {
            return &m_threadWorker[slot];
        }
        else
        {
            return NULL;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetThreadDataByHandle
    ///
    /// @brief  Utility function to return pointer to the thread data given a job handle
    ///
    /// @param  hJob Job handle
    ///
    /// @return Pointer to ThreadData
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE ThreadData* GetThreadDataByHandle(
        JobHandle hJob)
    {
        ThreadConfig* pCfg;
        pCfg = GetThreadConfigByHandle(hJob);

        if (NULL != pCfg)
        {
            return &pCfg->data;
        }
        else
        {
            return NULL;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetThreadControlByHandle
    ///
    /// @brief  Utility function to return pointer to the thread control given a job handle
    ///
    /// @param  hJob Job handle
    ///
    /// @return Pointer to ThreadControl
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE ThreadControl* GetThreadControlByHandle(
        JobHandle hJob)
    {
        ThreadConfig* pCfg;
        pCfg = GetThreadConfigByHandle(hJob);

        if (NULL != pCfg)
        {
            return &pCfg->ctrl;
        }
        else
        {
            return NULL;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetJobFunc
    ///
    /// @brief  Return function address of a registered job
    ///
    /// @param  hJob Handle to previously registered job
    ///
    /// @return Address of the job function of the job family
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE JobFunc GetJobFunc(
        JobHandle  hJob)
    {
        RegisteredJob* pRegisteredJob = GetJobByHandle(hJob);
        return pRegisteredJob->funcAddr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PackJobHandleFromRegisteredJob
    ///
    /// @brief  Utility function to pack Job handle
    ///
    /// @param  pRegisteredJob  Pointer to Register job slot
    ///
    /// @return JobHandle
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    JobHandle PackJobHandleFromRegisteredJob(
        RegisteredJob*  pRegisteredJob)
    {
        JobHandle handle = InvalidJobHandle;

        handle = pRegisteredJob->uniqueCounter;
        handle = handle << 32;
        handle |= pRegisteredJob->slot;

        return handle;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFlushStatus
    ///
    /// @brief  Get the current flush status of the registered job
    ///
    /// @param  hJob Handle to previously registered job
    ///
    /// @return JobFlushStatus
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE JobFlushStatus GetFlushStatus(
        JobHandle  hJob)
    {
        RegisteredJob* pRegisteredJob = GetJobByHandle(hJob);
        return ChxUtils::AtomicLoadU32(&pRegisteredJob->flushStatus);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetFlushStatus
    ///
    /// @brief  Set the current flush status of the registered job
    ///
    /// @param  hJob   Handle to previously registered job
    /// @param  val    Value to set
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID SetFlushStatus(
        JobHandle   hJob,
        JobFlushStatus val)
    {
        RegisteredJob* pRegisteredJob = GetJobByHandle(hJob);
        ChxUtils::AtomicStoreU32(&pRegisteredJob->flushStatus, val);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFlushBlockStatus
    ///
    /// @brief  Get the blocking status for the current flush request of the registered job
    ///
    /// @param  pCtrl Pointer to ThreadControl
    ///
    /// @return TRUE or FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE BOOL GetFlushBlockStatus(
        ThreadControl* pCtrl)
    {
        return static_cast<BOOL>(ChxUtils::AtomicLoadU32(&pCtrl->blockingStatus));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetFlushBlockStatus
    ///
    /// @brief  Set the blocking status for the current flush request of the registered job
    ///
    /// @param  pCtrl            Pointer to ThreadControl
    /// @param  blockingStatus   Value to set
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID SetFlushBlockStatus(
        ThreadControl* pCtrl,
        BOOL blockingStatus)
    {
        ChxUtils::AtomicStoreU32(&pCtrl->blockingStatus, blockingStatus);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStatus
    ///
    /// @brief  Get the current status of the thread
    ///
    /// @param  pCtrl  pointer to threadcontrol
    ///
    /// @return CoreStatus
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE CoreStatus GetStatus(
        ThreadControl* pCtrl) const
    {
        // NOWHINE CP036a: Since the function is const, had to add the const_cast
        return ChxUtils::AtomicLoadU32(const_cast<volatile UINT32*>(&pCtrl->status));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetStatus
    ///
    /// @brief  Set the current status of the thread
    ///
    /// @param  pCtrl  pointer to threadcontrol
    /// @param  status Value to set
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID SetStatus(
        ThreadControl* pCtrl,
        CoreStatus status)
    {
        ChxUtils::AtomicStoreU32(&pCtrl->status, status);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StartThreads
    ///
    /// @brief  Create & start worker threads, initialize mutex and conditions
    ///
    /// @param  slot      target slot
    /// @param  phJob     job handle
    ///
    /// @return Success or EFailed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult StartThreads(
        UINT32 slot,
        JobHandle*  phJob);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddToPriorityQueue
    ///
    /// @brief  Adds a runtime job to one of the job queues, based on priority
    ///
    /// @param  pJob Pointer to the runtime job
    ///
    /// @return Success or EFailed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult AddToPriorityQueue(
        RuntimeJob* pJob);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WorkerThreadBody
    ///
    /// @brief  Static, common landing function for all worker threads after creation
    ///
    /// @param  pArg Payload for the worker thread
    ///
    /// @return NULL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* WorkerThreadBody(
        VOID* pArg);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoWork
    ///
    /// @brief  Actual worker thread routine
    ///
    /// @param  pArg per thread config
    ///
    /// @return NULL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID* DoWork(
        VOID* pArg);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Trigger
    ///
    /// @brief  Trigger threads to process job queues
    ///
    /// @param  hJob working job
    ///
    /// @return Success or EFailed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Trigger(
        JobHandle hJob);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessJobQueue
    ///
    /// @brief  Main routine for worker threads to look into and execute job queues in order of priority
    ///
    /// @param  pCfg per threadConfig
    ///
    /// @return Success or EFailed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessJobQueue(
        VOID* pCfg);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DispatchJob
    ///
    /// @brief  Dispatch a job from the thread pool by calling its function
    ///
    /// @param  pJob Pointer to the runtime job
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DispatchJob(
        RuntimeJob* pJob);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StopThreads
    ///
    /// @brief  Stop worker threads, and de-initialize mutex and condition
    ///
    /// @param  hJob to be stopped
    ///
    /// @return Success or EFailed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult StopThreads(
        JobHandle hJob);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Dump
    ///
    /// @brief  Dump all the information of this threadmanager
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Dump();

    // Disable copy constructor and assignment operator
    CHIThreadManager(const CHIThreadManager&) = delete;
    CHIThreadManager& operator=(const CHIThreadManager&) = delete;

    CHAR          m_name[MaxNameLength];               ///< Brief human readable name for the pool
    UINT32        m_totalNumOfRegisteredJob;           ///< number of registeredJob = m_numThreads

    RegisteredJob m_registeredJobs[MaxRegisteredJobs];      ///< Array/list of registered jobs
    ThreadConfig  m_threadWorker[MaxNumThread];             ///< Array/list of thread configs
    Mutex*        m_pRegisteredJobLock;                     ///< registeredJob lock
};

#endif // CHITHREADMANAGER_H
