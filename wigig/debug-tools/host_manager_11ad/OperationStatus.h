/*
* Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
/*
* Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <string>
#include <ostream>
#include <sstream>

#include "DebugLogger.h"

class OperationStatus
{
public:
    explicit OperationStatus(bool success = true, const char* szMsg = nullptr)
        : OperationStatus(success, szMsg ? std::string(szMsg) : std::string())
    {}

    OperationStatus(bool success, std::string msg)
        : m_success(success)
        , m_msg(std::move(msg))
    {}

    ~OperationStatus() = default;
    OperationStatus(OperationStatus&&) = default;
    OperationStatus& operator=(OperationStatus&&) = default;
    // prevent accidentally copy during chain of calls
    OperationStatus(const OperationStatus&) = delete;
    OperationStatus& operator=(const OperationStatus&) = delete;

    operator bool() const { return m_success; }

    bool IsSuccess() const { return m_success; }
    const std::string& GetStatusMessage() const { return m_msg; }

    void AddPrefix(const std::string& prefix, bool addSeparator = true)
    {
        std::ostringstream msgBuilder;
        msgBuilder << prefix;
        if (addSeparator)
        {
            msgBuilder << ". ";
        }
        msgBuilder << m_msg;
        m_msg = msgBuilder.str();
    }

    static OperationStatus Merge(const OperationStatus& lhs, const OperationStatus& rhs)
    {
        // use ";" as separation to distinguish from commas that are part of the message
        std::ostringstream msgBuilder;
        msgBuilder << lhs.GetStatusMessage() << "; " << rhs.GetStatusMessage();
        return { lhs && rhs, msgBuilder.str() };
    }

private:
    // Allow object reusing for subsequent operations in a flow
    bool m_success;
    std::string m_msg;
};

inline std::ostream& operator<<(std::ostream& os, const OperationStatus& st)
{
    return os << "Completed: " << SuccessStr(st.IsSuccess())
              << " Message: [" << st.GetStatusMessage() << ']';
}
