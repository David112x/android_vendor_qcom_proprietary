////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Copyright (c) 2016-2018 Qualcomm Technologies, Inc.
/// All Rights Reserved.
/// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxsyncmanager.cpp
/// @brief Class definition for the sync manager class useful for accessing
/// sync services from the sync framework.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if ANDROID

#include "camxincs.h"
#include "camxsyncmanager.h"

using namespace std;

CAMX_NAMESPACE_BEGIN

struct SyncManagerCtrl SyncManager::s_ctrl;
const CHAR* SyncManager::pExitThreadCmd = "EXIT";
SyncManager* SyncManager::s_pSyncManagerInstance = NULL;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyncManager::CbDispatchJob
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* SyncManager::CbDispatchJob(
    VOID* pData)
{
    CAMX_ENTRYEXIT_NAME(CamxLogGroupCore, "SyncManager::CbDispatchJob");
    struct    v4l2_event ev;
    struct    cam_sync_ev_header* pEvHeader;
    uint64_t* pPayloadData = NULL;


    Utils::Memcpy(&ev, reinterpret_cast<struct v4l2_event*> (pData), sizeof(ev));

    pEvHeader = CAM_SYNC_GET_HEADER_PTR(ev);
    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Signal status = %d", pEvHeader->status);
    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Sync obj = %d", pEvHeader->sync_obj);

    pPayloadData = CAM_SYNC_GET_PAYLOAD_PTR(ev, uint64_t);
    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Dispatch Payload data0 = %llx", pPayloadData[0]);
    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Dispatch Payload data1 = %llx", pPayloadData[1]);

    if ((0 == pPayloadData[0]) || (0 == pPayloadData[1]))
    {
        CAMX_LOG_ERROR(CamxLogGroupSync, "Got NULL Payload Data from Kernel!!!! -- %llx %llx",
                       pPayloadData[0], pPayloadData[1]);
    }
    else
    {
        CSLFenceResult fenceResult = CSLFenceResultSuccess;
        UINT32 kernelResult = static_cast<UINT32>(pEvHeader->status);

        if (kernelResult != CAM_SYNC_STATE_SIGNALED_SUCCESS)
        {
            fenceResult = CSLFenceResultFailed;
        }

        (reinterpret_cast<CSLFenceHandler>(pPayloadData[0]))(reinterpret_cast<VOID* >(pPayloadData[1]),
                                           pEvHeader->sync_obj,
                                           fenceResult);
    }

    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Freeing up %p for sync obj = %d",
                     reinterpret_cast<struct v4l2_event* >(pData),
                     pEvHeader->sync_obj);
    CAMX_DELETE reinterpret_cast<struct v4l2_event*> (pData);
    pData = NULL;

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyncManager::FlushCbDispatchJob
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SyncManager::FlushCbDispatchJob(
    VOID* pData)
{
    CAMX_UNREFERENCED_PARAM(pData);

    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Flush of dispatch job called!");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyncManager::StoppedCbDispatchJob
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SyncManager::StoppedCbDispatchJob(
    VOID* pData)
{
    CAMX_UNREFERENCED_PARAM(pData);

    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Dispatch job stopped!");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyncManager::SyncManagerPollMethod
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* SyncManager::SyncManagerPollMethod(
    VOID* pPollData)
{
    struct SyncManagerCtrl* pCtrl = static_cast<struct SyncManagerCtrl*>(pPollData);
    struct v4l2_event* pEv = NULL;
    struct pollfd fds[2];
    CHAR pipeBuff[32] = {0};
    INT rc;
    CamxResult result;

    fds[0].fd = pCtrl->syncFd;
    fds[0].events = POLLPRI;

    fds[1].fd = GetReadFDFromPipe();
    fds[1].events = POLLIN;

    while (1)
    {
        rc = poll(fds, 2, -1);
        if (rc > 0)
        {
            if (fds[0].revents & POLLPRI)
            {
                // We have something on the device node
                // Allocate this here, free it in dispatch handler
                CAMX_LOG_VERBOSE(CamxLogGroupSync, "Got V4L2 event!");
                pEv = CAMX_NEW struct v4l2_event;
                if (NULL == pEv)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupSync, "No memory");
                    break;
                }

                rc = ioctl(pCtrl->syncFd, VIDIOC_DQEVENT, pEv);
                if (pEv->type == CAM_SYNC_V4L_EVENT)
                {
                    VOID* pData[] = {pEv, NULL};
                    struct cam_sync_ev_header* pEvHeader;
                    pEvHeader = CAM_SYNC_GET_HEADER_PTR((*pEv));

                    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Dispatching CB!");
                    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Allocating %p for sync obj = %d", pEv, pEvHeader->sync_obj);

                    result = pCtrl->pThreadManager->PostJob(pCtrl->hJob, SyncManager::StoppedCbDispatchJob,
                                                            pData, FALSE, FALSE);
                }
                else
                {
                    CAMX_DELETE pEv;
                    pEv = NULL;
                }
            }

            if (fds[1].revents & POLLIN)
            {
                // We have something on the IPC pipe
                CAMX_LOG_VERBOSE(CamxLogGroupSync, "Got cmd!");
                read(fds[1].fd, pipeBuff, sizeof(pipeBuff));
                if (0 == strncmp(pipeBuff, pExitThreadCmd, sizeof(pipeBuff)))
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Exiting!");
                    break;
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSync, "Unrecognized Cmd!");
                }
            }
        }
        else
        {
            CHAR errnoStr[100] = {0};
            OsUtils::StrError(errnoStr, sizeof(errnoStr), errno);
            CAMX_LOG_VERBOSE(CamxLogGroupSync, "Poll break: %d: %s", rc, errnoStr);
        }
    }

    close(GetReadFDFromPipe());
    close(GetWriteFDFromPipe());
    close(pCtrl->syncFd);
    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Thread killed");
    pthread_exit(NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyncManager::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SyncManager::Initialize(
    const CHAR* pDeviceName)
{
    INT rc;
    CamxResult result;

    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Sync Init!");

    // Return if already initialized before
    if (s_ctrl.syncFd >= 0)
    {
        /// @todo (CAMX-1015) No early/multiple returns
        return CamxResultSuccess;
    }

    result = pipe(s_ctrl.pipeFDs);
    if (result < 0)
    {
        return CamxResultEFailed;
    }

    result = ThreadManager::Create(&s_ctrl.pThreadManager, "SyncManager", 1);

    if (CamxResultSuccess != result)
    {
        close(s_ctrl.pipeFDs[0]);
        close(s_ctrl.pipeFDs[1]);
        return CamxResultEFailed;
    }

    // Register the callback dispatch job first
    result = s_ctrl.pThreadManager->RegisterJobFamily(SyncManager::CbDispatchJob, pCamxSyncCBDispatchJob,
        SyncManager::FlushCbDispatchJob,
        JobPriority::High, TRUE, &s_ctrl.hJob);

    if (CamxResultSuccess != result)
    {
        close(s_ctrl.pipeFDs[0]);
        close(s_ctrl.pipeFDs[1]);
        s_ctrl.pThreadManager->Destroy();
        s_ctrl.pThreadManager = NULL;
        return CamxResultEFailed;
    }

    s_ctrl.syncFd = open(pDeviceName, O_RDWR);
    struct v4l2_event_subscription sub = {};

    if (s_ctrl.syncFd < 0)
    {
        CAMX_LOG_ERROR(CamxLogGroupSync, "Failed to open sync device: sync_fd = %d", s_ctrl.syncFd);
        close(s_ctrl.pipeFDs[0]);
        close(s_ctrl.pipeFDs[1]);
        s_ctrl.pThreadManager->Destroy();
        s_ctrl.pThreadManager = NULL;
        return CamxResultEFailed;
    }

    OsUtils::StrLCpy(s_ctrl.deviceName, pDeviceName, DeviceNameSize);
    sub.id = CAM_SYNC_V4L_EVENT_ID_CB_TRIG;
    sub.type = CAM_SYNC_V4L_EVENT;

    rc = ioctl(s_ctrl.syncFd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    if (rc < 0)
    {
        CAMX_LOG_ERROR(CamxLogGroupSync, "v4l2 subscribe event ioctl() failed, rc=%d", rc);
        close(s_ctrl.pipeFDs[0]);
        close(s_ctrl.pipeFDs[1]);
        close(s_ctrl.syncFd);
        s_ctrl.pThreadManager->Destroy();
        s_ctrl.pThreadManager = NULL;
        return CamxResultEFailed;
    }

    result = OsUtils::ThreadCreate(SyncManagerPollMethod,
        &s_ctrl,
        &s_ctrl.hPollThread);

    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Creating hPollThread = %u", s_ctrl.hPollThread);

    if (result != CamxResultSuccess)
    {
        CAMX_LOG_ERROR(CamxLogGroupSync, "Failed to create polling thread");
        close(s_ctrl.pipeFDs[0]);
        close(s_ctrl.pipeFDs[1]);
        close(s_ctrl.syncFd);
        s_ctrl.pThreadManager->Destroy();
        s_ctrl.pThreadManager = NULL;
        return CamxResultEFailed;
    }

    s_ctrl.pCamxPollThreadName = "CAMX_Poll_Thread";
    pthread_setname_np(s_ctrl.hPollThread, s_ctrl.pCamxPollThreadName);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyncManager::Uninitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SyncManager::Uninitialize()
{
    struct v4l2_event_subscription sub = {};
    INT rc = 0;
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Sync Uninit!");

    // Unsubscribe the V4L event
    sub.id = CAM_SYNC_V4L_EVENT_ID_CB_TRIG;
    sub.type = CAM_SYNC_V4L_EVENT;
    rc = ioctl(s_ctrl.syncFd, VIDIOC_UNSUBSCRIBE_EVENT, &sub);
    if (rc < 0)
    {
        CAMX_LOG_ERROR(CamxLogGroupSync, "v4l2 unsubscribe event ioctl() failed, rc=%d", rc);
        result = CamxResultEFailed;
    }

    // Destroy thread manager instance
    if (NULL != s_ctrl.pThreadManager)
    {
        if (InvalidJobHandle != s_ctrl.hJob)
        {
            s_ctrl.pThreadManager->UnregisterJobFamily(SyncManager::CbDispatchJob, pCamxSyncCBDispatchJob, s_ctrl.hJob);
        }

        s_ctrl.pThreadManager->Destroy();
        s_ctrl.pThreadManager = NULL;
    }

    // Exit polling thread
    write(GetWriteFDFromPipe(), pExitThreadCmd, OsUtils::StrLen(pExitThreadCmd) + 1);
    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Joining hPollThread = %u", s_ctrl.hPollThread);
    OsUtils::ThreadWait(s_ctrl.hPollThread);
    s_ctrl.hPollThread = 0;

    // Close open file fds
    close(s_ctrl.syncFd);
    close(s_ctrl.pipeFDs[0]);
    close(s_ctrl.pipeFDs[1]);
    s_ctrl.syncFd = -1;
    s_ctrl.pipeFDs[0] = -1;
    s_ctrl.pipeFDs[1] = -1;

    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Completed Sync FW tear down");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyncManager::CreateSync
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SyncManager::CreateSync(
    const CHAR* pName,
    INT32* pSyncid)
{
    INT rc;
    struct cam_private_ioctl_arg sync_ioctl  = {};
    struct cam_sync_info         sync_create = {};
    CamxResult                   result      = CamxResultSuccess;

    OsUtils::StrLCpy(sync_create.name, pName, 64);

    sync_ioctl.id        = CAM_SYNC_CREATE;
    sync_ioctl.size      = sizeof(sync_create);
    sync_ioctl.ioctl_ptr = CamX::Utils::VoidPtrToUINT64(&sync_create);

    rc = ioctl(s_ctrl.syncFd,
        CAM_PRIVATE_IOCTL_CMD,
        &sync_ioctl);

    if (rc < 0)
    {
        result = CamxResultEFailed;
    }

    if (CamxResultSuccess == result)
    {
        *pSyncid = reinterpret_cast<struct cam_sync_info *>(sync_ioctl.ioctl_ptr)->sync_obj;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Create syncObj %d, result %d",
                   reinterpret_cast<struct cam_sync_info *>(sync_ioctl.ioctl_ptr)->sync_obj,
                   result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyncManager::DestroySync
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SyncManager::DestroySync(
    INT32 syncObj)
{
    INT                          rc         = 0;
    struct cam_private_ioctl_arg sync_ioctl = {};
    struct cam_sync_info         sync_info  = {};

    CAMX_LOG_VERBOSE(CamxLogGroupSync, "In destroy sync api");

    sync_info.sync_obj = syncObj;

    sync_ioctl.id        = CAM_SYNC_DESTROY;
    sync_ioctl.size      = sizeof(sync_info);
    sync_ioctl.ioctl_ptr = CamX::Utils::VoidPtrToUINT64(&sync_info);

    rc = ioctl(s_ctrl.syncFd,
        CAM_PRIVATE_IOCTL_CMD,
        &sync_ioctl);

    if (rc < 0)
    {
        CAMX_LOG_ERROR(CamxLogGroupSync, "Sync destroy IOCTL failed");
        return CamxResultEFailed;
    }
    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyncManager::Signal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SyncManager::Signal(
    INT32 syncObj,
    enum SyncSignalResult result)
{
    INT                          rc          = 0;
    INT                          state       = 0;
    struct cam_private_ioctl_arg sync_ioctl  = {};
    struct cam_sync_signal       sync_signal = {};

    if (SyncSignalSuccess == result)
    {
        state = CAM_SYNC_STATE_SIGNALED_SUCCESS;
    }
    else
    {
        state = CAM_SYNC_STATE_SIGNALED_ERROR;
    }

    sync_signal.sync_obj = syncObj;
    sync_signal.sync_state = state;

    sync_ioctl.id        = CAM_SYNC_SIGNAL;
    sync_ioctl.size      = sizeof(sync_signal);
    sync_ioctl.ioctl_ptr = CamX::Utils::VoidPtrToUINT64(&sync_signal);

    rc = ioctl(s_ctrl.syncFd,
        CAM_PRIVATE_IOCTL_CMD,
        &sync_ioctl);

    if (rc < 0)
    {
        CAMX_LOG_ERROR(CamxLogGroupSync,
                       "Sync signal IOCTL failed: syncObj:%d rc:%d",
                       syncObj,
                       rc);
        return CamxResultEFailed;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Signal syncObj %d", syncObj);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyncManager::Merge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SyncManager::Merge(
    INT32* pSyncobj,
    INT    numObjs,
    INT32* pMerged)
{
    INT                          rc         = 0;
    struct cam_private_ioctl_arg sync_ioctl = {};
    struct cam_sync_merge        sync_merge = {};
    UINT32*                      pTempArray = NULL;

    CAMX_LOG_VERBOSE(CamxLogGroupSync, "In Signal sync api");

    pTempArray = CAMX_NEW UINT32 [numObjs];
    if (NULL == pTempArray)
    {
        return CamxResultENoMemory;
    }
    Utils::Memcpy(pTempArray, pSyncobj, sizeof(UINT32) * numObjs);

    sync_merge.sync_objs = CamX::Utils::VoidPtrToUINT64(pTempArray);
    sync_merge.num_objs  = numObjs;

    sync_ioctl.id        = CAM_SYNC_MERGE;
    sync_ioctl.size      = sizeof(sync_merge);
    sync_ioctl.ioctl_ptr = CamX::Utils::VoidPtrToUINT64(&sync_merge);

    rc = ioctl(s_ctrl.syncFd,
        CAM_PRIVATE_IOCTL_CMD,
        &sync_ioctl);

    if (rc < 0)
    {
        CAMX_LOG_ERROR(CamxLogGroupSync, "Sync signal IOCTL failed");
        return CamxResultEFailed;
    }

    *pMerged = reinterpret_cast<struct cam_sync_merge *>(sync_ioctl.ioctl_ptr)->merged;
    CAMX_DELETE[] pTempArray;
    pTempArray = NULL;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyncManager::RegisterCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SyncManager::RegisterCallback(
    INT32 syncObj,
    CSLFenceHandler cb,
    VOID* pUserData)
{
    INT                              rc           = 0;
    struct cam_private_ioctl_arg     sync_ioctl   = {};
    struct cam_sync_userpayload_info payload_info = {};

    CAMX_LOG_VERBOSE(CamxLogGroupSync, "In register payload sync api");

    payload_info.sync_obj   = syncObj;
    payload_info.payload[0] = CamX::Utils::VoidPtrToUINT64(reinterpret_cast<VOID*>(cb));
    payload_info.payload[1] = CamX::Utils::VoidPtrToUINT64(pUserData);

    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Calling register cb with info");
    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Dispatch Reg func ptr = %llx", payload_info.payload[0]);
    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Dispatch Reg data ptr = %llx", payload_info.payload[1]);
    CAMX_LOG_VERBOSE(CamxLogGroupSync, "sync_obj = %u", payload_info.sync_obj);

    sync_ioctl.id        = CAM_SYNC_REGISTER_PAYLOAD;
    sync_ioctl.size      = sizeof(payload_info);
    sync_ioctl.ioctl_ptr = CamX::Utils::VoidPtrToUINT64(&payload_info);

    rc = ioctl(s_ctrl.syncFd,
        CAM_PRIVATE_IOCTL_CMD,
        &sync_ioctl);

    if (rc < 0)
    {
        CAMX_LOG_ERROR(CamxLogGroupSync,
                       "Sync register CB IOCTL failed: syncObj:%d cb:%p pUserData:%p rc:%d",
                       syncObj,
                       cb,
                       pUserData,
                       rc);
        return CamxResultEFailed;
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyncManager::DeregisterCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SyncManager::DeregisterCallback(
    INT32 syncObj,
    CSLFenceHandler cb,
    VOID* pUserData)
{
    INT                              rc           = 0;
    struct cam_private_ioctl_arg     sync_ioctl   = {};
    struct cam_sync_userpayload_info payload_info = {};

    CAMX_LOG_VERBOSE(CamxLogGroupSync, "In de-register payload sync api");

    payload_info.sync_obj   = syncObj;
    payload_info.payload[0] = CamX::Utils::VoidPtrToUINT64(reinterpret_cast<VOID*>(cb));
    payload_info.payload[1] = CamX::Utils::VoidPtrToUINT64(pUserData);

    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Calling deregister cb with info");
    CAMX_LOG_VERBOSE(CamxLogGroupSync, "func ptr = %llx", payload_info.payload[0]);
    CAMX_LOG_VERBOSE(CamxLogGroupSync, "data ptr = %llx", payload_info.payload[1]);
    CAMX_LOG_VERBOSE(CamxLogGroupSync, "sync_obj = %u", payload_info.sync_obj);

    sync_ioctl.id        = CAM_SYNC_DEREGISTER_PAYLOAD;
    sync_ioctl.size      = sizeof(payload_info);
    sync_ioctl.ioctl_ptr = CamX::Utils::VoidPtrToUINT64(&payload_info);

    rc = ioctl(s_ctrl.syncFd,
        CAM_PRIVATE_IOCTL_CMD,
        &sync_ioctl);

    if (rc < 0)
    {
        CAMX_LOG_ERROR(CamxLogGroupSync, "Sync deregister CB IOCTL failed");
        return CamxResultEFailed;
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyncManager::Wait
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SyncManager::Wait(
    INT32 syncObj,
    UINT64 timeOut)
{
    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupCore, "SyncManager::Wait %lld", timeOut);
    INT                          rc           = 0;
    INT32                        ioctl_result = {};
    struct cam_private_ioctl_arg sync_ioctl   = {};
    struct cam_sync_wait         sync_wait    = {};
    CamxResult                   result       = CamxResultEFailed;

    CAMX_LOG_VERBOSE(CamxLogGroupSync, "In Signal wait api");

    sync_wait.sync_obj = syncObj;
    sync_wait.timeout_ms = timeOut;

    sync_ioctl.id        = CAM_SYNC_WAIT;
    sync_ioctl.size      = sizeof(sync_wait);
    sync_ioctl.ioctl_ptr = CamX::Utils::VoidPtrToUINT64(&sync_wait);

    rc = ioctl(s_ctrl.syncFd,
        CAM_PRIVATE_IOCTL_CMD,
        &sync_ioctl);

    if (rc < 0)
    {
        CAMX_LOG_ERROR(CamxLogGroupSync, "wait IOCTL failed, rc= %d", rc);
        return CamxResultEFailed;
    }

    ioctl_result = static_cast<INT32>(sync_ioctl.result);
    CAMX_LOG_VERBOSE(CamxLogGroupSync, "WAIT: ioctl_result = %d, sync_ioctl.result = %d", ioctl_result, sync_ioctl.result);
    switch (ioctl_result)
    {
        case 0:
            result = CamxResultSuccess;
            CAMX_LOG_VERBOSE(CamxLogGroupSync, "Sync wait IOCTL succeeded");
            break;
        case -ETIMEDOUT:
            result = CamxResultETimeout;
            CAMX_LOG_ERROR(CamxLogGroupSync, "Sync wait IOCTL timedout");
            break;
        default:
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupSync, "Sync wait IOCTL failed");
            break;
    }

    CAMX_TRACE_SYNC_END(CamxLogGroupCore);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Private function definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyncManager::SyncManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SyncManager::SyncManager() {}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyncManager::~SyncManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SyncManager::~SyncManager() {}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyncManager::GetInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SyncManager* SyncManager::GetInstance()
{
    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Sync GetInstance!");
    if (NULL == s_pSyncManagerInstance)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSync, "First time: Creating instance");
        s_pSyncManagerInstance = CAMX_NEW SyncManager();
        s_ctrl.countLock = CamX::Mutex::Create("SyncController");
        s_ctrl.refCount = 0;
        s_ctrl.syncFd = -1;
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSync, "NOT First time: returning earlier instance");
    }

    s_ctrl.countLock->Lock();
    s_ctrl.refCount++;
    s_ctrl.countLock->Unlock();
    return s_pSyncManagerInstance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyncManager::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT SyncManager::Destroy()
{
    // We call the private destructor from here when refCount drops to zero
    CAMX_LOG_VERBOSE(CamxLogGroupSync, "Sync Destroy!");
    s_ctrl.countLock->Lock();
    s_ctrl.refCount--;
    if (0 == s_ctrl.refCount)
    {
        CAMX_DELETE s_pSyncManagerInstance;
        s_pSyncManagerInstance = NULL;
        s_ctrl.countLock->Unlock();
        s_ctrl.countLock->Destroy();
        s_ctrl.countLock = NULL;

        return CamxResultSuccess;
    }
    s_ctrl.countLock->Unlock();
    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyncManager::GetReadFDFromPipe
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT SyncManager::GetReadFDFromPipe()
{
    return s_ctrl.pipeFDs[0];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyncManager::GetWriteFDFromPipe
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT SyncManager::GetWriteFDFromPipe()
{
    return s_ctrl.pipeFDs[1];
}

CAMX_NAMESPACE_END

#endif // ANDROID
