////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  aecwrapper.cpp
/// @brief AEC algorithm interface implementation wrapper
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "aecwrapper.h"
#include "camxdefs.h"
#include "camxdebugprint.h"
#include "chicommon.h"
#include "chistatsalgo.h"
#include <system/camera_metadata.h>
#include <cutils/log.h>

// Logging function pointer for the algorithm
StatsLoggingFunction gStatsLogger;
#define AEC_MSG_ERROR(fmt, ...)     gStatsLogger(CamX::Log::GetFileName(__FILE__),__LINE__,__FUNCTION__,StatsLogGroupAEC,StatsLogError,fmt, ##__VA_ARGS__)
#define AEC_MSG_WARN(fmt, ...)      gStatsLogger(CamX::Log::GetFileName(__FILE__),__LINE__,__FUNCTION__,StatsLogGroupAEC,StatsLogWarning,fmt, ##__VA_ARGS__)
#define AEC_MSG_HIGH(fmt, ...)      gStatsLogger(CamX::Log::GetFileName(__FILE__),__LINE__,__FUNCTION__,StatsLogGroupAEC,StatsLogVerbose,fmt, ##__VA_ARGS__)
#define AEC_MSG_LOW(fmt, ...)       gStatsLogger(CamX::Log::GetFileName(__FILE__),__LINE__,__FUNCTION__,StatsLogGroupAEC,StatsLogVerbose,fmt, ##__VA_ARGS__)
#define AEC_MSG_INFO(fmt, ...)      gStatsLogger(CamX::Log::GetFileName(__FILE__),__LINE__,__FUNCTION__,StatsLogGroupAEC,StatsLogInfo,fmt, ##__VA_ARGS__)

