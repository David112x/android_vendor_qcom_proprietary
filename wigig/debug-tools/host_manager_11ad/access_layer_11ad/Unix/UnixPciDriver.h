/*
* Copyright (c) 2020 Qualcomm Technologies, Inc.
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

#ifndef _11AD_PCI_UNIX_PCI_DRIVER_H_
#define _11AD_PCI_UNIX_PCI_DRIVER_H_

#include "DriverAPI.h"
#include "OperationStatus.h"
#include "UnixDriverEventsTransport.h"
#include <string>
#include <vector>
#include <set>

class UnixPciDriver : public DriverAPI
{
public:
    explicit UnixPciDriver(const std::string& interfaceName);

    ~UnixPciDriver() override;

    // Base access functions (to be implemented by specific device)
    OperationStatus Read(DWORD address, DWORD& value) override;
    OperationStatus ReadBlock(DWORD address, DWORD blockSizeInDwords, std::vector<DWORD>& values) override;
    OperationStatus ReadBlock(DWORD addr, DWORD blockSizeInBytes, char *arrBlock) override;
    OperationStatus ReadRadarData(DWORD maxBlockSizeInDwords, std::vector<DWORD>& values) override;
    OperationStatus Write(DWORD address, DWORD value) override;
    OperationStatus WriteBlock(DWORD addr, std::vector<DWORD> values) override;
    OperationStatus WriteBlock(DWORD address, DWORD blockSizeInBytes, const char *valuesToWrite) override;

    OperationStatus DriverControl(uint32_t id, const void *inBuf, uint32_t inBufSize, void *outBuf, uint32_t outBufSize) override;

    OperationStatus AllocPmc(unsigned descSize, unsigned descNum) override;
    OperationStatus DeallocPmc(bool bSuppressError) override;
    OperationStatus CreatePmcFiles(unsigned refNumber) override;
    OperationStatus FindPmcDataFile(unsigned refNumber, std::string& foundPmcFilePath) override;
    OperationStatus FindPmcRingFile(unsigned refNumber, std::string& foundPmcFilePath) override;

    bool InitializeTransports() override;
    void RegisterDriverControlEvents(FwStateProvider* fwStateProvider) override;
    void StopDriverControlEvents() override;

    OperationStatus InterfaceReset() override;

private:
    struct IoctlIO
    {
        uint32_t op;
        uint32_t addr; /* should be 32-bit aligned */
        uint32_t val;
    };

    struct IoctlIOBlock
    {
        uint32_t op;
        uint32_t addr; /* should be 32-bit aligned */
        uint32_t size; /* represents the size in bytes. should be multiple of 4 */
        void* buf; /* block address */
    };

    struct IoctlRadarBlock
    {
        uint32_t max_size; /* maximal buffer size in bytes. should be multiple of 4 */
        void* buf;
    };

    OperationStatus SendRWIoctl(IoctlIO & io, int fd, const char* interfaceName);
    OperationStatus SendRWBIoctl(IoctlIOBlock & io, int fd, const char* interfaceName);
    bool SetDebugFsPath();
    const std::string& GetDebugFsPath();

    bool IsReady() const;

    int m_fileDescriptor;
    std::string m_debugFsPath;
    UnixDriverEventsTransport m_driverEventsHandler; // internal class for operations related to send/receive of Driver commands/events
};

#endif //_11AD_PCI_UNIX_PCI_DRIVER_H_
