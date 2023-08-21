////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifegamma16.cpp
/// @brief CAMXIFEGamma16 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxifegamma16titan17x.h"
#include "camxifegamma16titan480.h"
#include "camxifegamma16.h"
#include "camxiqinterface.h"
#include "camxispiqmodule.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGamma16::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGamma16::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEGamma16* pModule = CAMX_NEW IFEGamma16;

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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFEGamma16 object.");
        }
        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFEGamma16 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGamma16::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGamma16::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFEGamma16Titan480;
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFEGamma16Titan17x;
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
// IFEGamma16::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGamma16::Execute(
    ISPInputData* pInputData)
{
    CamxResult result  = CamxResultSuccess;
    VOID* pSettingData = static_cast<VOID*>(pInputData);

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            if (TRUE == pInputData->bankUpdate.isValid)
            {
                m_dependenceData.LUTBankSel = pInputData->bankUpdate.gammaBank;
            }
            result = RunCalculation(pInputData);

            if (CamxResultSuccess == result)
            {
                result = m_pHWSetting->CreateCmdList(pSettingData, &m_32bitDMIBufferOffsetDword);
                // Although the DMI offsets in the h/w are different,
                // each of the R,G,B Gamma LUTs will use the same LUT index (0/1)
                m_dependenceData.LUTBankSel ^= 1;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Gamma16 module calculation Failed.");
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
// IFEGamma16::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEGamma16::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    // Post module enable info
    pInputData->pCalculatedData->moduleEnable.IQModules.gammaEnable = m_moduleEnable;

    if (NULL != m_pGammaG[GammaLUTChannelG])
    {
        for (UINT i = 0; i < Gamma16NumberOfEntriesPerLUT; i++)
        {
            // mask off any bit beyond 10th bit to remove delta in output gamma table
            pInputData->pCalculatedData->gammaOutput.gammaG[i] = m_pGammaG[GammaLUTChannelG][i] & GammaMask;
        }
        pInputData->pCalculatedData->gammaOutput.isGammaValid = TRUE;
    }

    // post tuning metadata if setting is enabled
    if (NULL != pInputData->pIFETuningMetadata)
    {
        if (CamxResultSuccess != m_pHWSetting->UpdateTuningMetadata(pInputData->pIFETuningMetadata))
        {
            CAMX_LOG_WARN(CamxLogGroupISP, "UpdateTuningMetadata failed.");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGamma16::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEGamma16::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL                                isChanged      = FALSE;
    gamma_1_6_0::chromatix_gamma16Type* pChromatix     = NULL;
    TuningDataManager*                  pTuningManager = NULL;
    ISPHALTagsData*                     pHALTagsData   = pInputData->pHALTagsData;

    BOOL dynamicEnable = Utils::IsBitSet(pInputData->IFEDynamicEnableMask, static_cast<UINT>(DynamicIFEMaskBit::IFEGamma));

    if ((NULL != pHALTagsData)               &&
        (NULL != pInputData->pHwContext)     &&
        (NULL != pInputData->pAECUpdateData) &&
        (NULL != pInputData->pAWBUpdateData))
    {
        pTuningManager = pInputData->pTuningDataManager;
        CAMX_ASSERT(NULL != pTuningManager);

        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIFEIQSetting*>(pInputData->pOEMIQSetting))->GammaEnable;

            if (TRUE == m_moduleEnable)
            {
                isChanged = TRUE;
            }
        }
        else
        {

            if ((TRUE == pTuningManager->IsValidChromatix()) &&
                (NULL != pInputData->pTuningData))
            {

                pChromatix = pTuningManager->GetChromatix()->GetModule_gamma16_ife(
                    reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                    pInputData->pTuningData->noOfSelectionParameter);
            }

            CAMX_ASSERT(NULL != pChromatix);

            if (NULL != pChromatix)
            {
                if ((NULL                                       == m_dependenceData.pChromatix) ||
                    (m_dependenceData.pChromatix->SymbolTableID != pChromatix->SymbolTableID) ||
                    (pChromatix->enable_section.gamma_enable    != m_moduleEnable))
                {
                    m_dependenceData.pChromatix = pChromatix;
                    m_moduleEnable              = pChromatix->enable_section.gamma_enable;
                    if (TRUE == m_moduleEnable)
                    {
                        isChanged = TRUE;
                    }
                }
            }

            // Check for manual control
            if (TonemapModeContrastCurve == pInputData->pHALTagsData->tonemapCurves.tonemapMode)
            {
                m_moduleEnable = FALSE;
                isChanged = FALSE;
            }

            m_moduleEnable &= dynamicEnable;
            if ((TRUE == m_moduleEnable) && (m_dynamicEnable != dynamicEnable))
            {
                isChanged = TRUE;
            }
            m_dynamicEnable = dynamicEnable;

            if (TRUE == m_moduleEnable)
            {
                if ((TRUE ==
                    IQInterface::s_interpolationTable.gamma16TriggerUpdate(&pInputData->triggerData, &m_dependenceData)) ||
                    (TRUE == pInputData->forceTriggerUpdate))
                {
                    isChanged = TRUE;
                }
            }
        }

    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP,
                       "Invalid Input: pNewAECUpdate %x  pNewAWBupdate %x HwContext %x",
                       pInputData->pAECUpdateData,
                       pInputData->pAWBUpdateData,
                       pInputData->pHwContext);
    }

    return isChanged;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGamma16::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGamma16::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult        result        = CamxResultSuccess;
    UINT32*           pGammaDMIAddr = reinterpret_cast<UINT32*>(pInputData->p32bitDMIBufferAddr + m_32bitDMIBufferOffsetDword);
    Gamma16OutputData outputData;

    outputData.type         = PipelineType::IFE;
    outputData.pGDMIDataPtr = reinterpret_cast<UINT32*>(pGammaDMIAddr);
    outputData.pBDMIDataPtr = reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(outputData.pGDMIDataPtr) +
                                                                                 IFEGamma16LutTableSize);
    outputData.pRDMIDataPtr = reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(outputData.pBDMIDataPtr) +
                                                                                 IFEGamma16LutTableSize);

    // @todo (CAMX-1829) Dual IFE changes for Gamma
    result = IQInterface::Gamma16CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData, NULL);

    if (CamxResultSuccess == result)
    {
        m_pGammaG[GammaLUTChannelG] = outputData.pGDMIDataPtr;
        m_pGammaG[GammaLUTChannelB] = outputData.pBDMIDataPtr;
        m_pGammaG[GammaLUTChannelR] = outputData.pRDMIDataPtr;
    }
    else
    {
        m_pGammaG[GammaLUTChannelG] = NULL;
        m_pGammaG[GammaLUTChannelB] = NULL;
        m_pGammaG[GammaLUTChannelR] = NULL;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Gamma Calculation Failed")
    }

    if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
    {
        m_pHWSetting->DumpRegConfig();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGamma16::IFEGamma16
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEGamma16::IFEGamma16()
{
    m_type            = ISPIQModuleType::IFEGamma;
    m_64bitDMILength  = 0;
    m_moduleEnable    = TRUE;

    m_pGammaG[GammaLUTChannelG] = NULL;
    m_pGammaG[GammaLUTChannelB] = NULL;
    m_pGammaG[GammaLUTChannelR] = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEGamma16::~IFEGamma16
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEGamma16::~IFEGamma16()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
