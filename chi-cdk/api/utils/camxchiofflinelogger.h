////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxofflinelogger.h
/// @brief Debug Print related defines
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXOFFLINELOGGER_H
#define CAMXOFFLINELOGGER_H

#if defined (_WIN32)
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OFFLINELOG_ERROR(...)   printf(__VA_ARGS__)
#if defined (OFFLINELOG_DEBUG)
#define OFFLINELOG_INFO(...)    printf(__VA_ARGS__)
#else
#define OFFLINELOG_INFO(...)
#endif

#elif defined (_LINUX)
#include <log/log.h>
#include <stdio.h>

#define OFFLINELOG_ERROR(fmt, args...)   ALOGE("%s():%d " fmt "\n", __func__, __LINE__, ##args)
#if defined (OFFLINELOG_DEBUG)
#define OFFLINELOG_INFO(fmt, args...)   ALOGI("%s():%d " fmt "\n", __func__, __LINE__, ##args)
#else
#define OFFLINELOG_INFO(fmt, args...)
#endif
#endif // (_WIN32) (_LINUX)


#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <sstream>

#include "chiofflineloggerinterface.h"

#include "camxdefs.h"
#include "camxtypes.h"

CAMX_NAMESPACE_BEGIN

using namespace std;

/// @brief Per thread queue data structure
///        being passed into offlinelogger container
struct PerLogThreadInfo
{
   BOOL           bNeedFlush;          ///< When queue reaches flush threshold, set this flag to true
   BOOL           bHasFileName;        ///< If this thread had already created file successfully
   UINT           threadID;            ///< Thread ID this info structure associated with
   INT            registerID;          ///< Queue ID registered within memory pool
   UINT           sessionCount;        ///< Session count corresponding to this buffer
   INT            segment;             ///< When filesize reaches threshold, plus this count
   UINT           numByteToWrite;      ///< The buffer size need to be written to file
   string         offlinelogFileName;  ///< Thread's unique filename for filesystem operation
};

/// @brief Per thread information data structure
///        being passed for tracking thread lifetime in offlinelogger
struct LifetimeThreadInfo
{
    UINT               threadID;          ///< Thread ID this info structure associated with
    INT                registerID;        ///< Memory container ID registered within offlinelogger
    BOOL               enableLoggerType;  ///< Offlinelogger enable flag
    OfflineLoggerType  loggerType;        ///< Offlinelogger type(eg, Binary/ASCII)

    ~LifetimeThreadInfo();                ///< Destructor will be called while thread being killed
};

