////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxoverridesettingsfile.cpp
/// @brief Definitions for the OverrideSettingsFile class, implementing the IOverrideSettingsStore interface.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Common CamX Includes
#include "camxincs.h"
#include "camxmem.h"

// Core CamX Includes
#include "camxoverridesettingsfile.h"
#include "g_camxsettings.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Static data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Valid override settings file names
static const CHAR* OverrideSettingsTextFileName =
{
    "camxoverridesettings.txt"
};

// A prioritized list of where override settings text files may be found. All files found in all directories will be parsed.
// The last override value for the setting wins.
static const CHAR* OverrideSettingsTextFileDirectories[] =
{
    OverrideSettingsPath,
    "."
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OverrideSettingsFile::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OverrideSettingsFile* OverrideSettingsFile::Create()
{
    CamxResult              result                  = CamxResultSuccess;
    OverrideSettingsFile*   pOverrideSettingsFile   = CAMX_NEW OverrideSettingsFile();
    if (pOverrideSettingsFile != NULL)
    {
        result = pOverrideSettingsFile->Initialize();
        if (CamxResultSuccess != result)
        {
            CAMX_DELETE pOverrideSettingsFile;
            pOverrideSettingsFile = NULL;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory; cannot create OverrideSettingsFile");
    }

    return pOverrideSettingsFile;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OverrideSettingsFile::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OverrideSettingsFile::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OverrideSettingsFile::ReadSettingINT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OverrideSettingsFile::ReadSettingINT(
    UINT32  settingStringHash,
    INT*    pVal
    ) const
{
    CAMX_ASSERT(NULL != pVal);

    CamxResult result = CamxResultEFailed;

    SettingCacheEntry* pSettingCacheEntry = FindOverrideSetting(settingStringHash);
    if (NULL != pSettingCacheEntry)
    {
        result = ParseSettingInt(pSettingCacheEntry->valueString, pVal);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Empty INT value for setting 0x%x", pSettingCacheEntry->settingStringHash);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OverrideSettingsFile::ReadSettingUINT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OverrideSettingsFile::ReadSettingUINT(
    UINT32  settingStringHash,
    UINT*   pVal
    ) const
{
    CAMX_ASSERT(NULL != pVal);
    CamxResult result = CamxResultEFailed;

    SettingCacheEntry* pSettingCacheEntry = FindOverrideSetting(settingStringHash);
    if (NULL != pSettingCacheEntry)
    {

        result = ParseSettingUint(pSettingCacheEntry->valueString, pVal);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Empty UINT value for setting 0x%x", pSettingCacheEntry->settingStringHash);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OverrideSettingsFile::ReadSettingBOOL
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OverrideSettingsFile::ReadSettingBOOL(
    UINT32  settingStringHash,
    BOOL*   pVal
    ) const
{
    CAMX_ASSERT(NULL != pVal);
    CamxResult result = CamxResultEFailed;

    SettingCacheEntry* pSettingCacheEntry = FindOverrideSetting(settingStringHash);
    if (NULL != pSettingCacheEntry)
    {
        result = ParseSettingBool(pSettingCacheEntry->valueString, pVal);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Empty Boolean value for setting 0x%x", pSettingCacheEntry->settingStringHash);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OverrideSettingsFile::ReadSettingFLOAT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OverrideSettingsFile::ReadSettingFLOAT(
    UINT32  settingStringHash,
    FLOAT*  pVal
    ) const
{
    CAMX_ASSERT(NULL != pVal);
    CamxResult result = CamxResultEFailed;

    SettingCacheEntry* pSettingCacheEntry = FindOverrideSetting(settingStringHash);
    if (NULL != pSettingCacheEntry)
    {
        result = ParseSettingFloat(pSettingCacheEntry->valueString, pVal);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Empty FLOAT value for setting 0x%x", pSettingCacheEntry->settingStringHash);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OverrideSettingsFile::ReadSettingEnum
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OverrideSettingsFile::ReadSettingEnum(
    UINT32                  settingStringHash,
    INT*                    pVal,
    EnumeratorToHashMap*    pEnumeratorToHashMap,
    UINT                    enumeratorToHashMapSize
    ) const
{
    CAMX_ASSERT(NULL != pVal);

    CamxResult          result              = CamxResultSuccess;
    SettingCacheEntry*  pSettingCacheEntry  = FindOverrideSetting(settingStringHash);
    if (NULL != pSettingCacheEntry)
    {
        result = ParseSettingEnum(pSettingCacheEntry->valueString, pVal, pEnumeratorToHashMap, enumeratorToHashMapSize);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Empty Enum value for setting 0x%x", pSettingCacheEntry->settingStringHash);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OverrideSettingsFile::ReadSettingString
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OverrideSettingsFile::ReadSettingString(
    UINT32  settingStringHash,
    CHAR*   pVal,
    SIZE_T  stringLength
    ) const
{
    CAMX_ASSERT(NULL != pVal);

    SettingCacheEntry* pSettingCacheEntry = FindOverrideSetting(settingStringHash);

    if (NULL != pSettingCacheEntry)
    {
        ParseSettingString(pSettingCacheEntry->valueString, pVal, stringLength);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OverrideSettingsFile::IsSettingOverridden
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL OverrideSettingsFile::IsSettingOverridden(
    UINT32 settingStringHash
    ) const
{
    return (FindOverrideSetting(settingStringHash) == NULL) ? FALSE : TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OverrideSettingsFile::DumpOverriddenSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OverrideSettingsFile::DumpOverriddenSettings() const
{
    CAMX_LOG_VERBOSE(CamxLogGroupCore, "=============== BEGIN DUMP OF OVERRIDE SETTINGS ===============");
    CAMX_LOG_VERBOSE(CamxLogGroupCore, "<Setting> (<Hash>) = <Value>");

    m_pOverrideSettingsCache->Foreach(DumpData, FALSE);

    CAMX_LOG_VERBOSE(CamxLogGroupCore, "================ END DUMP OF OVERRIDE SETTINGS ================");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Protected Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OverrideSettingsFile::OverrideSettingsFile
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OverrideSettingsFile::OverrideSettingsFile()
    : m_pOverrideSettingsCache(NULL)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OverrideSettingsFile::~OverrideSettingsFile
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OverrideSettingsFile::~OverrideSettingsFile()
{
    // Free data from all the nodes
    m_pOverrideSettingsCache->Foreach(FreeData, TRUE);
    m_pOverrideSettingsCache->Destroy();
    m_pOverrideSettingsCache = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OverrideSettingsFile::DumpData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OverrideSettingsFile::DumpData(
    VOID* pData)
{
    if ((NULL != pData) && (NULL != *static_cast<VOID**>(pData)))
    {
        SettingCacheEntry* pSettingCacheEntry = *(reinterpret_cast<SettingCacheEntry**>(pData));

        CAMX_LOG_VERBOSE(CamxLogGroupCore,
                         "%s (0x%x) = %s",
                         pSettingCacheEntry->keyString,
                         pSettingCacheEntry->settingStringHash,
                         pSettingCacheEntry->valueString);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OverrideSettingsFile::SettingPropertyCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OverrideSettingsFile::SettingPropertyCallback(
    const CHAR* pKey,
    const CHAR* pVal,
    VOID* pData)
{
    /// @todo (CAMX-2089) - Refactor this and move it to a new file/class.
    static const CHAR DebugPrefix[] = "vendor.debug.camera.";
    static const CHAR PersistPrefix[] = "persist.vendor.camera.";

    // pVal[0] is checking to make sure the prop actually has a value assigned to it.
    if ((NULL != pKey) && (NULL != pVal) && (pData != NULL) && (pVal[0] != '\0'))
    {
        if ((0 == OsUtils::StrNICmp(pKey, DebugPrefix,   sizeof(DebugPrefix) - 1) ) ||
            (0 == OsUtils::StrNICmp(pKey, PersistPrefix, sizeof(PersistPrefix)- 1) ))
        {
            UINT32  settingStringHash = 0;

            // pData always maps to OverrideSettingsFile since the caller is responsible for passing
            // the function callback as well as the data.
            OverrideSettingsFile* pSettingsFile = reinterpret_cast<OverrideSettingsFile*>(pData);

            // Hash the entire property key since that's what the g_settings use.
            settingStringHash = GetSettingsStringHashValue(pKey);

            // Check if there is an existing entry. If not, create a new one. If so, update the value.
            SettingCacheEntry* pSettingCacheEntry = pSettingsFile->FindOverrideSetting(settingStringHash);
            if (NULL == pSettingCacheEntry)
            {
                // No existing entry, add a key/value entry to the override settings cache
                pSettingCacheEntry = static_cast<SettingCacheEntry*>(CAMX_CALLOC(sizeof(SettingCacheEntry)));
                if (NULL != pSettingCacheEntry)
                {
                    // Populate override setting entry data (value string is updated below)
                    pSettingCacheEntry->settingStringHash = settingStringHash;

                    OsUtils::StrLCpy(pSettingCacheEntry->keyString,
                                     pKey,
                                     sizeof(pSettingCacheEntry->keyString));

                    // Add the new override setting entry to override settings cache with the string has as the key
                    pSettingsFile->AddHash(settingStringHash, pSettingCacheEntry);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory; cannot allocate override setting entry.");
                }

            }

            // Set/overwrite value of setting
            OsUtils::StrLCpy(pSettingCacheEntry->valueString,
                             pVal,
                             sizeof(pSettingCacheEntry->valueString));
        }

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OverrideSettingsFile::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OverrideSettingsFile::Initialize()
{
    CamxResult result = CamxResultSuccess;

    // Create the hash map to hold the override settings. Key is the settings string hash and value is a pointer to a
    // SettingCacheEntry structure.
    HashmapParams hashmapParams = {0};
    hashmapParams.keySize       = sizeof(UINT32);
    hashmapParams.valSize       = 0;
    m_pOverrideSettingsCache    = Hashmap::Create(&hashmapParams);
    if (NULL == m_pOverrideSettingsCache)
    {
        result = CamxResultENoMemory;
    }

    if (CamxResultSuccess == result)
    {
        // Get the properties set on the device.
        UpdatePropertyList();

        // Since scratchString is used below to get the raw line from the override text file, the 128 should be way more than
        // enough for the max length of whatever non-value stuff is specified on the override line (i.e. variable name, space,
        // equals sign space, etc). Then, MaxStringLength is the max length string that a setting can have.
        CHAR    scratchString[MaxStringLength + 128]    = {0};
        FILE*   pOverrideSettingsTextFile               = NULL;

        // Search the paths to find the files
        for (UINT directory = 0; directory < CAMX_ARRAY_SIZE(OverrideSettingsTextFileDirectories); directory++)
        {
            {
                OsUtils::SNPrintF(scratchString,
                                  sizeof(scratchString),
                                  "%s%s%s",
                                  OverrideSettingsTextFileDirectories[directory],
                                  PathSeparator,
                                  OverrideSettingsTextFileName);
                pOverrideSettingsTextFile = OsUtils::FOpen(scratchString, "r");
                if (NULL == pOverrideSettingsTextFile)
                {
                    // We didn't find an override settings text file, try another path
                    CAMX_LOG_VERBOSE(CamxLogGroupCore, "Could not find override settings text file at: %s", scratchString);
                }
                else
                {
                    // We found an override settings text file.
                    CAMX_LOG_INFO(CamxLogGroupCore, "Opening override settings text file: %s", scratchString);

                    CHAR*   pSettingString      = NULL;
                    CHAR*   pValueString        = NULL;
                    CHAR*   pContext            = NULL;
                    UINT32  settingStringHash   = 0;
                    CHAR    strippedLine[MaxStringLength + 128];

                    // Parse the settings file one line at a time
                    while (NULL != OsUtils::FGetS(scratchString, sizeof(scratchString), pOverrideSettingsTextFile))
                    {
                        // First strip off all whitespace from the line to make it easier to handle enum type settings with
                        // combined values (e.g. A = B | C | D). After removing the whitespace, we only need to use '=' as the
                        // delimiter to extract the setting/value string pair (e.g. setting string = "A", value string =
                        // "B|C|D").
                        Utils::Memset(strippedLine, 0x0, sizeof(strippedLine));
                        OsUtils::StrStrip(strippedLine, scratchString, sizeof(strippedLine));

                        // Extract a setting/value string pair.
                        pSettingString  = OsUtils::StrTokReentrant(strippedLine, "=", &pContext);
                        pValueString    = OsUtils::StrTokReentrant(NULL,         "=", &pContext);

                        // Check for invalid lines
                        if ((NULL == pSettingString) || (NULL == pValueString) || ('\0' == pValueString[0]))
                        {
                            continue;
                        }

                        // Discard this line if the setting string starts with a semicolon, indicating a comment
                        if (';' == pSettingString[0])
                        {
                            continue;
                        }

                        // Check whether the setting string is either an obfuscated hash or a human-readable setting name
                        if (('0' == pSettingString[0]) &&
                            (('x' == pSettingString[1]) || ('X' == pSettingString[1])))
                        {
                            // Setting string is a hex value, indicating it is a hash
                            settingStringHash = static_cast<UINT32>(OsUtils::StrToUL(pSettingString, NULL, 0));
                        }
                        else
                        {
                            // Setting string is a non-hex value, so get the hash
                            settingStringHash = GetSettingsStringHashValue(pSettingString);
                        }

                        // Check if there is an existing entry. If not, create a new one. If so, update the value.
                        SettingCacheEntry* pSettingCacheEntry = FindOverrideSetting(settingStringHash);
                        if (NULL == pSettingCacheEntry)
                        {
                            // No existing entry, add a key/value entry to the override settings cache
                            pSettingCacheEntry = static_cast<SettingCacheEntry*>(CAMX_CALLOC(sizeof(SettingCacheEntry)));
                            if (NULL == pSettingCacheEntry)
                            {
                                CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory; cannot allocate override setting entry");
                                result = CamxResultENoMemory;
                                break;
                            }

                            // Populate override setting entry data (value string is updated below)
                            pSettingCacheEntry->settingStringHash = settingStringHash;

                            OsUtils::StrLCpy(pSettingCacheEntry->keyString,
                                             pSettingString,
                                             sizeof(pSettingCacheEntry->keyString));

                            // Add the new override setting entry to override settings cache with the string has as the key
                            m_pOverrideSettingsCache->Put(&settingStringHash, pSettingCacheEntry);
                        }

                        // Set/overwrite value of setting
                        OsUtils::StrLCpy(pSettingCacheEntry->valueString,
                                         pValueString,
                                         sizeof(pSettingCacheEntry->valueString));
                    }

                    OsUtils::FClose(pOverrideSettingsTextFile);
                    pOverrideSettingsTextFile = NULL;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OverrideSettingsFile::GetSettingsStringHashValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 OverrideSettingsFile::GetSettingsStringHashValue(
    const CHAR* pSettingsString)
{
    UINT   character  = 0;
    UINT32 highOrder  = 0;
    UINT32 hash       = 0;

    if (NULL != pSettingsString)
    {
        while ('\0' != pSettingsString[character])
        {
            highOrder = hash & 0xF8000000;
            hash      = hash << 5;
            hash      = hash ^ ((highOrder >> 27) & 0x1f);      // Mask is redundant but keeping consistent with perl
            hash      = hash ^ OsUtils::ToLower(pSettingsString[character]);
            character++;
        }
    }

    return hash;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OverrideSettingsFile::ParseSettingInt
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OverrideSettingsFile::ParseSettingInt(
    CHAR* pSettingStringVal,
    INT*  pVal)
{
    CamxResult result = CamxResultEFailed;
    if ('\0' != pSettingStringVal[0])
    {
        *pVal = OsUtils::StrToL(pSettingStringVal, NULL, 0);
        result = CamxResultSuccess;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OverrideSettingsFile::ParseSettingUint
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OverrideSettingsFile::ParseSettingUint(
    CHAR* pSettingStringVal,
    UINT* pVal)
{
    CamxResult result = CamxResultEFailed;

    if ('\0' != pSettingStringVal[0])
    {
        *pVal = OsUtils::StrToUL(pSettingStringVal, NULL, 0);
        result = CamxResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OverrideSettingsFile::ParseSettingBool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OverrideSettingsFile::ParseSettingBool(
    CHAR* pSettingStringVal,
    BOOL* pVal)
{
    CamxResult result = CamxResultSuccess;

    if ('\0' != pSettingStringVal[0])
    {
        // BOOLs can be expressed in several ways:
        if ('0' == pSettingStringVal[0])          // 0
        {
            *pVal = FALSE;
        }
        else if ('1' == pSettingStringVal[0])     // 1
        {
            *pVal = TRUE;
        }
        else if (('F' == pSettingStringVal[0]) || // FALSE, False, false, F, f
                 ('f' == pSettingStringVal[0]))
        {
            *pVal = FALSE;
        }
        else if (('T' == pSettingStringVal[0]) || // TRUE, True, true, T, t
                 ('t' == pSettingStringVal[0]))
        {
            *pVal = TRUE;
        }
        else
        {
            result = CamxResultEFailed;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OverrideSettingsFile::ParseSettingFloat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OverrideSettingsFile::ParseSettingFloat(
    CHAR*  pSettingStringVal,
    FLOAT* pVal)
{
    CamxResult result = CamxResultEFailed;

    if ('\0' != pSettingStringVal[0])
    {
        *pVal = static_cast<FLOAT>(OsUtils::StrToD(pSettingStringVal, NULL));
        result = CamxResultSuccess;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OverrideSettingsFile::ParseSettingEnum
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OverrideSettingsFile::ParseSettingEnum(
    CHAR*                pSettingStringVal,
    INT*                 pVal,
    EnumeratorToHashMap* pEnumeratorToHashMap,
    UINT                 enumeratorToHashMapSize)
{
    CamxResult          result = CamxResultSuccess;
    INT                 enumeratorValue = 0;

    // The user may combine multiple enumerators by specifying the OR (|) operator. Find each enumerator and combine their
    // values. Note that the value string has been stripped of all whitespace. The string is invalid if it is empty or
    // missing an operand for any of the OR operators.
    if (('\0' != pSettingStringVal[0])                                      &&
        ('|'  != pSettingStringVal[0])                                      &&
        (0     < OsUtils::StrLen(pSettingStringVal))                        &&
        ('|'  != pSettingStringVal[OsUtils::StrLen(pSettingStringVal) - 1]) &&
        (NULL == OsUtils::StrStr(pSettingStringVal, "||")))
    {
        // Extract the first enumerator separated by an OR operator.
        CHAR* pContext;
        CHAR* pEnumeratorString = OsUtils::StrTokReentrant(pSettingStringVal, "|", &pContext);
        while ((NULL != pEnumeratorString) && ('\0' != pEnumeratorString[0]))
        {
            UINT    index = 0;
            UINT32  enumeratorStringHash = 0;
            BOOL    enumeratorIsValid = FALSE;

            // An enumerator can be expressed in three ways:
            //  1) A hashed enumerator string
            //  2) A decimal UINT
            //  3) A human-readable enumerator string
            if (('0' == pEnumeratorString[0]) && (('x' == pEnumeratorString[1]) || ('X' == pEnumeratorString[1])))
            {
                // We found a hex value so try to find a hash value that matches
                enumeratorStringHash = static_cast<UINT32>(OsUtils::StrToUL(pEnumeratorString, NULL, 0));

                while (index < enumeratorToHashMapSize)
                {
                    if (pEnumeratorToHashMap[index].enumeratorStringHash == enumeratorStringHash)
                    {
                        // We found a match
                        enumeratorValue |= pEnumeratorToHashMap[index].enumerator;
                        enumeratorIsValid = TRUE;
                        break;
                    }

                    index++;
                }
            }
            else if ((pEnumeratorString[0] >= '0') && (pEnumeratorString[0] <= '9'))
            {
                // We found a numeric value so try to find an enumerator value that matches
                INT enumerator = static_cast<INT>(OsUtils::StrToUL(pEnumeratorString, NULL, 0));

                while (index < enumeratorToHashMapSize)
                {
                    if (pEnumeratorToHashMap[index].enumerator == enumerator)
                    {
                        // We found a match
                        enumeratorValue |= enumerator;
                        enumeratorIsValid = TRUE;
                        break;
                    }

                    index++;
                }
            }
            else
            {
                // We found a non-numeric value so hash it and try to find a hash value that matches
                enumeratorStringHash = GetSettingsStringHashValue(pEnumeratorString);

                while (index < enumeratorToHashMapSize)
                {
                    if (pEnumeratorToHashMap[index].enumeratorStringHash == enumeratorStringHash)
                    {
                        // We found a match
                        enumeratorValue |= pEnumeratorToHashMap[index].enumerator;
                        enumeratorIsValid = TRUE;
                        break;
                    }

                    index++;
                }
            }

            if (enumeratorIsValid == FALSE)
            {
                // Stop if current enumerator is invalid
                CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid enumerator: %s", pSettingStringVal);
                result = CamxResultEFailed;
                break;
            }
            else
            {
                result = CamxResultSuccess;
            }

            // Get next enumerator string separated by OR operator
            pEnumeratorString = OsUtils::StrTokReentrant(NULL, "|", &pContext);
        }
    }

    if (CamxResultSuccess == result)
    {
        *pVal = enumeratorValue;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OverrideSettingsFile::ParseSettingString
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OverrideSettingsFile::ParseSettingString(
    CHAR*  pSettingStringVal,
    CHAR*  pVal,
    SIZE_T stringLength)
{
    CamxResult result = CamxResultSuccess;

    OsUtils::StrLCpy(pVal, pSettingStringVal, stringLength);

    return result;
}

CAMX_NAMESPACE_END
