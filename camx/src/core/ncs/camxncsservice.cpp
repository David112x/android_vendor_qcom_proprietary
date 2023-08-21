////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxncsservice.cpp
/// @brief CamX NCS Service class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxncsintfqsee.h"
#include "camxncsservice.h"
#include "camxincs.h"
#include "camxosutils.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSService::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSService::Initialize(
    NCSInitializeInfo* pInitializeInfo)
{
    INCSIntfBase* pNCSIntfObject  = NULL;
    NCSIntfQSEE*  pNCSQSEEObject  = NULL;
    CamxResult    result          = CamxResultSuccess;

    if (NULL != pInitializeInfo                     &&
        NULL != pInitializeInfo->pThreadManager     &&
        NULL != pInitializeInfo->pCallbackData      &&
        NULL != pInitializeInfo->attachChiFence     &&
        NULL != pInitializeInfo->releaseChiFence    &&
        NULL != pInitializeInfo->signalChiFence)
    {
        m_hNCSPollThHandle.pThreadManager = pInitializeInfo->pThreadManager;

        m_numActiveClients  = 0;
        // Create and initialize the Sensor API interface object
        for (UINT i = 0; i < MaxNCSIntfType; i++ )
        {
            m_pNCSIntfObject[i]      = NULL;
            // for now only support QSEE. All other interface types are unsupported
            if (i == QSEE)
            {
                pNCSQSEEObject = NCSIntfQSEE::GetInstance(pInitializeInfo, this);
                if (NULL != pNCSQSEEObject)
                {
                    pNCSIntfObject = static_cast<INCSIntfBase*>(pNCSQSEEObject);
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_LOG_ERROR(CamxLogGroupNCS, "Can not get NCS interface instance");
                    break;
                }
            }
            else
            {
                continue;
            }

            result = pNCSIntfObject->GetListOfSensors();
            if (CamxResultSuccess == result)
            {
                result = pNCSIntfObject->QueryCapabilites();
                if (CamxResultSuccess == result)
                {
                    // for now assume all the mutex createion will success. will add check later.
                    m_pNCSServiceMutex                     = Mutex::Create("NCSServiceMutex");
                    m_hNCSPollThHandle.pNCSQMutex          = Mutex::Create("NCSJobQMutex");
                    m_hNCSPollThHandle.pNCSQCondVar        = Condition::Create("NCSJobQCondVar");
                    m_hNCSPollThHandle.pNCSFlushVar        = Semaphore::Create();
                    m_hNCSPollThHandle.pServiceObject      = this;
                    m_pNCSIntfObject[i]                    = pNCSIntfObject;

                    CAMX_LOG_INFO(CamxLogGroupNCS, "Successfully Initilzed NCS inteface type %d", i);
                }
                else
                {
                    CAMX_LOG_WARN(CamxLogGroupNCS, "Unable to get capabilities for interface type %d!", i);
                }
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupNCS, "Unable to get list of sensors for interface type %d!", i);
            }
        }
    }
    else
    {
        m_hNCSPollThHandle.pThreadManager = NULL;
        result                            = CamxResultEInvalidArg;
        CAMX_LOG_WARN(CamxLogGroupNCS, "Initialize Information is not completed!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSService::RegisterService
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
NCSSensor* NCSService::RegisterService(
    NCSSensorType sensorType,
    VOID*         pConfigParams)
{
    NCSSensor*        pSensorObject     = NULL;
    INCSIntfBase*     pNCSIntfObject    = NULL;
    CamxResult        result            = CamxResultSuccess;
    UINT              freeSlot          = 0;
    NCSSensorConfig*  pSensorConfig     = NULL;

    static const NCSIntfType InterfaceType = QSEE; // This should be passed from client

    if (pConfigParams == NULL)
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Invalid input parameters!");
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        // The sensortype is not really required as an input.
        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "NCS register client for sensor %d", sensorType);
        pSensorConfig = static_cast<NCSSensorConfig*>(pConfigParams);

        m_pNCSServiceMutex->Lock();
        pNCSIntfObject = m_pNCSIntfObject[InterfaceType];
        if (NULL != pNCSIntfObject)
        {
            for (freeSlot = 0; freeSlot < MaxSupportedSensorClients; freeSlot++)
            {
                if (NULL == m_pSensorClients[freeSlot])
                {
                    break;
                }
            }
            if (MaxSupportedSensorClients > freeSlot)
            {
                pSensorObject = CAMX_NEW  NCSSensor(pNCSIntfObject, freeSlot, InterfaceType, pSensorConfig);
                if (NULL != pSensorObject)
                {
                    result = pNCSIntfObject->RegisterService(pSensorObject);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupNCS, "Can not register Sensor client!");
                        CAMX_DELETE pSensorObject;
                        pSensorObject = NULL;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupNCS, "Can not create Sensor object!");
                    result = CamxResultENoMemory;
                }

                // When succeeds, increment num of active clients in Service context.
                if (NULL != pSensorObject)
                {
                    m_numActiveClients++;
                    // Start the NCS service when the first time a client is registered.
                    if (1 == m_numActiveClients)
                    {
                        result = StartNCSService();
                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupNCS, "Fatal: Can not Start NCS service!");
                            CAMX_DELETE pSensorObject;
                            pSensorObject = NULL;
                            m_numActiveClients--;
                        }
                        else
                        {
                            CAMX_LOG_INFO(CamxLogGroupNCS, "Started the NCS service successfully!");
                        }
                    }
                    if (NULL != pSensorObject)
                    {
                        m_pSensorClients[freeSlot] = pSensorObject;
                    }
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "NCS interface type %d exceed the maximum number of client %d!",
                               InterfaceType, MaxSupportedSensorClients);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "NCS interface type %d is not initialized", InterfaceType);
        }
        m_pNCSServiceMutex->Unlock();
    }

    if (NULL != pSensorObject)
    {
        CAMX_LOG_INFO(CamxLogGroupNCS, "Successfully register to the NCS service: obj %p, slot %d, number of clients: %d",
            pSensorObject, pSensorObject->GetClientId(), m_numActiveClients);
    }
    return pSensorObject;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSService::UnregisterService
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSService::UnregisterService(
    NCSSensor* pSensorObject)
{
    CamxResult        result             = CamxResultSuccess;
    INCSIntfBase*     pNCSIntfHandleQSEE = NULL;
    UINT              usedSlot           = 0;

    if (NULL != pSensorObject)
    {
        CAMX_LOG_INFO(CamxLogGroupNCS, "NCS Unregister client for sensor %d, obj: %p",
            pSensorObject->GetSensorType(), pSensorObject);

        m_pNCSServiceMutex->Lock();
        pNCSIntfHandleQSEE = m_pNCSIntfObject[pSensorObject->GetInterfaceType()];
        usedSlot           = pSensorObject->GetClientId();
        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "NCS Unregister client at slot %d", usedSlot);

        if (pSensorObject == m_pSensorClients[usedSlot])
        {
            if (NULL != pNCSIntfHandleQSEE)
            {
                result = pNCSIntfHandleQSEE->UnregisterService(pSensorObject);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupNCS, "Failure in unregistering the sensor object");
                }
            }

            m_numActiveClients--;
            if (0 == m_numActiveClients)
            {
                CAMX_LOG_INFO(CamxLogGroupNCS, "Stop NCS service");
                result = StopNCSService();
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupNCS, "Failure in stopping the NCS service");
                }
            }
            m_pSensorClients[usedSlot] = NULL;
        }

        CAMX_DELETE pSensorObject;
        pSensorObject = NULL;

        m_pNCSServiceMutex->Unlock();
    }

    CAMX_LOG_INFO(CamxLogGroupNCS, "Num of active client is %d", m_numActiveClients);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSService::QueryCapabilites
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSService::QueryCapabilites(
    NCSSensorCaps* pCaps,
    NCSSensorType  sensorType)
{
    CamxResult  result                     = CamxResultSuccess;

    /// todo (CAMX-2393) remove hardcode of QSEE
    static const NCSIntfType InterfaceType = QSEE; // This should be passed from client

    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Client Querying capabilities for sensor %d", sensorType);

    if (NULL == pCaps)
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Invalid input parameters");
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        m_pNCSServiceMutex->Lock();
        INCSIntfBase* pNCSIntfObject = m_pNCSIntfObject[InterfaceType];
        if (NULL != pNCSIntfObject)
        {
            result = pNCSIntfObject->FillCaps(pCaps, sensorType);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Invalid QSEE interface handle");
            result = CamxResultEInvalidPointer;
        }
        m_pNCSServiceMutex->Unlock();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSService::StartNCSService
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSService::StartNCSService()
{
    CamxResult     result             = CamxResultEFailed;
    VOID*          pThreadInputs[1]   = { m_pThreadData };

    result = m_hNCSPollThHandle.pThreadManager->RegisterJobFamily(NCSService::NCSServicePollThread,
                                                                  "NonCameraSensors",
                                                                  NULL,
                                                                  JobPriority::High,
                                                                  FALSE,
                                                                  &m_hNCSPollThHandle.hJobHandle);
    if (CamxResultSuccess == result)
    {
        if (NULL == m_pThreadData)
        {
            m_pThreadData = static_cast<NCSThreadData*>(CAMX_CALLOC(sizeof(NCSThreadData)));
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupNCS, "m_pThreadData already initialized");
        }

        if (NULL != m_pThreadData)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Starting the NCS Service thread");

            m_hNCSPollThHandle.isRunning    = TRUE;
            m_pThreadData->pJob             = NULL;
            m_pThreadData->pThreadContext   = &m_hNCSPollThHandle;
            pThreadInputs[0]                = m_pThreadData;

            m_hNCSPollThHandle.pThreadManager->PostJob(m_hNCSPollThHandle.hJobHandle,
                                                       NULL,
                                                       reinterpret_cast<VOID**>(&pThreadInputs),
                                                       FALSE,
                                                       FALSE);
        }
        else
        {
            result = m_hNCSPollThHandle.pThreadManager->UnregisterJobFamily(NULL,
                                                                            "NonCameraSensors",
                                                                            m_hNCSPollThHandle.hJobHandle);
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Out of memory!");
            result = CamxResultENoMemory;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Can not register job family!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSService::StopNCSService
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSService::StopNCSService()
{
    CamxResult result   = CamxResultSuccess;
    NCSJob*    pJob     = NULL;
    QSEEJob*   pQSEEJob = NULL;

    result = FlushNCSService();
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Flush failed !!");
    }

    m_hNCSPollThHandle.isRunning = FALSE;
    result = m_hNCSPollThHandle.pThreadManager->UnregisterJobFamily(NULL,
                                                                    "NonCameraSensors",
                                                                    m_hNCSPollThHandle.hJobHandle);
    m_hNCSPollThHandle.hJobHandle = InvalidJobHandle;

    LDLLNode* pNode = pNode = m_hNCSPollThHandle.NCSJobQueue.RemoveFromHead();
    while (NULL != pNode)
    {
        CAMX_LOG_INFO(CamxLogGroupNCS, "Leaked job exist!");

        pJob = static_cast<NCSJob*>(pNode->pData);
        if (NULL != pJob)
        {
            pQSEEJob = static_cast<QSEEJob*>(pJob->pPayload);
            if (NULL != pQSEEJob)
            {
                CAMX_FREE(pQSEEJob);
            }
            CAMX_FREE(pJob);
        }
        CAMX_FREE(pNode);
        pNode = m_hNCSPollThHandle.NCSJobQueue.RemoveFromHead();
    }

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_INFO(CamxLogGroupNCS, "NCS Service thread has been stopped");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSService::FlushNCSService
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSService::FlushNCSService()
{
    CamxResult result = CamxResultSuccess;
    NCSJob*    pJob   = NULL;

    // Flush and wait for flush to be done.
    pJob = static_cast<NCSJob*>(CAMX_CALLOC(sizeof(NCSJob)));
    if (NULL != pJob)
    {
        pJob->jobType = NCSJobTypeFlush;
        pJob->pPayload = NULL;

        // Send flush and wait for flush to be done.
        result = EnqueueJob(pJob);
        if (CamxResultSuccess == result)
        {
            // Wait for flush to be done.
            result = m_hNCSPollThHandle.pNCSFlushVar->TimedWait(1000);
            if (CamxResultETimeout == result)
            {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "Flush timedout");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "enqueue flush failed");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Out of memory!!");
        result = CamxResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSService::EnqueueJob
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSService::EnqueueJob(
    NCSJob* pJob)
{
    CamxResult result = CamxResultSuccess;

    m_hNCSPollThHandle.pNCSQMutex->Lock();
    if (InvalidJobHandle != m_hNCSPollThHandle.hJobHandle && TRUE == m_hNCSPollThHandle.isRunning)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Pushing Job %p to NCS job queue", pJob);
        if (NULL != pJob)
        {
            LDLLNode* pNode = static_cast<LDLLNode*>(CAMX_CALLOC(sizeof(LDLLNode)));
            if (NULL != pNode)
            {
                pNode->pData = pJob;
                m_hNCSPollThHandle.NCSJobQueue.InsertToTail(pNode);
            }
            else
            {
                result = CamxResultENoMemory;
            }
        }
        m_hNCSPollThHandle.pNCSQCondVar->Signal();
    }
    else
    {
        result = CamxResultEInvalidState;
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Can not enqueue jobs when thread is stopped");
    }
    m_hNCSPollThHandle.pNCSQMutex->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSService::~NCSService
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
NCSService::~NCSService()
{
    UINT i;
    BOOL bStopService = FALSE;

    for (i = 0; i < MaxSupportedSensorClients; i++)
    {
        if (NULL != m_pSensorClients[i])
        {
            m_pNCSIntfObject[m_pSensorClients[i]->GetInterfaceType()]->UnregisterService(m_pSensorClients[i]);
            CAMX_DELETE m_pSensorClients[i];
            m_pSensorClients[i] = NULL;
            bStopService = TRUE;
        }
    }

    m_numActiveClients = 0;
    if (TRUE == bStopService)
    {
        StopNCSService();
    }

    for (i = 0; i < MaxNCSIntfType; i++)
    {
        if (NULL != m_pNCSIntfObject[i])
        {
            m_pNCSIntfObject[i] = NULL;
        }
    }

    if (NULL != m_pNCSServiceMutex)
    {
        m_pNCSServiceMutex->Destroy();
        m_pNCSServiceMutex = NULL;
    }

    if (NULL != m_hNCSPollThHandle.pNCSQMutex)
    {
        m_hNCSPollThHandle.pNCSQMutex->Destroy();
        m_hNCSPollThHandle.pNCSQMutex       = NULL;
    }

    if (NULL != m_hNCSPollThHandle.pNCSQCondVar)
    {
        m_hNCSPollThHandle.pNCSQCondVar->Destroy();
        m_hNCSPollThHandle.pNCSQCondVar     = NULL;
    }

    if (NULL != m_hNCSPollThHandle.pNCSFlushVar)
    {
        m_hNCSPollThHandle.pNCSFlushVar->Destroy();
        m_hNCSPollThHandle.pNCSFlushVar = NULL;
    }

    if (NULL != m_pThreadData)
    {
        CAMX_FREE(m_pThreadData);
        m_pThreadData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSService::NCSServicePollThread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* NCSService::NCSServicePollThread(
    VOID* pPayload)
{
    NCSThreadContext* pNCSThreadContext = NULL;
    CamxResult        result            = CamxResultSuccess;
    BOOL              resetAll          = FALSE;

    CAMX_LOG_INFO(CamxLogGroupNCS, "Starting NCS poll thread");

    pNCSThreadContext = reinterpret_cast<NCSThreadData*>(pPayload)->pThreadContext;
    if (NULL != pNCSThreadContext)
    {
        while (TRUE)
        {
            // Wait for data to be pushed to Q
            pNCSThreadContext->pNCSQMutex->Lock();

            CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Before wait");
            result = pNCSThreadContext->pNCSQCondVar->TimedWait(
                pNCSThreadContext->pNCSQMutex->GetNativeHandle(), QSEECallbacksTimeout);

            // Process jobs only when somone has signalled on the queue
            if (CamxResultETimeout != result)
            {

                result = pNCSThreadContext->pServiceObject->ProcessPendingJobs(pNCSThreadContext);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupNCS, "Unable to process jobs %s", Utils::CamxResultToString(result));
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "No incoming sensor data, Reset All the connections!");
                resetAll = TRUE;
            }
            pNCSThreadContext->pNCSQMutex->Unlock();

            // if flush is is triggered then exit this thread loop.
            if (FALSE == pNCSThreadContext->isRunning)
            {
                break;
            }

            if (TRUE == resetAll)
            {
                pNCSThreadContext->pServiceObject->m_pNCSIntfObject[QSEE]->ResetConnection(NCSMaxSupportedConns);
                resetAll = FALSE;
            }
        }
        CAMX_LOG_INFO(CamxLogGroupNCS, "Exiting poll thread");
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Invalid Parameter");
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSService::ProcessPendingJobs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSService::ProcessPendingJobs(
    NCSThreadContext* pNCSThreadContext)
{
    NCSJob*           pJob              = NULL;
    QSEEJob*          pQSEEJob          = NULL;
    NCSService*       pNCSServiceObject = NULL;
    CamxResult        result            = CamxResultSuccess;

    pNCSServiceObject = static_cast<NCSService*>(pNCSThreadContext->pServiceObject);
    if (NULL != pNCSServiceObject)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "JobQ signalled, Pending Jobs %d", pNCSThreadContext->NCSJobQueue.NumNodes());
        LDLLNode* pNode = pNCSThreadContext->NCSJobQueue.RemoveFromHead();
        while (NULL != pNode)
        {
            pJob = static_cast<NCSJob*>(pNode->pData);
            CAMX_FREE(pNode);
            pNode = NULL;

            if (NULL != pJob)
            {
                switch (pJob->jobType)
                {
                    case NCSJobTypeFlush:
                        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Flushing poll thread with pending jobs %d",
                            pNCSThreadContext->NCSJobQueue.NumNodes());
                        pNCSThreadContext->isRunning = FALSE;
                        pNCSThreadContext->pNCSFlushVar->Signal();
                        break;
                    case NCSJobTypeResetConnection:
                        pQSEEJob = static_cast<QSEEJob*>(pJob->pPayload);
                        if (NULL != pQSEEJob)
                        {
                            pNCSThreadContext->pNCSQMutex->Unlock();
                            pNCSThreadContext->pServiceObject->m_pNCSIntfObject[QSEE]->ResetConnection(
                                static_cast<UINT>(pQSEEJob->connIndex));
                            pNCSThreadContext->pNCSQMutex->Lock();
                            CAMX_FREE(pQSEEJob);
                        }
                        break;
                    default:
                        CAMX_LOG_ERROR(CamxLogGroupNCS, "Unhandle job type");
                        result = CamxResultENotImplemented;
                        break;
                }

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupNCS, "Failure in processing the job");
                }
                CAMX_FREE(pJob);
                pJob = NULL;

                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "After processing Pending Jobs %d",
                                 pNCSThreadContext->NCSJobQueue.NumNodes());
            }

            pNode = pNCSThreadContext->NCSJobQueue.RemoveFromHead();
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Unable to process pending jobs, invalid pointer to the service object");
    }

    return result;
}

CAMX_NAMESPACE_END
