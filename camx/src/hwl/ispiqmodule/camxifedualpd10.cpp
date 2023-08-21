////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifedualpd10.cpp
/// @brief CAMXIFEDUALPD10 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxifedualpd10titan17x.h"
#include "camxifedualpd10.h"
#include "camxiqinterface.h"
#include "camxispiqmodule.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDUALPD10::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDUALPD10::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEDUALPD10* pModule = CAMX_NEW IFEDUALPD10;

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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFEDUALPD10 object.");
        }
        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFEDUALPD10 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDUALPD10::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDUALPD10::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFEDualPD10Titan17x;
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupISP, "Not supported Titan 0x%x", pCreateData->titanVersion);
            result = CamxResultEUnsupported;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        m_cmdLength      = m_pHWSetting->GetCommandLength();
        m_32bitDMILength = m_pHWSetting->Get32bitDMILength();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class, Titan 0x%x", pCreateData->titanVersion);
        result           = CamxResultENoMemory;
        m_cmdLength      = 0;
        m_32bitDMILength = 0;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDUALPD10::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDUALPD10::Execute(
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
                CAMX_ASSERT_ALWAYS_MESSAGE("Dual PD module calculation Failed.");
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
// IFEDUALPD10::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDUALPD10::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    pInputData->pCalculatedData->moduleEnable.IQModules.PDAFPathEnable = m_moduleEnable;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDUALPD10::GetRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* IFEDUALPD10::GetRegCmd()
{
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDUALPD10::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEDUALPD10::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if (NULL != pInputData->pPDHwConfig)
    {
        m_moduleEnable  = pInputData->pPDHwConfig->enablePDHw;
        isChanged       = TRUE;
    }

    return isChanged;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDUALPD10::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDUALPD10::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result       = CamxResultSuccess;
    VOID*      pSettingData = static_cast<VOID*>(pInputData);
    DualPD10OutputData outputData;

    if ((NULL != pInputData) && (NULL != m_pHWSetting) && (NULL != pInputData->p32bitDMIBufferAddr))
    {
        outputData.pDMIDataPtr = pInputData->p32bitDMIBufferAddr + m_32bitDMIBufferOffsetDword;
        result                 = m_pHWSetting->PackIQRegisterSetting(pSettingData, &outputData);

        if ((CamxResultSuccess == result) &&
            (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod)))
        {
            m_pHWSetting->DumpRegConfig();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDUALPD10::IFEDUALPD10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDUALPD10::IFEDUALPD10()
{
    m_type                      = ISPIQModuleType::IFEDUALPD;
    m_64bitDMILength            = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEDUALPD10::~IFEDUALPD10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDUALPD10::~IFEDUALPD10()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
