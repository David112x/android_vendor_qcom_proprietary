////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcafdiohandler.cpp
/// @brief This class provides utility functions to handle input and output of AF algorithm
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcafdiohandler.h"
#include "camxtitan17xdefs.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::CAFDIOHandler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAFDIOHandler::CAFDIOHandler()
{
    // Default Constructor: initialize parameters

    // Top level output structure
    m_outputList.outputCount    = AFDOutputTypeLastIndex;
    m_outputList.pAFDOutputList = m_outputArray;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::~CAFDIOHandler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAFDIOHandler::~CAFDIOHandler()
{
    if (NULL != m_pStatsAFDPropertyReadWrite)
    {
        CAMX_DELETE m_pStatsAFDPropertyReadWrite;
        m_pStatsAFDPropertyReadWrite = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDIOHandler::Initialize(
    const StatsInitializeData* pInitializeData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL == pInitializeData)
    {
        CAMX_LOG_ERROR(CamxLogGroupAFD, "StatsInitializeData is Invalid(NULL)");
        return CamxResultEInvalidArg;
    }

    m_pNode = pInitializeData->pNode;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::ReadRowSumStat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StatsRowSum* CAFDIOHandler::ReadRowSumStat()
{
    CamxResult   result  = CamxResultSuccess;
    StatsRowSum* pRSStat = NULL;

    result = ReadDataFromInternalPropertyPool(PropertyIDISPRSConfig, reinterpret_cast<VOID**>(&pRSStat));

    if (CamxResultSuccess == result)
    {
        result = ReadDataFromInternalPropertyPool(PropertyIDParsedRSStatsOutput, reinterpret_cast<VOID**>(&pRSStat));
    }

    if (CamxResultSuccess != result)
    {
        pRSStat = NULL;
    }

    return pRSStat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::ReadBGStat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StatsBayerGrid* CAFDIOHandler::ReadBGStat()
{
    CamxResult      result  = CamxResultSuccess;
    StatsBayerGrid* pBGStat = NULL;

    result = ReadDataFromInternalPropertyPool(PropertyIDISPHDRBEConfig, reinterpret_cast<VOID**>(&pBGStat));
    if (CamxResultSuccess == result)
    {
        result = ReadDataFromInternalPropertyPool(PropertyIDParsedHDRBEStatsOutput,
                                                  reinterpret_cast<VOID**>(&pBGStat));
    }
    if (CamxResultSuccess != result)
    {
        pBGStat = NULL;
    }

    return pBGStat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::PublishStatsControlToMainMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDIOHandler::PublishStatsControlToMainMetadata(
    const AFDAlgoOutputList* pOutput,
    const UINT64             requestId)
{
    AFDStatsControl control  = {};
    CamxResult      result   = CamxResultSuccess;
    UINT32          maxIndex = static_cast<UINT32>(CamX::Utils::MaxUINT32(AFDOutputTypeLastIndex, pOutput->outputCount));

    for (UINT32 i = 0; i < maxIndex; i++)
    {
        if (0 == pOutput->pAFDOutputList[i].sizeOfAFDOutput)
        {
            continue;
        }

        switch (pOutput->pAFDOutputList[i].outputType)
        {
            case AFDAlgoOutputRowSumConfig:
            {
                static const UINT WriteProps[]                             = { PropertyIDAFDStatsControl };
                const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)] = { &control };
                UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]  = { sizeof(control) };

                AFDAlgoRowSumConfig* pRSConfig = static_cast <AFDAlgoRowSumConfig *>(pOutput->pAFDOutputList[i].pAFDOutput);
                control.statsConfig.statsHNum                       = pRSConfig->statsHnum;
                control.statsConfig.statsVNum                       = pRSConfig->statsVnum;
                control.statsConfig.statsRSCSColorConversionEnable  = TRUE;
                CAMX_LOG_VERBOSE(CamxLogGroupAFD, "statsConfig RequestId=%llu Hnum:%u Vnum:%u",
                    requestId,
                    control.statsConfig.statsHNum,
                    control.statsConfig.statsVNum);

                result = m_pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
                break;
            }
            case AFDAlgoOutputFrameSetting:
                break;
            case AFDAlgoOutputVendorTag:
                break;
            default:
            {
                CAMX_LOG_ERROR(CamxLogGroupAFD, "Invalid output type %u!", pOutput->pAFDOutputList[i].outputType);
                break;
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::PublishFrameInfoToMainMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDIOHandler::PublishFrameInfoToMainMetadata(
    const AFDAlgoOutputList* pOutput,
    const UINT64             requestId)
{
    AFDFrameInfo info     = {};
    CamxResult   result   = CamxResultSuccess;
    UINT32       maxIndex = static_cast<UINT32>(CamX::Utils::MaxUINT32(AFDOutputTypeLastIndex, pOutput->outputCount));

    for (UINT32 i = 0; i < maxIndex; i++)
    {
        if (0 == pOutput->pAFDOutputList[i].sizeOfAFDOutput)
        {
            continue;
        }
        switch (pOutput->pAFDOutputList[i].outputType)
        {
            case AFDAlgoOutputFrameSetting:
            {
                static const UINT WriteProps[]                             = { PropertyIDAFDFrameInfo };
                const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)] = { &info };
                UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]  = { sizeof(info) };

                AFDAlgoFrameSetting* pFrameSetting =
                    static_cast <AFDAlgoFrameSetting *>(pOutput->pAFDOutputList[i].pAFDOutput);
                info.detectedAFDMode = pFrameSetting->currentAFDMode;
                CAMX_LOG_VERBOSE(CamxLogGroupAFD, "RequestId=%llu Detected AFD Mode - %d",
                    requestId, pFrameSetting->currentAFDMode);
                result = m_pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
                break;
            }
            case AFDAlgoOutputRowSumConfig:
                break;
            case AFDAlgoOutputVendorTag:
                break;
            default:
            {
                CAMX_LOG_ERROR(CamxLogGroupAFD, "Invalid output type!");
                break;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::PublishOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDIOHandler::PublishOutput(
    const AFDAlgoOutputList* pOutput,
    const UINT64 requestId)
{
    CamxResult result = CamxResultSuccess;

    result = PublishStatsControlToMainMetadata(pOutput, requestId);

    if (CamxResultSuccess == result)
    {
        result = PublishFrameInfoToMainMetadata(pOutput, requestId);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  CAFDIOHandler::GetAFDOutputBuffers()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AFDAlgoOutputList* CAFDIOHandler::GetAFDOutputBuffers()
{
    AFDAlgoOutputList* pOutputList = &m_outputList;

    return pOutputList;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::GetAlgoOutputBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAFDIOHandler::GetAlgoOutputBuffers(
    AFDAlgoOutputList*             pOutput)
{

    pOutput->outputCount = 0;

    pOutput->pAFDOutputList[AFDAlgoOutputFrameSetting].outputType      = AFDAlgoOutputFrameSetting;
    pOutput->pAFDOutputList[AFDAlgoOutputFrameSetting].pAFDOutput      = &m_frameSetting;
    pOutput->pAFDOutputList[AFDAlgoOutputFrameSetting].sizeOfAFDOutput = sizeof(AFDAlgoFrameSetting);
    pOutput->outputCount                                               = pOutput->outputCount +1;

    pOutput->pAFDOutputList[AFDAlgoOutputRowSumConfig].outputType      = AFDAlgoOutputRowSumConfig;
    pOutput->pAFDOutputList[AFDAlgoOutputRowSumConfig].pAFDOutput      = &m_rowSumConfig;
    pOutput->pAFDOutputList[AFDAlgoOutputRowSumConfig].sizeOfAFDOutput = sizeof(AFDAlgoRowSumConfig);
    pOutput->outputCount                                               = pOutput->outputCount + 1;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::ReadAECInput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AFDAECInfo* CAFDIOHandler::ReadAECInput()
{
    CamxResult  result = CamxResultSuccess;
    AFDAECInfo* pAECInfo = NULL;

    result = ReadDataFromMainPropertyPool( PropertyIDAECFrameInfo, reinterpret_cast<VOID**>(&pAECInfo));

    if (CamxResultSuccess != result)
    {
        pAECInfo = NULL;
    }

    return pAECInfo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::ReadAFInput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AFDAFInfo* CAFDIOHandler::ReadAFInput()
{
    CamxResult result  = CamxResultSuccess;
    AFDAFInfo* pAFInfo = NULL;

    result = ReadDataFromMainPropertyPool(PropertyIDAFFrameInfo, reinterpret_cast<VOID**>(&pAFInfo));

    if (CamxResultSuccess != result)
    {
        pAFInfo = NULL;
    }

    return pAFInfo;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::ReadSensorInput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AFDSensorInfo* CAFDIOHandler::ReadSensorInput()
{
    UsecaseSensorModes* pSensorModes    = NULL;
    SensorMode*         pSensorMode     = NULL;
    UINT32              index           = 0;
    CSIDBinningInfo     csidBinningInfo = { 0 };

    static const UINT GetProps[] =
    {
        PropertyIDUsecaseSensorCurrentMode,
        PropertyIDUsecaseSensorModes,
        SensorFrameDuration
    };
    static const UINT GetPropsLength          = CAMX_ARRAY_SIZE(GetProps);
    VOID*             pData[GetPropsLength]   = { 0 };
    UINT64            offsets[GetPropsLength] = { 0 };

    offsets[2] = m_pNode->GetMaximumPipelineDelay();

    m_pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);

    if (pData[0] != NULL)
    {
        index = *(reinterpret_cast<UINT32*>(pData[0]));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAFD, "unable to read PropertyIDUsecaseSensorCurrentMode index");
    }

    if (pData[1] != NULL)
    {
        pSensorModes = reinterpret_cast<UsecaseSensorModes*>(pData[1]);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAFD, "unable to read PropertyIDUsecaseSensorModes pSensorModes");
    }

    if (pSensorModes != NULL)
    {
        pSensorMode                             = &pSensorModes->allModes[index];
        m_sensorInfoInput.binningTypeH          = pSensorMode->binningTypeH;
        m_sensorInfoInput.binningTypeV          = pSensorMode->binningTypeV;
        m_sensorInfoInput.maxFPS                = static_cast<FLOAT>(pSensorMode->maxFPS);
        m_sensorInfoInput.outPixelClock         = pSensorMode->outPixelClock;
        m_sensorInfoInput.numPixelsPerLine      = pSensorMode->numPixelsPerLine;
        m_pNode->GetCSIDBinningInfo(&csidBinningInfo);
        if (TRUE == csidBinningInfo.isBinningEnabled)
        {
            m_sensorInfoInput.numLinesPerFrame >>= 1;
        }
        m_sensorInfoInput.sensorHeight          = pSensorMode->cropInfo.lastLine - pSensorMode->cropInfo.firstLine + 1;
        m_sensorInfoInput.sensorWidth           = pSensorMode->cropInfo.lastPixel - pSensorMode->cropInfo.firstPixel + 1;
        m_sensorInfoInput.numLinesPerFrame      = pSensorMode->numLinesPerFrame;
        m_sensorInfoInput.maxLineCount          = pSensorMode->maxLineCount;
        m_sensorInfoInput.minHorizontalBlanking = pSensorMode->minHorizontalBlanking;
        m_sensorInfoInput.minVerticalBlanking   = pSensorMode->minVerticalBlanking;
    }

    if (pData[2] != NULL)
    {
        UINT64 frameDuration     = *(reinterpret_cast<UINT64*>(pData[2]));
        m_sensorInfoInput.curFPS = static_cast<FLOAT>(NanoSecondsPerSecond) / static_cast<FLOAT>(frameDuration);
    }
    else
    {
        m_sensorInfoInput.curFPS = m_sensorInfoInput.maxFPS;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupAFD, "binning(H,V):(%d %d) maxFPS:%f maxLC:%d minHblanking:%d minVblanking:%d"
        "num.ofLinesPerframe:%d numPixelsPerLine:%d outPixelClock:%llu curFPS:%f",
        m_sensorInfoInput.binningTypeH,
        m_sensorInfoInput.binningTypeV,
        m_sensorInfoInput.maxFPS,
        m_sensorInfoInput.maxLineCount,
        m_sensorInfoInput.minHorizontalBlanking,
        m_sensorInfoInput.minVerticalBlanking,
        m_sensorInfoInput.numLinesPerFrame,
        m_sensorInfoInput.numPixelsPerLine,
        m_sensorInfoInput.outPixelClock,
        m_sensorInfoInput.curFPS);

    return (&m_sensorInfoInput);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::RetrieveAECFrameControlFromMainPool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDIOHandler::RetrieveAECFrameControlFromMainPool(
    VOID*       pVoidInput,
    AFDAECInfo* pAFDAECInfo)
{
    if (NULL == pAFDAECInfo)
    {
        CAMX_LOG_ERROR(CamxLogGroupAFD, "AEC info NULL pointer");
        return CamxResultEInvalidPointer;
    }

    AECFrameControl* pAECFrameControl = reinterpret_cast<AECFrameControl*>(pVoidInput);

    pAFDAECInfo->luxIDx               = static_cast<FLOAT>(pAECFrameControl->luxIndex);
    pAFDAECInfo->realGain             = pAECFrameControl->exposureInfo[ExposureIndexShort].linearGain;

    return CamxResultSuccess;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::ReadDataFromInternalPropertyPool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDIOHandler::ReadDataFromInternalPropertyPool(
    UINT32 propertyId,
    VOID** ppDataOut)
{
    CamxResult        result = CamxResultSuccess;

    switch (propertyId)
    {
        case PropertyIDISPHDRBEConfig:
        {
            const UINT GetProps[]              = { PropertyIDISPHDRBEConfig };
            const UINT GetPropsLength          = CAMX_ARRAY_SIZE(GetProps);
            VOID*      pData[GetPropsLength]   = { 0 };
            UINT64     offsets[GetPropsLength] = { m_pNode->GetMaximumPipelineDelay() };

            m_pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);
            if (NULL != pData[0])
            {
                result = RetrieveBGStatConfig(pData[0], &m_bayerGridStats);
                *ppDataOut = static_cast<VOID *>(&m_bayerGridStats);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupAFD, "unable to read PropertyIDISPHDRBEConfig BGStatsConfig");
                result = CamxResultENoSuch;
            }
            break;
        }
        case PropertyIDParsedHDRBEStatsOutput:
        {
            const UINT GetProps[]              = { PropertyIDParsedHDRBEStatsOutput };
            const UINT GetPropsLength          = CAMX_ARRAY_SIZE(GetProps);
            VOID*      pData[GetPropsLength]   = { 0 };
            UINT64     offsets[GetPropsLength] = { 0 };

            m_pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);
            if (NULL != pData[0])
            {
                result = RetrieveBGStatOutput(*static_cast<VOID**>(pData[0]), &m_bayerGridStats);
                *ppDataOut = static_cast<VOID *>(&m_bayerGridStats);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupAFD, "unable to read PropertyIDParsedHDRBEStatsOutput BGStatsConfig");
                result = CamxResultENoSuch;
            }
            break;
        }
        case PropertyIDParsedRSStatsOutput:
        {
            const UINT GetProps[]              = { PropertyIDParsedRSStatsOutput };
            const UINT GetPropsLength          = CAMX_ARRAY_SIZE(GetProps);
            VOID*      pData[GetPropsLength]   = { 0 };
            UINT64     offsets[GetPropsLength] = { 0 };

            m_pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);
            if (NULL != pData[0])
            {
                result = RetrieveRSStatsInfoFromInternalPool(*static_cast<VOID**>(pData[0]), &m_rowSumInfo);
                *ppDataOut = static_cast<VOID *>(&m_rowSumInfo);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupAFD, "PropertyIDParsedRSStatsOutput is NULL");
                result = CamxResultENoSuch;
            }
            break;
        }
        case PropertyIDISPRSConfig:
        {
            const UINT GetProps[]              = { PropertyIDISPRSConfig };
            const UINT GetPropsLength          = CAMX_ARRAY_SIZE(GetProps);
            VOID*      pData[GetPropsLength]   = { 0 };
            UINT       pipelineDelay           = (TRUE == m_pNode->IsCameraRunningOnBPS())
                ? 0 :  m_pNode->GetMaximumPipelineDelay();
            UINT64     offsets[GetPropsLength] = { pipelineDelay };

            m_pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);

            if (NULL != pData[0])
            {
                result = RetrieveRSStatsConfigFromInternalPool(pData[0], &m_rowSumInfo);

                *ppDataOut = static_cast<VOID *>(&m_rowSumInfo);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupAFD, "PropertyIDISPRSConfig is NULL");
                result = CamxResultENoSuch;
            }
            break;
        }
        default:
            CAMX_LOG_ERROR(CamxLogGroupAFD, "Unsupported PropertyId %d", propertyId);

            result = CamxResultENoSuch;
            break;
    }


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::RetrieveRSStatsConfigFromInternalPool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDIOHandler::RetrieveRSStatsConfigFromInternalPool(
    VOID*        pRSConfig,
    StatsRowSum* pRowSumConfig)
{
    if (NULL == pRSConfig)
    {
        CAMX_LOG_ERROR(CamxLogGroupAFD, "pRowSum stats NULL pointer");
        return CamxResultEInvalidPointer;
    }

    ISPRSStatsConfig* pRSStatsConfig = reinterpret_cast<ISPRSStatsConfig*>(pRSConfig);

    pRowSumConfig->horizontalRegionCount = pRSStatsConfig->RSConfig.statsHNum;
    pRowSumConfig->verticalRegionCount   = pRSStatsConfig->RSConfig.statsVNum;
    pRowSumConfig->regionHeight          = pRSStatsConfig->statsRgnHeight;
    pRowSumConfig->regionWidth           = pRSStatsConfig->statsRgnWidth;
    pRowSumConfig->bitDepth              = pRSStatsConfig->bitDepth;

    CAMX_LOG_VERBOSE(CamxLogGroupAFD, "HRC:%d VRC:%d rh:%d rw:%d",
        pRowSumConfig->horizontalRegionCount,
        pRowSumConfig->verticalRegionCount,
        pRowSumConfig->regionHeight,
        pRowSumConfig->regionWidth);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::RetrieveRSStatsInfoFromInternalPool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CamxResult CAFDIOHandler::RetrieveRSStatsInfoFromInternalPool(
    VOID*        pRowSum,
    StatsRowSum* pRowSumInfo)
{
    ParsedRSStatsOutput* pStats = reinterpret_cast<ParsedRSStatsOutput*>(pRowSum);
    pRowSumInfo->rowSum         = reinterpret_cast<UINT32*>(pStats->rowSum);

    return CamxResultSuccess;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::RetrieveBGStatConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDIOHandler::RetrieveBGStatConfig(
    VOID*           pBGStatConfig,
    StatsBayerGrid* pBGStat)
{
    if (NULL == pBGStat)
    {
        CAMX_LOG_ERROR(CamxLogGroupAFD, "BG stats NULL pointer");
        return CamxResultEInvalidPointer;
    }

    PropertyISPHDRBEStats* pParsedBGStatsConfig = reinterpret_cast<PropertyISPHDRBEStats*>(pBGStatConfig);

    pBGStat->horizontalRegionCount = pParsedBGStatsConfig->statsConfig.HDRBEConfig.horizontalNum;
    pBGStat->verticalRegionCount   = pParsedBGStatsConfig->statsConfig.HDRBEConfig.verticalNum;
    pBGStat->totalRegionCount      = (pBGStat->horizontalRegionCount * pBGStat->verticalRegionCount);
    pBGStat->regionWidth           = pParsedBGStatsConfig->statsConfig.regionWidth;
    pBGStat->regionHeight          = pParsedBGStatsConfig->statsConfig.regionHeight;
    pBGStat->bitDepth              = static_cast<UINT16>(pParsedBGStatsConfig->statsConfig.HDRBEConfig.outputBitDepth);

    CamX::Utils::Memcpy(pBGStat->satThreshold,
        pParsedBGStatsConfig->statsConfig.HDRBEConfig.channelGainThreshold,
        (ChannelIndexCount * sizeof(UINT32)));

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::RetrieveBGStatOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDIOHandler::RetrieveBGStatOutput(
    VOID*           pBGStatOutput,
    StatsBayerGrid* pBGStat)
{
    if (NULL == pBGStat)
    {
        CAMX_LOG_ERROR(CamxLogGroupAFD, "BG stats NULL pointer");
        return CamxResultEInvalidPointer;
    }

    ParsedHDRBEStatsOutput* pParsedBGStatsOutput = reinterpret_cast<ParsedHDRBEStatsOutput*>(pBGStatOutput);

    pBGStat->flags.hasSatInfo = pParsedBGStatsOutput->flags.hasSatInfo;
    pBGStat->flags.usesY      = pParsedBGStatsOutput->flags.usesY;
    pBGStat->numValidRegions  = pParsedBGStatsOutput->numROIs;
    pBGStat->SetChannelDataArray(reinterpret_cast<StatsBayerGridChannelInfo*>(pParsedBGStatsOutput->GetChannelDataArray()));

    return CamxResultSuccess;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::ReadDataFromMainPropertyPool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDIOHandler::ReadDataFromMainPropertyPool(
    UINT32 propertyId,
    VOID** ppDataOut)
{
    CamxResult        result                  = CamxResultSuccess;
    UINT              GetProps[]              = {propertyId};
    static const UINT GetPropsLength          = CAMX_ARRAY_SIZE(GetProps);
    VOID*             pData[GetPropsLength]   = { 0 };
    UINT64            offsets[GetPropsLength] = { m_pNode->GetMaximumPipelineDelay()};

    m_pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);

    if (NULL != pData[0])
    {
        switch (propertyId)
        {
            case PropertyIDAECFrameInfo:
                result = RetrieveAECFrameInfoFromMainPool(pData[0], &m_aecInfoInput);
                *ppDataOut = static_cast<VOID*>(&m_aecInfoInput);
                break;
            case PropertyIDAFFrameInfo:
                result = RetrieveAFFrameInfoFromMainPool(pData[0], &m_afInfoInput);
                *ppDataOut = static_cast<VOID*>(&m_afInfoInput);
                break;
            default:
                break;
        }
    }
    else
    {
        result = CamxResultENoSuch;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::RetrieveSensorCurrentModeFromMainPool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAFDIOHandler::RetrieveSensorCurrentModeFromMainPool(
    VOID* pVoidInput,
    UINT* pSensorCurrentMode)
{
    if (NULL != pSensorCurrentMode)
    {
        *pSensorCurrentMode = *reinterpret_cast<UINT*>(pVoidInput);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAFD, "Sensor Current Mode NULL pointer");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::RetrieveSensorInformationFromUsecasePool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAFDIOHandler::RetrieveSensorInformationFromUsecasePool(
    VOID*          pVoidInput,
    AFDSensorInfo* pSensorInfo)
{
    if (NULL == pSensorInfo)
    {
        CAMX_LOG_ERROR(CamxLogGroupAFD, "Sensor Info NULL pointer");
    }
    CAMX_UNREFERENCED_PARAM(pVoidInput);
    CAMX_UNREFERENCED_PARAM(pSensorInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::RetrieveAECFrameInfoFromMainPool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDIOHandler::RetrieveAECFrameInfoFromMainPool(
    VOID*       pVoidInput,
    AFDAECInfo* pAFDAECInfo)
{
    AECFrameInformation* pAECFrameInfo = reinterpret_cast<AECFrameInformation*>(pVoidInput);

    pAFDAECInfo->isAECsettled   = pAECFrameInfo->AECSettled ? TRUE : FALSE;
    pAFDAECInfo->currentExpTime = static_cast<FLOAT>(pAECFrameInfo->exposureInfo[ExposureIndexShort].exposureTime);
    pAFDAECInfo->luxIDx         = pAECFrameInfo->luxIndex;
    pAFDAECInfo->realGain       = pAECFrameInfo->exposureInfo[ExposureIndexShort].linearGain;
    CAMX_LOG_VERBOSE(CamxLogGroupAFD, "AEC Settled?? at AFD %d", pAECFrameInfo->AECSettled);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::RetrieveAFFrameInfoFromMainPool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDIOHandler::RetrieveAFFrameInfoFromMainPool(
    VOID*      pVoidInput,
    AFDAFInfo* pAFInfo)
{
    if (NULL == pAFInfo)
    {
        CAMX_LOG_ERROR(CamxLogGroupAFD, "AF info NULL pointer");
        return CamxResultEInvalidPointer;
    }

    AFFrameInformation* pAFFrameInfo = reinterpret_cast<AFFrameInformation*>(pVoidInput);

    pAFInfo->isAFSearching             = (pAFFrameInfo->focusStatus.status == AFStatusFocusing) ? TRUE : FALSE;

    pAFInfo->lensPositionInfo.moveType =
        (pAFFrameInfo->moveLensOutput.useDACValue == TRUE) ? AFLensMoveTypeDAC : AFLensMoveTypeLogical;

    if (AFLensMoveTypeLogical == pAFInfo->lensPositionInfo.moveType)
    {
        pAFInfo->lensPositionInfo.logicalLensMoveInfo.lensPositionInLogicalUnits =
            pAFFrameInfo->moveLensOutput.targetLensPosition;
    }
    else if ( AFLensMoveTypeDAC == pAFInfo->lensPositionInfo.moveType)
    {
        pAFInfo->lensPositionInfo.digitalLensMoveInfo.lensPositionInDACUnits =
            pAFFrameInfo->moveLensOutput.targetLensPosition;
    }


    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDIOHandler::SetHALParam
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDIOHandler::SetHALParam(
    AFDHALParam* pHALParam)
{
    CamxResult result  = CamxResultSuccess;

    switch(pHALParam->antiBandingMode)
    {
        case ControlAEAntibandingModeOff:
            m_frameSetting.currentAFDMode = StatisticsAntibandingOff;
            break;
        case ControlAEAntibandingMode50Hz:
            m_frameSetting.currentAFDMode = StatisticsAntibanding50Hz;
            break;
        case ControlAEAntibandingMode60Hz:
            m_frameSetting.currentAFDMode = StatisticsAntibanding60Hz;
            break;
        case ControlAEAntibandingModeAuto:
        {
            if (StatisticsAntibandingMax != m_lastAFDMode)
            {
                m_frameSetting.currentAFDMode = static_cast<StatisticsAntibanding>(m_lastAFDMode);
            }
            else
            {
                m_frameSetting.currentAFDMode = StatisticsAntibandingAuto;
            }
            break;
        }
        case ControlAEAntibandingAuto_50HZ:
        {
            if (StatisticsAntibandingMax != m_lastAFDMode)
            {
                m_frameSetting.currentAFDMode = static_cast<StatisticsAntibanding>(m_lastAFDMode);
            }
            else
            {
                m_frameSetting.currentAFDMode = StatisticsAntibandingAuto_50HZ;
            }
            break;
        }
        case ControlAEAntibandingAuto_60HZ:
        {
            if (StatisticsAntibandingMax != m_lastAFDMode)
            {
                m_frameSetting.currentAFDMode = static_cast<StatisticsAntibanding>(m_lastAFDMode);
            }
            else
            {
                m_frameSetting.currentAFDMode = StatisticsAntibandingAuto_60HZ;
            }
            break;
        }
        default:
            m_frameSetting.currentAFDMode = StatisticsAntibandingInvalid;
            CAMX_LOG_ERROR(CamxLogGroupAFD, "Invalid antibanding type");
            break;
    }

    return result;
}

CAMX_NAMESPACE_END