static const UINT32      MaxCameras         = 8;                            ///< Define max simultaneous cameras supported
CHIAECALGORITHMCALLBACKS g_AECLibCallbacks;
CHIAECAlgorithm*         g_AECLibAlgorithmIntf;

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AECGetParam
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult AECGetParam(
    CHIAECAlgorithm*    pAECAlgorithm,
    AECAlgoGetParam*    pGetParams)
{
    CDKResult   result          = CDKResultSuccess;
    UINT32      curCameraID     = MaxCameras; // Valid camera ID should range from 0 to MaxCameras - 1
    UINT32      index           = 0;

    if ((NULL == pAECAlgorithm) || (NULL == pGetParams))
    {
        return CDKResultEInvalidArg;
    }
    for (index = 0; index < pGetParams->inputList.numberOfAECGetInputParams; index++)
    {
        if (AECAlgoGetParamInputCameraInfo == pGetParams->inputList.pAECGetParamList[index].inputType)
        {
            curCameraID = reinterpret_cast<StatsCameraInfo*>(
                pGetParams->inputList.pAECGetParamList[index].pInputData)->cameraId;
        }
    }

    if (MaxCameras == curCameraID)
    {
        AEC_MSG_ERROR("Incorrect camera ID: %d", curCameraID);
        return CDKResultEInvalidArg;
    }

    /* AEC wrapper calls aec lib aec AECGetParam */
    if (NULL != g_AECLibAlgorithmIntf->AECGetParam)
    {
        result = g_AECLibAlgorithmIntf->AECGetParam(g_AECLibAlgorithmIntf, pGetParams);
    }

    AEC_MSG_INFO("curCameraID: %d, result %d", curCameraID, result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AECSetParam
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult AECSetParam(
    CHIAECAlgorithm*            pAECAlgorithm,
    const AECAlgoSetParamList*  pSetParams)
{
    if ((NULL == pAECAlgorithm) || (NULL == pSetParams))
    {
        return CDKResultEInvalidArg;
    }

    CDKResult   result          = CDKResultSuccess;
    UINT32      curCameraID     = MaxCameras; // Valid camera ID should range from 0 to MaxCameras - 1
    UINT32      index           = 0;

    for (index = 0; index < pSetParams->numberOfAECSetParams; index++)
    {
        if (AECAlgoSetParamCameraInfo == pSetParams->pAECSetParamList[index].setParamType)
        {
            curCameraID = reinterpret_cast<const StatsCameraInfo*>(pSetParams->pAECSetParamList[index].pAECSetParam)->cameraId;
        }
    }

    if (MaxCameras == curCameraID)
    {
        AEC_MSG_ERROR("Incorrect camera ID: %d", curCameraID);
        return CDKResultEInvalidArg;
    }

    /* AEC wrapper calls aec lib aec AECSetParam */
    if (NULL != g_AECLibAlgorithmIntf->AECSetParam)
    {
        result = g_AECLibAlgorithmIntf->AECSetParam(g_AECLibAlgorithmIntf, pSetParams);
        AEC_MSG_INFO("curCameraID: %d, result %d", curCameraID, result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AECDestroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID AECDestroy(
    CHIAECAlgorithm*                 pAECAlgorithm,
    const AECAlgoDestroyParamList*   pDestroyParams)
{
    UINT32 curCameraID  = MaxCameras; // Valid camera ID should range from 0 to MaxCameras - 1
    UINT32 index        = 0;

    if (NULL == pAECAlgorithm)
    {
        AEC_MSG_ERROR("pAECAlgorithm is NULL");
        return;
    }
    else
    {
        for (index = 0; index < pDestroyParams->paramCount; index++)
        {
            if (AECDestroyParamTypeCameraInfo == pDestroyParams->pParamList[index].destroyParamType)
            {
                curCameraID = reinterpret_cast<StatsCameraInfo*>(pDestroyParams->pParamList[index].pParam)->cameraId;
            }
        }
        /* AEC wrapper calls aec lib destroy */
        if (MaxCameras != curCameraID)
        {
            if (NULL != g_AECLibAlgorithmIntf->AECDestroy)
            {
                g_AECLibAlgorithmIntf->AECDestroy(g_AECLibAlgorithmIntf, pDestroyParams);
                g_AECLibAlgorithmIntf = NULL;
            }
        }
        else
        {
            AEC_MSG_ERROR("Incorrect camera ID: %d", curCameraID);
        }
    }

    AEC_MSG_INFO("curCameraID: %d", curCameraID);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AECProcess
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult AECProcess(
    CHIAECAlgorithm*        pAECAlgorithm,
    const AECAlgoInputList* pInputs,
    AECAlgoOutputList*      pOutputs)
{
    if ((NULL == pAECAlgorithm) || (NULL == pInputs) || (NULL == pOutputs))
    {
        return CDKResultEInvalidArg;
    }

    CDKResult   result          = CDKResultSuccess;
    UINT32      curCameraID     = MaxCameras; // Valid camera ID should range from 0 to MaxCameras - 1
    UINT32      index           = 0;
    StatsCameraInfo* pCamInfo   = NULL;

    for (index = 0; index < pInputs->numberOfAECInputs; index++)
    {
        if (AECAlgoInputCameraInfo == pInputs->pAECInputList[index].inputType)
        {
            pCamInfo = reinterpret_cast<StatsCameraInfo*>(pInputs->pAECInputList[index].pAECInput);
            curCameraID = pCamInfo->cameraId;
        }
    }

    if (MaxCameras == curCameraID)
    {
        AEC_MSG_ERROR("Incorrect camera ID: %d", curCameraID);
        return CDKResultEInvalidArg;
    }

    /* AEC wrapper calls aec lib aec process */
    if (NULL != g_AECLibAlgorithmIntf->AECProcess)
    {
        result = g_AECLibAlgorithmIntf->AECProcess(g_AECLibAlgorithmIntf, pInputs, pOutputs);
    }

    if (CDKResultSuccess == result)
    {
        for (UINT32 i = 0; i < pInputs->numberOfAECInputs; i++)
        {
            if (pInputs->pAECInputList[i].sizeOfAECInput == 0)
                continue;

            switch (pInputs->pAECInputList[i].inputType)
            {
                default:
                    break;
            }
        }
    }

    AEC_MSG_INFO("curCameraID: %d, result %d", curCameraID, result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateAECAlgorithm
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CreateAECAlgorithm(
    const AECAlgoCreateParamList*   pCreateParams,
    CHIAECAlgorithm**               ppAECAlgorithm)
{
    CDKResult result        = CDKResultSuccess;
    UINT32    curCameraID   = MaxCameras; // Valid camera ID should range from 0 to MaxCameras - 1

    if (pCreateParams->pCreateParamList != NULL)
    {
        AECAlgoCreateParam* pCreateLoggerFunction = NULL;
        pCreateLoggerFunction = &pCreateParams->pCreateParamList[AECAlgoCreateParamsLoggerFunctionPtr];
        gStatsLogger = reinterpret_cast<StatsLoggingFunction>(pCreateLoggerFunction->pCreateParam);
        result = CDKResultSuccess;

        // Create AEC lib instance
        if (NULL != g_AECLibCallbacks.pfnCreate)
        {
            result = g_AECLibCallbacks.pfnCreate(pCreateParams, ppAECAlgorithm);
        }
        else
        {
            AEC_MSG_ERROR("AEC lib create function is NULL pointer");
            result = CDKResultEFailed;
        }
        if (result == CDKResultSuccess)
        {
            UINT32 index = 0;

            for (index = 0; index < pCreateParams->numberOfCreateParams; index++)
            {
                if (AECAlgoCreateParamTypeCameraInfo == pCreateParams->pCreateParamList[index].createParamType)
                {
                    curCameraID = reinterpret_cast<StatsCameraInfo*>(
                        pCreateParams->pCreateParamList[index].pCreateParam)->cameraId;
                }
            }

            if (MaxCameras == curCameraID)
            {
                AEC_MSG_ERROR("Incorrect camera ID: %d", curCameraID);
                result = CDKResultEInvalidArg;
            }
        }
        if (result == CDKResultSuccess)
        {
            // Store aec lib API for aec wrapper to wrap the call
            g_AECLibAlgorithmIntf = *ppAECAlgorithm;

            /* Instantiate AEC wrapper and hook up API */
            AECWrapperInternalDataType* pAECWrapperInternalData = new AECWrapperInternalDataType;
            if (NULL == pAECWrapperInternalData)
            {
                result = CDKResultENoMemory;
            }

            if (CDKResultSuccess == result)
            {
                pAECWrapperInternalData->AECOps.AECProcess = AECProcess;
                pAECWrapperInternalData->AECOps.AECSetParam = AECSetParam;
                pAECWrapperInternalData->AECOps.AECGetParam = AECGetParam;
                pAECWrapperInternalData->AECOps.AECDestroy = AECDestroy;
                *ppAECAlgorithm =
                    reinterpret_cast<CHIAECAlgorithm*>(pAECWrapperInternalData);
                AEC_MSG_INFO("AEC Wrapper loaded: curCameraID %d", curCameraID);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AECGetCapabilities
///
/// @brief  This method creates an instance to the AECAlgorithm.
///
/// @param  pCapsInfo Pointer to Algo capability structure
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult AECGetCapabilities(CHIALGORITHMCAPSINFO* pCapsInfo)
{
    CDKResult   result = CDKResultSuccess;

    AEC_MSG_LOW("Get supported Capabilities size:%d, algo mask: %d", pCapsInfo->size, pCapsInfo->algorithmCapsMask);

    // AEC wrapper direct calling AEC lib GetCapabilities
    if (g_AECLibCallbacks.pfnGetCapabilities != NULL)
    {
        result = g_AECLibCallbacks.pfnGetCapabilities(pCapsInfo);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AECNodeQueryVendorTag
///
/// @brief  Implementation of PFNCHIQUERYVENDORTAG defined in chinode.h
///
/// @param  pQueryVendorTag Pointer to a structure that returns the exported vendor tag
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult AECNodeQueryVendorTag(
    CHIQUERYVENDORTAG* pQueryVendorTag)
{
    CDKResult result = CDKResultSuccess;

    // AEC wrapper direct calling AEC lib QueryVendorTag
    if (g_AECLibCallbacks.pfnQueryVendorTag != NULL)
    {
        result = g_AECLibCallbacks.pfnQueryVendorTag(pQueryVendorTag);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AECAlgoSetAlgoInterface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID AECAlgoSetAlgoInterface(
    ChiAlgorithmInterface* pAlgoInterface)
{
    CAMX_ASSERT((NULL != pAlgoInterface) &&
        (NULL != pAlgoInterface->pGetVendorTagBase) &&
        (NULL != pAlgoInterface->pGetMetadata) &&
        (NULL != pAlgoInterface->pSetMetaData));

    // AEC wrapper direct calling AEC lib SetAlgoInterface
    if (g_AECLibCallbacks.pfnSetAlgoInterface != NULL)
    {
        g_AECLibCallbacks.pfnSetAlgoInterface(pAlgoInterface);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiAECAlgorithmEntry
///
/// @brief  Entry point called by the Chi driver to initialize the custom AEC Algorithm.
///
/// @param [in,out] pAlgorithmCallbacks  Pointer to a structure that defines callbacks that the Chi driver sends to the
///                                      Algorithm. The Algorithm must fill in these function pointers.
///
/// @return VOID.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC VOID ChiAECAlgorithmEntry(CHIAECALGORITHMCALLBACKS* pCallBacks)
{
    const CHAR* pAECLibName = "com.qti.stats.aec";
    CHAR libFilename[FILENAME_MAX];

    if (NULL == pCallBacks)
    {
        AEC_MSG_ERROR("Invalid argument: pCallBacks is NULL");
    }
    else if (pCallBacks->size < sizeof(CHIAECALGORITHMCALLBACKS))
    {
        AEC_MSG_ERROR("pCallBacks is smaller than expected");
    }
    else
    {
        pCallBacks->pfnGetCapabilities  = AECGetCapabilities;
        pCallBacks->pfnCreate           = CreateAECAlgorithm;
        pCallBacks->pfnSetAlgoInterface = AECAlgoSetAlgoInterface;
        pCallBacks->pfnQueryVendorTag   = AECNodeQueryVendorTag;
    }

    (void)snprintf(libFilename, FILENAME_MAX, "%s%s%s.%s", CamX::VendorPartitionPath,"/components/", pAECLibName, "so");

    CamX::OSLIBRARYHANDLE       handle = CamX::OsUtils::LibMap(libFilename);

    if (NULL != handle)
    {
        VOID *pAddr = NULL;
        PFAECCHIALGORITHMENTRY      pAECAlgoEntry = NULL;
        const CHAR                  ChiAECEntry[] = "ChiAECAlgorithmEntry";

        pAddr = CamX::OsUtils::LibGetAddr(handle, ChiAECEntry);
        if (NULL != pAddr)
        {
            pAECAlgoEntry = reinterpret_cast<PFAECCHIALGORITHMENTRY>(pAddr);

            memset(&g_AECLibCallbacks, 0, sizeof(CHIAECALGORITHMCALLBACKS));

            if (NULL != pAECAlgoEntry)
            {
                g_AECLibCallbacks.size = sizeof(CHIAECALGORITHMCALLBACKS);

                // Call AEC lib entry function to retrieve callbacks
                pAECAlgoEntry(&g_AECLibCallbacks);
            }
        }
        else
        {
            AEC_MSG_ERROR("AEC lib entrt function is NULL");
        }
    }
    else
    {
        AEC_MSG_ERROR("Failed load AEC librart -%s", libFilename);
    }
}

#ifdef __cplusplus
}
#endif // __cplusplus
