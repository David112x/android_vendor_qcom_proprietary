/*
* Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#include "TaskExecutor.h"
#include "DebugLogger.h"
#include "Utils.h"

#include <string>

using namespace std;
using namespace chrono;

TaskExecutor::TaskExecutor(std::function<void()> task, std::string name, std::chrono::milliseconds interval)
    : m_task(std::move(task))
    , m_name(std::move(name))
    , m_interval(interval)
    , m_continue(true)
{
    m_thread = thread(&TaskExecutor::ExecutionLoop, this);
}

TaskExecutor::~TaskExecutor()
{
    LOG_DEBUG << m_name << ": Stop request" << endl;
    m_continue.store(false);
    if (m_thread.joinable())
    {
        m_thread.join();
        LOG_DEBUG << m_name << ": Stopped" << endl;
    }
    else
    {
        LOG_DEBUG << m_name << ": Was already stopped" << endl;
    }
}

void TaskExecutor::ExecutionLoop() const
{
    LOG_DEBUG << m_name << ": Starting thread loop" << endl;
    while (m_continue.load())
    {
        auto lastTriggered = system_clock::now();

        LOG_VERBOSE << m_name << ": Last triggered " << Utils::GetTimeString(lastTriggered) << endl;
        m_task(); // Run the function of the task

        auto sleepInterval = time_point_cast<milliseconds>(lastTriggered) + m_interval.load() - time_point_cast<milliseconds>(system_clock::now());
        LOG_VERBOSE << m_name << ": Sleep for " << sleepInterval.count() << "ms. Next Run: "
                    << Utils::GetTimeString(lastTriggered + m_interval.load()) << endl;

        if (sleepInterval.count() <= 0)
        {
            // sleep_until/ sleep_for negative value bug was officially fixed in gcc 4.9.3
            LOG_DEBUG << m_name << ": no need in sleep - interval is " << sleepInterval.count() << endl;
            continue;
        }

        this_thread::sleep_for(sleepInterval);
    }
    LOG_DEBUG << m_name << ": Exit thread loop" << endl;
}