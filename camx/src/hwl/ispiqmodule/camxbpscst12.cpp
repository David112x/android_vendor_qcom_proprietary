////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpscst12.cpp
/// @brief CAMXBPSCST12 class implementation
///        Color space transform (CST) as a standard 3x3 matrix multiplication to convert RGB to YUV or vice versa,
///        and Color conversion (CV) for converting RGB to YUV only with chroma enhancement capability.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "bps_data.h"
#include "camxbpscst12.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"
#include "camxbpscst12titan17x.h"
#include "camxbpscst12titan480.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCST12::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSCST12::Create(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;
    BPSCST12* pModule = NULL;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        pModule = CAMX_NEW BPSCST12(pCreateData->pNodeIdentifier);
        if (NULL == pModule)
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Memory allocation failed");
        }
        else
        {
            result = pModule->Initialize(pCreateData);
        }
        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCST12::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSCST12::Initialize(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            result = BPSCST12Titan480::Create(&m_pHWSetting);
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            result = BPSCST12Titan17x::Create(&m_pHWSetting);
            break;
        default:
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Titan Version: %d", pCreateData->titanVersion);
            break;
    }

    if (CamxResultSuccess == result)
    {
        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
        m_cmdLength                 = m_pHWSetting->GetCommandLength();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to create HW Setting Class, no memory");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCST12::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSCST12::CheckDependenceChange(
    const ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData)           &&
        (NULL != pInputData->pHwContext))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMBPSIQSetting*>(pInputData->pOEMIQSetting))->CSTEnable;

            isChanged      = (TRUE == m_moduleEnable);
        }
        else
        {
            if (NULL != pInputData->pTuningData)
            {
                TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
                CAMX_ASSERT(NULL != pTuningManager);

                // Search through the tuning data (tree), only when there
                // are changes to the tuning mode data as an optimization
                if ((TRUE == pInputData->tuningModeChanged)    &&
                    (TRUE == pTuningManager->IsValidChromatix()))
                {
                    m_pChromatix = pTuningManager->GetChromatix()->GetModule_cst12_bps(
                                       reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                       pInputData->pTuningData->noOfSelectionParameter);
                }

                CAMX_ASSERT(NULL != m_pChromatix);
                if (NULL != m_pChromatix)
                {
                    if (m_pChromatix != m_dependenceData.pChromatix)
                    {
                        m_dependenceData.pChromatix = m_pChromatix;
                        m_moduleEnable              = m_pChromatix->enable_section.cst_enable;

                        isChanged                   = (TRUE == m_moduleEnable);
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to get Chromatix");
                }
            }
        }
    }
    else
    {
        if (NULL != pInputData)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Input: pHwContext %p", pInputData->pHwContext);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
        }
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCST12::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSCST12::RunCalculation(
    const ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        CST12OutputData outputData;

        outputData.type                  = PipelineType::BPS;

        result = IQInterface::CST12CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "CST12 Calculation Failed. result %d", result);
        }
        if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
        {
            m_pHWSetting->DumpRegConfig();
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCST12::UpdateBPSInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSCST12::UpdateBPSInternalData(
    ISPInputData* pInputData
    ) const
{
    CamxResult result = CamxResultSuccess;

    result = m_pHWSetting->UpdateFirmwareData(pInputData, m_moduleEnable);

    if (CamxResultSuccess == result)
    {
        // Post tuning metadata if setting is enabled
        if (NULL != pInputData->pBPSTuningMetadata)
        {
            result = m_pHWSetting->UpdateTuningMetadata(pInputData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupBPS, "UpdateTuningMetadata failed.");
                result = CamxResultSuccess; // Non-fatal error
            }
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupBPS, "UpdateFirmwareData failed.");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCST12::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSCST12::Execute(
    ISPInputData* pInputData)
{
    CamxResult result       = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            if (FALSE == pInputData->useHardcodedRegisterValues)
            {
                result = RunCalculation(pInputData);
            }
        }

        if ((CamxResultSuccess == result) && (TRUE == m_moduleEnable))
        {
            result = m_pHWSetting->CreateCmdList(pInputData, NULL);
        }

        if (CamxResultSuccess == result)
        {
            UpdateBPSInternalData(pInputData);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Operation failed %d", result);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer %p, m_pHWSetting %p", pInputData, m_pHWSetting);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCST12::BPSCST12
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSCST12::BPSCST12(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier           = pNodeIdentifier;
    m_type                      = ISPIQModuleType::BPSCST;
    m_moduleEnable              = FALSE;
    m_pChromatix                = NULL;
    m_32bitDMILength            = 0;
    m_64bitDMILength            = 0;
    m_dependenceData.pChromatix = NULL;
    m_pHWSetting                = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCST12::~BPSCST12
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSCST12::~BPSCST12()
{
    m_pChromatix = NULL;
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
