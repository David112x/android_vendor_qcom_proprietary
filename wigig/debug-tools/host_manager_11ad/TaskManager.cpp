/*
* Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#include "DebugLogger.h"
#include "TaskManager.h"

#include <string>
#include <sstream>

using namespace std;

bool TaskManager::RegisterTask(std::function<void()> task, const std::string& name, const std::chrono::milliseconds interval)
{
    LOG_DEBUG << "Asked to register new task: " << name << endl;
    lock_guard<mutex> lock(m_taskExecutorsMutex);

    // map element may be constructed even if this key already presents in the map, test for element existence first
    // "try_emplace" was added in c++17
    if (m_taskExecutors.find(name) != m_taskExecutors.cend())
    {
        LOG_ERROR << "A periodic task with name " << name << " is already registered. " << PrintTaskExecutorsContainer() << endl;
        return false;
    }

    // no need to test insertion result since key existence was already tested
    m_taskExecutors.emplace(name, std::unique_ptr<TaskExecutor>(new TaskExecutor(std::move(task), name, interval)));
    LOG_DEBUG << "Registered new task: " << name << endl;
    LOG_VERBOSE << PrintTaskExecutorsContainer() << endl;
    return true;
}

void TaskManager::UnregisterTaskBlocking(const string& name)
{
    LOG_DEBUG << "Asked to unregister new task: " << name << endl;
    unique_lock<mutex> lock(m_taskExecutorsMutex);

    const auto it = m_taskExecutors.find(name);
    if (m_taskExecutors.end() == it)
    {
        LOG_DEBUG << "Task was already unregistered: " << name << ". "
                  << PrintTaskExecutorsContainer() << endl;
        return;
    }

    // make sure task is destructed outside of the lock to prevent potential dead lock
    // keep empty task entry in the map to prevent new registration before current entry destruction
    auto upTaskExecutor = std::move(it->second);
    lock.unlock();

    upTaskExecutor.reset(); // blocking operation -> calling TaskExecutor's d-tor
    lock.lock();
    m_taskExecutors.erase(name); // only erases the empty task entry
    lock.unlock();
    LOG_DEBUG << name << " task was unregistered" << endl;
    LOG_VERBOSE << PrintTaskExecutorsContainer() << endl;
}

void TaskManager::ChangeExecutionInterval(const string& name, std::chrono::milliseconds interval)
{
    LOG_DEBUG << "Asked to change interval for task: " << name << endl;
    lock_guard<mutex> lock(m_taskExecutorsMutex);

    auto it = m_taskExecutors.find(name);
    if (m_taskExecutors.end() == it)
    {
        LOG_ERROR << "Failed to change interval for task " << name
                  << " - name doesn't exist in Task Manager" << PrintTaskExecutorsContainer() << endl;
        return;
    }

    if (!it->second)
    {
        LOG_DEBUG << "Asked to change interval during removal of task: " << name << endl;
        return;
    }

    it->second->SetInterval(interval);
    LOG_DEBUG << "Changed interval for " << name << " to " << interval.count() << endl;
    LOG_VERBOSE << PrintTaskExecutorsContainer() << endl;
}

void TaskManager::StopBlocking()
{
    LOG_DEBUG << "Task Manager was asked to stop" << endl;
    lock_guard<mutex> lock(m_taskExecutorsMutex);
    LOG_DEBUG << "Terminate all working threads (blocking)" << endl;
    m_taskExecutors.clear();
    LOG_VERBOSE << PrintTaskExecutorsContainer() << endl;
}

string TaskManager::PrintTaskExecutorsContainer()
{
    stringstream ss;
    ss << "Map content: ";
    for (auto& te : m_taskExecutors)
    {
        ss << te.first << " ";
    }
    return ss.str();
}
