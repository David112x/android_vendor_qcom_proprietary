////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  afwrapper.cpp
/// @brief AF algorithm interface implementation wrapper
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "afwrapper.h"
#include "camxdefs.h"
#include "camxdebugprint.h"
#include "chicommon.h"
#include "chistatsalgo.h"
#include <system/camera_metadata.h>
#include <cutils/log.h>

// Logging function pointer for the algorithm
StatsLoggingFunction gStatsLogger;
#define AF_MSG_ERROR(fmt, ...)     gStatsLogger(CamX::Log::GetFileName(__FILE__),__LINE__,__FUNCTION__,StatsLogGroupAF,StatsLogError,fmt, ##__VA_ARGS__)
#define AF_MSG_WARN(fmt, ...)      gStatsLogger(CamX::Log::GetFileName(__FILE__),__LINE__,__FUNCTION__,StatsLogGroupAF,StatsLogWarning,fmt, ##__VA_ARGS__)
#define AF_MSG_HIGH(fmt, ...)      gStatsLogger(CamX::Log::GetFileName(__FILE__),__LINE__,__FUNCTION__,StatsLogGroupAF,StatsLogVerbose,fmt, ##__VA_ARGS__)
#define AF_MSG_LOW(fmt, ...)       gStatsLogger(CamX::Log::GetFileName(__FILE__),__LINE__,__FUNCTION__,StatsLogGroupAF,StatsLogVerbose,fmt, ##__VA_ARGS__)
#define AF_MSG_INFO(fmt, ...)      gStatsLogger(CamX::Log::GetFileName(__FILE__),__LINE__,__FUNCTION__,StatsLogGroupAF,StatsLogInfo,fmt, ##__VA_ARGS__)

