////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxswtmc11.cpp
/// @brief CAMXSWTMC11 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxswtmc11.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SWTMC11::CreateBPS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SWTMC11::CreateBPS(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        SWTMC11* pModule = CAMX_NEW SWTMC11;

        if (NULL != pModule)
        {
            result = pModule->Initialize();
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
            CAMX_ASSERT_ALWAYS_MESSAGE("Failed to create SWTMC11 object.");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Null Input Data for SWTMC11 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SWTMC11::CreateIFE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SWTMC11::CreateIFE(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        SWTMC11* pModule = CAMX_NEW SWTMC11;

        if (NULL != pModule)
        {
            result = pModule->Initialize();
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
            CAMX_ASSERT_ALWAYS_MESSAGE("Failed to create SWTMC11 object.");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Null Input Data for SWTMC11 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SWTMC11::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SWTMC11::Initialize()
{
    CamxResult result = CamxResultSuccess;

    if (TRUE == m_preGamma15Enabled)
    {
        m_offsetLUTCmdBuffer[GammaLUTChannel0] = 0;
        m_offsetLUTCmdBuffer[GammaLUTChannel1] = Gamma15LUTNumEntriesPerChannelSize;
        m_offsetLUTCmdBuffer[GammaLUTChannel2] = Gamma15LUTNumEntriesPerChannelSize + m_offsetLUTCmdBuffer[GammaLUTChannel1];

        result = AllocatePreGamma15Data();
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Not support Gamma15 pre-calculation in TMC11, use hardcoded gamma table");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SWTMC11::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SWTMC11::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            GetGammaOutput(pInputData);

            result = RunCalculation(pInputData);
        }
    }

    if (CamxResultSuccess == result)
    {
        UpdateIFEInternalData(pInputData);
    }

    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Input: pInputData %p", pInputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SWTMC11::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SWTMC11::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    CAMX_UNREFERENCED_PARAM(pInputData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SWTMC11::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SWTMC11::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged      = FALSE;
    BOOL dynamicEnable  = Utils::IsBitSet(pInputData->IFEDynamicEnableMask, static_cast<UINT>(DynamicIFEMaskBit::SWTMC));

    TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
    CAMX_ASSERT(NULL != pTuningManager);

    // Search through the tuning data (tree), only when there
    // are changes to the tuning mode data as an optimization
    if ((TRUE == pInputData->tuningModeChanged)    &&
        (TRUE == pTuningManager->IsValidChromatix()))
    {
        CAMX_ASSERT(NULL != pInputData->pTuningData);

        m_pChromatix = pTuningManager->GetChromatix()->GetModule_tmc11_sw(
                           reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                           pInputData->pTuningData->noOfSelectionParameter);
    }

    CAMX_ASSERT(NULL != m_pChromatix);
    if (NULL != m_pChromatix)
    {
        if ((NULL                        == m_dependenceData.pChromatix)                ||
            (m_pChromatix->SymbolTableID != m_dependenceData.pChromatix->SymbolTableID) ||
            (m_moduleEnable              != m_pChromatix->enable_section.tmc_enable))
        {
            m_dependenceData.pChromatix = m_pChromatix;
            m_moduleEnable              = m_pChromatix->enable_section.tmc_enable;
            isChanged                   = TRUE;
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "TMC11 : m_pChromatix->enable_section.tmc_enable = %d", m_moduleEnable);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to get Chromatix");
    }

    m_moduleEnable &= dynamicEnable;
    if ((TRUE == m_moduleEnable) && (m_dynamicEnable != dynamicEnable))
    {
        isChanged = TRUE;
    }
    m_dynamicEnable = dynamicEnable;

    if ((m_moduleEnable == TRUE) &&
        (TRUE           == IQInterface::s_interpolationTable.TMC11TriggerUpdate(&pInputData->triggerData, &m_dependenceData) ||
        (TRUE == pInputData->forceTriggerUpdate)))
    {
        isChanged = TRUE;
    }

    // Assign memory for parsing ADRC Specific data.
    if (NULL == m_pADRCData)
    {
        m_pADRCData = static_cast<ADRCData*>(CAMX_CALLOC(sizeof(ADRCData)));
        if (NULL == m_pADRCData)
        {
            isChanged                 = FALSE;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Memory allocation failed for ADRC Specific data");
        }
    }

    pInputData->triggerData.pADRCData         = m_pADRCData;
    pInputData->triggerData.enabledTMCversion = (TRUE == m_moduleEnable)? SWTMCVersion::TMC11: SWTMCVersion::TMC10;

    if (TRUE == m_moduleEnable)
    {
        m_dependenceData.pAdrcOutputData   = m_pADRCData;
    }
    else
    {
        isChanged                          = FALSE;
        m_dependenceData.pAdrcOutputData   = NULL;
    }

    if ((ControlAWBLockOn == pInputData->pHALTagsData->controlAWBLock) &&
        (ControlAELockOn  == pInputData->pHALTagsData->controlAECLock))
    {
        isChanged = FALSE;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SWTMC11::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SWTMC11::RunCalculation(
    const ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    result = IQInterface::TMC11CalculateSetting(&m_dependenceData);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Tmc11 Calculation Failed.");
    }

    if (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod))
    {
        DumpRegConfig();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SWTMC11::SWTMC11
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SWTMC11::SWTMC11()
{
    m_type              = ISPIQModuleType::SWTMC;
    m_cmdLength         = 0;
    m_32bitDMILength    = 0;
    m_64bitDMILength    = 0;
    m_pChromatix        = NULL;
    m_pADRCData         = NULL;
    m_preGamma15Enabled = HwEnvironment::GetInstance()->GetStaticSettings()->enableGamma15PreCalculate;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SWTMC11::~SWTMC11
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SWTMC11::~SWTMC11()
{
    if (NULL != m_pADRCData)
    {
        CAMX_FREE(m_pADRCData);
        m_pADRCData = NULL;
    }

    DeallocatePreGamma15Data();

    m_pChromatix         = NULL;
    m_pPreGammaChromatix = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SWTMC11::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SWTMC11::DumpRegConfig()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SWTMC11::AllocatePreGamma15Data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SWTMC11::AllocatePreGamma15Data()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(gamma_1_5_0::mod_gamma15_cct_dataType) * (Gamma15MaxNonLeafNode + 1));

    if (NULL == m_preGamma15Data.pInterpolationData)
    {
        // Alloc for gamma_1_5_0::mod_gamma15_cct_dataType
        m_preGamma15Data.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_preGamma15Data.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SWTMC11::DeallocatePreGamma15Data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SWTMC11::DeallocatePreGamma15Data()
{
    if (NULL != m_preGamma15Data.pInterpolationData)
    {
        CAMX_FREE(m_preGamma15Data.pInterpolationData);
        m_preGamma15Data.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SWTMC11::GetGammaOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SWTMC11::GetGammaOutput(
    ISPInputData* pInputData)
{
    CamxResult  result          = CamxResultSuccess;
    FLOAT*      pGammaOutput    = NULL;

    if ((TRUE == m_preGamma15Enabled) && (TRUE == PreGammaCheckDependenceChange(pInputData)))
    {
        Gamma15OutputData outputData;
        outputData.type = PipelineType::IPE;

        for (UINT count = 0; count < MaxGammaLUTNum; count++)
        {
            outputData.pLUT[count] =
                reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(m_pPreCalculationPacked) + m_offsetLUTCmdBuffer[count]);
        }

        result = IQInterface::IPEGamma15CalculateSetting(&m_preGamma15Data, pInputData->pOEMIQSetting, &outputData);

        if (CamxResultSuccess == result)
        {
            pGammaOutput = GetCalculatedUnpackedLUT(m_preGamma15Data.pInterpolationData);
            pInputData->pCalculatedData->IPEGamma15PreCalculationOutput.isPreCalculated = TRUE;
        }
    }
    else
    {
        pGammaOutput = &gamma15tableTMC11[0];
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "TMC11 : Gamma15 pre-calculation is not enabled, use hardcoded gamma table");
    }

    if ((CamxResultSuccess == result) && (NULL != pGammaOutput))
    {
        for (UINT32 i = 0; i < Gamma15TableSize; i++)
        {
            m_dependenceData.IPEGammaOutput[i] = pGammaOutput[i];
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "TMC11 : cannot get input gamma");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SWTMC11::PreGammaCheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SWTMC11::PreGammaCheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged          = FALSE;
    BOOL preGamma15Enabled  = TRUE;

    if ((NULL != pInputData)                  &&
        (NULL != pInputData->pAECUpdateData)  &&
        (NULL != pInputData->pAWBUpdateData)  &&
        (NULL != pInputData->pHwContext)      &&
        (NULL != pInputData->pTuningData)     &&
        (NULL != pInputData->pHALTagsData))
    {
        ISPHALTagsData* pHALTagsData = pInputData->pHALTagsData;

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Lux Index [0x%f] ", pInputData->pAECUpdateData->luxIndex);

        // Check for non-manual control
        if ((TonemapModeContrastCurve != pHALTagsData->tonemapCurves.tonemapMode) ||
            (0 == pInputData->pHALTagsData->tonemapCurves.curvePoints))
        {
            if (NULL != pInputData->pOEMIQSetting)
            {
                preGamma15Enabled = (static_cast<OEMIPEIQSetting*>(pInputData->pOEMIQSetting))->GammaEnable;
                isChanged         = (TRUE == preGamma15Enabled);
            }
            else
            {
                if (NULL != pInputData->pTuningData)
                {
                    TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
                    CAMX_ASSERT(NULL != pTuningManager);

                    // Search through the tuning data (tree), only when there
                    // are changes to the tuning mode data as an optimization
                    if ((TRUE == pInputData->tuningModeChanged)    &&
                        (TRUE == pTuningManager->IsValidChromatix()))
                    {
                        m_pPreGammaChromatix = pTuningManager->GetChromatix()->GetModule_gamma15_ipe(
                                           reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                           pInputData->pTuningData->noOfSelectionParameter);
                    }

                    m_preGamma15Data.pLibData = pInputData->pLibInitialData;

                    CAMX_ASSERT(NULL != m_pPreGammaChromatix);
                    if (NULL != m_pPreGammaChromatix)
                    {
                        if ((m_pPreGammaChromatix != m_preGamma15Data.pChromatix) ||
                            (m_pPreGammaChromatix->SymbolTableID != m_preGamma15Data.pChromatix->SymbolTableID) ||
                            (m_pPreGammaChromatix->enable_section.gamma_enable != preGamma15Enabled))
                        {
                            m_preGamma15Data.pChromatix = m_pPreGammaChromatix;
                            preGamma15Enabled           = m_pPreGammaChromatix->enable_section.gamma_enable;

                            isChanged = (TRUE == preGamma15Enabled);
                        }

                        if (TRUE == preGamma15Enabled)
                        {
                            if (TRUE == IQInterface::s_interpolationTable.gamma15TriggerUpdate(
                                            &pInputData->triggerData, &m_preGamma15Data))
                            {
                                isChanged = TRUE;
                            }

                            if (m_preGamma15Data.contrastLevel != pHALTagsData->contrastLevel)
                            {
                                isChanged = TRUE;
                                m_preGamma15Data.contrastLevel = pHALTagsData->contrastLevel;
                            }

                            //  Update the variables for gamma15 pre-calculation
                            m_pPreCalculationPacked = pInputData->pCalculatedData->IPEGamma15PreCalculationOutput.packedLUT;
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to get Chromatix");
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Tuning Pointer is NULL");
                }
            }
        }
    }
    else
    {
        if (NULL != pInputData)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc,
                           "Invalid Input: pNewAECUpdate %p pNewAWBUpdate %p pHwContext %p",
                           pInputData->pAECUpdateData,
                           pInputData->pAWBUpdateData,
                           pInputData->pHwContext);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Null Input Pointer");
        }
    }

    return isChanged;
}

CAMX_NAMESPACE_END
