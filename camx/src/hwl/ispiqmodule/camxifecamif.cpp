////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecamif.cpp
/// @brief CAMXIFECAMIF class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxcslifedefs.h"
#include "camxifecamiftitan17x.h"
#include "camxifecamiflitetitan17x.h"
#include "camxifecamiflcrtitan480.h"
#include "camxifecamifpdaftitan480.h"
#include "camxifecamifpptitan480.h"
#include "camxifecamifrdititan480.h"

#include "camxifecamif.h"
#include "camxiqinterface.h"
#include "camxisphwsetting.h"
#include "camxispiqmodule.h"
#include "camxtuningdatamanager.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIF::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIF::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFECAMIF* pModule = CAMX_NEW IFECAMIF;

        if (NULL != pModule)
        {
            pModule->m_type = pCreateData->pipelineData.IFEModuleType;
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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFECAMIF object.");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFECAMIF Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIF::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIF::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            switch (m_type)
            {
                case ISPIQModuleType::IFECAMIF:
                    m_pHWSetting = CAMX_NEW IFECAMIFPPTitan480(0);  // pp instance 0
                    m_modulePath = IFECSIDIPP;
                    break;
                case ISPIQModuleType::IFECAMIFRDI0:
                    m_pHWSetting = CAMX_NEW IFECAMIFRdiTitan480(0); // rdi instance 0
                    m_modulePath = IFECSIDRDI0;
                    break;
                case ISPIQModuleType::IFECAMIFRDI1:
                    m_pHWSetting = CAMX_NEW IFECAMIFRdiTitan480(1); // rdi instance 1
                    m_modulePath = IFECSIDRDI1;
                    break;
                case ISPIQModuleType::IFECAMIFRDI2:
                    m_pHWSetting = CAMX_NEW IFECAMIFRdiTitan480(2); // rdi instance 2
                    m_modulePath = IFECSIDRDI2;
                    break;
                case ISPIQModuleType::IFECAMIFRDI3:
                    m_pHWSetting = CAMX_NEW IFECAMIFRdiTitan480(3); // rdi instance 3
                    m_modulePath = IFECSIDRDI3;
                    break;
                case ISPIQModuleType::IFECAMIFDualPD:
                    m_pHWSetting = CAMX_NEW IFECAMIFPDAFTitan480(0); // PDAF Instance
                    m_modulePath = IFECSIDPPP;
                    break;
                case ISPIQModuleType::IFECAMIFLCR:
                    m_pHWSetting = CAMX_NEW IFECAMIFLCRTitan480(0); // LCR Instance
                    m_modulePath = IFECSIDIPP;
                    break;
                default:
                    CAMX_LOG_ERROR(CamxLogGroupISP, "Unsupported CAMIF type %d", pCreateData->pModule->GetIQType());
                    break;
            }
            break;
        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            switch(m_type)
            {
                case ISPIQModuleType::IFECAMIF:
                    m_pHWSetting = CAMX_NEW IFECAMIFTitan17x;
                    break;
                case ISPIQModuleType::IFECAMIFLite:
                    m_pHWSetting = CAMX_NEW IFECAMIFLiteTitan17x;
                    break;
                default:
                    CAMX_LOG_ERROR(CamxLogGroupISP, "Unsupported CAMIF type %d", pCreateData->pModule->GetIQType());
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
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Version %d, Create IQ module type %d", pCreateData->titanVersion, m_type);

        m_cmdLength = m_pHWSetting->GetCommandLength();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIF::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIF::Execute(
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
                CAMX_ASSERT_ALWAYS_MESSAGE("CAMIF module calculation Failed.");
            }
        }

        if (FALSE == m_moduleEnable)
        {
            result = m_pHWSetting->SetupRegisterSetting(&m_moduleEnable);

            if (CamxResultSuccess == result)
            {
                result = m_pHWSetting->CreateSubCmdList(pSettingData, NULL);
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
// IFECAMIF::GetRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* IFECAMIF::GetRegCmd()
{
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIF::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFECAMIF::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL  isChanged     = FALSE;
    BOOL  dynamicEnable = TRUE;

    if (NULL != pInputData)
    {
        dynamicEnable = Utils::IsBitSet(pInputData->IFEDynamicEnableMask, static_cast<UINT>(DynamicIFEMaskBit::IFECAMIF));

        if (TRUE == pInputData->pipelineIFEData.programCAMIF)
        {
            IFECAMIFCfgInfo* pPathInfo = pInputData->pStripeConfig->pCAMIFConfigInfo;
            switch (pInputData->titanVersion)
            {
                case CSLCameraTitanVersion::CSLTitan480:
                    switch (m_type)
                    {
                        case ISPIQModuleType::IFECAMIF:
                            isChanged         = pPathInfo->enableCAMIFPath[IFECAMIFPXLPath];
                            break;
                        case ISPIQModuleType::IFECAMIFRDI0:
                            isChanged         = pPathInfo->enableCAMIFPath[IFECAMIFRDI0Path];
                            break;
                        case ISPIQModuleType::IFECAMIFRDI1:
                            isChanged         = pPathInfo->enableCAMIFPath[IFECAMIFRDI1Path];
                            break;
                        case ISPIQModuleType::IFECAMIFRDI2:
                            isChanged         = pPathInfo->enableCAMIFPath[IFECAMIFRDI2Path];
                            break;
                        case ISPIQModuleType::IFECAMIFRDI3:
                            isChanged         = pPathInfo->enableCAMIFPath[IFECAMIFRDI3Path];
                            break;
                        case ISPIQModuleType::IFECAMIFDualPD:
                            isChanged         = pPathInfo->enableCAMIFPath[IFECAMIFDualPDPath];
                            break;
                        case ISPIQModuleType::IFECAMIFLCR:
                            isChanged         = pPathInfo->enableCAMIFPath[IFECAMIFLCRPath];
                            break;
                        default:
                            CAMX_LOG_ERROR(CamxLogGroupISP, "Unsupported CAMIF type %d", m_type);
                            break;
                    }
                    break;
                case CSLCameraTitanVersion::CSLTitan150:
                case CSLCameraTitanVersion::CSLTitan160:
                case CSLCameraTitanVersion::CSLTitan170:
                case CSLCameraTitanVersion::CSLTitan175:
                    isChanged = TRUE;
                    break;
                default:
                    CAMX_LOG_ERROR(CamxLogGroupISP, "Not supported Titan 0x%x", pInputData->titanVersion);
                    break;
            }

            m_moduleEnable = dynamicEnable;
            if ((TRUE == m_moduleEnable) && (m_dynamicEnable != dynamicEnable))
            {
                isChanged = TRUE;
            }
            m_dynamicEnable = dynamicEnable;
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Module type %d, is changed %d", m_type, isChanged);
    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIF::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIF::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    VOID* pSettingData = static_cast<VOID*>(pInputData);
    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        result = m_pHWSetting->SetupRegisterSetting(pSettingData);
        if (CamxResultSuccess == result)
        {
            // Update bayer pattern from sensor format
            result = IQInterface::GetPixelFormat(&pInputData->sensorData.format, &m_bayerPattern);
            if (CamxResultSuccess == result)
            {
                result = m_pHWSetting->PackIQRegisterSetting(pSettingData, &m_bayerPattern);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Unsupported sensor format : %d", pInputData->sensorData.format);
            }
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
// IFECAMIF::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECAMIF::UpdateIFEInternalData(
    ISPInputData* pInputData)
{

    BOOL*               pFrameBased = pInputData->pFrameBased;
    StreamDimension*    pRDIStreams = pInputData->pRDIStreams;

    // Update WM Config
    UINT32 wmUpdateIndex = pInputData->pCalculatedData->WMUpdate.numberOfWMUpdates;

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "m_modulePath %d, Framebased for RDI path[%d, %d, %d, %d]",
                     m_modulePath,
                     pFrameBased[IFECSIDRDI0],
                     pFrameBased[IFECSIDRDI1],
                     pFrameBased[IFECSIDRDI2],
                     pFrameBased[IFECSIDRDI3]);

    if (((m_modulePath == IFECSIDRDI0) && (TRUE == pFrameBased[IFECSIDRDI0])) ||
        ((m_modulePath == IFECSIDRDI1) && (TRUE == pFrameBased[IFECSIDRDI1])) ||
        ((m_modulePath == IFECSIDRDI2) && (TRUE == pFrameBased[IFECSIDRDI2])) ||
        ((m_modulePath == IFECSIDRDI3) && (TRUE == pFrameBased[IFECSIDRDI3])))
    {
        if (m_modulePath == IFECSIDRDI0)
        {
            pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].portID  = IFEOutputRDI0;
            pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].width   = pRDIStreams[IFECSIDRDI0].width;
            pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].width   = pRDIStreams[IFECSIDRDI0].height;
        }
        else if (m_modulePath == IFECSIDRDI1)
        {
            pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].portID  = IFEOutputRDI1;
            pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].width   = pRDIStreams[IFECSIDRDI1].width;
            pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].width   = pRDIStreams[IFECSIDRDI1].height;
        }
        else if (m_modulePath == IFECSIDRDI2)
        {
            pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].portID  = IFEOutputRDI2;
            pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].width   = pRDIStreams[IFECSIDRDI2].width;
            pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].width   = pRDIStreams[IFECSIDRDI2].height;
        }
        else
        {
            pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].portID  = IFEOutputRDI3;
            pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].width   = pRDIStreams[IFECSIDRDI3].width;
            pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].width   = pRDIStreams[IFECSIDRDI3].height;
        }

        pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].mode = IFEWMModeFrameBased;
        pInputData->pCalculatedData->WMUpdate.numberOfWMUpdates++;
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Frame based config , numberOfWMUpdates %d",
            pInputData->pCalculatedData->WMUpdate.numberOfWMUpdates);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIF::IFECAMIF
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECAMIF::IFECAMIF()
{
    m_type           = ISPIQModuleType::IFECAMIF;
    m_cmdLength      = 0;
    m_32bitDMILength = 0;
    m_64bitDMILength = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFECAMIF::~IFECAMIF
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECAMIF::~IFECAMIF()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