static const UINT32      MaxCameras         = 8;                            ///< Define max simultaneous cameras supported
CHIAFALGORITHMCALLBACKS g_AFLibCallbacks;
CHIAFAlgorithm*         g_AFLibAlgorithmIntf;

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AFGetParam
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult AFGetParam(
    CHIAFAlgorithm*    pAFAlgorithm,
    AFAlgoGetParam*    pGetParams)
{
    CDKResult   result          = CDKResultSuccess;
    UINT32      curCameraID     = MaxCameras; // Valid camera ID should range from 0 to MaxCameras - 1
    UINT32      index           = 0;

    if ((NULL == pAFAlgorithm) || (NULL == pGetParams))
    {
        return CDKResultEInvalidArg;
    }
    for (index = 0; index < pGetParams->inputInfo.getParamInputCount; index++)
    {
        if (AFGetParamPassCameraInfo == pGetParams->inputInfo.pGetParamInputs[index].type)
        {
            curCameraID = reinterpret_cast<StatsCameraInfo*>(
                pGetParams->inputInfo.pGetParamInputs[index].pInputData)->cameraId;
        }
    }

    /* AF wrapper calls af lib af AFGetParam */
    if (NULL != g_AFLibAlgorithmIntf->AFGetParam)
    {
        result = g_AFLibAlgorithmIntf->AFGetParam(g_AFLibAlgorithmIntf, pGetParams);
    }

    AF_MSG_INFO("curCameraID: %d, result %d", curCameraID, result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AFSetParam
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult AFSetParam(
    CHIAFAlgorithm*            pAFAlgorithm,
    const AFAlgoSetParamList*  pSetParams)
{
    if ((NULL == pAFAlgorithm) || (NULL == pSetParams))
    {
        return CDKResultEInvalidArg;
    }

    CDKResult   result          = CDKResultSuccess;
    UINT32      curCameraID     = MaxCameras; // Valid camera ID should range from 0 to MaxCameras - 1
    UINT32      index           = 0;

    for (index = 0; index < pSetParams->numberOfSetParam; index++)
    {
        if (AFSetParamTypeCameraInfo == pSetParams->pAFSetParams[index].setParamType)
        {
            curCameraID = reinterpret_cast<const StatsCameraInfo*>(pSetParams->pAFSetParams[index].pAFSetParam)->cameraId;
        }
    }

    AF_MSG_INFO(" curCameraID: %d, result %d", curCameraID, result);

    /* AF wrapper calls af lib af AFSetParam */
    if (NULL != g_AFLibAlgorithmIntf->AFSetParam)
    {
        result = g_AFLibAlgorithmIntf->AFSetParam(g_AFLibAlgorithmIntf, pSetParams);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AFDestroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID AFDestroy(
    CHIAFAlgorithm*                 pAFAlgorithm,
    const AFAlgoDestroyParamList*   pDestroyParams)
{
    UINT32 curCameraID  = MaxCameras; // Valid camera ID should range from 0 to MaxCameras - 1
    UINT32 index        = 0;

    if (NULL == pAFAlgorithm)
    {
        AF_MSG_ERROR("pAFAlgorithm is NULL");
        return;
    }
    else
    {
        for (index = 0; index < pDestroyParams->paramCount; index++)
        {
            if (AFDestroyParamTypeCameraInfo == pDestroyParams->pParamList[index].destroyParamType)
            {
                curCameraID = reinterpret_cast<StatsCameraInfo*>(pDestroyParams->pParamList[index].pParam)->cameraId;
            }
        }
        /* AF wrapper calls af lib destroy */
        if (MaxCameras != curCameraID)
        {
            if (NULL != g_AFLibAlgorithmIntf->AFDestroy)
            {
                g_AFLibAlgorithmIntf->AFDestroy(g_AFLibAlgorithmIntf, pDestroyParams);
                g_AFLibAlgorithmIntf = NULL;
            }
        }
        else
        {
            AF_MSG_ERROR("Incorrect camera ID: %d", curCameraID);
        }
    }

    AF_MSG_INFO("curCameraID: %d", curCameraID);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AFProcess
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult AFProcess(
    CHIAFAlgorithm*        pAFAlgorithm,
    const AFAlgoInputList* pInputs,
    AFAlgoOutputList*      pOutputs)
{
    if ((NULL == pAFAlgorithm) || (NULL == pInputs) || (NULL == pOutputs))
    {
        return CDKResultEInvalidArg;
    }

    CDKResult   result          = CDKResultSuccess;
    UINT32      curCameraID     = MaxCameras; // Valid camera ID should range from 0 to MaxCameras - 1
    UINT32      index           = 0;
    StatsCameraInfo* pCamInfo   = NULL;

    for (index = 0; index < pInputs->numberOfInputParam; index++)
    {
        if (AFInputTypeCameraInfo == pInputs->pAFInputs[index].inputType)
        {
            pCamInfo = reinterpret_cast<StatsCameraInfo*>(pInputs->pAFInputs[index].pAFInput);
            curCameraID = pCamInfo->cameraId;
        }
    }

    /* AF wrapper calls af lib af process */
    if (NULL != g_AFLibAlgorithmIntf->AFProcess)
    {
        result = g_AFLibAlgorithmIntf->AFProcess(g_AFLibAlgorithmIntf, pInputs, pOutputs);
    }

    if (CDKResultSuccess == result)
    {
        CAMX_ASSERT_MESSAGE(NULL != pOutputs, "Output NULL pointer");

        for (UINT32 i = 0; i < pInputs->numberOfInputParam; i++)
        {
            if (pInputs->pAFInputs[i].sizeOfInput == 0)
                continue;

            switch (pInputs->pAFInputs[i].inputType)
            {
                default:
                    break;
            }
        }
    }

    AF_MSG_INFO("curCameraID: %d, result %d", curCameraID, result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateAFAlgorithm
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CreateAFAlgorithm(
    const AFAlgoCreateParamList*   pCreateParams,
    CHIAFAlgorithm**               ppAFAlgorithm)
{
    CDKResult result        = CDKResultSuccess;
    UINT32    curCameraID   = MaxCameras; // Valid camera ID should range from 0 to MaxCameras - 1

    if (pCreateParams->pParamList != NULL)
    {
        AFAlgoCreateParam* pCreateLoggerFunction = NULL;
        pCreateLoggerFunction = &pCreateParams->pParamList[AFCreateParamTypeLogFunctionPtr];
        gStatsLogger = reinterpret_cast<StatsLoggingFunction>(pCreateLoggerFunction->pParam);
        result = CDKResultSuccess;

        // Create AF lib instance
        if (NULL != g_AFLibCallbacks.pfnCreate)
        {
            result = g_AFLibCallbacks.pfnCreate(pCreateParams, ppAFAlgorithm);
        }
        else
        {
            AF_MSG_ERROR( "AF lib create function is NULL pointer");
            result = CDKResultEFailed;
        }

        if (result == CDKResultSuccess)
        {
            UINT32 index = 0;

            for (index = 0; index < pCreateParams->paramCount; index++)
            {
                if (AFCreateParamTypeCameraInfo == pCreateParams->pParamList[index].createParamType)
                {
                    curCameraID = reinterpret_cast<StatsCameraInfo*>(
                        pCreateParams->pParamList[index].pParam)->cameraId;
                }
            }

            if (MaxCameras == curCameraID)
            {
                AF_MSG_ERROR("Incorrect camera ID: %d", curCameraID);
                result = CDKResultEInvalidArg;
            }
        }

        if (result == CDKResultSuccess)
        {
            // Store af lib API for af wrapper to wrap the call
            g_AFLibAlgorithmIntf = *ppAFAlgorithm;

            /* Instantiate AF wrapper and hook up API */
            AFWrapperInternalDataType* pAFWrapperInternalData = new AFWrapperInternalDataType;
            if (NULL == pAFWrapperInternalData)
            {
                result = CDKResultENoMemory;
            }

            if (CDKResultSuccess == result)
            {
                pAFWrapperInternalData->AFOps.AFProcess  = AFProcess;
                pAFWrapperInternalData->AFOps.AFSetParam = AFSetParam;
                pAFWrapperInternalData->AFOps.AFGetParam = AFGetParam;
                pAFWrapperInternalData->AFOps.AFDestroy  = AFDestroy;
                *ppAFAlgorithm                             =
                    reinterpret_cast<CHIAFAlgorithm*>(pAFWrapperInternalData);
                AF_MSG_INFO("AF Wrapper loaded: curCameraID %d", curCameraID);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AFGetCapabilities
///
/// @brief  This method creates an instance to the AFAlgorithm.
///
/// @param  pCapsInfo Pointer to Algo capability structure
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult AFGetCapabilities(CHIALGORITHMCAPSINFO* pCapsInfo)
{
    CDKResult   result = CDKResultSuccess;

    AF_MSG_LOW("Get supported Capabilities size:%d, algo mask: %d", pCapsInfo->size, pCapsInfo->algorithmCapsMask);

    // AF wrapper direct calling AF lib GetCapabilities
    if (g_AFLibCallbacks.pfnGetCapabilities != NULL)
    {
        result = g_AFLibCallbacks.pfnGetCapabilities(pCapsInfo);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AFNodeQueryVendorTag
///
/// @brief  Implementation of PFNCHIQUERYVENDORTAG defined in chinode.h
///
/// @param  pQueryVendorTag Pointer to a structure that returns the exported vendor tag
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult AFNodeQueryVendorTag(
    CHIQUERYVENDORTAG* pQueryVendorTag)
{
    CDKResult result = CDKResultSuccess;
    CAMX_UNREFERENCED_PARAM(pQueryVendorTag);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AFAlgoSetAlgoInterface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID AFAlgoSetAlgoInterface(
    ChiAlgorithmInterface* pAlgoInterface)
{
    CAMX_ASSERT((NULL != pAlgoInterface) &&
        (NULL != pAlgoInterface->pGetVendorTagBase) &&
        (NULL != pAlgoInterface->pGetMetadata) &&
        (NULL != pAlgoInterface->pSetMetaData));

    // AF wrapper direct calling AF lib SetAlgoInterface
    if (g_AFLibCallbacks.pfnSetAlgoInterface != NULL)
    {
        g_AFLibCallbacks.pfnSetAlgoInterface(pAlgoInterface);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiAFAlgorithmEntry
///
/// @brief  Entry point called by the Chi driver to initialize the custom AF Algorithm.
///
/// @param [in,out] pAlgorithmCallbacks  Pointer to a structure that defines callbacks that the Chi driver sends to the
///                                      Algorithm. The Algorithm must fill in these function pointers.
///
/// @return VOID.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC VOID ChiAFAlgorithmEntry(CHIAFALGORITHMCALLBACKS* pCallBacks)
{

    const CHAR* pAFLibName ="com.qti.stats.af";
#if defined(_LP64)
    const CHAR* pLibPath = "/vendor/lib64/camera/components/";
#else // _LP64
    const CHAR* pLibPath = "/vendor/lib/camera/components/";
#endif // _LP64
    CHAR        libFilename[FILENAME_MAX];

    if (NULL == pCallBacks)
    {
        AF_MSG_ERROR( "Invalid argument: pCallBacks is NULL");
    }
    else if (pCallBacks->size < sizeof(CHIAFALGORITHMCALLBACKS))
    {
        AF_MSG_ERROR( "pCallBacks is smaller than expected");
    }
    else
    {
        pCallBacks->pfnGetCapabilities  = AFGetCapabilities;
        pCallBacks->pfnCreate           = CreateAFAlgorithm;
        pCallBacks->pfnSetAlgoInterface = AFAlgoSetAlgoInterface;
        pCallBacks->pfnQueryVendorTag   = AFNodeQueryVendorTag;
    }
        (void)snprintf(libFilename, FILENAME_MAX, "%s%s.%s", pLibPath, pAFLibName, "so");

        CamX::OSLIBRARYHANDLE       handle = CamX::OsUtils::LibMap(libFilename);

        if (NULL != handle)
        {
            VOID *pAddr = NULL;
            PFAFCHIALGORITHMENTRY       pAFAlgoEntry = NULL;
            const CHAR                  ChiAFEntry[] = "ChiAFAlgorithmEntry";

            pAddr = CamX::OsUtils::LibGetAddr(handle, ChiAFEntry);
            if (NULL != pAddr)
            {
                pAFAlgoEntry = reinterpret_cast<PFAFCHIALGORITHMENTRY>(pAddr);

                memset(&g_AFLibCallbacks, 0, sizeof(CHIAFALGORITHMCALLBACKS));

                if (NULL != pAFAlgoEntry)
                {
                    g_AFLibCallbacks.size = sizeof(CHIAFALGORITHMCALLBACKS);

                    // Call AF lib entry function to retrieve callbacks
                    pAFAlgoEntry(&g_AFLibCallbacks);
                }
            }
            else
            {
                AF_MSG_ERROR("AF lib entrt function is NULL");
            }
        }
        else
        {
            AF_MSG_ERROR("Failed load AF librart -%s", libFilename);
        }

}

#ifdef __cplusplus
}
#endif // __cplusplus

