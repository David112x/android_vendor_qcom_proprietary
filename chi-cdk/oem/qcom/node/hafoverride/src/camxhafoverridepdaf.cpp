////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxhafoverride.h"

struct PDAFInternalData
{
    CHIHAFAlgorithm         algoOps;
    FLOAT                   currentGain;
    INT                     currentDefocus;
    INT                     confidence;
    INT                     currentPosition;
    FLOAT                   actuatorSensitivity;
};

CDKResult PDAFSetParam(
    CHIHAFAlgorithm* pHAFAlgo,
    HAFAlgoSetParamList* pSetParams)
{
    CDKResult result = CDKResultSuccess;
    PDAFInternalData* pPDAFData = reinterpret_cast<PDAFInternalData*>(pHAFAlgo);
    if (NULL == pPDAFData || NULL == pSetParams)
    {
        HAF_MSG_ERROR("Error, NULL pointer");
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

        switch (pSetParam->setParamType)
        {
        case HAFSetParamTypeAECInfo:
        {
            AFAECInfo* pAECInfo = (AFAECInfo*)pSetParam->pData;
            pPDAFData->currentGain = pAECInfo->currentGain;
            break;
        }

        case HAFSetParamTypePDData:
        {
            AFPDAFData* pData = (AFPDAFData*)pSetParam->pData;
            pPDAFData->currentDefocus = pData->pDefocus.pDefocus[0];
            pPDAFData->confidence = pData->pDefocus.pConfidenceLevel[0];
            HAF_MSG_VERBOSE("Defocus: %d, confidence: %d",
                pPDAFData->currentDefocus, pPDAFData->confidence);
            break;
        }

        case HAFSetParamTypeSensorInfo:
        {
            AFSensorInfo* pSensorInfo = (AFSensorInfo*)pSetParam->pData;
            pPDAFData->actuatorSensitivity = pSensorInfo->actuatorSensitivity;
            HAF_MSG_VERBOSE("actuatorSensitivity: %f", pPDAFData->actuatorSensitivity);
            break;
        }

        case HAFSetParamTypeTuningData: /// Add more here
        default:
            break;
        }
    }

    return result;
}

CDKResult PDAFGetParam(
    CHIHAFAlgorithm* pHAFAlgo,
    HAFAlgoGetParam* pGetParam)
{
    CDKResult result = CDKResultSuccess;
    PDAFInternalData* pPDAFData = reinterpret_cast<PDAFInternalData*>(pHAFAlgo);
    if (NULL == pPDAFData || NULL == pGetParam)
    {
        HAF_MSG_ERROR("Error, NULL pointer");
        return CDKResultEFailed;
    }

    return result;
}

CDKResult PDAFProcessMonitor(
    CHIHAFAlgorithm*                pHAFAlgo,
    HAFAlgoMonitorInputParamList*   pMonitorInputParamList,
    HAFAlgoMonitorOutputParam*      pMonitorOutputParam)
{
    CDKResult result = CDKResultSuccess;
    PDAFInternalData* pPDAFData = reinterpret_cast<PDAFInternalData*>(pHAFAlgo);
    if (NULL == pPDAFData || NULL == pMonitorInputParamList || NULL == pMonitorOutputParam)
    {
        HAF_MSG_ERROR("NULL pointer");
        return CDKResultEInvalidArg;
    }

    VOID* pParam = GetParamFromMonitorInputList(pMonitorInputParamList, HAFMonitorInputParamTypeCurrentPosition);
    if (NULL != pParam)
    {
        pPDAFData->currentPosition = *reinterpret_cast<INT*>(pParam);
    }
    HAF_MSG_VERBOSE("currentPosition: %d,", pPDAFData->currentPosition);

    // Put customized logic here to handle more usecase based on preference
    // The example code here is very simple:
    // 1. if pd is not confident, pd algorithm just tell mixer it is not confident to make any decision
    // 2. if pd is confident and defocus is large, pd algorithm will report trigger refocus with high confidence
    // 3. if pd is confident and defocus is small, pd algorithm will report not trigger refocus with high confidence
    // 200 is used as a hard threshold to decide if pd is confident
    // 20 is used as a hard threshold to decide if defocus is bigger enough to retrigger refocus
    if (pPDAFData->confidence > 200)
    {
        INT defocusPosition = INT(pPDAFData->actuatorSensitivity * pPDAFData->currentDefocus);
        if (ABS(defocusPosition) > 20)
        {
            pMonitorOutputParam->triggerRefocus = TRUE;
        }
        else
        {
            pMonitorOutputParam->triggerRefocus = FALSE;
        }
        pMonitorOutputParam->confidence = HAFConfidenceLevelHigh;
    }
    else
    {
        pMonitorOutputParam->confidence = HAFConfidenceLevelLow;
    }

    return result;
}

