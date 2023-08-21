/*
* Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#pragma once

#include "TaskExecutor.h"

#include <map>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>

class TaskManager final
{
public:
    TaskManager() = default;
    bool RegisterTask(std::function<void()> task, const std::string& name, std::chrono::milliseconds interval);
    void UnregisterTaskBlocking(const std::string& name);
    void ChangeExecutionInterval(const std::string& name, std::chrono::milliseconds interval);
    void StopBlocking();

    // Non-copyable, non-movable
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;

private:
    std::string PrintTaskExecutorsContainer();
    std::map<std::string, std::unique_ptr<TaskExecutor>> m_taskExecutors;
    std::mutex m_taskExecutorsMutex;
};
