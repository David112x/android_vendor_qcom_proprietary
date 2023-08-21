////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipegamma15.cpp
/// @brief IPEGamma class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcdmdefs.h"
#include "camxdefs.h"
#include "camxipegamma15.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "ipe_data.h"
#include "parametertuningtypes.h"
#include "camxtitan17xcontext.h"
#include "camxipegamma15titan17x.h"
#include "camxipegamma15titan480.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGamma15::Create(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        IPEGamma15* pModule = CAMX_NEW IPEGamma15(pCreateData->pNodeIdentifier);
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
// IPEGamma15::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGamma15::Initialize(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    m_pHWSetting = NULL;

    // Check Module Hardware Version and Create HW Setting Object
    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IPEGamma15Titan480;
            break;
        default:
            m_pHWSetting = CAMX_NEW IPEGamma15Titan17x;
            break;
    }

    if (NULL != m_pHWSetting)
    {
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

    if (CamxResultSuccess == result)
    {
        // Precompute Offset of Look up table with LUT command buffer for ease of patching

        m_offsetLUTCmdBuffer[GammaLUTChannel0] = 0;
        m_offsetLUTCmdBuffer[GammaLUTChannel1] = Gamma15LUTNumEntriesPerChannelSize;
        m_offsetLUTCmdBuffer[GammaLUTChannel2] =
            Gamma15LUTNumEntriesPerChannelSize + m_offsetLUTCmdBuffer[GammaLUTChannel1];
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15::FillCmdBufferManagerParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGamma15::FillCmdBufferManagerParams(
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
                                  (NULL != m_pNodeIdentifier) ? m_pNodeIdentifier : " ", "IPEGamma15");
                pResourceParams->resourceSize                 = Gamma15LUTBufferSize;
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
// IPEGamma15::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGamma15::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(gamma_1_5_0::mod_gamma15_cct_dataType) * (Gamma15MaxNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for gamma_1_5_0::mod_gamma15_cct_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15::UpdateIPEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGamma15::UpdateIPEInternalData(
    ISPInputData* pInputData
    ) const
{
    CamxResult      result         = CamxResultSuccess;
    IpeIQSettings*  pIPEIQSettings = reinterpret_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);
    ISPHALTagsData* pHALTagsData   = pInputData->pHALTagsData;
    UINT32          toneMapCurvePoints;

    pIPEIQSettings->glutParameters.moduleCfg.EN = m_moduleEnable;

    /// @todo (CAMX-738) Add metadata information

    // Post tuning metadata if setting is enabled
    if (NULL != pInputData->pIPETuningMetadata)
    {
        IPEGamma15Curve* pIPEGamma15LUT = NULL;

        switch (pInputData->titanVersion)
        {
            case CSLCameraTitanVersion::CSLTitan480:
                pIPEGamma15LUT = &pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEDMIData.gamma[0];
                break;
            case CSLCameraTitanVersion::CSLTitan150:
            case CSLCameraTitanVersion::CSLTitan160:
            case CSLCameraTitanVersion::CSLTitan170:
            case CSLCameraTitanVersion::CSLTitan175:
                pIPEGamma15LUT = &pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPEDMIData.gamma[0];
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid hardware version: %d", pInputData->titanVersion);
                break;
        }

        CAMX_STATIC_ASSERT(Gamma15LUTBufferSize ==
            sizeof(pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPEDMIData.gamma));
        CAMX_STATIC_ASSERT(Gamma15LUTBufferSize ==
            sizeof(pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEDMIData.gamma));

        if (NULL != pIPEGamma15LUT)
        {
            if (NULL != m_pGammaG[GammaLUTChannel0])
            {
                Utils::Memcpy(pIPEGamma15LUT[GammaLUTChannel0].curve,
                              m_pGammaG[GammaLUTChannel0],
                              Gamma15LUTNumEntriesPerChannelSize);
            }

            if (NULL != m_pGammaG[GammaLUTChannel1])
            {
                Utils::Memcpy(pIPEGamma15LUT[GammaLUTChannel1].curve,
                              m_pGammaG[GammaLUTChannel1],
                              Gamma15LUTNumEntriesPerChannelSize);
            }

            if (NULL != m_pGammaG[GammaLUTChannel2])
            {
                Utils::Memcpy(pIPEGamma15LUT[GammaLUTChannel2].curve,
                              m_pGammaG[GammaLUTChannel2],
                              Gamma15LUTNumEntriesPerChannelSize);
            }
            if (TRUE == pIPEIQSettings->glutParameters.moduleCfg.EN)
            {
                DebugDataTagID dataTagID;

                if (TRUE == pInputData->pipelineIPEData.realtimeFlag)
                {
                    dataTagID = DebugDataTagID::TuningIPEGamma15PackedLUT;
                }
                else
                {
                    dataTagID = DebugDataTagID::TuningIPEGamma15PackedLUTOffline;
                }

                result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                    dataTagID,
                    DebugDataTagType::TuningIPEGamma15Curve,
                    GammaMaxChannel,
                    pIPEGamma15LUT,
                    Gamma15LUTBufferSize);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
                    result = CamxResultSuccess; // Non-fatal error
                }
            }
        }
    }
    if (NULL != pInputData->pCalculatedData)
    {
        pInputData->pCalculatedData->toneMapData.tonemapMode = m_tonemapMode;

        // Post the tone map curve
        toneMapCurvePoints = Utils::MinUINT32(Gamma15LUTNumEntriesPerChannel,
                                              pInputData->pHALTagsData->tonemapCurves.curvePoints / 2);

        // tone map mode and no of tone map curve points
        pInputData->pCalculatedData->toneMapData.curvePoints = toneMapCurvePoints * 2;

        for (UINT32 idx = 0; idx < toneMapCurvePoints; idx++)
        {
            UINT32 srcIndex =
                Utils::RoundFLOAT(static_cast<FLOAT>(idx * Gamma15LUTNumEntriesPerChannel)) /(toneMapCurvePoints - 1);
            if (0 != srcIndex)
            {
                srcIndex -= 1;
            }

            if ((TonemapModeContrastCurve == m_tonemapMode) &&
                (NULL != m_pGamma[GammaLUTChannelG])        &&
                (NULL != m_pGamma[GammaLUTChannelB])        &&
                (NULL != m_pGamma[GammaLUTChannelR]))
            {
                // Green tone map curve
                pInputData->pCalculatedData->toneMapData.tonemapCurveGreen[idx].point[0] =
                    static_cast<FLOAT>(idx) / (toneMapCurvePoints - 1);
                pInputData->pCalculatedData->toneMapData.tonemapCurveGreen[idx].point[1] =
                    m_pGamma[GammaLUTChannelG][srcIndex] / IFEGamma15MaxGammaValue;

                // Blue tone map curve
                pInputData->pCalculatedData->toneMapData.tonemapCurveBlue[idx].point[0] =
                    static_cast<FLOAT>(idx) / (toneMapCurvePoints - 1);
                pInputData->pCalculatedData->toneMapData.tonemapCurveBlue[idx].point[1] =
                    m_pGamma[GammaLUTChannelB][srcIndex] / IFEGamma15MaxGammaValue;

                // Red tone map curve
                pInputData->pCalculatedData->toneMapData.tonemapCurveRed[idx].point[0] =
                    static_cast<FLOAT>(idx) / (toneMapCurvePoints - 1);
                pInputData->pCalculatedData->toneMapData.tonemapCurveRed[idx].point[1] =
                    m_pGamma[GammaLUTChannelR][srcIndex] / IFEGamma15MaxGammaValue;

                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "idx %d, srcIndex %d, toneMapCurvePoints %d, Reporting Curve [%f, %f],"
                                 "HW LUT data[%f]",
                                 idx,
                                 srcIndex,
                                 toneMapCurvePoints,
                                 pInputData->pCalculatedData->toneMapData.tonemapCurveRed[idx].point[0],
                                 pInputData->pCalculatedData->toneMapData.tonemapCurveRed[idx].point[1],
                                 m_pGamma[GammaLUTChannelR][srcIndex]);
            }
            else if ((NULL != m_pGammaG[GammaLUTChannel0])  &&
                     (NULL != m_pGammaG[GammaLUTChannel1])  &&
                     (NULL != m_pGammaG[GammaLUTChannel2]))
            {
                // Green tone map curve
                pInputData->pCalculatedData->toneMapData.tonemapCurveGreen[idx].point[0] =
                    static_cast<FLOAT>(idx) / (toneMapCurvePoints - 1);
                pInputData->pCalculatedData->toneMapData.tonemapCurveGreen[idx].point[1] =
                    static_cast<FLOAT>(m_pGammaG[GammaLUTChannel0][srcIndex] & IFEGamma15MaxGammaValue) /
                    IFEGamma15MaxGammaValue;

                // Blue tone map curve
                pInputData->pCalculatedData->toneMapData.tonemapCurveBlue[idx].point[0] =
                    static_cast<FLOAT>(idx) / (toneMapCurvePoints - 1);
                pInputData->pCalculatedData->toneMapData.tonemapCurveBlue[idx].point[1] =
                    static_cast<FLOAT>(m_pGammaG[GammaLUTChannel1][srcIndex] & IFEGamma15MaxGammaValue) /
                    IFEGamma15MaxGammaValue;

                // Red tone map curve
                pInputData->pCalculatedData->toneMapData.tonemapCurveRed[idx].point[0] =
                    static_cast<FLOAT>(idx) / (toneMapCurvePoints - 1);
                pInputData->pCalculatedData->toneMapData.tonemapCurveRed[idx].point[1] =
                    static_cast<FLOAT>(m_pGammaG[GammaLUTChannel2][srcIndex] & IFEGamma15MaxGammaValue) /
                    IFEGamma15MaxGammaValue;
            }
        }

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "toneMapCurvePoints %d", toneMapCurvePoints);

        for ( UINT32 idx = 0; idx < toneMapCurvePoints; idx++)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "toneMapCurvePoints %d, Red Reporting Curve [idx = %d] [%f, %f]",
                toneMapCurvePoints,
                idx,
                pInputData->pCalculatedData->toneMapData.tonemapCurveRed[idx].point[0],
                pInputData->pCalculatedData->toneMapData.tonemapCurveRed[idx].point[1]);
        }

        for (UINT32 idx = 0; idx < toneMapCurvePoints; idx++)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "toneMapCurvePoints %d, Blue Reporting Curve [idx = %d] [%f, %f]",
                toneMapCurvePoints,
                idx,
                pInputData->pCalculatedData->toneMapData.tonemapCurveGreen[idx].point[0],
                pInputData->pCalculatedData->toneMapData.tonemapCurveGreen[idx].point[1]);
        }

        for (UINT32 idx = 0; idx < toneMapCurvePoints; idx++)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "toneMapCurvePoints %d, Blue Reporting Curve [idx = %d] [%f, %f]",
                toneMapCurvePoints,
                idx,
                pInputData->pCalculatedData->toneMapData.tonemapCurveBlue[idx].point[0],
                pInputData->pCalculatedData->toneMapData.tonemapCurveBlue[idx].point[1]);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid input data");
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGamma15::ValidateDependenceParams(
    const ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    /// @todo (CAMX-730) validate dependency parameters
    if ((NULL == pInputData)                                                       ||
        (NULL == pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader])   ||
        (NULL == pInputData->pipelineIPEData.pIPEIQSettings))
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc,
            "Invalid Input: pInputData %p",
            pInputData);

        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15::GetRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* IPEGamma15::GetRegCmd()
{
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPEGamma15::CheckDependenceChange(
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
        ISPHALTagsData* pHALTagsData = pInputData->pHALTagsData;

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Lux Index [0x%f] ", pInputData->pAECUpdateData->luxIndex);

        m_tonemapMode = pHALTagsData->tonemapCurves.tonemapMode;

        // Check for manual control
        if ((TonemapModeContrastCurve == pHALTagsData->tonemapCurves.tonemapMode) &&
            (0 != pInputData->pHALTagsData->tonemapCurves.curvePoints))
        {
            m_manualCCMOverride = TRUE;
            m_moduleEnable      = TRUE;
            isChanged           = TRUE;
        }
        else
        {
            m_manualCCMOverride = FALSE;
            if (NULL != pInputData->pOEMIQSetting)
            {
                m_moduleEnable = (static_cast<OEMIPEIQSetting*>(pInputData->pOEMIQSetting))->GammaEnable;

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
                        m_pChromatix = pTuningManager->GetChromatix()->GetModule_gamma15_ipe(
                                           reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                           pInputData->pTuningData->noOfSelectionParameter);
                    }

                    m_dependenceData.pLibData = pInputData->pLibInitialData;

                    CAMX_ASSERT(NULL != m_pChromatix);
                    if (NULL != m_pChromatix)
                    {
                        if ((m_pChromatix != m_dependenceData.pChromatix) ||
                            (m_pChromatix->SymbolTableID != m_dependenceData.pChromatix->SymbolTableID) ||
                            (m_pChromatix->enable_section.gamma_enable != m_moduleEnable))
                        {
                            m_dependenceData.pChromatix = m_pChromatix;
                            m_moduleEnable              = m_pChromatix->enable_section.gamma_enable;

                            isChanged = (TRUE == m_moduleEnable);
                        }

                        if (TRUE == m_moduleEnable)
                        {
                            if (TRUE == IQInterface::s_interpolationTable.gamma15TriggerUpdate(
                                            &pInputData->triggerData, &m_dependenceData))
                            {
                                isChanged = TRUE;
                            }

                            if (m_dependenceData.contrastLevel != pHALTagsData->contrastLevel)
                            {
                                isChanged = TRUE;
                                m_dependenceData.contrastLevel = pHALTagsData->contrastLevel;
                            }

                            //  Update the variables for gamma15 pre-calculation
                            m_isGamma15PreCalculated  =
                                            pInputData->pCalculatedData->IPEGamma15PreCalculationOutput.isPreCalculated;
                            m_pPreCalculationPacked   =
                                            pInputData->pCalculatedData->IPEGamma15PreCalculationOutput.packedLUT;
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15::FetchDMIBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGamma15::FetchDMIBuffer()
{
    CamxResult      result          = CamxResultSuccess;
    PacketResource* pPacketResource = NULL;

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
        CAMX_LOG_ERROR(CamxLogGroupPProc, "LUT Cmd Buffer Manager is NULL");
    }

    if (CamxResultSuccess == result)
    {
        m_pLUTCmdBuffer = static_cast<CmdBuffer*>(pPacketResource);
    }
    else
    {
        m_pLUTCmdBuffer = NULL;
        result          = CamxResultEUnableToLoad;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to fetch DMI Buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15::InterpretGammaCurve
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void IPEGamma15::InterpretGammaCurve(
    const ISPInputData*  pInputData,
    ISPTonemapPoint*     pManualCurve,
    FLOAT*               pDerivedCurve)
{
    UINT32 dstIndex;
    UINT32 index1;
    UINT32 index2;
    FLOAT referenceIndex;
    FLOAT result;

    UINT32 appCurvePoints = pInputData->pHALTagsData->tonemapCurves.curvePoints / 2;

    // Application sends the gamma values from the range of 0.0 to 1.0 , and also the number of curve points
    // can be <= to the max gamma cure points. We need to interpret it into the ISP native format and count

    // We are assuming referenceIndex coordinate of input curve is evenly sampled.
    for (dstIndex = 0; dstIndex < Gamma15LUTNumEntriesPerChannel; dstIndex++)
    {
        referenceIndex = 1.0f * dstIndex * (appCurvePoints - 1) / (Gamma15LUTNumEntriesPerChannel - 1);

        index1 = static_cast<UINT32>(referenceIndex);
        index2 = index1 + 1;

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "GDBG: referenceIndex %f, dstIndex %u, appCurvePoints %u,"
            "Gamma15LUTNumEntriesPerChannel %u, index1 %u, index2 %u",
            referenceIndex, dstIndex, appCurvePoints, appCurvePoints, Gamma15LUTNumEntriesPerChannel, index1, index2);

        if (index1 < (appCurvePoints - 1))
        {
            result = IFEGamma15MaxGammaValue *
                (((referenceIndex - index1) * pManualCurve[index2].point[1]) +
                 ((index2 - referenceIndex) * pManualCurve[index1].point[1]));
        }
        else
        {
            result = IFEGamma15MaxGammaValue * pManualCurve[index1].point[1];
        }

        if (result > IFEGamma15MaxGammaValue)
        {
            pDerivedCurve[dstIndex] = IFEGamma15MaxGammaValue;
        }
        else
        {
            pDerivedCurve[dstIndex] = result;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupPProc,
                         "idx1 = %d, idxx2 = %d, refId = %f, point[index1] = %f,point[index2] = %f, pDerivedCurve[%d] = %f",
                         index1,
                         index2,
                         referenceIndex,
                         pManualCurve[index1].point[1],
                         pManualCurve[index2].point[1],
                         dstIndex,
                         pDerivedCurve[dstIndex]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15::PackDMIData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void IPEGamma15::PackDMIData(
    FLOAT*  pGammaTable,
    UINT32* pLUT)
{
    INT32  LUTDelta = 0;
    UINT16 x        = 0;
    UINT16 y        = 0;
    UINT32 i        = 0;
    UINT32  tmp;

    const UINT16 mask = NMAX10 - 1;

    // Gamma table in Chromatix has 256 entries and the HW LUT has 255. The 255th chromatix entry is used to
    // compute the delta of last HW LUT entry.
    for (i = 0; i < Gamma15LUTNumEntriesPerChannel; i++)
    {

        tmp = static_cast<UINT32>(pGammaTable[i]);
        x   = static_cast<UINT16>(IQSettingUtils::ClampUINT32(tmp, 0, (NMAX10 - 1)));

        if (i < Gamma15LUTNumEntriesPerChannel - 1)
        {
            tmp = static_cast<UINT32>(pGammaTable[i + 1]);
            y = static_cast<UINT16>(IQSettingUtils::ClampUINT32(tmp, 0, (NMAX10 - 1)));
        }
        else
        {
            y =  NMAX10;
        }

        LUTDelta = y - x;
        LUTDelta = Utils::ClampINT32(LUTDelta, MIN_INT10, MAX_INT10);
        LUTDelta = (LUTDelta & mask);  // bpp
        // Each Gamma LUT entry has 10 bit LSB base value and 10 bit MSB delta
        pLUT[i]   = (x) | (LUTDelta << IFEGamma15HwPackBit);
    }


    return;
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15::UpdateManualToneMap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void IPEGamma15::UpdateManualToneMap(
    const ISPInputData* pInputData)
{
    UINT32* pGammaDMIAddr = GetLUTCmdBuffer(Gamma15LUTBufferDWordSize);
    ISPHALTagsData* pHALTagsData = pInputData->pHALTagsData;

    // Interpret the gamma curve from app entries from 0 - 1
    InterpretGammaCurve(pInputData, &pHALTagsData->tonemapCurves.tonemapCurveBlue[0], &m_manualGammaCurve.gammaB[0]);
    InterpretGammaCurve(pInputData, &pHALTagsData->tonemapCurves.tonemapCurveGreen[0], &m_manualGammaCurve.gammaG[0]);
    InterpretGammaCurve(pInputData, &pHALTagsData->tonemapCurves.tonemapCurveRed[0], &m_manualGammaCurve.gammaR[0]);

    for (UINT32 i = 0; i < Gamma15LUTNumEntriesPerChannel; i += 8)
    {

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Red Curve i [%d to %d][%f, %f, %f, %f, %f, %f, %f, %f]",
            i,
            (i + 8),
            m_manualGammaCurve.gammaR[i + 0],
            m_manualGammaCurve.gammaR[i + 1],
            m_manualGammaCurve.gammaR[i + 2],
            m_manualGammaCurve.gammaR[i + 3],
            m_manualGammaCurve.gammaR[i + 4],
            m_manualGammaCurve.gammaR[i + 5],
            m_manualGammaCurve.gammaR[i + 6],
            m_manualGammaCurve.gammaR[i + 7]);
    }

    for (UINT32 i = 0; i < Gamma15LUTNumEntriesPerChannel; i += 8)
    {

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Green Curve i [%d to %d][%f, %f, %f, %f, %f, %f, %f, %f]",
            i,
            (i + 8),
            m_manualGammaCurve.gammaG[i + 0],
            m_manualGammaCurve.gammaG[i + 1],
            m_manualGammaCurve.gammaG[i + 2],
            m_manualGammaCurve.gammaG[i + 3],
            m_manualGammaCurve.gammaG[i + 4],
            m_manualGammaCurve.gammaG[i + 5],
            m_manualGammaCurve.gammaG[i + 6],
            m_manualGammaCurve.gammaG[i + 7]);
    }

    for (UINT32 i = 0; i < Gamma15LUTNumEntriesPerChannel; i += 8)
    {

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Blue Curve i [%d to %d][%f, %f, %f, %f, %f, %f, %f, %f]",
            i,
            (i + 8),
            m_manualGammaCurve.gammaB[i + 0],
            m_manualGammaCurve.gammaB[i + 1],
            m_manualGammaCurve.gammaB[i + 2],
            m_manualGammaCurve.gammaB[i + 3],
            m_manualGammaCurve.gammaB[i + 4],
            m_manualGammaCurve.gammaB[i + 5],
            m_manualGammaCurve.gammaB[i + 6],
            m_manualGammaCurve.gammaB[i + 7]);
    }

    // pGammaDMIAddr = reinterpret_cast<UINT32*>(pGammaDMIAddr);
    if (NULL != pGammaDMIAddr)
    {
        pGammaDMIAddr = reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(pGammaDMIAddr));
        PackDMIData(&m_manualGammaCurve.gammaG[0], pGammaDMIAddr);
        m_pGamma[GammaLUTChannelG] = &m_manualGammaCurve.gammaG[0];

        pGammaDMIAddr = reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(pGammaDMIAddr) + Gamma15LUTNumEntriesPerChannelSize);
        PackDMIData(&m_manualGammaCurve.gammaB[0], pGammaDMIAddr);
        m_pGamma[GammaLUTChannelB] = &m_manualGammaCurve.gammaB[0];

        // pGammaDMIAddr = reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(pGammaDMIAddr) + IFEGamma16LutTableSize);
        pGammaDMIAddr = reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(pGammaDMIAddr) + Gamma15LUTNumEntriesPerChannelSize);
        PackDMIData(&m_manualGammaCurve.gammaR[0], pGammaDMIAddr);
        m_pGamma[GammaLUTChannelR] = &m_manualGammaCurve.gammaR[0];
    }

    // Commit the data
    m_pLUTCmdBuffer->CommitCommands();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGamma15::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result                            = CamxResultSuccess;
    const IPEInstanceProperty* pInstanceProperty = &pInputData->pipelineIPEData.instanceProperty;

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Lux Index [0x%f] m_manualCCMOverride %d",
        pInputData->pAECUpdateData->luxIndex, m_manualCCMOverride);

    result = FetchDMIBuffer();

    if (CamxResultSuccess == result)
    {
        if ((pInputData->parentNodeID == IFE) &&
            (TRUE == pInputData->isHDR10On))
        {
            m_dependenceData.isHDR10Mode = TRUE;
            if (pInstanceProperty->processingType == IPEProcessingType::IPEProcessingPreview)
            {
                m_dependenceData.usecaseMode = CONFIG_PREVIEW;
            }
            else
            {
                m_dependenceData.usecaseMode = CONFIG_VIDEO;
            }
        }

        if (TRUE == m_manualCCMOverride)
        {
            UpdateManualToneMap(pInputData);
        }
        else
        {
            Gamma15OutputData outputData;
            UINT32* pLUT = GetLUTCmdBuffer(Gamma15LUTBufferDWordSize);

            if (NULL != pLUT)
            {
                //  This part is only excuted when we haven't have a pre-calculation output yet
                if (FALSE == m_isGamma15PreCalculated)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "no gamma pre-calculation from meta");

                    outputData.type = PipelineType::IPE;
                    for (UINT count = 0; count < MaxGammaLUTNum; count++)
                    {
                        outputData.pLUT[count] =
                            reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(pLUT) + m_offsetLUTCmdBuffer[count]);
                    }

                    result = IQInterface::IPEGamma15CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);
                }
                else
                {
                    //  Due to we already have the pre-calculation output, we only need to assign the result here
                    //  and no need to do the calculation again.
                    if (NULL != m_pPreCalculationPacked)
                    {
                        Utils::Memcpy(pLUT, m_pPreCalculationPacked, Gamma15LUTBufferDWordSize * sizeof(UINT32));
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "Got the NULL pointer for m_pPreCalculationPacked");
                    }

                    outputData.pLUT[GammaLUTChannel0] = pLUT;
                    outputData.pLUT[GammaLUTChannel1] = pLUT;
                    outputData.pLUT[GammaLUTChannel2] = pLUT;
                }
            }

            if (CamxResultSuccess == result)
            {
                result = m_pLUTCmdBuffer->CommitCommands();
                if ((NULL != pLUT) &&
                    (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex)))
                {
                    m_pHWSetting->DumpRegConfig();
                }

                if (CamxResultSuccess == result)
                {
                    m_pGammaG[GammaLUTChannel0] = outputData.pLUT[GammaLUTChannel0];
                    m_pGammaG[GammaLUTChannel1] = outputData.pLUT[GammaLUTChannel1];
                    m_pGammaG[GammaLUTChannel2] = outputData.pLUT[GammaLUTChannel2];
                }
                else
                {
                    m_pGammaG[GammaLUTChannel0] = NULL;
                    m_pGammaG[GammaLUTChannel1] = NULL;
                    m_pGammaG[GammaLUTChannel2] = NULL;
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Gamma15 Calculation Failed. result %d", result);
                }
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Cannot get buffer from CmdBufferManager");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGamma15::Execute(
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

                GammaHWSettingParams moduleHWSettingParams = { 0 };

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
// IPEGamma15::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEGamma15::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15::IPEGamma15
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEGamma15::IPEGamma15(
    const CHAR* pNodeIdentifier)
    : m_isGamma15PreCalculated(FALSE)
    , m_pPreCalculationPacked(NULL)
{
    m_pNodeIdentifier   = pNodeIdentifier;
    m_type              = ISPIQModuleType::IPEGamma;
    m_moduleEnable      = TRUE;
    m_cmdLength         = 0;
    m_numLUT            = MaxGammaLUTNum;
    m_pLUTCmdBuffer     = NULL;
    m_pChromatix        = NULL;

    m_pGamma[GammaLUTChannelG] = NULL;
    m_pGamma[GammaLUTChannelB] = NULL;
    m_pGamma[GammaLUTChannelR] = NULL;

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE Gamma m_cmdLength %d ", m_cmdLength);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEGamma15::~IPEGamma15
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEGamma15::~IPEGamma15()
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
// IPEGamma15::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEGamma15::DumpRegConfig(
    UINT32* pLUT
    ) const
{
    CHAR  dumpFilename[256];
    FILE* pFile = NULL;
    CamX::OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename), "%s/IPE_regdump.txt", ConfigFileDirectory);
    pFile = CamX::OsUtils::FOpen(dumpFilename, "a+");

    if (NULL != pFile)
    {
        CamX::OsUtils::FPrintF(pFile, "******** IPE Gamma15 ********\n");

        if (TRUE == m_moduleEnable)
        {
            UINT32 offset               = 0;

            CamX::OsUtils::FPrintF(pFile, "* glut15Data.g_lut_in_cfg.gamma_table[2][256] = \n");
            for (UINT index = 0; index < Gamma15LUTNumEntriesPerChannel; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", (*(pLUT + offset + index)));
            }

            CamX::OsUtils::FPrintF(pFile, "\n* glut15Data.b_lut_in_cfg.gamma_table[2][256] = \n");
            offset += Gamma15LUTNumEntriesPerChannel;
            for (UINT index = 0; index < Gamma15LUTNumEntriesPerChannel; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", (*(pLUT + offset + index)));
            }

            CamX::OsUtils::FPrintF(pFile, "\n* glut15Data.r_lut_in_cfg.gamma_table[2][256] = \n");
            offset += Gamma15LUTNumEntriesPerChannel;
            for (UINT index = 0; index < Gamma15LUTNumEntriesPerChannel; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", (*(pLUT + offset + index)));
            }
        }
        else
        {
            CamX::OsUtils::FPrintF(pFile, "NOT ENABLED");
        }

        CamX::OsUtils::FPrintF(pFile, "\n\n");
        CamX::OsUtils::FClose(pFile);
    }
    /// @todo (CAMX-666) More debug logs
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "%s", __FUNCTION__);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15::GetLUTCmdBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32* IPEGamma15::GetLUTCmdBuffer(
    UINT32 numDwordsToReserve)
{
    return reinterpret_cast<UINT32*>(m_pLUTCmdBuffer->BeginCommands(numDwordsToReserve));
}

CAMX_NAMESPACE_END
