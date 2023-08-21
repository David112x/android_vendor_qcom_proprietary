////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipeupscaler12.cpp
/// @brief CAMXIPEUpscaler class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxipeupscaler12.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "ipe_data.h"
#include "camxtitan17xcontext.h"
#include "parametertuningtypes.h"
#include "camxipeupscaler12titan17x.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler12::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler12::Create(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        IPEUpscaler12* pModule = CAMX_NEW IPEUpscaler12(pCreateData->pNodeIdentifier);

        if (NULL == pModule)
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Memory allocation failed");
        }
        else
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Module initialization failed");
                CAMX_DELETE pModule;
                pModule = NULL;
            }
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Null input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler12::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler12::Initialize(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    m_pHWSetting = NULL;

    // Check Module Hardware Version and Create HW Setting Object
    switch (static_cast<Titan17xContext*>(pCreateData->initializationData.pHwContext)->GetTitanVersion())
    {
        default:
            m_pHWSetting = CAMX_NEW IPEUpscaler12Titan17x;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to create HW Setting Class, no memory");
        result = CamxResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler12::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPEUpscaler12::CheckDependenceChange(
    const ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData)                  &&
        (NULL != pInputData->pHwContext))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Lux Index [0x%f] ", pInputData->pAECUpdateData->luxIndex);

        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIPEIQSetting*>(pInputData->pOEMIQSetting))->US12Enable;
            isChanged      = (TRUE == m_moduleEnable);
        }
        else
        {
            m_moduleEnable                               = TRUE;
            isChanged                                    = (TRUE == m_moduleEnable);
            m_dependenceData.lumaVScaleFirAlgorithm      = UpscaleBiCubicInterpolation;
            m_dependenceData.lumaHScaleFirAlgorithm      = UpscaleBiCubicInterpolation;
            m_dependenceData.lumaInputDitheringDisable   = UpscaleDitheringDisable;
            m_dependenceData.lumaInputDitheringMode      = UpscaleRegularRound;
            m_dependenceData.chromaVScaleFirAlgorithm    = UpscaleBiCubicInterpolation;
            m_dependenceData.chromaHScaleFirAlgorithm    = UpscaleBiCubicInterpolation;
            m_dependenceData.chromaInputDitheringDisable = UpscaleDitheringDisable;
            m_dependenceData.chromaInputDitheringMode    = UpscaleCheckerBoardABBA;
            m_dependenceData.chromaRoundingModeV         = UpscaleCheckerBoardABBA;
            m_dependenceData.chromaRoundingModeH         = UpscaleCheckerBoardABBA;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Null Input Pointer");
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler12::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler12::RunCalculation(
    const ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Lux Index [0x%f] ", pInputData->pAECUpdateData->luxIndex);

    if (CamxResultSuccess == result)
    {
        result = IQInterface::IPEUpscale12CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Operation failed %d", result);
        }

        if (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod))
        {
            m_pHWSetting->DumpRegConfig();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler12::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler12::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        if (TRUE == m_moduleEnable)
        {
            if (TRUE == CheckDependenceChange(pInputData))
            {
                result = RunCalculation(pInputData);
            }

            // Regardless of any update in dependency parameters, command buffers and IQSettings/Metadata shall be updated.
            if ((CamxResultSuccess == result) && (TRUE == m_moduleEnable))
            {
                result = m_pHWSetting->CreateCmdList(pInputData, NULL);
            }

            if ((CamxResultSuccess == result) && (NULL != m_pHWSetting))
            {
                m_pHWSetting->SetupInternalData(pInputData);
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler12::IPEUpscaler12
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEUpscaler12::IPEUpscaler12(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier   = pNodeIdentifier;
    m_type              = ISPIQModuleType::IPEUpscaler;
    m_moduleEnable      = TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEUpscaler12::~IPEUpscaler12
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEUpscaler12::~IPEUpscaler12()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
