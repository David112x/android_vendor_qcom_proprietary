////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifepdaf20.cpp
/// @brief CAMXIFEPDAF20 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcslifedefs.h"
#include "camxcslispdefs.h"
#include "camxdefs.h"
#include "camxifepdaf20titan480.h"
#include "camxifepdaf20.h"
#include "camxiqinterface.h"
#include "camxispiqmodule.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDAF20::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDAF20::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEPDAF20* pModule = CAMX_NEW IFEPDAF20;

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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFEPDAF20 object.");
        }
        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFEPDAF20 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDAF20::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDAF20::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFEPDAF20Titan480;
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
// IFEPDAF20::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDAF20::Execute(
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
// IFEPDAF20::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEPDAF20::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    // Update WM Config
    UINT32 wmUpdateIndex = pInputData->pCalculatedData->WMUpdate.numberOfWMUpdates;

    if ((NULL != pInputData) && (NULL != pInputData->pPDHwConfig))
    {
        if (ISPMaxOutputPorts > wmUpdateIndex)
        {
            if (FALSE == pInputData->pPDHwConfig->SADConfig.sadEnable)
            {
                pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].portID = IFEOutputDualPD;
                pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].width  = m_outputData.width;
                pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].height = m_outputData.height;
                pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].mode   = IFEWMModeLineBased;
                pInputData->pCalculatedData->WMUpdate.numberOfWMUpdates++;
            }
            else
            {
                pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].portID = IFEOutputDualPD;
                pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].width  = 0;
                pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].height = 0;
                pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].mode   = IFEWMModeFrameBased;
                pInputData->pCalculatedData->WMUpdate.numberOfWMUpdates++;
            }
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupIQMod, "Invalid Number of Ports %d", wmUpdateIndex);
        }
    }
    else
    {
        if (NULL != pInputData)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid pPDHWConfig %p", pInputData->pPDHwConfig);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid input Args pInputData %p", pInputData);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDAF20::GetRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* IFEPDAF20::GetRegCmd()
{
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDAF20::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEPDAF20::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if (NULL != pInputData->pOEMIQSetting)
    {
        pInputData->pPDHwConfig = &(static_cast<OEMIFEIQSetting*>(pInputData->pOEMIQSetting)->IFEPDHwConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "PD OEM IQ Confif, Module Enable %d", pInputData->pPDHwConfig->enablePDHw);

        m_moduleEnable  = pInputData->pPDHwConfig->enablePDHw;
        isChanged       = TRUE;
    }
    else if (NULL != pInputData->pPDHwConfig)
    {
        m_moduleEnable  = pInputData->pPDHwConfig->enablePDHw;
        isChanged       = TRUE;
    }

    return isChanged;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDAF20::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDAF20::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result       = CamxResultSuccess;
    VOID*      pSettingData = static_cast<VOID*>(pInputData);

    if ((NULL != pInputData) && (NULL != m_pHWSetting) && (NULL != pInputData->p32bitDMIBufferAddr))
    {
        m_outputData.pDMIDataPtr = static_cast<VOID*>(pInputData->p32bitDMIBufferAddr + m_32bitDMIBufferOffsetDword);
        result                   = m_pHWSetting->PackIQRegisterSetting(pSettingData, &m_outputData);

        if ((CamxResultSuccess == result) &&
            (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex)))
        {
            m_pHWSetting->DumpRegConfig();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDAF20::IFEPDAF20
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEPDAF20::IFEPDAF20()
{
    m_type = ISPIQModuleType::IFEPDAF;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEPDAF20::~IFEPDAF20
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEPDAF20::~IFEPDAF20()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
