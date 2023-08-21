////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpslsc40.cpp
/// @brief CAMXBPSLSC40 class implementation
///        Corrects image intensity rolloff from center to corners due to lens optics by applying inverse gain to
///        compensate the lens rolloff effect
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "bps_data.h"
#include "camxbpslsc40.h"
#include "camxbpslsc40titan480.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC40::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC40::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(lsc_4_0_0::lsc40_rgn_dataType) * (LSC40MaxmiumNonLeafNode + 1));

    // Alloc for lsc_4_0_0::lsc40_rgn_dataType
    m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
    if (NULL == m_dependenceData.pInterpolationData)
    {
        result = CamxResultENoMemory;
    }

    m_tintlessRatio.pRGain = NULL;
    if (CamxResultSuccess == result)
    {
        // Allocate a Float array of 17 x 13 to store tintless ratio.
        // This will store the tintless ratio applied.
        // This will be used if tintless algo doesnt run
        m_tintlessRatio.pRGain = static_cast<FLOAT*>(CAMX_CALLOC(sizeof(FLOAT) * MESH_ROLLOFF40_SIZE));
        if (NULL == m_tintlessRatio.pRGain)
        {
            result = CamxResultENoMemory;
        }
    }

    m_tintlessRatio.pGrGain = NULL;
    if (CamxResultSuccess == result)
    {
        m_tintlessRatio.pGrGain = static_cast<FLOAT*>(CAMX_CALLOC(sizeof(FLOAT) * MESH_ROLLOFF40_SIZE));
        if (NULL == m_tintlessRatio.pGrGain)
        {
            result = CamxResultENoMemory;
        }
    }

    m_tintlessRatio.pGbGain = NULL;
    if (CamxResultSuccess == result)
    {
        m_tintlessRatio.pGbGain = static_cast<FLOAT*>(CAMX_CALLOC(sizeof(FLOAT) * MESH_ROLLOFF40_SIZE));
        if (NULL == m_tintlessRatio.pGbGain)
        {
            result = CamxResultENoMemory;
        }
    }

    m_tintlessRatio.pBGain = NULL;
    if (CamxResultSuccess == result)
    {
        m_tintlessRatio.pBGain = static_cast<FLOAT*>(CAMX_CALLOC(sizeof(FLOAT) * MESH_ROLLOFF40_SIZE));
        if (NULL == m_tintlessRatio.pBGain)
        {
            result = CamxResultENoMemory;
        }
    }

    m_pLSCCalibrationData = NULL;
    if (CamxResultSuccess == result)
    {
        // This allocation will happen once per session
        m_pLSCCalibrationData =
            static_cast<LSCCalibrationData*>(CAMX_CALLOC(MaxLightTypes * sizeof(LSCCalibrationData)));
        if (NULL == m_pLSCCalibrationData)
        {
            result = CamxResultENoMemory;
        }
    }

    m_dependenceData.pCalibrationTable = NULL;
    if (CamxResultSuccess == result)
    {
        m_dependenceData.numTables = MaxLightTypes;
        m_dependenceData.pCalibrationTable =
            static_cast<LSC40CalibrationDataTable*>(CAMX_CALLOC(MaxLightTypes * sizeof(LSC40CalibrationDataTable)));
        if (NULL == m_dependenceData.pCalibrationTable)
        {
            result = CamxResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC40::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSLSC40::DeallocateCommonLibraryData()
{
    CAMX_FREE(m_dependenceData.pInterpolationData);
    m_dependenceData.pInterpolationData = NULL;

    CAMX_FREE(m_dependenceData.pCalibrationTable);
    m_dependenceData.pCalibrationTable = NULL;

    CAMX_FREE(m_pLSCCalibrationData);
    m_pLSCCalibrationData = NULL;

    CAMX_FREE(m_tintlessRatio.pRGain);
    m_tintlessRatio.pRGain = NULL;

    CAMX_FREE(m_tintlessRatio.pGrGain);
    m_tintlessRatio.pGrGain = NULL;

    CAMX_FREE(m_tintlessRatio.pGbGain);
    m_tintlessRatio.pGbGain = NULL;

    CAMX_FREE(m_tintlessRatio.pBGain);
    m_tintlessRatio.pBGain = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC40::BPSLSC40
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSLSC40::BPSLSC40(
    const CHAR* pNodeIdentifier)
{
    CamxResult result = CamxResultSuccess;

    m_pNodeIdentifier    = pNodeIdentifier;
    m_type               = ISPIQModuleType::BPSLSC;
    m_moduleEnable       = TRUE;
    m_numLUT             = LSC40NumDMITables;  // need to have an API function to get the LUT table number
    m_cmdLength          = 0;
    m_pGRRLUTDMIBuffer   = NULL;
    m_pGBBLUTDMIBuffer   = NULL;
    m_pGridLUTDMIBuffer  = NULL;
    m_pChromatix         = NULL;
    m_pHWSetting         = NULL;
    m_pTintlessChromatix = NULL;
    m_AWBLock            = ControlAWBLockOff;
    m_AELock             = ControlAELockOff;
    m_pAWBBGStats        = NULL;
    m_pTintlessBGStats   = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC40::~BPSLSC40
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSLSC40::~BPSLSC40()
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

    if (NULL != m_pTintlessAlgo)
    {
        m_pTintlessAlgo->TintlessDestroy(m_pTintlessAlgo);
        m_pTintlessAlgo = NULL;
    }

    if (NULL != m_hHandleTintless)
    {
        CamX::OsUtils::LibUnmap(m_hHandleTintless);
        m_hHandleTintless = NULL;
    }

    if (NULL != m_pAlscAlgo)
    {
        m_pAlscAlgo->ALSCDestroy(m_pAlscAlgo);
        m_pAlscAlgo = NULL;
    }

    if (NULL != m_hHandleAlsc)
    {
        CamX::OsUtils::LibUnmap(m_hHandleAlsc);
        m_hHandleAlsc = NULL;
    }

    m_pChromatix = NULL;
    m_pTintlessChromatix = NULL;

    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }

    DeallocateCommonLibraryData();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC40::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC40::Create(
    BPSModuleCreateData* pCreateData)
{
    CamxResult          result         = CamxResultSuccess;
    CREATETINTLESS      pTintlessFunc  = NULL;
    CREATEALSC          pAlscFunc      = NULL;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        BPSLSC40* pModule = CAMX_NEW BPSLSC40(pCreateData->pNodeIdentifier);
        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess == result)
            {
                // Load tintless library
                pTintlessFunc = reinterpret_cast<CREATETINTLESS>
                    (OsUtils::LoadPreBuiltLibrary(pTintlessLibName, pTintlessFuncName, &pModule->m_hHandleTintless));
                if (NULL != pTintlessFunc)
                {
                    CDKResult ret = (*pTintlessFunc)(&pModule->m_pTintlessAlgo);
                    if (CDKResultSuccess != ret)
                    {
                        result = CamxResultEFailed;
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to create tintless algorithm");
                    }
                }
                else
                {
                    result = CamxResultEInvalidPointer;
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "Error loading tintless library");
                }

                // Load ALSC library
                pAlscFunc = reinterpret_cast<CREATEALSC>
                    (OsUtils::LoadPreBuiltLibrary(pTintlessLibName, pAlscFuncName, &pModule->m_hHandleAlsc));
                if (NULL != pAlscFunc)
                {
                    CDKResult ret = (*pAlscFunc)(&pModule->m_pAlscAlgo);
                    if (CDKResultSuccess != ret)
                    {
                        result = CamxResultEFailed;
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to create ALSC algorithm");
                    }
                }
                else
                {
                    result = CamxResultEInvalidPointer;
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "Error loading ALSC library");
                }

                if (CamxResultSuccess == result)
                {
                    result = pModule->AllocateCommonLibraryData();

                    if (result != CamxResultSuccess)
                    {
                        result = CamxResultENoMemory;
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "Create BPS LSC 40 module with error %d", result);
                    }
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Module initialization failed!!");
            }

            if (CamxResultSuccess != result)
            {
                pModule->Destroy();
                pModule = NULL;
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_ASSERT_ALWAYS_MESSAGE("Memory allocation failed");
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
// BPSLSC40::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC40::Initialize(
    BPSModuleCreateData* pCreateData)
{
    ResourceParams cmdBufferConfig = { 0 };
    CamxResult     result          = CamxResultSuccess;
    ISPInputData*  pInputData      = &pCreateData->initializationData;

    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            result = BPSLSC40Titan480::Create(&m_pHWSetting);
            break;
        default:
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid hardware version %d", pCreateData->titanVersion);
            break;
    }

    if (CamxResultSuccess == result)
    {
        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
        m_cmdLength                 = m_pHWSetting->GetCommandLength();
        m_32bitDMILength            = m_pHWSetting->Get32bitDMILength();
        m_64bitDMILength            = m_pHWSetting->Get64bitDMILength();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to create HW Setting Class, no memory");
    }

    // 32 bit LUT buffer manager is created externally by BPS node
    m_pLUTCmdBufferManager = NULL;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC40::FillCmdBufferManagerParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC40::FillCmdBufferManagerParams(
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
                                  (NULL != m_pNodeIdentifier) ? m_pNodeIdentifier : " ", "BPSLSC40");
                pResourceParams->resourceSize                 = (m_32bitDMILength * sizeof(UINT32));
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
// BPSLSC40::TranslateCalibrationTableToCommonLibrary
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSLSC40::TranslateCalibrationTableToCommonLibrary(
    const ISPInputData* pInputData)
{
    INT32 indexEEPROMAvailable = 0;

    CAMX_ASSERT(NULL != pInputData);

    if (NULL != pInputData->pOTPData)
    {
        for (UINT32 lightIndex = 0; lightIndex < MaxLightTypes; lightIndex++)
        {
            if (TRUE == pInputData->pOTPData->LSCCalibration[lightIndex].isAvailable)
            {
                // This memcpy will happen one first request and subsequent chromatix change
                Utils::Memcpy(&(m_pLSCCalibrationData[indexEEPROMAvailable]),
                    &(pInputData->pOTPData->LSCCalibration[lightIndex]),
                    sizeof(LSCCalibrationData));

                m_dependenceData.pCalibrationTable[indexEEPROMAvailable].pBGain =
                    m_pLSCCalibrationData[indexEEPROMAvailable].bGain;
                m_dependenceData.pCalibrationTable[indexEEPROMAvailable].pRGain =
                    m_pLSCCalibrationData[indexEEPROMAvailable].rGain;
                m_dependenceData.pCalibrationTable[indexEEPROMAvailable].pGBGain =
                    m_pLSCCalibrationData[indexEEPROMAvailable].gbGain;
                m_dependenceData.pCalibrationTable[indexEEPROMAvailable].pGRGain =
                    m_pLSCCalibrationData[indexEEPROMAvailable].grGain;
                m_dependenceData.toCalibrate       = TRUE;
                m_dependenceData.enableCalibration = TRUE;

                indexEEPROMAvailable++;
            }

            m_dependenceData.numberOfEEPROMTable = indexEEPROMAvailable;

            if (0 == indexEEPROMAvailable)
            {
                CAMX_LOG_INFO(CamxLogGroupBPS, "No EEPROM data for LSC");
                m_dependenceData.toCalibrate       = FALSE;
                m_dependenceData.enableCalibration = FALSE;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Calibration data from sensor is not present");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC40::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSLSC40::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL    isChanged = FALSE;
    UINT16  scale     = 1;

    if ((NULL != pInputData) &&
        (NULL != pInputData->pHwContext) &&
        (NULL != pInputData->pHALTagsData))
    {
        ISPHALTagsData* pHALTagsData = pInputData->pHALTagsData;

        m_shadingMode = pHALTagsData->shadingMode;
        m_lensShadingMapMode = pHALTagsData->statisticsLensShadingMapMode;
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMBPSIQSetting*>(pInputData->pOEMIQSetting))->LSCEnable;
            isChanged = (TRUE == m_moduleEnable);
        }
        else
        {
            TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
            CAMX_ASSERT(NULL != pTuningManager);

            if (TRUE == pTuningManager->IsValidChromatix())
            {
                CAMX_ASSERT(NULL != pInputData->pTuningData);

                // Search through the tuning data (tree), only when there
                // are changes to the tuning mode data as an optimization
                if (TRUE == pInputData->tuningModeChanged)
                {
                    m_pChromatix = pTuningManager->GetChromatix()->GetModule_lsc40_bps(
                        reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                        pInputData->pTuningData->noOfSelectionParameter);
                    m_pTintlessChromatix = pTuningManager->GetChromatix()->GetModule_tintless20_sw(
                        reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                        pInputData->pTuningData->noOfSelectionParameter);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invaild chromatix service");
            }

            if ((NULL != m_pTintlessChromatix) && (m_pTintlessChromatix != m_dependenceData.pTintlessChromatix))
            {
                m_dependenceData.pTintlessChromatix = m_pTintlessChromatix;
                m_dependenceData.enableTintless = m_pTintlessChromatix->enable_section.tintless_en;
                if (TRUE != pInputData->registerBETEn)
                {
                    m_dependenceData.enableTintless &=
                        (TRUE == pInputData->pHwContext->GetStaticSettings()->tintlessEnable);

                    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "BPS LSC40 Tintless %s",
                        (TRUE == m_dependenceData.enableTintless) ? "enabled" : "disabled");
                }
            }

            if (NULL != m_pChromatix)
            {
                if (m_pChromatix != m_dependenceData.pChromatix)
                {
                    m_dependenceData.pChromatix = m_pChromatix;
                    m_moduleEnable = m_pChromatix->enable_section.rolloff_enable;
                    isChanged = (TRUE == m_moduleEnable);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to get Chromatix");
            }
        }

        if ((TRUE == m_moduleEnable) &&
            (ShadingModeOff == pHALTagsData->shadingMode))
        {
            m_moduleEnable = FALSE;
            isChanged = FALSE;
        }
        else if (FALSE == m_moduleEnable)
        {
            m_shadingMode = ShadingModeOff;
        }

        /// @todo (CAMX-1164) Dual IFE changes for LSC to set the right sensor width/height.
        if (TRUE == m_moduleEnable)
        {
            TranslateCalibrationTableToCommonLibrary(pInputData);

            m_dependenceData.fullResWidth  = pInputData->sensorData.fullResolutionWidth;
            m_dependenceData.fullResHeight = pInputData->sensorData.fullResolutionHeight;
            scale                          = static_cast<UINT32>(pInputData->sensorData.sensorScalingFactor *
                                                                 pInputData->sensorData.sensorBinningFactor);
            m_dependenceData.offsetX       = static_cast<UINT32>(pInputData->sensorData.sensorOut.offsetX     +
                                                                (pInputData->sensorData.CAMIFCrop.firstPixel  *
                                                                 scale));
            m_dependenceData.offsetY       = static_cast<UINT32>(pInputData->sensorData.sensorOut.offsetY     +
                                                                (pInputData->sensorData.CAMIFCrop.firstLine   *
                                                                 scale));
            m_dependenceData.imageWidth    = (pInputData->sensorData.CAMIFCrop.lastPixel  -
                                              pInputData->sensorData.CAMIFCrop.firstPixel + 1);
            m_dependenceData.imageHeight   = (pInputData->sensorData.CAMIFCrop.lastLine   -
                                              pInputData->sensorData.CAMIFCrop.firstLine  + 1);
            m_dependenceData.scalingFactor = scale;

            if (TRUE == IQInterface::s_interpolationTable.LSC40TriggerUpdate(&(pInputData->triggerData),
                &m_dependenceData))
            {
                isChanged = TRUE;
            }
        }
        CAMX_LOG_VERBOSE(CamxLogGroupBPS, "colorCorrectionMode = %d, controlAWBMode = %d, controlMode = %d",
            pHALTagsData->colorCorrectionMode,
            pHALTagsData->controlAWBMode,
            pHALTagsData->controlMode);

        if (TRUE == isChanged)
        {
            m_tintlessConfig.statsConfig.camifWidth =
                pInputData->sensorData.CAMIFCrop.lastPixel - pInputData->sensorData.CAMIFCrop.firstPixel + 1;
            m_tintlessConfig.statsConfig.camifHeight =
                pInputData->sensorData.CAMIFCrop.lastLine - pInputData->sensorData.CAMIFCrop.firstLine + 1;
            m_tintlessConfig.statsConfig.postBayer = 0;
            m_tintlessConfig.statsConfig.statsBitDepth =
                pInputData->pStripeConfig->statsDataForISP.tintlessBGConfig.tintlessBGConfig.outputBitDepth;
            m_tintlessConfig.statsConfig.statsRegionHeight =
                pInputData->pStripeConfig->statsDataForISP.tintlessBGConfig.tintlessBGConfig.regionHeight;
            m_tintlessConfig.statsConfig.statsRegionWidth =
                pInputData->pStripeConfig->statsDataForISP.tintlessBGConfig.tintlessBGConfig.regionWidth;
            m_tintlessConfig.statsConfig.statsNumberOfHorizontalRegions =
                pInputData->pStripeConfig->statsDataForISP.tintlessBGConfig.tintlessBGConfig.horizontalNum;
            m_tintlessConfig.statsConfig.statsNumberOfVerticalRegions =
                pInputData->pStripeConfig->statsDataForISP.tintlessBGConfig.tintlessBGConfig.verticalNum;

            m_tintlessConfig.tintlessParamConfig.applyTemporalFiltering = 0;

            // Dont Update stats for subsequent frames in case of AEC locked
            if ((ControlAEModeOff != pInputData->pHALTagsData->controlAEMode) &&
                (ControlModeOff != pInputData->pHALTagsData->controlMode) &&
                ((m_AELock != ControlAELockOn) ||
                (m_AELock != pInputData->pHALTagsData->controlAECLock)))
            {
                m_pTintlessBGStats = pInputData->pStripeConfig->statsDataForISP.pParsedTintlessBGStats;
                m_pAWBBGStats = pInputData->pStripeConfig->statsDataForISP.pParsedAWBBGStats;
                m_AELock = pInputData->pHALTagsData->controlAECLock;
            }
            // If stats are null then mark it null, Dont use Previous stats not updated in locked condition
            if (NULL == pInputData->pStripeConfig->statsDataForISP.pParsedTintlessBGStats)
            {
                m_pTintlessBGStats = NULL;
            }
            if (NULL == pInputData->pStripeConfig->statsDataForISP.pParsedAWBBGStats)
            {
                m_pAWBBGStats = NULL;
            }
        }
        CAMX_LOG_VERBOSE(CamxLogGroupBPS, "pAwbbgstats %p m_pAWBBGStats:%p isChanged:%d m_moduleEnable:%d shadingmode:%d"
            "m_AELock:%d controlAECLock:%d aeMOde:%d controlMode:%d",
            pInputData->pStripeConfig->statsDataForISP.pParsedAWBBGStats,
            m_pAWBBGStats, isChanged, m_moduleEnable, pInputData->pHALTagsData->shadingMode,
            m_AELock, pInputData->pHALTagsData->controlAECLock,
            pInputData->pHALTagsData->controlAEMode, pInputData->pHALTagsData->controlMode);

    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Input: pInputData %p", pInputData);
    }

    m_dependenceData.pALSCBuffer        = m_alscHelperBuffer;
    m_dependenceData.ALSCBufferSize     = ALSC_SCRATCH_BUFFER_SIZE_IN_DWORD;

    isChanged = TRUE;

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC40::FetchDMIBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC40::FetchDMIBuffer(
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
        CAMX_ASSERT_ALWAYS_MESSAGE("Failed to fetch DMI Buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC40::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC40::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        result = FetchDMIBuffer(pInputData);

        if (CamxResultSuccess == result)
        {
            LSC40OutputData outputData;

            // Update the LUT DMI buffer with Red LUT data
            outputData.pGRRLUTDMIBuffer  =
                reinterpret_cast<UINT32*>(m_pLUTDMICmdBuffer->BeginCommands(m_32bitDMILength));
            // BET ONLY - InputData is different per module tested
            pInputData->pBetDMIAddr = static_cast<VOID*>(outputData.pGRRLUTDMIBuffer);

            // Update the LUT DMI buffer with Blue LUT data
            outputData.pGBBLUTDMIBuffer  =
                reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(outputData.pGRRLUTDMIBuffer) +
                    sizeof(UINT32) * MESH_ROLLOFF40_SIZE);

            // update the LUT DMI buffer for Grid LUT
            outputData.pGridLUTDMIBuffer =
                reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(outputData.pGBBLUTDMIBuffer) +
                    sizeof(UINT32) * MESH_ROLLOFF40_SIZE);

            outputData.type              = PipelineType::BPS;
            outputData.pUnpackedField    = &m_unpackedData.unpackedData;

            if ((pInputData->pHALTagsData->controlAWBLock == m_AWBLock)    &&
                (pInputData->pHALTagsData->controlAWBLock == ControlAWBLockOn))
            {
                m_dependenceData.enableTintless = FALSE;
            }
            else
            {
                m_AWBLock = pInputData->pHALTagsData->controlAWBLock;
            }

            // set tintless configuration
            if (TRUE == m_dependenceData.enableTintless)
            {
                m_dependenceData.pTintlessAlgo   = m_pTintlessAlgo;
                m_dependenceData.pTintlessConfig = &m_tintlessConfig;
                m_dependenceData.pTintlessStats  = m_pTintlessBGStats;
            }
            else
            {
                m_dependenceData.pTintlessAlgo   = NULL;
                m_dependenceData.pTintlessConfig = NULL;
                m_dependenceData.pTintlessStats  = NULL;
            }

            // Maybe should update the isValid flag after calculation
            m_dependenceData.tintlessConfig.isValid = m_tintlessRatio.isValid;
            m_dependenceData.tintlessConfig.pGrGain = m_tintlessRatio.pGrGain;
            m_dependenceData.tintlessConfig.pGbGain = m_tintlessRatio.pGbGain;
            m_dependenceData.tintlessConfig.pRGain  = m_tintlessRatio.pRGain;
            m_dependenceData.tintlessConfig.pBGain  = m_tintlessRatio.pBGain;

            // set ALSC configuration
            if (TRUE == m_dependenceData.pChromatix->enable_section.alsc_enable)
            {
                m_dependenceData.pAWBBGStats    = m_pAWBBGStats;
                m_dependenceData.pALSCAlgo      = m_pAlscAlgo;
            }
            else
            {
                m_dependenceData.pAWBBGStats    = NULL;
                m_dependenceData.pALSCAlgo      = NULL;
            }

            result = IQInterface::LSC40CalculateSetting(&m_dependenceData,
                                                        pInputData->pOEMIQSetting,
                                                        pInputData,
                                                        &outputData);
            if (CamxResultSuccess == result)
            {
                m_pLUTDMICmdBuffer->CommitCommands();

                if (TRUE == m_dependenceData.toCalibrate)
                {
                    m_dependenceData.toCalibrate = FALSE;
                }

                if (NULL != pInputData->pBPSTuningMetadata)
                {
                    m_pGRRLUTDMIBuffer  = outputData.pGRRLUTDMIBuffer;
                    m_pGBBLUTDMIBuffer  = outputData.pGBBLUTDMIBuffer;
                    m_pGridLUTDMIBuffer = outputData.pGridLUTDMIBuffer;
                }
                if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
                {
                    m_pHWSetting->DumpRegConfig();
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "LSC Calculation Failed %d", result);
            }
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_ASSERT_ALWAYS_MESSAGE("Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC40::UpdateBPSInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSLSC40::UpdateBPSInternalData(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    result = m_pHWSetting->UpdateFirmwareData(pInputData, m_moduleEnable);

    if (CamxResultSuccess == result)
    {
        // post any LSC information in case down stream module requires
        if ((ShadingModeOff == m_shadingMode) &&
            (StatisticsLensShadingMapModeOn == m_lensShadingMapMode))
        {
            // Pass the unity matrix for shading mode is OFF.
            for (UINT16 i = 0; i < (4 * LSC_MESH_PT_V_V40 * LSC_MESH_PT_H_V40); i++)
            {
                pInputData->pCalculatedData->lensShadingInfo.lensShadingMap[i] = 1.0f;
            }
        }
        else if (StatisticsLensShadingMapModeOn == pInputData->pHALTagsData->statisticsLensShadingMapMode)
        {
            IQInterface::CopyLSCMapDataV40(pInputData, &m_unpackedData.unpackedData);
        }

        // Post tuning metadata if setting is enabled
        if (NULL != pInputData->pBPSTuningMetadata)
        {
            result = m_pHWSetting->UpdateTuningMetadata(pInputData);
            if (CamxResultSuccess == result)
            {
                CAMX_STATIC_ASSERT((sizeof(UINT32) * MESH_ROLLOFF40_SIZE) <=
                                sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.LSCMesh.GRRLUT));
                CAMX_STATIC_ASSERT((sizeof(UINT32) * MESH_ROLLOFF40_SIZE) <=
                                sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.LSCMesh.GBBLUT));

                if (NULL != m_pGRRLUTDMIBuffer)
                {
                    Utils::Memcpy(&pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.LSCMesh.GRRLUT[0],
                                m_pGRRLUTDMIBuffer,
                                (sizeof(UINT32) * MESH_ROLLOFF40_SIZE));
                }
                if (NULL != m_pGBBLUTDMIBuffer)
                {
                    Utils::Memcpy(&pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.LSCMesh.GBBLUT[0],
                                m_pGBBLUTDMIBuffer,
                                (sizeof(UINT32) * MESH_ROLLOFF40_SIZE));
                }
                if (NULL != m_pGridLUTDMIBuffer)
                {
                    Utils::Memcpy(&pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.LSCMesh.gridLUT[0],
                                  m_pGridLUTDMIBuffer,
                                  (sizeof(UINT32) * MESH_ROLLOFF40_SIZE));
                }

                if ((TRUE == m_moduleEnable)  && (NULL != pInputData->pipelineBPSData.pDebugDataWriter))
                {
                    result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                        DebugDataTagID::TuningBPSLSC40PackedMesh,
                        DebugDataTagType::TuningLSC40MeshLUT,
                        1,
                        &pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.LSCMesh,
                        sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.LSCMesh));

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
                        result = CamxResultSuccess; // Non-fatal error
                    }
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
// BPSLSC40::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC40::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pInputData)                                                        &&
        (NULL != m_pHWSetting)                                                      &&
        (NULL != pInputData->pStripeConfig))
    {
        /// @todo (CAMX-1164) Dual IFE changes for LSC.
        if (TRUE == CheckDependenceChange(pInputData))
        {
            result = RunCalculation(pInputData);
        }

        if (CamxResultSuccess == result)
        {
            pInputData->p32bitDMIBuffer = m_pLUTDMICmdBuffer;

            if ((CamxResultSuccess == result) && (TRUE == m_moduleEnable))
            {
                result = m_pHWSetting->CreateCmdList(pInputData, NULL);
                m_dependenceData.bankSelect ^= 1;
            }
            if (CamxResultSuccess == result)
            {
                UpdateBPSInternalData(pInputData);
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Operation failed %d", result);
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Input: pInputData %p hwSetting %p", pInputData, m_pHWSetting);
    }

    return result;
}

CAMX_NAMESPACE_END
