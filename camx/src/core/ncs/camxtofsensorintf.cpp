////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxtofsensorintf.cpp
/// @brief CamX Interface for TOF sensor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxtofsensorintf.h"
#include "camxdebugprint.h"
#include "camxatomic.h"

CAMX_NAMESPACE_BEGIN

TOFSensorIntf TOFSensorIntf::s_laserSensor;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TOFSensorIntf::TOFSensorIntf
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TOFSensorIntf::TOFSensorIntf()
{
    m_singletonObjectCreated = FALSE;
    m_refCount               = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TOFSensorIntf::GetInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TOFSensorIntf*  TOFSensorIntf::GetInstance(
    ThreadManager*   pThreadManager,
    const CHAR*      pSensorLibrary)
{
    CamxResult result         = CamxResultSuccess;

    TOFSensorIntf* pTOFObject = NULL;

    if (s_laserSensor.m_singletonObjectCreated == FALSE)
    {
        result = s_laserSensor.Initialize(pThreadManager, pSensorLibrary);
        if (result != CamxResultSuccess)
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Unable to init the TOF object error %d", result);
        }
        else
        {
            s_laserSensor.m_singletonObjectCreated = TRUE;
        }
    }

    if (CamxResultSuccess == result)
    {
        pTOFObject = &s_laserSensor;
    }

    return pTOFObject;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TOFSensorIntf::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TOFSensorIntf::Destroy()
{
    CamxResult result = CamxResultSuccess;
    CamxAtomicDecU(&m_refCount);
    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "refCount:%u", m_refCount);
    if (m_refCount == 0)
    {
        result = TearDownTOFLink();
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Failed to tear down TOF link");
        }

        if (NULL != m_pTOFDataMutex)
        {
            m_pTOFDataMutex->Destroy();
            m_pTOFDataMutex = NULL;
        }

        m_singletonObjectCreated = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TOFSensorIntf::ConfigureTOFSensor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TOFSensorIntf::ConfigureTOFSensor(
    TOFSensorConfig* pConfig)
{
    CamxResult result = CamxResultSuccess;

    struct sensors_poll_device_t* pDevice = m_TOFSensorObj.pDevice;

    if (NULL != pDevice)
    {
        if (m_refCount == 0)
        {
            INT retVal = pDevice->setDelay(pDevice, m_TOFSensorObj.hSensorHandle, pConfig->reportRate * 1000);
            if (0 != retVal)
            {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "Failure setting reporting rate. Error: %d", retVal);
            }

            m_TOFSensorObj.reportRate = pConfig->reportRate;

            CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Configure report rate: %u us", m_TOFSensorObj.reportRate);
            /// Activate the sensor now
            result = ActivateTOFSensor(TRUE);

            if (CamxResultSuccess == result)
            {
                /// Start a thread to poll for TOF sensor data.  Poll is a blocking call.
                /// Register thread family only once.
                result = StartPollThread();
                if (CamxResultSuccess == result)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Started thread for laser sensor successfully");
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupNCS, "Failed to start thread for TOF sensor");
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "Failed to activate TOF sensor");
            }
        }
        if (CamxResultSuccess == result)
        {
            CamxAtomicIncU(&m_refCount);
        }

        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "refCount:%u", m_refCount);
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupNCS, "No device!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TOFSensorIntf::GetLastNSamples
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TOFSensorIntf::GetLastNSamples(
    DataTOF* pData,
    INT      numOfSamples)
{
    INT        readIndex      = -1;
    CamxResult result         = CamxResultSuccess;
    UINT64     currTimestamp  = 0;
    UINT64     validTimeRange = 0;

    CAMX_ASSERT(NULL != pData);

    CAMX_ASSERT_MESSAGE(NULL != m_pTOFDataMutex, "Invalid m_pTOFDataMutex %p", m_pTOFDataMutex);

    m_pTOFDataMutex->Lock();

    if ((m_latestSampleIndex < 0) || (TOFMaxSamples < numOfSamples))
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "%d sample(s) not available", numOfSamples);
        result = CamxResultEFailed;
    }
    else
    {
        currTimestamp  = OsUtils::GetNanoSeconds();
        // Calculate the valid time range to identify if sample returned is relevant
        validTimeRange = (numOfSamples + TOFSampleThresh) * m_TOFSensorObj.reportRate * 1000;
        // find the index to start reading samples from TOF data queue
        readIndex      = (m_latestSampleIndex - numOfSamples + TOFMaxSamples + 1) % TOFMaxSamples;

        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "currTimestamp %llu validTimeRange %llu", currTimestamp, validTimeRange);

        // Check if the sample(s) available is within valid range. It makes sure that the
        // sample returned is the latest one.
        if (m_TOFData[readIndex].timestamp < (currTimestamp - validTimeRange))
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Latest samples not available. Last available sample: %llu",
                m_TOFData[readIndex].timestamp);
            result = CamxResultEFailed;
        }
        else
        {
            for (INT i = 0; i < numOfSamples; i++, readIndex++)
            {
                readIndex = readIndex % TOFMaxSamples;
                Utils::Memcpy(&pData[i], &m_TOFData[readIndex], sizeof(DataTOF));

                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "m_TOFData[%d]: %f %f %f %d %f %llu %f %f %f",
                    readIndex,
                    m_TOFData[readIndex].confidence,
                    m_TOFData[readIndex].distance,
                    m_TOFData[readIndex].farLimit,
                    m_TOFData[readIndex].maxRange,
                    m_TOFData[readIndex].nearLimit,
                    m_TOFData[readIndex].timestamp,
                    m_TOFData[readIndex].type,
                    m_TOFData[readIndex].version,
                    m_TOFData[readIndex].ambientRate);
            }
        }
    }
    m_pTOFDataMutex->Unlock();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TOFSensorIntf::~TOFSensorIntf
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TOFSensorIntf::~TOFSensorIntf()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TOFSensorIntf::StartPollThread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TOFSensorIntf::StartPollThread()
{
    CamxResult     result           = CamxResultEFailed;
    TOFThreadData* pThreadData      = NULL;
    VOID*          pThreadInputs[1] = { pThreadData };

    result = m_hTOFPollThHandle.pThreadManager->RegisterJobFamily(TOFSensorIntf::TOFPollThread,
                                                                  "TOFSensor",
                                                                  NULL,
                                                                  JobPriority::Normal,
                                                                  FALSE,
                                                                  &m_hTOFPollThHandle.hJobHandle);

    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Starting the polling thread for TOF");

    if (CamxResultSuccess == result)
    {
        m_hTOFPollThHandle.threadStatus     = TOFThreadStatusOff;
        m_hTOFPollThHandle.pTOFStateMutex   = Mutex::Create("TOFStateMutex");
        m_hTOFPollThHandle.pTOFStateCondVar = Condition::Create("TOFStateCondVar");
        m_hTOFPollThHandle.pSensorIntf      = this;

        pThreadData = static_cast<TOFThreadData*>(CAMX_CALLOC(sizeof(TOFThreadData)));

        if ((NULL == m_hTOFPollThHandle.pTOFStateMutex) ||
            (NULL == m_hTOFPollThHandle.pTOFStateCondVar) ||
            (NULL == pThreadData))
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Unable to create new object, no memory"
                "pTOFStateMutex %p, pTOFStateCondVar %p, pThreadData %p",
                m_hTOFPollThHandle.pTOFStateMutex,
                m_hTOFPollThHandle.pTOFStateCondVar,
                pThreadData);

            if (NULL != m_hTOFPollThHandle.pTOFStateMutex)
            {
                m_hTOFPollThHandle.pTOFStateMutex->Destroy();
                m_hTOFPollThHandle.pTOFStateMutex   = NULL;
            }

            if (NULL != m_hTOFPollThHandle.pTOFStateCondVar)
            {
                m_hTOFPollThHandle.pTOFStateCondVar->Destroy();
                m_hTOFPollThHandle.pTOFStateCondVar = NULL;
            }

            if (NULL != pThreadData)
            {
                CAMX_FREE(pThreadData);
                pThreadData = NULL;
            }
            result = CamxResultENoMemory;
        }
        else
        {
            pThreadData->pThreadContext = &m_hTOFPollThHandle;

            pThreadInputs[0]   = pThreadData;
            m_hTOFPollThHandle.pThreadManager->PostJob(m_hTOFPollThHandle.hJobHandle,
                                                       NULL,
                                                       reinterpret_cast<VOID**>(&pThreadInputs),
                                                       FALSE,
                                                       FALSE);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TOFSensorIntf::StopPollThread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TOFSensorIntf::StopPollThread()
{
    CamxResult result = CamxResultSuccess;

    m_hTOFPollThHandle.threadStatus = TOFThreadStatusOff;

    if ((NULL == m_hTOFPollThHandle.pTOFStateMutex) ||
        (NULL == m_hTOFPollThHandle.pTOFStateCondVar))
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Invalid ptr");
        result = CamxResultEFailed;
    }
    else
    {
        m_hTOFPollThHandle.pTOFStateMutex->Lock();
        result = m_hTOFPollThHandle.pTOFStateCondVar->TimedWait(m_hTOFPollThHandle.pTOFStateMutex->GetNativeHandle(),
                                                                1000);
        if (CamxResultETimeout == result)
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "stop timedout");
        }

        m_hTOFPollThHandle.pTOFStateMutex->Unlock();

        if (InvalidJobHandle != m_hTOFPollThHandle.hJobHandle)
        {

            result = m_hTOFPollThHandle.pThreadManager->UnregisterJobFamily(NULL,
                                                                        "TOFSensor",
                                                                        m_hTOFPollThHandle.hJobHandle);

            if (CamxResultSuccess == result)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Stopped the TOF polling thread");
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "Failed to stopped the TOF polling thread");
            }

            m_hTOFPollThHandle.hJobHandle = InvalidJobHandle;
        }
    }

    if (NULL != m_hTOFPollThHandle.pTOFStateMutex)
    {
        m_hTOFPollThHandle.pTOFStateMutex->Destroy();
        m_hTOFPollThHandle.pTOFStateMutex   = NULL;
    }

    if (NULL != m_hTOFPollThHandle.pTOFStateCondVar)
    {
        m_hTOFPollThHandle.pTOFStateCondVar->Destroy();
        m_hTOFPollThHandle.pTOFStateCondVar = NULL;
    }


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TOFSensorIntf::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TOFSensorIntf::Initialize(
    ThreadManager*   pThreadManager,
    const CHAR*      pSensorLibrary)
{
    CamxResult result = CamxResultSuccess;

    m_pTOFDataMutex     = NULL;
    m_latestSampleIndex = -1;
    m_pTOFDataMutex     = Mutex::Create("TOFDataMutex");

    if (NULL == m_pTOFDataMutex)
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Unable to create mutex, no memory m_pTOFDataMutex %p", m_pTOFDataMutex);
        result = CamxResultENoMemory;
    }
    else
    {
        if (NULL != pThreadManager && NULL != pSensorLibrary)
        {
            m_hTOFPollThHandle.pThreadManager = pThreadManager;

            /// Setup TOF sensor connection
            result = SetupTOFLink(pSensorLibrary);
            if (CamxResultSuccess == result)
            {
                result        = GetSensorCapability();
                if (CamxResultSuccess == result)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Probed TOF sensor");
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupNCS, "Unable to get list of sensors");
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "Failed to setup TOF Link");
            }
        }
        else
        {
            m_hTOFPollThHandle.pThreadManager = NULL;
            result                            = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Initialize information missing!");
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Thread Manager=%p, Sensor Library=%p", pThreadManager, pSensorLibrary);
        }

        if (CamxResultSuccess != result)
        {
            m_pTOFDataMutex->Destroy();
            m_pTOFDataMutex = NULL;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TOFSensorIntf::SetupTOFLink
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TOFSensorIntf::SetupTOFLink(
    const CHAR*      pSensorLibrary)
{
    CamxResult                    result          = CamxResultSuccess;
    OSLIBRARYHANDLE               hSensorLib      = NULL;
    struct sensors_module_t*      pModule         = NULL;
    struct sensors_poll_device_t* pDevice         = NULL;

    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Open TOF sensor library: %s", pSensorLibrary);
    hSensorLib = OsUtils::LibMap(pSensorLibrary);
    if (NULL == hSensorLib)
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Failure opening TOF sensor library!");
        result = CamxResultEUnableToLoad;
    }

    if (CamxResultSuccess == result)
    {
        /// Current TOF sensor library implements Android sensor HAL interface. As per specification, every hardware
        /// module must have a data structure named HAL_MODULE_INFO_SYM
        /// Get sensor module handle address

        pModule = static_cast<struct sensors_module_t *> (OsUtils::LibGetAddr(hSensorLib, HAL_MODULE_INFO_SYM_AS_STR));
        if (NULL == pModule)
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Failure getting TOF sensor module address!");
            OsUtils::LibUnmap(hSensorLib);
            hSensorLib = NULL;
            result     = CamxResultEFailed;
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupNCS, "TOF HW Module id: %s name: %s", pModule->common.id, pModule->common.name);

            /// Call open sensor for the module
            INT retVal = pModule->common.methods->open(&pModule->common, SENSORS_HARDWARE_POLL, (hw_device_t **)&pDevice);
            if (0 != retVal)
            {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "Failure opening TOF sensor module! Error - %d.", retVal);
                OsUtils::LibUnmap(hSensorLib);
                hSensorLib = NULL;
                pModule    = NULL;
                result     = CamxResultEFailed;
            }
        }
    }

    m_TOFSensorObj.phLibHandle = static_cast<VOID *>(hSensorLib);
    m_TOFSensorObj.pModule     = pModule;
    m_TOFSensorObj.pDevice     = pDevice;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TOFSensorIntf::TearDownTOFLink
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TOFSensorIntf::TearDownTOFLink()
{
    CamxResult result = CamxResultSuccess;

    /// If we already have thread running
    if (TOFThreadStatusOff != m_hTOFPollThHandle.threadStatus)
    {
        /// Stop the polling thread
        result = StopPollThread();
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Failed to stop TOF sensor poll thread");
        }
    }

    /// Disable the sensor
    result = ActivateTOFSensor(FALSE);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Failed to deactivate TOF sensor");
    }
    /// Close the sensor
    if (NULL != m_TOFSensorObj.pDevice)
    {
        struct sensors_poll_device_t* pDevice = m_TOFSensorObj.pDevice;
        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "closing sensor device.");
        pDevice->common.close(static_cast<hw_device_t*>(&pDevice->common));
    }

    /// Close the library as well
    result = OsUtils::LibUnmap(m_TOFSensorObj.phLibHandle);

    m_TOFSensorObj.phLibHandle = NULL;
    m_TOFSensorObj.pModule     = NULL;
    m_TOFSensorObj.pDevice     = NULL;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TOFSensorIntf::ActivateTOFSensor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TOFSensorIntf::ActivateTOFSensor(
    BOOL onOffFlag)
{
    CamxResult                    result  = CamxResultSuccess;
    struct sensors_poll_device_t* pDevice = m_TOFSensorObj.pDevice;

    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "onOffFlag %d", onOffFlag);
    if (NULL != pDevice)
    {
        INT  retVal = pDevice->activate(pDevice, m_TOFSensorObj.hSensorHandle, (onOffFlag == TRUE) ? 1 : 0);

        if (retVal < 0)
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Failure Activating/DeActivating TOF sensor! OnOffFlag: %d Error: %d",
                onOffFlag,
                retVal);
            result = CamxResultEFailed;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "TOF device access invalid");
        result = CamxResultENoSuch;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TOFSensorIntf::TOFPollThread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* TOFSensorIntf::TOFPollThread(
    VOID* pData)
{
    TOFThreadContext*             pTOFThreadContext = NULL;
    TOFSensorIntf*                pSensorIntf       = NULL;
    struct sensors_poll_device_t* pDevice           = NULL;
    sensors_event_t               event;

    pTOFThreadContext = reinterpret_cast<TOFThreadData*>(pData)->pThreadContext;
    pSensorIntf       = static_cast<TOFSensorIntf*>(pTOFThreadContext->pSensorIntf);
    pDevice           = pSensorIntf->m_TOFSensorObj.pDevice;

    CAMX_FREE(pData);
    pData = NULL;

    pTOFThreadContext->threadStatus = TOFThreadStatusOn;

    CAMX_LOG_INFO(CamxLogGroupNCS, "Thread handler in.");
    do
    {
        INT count = pDevice->poll(pDevice, &event, 1);
        if (count < 0)
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Failure polling the sensor! Count: %d!", count);
            break;
        }

        pSensorIntf->FillTOFSensorData(&event);

    } while (pTOFThreadContext->threadStatus == TOFThreadStatusOn);

    if ((NULL != pTOFThreadContext->pTOFStateMutex) &&
        (NULL != pTOFThreadContext->pTOFStateCondVar))
    {
        pTOFThreadContext->pTOFStateMutex->Lock();
        pTOFThreadContext->pTOFStateCondVar->Signal();
        pTOFThreadContext->pTOFStateMutex->Unlock();
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TOFSensorIntf::GetSensorCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TOFSensorIntf::GetSensorCapability()
{
    CamxResult result = CamxResultSuccess;

    if (NULL != m_TOFSensorObj.phLibHandle)
    {
        struct sensors_module_t*      pModule = m_TOFSensorObj.pModule;
        struct sensor_t const*        pSensorList;

        INT sensorCount = pModule->get_sensors_list(pModule, &pSensorList);
        if (0 == sensorCount)
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "TOF sensor not supported!");
            result = CamxResultENoSuch;
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupNCS, "TOF sensor name: %s, vendor: %s, handle: %d, type: %d, "
                "maxRange: %f, minDelay: %d",
                pSensorList[0].name, pSensorList[0].vendor, pSensorList[0].handle, pSensorList[0].type,
                pSensorList[0].maxRange, pSensorList[0].minDelay);
            m_TOFSensorObj.maxRange      = static_cast<INT>(pSensorList[0].maxRange);
            m_TOFSensorObj.hSensorHandle = pSensorList->handle;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "TOF sensor not inited!");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TOFSensorIntf::FillTOFSensorData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TOFSensorIntf::FillTOFSensorData(
    sensors_event_t* pSensorData)
{
    CAMX_ASSERT(NULL != pSensorData);

    if (NULL != m_pTOFDataMutex)
    {
        m_pTOFDataMutex->Lock();

        m_latestSampleIndex                         = (m_latestSampleIndex + 1) % TOFMaxSamples;
        m_TOFData[m_latestSampleIndex].timestamp    = pSensorData->timestamp;
        m_TOFData[m_latestSampleIndex].version      = pSensorData->data[0];
        m_TOFData[m_latestSampleIndex].type         = pSensorData->data[1];
        m_TOFData[m_latestSampleIndex].distance     = pSensorData->data[2];
        m_TOFData[m_latestSampleIndex].confidence   = pSensorData->data[3];
        m_TOFData[m_latestSampleIndex].nearLimit    = pSensorData->data[4];
        m_TOFData[m_latestSampleIndex].farLimit     = pSensorData->data[5];
        m_TOFData[m_latestSampleIndex].maxRange     = m_TOFSensorObj.maxRange;
        m_TOFData[m_latestSampleIndex].ambientRate  = pSensorData->data[12];

        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "m_TOFData[%d]: %f %f %f %d %f %llu %f %f",
            m_latestSampleIndex,
            m_TOFData[m_latestSampleIndex].confidence,
            m_TOFData[m_latestSampleIndex].distance,
            m_TOFData[m_latestSampleIndex].farLimit,
            m_TOFData[m_latestSampleIndex].maxRange,
            m_TOFData[m_latestSampleIndex].nearLimit,
            m_TOFData[m_latestSampleIndex].timestamp,
            m_TOFData[m_latestSampleIndex].type,
            m_TOFData[m_latestSampleIndex].version,
            m_TOFData[m_latestSampleIndex].ambientRate);

        m_pTOFDataMutex->Unlock();
    }

}

CAMX_NAMESPACE_END
