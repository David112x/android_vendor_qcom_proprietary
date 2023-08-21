////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpslinearization34.cpp
/// @brief BPSLinearization34 class implementation
///        Corrects nonlinearity of sensor response and also subtracts black level
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "bps_data.h"
#include "camxbpslinearization34.h"
#include "camxdefs.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "linearization_3_4_0.h"
#include "parametertuningtypes.h"
#include "camxbpslinearization34titan17x.h"
#include "camxbpslinearization34titan480.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLinearization34::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLinearization34::Create(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        BPSLinearization34* pModule = CAMX_NEW BPSLinearization34(pCreateData->pNodeIdentifier);

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
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLinearization34::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLinearization34::Initialize(
    BPSModuleCreateData* pCreateData)
{
    ResourceParams cmdBufferConfig = { 0 };
    CamxResult     result          = CamxResultSuccess;
    ISPInputData*  pInputData      = &pCreateData->initializationData;

    // 32 bit LUT buffer manager is created externally by BPS node
    m_pLUTCmdBufferManager = NULL;

    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            result = BPSLinearization34Titan480::Create(&m_pHWSetting);
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            result = BPSLinearization34Titan17x::Create(&m_pHWSetting);
            break;
        default:
            result = CamxResultEInvalidState;
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
    m_stretchGainBlue      = 1.0;
    m_stretchGainRed       = 1.0;
    m_stretchGainGreenEven = 1.0;
    m_stretchGainGreenOdd  = 1.0;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLinearization34::FillCmdBufferManagerParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLinearization34::FillCmdBufferManagerParams(
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
                                  (NULL != m_pNodeIdentifier) ? m_pNodeIdentifier : " ", "BPSLinearization34");
                pResourceParams->resourceSize                 = (BPSLinearization34LUTLengthDword * sizeof(UINT32));
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
// BPSLinearization34::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLinearization34::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(linearization_3_4_0::linearization34_rgn_dataType) *
                              (Linearization34MaxmiumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for linearization_3_4_0::linearization34_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLinearization34::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSLinearization34::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged               = FALSE;

    if ((NULL != pInputData)                            &&
        (NULL != pInputData->pHwContext)                &&
        (NULL != pInputData->pHALTagsData)              &&
        (NULL != pInputData->pipelineBPSData.pIQSettings))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMBPSIQSetting*>(pInputData->pOEMIQSetting))->LinearizationEnable;
            isChanged      = (TRUE == m_moduleEnable);
        }
        else
        {
            if (((pInputData->pHALTagsData->blackLevelLock == m_blacklevelLock) &&
                 (pInputData->pHALTagsData->blackLevelLock == BlackLevelLockOn)) ||
                ((pInputData->pHALTagsData->controlAWBLock == m_AWBLock) &&
                 (pInputData->pHALTagsData->controlAWBLock == ControlAWBLockOn)))
            {
                isChanged = FALSE;
            }
            else
            {
                TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
                CAMX_ASSERT(NULL != pTuningManager);

                m_blacklevelLock = pInputData->pHALTagsData->blackLevelLock;

                // Search through the tuning data (tree), only when there
                // are changes to the tuning mode data as an optimization
                if ((TRUE == pInputData->tuningModeChanged)    &&
                    (TRUE == pTuningManager->IsValidChromatix()))
                {
                    CAMX_ASSERT(NULL != pInputData->pTuningData);

                    m_pChromatix = pTuningManager->GetChromatix()->GetModule_linearization34_bps(
                                       reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                       pInputData->pTuningData->noOfSelectionParameter);
                }

                CAMX_ASSERT(NULL != m_pChromatix);
                if (NULL != m_pChromatix)
                {
                    if (m_pChromatix != m_dependenceData.pChromatix)
                    {
                        m_dependenceData.pChromatix      = m_pChromatix;
                        m_moduleEnable                   = m_pChromatix->enable_section.linearization_enable;
                        m_dependenceData.symbolIDChanged = TRUE;
                        isChanged                        = (TRUE == m_moduleEnable);
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to get Chromatix");
                }
            }
            CAMX_LOG_VERBOSE(CamxLogGroupBPS, "m_pChromatix:%p isChanged: %d m_moduleEnable:%d"
                "m_AWBLock:%d m_blacklevelLock:%d blackLevelLock:%d controlAWBLock:%d",
                m_pChromatix, isChanged, m_moduleEnable, m_AWBLock, m_blacklevelLock,
                pInputData->pHALTagsData->blackLevelLock, pInputData->pHALTagsData->controlAWBLock);
        }

        if ((TRUE == m_moduleEnable)             &&
            (BlackLevelLockOn != m_blacklevelLock))
        {
            BpsIQSettings* pBPSIQSettings = reinterpret_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
            CamxResult     result         = IQInterface::GetPixelFormat(&pInputData->sensorData.format,
                                                                        &m_dependenceData.bayerPattern);

            m_dependenceData.pedestalEnable = pBPSIQSettings->pedestalParameters.moduleCfg.EN;

            if (TRUE == IQInterface::s_interpolationTable.Linearization34TriggerUpdate(&(pInputData->triggerData),
                                                                                          &m_dependenceData))
            {
                isChanged = TRUE;
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
// BPSLinearization34::FetchDMIBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLinearization34::FetchDMIBuffer(
    const ISPInputData* pInputData)
{
    CamxResult      result          = CamxResultSuccess;
    PacketResource* pPacketResource = NULL;

    CAMX_UNREFERENCED_PARAM(pInputData);

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
        CAMX_LOG_ERROR(CamxLogGroupBPS, "LUT Command Buffer Manager is NULL");
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
// BPSLinearization34::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLinearization34::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        result = FetchDMIBuffer(pInputData);

        if (CamxResultSuccess == result)
        {
            Linearization34OutputData outputData;

            outputData.pDMIDataPtr = reinterpret_cast<UINT32*>(
                                         m_pLUTDMICmdBuffer->BeginCommands(BPSLinearization34LUTLengthDword));
            // BET ONLY - InputData is different per module tested
            pInputData->pBetDMIAddr = static_cast<VOID*>(outputData.pDMIDataPtr);
            CAMX_ASSERT(NULL != outputData.pDMIDataPtr);

            result = IQInterface::BPSLinearization34CalculateSetting(&m_dependenceData,
                                                                     pInputData->pOEMIQSetting,
                                                                     &outputData);
            m_stretchGainBlue      = outputData.stretchGainBlue;
            m_stretchGainRed       = outputData.stretchGainRed;
            m_stretchGainGreenEven = outputData.stretchGainGreenEven;
            m_stretchGainGreenOdd  = outputData.stretchGainGreenOdd;

            if (CamxResultSuccess == result)
            {
                m_pLUTDMICmdBuffer->CommitCommands();
                if (NULL != pInputData->pBPSTuningMetadata)
                {
                    m_pLUTDMIBuffer = outputData.pDMIDataPtr;
                }
                if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
                {
                    m_pHWSetting->DumpRegConfig();
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Linearization Calculation Failed. result %d", result);
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
// BPSLinearization34::UpdateBPSInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSLinearization34::UpdateBPSInternalData(
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
                switch (pInputData->titanVersion)
                {
                    case CSLCameraTitanVersion::CSLTitan480:
                        CAMX_STATIC_ASSERT(BPSLinearization34LUTLengthBytes <=
                            sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.linearizationLUT));

                        if (NULL != m_pLUTDMIBuffer)
                        {
                            Utils::Memcpy(
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.linearizationLUT,
                                m_pLUTDMIBuffer,
                                BPSLinearization34LUTLengthBytes);
                        }

                        if ((TRUE == m_moduleEnable) &&
                            (NULL != pInputData->pipelineBPSData.pDebugDataWriter))
                        {
                            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                                DebugDataTagID::TuningBPSLinearizationPackedLUT,
                                DebugDataTagType::UInt32,
                                CAMX_ARRAY_SIZE(pInputData->pBPSTuningMetadata->
                                    BPSTuningMetadata480.BPSDMIData.linearizationLUT.linearizationLUT),
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.linearizationLUT,
                                sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.linearizationLUT));

                            if (CamxResultSuccess != result)
                            {
                                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
                            }
                        }

                        break;
                    case CSLCameraTitanVersion::CSLTitan150:
                    case CSLCameraTitanVersion::CSLTitan160:
                    case CSLCameraTitanVersion::CSLTitan170:
                    case CSLCameraTitanVersion::CSLTitan175:
                        CAMX_STATIC_ASSERT(BPSLinearization34LUTLengthBytes <=
                            sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.linearizationLUT));

                        if (NULL != m_pLUTDMIBuffer)
                        {
                            Utils::Memcpy(
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.linearizationLUT,
                                m_pLUTDMIBuffer,
                                BPSLinearization34LUTLengthBytes);
                        }

                        if ((TRUE == m_moduleEnable) && (pInputData->pipelineBPSData.pDebugDataWriter != NULL))
                        {
                            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                                DebugDataTagID::TuningBPSLinearizationPackedLUT,
                                DebugDataTagType::UInt32,
                                CAMX_ARRAY_SIZE(pInputData->pBPSTuningMetadata->
                                    BPSTuningMetadata17x.BPSDMIData.linearizationLUT.linearizationLUT),
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.linearizationLUT,
                                sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.linearizationLUT));

                            if (CamxResultSuccess != result)
                            {
                                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
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
// BPSLinearization34::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLinearization34::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            result = RunCalculation(pInputData);
        }

        pInputData->pCalculatedMetadata->stretchGainBlue      = m_stretchGainBlue;
        pInputData->pCalculatedMetadata->stretchGainRed       = m_stretchGainRed;
        pInputData->pCalculatedMetadata->stretchGainGreenEven = m_stretchGainGreenEven;
        pInputData->pCalculatedMetadata->stretchGainGreenOdd  = m_stretchGainGreenOdd;

        pInputData->p64bitDMIBuffer = m_pLUTDMICmdBuffer;

        if ((CamxResultSuccess == result) && (TRUE == m_moduleEnable))
        {
            result = m_pHWSetting->CreateCmdList(pInputData, NULL);
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
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer %p, m_pHWSetting %p", pInputData, m_pHWSetting);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLinearization34::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSLinearization34::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLinearization34::BPSLinearization34
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSLinearization34::BPSLinearization34(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier = pNodeIdentifier;
    m_type            = ISPIQModuleType::BPSLinearization;
    m_moduleEnable    = FALSE;
    m_numLUT          = LinearizationMaxLUT;
    m_pLUTDMIBuffer   = NULL;
    m_pChromatix      = NULL;
    m_pHWSetting      = NULL;
    m_AWBLock         = ControlAWBLockOff;
    m_blacklevelLock  = BlackLevelLockOff;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLinearization34::~BPSLinearization34
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSLinearization34::~BPSLinearization34()
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
