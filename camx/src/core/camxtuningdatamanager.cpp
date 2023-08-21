////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxtuningdatamanager.cpp
/// @brief Implements TuningDataManager methods. This will interact with auto generated code
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxhal3defs.h"
#include "camxincs.h"
#include "camxmem.h"
#include "camxtuningdatamanager.h"

CAMX_NAMESPACE_BEGIN

/// Constants
static const UINT MaxTunableModules         = 128;  ///< Max number of modules that can be tuned

/// @brief Describes a node in the mode tree of a tuned module
struct TunedNode
{
    ParameterModule* pTunedModule;              ///< Autogen ParameterModule representing a node in a mode tree
    UINT             numTunedModes;             ///< Number of modes that this node in the mode tree represents
    TuningMode       tunedModes[MaxTunedModes]; ///< Set of override modes that this node in the mode tree represents
};

/// @brief Describes a particular tuned module and represents its mode tree
struct TunedModule
{
    const CHAR*      pModuleName;               ///< Tuned module name
    UINT             numTunedModes;             ///< Number of tuned modes for this module
    TunedNode        tunedNode[MaxTunedNodes];  ///< Tuned data for all override modes of this module
    ParameterModule* pDefaultModule;            ///< Cache away the default mode
};

/// @brief Describes the mode trees for all tuned modules for a particular sensor
struct TunedModulesInfo
{
    TuningSetManager*   pTuningSetManager;  ///< Pointer to autogen code TuningSetManager

    BYTE*               pTunedDataBuf;      ///< Pointer to memory holding sensor specific tuned binary data
    UINT64              tunedBufLength;     ///< Length of sensor specific tuned binary data
    UINT                numTunedModules;    ///< Number of tuned modules for this sensor
    TunedModule*        pTunedModulesList;  ///< Array of tuned modules for this sensor
};

