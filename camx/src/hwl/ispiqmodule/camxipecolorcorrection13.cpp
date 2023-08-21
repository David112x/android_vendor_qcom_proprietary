////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipecolorcorrection13.cpp
/// @brief IPEColorCorrection class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxipecolorcorrection13.h"
#include "camxipecc13titan17x.h"
#include "camxipecc13titan480.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "camxtitan17xcontext.h"
#include "ipe_data.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEColorCorrection13::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEColorCorrection13::Create(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        IPEColorCorrection13* pModule = CAMX_NEW IPEColorCorrection13(pCreateData->pNodeIdentifier);

        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Module initialization failed !!");
                CAMX_DELETE pModule;
                pModule = NULL;
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Memory allocation failed");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Null input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEColorCorrection13::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEColorCorrection13::Initialize(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    m_pHWSetting = NULL;

    // Check Module Hardware Version and Create HW Setting Object
    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IPECC13Titan480;
            break;

        default:
            m_pHWSetting = CAMX_NEW IPECC13Titan17x;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        UINT size = m_pHWSetting->GetRegSize();

        m_cmdLength = PacketBuilder::RequiredWriteRegRangeSizeInDwords(size / RegisterWidthInBytes) *sizeof(UINT32);

        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);

        result = AllocateCommonLibraryData();
        if (result != CamxResultSuccess)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to initilize common library data, no memory");
            CAMX_DELETE m_pHWSetting;
            m_pHWSetting = NULL;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to create HW Setting Class, no memory");
        result = CamxResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEColorCorrection13::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEColorCorrection13::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(cc_1_3_0::cc13_rgn_dataType) * (CC13MaxNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for cc_1_3_0::cc13_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEColorCorrection13::UpdateIPEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEColorCorrection13::UpdateIPEInternalData(
    ISPInputData* pInputData
    ) const
{
    CamxResult      result          = CamxResultSuccess;
    IpeIQSettings*  pIPEIQSettings  = reinterpret_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);

    pIPEIQSettings->colorCorrectParameters.moduleCfg.EN = m_moduleEnable;
    /// @todo (CAMX-738) Add metadata information

    // Post tuning metadata if setting is enabled
    if (NULL != pInputData->pIPETuningMetadata)
    {
        result = m_pHWSetting->UpdateTuningMetadata(pInputData);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "Update Tuning Metadata failed");
            result = CamxResultSuccess; // Non-fatal error
        }
    }

    if ((pInputData->pCalculatedData) && (NULL != m_pHWSetting))
    {
        m_pHWSetting->SetupInternalData(static_cast<VOID*>(pInputData->pCalculatedData));

        pInputData->pCalculatedData->colorCorrectionMode = m_colorCorrectionMode;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEColorCorrection13::PopulateEffectMatrix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID PopulateEffectMatrix(FLOAT matrix[3][3], FLOAT saturation)
{
    matrix[0][0] = static_cast<FLOAT>(0.2990 + 1.4075 * 0.498 * saturation);
    matrix[0][1] = static_cast<FLOAT>(0.5870 - 1.4075 * 0.417 * saturation);
    matrix[0][2] = static_cast<FLOAT>(0.1140 - 1.4075 * 0.081 * saturation);
    matrix[1][0] = static_cast<FLOAT>(0.2990 + 0.3455 * 0.168 * saturation - 0.7169 * 0.498 * saturation);
    matrix[1][1] = static_cast<FLOAT>(0.5870 + 0.3455 * 0.330 * saturation + 0.7169 * 0.417 * saturation);
    matrix[1][2] = static_cast<FLOAT>(0.1140 - 0.3455 * 0.498 * saturation + 0.7169 * 0.081 * saturation);
    matrix[2][0] = static_cast<FLOAT>(0.2990 - 1.7790 * 0.168 * saturation);
    matrix[2][1] = static_cast<FLOAT>(0.5870 - 1.7790 * 0.330 * saturation);
    matrix[2][2] = static_cast<FLOAT>(0.1140 + 1.7790 * 0.498 * saturation);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEColorCorrection13::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEColorCorrection13::ValidateDependenceParams(
    const ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    /// @todo (CAMX-730) validate dependency parameters
    if ((NULL == pInputData)                                                   ||
        (NULL == pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferPostLTM]) ||
        (NULL == pInputData->pipelineIPEData.pIPEIQSettings))
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEColorCorrection13::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPEColorCorrection13::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData)                  &&
        (NULL != pInputData->pAECUpdateData)  &&
        (NULL != pInputData->pAWBUpdateData)  &&
        (NULL != pInputData->pHwContext)      &&
        (NULL != pInputData->pTuningData)     &&
        (NULL != pInputData->pHALTagsData))
    {
        m_manualCCMOverride = FALSE;
        ISPHALTagsData* pHALTagsData = pInputData->pHALTagsData;

        if ((ControlAWBLockOn == pHALTagsData->controlAWBLock) && (TRUE == m_validCCM))
        {
            CAMX_LOG_INFO(CamxLogGroupPProc, "AWBLock enabled");
        }
               // Check for the manual update
        else if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIPEIQSetting*>(pInputData->pOEMIQSetting))->CCEnable;
            isChanged      = (TRUE == m_moduleEnable);
        }
        else
        {
            m_colorCorrectionMode = pHALTagsData->colorCorrectionMode;

            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "m_colorCorrectionMode %d ", m_colorCorrectionMode);

            // Check for the manual update
            if ((ColorCorrectionModeTransformMatrix == pHALTagsData->colorCorrectionMode) &&
               (((ControlAWBModeOff                 == pHALTagsData->controlAWBMode)      &&
                (ControlModeAuto                    == pHALTagsData->controlMode))        ||
                (ControlModeOff                     == pHALTagsData->controlMode)))
            {
                m_manualCCMOverride = TRUE;
                isChanged           = TRUE;
                m_moduleEnable      = TRUE;

                // Translate the transform matrix
                ISPColorCorrectionTransform* pCCMatrix = &pHALTagsData->colorCorrectionTransform;
                for (UINT32 i = 0; i < 3; i++ )
                {
                    m_manualCCM[i][0] = static_cast<FLOAT>(pCCMatrix->transformMatrix[i][0].numerator) /
                        static_cast<FLOAT>(pCCMatrix->transformMatrix[i][0].denominator);

                    m_manualCCM[i][1] = static_cast<FLOAT>(pCCMatrix->transformMatrix[i][1].numerator) /
                        static_cast<FLOAT>(pCCMatrix->transformMatrix[i][1].denominator);

                    m_manualCCM[i][2] = static_cast<FLOAT>(pCCMatrix->transformMatrix[i][2].numerator) /
                        static_cast<FLOAT>(pCCMatrix->transformMatrix[i][2].denominator);
                }

                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "APP CCM [%f, %f, %f, %f, %f, %f, %f, %f, %f]",
                     m_manualCCM[0][0],
                     m_manualCCM[0][1],
                     m_manualCCM[0][2],
                     m_manualCCM[1][0],
                     m_manualCCM[1][1],
                     m_manualCCM[1][2],
                     m_manualCCM[2][0],
                     m_manualCCM[2][1],
                     m_manualCCM[2][2]);
            }
            else
            {
                AECFrameControl*   pNewAECUpdate  = pInputData->pAECUpdateData;
                AWBFrameControl*   pNewAWBUpdate  = pInputData->pAWBUpdateData;
                TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
                CAMX_ASSERT(NULL != pTuningManager);

                // Search through the tuning data (tree), only when there
                // are changes to the tuning mode data as an optimization
                if ((TRUE == pInputData->tuningModeChanged)    &&
                    (TRUE == pTuningManager->IsValidChromatix()))
                {
                    CAMX_ASSERT(NULL != pInputData->pTuningData);

                    m_pChromatix = pTuningManager->GetChromatix()->GetModule_cc13_ipe(
                                       reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                       pInputData->pTuningData->noOfSelectionParameter);
                }

                CAMX_ASSERT(NULL != m_pChromatix);
                if (NULL != m_pChromatix)
                {
                    if ((m_pChromatix   != m_dependenceData.pChromatix)          ||
                        (m_moduleEnable != m_pChromatix->enable_section.cc_enable))
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "updating chromatix pointer");
                        m_dependenceData.pChromatix = m_pChromatix;
                        m_moduleEnable              = m_pChromatix->enable_section.cc_enable;

                        isChanged = (TRUE == m_moduleEnable);
                    }

                    if (TRUE == m_moduleEnable)
                    {

                        m_numValidCCMs = pNewAWBUpdate->numValidCCMs;
                        if (m_numValidCCMs > MaxCCMs - 1)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid CCM value %d", m_numValidCCMs);
                            m_numValidCCMs = MaxCCMs - 1;
                        }
                        for (UINT i = 0; i < m_numValidCCMs; i++)
                        {
                            m_AWBCCM[i] = pNewAWBUpdate->AWBCCM[i];
                            if (TRUE == m_AWBCCM[i].isCCMOverrideEnabled)
                            {
                                isChanged = TRUE;
                            }
                        }

                        if (TRUE ==
                            IQInterface::s_interpolationTable.CC13TriggerUpdate(&pInputData->triggerData, &m_dependenceData))
                        {
                            isChanged = TRUE;
                        }
                    }
                    else
                    {
                        CAMX_LOG_INFO(CamxLogGroupPProc, "CC13 Module is not enabled");
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to get Chromatix");
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEColorCorrection13::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEColorCorrection13::RunCalculation(
    const ISPInputData* pInputData)
{
    CamxResult     result                        = CamxResultSuccess;
    CC13OutputData outputData                    = { PipelineType::IPE };
    FLOAT          saturation                    = 0.0f;
    const IPEInstanceProperty* pInstanceProperty = &pInputData->pipelineIPEData.instanceProperty;

    BOOL isCCMOverrideEnabled = FALSE;
    UINT indexCCM             = 0;
    if (1 == pInputData->pAWBUpdateData->numValidCCMs)
    {
        isCCMOverrideEnabled = m_AWBCCM[0].isCCMOverrideEnabled;
    }
    else
    {
        // 3 valid CCMs
        if (pInputData->parentNodeID == IFE)
        {
            if ((TRUE == pInputData->isHDR10On) &&
                (pInstanceProperty->processingType != IPEProcessingType::IPEProcessingPreview))
            {
                isCCMOverrideEnabled = m_AWBCCM[2].isCCMOverrideEnabled;
                indexCCM             = 2;
            }
            else
            {
                isCCMOverrideEnabled = m_AWBCCM[0].isCCMOverrideEnabled;
            }
        }
        else
        {
            isCCMOverrideEnabled = m_AWBCCM[1].isCCMOverrideEnabled;
            indexCCM             = 1;
        }
    }

    if (NULL != pInputData->pHALTagsData)
    {
        m_dependenceData.saturation = pInputData->pHALTagsData->saturation;
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "failed to get HAL Tags Data. Use default Saturation Value");
    }

    if ((TRUE == m_manualCCMOverride) || (TRUE == isCCMOverrideEnabled))
    {
        ManualSetting input;

        input.manualCCMEnable    = m_manualCCMOverride;
        input.pManualCCM         = &m_manualCCM[0][0];
        input.manualAWBCCMEnable = isCCMOverrideEnabled;
        input.pAWBCCM            = &m_AWBCCM[indexCCM];
        input.arraySize          = 3;
        input.pChromatix         = m_dependenceData.pChromatix;

        m_pHWSetting->SetupRegisterSetting(static_cast<VOID*>(&input));
    }
    else
    {
        if (DefaultSaturation != m_dependenceData.saturation)
        {
            saturation = (static_cast<FLOAT>(m_dependenceData.saturation) / 10.0f) * 2.0f;
            PopulateEffectMatrix(m_dependenceData.effectsMatrix, saturation);
        }

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Lux Index [0x%f] ", m_dependenceData.luxIndex);

        result = IQInterface::CC13CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "IPE CC13 Calculation Failed.");
        }
    }

    if ((NULL != m_pHWSetting) && (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex)))
    {
        m_pHWSetting->DumpRegConfig();
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEColorCorrection13::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEColorCorrection13::Execute(
    ISPInputData* pInputData)
{
    CamxResult result;

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        // Check if dependency is published and valid
        result = ValidateDependenceParams(pInputData);

        if ((CamxResultSuccess == result))
        {
            if (TRUE == CheckDependenceChange(pInputData))
            {
                result = RunCalculation(pInputData);
            }

            // Regardless of any update in dependency parameters, command buffers and IQSettings/Metadata shall be updated.
            if ((CamxResultSuccess == result) && (TRUE == m_moduleEnable))
            {
                result = m_pHWSetting->CreateCmdList(pInputData, NULL);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Runcalculation Failed");
            }

            if (CamxResultSuccess == result)
            {
                result     = UpdateIPEInternalData(pInputData);
                m_validCCM = TRUE;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "CreateCmdList Failed");
            }


            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Operation failed %d", result);
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEColorCorrection13::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEColorCorrection13::DeallocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEColorCorrection13::IPEColorCorrection13
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEColorCorrection13::IPEColorCorrection13(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier           = pNodeIdentifier;
    m_type                      = ISPIQModuleType::IPEColorCorrection;
    m_moduleEnable              = TRUE;
    m_numLUT                    = 0;
    m_offsetLUT                 = 0;
    m_pChromatix                = NULL;
    m_dependenceData.saturation = DefaultSaturation;
    m_validCCM                  = FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEColorCorrection13::~IPEColorCorrection13
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEColorCorrection13::~IPEColorCorrection13()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
    m_pChromatix = NULL;
    DeallocateCommonLibraryData();
}

CAMX_NAMESPACE_END
