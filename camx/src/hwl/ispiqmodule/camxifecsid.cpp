////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecsid.cpp
/// @brief CAMXIFECSID class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxifecsidrdititan480.h"

#include "camxifecsid.h"
#include "camxiqinterface.h"
#include "camxisphwsetting.h"
#include "camxispiqmodule.h"
#include "camxtuningdatamanager.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSID::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECSID::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFECSID* pModule = CAMX_NEW IFECSID;

        if (NULL != pModule)
        {
            pModule->m_type = pCreateData->pipelineData.IFEModuleType;
            pModule->m_path = pCreateData->pipelineData.IFEPath;
            result = pModule->Initialize(pCreateData);
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_DELETE pModule;
            pModule = NULL;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFECSID object.");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFECSID Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSID::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECSID::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            switch (m_path)
            {
                case IFEPipelinePath::RDI0Path:
                case IFEPipelinePath::RDI1Path:
                case IFEPipelinePath::RDI2Path:
                    m_pHWSetting = CAMX_NEW IFECSIDRDITitan480(m_path);
                    break;
                default:
                    CAMX_LOG_ERROR(CamxLogGroupISP, "Unsupported CSID RDI Path %d", m_path);
                    break;
            }
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupISP, "Not supported Titan 0x%x", pCreateData->titanVersion);
            result = CamxResultEUnsupported;
    }

    if (NULL == m_pHWSetting)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class");
        result = CamxResultENoMemory;
    }
    else
    {
        m_cmdLength = m_pHWSetting->GetCommandLength();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSID::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECSID::Execute(
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
                result = m_pHWSetting->CreateCmdList(pSettingData, NULL);
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("CSID module calculation Failed.");
            }
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
// IFECSID::GetRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* IFECSID::GetRegCmd()
{
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSID::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFECSID::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL                   isChanged     = FALSE;
    IFECSIDExtractionInfo* pCSIDDropInfo = NULL;

    if ((NULL != pInputData)                                    &&
        (NULL != pInputData->pStripeConfig)                     &&
        (NULL != pInputData->pStripeConfig->pCSIDSubsampleInfo))
    {

        if (TRUE == pInputData->isInitPacket)
        {
            switch (m_path)
            {
                case IFEPipelinePath::RDI0Path:
                    pCSIDDropInfo = &pInputData->pStripeConfig->pCSIDSubsampleInfo[IFECSIDRDI0];
                    break;
                case IFEPipelinePath::RDI1Path:
                    pCSIDDropInfo = &pInputData->pStripeConfig->pCSIDSubsampleInfo[IFECSIDRDI1];
                    break;
                case IFEPipelinePath::RDI2Path:
                    pCSIDDropInfo = &pInputData->pStripeConfig->pCSIDSubsampleInfo[IFECSIDRDI2];
                    break;
                default:
                    break;
            }
            if ((NULL != pCSIDDropInfo) && (TRUE == pCSIDDropInfo->enableCSIDSubsample))
            {
                CAMX_LOG_INFO(CamxLogGroupISP, "Path %d CSIDDBG Submsample Enable %d",
                    m_path, pCSIDDropInfo->enableCSIDSubsample);
                isChanged = TRUE;
            }
        }
    }
    else
    {
        if (NULL != pInputData)
        {
            if (NULL != pInputData->pStripeConfig)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid pCSIDSubsampleInfo %p",
                    pInputData->pStripeConfig->pCSIDSubsampleInfo);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid pStripeConfig %p",
                    pInputData->pStripeConfig);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Args pInputData %p ", pInputData);
        }
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSID::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECSID::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    VOID* pSettingData = static_cast<VOID*>(pInputData);
    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        if (CamxResultSuccess == result)
        {
            result = m_pHWSetting->PackIQRegisterSetting(pSettingData, NULL);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Fail to SetupRegisterSetting");
        }

        if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
        {
            m_pHWSetting->DumpRegConfig();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSID::IFECSID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECSID::IFECSID()
{
    m_type           = ISPIQModuleType::IFECSID;
    m_cmdLength      = 0;
    m_32bitDMILength = 0;
    m_64bitDMILength = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFECSID::~IFECSID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECSID::~IFECSID()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE static_cast<IFECSIDRDITitan480*>(m_pHWSetting);
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
