/******************************************************************************
#  Copyright (c) 2020 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/

#include <framework/Log.h>
#include <framework/ThreadPoolManager2.h>

#define TAG "RILQ"

WorkerThread::WorkerThread(std::string name) {
  std::thread([=] { run(name); }).detach();
}

WorkerThread::~WorkerThread() {
  std::unique_lock<std::mutex> lock(mMutex);
  mStop = true;
  lock.unlock();
  mCond.notify_all();
}

/**
 * WorkerThread routine.
 * Waits for the ThreadPoolManager2 to assign the Tasks and execute them.
 */
void WorkerThread::run(std::string name) {
  setThreadName(name.c_str());
  while (true) {
    QCRIL_LOG_DEBUG("waiting for condition");
    {
      std::unique_lock<std::mutex> lock(mMutex);
      mCond.wait(lock, [&] { return mStop || !mTaskQueue.empty(); });
    }
    if (mStop) break;

    QCRIL_LOG_DEBUG("WorkerThread::run: mTaskQueue.size = %d", mTaskQueue.size());

    while (true) {
      std::shared_ptr<ThreadPoolTask> task = nullptr;
      // Pick the first tast from the queue and execute that.
      {
        std::unique_lock<std::mutex> lock(mMutex);
        task = mTaskQueue.front();
      }

      if (task) {
        QCRIL_LOG_DEBUG("Running the task: %s", task->to_string().c_str());
        task->run();
      }

      // After completing the task, remove it from the TaskQueue
      {
        std::unique_lock<std::mutex> lock(mMutex);
        mTaskQueue.pop_front();
        mTaskQueue.shrink_to_fit();
        if (mTaskQueue.empty()) {
          break;
        }
      }
    }
  }
}

void WorkerThread::assign(std::shared_ptr<ThreadPoolTask> task) {
  QCRIL_LOG_DEBUG("WorkerThread::assign: %s", task->to_string().c_str());
  std::unique_lock<std::mutex> lock(mMutex);
  mTaskQueue.push_back(task);
  lock.unlock();
  mCond.notify_all();
  QCRIL_LOG_DEBUG("assign complete");
}

WorkerThread* ThreadPoolManager2::TaskScheduler::identifyWorker(
    std::shared_ptr<ThreadPoolTask> task) {
  if (mManager == nullptr) {
    return nullptr;
  }
  WorkerThread* workerThread = nullptr;
  const Module* m = task->getModule();
  if (m != nullptr) {
    // Default thread / system thread is the first thread from the ThreadPool.
    WorkerThread* systemThread = mManager->mThreadPool.front();

    // Check the message handling policy for this message from the Module
    // and assign an appropriate thread.
    Module::MessageHandlingPolicy policy = m->getMessageHandlingPolicy(task->getMessage());
    if (policy == Module::MessageHandlingPolicy::SYSTEM_SHARED_THREAD) {
      QCRIL_LOG_DEBUG("Module: %s, Msg: %s : SYSTEM_SHARED_THREAD", m->to_string().c_str(),
          task->getMessage()->to_string().c_str());
      // Use system thread
      workerThread = systemThread;
    } else if (policy == Module::MessageHandlingPolicy::MODULE_SHARED_THREAD) {
      QCRIL_LOG_DEBUG("Module: %s, Msg: %s : MODULE_SHARED_THREAD", m->to_string().c_str(),
          task->getMessage()->to_string().c_str());
      if (mThreadMap.find(m) == mThreadMap.end()) {
        QCRIL_LOG_DEBUG("Next available thread index = %d", mNextAvailableThreadIndex);
        // Find next available thread
        mThreadMap[m] = mManager->mThreadPool[mNextAvailableThreadIndex];
        mNextAvailableThreadIndex++;
        if (mNextAvailableThreadIndex >= mManager->mThreadPool.size()) {
          mNextAvailableThreadIndex = 0;
        }
      }
      workerThread = mThreadMap[m];
    } else {
      QCRIL_LOG_DEBUG("Module: %s, Msg: %s : NONE", m->to_string().c_str(),
          task->getMessage()->to_string().c_str());
      // ie., Module::MessageHandlingPolicy::NONE
      // The Task can be executed by any of the available thread.
      workerThread = systemThread;
      size_t taskQueueSize = systemThread->getTaskQueueSize();
      // Find a thread with less load and use that.
      for (WorkerThread* thread : mManager->mThreadPool) {
        size_t size = thread->getTaskQueueSize();
        if (size < taskQueueSize) {
          workerThread = thread;
        }
      }
    }
  }
  return workerThread;
}

ThreadPoolManager2::ThreadPoolManager2(uint32_t maxPoolSize) {
  QCRIL_LOG_DEBUG("MaxPoolSize = %d", maxPoolSize);
  mTaskScheduler = new TaskScheduler(this);
  // Create the WorkerThreads
  for (size_t i = 0; i < maxPoolSize; ++i) {
    WorkerThread* wt = new WorkerThread("WorkerThread:" + std::to_string(i));
    mThreadPool.push_back(wt);
  }
  // Start the main thread for ThreadPoolManager2
  std::thread([=] { run(); }).detach();
}

ThreadPoolManager2::~ThreadPoolManager2() {
  delete mTaskScheduler;
  std::unique_lock<std::mutex> lock(mMutex);
  mStop = true;
  lock.unlock();
  mCond.notify_all();

  for (auto it = mThreadPool.begin(); it != mThreadPool.end(); ) {
    auto thread = *it;
    it = mThreadPool.erase(it);
    delete thread;
  }
}

/**
 * Schedule a task to the thread pool.
 */
void ThreadPoolManager2::schedule(std::shared_ptr<ThreadPoolTask> task) {
  QCRIL_LOG_DEBUG("scheduling task: %s", task->to_string().c_str());
  std::unique_lock<std::mutex> lock(mMutex);
  mTaskQueue.push_back(task);
  lock.unlock();
  mCond.notify_all();
}

/**
 * ThreadPoolManager2 main thread.
 * Receives the Tasks from the clients and assign it to an appropriate WorkerThread.
 */
void ThreadPoolManager2::run() {
  setThreadName("ThreadPoolMgr2");
  while (true) {
    QCRIL_LOG_DEBUG("waiting on condition variable");
    std::unique_lock<std::mutex> lock(mMutex);
    mCond.wait(lock, [&] { return mStop || !mTaskQueue.empty(); });

    if (mStop) break;

    QCRIL_LOG_DEBUG("mTaskQueue.size = %d", mTaskQueue.size());

    while (!mTaskQueue.empty()) {
      auto task = mTaskQueue.front();
      mTaskQueue.pop_front();
      mTaskQueue.shrink_to_fit();
      // Get a WorkerThread for this task.
      WorkerThread* thread = mTaskScheduler->identifyWorker(task);
      if (thread) {
        thread->assign(task);
      }
    }
  }
}

/**
 * Creates the ThreadPoolManager2 and return the reference to it.
 */
ThreadPoolManager2& ThreadPoolManager2::getThreadPoolManager() {
  static ThreadPoolManager2 sThreadPool{};
  return sThreadPool;
}
