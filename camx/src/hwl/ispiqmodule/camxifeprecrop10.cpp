////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifeprecrop10.cpp
/// @brief CAMXIFEPRECROP10 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifeprecrop10titan17x.h"
#include "camxifeprecrop10.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPreCrop10::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPreCrop10::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEPreCrop10* pModule = CAMX_NEW IFEPreCrop10(pCreateData);
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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFEPreCrop object.");
        }
        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Input Null pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPreCrop10::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPreCrop10::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFEPreCrop10Titan17x;
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
        m_cmdLength = 0;
        result      = CamxResultENoMemory;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPreCrop10::GetRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* IFEPreCrop10::GetRegCmd()
{
    return NULL;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPreCrop10::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPreCrop10::ValidateDependenceParams(
    ISPInputData* pInputData)
{
    CamxResult       result                  = CamxResultSuccess;

    if (((IFEPipelinePath::VideoDS4Path == m_modulePath)                    &&
         ((0 == pInputData->pCalculatedData->preCropInfo.YCrop.lastPixel)   ||
          (0 == pInputData->pCalculatedData->preCropInfo.YCrop.lastLine)    ||
          (0 == pInputData->pCalculatedData->preCropInfo.CrCrop.lastPixel)  ||
          (0 == pInputData->pCalculatedData->preCropInfo.CrCrop.lastLine))) ||
        ((IFEPipelinePath::VideoDS16Path == m_modulePath)                       &&
         ((0 == pInputData->pCalculatedData->preCropInfoDS16.YCrop.lastPixel)   ||
          (0 == pInputData->pCalculatedData->preCropInfoDS16.YCrop.lastLine)    ||
          (0 == pInputData->pCalculatedData->preCropInfoDS16.CrCrop.lastPixel)  ||
          (0 == pInputData->pCalculatedData->preCropInfoDS16.CrCrop.lastLine))))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid PreCrop Input for DS4/DS16 path");
    }

    if (((IFEPipelinePath::DisplayDS4Path == m_modulePath)                    &&
        ((0 == pInputData->pCalculatedData->dispPreCropInfo.YCrop.lastPixel)   ||
         (0 == pInputData->pCalculatedData->dispPreCropInfo.YCrop.lastLine)    ||
         (0 == pInputData->pCalculatedData->dispPreCropInfo.CrCrop.lastPixel)  ||
         (0 == pInputData->pCalculatedData->dispPreCropInfo.CrCrop.lastLine))) ||
        ((IFEPipelinePath::DisplayDS16Path == m_modulePath)                        &&
         ((0 == pInputData->pCalculatedData->dispPreCropInfoDS16.YCrop.lastPixel)   ||
          (0 == pInputData->pCalculatedData->dispPreCropInfoDS16.YCrop.lastLine)    ||
          (0 == pInputData->pCalculatedData->dispPreCropInfoDS16.CrCrop.lastPixel)  ||
          (0 == pInputData->pCalculatedData->dispPreCropInfoDS16.CrCrop.lastLine))))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid PreCrop Input for Display DS4/DS16 path");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPreCrop10::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEPreCrop10::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL result = FALSE;

    m_moduleEnable = FALSE;
    // For now enable the module only for single VFE case
    if (CropTypeCentered == pInputData->pStripeConfig->cropType)
    {
        if ((IFEPipelinePath::VideoDS4Path == m_modulePath) || (IFEPipelinePath::DisplayDS4Path == m_modulePath))
        {
            m_moduleEnable = TRUE;
            result         = TRUE;
        }
    }
    else
    {
        if (TRUE == pInputData->pStripeConfig->overwriteStripes)
        {
            if ((IFEPipelinePath::VideoDS4Path    == m_modulePath) ||
                (IFEPipelinePath::VideoDS16Path   == m_modulePath) ||
                (IFEPipelinePath::DisplayDS4Path  == m_modulePath) ||
                (IFEPipelinePath::DisplayDS16Path == m_modulePath))
            {
                m_moduleEnable = TRUE;
                result         = TRUE;
            }
        }
    }

    if ((TRUE == m_moduleEnable) && (TRUE == pInputData->forceTriggerUpdate))
    {
        result = TRUE;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPreCrop10::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPreCrop10::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result      = CamxResultSuccess;
    PreCrop10OutputData outputData;
    outputData.modulePath       = m_modulePath;

    VOID* pSettingData = static_cast<VOID*>(pInputData);
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
// IFEPreCrop10::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEPreCrop10::UpdateIFEInternalData(
    ISPInputData* pInputData)
{

    if (IFEPipelinePath::VideoDS4Path == m_modulePath)
    {
        pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS4PreCropEnable  =
            (TRUE == m_moduleEnable) ? 1 : 0;

        if (FALSE == pInputData->pStripeConfig->overwriteStripes)
        {
            if ((TRUE == m_moduleEnable) && (CropTypeCentered == pInputData->pStripeConfig->cropType))
            {
                // In single IFE mode disable the postcrop modules and enable only the precrop in the DS4 path.
                // DS4/DS16 postcrop modules may output frames with slight drift in pixels which can cause
                // artifacts that will be prominent when zoom is applied
                pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS16PostCropEnable = 0;
                pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS4PostCropEnable = 0;
            }
        }
    }
    else if (IFEPipelinePath::VideoDS16Path == m_modulePath)
    {
        pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS16PreCropEnable =
            (TRUE == m_moduleEnable) ? 1 : 0;
    }

    if (IFEPipelinePath::DisplayDS4Path == m_modulePath)
    {
        pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS4PreCropEnable  =
            (TRUE == m_moduleEnable) ? 1 : 0;

        if (FALSE == pInputData->pStripeConfig->overwriteStripes)
        {
            if ((TRUE == m_moduleEnable) && (CropTypeCentered == pInputData->pStripeConfig->cropType))
            {
                // In single IFE mode disable the postcrop modules and enable only the precrop in the DS4 path.
                // DS4/DS16 postcrop modules may output frames with slight drift in pixels which can cause
                // artifacts that will be prominent when zoom is applied
                pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS16PostCropEnable = 0;
                pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS4PostCropEnable = 0;
            }
        }
    }
    else if (IFEPipelinePath::DisplayDS16Path == m_modulePath)
    {
        pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS16PreCropEnable  =
            (TRUE == m_moduleEnable) ? 1 : 0;
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPreCrop10::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPreCrop10::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(static_cast<UINT>(m_modulePath) < IFEMaxNonCommonPaths);

    m_pInputData = pInputData;

    if (NULL != pInputData)
    {
        // Check if dependent is valid and been updated compared to last request
        result = ValidateDependenceParams(pInputData);
        if (CamxResultSuccess == result)
        {
            if (TRUE == CheckDependenceChange(pInputData))
            {
                result = RunCalculation(pInputData);

                if (CamxResultSuccess == result)
                {
                    VOID* pSettingData = static_cast<VOID*>(pInputData);
                    result = m_pHWSetting->CreateCmdList(pSettingData, NULL);
                }
            }
            if (CamxResultSuccess == result)
            {
                UpdateIFEInternalData(pInputData);
            }
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
// IFEPreCrop10::IFEPreCrop10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEPreCrop10::IFEPreCrop10(
    IFEModuleCreateData* pCreateData)
{
    m_type              = ISPIQModuleType::IFEPreCrop;
    m_moduleEnable      = FALSE;
    m_32bitDMILength    = 0;
    m_64bitDMILength    = 0;
    m_modulePath        = pCreateData->pipelineData.IFEPath;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEPreCrop10::~IFEPreCrop10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEPreCrop10::~IFEPreCrop10()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
