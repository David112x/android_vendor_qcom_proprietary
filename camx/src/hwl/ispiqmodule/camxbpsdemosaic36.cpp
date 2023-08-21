////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpsdemosaic36.cpp
/// @brief BPSDEMOSAIC36 class implementation
///        Demosic Interpolates R/Gr/Gb/B Bayer mosaicked image to R, G, B full sized images
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "bps_data.h"
#include "camxbpsdemosaic36.h"
#include "camxbpsdemosaic36titan17x.h"
#include "camxbpsdemosaic36titan480.h"
#include "camxiqinterface.h"
#include "camxisphwsetting.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemosaic36::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemosaic36::Create(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        BPSDemosaic36* pModule = CAMX_NEW BPSDemosaic36(pCreateData->pNodeIdentifier);

        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Module initialization failed");
                pModule->Destroy();
                pModule = NULL;
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Memory allocation failed");
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
// BPSDemosaic36::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemosaic36::Initialize(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    m_pHWSetting = NULL;

    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            result = BPSDemosaic36Titan480::Create(&m_pHWSetting);
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            result = BPSDemosaic36Titan17x::Create(&m_pHWSetting);
            break;
        default:
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Titan Version: %d", pCreateData->titanVersion);
            break;
    }

    if (CamxResultSuccess == result)
    {
        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
        m_cmdLength = m_pHWSetting->GetCommandLength();

        result = AllocateCommonLibraryData();
        if (result != CamxResultSuccess)
        {
            CAMX_DELETE m_pHWSetting;
            m_pHWSetting = NULL;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to initilize common library data, no memory");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to create HW Setting Class, no memory");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemosaic36::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemosaic36::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(demosaic_3_6_0::demosaic36_rgn_dataType) * (Demosaic36MaximumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for demosaic_3_6_0::demosaic36_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemosaic36::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSDemosaic36::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData)           &&
        (NULL != pInputData->pHwContext))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMBPSIQSetting*>(pInputData->pOEMIQSetting))->DemosaicEnable;

            isChanged      = (TRUE == m_moduleEnable);
        }
        else
        {
            TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
            CAMX_ASSERT(NULL != pTuningManager);

            // Search through the tuning data (tree), only when there
            // are changes to the tuning mode data as an optimization
            if ((TRUE == pInputData->tuningModeChanged)    &&
                (TRUE == pTuningManager->IsValidChromatix()))
            {
                CAMX_ASSERT(NULL != pInputData->pTuningData);

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_demosaic36_bps(
                                   reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                   pInputData->pTuningData->noOfSelectionParameter);
            }

            CAMX_ASSERT(NULL != m_pChromatix);
            if (NULL != m_pChromatix)
            {
                if (m_pChromatix != m_dependenceData.pChromatix)
                {
                    m_dependenceData.pChromatix = m_pChromatix;
                    m_moduleEnable              = m_pChromatix->enable_section.demosaic_enable;

                    isChanged                   = (TRUE == m_moduleEnable);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to get Chromatix");
            }
        }
        if (TRUE == m_moduleEnable)
        {
            if (TRUE == IQInterface::s_interpolationTable.
                demosaic36TriggerUpdate(&(pInputData->triggerData), &m_dependenceData))
            {
                isChanged = TRUE;
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
// BPSDemosaic36::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemosaic36::RunCalculation(
    const ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        BpsIQSettings*       pBPSIQSettings        = reinterpret_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
        Demosaic36OutputData outputData;

        outputData.type              = PipelineType::BPS;

        result = IQInterface::Demosaic36CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Demosaic Calculation Failed. result %d", result);
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
// BPSDemosaic36::UpdateBPSInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSDemosaic36::UpdateBPSInternalData(
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
// BPSDemosaic36::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemosaic36::Execute(
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
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemosaic36::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSDemosaic36::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemosaic36::BPSDemosaic36
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSDemosaic36::BPSDemosaic36(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier = pNodeIdentifier;
    m_type            = ISPIQModuleType::BPSDemosaic;
    m_moduleEnable    = FALSE;
    m_pChromatix      = NULL;
    m_pHWSetting      = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemosaic36::~BPSDemosaic36
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSDemosaic36::~BPSDemosaic36()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
    DeallocateCommonLibraryData();
}

CAMX_NAMESPACE_END
