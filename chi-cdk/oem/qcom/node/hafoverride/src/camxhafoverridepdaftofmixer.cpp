////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxhafoverride.h"

struct MixerInternalData
{
    CHIHAFAlgorithm         algoOps;
    CHIHAFAlgorithm*        pTOFAlgo;
    CHIHAFAlgorithm*        pPDAFAlgo;
};

CDKResult MixerSetParam(
    CHIHAFAlgorithm* pHAFAlgo,
    HAFAlgoSetParamList* pSetParams)
{
    CDKResult result = CDKResultSuccess;
    MixerInternalData* pMixerData = reinterpret_cast<MixerInternalData*>(pHAFAlgo);
    if (NULL == pMixerData || NULL == pSetParams)
    {
        HAF_MSG_ERROR("Error, NULL pointer");
        return CDKResultEFailed;
    }

    if (NULL == pMixerData->pTOFAlgo || NULL == pMixerData->pPDAFAlgo)
    {
        HAF_MSG_ERROR("Sub algo is NULL");
        return CDKResultEFailed;
    }

    for (INT32 index = 0; index < (INT32)pSetParams->numberOfParams; index++)
    {
        HAFAlgoSetParam* pSetParam = &pSetParams->pParamList[index];

        if (NULL == pSetParam || NULL == pSetParam->pData)
        {
            HAF_MSG_ERROR("Error, NULL pointer");
            continue;
        }

        // put customized logic here to receive necessary info
        switch (pSetParam->setParamType)
        {
        case HAFSetParamTypeAECInfo:
        default:
            break;
        }
    }

    result = pMixerData->pTOFAlgo->HAFAlgorithmSetParam(pMixerData->pTOFAlgo, pSetParams);
    if (CDKResultSuccess != result)
    {
        HAF_MSG_ERROR("TOF Set Param failed");
        return result;
    }

    result = pMixerData->pPDAFAlgo->HAFAlgorithmSetParam(pMixerData->pPDAFAlgo, pSetParams);
    if (CDKResultSuccess != result)
    {
        HAF_MSG_ERROR("PDAF Set Param  failed");
        return result;
    }

    return result;
}

CDKResult MixerGetParam(
    CHIHAFAlgorithm* pHAFAlgo,
    HAFAlgoGetParam* pGetParam)
{
    CDKResult result = CDKResultSuccess;
    MixerInternalData* pMixerData = reinterpret_cast<MixerInternalData*>(pHAFAlgo);

    if (NULL == pMixerData || NULL == pGetParam)
    {
        HAF_MSG_ERROR("Error, NULL pointer");
        return CDKResultEFailed;
    }

    if (NULL == pMixerData->pTOFAlgo || NULL == pMixerData->pPDAFAlgo)
    {
        HAF_MSG_ERROR("Sub algo is NULL");
        return CDKResultEFailed;
    }

    result = pMixerData->pTOFAlgo->HAFAlgorithmGetParam(pMixerData->pTOFAlgo, pGetParam);
    if (CDKResultSuccess != result)
    {
        HAF_MSG_ERROR("TOF Get Param failed");
        return result;
    }

    result = pMixerData->pPDAFAlgo->HAFAlgorithmGetParam(pMixerData->pPDAFAlgo, pGetParam);
    if (CDKResultSuccess != result)
    {
        HAF_MSG_ERROR("PDAF Get Param  failed");
        return result;
    }

    return result;
}

CDKResult MixerProcessMonitor(
    CHIHAFAlgorithm*                pHAFAlgo,
    HAFAlgoMonitorInputParamList*   pMonitorInputParamList,
    HAFAlgoMonitorOutputParam*      pMonitorOutputParam)
{
    CDKResult result = CDKResultSuccess;
    MixerInternalData* pMixerData = reinterpret_cast<MixerInternalData*>(pHAFAlgo);
    HAFAlgoMonitorOutputParam tofMonitorOutput;
    HAFAlgoMonitorOutputParam pdafMonitorOutput;

    if (NULL == pMixerData || NULL == pMonitorInputParamList || NULL == pMonitorOutputParam)
    {
        HAF_MSG_ERROR("Error, NULL pointer");
        return CDKResultEFailed;
    }

    if (NULL == pMixerData->pTOFAlgo || NULL == pMixerData->pPDAFAlgo)
    {
        HAF_MSG_ERROR("Sub algo is NULL");
        return CDKResultEFailed;
    }

    result = pMixerData->pTOFAlgo->HAFAlgorithmProcessMonitor(
        pMixerData->pTOFAlgo, pMonitorInputParamList, &tofMonitorOutput);
    if (CDKResultSuccess != result)
    {
        HAF_MSG_ERROR("TOF process monitor failed");
        return result;
    }

    result = pMixerData->pPDAFAlgo->HAFAlgorithmProcessMonitor(
        pMixerData->pPDAFAlgo, pMonitorInputParamList, &pdafMonitorOutput);
    if (CDKResultSuccess != result)
    {
        HAF_MSG_ERROR("PDAF process monitor failed");
        return result;
    }

    // Put customized logic here to use which output,
    // the example code will use PDAF output if pdaf is confident, then use tof is tof is confident
    // customer can put more heuristic logic here
    if (pdafMonitorOutput.confidence >= HAFConfidenceLevelHigh)
    {
        *pMonitorOutputParam = pdafMonitorOutput;
    }
    else if (tofMonitorOutput.confidence >= HAFConfidenceLevelHigh)
    {
        *pMonitorOutputParam = tofMonitorOutput;
    }
    else
    {
        *pMonitorOutputParam = pdafMonitorOutput;
    }

    return result;
}

