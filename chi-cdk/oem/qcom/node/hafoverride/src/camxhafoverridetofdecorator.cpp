////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxhafoverride.h"

struct TOFInternalData
{
    CHIHAFAlgorithm         algoOps;
    CHIHAFAlgorithm*        pQtiTOFAlgo;
    HAFAlgoUtilInterface*   pAlgoUtils;
    AFTOFData               tofStats;
    AFAECInfo               aecInfo;
    INT                     currentPosition;
};

CDKResult TOFSetParam(
    CHIHAFAlgorithm* pHAFAlgo,
    HAFAlgoSetParamList* pSetParams)
{
    CDKResult result = CDKResultSuccess;
    TOFInternalData* pTOFData = reinterpret_cast<TOFInternalData*>(pHAFAlgo);
    if (NULL == pTOFData || NULL == pSetParams)
    {
        HAF_MSG_ERROR("Error, NULL pointer");
        return CDKResultEFailed;
    }

    if (NULL == pTOFData->pQtiTOFAlgo)
    {
        HAF_MSG_ERROR("pQtiTOFAlgo is NULL");
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
        {
            AFAECInfo* pAECInfo = (AFAECInfo*)pSetParam->pData;
            pTOFData->aecInfo = *pAECInfo;
        }
        break;

        case HAFSetParamTypeTOFData:
        {
            AFTOFData* pData = (AFTOFData*)pSetParam->pData;
            pTOFData->tofStats = *pData;
        }
        break;

        default:
            break;
        }
    }

    result = pTOFData->pQtiTOFAlgo->HAFAlgorithmSetParam(pTOFData->pQtiTOFAlgo, pSetParams);

    return result;
}

CDKResult TOFGetParam(
    CHIHAFAlgorithm* pHAFAlgo,
    HAFAlgoGetParam* pGetParam)
{
    CDKResult result = CDKResultSuccess;
    TOFInternalData* pTOFData = reinterpret_cast<TOFInternalData*>(pHAFAlgo);
    if (NULL == pTOFData || NULL == pGetParam)
    {
        HAF_MSG_ERROR("Error, NULL pointer");
        return CDKResultEFailed;
    }

    result = pTOFData->pQtiTOFAlgo->HAFAlgorithmGetParam(pTOFData->pQtiTOFAlgo, pGetParam);

    return result;
}

CDKResult TOFProcessMonitor(
    CHIHAFAlgorithm*                pHAFAlgo,
    HAFAlgoMonitorInputParamList*   pMonitorInputParamList,
    HAFAlgoMonitorOutputParam*      pMonitorOutputParam)
{
    CDKResult result = CDKResultSuccess;
    TOFInternalData* pTOFData = reinterpret_cast<TOFInternalData*>(pHAFAlgo);

    if (NULL == pTOFData->pQtiTOFAlgo)
    {
        HAF_MSG_ERROR("pQtiTOFAlgo is NULL");
        return CDKResultEFailed;
    }

    result = pTOFData->pQtiTOFAlgo->HAFAlgorithmProcessMonitor(
        pTOFData->pQtiTOFAlgo, pMonitorInputParamList, pMonitorOutputParam);

    // Override Qti TOF algorithm monitor output here
    // the example code is if in bright light, if Qti TOF report trigger refocus, we can silent it
    if (pTOFData->aecInfo.luxIndex < 200.0f && TRUE == pMonitorOutputParam->triggerRefocus)
    {
        pMonitorOutputParam->triggerRefocus = FALSE;
        pMonitorOutputParam->confidence = HAFConfidenceLevelLow;
    }

    return result;
}

CDKResult TOFProcessSearch(
    CHIHAFAlgorithm*                pHAFAlgo,
    HAFAlgoSearchInputParamList*    pSearchInputParamList,
    HAFAlgoSearchOutputParam*       pSearchOutputParam)
{
    CDKResult result = CDKResultSuccess;
    TOFInternalData* pTOFData = reinterpret_cast<TOFInternalData*>(pHAFAlgo);

    if (NULL == pTOFData->pQtiTOFAlgo)
    {
        HAF_MSG_ERROR("pQtiTOFAlgo is NULL");
        return CDKResultEFailed;
    }

    VOID* pParam = GetParamFromSearchInputList(pSearchInputParamList, HAFSearchInputParamTypeCurrentPosition);
    if (NULL != pParam)
    {
        pTOFData->currentPosition = *reinterpret_cast<INT*>(pParam);
    }

    result = pTOFData->pQtiTOFAlgo->HAFAlgorithmProcessSearch(
        pTOFData->pQtiTOFAlgo, pSearchInputParamList, pSearchOutputParam);

    // Put customized code here to override Qti TOF algorithm search output
    // the example code is if measured confident is semi-confident, we still want to benifit from TOF sensor
    // since TOF will fallback, we can override this behavior
    if (pTOFData->tofStats.confidence == 2) //0-not confident, 1 - confident, 2 - semiconfent
    {
        INT distMM = pTOFData->tofStats.distanceInMilliMeters;
        INT targetPos = pTOFData->pAlgoUtils->DistanceToLensPosition(pTOFData->pAlgoUtils, distMM);
        INT defocus = targetPos - pTOFData->currentPosition;
        INT dof = 10;//pTOFData->pAlgoUtils->CalcDOF(pTOFData->pAlgoUtils);
        if (ABS(defocus) > 20)
        {
            pSearchOutputParam->nextPosition = pTOFData->currentPosition + INT(defocus * 0.8f);
            pSearchOutputParam->targetPosition = pTOFData->currentPosition + defocus;
            pSearchOutputParam->progress = HAFProgressLevel50;
            pSearchOutputParam->confidence = HAFConfidenceLevelMedium;
            pSearchOutputParam->rangeNear = pSearchOutputParam->targetPosition - 2 * dof;
            pSearchOutputParam->rangeFar = pSearchOutputParam->targetPosition + 2 * dof;
            pSearchOutputParam->fineStepSize = dof;
        }
        else
        {
            pSearchOutputParam->nextPosition = pTOFData->currentPosition;
            pSearchOutputParam->targetPosition = pTOFData->currentPosition + defocus;
            pSearchOutputParam->progress = HAFProgressLevel100;
            pSearchOutputParam->confidence = HAFConfidenceLevelMedium;
            pSearchOutputParam->rangeNear = pSearchOutputParam->targetPosition - 2 * dof;
            pSearchOutputParam->rangeFar = pSearchOutputParam->targetPosition + 2 * dof;
            pSearchOutputParam->fineStepSize = dof;
        }
    }

    return result;
}

