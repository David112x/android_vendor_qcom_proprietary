////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchihvxaddconstant.cpp
/// @brief HVX add constamnt example
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "camxdefs.h"
#include "chiisphvxdefs.h"

// NOWHINE FILE CP040: Keyword new not allowed. Use the CAMX_NEW/CAMX_DELETE functions insteads

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AddConstantHVXInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult  AddConstantHVXInitialize(
    VOID** ppOEMData)
{
    UINT32* pData = static_cast<UINT32*>(calloc(1, sizeof(UINT32)));
    CDKResult result = CDKResultSuccess;

    if (NULL != pData)
    {
        *ppOEMData = static_cast<VOID*>(pData);
        result     = CDKResultSuccess;
    }
    else
    {
          /* log error */
        LOG_ERROR(CamxLogGroupChi, "failed: data %p\n", pData);
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AddConstantResolutionInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult AddConstantResolutionInfo(
    VOID*              pOEMData,
    HVXResolutionInfo** ppResInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL != pOEMData) && (NULL != ppResInfo) && (NULL != *ppResInfo))
    {
        (*ppResInfo)->tappingPoint  = IFEBEGINNING;
        (*ppResInfo)->hvxOutWidth   =  (*ppResInfo)->sensorWidth;
        (*ppResInfo)->hvxOutHeight  =  (*ppResInfo)->sensorHeight;
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "failed: pOEMData %p pHVXInfo %p\n", pOEMData, ppResInfo);
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AddConstantGetHvxInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult AddConstantGetHvxInfo(
    VOID*          pOEMData,
    HVXGetSetInfo* pHVXInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL != pOEMData) && (NULL != pHVXInfo))
    {
        pHVXInfo->hvxEnable    = HVX_TRUE;
#if defined (_WINDOWS)
        strncpy_s(pHVXInfo->algorithmName, "add_constant", sizeof(pHVXInfo->algorithmName));
#elif defined (ANDROID)
        strlcpy(pHVXInfo->algorithmName, "add_constant", sizeof(pHVXInfo->algorithmName));
#endif // (_WINDOWS) (ANDROID)


        if (pHVXInfo->availableHVXUnits >= 1)
        {
            pHVXInfo->requestHVXUnits = 1;
        }

        if (pHVXInfo->availableHVXVectorMode >= HVX_LIB_VECTOR_128)
        {
            pHVXInfo->requestHVXVectorMode = HVX_LIB_VECTOR_128;
        }
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "failed: pOEMData %p pHVXInfo %p\n", pOEMData, pHVXInfo);
        result =  CDKResultEFailed;
    }



    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AddConstantSetConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult AddConstantSetConfig(
    VOID*                pOEMData,
    HVXConfig*           pStubConfig,
    const HVXDSPConfig*  pAdspConfig,
    INT*                 phDSPHandle)
{
    CDKResult result   = CDKResultSuccess;
    UINT32*   pData = NULL;
    HVXStubStaticConfig* pStaticConfig = NULL;

    if ((NULL != pOEMData)                       &&
        (NULL != pStubConfig)                    &&
        (NULL != pAdspConfig)                    &&
        (NULL != pAdspConfig->pDSPDynamicConfig) &&
        (NULL != pAdspConfig->pDSPStaticConfig))
    {
        pStaticConfig = &pStubConfig->stubStaticConfig;
        pData = static_cast<UINT32*>(pOEMData);

        result = pAdspConfig->pDSPStaticConfig(phDSPHandle, pStaticConfig);
        if (result != CDKResultSuccess)
        {
            /* log error */
            LOG_ERROR(CamxLogGroupChi, "failed: adspConfig_call ret %d\n", result);
        }

        *pData = 5;

           /* Call pAdspConfig to pass call to ADSP */
        result = pAdspConfig->pDSPDynamicConfig(phDSPHandle, pData, sizeof(UINT32));
        if (result != CDKResultSuccess)
        {
            /* log error */
            LOG_ERROR(CamxLogGroupChi, "failed: pDSPDynamicConfig ret %d\n", result);
        }

    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "failed: %p %p %p\n", pOEMData, pStaticConfig, pAdspConfig);
        result =  CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AddConstantExecute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult AddConstantExecute(
    VOID*               pOEMData,
    const HVXInputData* pInputParams,
    const HVXDSPConfig* pAdspConfig,
    INT*                phDSPHandle)
{
    CDKResult  result   = CDKResultSuccess;
    UINT32*    pData = NULL;

    if ((NULL != pOEMData)                       &&
        (NULL != pAdspConfig)                    &&
        (NULL != pAdspConfig->pDSPDynamicConfig) &&
        (NULL != pInputParams))
    {
        pData = static_cast<UINT32*>(pOEMData);

        *pData = *pData + 5;
        if (*pData >= 100)
        {
            *pData = 5;
        }

        /* Call pAdspConfig to pass call to ADSP */
        result = pAdspConfig->pDSPDynamicConfig(phDSPHandle, pData, sizeof(UINT32));
        if (result != CDKResultSuccess)
        {
            /* log error */
            LOG_ERROR(CamxLogGroupChi, "failed: pDSPDynamicConfig ret %d\n", result);
        }

    }
    else
    {
                /* log error */
        LOG_ERROR(CamxLogGroupChi, "failed: %p %p %p\n", pOEMData, pAdspConfig, pInputParams);
        result =  CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AddConstantDestroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult AddConstantDestroy(
    VOID* pOEMData)
{
    if (NULL != pOEMData)
    {
        free(pOEMData);
    }
    else
    {            /* log error */
        LOG_ERROR(CamxLogGroupChi, "failed: hvx_lib_close pOEMData %p\n", pOEMData);
        return CDKResultEFailed;
    }

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiISPHVXAlgorithmEntry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC CDKResult ChiISPHVXAlgorithmEntry(
    CHIISPHVXALGORITHMCALLBACKS* pCallBacks)
{
    if (NULL != pCallBacks)
    {
        pCallBacks->pHVXInitialize        = AddConstantHVXInitialize;
        pCallBacks->pHVXGetResolutionInfo = AddConstantResolutionInfo;
        pCallBacks->pHVXGetSetHVXInfo     = AddConstantGetHvxInfo;
        pCallBacks->pHVXExecute           = AddConstantExecute;
        pCallBacks->pHVXSetConfig         = AddConstantSetConfig;
        pCallBacks->pHVXDestroy           = AddConstantDestroy;
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "func_table %p\n", pCallBacks);
        return CDKResultEFailed;
    }

    return CDKResultSuccess;
}
