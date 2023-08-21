////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipe2dlut10.cpp
/// @brief IPE2DLUT class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcdmdefs.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"
#include "camxiqinterface.h"
#include "camxispiqmodule.h"
#include "camxnode.h"
#include "camxtitan17xcontext.h"
#include "camxipe2dlut10.h"
#include "camxipe2dlut10titan17x.h"
#include "camxipe2dlut10titan480.h"

CAMX_NAMESPACE_BEGIN

static const UINT32 IPE2DLUTLUTBufferSizeInDwords = IPEMax2DLUTLUTNumEntries;
static const UINT32 IPE2DLUTLUTBufferSize         = IPE2DLUTLUTBufferSizeInDwords * sizeof(UINT32);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPE2DLUT10::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPE2DLUT10::Create(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        IPE2DLUT10* pModule = CAMX_NEW IPE2DLUT10(pCreateData->pNodeIdentifier);

        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Module initialization failed !!");
                CAMX_DELETE pModule;
                pModule =  NULL;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Memory allocation failed");
            result = CamxResultENoMemory;
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
// IPE2DLUT10::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPE2DLUT10::Initialize(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    m_pHWSetting = NULL;

    // Check Module Hardware Version and Create HW Setting Object
    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IPE2DLUT10Titan480;
            break;
        default:
            m_pHWSetting = CAMX_NEW IPE2DLUT10Titan17x;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
        UINT size                   = m_pHWSetting->GetRegSize() / RegisterWidthInBytes;
        m_cmdLength                 = PacketBuilder::RequiredWriteRegRangeSizeInDwords(size) * sizeof(UINT32);
        result = AllocateCommonLibraryData();
        if (result != CamxResultSuccess)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to Allocate commonLibrary Data, no memory");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class, no memory");
        result = CamxResultENoMemory;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPE2DLUT10::FillCmdBufferManagerParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPE2DLUT10::FillCmdBufferManagerParams(
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
                                  (NULL != m_pNodeIdentifier) ? m_pNodeIdentifier : " ", "IPE2DLUT10");
                pResourceParams->resourceSize                 = IPE2DLUTLUTBufferSize;
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
                CAMX_LOG_ERROR(CamxLogGroupISP, "Out Of Memory");
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Out Of Memory");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Input Error %p", pParam);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPE2DLUT10::UpdateIPEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPE2DLUT10::UpdateIPEInternalData(
    ISPInputData* pInputData
    ) const
{
    CamxResult      result          = CamxResultSuccess;
    IpeIQSettings*  pIPEIQSettings  = reinterpret_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);

    if (NULL != pIPEIQSettings)
    {
        pIPEIQSettings->lut2dParameters.moduleCfg.EN = m_moduleEnable;

        // Post tuning metadata if setting is enabled
        if (NULL != pInputData->pIPETuningMetadata)
        {
            IPE2DLUT10LUT* pLUT2D = NULL;

            result = m_pHWSetting->UpdateTuningMetadata(pInputData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "Update Tuning Metadata failed");
                result = CamxResultSuccess; // Non-fatal error
            }

            CAMX_STATIC_ASSERT(IPE2DLUTLUTBufferSize ==
                sizeof(pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEDMIData.LUT2D));

            CAMX_STATIC_ASSERT(IPE2DLUTLUTBufferSize ==
                sizeof(pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPEDMIData.LUT2D));

            switch (pInputData->titanVersion)
            {
                case CSLCameraTitanVersion::CSLTitan480:
                    pLUT2D = &pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEDMIData.LUT2D;
                    break;
                case CSLCameraTitanVersion::CSLTitan150:
                case CSLCameraTitanVersion::CSLTitan160:
                case CSLCameraTitanVersion::CSLTitan170:
                case CSLCameraTitanVersion::CSLTitan175:
                    pLUT2D = &pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPEDMIData.LUT2D;
                    break;
                default:
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid hardware version: %d", pInputData->titanVersion);
                    break;
            }

            if (NULL != pLUT2D)
            {
                if (NULL != m_p2DLUTLUTs)
                {
                    Utils::Memcpy(pLUT2D, m_p2DLUTLUTs, IPE2DLUTLUTBufferSize);
                }

                if (TRUE == pIPEIQSettings->lut2dParameters.moduleCfg.EN)
                {
                    DebugDataTagID dataTagID;

                    if (TRUE == pInputData->pipelineIPEData.realtimeFlag)
                    {
                        dataTagID = DebugDataTagID::TuningIPE2DLUT10PackedLUT;
                    }
                    else
                    {
                        dataTagID = DebugDataTagID::TuningIPE2DLUT10PackedLUTOffline;
                    }

                    result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                        dataTagID,
                        DebugDataTagType::Tuning2DLUT10LUT,
                        1,
                        pLUT2D,
                        sizeof(IPE2DLUT10LUT));

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
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Null input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPE2DLUT10::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPE2DLUT10::ValidateDependenceParams(
    const ISPInputData* pInputData
    ) const
{
    CamxResult result = CamxResultSuccess;

    /// @todo (CAMX-730) validate dependency parameters
    if ((NULL == pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferPostLTM]) ||
        (NULL == pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader]) ||
        (NULL == pInputData->pipelineIPEData.pIPEIQSettings))
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc,
            "Invalid Input: pInputData %p ppIPECmdBuffer[CmdBufferDMIHeader] "
            "%p ppIPECmdBuffer[CmdBufferPostLTM] %p pIPEIQSettings %p",
            pInputData,
            pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader],
            pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferPostLTM],
            pInputData->pipelineIPEData.pIPEIQSettings);

        result = CamxResultEInvalidArg;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPE2DLUT10::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPE2DLUT10::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData)                 &&
        (NULL != pInputData->pAECUpdateData) &&
        (NULL != pInputData->pAWBUpdateData) &&
        (NULL != pInputData->pHwContext)     &&
        (NULL != pInputData->pHALTagsData))
    {
        ISPHALTagsData* pHALTagsData = pInputData->pHALTagsData;

        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIPEIQSetting*>(pInputData->pOEMIQSetting))->TDL10Enable;

            isChanged      = TRUE;
        }
        else
        {
            CAMX_ASSERT(NULL != pInputData->pTuningData);
            if (NULL != pInputData->pTuningData)
            {
                TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
                CAMX_ASSERT(NULL != pTuningManager);

                // Search through the tuning data (tree), only when there
                // are changes to the tuning mode data as an optimization
                if ((TRUE == pInputData->tuningModeChanged)    &&
                    (TRUE == pTuningManager->IsValidChromatix()))
                {
                    m_pChromatix = pTuningManager->GetChromatix()->GetModule_tdl10_ipe(
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
                        m_moduleEnable              = m_pChromatix->enable_section.twodlut_enable;
                        if (TRUE == m_moduleEnable)
                        {
                            isChanged = TRUE;
                        }
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to get Chromatix");
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Tuning Pointer is NULL");
            }
        }

        // Check for manual tone map mode, if yes disable 2dLUT
        if ((TRUE                     == m_moduleEnable)                          &&
            (TonemapModeContrastCurve == pHALTagsData->tonemapCurves.tonemapMode))
        {
            m_moduleEnable = FALSE;
            isChanged      = FALSE;
        }

        if (TRUE == m_moduleEnable)
        {
            if (TRUE ==
                IQInterface::s_interpolationTable.IPETDL10TriggerUpdate(&pInputData->triggerData,
                                                                        &m_dependenceData))
            {
                isChanged = TRUE;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Null Input Pointer");
    }

    return isChanged;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPE2DLUT10::FetchDMIBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPE2DLUT10::FetchDMIBuffer()
{
    PacketResource* pPacketResource = NULL;
    CamxResult      result          = CamxResultSuccess;

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
        CAMX_LOG_ERROR(CamxLogGroupISP, "LUT Command Buffer Mangager is NULL");
        result = CamxResultEFailed;
    }

    if (CamxResultSuccess == result)
    {
        m_pLUTDMICmdBuffer = static_cast<CmdBuffer*>(pPacketResource);
        if (m_pLUTDMICmdBuffer == NULL)
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Input pointer");
        }
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
// IPE2DLUT10::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPE2DLUT10::RunCalculation(
    ISPInputData* pInputData)
{
    CAMX_UNREFERENCED_PARAM(pInputData);

    CamxResult       result = CamxResultSuccess;
    TDL10OutputData  outputData;

    result = FetchDMIBuffer();

    if (CamxResultSuccess == result)
    {
        outputData.pD2H0LUT = reinterpret_cast<UINT32*>(m_pLUTDMICmdBuffer->BeginCommands(IPE2DLUTLUTBufferSizeInDwords));

        CAMX_ASSERT(NULL != outputData.pD2H0LUT);

        outputData.pD2H1LUT = outputData.pD2H0LUT + (IPE2DLUTLUTSize[LUT2DIndexHue0]);
        CAMX_ASSERT(NULL != outputData.pD2H1LUT);

        outputData.pD2H2LUT = outputData.pD2H1LUT + (IPE2DLUTLUTSize[LUT2DIndexHue1]);
        CAMX_ASSERT(NULL != outputData.pD2H2LUT);

        outputData.pD2H3LUT = outputData.pD2H2LUT + (IPE2DLUTLUTSize[LUT2DIndexHue2]);
        CAMX_ASSERT(NULL != outputData.pD2H3LUT);

        outputData.pD2S0LUT = outputData.pD2H3LUT + (IPE2DLUTLUTSize[LUT2DIndexHue3]);
        CAMX_ASSERT(NULL != outputData.pD2S0LUT);

        outputData.pD2S1LUT = outputData.pD2S0LUT + (IPE2DLUTLUTSize[LUT2DIndexSaturation0]);
        CAMX_ASSERT(NULL != outputData.pD2S1LUT);

        outputData.pD2S2LUT = outputData.pD2S1LUT + (IPE2DLUTLUTSize[LUT2DIndexSaturation1]);
        CAMX_ASSERT(NULL != outputData.pD2S2LUT);

        outputData.pD2S3LUT = outputData.pD2S2LUT + (IPE2DLUTLUTSize[LUT2DIndexSaturation2]);
        CAMX_ASSERT(NULL != outputData.pD2S3LUT);

        outputData.pD1IHLUT = outputData.pD2S3LUT + (IPE2DLUTLUTSize[LUT2DIndexSaturation3]);
        CAMX_ASSERT(NULL != outputData.pD1IHLUT);

        outputData.pD1ISLUT = outputData.pD1IHLUT + (IPE2DLUTLUTSize[LUT2DIndexInverseHue]);
        CAMX_ASSERT(NULL != outputData.pD1ISLUT);

        outputData.pD1HLUT = outputData.pD1ISLUT + (IPE2DLUTLUTSize[LUT2DIndexInverseSaturation]);
        CAMX_ASSERT(NULL != outputData.pD1HLUT);

        outputData.pD1SLUT = outputData.pD1HLUT + (IPE2DLUTLUTSize[LUT2DIndex1DHue]);
        CAMX_ASSERT(NULL != outputData.pD1SLUT);

        result = IQInterface::TDL10CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);
        if (CamxResultSuccess == result)
        {
            result = m_pLUTDMICmdBuffer->CommitCommands();

            if (NULL != pInputData->pIPETuningMetadata)
            {
                m_p2DLUTLUTs  = outputData.pD2H0LUT;
            }
            if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
            {
                m_pHWSetting->DumpRegConfig();
                DumpRegConfig(outputData.pD2H0LUT);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "TDL10 Calculation Failed %d", result);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Cannot get buffer from CmdBufferManager");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPE2DLUT10::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPE2DLUT10::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(tdl_1_0_0::tdl10_rgn_dataType) * (TDL10MaxNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for tdl_1_0_0::tdl10_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPE2DLUT10::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPE2DLUT10::Execute(
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
                // This offset holds CDM header of LTM LUTs, store this offset and node will patch it in top level payload
                CmdBuffer*  pDMICmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader];
                m_offsetLUT               = pDMICmdBuffer->GetResourceUsedDwords() * sizeof(UINT32);

                TDLHWSettingParams moduleHWSettingParams = { 0 };

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
// IPE2DLUT10::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPE2DLUT10::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPE2DLUT10::IPE2DLUT10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPE2DLUT10::IPE2DLUT10(
    const CHAR* pNodeIdentifier)
    : m_pLUTDMICmdBuffer(NULL)
{
    m_pNodeIdentifier   = pNodeIdentifier;
    m_type              = ISPIQModuleType::IPE2DLUT;
    m_moduleEnable      = TRUE;
    m_numLUT            = LUT2DIndexMax;
    m_p2DLUTLUTs        = NULL;
    m_pChromatix        = NULL;

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE Local Tone Map m_cmdLength %d ", m_cmdLength);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPE2DLUT10::~IPE2DLUT10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPE2DLUT10::~IPE2DLUT10()
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
    DeallocateCommonLibraryData();

    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPE2DLUT10::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPE2DLUT10::DumpRegConfig(
    UINT32* p2DLUTLUTs
    ) const
{
    CHAR  dumpFilename[256];
    FILE* pFile = NULL;
    CamX::OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename), "%s/IPE_regdump.txt", ConfigFileDirectory);
    pFile = CamX::OsUtils::FOpen(dumpFilename, "a+");

    if (NULL != pFile)
    {
        // Register dump is done in HW setting class
        if (0 != m_moduleEnable)
        {
            UINT       index  = 0;
            UINT       index0 = 0;
            UINT       index1 = 0;
            UINT       index2 = 0;
            UINT       index3 = 0;

            UINT32*    pD2H0LUT = p2DLUTLUTs;
            UINT32*    pD2H1LUT = pD2H0LUT + (IPE2DLUTLUTSize[LUT2DIndexHue0]);
            UINT32*    pD2H2LUT = pD2H1LUT + (IPE2DLUTLUTSize[LUT2DIndexHue1]);
            UINT32*    pD2H3LUT = pD2H2LUT + (IPE2DLUTLUTSize[LUT2DIndexHue2]);
            UINT32*    pD2S0LUT = pD2H3LUT + (IPE2DLUTLUTSize[LUT2DIndexHue3]);
            UINT32*    pD2S1LUT = pD2S0LUT + (IPE2DLUTLUTSize[LUT2DIndexSaturation0]);
            UINT32*    pD2S2LUT = pD2S1LUT + (IPE2DLUTLUTSize[LUT2DIndexSaturation1]);
            UINT32*    pD2S3LUT = pD2S2LUT + (IPE2DLUTLUTSize[LUT2DIndexSaturation2]);
            UINT32*    pD1IHLUT = pD2S3LUT + (IPE2DLUTLUTSize[LUT2DIndexSaturation3]);
            UINT32*    pD1ISLUT = pD1IHLUT + (IPE2DLUTLUTSize[LUT2DIndexInverseHue]);
            UINT32*    pD1HLUT  = pD1ISLUT + (IPE2DLUTLUTSize[LUT2DIndexInverseSaturation]);
            UINT32*    pD1SLUT  = pD1HLUT  + (IPE2DLUTLUTSize[LUT2DIndex1DHue]);

            CamX::OsUtils::FPrintF(pFile, "* lut2D10Data.hs_lut.lut_1d_h[2][25] = \n");
            for (index = 0; index < H_GRID; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<INT32>(*(pD1IHLUT + index)));
            }
            CamX::OsUtils::FPrintF(pFile, "* lut2D10Data.hs_lut.lut_1d_s[2][16] = \n");
            for (index = 0; index < S_GRID; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<INT32>(*(pD1SLUT + index)));
            }

            CamX::OsUtils::FPrintF(pFile, "* lut2D10Data.hs_lut.lut_2d_h[2][400] = \n");
            for (UINT hueCount = 0; hueCount < H_GRID; hueCount++)
            {
                for (UINT satCount = 0; satCount < S_GRID; satCount++)
                {
                    if (TRUE == (Utils::IsUINT32Odd(hueCount)))
                    {
                        if (TRUE == (Utils::IsUINT32Odd(satCount)))
                        {
                            CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<INT32>(*(pD2H3LUT + index3)));
                            index3++;
                            index++;
                        }
                        else
                        {
                            CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<INT32>(*(pD2H2LUT + index2)));
                            index2++;
                            index++;
                        }
                    }
                    else
                    {
                        if (TRUE == (Utils::IsUINT32Odd(satCount)))
                        {
                            CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<INT32>(*(pD2H1LUT + index1)));
                            index1++;
                            index++;
                        }
                        else
                        {
                            CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<INT32>(*(pD2H0LUT + index0)));
                            index0++;
                            index++;
                        }
                    }
                }
            }
            index  = 0;
            index0 = 0;
            index1 = 0;
            index2 = 0;
            index3 = 0;
            CamX::OsUtils::FPrintF(pFile, "* lut2D10Data.hs_lut.lut_2d_s[2][400] = \n");
            for (UINT hueCount = 0; hueCount < H_GRID; hueCount++)
            {
                for (UINT satCount = 0; satCount < S_GRID; satCount++)
                {
                    if (TRUE == (Utils::IsUINT32Odd(hueCount)))
                    {
                        if (TRUE == (Utils::IsUINT32Odd(satCount)))
                        {
                            CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<INT32>(*(pD2S3LUT + index3)));
                            index3++;
                            index++;
                        }
                        else
                        {
                            CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<INT32>(*(pD2S2LUT + index2)));
                            index2++;
                            index++;
                        }
                    }
                    else
                    {
                        if (TRUE == (Utils::IsUINT32Odd(satCount)))
                        {
                            CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<INT32>(*(pD2S1LUT + index1)));
                            index1++;
                            index++;
                        }
                        else
                        {
                            CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<INT32>(*(pD2S0LUT + index0)));
                            index0++;
                            index++;
                        }
                    }
                }
            }

            CamX::OsUtils::FPrintF(pFile, "* lut2D10Data.hs_lut.lut_1d_h_inv[2][24] = \n");
            for (index = 0; index < (H_GRID - 1) ; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<INT32>(*(pD1IHLUT + index)));
            }
            CamX::OsUtils::FPrintF(pFile, "* lut2D10Data.hs_lut.lut_1d_s_inv[2][15] = \n");
            for (index = 0; index < (S_GRID - 1); index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", static_cast<INT32>(*(pD1ISLUT + index)));
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
