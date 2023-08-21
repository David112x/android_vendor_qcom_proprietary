////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxsettingsmanager.cpp
/// @brief Definitions for the SettingsManager class.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Common CamX Includes
#include "camxincs.h"
#include "camxmem.h"

// Core CamX Includes
#include "camxsettingsmanager.h"
#include "g_camxsettings.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constant Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const UINT MaxNumOverrideSettingStores = 2;  ///< The maximum number of override settings stores used by this
                                                    ///  SettingsManager.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SettingsManager::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SettingsManager* SettingsManager::Create(
    StaticSettings* pStaticSettings)
{
    CamxResult result = CamxResultSuccess;

    // Since this creation function is only used for static initialization, we don't want to track memory.
    SettingsManager* pSettingsManager = CAMX_NEW SettingsManager();
    if (pSettingsManager != NULL)
    {
        result = pSettingsManager->Initialize(pStaticSettings);
        if (CamxResultSuccess != result)
        {
            CAMX_DELETE pSettingsManager;
            pSettingsManager = NULL;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory; cannot create SettingsManager");
    }

    return pSettingsManager;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SettingsManager::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SettingsManager::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Protected Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SettingsManager::SettingsManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SettingsManager::SettingsManager()
    : m_internallyAllocatedStaticSettings(FALSE)
    , m_pStaticSettings(NULL)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SettingsManager::~SettingsManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SettingsManager::~SettingsManager()
{
    // Destroy in reverse order of creation
    if (TRUE == m_internallyAllocatedStaticSettings)
    {
        if (NULL != m_pStaticSettings)
        {
            CAMX_FREE(m_pStaticSettings);
            m_pStaticSettings = NULL;
        }
    }

    if (NULL != m_pOverrideSettingsStore)
    {
        m_pOverrideSettingsStore->Destroy();
        m_pOverrideSettingsStore = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SettingsManager::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SettingsManager::Initialize(
    StaticSettings* pStaticSettings)
{
    CamxResult result = CamxResultSuccess;

    // If the client gave us a static settings, use that. Otherwise, create our own.
    if (NULL != pStaticSettings)
    {
        m_pStaticSettings = pStaticSettings;
    }
    else
    {
        m_pStaticSettings = reinterpret_cast<StaticSettings*>(CAMX_CALLOC(sizeof(StaticSettings)));
        if (NULL != m_pStaticSettings)
        {
            m_internallyAllocatedStaticSettings = TRUE;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory; cannot allocate static settings structure");
            result = CamxResultENoMemory;
        }
    }

    // Create the override settings file helper
    m_pOverrideSettingsStore = OverrideSettingsFile::Create();

    if (NULL == m_pOverrideSettingsStore)
    {
        result = CamxResultEFailed;
    }

    // Initialize the settings structure and override with user's values
    if (CamxResultSuccess == result)
    {
        // Populate the default settings
        InitializeDefaultSettings();
        InitializeDefaultDebugSettings();

#if SETTINGS_DUMP_ENABLE
        if (CamxResultSuccess == result)
        {
            // Print all current settings
            DumpSettings();

            // Dump the override settings from our override settings stores
            m_pOverrideSettingsStore->DumpOverriddenSettings();
        }
#endif // SETTINGS_DUMP_ENABLE

        // Load the override settings from our override settings stores
        result = LoadOverrideSettings(m_pOverrideSettingsStore);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to load override settings.");
        }

        if (CamxResultSuccess == result)
        {
            result = LoadOverrideProperties(m_pOverrideSettingsStore, TRUE);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to load override properties.");
            }
        }
    }

    // Validate the updated settings structures
    if (CamxResultSuccess == result)
    {
        result = ValidateSettings();
    }

#if SETTINGS_DUMP_ENABLE
    if (CamxResultSuccess == result)
    {
        // Print all current settings
        DumpSettings();

        // Dump the override settings from our override settings stores
        m_pOverrideSettingsStore->DumpOverriddenSettings();
    }
#endif // SETTINGS_DUMP_ENABLE

    // Push log settings to utils
    UpdateLogSettings();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SettingsManager::HwInitializeDefaultSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SettingsManager::HwInitializeDefaultSettings()
{
    // This method will be overridden by the hardware dependent layers
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SettingsManager::HwInitializeDefaultDebugSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SettingsManager::HwInitializeDefaultDebugSettings()
{
    // This method will be overridden by the hardware dependent layers
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SettingsManager::HwLoadOverrideSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SettingsManager::HwLoadOverrideSettings(
    IOverrideSettingsStore* pOverrideSettingsStore)
{
    CAMX_UNREFERENCED_PARAM(pOverrideSettingsStore);

    // This method will be overridden by the hardware dependent layers
    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SettingsManager::HwLoadOverrideProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SettingsManager::HwLoadOverrideProperties(
    IOverrideSettingsStore* pOverrideSettingsStore,
    BOOL                    updateStatic)
{
    CAMX_UNREFERENCED_PARAM(pOverrideSettingsStore);
    CAMX_UNREFERENCED_PARAM(updateStatic);

    // This method will be overridden by the hardware dependent layers
    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SettingsManager::HwValidateSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SettingsManager::HwValidateSettings()
{
    // This method will be overridden by the hardware dependent layers
    return CamxResultSuccess;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SettingsManager::HwDumpSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SettingsManager::HwDumpSettings() const
{
    // This method will be overridden by the hardware dependent layers
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SettingsManager::ValidateSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SettingsManager::ValidateSettings()
{
    CamxResult result = CamxResultSuccess;

    result = HwValidateSettings();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SettingsManager::UpdateOverrideProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SettingsManager::UpdateOverrideProperties()
{
    // First enumerate all of the properties that may have been set and add them to
    // the setting's store hashmap
    reinterpret_cast<OverrideSettingsFile*>(m_pOverrideSettingsStore)->UpdatePropertyList();

    // Now actually update the values based on the property strings
    CamxResult result = LoadOverrideProperties(m_pOverrideSettingsStore, FALSE);

    if (result != CamxResultSuccess)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Updating override properties failed: %d", result);
    }

    // Since log messages can be dynamic update the log groups and turn on/off anything that may have changed
    UpdateLogSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SettingsManager::UpdateLogSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SettingsManager::UpdateLogSettings()
{
    if (NULL != m_pStaticSettings)
    {
        // Update asserts
        CamxUpdateAssertMask(static_cast<CamxAssert>(m_pStaticSettings->assertMask));

        // Update logging
        DebugLogInfo newLogInfo;
        newLogInfo.groupsEnable[CamxLogConfig]    = m_pStaticSettings->logConfigMask;
        newLogInfo.groupsEnable[CamxLogDump]      = m_pStaticSettings->logDumpMask;
        newLogInfo.groupsEnable[CamxLogWarning]   = m_pStaticSettings->logWarningMask;
        newLogInfo.groupsEnable[CamxLogEntryExit] = m_pStaticSettings->logEntryExitMask;
        newLogInfo.groupsEnable[CamxLogInfo]      = m_pStaticSettings->logInfoMask;
        newLogInfo.groupsEnable[CamxLogPerfInfo]  = m_pStaticSettings->logPerfInfoMask;
        newLogInfo.groupsEnable[CamxLogVerbose]   = m_pStaticSettings->logVerboseMask;
        newLogInfo.groupsEnable[CamxLogDRQ]       = (TRUE == m_pStaticSettings->logDRQEnable) ? CamxLogGroupDRQ : 0;
        newLogInfo.groupsEnable[CamxLogMeta]      = (TRUE == m_pStaticSettings->logMetaEnable) ? CamxLogGroupMeta : 0;
        newLogInfo.groupsEnable[CamxLogReqMap]    = (TRUE == m_pStaticSettings->logRequestMapping) ? 0xFFFFFFFF : 0;
        newLogInfo.systemLogEnable                = m_pStaticSettings->systemLogEnable;
        newLogInfo.enableAsciiLogging             = m_pStaticSettings->enableAsciiLogging;
        newLogInfo.isUpdated                      = TRUE;


        if ('\0' != m_pStaticSettings->debugLogFilename[0])
        {
            FILE*   pDebugLogFile           = NULL;
            CHAR    filename[FILENAME_MAX]  = {0};
            OsUtils::SNPrintF(filename, sizeof(filename),
                              "%s%s%s.bin",
                              ConfigFileDirectory, PathSeparator, m_pStaticSettings->debugLogFilename);

            pDebugLogFile               = OsUtils::FOpen(filename, "w");
            newLogInfo.pDebugLogFile    = pDebugLogFile;
        }
        else
        {
            newLogInfo.pDebugLogFile = NULL;
        }
        Log::UpdateLogInfo(&newLogInfo, TRUE);

        // Update trace
        g_traceInfo.groupsEnable        = m_pStaticSettings->traceGroupsEnable;
        g_traceInfo.traceErrorLogEnable = m_pStaticSettings->traceErrorEnable;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SettingsManager::DumpOverrideSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SettingsManager::DumpOverrideSettings() const
{
    if (NULL != m_pOverrideSettingsStore)
    {
        // Dump the override settings from our override settings stores
        m_pOverrideSettingsStore->DumpOverriddenSettings();
    }
}

CAMX_NAMESPACE_END
