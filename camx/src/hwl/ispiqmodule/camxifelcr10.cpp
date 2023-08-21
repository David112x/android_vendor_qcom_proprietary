////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifelcr10.cpp
/// @brief CAMXIFELCR10 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcslifedefs.h"
#include "camxcslispdefs.h"
#include "camxdefs.h"
#include "camxifelcr10titan480.h"
#include "camxifelcr10.h"
#include "camxiqinterface.h"
#include "camxispiqmodule.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELCR10::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELCR10::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFELCR10* pModule = CAMX_NEW IFELCR10;

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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFELCR10 object.");
        }
        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFELCR10 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELCR10::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELCR10::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFELCR10Titan480;
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
// IFELCR10::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELCR10::Execute(
    ISPInputData* pInputData)
{
    CamxResult result  = CamxResultSuccess;
    VOID* pSettingData = static_cast<VOID*>(pInputData);

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        result = ValidateDependenceParams(pInputData);

        if (CamxResultSuccess == result)
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
                    CAMX_LOG_ERROR(CamxLogGroupISP, "Dual PD module calculation Failed.");
                }
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
// IFELCR10::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFELCR10::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    // Update WM Config
    UINT32 wmUpdateIndex = pInputData->pCalculatedData->WMUpdate.numberOfWMUpdates;

    if (ISPMaxOutputPorts > wmUpdateIndex)
    {
        pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].portID = IFEOutputLCR;
        pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].width  = m_outputData.width;
        pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].height = m_outputData.height;
        pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].mode   = IFEWMModeLineBased;
        pInputData->pCalculatedData->WMUpdate.numberOfWMUpdates++;
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "Invalid Number of Ports %d", wmUpdateIndex);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELCR10::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELCR10::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != pInputData->pPDHwConfig) && (NULL != pInputData->pStripingInput))
    {

        result = ValidateDependenceParams(pInputData);

        if (CamxResultSuccess == result)
        {
            if (TRUE == CheckDependenceChange(pInputData))
            {
                result = RunCalculation(pInputData);
            }
        }

        if (CamxResultSuccess == result)
        {
            pInputData->pStripingInput->stripingInput.LCREnable    =
                pInputData->pPDHwConfig->LCRConfig.enable;
            pInputData->pStripingInput->stripingInput.LCROutWidth =
                m_outputData.width;
            pInputData->pStripingInput->stripingInput.LCROutHeight =
                m_outputData.height;
            pInputData->pStripingInput->stripingInput.LCRInput.blockWidth = 2;
            pInputData->pStripingInput->stripingInput.LCRInput.firstPDCol = 0;
            pInputData->pStripingInput->stripingInput.LCRInput.firstPixel =
                pInputData->pPDHwConfig->LCRConfig.crop.firstPixel;
            pInputData->pStripingInput->stripingInput.LCRInput.lastPixel =
                pInputData->pPDHwConfig->LCRConfig.crop.lastPixel;
        }

    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Params pInput %p", pInputData);
        if (NULL != pInputData)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Params pPDHWconfig %p pStripingInput %p",
                           pInputData->pPDHwConfig, pInputData->pStripingInput);
        }
        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELCR10::GetRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* IFELCR10::GetRegCmd()
{
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELCR10::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFELCR10::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if (NULL != pInputData->pPDHwConfig)
    {
        m_moduleEnable = pInputData->pPDHwConfig->LCRConfig.enable;
        isChanged       = TRUE;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELCR10::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELCR10::ValidateDependenceParams(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != pInputData->pPDHwConfig))
    {
        if ((pInputData->pPDHwConfig->LCRConfig.blockHeight > MaxLCRBlockHeight) ||
            (pInputData->pPDHwConfig->LCRConfig.blockHeight < MinLCRBlockHeight))
        {
            result = CamxResultEOutOfBounds;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }
    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELCR10::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELCR10::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result       = CamxResultSuccess;
    VOID*      pSettingData = static_cast<VOID*>(pInputData);

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {

        result = m_pHWSetting->PackIQRegisterSetting(pSettingData, &m_outputData);

        if ((CamxResultSuccess == result) &&
            (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex)))
        {
            m_pHWSetting->DumpRegConfig();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELCR10::IFELCR10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFELCR10::IFELCR10()
{
    m_type = ISPIQModuleType::IFELCR;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFELCR10::~IFELCR10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFELCR10::~IFELCR10()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
