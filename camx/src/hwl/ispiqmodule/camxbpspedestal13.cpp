////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpspedestal13.cpp
/// @brief CAMXBPSPEDESTAL13 class implementation
///        Corrects spatially non-uniform black levels from the pixel data by programmable meshes and optimally scale
///        the pedestal corrected pixel data to maximum dynamic range.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxbpspedestal13.h"
#include "camxbpspedestal13titan17x.h"
#include "camxbpspedestal13titan480.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxisphwsetting.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"
#include "pedestal13setting.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSPedestal13::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPedestal13::Create(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        BPSPedestal13* pModule = CAMX_NEW BPSPedestal13(pCreateData->pNodeIdentifier);

        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Module initialization failed !!");
                CAMX_DELETE(pModule);
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
// BPSPedestal13::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPedestal13::Initialize(
    BPSModuleCreateData* pCreateData)
{
    ResourceParams cmdBufferConfig = { 0 };
    CamxResult     result          = CamxResultSuccess;
    ISPInputData* pInputData       = &pCreateData->initializationData;

    // 32 bit LUT buffer manager is created externally by BPS node
    m_pLUTCmdBufferManager = NULL;

    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            result = BPSPedestal13Titan480::Create(&m_pHWSetting);
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            result = BPSPedestal13Titan17x::Create(&m_pHWSetting);
            break;
        default:
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid hardware version: %d", pCreateData->titanVersion);
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
// BPSPedestal13::FillCmdBufferManagerParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPedestal13::FillCmdBufferManagerParams(
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
                                  (NULL != m_pNodeIdentifier) ? m_pNodeIdentifier : " ", "BPSPedestal13");
                pResourceParams->resourceSize                 = (BPSPedestal13DMILengthDword * sizeof(UINT32));
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
// BPSPedestal13::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPedestal13::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(pedestal_1_3_0::pedestal13_rgn_dataType) * (Pedestal13MaxmiumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for pedestal_1_3_0::pedestal13_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSPedestal13::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSPedestal13::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData)               &&
        (NULL != pInputData->pHwContext)   &&
        (NULL != pInputData->pHALTagsData))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMBPSIQSetting*>(pInputData->pOEMIQSetting))->PedestalEnable;
            isChanged      = (TRUE == m_moduleEnable);
        }
        else
        {
            m_blacklevelLock = pInputData->pHALTagsData->blackLevelLock;
            if (BlackLevelLockOn == m_blacklevelLock)
            {
                isChanged = FALSE;
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

                    m_pChromatix = pTuningManager->GetChromatix()->GetModule_pedestal13_bps(
                                       reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                       pInputData->pTuningData->noOfSelectionParameter);
                }

                CAMX_ASSERT(NULL != m_pChromatix);
                if (NULL != m_pChromatix)
                {
                    if (m_pChromatix != m_dependenceData.pChromatix)
                    {
                        m_dependenceData.pChromatix      = m_pChromatix;
                        m_moduleEnable                   = m_pChromatix->enable_section.pedestal_enable;
                        m_dependenceData.symbolIDChanged = TRUE;
                        isChanged                        = (TRUE == m_moduleEnable);
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to get Chromatix");
                    }
                }
            }
        }

        CamxResult result = IQInterface::GetPixelFormat(&pInputData->sensorData.format, &m_dependenceData.bayerPattern);

        if ((TRUE             == m_moduleEnable) &&
            (BlackLevelLockOn != m_blacklevelLock))
        {
            if (TRUE == IQInterface::s_interpolationTable.pedestal13TriggerUpdate(&(pInputData->triggerData),
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
                           "Invalid Input: pHwContext:%p",
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
// BPSPedestal13::FetchDMIBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPedestal13::FetchDMIBuffer()
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
// BPSPedestal13::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPedestal13::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        result = FetchDMIBuffer();

        if (CamxResultSuccess == result)
        {
            Pedestal13OutputData outputData;

            outputData.type = PipelineType::BPS;

            outputData.pGRRLUTDMIBuffer =
                reinterpret_cast<UINT32*>(m_pLUTDMICmdBuffer->BeginCommands(BPSPedestal13DMILengthDword));
            CAMX_ASSERT(NULL != outputData.pGRRLUTDMIBuffer);
            // BET ONLY - InputData is different per module tested
            pInputData->pBetDMIAddr = static_cast<VOID*>(outputData.pGRRLUTDMIBuffer);

            outputData.pGBBLUTDMIBuffer =
                reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(outputData.pGRRLUTDMIBuffer) + BPSPedestal13LUTTableSize);
            CAMX_ASSERT(NULL != outputData.pGBBLUTDMIBuffer);

            result = IQInterface::Pedestal13CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);
            if (CamxResultSuccess == result)
            {
                m_pLUTDMICmdBuffer->CommitCommands();

                if (NULL != pInputData->pBPSTuningMetadata)
                {
                    m_pGRRLUTDMIBuffer  = outputData.pGRRLUTDMIBuffer;
                    m_pGBBLUTDMIBuffer  = outputData.pGBBLUTDMIBuffer;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Pedestal Calculation Failed %d", result);
            }
        }
        if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
        {
            m_pHWSetting->DumpRegConfig();
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
// BPSPedestal13::UpdateBPSInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSPedestal13::UpdateBPSInternalData(
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
                        CAMX_STATIC_ASSERT(BPSPedestal13LUTTableSize <=
                            sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.pedestalLUT.GRRLUT));

                        if (NULL != m_pGRRLUTDMIBuffer)
                        {
                            Utils::Memcpy(
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.pedestalLUT.GRRLUT[0],
                                m_pGRRLUTDMIBuffer,
                                BPSPedestal13LUTTableSize);
                        }
                        if (NULL != m_pGBBLUTDMIBuffer)
                        {
                            Utils::Memcpy(
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.pedestalLUT.GBBLUT[0],
                                m_pGBBLUTDMIBuffer,
                                BPSPedestal13LUTTableSize);
                        }

                        if ((TRUE == m_moduleEnable) && (NULL != pInputData->pipelineBPSData.pDebugDataWriter))
                        {
                            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                                DebugDataTagID::TuningBPSPedestalPackedLUT,
                                DebugDataTagType::TuningPedestalLUT,
                                1,
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.pedestalLUT,
                                sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.pedestalLUT));

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
                        CAMX_STATIC_ASSERT(BPSPedestal13LUTTableSize <=
                            sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.pedestalLUT.GRRLUT));

                        if (NULL != m_pGRRLUTDMIBuffer)
                        {
                            Utils::Memcpy(
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.pedestalLUT.GRRLUT[0],
                                m_pGRRLUTDMIBuffer,
                                BPSPedestal13LUTTableSize);
                        }
                        if (NULL != m_pGBBLUTDMIBuffer)
                        {
                            Utils::Memcpy(
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.pedestalLUT.GBBLUT[0],
                                m_pGBBLUTDMIBuffer,
                                BPSPedestal13LUTTableSize);
                        }

                        if ((TRUE == m_moduleEnable) && (NULL != pInputData->pipelineBPSData.pDebugDataWriter))
                        {
                            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                                DebugDataTagID::TuningBPSPedestalPackedLUT,
                                DebugDataTagType::TuningPedestalLUT,
                                1,
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.pedestalLUT,
                                sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.pedestalLUT));

                            if (CamxResultSuccess != result)
                            {
                                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
                                result = CamxResultSuccess; // Non-fatal error
                            }
                        }
                        break;
                    default:
                        CAMX_LOG_WARN(CamxLogGroupBPS, "Invalid hardware version: %d", pInputData->titanVersion);
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
// BPSPedestal13::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPedestal13::Execute(
    ISPInputData* pInputData)
{
    CamxResult result  = CamxResultSuccess;

    if (NULL != pInputData)
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            result = RunCalculation(pInputData);
        }

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
// BPSPedestal13::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSPedestal13::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSPedestal13::BPSPedestal13
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSPedestal13::BPSPedestal13(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier     = pNodeIdentifier;
    m_type                = ISPIQModuleType::BPSPedestalCorrection;
    m_moduleEnable        = FALSE;
    m_pGRRLUTDMIBuffer    = NULL;
    m_pGBBLUTDMIBuffer    = NULL;
    m_pChromatix          = NULL;
    m_pHWSetting          = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSPedestal13::~BPSPedestal13
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSPedestal13::~BPSPedestal13()
{
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
    DeallocateCommonLibraryData();
}

CAMX_NAMESPACE_END
