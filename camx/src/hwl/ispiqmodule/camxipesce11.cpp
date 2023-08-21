////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipesce11.cpp
/// @brief IPE SCE class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxipesce11.h"
#include "camxipesce11titan17x.h"
#include "camxipesce11titan480.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "camxtitan17xcontext.h"
#include "ipe_data.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPESCE11::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPESCE11::Create(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        IPESCE11* pModule = CAMX_NEW IPESCE11(pCreateData->pNodeIdentifier);

        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (result != CamxResultSuccess)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to initilize common library data, no memory");
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
// IPESCE11::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPESCE11::Initialize(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    m_pHWSetting = NULL;

    // Check Module Hardware Version and Create HW Setting Object
    switch (static_cast<Titan17xContext*>(pCreateData->initializationData.pHwContext)->GetTitanVersion())
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IPESCE11Titan480;
            break;

        default:
            m_pHWSetting = CAMX_NEW IPESCE11Titan17x;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        UINT size = m_pHWSetting->GetRegSize();

        m_cmdLength = PacketBuilder::RequiredWriteRegRangeSizeInDwords(size / RegisterWidthInBytes) *sizeof(UINT32);
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
// IPESCE11::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPESCE11::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(sce_1_1_0::sce11_rgn_dataType) * (SCE11MaxNoLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for sce_1_1_0::sce11_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPESCE11::UpdateIPEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPESCE11::UpdateIPEInternalData(
    ISPInputData* pInputData
    ) const
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData->pipelineIPEData.pIPEIQSettings)
    {
        IpeIQSettings* pIPEIQSettings = reinterpret_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);

        pIPEIQSettings->skinEnhancementParameters.moduleCfg.EN = m_moduleEnable;

        // Post tuning metadata if setting is enabled
        if (NULL != pInputData->pIPETuningMetadata)
        {
            result = m_pHWSetting->UpdateTuningMetadata(pInputData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "Update Tuning Metadata failed");
                result = CamxResultSuccess; // Non-fatal error
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Input data");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPESCE11::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPESCE11::ValidateDependenceParams(
    const ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    /// @todo (CAMX-730) validate dependency parameters
    if ((NULL == pInputData)                                                   ||
        (NULL == pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferPostLTM]) ||
        (NULL == pInputData->pipelineIPEData.pIPEIQSettings))
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Input: pInputData %p", pInputData);

        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPESCE11::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPESCE11::CheckDependenceChange(
    const ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData)                  &&
        (NULL != pInputData->pAECUpdateData)  &&
        (NULL != pInputData->pAWBUpdateData)  &&
        (NULL != pInputData->pHwContext)      &&
        (NULL != pInputData->pHALTagsData))
    {
        AECFrameControl* pNewAECUpdate   = pInputData->pAECUpdateData;
        AWBFrameControl* pNewAWBUpdate   = pInputData->pAWBUpdateData;
        FLOAT            newExposureTime = static_cast<FLOAT>(pNewAECUpdate->exposureInfo[ExposureIndexSafe].exposureTime);

        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIPEIQSetting*>(pInputData->pOEMIQSetting))->SkinEnhancementEnable;

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
                    m_pChromatix = pTuningManager->GetChromatix()->GetModule_sce11_ipe(
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
                        m_moduleEnable              = m_pChromatix->enable_section.sce_enable;

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
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Tuning Pointer is NULL");
            }
        }

        // Check for manual tone map mode, if yes disable SCE
        if ((TRUE                     == m_moduleEnable)                          &&
            (TonemapModeContrastCurve == pInputData->pHALTagsData->tonemapCurves.tonemapMode))
        {
            m_moduleEnable = FALSE;
            isChanged      = FALSE;
        }

        if ((TRUE == m_moduleEnable) &&
            ((FALSE == Utils::FEqual(m_dependenceData.luxIndex, pNewAECUpdate->luxIndex))         ||
             (FALSE == Utils::FEqual(m_dependenceData.realGain,
                                     pNewAECUpdate->exposureInfo[ExposureIndexSafe].linearGain))  ||
             (FALSE == Utils::FEqual(m_dependenceData.AECSensitivity,
                                     pNewAECUpdate->exposureInfo[ExposureIndexSafe].sensitivity)) ||
             (FALSE == Utils::FEqual(m_dependenceData.exposureTime, newExposureTime))))

        {
            m_dependenceData.luxIndex       = pNewAECUpdate->luxIndex;
            m_dependenceData.realGain       = pNewAECUpdate->exposureInfo[ExposureIndexSafe].linearGain;
            m_dependenceData.AECSensitivity = pNewAECUpdate->exposureInfo[ExposureIndexSafe].sensitivity;
            m_dependenceData.exposureTime   = newExposureTime;

            if (FALSE == Utils::FEqual(pNewAECUpdate->exposureInfo[ExposureIndexShort].sensitivity, 0.0f))
            {
                m_dependenceData.DRCGain = pNewAECUpdate->exposureInfo[ExposureIndexSafe].sensitivity /
                    pNewAECUpdate->exposureInfo[ExposureIndexShort].sensitivity;
            }
            else
            {
                m_dependenceData.DRCGain = 0.0f;
                CAMX_LOG_WARN(CamxLogGroupPProc, "Invalid short exposure, cannot obtain DRC gain");
            }

            isChanged = TRUE;
        }
    }
    else
    {
        if (NULL != pInputData)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc,
                           "Invalid Input: pNewAECUpdate %x pNewAWBUpdate %x  HwContext %x",
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
// IPESCE11::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPESCE11::RunCalculation(
    const ISPInputData* pInputData)
{
    CamxResult      result      = CamxResultSuccess;
    SCE11OutputData outputData;

    result = IQInterface::SCE11CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "SCE11 Calculation Failed. result %d", result);
    }

    if (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod))
    {
        m_pHWSetting->DumpRegConfig();
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPESCE11::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPESCE11::Execute(
    ISPInputData* pInputData)
{
    CamxResult result;

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
// IPESCE11::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPESCE11::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPESCE11::IPESCE11
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPESCE11::IPESCE11(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier   = pNodeIdentifier;
    m_type              = ISPIQModuleType::IPESCE;
    m_moduleEnable      = TRUE;
    m_numLUT            = 0;
    m_offsetLUT         = 0;
    m_pChromatix        = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPESCE11::~IPESCE11
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPESCE11::~IPESCE11()
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