class OfflineLogger
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief The maximum length for a log file name
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static const UINT MaxTimeStampLength = 50;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief The maximum log path length for a log
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static const UINT MaxPathLength = 128;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief The refresh rate for the offline logger
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static const UINT ReFlushRateToFile = 200;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief The maximum support different types of loggers
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static const UINT MaxSupportLoggerTypeCount = static_cast<UINT>(OfflineLoggerType::NUM_OF_TYPE);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInstance
    ///
    /// @brief  Return the reference singleton instance of OfflineLogger based on different Offlinelogger type
    ///
    /// @return OfflineLogger reference instance
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE OfflineLogger* GetInstance(OfflineLoggerType type)
    {
        // First time call getInstance, we need to prepare the different type of offlinelogger
        if (FALSE == s_isInit)
        {
            std::lock_guard<std::mutex> lockInitialization(s_initMutex);
            if (FALSE == s_isInit)
            {
                s_isInitValid = TRUE;

                // For ASCII logger
                s_pOfflineLoggerInstancePool.push_back(new OfflineLogger(OfflineLoggerType::ASCII));
                // For Binary logger
                s_pOfflineLoggerInstancePool.push_back(new OfflineLogger(OfflineLoggerType::BINARY));
                s_isInit = TRUE;
            }
        }
        return s_pOfflineLoggerInstancePool[static_cast<UINT8>(type)];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddLog
    ///
    /// @brief  User through the API to push the log to offlinelogger and store it into a log container
    ///
    /// @param  plog  text to andriod file system
    /// @param  tid   current thread id
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_VISIBILITY_PUBLIC CamxResult AddLog(const CHAR* pLog, UINT logSize);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FlushLogToFile
    ///
    /// @brief  Pop the text log from offlinelogger`s text log container and write it to file system
    ///
    /// @param  forceFlush  force flush all text from log container to file
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FlushLogToFile(BOOL forceFlush = FALSE);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequestNewLogQueue
    ///
    /// @brief  local thread to require a new log container
    ///
    /// @param  newQueue  pass local thread container pointer and assinged a new one by offlinelogger
    /// @param  tid       current local thread id
    /// @param  segment   local thread own its file segment count and notice to offlinelogger
    ///
    /// @return Queue ID registered within memory pool
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT        RequestNewLogQueue(BYTE* &newQueue, UINT tid, INT segment);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequestFlushLogQueue
    ///
    /// @brief  local thread to request need to flush its log container
    ///
    /// @param  registerId   local thread ask offlinelogger for flush its container by passing the registerId
    ///
    /// @return VOID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID       RequestFlushLogQueue(INT registerId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyCameraOpen
    ///
    /// @brief  Notify offlinelogger and trigger camera open event
    ///
    /// @return VOID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_VISIBILITY_PUBLIC VOID       NotifyCameraOpen();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyCameraClose
    ///
    /// @brief  Notify offlinelogger and trigger camera close event
    ///
    /// @return VOID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_VISIBILITY_PUBLIC VOID       NotifyCameraClose();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DisableOfflinelogger
    ///
    /// @brief  Disable the offlinelogger
    ///
    /// @return VOID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID       DisableOfflinelogger();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsEnableOfflinelogger
    ///
    /// @brief  Get the Enable/Disable status of offlinelogger
    ///
    /// @return enable/disable status of offlinelogger
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_VISIBILITY_PUBLIC BOOL IsEnableOfflinelogger();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetFlushTriggerLink
    ///
    /// @brief  Set the flush trigger thread signal function pointer
    ///
    /// @return result of this operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_VISIBILITY_PUBLIC BOOL SetFlushTriggerLink(PFNSIGNALOFFLINETHREAD pConditionVar);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLogInfoMask
    ///
    /// @brief  This API will allow each copy of DebugPrint to fetch correct LogMask
    ///
    /// @return result of this operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_VISIBILITY_PUBLIC DebugLogInfo GetLogInfoMask();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetLogInfoMask
    ///
    /// @brief  This API will store CamX setting logmask to OfflineLogger
    ///
    /// @return result of this operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_VISIBILITY_PUBLIC VOID SetLogInfoMask(DebugLogInfo *pNewLogInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSettingLink
    ///
    /// @brief  This API will get override setting from CHI API
    ///
    /// @return result of this operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_VISIBILITY_PUBLIC BOOL GetSettingLink(PFNGETSETTINGS pGetSetting);

private:

    /// Constructor
    CAMX_VISIBILITY_PUBLIC OfflineLogger(OfflineLoggerType type);
    /// Destructor
    ~OfflineLogger();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFilename
    ///
    /// @brief  Get current file name by current system timestamp
    ///
    /// @return full file name and path
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    string     GetPrefixFilename();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateLogFile
    ///
    /// @brief  Create a new file for log
    ///
    /// @param  fileName   create a new file by offering a complete filename and path
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateLogFile(const char* fileName);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAllToDefaultValue
    ///
    /// @brief  Clear and set member to default value
    ///
    /// @return VOID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID       SetAllToDefaultValue();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AllocateLogQueue
    ///
    /// @brief  Allocate centralize queue memory pool
    ///
    /// @return TRUE if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL AllocateLogQueueMemPool();

    /// Do not support the copy constructor or assignment operator
    OfflineLogger(const OfflineLogger& rOfflineLogger)             = delete;
    OfflineLogger& operator= (const OfflineLogger& rOfflineLogger) = delete;

    static thread_local BYTE* m_threadLocalLogBuffer[MaxSupportLoggerTypeCount];   ///< Log buffer container
    static thread_local INT   m_threadLocalQueuePoolId[MaxSupportLoggerTypeCount]; ///< Buffer container register id
    static thread_local UINT  m_threadLocalSessionCnt[MaxSupportLoggerTypeCount];  ///< Camera reopen count
    static thread_local UINT  m_threadLocalSegmentCnt[MaxSupportLoggerTypeCount];  ///< Log segment for current thread
    static thread_local UINT  m_threadLocalLogSize[MaxSupportLoggerTypeCount];     ///< Log size for current thread
    static thread_local UINT  m_threadLocalLogCurPos[MaxSupportLoggerTypeCount];   ///< Current buffer cursor location
    static thread_local BOOL  m_threadLocalFlushDone[MaxSupportLoggerTypeCount];   ///< To store if force flush happened
    BOOL                      m_enableLoggerType[MaxSupportLoggerTypeCount];       ///< Enable status for each logger type
    static thread_local LifetimeThreadInfo m_threadLocalLifeTime[MaxSupportLoggerTypeCount]; /// Tracking for thread lifetime

    vector<BYTE*>             m_memoryLogPool;                    ///< log container memory pool
    vector<PerLogThreadInfo>  m_perLogInfoPool;                   ///< Each thread register pool
    queue<INT>                m_availableQueueIdPool;             ///< Available memory id pool
    queue<INT>                m_availableRegisterIdPool;          ///< Available registered id for log container
    queue<INT>                m_registerIdFlushOrderQueue;        ///< Queue to track flush order, so timestamp will be sorted
    string                    m_offlinelogPreFileName;            ///< Pre-filename for offlinelog
    string                    m_offlinelogPreLogTypeName;         ///< Pre-logType for offlinelog, eg:ASCII/Binary
    BOOL                      m_offlinelogHasPreFileName;         ///< The flag for pre-filename created
    OfflineLoggerStatus       m_offlinelogStatus;                 ///< Offlinelogger status
    OfflineLoggerType         m_offlinelogType;                   ///< Offlinelogger type(eg, Binary/ASCII)
    UINT                      m_sessionCount;                     ///< Camera reopen count
    BOOL                      m_forceFlushDone;                   ///< Force flush done flag
    UINT                      m_needFlushThreadCount;             ///< Count of threads needing flush
    struct DebugLogInfo       m_debugLogInfo;                     ///< Store CamX setting logmask
    static mutex              m_pOfflineLoggerLock;               ///< Mutex to protect queue internal thread state

    /* Below static variables are specifically exported as they are accessed by client which calls inline
       function GetInstance() that uses these static member variables requiring their symbol*/
    CAMX_VISIBILITY_PUBLIC static BOOL    s_isInit;               ///< whether the Offlinelogger singleton has been initialize
    CAMX_VISIBILITY_PUBLIC static BOOL    s_isInitValid;          ///< whether the Offlinelogger singleton is in a valid state
    CAMX_VISIBILITY_PUBLIC static mutex   s_initMutex;            ///< Mutex to protect initialization of logger instances
    CAMX_VISIBILITY_PUBLIC static vector<OfflineLogger*>
                              s_pOfflineLoggerInstancePool;       ///< Offlinelogger singleton instance pool

    PFNSIGNALOFFLINETHREAD    m_pSignalFlushTriggerThread;        ///< Function pointer to signal flush trigger thread
    PFNGETSETTINGS            m_pOverrideSetting;                 ///< CHI get override setting function pointer
};

CAMX_NAMESPACE_END
#endif
