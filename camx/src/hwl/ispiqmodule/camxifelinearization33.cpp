////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifelinearization33.cpp
/// @brief CAMXIFELinearization33 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxifelinearization33titan17x.h"
#include "camxifelinearization33.h"
#include "camxiqinterface.h"
#include "camxisphwsetting.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "linearization_3_3_0.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization33::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELinearization33::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFELinearization33* pModule = CAMX_NEW IFELinearization33;

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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFELinearization33 object.");
        }
        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFELinearization33 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization33::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELinearization33::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFELinearization33Titan17x;
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
        m_64bitDMILength            = m_pHWSetting->Get64bitDMILength();
        result = AllocateCommonLibraryData();
        if (result != CamxResultSuccess)
        {
            CAMX_DELETE m_pHWSetting;
            m_pHWSetting     = NULL;
            m_cmdLength      = 0;
            m_64bitDMILength = 0;
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
// IFELinearization33::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELinearization33::Execute(
    ISPInputData* pInputData)
{
    CamxResult result  = CamxResultSuccess;
    VOID* pSettingData = static_cast<VOID*>(pInputData);

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            if (TRUE == pInputData->bankUpdate.isValid)
            {
                m_dependenceData.lutBankSel = pInputData->bankUpdate.linearizaionBank;
            }

            result = RunCalculation(pInputData);
            if (CamxResultSuccess == result)
            {
                result = m_pHWSetting->CreateCmdList(pSettingData, &m_64bitDMIBufferOffsetDword);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Linearization module calculation Failed.");
            }
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
// IFELinearization33::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFELinearization33::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    // Update metdata
    pInputData->pCalculatedData->blackLevelLock                    = m_blacklevelLock;
    pInputData->pCalculatedMetadata->linearizationAppliedBlackLevel[0] = m_dynamicBlackLevel[0];
    pInputData->pCalculatedMetadata->linearizationAppliedBlackLevel[1] = m_dynamicBlackLevel[1];
    pInputData->pCalculatedMetadata->linearizationAppliedBlackLevel[2] = m_dynamicBlackLevel[2];
    pInputData->pCalculatedMetadata->linearizationAppliedBlackLevel[3] = m_dynamicBlackLevel[3];

    // Update module enable info
    pInputData->pCalculatedData->moduleEnable.IQModules.liniearizationEnable = m_moduleEnable;

    // Update stretch Gains
    pInputData->pCalculatedMetadata->stretchGainBlue      = m_stretchGainBlue;
    pInputData->pCalculatedMetadata->stretchGainGreenEven = m_stretchGainGreenEven;
    pInputData->pCalculatedMetadata->stretchGainGreenOdd  = m_stretchGainGreenOdd;
    pInputData->pCalculatedMetadata->stretchGainRed       = m_stretchGainRed;

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
// IFELinearization33::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELinearization33::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(linearization_3_3_0::linearization33_rgn_dataType) *
                             (Linearization33MaxmiumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for linearization_3_3_0::linearization33_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization33::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFELinearization33::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged      = FALSE;
    BOOL dynamicEnable  = TRUE;

    if ((NULL != pInputData)                 &&
        (NULL != pInputData->pHwContext)     &&
        (NULL != pInputData->pAECUpdateData) &&
        (NULL != pInputData->pAWBUpdateData) &&
        (NULL != pInputData->pHALTagsData))
    {
        dynamicEnable = Utils::IsBitSet(pInputData->IFEDynamicEnableMask,
                                             static_cast<UINT>(DynamicIFEMaskBit::IFELinearization));

        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIFEIQSetting*>(pInputData->pOEMIQSetting))->LinearizationEnable;
            m_dependenceData.pedestalEnable = (static_cast<OEMIFEIQSetting*>(pInputData->pOEMIQSetting))->PedestalEnable;

            if (TRUE == m_moduleEnable)
            {
                isChanged = TRUE;
            }
        }
        else
        {
            m_blacklevelLock = pInputData->pHALTagsData->blackLevelLock;
            m_AWBLock        = pInputData->pHALTagsData->controlAWBLock;

            if (((pInputData->pHALTagsData->blackLevelLock == m_blacklevelLock) &&
                 (pInputData->pHALTagsData->blackLevelLock == BlackLevelLockOn)) ||
                ((pInputData->pHALTagsData->controlAWBLock == m_AWBLock) &&
                 (pInputData->pHALTagsData->controlAWBLock == ControlAWBLockOn)))
            {
                isChanged = FALSE;
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

                    m_pChromatix = pTuningManager->GetChromatix()->GetModule_linearization33_ife(
                        reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                        pInputData->pTuningData->noOfSelectionParameter);
                }

                CAMX_ASSERT(NULL != m_pChromatix);
                if (NULL != m_pChromatix)
                {
                    if ((NULL                        == m_dependenceData.pChromatix)                        ||
                        (m_pChromatix->SymbolTableID != m_dependenceData.pChromatix->SymbolTableID)         ||
                        (m_moduleEnable              != m_pChromatix->enable_section.linearization_enable))
                    {
                        m_dependenceData.pChromatix      = m_pChromatix;
                        m_moduleEnable                   = m_pChromatix->enable_section.linearization_enable;
                        m_dependenceData.pedestalEnable  =
                            pInputData->pCalculatedData->moduleEnable.IQModules.pedestalEnable;
                        m_dependenceData.symbolIDChanged = TRUE;

                        if (TRUE == m_moduleEnable)
                        {
                            isChanged = TRUE;
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
                    m_dependenceData.pedestalEnable =
                       pInputData->pCalculatedData->moduleEnable.IQModules.pedestalEnable;

                    if ((TRUE == IQInterface::s_interpolationTable.IFELinearization33TriggerUpdate(&pInputData->triggerData,
                                                                                                  &m_dependenceData)) ||
                        (TRUE == pInputData->forceTriggerUpdate))
                    {
                        isChanged = TRUE;
                    }
                }
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Pointer");
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization33::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELinearization33::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult                result     = CamxResultSuccess;
    UINT32*                   pDMIAddr   = NULL;
    Linearization33OutputData outputData = {0, };

    if ((NULL != pInputData)                      &&
        (NULL != pInputData->p64bitDMIBufferAddr) &&
        (NULL != m_pHWSetting))
    {
        pDMIAddr = pInputData->p64bitDMIBufferAddr + m_64bitDMIBufferOffsetDword;

        CAMX_ASSERT(NULL != pDMIAddr);
        /// @todo (CAMX-1791) Revisit this function to see if it needs to act differently in the Dual IFE case

        outputData.pDMIDataPtr      = reinterpret_cast<UINT64*>(pDMIAddr);
        outputData.registerBETEn    = pInputData->registerBETEn;
        m_dependenceData.lutBankSel = m_bankSelect;
        m_bankSelect               ^= 1;

        result = IQInterface::IFELinearization33CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);

        m_stretchGainBlue      = outputData.stretchGainBlue;
        m_stretchGainRed       = outputData.stretchGainRed;
        m_stretchGainGreenEven = outputData.stretchGainGreenEven;
        m_stretchGainGreenOdd  = outputData.stretchGainGreenOdd;
        m_dynamicBlackLevel[0] = outputData.dynamicBlackLevel[0];
        m_dynamicBlackLevel[1] = outputData.dynamicBlackLevel[1];
        m_dynamicBlackLevel[2] = outputData.dynamicBlackLevel[2];
        m_dynamicBlackLevel[3] = outputData.dynamicBlackLevel[3];
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Linearization Calculation Failed. result %d", result);
        }

        if ((CamxResultSuccess == result) &&
            (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod)))
        {
            m_pHWSetting->DumpRegConfig();
        }
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Null Input Data");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization33::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFELinearization33::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization33::IFELinearization33
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFELinearization33::IFELinearization33()
{
    m_type           = ISPIQModuleType::IFELinearization;
    m_32bitDMILength = 0;
    m_moduleEnable   = TRUE;
    m_bankSelect     = 0;
    m_pChromatix     = NULL;
    m_AWBLock        = ControlAWBLockOff;
    m_blacklevelLock = BlackLevelLockOff;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFELinearization33::~IFELinearization33
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFELinearization33::~IFELinearization33()
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