/// @brief Describes a node match in a mode tree
struct MatchParams
{
    UINT             matchCount;        ///< Number of mode specifiers that is a match at this node
    UINT             precedence;        ///< Total value of the precedence at this node, in case of a tie
    ParameterModule* pMatchedModule;    ///< The autogen ParameterModule for the potential matched node
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TuningDataManager::LoadFile
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TuningDataManager::LoadFile(
    const CHAR* pFilename,
    BYTE**      ppBuffer,
    UINT64*     pBufferLength)
{
    CamxResult  result       = CamxResultEFailed;
    FILE*       phFileHandle = NULL;

    phFileHandle = OsUtils::FOpen(pFilename, "rb");

    if (NULL != phFileHandle)
    {
        UINT64 fileSize = OsUtils::GetFileSize(pFilename);
        CAMX_ASSERT(0 != fileSize);

        *ppBuffer = static_cast<BYTE*>(CAMX_CALLOC(static_cast<SIZE_T>(fileSize)));
        if (NULL != *ppBuffer)
        {
            UINT64 sizeRead = OsUtils::FRead(*ppBuffer,
                                             static_cast<SIZE_T>(fileSize),
                                             1,
                                             static_cast<SIZE_T>(fileSize),
                                             phFileHandle);
            if (fileSize == sizeRead)
            {
                *pBufferLength = fileSize;
                result = CamxResultSuccess;
            }
        }

        OsUtils::FClose(phFileHandle);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TuningDataManager::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TuningDataManager* TuningDataManager::Create()
{
    CamxResult          result          = CamxResultEFailed;
    TuningDataManager*  pLocalInstance  = NULL;

    pLocalInstance = CAMX_NEW TuningDataManager();
    if (NULL != pLocalInstance)
    {
        result = pLocalInstance->Initialize();
        if (CamxResultSuccess != result)
        {
            CAMX_DELETE pLocalInstance;
            pLocalInstance = NULL;
        }
    }

    return pLocalInstance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TuningDataManager::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TuningDataManager::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TuningDataManager::TuningDataManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TuningDataManager::TuningDataManager()
    : m_pTunedModulesInfo(NULL)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TuningDataManager::~TuningDataManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TuningDataManager::~TuningDataManager()
{
    if (NULL != m_pTunedModulesInfo)
    {
        if (NULL != m_pTunedModulesInfo->pTuningSetManager)
        {
            CAMX_DELETE m_pTunedModulesInfo->pTuningSetManager;
            m_pTunedModulesInfo->pTuningSetManager = NULL;
        }

        if (NULL != m_pTunedModulesInfo->pTunedModulesList)
        {
            CAMX_FREE(m_pTunedModulesInfo->pTunedModulesList);
            m_pTunedModulesInfo->pTunedModulesList = NULL;
        }

        CAMX_FREE(m_pTunedModulesInfo);
        m_pTunedModulesInfo = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TuningDataManager::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TuningDataManager::Initialize()
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL == m_pTunedModulesInfo);

    if (NULL == m_pTunedModulesInfo)
    {
        TunedModulesInfo* pTunedModuleInfo = static_cast<TunedModulesInfo*>(CAMX_CALLOC(sizeof(TunedModulesInfo)));

        if (NULL != pTunedModuleInfo)
        {
            pTunedModuleInfo->pTuningSetManager  = NULL;
            pTunedModuleInfo->numTunedModules    = 0;
            pTunedModuleInfo->pTunedModulesList  =
                static_cast<TunedModule*>(CAMX_CALLOC(MaxTunableModules * sizeof(TunedModule)));

            if (NULL != pTunedModuleInfo->pTunedModulesList)
            {
                TunedModule* pModuleList = pTunedModuleInfo->pTunedModulesList;

                for (UINT moduleIndex = 0; moduleIndex < MaxTunableModules; moduleIndex++)
                {
                    TunedModule* pModule = &pModuleList[moduleIndex];

                    pModule->numTunedModes  = 0;
                    pModule->pDefaultModule = NULL;

                    for (UINT modeIndex = 0; modeIndex < MaxTunedNodes; modeIndex++)
                    {
                        pModule->tunedNode[modeIndex].pTunedModule  = NULL;
                        pModule->tunedNode[modeIndex].numTunedModes = 0;
                    }
                }

                m_pTunedModulesInfo = pTunedModuleInfo;
            }
            else
            {
                result = CamxResultENoMemory;
            }
        }
        else
        {
            result = CamxResultENoMemory;
        }
    }
    else
    {
        result = CamxResultEInvalidState;
    }

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCore, "TuningDataManager initialized successfully");
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Couldn't initialize TuningDataManager!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TuningDataManager::LoadTunedDataFile
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TuningDataManager::LoadTunedDataFile(
    const CHAR* pFilename)
{
    CamxResult result = CamxResultEFailed;

    result = LoadFile(pFilename, &m_pTunedModulesInfo->pTunedDataBuf, &m_pTunedModulesInfo->tunedBufLength);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TuningDataManager::IsValidChromatix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL TuningDataManager::IsValidChromatix()
{
    if ((NULL != m_pTunedModulesInfo) && (NULL != m_pTunedModulesInfo->pTuningSetManager))
    {
        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TuningDataManager::GetChromatix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TuningSetManager* TuningDataManager::GetChromatix()
{
    if ((NULL != m_pTunedModulesInfo) && (NULL != m_pTunedModulesInfo->pTuningSetManager))
    {
        return m_pTunedModulesInfo->pTuningSetManager;
    }
    return NULL;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TuningDataManager::CreateTunedModeTree
// Autogen code is not fully CamX compliant. Autogen code signature in this function, with lack of compliance, is to be ignored
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TuningDataManager::CreateTunedModeTree()
{
    CamxResult result = CamxResultEFailed;

    CAMX_ASSERT(NULL != m_pTunedModulesInfo);

    if (NULL != m_pTunedModulesInfo)
    {
        m_pTunedModulesInfo->pTuningSetManager = CAMX_NEW TuningSetManager;

        if ((NULL != m_pTunedModulesInfo->pTuningSetManager) &&
            (NULL != m_pTunedModulesInfo->pTunedDataBuf)     &&
            (0    != m_pTunedModulesInfo->tunedBufLength))
        {
            TuningSetManager* pManager         = m_pTunedModulesInfo->pTuningSetManager;

            // 2nd parameter is down cast temporarily, pending fix in autogen code
            if (TRUE == pManager->LoadBinaryParameters(m_pTunedModulesInfo->pTunedDataBuf, m_pTunedModulesInfo->tunedBufLength))
            {
                result = CamxResultSuccess;
            }
        }

        CAMX_FREE(m_pTunedModulesInfo->pTunedDataBuf);
        m_pTunedModulesInfo->pTunedDataBuf = NULL;
    }

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCore, "Tuning Mode Tree created successfully");
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Couldn't create Tuning Mode Tree!");
    }

    return result;
}

CAMX_NAMESPACE_END