CDKResult PDAFProcessSearch(
    CHIHAFAlgorithm*                pHAFAlgo,
    HAFAlgoSearchInputParamList*    pSearchInputParamList,
    HAFAlgoSearchOutputParam*       pSearchOutputParam)
{
    CDKResult result = CDKResultSuccess;
    PDAFInternalData* pPDAFData = reinterpret_cast<PDAFInternalData*>(pHAFAlgo);
    if (NULL == pPDAFData || NULL == pSearchInputParamList || NULL == pSearchOutputParam)
    {
        HAF_MSG_ERROR("NULL pointer");
        return CDKResultEInvalidArg;
    }

    INT defocus = 0;
    VOID* pParam = GetParamFromSearchInputList(pSearchInputParamList, HAFSearchInputParamTypeCurrentPosition);
    if (NULL != pParam)
    {
        pPDAFData->currentPosition = *reinterpret_cast<INT*>(pParam);
    }

    // Put customized logic here to handle more usecase based on preference
    // The example code here is very simple:
    // 1. if pd is not confident, pd algorith will simple tell mixer pd search can't proceed but with low confidence
    // 2. if pd is confident, and defocus is large, pd algorithm will calcualte next position and set progress to 50
    // 3. if pd is confident and defocus is small, pd algorithm will report pd is done with high confidence
    // range near/far is PD estimate the CAF fine scan range, if they are equal, fine scan will be skiped
    // if pd is not confident, then we call pd will fallback, in this situation, the searchDirection is used for tell
    // Contrast AF the most possible target direction is.
    if (pPDAFData->confidence > 200)
    {
        defocus = INT(pPDAFData->actuatorSensitivity * pPDAFData->currentDefocus);
        INT dof = 10;//pTOFData->pAlgoUtils->CalcDOF(pTOFData->pAlgoUtils);
        if (ABS(defocus) > 20)
        {
            pSearchOutputParam->progress = HAFProgressLevel50;
            pSearchOutputParam->targetPosition = pPDAFData->currentPosition + defocus;
            pSearchOutputParam->nextPosition = INT(pPDAFData->currentPosition + defocus * 0.5);
            pSearchOutputParam->isFrameSkip = FALSE;
        }
        else
        {
            pSearchOutputParam->progress = HAFProgressLevel100;
            pSearchOutputParam->targetPosition = pPDAFData->currentPosition;
            pSearchOutputParam->nextPosition = pPDAFData->currentPosition + defocus;
            pSearchOutputParam->rangeNear = pSearchOutputParam->targetPosition - 2 * dof;
            pSearchOutputParam->rangeFar = pSearchOutputParam->targetPosition + 2 * dof;
            pSearchOutputParam->fineStepSize = dof;
            pSearchOutputParam->isFrameSkip = FALSE;
        }
        pSearchOutputParam->confidence = HAFConfidenceLevelHigh;
    }
    else
    {
        pSearchOutputParam->confidence = HAFConfidenceLevelLow;
        pSearchOutputParam->progress = HAFProgressLevel100;
        pSearchOutputParam->targetPosition = pPDAFData->currentPosition;
        pSearchOutputParam->nextPosition = pPDAFData->currentPosition;
        pSearchOutputParam->searchDirection = pPDAFData->currentDefocus > 0 ? AFMoveDirectionFar : AFMoveDirectionNear;
    }

    HAF_MSG_VERBOSE("defocus: %d confidence: %d, progress: %d, nextPosition: %d",
        defocus, pSearchOutputParam->confidence, pSearchOutputParam->progress, pSearchOutputParam->nextPosition);

    return result;
}

CDKResult PDAFRebaseReference(
    CHIHAFAlgorithm* pHAFAlgo)
{
    CDKResult result = CDKResultSuccess;
    PDAFInternalData* pPDAFData = reinterpret_cast<PDAFInternalData*>(pHAFAlgo);
    if (NULL == pPDAFData)
    {
        HAF_MSG_ERROR("NULL pointer");
        return CDKResultEInvalidArg;
    }

    return result;
}


CDKResult PDAFClearData(
    CHIHAFAlgorithm* pHAFAlgo)
{
    CDKResult result = CDKResultSuccess;
    PDAFInternalData* pPDAFData = reinterpret_cast<PDAFInternalData*>(pHAFAlgo);
    if (NULL == pPDAFData)
    {
        HAF_MSG_ERROR("NULL pointer");
        return CDKResultEInvalidArg;
    }

    return result;
}

VOID PDAFDestroy(
    CHIHAFAlgorithm* pHAFAlgo)
{
    PDAFInternalData* pPDAFData = reinterpret_cast<PDAFInternalData*>(pHAFAlgo);

    if (pPDAFData)
    {
        CAMX_DELETE pPDAFData;
    }
}


CDKResult CreateHAFOverridePDAFAlgorithm(
    const AFAlgoCreateParamList*    pCreateParams,
    CHIHAFAlgorithm**               ppAlgoHandle)
{
    CAMX_UNREFERENCED_PARAM(pCreateParams);
    CDKResult result = CDKResultSuccess;

    PDAFInternalData* pPDAFData = CAMX_NEW PDAFInternalData;
    if (NULL == pPDAFData)
    {
        *ppAlgoHandle = NULL;
        return CDKResultENoMemory;
    }

    pPDAFData->algoOps.HAFAlgorithmSetParam         = PDAFSetParam;
    pPDAFData->algoOps.HAFAlgorithmGetParam         = PDAFGetParam;
    pPDAFData->algoOps.HAFAlgorithmProcessMonitor   = PDAFProcessMonitor;
    pPDAFData->algoOps.HAFAlgorithmProcessSearch    = PDAFProcessSearch;
    pPDAFData->algoOps.HAFAlgorithmClearData        = PDAFClearData;
    pPDAFData->algoOps.HAFAlgorithmRebaseReference  = PDAFRebaseReference;
    pPDAFData->algoOps.HAFAlgorithmDestroy          = PDAFDestroy;

    *ppAlgoHandle = reinterpret_cast<CHIHAFAlgorithm*>(pPDAFData);

    return result;
}