CDKResult MixerProcessSearch(
    CHIHAFAlgorithm*                pHAFAlgo,
    HAFAlgoSearchInputParamList*    pSearchInputParamList,
    HAFAlgoSearchOutputParam*       pSearchOutputParam)
{
    CDKResult result = CDKResultEFailed;
    MixerInternalData* pMixerData = reinterpret_cast<MixerInternalData*>(pHAFAlgo);
    HAFAlgoSearchOutputParam tofSearchOutput;
    HAFAlgoSearchOutputParam pdafSearchOutput;

    if (NULL == pMixerData || NULL == pSearchInputParamList || NULL == pSearchOutputParam)
    {
        HAF_MSG_ERROR("Error, NULL pointer");
        return CDKResultEFailed;
    }

    if (NULL == pMixerData->pTOFAlgo || NULL == pMixerData->pPDAFAlgo)
    {
        HAF_MSG_ERROR("Sub algo is NULL");
        return CDKResultEFailed;
    }

    result = pMixerData->pTOFAlgo->HAFAlgorithmProcessSearch(
        pMixerData->pTOFAlgo, pSearchInputParamList, &tofSearchOutput);
    if (CDKResultSuccess != result)
    {
        HAF_MSG_ERROR("TOF process monitor failed");
        return result;
    }

    result = pMixerData->pPDAFAlgo->HAFAlgorithmProcessSearch(
        pMixerData->pPDAFAlgo, pSearchInputParamList, &pdafSearchOutput);
    if (CDKResultSuccess != result)
    {
        HAF_MSG_ERROR("PDAF process monitor failed");
        return result;
    }

    // Put customized logic here to use which output,
    // the example code will use TOF output if TOF is confident, then use PDAF is PDAF is confident
    // customer can put more heuristic logic here
    if (tofSearchOutput.confidence >= HAFConfidenceLevelHigh)
    {
        *pSearchOutputParam = tofSearchOutput;
    }
    else
    {
        *pSearchOutputParam = pdafSearchOutput;
    }

    return result;
}

CDKResult MixerRebaseReference(
    CHIHAFAlgorithm* pHAFAlgo)
{
    CDKResult result = CDKResultSuccess;
    MixerInternalData* pMixerData = reinterpret_cast<MixerInternalData*>(pHAFAlgo);

    if (NULL == pMixerData)
    {
        HAF_MSG_ERROR("Error, NULL pointer");
        return result;
    }

    if (NULL == pMixerData->pTOFAlgo || NULL == pMixerData->pPDAFAlgo)
    {
        HAF_MSG_ERROR("Sub algo is NULL");
        return CDKResultEFailed;
    }


    result = pMixerData->pTOFAlgo->HAFAlgorithmClearData(pMixerData->pTOFAlgo);
    if (CDKResultSuccess != result)
    {
        HAF_MSG_ERROR("TOF rebase reference failed");
        return result;
    }

    pMixerData->pPDAFAlgo->HAFAlgorithmClearData(pMixerData->pPDAFAlgo);
    if (CDKResultSuccess != result)
    {
        HAF_MSG_ERROR("PDAF rebase reference failed");
        return result;
    }

    return result;
}

