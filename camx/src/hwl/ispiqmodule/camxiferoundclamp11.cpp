////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxiferoundclamp11.cpp
/// @brief CAMXIFEROUNDCLAMP class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxiferoundclamp11titan17x.h"
#include "camxiferoundclamp11titan480.h"
#include "camxiferoundclamp11.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFERoundClamp11::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEPipelinePath modulePath = pCreateData->pipelineData.IFEPath;

        if ((IFEPipelinePath::FDPath           == modulePath) ||
            (IFEPipelinePath::VideoFullPath    == modulePath) ||
            (IFEPipelinePath::VideoDS4Path     == modulePath) ||
            (IFEPipelinePath::VideoDS16Path    == modulePath) ||
            (IFEPipelinePath::DisplayFullPath  == modulePath) ||
            (IFEPipelinePath::DisplayDS4Path   == modulePath) ||
            (IFEPipelinePath::DisplayDS16Path  == modulePath) ||
            (IFEPipelinePath::PixelRawDumpPath == modulePath))
        {
            IFERoundClamp11* pModule = CAMX_NEW IFERoundClamp11(pCreateData);
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
                CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFERoundClamp object.");
            }
            pCreateData->pModule = pModule;
        }
        else
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid IFE Pipeline Path");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Input Null pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFERoundClamp11::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFERoundClamp11Titan480;
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFERoundClamp11Titan17x;
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupISP, "Not supported Titan 0x%x", pCreateData->titanVersion);
            result = CamxResultEUnsupported;
            break;
    }

    if (NULL == m_pHWSetting)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class");
        m_cmdLength = 0;
        result      = CamxResultENoMemory;
    }
    else
    {
        m_cmdLength = m_pHWSetting->GetCommandLength();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFERoundClamp11::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL             result     = FALSE;
    StreamDimension* pHALStream = &pInputData->pStripeConfig->stream[m_output];
    StreamDimension* pDimension = NULL;

    switch (m_modulePath)
    {
        case IFEPipelinePath::FDPath:
            pDimension = &m_pState->FDDimension;
            break;

        case IFEPipelinePath::VideoFullPath:
            pDimension = &m_pState->fullDimension;
            break;

        case IFEPipelinePath::VideoDS4Path:
            pDimension = &m_pState->DS4Dimension;
            break;

        case IFEPipelinePath::VideoDS16Path:
            pDimension = &m_pState->DS16Dimension;
            break;

        case IFEPipelinePath::DisplayFullPath:
            pDimension = &m_pState->displayFullDimension;
            break;

        case IFEPipelinePath::DisplayDS4Path:
            pDimension = &m_pState->displayDS4Dimension;
            break;

        case IFEPipelinePath::DisplayDS16Path:
            pDimension = &m_pState->displayDS16Dimension;
            break;

        case IFEPipelinePath::PixelRawDumpPath:
            pDimension = &m_pState->pixelRawDimension;
            break;

        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("We would never runinto this case");
            return FALSE;
            break;
    }

    // Check if Full/FD/DS/DS16 dimensions have changed
    if ((pHALStream->width  != pDimension->width) ||
        (pHALStream->height != pDimension->height))
    {
        *pDimension = *pHALStream;

        result = TRUE;
    }

    if (pInputData->HALData.format[m_output] != m_pixelFormat)
    {
        m_pixelFormat = pInputData->HALData.format[m_output];

        result = TRUE;

        if (IFEPipelinePath::PixelRawDumpPath == m_modulePath)
        {
            m_bitWidth = BitWidthFourteen;
        }
        else if (TRUE == ImageFormatUtils::Is10BitFormat(m_pixelFormat))
        {
            m_bitWidth = BitWidthTen;
        }
        else
        {
            m_bitWidth = BitWidthEight;
        }
    }

    if (TRUE == pInputData->forceTriggerUpdate)
    {
        result = TRUE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERoundClamp11::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    if (IFEPipelinePath::FDPath == m_modulePath)
    {
        pInputData->pCalculatedData->moduleEnable.FDprocessingModules.RoundClampLumaClampEnable     = TRUE;
        pInputData->pCalculatedData->moduleEnable.FDprocessingModules.RoundClampLumaRoundEnable     = TRUE;
        pInputData->pCalculatedData->moduleEnable.FDprocessingModules.RoundClampChromaClampEnable   = TRUE;
        pInputData->pCalculatedData->moduleEnable.FDprocessingModules.RoundClampChromaRoundEnable   = TRUE;
    }

    if (IFEPipelinePath::VideoFullPath == m_modulePath)
    {
        pInputData->pCalculatedData->moduleEnable.videoProcessingModules.RoundClampLumaClampEnable     = TRUE;
        pInputData->pCalculatedData->moduleEnable.videoProcessingModules.RoundClampLumaRoundEnable     = TRUE;
        pInputData->pCalculatedData->moduleEnable.videoProcessingModules.RoundClampChromaClampEnable   = TRUE;
        pInputData->pCalculatedData->moduleEnable.videoProcessingModules.RoundClampChromaRoundEnable   = TRUE;
    }

    if (IFEPipelinePath::VideoDS4Path == m_modulePath)
    {
        pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS4RoundClampLumaClampEnable     = TRUE;
        pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS4RoundClampLumaRoundEnable     = TRUE;
        pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS4RoundClampChromaClampEnable   = TRUE;
        pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS4RoundClampChromaRoundEnable   = TRUE;
    }

    if (IFEPipelinePath::VideoDS16Path == m_modulePath)
    {
        pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS16RoundClampLumaClampEnable     = TRUE;
        pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS16RoundClampLumaRoundEnable     = TRUE;
        pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS16RoundClampChromaClampEnable   = TRUE;
        pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS16RoundClampChromaRoundEnable   = TRUE;
    }

    if (IFEPipelinePath::DisplayFullPath == m_modulePath)
    {
        pInputData->pCalculatedData->moduleEnable.displayProcessingModules.RoundClampLumaClampEnable     = TRUE;
        pInputData->pCalculatedData->moduleEnable.displayProcessingModules.RoundClampLumaRoundEnable     = TRUE;
        pInputData->pCalculatedData->moduleEnable.displayProcessingModules.RoundClampChromaClampEnable   = TRUE;
        pInputData->pCalculatedData->moduleEnable.displayProcessingModules.RoundClampChromaRoundEnable   = TRUE;
    }

    if (IFEPipelinePath::DisplayDS4Path == m_modulePath)
    {
        pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS4RoundClampLumaClampEnable     = TRUE;
        pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS4RoundClampLumaRoundEnable     = TRUE;
        pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS4RoundClampChromaClampEnable   = TRUE;
        pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS4RoundClampChromaRoundEnable   = TRUE;
    }

    if (IFEPipelinePath::DisplayDS16Path == m_modulePath)
    {
        pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS16RoundClampLumaClampEnable     = TRUE;
        pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS16RoundClampLumaRoundEnable     = TRUE;
        pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS16RoundClampChromaClampEnable   = TRUE;
        pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS16RoundClampChromaRoundEnable   = TRUE;
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFERoundClamp11::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(static_cast<UINT>(m_modulePath) < IFEMaxNonCommonPaths);

    if ((NULL != pInputData))
    {
        m_pState = &pInputData->pStripeConfig->stateRoundClamp[static_cast<UINT>(m_modulePath)];

        // Check if dependent has been updated compared to last request
        if (TRUE == CheckDependenceChange(pInputData))
        {
            RunCalculation(pInputData);
            VOID* pSettingData = static_cast<VOID*>(pInputData);
            result = m_pHWSetting->CreateCmdList(pSettingData, NULL);
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
// IFERoundClamp11::GetRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* IFERoundClamp11::GetRegCmd()
{
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERoundClamp11::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;


    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        VOID* pSettingData = static_cast<VOID*>(pInputData);

        pInputData->modulePath = m_modulePath;
        pInputData->bitWidth   = m_bitWidth;

        result = m_pHWSetting->PackIQRegisterSetting(pSettingData, NULL);

        if ((CamxResultSuccess == result) &&
            (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex)))
        {
            m_pHWSetting->DumpRegConfig();
        }
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11::IFERoundClamp11
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFERoundClamp11::IFERoundClamp11(
    IFEModuleCreateData* pCreateData)
{
    m_type         = ISPIQModuleType::IFERoundClamp;
    m_moduleEnable = TRUE;

    m_32bitDMILength = 0;
    m_64bitDMILength = 0;

    m_modulePath = pCreateData->pipelineData.IFEPath;
    m_output     = FullOutput;
    switch (m_modulePath)
    {
        case IFEPipelinePath::FDPath:
            m_output = FDOutput;
            m_cmdLength =
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFERoundClamp11FDLumaReg) / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFERoundClamp11FDChromaReg) / RegisterWidthInBytes);
            break;

        case IFEPipelinePath::VideoFullPath:
            m_output = FullOutput;
            m_cmdLength =
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFERoundClamp11FullLumaReg) / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFERoundClamp11FullChromaReg) / RegisterWidthInBytes);
            break;

        case IFEPipelinePath::VideoDS4Path:
            m_output = DS4Output;
            m_cmdLength =
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFERoundClamp11DS4LumaReg) / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFERoundClamp11DS4ChromaReg) / RegisterWidthInBytes);
            break;

        case IFEPipelinePath::VideoDS16Path:
            m_output = DS16Output;
            m_cmdLength =
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFERoundClamp11DS16LumaReg) / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFERoundClamp11DS16ChromaReg) / RegisterWidthInBytes);
            break;

        case IFEPipelinePath::DisplayFullPath:
            m_output = DisplayFullOutput;
            m_cmdLength =
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFERoundClamp11DisplayLumaReg) / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFERoundClamp11DisplayChromaReg) /
                                                                 RegisterWidthInBytes);
            break;

        case IFEPipelinePath::DisplayDS4Path:
            m_output = DisplayDS4Output;
            m_cmdLength =
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFERoundClamp11DisplayDS4LumaReg) /
                                                                 RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFERoundClamp11DisplayDS4ChromaReg) /
                                                                 RegisterWidthInBytes);
            break;

        case IFEPipelinePath::DisplayDS16Path:
            m_output = DisplayDS16Output;
            m_cmdLength =
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFERoundClamp11displayDS16LumaReg) /
                                                                 RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFERoundClamp11displayDS16ChromaReg) /
                                                                 RegisterWidthInBytes);
            break;

        case IFEPipelinePath::PixelRawDumpPath:
            m_output = PixelRawOutput;
            break;

        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid path");
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFERoundClamp11::~IFERoundClamp11
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFERoundClamp11::~IFERoundClamp11()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
