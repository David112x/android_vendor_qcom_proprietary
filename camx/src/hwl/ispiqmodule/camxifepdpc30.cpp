////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifepdpc30.cpp
/// @brief CAMXIFEPDPC30 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"
#include "camxiqinterface.h"
#include "camxispiqmodule.h"
#include "pdpc30setting.h"
#include "pdpc_3_0_0.h"
#include "camxnode.h"
#include "camxifepdpc30titan480.h"
#include "camxifepdpc30.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC30::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDPC30::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEPDPC30* pModule = CAMX_NEW IFEPDPC30;

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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFEPDPC30 object.");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFEPDPC30 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC30::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDPC30::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFEPDPC30Titan480;
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
        result                      = AllocateCommonLibraryData();
        if (CamxResultSuccess != result)
        {
            CAMX_DELETE m_pHWSetting;
            m_pHWSetting     = NULL;
            m_cmdLength      = 0;
            m_32bitDMILength = 0;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to initilize common library data, no memory");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class, Titan 0x%x", pCreateData->titanVersion);
        result = CamxResultENoMemory;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC30::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDPC30::Execute(
    ISPInputData* pInputData)
{
    CamxResult result  = CamxResultSuccess;
    VOID* pSettingData = static_cast<VOID*>(pInputData);

    if ((NULL != pInputData)                      &&
        (NULL != pInputData->pCmdBuffer)          &&
        (NULL != pInputData->p32bitDMIBuffer)     &&
        (NULL != pInputData->p32bitDMIBufferAddr) &&
        (NULL != m_pHWSetting))
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            result = RunCalculation(pInputData);
            if (CamxResultSuccess == result)
            {
                result = m_pHWSetting->CreateCmdList(pSettingData, &m_32bitDMIBufferOffsetDword);
                if ((NULL != pInputData->pStripeConfig) &&
                    ((CropTypeFromLeft == pInputData->pStripeConfig->cropType) ||
                    (FALSE == pInputData->pStripeConfig->overwriteStripes)))
                {
                    m_dependenceData.LUTBankSel ^= 1;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "PDPC module calculation Failed.");
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
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC30::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDPC30::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(pdpc_3_0_0::pdpc30_rgn_dataType) * (PDPC30MaxNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for pdpc_3_0_0::pdpc30_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }

    if (NULL == m_pStripingState)
    {
        // Alloc for PDC30State
        m_pStripingState = CAMX_CALLOC(sizeof(PDPC30State));
        if (NULL == m_pStripingState)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC30::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDPC30::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            result = RunCalculation(pInputData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "PDPC module calculation Failed.");
            }
        }

        if (NULL != pInputData->pStripingInput)
        {
            PDPC30State* pStripingState = reinterpret_cast<PDPC30State*>(m_pStripingState);

            if (NULL != pStripingState)
            {
                pInputData->pStripingInput->enableBits.PDAF                                = m_moduleEnable;
                pInputData->pStripingInput->stripingInput.PDAFEnable                       =
                    static_cast<int16_t>(m_moduleEnable);
                pInputData->pStripingInput->stripingInput.PDPCInputV30.enable              = pStripingState->enable;
                pInputData->pStripingInput->stripingInput.PDPCInputV30.PDAFEndX            = pStripingState->PDAFEndX;
                pInputData->pStripingInput->stripingInput.PDPCInputV30.PDAFGlobalOffsetX   =
                    pStripingState->PDAFGlobalOffsetX;
                pInputData->pStripingInput->stripingInput.PDPCInputV30.PDAFPDPCEnable      = pStripingState->PDAFPDPCEnable;
                pInputData->pStripingInput->stripingInput.PDPCInputV30.PDAFTableoffsetX    = pStripingState->PDAFTableoffsetX;
                pInputData->pStripingInput->stripingInput.PDPCInputV30.PDAFzzHDRFirstRBExp =
                    pStripingState->PDAFzzHDRFirstRBExp;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Striping Params State is NULL..");
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC30::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEPDPC30::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    pInputData->pCalculatedData->moduleEnable.IQModules.PDAFEnable     = m_moduleEnable;
    pInputData->pCalculatedMetadata->hotPixelMode                      = m_hotPixelMode;

    // Post tuning metadata if setting is enabled
    if (NULL != pInputData->pIFETuningMetadata)
    {
        if (CamxResultSuccess != m_pHWSetting->UpdateTuningMetadata(pInputData->pIFETuningMetadata))
        {
            CAMX_LOG_WARN(CamxLogGroupISP, "UpdateTuningMetadata failed.");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC30::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEPDPC30::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL               isChanged      = FALSE;
    CamxResult         result         = CamxResultSuccess;
    TuningDataManager* pTuningManager = NULL;

    /// @todo (CAMX-1207) PDAF datas need to send from the sensor. This will send at the start.
    /// Will update TRUE if the PDAF data available.
    BOOL BPCPDPCDataAvailable = FALSE;
    BOOL dynamicEnable        = TRUE;

    if ((NULL != pInputData)                 &&
        (NULL != pInputData->pHwContext)     &&
        (NULL != pInputData->pAECUpdateData))
    {
        dynamicEnable   = Utils::IsBitSet(pInputData->IFEDynamicEnableMask, static_cast<UINT>(DynamicIFEMaskBit::IFEPDPC));
        pTuningManager  = pInputData->pTuningDataManager;

        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIFEIQSetting*>(pInputData->pOEMIQSetting))->PDPCEnable;

            if (TRUE == m_moduleEnable)
            {
                isChanged = TRUE;
            }
        }
        else
        {
            // Search through the tuning data (tree), only when there
            // are changes to the tuning mode data as an optimization
            if ((TRUE == pInputData->tuningModeChanged)      &&
                (TRUE == pTuningManager->IsValidChromatix()) &&
                (NULL != pInputData->pTuningData))
            {

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_pdpc30_ife(
                                   reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                   pInputData->pTuningData->noOfSelectionParameter);
            }

            if (NULL != m_pChromatix)
            {
                if ((NULL                        == m_dependenceData.pChromatix)                        ||
                    (m_pChromatix->SymbolTableID != m_dependenceData.pChromatix->SymbolTableID)         ||
                    (NULL                        != pInputData->pCalculatedData))
                {
                    m_dependenceData.pChromatix = m_pChromatix;
                    m_moduleEnable              = (TRUE == m_pChromatix->enable_section.enable                &&
                                                  ((TRUE == m_pChromatix->enable_section.pdpc_enable          &&
                                                   pInputData->sensorData.sensorPDAFInfo.PDAFPixelCount > 0)  ||
                                                   TRUE == m_pChromatix->enable_section.bpc_enable            ||
                                                   TRUE == m_pChromatix->enable_section.gic_enable            ||
                                                   TRUE == pInputData->pCalculatedData->moduleEnable.IQModules.demuxEnable));
                    if (TRUE == m_moduleEnable)
                    {
                        isChanged = TRUE;
                    }
                    else
                    {
                        CAMX_LOG_WARN(CamxLogGroupISP, "PDPC cluster is disabled, Master:%d, PDPC:%d, BPC:%d, GIC:%d, Demux:%d",
                            m_pChromatix->enable_section.enable,
                            m_pChromatix->enable_section.pdpc_enable,
                            m_pChromatix->enable_section.bpc_enable,
                            pInputData->pCalculatedData->moduleEnable.IQModules.demuxEnable);
                    }
                }

                m_dependenceData.bpcEnable  = m_pChromatix->enable_section.bpc_enable;
                m_dependenceData.pdpcEnable = (TRUE == m_pChromatix->enable_section.pdpc_enable) &&
                                              (pInputData->sensorData.sensorPDAFInfo.PDAFPixelCount > 0);
                m_dependenceData.gicEnable  = m_pChromatix->enable_section.gic_enable;
            }
        }

        m_moduleEnable &= dynamicEnable;
        if ((TRUE == m_moduleEnable) && (m_dynamicEnable != dynamicEnable))
        {
            isChanged = TRUE;
        }
        m_dynamicEnable = dynamicEnable;

        m_dependenceData.moduleEnable = m_moduleEnable;

        m_hotPixelMode = pInputData->pHALTagsData->hotPixelMode;
        if ((TRUE == m_moduleEnable) &&
            (HotPixelModeOff == pInputData->pHALTagsData->hotPixelMode))
        {
            m_dependenceData.bpcEnable            = FALSE;
            m_dependenceData.directionalBPCEnable = FALSE;
        }

        // Dependence data still needs to be updated even in the OEM setting case
        if ((TRUE == m_moduleEnable) && (NULL != m_pChromatix))
        {
            if (TRUE ==
                IQInterface::s_interpolationTable.PDPC30TriggerUpdate(&pInputData->triggerData, &m_dependenceData) ||
                (TRUE == pInputData->forceTriggerUpdate))
            {
                isChanged = TRUE;
            }

            BPCPDPCDataAvailable = (TRUE == m_pChromatix->enable_section.enable       &&
                                   (TRUE == m_pChromatix->enable_section.pdpc_enable  ||
                                    TRUE == m_pChromatix->enable_section.bpc_enable   ||
                                    TRUE == m_pChromatix->enable_section.gic_enable));

            // PDAF data is fixed, and expected to comes only during the stream start
            if (TRUE == BPCPDPCDataAvailable)
            {
                CamxResult result      = CamxResultSuccess;
                UINT32     inputWidth  = 0;
                UINT32     inputHeight = 0;


                inputWidth = pInputData->sensorData.CAMIFCrop.lastPixel -
                        pInputData->sensorData.CAMIFCrop.firstPixel + 1;
                inputHeight = pInputData->sensorData.CAMIFCrop.lastLine -
                        pInputData->sensorData.CAMIFCrop.firstLine + 1;

                result = IQInterface::IFEGetSensorMode(&pInputData->sensorData.format, &m_dependenceData.sensorType);
                if (CamxResultSuccess == result)
                {
                    m_dependenceData.imageWidth        = inputWidth;
                    m_dependenceData.imageHeight       = inputHeight;
                    m_dependenceData.PDAFBlockWidth    = pInputData->sensorData.sensorPDAFInfo.PDAFBlockWidth;
                    m_dependenceData.PDAFBlockHeight   = pInputData->sensorData.sensorPDAFInfo.PDAFBlockHeight;
                    m_dependenceData.PDAFGlobaloffsetX =
                        static_cast<UINT16>(pInputData->sensorData.sensorPDAFInfo.PDAFGlobaloffsetX);
                    m_dependenceData.PDAFGlobaloffsetY =
                        static_cast<UINT16>(pInputData->sensorData.sensorPDAFInfo.PDAFGlobaloffsetY);
                    m_dependenceData.PDAFPixelCount    = pInputData->sensorData.sensorPDAFInfo.PDAFPixelCount;
                    m_dependenceData.ZZHDRPattern      = static_cast<UINT16>(pInputData->sensorData.ZZHDRColorPattern);
                    m_dependenceData.blackLevelOffset  = pInputData->pCalculatedMetadata->BLSblackLevelOffset;

                    switch(pInputData->sensorData.ZZHDRFirstExposure)
                    {
                        case ZZHDRFirstExposurePattern::SHORTEXPOSURE:
                            m_dependenceData.zzHDRFirstRBEXP = 1;
                            break;
                        case ZZHDRFirstExposurePattern::LONGEXPOSURE:
                            m_dependenceData.zzHDRFirstRBEXP = 0;
                            break;
                        default:
                            CAMX_LOG_ERROR(CamxLogGroupISP, "UnSupported ZZHDRFirstExposurePattern");
                    }

                    if ((NULL != pInputData->pStripeConfig) && (NULL != pInputData->pStripeConfig->pStripeOutput))
                    {
                        if (TRUE == pInputData->pStripeConfig->overwriteStripes)
                        {
                            m_dependenceData.imageWidth =
                                pInputData->pStripeConfig->pStripeOutput->PDPCOutv30.PDAFEndX;
                            m_dependenceData.PDAFGlobaloffsetX =
                                pInputData->pStripeConfig->pStripeOutput->PDPCOutv30.PDAFGlobalOffset;
                            m_dependenceData.zzHDRFirstRBEXP =
                                pInputData->pStripeConfig->pStripeOutput->PDPCOutv30.PDAFzzHDRFirstRBExp;
                        }
                    }

                    // Update bayer pattern from sensor format
                    if (CamxResultSuccess != IQInterface::GetPixelFormat(&pInputData->sensorData.format,
                                                                         &(m_dependenceData.bayerPattern)))
                    {
                        CAMX_LOG_ERROR(CamxLogGroupISP, "Unsupported sensor format : %d", pInputData->sensorData.format);
                        // Disable PDPC if sensor format is not supported.
                        isChanged = FALSE;
                    }
                    else
                    {

                        Utils::Memcpy(m_dependenceData.PDAFPixelCoords,
                                      &pInputData->sensorData.sensorPDAFInfo.PDAFPixelCoords,
                                      sizeof(PDPixelCoordinates) * m_dependenceData.PDAFPixelCount);
                        isChanged            = TRUE;
                        m_isLUTLoaded        = TRUE;
                        BPCPDPCDataAvailable = FALSE;
                    }
                }
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Pointer");
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC30::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDPC30::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult       result     = CamxResultSuccess;
    UINT32*          pDMIAddr   = pInputData->p32bitDMIBufferAddr;
    IFEPDPC30OutputData outputData;

    pDMIAddr                         += m_32bitDMIBufferOffsetDword + (pInputData->pStripeConfig->stripeId * m_32bitDMILength);
    outputData.pDMIPDAFMaskLUTDataPtr = pDMIAddr;
    outputData.pDMINoiseStdLUTDataPtr = reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(pDMIAddr) +
                                                                  IFEPDPC30PDAFDMISize);
    outputData.pSettingsData          = static_cast<VOID*>(pInputData);

    m_dependenceData.isPrepareStripeInputContext = pInputData->isPrepareStripeInputContext;

    if (TRUE == pInputData->bankUpdate.isValid)
    {
        m_dependenceData.LUTBankSel = pInputData->bankUpdate.BPCPDPCBank;
    }

    result = IQInterface::IFEPDPC30CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "PDPC Calculation Failed.");
    }

    if (TRUE == pInputData->isPrepareStripeInputContext)
    {
        PDPC30State* pStripingState = reinterpret_cast<PDPC30State*>(m_pStripingState);

        if (NULL != pStripingState)
        {
            Utils::Memcpy(pStripingState, &outputData.pdpcState, sizeof(PDPC30State));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC30::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEPDPC30::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }

    if (NULL != m_pStripingState)
    {
        CAMX_FREE(m_pStripingState);
        m_pStripingState = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC30::IFEPDPC30
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEPDPC30::IFEPDPC30()
{
    m_type                                = ISPIQModuleType::IFEPDPC;
    m_moduleEnable                        = FALSE;
    m_isLUTLoaded                         = FALSE;
    m_pChromatix                          = NULL;
    m_dependenceData.directionalBPCEnable = TRUE;  ///< TRUE means to use interpolation data
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEPDPC30::~IFEPDPC30
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEPDPC30::~IFEPDPC30()
{
    m_pChromatix = NULL;
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
    DeallocateCommonLibraryData();
}

CAMX_NAMESPACE_END