CDKResult MixerClearData(
    CHIHAFAlgorithm* pHAFAlgo)
{
    CDKResult result = CDKResultSuccess;
    MixerInternalData* pMixerData = reinterpret_cast<MixerInternalData*>(pHAFAlgo);

    if (NULL == pMixerData)
    {
        HAF_MSG_ERROR("Error, NULL pointer");
        return CDKResultEFailed;
    }

    if (NULL == pMixerData->pTOFAlgo || NULL == pMixerData->pPDAFAlgo)
    {
        HAF_MSG_ERROR("Sub algo is NULL");
        return CDKResultEFailed;
    }

    result = pMixerData->pTOFAlgo->HAFAlgorithmClearData(pMixerData->pTOFAlgo);
    if (CDKResultSuccess != result)
    {
        HAF_MSG_ERROR("TOF Clear data failed");
        return result;
    }

    result = pMixerData->pPDAFAlgo->HAFAlgorithmClearData(pMixerData->pPDAFAlgo);
    if (CDKResultSuccess != result)
    {
        HAF_MSG_ERROR("PDAF Clear data failed");
        return result;
    }

    return result;
}

VOID MixerDestroy(
    CHIHAFAlgorithm* pHAFAlgo)
{
    MixerInternalData* pMixerData = reinterpret_cast<MixerInternalData*>(pHAFAlgo);

    if (pMixerData)
    {
        if (pMixerData->pTOFAlgo)
        {
            pMixerData->pTOFAlgo->HAFAlgorithmDestroy(pMixerData->pTOFAlgo);
        }

        if (pMixerData->pPDAFAlgo)
        {
            pMixerData->pPDAFAlgo->HAFAlgorithmDestroy(pMixerData->pPDAFAlgo);
        }

        pMixerData->pTOFAlgo = NULL;
        pMixerData->pPDAFAlgo = NULL;
        CAMX_DELETE pMixerData;
    }
}

CDKResult CreateHAFOverridePDAFTOFMixer(
    const AFAlgoCreateParamList*    pCreateParams,
    CHIHAFAlgorithm**               ppAlgoHandle)
{
    CDKResult result                        = CDKResultSuccess;
    VOID* pParam                            = NULL;
    CREATEHAFALGORITHM CreateHAFAlgorithm   = NULL;

    MixerInternalData* pMixerData = CAMX_NEW MixerInternalData;
    if (NULL == pMixerData)
    {
        *ppAlgoHandle = NULL;
        return CDKResultENoMemory;
    }

    pParam = GetParamFromCreateList(pCreateParams, AFCreateParamTypeCreateQtiHAFFunctionPtr);
    if (NULL == pParam)
    {
        HAF_MSG_ERROR("Invalid input");
        goto err;
    }
    CreateHAFAlgorithm = reinterpret_cast<CREATEHAFALGORITHM>(pParam);

    result = CreateHAFAlgorithm(pCreateParams, HAFAlgoTypeTOF, &pMixerData->pTOFAlgo);
    if (CDKResultSuccess != result || NULL == pMixerData->pTOFAlgo)
    {
        HAF_MSG_ERROR("Create TOF algorithm failed");
        goto err;
    }

    result = CreateHAFAlgorithm(pCreateParams, HAFAlgoTypePDAF, &pMixerData->pPDAFAlgo);
    if (CDKResultSuccess != result || NULL == pMixerData->pPDAFAlgo)
    {
        HAF_MSG_ERROR("Create PDAF algorithm failed");
        goto err;
    }

    pMixerData->algoOps.HAFAlgorithmSetParam         = MixerSetParam;
    pMixerData->algoOps.HAFAlgorithmGetParam         = MixerGetParam;
    pMixerData->algoOps.HAFAlgorithmProcessMonitor   = MixerProcessMonitor;
    pMixerData->algoOps.HAFAlgorithmProcessSearch    = MixerProcessSearch;
    pMixerData->algoOps.HAFAlgorithmClearData        = MixerClearData;
    pMixerData->algoOps.HAFAlgorithmRebaseReference  = MixerRebaseReference;
    pMixerData->algoOps.HAFAlgorithmDestroy          = MixerDestroy;

    *ppAlgoHandle = reinterpret_cast<CHIHAFAlgorithm*>(pMixerData);

    return result;

err:
    if (pMixerData)
    {
        if (pMixerData->pTOFAlgo)
        {
            pMixerData->pTOFAlgo->HAFAlgorithmDestroy(pMixerData->pTOFAlgo);
        }

        if (pMixerData->pPDAFAlgo)
        {
            pMixerData->pPDAFAlgo->HAFAlgorithmDestroy(pMixerData->pPDAFAlgo);
        }
        CAMX_DELETE pMixerData;
    }

    *ppAlgoHandle = NULL;
    return result;
}
