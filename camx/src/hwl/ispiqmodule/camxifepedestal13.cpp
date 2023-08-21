////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifepedestal13.cpp
/// @brief CAMXIFEPEDESTAL13 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxifepedestal13titan17x.h"
#include "camxifepedestal13titan480.h"
#include "camxifepedestal13.h"
#include "camxiqinterface.h"
#include "camxisphwsetting.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"
#include "pedestal_1_3_0.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPedestal13::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPedestal13::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEPedestal13* pModule = CAMX_NEW IFEPedestal13;

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
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFEPedestal13 object.");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFEPedestal13 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPedestal13::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPedestal13::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFEPedestal13Titan480;
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFEPedestal13Titan17x;
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
        m_32bitDMILength            = m_pHWSetting->Get32bitDMILength();
        result                      = AllocateCommonLibraryData();
        if (CamxResultSuccess != result)
        {
            CAMX_DELETE m_pHWSetting;
            m_pHWSetting     = NULL;
            m_cmdLength      = 0;
            m_32bitDMILength = 0;
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
// IFEPedestal13::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPedestal13::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(pedestal_1_3_0::pedestal13_rgn_dataType) * (Pedestal13MaxmiumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for pedestal_1_3_0::pedestal13_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPedestal13::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPedestal13::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;
    VOID* pSettingData = static_cast<VOID*>(pInputData);

    if (NULL != pInputData && (NULL != m_pHWSetting))
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            result = RunCalculation(pInputData);
            if (CamxResultSuccess == result)
            {
                result = m_pHWSetting->CreateCmdList(pSettingData, &m_32bitDMIBufferOffsetDword);
            }
        }

        if (FALSE == m_moduleEnable)
        {
            result = m_pHWSetting->SetupRegisterSetting(&m_moduleEnable);

            if (CamxResultSuccess == result)
            {
                result = m_pHWSetting->CreateSubCmdList(pSettingData, &m_32bitDMIBufferOffsetDword);
            }
        }

        if (CamxResultSuccess == result)
        {
            UpdateIFEInternalData(pInputData);
        }
        else
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Operation failed %d", result);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Input: pInputData %p, m_pHWSetting %p", pInputData, m_pHWSetting);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPedestal13::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEPedestal13::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    pInputData->pCalculatedData->moduleEnable.IQModules.pedestalEnable = m_moduleEnable;
    pInputData->pCalculatedData->blackLevelLock                        = m_blacklevelLock;

    // Post tuning metadata if setting is enabled
    if (NULL != pInputData->pIFETuningMetadata)
    {
        if (CamxResultSuccess != m_pHWSetting->UpdateTuningMetadata(pInputData->pIFETuningMetadata))
        {
            CAMX_LOG_WARN(CamxLogGroupISP, "UpdateTuningMetadata failed.");
        }
    }

    if (NULL != pInputData->pStripingInput)
    {
        pInputData->pStripingInput->enableBits.pedestal = m_moduleEnable;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPedestal13::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEPedestal13::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;
    BOOL dynamicEnable = Utils::IsBitSet(pInputData->IFEDynamicEnableMask,
                                         static_cast<UINT>(DynamicIFEMaskBit::IFEPedestalCorrection));

    if ((NULL != pInputData->pAECUpdateData) &&
        (NULL != pInputData->pHwContext)     &&
        (NULL != pInputData->pAWBUpdateData) &&
        (NULL != pInputData->pHALTagsData))
    {
        m_blacklevelLock = pInputData->pHALTagsData->blackLevelLock;
        if (BlackLevelLockOn == m_blacklevelLock)
        {
            isChanged = FALSE;
        }
        else
        {
            if (NULL != pInputData->pOEMIQSetting)
            {
                m_moduleEnable = (static_cast<OEMIFEIQSetting*>(pInputData->pOEMIQSetting))->PedestalEnable;

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

                    m_pChromatix = pTuningManager->GetChromatix()->GetModule_pedestal13_ife(
                                       reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                       pInputData->pTuningData->noOfSelectionParameter);
                }

                CAMX_ASSERT(NULL != m_pChromatix);
                if (NULL != m_pChromatix)
                {
                    if ((NULL                        == m_dependenceData.pChromatix)                   ||
                        (m_pChromatix->SymbolTableID != m_dependenceData.pChromatix->SymbolTableID)    ||
                        (m_moduleEnable              != m_pChromatix->enable_section.pedestal_enable))
                    {
                        m_dependenceData.pChromatix = m_pChromatix;
                        m_moduleEnable              = m_pChromatix->enable_section.pedestal_enable;
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
                    IQInterface::s_interpolationTable.pedestal13TriggerUpdate(&pInputData->triggerData, &m_dependenceData)) ||
                    (TRUE == pInputData->forceTriggerUpdate))
                {
                    isChanged = TRUE;
                }
            }
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
// IFEPedestal13::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPedestal13::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;
    /// @todo (CAMX-1308) This is a reduced version of Execute. Revisit when things settle to see if they should be merged.
    if (NULL != pInputData)
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            result = RunCalculation(pInputData);

            if (CamxResultSuccess == result)
            {
                UpdateIFEInternalData(pInputData);
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("LSC module calculation Failed.");
            }
        }
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Null Input Data");
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPedestal13::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPedestal13::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result               = CamxResultSuccess;
    UINT32*    pPedestalDMIAddr     = reinterpret_cast<UINT32*>(pInputData->p32bitDMIBufferAddr +
                                                      m_32bitDMIBufferOffsetDword               +
                                                      (pInputData->pStripeConfig->stripeId * IFEPedestal13DMILengthDword));
    Pedestal13OutputData outputData = {};
    outputData.type                 = PipelineType::IFE;
    outputData.pGRRLUTDMIBuffer     = pPedestalDMIAddr;
    outputData.pGBBLUTDMIBuffer     = reinterpret_cast<UINT32*>((reinterpret_cast<UCHAR*>(outputData.pGRRLUTDMIBuffer) +
                                                                 IFEPedestal13LUTTableSize));
    m_dependenceData.LUTBankSel    ^= 1;

    result = IQInterface::Pedestal13CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Pedestal Calculation Failed.");
    }

    if (NULL != pInputData->pStripingInput)
    {
        pInputData->pStripingInput->enableBits.pedestal                        = m_moduleEnable;
        pInputData->pStripingInput->stripingInput.pedestalParam.enable         = m_moduleEnable;
        pInputData->pStripingInput->stripingInput.pedestalParam.blockWidth     = outputData.pedState.bwidth_l;
        pInputData->pStripingInput->stripingInput.pedestalParam.bx_d1_l        = outputData.pedState.bx_d1_l;
        pInputData->pStripingInput->stripingInput.pedestalParam.bxStart        = outputData.pedState.bx_start_l;
        pInputData->pStripingInput->stripingInput.pedestalParam.lxStart        = outputData.pedState.lx_start_l;
        pInputData->pStripingInput->stripingInput.pedestalParam.meshGridBwidth = outputData.pedState.meshGridBwidth_l;
    }

    if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
    {
        m_pHWSetting->DumpRegConfig();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPedestal13::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEPedestal13::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPedestal13::IFEPedestal13
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEPedestal13::IFEPedestal13()
{
    m_type              = ISPIQModuleType::IFEPedestalCorrection;
    m_moduleEnable      = TRUE;
    m_pChromatix        = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEPedestal13::~IFEPedestal13
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEPedestal13::~IFEPedestal13()
{
    m_pChromatix = NULL;
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
    DeallocateCommonLibraryData();
}

CAMX_NAMESPACE_END
