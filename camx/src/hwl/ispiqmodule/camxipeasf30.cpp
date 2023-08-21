////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipeasf30.cpp
/// @brief IPEASF30 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcdmdefs.h"
#include "camxdefs.h"
#include "camxipeasf30.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "ipe_data.h"
#include "parametertuningtypes.h"
#include "camxtitan17xcontext.h"
#include "asf30setting.h"
#include "camxipeasf30titan17x.h"
#include "camxipeasf30titan480.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEASF30::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEASF30::Create(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        IPEASF30* pModule = CAMX_NEW IPEASF30(pCreateData->pNodeIdentifier);

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
// IPEASF30::UpdateIPEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEASF30::UpdateIPEInternalData(
    ISPInputData* pInputData
    ) const
{
    CamxResult      result         = CamxResultSuccess;
    IpeIQSettings*  pIPEIQSettings = static_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);

    CAMX_ASSERT(NULL != pInputData);

    if (NULL != pIPEIQSettings)
    {
        if (TRUE == m_moduleEnable)
        {
            CamX::Utils::Memcpy(&pIPEIQSettings->asfParameters, &m_ASFParameters, sizeof(AsfParameters));
            if (TRUE == m_bypassMode)
            {
                pIPEIQSettings->asfParameters.moduleCfg.EN            = 1;
                pIPEIQSettings->asfParameters.moduleCfg.LAYER_1_EN    = 0;
                pIPEIQSettings->asfParameters.moduleCfg.LAYER_2_EN    = 0;
                pIPEIQSettings->asfParameters.moduleCfg.EDGE_ALIGN_EN = 0;
                pIPEIQSettings->asfParameters.moduleCfg.CONTRAST_EN   = 0;
            }
        }
        else
        {
            pIPEIQSettings->asfParameters.moduleCfg.EN = m_moduleEnable;
        }
    }

    // Disable additional ASF modules for realtime streams
    if ((TRUE == pInputData->pipelineIPEData.realtimeFlag) &&
        (TRUE == pInputData->pipelineIPEData.compressiononOutput) &&
        (NULL != pIPEIQSettings))
    {
        pIPEIQSettings->asfParameters.moduleCfg.LAYER_2_EN     = 0;
        pIPEIQSettings->asfParameters.moduleCfg.EDGE_ALIGN_EN  = 0;
        pIPEIQSettings->asfParameters.moduleCfg.CONTRAST_EN    = 0;
        pIPEIQSettings->asfParameters.moduleCfg.CHROMA_GRAD_EN = 0;
    }

    // Post tuning metadata if setting is enabled
    if (NULL != pInputData->pIPETuningMetadata)
    {
        /// @todo (CAMX-2846) Tuning data has been keep enough for the 68 registers, but writing only 65, data for others,
        ///                   should be obtain from FW.
        result = m_pHWSetting->UpdateTuningMetadata(pInputData);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Update Tuning Metadata failed.");
            result = CamxResultSuccess; // Non-fatal error
        }

        TuningFaceData* pASFFaceDetection = NULL;
        IPEASF30LUT*    pTuningASFLUT     = NULL;

        switch (pInputData->titanVersion)
        {
            case CSLCameraTitanVersion::CSLTitan480:
                pASFFaceDetection = &pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEASFFaceDetection;
                pTuningASFLUT     = &pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEDMIData.ASFLUT;
                break;
            case CSLCameraTitanVersion::CSLTitan150:
            case CSLCameraTitanVersion::CSLTitan160:
            case CSLCameraTitanVersion::CSLTitan170:
            case CSLCameraTitanVersion::CSLTitan175:
                pASFFaceDetection = &pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPEASFFaceDetection;
                pTuningASFLUT     = &pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPEDMIData.ASFLUT;
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid hardware version: %d", pInputData->titanVersion);
                break;
        }

        if (IPEASFLUTBufferSize == sizeof(IPEASF30LUT))
        {
            if ((NULL != pTuningASFLUT) && (NULL != m_pASFLUTs))
            {
                Utils::Memcpy(pTuningASFLUT, m_pASFLUTs, IPEASFLUTBufferSize);
            }
        }
        else
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Tuning data, incorrect LUT buffer size");
        }

        CAMX_STATIC_ASSERT(MAX_FACE_NUM <= TuningMaxFaceNumber);

        if ((NULL != pASFFaceDetection) && (NULL != m_dependenceData.pFDData))
        {
            pASFFaceDetection->numberOfFace =
                ((m_dependenceData.pFDData->numberOfFace) > MAX_FACE_NUM) ?
                MAX_FACE_NUM : (m_dependenceData.pFDData->numberOfFace);

            for (INT32 count = 0; count < pASFFaceDetection->numberOfFace; count++)
            {
                pASFFaceDetection->faceRadius[count]  = m_dependenceData.pFDData->faceRadius[count];
                pASFFaceDetection->faceCenterX[count] = m_dependenceData.pFDData->faceCenterX[count];
                pASFFaceDetection->faceCenterY[count] = m_dependenceData.pFDData->faceCenterY[count];
            }
        }
        if ((NULL != pASFFaceDetection) &&
            (NULL != pTuningASFLUT)     &&
            (NULL != pIPEIQSettings)    &&
            (TRUE == pIPEIQSettings->asfParameters.moduleCfg.EN))
        {
            DebugDataTagID faceTagID;
            DebugDataTagID packedLUTTagId;

            if (TRUE == pInputData->pipelineIPEData.realtimeFlag)
            {
                faceTagID      = DebugDataTagID::TuningIPEASFFace;
                packedLUTTagId = DebugDataTagID::TuningIPEASF30PackedLUT;
            }
            else
            {
                faceTagID      = DebugDataTagID::TuningIPEASFFaceOffline;
                packedLUTTagId = DebugDataTagID::TuningIPEASF30PackedLUTOffline;
            }
            result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                faceTagID,
                DebugDataTagType::TuningFaceData,
                1,
                pASFFaceDetection,
                sizeof(TuningFaceData));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
                result = CamxResultSuccess; // Non-fatal error
            }

            result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                packedLUTTagId,
                DebugDataTagType::TuningASF30LUT,
                1,
                pTuningASFLUT,
                sizeof(IPEASF30LUT));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
                result = CamxResultSuccess; // Non-fatal error
            }
        }
    }

    if (NULL != pInputData->pCalculatedData)
    {
        pInputData->pCalculatedData->metadata.edgeMode = m_dependenceData.edgeMode;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEASF30::FillCmdBufferManagerParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEASF30::FillCmdBufferManagerParams(
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
                                  (NULL != m_pNodeIdentifier) ? m_pNodeIdentifier : " ", "IPEASF30");
                pResourceParams->resourceSize                 = IPEASFLUTBufferSize;
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
// IPEASF30::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEASF30::Initialize(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    m_pHWSetting = NULL;

    // Check Module Hardware Version and Create HW Setting Object
    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IPEASF30Titan480;
            break;
        default:
            m_pHWSetting = CAMX_NEW IPEASF30Titan17x;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        UINT regRangeSizeInDwords = PacketBuilder::RequiredWriteRegRangeSizeInDwords(m_pHWSetting->GetRegSize());
        m_cmdLength               = regRangeSizeInDwords * RegisterWidthInBytes;

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
// IPEASF30::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEASF30::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(asf_3_0_0::asf30_rgn_dataType) * (ASF30MaxNoLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Allocate memory for asf_3_0_0::asf30_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEASF30::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEASF30::ValidateDependenceParams(
    const ISPInputData* pInputData
    ) const
{
    CamxResult result = CamxResultSuccess;

    /// @todo (CAMX-1807) validate dependency parameters
    if ((NULL == pInputData)                                                     ||
        (NULL == pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader]) ||
        (NULL == pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferPostLTM])   ||
        (NULL == pInputData->pipelineIPEData.pIPEIQSettings))
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEASF30::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPEASF30::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData)                  &&
        (NULL != pInputData->pAECUpdateData)  &&
        (NULL != pInputData->pAWBUpdateData))
    {
        ISPHALTagsData* pHALTagsData = pInputData->pHALTagsData;
        UINT32          sceneMode    = 0;
        UINT32          splEffect    = 0;

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Lux Index [0x%f] ", pInputData->pAECUpdateData->luxIndex);


        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIPEIQSetting*>(pInputData->pOEMIQSetting))->ASFEnable;

            isChanged      = (TRUE == m_moduleEnable);
        }
        else
        {
            if ((NULL != pInputData->pTuningData)  &&
                (NULL != pInputData->pHALTagsData))
            {
                TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
                CAMX_ASSERT(NULL != pTuningManager);

                // Search through the tuning data (tree), only when there
                // are changes to the tuning mode data as an optimization
                if ((TRUE == pInputData->tuningModeChanged)    &&
                    (TRUE == pTuningManager->IsValidChromatix()))
                {
                    if (pInputData->pTuningData->noOfSelectionParameter == 7)
                    {
                        sceneMode = static_cast<UINT32>(pInputData->pTuningData->TuningMode[5].subMode.scene);
                        splEffect = static_cast<UINT32>(pInputData->pTuningData->TuningMode[6].subMode.effect);
                    }

                    m_pChromatix = pTuningManager->GetChromatix()->GetModule_asf30_ipe(
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
                        m_moduleEnable              = m_pChromatix->enable_section.asf_enable;

                        if (TRUE == m_moduleEnable)
                        {
                            isChanged = TRUE;
                        }
                    }

                    if (TRUE == m_moduleEnable)
                    {
                        m_dependenceData.layer1Enable    = static_cast<UINT16>(
                            m_dependenceData.pChromatix->chromatix_asf30_reserve.layer_1_enable);
                        m_dependenceData.layer2Enable    = static_cast<UINT16>(
                            m_dependenceData.pChromatix->chromatix_asf30_reserve.layer_2_enable);
                        m_dependenceData.contrastEnable  = static_cast<UINT16>(
                            m_dependenceData.pChromatix->chromatix_asf30_reserve.contrast_enable);
                        m_dependenceData.radialEnable    = static_cast<UINT16>(
                            m_dependenceData.pChromatix->chromatix_asf30_reserve.radial_enable);
                        m_dependenceData.edgeAlignEnable = static_cast<UINT16>(
                            m_dependenceData.pChromatix->chromatix_asf30_reserve.edge_alignment_enable);
                    }
                    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "EdgeMode chromatix %d hal %d edgeMode %d m_moduleEnable %d",
                        m_pChromatix->enable_section.asf_enable,
                        pHALTagsData->edgeMode, m_dependenceData.edgeMode, m_moduleEnable);
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

        if (TRUE == m_moduleEnable)
        {
            SetAdditionalASF30Input(pInputData);

            if (TRUE ==
                IQInterface::s_interpolationTable.ASF30TriggerUpdate(&pInputData->triggerData, &m_dependenceData))
            {
                isChanged = TRUE;
            }
        }
        if (NULL != pHALTagsData)
        {
            if (m_dependenceData.sharpness != pHALTagsData->sharpness)
            {
                isChanged                  = TRUE;
                m_dependenceData.sharpness = pHALTagsData->sharpness;
            }
            m_dependenceData.edgeMode = pHALTagsData->edgeMode;

            m_bypassMode = FALSE;
            if (((EdgeModeOff            == pHALTagsData->edgeMode)         ||
                ((EdgeModeZeroShutterLag == pHALTagsData->edgeMode)         &&
                (FALSE == pInputData->pipelineIPEData.isLowResolution)))    &&
                (0                       == sceneMode)                      &&
                (0                       == splEffect))
            {
                m_bypassMode = TRUE;
            }

            CAMX_LOG_VERBOSE(CamxLogGroupPProc,
                "EdgeMode %d m_bypassMode %d sceneMode %d splEffect %d sharpness %f",
                pHALTagsData->edgeMode, m_bypassMode, sceneMode, splEffect, m_dependenceData.sharpness);
        }
    }
    else
    {
        if (NULL != pInputData)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc,
                    "Invalid Input: pNewAECUpdate %p pNewAWBUpdate %p",
                    pInputData->pAECUpdateData,
                    pInputData->pAWBUpdateData);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Null Input Pointer");
        }
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEASF30::FetchDMIBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEASF30::FetchDMIBuffer()
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

        // Fetch a fresh LUT DMI cmd buffer
        result = m_pLUTCmdBufferManager->GetBuffer(&pPacketResource);
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "LUT Cmd Buffer Manager is NULL");
    }

    if (CamxResultSuccess == result)
    {
        CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().cmdBuffer);
        m_pLUTDMICmdBuffer = static_cast<CmdBuffer*>(pPacketResource);
    }
    else
    {
        m_pLUTDMICmdBuffer = NULL;
        result             = CamxResultEUnableToLoad;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to fetch DMI Buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEASF30::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEASF30::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult      result         = CamxResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Lux Index [0x%f] ", pInputData->pAECUpdateData->luxIndex);

    result = FetchDMIBuffer();

    if (CamxResultSuccess == result)
    {
        ASF30OutputData outputData = { 0 };
        UINT32*         pLUT       = reinterpret_cast<UINT32*>(m_pLUTDMICmdBuffer->BeginCommands(IPEASFLUTBufferSizeInDWord));
        CAMX_ASSERT(NULL != pLUT);

        outputData.pDMIDataPtr    = pLUT;
        outputData.pAsfParameters = &m_ASFParameters;

        result = IQInterface::IPEASF30CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);
        if (CamxResultSuccess == result)
        {
            result = m_pLUTDMICmdBuffer->CommitCommands();

            if (NULL != pInputData->pIPETuningMetadata)
            {
                m_pASFLUTs = pLUT;
            }
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "ASF30 Calculation Failed. result %d", result);
        }

        if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
        {
            m_pHWSetting->DumpRegConfig();
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Cannot get buffer from CmdBufferManager");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEASF30::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEASF30::Execute(
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
                // Record offset where CDM DMI header starts for LUTs for the module
                // Patch the LUTs DMI buffer into CDM pack
                CmdBuffer* pDMICmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader];
                m_offsetLUT              = (pDMICmdBuffer->GetResourceUsedDwords() * sizeof(UINT32));

                ASFHwSettingParams  moduleHWSettingParams;
                moduleHWSettingParams.pLUTDMICmdBuffer = m_pLUTDMICmdBuffer;

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
// IPEASF30::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEASF30::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEASF30::IPEASF30
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEASF30::IPEASF30(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier         = pNodeIdentifier;
    m_type                    = ISPIQModuleType::IPEASF;
    m_moduleEnable            = TRUE;
    m_numLUT                  = MaxASFLUT - 1;

    m_pASFLUTs                = NULL;
    m_pChromatix              = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEASF30::~IPEASF30
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEASF30::~IPEASF30()
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

    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }

    m_pChromatix = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEASF30::SetAdditionalASF30Input
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEASF30::SetAdditionalASF30Input(
    ISPInputData* pInputData)
{
    UINT8 negateAbsoluteY1            = 0;
    UINT8 specialEffectAbsoluteEnable = 0;
    UINT8 specialEffectEnable         = 0;

    CAMX_ASSERT(NULL != pInputData);

    for (UINT i = 0; i < pInputData->pTuningData->noOfSelectionParameter; i++)
    {
        if (ChiModeType::Effect == pInputData->pTuningData->TuningMode[i].mode)
        {
            specialEffectEnable = 1;
            switch(pInputData->pTuningData->TuningMode[i].subMode.effect)
            {
                case ChiModeEffectSubModeType::Emboss:
                    break;
                case ChiModeEffectSubModeType::Sketch:
                    specialEffectAbsoluteEnable = 1;
                    negateAbsoluteY1            = 1;
                    break;
                case ChiModeEffectSubModeType::Neon:
                    specialEffectAbsoluteEnable = 1;
                    break;
                default:
                    specialEffectEnable         = 0;
                    break;
            }
        }
    }

    m_dependenceData.negateAbsoluteY1            = negateAbsoluteY1;
    m_dependenceData.specialEffectAbsoluteEnable = specialEffectAbsoluteEnable;
    m_dependenceData.specialEffectEnable         = specialEffectEnable;
    m_dependenceData.specialPercentage           = 0;
    m_dependenceData.smoothPercentage            = 0;

    if (1 == specialEffectEnable)
    {
        m_dependenceData.nonZero[0] = 0x1;
        m_dependenceData.nonZero[1] = 0x1;
        m_dependenceData.nonZero[2] = 0x0;
        m_dependenceData.nonZero[3] = 0x3;
        m_dependenceData.nonZero[4] = 0x3;
        m_dependenceData.nonZero[5] = 0x3;
        m_dependenceData.nonZero[6] = 0x0;
        m_dependenceData.nonZero[7] = 0x1;

    }
    else
    {
        for (UINT i = 0; i < NUM_OF_NZ_ENTRIES; i++)
        {
            m_dependenceData.nonZero[i] = 1;
        }
    }

    // @todo (CAMX-2109) Map scale ratio from ISP to algorithm interface
    // m_dependenceData.totalScaleRatio   = 1.0f;

    m_dependenceData.chYStreamInWidth  = pInputData->pipelineIPEData.inputDimension.widthPixels;
    m_dependenceData.chYStreamInHeight = pInputData->pipelineIPEData.inputDimension.heightLines;

    // @todo (CAMX-2143) Map ASF Face parameters from ISP to algorithm interface
    m_dependenceData.pFDData               = &pInputData->fDData;
    m_dependenceData.pWarpGeometriesOutput = pInputData->pipelineIPEData.pWarpGeometryData;
    m_dependenceData.faceHorzOffset        = 0;
    m_dependenceData.faceVertOffset        = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEASF30::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEASF30::DumpRegConfig(
    UINT32* pDMIDataPtr
    ) const
{
    CHAR  dumpFilename[256];
    FILE* pFile = NULL;
    CamX::OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename), "%s/IPE_regdump.txt", ConfigFileDirectory);
    pFile = CamX::OsUtils::FOpen(dumpFilename, "a+");

    if (NULL != pFile)
    {
        // Register dump done in ASF HW setting class

        CamX::OsUtils::FPrintF(pFile, "\n* ASF30 Module CGF \n");

        CamX::OsUtils::FPrintF(pFile, "asfParameters.moduleCfg.EN                 = %d\n",
            m_ASFParameters.moduleCfg.EN);
        CamX::OsUtils::FPrintF(pFile, "asfParameters.moduleCfg.SP_EFF_EN          = %d\n",
            m_ASFParameters.moduleCfg.SP_EFF_EN);
        CamX::OsUtils::FPrintF(pFile, "asfParameters.moduleCfg.SP_EFF_ABS_EN      = %d\n",
            m_ASFParameters.moduleCfg.SP_EFF_ABS_EN);
        CamX::OsUtils::FPrintF(pFile, "asfParameters.moduleCfg.LAYER_1_EN         = %d\n",
            m_ASFParameters.moduleCfg.LAYER_1_EN);
        CamX::OsUtils::FPrintF(pFile, "asfParameters.moduleCfg.LAYER_2_EN         = %d\n",
            m_ASFParameters.moduleCfg.LAYER_2_EN);
        CamX::OsUtils::FPrintF(pFile, "asfParameters.moduleCfg.CONTRAST_EN        = %d\n",
            m_ASFParameters.moduleCfg.CONTRAST_EN);
        CamX::OsUtils::FPrintF(pFile, "asfParameters.moduleCfg.CHROMA_GRAD_EN     = %d\n",
            m_ASFParameters.moduleCfg.CHROMA_GRAD_EN);
        CamX::OsUtils::FPrintF(pFile, "asfParameters.moduleCfg.EDGE_ALIGN_EN      = %d\n",
            m_ASFParameters.moduleCfg.EDGE_ALIGN_EN);
        CamX::OsUtils::FPrintF(pFile, "asfParameters.moduleCfg.SKIN_EN            = %d\n",
            m_ASFParameters.moduleCfg.SKIN_EN);
        CamX::OsUtils::FPrintF(pFile, "asfParameters.moduleCfg.RNR_ENABLE         = %d\n",
            m_ASFParameters.moduleCfg.RNR_ENABLE);
        CamX::OsUtils::FPrintF(pFile, "asfParameters.moduleCfg.NEG_ABS_Y1         = %d\n",
            m_ASFParameters.moduleCfg.NEG_ABS_Y1);
        CamX::OsUtils::FPrintF(pFile, "asfParameters.moduleCfg.SP                 = %d\n",
            m_ASFParameters.moduleCfg.SP);


        if (TRUE == m_moduleEnable)
        {
            UINT32 offset               = 0;

            CamX::OsUtils::FPrintF(pFile, "* asf30Data.layer_1_soft_threshold_lut[2][256] = \n");
            for (UINT index = 0; index < DMI_WEIGHT_MOD_SIZE; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT8>((*(pDMIDataPtr+offset+index)) & 0xFF));
            }
            CamX::OsUtils::FPrintF(pFile, "\n* asf30Data.layer_1_gain_negative_lut[2][256] = \n");
            for (UINT index = 0; index < DMI_WEIGHT_MOD_SIZE; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT8>((*(pDMIDataPtr+offset+index) >> 8) & 0xFF));
            }
            CamX::OsUtils::FPrintF(pFile, "\n* asf30Data.layer_1_gain_positive_lut[2][256] = \n");
            for (UINT index = 0; index < DMI_WEIGHT_MOD_SIZE; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT8>((*(pDMIDataPtr+offset+index) >> 16) & 0xFF));
            }
            CamX::OsUtils::FPrintF(pFile, "\n* asf30Data.layer_1_activity_normalization_lut[2][256] = \n");
            for (UINT index = 0; index < DMI_WEIGHT_MOD_SIZE; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT8>((*(pDMIDataPtr+offset+index) >> 24) & 0xFF));
            }

            offset += DMI_WEIGHT_MOD_SIZE;
            CamX::OsUtils::FPrintF(pFile, "\n* asf30Data.layer_1_gain_weight_lut[2][256] = \n");
            for (UINT index = 0; index < DMI_WEIGHT_MOD_SIZE; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT8>((*(pDMIDataPtr + offset + index)) & 0xFF));
            }

            offset += DMI_WEIGHT_MOD_SIZE;
            CamX::OsUtils::FPrintF(pFile, "\n* asf30Data.layer_1_soft_threshold_weight_lut[2][256] = \n");
            for (UINT index = 0; index < DMI_WEIGHT_MOD_SIZE; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT8>((*(pDMIDataPtr + offset + index)) & 0xFF));
            }

            offset += DMI_WEIGHT_MOD_SIZE;
            CamX::OsUtils::FPrintF(pFile, "\n* asf30Data.layer_2_soft_threshold_lut[2][256] = \n");
            for (UINT index = 0; index < DMI_WEIGHT_MOD_SIZE; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT8>((*(pDMIDataPtr+offset+index)) & 0xFF));
            }
            CamX::OsUtils::FPrintF(pFile, "\n* asf30Data.layer_2_gain_negative_lut[2][256] = \n");
            for (UINT index = 0; index < DMI_WEIGHT_MOD_SIZE; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT8>((*(pDMIDataPtr+offset+index) >> 8) & 0xFF));
            }
            CamX::OsUtils::FPrintF(pFile, "\n* asf30Data.layer_2_gain_positive_lut[2][256] = \n");
            for (UINT index = 0; index < DMI_WEIGHT_MOD_SIZE; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT8>((*(pDMIDataPtr+offset+index) >> 16) & 0xFF));
            }
            CamX::OsUtils::FPrintF(pFile, "\n* asf30Data.layer_2_activity_normalization_lut[2][256] = \n");
            for (UINT index = 0; index < DMI_WEIGHT_MOD_SIZE; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT8>((*(pDMIDataPtr+offset+index) >> 24) & 0xFF));
            }

            offset += DMI_WEIGHT_MOD_SIZE;
            CamX::OsUtils::FPrintF(pFile, "\n* asf30Data.layer_2_gain_weight_lut[2][256] = \n");
            for (UINT index = 0; index < DMI_WEIGHT_MOD_SIZE; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT8>((*(pDMIDataPtr + offset + index)) & 0xFF));
            }

            offset += DMI_WEIGHT_MOD_SIZE;
            CamX::OsUtils::FPrintF(pFile, "\n* asf30Data.layer_2_soft_threshold_weight_lut[2][256] = \n");
            for (UINT index = 0; index < DMI_WEIGHT_MOD_SIZE; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT8>((*(pDMIDataPtr + offset + index)) & 0xFF));
            }

            offset += DMI_WEIGHT_MOD_SIZE;
            CamX::OsUtils::FPrintF(pFile, "\n* asf30Data.chroma_gradient_negative_lut[2][256] = \n");
            for (UINT index = 0; index < DMI_WEIGHT_MOD_SIZE; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT8>((*(pDMIDataPtr + offset + index)) & 0xFF));
            }
            CamX::OsUtils::FPrintF(pFile, "\n* asf30Data.chroma_gradient_positive_lut[2][256] = \n");
            for (UINT index = 0; index < DMI_WEIGHT_MOD_SIZE; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT8>((*(pDMIDataPtr + offset + index)>>8) & 0xFF));
            }

            offset += DMI_WEIGHT_MOD_SIZE;
            CamX::OsUtils::FPrintF(pFile, "\n* asf30Data.gain_contrast_negative_lut[2][256] = \n");
            for (UINT index = 0; index < DMI_WEIGHT_MOD_SIZE; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT8>((*(pDMIDataPtr + offset + index)) & 0xFF));
            }
            CamX::OsUtils::FPrintF(pFile, "\n* asf30Data.gain_contrast_positive_lut[2][256] = \n");
            for (UINT index = 0; index < DMI_WEIGHT_MOD_SIZE; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT8>((*(pDMIDataPtr + offset + index)>>8) & 0xFF));
            }

            offset += DMI_WEIGHT_MOD_SIZE;
            CamX::OsUtils::FPrintF(pFile, "\n* asf30Data.skin_gain_lut[2][17] = \n");
            for (UINT index = 0; index < DMI_SKIN_SIZE; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT8>((*(pDMIDataPtr + offset + index)) & 0xFF));
            }

            CamX::OsUtils::FPrintF(pFile, "\n* asf30Data.skin_activity_lut[2][17] = \n");
            for (UINT index = 0; index < DMI_WEIGHT_MOD_SIZE; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<UINT8>((*(pDMIDataPtr + offset + index)>>8) & 0xFF));
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
