////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipegrainadder10.cpp
/// @brief CAMXIPEGrainAdder class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcdmdefs.h"
#include "camxdefs.h"
#include "camxipegrainadder10.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtitan17xcontext.h"
#include "camxtuningdatamanager.h"
#include "ipe_data.h"
#include "parametertuningtypes.h"
#include "camxipegra10titan17x.h"
#include "camxipegra10titan480.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGrainAdder10::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGrainAdder10::Create(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        IPEGrainAdder10* pModule = CAMX_NEW IPEGrainAdder10(pCreateData->pNodeIdentifier, pCreateData);

        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Module initialization failed");
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
// IPEGrainAdder10::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGrainAdder10::Initialize(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    m_pHWSetting = NULL;

    // Check Module Hardware Version and Create HW Setting Object
    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IPEGRA10Titan480;
            break;
        default:
            m_pHWSetting = CAMX_NEW IPEGRA10Titan17x;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
        result = AllocateCommonLibraryData();
        if (CamxResultSuccess != result)
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

    if (CamxResultSuccess == result)
    {
        // Precompute Offset of Look up table with LUT command buffer for ease of patching

        m_offsetLUTCmdBuffer[GRALUTChannel0] = 0;
        m_offsetLUTCmdBuffer[GRALUTChannel1] = GRA10LUTNumEntriesPerChannelSize;
        m_offsetLUTCmdBuffer[GRALUTChannel2] = GRA10LUTNumEntriesPerChannelSize + m_offsetLUTCmdBuffer[GRALUTChannel1];
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGrainAdder10::FillCmdBufferManagerParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGrainAdder10::FillCmdBufferManagerParams(
    const ISPInputData*     pInputData,
    IQModuleCmdBufferParam* pParam)
{
    CamxResult result                  = CamxResultSuccess;
    ResourceParams* pResourceParams    = NULL;
    CHAR*           pBufferManagerName = NULL;

    if ((NULL != pParam) && (NULL != pParam->pCmdBufManagerParam) && (NULL != pInputData))
    {
        // The Resource Params and Buffer Manager Name will be freed by caller Node
        pResourceParams = static_cast<ResourceParams*>(CAMX_CALLOC(sizeof(ResourceParams)));
        if (NULL != pResourceParams)
        {
            pBufferManagerName = static_cast<CHAR*>(CAMX_CALLOC((sizeof(CHAR) * MaxStringLength256)));
            if (NULL != pBufferManagerName)
            {
                OsUtils::SNPrintF(pBufferManagerName, (sizeof(CHAR) * MaxStringLength256), "CBM_%s_%s",
                                  (NULL != m_pNodeIdentifier) ? m_pNodeIdentifier : " ", "IPEGrainAdder10");
                pResourceParams->resourceSize                 = GrainAdder10LUTBufferSize;
                pResourceParams->poolSize                     = (pInputData->requestQueueDepth * pResourceParams->resourceSize);
                pResourceParams->usageFlags.cmdBuffer         = 1;
                pResourceParams->cmdParams.type               = CmdType::CDMDMI;
                pResourceParams->alignment                    = CamxCommandBufferAlignmentInBytes;
                pResourceParams->cmdParams.enableAddrPatching = 0;
                pResourceParams->cmdParams.maxNumNestedAddrs  = 0;
                pResourceParams->memFlags                     = CSLMemFlagUMDAccess;
                pResourceParams->pDeviceIndices               = pInputData->pipelineIPEData.pDeviceIndex;
                pResourceParams->numDevices                   = 1;

                pParam->pCmdBufManagerParam[pParam->numberOfCmdBufManagers].pBufferManagerName = pBufferManagerName;
                pParam->pCmdBufManagerParam[pParam->numberOfCmdBufManagers].pParams            = pResourceParams;
                pParam->pCmdBufManagerParam[pParam->numberOfCmdBufManagers].ppCmdBufferManager = &m_pLUTCmdBufferManager;
                pParam->numberOfCmdBufManagers++;

            }
            else
            {
                result = CamxResultENoMemory;
                CAMX_FREE(pResourceParams);
                pResourceParams = NULL;
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Out Of Memory");
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Out Of Memory");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Input Error %p", pParam);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGrainAdder10::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGrainAdder10::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(gra_1_0_0::gra10_rgn_dataType) * (GRA10MaxNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for gra_1_0_0::gra10_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGrainAdder10::UpdateIPEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGrainAdder10::UpdateIPEInternalData(
    ISPInputData* pInputData
    ) const
{
    CamxResult     result         = CamxResultSuccess;

    if (NULL != pInputData)
    {
        IpeIQSettings* pIPEIQSettings = reinterpret_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);
        pIPEIQSettings->graParameters.moduleCfg.EN          = m_moduleEnable;
        pIPEIQSettings->graParameters.moduleCfg.EN_DITHER_Y = m_enableDitheringY;
        pIPEIQSettings->graParameters.moduleCfg.EN_DITHER_C = m_enableDitheringC;
        pIPEIQSettings->graParameters.grainStrength         = m_grainStrength;
        pIPEIQSettings->graParameters.grainSeed             = m_grainSeed;
        pIPEIQSettings->graParameters.mcgA                  = m_mcgA;
        pIPEIQSettings->graParameters.skipAheadJump         = m_skipAheadJump;

        // Post tuning metadata if setting is enabled
        if (NULL != pInputData->pIPETuningMetadata)
        {
            IPEGrainAdder10LUT* pTuningGrainAdderLUT = NULL;

            CAMX_STATIC_ASSERT(GrainAdder10LUTBufferSize ==
                               sizeof(pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPEDMIData.grainAdderLUT));

            CAMX_STATIC_ASSERT(GrainAdder10LUTBufferSize ==
                               sizeof(pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEDMIData.grainAdderLUT));

            switch (pInputData->titanVersion)
            {
                case CSLCameraTitanVersion::CSLTitan480:
                    pTuningGrainAdderLUT = &pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEDMIData.grainAdderLUT;
                    break;
                case CSLCameraTitanVersion::CSLTitan150:
                case CSLCameraTitanVersion::CSLTitan160:
                case CSLCameraTitanVersion::CSLTitan170:
                case CSLCameraTitanVersion::CSLTitan175:
                    pTuningGrainAdderLUT = &pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPEDMIData.grainAdderLUT;
                    break;
                default:
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid hardware version: %d", pInputData->titanVersion);
                    break;
            }

            if (NULL != pTuningGrainAdderLUT)
            {
                if (NULL != m_pGrainAdderLUTs)
                {
                    Utils::Memcpy(pTuningGrainAdderLUT, m_pGrainAdderLUTs, GrainAdder10LUTBufferSize);
                }

                if (TRUE == pIPEIQSettings->graParameters.moduleCfg.EN)
                {
                    DebugDataTagID dataTagID;

                    if (TRUE == pInputData->pipelineIPEData.realtimeFlag)
                    {
                        dataTagID = DebugDataTagID::TuningIPEGrainAdder10PackedLUT;
                    }
                    else
                    {
                        dataTagID = DebugDataTagID::TuningIPEGrainAdder10PackedLUTOffline;
                    }
                    result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                        dataTagID,
                        DebugDataTagType::TuningGrainAdder10LUT,
                        1,
                        pTuningGrainAdderLUT,
                        sizeof(IPEGrainAdder10LUT));

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
                        result = CamxResultSuccess; // Non-fatal error
                    }
                }
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Null input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGrainAdder10::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGrainAdder10::ValidateDependenceParams(
    const ISPInputData* pInputData
    ) const
{
    CamxResult result = CamxResultSuccess;

    /// @todo (CAMX-730) validate dependency parameters
    if ((NULL == pInputData)                                                     ||
        (NULL == pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader]) ||
        (NULL == pInputData->pipelineIPEData.pIPEIQSettings))
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc,
            "Invalid Input: pInputData %p",
            pInputData);

        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGrainAdder10::GetRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* IPEGrainAdder10::GetRegCmd()
{
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGrainAdder10::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPEGrainAdder10::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if (TRUE == m_enableCommonIQ)
    {
        if ((NULL != pInputData)           &&
            (NULL != pInputData->pHwContext))
        {
            if (NULL != pInputData->pOEMIQSetting)
            {
                m_moduleEnable = (static_cast<OEMIPEIQSetting*>(pInputData->pOEMIQSetting))->GrainAdderEnable;
                isChanged      = (TRUE == m_moduleEnable);
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
                        m_pChromatix = pTuningManager->GetChromatix()->GetModule_gra10_ipe(
                                           reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                           pInputData->pTuningData->noOfSelectionParameter);
                    }

                    CAMX_ASSERT(NULL != m_pChromatix);
                    if (NULL != m_pChromatix)
                    {
                        if (m_pChromatix != m_dependenceData.pChromatix)
                        {
                            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "updating chromatix pointer");
                            m_dependenceData.pChromatix = m_pChromatix;
                            m_moduleEnable              = m_pChromatix->enable_section.gra_enable;
                            isChanged                   = (TRUE == m_moduleEnable);
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to get chromatix pointer");
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Tuning Pointer is NULL");
                }
            }

            if (TRUE == m_moduleEnable)
            {
                m_dependenceData.preScaleRatio = pInputData->preScaleRatio;
                m_dependenceData.frameNum      = static_cast<UINT32>(pInputData->frameNum);

                // Check for trigger update status
                if (TRUE == IQInterface::s_interpolationTable.gra10TriggerUpdate(&pInputData->triggerData, &m_dependenceData))
                {
                    if (NULL == pInputData->pOEMIQSetting)
                    {
                        // Check for module dynamic enable trigger hysterisis
                        m_moduleEnable = IQSettingUtils::GetDynamicEnableFlag(
                            m_dependenceData.pChromatix->dynamic_enable_triggers.gra_enable.enable,
                            m_dependenceData.pChromatix->dynamic_enable_triggers.gra_enable.hyst_control_var,
                            m_dependenceData.pChromatix->dynamic_enable_triggers.gra_enable.hyst_mode,
                            &(m_dependenceData.pChromatix->dynamic_enable_triggers.gra_enable.hyst_trigger),
                            static_cast<VOID*>(&pInputData->triggerData),
                            &m_dependenceData.moduleEnable);

                        // Set status to isChanged to avoid calling RunCalculation if the module is disabled
                        isChanged = (TRUE == m_moduleEnable);
                    }
                }
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "GRA_1_0_0 Module is not enabled");
            }
        }
        else
        {
            if (NULL != pInputData)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid HwContext: %p", pInputData->pHwContext);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "NULL Input Pointer");
            }
        }
    }
    else
    {
        isChanged = TRUE;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGrainAdder10::FetchDMIBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGrainAdder10::FetchDMIBuffer()
{
    PacketResource* pPacketResource = NULL;
    CamxResult      result          = CamxResultSuccess;

    if (NULL != m_pLUTCmdBufferManager)
    {
        // Recycle the last updated LUT DMI cmd buffer
        if (NULL != m_pLUTCmdBuffer)
        {
            m_pLUTCmdBufferManager->Recycle(m_pLUTCmdBuffer);
        }

        // fetch a fresh LUT DMI cmd buffer
        result = m_pLUTCmdBufferManager->GetBuffer(&pPacketResource);
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "LUT Command Buffer is NULL")
    }

    if (CamxResultSuccess == result)
    {
        m_pLUTCmdBuffer = static_cast<CmdBuffer*>(pPacketResource);
    }
    else
    {
        m_pLUTCmdBuffer = NULL;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to fetch DMI Buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGrainAdder10::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGrainAdder10::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;
    UINT32*         pLUT = NULL;

    if (TRUE == m_enableCommonIQ)
    {
        result = FetchDMIBuffer();

        if (CamxResultSuccess == result)
        {
            GRA10OutputData outputData;
            if (NULL != m_pLUTCmdBuffer)
            {
                pLUT = reinterpret_cast<UINT32*>(m_pLUTCmdBuffer->BeginCommands(GrainAdder10LUTBufferDSize));
            }

            CAMX_ASSERT(NULL != pLUT);

            outputData.type = PipelineType::IPE;
            for (UINT count = 0; count < GRALUTMax; count++)
            {
                outputData.pLUT[count] = reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(pLUT) +
                                                                   m_offsetLUTCmdBuffer[count]);
            }

            result = IQInterface::IPEGRA10CalculateSetting(&m_dependenceData,
                                                           pInputData->pOEMIQSetting,
                                                           &outputData);
            if (CamxResultSuccess == result)
            {
                m_enableDitheringC = outputData.enableDitheringC;
                m_enableDitheringY = outputData.enableDitheringY;
                m_grainStrength    = outputData.grainStrength;
                m_grainSeed        = outputData.grainSeed;
                m_mcgA             = outputData.mcgA;
                m_skipAheadJump    = outputData.skiAheadAJump;

                if (NULL != m_pLUTCmdBuffer)
                {
                    result = m_pLUTCmdBuffer->CommitCommands();
                }
                if (CamxResultSuccess != result)
                {
                    m_pLUTCmdBufferManager->Recycle(m_pLUTCmdBuffer);
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Commit command failed result %s", Utils::CamxResultToString(result));
                }

                if (NULL != pInputData->pIPETuningMetadata)
                {
                    m_pGrainAdderLUTs = pLUT;
                }
            }
            else
            {
                if (NULL != m_pLUTCmdBuffer)
                {
                    m_pLUTCmdBufferManager->Recycle(m_pLUTCmdBuffer);
                }
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Calculate setting failed result %s", Utils::CamxResultToString(result));
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Cannot get buffer from CmdBufferManager");
        }
    }
    else
    {
        m_grainStrength    = 0x00000003;
        m_grainSeed        = 0x00000007;
        m_mcgA             = 0x0A1E678D;
        m_skipAheadJump    = 0x0E8CDF45;
        m_enableDitheringC = 0;
        m_enableDitheringY = 0;

        result = FetchDMIBuffer();

        if (CamxResultSuccess == result)
        {
            pLUT = reinterpret_cast<UINT32*>(m_pLUTCmdBuffer->BeginCommands(GrainAdder10LUTBufferDSize));

            if (NULL != pLUT)
            {
                UpdateLUTFromChromatix(pLUT);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "pLUT is NULL!");
                result = CamxResultENoMemory;
            }

            if (CamxResultSuccess == result)
            {
                result = m_pLUTCmdBuffer->CommitCommands();
            }

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "GrainAdder10 LUT Commit Failed. result %d", result);
            }
        }
        else
        {
            m_pLUTCmdBuffer = NULL;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Cannot get buffer from CmdBufferManager");
        }
    }

    if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
    {
        m_pHWSetting->DumpRegConfig();
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGrainAdder10::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGrainAdder10::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        // Check if dependency is published and valid
        result = ValidateDependenceParams(pInputData);

        if (CamxResultSuccess == result)
        {
            if (TRUE == CheckDependenceChange(pInputData))
            {
                result = RunCalculation(pInputData);
            }

            // Regardless of any update in dependency parameters, command buffers and IQSettings/Metadata shall be updated.
            if ((CamxResultSuccess == result) && (TRUE == m_moduleEnable))
            {
                // Record the offset within DMI Header buffer,
                // it is the offset at which this module has written a CDM DMI header.
                CmdBuffer* pDMICmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader];
                m_offsetLUT = pDMICmdBuffer->GetResourceUsedDwords() * sizeof(UINT32);

                GRAHWSettingParams moduleHWSettingParams = { 0 };

                moduleHWSettingParams.pLUTCmdBuffer       = m_pLUTCmdBuffer;
                moduleHWSettingParams.pOffsetLUTCmdBuffer = &m_offsetLUTCmdBuffer[0];

                result = m_pHWSetting->CreateCmdList(pInputData, reinterpret_cast<UINT32*>(&moduleHWSettingParams));
            }

            if (CamxResultSuccess == result)
            {
                result = UpdateIPEInternalData(pInputData);
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
// IPEGrainAdder10::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEGrainAdder10::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGrainAdder10::IPEGrainAdder10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEGrainAdder10::IPEGrainAdder10(
    const CHAR*          pNodeIdentifier,
    IPEModuleCreateData* pCreateData)
{
    m_pNodeIdentifier             = pNodeIdentifier;
    m_type                        = ISPIQModuleType::IPEGrainAdder;
    m_moduleEnable                = TRUE;
    m_cmdLength                   = 0;
    m_numLUT                      = 3;
    m_pLUTCmdBuffer               = NULL;
    m_pGrainAdderLUTs             = NULL;
    m_pChromatix                  = NULL;
    m_dependenceData.moduleEnable = FALSE;   ///< First frame is always FALSE

    if (TRUE == pCreateData->initializationData.registerBETEn)
    {
        m_enableCommonIQ = TRUE;
    }
    else
    {
        Titan17xContext* pContext = static_cast<Titan17xContext*>(pCreateData->initializationData.pHwContext);
        m_enableCommonIQ = pContext->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->enableGRACommonIQModule;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE Grain Adder m_numLUT %d , m_enableCommonIQ %d", m_numLUT, m_enableCommonIQ);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEGrainAdder10::~IPEGrainAdder10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEGrainAdder10::~IPEGrainAdder10()
{
    DeallocateCommonLibraryData();

    if (NULL != m_pLUTCmdBufferManager)
    {
        if (NULL != m_pLUTCmdBuffer)
        {
            m_pLUTCmdBufferManager->Recycle(m_pLUTCmdBuffer);
            m_pLUTCmdBuffer = NULL;
        }

        m_pLUTCmdBufferManager->Uninitialize();
        CAMX_DELETE m_pLUTCmdBufferManager;
        m_pLUTCmdBufferManager = NULL;
    }

    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }

    m_pChromatix = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGrainAdder10::UpdateLUTFromChromatix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEGrainAdder10::UpdateLUTFromChromatix(
    UINT32* pLUT)
{
    /// @todo (CAMX-735) Fetch LUT from Chroamtix
    CamX::Utils::Memcpy(pLUT, &DMI_Y_LUT, sizeof(DMI_Y_LUT));

    pLUT += (sizeof(DMI_Y_LUT) / sizeof(UINT32));
    CamX::Utils::Memcpy(pLUT, &DMI_Cb_LUT, sizeof(DMI_Cb_LUT));

    pLUT += (sizeof(DMI_Cb_LUT) / sizeof(UINT32));
    CamX::Utils::Memcpy(pLUT, &DMI_Cr_LUT, sizeof(DMI_Cr_LUT));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGrainAdder10::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEGrainAdder10::DumpRegConfig(
    UINT32* pLUT
    ) const
{
    CHAR  dumpFilename[256];
    FILE* pFile = NULL;
    CamX::OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename), "%s/IPE_regdump.txt", ConfigFileDirectory);
    pFile = CamX::OsUtils::FOpen(dumpFilename, "a+");

    if (NULL != pFile)
    {
        CamX::OsUtils::FPrintF(pFile, "******** IPE Grain Adder10 ********\n");
        CamX::OsUtils::FPrintF(pFile, "TO DO FOR LUT\n");

        CamX::OsUtils::FPrintF(pFile, "\n* Grain Adder10Module CGF \n");
        CamX::OsUtils::FPrintF(pFile, "graParameters.moduleCfg.EN           = %d\n", m_moduleEnable);
        CamX::OsUtils::FPrintF(pFile, "asfParameters.moduleCfg.EN_DITHER_Y  = %d\n", m_enableDitheringY);
        CamX::OsUtils::FPrintF(pFile, "asfParameters.moduleCfg.EN_DITHER_C  = %d\n", m_enableDitheringC);
        CamX::OsUtils::FPrintF(pFile, "asfParameters.grainStrength          = %d\n", m_grainStrength);
        CamX::OsUtils::FPrintF(pFile, "asfParameters.grainSeed              = %d\n", m_grainSeed);
        CamX::OsUtils::FPrintF(pFile, "asfParameters.mcgA                   = %d\n", m_mcgA);
        CamX::OsUtils::FPrintF(pFile, "asfParameters.skipAheadJump          = %d\n", m_skipAheadJump);

        if (TRUE == m_moduleEnable)
        {
            UINT32 offset               = 0;

            CamX::OsUtils::FPrintF(pFile, "* gra10Data.ch0_LUT.weight_table[2][32] = \n");
            for (UINT index = 0; index < GRA10LUTNumEntriesPerChannel; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT32>(*(pLUT + offset + index)));
            }

            offset += GRA10LUTNumEntriesPerChannel;
            CamX::OsUtils::FPrintF(pFile, "\n* gra10Data.ch1_LUT.weight_table[2][32] = \n");
            for (UINT index = 0; index < GRA10LUTNumEntriesPerChannel; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT32>(*(pLUT + offset + index)));
            }

            offset += GRA10LUTNumEntriesPerChannel;
            CamX::OsUtils::FPrintF(pFile, "\n* gra10Data.ch2_LUT.weight_table[2][32] = \n");
            for (UINT index = 0; index < GRA10LUTNumEntriesPerChannel; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT32>(*(pLUT + offset + index)));
            }
        }
        else
        {
            CamX::OsUtils::FPrintF(pFile, "NOT ENABLED");
        }

        CamX::OsUtils::FPrintF(pFile, "\n\n");
        CamX::OsUtils::FClose(pFile);
    }
}

CAMX_NAMESPACE_END
