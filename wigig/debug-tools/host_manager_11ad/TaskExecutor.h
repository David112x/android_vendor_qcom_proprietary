/*
* Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#pragma once

#include <chrono>
#include <thread>
#include <atomic>
#include <string>
#include <functional>

class TaskExecutor final
{
public:

    TaskExecutor(std::function<void()> task, std::string name, std::chrono::milliseconds interval);
    ~TaskExecutor();

     //non-copyable
    TaskExecutor(const TaskExecutor& task) = delete;
    TaskExecutor& operator=(const TaskExecutor& task) = delete;

    const std::string& Name() const { return m_name; }
    void SetInterval(std::chrono::milliseconds interval) { m_interval.store(interval); }

private:
    void ExecutionLoop() const;

    const std::function<void()> m_task;
    const std::string m_name;
    std::atomic<std::chrono::milliseconds> m_interval;
    std::atomic<bool> m_continue;
    std::thread m_thread;
};
