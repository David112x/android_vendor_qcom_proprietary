////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecamiflite.cpp
/// @brief CAMXIFECAMIF class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "camxdefs.h"
#include "camxifecamiflitetitan17x.h"
#include "camxifecamiflite.h"
#include "camxiqinterface.h"
#include "camxisphwsetting.h"
#include "camxispiqmodule.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"
#include "camxnode.h"


CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFLite::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIFLite::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFECAMIFLite* pModule = CAMX_NEW IFECAMIFLite;

        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Module initialization failed");
                CAMX_DELETE pModule;
                pModule = NULL;
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFECAMIFLite object.");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFECAMIFLite Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFLite::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIFLite::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFECAMIFLiteTitan17x;
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupISP, "Not supported Titan 0x%x", pCreateData->titanVersion);
            result = CamxResultEUnsupported;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        m_cmdLength = m_pHWSetting->GetCommandLength();

    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class, Titan 0x%x", pCreateData->titanVersion);
        m_cmdLength  = 0;
        result       = CamxResultENoMemory;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFLite::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIFLite::Execute(
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
                CAMX_ASSERT_ALWAYS_MESSAGE("CAMIFLite module calculation Failed.");
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
// IFECAMIFLite::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFECAMIFLite::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL  isChanged = FALSE;

    if (NULL != pInputData)
    {
        if (TRUE  == pInputData->dualPDPipelineData.programCAMIFLite)
        {
            isChanged = TRUE;
        }
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFLite::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIFLite::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    VOID* pSettingData = static_cast<VOID*>(pInputData);
    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        result = m_pHWSetting->SetupRegisterSetting(pSettingData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFLite::IFECAMIFLite
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECAMIFLite::IFECAMIFLite()
{
    m_type           = ISPIQModuleType::IFECAMIFLite;
    m_32bitDMILength = 0;
    m_64bitDMILength = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFECAMIFLite::~IFECAMIFLite
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECAMIFLite::~IFECAMIFLite()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
