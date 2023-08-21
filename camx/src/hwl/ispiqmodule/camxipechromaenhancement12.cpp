////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipechromaenhancement12.cpp
/// @brief IPEChromaEnhancement class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxipechromaenhancement12.h"
#include "camxipechromaenhancement12titan17x.h"
#include "camxipechromaenhancement12titan480.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtitan17xcontext.h"
#include "camxtuningdatamanager.h"
#include "ipe_data.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEChromaEnhancement12::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEChromaEnhancement12::Create(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        IPEChromaEnhancement12* pModule = CAMX_NEW IPEChromaEnhancement12(pCreateData->pNodeIdentifier);

        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (result != CamxResultSuccess)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to initilize LUTCmdBufferManager, no memory");
                CAMX_DELETE pModule;
                pModule = NULL;
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Memory allocation failed");
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
// IPEChromaEnhancement12::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEChromaEnhancement12::Initialize(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    m_pHWSetting = NULL;

    // Check Module Hardware Version and Create HW Setting Object
    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IPEChromaEnhancement12Titan480;
            break;

        default:
            m_pHWSetting = CAMX_NEW IPEChromaEnhancement12Titan17x;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        UINT size = m_pHWSetting->GetRegSize();

        m_cmdLength = PacketBuilder::RequiredWriteRegRangeSizeInDwords(size / RegisterWidthInBytes) * sizeof(UINT32);

        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
        result = AllocateCommonLibraryData();
        if (result != CamxResultSuccess)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to initilize common library data, no memory");
            CAMX_DELETE m_pHWSetting;
            m_pHWSetting = NULL;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to create HW Setting Class, no memory");
        result = CamxResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEChromaEnhancement12::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEChromaEnhancement12::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(cv_1_2_0::cv12_rgn_dataType) * (CV12MaxNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for cv_1_2_0::cv12_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEChromaEnhancement12::UpdateIPEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEChromaEnhancement12::UpdateIPEInternalData(
    ISPInputData* pInputData
    ) const
{
    CamxResult     result         = CamxResultSuccess;
    IpeIQSettings* pIPEIQSettings = reinterpret_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);

    pIPEIQSettings->chromaEnhancementParameters.moduleCfg.EN = m_moduleEnable;
    /// @todo (CAMX-738) Add metadata information

    // Post tuning metadata if setting is enabled
    if (NULL != pInputData->pIPETuningMetadata)
    {
        result = m_pHWSetting->UpdateTuningMetadata(pInputData);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "Update Tuning Metadata failed");
            result = CamxResultSuccess; // Non-fatal errorv
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEChromaEnhancement12::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEChromaEnhancement12::ValidateDependenceParams(
    const ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    /// @todo (CAMX-730) validate dependency parameters
    if ((NULL == pInputData)                                                   ||
        (NULL == pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferPostLTM]) ||
        (NULL == pInputData->pipelineIPEData.pIPEIQSettings))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEChromaEnhancement12::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPEChromaEnhancement12::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData)                  &&
        (NULL != pInputData->pAECUpdateData)  &&
        (NULL != pInputData->pAWBUpdateData)  &&
        (NULL != pInputData->pHwContext))
    {
        /// @todo (CAMX-730) Check module dependency
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Lux Index [0x%f] ", pInputData->pAECUpdateData->luxIndex);

        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIPEIQSetting*>(pInputData->pOEMIQSetting))->ChromaEnhancementEnable;

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
                    m_pChromatix = pTuningManager->GetChromatix()->GetModule_cv12_ipe(
                                       reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                       pInputData->pTuningData->noOfSelectionParameter);
                }

                CAMX_ASSERT(NULL != m_pChromatix);
                if (NULL != m_pChromatix)
                {
                    if (m_pChromatix != m_dependenceData.pChromatix)
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "updating chromatix pointer");
                        m_dependenceData.pChromatix = m_pChromatix;
                        m_moduleEnable              = m_pChromatix->enable_section.cv_enable;

                        isChanged                   = (TRUE == m_moduleEnable);
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to get Chromatix");
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to get Tuning Pointer");
            }
        }

        if (TRUE == m_moduleEnable)
        {
            if (TRUE ==
                IQInterface::s_interpolationTable.CV12TriggerUpdate(&pInputData->triggerData, &m_dependenceData))
            {
                isChanged = TRUE;
            }
        }
    }
    else
    {
        if (NULL != pInputData)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc,
                           "Invalid Input: pNewAECUpdate %p pNewAWBUpdate %p pHwContext %p",
                           pInputData->pAECUpdateData,
                           pInputData->pAWBUpdateData,
                           pInputData->pHwContext);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Null Input Pointer");
        }
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEChromaEnhancement12::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEChromaEnhancement12::RunCalculation(
    const ISPInputData* pInputData)
{
    CamxResult     result = CamxResultSuccess;
    CV12OutputData outputData;

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Lux Index [0x%f] ", pInputData->pAECUpdateData->luxIndex);

    result             = IQInterface::IPECV12CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "IPE CV12 Calculation Failed. result %d", result);
    }

    if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
    {
        m_pHWSetting->DumpRegConfig();
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEChromaEnhancement12::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEChromaEnhancement12::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        // Check if dependency is published and valid
        result = ValidateDependenceParams(pInputData);

        if (CamxResultSuccess == result)
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

            if (CamxResultSuccess == result)
            {
                result = UpdateIPEInternalData(pInputData);
            }

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Operation failed %d", result);
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
// IPEChromaEnhancement12::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEChromaEnhancement12::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEChromaEnhancement12::IPEChromaEnhancement12
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEChromaEnhancement12::IPEChromaEnhancement12(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier   = pNodeIdentifier;
    m_type              = ISPIQModuleType::IPEChromaEnhancement;
    m_moduleEnable      = TRUE;
    m_numLUT            = 0;
    m_offsetLUT         = 0;
    m_pChromatix        = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEChromaEnhancement12::~IPEChromaEnhancement12
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEChromaEnhancement12::~IPEChromaEnhancement12()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
    m_pChromatix = NULL;
    DeallocateCommonLibraryData();
}

CAMX_NAMESPACE_END
