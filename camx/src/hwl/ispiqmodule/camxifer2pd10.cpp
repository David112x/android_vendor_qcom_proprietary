////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifer2pd10.cpp
/// @brief CAMXIFER2PD class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifer2pd10titan17x.h"
#include "camxifer2pd10.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFER2PD10::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFER2PD10::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFER2PD10* pModule = CAMX_NEW IFER2PD10(pCreateData);

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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFER2PD10 object.");
        }
        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFER2PD10 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFER2PD10::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFER2PD10::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFER2PD10Titan17x;
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
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class");
        m_cmdLength = 0;
        result      = CamxResultENoMemory;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFER2PD10::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFER2PD10::CheckDependenceChange(
    ISPInputData* pInputData)
{
    CAMX_UNREFERENCED_PARAM(pInputData);
    /// @todo (CAMX-666) Update bitwidth based on HAL request and check for update
    m_bitWidth = BitWidthTen;

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFER2PD10::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFER2PD10::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    switch (m_modulePath)
    {
        case IFEPipelinePath::VideoDS4Path:
            pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS4R2PDEnable = TRUE;
            break;

        case IFEPipelinePath::VideoDS16Path:
            pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS16R2PDEnable = TRUE;
            break;

        case IFEPipelinePath::DisplayDS4Path:
            pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS4R2PDEnable = TRUE;
            break;

        case IFEPipelinePath::DisplayDS16Path:
            pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS16R2PDEnable = TRUE;
            break;

        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid module path");
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFER2PD10::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFER2PD10::Execute(
    ISPInputData* pInputData)
{
    CamxResult result  = CamxResultSuccess;
    VOID* pSettingData = static_cast<VOID*>(pInputData);

    if ((NULL != pInputData))
    {
        // Check if dependent has been updated compared to last request
        if (TRUE == CheckDependenceChange(pInputData))
        {
            result = RunCalculation(pInputData);
            if (CamxResultSuccess == result)
            {
                result = m_pHWSetting->CreateCmdList(pSettingData, &m_32bitDMIBufferOffsetDword);
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
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFER2PD10::GetRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* IFER2PD10::GetRegCmd()
{
    return NULL;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFER2PD10::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFER2PD10::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result       = CamxResultSuccess;
    VOID*  pSettingData     = static_cast<VOID*>(pInputData);
    UINT32 packMode         = 0;
    R2PD10OutputData outputData;

    /// @todo (CAMX-666) Update packmode based on HAL request and check for update
    if (BitWidthEight == m_bitWidth)
    {
        packMode = 1;
    }

    outputData.modulePath = m_modulePath;
    outputData.packMode   = packMode;

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        result = m_pHWSetting->PackIQRegisterSetting(pSettingData, &outputData);

        if ((CamxResultSuccess == result) &&
            (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod)))
        {
            m_pHWSetting->DumpRegConfig();
        }
    }


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFER2PD10::IFER2PD10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFER2PD10::IFER2PD10(
    IFEModuleCreateData* pCreateData)
{
    m_type           = ISPIQModuleType::IFER2PD;
    m_moduleEnable   = TRUE;
    m_modulePath     = pCreateData->pipelineData.IFEPath;
    m_32bitDMILength = 0;
    m_64bitDMILength = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFER2PD10::~IFER2PD10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFER2PD10::~IFER2PD10()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