CDKResult TOFRebaseReference(
    CHIHAFAlgorithm* pHAFAlgo)
{
    CDKResult result = CDKResultSuccess;
    TOFInternalData* pTOFData = reinterpret_cast<TOFInternalData*>(pHAFAlgo);

    result = pTOFData->pQtiTOFAlgo->HAFAlgorithmRebaseReference(pTOFData->pQtiTOFAlgo);

    return result;
}

CDKResult TOFClearData(
    CHIHAFAlgorithm* pHAFAlgo)
{
    CDKResult result = CDKResultSuccess;
    TOFInternalData* pTOFData = reinterpret_cast<TOFInternalData*>(pHAFAlgo);

    result = pTOFData->pQtiTOFAlgo->HAFAlgorithmClearData(pTOFData->pQtiTOFAlgo);

    return result;
}

VOID TOFDestroy(
    CHIHAFAlgorithm* pHAFAlgo)
{
    TOFInternalData* pTOFData = reinterpret_cast<TOFInternalData*>(pHAFAlgo);

    pTOFData->pQtiTOFAlgo->HAFAlgorithmDestroy(pTOFData->pQtiTOFAlgo);
}

CDKResult CreateHAFOverrideTOFDecorator(
    const AFAlgoCreateParamList*    pCreateParams,
    CHIHAFAlgorithm**               ppAlgoHandle)
{
    CDKResult result                        = CDKResultSuccess;
    CREATEHAFALGORITHM CreateHAFAlgorithm   = NULL;
    VOID* pParam                            = NULL;

    TOFInternalData* pTOFData = CAMX_NEW TOFInternalData;
    if (NULL == pTOFData)
    {
        *ppAlgoHandle = NULL;
        return CDKResultENoMemory;
    }

    pParam = GetParamFromCreateList(pCreateParams, AFCreateParamTypeCreateQtiHAFFunctionPtr);
    if (NULL == pParam)
    {
        HAF_MSG_ERROR( "Invalid input");
        goto err;
    }

    CreateHAFAlgorithm = reinterpret_cast<CREATEHAFALGORITHM>(pParam);
    result = CreateHAFAlgorithm(pCreateParams, HAFAlgoTypeTOF, &pTOFData->pQtiTOFAlgo);
    if (CDKResultSuccess != result)
    {
        HAF_MSG_ERROR( "Create QTI TOF algorithm failed");
        goto err;
    }

    pParam = GetParamFromCreateList(pCreateParams, AFCreateParamTypeHAFALgoUtil);
    if (NULL == pParam)
    {
        HAF_MSG_ERROR( "Get Algorithm Util function failed");
        goto err;
    }
    pTOFData->pAlgoUtils = reinterpret_cast<HAFAlgoUtilInterface*>(pParam);

    pTOFData->algoOps.HAFAlgorithmSetParam         = TOFSetParam;
    pTOFData->algoOps.HAFAlgorithmGetParam         = TOFGetParam;
    pTOFData->algoOps.HAFAlgorithmProcessMonitor   = TOFProcessMonitor;
    pTOFData->algoOps.HAFAlgorithmProcessSearch    = TOFProcessSearch;
    pTOFData->algoOps.HAFAlgorithmClearData        = TOFClearData;
    pTOFData->algoOps.HAFAlgorithmRebaseReference  = TOFRebaseReference;
    pTOFData->algoOps.HAFAlgorithmDestroy          = TOFDestroy;

    *ppAlgoHandle = reinterpret_cast<CHIHAFAlgorithm*>(pTOFData);
    return result;

err:
    if (pTOFData)
    {
        if (pTOFData->pQtiTOFAlgo)
        {
            pTOFData->pQtiTOFAlgo->HAFAlgorithmDestroy(pTOFData->pQtiTOFAlgo);
        }
        CAMX_DELETE pTOFData;
        *ppAlgoHandle = NULL;
    }
    return CDKResultEFailed;
}

