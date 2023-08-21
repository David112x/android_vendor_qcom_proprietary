/******************************************************************************
  @file    HintExtHandler.cpp
  @brief   Implementation of hint extensions

  DESCRIPTION

  ---------------------------------------------------------------------------
  Copyright (c) 2020 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.
  ---------------------------------------------------------------------------
******************************************************************************/

#include <cutils/log.h>
#include <thread>
#include "HintExtHandler.h"
#include "client.h"
#include "OptsData.h"

#define LOG_TAG    "ANDR-PERF-HINTEXT"
using namespace std;

HintExtHandler::HintExtHandler() {
    Reset();
}

HintExtHandler::~HintExtHandler() {
    Reset();
}

bool HintExtHandler::Register(uint32_t hintId, HintExtAction preAction, HintExtAction postAction) {
    lock_guard<mutex> lock(mMutex);
    if (mNumHandlers >= MAX_HANDLERS) {
        //no more registrations
        return false;
    }

    mExtHandlers[mNumHandlers].mHintId = hintId;
    mExtHandlers[mNumHandlers].mPreAction = preAction;
    mExtHandlers[mNumHandlers].mPostAction = postAction;
    mNumHandlers++;
    return true;
}

bool HintExtHandler::FetchConfigPreAction(mpctl_msg_t *pMsg) {
    int i = 0;
    if (NULL == pMsg) {
        return false;
    }

    lock_guard<mutex> lock(mMutex);
    for (i=0; i < mNumHandlers; i++) {
        if ((pMsg->hint_id == mExtHandlers[i].mHintId) && mExtHandlers[i].mPreAction) {
            mExtHandlers[i].mPreAction(pMsg);
            break;
        }
    }
    return true;
}

bool HintExtHandler::FetchConfigPostAction(mpctl_msg_t *pMsg) {
    int i = 0;
    if (NULL == pMsg) {
        return false;
    }

    lock_guard<mutex> lock(mMutex);
    for (i=0; i < mNumHandlers; i++) {
        if ((pMsg->hint_id == mExtHandlers[i].mHintId) && mExtHandlers[i].mPostAction) {
            mExtHandlers[i].mPostAction(pMsg);
            break;
        }
    }
    return true;
}

void HintExtHandler::Reset() {
    lock_guard<mutex> lock(mMutex);
    int i = 0;
    mNumHandlers = 0;

    for (i=0; i < MAX_HANDLERS; i++) {
        mExtHandlers[i].mHintId = 0;
        mExtHandlers[i].mPreAction = NULL;
        mExtHandlers[i].mPostAction = NULL;
    }
    return;
}

//hint extension actions by modules
/**taskboost's fecth config post action
 * since perfhint attach_application used the param slot which is designed to pass timeout to pass process id
 * so for this perfhint, we also use the timeout defined in xml
 */
bool TaskBoostAction::TaskBoostPostAction(mpctl_msg_t *pMsg) {
    if (NULL == pMsg) {
        return false;
    }

    int size = pMsg->data;

    if (size > MAX_ARGS_PER_REQUEST)
        size = MAX_ARGS_PER_REQUEST;

    if (pMsg->hint_id == VENDOR_HINT_BOOST_RENDERTHREAD && pMsg->pl_time > 0) {
        renderThreadTidOfTopApp = pMsg->pl_time;
        for (int i = 0; i < size-1; i = i + 2) {
            if(pMsg->pl_args[i] == MPCTLV3_SCHED_ENABLE_TASK_BOOST_RENDERTHREAD)
            {
                QLOGI("renderThreadTidOfTopApp:%d, currentFPS:%d",
                       renderThreadTidOfTopApp, FpsUpdateAction::getInstance().GetFps());
                if(FpsUpdateAction::getInstance().GetFps() < FpsUpdateAction::getInstance().FPS144)
                {
                    pMsg->pl_args[i] = MPCTLV3_SCHED_DISABLE_TASK_BOOST_RENDERTHREAD;
                }
                pMsg->pl_args[i+1] = pMsg->pl_time;
                break;
            }
        }
        pMsg->pl_time = -1;
    }

    if (pMsg->hint_type == LAUNCH_TYPE_ATTACH_APPLICATION) {
        for (int i = 0; i < size-1; i = i + 2) {
            if (pMsg->pl_args[i] == MPCTLV3_SCHED_TASK_BOOST) {
                pMsg->pl_args[i+1] = pMsg->pl_time;
                break;
            }
        }
        pMsg->pl_time = -1;
    }
    return true;
}

FpsUpdateAction::FpsUpdateAction() {
    mCurFps = 0;
    mHandle = -1;
}

FpsUpdateAction::~FpsUpdateAction() {
    mCurFps = 0;
    mHandle = -1;
}

