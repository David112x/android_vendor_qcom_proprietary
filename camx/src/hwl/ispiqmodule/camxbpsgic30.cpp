////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpsgic30.cpp
/// @brief BPSGIC30 class implementation
///        Corrects localized Gb/Gr imbalance artifacts in image
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "bps_data.h"
#include "camxbpsgic30.h"
#include "camxiqinterface.h"
#include "camxbpsgic30titan17x.h"
#include "camxbpsgic30titan480.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"
#include "bpsgic30setting.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGIC30::Create(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        BPSGIC30* pModule = CAMX_NEW BPSGIC30(pCreateData->pNodeIdentifier);

        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Module initialization failed !!");
                CAMX_DELETE pModule;
                pModule = NULL;
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Memory allocation failed");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGIC30::Initialize(
    BPSModuleCreateData* pCreateData)
{
    CamxResult     result     = CamxResultSuccess;
    ISPInputData*  pInputData = &pCreateData->initializationData;

    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            result = BPSGIC30Titan480::Create(&m_pHWSetting);
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            result = BPSGIC30Titan17x::Create(&m_pHWSetting);
            break;
        default:
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Titan Version: %d", pCreateData->titanVersion);
            break;
    }

    if (CamxResultSuccess == result)
    {
        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
        m_cmdLength                 = m_pHWSetting->GetCommandLength();

        result = AllocateCommonLibraryData();
        if (result != CamxResultSuccess)
        {
            CAMX_DELETE m_pHWSetting;
            m_pHWSetting = NULL;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to initilize common library data, no memory");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to create HW Setting Class, no memory");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30::FillCmdBufferManagerParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGIC30::FillCmdBufferManagerParams(
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
                                  (NULL != m_pNodeIdentifier) ? m_pNodeIdentifier : " ", "BPSGIC30");
                pResourceParams->resourceSize                 = BPSGICNoiseLUTBufferSize;
                pResourceParams->poolSize                     = (pInputData->requestQueueDepth * pResourceParams->resourceSize);
                pResourceParams->usageFlags.cmdBuffer         = 1;
                pResourceParams->cmdParams.type               = CmdType::CDMDMI;
                pResourceParams->alignment                    = CamxCommandBufferAlignmentInBytes;
                pResourceParams->cmdParams.enableAddrPatching = 0;
                pResourceParams->cmdParams.maxNumNestedAddrs  = 0;
                pResourceParams->memFlags                     = CSLMemFlagUMDAccess;
                pResourceParams->pDeviceIndices               = pInputData->pipelineBPSData.pDeviceIndex;
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
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Out Of Memory");
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Out Of Memory");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Input Error %p", pParam);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGIC30::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(gic_3_0_0::gic30_rgn_dataType) * (GIC30MaxmiumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Allocate memory for gic_3_0_0::gic30_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSGIC30::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData)           &&
        (NULL != pInputData->pHwContext))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMBPSIQSetting*>(pInputData->pOEMIQSetting))->GICEnable;
            isChanged      = (TRUE == m_moduleEnable);
        }
        else
        {
            TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
            CAMX_ASSERT(NULL != pTuningManager);

            // Search through the tuning data (tree), only when there
            // are changes to the tuning mode data as an optimization
            if ((TRUE == pInputData->tuningModeChanged)    &&
                (TRUE == pTuningManager->IsValidChromatix()))
            {
                CAMX_ASSERT(NULL != pInputData->pTuningData);

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_gic30_bps(
                    reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                    pInputData->pTuningData->noOfSelectionParameter);
            }

            CAMX_ASSERT(NULL != m_pChromatix);
            if (NULL != m_pChromatix)
            {
                if (m_pChromatix != m_dependenceData.pChromatix)
                {
                    m_dependenceData.pChromatix = m_pChromatix;
                    m_moduleEnable              = m_pChromatix->enable_section.gic_global_enable;
                    isChanged                   = (TRUE == m_moduleEnable);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to get Chromatix");
            }
        }

        if (TRUE == m_moduleEnable)
        {
            // Check for trigger update status
            if (TRUE == IQInterface::s_interpolationTable.BPSGIC30TriggerUpdate(&(pInputData->triggerData), &m_dependenceData))
            {
                if (NULL == pInputData->pOEMIQSetting)
                {
                    // Check for module dynamic enable trigger hysterisis
                    m_moduleEnable = IQSettingUtils::GetDynamicEnableFlag(
                        m_dependenceData.pChromatix->dynamic_enable_triggers.gic_global_enable.enable,
                        m_dependenceData.pChromatix->dynamic_enable_triggers.gic_global_enable.hyst_control_var,
                        m_dependenceData.pChromatix->dynamic_enable_triggers.gic_global_enable.hyst_mode,
                        &(m_dependenceData.pChromatix->dynamic_enable_triggers.gic_global_enable.hyst_trigger),
                        static_cast<VOID*>(&(pInputData->triggerData)),
                        &m_dependenceData.moduleEnable);

                    // Set status to isChanged to avoid calling RunCalculation if the module is disabled
                    isChanged = (TRUE == m_moduleEnable);
                }
            }
        }
    }
    else
    {
        if (NULL != pInputData)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS,
                           "Invalid Input: pHwContext %p",
                           pInputData->pHwContext);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
        }
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30::FetchDMIBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGIC30::FetchDMIBuffer()
{
    CamxResult      result          = CamxResultSuccess;
    PacketResource* pPacketResource = NULL;

    if (NULL != m_pLUTCmdBufferManager)
    {
        // Recycle the last updated LUT DMI cmd buffer
        if (NULL != m_pLUTDMICmdBuffer)
        {
            m_pLUTCmdBufferManager->Recycle(m_pLUTDMICmdBuffer);
        }

        // fetch a fresh LUT DMI cmd buffer
        result = m_pLUTCmdBufferManager->GetBuffer(&pPacketResource);
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "LUT Command Buffer is NULL");
    }

    if (CamxResultSuccess == result)
    {
        m_pLUTDMICmdBuffer = static_cast<CmdBuffer*>(pPacketResource);
    }
    else
    {
        m_pLUTDMICmdBuffer = NULL;
        result             = CamxResultEUnableToLoad;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to fetch DMI Buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGIC30::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        result = FetchDMIBuffer();

        if (CamxResultSuccess == result)
        {
            GIC30OutputData outputData;

            outputData.pDMIData     = reinterpret_cast<UINT32*>(m_pLUTDMICmdBuffer->BeginCommands(BPSGIC30DMILengthDword));
            // BET ONLY - InputData is different per module tested
            pInputData->pBetDMIAddr = static_cast<VOID*>(outputData.pDMIData);
            CAMX_ASSERT(NULL != outputData.pDMIData);

            result = IQInterface::BPSGIC30CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);
            if (CamxResultSuccess == result)
            {
                m_pLUTDMICmdBuffer->CommitCommands();
                if (NULL != pInputData->pBPSTuningMetadata)
                {
                    m_pGICNoiseLUT = outputData.pDMIData;
                }
                if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
                {
                    m_pHWSetting->DumpRegConfig();
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "GIC Calculation Failed. result %d", result);
            }
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGIC30::ValidateDependenceParams(
    const ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {

        if ((NULL == pInputData->pHwContext) || (NULL == pInputData->pAECUpdateData))
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS,
                "Invalid Input: pInputData %p pNewAECUpdate %p  HwContext %p",
                pInputData,
                pInputData->pAECUpdateData,
                pInputData->pHwContext);
            result = CamxResultEFailed;
        }
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30::GetRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* BPSGIC30::GetRegCmd()
{
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30::UpdateBPSInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSGIC30::UpdateBPSInternalData(
    ISPInputData* pInputData
    ) const
{
    CamxResult result = CamxResultSuccess;

    result = m_pHWSetting->UpdateFirmwareData(pInputData, m_moduleEnable);

    if (CamxResultSuccess == result)
    {
        // Post tuning metadata if setting is enabled
        if (NULL != pInputData->pBPSTuningMetadata)
        {
            result = m_pHWSetting->UpdateTuningMetadata(pInputData);

            if (CamxResultSuccess == result)
            {
                switch(pInputData->titanVersion)
                {
                    case CSLCameraTitanVersion::CSLTitan480:
                        CAMX_STATIC_ASSERT(BPSGICNoiseLUTBufferSize <=
                                        sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.GICLUT));

                        if (NULL != m_pGICNoiseLUT)
                        {
                            Utils::Memcpy(&pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.GICLUT,
                                        m_pGICNoiseLUT,
                                        BPSGICNoiseLUTBufferSize);
                        }
                        if ((TRUE == m_moduleEnable) && (NULL != pInputData->pipelineBPSData.pDebugDataWriter))
                        {
                            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                                DebugDataTagID::TuningBPSGICPackedNoiseLUT,
                                DebugDataTagType::UInt32,
                                CAMX_ARRAY_SIZE(pInputData->pBPSTuningMetadata->
                                    BPSTuningMetadata480.BPSDMIData.GICLUT.BPSGICNoiseLUT),
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.GICLUT,
                                sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.GICLUT));

                            if (CamxResultSuccess != result)
                            {
                                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
                                result = CamxResultSuccess; // Non-fatal error
                            }
                        }
                        break;
                    case CSLCameraTitanVersion::CSLTitan150:
                    case CSLCameraTitanVersion::CSLTitan160:
                    case CSLCameraTitanVersion::CSLTitan170:
                    case CSLCameraTitanVersion::CSLTitan175:
                        CAMX_STATIC_ASSERT(BPSGICNoiseLUTBufferSize <=
                                        sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.GICLUT));

                        if (NULL != m_pGICNoiseLUT)
                        {
                            Utils::Memcpy(&pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.GICLUT,
                                        m_pGICNoiseLUT,
                                        BPSGICNoiseLUTBufferSize);
                        }
                        if ((TRUE == m_moduleEnable) && (NULL != pInputData->pipelineBPSData.pDebugDataWriter))
                        {
                            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                                DebugDataTagID::TuningBPSGICPackedNoiseLUT,
                                DebugDataTagType::UInt32,
                                CAMX_ARRAY_SIZE(pInputData->pBPSTuningMetadata->
                                    BPSTuningMetadata17x.BPSDMIData.GICLUT.BPSGICNoiseLUT),
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.GICLUT,
                                sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.GICLUT));

                            if (CamxResultSuccess != result)
                            {
                                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
                                result = CamxResultSuccess; // Non-fatal error
                            }
                        }
                        break;
                    default:
                        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Titan Version: %d", pInputData->titanVersion);
                        break;
                }
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupBPS, "UpdateTuningMetadata failed.");
                result = CamxResultSuccess; // Non-fatal error
            }
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupBPS, "UpdateFirmwareData failed.");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGIC30::Execute(
    ISPInputData* pInputData)
{
    CamxResult result           = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        // Check if dependent is valid and been updated compared to last request
        result = ValidateDependenceParams(pInputData);

        if (CamxResultSuccess == result)
        {
            if (TRUE == CheckDependenceChange(pInputData))
            {
                result = RunCalculation(pInputData);
            }

            pInputData->p64bitDMIBuffer = m_pLUTDMICmdBuffer;
            VOID*      pSettingData = static_cast<VOID*>(pInputData);

            if ((CamxResultSuccess == result) && (TRUE == m_moduleEnable))
            {
                result = m_pHWSetting->CreateCmdList(pSettingData, NULL);
                // Switch Banks after update
                m_dependenceData.LUTBankSel ^= 1;
            }

            if (CamxResultSuccess == result)
            {
                UpdateBPSInternalData(pInputData);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Operation failed %d", result);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Parameter dependencies failed");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSGIC30::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30::BPSGIC30
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSGIC30::BPSGIC30(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier     = pNodeIdentifier;
    m_type                = ISPIQModuleType::BPSGIC;
    m_moduleEnable        = FALSE;
    m_numLUT              = MaxGICNoiseLUT;

    m_pGICNoiseLUT = NULL;
    m_pChromatix   = NULL;
    m_pHWSetting   = NULL;

    m_dependenceData.moduleEnable = FALSE; ///< First frame is always FALSE
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30::~BPSGIC30
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSGIC30::~BPSGIC30()
{
    DeallocateCommonLibraryData();
    if (NULL != m_pLUTCmdBufferManager)
    {
        if (NULL != m_pLUTDMICmdBuffer)
        {
            m_pLUTCmdBufferManager->Recycle(m_pLUTDMICmdBuffer);
            m_pLUTDMICmdBuffer = NULL;
        }

        m_pLUTCmdBufferManager->Uninitialize();
        CAMX_DELETE m_pLUTCmdBufferManager;
        m_pLUTCmdBufferManager = NULL;
    }

    m_pChromatix = NULL;
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
