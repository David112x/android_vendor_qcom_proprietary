/******************************************************************************
  @file    HintExtHandler.h
  @brief   Declaration of hint extensions

  DESCRIPTION

  ---------------------------------------------------------------------------
  Copyright (c) 2020 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.
  ---------------------------------------------------------------------------
******************************************************************************/

#ifndef __HINT_EXT_HANDLER__H_
#define __HINT_EXT_HANDLER__H_

#include <mutex>
#include "mp-ctl.h"

#define MAX_HANDLERS 3

typedef bool (*HintExtAction)(mpctl_msg_t *);

typedef struct HintActionInfo {
    uint32_t mHintId;
    HintExtAction mPreAction;
    HintExtAction mPostAction;
} HintActionInfo;

class HintExtHandler {
    private:
        std::mutex mMutex;
        int mNumHandlers;
        HintActionInfo mExtHandlers[MAX_HANDLERS];

    private:
        HintExtHandler();
        HintExtHandler(HintExtHandler const&);
        HintExtHandler& operator=(HintExtHandler const&);

    public:
        ~HintExtHandler();
        static HintExtHandler &getInstance() {
            static HintExtHandler mHintExtHandler;
            return mHintExtHandler;
        }

        void Reset();
        bool Register(uint32_t hintId, HintExtAction preAction, HintExtAction postAction);
        bool FetchConfigPreAction(mpctl_msg_t *pMsg);
        bool FetchConfigPostAction(mpctl_msg_t *pMsg);
};

//pre/post actions of modules
class TaskBoostAction  {
    public:
        static bool TaskBoostPostAction(mpctl_msg_t *pMsg);
};

#define CURRENT_FPS_FILE "/data/vendor/perfd/current_fps"
class FpsUpdateAction {
    private:
        int mCurFps;
        int mHandle;
        std::mutex mMutex;
    private:
        FpsUpdateAction();
        FpsUpdateAction(FpsUpdateAction const&);
        FpsUpdateAction& operator=(FpsUpdateAction const&);
        ~FpsUpdateAction();

        void StandardizeFps(int32_t &fps);
        void CallHintReset();

    public:
        static FpsUpdateAction& getInstance() {
            static FpsUpdateAction mInstance;
            return mInstance;
        }
        static bool FpsUpdatePreAction(mpctl_msg_t *pMsg);
        static bool FpsUpdatePostAction(mpctl_msg_t *pMsg);
        void SetFps();
        int GetFps() {
            return mCurFps;
        }
        enum fps {
            FPS_MIN_LIMIT = 10,
            FPS30 = 30,
            FPS30_MAX_RANGE = 37,
            FPS45 = 45,
            FPS45_MAX_RANGE = 52,
            FPS60 = 60,
            FPS60_MAX_RANGE = 75,
            FPS90 = 90,
            FPS90_MAX_RANGE = 105,
            FPS120 = 120,
            FPS120_MAX_RANGE = 132,
            FPS144 = 144,
            FPS144_MAX_RANGE = 162,
            FPS180 = 180,
            FPS180_MAX_RANGE = 210,
            FPS240 = 240,
            FPS_MAX_LIMIT = 260
        };
};

#endif /*__HINT_EXT_HANDLER__H_*/