//If fps value sent by AOSP is not standard, then standardize it as below
void FpsUpdateAction::StandardizeFps(int32_t &fps) {
    if (fps <= FPS30_MAX_RANGE) {
        fps = FPS30;
    }
    else if (fps <= FPS45_MAX_RANGE) {
        fps = FPS45;
    }
    else if (fps <= FPS60_MAX_RANGE) {
        fps = FPS60;
    }
    else if (fps <= FPS90_MAX_RANGE) {
        fps = FPS90;
    }
    else if (fps <= FPS120_MAX_RANGE) {
        fps = FPS120;
    }
    else if (fps <= FPS144_MAX_RANGE) {
        fps = FPS144;
    }
    else if (fps <= FPS180_MAX_RANGE) {
        fps = FPS180;
    }
    else if (fps <= FPS_MAX_LIMIT) {
        fps = FPS240;
    }
}

void FpsUpdateAction::CallHintReset() {
    FpsUpdateAction &pFpsUpdateObj = FpsUpdateAction::getInstance();
    int handle = -1;
    {
        lock_guard<mutex> lock(pFpsUpdateObj.mMutex);
        handle = pFpsUpdateObj.mHandle;
        pFpsUpdateObj.mHandle = -1;
    }
    perf_lock_rel(handle);
}

bool FpsUpdateAction::FpsUpdatePreAction(mpctl_msg_t *pMsg) {
    FILE *fpsVal;
    char buf[NODE_MAX];
    int locFps = 0;

    if (NULL == pMsg) {
        return false;
    }

    FpsUpdateAction &pFpsUpdateObj = FpsUpdateAction::getInstance();

    {
        lock_guard<mutex> lock(pFpsUpdateObj.mMutex);
        if (pFpsUpdateObj.mHandle > 0) {
            thread t1(&FpsUpdateAction::CallHintReset, &pFpsUpdateObj);
            t1.detach();
        }
    }

    if (pMsg->hint_type < FPS_MIN_LIMIT || pMsg->hint_type > FPS_MAX_LIMIT) {
        QLOGE("FPS Update for values < %d & > %d  are unsupported",
                FPS_MIN_LIMIT, FPS_MAX_LIMIT);
        return false;
    }
    QLOGI("ORIGINAL hintid: 0x%x, hint_type: %d", pMsg->hint_id, pMsg->hint_type);
    pFpsUpdateObj.StandardizeFps(pMsg->hint_type);
    QLOGI("STANDARDIZED hintid: 0x%x, hint_type: %d", pMsg->hint_id, pMsg->hint_type);

    {
        lock_guard<mutex> lock(pFpsUpdateObj.mMutex);
        pFpsUpdateObj.mCurFps = pMsg->hint_type;
        locFps = pFpsUpdateObj.mCurFps;
    }
    fpsVal = fopen(CURRENT_FPS_FILE, "w");
    if (fpsVal == NULL) {
        QLOGE("Cannot open/create fps value file");
        return false;
    }
    snprintf(buf, NODE_MAX, "%d", locFps);

    fwrite(buf, sizeof(char), strlen(buf), fpsVal);
    fclose(fpsVal);
    return true;
}

bool FpsUpdateAction::FpsUpdatePostAction(mpctl_msg_t *pMsg) {
    if (NULL == pMsg) {
        return false;
    }
    int size = pMsg->data;
    if (size > MAX_ARGS_PER_REQUEST)
        size = MAX_ARGS_PER_REQUEST;

    if(pMsg->hint_type >= FPS144) {
        for (int i = 0; i < size-1; i = i + 2) {
            if (pMsg->pl_args[i] == MPCTLV3_SCHED_ENABLE_TASK_BOOST_RENDERTHREAD) {
                QLOGI("FPS from FW:%d, apply task boost on top-app renderThreadTid:%d",
                       pMsg->hint_type, renderThreadTidOfTopApp);
                pMsg->pl_args[i+1] = renderThreadTidOfTopApp;
            }
        }
    }
    else if(pMsg->hint_type < FPS144) {
        for (int i = 0; i < size-1; i = i + 2) {
            if (pMsg->pl_args[i] == MPCTLV3_SCHED_DISABLE_TASK_BOOST_RENDERTHREAD) {
                QLOGI("FPS from FW:%d, disable task boost on top-app renderThreadTid:%d",
                       pMsg->hint_type, renderThreadTidOfTopApp);
                pMsg->pl_args[i+1] = renderThreadTidOfTopApp;
            }
        }
    }
    return true;
}

void FpsUpdateAction::SetFps() {
    FILE *fpsVal;
    int fps = 0, handle = -1;
    fpsVal = fopen(CURRENT_FPS_FILE, "r");
    if (fpsVal != NULL) {
        fscanf(fpsVal,"%d", &fps);
        fclose(fpsVal);
    }

    handle = perf_hint(VENDOR_HINT_FPS_UPDATE, nullptr, 0, fps);
    {
        lock_guard<mutex> lock(mMutex);
        mCurFps = fps;
        mHandle = handle;
    }
}
