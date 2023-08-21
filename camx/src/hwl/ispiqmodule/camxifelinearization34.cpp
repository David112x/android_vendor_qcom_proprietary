////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifelinearization34.cpp
/// @brief CAMXIFELinearization34 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxifelinearization34titan480.h"
#include "camxifelinearization34.h"
#include "camxiqinterface.h"
#include "camxisphwsetting.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "linearization_3_4_0.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization34::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELinearization34::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFELinearization34* pModule = CAMX_NEW IFELinearization34;

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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFELinearization34 object.");
        }
        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFELinearization34 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization34::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELinearization34::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFELinearization34Titan480;
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
        result = AllocateCommonLibraryData();
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
// IFELinearization34::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELinearization34::Execute(
    ISPInputData* pInputData)
{
    CamxResult result  = CamxResultSuccess;
    VOID* pSettingData = static_cast<VOID*>(pInputData);

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            result = RunCalculation(pInputData);
            if (CamxResultSuccess == result)
            {
                result = m_pHWSetting->CreateCmdList(pSettingData, &m_32bitDMIBufferOffsetDword);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Linearization module calculation Failed.");
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
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Input: pInputData %p m_pHWSetting %p", pInputData, m_pHWSetting);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization34::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFELinearization34::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    // Update metdata
    pInputData->pCalculatedData->blackLevelLock                        = m_blacklevelLock;
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
// IFELinearization34::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELinearization34::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(linearization_3_4_0::linearization34_rgn_dataType) *
                             (Linearization34MaxmiumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for linearization_3_4_0::linearization34_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization34::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFELinearization34::CheckDependenceChange(
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

            isChanged      = (TRUE == m_moduleEnable);
        }
        else
        {
            m_blacklevelLock = pInputData->pHALTagsData->blackLevelLock;
            m_AWBLock        = pInputData->pHALTagsData->controlAWBLock;

            if (((pInputData->pHALTagsData->blackLevelLock == m_blacklevelLock)  &&
                 (pInputData->pHALTagsData->blackLevelLock == BlackLevelLockOn)) ||
                ((pInputData->pHALTagsData->controlAWBLock == m_AWBLock)         &&
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

                    m_pChromatix = pTuningManager->GetChromatix()->GetModule_linearization34_ife(
                        reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                        pInputData->pTuningData->noOfSelectionParameter);
                }

                CAMX_ASSERT(NULL != m_pChromatix);
                if (NULL != m_pChromatix)
                {
                    if ((NULL                           == m_dependenceData.pChromatix)                         ||
                        (m_pChromatix->SymbolTableID    != m_dependenceData.pChromatix->SymbolTableID)          ||
                        (m_moduleEnable                 != m_pChromatix->enable_section.linearization_enable))
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
                if ((TRUE == m_moduleEnable) && (m_dynamicEnable!=dynamicEnable))
                {
                    isChanged = TRUE;
                }
                m_dynamicEnable = dynamicEnable;

                if (TRUE == m_moduleEnable)
                {
                    m_dependenceData.pedestalEnable =
                       pInputData->pCalculatedData->moduleEnable.IQModules.pedestalEnable;

                    // Update bayer pattern from sensor format
                    if (CamxResultSuccess != IQInterface::GetPixelFormat(&pInputData->sensorData.format,
                                                                         &(m_dependenceData.bayerPattern)))
                    {
                        CAMX_LOG_ERROR(CamxLogGroupISP, "Unsupported sensor format : %d", pInputData->sensorData.format);
                        // Disable Linearization if sensor format is not supported
                        isChanged = FALSE;
                    }

                    if ((TRUE == IQInterface::s_interpolationTable.Linearization34TriggerUpdate(&pInputData->triggerData,
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
// IFELinearization34::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELinearization34::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult                result     = CamxResultSuccess;
    UINT32*                   pDMIAddr   = NULL;
    Linearization34OutputData outputData = {0, };

    if ((NULL != pInputData)                      &&
        (NULL != pInputData->p32bitDMIBufferAddr) &&
        (NULL != m_pHWSetting))
    {
        pDMIAddr = pInputData->p32bitDMIBufferAddr + m_32bitDMIBufferOffsetDword;

        CAMX_ASSERT(NULL != pDMIAddr);
        /// @todo (CAMX-1791) Revisit this function to see if it needs to act differently in the Dual IFE case

        outputData.pDMIDataPtr      = pDMIAddr;

        if (TRUE == pInputData->bankUpdate.isValid)
        {
            m_dependenceData.LUTBankSel = pInputData->bankUpdate.linearizaionBank;
        }

        result = IQInterface::IFELinearization34CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);

        m_dependenceData.LUTBankSel ^= 1;

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
            (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex)))
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
// IFELinearization34::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFELinearization34::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization34::IFELinearization34
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFELinearization34::IFELinearization34()
{
    m_type           = ISPIQModuleType::IFELinearization;
    m_64bitDMILength = 0;
    m_moduleEnable   = TRUE;
    m_pChromatix     = NULL;
    m_AWBLock        = ControlAWBLockOff;
    m_blacklevelLock = BlackLevelLockOff;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFELinearization34::~IFELinearization34
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFELinearization34::~IFELinearization34()
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
