/******************************************************************************
#  Copyright (c) 2020 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

#include "framework/ThreadPoolAgent.h"
#include "framework/Message.h"
#include "framework/Module.h"
#include "framework/Util.h"

/**
 * ThreadPoolManager2 is a thread pool implementation for the Loopers.
 * ThreadPool will have a list of threads, and the modules can ask to execute the handlers using
 * any of the threads. The modules can specify certain policy for using the threads for handling
 * the Messages.
 * Below are the different MessageHandlingPolicies
 *  - SYSTEM_SHARED_THREAD: Use a default/system shared thread for handling the message
 *  - MODULE_SHARED_THREAD: Use a dedicated thread for this module.
 *  - NONE: The Message can be handled using any of the available threads.
 *
 *  Modules need to use ThreadPoolAgent object as the looper to make use of the thread pool.,
 *  and it can extend the virtual function getMessageHandlingPolicy() to specify the policy
 *  to be applied when handling a specific message.  The default message handling policy is
 *  SYSTEM_SHARED_THREAD.
 */


/**
 * Task - An object that embodies the action to be taken
 */
class ThreadPoolTask {
 private:
  Module* mModule;
  std::shared_ptr<Message> mMessage;
  std::function<void(std::shared_ptr<Message>)> mHandler;

 public:
  ThreadPoolTask(Module* module, std::shared_ptr<Message> message,
                 std::function<void(std::shared_ptr<Message>)> h)
      : mModule(module), mMessage(message), mHandler(h) {
  }

  const Module* getModule() const {
    return mModule;
  }

  std::shared_ptr<Message> getMessage() const {
    return mMessage;
  }

  void run() {
    if (mHandler) {
      mHandler(mMessage);
    }
  }
  std::string to_string() const {
    std::string ret =
        "Task{Module:" + mModule->to_string() + ",Message:" + mMessage->to_string() + "}";
    return ret;
  }
};

/**
 * WorkerThread - thread in action.
 * WorkerThread will wait for the ThreadPoolManager2 to assign the Tasks and executes it.
 */
class WorkerThread {
 private:
  std::mutex mMutex;
  std::condition_variable mCond;
  std::atomic<bool> mStop{ false };
  std::deque<std::shared_ptr<ThreadPoolTask>> mTaskQueue;

  void run(std::string name);

 public:
  WorkerThread(std::string name);
  ~WorkerThread();
  void assign(std::shared_ptr<ThreadPoolTask> task);
  size_t getTaskQueueSize() {
    std::unique_lock<std::mutex> lock(mMutex);
    return mTaskQueue.size();
  }
};

/**
 * ThreadPoolManager2 - a singleton class which maintains the list of WorkerThreads.
 * This is the entry point to the thread pool.
 * The clients (ie., ThreadPoolAgents) can create new tasks and call the function
 * ThreadPoolManager2::schedule to schedule it in the thread pool.
 */
class ThreadPoolManager2 {
  /**
   * TaskScheduler applies policy while selecting the WorkerThread to assign for the next task.
   * This will select the WorkerThread based on parameters from the task, and give to the
   * ThreadPoolManager2.
   */
  class TaskScheduler {
   private:
    ThreadPoolManager2* mManager;
    std::unordered_map<const Module *, WorkerThread *> mThreadMap;
    uint32_t mNextAvailableThreadIndex = 0;

   public:
    TaskScheduler(ThreadPoolManager2* m) : mManager(m) {
    }
    ~TaskScheduler() {
    }
    WorkerThread* identifyWorker(std::shared_ptr<ThreadPoolTask> task);
  };

 private:
  TaskScheduler *mTaskScheduler;
  std::mutex mMutex;
  std::condition_variable mCond;
  std::deque<std::shared_ptr<ThreadPoolTask>> mTaskQueue;
  std::atomic<bool> mStop{ false };
  std::vector<WorkerThread*> mThreadPool;

  void run();

  explicit ThreadPoolManager2(uint32_t maxPoolSize = 1);

 public:
  ~ThreadPoolManager2();

  void schedule(std::shared_ptr<ThreadPoolTask> task);

  static ThreadPoolManager2& getThreadPoolManager();
};
