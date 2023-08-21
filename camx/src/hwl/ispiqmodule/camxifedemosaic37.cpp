////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifedemosaic37.cpp
/// @brief CAMXIFEDEMOSAIC37 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxifedemosaic37titan17x.h"
#include "camxifedemosaic37.h"
#include "camxiqinterface.h"
#include "camxisphwsetting.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "demosaic_3_7_0.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic37::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemosaic37::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEDemosaic37* pModule = CAMX_NEW IFEDemosaic37;

        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Module initialization failed !!");
                CAMX_DELETE pModule;
                pModule = NULL;
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Memory allocation failed");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFEDemosaic34 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic37::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemosaic37::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFEDemosaic37Titan17x;
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupISP, "Not supported Titan 0x%x", pCreateData->titanVersion);
            result = CamxResultEUnsupported;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
        m_cmdLength                 = m_pHWSetting->GetCommandLength();
        result                      = AllocateCommonLibraryData();
        if (CamxResultSuccess != result)
        {
            CAMX_DELETE static_cast<IFEDemosaic37Titan17x*>(m_pHWSetting);
            m_pHWSetting                = NULL;
            m_cmdLength                 = 0;
            m_dependenceData.pHWSetting = NULL;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to initilize common library data, no memory");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class, Titan 0x%x", pCreateData->titanVersion);
        result = CamxResultENoMemory;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic37::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemosaic37::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(demosaic_3_7_0::demosaic37_rgn_dataType) * (Demosaic37MaximumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for demosaic_3_7_0::demosaic37_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic37::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemosaic37::Execute(
    ISPInputData* pInputData)
{
    CamxResult result  = CamxResultSuccess;
    VOID* pSettingData = static_cast<VOID*>(pInputData);

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        if (FALSE == pInputData->useHardcodedRegisterValues)
        {
            if (TRUE == CheckDependenceChange(pInputData))
            {
                result = RunCalculation(pInputData);
                if (CamxResultSuccess == result)
                {
                    result = m_pHWSetting->CreateCmdList(pSettingData, NULL);
                }
            }
        }
        else
        {
            result = m_pHWSetting->CreateCmdList(pSettingData, NULL);
        }

        if (CamxResultSuccess == result)
        {
            UpdateIFEInternalData(pInputData);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Input: pInputData %p m_pHWSetting %p", pInputData, m_pHWSetting);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic37::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDemosaic37::UpdateIFEInternalData(
    const ISPInputData* pInputData)
{
    pInputData->pCalculatedData->moduleEnable.IQModules.demosaicEnable = m_moduleEnable;

    // Post tuning metadata if setting is enabled
    if (NULL != pInputData->pIFETuningMetadata)
    {
        if (CamxResultSuccess != m_pHWSetting->UpdateTuningMetadata(pInputData->pIFETuningMetadata))
        {
            CAMX_LOG_WARN(CamxLogGroupISP, "UpdateTuningMetadata failed.");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic37::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEDemosaic37::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;
    BOOL dynamicEnable = Utils::IsBitSet(pInputData->IFEDynamicEnableMask, static_cast<UINT>(DynamicIFEMaskBit::IFEDemosaic));

    /// @todo (CAMX-561) how to determine the pSelectors and numSelector

    if ((NULL != pInputData->pAECUpdateData)  &&
        (NULL != pInputData->pHwContext))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIFEIQSetting*>(pInputData->pOEMIQSetting))->DemosaicEnable;

            if (TRUE == m_moduleEnable)
            {
                isChanged = TRUE;
            }
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

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_demosaic37_ife(
                    reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                    pInputData->pTuningData->noOfSelectionParameter);
            }

            CAMX_ASSERT(NULL != m_pChromatix);
            if (NULL != m_pChromatix)
            {
                if ((NULL                        == m_dependenceData.pChromatix)                    ||
                    (m_pChromatix->SymbolTableID != m_dependenceData.pChromatix->SymbolTableID)     ||
                    (m_moduleEnable              != m_pChromatix->enable_section.demosaic_enable))
                {
                    m_dependenceData.pChromatix = m_pChromatix;
                    m_moduleEnable              = m_pChromatix->enable_section.demosaic_enable;
                    if (TRUE == m_moduleEnable)
                    {
                        isChanged = TRUE;
                    }
                }
            }
        }

        m_moduleEnable &= dynamicEnable;
        if ((TRUE == m_moduleEnable) && (m_dynamicEnable != dynamicEnable))
        {
            isChanged = TRUE;
        }
        m_dynamicEnable = dynamicEnable;

        if (TRUE == m_moduleEnable)
        {
            if ((TRUE ==
                IQInterface::s_interpolationTable.demosaic37TriggerUpdate(&pInputData->triggerData, &m_dependenceData)) ||
                (TRUE == pInputData->forceTriggerUpdate))
            {
                isChanged = TRUE;
            }
        }

        if (FALSE == m_moduleEnable&&
            TRUE == pInputData->sensorData.isMono)
        {
            isChanged = TRUE;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP,
                       "Invalid Input: pAECUpdateData %p  pHwContext %p pNewAWBUpdate %p",
                       pInputData->pAECUpdateData,
                       pInputData->pHwContext,
                       pInputData->pAWBUpdateData);
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic37::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemosaic37::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult           result = CamxResultSuccess;
    Demosaic37OutputData outputData;

    outputData.type           = PipelineType::IFE;

    if ((FALSE == m_moduleEnable) &&
        (TRUE  == pInputData->sensorData.isMono))
    {
        m_pHWSetting->SetupRegisterSetting(NULL);
    }
    else
    {
        result = IQInterface::Demosaic37CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);

        if (CamxResultSuccess != result)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Demosaic Calculation Failed. result %d", result);
        }
    }

    if (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod))
    {
        m_pHWSetting->DumpRegConfig();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic37::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDemosaic37::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic37::IFEDemosaic37
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDemosaic37::IFEDemosaic37()
{
    m_type         = ISPIQModuleType::IFEDemosaic;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEDemosaic37::~IFEDemosaic37
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDemosaic37::~IFEDemosaic37()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }

    DeallocateCommonLibraryData();
}

CAMX_NAMESPACE_END
