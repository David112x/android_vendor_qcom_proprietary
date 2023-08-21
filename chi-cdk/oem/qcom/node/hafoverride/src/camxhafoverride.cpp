////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxhafcuscom.cpp
/// @brief The class that implements override HAF algorithm
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxhafoverride.h"

StatsLoggingFunction gStatsLogger;

CDKResult CreateHAFOverrideTOFDecorator(
    const AFAlgoCreateParamList*    pCreateParams,
    CHIHAFAlgorithm**               ppAlgoHandle);
CDKResult CreateHAFOverridePDAFAlgorithm(
    const AFAlgoCreateParamList*    pCreateParams,
    CHIHAFAlgorithm**               ppAlgoHandle);
CDKResult CreateHAFOverridePDAFTOFMixer(
    const AFAlgoCreateParamList*    pCreateParams,
    CHIHAFAlgorithm**               ppAlgoHandle);


VOID* GetParamFromCreateList(
    const AFAlgoCreateParamList* pCreateParams,
    AFAlgoCreateParamType paramType)
{
    VOID* pParam = NULL;

    if (NULL != pCreateParams &&
        NULL != pCreateParams->pParamList &&
        pCreateParams->paramCount > (UINT32) paramType)
    {
        pParam = pCreateParams->pParamList[paramType].pParam;
    }

    return pParam;
}

VOID* GetParamFromMonitorInputList(
    const HAFAlgoMonitorInputParamList*   pMonitorInputParamList,
    HAFAlgoMonitorInputParamType paramType)
{
    VOID* pData = NULL;

    if (NULL != pMonitorInputParamList &&
        NULL != pMonitorInputParamList->pParamList &&
        pMonitorInputParamList->numberOfInputParam > (UINT32) paramType)
    {
        pData = pMonitorInputParamList->pParamList[paramType].pData;
    }

    return pData;
}

VOID* GetParamFromSearchInputList(
    HAFAlgoSearchInputParamList*    pSearchInputParamList,
    HAFAlgoSearchInputParamType     paramType)
{
    VOID* pData = NULL;

    if (NULL != pSearchInputParamList &&
        NULL != pSearchInputParamList->pParamList &&
        pSearchInputParamList->paramCount > (UINT32) paramType)
    {
        pData = pSearchInputParamList->pParamList[paramType].pData;
    }

    return pData;
}

#ifdef __cplusplus
extern "C"
{
#endif

CAMX_VISIBILITY_PUBLIC CDKResult CreateHAFAlgorithm(
    const AFAlgoCreateParamList*    pCreateParams,
    HAFAlgoType                     algoType,
    CHIHAFAlgorithm**               ppAlgoHandle)
{
    CDKResult result                = CDKResultSuccess;
    AFAlgoSettingInfo* pSettingInfo = NULL;
    VOID* pParam                    = NULL;

    pParam = GetParamFromCreateList(pCreateParams, AFCreateParamTypeLogFunctionPtr);
    if (NULL == pParam)
    {
        return CDKResultEFailed;
    }
    gStatsLogger = reinterpret_cast<StatsLoggingFunction>(pParam);

    pParam = GetParamFromCreateList(pCreateParams, AFCreateParamTypeSettingsPtr);
    if (NULL == pParam)
    {
        HAF_MSG_ERROR( "Invalid input");
        return CDKResultEFailed;
    }
    pSettingInfo = reinterpret_cast<AFAlgoSettingInfo*>(pParam);

    HAF_MSG_INFO("Create HAF override algorithm: algoType = 0x%x, Mask = 0x%x",
        algoType, pSettingInfo->overrideHAFAlgoMask);
    UINT32 overrideAlgoEnable = pSettingInfo->overrideHAFAlgoMask & (1 << algoType);
    if (0 == overrideAlgoEnable)
    {
        *ppAlgoHandle = NULL;
        return result;
    }

    switch (algoType)
    {
    case HAFAlgoTypeTOF:
        result = CreateHAFOverrideTOFDecorator(pCreateParams, ppAlgoHandle);
        break;

    case HAFAlgoTypePDAF:
        result = CreateHAFOverridePDAFAlgorithm(pCreateParams, ppAlgoHandle);
        break;

    case HAFAlgoTypePDAFTOFMixer:
        result = CreateHAFOverridePDAFTOFMixer(pCreateParams, ppAlgoHandle);
        break;

    case HAFAlgoTypeCAF:
    case HAFAlgoTypeFOLLOWER:
    case HAFAlgoTypeMixer:
    case HAFAlgoTypeDepthAF:
    case HAFAlgoTypePDAFDepthAFMixer:
    default:
        *ppAlgoHandle = NULL;
    }

    return result;
}

#ifdef __cplusplus
}
#endif
