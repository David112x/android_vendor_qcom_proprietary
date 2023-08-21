////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcawbioutil.cpp
/// @brief The class that implements input/output for AWB stats processor class.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcawbioutil.h"
#include "camxhal3module.h"
#include "camxhal3metadatautil.h"
#include "camxmem.h"
#include "camxcsljumptable.h"
#include "camxhal3metadatautil.h"
#include "camxstatsdebuginternal.h"
#include "camxtitan17xdefs.h"
#include "camxtuningdatamanager.h"
#include "camxtrace.h"
#include "camximagesensormoduledata.h"
#include "camximagesensordata.h"


CAMX_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::CAWBIOUtil
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAWBIOUtil::CAWBIOUtil()
    : m_algoFlashEstimationState(AWBAlgoFlashEstimationInactive)
{
    CAMX_ENTRYEXIT(CamxLogGroupAWB);

    // Initialize default non-zero output
    m_outputs.illuminantType    = StatsIlluminantInvalid;
    m_outputs.state             = AWBAlgoStateInactive;
    m_outputs.mode              = AWBAlgoModeAuto;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::Initialize(
    const StatsInitializeData* pInitializeData)
{
    CamxResult result = CamxResultSuccess;
    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

    if (NULL == pInitializeData)
    {
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        m_pNode             = pInitializeData->pNode;
        m_pDebugDataPool    = pInitializeData->pDebugDataPool;
        m_pStatsParser      = pInitializeData->pHwContext->GetStatsParser();
        m_pTuningManager    = pInitializeData->pTuningDataManager;
        m_inputs.cameraInfo = pInitializeData->cameraInfo;
        m_pStaticSettings   = pInitializeData->pHwContext->GetStaticSettings();

        if (NULL == m_pTuningManager || NULL == m_pStaticSettings)
        {
            CAMX_LOG_ERROR(CamxLogGroupAWB, "m_pTuningManager: %p or m_pStaticSettings: %p is Null",
                m_pTuningManager, m_pStaticSettings);
            return CamxResultEInvalidPointer;
        }

        m_pNode->GetSensorModeData(&m_pSensorData);
        m_pNode->GetIFEInputResolution(&m_pIFEInput);


        // Get sensor static capability
        SensorModuleStaticCaps       sensorStaticCaps    = { 0 };
        HwContext*                   pHwContext          = pInitializeData->pHwContext;
        const ImageSensorModuleData* pSensorModuleData   =
            pHwContext->GetImageSensorModuleData(pInitializeData->pPipeline->GetCameraId());
        ImageSensorData*             pImageSensorData    = pSensorModuleData->GetSensorDataObject();

        pImageSensorData->GetSensorStaticCapability(&sensorStaticCaps, pInitializeData->pPipeline->GetCameraId());

        m_setInputs.statsSensorInfo.sensorActiveResWidth    = static_cast<UINT32>(sensorStaticCaps.activeArraySize.width);
        m_setInputs.statsSensorInfo.sensorActiveResHeight   =  static_cast<UINT32>(sensorStaticCaps.activeArraySize.height);
        m_setInputs.statsSensorInfo.sensorResWidth          = static_cast<UINT32>(m_pSensorData->resolution.outputWidth);
        m_setInputs.statsSensorInfo.sensorResHeight         = static_cast<UINT32>(m_pSensorData->resolution.outputHeight);

        /* In sum binning use cases, HVX will do the binning, Input to the IFE is different from the sensor.
        Stats modules need to calculate ROI's based on IFE's input*/
        if (NULL!=m_pIFEInput &&
            ((m_pIFEInput->resolution.width < m_setInputs.statsSensorInfo.sensorResWidth) ||
            (m_pIFEInput->resolution.height < m_setInputs.statsSensorInfo.sensorResHeight)))
        {
            m_setInputs.statsSensorInfo.sensorResWidth = m_pIFEInput->resolution.width;
            m_setInputs.statsSensorInfo.sensorResHeight = m_pIFEInput->resolution.height;
        }

        m_pAWBAlgorithm = NULL;
    }

    m_setInputs.dynamicConvergenceSpeed = 0.0f;

    if (CamxResultSuccess == result)
    {
        m_inputs.statsSession.Initialize(pInitializeData);
    }

    if (CamxResultSuccess == result)
    {
        m_pStatsAWBPropertyReadWrite = CAMX_NEW StatsPropertyReadAndWrite(pInitializeData);
    }

    if (CamxResultSuccess == result)
    {
        if (TRUE == pStaticSettings->enableTOFInterface)
        {
            result = SetupTOFLink(pInitializeData->pChiContext);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupAWB, "Failed setting up TOF link");
            }
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupAWB, "TOF interface not enabled");
        }
    }

    InitializeReadProperties();
    InitializeWriteProperties();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::InitializeReadProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::InitializeReadProperties()
{
    m_AWBReadProperties[AWBReadTypePropertyIDParsedAWBBGStatsOutput] =
    {
        PropertyIDParsedAWBBGStatsOutput,
        0,
        NULL
    };
    m_AWBReadProperties[AWBReadTypePropertyIDISPAWBBGConfig]         =
    {
        PropertyIDISPAWBBGConfig,
        m_pNode->GetMaximumPipelineDelay(),
        NULL
    };
    m_AWBReadProperties[AWBReadTypePropertyIDAECFrameInfo]           =
    {
        PropertyIDAECFrameInfo,
        1,
        NULL
    };
    m_AWBReadProperties[AWBReadTypeInputControlAETargetFpsRange]     =
    {
        InputControlAETargetFpsRange,
        0,
        NULL
    };
    m_AWBReadProperties[AWBReadTypePropertyIDUsecaseLensInfo]        =
    {
        PropertyIDUsecaseLensInfo,
        0,
        NULL
    };
    m_AWBReadProperties[AWBReadTypeInputScalerCropRegion]            =
    {
        InputScalerCropRegion,
        0,
        NULL
    };
    m_AWBReadProperties[AWBReadTypeInputControlAWBMode]              =
    {
        InputControlAWBMode,
        0,
        NULL
    };
    m_AWBReadProperties[AWBReadTypeInputControlSceneMode]            =
    {
        InputControlSceneMode,
        0,
        NULL
    };
    m_AWBReadProperties[AWBReadTypeInputControlAWBLock]              =
    {
        InputControlAWBLock,
        0,
        NULL
    };
    m_AWBReadProperties[AWBReadTypeInputControlAWBRegions]           =
    {
        InputControlAWBRegions,
        0,
        NULL
    };
    m_AWBReadProperties[AWBReadTypePropertyIDAECInternal]            =
    {
        PropertyIDAECInternal,
        1,
        NULL
    };
    m_AWBReadProperties[AWBReadTypePropertyIDAECFrameControl]        =
    {
        PropertyIDAECFrameControl,
        1,
        NULL
    };
    m_AWBReadProperties[AWBReadTypeInputControlCaptureIntent]        =
    {
        InputControlCaptureIntent,
        0,
        NULL
    };
    m_AWBReadProperties[AWBReadTypeFlashMode]                        =
    {
        FlashMode,
        1,
        NULL
    };
    m_AWBReadProperties[AWBReadTypePropertyIDAWBFrameControl]        =
    {
        PropertyIDAWBFrameControl,
        1,
        NULL
    };
    m_AWBReadProperties[AWBReadTypePropertyIDAWBFrameInfo]           =
    {
        PropertyIDAWBFrameInfo,
        1,
        NULL
    };
    m_AWBReadProperties[AWBReadTypePropertyIDAWBInternal]            =
    {
        PropertyIDAWBInternal,
        1,
        NULL
    };
    m_AWBReadProperties[AWBReadTypePropertyIDAWBStatsControl]        =
    {
        PropertyIDAWBStatsControl,
        1,
        NULL
    };
    m_AWBReadProperties[AWBReadTypePropertyIDDebugDataAll]           =
    {
        PropertyIDDebugDataAll,
        0,
        NULL
    };

    m_pStatsAWBPropertyReadWrite->SetReadProperties(m_AWBReadProperties);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::InitializeWriteProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::InitializeWriteProperties()
{
    m_AWBWriteProperties[0] =
    {
        PropertyIDCrossAWBStats,
        sizeof(UINT64),
        &m_currProcessingRequestId,
    };
    m_AWBWriteProperties[1] =
    {
        PropertyIDAWBPeerInfo,
        sizeof(VOID*),
        &m_pPeerInfo,
    };
    m_AWBWriteProperties[2] =
    {
        PropertyIDAWBFrameInfo,
        sizeof(AWBFrameInfo),
        &m_frameInfo,
    };
    m_AWBWriteProperties[3] =
    {
        PropertyIDAWBInternal,
        sizeof(AWBOutputInternal),
        &m_outputInternal
    };

    m_AWBWriteProperties[4] =
    {
        ControlAWBLock,
        1,
        &m_AWBHALData.lock
    };
    m_AWBWriteProperties[5] =
    {
        ControlAWBMode,
        1,
        &m_AWBHALData.mode,
    };
    m_AWBWriteProperties[6] =
    {
        ControlAWBState,
        1,
        &m_AWBHALData.state,
    };
    m_AWBWriteProperties[7] =
    {
        PropertyIDAWBFrameControl,
        sizeof(AWBFrameControl),
        &m_frameControl,
    };
    m_AWBWriteProperties[8] =
    {
        PropertyIDAWBStatsControl,
        sizeof(AWBStatsControl),
        &m_statsControl,
    };

    m_pStatsAWBPropertyReadWrite->SetWriteProperties(m_AWBWriteProperties);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::PopulateTOFData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::PopulateTOFData(
    DataTOF* pData)
{
    CamxResult result = CamxResultEFailed;

    if (m_pTOFInterfaceObject != NULL)
    {
        // We need only one sample per frame for TOF sensor.
        result = m_pTOFInterfaceObject->GetLastNSamples(pData, 1);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupAF, "TOF samples not available");
            Utils::Memset(pData, 0, sizeof(DataTOF));
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::SetupTOFLink
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::SetupTOFLink(
    ChiContext* pChiContext)
{
    CamxResult result = CamxResultSuccess;

    if (NULL == m_pTOFInterfaceObject)
    {
        m_pTOFInterfaceObject = TOFSensorIntf::GetInstance(pChiContext->GetThreadManager(),
            HwEnvironment::GetInstance()->GetStaticSettings()->customTOFSensorLib);
        if (NULL != m_pTOFInterfaceObject)
        {
            TOFSensorConfig sensorConfig = { 0 };

            const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

            CAMX_LOG_VERBOSE(CamxLogGroupAWB, "Setting up an TOF interface link SR = %f, RR = %d",
                pStaticSettings->TOFSensorSamplingRate,
                pStaticSettings->TOFDataReportRate);

            sensorConfig.samplingRate = pStaticSettings->TOFSensorSamplingRate;
            sensorConfig.reportRate   = pStaticSettings->TOFDataReportRate;
            result                    = m_pTOFInterfaceObject->ConfigureTOFSensor(&sensorConfig);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupAWB, "Unable to configure TOF sensor !!");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupAWB, "Failed to create TOF interface m_pTOFInterfaceObject %p result %d",
                m_pTOFInterfaceObject, result);
            result = CamxResultEFailed;
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::SetAWBBayerGrid
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::SetAWBBayerGrid(
    ParsedAWBBGStatsOutput* pBayerGridOutput,
    ISPAWBBGStatsConfig*    pStatsConfig,
    StatsBayerGrid*         pBayerGrid,
    StatsRectangle*         pBayerGridROI)
{
    BGBEConfig* pBGConfig = &pStatsConfig->AWBBGConfig;

    pBayerGrid->horizontalRegionCount       = pBGConfig->horizontalNum;
    pBayerGrid->verticalRegionCount         = pBGConfig->verticalNum;
    pBayerGrid->totalRegionCount            = pBGConfig->horizontalNum * pBGConfig->verticalNum;
    pBayerGrid->regionWidth                 = pStatsConfig->regionWidth;
    pBayerGrid->regionHeight                = pStatsConfig->regionHeight;
    pBayerGrid->bitDepth                    = static_cast<UINT16>(pBGConfig->outputBitDepth);


    pBayerGrid->flags.hasSatInfo            = pBayerGridOutput->flags.hasSatInfo;
    pBayerGrid->flags.usesY                 = pBayerGridOutput->flags.usesY;
    pBayerGrid->numValidRegions             = pBayerGridOutput->numROIs;

    pBayerGridROI->left                     = pBGConfig->ROI.left;
    pBayerGridROI->top                      = pBGConfig->ROI.top;
    pBayerGridROI->width                    = pBGConfig->ROI.width;
    pBayerGridROI->height                   = pBGConfig->ROI.height;

    pBayerGrid->SetChannelDataArray(reinterpret_cast<StatsBayerGridChannelInfo*>(pBayerGridOutput->GetChannelDataArray()));


    CAMX_TRACE_MESSAGE_F(CamxLogGroupAWB, "SetAWBBayerGrid: numvalid %d, totalreg %d, h %d, v %d", pBayerGrid->numValidRegions,
        pBayerGrid->totalRegionCount, pBayerGrid->horizontalRegionCount, pBayerGrid->verticalRegionCount);

    CAMX_LOG_VERBOSE(CamxLogGroupAWB, "SetAWBBayerGrid: numvalid %d, totalreg %d, h %d, v %d ROI:(%d %d %d %d)",
                     pBayerGrid->numValidRegions,
                     pBayerGrid->totalRegionCount,
                     pBayerGrid->horizontalRegionCount,
                     pBayerGrid->verticalRegionCount,
                     pBayerGridROI->left,
                     pBayerGridROI->top,
                     pBayerGridROI->width,
                     pBayerGridROI->height);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::GetRequestNumber
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::GetRequestNumber(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AWBAlgoInputList*              pInputList)
{
    m_inputs.requestNumber                                              = pStatsProcessRequestDataInfo->requestId;
    pInputList->pAWBInputs[AWBInputTypeRequestNumber].inputType         = AWBInputTypeRequestNumber;
    pInputList->pAWBInputs[AWBInputTypeRequestNumber].pAWBInput         = &m_inputs.requestNumber;
    pInputList->pAWBInputs[AWBInputTypeRequestNumber].sizeOfAWBInput    = sizeof(UINT64);


}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::GetStatistics
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::GetStatistics(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AWBAlgoInputList*              pInputList,
    BOOL                           processStatus)
{
    CAMX_UNREFERENCED_PARAM(pStatsProcessRequestDataInfo);
    CamxResult result = CamxResultSuccess;

    if (FALSE == processStatus)
    {
        m_inputs.statsBayerGrid.startupMode                        = StatisticsStartUpInvalid;
        pInputList->pAWBInputs[AWBInputTypeBGStats].inputType      = AWBInputTypeBGStats;
        pInputList->pAWBInputs[AWBInputTypeBGStats].pAWBInput      = &m_inputs.statsBayerGrid;
        pInputList->pAWBInputs[AWBInputTypeBGStats].sizeOfAWBInput = sizeof(StatsBayerGrid);
    }
    else
    {
        static const UINT StatsData[] =
        {
            PropertyIDParsedAWBBGStatsOutput,
            PropertyIDISPAWBBGConfig,
        };

        static const UINT StatsLength = CAMX_ARRAY_SIZE(StatsData);
        VOID*             pData[StatsLength] = { 0 };
        pData[0] = m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypePropertyIDParsedAWBBGStatsOutput);
        pData[1] = m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypePropertyIDISPAWBBGConfig);

        if (NULL != pData[0])
        {
            ParsedAWBBGStatsOutput* pBGStats =
                reinterpret_cast<ParsedAWBBGStatsOutput*>(*static_cast<VOID**>(pData[0]));
            if (NULL != pBGStats && NULL != pData[1])
            {
                // Save the BG stats inside core's input structure
                SetAWBBayerGrid(
                    reinterpret_cast<ParsedAWBBGStatsOutput*>
                    (*static_cast<VOID**>(pData[0])),
                    reinterpret_cast<ISPAWBBGStatsConfig*>(pData[1]),
                    &m_inputs.statsBayerGrid,
                    &m_inputs.statsBayerGridROI);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupAWB, "BG data array NULL");
                result = CamxResultEFailed;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupAWB,
                "PropertyID %08x (AWB BG stats) has not been published! ", PropertyIDParsedAWBBGStatsOutput);
        }

        if (CamxResultSuccess == result)
        {

            m_inputs.statsBayerGrid.startupMode = StatisticsStartUpValid;
            pInputList->pAWBInputs[AWBInputTypeBGStats].inputType = AWBInputTypeBGStats;
            pInputList->pAWBInputs[AWBInputTypeBGStats].pAWBInput = &m_inputs.statsBayerGrid;
            pInputList->pAWBInputs[AWBInputTypeBGStats].sizeOfAWBInput = sizeof(StatsBayerGrid);

            pInputList->pAWBInputs[AWBInputTypeBGStatsROI].inputType = AWBInputTypeBGStatsROI;
            pInputList->pAWBInputs[AWBInputTypeBGStatsROI].pAWBInput = &m_inputs.statsBayerGridROI;
            pInputList->pAWBInputs[AWBInputTypeBGStatsROI].sizeOfAWBInput = sizeof(StatsRectangle);

        }
    }

    // Indicating parsed stats are related to online BPSAWBBG stats
    m_isOffline = FALSE;
    pInputList->pAWBInputs[AWBInputTypeIsOffline].inputType = AWBInputTypeIsOffline;
    pInputList->pAWBInputs[AWBInputTypeIsOffline].pAWBInput = &m_isOffline;
    pInputList->pAWBInputs[AWBInputTypeIsOffline].sizeOfAWBInput = sizeof(BOOL);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::GetDefaultSensorResolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::GetDefaultSensorResolution(
    StatsSensorInfo** ppSensorInfo)
{
    CamxResult    result = CamxResultSuccess;
    *ppSensorInfo        = &m_setInputs.statsSensorInfo;

    CAMX_LOG_INFO(CamxLogGroupAWB, "Default Sensor Resolution:W:%u H:%u",
        m_setInputs.statsSensorInfo.sensorResWidth,
        m_setInputs.statsSensorInfo.sensorResHeight);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::LoadSetInputParamList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::LoadSetInputParamList(
    AWBAlgoSetParamList* pInputList,
    AWBAlgoSetParamType  setParamType,
    const VOID*          pAWBSetParam,
    UINT32               sizeOfInputParam)
{
    UINT32 index = pInputList->inputParamsCount;

    pInputList->pAWBSetParams[index].setParamType     = setParamType;
    pInputList->pAWBSetParams[index].pAWBSetParam     = pAWBSetParam;
    pInputList->pAWBSetParams[index].sizeOfInputParam = sizeOfInputParam;
    pInputList->inputParamsCount++;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::PublishCrossProperty
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::PublishCrossProperty(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo)
{
    CamxResult result = CamxResultSuccess;

    // Publish AWB cross pipeline property for dual camera usecase.
    m_currProcessingRequestId = pStatsProcessRequestDataInfo->requestId;
    CAMX_LOG_VERBOSE(CamxLogGroupAWB, "Published PropertyIDCrossAWBStats:%X for Req:%llu result:%s",
                     PropertyIDCrossAWBStats, m_currProcessingRequestId, Utils::CamxResultToString(result));

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::PublishPeerInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::PublishPeerInfo(
    const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
    VOID*                           pPeerInfo)
{
    CamxResult  result      = CamxResultSuccess;
    UINT64      requestId   = pStatsProcessRequestDataInfo->requestId;
    m_pPeerInfo             = pPeerInfo;

    CAMX_LOG_VERBOSE(CamxLogGroupAWB, "Published PropertyIDAWBPeerInfo:%X with peer info:%p for Req:%llu result:%s",
                     PropertyIDAWBPeerInfo, m_pPeerInfo, requestId, Utils::CamxResultToString(result));

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::GetAECData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::GetAECData(
    AWBAlgoInputList*              pInputList)
{
    CamxResult          result                  = CamxResultSuccess;
    static const UINT StatsData[] =
    {
        PropertyIDAECFrameInfo,
    };

    static const UINT StatsLength = CAMX_ARRAY_SIZE(StatsData);
    VOID*             pData[StatsLength] = { 0 };
    pData[0] = m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypePropertyIDAECFrameInfo);

    if (NULL == pData[0])
    {
        CAMX_LOG_WARN(CamxLogGroupAWB, "PropertyIDAECFrameInfo is not published!");
        result = CamxResultEFailed;
    }
    else
    {
        Utils::Memset(&m_inputs.exposureInfo, 0, sizeof(m_inputs.exposureInfo));

        m_inputs.exposureInfo.luxIndex      =
            reinterpret_cast<AECFrameInformation*>(pData[0])->luxIndex;
        m_inputs.exposureInfo.AECSettled    =
            reinterpret_cast<AECFrameInformation*>(pData[0])->AECSettled;
        m_inputs.exposureInfo.frameDuration =
            reinterpret_cast<AECFrameInformation*>(pData[0])->frameDuration;

        pInputList->pAWBInputs[AWBInputTypeAECData].inputType = AWBInputTypeAECData;
        pInputList->pAWBInputs[AWBInputTypeAECData].pAWBInput = &m_inputs.exposureInfo;
        pInputList->pAWBInputs[AWBInputTypeAECData].sizeOfAWBInput = sizeof(m_inputs.exposureInfo);
        pInputList->inputCount++;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::GetVendorTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::GetVendorTags(
    AWBAlgoInputList*              pInputList)
{
    CamxResult result = CamxResultSuccess;

    if (NULL == pInputList)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "pInputList is Null");
        return CamxResultEInvalidPointer;
    }

    m_inputs.vendorTagInputList.vendorTagCount = 0;
    for (UINT32 i = 0; i < m_vendorTagInputList.vendorTagCount; i++)
    {
        UINT   pVendorTag[1]                    = { m_vendorTagInputList.vendorTag[i].vendorTagId };
        VOID*  pData[1]                         = { 0 };
        UINT64 pVendorTagOffset[1]              = { 0 };

        m_pNode->GetDataList(pVendorTag, pData, pVendorTagOffset, 1);

        m_inputs.vendorTagInputList.vendorTag[i].pData              = pData[0];
        m_inputs.vendorTagInputList.vendorTag[i].vendorTagId        = pVendorTag[0];
        m_inputs.vendorTagInputList.vendorTag[i].dataSize           =
            static_cast<UINT32>(HAL3MetadataUtil::GetMaxSizeByTag(pVendorTag[0]));
        m_inputs.vendorTagInputList.vendorTag[i].appliedDelay       = 0;
        m_inputs.vendorTagInputList.vendorTag[i].sizeOfWrittenData  = 0;

        if ((m_inputs.vendorTagInputList.vendorTag[i].pData != NULL) && (m_inputs.vendorTagInputList.vendorTag[i].dataSize > 0))
        {
            m_inputs.vendorTagInputList.vendorTagCount++;
            CAMX_LOG_VERBOSE(CamxLogGroupAWB,
                "SUCCESS to retrieve input vendor tag data. Id: %u pData:%p dataSize:%u",
                m_inputs.vendorTagInputList.vendorTag[i].vendorTagId,
                m_inputs.vendorTagInputList.vendorTag[i].pData,
                m_inputs.vendorTagInputList.vendorTag[i].dataSize);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupAWB,
                           "Failed to retrieve input vendor tag data. Id: %u pData:%p dataSize:%u",
                           m_inputs.vendorTagInputList.vendorTag[i].vendorTagId,
                           m_inputs.vendorTagInputList.vendorTag[i].pData,
                           m_inputs.vendorTagInputList.vendorTag[i].dataSize);
            result = CamxResultEFailed;
            break;
        }
    }

    if (CamxResultSuccess == result)
    {
        pInputList->pAWBInputs[AWBInputTypeVendorTag].inputType = AWBInputTypeVendorTag;
        pInputList->pAWBInputs[AWBInputTypeVendorTag].pAWBInput = &m_inputs.vendorTagInputList;
        pInputList->pAWBInputs[AWBInputTypeVendorTag].sizeOfAWBInput = sizeof(m_inputs.vendorTagInputList);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::GetDebugDataBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::GetDebugDataBuffer(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AWBAlgoInputList*              pInputList)
{
    UINT64      requestId   = pStatsProcessRequestDataInfo->requestId;
    DebugData*  pDebugData  = NULL;
    CamxResult  result      = CamxResultSuccess;

    // Only use debug data on the master camera
    if ((StatsAlgoRoleDefault   == pStatsProcessRequestDataInfo->cameraInfo.algoRole)   ||
        (StatsAlgoRoleMaster    == pStatsProcessRequestDataInfo->cameraInfo.algoRole))
    {
        result = StatsUtil::GetDebugDataBuffer(m_pDebugDataPool, requestId, PropertyIDDebugDataAWB, &pDebugData);
    }

    if (CamxResultSuccess == result)
    {
        if (NULL != pDebugData && NULL != pDebugData->pData && 0 != pDebugData->size)
        {
            m_inputs.debugData.dataSize = static_cast<UINT32>(pDebugData->size);
            m_inputs.debugData.pData    = pDebugData->pData;

            pInputList->pAWBInputs[AWBInputTypeDebugData].inputType      = AWBInputTypeDebugData;
            pInputList->pAWBInputs[AWBInputTypeDebugData].pAWBInput      = &m_inputs.debugData;
            pInputList->pAWBInputs[AWBInputTypeDebugData].sizeOfAWBInput = sizeof(StatsDataPointer);
        }
        else
        {
            m_inputs.debugData.dataSize = 0;
            m_inputs.debugData.pData    = NULL;

            pInputList->pAWBInputs[AWBInputTypeDebugData].inputType      = AWBInputTypeDebugData;
            pInputList->pAWBInputs[AWBInputTypeDebugData].pAWBInput      = &m_inputs.debugData;
            pInputList->pAWBInputs[AWBInputTypeDebugData].sizeOfAWBInput = sizeof(StatsDataPointer);
        }
    }

    // Treat debug data failures as passive error - always return success
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::GetChiStatsSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::GetChiStatsSession(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AWBAlgoInputList*              pInputList)
{
    m_inputs.statsSession.SetStatsProcessorRequestData(pStatsProcessRequestDataInfo);

    pInputList->pAWBInputs[AWBInputTypeStatsChiHandle].inputType      = AWBInputTypeStatsChiHandle;
    pInputList->pAWBInputs[AWBInputTypeStatsChiHandle].pAWBInput      = &m_inputs.statsSession;
    pInputList->pAWBInputs[AWBInputTypeStatsChiHandle].sizeOfAWBInput = sizeof(ChiStatsSession);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::SetCameraInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::SetCameraInformation(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AWBAlgoInputList*              pInputList,
    BOOL                           processStats)
{
    if (TRUE== processStats)
    {
        PopulateCameraInformation(pStatsProcessRequestDataInfo);
    }
    pInputList->pAWBInputs[AWBInputTypeCameraInfo].inputType      = AWBInputTypeCameraInfo;
    pInputList->pAWBInputs[AWBInputTypeCameraInfo].pAWBInput      = &m_inputs.cameraInfo;
    pInputList->pAWBInputs[AWBInputTypeCameraInfo].sizeOfAWBInput = sizeof(StatsCameraInfo);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::PopulateCameraInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::PopulateCameraInformation(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo)
{
    m_inputs.cameraInfo.algoRole = pStatsProcessRequestDataInfo->cameraInfo.algoRole;
    m_inputs.cameraInfo.cameraId = pStatsProcessRequestDataInfo->cameraInfo.cameraId;

    CAMX_LOG_VERBOSE(CamxLogGroupAWB, "camera info: algo role: %d, cam id: %d",
        m_inputs.cameraInfo.algoRole,
        m_inputs.cameraInfo.cameraId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::RetrieveSensorInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::RetrieveSensorInfo(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AWBAlgoSetParamList*           pSetInputList)
{
    CAMX_UNREFERENCED_PARAM(pStatsProcessRequestDataInfo);

    CamxResult          result                  = CamxResultSuccess;
    StatsSensorInfo*    pSensorInfo             = &(m_setInputs.statsSensorInfo);

    UINT32 metaTagDisableFPSLimits            = 0;
    UINT32 metaTagOverrideSensorFrameDuration = 0;
    VendorTagManager::QueryVendorTagLocation("org.quic.camera.manualExposure",
                                             "disableFPSLimits",
                                             &metaTagDisableFPSLimits);
    VendorTagManager::QueryVendorTagLocation("org.quic.camera.manualExposure",
                                             "overrideSensorFrameDuration",
                                             &metaTagOverrideSensorFrameDuration);

    UINT metaTagFpsOverrideData[] =
    {
        metaTagDisableFPSLimits            | InputMetadataSectionMask,  // 0
        metaTagOverrideSensorFrameDuration | InputMetadataSectionMask   // 1
    };
    const UINT32        pInputDataSize                   = CAMX_ARRAY_SIZE(metaTagFpsOverrideData);
    VOID*               pInputData[pInputDataSize]       = { 0 };
    UINT64              pInputDataOffset[pInputDataSize] = { 0 };

    m_pNode->GetDataList(metaTagFpsOverrideData, pInputData, pInputDataOffset, pInputDataSize);
    BOOL   disableFPSLimits        = (NULL != pInputData[0] ? *(static_cast<BYTE*>(pInputData[0]))   : FALSE);
    UINT64 overrideFrameDurationNs = (NULL != pInputData[1] ? *(static_cast<UINT64*>(pInputData[1])) : 0);

    static const UINT StatsData[] =
    {
        InputControlAETargetFpsRange,
        PropertyIDUsecaseLensInfo,
    };

    static const UINT StatsLength = CAMX_ARRAY_SIZE(StatsData);
    VOID*             pData[StatsLength] = { 0 };
    pData[0] = m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypeInputControlAETargetFpsRange);
    pData[1] = m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypePropertyIDUsecaseLensInfo);

    if ((NULL == pData[0]) || (NULL == pData[1]))
    {
        CAMX_LOG_WARN(CamxLogGroupAWB,
                       "ControlAETargetFpsRange: %p or PropertyIDUsecaseLensInfo: % is not published!", pData[0], pData[1]);
        result = CamxResultEFailed;
    }
    else
    {
        // if frame-duration-override enabled, override FPS-range
        if ((TRUE == disableFPSLimits) && (0 != overrideFrameDurationNs))
        {
            RangeINT32* pFpsRange = reinterpret_cast<RangeINT32*>(pData[0]);
            pFpsRange->min = 1 + static_cast<INT32>(NanoSecondsPerSecond / overrideFrameDurationNs);
            pFpsRange->max = pFpsRange->min;
        }

        FLOAT maxVal                      = static_cast<FLOAT>(reinterpret_cast<RangeINT32*>(pData[0])->max);
        pSensorInfo->currentFPS           = Utils::FloatToQNumber(maxVal, 256);
        pSensorInfo->fNumber              = reinterpret_cast<LensInfo*>(pData[1])->fNumber;
        pSensorInfo->maxFPS               = Utils::FloatToQNumber(static_cast<FLOAT>(m_pSensorData->maxFPS), 256);
        pSensorInfo->currentLinesPerFrame = static_cast<UINT32>(m_pSensorData->numLinesPerFrame);
        pSensorInfo->currentMaxLineCount  = static_cast<UINT32>(m_pSensorData->maxLineCount);
        pSensorInfo->maxGain              = static_cast<FLOAT>(m_pSensorData->maxGain);
        pSensorInfo->pixelClock           = static_cast<UINT32>(m_pSensorData->outPixelClock);
        pSensorInfo->pixelClockPerLine    = static_cast<UINT32>(m_pSensorData->numPixelsPerLine);
        pSensorInfo->sensorResWidth       = static_cast<UINT32>(m_pSensorData->resolution.outputWidth);
        pSensorInfo->sensorResHeight      = static_cast<UINT32>(m_pSensorData->resolution.outputHeight);

        /* In sum binning use cases, HVX will do the binning, Input to the IFE is different from the sensor.
        Stats modules need to calculate ROI's based on IFE's input*/
        if (NULL!=m_pIFEInput &&
            ((m_pIFEInput->resolution.width < pSensorInfo->sensorResWidth) ||
            (m_pIFEInput->resolution.height < pSensorInfo->sensorResHeight)))
        {
            pSensorInfo->sensorResWidth  = m_pIFEInput->resolution.width;
            pSensorInfo->sensorResHeight = m_pIFEInput->resolution.height;
        }
        pSensorInfo->pixelSumFactor       = static_cast<UINT16>(m_pSensorData->binningTypeH);

        LoadSetInputParamList(pSetInputList,
                              AWBSetParamTypeSensorInfo,
                              static_cast<const VOID*>(&m_setInputs.statsSensorInfo),
                              sizeof(StatsSensorInfo));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::RetrieveStatsWindowInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::RetrieveStatsWindowInfo(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AWBAlgoSetParamList*           pSetInputList)
{
    CAMX_UNREFERENCED_PARAM(pStatsProcessRequestDataInfo);

    CropWindow          HALCropWindow           = { 0 };
    CamxResult          result                  = CamxResultSuccess;
    FLOAT               widthRatio              = static_cast<FLOAT>(m_setInputs.statsSensorInfo.sensorResWidth) /
                                                                      (m_setInputs.statsSensorInfo.sensorActiveResWidth);
    FLOAT               heightRatio             = static_cast<FLOAT>(m_setInputs.statsSensorInfo.sensorResHeight) /
                                                                      (m_setInputs.statsSensorInfo.sensorActiveResHeight);

    static const UINT StatsData[] =
    {
        InputScalerCropRegion,
    };

    static const UINT StatsLength = CAMX_ARRAY_SIZE(StatsData);
    VOID*             pData[StatsLength] = { 0 };
    pData[0] = m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypeInputScalerCropRegion);

    if (NULL != pData[0])
    {
        HALCropWindow    = (*reinterpret_cast<CropWindow*>(pData[0]));
    }

    if (HALCropWindow.left + HALCropWindow.width > static_cast<INT32>(m_setInputs.statsSensorInfo.sensorActiveResWidth))
    {
        CAMX_LOG_WARN(CamxLogGroupAWB, "Wrong input: HALCropWindow left(%d) + width(%d) > Active Array width(%d)",
            HALCropWindow.left, HALCropWindow.width, m_setInputs.statsSensorInfo.sensorActiveResWidth);
        HALCropWindow.width = m_setInputs.statsSensorInfo.sensorActiveResWidth - HALCropWindow.left;
    }
    if (HALCropWindow.top + HALCropWindow.height > static_cast<INT32>(m_setInputs.statsSensorInfo.sensorActiveResHeight))
    {
        CAMX_LOG_WARN(CamxLogGroupAWB, "Wrong input: HALCropWindow top(%d) + height(%d) > Active Array height(%d)",
            HALCropWindow.top, HALCropWindow.height, m_setInputs.statsSensorInfo.sensorActiveResHeight);
        HALCropWindow.height = m_setInputs.statsSensorInfo.sensorActiveResHeight - HALCropWindow.top;
    }

    if (HALCropWindow.width != 0 &&
        HALCropWindow.height != 0)
    {
        // mapping crop window to CAMIF size from Sensor Active pixel size
        m_setInputs.statsWindowInfo.cropWindow.width  = Utils::RoundFLOAT(HALCropWindow.width * widthRatio);
        m_setInputs.statsWindowInfo.cropWindow.height = Utils::RoundFLOAT(HALCropWindow.height * heightRatio);
        m_setInputs.statsWindowInfo.cropWindow.left   = Utils::RoundFLOAT(HALCropWindow.left * widthRatio);
        m_setInputs.statsWindowInfo.cropWindow.top    = Utils::RoundFLOAT(HALCropWindow.top * heightRatio);
    }
    else
    {
        m_setInputs.statsWindowInfo.cropWindow.left   = 0;
        m_setInputs.statsWindowInfo.cropWindow.top    = 0;
        m_setInputs.statsWindowInfo.cropWindow.width  = m_setInputs.statsSensorInfo.sensorResWidth;
        m_setInputs.statsWindowInfo.cropWindow.height = m_setInputs.statsSensorInfo.sensorResHeight;
        CAMX_LOG_WARN(CamxLogGroupAWB, "Wrong input: HALCropWindow width,height=%d %d",
        HALCropWindow.width, HALCropWindow.height);
    }

    LoadSetInputParamList(pSetInputList,
                          AWBSetParamTypeStatsWindow,
                          static_cast<const VOID*>(&m_setInputs.statsWindowInfo),
                          sizeof(StatsWindowInfo));

    CAMX_LOG_VERBOSE(CamxLogGroupAWB,
        "Stats crop window: HALCrop %d %d %d %d, AdjustCrop %d %d %d %d, SensorRes %d %d, ActiveArray %d %d, w h ratio %f %f",
        HALCropWindow.left,
        HALCropWindow.top,
        HALCropWindow.width,
        HALCropWindow.height,
        m_setInputs.statsWindowInfo.cropWindow.left,
        m_setInputs.statsWindowInfo.cropWindow.top,
        m_setInputs.statsWindowInfo.cropWindow.width,
        m_setInputs.statsWindowInfo.cropWindow.height,
        m_setInputs.statsSensorInfo.sensorResWidth,
        m_setInputs.statsSensorInfo.sensorResHeight,
        m_setInputs.statsSensorInfo.sensorActiveResWidth,
        m_setInputs.statsSensorInfo.sensorActiveResHeight,
        widthRatio,
        heightRatio);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::RetrieveCalibrationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::RetrieveCalibrationData(
    const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
    AWBAlgoSetParamList*            pSetInputList)
{
    /// @todo (CAMX-151): Implement calibration data input, including:
    /// Illuminants calibration
    /// Geometrical disparity calibration
    /// LED calibration

    CamxResult      result = CamxResultSuccess;
    HwCameraInfo    cameraInfo;

    result = HwEnvironment::GetInstance()->GetCameraInfo(pStatsProcessRequestDataInfo->cameraInfo.cameraId, &cameraInfo);

    if (CamxResultSuccess == result)
    {
        FillIlluminantCalibrationFactor(&cameraInfo.pSensorCaps->OTPData.WBCalibration[0],
                        &m_setInputs.illuminantsCalibrationFactor);

        /// @note Retrieved illuminants calibration data should be filled into m_setInputs.illuminantsCalibrationFactor
        LoadSetInputParamList(pSetInputList,
                              AWBSetParamTypeIlluminantsCalibrationFactor,
                              static_cast<const VOID*>(&m_setInputs.illuminantsCalibrationFactor),
                              sizeof(AWBAlgoIlluminantsCalibrationFactor));

        /// @note Retrieved geometrical calibration data should be filled into m_setInputs.geometricalDisparityCalibration
        LoadSetInputParamList(pSetInputList,
                              AWBSetParamTypeGeometricalDisparityCalibration,
                              static_cast<const VOID*>(&m_setInputs.geometricalDisparityCalibration),
                              sizeof(AWBAlgoGeometricalDisparityCalibration));

        /// @note Retrieved LED calibration data should be filled into m_setInputs.LEDCalibrationDataInput
        LoadSetInputParamList(pSetInputList,
                              AWBSetParamTypeLEDCalibrationData,
                              static_cast<const VOID*>(&m_setInputs.LEDCalibrationDataInput),
                              sizeof(StatsLEDCalibrationDataInput));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "Failed to get OTP data, result: %s", Utils::CamxResultToString(result));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::RetrieveAWBMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::RetrieveAWBMode(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AWBAlgoSetParamList*           pSetInputList)
{
    CAMX_UNREFERENCED_PARAM(pStatsProcessRequestDataInfo);

    CamxResult              result                  = CamxResultSuccess;
    static const UINT StatsData[] =
    {
        InputControlAWBMode,
    };

    static const UINT StatsLength = CAMX_ARRAY_SIZE(StatsData);
    VOID*             pData[StatsLength] = { 0 };
    pData[0] = m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypeInputControlAWBMode);

    if (NULL == pData[0])
    {
        CAMX_LOG_WARN(CamxLogGroupAWB, "InputControlAWBMode is not published!");
        result = CamxResultEFailed;
    }
    else
    {
        switch (*reinterpret_cast<ControlAWBModeValues*>(pData[0]))
        {
            case ControlAWBModeOff:
                m_setInputs.algoMode = AWBAlgoModeOff;
                break;
            case ControlAWBModeAuto:
                m_setInputs.algoMode = AWBAlgoModeAuto;
                break;
            case ControlAWBModeIncandescent:
                m_setInputs.algoMode = AWBAlgoModeIncandescent;
                break;
            case ControlAWBModeFluorescent:
                m_setInputs.algoMode = AWBAlgoModeFluorescent;
                break;
            case ControlAWBModeWarmFluorescent:
                m_setInputs.algoMode = AWBAlgoModeWarmFluorescent;
                break;
            case ControlAWBModeDaylight:
                m_setInputs.algoMode = AWBAlgoModeDaylight;
                break;
            case ControlAWBModeCloudyDaylight:
                m_setInputs.algoMode = AWBAlgoModeCloudyDaylight;
                break;
            case ControlAWBModeTwilight:
                m_setInputs.algoMode = AWBAlgoModeTwilight;
                break;
            case ControlAWBModeShade:
                m_setInputs.algoMode = AWBAlgoModeShade;
                break;
            default:
                m_setInputs.algoMode = AWBAlgoModeAuto;
                break;
        }

        m_outputs.mode = m_setInputs.algoMode;
        LoadSetInputParamList(pSetInputList,
                              AWBSetParamTypeWhiteBalanceMode,
                              static_cast<const VOID*>(&m_setInputs.algoMode),
                              sizeof(AWBAlgoMode));

    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::RetrieveSceneMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::RetrieveSceneMode(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AWBAlgoSetParamList*           pSetInputList)
{
    CAMX_UNREFERENCED_PARAM(pStatsProcessRequestDataInfo);

    CamxResult              result                  = CamxResultSuccess;
    static const UINT StatsData[] =
    {
        InputControlSceneMode,
    };

    static const UINT StatsLength = CAMX_ARRAY_SIZE(StatsData);
    VOID*             pData[StatsLength] = { 0 };
    pData[0] = m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypeInputControlSceneMode);

    if (NULL == pData[0])
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAWB, "InputControlSceneMode is not published!");
    }
    else
    {
        m_setInputs.sceneMode = *reinterpret_cast<UINT32*>(pData[0]);
        if (TRUE == HwEnvironment::GetInstance()->GetStaticSettings()->ignoreSceneMode)
        {
            m_setInputs.sceneMode = ControlSceneModeDisabled;
        }
        LoadSetInputParamList(pSetInputList,
                              AWBSetParamTypeSceneMode,
                              static_cast<const VOID*>(&m_setInputs.sceneMode),
                              sizeof(UINT32));
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::RetrieveAWBLockInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::RetrieveAWBLockInfo(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AWBAlgoSetParamList*           pSetInputList)
{
    CAMX_UNREFERENCED_PARAM(pStatsProcessRequestDataInfo);

    CamxResult          result                  = CamxResultSuccess;
    static const UINT StatsData[] =
    {
        InputControlAWBLock,
    };

    static const UINT StatsLength = CAMX_ARRAY_SIZE(StatsData);
    VOID*             pData[StatsLength] = { 0 };
    pData[0] = m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypeInputControlAWBLock);

    if (NULL == pData[0])
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAWB, "InputControlAWBLock is not published!");
        result = CamxResultEFailed;
    }
    else
    {
        if (ControlAWBLockOn == *reinterpret_cast<ControlAWBLockValues*>(pData[0]))
        {
            m_setInputs.lock = TRUE;
        }
        else
        {
            m_setInputs.lock = FALSE;
        }
        LoadSetInputParamList(pSetInputList,
                              AWBSetParamTypeLock,
                              static_cast<const VOID*>(&m_setInputs.lock),
                              sizeof(BOOL));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::RetrieveROIInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::RetrieveROIInfo(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AWBAlgoSetParamList*           pSetInputList)
{
    CAMX_UNREFERENCED_PARAM(pStatsProcessRequestDataInfo);
    UINT32     metaTagFDROI = 0;
    CamxResult result       = CamxResultSuccess;

    if (TRUE == m_pNode->GetStaticSettings()->useFDUseCasePool)
    {
        metaTagFDROI = PropertyIDUsecaseFDResults;
    }
    else
    {
        result = VendorTagManager::QueryVendorTagLocation(VendorTagSectionOEMFDResults,
            VendorTagNameOEMFDResults,
            &metaTagFDROI);
    }
    static const UINT       GetProps[]              = { metaTagFDROI };
    static const UINT       GerPropsLength          = CAMX_ARRAY_SIZE(GetProps);
    VOID*                   pData[GerPropsLength]   = { 0 };
    UINT64                  offsets[GerPropsLength] = { m_pNode->GetMaximumPipelineDelay() };

    if (CamxResultSuccess == result)
    {
        m_pNode->GetDataList(GetProps, pData, offsets, GerPropsLength);
    }

    if (NULL != pData[0])
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAWB, "FD results published");
        FaceROIInformation*    pFaceROIInfo = reinterpret_cast<FaceROIInformation*>(pData[0]);
        for (UINT32 index = 0; index < pFaceROIInfo->ROICount; index++)
        {
            m_setInputs.statsFaceInfo.face[index].roi.left   = pFaceROIInfo->stabilizedROI[index].faceRect.left;
            m_setInputs.statsFaceInfo.face[index].roi.top    = pFaceROIInfo->stabilizedROI[index].faceRect.top;
            m_setInputs.statsFaceInfo.face[index].roi.width  = pFaceROIInfo->stabilizedROI[index].faceRect.width;
            m_setInputs.statsFaceInfo.face[index].roi.height = pFaceROIInfo->stabilizedROI[index].faceRect.height;

            CAMX_LOG_VERBOSE(CamxLogGroupAWB,
                             "Face ROI[%d]: id %d (left: %d, top: %d, width: %d, height: %d)",
                             index,
                             pFaceROIInfo->stabilizedROI[index].id,
                             pFaceROIInfo->stabilizedROI[index].faceRect.left,
                             pFaceROIInfo->stabilizedROI[index].faceRect.top,
                             pFaceROIInfo->stabilizedROI[index].faceRect.width,
                             pFaceROIInfo->stabilizedROI[index].faceRect.height);
        }
        m_setInputs.statsFaceInfo.faceCount = pFaceROIInfo->ROICount;
        m_setInputs.statsFaceInfo.requestID = static_cast<UINT32>(pStatsProcessRequestDataInfo->requestId);

        LoadSetInputParamList(pSetInputList,
                              AWBSetParamTypeFaceROI,
                              &m_setInputs.statsFaceInfo,
                              sizeof(StatsFaceInformation));

        // new face information interface with same structure in 3A
        for (UINT32 index = 0; index < pFaceROIInfo->ROICount; index++)
        {
            m_setInputs.unstabilizedFaceInfo.roi[index].rect.left   = pFaceROIInfo->unstabilizedROI[index].faceRect.left;
            m_setInputs.unstabilizedFaceInfo.roi[index].rect.top    = pFaceROIInfo->unstabilizedROI[index].faceRect.top;
            m_setInputs.unstabilizedFaceInfo.roi[index].rect.width  = pFaceROIInfo->unstabilizedROI[index].faceRect.width;
            m_setInputs.unstabilizedFaceInfo.roi[index].rect.height = pFaceROIInfo->unstabilizedROI[index].faceRect.height;
            m_setInputs.unstabilizedFaceInfo.roi[index].roiID       = pFaceROIInfo->unstabilizedROI[index].id;
            CAMX_LOG_VERBOSE(CamxLogGroupAWB,
                             "Face ROI[%d]: id %d (left: %d, top: %d, width: %d, height: %d)",
                             index,
                             pFaceROIInfo->unstabilizedROI[index].id,
                             pFaceROIInfo->unstabilizedROI[index].faceRect.left,
                             pFaceROIInfo->unstabilizedROI[index].faceRect.top,
                             pFaceROIInfo->unstabilizedROI[index].faceRect.width,
                             pFaceROIInfo->unstabilizedROI[index].faceRect.height);
        }
        m_setInputs.unstabilizedFaceInfo.roiCount  = pFaceROIInfo->ROICount;
        m_setInputs.unstabilizedFaceInfo.requestID = static_cast<UINT64>(pStatsProcessRequestDataInfo->requestId);

        LoadSetInputParamList(pSetInputList,
                              AWBSetParamTypeUnstabilizedFaceROI,
                              &m_setInputs.unstabilizedFaceInfo,
                              sizeof(UnstabilizedROIInformation));
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAWB, "FD results not published");
    }

    if (NULL != m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypeInputControlAWBRegions))
    {
        StatsWeightedROI* pTouchROI = reinterpret_cast<StatsWeightedROI*>(
            m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypeInputControlAWBRegions));

        m_setInputs.statsTouchROI.ROI.left = pTouchROI->ROI.left;
        m_setInputs.statsTouchROI.ROI.top    = pTouchROI->ROI.top;
        m_setInputs.statsTouchROI.ROI.width  = pTouchROI->ROI.width;
        m_setInputs.statsTouchROI.ROI.height = pTouchROI->ROI.height;
        m_setInputs.statsTouchROI.ROIWeight = pTouchROI->ROIWeight;

        LoadSetInputParamList(pSetInputList,
                              AWBSetParamTypeTouchROI,
                              &m_setInputs.statsTouchROI,
                              sizeof(StatsRectangle));

        CAMX_LOG_VERBOSE(CamxLogGroupAWB,
                         "Touch ROI / ControlAWBRegions : (left: %d, top: %d, width: %d, height: %d)",
                         pTouchROI->ROI.left,
                         pTouchROI->ROI.top,
                         pTouchROI->ROI.width,
                         pTouchROI->ROI.height);

    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::RetrieveFlashInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::RetrieveFlashInfo(
    const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
    AWBAlgoSetParamList*            pSetInputList)
{
    CAMX_UNREFERENCED_PARAM(pStatsProcessRequestDataInfo);

    CamxResult              result                  = CamxResultSuccess;
    static const UINT StatsData[] =
    {
        PropertyIDAECInternal,
        PropertyIDAECFrameInfo,
        InputControlCaptureIntent,
        InputFlashMode
    };

    static const UINT StatsLength               = CAMX_ARRAY_SIZE(StatsData);
    VOID*             pData[StatsLength]        = { 0 };
    UINT64            statsOffsets[StatsLength] = { 0 };

    if (FirstValidRequestId < pStatsProcessRequestDataInfo->requestId)
    {
        pData[0] = m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypePropertyIDAECInternal);
        pData[1] = m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypePropertyIDAECFrameInfo);
        pData[2] = m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypeInputControlCaptureIntent);
        pData[3] = m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypeFlashMode);
    }
    else
    {
        m_pNode->GetDataList(StatsData, pData, statsOffsets, StatsLength);
    }

    if ((NULL == pData[0]) || (NULL == pData[1]) || (NULL == pData[2]) || (NULL == pData[3]))
    {
        CAMX_LOG_WARN(CamxLogGroupAWB,
                       "reqId: %llu PropertyID:AECInternal:%p AECFrameInfo:%p CI:%p FlashMode:%p not published",
                       pStatsProcessRequestDataInfo->requestId,
                       pData[0],
                       pData[1],
                       pData[2],
                       pData[3]);
        result = CamxResultEFailed;
    }
    else
    {
        AECOutputInternal*          pAECInternalProperty = reinterpret_cast<AECOutputInternal*>(pData[0]);
        AECFrameInformation*        pAECFrameInfo        = reinterpret_cast<AECFrameInformation*>(pData[1]);
        ControlCaptureIntentValues* pCaptureIntent       = reinterpret_cast<ControlCaptureIntentValues*>(pData[2]);
        FlashModeValues             flashMode            = *reinterpret_cast<FlashModeValues*>(pData[3]);

        // Only LED type flash for now
        m_setInputs.flashType          = AWBAlgoFlashTypeLED;
        m_setInputs.flashInfo.bIsTorch = (FlashModeTorch == flashMode)? TRUE : FALSE;
        LoadSetInputParamList(pSetInputList,
                              AWBSetParamTypeFlashType,
                              static_cast<const VOID*>(&m_setInputs.flashType),
                              sizeof(AWBAlgoFlashType));

        m_setInputs.flashInfo.LEDInfluenceRatio = pAECInternalProperty->LEDInfluenceRatio;
        m_setInputs.flashInfo.LEDBGRatio        = pAECInternalProperty->LEDBGRatio;
        m_setInputs.flashInfo.LEDRGRatio        = pAECInternalProperty->LEDRGRatio;

        switch (pAECFrameInfo->AECPreFlashState)
        {
            case PreFlashStateInactive:
                m_setInputs.flashInfo.flashState = AWBAlgoFlashInactive;
                break;
            case PreFlashStateStart:
                m_setInputs.flashInfo.flashState = AWBAlgoPreFlashStart;
                break;
            case PreFlashStateTriggerFD:
                m_setInputs.flashInfo.flashState = AWBAlgoPreFlashTriggerFD;
                break;
            case PreFlashStateTriggerAF:
                m_setInputs.flashInfo.flashState = AWBAlgoPreFlashTriggerAF;
                break;
            case PreFlashStateTriggerAWB:
                m_setInputs.flashInfo.flashState = AWBAlgoPreFlashTriggerAWB;
                break;
            case PreFlashStateCompleteLED:
                m_setInputs.flashInfo.flashState = AWBAlgoPreFlashCompleteLED;
                break;
            case PreFlashStateCompleteNoLED:
                m_setInputs.flashInfo.flashState = AWBAlgoPreFlashCompleteNoLED;
                break;
            default:
                m_setInputs.flashInfo.flashState = AWBAlgoFlashInactive;
                break;
        }

        static const UINT GetData[] =
        {
            PropertyIDAECFrameControl,
        };

        static const UINT GetDataLength = CAMX_ARRAY_SIZE(GetData);
        VOID*             pAECFrameControlData[GetDataLength] = { 0 };
        UINT64            offsets[GetDataLength] = { 1 };
        // For snapshot case AWB reads AECFrameControl from current frame as LEDCurrent is 0 in previous frame
        if (ControlCaptureIntentStillCapture == *pCaptureIntent)
        {
            offsets[0] = 0;
        }
        m_pNode->GetDataList(GetData, pAECFrameControlData, offsets, GetDataLength);
        AECFrameControl*            pAECFrameControl = NULL;
        if (NULL != pAECFrameControlData[0])
        {
            pAECFrameControl = reinterpret_cast<AECFrameControl*>(pAECFrameControlData[0]);
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupAWB,
                "reqId: %llu PropertyIDAECFrameControl:%p  not published",
                pStatsProcessRequestDataInfo->requestId,
                pAECFrameControlData[0]);
            result = CamxResultEFailed;
        }
        if (CamxResultSuccess == result)
        {
            if ((ControlCaptureIntentStillCapture == *pCaptureIntent) &&
                ((pAECFrameControl->LEDCurrents[LEDSetting1] > 0) || (pAECFrameControl->LEDCurrents[LEDSetting2] > 0)))
            {
                m_setInputs.flashInfo.flashState = AWBAlgoMainFlash;
            }

            switch (pAECFrameControl->flashInfo)
            {
                case FlashInfoTypeOff:
                    m_setInputs.flashInfo.flashType = AWBAlgoFlashOff;
                    break;
                case FlashInfoTypePre:
                    m_setInputs.flashInfo.flashType = AWBAlgoFlashPre;
                    break;
                case FlashInfoTypeMain:
                    m_setInputs.flashInfo.flashType = AWBAlgoFlashMain;
                    break;
                case FlashInfoTypeMax:
                    m_setInputs.flashInfo.flashType = AWBAlgoFlashMax;
                    break;
                default:
                    CAMX_LOG_ERROR(CamxLogGroupAWB, "Invalid Flash info type");
                    break;
            }

            LoadSetInputParamList(pSetInputList,
                                  AWBSetParamTypeFlashData,
                                  static_cast<const VOID*>(&m_setInputs.flashInfo),
                                  sizeof(AWBAlgoFlashInformation));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::ReadDynamicConvergenceSpeed
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::ReadDynamicConvergenceSpeed(
    const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
    AWBAlgoSetParamList*            pSetInputList)
{
    FLOAT      dynamicConvergenceSpeed = 0.0f;
    CamxResult result                  = CamxResultSuccess;
    VOID*      pData                   = NULL;

    StatsUtil::ReadVendorTag(m_pNode, "org.codeaurora.qcamera3.awb_convergence_speed", "awb_speed", &pData);

    if (NULL != pData)
    {
        Utils::Memcpy(&dynamicConvergenceSpeed, pData, sizeof(FLOAT));

        m_setInputs.dynamicConvergenceSpeed = dynamicConvergenceSpeed;
        LoadSetInputParamList(pSetInputList,
                              AWBSetParamTypeDynamicConvergenceSpeed,
                              static_cast<const VOID*>(&m_setInputs.dynamicConvergenceSpeed),
                              sizeof(FLOAT));
        CAMX_LOG_VERBOSE(CamxLogGroupAWB, "ReqId: %llu dynamicConvergenceSpeed:%f)",
                         pStatsProcessRequestDataInfo->requestId,
                         m_setInputs.dynamicConvergenceSpeed);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "pData is NULL for requestId:%llu", pStatsProcessRequestDataInfo->requestId);
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::RetrieveWarmstartInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::RetrieveWarmstartInfo(
    const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
    AWBAlgoSetParamList*            pSetInputList)
{
    CAMX_UNREFERENCED_PARAM(pStatsProcessRequestDataInfo);

    FLOAT*  pGains    = NULL;
    FLOAT*  pCCT      = NULL;
    FLOAT*  pDecision = NULL;
    CamxResult              result                  = CamxResultSuccess;

    result = ReadVendorTag("org.quic.camera2.statsconfigs", "AWBWarmstartGain", reinterpret_cast<VOID**>(&pGains));
    result = ReadVendorTag("org.quic.camera2.statsconfigs", "AWBWarmstartCCT", reinterpret_cast<VOID**>(&pCCT));
    result = ReadVendorTag("org.quic.camera2.statsconfigs", "AWBDecisionAfterTC", reinterpret_cast<VOID**>(&pDecision));

    if ((NULL == pGains) || (NULL == pCCT) || (NULL == pDecision))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAWB, "ReqId: %llu Warm-start gain:%p or CCT:%p or curDecision:%p not published",
            pStatsProcessRequestDataInfo->requestId, pGains, pCCT, pDecision);
    }
    else
    {
        m_setInputs.warmstartInfo.gains.red                  = pGains[0];
        m_setInputs.warmstartInfo.gains.green                = pGains[1];
        m_setInputs.warmstartInfo.gains.blue                 = pGains[2];
        m_setInputs.warmstartInfo.correlatedColorTemperature = pCCT[0];
        m_setInputs.warmstartInfo.curFrameDecisionAfterTC.rg = pDecision[0];
        m_setInputs.warmstartInfo.curFrameDecisionAfterTC.bg = pDecision[1];

        LoadSetInputParamList(pSetInputList,
                              AWBSetParamTypeWarmstart,
                              static_cast<const VOID*>(&m_setInputs.warmstartInfo),
                              sizeof(AWBAlgoWarmstartInformation));

        CAMX_LOG_VERBOSE(CamxLogGroupAWB, "ReqId: %llu Warm-start gain:(R:%f G:%f B:%f) CCT:%d or curDecision:(rg:%f bg:%f)",
            pStatsProcessRequestDataInfo->requestId,
            m_setInputs.warmstartInfo.gains.red,
            m_setInputs.warmstartInfo.gains.green,
            m_setInputs.warmstartInfo.gains.blue,
            m_setInputs.warmstartInfo.correlatedColorTemperature,
            m_setInputs.warmstartInfo.curFrameDecisionAfterTC.rg,
            m_setInputs.warmstartInfo.curFrameDecisionAfterTC.bg);
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::RetrieveExtensionTriggerInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::RetrieveExtensionTriggerInfo(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AWBAlgoSetParamList*           pSetInputList)
{
    CamxResult result = CamxResultSuccess;

    for (INT32 i = 0; i < static_cast<INT32>(AWBAlgoExternalInputSize); i++)
    {
        m_setInputs.extensionTriggerInfo.inputData[i] = 0.0f;
    }

    result = PopulateTOFData(&m_TOFData);

    if (CamxResultEFailed == result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAWB, "ReqId: %llu ambientRate not published",
                pStatsProcessRequestDataInfo->requestId);
    }
    else
    {
        m_setInputs.extensionTriggerInfo.inputData[0] = m_TOFData.ambientRate;
        LoadSetInputParamList(pSetInputList,
                AWBSetParamTypeExternalInfo,
                static_cast<const VOID*>(&m_setInputs.extensionTriggerInfo),
                sizeof(AWBAlgoExternalInformation));
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::SetPeerInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::SetPeerInfo(
    const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
    AWBAlgoSetParamList*            pSetInputList)
{
    CamxResult  result          = CamxResultEFailed;
    UINT64      requestID       = pStatsProcessRequestDataInfo->requestId;
    BOOL        needSync        = pStatsProcessRequestDataInfo->peerSyncInfo.needSync;
    INT64       requestDelta    = pStatsProcessRequestDataInfo->peerSyncInfo.requestDelta;
    UINT        peerPipelineId  = pStatsProcessRequestDataInfo->peerSyncInfo.peerPipelineID;

    if (TRUE == needSync)
    {
        // Get property from peer pipeline
        const UINT  getProps[]  = { PropertyIDAWBPeerInfo };
        VOID*       pData[1]    = { 0 };
        UINT64      offsets[1]  = { static_cast<UINT64>(abs(requestDelta)) };
        BOOL        negates[1]  = { (requestDelta < 0) ? TRUE : FALSE };

        result = m_pNode->GetDataListFromPipeline(getProps, pData, offsets, 1, negates, peerPipelineId);
        if (NULL == pData[0])
        {
            CAMX_LOG_VERBOSE(CamxLogGroupAWB, "PropertyIDAWBPeerInfo not published, Req:%llu requestDelta:%lld",
                             requestID, requestDelta);
        }
        else
        {
            m_setInputs.pPeerInfo = *(static_cast<VOID**>(pData[0]));
            CAMX_LOG_VERBOSE(CamxLogGroupAWB, "Peer info: %p, Req:%llu pipeline:%d Peer_Req:%llu Peer_Pipeline:%d",
                        m_setInputs.pPeerInfo,
                        requestID,
                        m_pNode->GetPipelineId(),
                        static_cast<INT64>(requestID) - static_cast<INT64>(requestDelta),
                        peerPipelineId);

            LoadSetInputParamList(pSetInputList, AWBSetParamTypePeerInfo, m_setInputs.pPeerInfo, sizeof(VOID*));
        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAWB, "No need to sync for Req:%llu on pipeline:%d",
            requestID, m_pNode->GetPipelineId());

        m_setInputs.pPeerInfo = NULL;

        LoadSetInputParamList(pSetInputList, AWBSetParamTypePeerInfo, m_setInputs.pPeerInfo, sizeof(VOID*));

        result = CamxResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::GetAlgoProcessInput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::GetAlgoProcessInput(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AWBAlgoInputList*              pInputList,
    BOOL                           processStats)
{
    CAMX_ENTRYEXIT(CamxLogGroupAWB);

    CamxResult result       = CamxResultSuccess;
    pInputList->pAWBInputs  = m_processInputArray;
    pInputList->inputCount  = AWBInputTypeLastIndex - 1;

    GetRequestNumber(pStatsProcessRequestDataInfo, pInputList);

    result = GetStatistics(pStatsProcessRequestDataInfo, pInputList, processStats);


    UINT64 requestIdOffsetFromLastFlush =
        m_pNode->GetRequestIdOffsetFromLastFlush(pStatsProcessRequestDataInfo->requestId);

    if (CamxResultSuccess == result && FirstValidRequestId < requestIdOffsetFromLastFlush)
    {
        result = GetAECData(pInputList);
    }

    if (CamxResultSuccess == result)
    {
        // we can ignore the return result here since some of the vendor tags may not be available
        GetVendorTags(pInputList);
    }

    if (CamxResultSuccess == result)
    {
        result = GetDebugDataBuffer(pStatsProcessRequestDataInfo, pInputList);
    }

    if (CamxResultSuccess == result)
    {
        result = GetChiStatsSession(pStatsProcessRequestDataInfo, pInputList);
    }

    if (CamxResultSuccess == result)
    {
        result = SetCameraInformation(pStatsProcessRequestDataInfo, pInputList, processStats);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::GetAlgoExpectedOutputList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::GetAlgoExpectedOutputList(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AWBAlgoOutputList*             pOutputList)
{
    CAMX_ENTRYEXIT(CamxLogGroupAWB);

    CAMX_UNREFERENCED_PARAM(pStatsProcessRequestDataInfo);

    pOutputList->pAWBOutputs = m_processOutputArray;
    pOutputList->outputCount = AWBOutputTypeLastIndex;

    /// @todo (CAMX-1233): Once debug data is defined, add it here.
    // pOutputList->pAWBOutputs[AWBOutputTypeDebugData].outputType          = AWBOutputTypeDebugData;
    // pOutputList->pAWBOutputs[AWBOutputTypeDebugData].pAWBOutput          = &m_outputs.debugData;
    // pOutputList->pAWBOutputs[AWBOutputTypeDebugData].sizeOfAWBOutput     = sizeof(m_outputs.debugData);

    pOutputList->pAWBOutputs[AWBOutputTypeGains].outputType                 = AWBOutputTypeGains;
    pOutputList->pAWBOutputs[AWBOutputTypeGains].pAWBOutput                 = &m_outputs.gains;
    pOutputList->pAWBOutputs[AWBOutputTypeGains].sizeOfAWBOutput            = sizeof(m_outputs.gains);

    pOutputList->pAWBOutputs[AWBOutputTypeColorTemperature].outputType      = AWBOutputTypeColorTemperature;
    pOutputList->pAWBOutputs[AWBOutputTypeColorTemperature].pAWBOutput      = &m_outputs.cct;
    pOutputList->pAWBOutputs[AWBOutputTypeColorTemperature].sizeOfAWBOutput = sizeof(m_outputs.cct);

    pOutputList->pAWBOutputs[AWBOutputTypeIlluminantType].outputType        = AWBOutputTypeIlluminantType;
    pOutputList->pAWBOutputs[AWBOutputTypeIlluminantType].pAWBOutput        = &m_outputs.illuminantType;
    pOutputList->pAWBOutputs[AWBOutputTypeIlluminantType].sizeOfAWBOutput   = sizeof(m_outputs.illuminantType);

    pOutputList->pAWBOutputs[AWBOutputTypeSampleDecision].outputType        = AWBOutputTypeSampleDecision;
    pOutputList->pAWBOutputs[AWBOutputTypeSampleDecision].pAWBOutput        = m_outputs.sampleDecision;
    pOutputList->pAWBOutputs[AWBOutputTypeSampleDecision].sizeOfAWBOutput   = sizeof(m_outputs.sampleDecision);

    pOutputList->pAWBOutputs[AWBOutputTypeBGConfig].outputType              = AWBOutputTypeBGConfig;
    pOutputList->pAWBOutputs[AWBOutputTypeBGConfig].pAWBOutput              = &m_outputs.BGConfig;
    pOutputList->pAWBOutputs[AWBOutputTypeBGConfig].sizeOfAWBOutput         = sizeof(m_outputs.BGConfig);

    pOutputList->pAWBOutputs[AWBOutputTypeState].outputType                 = AWBOutputTypeState;
    pOutputList->pAWBOutputs[AWBOutputTypeState].pAWBOutput                 = &m_outputs.state;
    pOutputList->pAWBOutputs[AWBOutputTypeState].sizeOfAWBOutput            = sizeof(m_outputs.state);

    pOutputList->pAWBOutputs[AWBOutputTypeMode].outputType                  = AWBOutputTypeMode;
    pOutputList->pAWBOutputs[AWBOutputTypeMode].pAWBOutput                  = &m_outputs.mode;
    pOutputList->pAWBOutputs[AWBOutputTypeMode].sizeOfAWBOutput             = sizeof(m_outputs.mode);

    pOutputList->pAWBOutputs[AWBOutputTypeLock].outputType                  = AWBOutputTypeLock;
    pOutputList->pAWBOutputs[AWBOutputTypeLock].pAWBOutput                  = &m_outputs.lock;
    pOutputList->pAWBOutputs[AWBOutputTypeLock].sizeOfAWBOutput             = sizeof(m_outputs.lock);

    pOutputList->pAWBOutputs[AWBOutputTypeVendorTag].outputType             = AWBOutputTypeVendorTag;
    pOutputList->pAWBOutputs[AWBOutputTypeVendorTag].pAWBOutput             = &m_outputs.vendorTagList;
    pOutputList->pAWBOutputs[AWBOutputTypeVendorTag].sizeOfAWBOutput        = sizeof(m_outputs.vendorTagList);

    pOutputList->pAWBOutputs[AWBOutputTypeCCM].outputType                   = AWBOutputTypeCCM;
    pOutputList->pAWBOutputs[AWBOutputTypeCCM].pAWBOutput                   = &m_outputs.CCMList;
    pOutputList->pAWBOutputs[AWBOutputTypeCCM].sizeOfAWBOutput              = sizeof(StatsAWBCCMList);

    pOutputList->pAWBOutputs[AWBOutputTypeCurrentDecision].outputType       = AWBOutputTypeCurrentDecision;
    pOutputList->pAWBOutputs[AWBOutputTypeCurrentDecision].pAWBOutput       = &m_outputs.currentDecision;
    pOutputList->pAWBOutputs[AWBOutputTypeCurrentDecision].sizeOfAWBOutput  = sizeof(AWBAlgoDecisionInformation);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::GetAlgoGetParamInputOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::GetAlgoGetParamInputOutput(
    const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
    AWBAlgoGetParam*                pGetParam)
{
    CAMX_ENTRYEXIT(CamxLogGroupAWB);


    pGetParam->inputInfoList.pGetParamInputs      = m_getParamInputArray;
    pGetParam->inputInfoList.getParamInputCount   = AWBGetParamInputTypeLastIndex;

    pGetParam->outputInfoList.pGetParamOutputs    = m_getParamOutputArray;
    pGetParam->outputInfoList.getParamOutputCount = AWBGetParamOutputTypeLastIndex;

    // invalidate input/output list

    for (UINT32 i = 0; i < AWBGetParamInputTypeLastIndex; i++)
    {
        m_getParamInputArray[i].getParamInputType = AWBGetParamInputTypeInvalid;
    }
    for (UINT32 i = 0; i < AWBGetParamOutputTypeLastIndex; i++)
    {
        m_getParamOutputArray[i].getParamOutputType = AWBGetParamOutputTypeInvalid;
    }

    // Set camera information for get
    m_getParamInputArray[AWBGetParamInputTypeCameraInfo].getParamInputType   = AWBGetParamInputTypeCameraInfo;
    m_getParamInputArray[AWBGetParamInputTypeCameraInfo].pGetParamInput      = reinterpret_cast<VOID*>(&m_inputs.cameraInfo);
    m_getParamInputArray[AWBGetParamInputTypeCameraInfo].sizeOfGetParamInput = sizeof(StatsCameraInfo);

    if (AWBGetParamTypeLastOutput == pGetParam->type)
    {
        GetAlgoExpectedOutputList(pStatsProcessRequestDataInfo, &m_outputs.lastOutputList);

        m_getParamOutputArray[AWBGetParamOutputTypeOutputList].getParamOutputType          = AWBGetParamOutputTypeOutputList;
        m_getParamOutputArray[AWBGetParamOutputTypeOutputList].pGetParamOutput             = &m_outputs.lastOutputList;
        m_getParamOutputArray[AWBGetParamOutputTypeOutputList].sizeOfGetParamOutput        = sizeof(m_outputs.lastOutputList);
        m_getParamOutputArray[AWBGetParamOutputTypeOutputList].sizeOfWrittenGetParamOutput = 0;
    }
    else if (AWBGetParamTypeFlashEstimationState == pGetParam->type)
    {
        m_getParamOutputArray[AWBGetParamOutputTypeFlashEstimationProgress].getParamOutputType          =
            AWBGetParamOutputTypeFlashEstimationProgress;
        m_getParamOutputArray[AWBGetParamOutputTypeFlashEstimationProgress].pGetParamOutput             =
            &m_algoFlashEstimationState;
        m_getParamOutputArray[AWBGetParamOutputTypeFlashEstimationProgress].sizeOfGetParamOutput        =
            sizeof(m_algoFlashEstimationState);
        m_getParamOutputArray[AWBGetParamOutputTypeFlashEstimationProgress].sizeOfWrittenGetParamOutput = 0;
    }
    else if (AWBGetParamTypeDependentVendorTags == pGetParam->type)
    {
        Utils::Memset(&m_vendorTagInputList, 0, sizeof(m_vendorTagInputList));

        m_getParamOutputArray[AWBGetParamTypeDependentVendorTags].getParamOutputType          =
            AWBGetParamOutputTypeDependentVendorTags;
        m_getParamOutputArray[AWBGetParamTypeDependentVendorTags].pGetParamOutput             = &m_vendorTagInputList;
        m_getParamOutputArray[AWBGetParamTypeDependentVendorTags].sizeOfGetParamOutput        =
            sizeof(m_vendorTagInputList);
        m_getParamOutputArray[AWBGetParamTypeDependentVendorTags].sizeOfWrittenGetParamOutput = 0;
    }
    else if (AWBGetParamTypePublishingVendorTagsInfo == pGetParam->type)
    {
        Utils::Memset(&m_vendorTagInfoOutputputList, 0, sizeof(m_vendorTagInfoOutputputList));

        m_getParamOutputArray[AWBGetParamOutputTypePublishingVendorTagsInfo].getParamOutputType          =
            AWBGetParamOutputTypePublishingVendorTagsInfo;
        m_getParamOutputArray[AWBGetParamOutputTypePublishingVendorTagsInfo].pGetParamOutput             =
            &m_vendorTagInfoOutputputList;
        m_getParamOutputArray[AWBGetParamOutputTypePublishingVendorTagsInfo].sizeOfGetParamOutput        =
            sizeof(m_vendorTagInfoOutputputList);
        m_getParamOutputArray[AWBGetParamOutputTypePublishingVendorTagsInfo].sizeOfWrittenGetParamOutput = 0;
    }
    else if (AWBGetParamTypeFlashOutput == pGetParam->type)
    {
        for (UINT32 i = 0; i < AWBOutputTypeLastIndex; i++)
        {
            m_processOutputArray[i].outputType = AWBOutputTypeInvalid;
        }
        m_processOutputArray[AWBOutputTypeGains].outputType                                = AWBOutputTypeGains;
        m_processOutputArray[AWBOutputTypeGains].pAWBOutput                                = &m_outputs.flashGains;
        m_processOutputArray[AWBOutputTypeGains].sizeOfAWBOutput                           = sizeof(m_outputs.flashGains);

        m_processOutputArray[AWBOutputTypeColorTemperature].outputType                     = AWBOutputTypeColorTemperature;
        m_processOutputArray[AWBOutputTypeColorTemperature].pAWBOutput                     = &m_outputs.flashCCT;
        m_processOutputArray[AWBOutputTypeColorTemperature].sizeOfAWBOutput                = sizeof(m_outputs.flashCCT);

        m_processOutputArray[AWBOutputTypeCCM].outputType                                  = AWBOutputTypeCCM;
        m_processOutputArray[AWBOutputTypeCCM].pAWBOutput                                  = &m_outputs.flashCCMList;
        m_processOutputArray[AWBOutputTypeCCM].sizeOfAWBOutput                             = sizeof(m_outputs.flashCCMList);

        m_outputs.flashOutputList.pAWBOutputs                                              = m_processOutputArray;
        m_outputs.flashOutputList.outputCount                                              = AWBOutputTypeLastIndex;

        m_getParamOutputArray[AWBGetParamOutputTypeOutputList].getParamOutputType          = AWBGetParamOutputTypeOutputList;
        m_getParamOutputArray[AWBGetParamOutputTypeOutputList].pGetParamOutput             = &m_outputs.flashOutputList;
        m_getParamOutputArray[AWBGetParamOutputTypeOutputList].sizeOfGetParamOutput        = sizeof(m_outputs.flashOutputList);
        m_getParamOutputArray[AWBGetParamOutputTypeOutputList].sizeOfWrittenGetParamOutput = 0;
    }
    else if (AWBGetParamTypePeerInfo == pGetParam->type)
    {
        m_getParamOutputArray[AWBGetParamOutputTypePeerInfo].getParamOutputType          = AWBGetParamOutputTypePeerInfo;
        m_getParamOutputArray[AWBGetParamOutputTypePeerInfo].pGetParamOutput             = &m_outputs.pPeerInfo;
        m_getParamOutputArray[AWBGetParamOutputTypePeerInfo].sizeOfGetParamOutput        = sizeof(VOID*);
        m_getParamOutputArray[AWBGetParamOutputTypePeerInfo].sizeOfWrittenGetParamOutput = 0;
    }
    else if (AWBGetParamTypeBGConfig == pGetParam->type)
    {
        m_getParamOutputArray[AWBGetParamOutputTypeBGConfig].getParamOutputType          = AWBGetParamOutputTypeBGConfig;
        m_getParamOutputArray[AWBGetParamOutputTypeBGConfig].pGetParamOutput             = &m_outputs.BGConfig;
        m_getParamOutputArray[AWBGetParamOutputTypeBGConfig].sizeOfGetParamOutput        = sizeof(m_outputs.BGConfig);
        m_getParamOutputArray[AWBGetParamOutputTypeBGConfig].sizeOfWrittenGetParamOutput = 0;
    }
    else if (AWBGetParamTypeGainToCCT == pGetParam->type)
    {
        m_getParamInputArray[AWBGetParamInputTypeGain].getParamInputType            = AWBGetParamInputTypeGain;
        m_getParamInputArray[AWBGetParamInputTypeGain].pGetParamInput               = &m_inputs.gains;
        m_getParamInputArray[AWBGetParamInputTypeGain].sizeOfGetParamInput          = sizeof(AWBAlgoGains);

        m_getParamOutputArray[AWBGetParamOutputTypeCCT].getParamOutputType          = AWBGetParamOutputTypeCCT;
        m_getParamOutputArray[AWBGetParamOutputTypeCCT].pGetParamOutput             = &m_outputs.cct;
        m_getParamOutputArray[AWBGetParamOutputTypeCCT].sizeOfGetParamOutput        = sizeof(UINT32);
        m_getParamOutputArray[AWBGetParamOutputTypeCCT].sizeOfWrittenGetParamOutput = 0;
    }
    else if (AWBGetParamTypeCctToGain == pGetParam->type)
    {
        m_getParamInputArray[AWBGetParamInputTypeCCT].getParamInputType               = AWBGetParamInputTypeCCT;
        m_getParamInputArray[AWBGetParamInputTypeCCT].pGetParamInput                  = &m_inputs.cct;
        m_getParamInputArray[AWBGetParamInputTypeCCT].sizeOfGetParamInput             = sizeof(UINT32);

        m_getParamOutputArray[AWBGetParamOutputTypeGains].getParamOutputType          = AWBGetParamOutputTypeGains;
        m_getParamOutputArray[AWBGetParamOutputTypeGains].pGetParamOutput             = &m_outputs.gains;
        m_getParamOutputArray[AWBGetParamOutputTypeGains].sizeOfGetParamOutput        = sizeof(AWBAlgoGains);
        m_getParamOutputArray[AWBGetParamOutputTypeGains].sizeOfWrittenGetParamOutput = 0;
    }



}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::GetAlgoSetParamInput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::GetAlgoSetParamInput(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AWBAlgoSetParamList*           pInputList)
{
    CamxResult result            = CamxResultSuccess;

    pInputList->inputParamsCount = 0;
    pInputList->pAWBSetParams    = m_setParamInputArray;

    // AWB set debug data mode
    m_setInputs.debugDataMode = 0; // Hardcode now as debug data mode is not defined yet
    LoadSetInputParamList(pInputList,
                          AWBSetParamTypeInvalid,
                          static_cast<const VOID*>(&m_setInputs.debugDataMode),
                          sizeof(INT32));

    // AWB set Chromatix data
    if (NULL != pStatsProcessRequestDataInfo->pTuningModeData)
    {
        m_setInputs.statsTuningData.pTuningSetManager    = static_cast<VOID*>(m_pTuningManager->GetChromatix());
        m_setInputs.statsTuningData.pTuningModeSelectors =
            reinterpret_cast<TuningMode*>(&pStatsProcessRequestDataInfo->pTuningModeData->TuningMode[0]);
        m_setInputs.statsTuningData.numSelectors =
            pStatsProcessRequestDataInfo->pTuningModeData->noOfSelectionParameter;
        CAMX_LOG_VERBOSE(CamxLogGroupAWB,
            "Tuning data as mode: %d usecase %d  feature1 %d feature2 %d scene %d, effect %d,",
            pStatsProcessRequestDataInfo->pTuningModeData->TuningMode[0].mode,
            pStatsProcessRequestDataInfo->pTuningModeData->TuningMode[2].subMode.usecase,
            pStatsProcessRequestDataInfo->pTuningModeData->TuningMode[3].subMode.feature1,
            pStatsProcessRequestDataInfo->pTuningModeData->TuningMode[4].subMode.feature2,
            pStatsProcessRequestDataInfo->pTuningModeData->TuningMode[5].subMode.scene,
            pStatsProcessRequestDataInfo->pTuningModeData->TuningMode[6].subMode.effect);
        LoadSetInputParamList(pInputList,
                              AWBSetParamTypeChromatixData,
                              static_cast<const VOID*>(&m_setInputs.statsTuningData),
                              sizeof(StatsTuningData));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "Failed to set tuning data. pTuningModeData is NULL");
        result = CamxResultEFailed;
    }

    if (CamxResultSuccess == result)
    {
        // AWB set camera infomation
        LoadSetInputParamList(pInputList,
                              AWBSetParamTypeCameraInfo,
                              static_cast<const VOID*>(&m_inputs.cameraInfo),
                              sizeof(StatsCameraInfo));

        // AWB set current request id infomation
        LoadSetInputParamList(pInputList,
                              AWBSetParamTypeFrameId,
                              static_cast<const VOID*>(&pStatsProcessRequestDataInfo->requestId),
                              sizeof(pStatsProcessRequestDataInfo->requestId));

        // AWB set sensor info
        RetrieveSensorInfo(pStatsProcessRequestDataInfo, pInputList);

        // AWB set stats window info
        RetrieveStatsWindowInfo(pStatsProcessRequestDataInfo, pInputList);

        // AWB set calibration data
        RetrieveCalibrationData(pStatsProcessRequestDataInfo, pInputList);

        // AWB set white balance mode
        RetrieveAWBMode(pStatsProcessRequestDataInfo, pInputList);

        // AWB set lock
        RetrieveAWBLockInfo(pStatsProcessRequestDataInfo, pInputList);

        // AWB set scene mode
        RetrieveSceneMode(pStatsProcessRequestDataInfo, pInputList);

        // AWB set face and touch ROI
        RetrieveROIInfo(pStatsProcessRequestDataInfo, pInputList);

        // Read dynamic convergence speed from vendor tag
        ReadDynamicConvergenceSpeed(pStatsProcessRequestDataInfo, pInputList);

        // AWB set flash type and data
        RetrieveFlashInfo(pStatsProcessRequestDataInfo, pInputList);

        // AWB set AWB warm-start Gains and CCT only for first PCR
        if (FirstValidRequestId == pStatsProcessRequestDataInfo->requestId)
        {
            RetrieveWarmstartInfo(pStatsProcessRequestDataInfo, pInputList);
        }

        // AWB set ambient rate
        RetrieveExtensionTriggerInfo(pStatsProcessRequestDataInfo, pInputList);

        CAMX_LOG_VERBOSE(CamxLogGroupAWB,
            "AWB Input: Mode:%d Lock:%d SceneMode:%d faceCnt:%d, "
            "FlashState:%d FlashInfoType:%d LED: InfluenceRatio:%f R/G_Ratio:%f B/G_Ratio:%f bIsTorch %d"
            "Warm-start Gains(R:%f, G:%f, B:%f) CCT: %f dynamicConvergenceSpeed: %f",
            m_setInputs.algoMode,
            m_setInputs.lock,
            m_setInputs.sceneMode,
            m_setInputs.statsFaceInfo.faceCount,
            m_setInputs.flashInfo.flashState,
            m_setInputs.flashInfo.flashType,
            m_setInputs.flashInfo.LEDInfluenceRatio,
            m_setInputs.flashInfo.LEDRGRatio,
            m_setInputs.flashInfo.LEDBGRatio,
            m_setInputs.flashInfo.bIsTorch,
            m_setInputs.warmstartInfo.gains.red,
            m_setInputs.warmstartInfo.gains.green,
            m_setInputs.warmstartInfo.gains.blue,
            m_setInputs.warmstartInfo.correlatedColorTemperature,
            m_setInputs.dynamicConvergenceSpeed);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::GetFrameAndStatControl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::GetFrameAndStatControl(
    AWBFrameControl*         pFrameControl,
    AWBStatsControl*         pStatsControl)
{
    // Set control gains
    pFrameControl->AWBGains.rGain = m_outputs.gains.red;
    pFrameControl->AWBGains.gGain = m_outputs.gains.green;
    pFrameControl->AWBGains.bGain = m_outputs.gains.blue;

    // Set Color temperature
    pFrameControl->colorTemperature = m_outputs.cct;

    // set CCM values
    pFrameControl->numValidCCMs = m_outputs.CCMList.numValidCCMs;

    for (UINT32 ccmIndex = 0; ccmIndex < pFrameControl->numValidCCMs && ccmIndex < MaxCCMs; ccmIndex++)
    {
        pFrameControl->AWBCCM[ccmIndex].isCCMOverrideEnabled = m_outputs.CCMList.CCM[ccmIndex].isCCMOverrideEnabled;

        if (TRUE == pFrameControl->AWBCCM[ccmIndex].isCCMOverrideEnabled)
        {
            for (UINT i = 0; i < AWBNumCCMRows; i++)
            {
                for (UINT j = 0; j < AWBNumCCMCols; j++)
                {
                    pFrameControl->AWBCCM[ccmIndex].CCM[i][j] = m_outputs.CCMList.CCM[ccmIndex].CCM[i][j];
                }

                pFrameControl->AWBCCM[ccmIndex].CCMOffset[i] = m_outputs.CCMList.CCM[ccmIndex].CCMOffset[i];
            }
        }
    }

    // Set decision information
    pFrameControl->AWBDecision.decisionCount = m_outputs.currentDecision.decisionCount;
    for (UINT32 decisionIndex = 0; decisionIndex < pFrameControl->AWBDecision.decisionCount; decisionIndex++)
    {
        pFrameControl->AWBDecision.point[decisionIndex].rg = m_outputs.currentDecision.point[decisionIndex].rg;
        pFrameControl->AWBDecision.point[decisionIndex].bg = m_outputs.currentDecision.point[decisionIndex].bg;
        pFrameControl->AWBDecision.correlatedColorTemperature[decisionIndex] =
            m_outputs.currentDecision.correlatedColorTemperature[decisionIndex];
    }

    // Set stats configuration
    FillBGConfigurationData(&pStatsControl->statsConfig, &m_outputs.BGConfig);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::IsAECSettled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAWBIOUtil::IsAECSettled()
{
    BOOL aecSettled = FALSE;
    static const UINT StatsData[] =
    {
        PropertyIDAECFrameInfo,
    };

    static const UINT StatsLength = CAMX_ARRAY_SIZE(StatsData);
    VOID*             pData[StatsLength] = { 0 };
    pData[0] = m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypePropertyIDAECFrameInfo);

    if (NULL == pData[0])
    {
        CAMX_LOG_WARN(CamxLogGroupAWB, "PropertyIDAECFrameInfo is not published!");
    }
    else
    {
        aecSettled = reinterpret_cast<AECFrameInformation*>(pData[0])->AECSettled;
    }

    return aecSettled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::GetFlashFrameControl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::GetFlashFrameControl(
    AWBFrameControl*   pFlashFrameControl)
{
    if ((m_outputs.flashGains.red <= 0.0f) || (m_outputs.flashGains.green <= 0.0f) || (m_outputs.flashGains.blue <= 0.0f))
    {
        m_outputs.flashGains = m_outputs.gains;
        m_outputs.flashCCT   = m_outputs.cct;

        CAMX_LOG_WARN(CamxLogGroupAWB, "Invalid Algo Main Flash gain. Using preflash gain as main flash gain.");
    }

    // Set flash control gains
    pFlashFrameControl->AWBGains.rGain = m_outputs.flashGains.red;
    pFlashFrameControl->AWBGains.gGain = m_outputs.flashGains.green;
    pFlashFrameControl->AWBGains.bGain = m_outputs.flashGains.blue;

    // Set flash Color temperature
    pFlashFrameControl->colorTemperature = m_outputs.flashCCT;

    // set CCM values
    pFlashFrameControl->numValidCCMs = m_outputs.flashCCMList.numValidCCMs;

    for (UINT32 ccmIndex = 0; ccmIndex < pFlashFrameControl->numValidCCMs; ccmIndex++)
    {
        pFlashFrameControl->AWBCCM[ccmIndex].isCCMOverrideEnabled = m_outputs.flashCCMList.CCM[ccmIndex].isCCMOverrideEnabled;

        if (TRUE == pFlashFrameControl->AWBCCM[ccmIndex].isCCMOverrideEnabled)
        {
            for (UINT i = 0; i < AWBNumCCMRows; i++)
            {
                for (UINT j = 0; j < AWBNumCCMCols; j++)
                {
                    pFlashFrameControl->AWBCCM[ccmIndex].CCM[i][j] = m_outputs.flashCCMList.CCM[ccmIndex].CCM[i][j];
                }

                pFlashFrameControl->AWBCCM[ccmIndex].CCMOffset[i] = m_outputs.flashCCMList.CCM[ccmIndex].CCMOffset[i];
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::PrePublishMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::PrePublishMetadata()
{
    AWBFrameControl frameControl = {{0}};
    AWBStatsControl statsControl = {{{0}}};

    GetFrameAndStatControl(&frameControl, &statsControl);

    static const UINT WriteProps[] =
    {
        PropertyIDUsecaseAWBFrameControl,
        PropertyIDUsecaseAWBStatsControl
    };
    const VOID* pOutputData[CAMX_ARRAY_SIZE(WriteProps)] =
    {
        &frameControl,
        &statsControl
    };
    UINT pDataCount[CAMX_ARRAY_SIZE(WriteProps)]  =
    {
        sizeof(frameControl),
        sizeof(statsControl)
    };

    // Writing only usecase, so can write outside EPR
    m_pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
    CAMX_LOG_VERBOSE(CamxLogGroupAWB, "Writing to UsecasePool by AWB");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::PublishSkippedFrameOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::PublishSkippedFrameOutput() const
{
    CamxResult result                                       = CamxResultSuccess;
    static const UINT AWBPropertyList[]                     =
    {
        PropertyIDAWBFrameControl,
        PropertyIDAWBFrameInfo,
        PropertyIDAWBInternal,
        PropertyIDAWBStatsControl
    };
    static const UINT AWBPropertyListLength                 = CAMX_ARRAY_SIZE(AWBPropertyList);
    VOID*             pData[AWBPropertyListLength] = { 0 };

    pData[0] = m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypePropertyIDAWBFrameControl);
    pData[1] = m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypePropertyIDAWBFrameInfo);
    pData[2] = m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypePropertyIDAWBInternal);
    pData[3] = m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypePropertyIDAWBStatsControl);

    if ((NULL == pData[0]) || (NULL == pData[1]) || (NULL == pData[2]) || (NULL == pData[3]))
    {
        CAMX_LOG_WARN(CamxLogGroupAWB,
                       "PropertyIDAWBFrameControl(0x%x) PropertyIDAWBFrameInfo(0x%x) "
                       "PropertyIDAWBInternal(0x%x) PropertyIDAWBStatsControl(0x%x) is not published",
                        pData[0],
                        pData[1],
                        pData[2],
                        pData[3]);
        result = CamxResultEInvalidState;
    }
    else
    {
        const VOID* pOutputData[AWBPropertyListLength]  =
        {
            pData[0],
            pData[1],
            pData[2],
            pData[3]
        };
        UINT pDataSize[AWBPropertyListLength]           =
        {
            sizeof(AWBFrameControl),
            sizeof(AWBFrameInfo),
            sizeof(AWBOutputInternal),
            sizeof(AWBStatsControl)
        };

        result = m_pNode->WriteDataList(AWBPropertyList, pOutputData, pDataSize, AWBPropertyListLength);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::PublishMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::PublishMetadata(
    const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
    AWBAlgoOutputList*              pProcessOutput)
{
    CamxResult result = CamxResultSuccess;

    if (NULL == pStatsProcessRequestDataInfo || NULL == pProcessOutput)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "pStatsProcessRequestDataInfo: %p or pProcessOutput:%p is Null",
            pStatsProcessRequestDataInfo, pProcessOutput);
        return CamxResultEInvalidPointer;
    }

    result = PublishFrameControlToMainMetadata(pStatsProcessRequestDataInfo, &m_frameControl, &m_statsControl);
    DumpAWBStats();

    if (CamxResultSuccess == result)
    {
        PublishFrameInformationToMainMetadata(&m_frameInfo);
        DumpFrameInfo();
        if (TRUE == pStatsProcessRequestDataInfo->reportConverged)
        {
            m_outputs.state = AWBAlgoStateConverged;
        }
        result = PublishExternalCameraMetadata(pProcessOutput, &m_AWBHALData);
        DumpHALData();
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupAWB, "AWB Frame Control Metadata not published");
    }

    if (CamxResultSuccess == result)
    {
        PublishToInternalMetadata(&m_outputInternal);
        result = PublishPropertyDebugData();
    }
    m_pStatsAWBPropertyReadWrite->WriteProperties(NumAWBPropertyWriteTags);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::PublishExternalCameraMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::PublishExternalCameraMetadata(
    AWBAlgoOutputList*              pProcessOutput,
    AWBHALData*                     pAWBHALData)
{
    CamxResult        result = CamxResultSuccess;

    if (NULL == pAWBHALData || NULL == pProcessOutput)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "pAWBHALData: %p or pProcessOutput:%p is Null",
            pAWBHALData, pProcessOutput);
        return CamxResultEInvalidPointer;
    }

    BYTE              lockState                                = static_cast<BYTE>(m_outputs.lock);
    UINT8             mode                                     = static_cast<UINT8>(m_outputs.mode);
    UINT8             state                                    = static_cast<UINT8>(m_outputs.state);
    pAWBHALData->lock      = lockState;
    pAWBHALData->mode      = mode;
    pAWBHALData->state     = state;

    result = PublishVendorTagMetadata(pProcessOutput);
    CAMX_LOG_VERBOSE(CamxLogGroupAWB, "Published AWB Meta: Lock: %d Mode: %d state: %d",
                     lockState, mode, state);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAWBIOUtil::PublishVendorTagMetadata()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::PublishVendorTagMetadata(
    AWBAlgoOutputList*              pProcessOutput)
{

    CamxResult      result          = CamxResultSuccess;

    if (NULL == pProcessOutput)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "pProcessOutput is Null");
        return CamxResultEInvalidPointer;
    }

    if (NULL != pProcessOutput->pAWBOutputs && 0 != pProcessOutput->pAWBOutputs[AWBOutputTypeVendorTag].sizeOfAWBOutput)
    {
        StatsVendorTagList* pVendorTagOutput =
            static_cast<StatsVendorTagList*>(pProcessOutput->pAWBOutputs[AWBOutputTypeVendorTag].pAWBOutput);

        if (NULL == pVendorTagOutput)
        {
            CAMX_LOG_ERROR(CamxLogGroupAWB, "pVendorTagOutput is Null");
            return CamxResultEInvalidPointer;
        }

        for (UINT32 i = 0; i < pVendorTagOutput->vendorTagCount; i++)
        {
            if (0 == pVendorTagOutput->vendorTag[i].sizeOfWrittenData)
            {
                continue;
            }

            static const UINT WriteProps[]                             = {pVendorTagOutput->vendorTag[i].vendorTagId};
            const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)] = { pVendorTagOutput->vendorTag[i].pData };
            UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]  = { 1 };

            result = m_pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));

            CAMX_LOG_VERBOSE(CamxLogGroupAWB,
                             "Published VendorTag(%u) size(%d) pData(%p). Result = %s",
                             pVendorTagOutput->vendorTag[i].vendorTagId,
                             pVendorTagOutput->vendorTag[i].dataSize,
                             pVendorTagOutput->vendorTag[i].pData,
                             Utils::CamxResultToString(result));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::PublishFrameControlToMainMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::PublishFrameControlToMainMetadata(
    const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
    AWBFrameControl*                pFrameControl,
    AWBStatsControl*                pStatsControl)
{
    CamxResult      result          = CamxResultSuccess;

    if (NULL == pFrameControl || NULL == pStatsControl)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "pFrameControl: %p or pStatsControl:%p is Null",
            pFrameControl, pStatsControl);
        return CamxResultEInvalidPointer;
    }

    GetFrameAndStatControl(pFrameControl, pStatsControl);

    // Overwrite Frame control with flash gains for flash snapshot
    if ((AWBAlgoMainFlash == m_setInputs.flashInfo.flashState) ||
        (m_numberOfFramesToSkip > 0))
    {
        /* We need to skip "processing of stats" for number of frames after main flash */
        if (m_numberOfFramesToSkip == 0)
        {
            m_numberOfFramesToSkip = m_pStaticSettings->numberOfFramesToSkip;
        }
        else
        {
            m_numberOfFramesToSkip--;
        }
        GetFlashFrameControl(pFrameControl);
        CAMX_LOG_INFO(CamxLogGroupAWB,
                      "Main Flash Snapshot is triggered for reqID: %llu. Gain(R:%f, G:%f, B:%f) CCT(%d)",
                      pStatsProcessRequestDataInfo->requestId,
                      pFrameControl->AWBGains.rGain,
                      pFrameControl->AWBGains.gGain,
                      pFrameControl->AWBGains.bGain,
                      pFrameControl->colorTemperature);
    }
    PartialMWBOverride(pStatsProcessRequestDataInfo, pFrameControl);

    UINT32 awbTag               = 0;
    UINT32 awbRGainTag          = 0;
    UINT32 awbGGainTag          = 0;
    UINT32 awbBGainTag          = 0;
    UINT32 awbCCTTag            = 0;
    UINT32 awbDecisionTag       = 0;

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.statsconfigs", "AWBFrameControl", &awbTag);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "QueryVendorTagLocation for AWBFrameControl Failed");
    }

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.statsconfigs", "AWBFrameControlRGain", &awbRGainTag);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "QueryVendorTagLocation for AWBFrameControlRGain Failed");
    }

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.statsconfigs", "AWBFrameControlGGain", &awbGGainTag);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "QueryVendorTagLocation for AWBFrameControlGGain Failed");
    }

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.statsconfigs", "AWBFrameControlBGain", &awbBGainTag);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "QueryVendorTagLocation for AWBFrameControlBGain Failed");
    }

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.statsconfigs", "AWBFrameControlCCT", &awbCCTTag);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "QueryVendorTagLocation for AWBFrameControlCCT Failed");
    }

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.statsconfigs", "AWBDecisionAfterTC", &awbDecisionTag);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "QueryVendorTagLocation for AWBDecisionAfterTC Failed");
    }

    static const UINT WriteProps[] =
    {
        awbTag,
        awbRGainTag,
        awbGGainTag,
        awbBGainTag,
        awbCCTTag,
        awbDecisionTag,
    };

    const VOID* pOutputData[CAMX_ARRAY_SIZE(WriteProps)] =
    {
        pFrameControl,
        &pFrameControl->AWBGains.rGain,
        &pFrameControl->AWBGains.gGain,
        &pFrameControl->AWBGains.bGain,
        &pFrameControl->colorTemperature,
        &pFrameControl->AWBDecision.point[1],
    };

    UINT pDataCount[CAMX_ARRAY_SIZE(WriteProps)] =
    {
        sizeof(AWBFrameControl),
        1,
        1,
        1,
        1,
        2,
    };
    UINT length = CAMX_ARRAY_SIZE(WriteProps);

    // Write and publish the AWB properties
    result = m_pNode->WriteDataList(WriteProps, pOutputData, pDataCount, length);

    CAMX_LOG_INFO(CamxLogGroupAWB,
                     "Published PropertyIDAWBFrameControl for reqID: %llu camId:%d"
                     " Gain(R:%f, G:%f, B:%f) CCT(%d),"
                     " Decision_AfterTC:(R/G:%f B/G:%f)"
                     " FlashState: %d",
                     pStatsProcessRequestDataInfo->requestId,
                     pStatsProcessRequestDataInfo->cameraInfo.cameraId,
                     pFrameControl->AWBGains.rGain,
                     pFrameControl->AWBGains.gGain,
                     pFrameControl->AWBGains.bGain,
                     pFrameControl->colorTemperature,
                     pFrameControl->AWBDecision.point[1].rg,
                     pFrameControl->AWBDecision.point[1].bg,
                     m_setInputs.flashInfo.flashState);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::PublishFrameInformationToMainMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::PublishFrameInformationToMainMetadata(
    AWBFrameInfo*    pFrameInfo)
{
    if (NULL == pFrameInfo)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "pFrameInfo is Null");
        return;
    }

    // Set information gains
    pFrameInfo->AWBGains.rGain = m_outputs.gains.red;
    pFrameInfo->AWBGains.gGain = m_outputs.gains.green;
    pFrameInfo->AWBGains.bGain = m_outputs.gains.blue;

    // Set pool color temperature
    pFrameInfo->colorTemperature = m_outputs.cct;


}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::PublishToInternalMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::PublishToInternalMetadata(
    AWBOutputInternal*  pOutputInternal)
{
    // Set flash estimation status
    pOutputInternal->flashEstimationStatus = static_cast<AWBFlashEstimationState>(m_algoFlashEstimationState);

    // Set illuminant decision
    pOutputInternal->AWBDecision = static_cast<AWBIlluminant>(m_outputs.illuminantType);

    for (UINT32 sampleIndex = 0; sampleIndex < AWBDecisionMapSize; sampleIndex++)
    {
        pOutputInternal->AWBSampleDecision[sampleIndex] = static_cast<UINT8>(m_outputs.sampleDecision[sampleIndex]);
    }


}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::PublishPropertyDebugData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::PublishPropertyDebugData()
{
    CamxResult          result                      = CamxResultSuccess;
    UINT32              metaTag                     = 0;

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.debugdata", "DebugDataAll", &metaTag);
    if (CamxResultSuccess == result)
    {
        const UINT  DebugDataVendorTag[]    = { metaTag };
        const VOID* pDstData[1]             =
        { m_pStatsAWBPropertyReadWrite->GetPropertyData(AWBReadTypePropertyIDDebugDataAll) };
        UINT        pDataCount[1]           = { sizeof(DebugData) };
        result = m_pNode->WriteDataList(DebugDataVendorTag, pDstData, pDataCount, CAMX_ARRAY_SIZE(DebugDataVendorTag));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::AllocateMemoryVendorTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::AllocateMemoryVendorTag(
    AWBAlgoGetParam* pAlgoGetParam)
{
    CAMX_ENTRYEXIT(CamxLogGroupAWB);

    if (NULL == pAlgoGetParam)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "pAlgoGetParam is Null");
        return CamxResultEInvalidPointer;
    }

    CamxResult              result          = CamxResultSuccess;
    AWBAlgoGetParamOutput*  pGetParamOutput =
        &pAlgoGetParam->outputInfoList.pGetParamOutputs[AWBGetParamOutputTypePublishingVendorTagsInfo];

    if (pGetParamOutput->sizeOfWrittenGetParamOutput > 0)
    {
        StatsVendorTagInfoList* pVendorTagInfoList = static_cast<StatsVendorTagInfoList*>(pGetParamOutput->pGetParamOutput);

        for (UINT32 j = 0; j < pVendorTagInfoList->vendorTagCount; j++)
        {
            SIZE_T size = HAL3MetadataUtil::GetMaxSizeByTag(pVendorTagInfoList->vendorTag[j].vendorTagId);
            if (0 != size)
            {
                m_outputs.vendorTagList.vendorTag[j].vendorTagId        = pVendorTagInfoList->vendorTag[j].vendorTagId;
                m_outputs.vendorTagList.vendorTag[j].appliedDelay       = pVendorTagInfoList->vendorTag[j].appliedDelay;
                m_outputs.vendorTagList.vendorTag[j].pData              = CAMX_CALLOC(size);
                m_outputs.vendorTagList.vendorTag[j].sizeOfWrittenData  = 0;

                if (NULL == m_outputs.vendorTagList.vendorTag[j].pData)
                {
                    CAMX_LOG_ERROR(CamxLogGroupAWB,
                        "Allocating memory for vendor tagID[%d]: %d failed.",
                        j,
                        pVendorTagInfoList->vendorTag[j].vendorTagId);
                    result = CamxResultENoMemory;
                }
                else
                {
                    m_outputs.vendorTagList.vendorTag[j].dataSize = static_cast<UINT32>(size);
                    m_outputs.vendorTagList.vendorTagCount++;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::GetDefaultBGConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::GetDefaultBGConfig(
    StatsBayerGridBayerExposureConfig* pAWBBGConfigData
    ) const
{
    CamxResult result                       = CamxResultSuccess;

    pAWBBGConfigData->horizontalRegionCount                     = BGConfigHorizontalRegions;
    pAWBBGConfigData->verticalRegionCount                       = BGConfigVerticalRegions;

    UINT32 bitShift = IFEPipelineBitWidth - BGStatsConsumpBitWidth;
    UINT32 satThreshold = BGConfigSaturationThreshold << bitShift;

    pAWBBGConfigData->channelGainThreshold[StatsColorChannelR] = satThreshold - 1;
    pAWBBGConfigData->channelGainThreshold[StatsColorChannelGR] = satThreshold - 1;
    pAWBBGConfigData->channelGainThreshold[StatsColorChannelGB] = satThreshold - 1;
    pAWBBGConfigData->channelGainThreshold[StatsColorChannelB] = satThreshold - 1;

    // Configuring stats to maximum resolution
    pAWBBGConfigData->ROI.left   = 0;
    pAWBBGConfigData->ROI.top    = 0;
    pAWBBGConfigData->ROI.width  = m_setInputs.statsSensorInfo.sensorResWidth;
    pAWBBGConfigData->ROI.height = m_setInputs.statsSensorInfo.sensorResHeight;

    CAMX_LOG_VERBOSE(CamxLogGroupAWB, "Stats ROI(l:%d t:%d w:%d h:%d)",
                     pAWBBGConfigData->ROI.left,
                     pAWBBGConfigData->ROI.top,
                     pAWBBGConfigData->ROI.width,
                     pAWBBGConfigData->ROI.height);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::InvalidateIO
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::InvalidateIO()
{
    UINT32 i = 0;

    for (i = 0; i < AWBInputTypeLastIndex; i++)
    {
        m_processInputArray[i].inputType = AWBInputTypeInvalid;
    }

    for (i = 0; i < AWBOutputTypeLastIndex; i++)
    {
        m_processOutputArray[i].outputType = AWBOutputTypeInvalid;
    }

    for (i = 0; i < AWBGetParamInputTypeLastIndex; i++)
    {
        m_getParamInputArray[i].getParamInputType = AWBGetParamInputTypeInvalid;
    }

    for (i = 0; i < AWBGetParamOutputTypeLastIndex; i++)
    {
        m_getParamOutputArray[i].getParamOutputType = AWBGetParamOutputTypeInvalid;
    }

    for (i = 0; i < AWBSetParamTypeLastIndex; i++)
    {
        m_setParamInputArray[i].setParamType = AWBSetParamTypeInvalid;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::FillBGConfigurationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::FillBGConfigurationData(
    AWBConfig*                                  pAWBConfigData,
    const StatsBayerGridBayerExposureConfig*    pAWBAlgoConfig)
{
    if (NULL == pAWBConfigData || NULL == pAWBAlgoConfig)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "pAWBConfigData: %p or pProcessOutput:%p is Null",
            pAWBConfigData, pAWBAlgoConfig);
        return;
    }

    pAWBConfigData->BGConfig.horizontalNum                        = pAWBAlgoConfig->horizontalRegionCount;
    pAWBConfigData->BGConfig.verticalNum                          = pAWBAlgoConfig->verticalRegionCount;
    pAWBConfigData->BGConfig.channelGainThreshold[ChannelIndexR]  = pAWBAlgoConfig->channelGainThreshold[StatsColorChannelR];
    pAWBConfigData->BGConfig.channelGainThreshold[ChannelIndexGR] = pAWBAlgoConfig->channelGainThreshold[StatsColorChannelGR];
    pAWBConfigData->BGConfig.channelGainThreshold[ChannelIndexGB] = pAWBAlgoConfig->channelGainThreshold[StatsColorChannelGB];
    pAWBConfigData->BGConfig.channelGainThreshold[ChannelIndexB]  = pAWBAlgoConfig->channelGainThreshold[StatsColorChannelB];
    pAWBConfigData->BGConfig.outputBitDepth                       = pAWBAlgoConfig->outputBitDepth;
    if (TRUE == pAWBAlgoConfig->enableSaturationStats)
    {
        pAWBConfigData->BGConfig.outputMode = BGBESaturationEnabled;
    }
    else if (TRUE == pAWBAlgoConfig->enableYStatsComputation)
    {
        pAWBConfigData->BGConfig.outputMode = BGBEYStatsEnabled;
    }
    else
    {
        pAWBConfigData->BGConfig.outputMode = BGBERegular;
    }
    pAWBConfigData->BGConfig.enableQuadSync                       = pAWBAlgoConfig->enableQuadSync;


    if (StatsROISelectionFullFOV == pAWBAlgoConfig->ROISelection)
    {
        pAWBConfigData->BGConfig.ROI.left   = 0;
        pAWBConfigData->BGConfig.ROI.top    = 0;
        pAWBConfigData->BGConfig.ROI.width  = m_setInputs.statsSensorInfo.sensorResWidth;
        pAWBConfigData->BGConfig.ROI.height = m_setInputs.statsSensorInfo.sensorResHeight;
    }
    else if (StatsROISelectionCustom == pAWBAlgoConfig->ROISelection)
    {
        if (pAWBAlgoConfig->ROI.left + pAWBAlgoConfig->ROI.width >
            m_setInputs.statsSensorInfo.sensorResWidth ||
            pAWBAlgoConfig->ROI.top + pAWBAlgoConfig->ROI.height >
            m_setInputs.statsSensorInfo.sensorResHeight ||
            (pAWBAlgoConfig->ROI.width == 0) || (pAWBAlgoConfig->ROI.height == 0))
        {
            pAWBConfigData->BGConfig.ROI.left = 0;
            pAWBConfigData->BGConfig.ROI.top = 0;
            pAWBConfigData->BGConfig.ROI.width = m_setInputs.statsSensorInfo.sensorResWidth;
            pAWBConfigData->BGConfig.ROI.height = m_setInputs.statsSensorInfo.sensorResHeight;
        }
        else
        {
            pAWBConfigData->BGConfig.ROI.left   = pAWBAlgoConfig->ROI.left;
            pAWBConfigData->BGConfig.ROI.top    = pAWBAlgoConfig->ROI.top;
            pAWBConfigData->BGConfig.ROI.width  = pAWBAlgoConfig->ROI.width;
            pAWBConfigData->BGConfig.ROI.height = pAWBAlgoConfig->ROI.height;
        }
    }
    else if (StatsROISelectionCroppedFOV == pAWBAlgoConfig->ROISelection)
    {
        if (m_setInputs.statsWindowInfo.cropWindow.left + m_setInputs.statsWindowInfo.cropWindow.width >
            m_setInputs.statsSensorInfo.sensorResWidth ||
            m_setInputs.statsWindowInfo.cropWindow.top + m_setInputs.statsWindowInfo.cropWindow.height >
            m_setInputs.statsSensorInfo.sensorResHeight ||
            (m_setInputs.statsWindowInfo.cropWindow.width == 0) || (m_setInputs.statsWindowInfo.cropWindow.height == 0))
        {
            CAMX_LOG_INFO(CamxLogGroupAWB, "Invalid crop Window (%d %d %d %d) lies outside of (0 0 %d %d)",
                m_setInputs.statsWindowInfo.cropWindow.left,
                m_setInputs.statsWindowInfo.cropWindow.top,
                m_setInputs.statsWindowInfo.cropWindow.width,
                m_setInputs.statsWindowInfo.cropWindow.height,
                m_setInputs.statsSensorInfo.sensorResWidth,
                m_setInputs.statsSensorInfo.sensorResHeight);
            // Even if wrong crop window is recieved do not overwrite input, because input is used in metadata
            // if wrong crop window configuring stats Window to IFE input resolution.
            pAWBConfigData->BGConfig.ROI.left = 0;
            pAWBConfigData->BGConfig.ROI.top = 0;
            pAWBConfigData->BGConfig.ROI.width = m_setInputs.statsSensorInfo.sensorResWidth;
            pAWBConfigData->BGConfig.ROI.height = m_setInputs.statsSensorInfo.sensorResHeight;
        }
        else
        {
            pAWBConfigData->BGConfig.ROI.left   = m_setInputs.statsWindowInfo.cropWindow.left;
            pAWBConfigData->BGConfig.ROI.top    = m_setInputs.statsWindowInfo.cropWindow.top;
            pAWBConfigData->BGConfig.ROI.width  = m_setInputs.statsWindowInfo.cropWindow.width;
            pAWBConfigData->BGConfig.ROI.height = m_setInputs.statsWindowInfo.cropWindow.height;
        }
    }

    if ((CSLPresilEnabled == GetCSLMode()) || (CSLPresilRUMIEnabled == GetCSLMode()))
    {
        pAWBConfigData->BGConfig.ROI.left   = 0;
        pAWBConfigData->BGConfig.ROI.top    = 0;
        pAWBConfigData->BGConfig.ROI.width  = m_pStaticSettings->IFETestImageSizeWidth;
        pAWBConfigData->BGConfig.ROI.height = m_pStaticSettings->IFETestImageSizeHeight;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupAWB, "BG Stats Config reqId:%lld HNUM:%d VNUM:%d CH_GainThr[R:%d GR:%d GB:%d B:%d] "
                     "O/P BitDepth:%d O/P Mode:%d ROI Selection:%d ROI:(l:%d t:%d w:%d h:%d) enableQuadSync:%d",
                     m_inputs.requestNumber,
                     pAWBConfigData->BGConfig.horizontalNum,
                     pAWBConfigData->BGConfig.verticalNum,
                     pAWBConfigData->BGConfig.channelGainThreshold[ChannelIndexR],
                     pAWBConfigData->BGConfig.channelGainThreshold[ChannelIndexGR],
                     pAWBConfigData->BGConfig.channelGainThreshold[ChannelIndexGB],
                     pAWBConfigData->BGConfig.channelGainThreshold[ChannelIndexB],
                     pAWBConfigData->BGConfig.outputBitDepth,
                     pAWBConfigData->BGConfig.outputMode,
                     pAWBAlgoConfig->ROISelection,
                     pAWBConfigData->BGConfig.ROI.left,
                     pAWBConfigData->BGConfig.ROI.top,
                     pAWBConfigData->BGConfig.ROI.width,
                     pAWBConfigData->BGConfig.ROI.height,
                     pAWBConfigData->BGConfig.enableQuadSync);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::FillIlluminantCalibrationFactor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::FillIlluminantCalibrationFactor(
    const WBCalibrationData*                pWBCalibrationData,
    AWBAlgoIlluminantsCalibrationFactor*    pIlluminantsCalibrationFactor)
{
    if (NULL == pWBCalibrationData)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "pWBCalibrationData is Null");
        return;
    }

    UINT32 colorTemperature     = AWBAlgoColorTemperateD50;
    UINT32 calibrationDataCount = 0;

    for (UINT32 i = 0; i < MaxLightTypes; i++)
    {
        if (pWBCalibrationData[i].isAvailable)
        {
            switch (pWBCalibrationData[i].illuminant)
            {
                case EEPROMIlluminantType::D65:
                    colorTemperature = AWBAlgoColorTemperateD65;
                    break;
                case EEPROMIlluminantType::TL84:
                    colorTemperature = AWBAlgoColorTemperateTL84;
                    break;
                case EEPROMIlluminantType::A:
                    colorTemperature = AWBAlgoColorTemperateIncandescent;
                    break;
                case EEPROMIlluminantType::D50:
                    colorTemperature = AWBAlgoColorTemperateD50;
                    break;
                case EEPROMIlluminantType::H:
                    colorTemperature = AWBAlgoColorTemperateHorizon;
                    break;
                default:
                    break;
            }
            pIlluminantsCalibrationFactor->illuminant[calibrationDataCount].isAvailable = TRUE;
            pIlluminantsCalibrationFactor->illuminant[calibrationDataCount].correlatedColorTemperature = colorTemperature;
            pIlluminantsCalibrationFactor->illuminant[calibrationDataCount].ratioRG = pWBCalibrationData[i].rOverG;
            pIlluminantsCalibrationFactor->illuminant[calibrationDataCount].ratioBG = pWBCalibrationData[i].bOverG;
            CAMX_LOG_VERBOSE(CamxLogGroupAWB, "colorTemperature: %d  ratioRG: %f  ratioBG: %f",
                             colorTemperature, pWBCalibrationData[i].rOverG, pWBCalibrationData[i].bOverG);
            calibrationDataCount++;
        }
    }

    pIlluminantsCalibrationFactor->numberOfIlluminants = calibrationDataCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::~CAWBIOUtil
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAWBIOUtil::~CAWBIOUtil()
{
    // Destroy all the created objects.
    // free memory allocated for holding vendor tag data
    for (UINT32 i = 0; i < m_outputs.vendorTagList.vendorTagCount; i++)
    {
        if (NULL != m_outputs.vendorTagList.vendorTag[i].pData)
        {
            CAMX_FREE(m_outputs.vendorTagList.vendorTag[i].pData);
            m_outputs.vendorTagList.vendorTag[i].pData = NULL;
        }
    }

    if (NULL != m_pStatsAWBPropertyReadWrite)
    {
        CAMX_DELETE m_pStatsAWBPropertyReadWrite;
        m_pStatsAWBPropertyReadWrite = NULL;
    }

    if (NULL != m_pTOFInterfaceObject)
    {
        m_pTOFInterfaceObject->Destroy();
        m_pTOFInterfaceObject = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::SetAlgoChromatix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::SetAlgoChromatix(
    ChiTuningModeParameter* pInputTuningModeData,
    AWBAlgoSetParamList*    pAlgoSetParamList)
{
    CamxResult              result = CamxResultSuccess;

    if (NULL != pInputTuningModeData)
    {
        m_setInputs.statsTuningData = { 0 };
        m_setInputs.statsTuningData.pTuningSetManager       = reinterpret_cast<VOID*>(m_pTuningManager->GetChromatix());
        m_setInputs.statsTuningData.pTuningModeSelectors    =
            reinterpret_cast<TuningMode*>(&pInputTuningModeData->TuningMode[0]);
        m_setInputs.statsTuningData.numSelectors            = pInputTuningModeData->noOfSelectionParameter;
        CAMX_LOG_VERBOSE(CamxLogGroupAWB,
                         "Tuning data as mode: %d usecase %d  feature1 %d feature2 %d scene %d, effect %d,",
                         pInputTuningModeData->TuningMode[0].mode,
                         pInputTuningModeData->TuningMode[2].subMode.usecase,
                         pInputTuningModeData->TuningMode[3].subMode.feature1,
                         pInputTuningModeData->TuningMode[4].subMode.feature2,
                         pInputTuningModeData->TuningMode[5].subMode.scene,
                         pInputTuningModeData->TuningMode[6].subMode.effect);

        pAlgoSetParamList->pAWBSetParams    = m_setParamInputArray;
        pAlgoSetParamList->inputParamsCount = 0;

        // AWB set tuning information
        LoadSetInputParamList(pAlgoSetParamList,
                              AWBSetParamTypeChromatixData,
                              static_cast<VOID*>(&m_setInputs.statsTuningData),
                              sizeof(StatsTuningData));

        // AWB set camera information
        LoadSetInputParamList(pAlgoSetParamList,
                              AWBSetParamTypeCameraInfo,
                              static_cast<VOID*>(&m_inputs.cameraInfo),
                              sizeof(StatsCameraInfo));

    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "Input tuning data is NULL pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::ReadAWBProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::ReadAWBProperties(
    UINT64 requestIdOffsetFromLastFlush)
{
    m_pStatsAWBPropertyReadWrite->GetReadProperties(AWBReadTypePropertyIDCount, requestIdOffsetFromLastFlush);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::WriteAWBProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::WriteAWBProperties()
{
    m_pStatsAWBPropertyReadWrite->WriteProperties(NumAWBPropertyWriteTags);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::ReadVendorTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::ReadVendorTag(
    const CHAR* pSectionName,
    const CHAR* pTagName,
    VOID**      ppArg)
{
    return StatsUtil::ReadVendorTag(m_pNode, pSectionName, pTagName, ppArg);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::GetPartialMWBMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PartialMWBMode CAWBIOUtil::GetPartialMWBMode()
{
    CamxResult result;
    PartialMWBMode* pMode = NULL;

    result = ReadVendorTag("org.codeaurora.qcamera3.manualWB", "partial_mwb_mode", reinterpret_cast<VOID**>(&pMode));
    if (result != CamxResultSuccess)
    {
        return PartialMWBMode::Disable;
    }
    else
    {
        return *pMode;
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::PartialMWBOverride
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::PartialMWBOverride(
    const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
    AWBFrameControl*                pFrameControl)
{
    PartialMWBMode mode;
    CamxResult result;

    mode = GetPartialMWBMode();
    CAMX_LOG_VERBOSE(CamxLogGroupAWB, "Selected Mode: %d", mode);
    if (PartialMWBMode::CCT == mode)
    {
        UINT32* pCCT = NULL;
        result = ReadVendorTag("org.codeaurora.qcamera3.manualWB", "color_temperature", reinterpret_cast<VOID**>(&pCCT));
        if (result == CamxResultSuccess && *pCCT > 0)
        {
            result = GetGainsFromCCT(pStatsProcessRequestDataInfo, &pFrameControl->AWBGains, *pCCT);
            pFrameControl->colorTemperature = *pCCT;
        }
    }
    else if (PartialMWBMode::Gains == mode)
    {
        FLOAT* pGains = NULL;
        result = ReadVendorTag("org.codeaurora.qcamera3.manualWB", "gains", reinterpret_cast<VOID**>(&pGains));
        if (CamxResultSuccess == result && pGains[0] > 0.0 && pGains[1] > 0.0 && pGains[2] > 0.0)
        {
            result = GetCCTFromGains(pStatsProcessRequestDataInfo, &pFrameControl->colorTemperature, pGains);
            pFrameControl->AWBGains.rGain = pGains[0];
            pFrameControl->AWBGains.gGain = pGains[1];
            pFrameControl->AWBGains.bGain = pGains[2];
        }

    }
    /* else do nothing */
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::SetAwbAlgo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::SetAwbAlgo(
    CHIAWBAlgorithm* pAlgo)
{
    m_pAWBAlgorithm = pAlgo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::GetGainsFromCCT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::GetGainsFromCCT(
    const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
    AWBGainParams* pGains,
    UINT32         CCT)
{
    AWBAlgoGetParam algoGetParam = {};
    CamxResult result = CamxResultSuccess;

    if (NULL == m_pAWBAlgorithm)
    {
        return CamxResultEFailed;
    }

    algoGetParam.type = AWBGetParamTypeCctToGain;
    m_inputs.cct = CCT;
    GetAlgoGetParamInputOutput(pStatsProcessRequestDataInfo, &algoGetParam);

    if (CamxResultSuccess == result)
    {
        result = m_pAWBAlgorithm->AWBGetParam(m_pAWBAlgorithm, &algoGetParam);
    }

    if (CamxResultSuccess == result)
    {
        pGains->rGain = m_outputs.gains.red;
        pGains->gGain = m_outputs.gains.green;
        pGains->bGain = m_outputs.gains.blue;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::GetCCTFromGains
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBIOUtil::GetCCTFromGains(
    const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
    UINT32* pCCT,
    FLOAT* pGains)
{
    AWBAlgoGetParam algoGetParam = {};
    CamxResult result = CamxResultSuccess;

    if (NULL == m_pAWBAlgorithm)
    {
        return CamxResultEFailed;
    }

    algoGetParam.type = AWBGetParamTypeGainToCCT;
    m_inputs.gains.red   = pGains[0];
    m_inputs.gains.green = pGains[1];
    m_inputs.gains.blue  = pGains[2];
    GetAlgoGetParamInputOutput(pStatsProcessRequestDataInfo, &algoGetParam);


    result = m_pAWBAlgorithm->AWBGetParam(m_pAWBAlgorithm, &algoGetParam);


    if (CamxResultSuccess == result)
    {
        *pCCT = m_outputs.cct;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::UpdateTraceEvents
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::UpdateTraceEvents()
{
    if (m_lastAWBAlgoState != m_outputs.state)
    {
        if (AWBAlgoStateSearching == m_outputs.state)
        {
            CAMX_TRACE_ASYNC_BEGIN_F(CamxLogGroupAWB, 0, "AWB: Convergence");
        }
        else if (AWBAlgoStateConverged == m_outputs.state)
        {
            CAMX_TRACE_ASYNC_END_F(CamxLogGroupAWB, 0, "AWB: Convergence");
        }
        m_lastAWBAlgoState = m_outputs.state;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::DumpAWBStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::DumpAWBStats()
{
    UINT32 outputMask = HwEnvironment::GetInstance()->GetStaticSettings()->dumpAWB;

    if (outputMask & AWBStatsControlDump)
    {
        CAMX_LOG_VERBOSE(
            CamxLogGroupAWB,
            "[AWB STATS] BG & BE stats config: horizontal regions: %d, vertical regions: %d, "
            "O/P BitDepth: %d, ROI coordinates: left: %d, top:%d, width: %d, height: %d "
            "CH_GainThr[R:%d GR:%d GB:%d B:%d]",
            m_statsControl.statsConfig.BGConfig.horizontalNum,
            m_statsControl.statsConfig.BGConfig.verticalNum,
            m_statsControl.statsConfig.BGConfig.outputBitDepth,
            m_statsControl.statsConfig.BGConfig.ROI.left,
            m_statsControl.statsConfig.BGConfig.ROI.top,
            m_statsControl.statsConfig.BGConfig.ROI.width,
            m_statsControl.statsConfig.BGConfig.ROI.height,
            m_statsControl.statsConfig.BGConfig.channelGainThreshold[ChannelIndexR],
            m_statsControl.statsConfig.BGConfig.channelGainThreshold[ChannelIndexGR],
            m_statsControl.statsConfig.BGConfig.channelGainThreshold[ChannelIndexGB],
            m_statsControl.statsConfig.BGConfig.channelGainThreshold[ChannelIndexB]);

        CAMX_LOG_VERBOSE(
            CamxLogGroupAWB,
            "[AWB STATS] Stats O/P mode: %d, BGBEConfig output mode type: %d "
            "subgrid region: height: %d, width: %d",
            m_statsControl.statsConfig.BGConfig.outputMode,
            m_statsControl.statsConfig.BGConfig.greenType,
            m_statsControl.statsConfig.BGConfig.regionHeight,
            m_statsControl.statsConfig.BGConfig.regionWidth);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::DumpFrameInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::DumpFrameInfo()
{
    UINT32 outputMask = HwEnvironment::GetInstance()->GetStaticSettings()->dumpAWB;

    if (outputMask & AWBFrameInfoDump)
    {
        CAMX_LOG_VERBOSE(
            CamxLogGroupAWB,
            "[AWB FRAME INFO] AWB gains[rGain: %f, gGain: %f, bGain: %f], colorTemp: %d",
            m_frameInfo.AWBGains.rGain,
            m_frameInfo.AWBGains.gGain,
            m_frameInfo.AWBGains.bGain,
            m_frameInfo.colorTemperature);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBIOUtil::DumpHALData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBIOUtil::DumpHALData()
{
    UINT32 outputMask = HwEnvironment::GetInstance()->GetStaticSettings()->dumpAWB;

    if (outputMask & AWBHALDump)
    {
        CAMX_LOG_VERBOSE(
            CamxLogGroupAWB,
            "[AWB HAL] lock: %d, mode: %d, state: %d",
            m_AWBHALData.lock,
            m_AWBHALData.mode,
            m_AWBHALData.state);
    }
}


CAMX_NAMESPACE_END
