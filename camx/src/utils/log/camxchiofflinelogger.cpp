////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchiofflinelogger.cpp
/// @brief Write Debug Log to Files
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE CP006: used standard libraries for performance improvements
// NOWHINE FILE CP040: Circular dependency problem with camxosutils. Because osutils still need to print log.

#include <ctime>
#include <cutils/properties.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>

#include "camxchiofflinelogger.h"
#include "camxsettingsmanager.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

static CamX::OfflineLogger* s_pOfflineLogger[OFFLINELOG_MAX_TYPE] = { NULL };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineLoggerGetInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CAMX_INLINE CamX::OfflineLogger* OfflineLoggerGetInstance(
    OfflineLoggerType type)
{
    if (NULL == s_pOfflineLogger[static_cast<UINT>(type)])
    {
        s_pOfflineLogger[static_cast<UINT>(type)] = CamX::OfflineLogger::GetInstance(type);
    }
    return s_pOfflineLogger[static_cast<UINT>(type)];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflinelogAddLogInterface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID OfflinelogAddLogInterface(
    OfflineLoggerType type,
    const CHAR* pText)
{
    CamX::OfflineLogger* pOfflineLogger = OfflineLoggerGetInstance(type);
    pOfflineLogger->AddLog(pText, static_cast<UINT>(strlen(pText)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflinelogFlushLogInterface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID OfflinelogFlushLogInterface(
    OfflineLoggerType type,
    BOOL forceFlush)
{
    CamX::OfflineLogger* pOfflineLogger = OfflineLoggerGetInstance(type);
    pOfflineLogger->FlushLogToFile(forceFlush);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflinelogNoticeCameraOpenInterface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID OfflinelogNoticeCameraOpenInterface(
    OfflineLoggerType type)
{
    CamX::OfflineLogger* pOfflineLogger = OfflineLoggerGetInstance(type);
    pOfflineLogger->NotifyCameraOpen();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflinelogNoticeCameraCloseInterface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID OfflinelogNoticeCameraCloseInterface(
    OfflineLoggerType type)
{
    CamX::OfflineLogger* pOfflineLogger = OfflineLoggerGetInstance(type);
    pOfflineLogger->NotifyCameraClose();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflinelogDisableInterface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID OfflinelogDisableInterface(
    OfflineLoggerType type)
{
    CamX::OfflineLogger* pOfflineLogger = OfflineLoggerGetInstance(type);
    pOfflineLogger->DisableOfflinelogger();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflinelogIsEnableInterface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL OfflinelogIsEnableInterface(
    OfflineLoggerType type)
{
    CamX::OfflineLogger* pOfflineLogger = OfflineLoggerGetInstance(type);
    return pOfflineLogger->IsEnableOfflinelogger();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflinelogLinkFlushTriggerInterface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL OfflinelogLinkFlushTriggerInterface(
    OfflineLoggerType type,
    PFNSIGNALOFFLINETHREAD pSignalFunc)
{
    CamX::OfflineLogger* pOfflineLogger = OfflineLoggerGetInstance(type);
    return pOfflineLogger->SetFlushTriggerLink(pSignalFunc);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflinelogGetSettingInterface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL OfflinelogGetSettingInterface(
    OfflineLoggerType type,
    PFNGETSETTINGS pSetting)
{
    CamX::OfflineLogger* pOfflineLogger = OfflineLoggerGetInstance(type);
    return pOfflineLogger->GetSettingLink(pSetting);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiOfflineLogEntry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC VOID ChiOfflineLogEntry(
    CHIOFFLINELOGOPS* pOfflineLogOps)
{
    if (NULL != pOfflineLogOps)
    {
        pOfflineLogOps->size                     = sizeof(ChiOfflineLogOps);

        pOfflineLogOps->pAddLog                  = OfflinelogAddLogInterface;
        pOfflineLogOps->pFlushLog                = OfflinelogFlushLogInterface;
        pOfflineLogOps->pNotifyCameraOpen        = OfflinelogNoticeCameraOpenInterface;
        pOfflineLogOps->pNotifyCameraClose       = OfflinelogNoticeCameraCloseInterface;
        pOfflineLogOps->pDisableOfflineLog       = OfflinelogDisableInterface;
        pOfflineLogOps->pIsEnableOfflineLog      = OfflinelogIsEnableInterface;
        pOfflineLogOps->pLinkFlushTrigger        = OfflinelogLinkFlushTriggerInterface;
        pOfflineLogOps->pGetSetting              = OfflinelogGetSettingInterface;
    }
}

#ifdef __cplusplus
}
#endif // __cplusplus

CAMX_NAMESPACE_BEGIN

static struct sigaction g_defaultsigsegvhndlr;
static struct sigaction g_defaultsigabrthndlr;
static struct sigaction g_defaultsigfpehndlr;
static struct sigaction g_defaultsigillhndlr;
static struct sigaction g_defaultsigbushndlr;
static struct sigaction g_defaultsigsyshndlr;
static struct sigaction g_defaultsigtraphndlr;
static BOOL             g_signalRegistered = FALSE;

thread_local BYTE* OfflineLogger::m_threadLocalLogBuffer[MaxSupportLoggerTypeCount]   = { 0 };
thread_local INT   OfflineLogger::m_threadLocalQueuePoolId[MaxSupportLoggerTypeCount] = { 0 };
thread_local UINT  OfflineLogger::m_threadLocalSessionCnt[MaxSupportLoggerTypeCount]  = { 0 };
thread_local UINT  OfflineLogger::m_threadLocalSegmentCnt[MaxSupportLoggerTypeCount]  = { 1, 1};
thread_local UINT  OfflineLogger::m_threadLocalLogSize[MaxSupportLoggerTypeCount]     = { 0 };
thread_local UINT  OfflineLogger::m_threadLocalLogCurPos[MaxSupportLoggerTypeCount]   = { 0 };
thread_local BOOL  OfflineLogger::m_threadLocalFlushDone[MaxSupportLoggerTypeCount]   = { FALSE, FALSE};
thread_local LifetimeThreadInfo OfflineLogger::m_threadLocalLifeTime[MaxSupportLoggerTypeCount];

BOOL                        OfflineLogger::s_isInit                      = FALSE;
BOOL                        OfflineLogger::s_isInitValid                 = FALSE;
std::vector<OfflineLogger*> OfflineLogger::s_pOfflineLoggerInstancePool;
mutex                       OfflineLogger::m_pOfflineLoggerLock;
mutex                       OfflineLogger::s_initMutex;

static const UINT MaxPreAllocateQueue        = 25;                      // Total memory buckets    : 25
static const INT  MaxFileSize                = 52428800;                // Max size of one log fize: 50 (MB)
static const INT  MaxLocalBufferSize         = 3145728;                 // Each memory bucket size : 3  (MB)
static const UINT MinBufferNumFlushCriteria  = MaxPreAllocateQueue / 2; // Exceed this mem buckets will trigger flush

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// InvokeDefaultHndlr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID InvokeDefaultHndlr(
    struct sigaction* pAction,
    INT32             sigNum,
    siginfo_t*        pSigInfo,
    VOID*             pContext)
{
    if (pAction->sa_flags & SA_SIGINFO)
    {
        pAction->sa_sigaction(sigNum, pSigInfo, pContext);
    }
    else
    {
        pAction->sa_handler(sigNum);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SignalCatcher
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SignalCatcher(
    INT        sigNum,
    siginfo_t* pSigInfo,
    VOID*      pContext)
{
    string beginOfCrash("--------- beginning of crash");
    CamX::OfflineLogger* pOfflineLogger = OfflineLoggerGetInstance(OfflineLoggerType::ASCII);
    pOfflineLogger->AddLog(beginOfCrash.c_str(), beginOfCrash.length());

    // Force to do the last time flush to file to ensure we get complete error log.
    pOfflineLogger->FlushLogToFile(TRUE);

    switch (sigNum)
    {
        case SIGSEGV:
            InvokeDefaultHndlr(&g_defaultsigsegvhndlr, sigNum, pSigInfo, pContext);
            break;
        case SIGABRT:
            InvokeDefaultHndlr(&g_defaultsigabrthndlr, sigNum, pSigInfo, pContext);
            break;
        case SIGFPE:
            InvokeDefaultHndlr(&g_defaultsigfpehndlr, sigNum, pSigInfo, pContext);
            break;
        case SIGBUS:
            InvokeDefaultHndlr(&g_defaultsigbushndlr, sigNum, pSigInfo, pContext);
            break;
        case SIGSYS:
            InvokeDefaultHndlr(&g_defaultsigsyshndlr, sigNum, pSigInfo, pContext);
            break;
        case SIGTRAP:
            InvokeDefaultHndlr(&g_defaultsigtraphndlr, sigNum, pSigInfo, pContext);
            break;
        case SIGILL:
            InvokeDefaultHndlr(&g_defaultsigillhndlr, sigNum, pSigInfo, pContext);
            break;
        default:
            OFFLINELOG_ERROR("Signal %d received, No action matching", sigNum);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RegisterSignalHandlers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID RegisterSignalHandlers()
{
    struct sigaction newSigAction;

    memset(&newSigAction, '\0', sizeof(struct sigaction));
    if (sigaction(SIGSEGV, NULL, &g_defaultsigsegvhndlr) < 0)
    {
        OFFLINELOG_ERROR("Failed to get signal handler for SIGSEGV");
    }
    newSigAction              = g_defaultsigsegvhndlr;
    newSigAction.sa_flags    |= SA_SIGINFO;
    newSigAction.sa_sigaction = SignalCatcher;
    if (sigaction(SIGSEGV, &newSigAction, NULL) < 0)
    {
        OFFLINELOG_ERROR("Failed to register signal handler for SIGSEGV");
    }
    if (sigaction(SIGABRT, &newSigAction, &g_defaultsigabrthndlr) < 0)
    {
        OFFLINELOG_ERROR("Failed to register signal handler for SIGABRT");
    }
    if (sigaction(SIGBUS, &newSigAction, &g_defaultsigbushndlr) < 0)
    {
        OFFLINELOG_ERROR("Failed to register signal handler for SIGBUS");
    }
    if (sigaction(SIGFPE, &newSigAction, &g_defaultsigfpehndlr) < 0)
    {
        OFFLINELOG_ERROR("Failed to register signal handler for SIGFPE");
    }
    if (sigaction(SIGILL, &newSigAction, &g_defaultsigillhndlr) < 0)
    {
        OFFLINELOG_ERROR("Failed to register signal handler for SIGILL");
    }
    if (sigaction(SIGSYS, &newSigAction, &g_defaultsigsyshndlr) < 0)
    {
        OFFLINELOG_ERROR("Failed to register signal handler for SIGSYS");
    }
    if (sigaction(SIGTRAP, &newSigAction, &g_defaultsigtraphndlr) < 0)
    {
        OFFLINELOG_ERROR("Failed to register signal handler for SIGTRAP");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineLogger::OfflineLogger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OfflineLogger::OfflineLogger(
    OfflineLoggerType type)
{
    m_offlinelogHasPreFileName = FALSE;
    m_offlinelogStatus         = OfflineLoggerStatus::DISABLE;
    m_sessionCount             = 1;
    m_offlinelogType           = type;
    m_forceFlushDone           = FALSE;

    switch (type)
    {
        case OfflineLoggerType::ASCII:
            m_offlinelogPreLogTypeName = "OfflineLog";
            break;
        case OfflineLoggerType::BINARY:
            m_offlinelogPreLogTypeName = "BinaryLog";
            break;
        default:
            m_offlinelogPreLogTypeName = "UnKnow";
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineLogger::~OfflineLogger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OfflineLogger::~OfflineLogger()
{
    // Clear the element in our log memory pool
    for (auto buffer : m_memoryLogPool)
    {
        free(buffer);
    }
    m_memoryLogPool.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineLogger::NotifyCameraOpen
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OfflineLogger::NotifyCameraOpen()
{
    OFFLINELOG_INFO("OfflineLogger Type %d, Status:%d NotifyCameraOpen!!",
            static_cast<UINT>(m_offlinelogType), static_cast<UINT>(m_offlinelogStatus));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineLogger::NotifyCameraClose
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OfflineLogger::NotifyCameraClose()
{
    // We should not enter this below logic while disabling offlinelogger
    if (OfflineLoggerStatus::DISABLE != m_offlinelogStatus)
    {
        if (OfflineLoggerStatus::ACTIVE == m_offlinelogStatus)
        {
            m_offlinelogStatus = OfflineLoggerStatus::CAMERA_CLOSING;
            // Force the flush to file
            FlushLogToFile(TRUE);
        }

        SetAllToDefaultValue();

        m_forceFlushDone = FALSE;
        m_sessionCount++;
        m_offlinelogStatus = OfflineLoggerStatus::ACTIVE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineLogger::DisableOfflinelogger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OfflineLogger::DisableOfflinelogger()
{
    if (OfflineLoggerStatus::DISABLE != m_offlinelogStatus)
    {
        // Set OfflineLoggerStatus to disable to let all threads stop Addlog() and FlushLogToFile()
        m_offlinelogStatus = OfflineLoggerStatus::DISABLE;

        SetAllToDefaultValue();

        // Release allocated log memory pool
        for (vector<BYTE*>::iterator it = m_memoryLogPool.begin() ; it != m_memoryLogPool.end(); ++it)
        {
            delete (*it);
        }
        m_memoryLogPool.clear();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineLogger::IsEnableOfflinelogger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL OfflineLogger::IsEnableOfflinelogger()
{
    BOOL result = TRUE;

    if (OfflineLoggerStatus::DISABLE == m_offlinelogStatus)
    {
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineLogger::SetFlushTriggerLink
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL OfflineLogger::SetFlushTriggerLink(
    PFNSIGNALOFFLINETHREAD pSignalFunc)
{
    BOOL result = TRUE;

    if (NULL == pSignalFunc)
    {
        result = FALSE;
    }
    else
    {
        m_pSignalFlushTriggerThread = pSignalFunc;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineLogger::GetSettingLink
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL OfflineLogger::GetSettingLink(
    PFNGETSETTINGS pSetting)
{
    BOOL result        = TRUE;
    m_pOverrideSetting = pSetting;

    if (NULL != m_pOverrideSetting)
    {
        // Get Override Setting from CHI public API
        const UINT32       numExtendSettings = static_cast<UINT32>(ChxSettingsToken::CHX_SETTINGS_TOKEN_COUNT);
        CHIEXTENDSETTINGS* pExtendSettings   = NULL;
        CHIMODIFYSETTINGS* pModifySettings   = NULL;
        m_pOverrideSetting(&pExtendSettings, &pModifySettings);

        // Loading ASCII/Binary override setting
        ChiModifySettings* pMod            = NULL;
        UINT32             enableAsciilog  = static_cast<UINT32>(ChxSettingsToken::EnableAsciilog);
        UINT32             enableBinarylog = static_cast<UINT32>(ChxSettingsToken::EnableBinarylog);
        UINT8              asciilogtype    = static_cast<UINT8>(OfflineLoggerType::ASCII);
        UINT8              binarylogtype   = static_cast<UINT8>(OfflineLoggerType::BINARY);
        UINT32             logEnable       = 0;

        for (UINT32 i = 0; i < numExtendSettings; i++)
        {
            pMod = static_cast<ChiModifySettings*>(&pModifySettings[i]);

            if (pMod->token.id == enableAsciilog)
            {
                logEnable                         = *static_cast<UINT32*>(pMod->pData);
                m_enableLoggerType[asciilogtype]  = logEnable != 0;
            }
            else if (pMod->token.id == enableBinarylog)
            {
                logEnable                         = *static_cast<UINT32*>(pMod->pData);
                m_enableLoggerType[binarylogtype] = logEnable != 0;
            }
        }

        // If feature is enable then we can pre-allocate memory/set logger status as active/register signal handler
        UINT8 currentLoggerType = static_cast<UINT8>(m_offlinelogType);

        if ((currentLoggerType < OFFLINELOG_MAX_TYPE) && (TRUE == m_enableLoggerType[currentLoggerType]))
        {
            // Allocate Log queue pool
            m_pOfflineLoggerLock.lock();
            result = AllocateLogQueueMemPool();
            m_pOfflineLoggerLock.unlock();

            // If we allocate memory success then we can set logger status as active
            if (TRUE == result)
            {
                m_offlinelogStatus = OfflineLoggerStatus::ACTIVE;
            }

            // Signal handler should be registered only one time if feature enable,
            // otherwise the signal cannot get back to parent process
            if (FALSE == g_signalRegistered && TRUE == result)
            {
                RegisterSignalHandlers();
                g_signalRegistered = TRUE;
            }
        }
    }
    else
    {
        result = FALSE;
        OFFLINELOG_ERROR("OfflineLogger get override setting failed");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineLogger::GetLogInfoMask
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DebugLogInfo OfflineLogger::GetLogInfoMask()
{
    return m_debugLogInfo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineLogger::SetLogInfoMask
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OfflineLogger::SetLogInfoMask(
    DebugLogInfo* pNewLogInfo)
{
    m_debugLogInfo.groupsEnable[CamxLogConfig]    = pNewLogInfo->groupsEnable[CamxLogConfig];
    m_debugLogInfo.groupsEnable[CamxLogDump]      = pNewLogInfo->groupsEnable[CamxLogDump];
    m_debugLogInfo.groupsEnable[CamxLogWarning]   = pNewLogInfo->groupsEnable[CamxLogWarning];
    m_debugLogInfo.groupsEnable[CamxLogEntryExit] = pNewLogInfo->groupsEnable[CamxLogEntryExit];
    m_debugLogInfo.groupsEnable[CamxLogInfo]      = pNewLogInfo->groupsEnable[CamxLogInfo];
    m_debugLogInfo.groupsEnable[CamxLogPerfInfo]  = pNewLogInfo->groupsEnable[CamxLogPerfInfo];
    m_debugLogInfo.groupsEnable[CamxLogVerbose]   = pNewLogInfo->groupsEnable[CamxLogVerbose];
    m_debugLogInfo.groupsEnable[CamxLogDRQ]       = pNewLogInfo->groupsEnable[CamxLogDRQ];
    m_debugLogInfo.groupsEnable[CamxLogMeta]      = pNewLogInfo->groupsEnable[CamxLogMeta];
    m_debugLogInfo.groupsEnable[CamxLogReqMap]    = pNewLogInfo->groupsEnable[CamxLogReqMap];
    m_debugLogInfo.pDebugLogFile                  = pNewLogInfo->pDebugLogFile;
    m_debugLogInfo.systemLogEnable                = pNewLogInfo->systemLogEnable;
    m_debugLogInfo.enableAsciiLogging             = pNewLogInfo->enableAsciiLogging;
    m_debugLogInfo.isUpdated                      = pNewLogInfo->isUpdated;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineLogger::AllocateLogQueueMemPool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL OfflineLogger::AllocateLogQueueMemPool()
{
    BOOL result = TRUE;

    for (UINT i = 0; i < MaxPreAllocateQueue; i++)
    {
        BYTE* pData = (BYTE*) malloc(MaxLocalBufferSize);

        if (NULL != pData)
        {
            memset(pData, 0, MaxLocalBufferSize);
            m_memoryLogPool.push_back(pData);
            m_availableQueueIdPool.push(static_cast<INT>(m_memoryLogPool.size() - 1));
        }
        else
        {
            result = FALSE;
            OFFLINELOG_ERROR("OfflineLogger allocate memory failed");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineLogger::RequestFlushLogQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OfflineLogger::RequestFlushLogQueue(
    INT registerId)
{
    m_pOfflineLoggerLock.lock();

    m_perLogInfoPool[registerId].bNeedFlush = TRUE;
    // Increment count of threads needing flush
    m_needFlushThreadCount++;
    // Push in registerID to queue to track buffer flush order
    m_registerIdFlushOrderQueue.push(registerId);
    // Buffer is full, signal CHI flush thread to trigger a flush
    if (NULL != m_pSignalFlushTriggerThread)
    {
        OFFLINELOG_INFO("OfflineLogger Type %d: RequestFlushLogQueue Signaled!!"
        "Signal = %p, m_needFlushThreadCount:%d",
            static_cast<UINT>(m_offlinelogType), m_pSignalFlushTriggerThread, m_needFlushThreadCount);
        m_pSignalFlushTriggerThread(m_offlinelogType);
    }

    m_pOfflineLoggerLock.unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LifetimeThreadInfo::~LifetimeThreadInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LifetimeThreadInfo::~LifetimeThreadInfo()
{
    // Trigger flush while thread being killed
    if (TRUE == enableLoggerType)
    {
        CamX::OfflineLogger* pOfflineLogger = OfflineLoggerGetInstance(loggerType);
        pOfflineLogger->RequestFlushLogQueue(registerID);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineLogger::RequestNewLogQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT OfflineLogger::RequestNewLogQueue(
    BYTE* &rpNewQueue,
    UINT tid,
    INT  segment)
{
    m_pOfflineLoggerLock.lock();

    INT  memoryPoolId  = -1;
    INT  registerId    = -1;
    UINT sessionCount  = m_threadLocalSessionCnt[static_cast<UINT>(m_offlinelogType)];

    if (FALSE == m_availableQueueIdPool.empty())
    {
        memoryPoolId = m_availableQueueIdPool.front();
        m_availableQueueIdPool.pop();

        if (FALSE == m_availableRegisterIdPool.empty())
        {
            registerId = m_availableRegisterIdPool.front();
            m_availableRegisterIdPool.pop();

            m_perLogInfoPool[registerId].bNeedFlush     = FALSE;
            m_perLogInfoPool[registerId].bHasFileName   = FALSE;
            m_perLogInfoPool[registerId].registerID     = memoryPoolId;
            m_perLogInfoPool[registerId].threadID       = tid;
            m_perLogInfoPool[registerId].sessionCount   = sessionCount;
            m_perLogInfoPool[registerId].segment        = segment;
            m_perLogInfoPool[registerId].numByteToWrite = 0;
        }
        else
        {
            PerLogThreadInfo info;
            info.bNeedFlush     = FALSE;
            info.bHasFileName   = FALSE;
            info.registerID     = memoryPoolId;
            info.threadID       = tid;
            info.sessionCount   = sessionCount;
            info.segment        = segment;
            info.numByteToWrite = 0;
            m_perLogInfoPool.push_back(info);
            registerId = m_perLogInfoPool.size()-1;
        }
    }
    else
    {
        // We don`t have enough available memory so need to expand it
        AllocateLogQueueMemPool();

        if (FALSE == m_availableQueueIdPool.empty())
        {
            memoryPoolId = m_availableQueueIdPool.front();
            m_availableQueueIdPool.pop();

            PerLogThreadInfo info;
            info.bNeedFlush     = FALSE;
            info.bHasFileName   = FALSE;
            info.registerID     = memoryPoolId;
            info.threadID       = tid;
            info.sessionCount   = sessionCount;
            info.segment        = segment;
            info.numByteToWrite = 0;
            m_perLogInfoPool.push_back(info);
            registerId = m_perLogInfoPool.size()-1;
        }
    }

    if (registerId == -1)
    {
        // registerId equal to default value, it must be a serious failure
        OFFLINELOG_ERROR("Offlinelog failed to request new container");
        rpNewQueue = NULL;
    }
    else
    {
        rpNewQueue = m_memoryLogPool[memoryPoolId];
    }

    m_threadLocalLifeTime[static_cast<UINT>(m_offlinelogType)].threadID         = tid;
    m_threadLocalLifeTime[static_cast<UINT>(m_offlinelogType)].registerID       = registerId;
    m_threadLocalLifeTime[static_cast<UINT>(m_offlinelogType)].enableLoggerType = TRUE;
    m_threadLocalLifeTime[static_cast<UINT>(m_offlinelogType)].loggerType       = m_offlinelogType;

    m_pOfflineLoggerLock.unlock();

    return registerId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineLogger::AddLog
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OfflineLogger::AddLog(
    const CHAR* pLog,
    UINT logSize)
{
    CamxResult result = CamxResultSuccess;

    if (FALSE == s_isInitValid)
    {
        result = CamxResultEFailed;
    }

    // The offline logger is disable
    if (OfflineLoggerStatus::DISABLE == m_offlinelogStatus)
    {
        result = CamxResultEUnsupported;
    }

    if (CamxResultSuccess == result)
    {
        UINT8 offlinelogtype = static_cast<UINT8>(m_offlinelogType);
        // If the camera is reopen, the thread local session count will minus one than offlinelogger
        if (m_threadLocalSessionCnt[offlinelogtype] != m_sessionCount)
        {
            m_threadLocalSessionCnt[offlinelogtype]  = m_sessionCount;

            // Reset the segment and file size
            m_threadLocalLogSize[offlinelogtype]      = 0;
            m_threadLocalSegmentCnt[offlinelogtype]   = 1;
            m_threadLocalLogCurPos[offlinelogtype]    = 0;
            m_threadLocalFlushDone[offlinelogtype]    = FALSE;

            // Done reset, request a new buffer to add log
            m_threadLocalQueuePoolId[offlinelogtype] = RequestNewLogQueue(m_threadLocalLogBuffer[offlinelogtype],
                gettid(),
                m_threadLocalSegmentCnt[offlinelogtype]);

            if (NULL != m_threadLocalLogBuffer[offlinelogtype])
            {
                auto memoryAddressToWrite = &(m_threadLocalLogBuffer[offlinelogtype]
                                             [m_threadLocalLogCurPos[offlinelogtype]]);
                m_threadLocalLogCurPos[offlinelogtype] += logSize;
                m_threadLocalLogSize[offlinelogtype]   += logSize;
                m_perLogInfoPool[m_threadLocalQueuePoolId[offlinelogtype]].numByteToWrite
                                                                          = m_threadLocalLogCurPos[offlinelogtype];
                memcpy(memoryAddressToWrite, pLog, logSize);
            }
        }
        else
        {
            if (NULL != m_threadLocalLogBuffer[offlinelogtype])
            {
                // If we find local flush flag != central flush flag means
                // force flash happened in central and flush all the container done
                // so need to reset local log size and cursor pos
                if (m_threadLocalFlushDone[offlinelogtype] != m_forceFlushDone &&
                    m_threadLocalFlushDone[offlinelogtype] == FALSE)
                {
                    m_threadLocalFlushDone[offlinelogtype] = m_forceFlushDone;
                    m_threadLocalLogSize[offlinelogtype]   = 0;
                    m_threadLocalLogCurPos[offlinelogtype] = 0;
                    m_perLogInfoPool[m_threadLocalQueuePoolId[offlinelogtype]].numByteToWrite = 0;
                }
                if (m_threadLocalLogCurPos[offlinelogtype] + logSize > MaxLocalBufferSize)
                {
                    // request offlinelog to flush the current container
                    RequestFlushLogQueue(m_threadLocalQueuePoolId[offlinelogtype]);

                    // request new queue for thread local container
                    m_threadLocalQueuePoolId[offlinelogtype] = RequestNewLogQueue(m_threadLocalLogBuffer[offlinelogtype],
                                                                                    gettid(),
                                                                                    m_threadLocalSegmentCnt[offlinelogtype]);
                    m_threadLocalLogCurPos[offlinelogtype]   = 0;

                    if (NULL != m_threadLocalLogBuffer[offlinelogtype])
                    {
                        auto memoryAddressToWrite = &(m_threadLocalLogBuffer[offlinelogtype]
                                                     [m_threadLocalLogCurPos[offlinelogtype]]);
                        m_threadLocalLogCurPos[offlinelogtype] += logSize;
                        m_threadLocalLogSize[offlinelogtype]   += logSize;
                        m_perLogInfoPool[m_threadLocalQueuePoolId[offlinelogtype]].numByteToWrite
                                                                                  = m_threadLocalLogCurPos[offlinelogtype];
                        memcpy(memoryAddressToWrite, pLog, logSize);
                    }
                }
                else if (m_threadLocalLogSize[offlinelogtype] > MaxFileSize)
                {
                    m_threadLocalSegmentCnt[offlinelogtype]++;
                    m_threadLocalLogSize[offlinelogtype] = 0 ;

                    RequestFlushLogQueue(m_threadLocalQueuePoolId[offlinelogtype]);

                    m_threadLocalQueuePoolId[offlinelogtype] = RequestNewLogQueue(m_threadLocalLogBuffer[offlinelogtype],
                                                                  gettid(),
                                                                  m_threadLocalSegmentCnt[offlinelogtype]);
                    m_threadLocalLogCurPos[offlinelogtype]   = 0;

                    if (NULL != m_threadLocalLogBuffer[offlinelogtype])
                    {
                        auto memoryAddressToWrite = &(m_threadLocalLogBuffer[offlinelogtype]
                                                     [m_threadLocalLogCurPos[offlinelogtype]]);
                        m_threadLocalLogCurPos[offlinelogtype] += logSize;
                        m_threadLocalLogSize[offlinelogtype]   += logSize;
                        m_perLogInfoPool[m_threadLocalQueuePoolId[offlinelogtype]].numByteToWrite
                                                                                  = m_threadLocalLogCurPos[offlinelogtype];
                        memcpy(memoryAddressToWrite, pLog, logSize);
                    }
                }
                else
                {
                    auto memoryAddressToWrite = &(m_threadLocalLogBuffer[offlinelogtype]
                                  [m_threadLocalLogCurPos[offlinelogtype]]);
                    m_threadLocalLogCurPos[offlinelogtype] += logSize;
                    m_threadLocalLogSize[offlinelogtype]   += logSize;
                    m_perLogInfoPool[m_threadLocalQueuePoolId[offlinelogtype]].numByteToWrite
                                                                              = m_threadLocalLogCurPos[offlinelogtype];
                    memcpy(memoryAddressToWrite, pLog, logSize);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineLogger::FlushLogToFile
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OfflineLogger::FlushLogToFile(
    BOOL lastFlush)
{
    CamxResult result = CamxResultSuccess;

    if (FALSE == s_isInitValid)
    {
        result = CamxResultEFailed;
    }

    // The offline logger is disable
    if (OfflineLoggerStatus::DISABLE == m_offlinelogStatus)
    {
        result = CamxResultEUnsupported;
    }

    if (CamxResultSuccess == result)
    {
        // While we do the last flush means we flush all the container we have
        // so we need to mark we are in the closing status and each thread can deal with the special case
        if (TRUE == lastFlush)
        {
            m_offlinelogStatus = OfflineLoggerStatus::CAMERA_CLOSING;

            // Push all valid registerIDs to queue so they will be force flushed
            for (UINT i = 0; i < m_perLogInfoPool.size(); i++)
            {
                m_registerIdFlushOrderQueue.push(i);
            }
        }

        // Get Pre-fileName
        if (FALSE == m_offlinelogHasPreFileName)
        {
            GetPrefixFilename();
        }

        m_pOfflineLoggerLock.lock();
        // Only continue flush if at least half of buffers full or this is a force flush.
        // Otherwise skip below operations for performance
        if (m_needFlushThreadCount >= MinBufferNumFlushCriteria || lastFlush)
        {
            // Flush in order, or if lastFlush = TRUE, flush all
            while (TRUE != m_registerIdFlushOrderQueue.empty())
            {
                auto it = std::next(m_perLogInfoPool.begin(), m_registerIdFlushOrderQueue.front());

                // First time flushing need to get the complete file name and store it
                // Or if got to do the last flush in a session, we need to get a file name
                if ((TRUE == (*it).bNeedFlush && FALSE == (*it).bHasFileName) ||
                    (FALSE == (*it).bNeedFlush && FALSE == (*it).bHasFileName &&
                    (TRUE == lastFlush && (*it).numByteToWrite > 0)))
                {
                    // Better performance with append string instead of using "+" only
                    ((*it).offlinelogFileName) = m_offlinelogPreFileName;
                    ((*it).offlinelogFileName) += "_Tid";
                    ((*it).offlinelogFileName) += std::to_string((*it).threadID);
                    ((*it).offlinelogFileName) += "_Session";
                    ((*it).offlinelogFileName) += std::to_string((*it).sessionCount);
                    ((*it).offlinelogFileName) += "_Segment";
                    ((*it).offlinelogFileName) += std::to_string((*it).segment);
                    if (OfflineLoggerType::BINARY == m_offlinelogType)
                    {
                        ((*it).offlinelogFileName) += ".bin";
                    }
                    else
                    {
                        ((*it).offlinelogFileName) += ".txt";
                    }
                    (*it).bHasFileName = TRUE;
                }

                // Already has a filename, so flush out the log to file
                // or if got to do last flush in a session, we flush out all log to files
                if ((TRUE == (*it).bNeedFlush && TRUE == (*it).bHasFileName) ||
                    (FALSE == (*it).bNeedFlush && TRUE == (*it).bHasFileName &&
                    (TRUE == lastFlush && (*it).numByteToWrite > 0)))
                {
                    FILE* pFile = NULL;
                    if (OfflineLoggerType::BINARY == m_offlinelogType)
                    {
                        pFile = fopen(((*it).offlinelogFileName).c_str(), "ab+");
                    }
                    else
                    {
                        pFile = fopen(((*it).offlinelogFileName).c_str(), "a+");
                    }

                    if (NULL != pFile)
                    {
                        fwrite(m_memoryLogPool[(*it).registerID], sizeof(CHAR), (*it).numByteToWrite, pFile);
                        memset(m_memoryLogPool[(*it).registerID], 0, MaxLocalBufferSize);

                        fclose(pFile);

                        (*it).bNeedFlush = FALSE;
                        (*it).bHasFileName = FALSE;
                        (*it).numByteToWrite = 0;

                        if (m_needFlushThreadCount >= 1)
                        {
                            m_needFlushThreadCount--;
                        }

                        // Push available id to pool and remove flush job from job pool
                        m_availableQueueIdPool.push((*it).registerID);
                        m_availableRegisterIdPool.push(m_registerIdFlushOrderQueue.front());
                    }
                    else
                    {
                        OFFLINELOG_ERROR("Offlinelog failed to open file and write");
                    }
                }
                m_registerIdFlushOrderQueue.pop();
            }
        }

        if (TRUE == lastFlush)
        {
            m_forceFlushDone  = TRUE;
        }
        m_pOfflineLoggerLock.unlock();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineLogger::SetAllToDefaultValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OfflineLogger::SetAllToDefaultValue()
{
    m_pOfflineLoggerLock.lock();

    m_offlinelogHasPreFileName = FALSE;
    m_offlinelogPreFileName    = "";

    // Empty the available memory pool
    while (!m_availableQueueIdPool.empty())
    {
        m_availableQueueIdPool.pop();
    }

    // Mark all the memory queue pool as available
    for (SIZE_T i = 0; i < m_memoryLogPool.size(); i++)
    {
        m_availableQueueIdPool.push(static_cast<INT>(i));
    }

    // Empty the available register pool
    while (!m_availableRegisterIdPool.empty())
    {
        m_availableRegisterIdPool.pop();
    }

    // Empty the flush order queue
    while (!m_registerIdFlushOrderQueue.empty())
    {
        m_registerIdFlushOrderQueue.pop();
    }

    // Empty per thread info pool
    m_perLogInfoPool.clear();

    m_pOfflineLoggerLock.unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineLogger::GetPrefixFilename
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
string OfflineLogger::GetPrefixFilename()
{
    CHAR dumpFilename[FILENAME_MAX];
    CHAR timeStamp[MaxTimeStampLength];

    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    struct tm* pCurrentTime = localtime((time_t *)&tv.tv_sec);
    if (pCurrentTime != NULL)
    {
        snprintf(timeStamp, MaxTimeStampLength, "%2d-%02d-%02d--%02d-%02d-%02d-%06ld",
            (pCurrentTime->tm_year + 1900),
            (pCurrentTime->tm_mon + 1),
            pCurrentTime->tm_mday,
            pCurrentTime->tm_hour,
            pCurrentTime->tm_min,
            pCurrentTime->tm_sec,
            tv.tv_usec);
    }
    else
    {
        timeStamp[0] = '\0';
    }

    snprintf(dumpFilename,
            sizeof(dumpFilename),
            "%s/Camx_%s_%s_",
            CamX::ConfigFileDirectory,
            m_offlinelogPreLogTypeName.c_str(),
            timeStamp);

    m_offlinelogPreFileName    = dumpFilename;
    m_offlinelogHasPreFileName = TRUE;

    return dumpFilename;
}

CAMX_NAMESPACE_END