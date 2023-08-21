////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2hwmultiframe.cpp
/// @brief HWMF CHI feature2 implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2base.h"
#include "chifeature2featurepool.h"
#include "chifeature2hwmultiframe.h"
#include "chifeature2utils.h"

// NOWHINE FILE CP006: Need vector to pass filtered port information
// NOWHINE FILE GR027: Need whiner update

static const UINT32 Feature2MajorVersion  = 0;
static const UINT32 Feature2MinorVersion  = 1;
static const CHAR*  VendorName            = "QTI";
static const UINT32 DS64DisableWidth      = 1920;
static const UINT32 DS64DisableHeight     = 1664;

/// @brief HWMF min buffer count to set in Meta TBM
static const UINT32 FEATURE2_HWMF_MIN_METADATA_BUFFER_COUNT = 1;
/// @brief  HWMF  internal Buffer Count
static const UINT32   PREFILTER_POSTFILTER_OUT_BUFFER_MIN_COUNT = 0;
static const UINT32   PREFILTER_POSTFILTER_OUT_BUFFER_MAX_COUNT = 1;
static const UINT32   BLEND_OUT_BUFFER_MIN_COUNT                = 0;
static const UINT32   BLEND_OUT_BUFFER_MAX_COUNT                = 3;

/// @brief  HWMF Prefilter ubwc-out port Id
static const ChiFeature2Identifier HWMFPreFilterOutputPortUBWC      = MfsrPrefilterOutPutPortDescriptors[0].globalId;
/// @brief  HWMF Prefilter pd10DS4-out port Id
static const ChiFeature2Identifier HWMFPreFilterOutputPortPD10DS4   = MfsrPrefilterOutPutPortDescriptors[1].globalId;
/// @brief  HWMF Prefilter pd10DS16-out port Id
static const ChiFeature2Identifier HWMFPreFilterOutputPortPD10DS16  = MfsrPrefilterOutPutPortDescriptors[2].globalId;
/// @brief  HWMF Prefilter pd10DS64-out port Id
static const ChiFeature2Identifier HWMFPreFilterOutputPortPD10DS64  = MfsrPrefilterOutPutPortDescriptors[3].globalId;
/// @brief  HWMF Prefilter reg-out port Id
static const ChiFeature2Identifier HWMFPreFilterOutputPortREG       = MfsrPrefilterOutPutPortDescriptors[4].globalId;
/// @brief  HWMF Prefilter meta-out port Id
static const ChiFeature2Identifier HWMFPreFilterOutputPortMETA      = MfsrPrefilterOutPutPortDescriptors[5].globalId;
/// @brief  HWMF Prefilter cvp-out port Id
static const ChiFeature2Identifier HWMFPreFilterOutputPortCVP       = MfsrPrefilterOutPutPortDescriptors[6].globalId;

/// @brief  HWMF Blend ubwc-out port Id
static const ChiFeature2Identifier HWMFBlendOutputPortUBWC      = MfsrBlendOutPutPortDescriptors[0].globalId;
/// @brief  HWMF Blend pd10DS4-out port Id
static const ChiFeature2Identifier HWMFBlendOutputPortPD10DS4   = MfsrBlendOutPutPortDescriptors[1].globalId;
/// @brief  HWMF Blend meta-out port Id
static const ChiFeature2Identifier HWMFBlendOutputPortMeta      = MfsrBlendOutPutPortDescriptors[2].globalId;
/// @brief  HWMF Blend cvp-out port Id
static const ChiFeature2Identifier HWMFBlendOutputPortCVP       = MfsrBlendOutPutPortDescriptors[3].globalId;
/// @brief  HWMF Blend cvpctx-out port Id
static const ChiFeature2Identifier HWMFBlendOutputPortCVPCTXT   = MfsrBlendOutPutPortDescriptors[4].globalId;

/// @brief  HWMF postfilter yuv-out port Id
static const ChiFeature2Identifier HWMFPostFilterOutputPortYUV      = MfsrPostFilterOutPutPortDescriptors[0].globalId;

/// @brief  HWMF BelndInit inputport dependency port Id
static const ChiFeature2Identifier HWMFBlendInitInputPortUBWC = MfsrBlendInputPortDescriptors[1].globalId;
/// @brief  HWMF BelndLoop inputport dependency port Id
static const ChiFeature2Identifier HWMFBlendLoopInputPortUBWC = MfsrBlendInputPortDescriptors[6].globalId;

static const CHAR*  Feature2HwMultiFrameCaps[] =
{
    "MFSR",
    "MFNR"
};

/// @brief This structure encapsulates an intermediate dimension
struct ChiFeature2IntermediateDimensions
{
    UINT32 width;   ///< intermediate width
    UINT32 height;  ///< intermediate height
    FLOAT  ratio;   ///< intermediate ratio
};

/// @brief This enumerates the stages for HWMF
enum ChiFeature2HWMFStage
{
    HWMFStagePrefilter = 0,     ///< HWMF Prefilter stage
    HWMFStageBlendInit,         ///< HWMF Blend Init stage
    HWMFStageBlendLoop,         ///< HWMF Blend Loop stage
    HWMFStagePostfilter,        ///< HWMF Postfilter stage
    HWMFStageNoiseReprocess,    ///< HWMF NoiseReprocess stage
    HWMFStageMax                ///< HWMF Max stage
};



/// @brief This structure encapsulates HWMF feature information
struct ChiFeature2HWMFFeatureInfo
{
    ChiFeature2BufferMetadataInfo       inputRDIBufferInfo[BufferQueueDepth];        ///< RDI Buffer info
    ChiFeature2BufferMetadataInfo       inputRDIMetaInfo[BufferQueueDepth];          ///< RDI Buffer's Metadata info
    ChiFeature2BufferMetadataInfo       inputFDBufferMetaInfo[BufferQueueDepth];     ///< FD Buffer and Metahandle info
    ChiMetadata*                        pHWMFPrefilterStageMeta;                     ///< HWMF Prefilter stage Meta
    UINT32                              hwmfTotalNumFrames;                          ///< number of frames needed for HWMF
    ChiFeature2HWMFStage                currentHWMFStage;                            ///< To Store HWMF stage
    UINT32                              hwmfCurrentStageCount;                       ///< To Store HWMF current stage count
    UINT32                              tuningMode;                                  ///< To Store tuniing mode
    ChiFeature2IntermediateDimensions   intermediateDimensions;                      ///< Intermediate Dimensions
    ChiFeature2FinalOutputDimensions    finalHWMFDimensions;                         ///< Final HWMF o/p Dimensions
    BOOL                                bIsRDIRequiredForPostFilterStage;            ///< To check whether RDI is needed
                                                                                     ///  for Post filter stage
    UINT32                              preFilterHWMFStageBuffersExpected;           ///< To store Prefilter Stage o/p buffers
    UINT32                              blendHWMFStageBuffersExpected;               ///< To store blend Stage o/p buffers
    UINT32                              postFilterHWMFStageBuffersExpected;          ///< To store postfilter Stage o/p buffers
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2HWmultiframe * ChiFeature2HWmultiframe::Create(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    ChiFeature2HWmultiframe*    pFeature    = CHX_NEW(ChiFeature2HWmultiframe);
    CDKResult                   result      = CDKResultSuccess;

    if (NULL == pFeature)
    {
        CHX_LOG_ERROR("Out of memory: pFeature is NULL");
    }
    else
    {
        result = pFeature->Initialize(pCreateInputInfo);
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Feature failed to initialize!!");
            pFeature->Destroy();
            pFeature = NULL;
        }
        else
        {
            pFeature->m_disableZoomCrop = pCreateInputInfo->bDisableZoomCrop;
        }
    }

    return pFeature;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2HWmultiframe::Destroy()
{
    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::~ChiFeature2HWmultiframe
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2HWmultiframe::~ChiFeature2HWmultiframe()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::ShouldDisableDS64
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL ShouldDisableDS64(
    ChiStream* pChiStream)
{
    return (pChiStream->streamType == ChiStreamTypeInput) &&
           ((pChiStream->width <= DS64DisableWidth) || (pChiStream->height <= DS64DisableHeight));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::OnInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2HWmultiframe::OnInitialize(
    ChiFeature2CreateInputInfo * pCreateInputInfo)
{
    CDKResult   result          = CDKResultSuccess;
    CHISTREAM** ppChiStreams    = pCreateInputInfo->pStreamConfig->pChiStreams;
    m_disableDS64Port           = FALSE;
    m_hwmfFinalDimensions       = { 0 };

    for (UINT32 i = 0; i < pCreateInputInfo->pStreamConfig->numStreams; i++)
    {
        if (ppChiStreams[i]->streamType == ChiStreamTypeOutput)
        {
            m_hwmfFinalDimensions.width  = ppChiStreams[i]->width;
            m_hwmfFinalDimensions.height = ppChiStreams[i]->height;
        }

        if (ppChiStreams[i]->streamType == ChiStreamTypeInput)
        {
            if (TRUE == ShouldDisableDS64(ppChiStreams[i]))
            {
                CHX_LOG_INFO("Disable DS64 port");
                m_disableDS64Port = TRUE;
            }
        }
    }

    result = ChiFeature2Base::OnInitialize(pCreateInputInfo);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::IsRDIRequiredPostFilterStage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2HWmultiframe::IsRDIRequiredPostFilterStage() const
{
    CDKResult result         = CDKResultSuccess;
    BOOL      bIsRDIRequired = FALSE;

    ChiFeature2PortIdList inputList = { 0 };

    result = GetInputPortsForStage(HWMFStagePostfilter, &inputList);

    if (result == CDKResultSuccess)
    {
        ChiFeature2Identifier               portidentifier      = inputList.pPorts[0];
        const   ChiFeature2PortDescriptor*  pPortDescriptor     = GetPortDescriptorFromPortId(&portidentifier);

        if ((NULL != pPortDescriptor) &&
            (NULL != pPortDescriptor->pTargetDescriptor) &&
            (NULL != pPortDescriptor->pTargetDescriptor->pTargetName))
        {
            if (0 == CdkUtils::StrCmp(pPortDescriptor->pTargetDescriptor->pTargetName, "TARGET_BUFFER_RAW"))
            {
                CHX_LOG_INFO("Postfilter needs RDI");
                bIsRDIRequired = TRUE;
            }
        }
    }

    return bIsRDIRequired;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::OnPortCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2HWmultiframe::OnPortCreate(
    ChiFeature2Identifier* pKey)
{
    CDKResult result = CDKResultSuccess;

    result = ChiFeature2Base::OnPortCreate(pKey);

    if (NULL != pKey)
    {
        if ((*pKey == HWMFPreFilterOutputPortUBWC) ||
            (*pKey == HWMFPreFilterOutputPortPD10DS4) ||
            (*pKey == HWMFPreFilterOutputPortPD10DS16) ||
            (*pKey == HWMFPreFilterOutputPortPD10DS64) ||
            (*pKey == HWMFPreFilterOutputPortREG) ||
            (*pKey == HWMFPreFilterOutputPortMETA) ||
            (*pKey == HWMFPreFilterOutputPortCVP) ||
            (*pKey == HWMFPostFilterOutputPortYUV))
        {
            ChiFeaturePortData* pPortData = GetPortData(pKey);
            if ((NULL != pPortData) && (NULL != pPortData->pTarget))
            {
                pPortData->maxBufferCount = PREFILTER_POSTFILTER_OUT_BUFFER_MAX_COUNT;
                pPortData->minBufferCount = PREFILTER_POSTFILTER_OUT_BUFFER_MIN_COUNT;
            }
        }

        if ((*pKey == HWMFBlendOutputPortPD10DS4) ||
            (*pKey == HWMFBlendOutputPortMeta) ||
            (*pKey == HWMFBlendOutputPortCVP) ||
            (*pKey == HWMFBlendOutputPortCVPCTXT) ||
            (*pKey == HWMFBlendOutputPortUBWC))
        {
            ChiFeaturePortData* pPortData = GetPortData(pKey);
            if ((NULL != pPortData) && (NULL != pPortData->pTarget))
            {
                pPortData->minBufferCount = BLEND_OUT_BUFFER_MIN_COUNT;
                pPortData->maxBufferCount = BLEND_OUT_BUFFER_MAX_COUNT;
            }
        }

        ChiFeaturePortData* pPortData = GetPortData(pKey);
        if (NULL != pPortData)
        {
            if (ChiFeature2PortType::MetaData == pPortData->globalId.portType)
            {
                pPortData->minBufferCount = FEATURE2_HWMF_MIN_METADATA_BUFFER_COUNT;
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid Key");
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::OnPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2HWmultiframe::OnPipelineCreate(
    ChiFeature2Identifier* pKey)
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pKey)
    {
        ChiFeaturePipelineData* pPipelineData = GetPipelineData(pKey);
        if (NULL != pPipelineData)
        {
            pPipelineData->minMetaBufferCount = FEATURE2_HWMF_MIN_METADATA_BUFFER_COUNT;
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid key");
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::OnPrepareRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2HWmultiframe::OnPrepareRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pRequestObject)
    {
        ChiFeature2Hint*            pFeatureHint        = pRequestObject->GetFeatureHint();
        ChiFeature2HWMFFeatureInfo* pHWMFFeatureInfo    =
            static_cast<ChiFeature2HWMFFeatureInfo*>(CHX_CALLOC(sizeof(ChiFeature2HWMFFeatureInfo)));
        if (NULL == pHWMFFeatureInfo)
        {
            CHX_LOG_ERROR("Error Allocating memory");
            result = CDKResultENoMemory;
        }
        else
        {
            if ((NULL != pFeatureHint) && (0 < pFeatureHint->numFrames))
            {
                pHWMFFeatureInfo->hwmfTotalNumFrames = pFeatureHint->numFrames;
            }
            else
            {
                pHWMFFeatureInfo->hwmfTotalNumFrames = 3;
            }
            pHWMFFeatureInfo->bIsRDIRequiredForPostFilterStage = IsRDIRequiredPostFilterStage();
            CHX_LOG_INFO("Number of frames to process %d IsRDIRequiredPostFilterStage %d",
                pHWMFFeatureInfo->hwmfTotalNumFrames, pHWMFFeatureInfo->bIsRDIRequiredForPostFilterStage);

            pHWMFFeatureInfo->pHWMFPrefilterStageMeta = ChiMetadata::Create();

            if (NULL == pHWMFFeatureInfo->pHWMFPrefilterStageMeta)
            {
                CHX_LOG_ERROR("Failed to create hwmf prefilterstage meta");
            }
            SetFeaturePrivContext(pRequestObject, pHWMFFeatureInfo);

            VOID* pPrivateInterData = pRequestObject->GetInterFeatureRequestPrivateData(m_featureId,
                                                                                   m_pInstanceProps->cameraId,
                                                                                   m_pInstanceProps->instanceId);
            if ((NULL == pPrivateInterData) && (NULL != pHWMFFeatureInfo))
            {
                UINT* pHintNumFrames = static_cast<UINT*>(CHX_CALLOC(sizeof(UINT)));
                if (NULL != pHintNumFrames)
                {
                    *pHintNumFrames = pHWMFFeatureInfo->hwmfTotalNumFrames;
                    pRequestObject->SetInterFeatureRequestPrivateData(m_featureId,
                                                                      m_pInstanceProps->cameraId,
                                                                      m_pInstanceProps->instanceId,
                                                                      pHintNumFrames);
                }
                else
                {
                    CHX_LOG_ERROR("Allocate intra private information failed!");
                    result = CDKResultENoMemory;
                }
            }

        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid Request Object");
        result = CDKResultEInvalidState;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::OnExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2HWmultiframe::OnExecuteProcessRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDKResult result                        = CDKResultSuccess;
    UINT8     stageId                       = InvalidStageId;
    UINT8     nextStageId                   = InvalidStageId;
    UINT8     stageSequenceId               = InvalidStageSequenceId;
    ChiFeature2StageInfo stageInfo          = {0};
    UINT8      numBlendLoops                = 0;
    UINT8      numDependencyLists           = 0;

    ChiFeature2HWMFFeatureInfo*         pHWMFFeatureInfo    = NULL;
    const ChiFeature2StageDescriptor*   pStageDescriptor    = NULL;

    if (NULL != pRequestObject)
    {
        result = GetCurrentStageInfo(pRequestObject, &stageInfo);

        if (CDKResultSuccess == result)
        {
            stageId = stageInfo.stageId;
            stageSequenceId = stageInfo.stageSequenceId;
            nextStageId = stageId + 1;

            switch (stageId)
            {
                case InvalidStageId:
                    // Setting this tells the request object how many overall sequences we will run
                    SetConfigInfo(pRequestObject, 10);
                    pStageDescriptor = GetStageDescriptor(nextStageId);
                    if (NULL != pStageDescriptor)
                    {
                        pHWMFFeatureInfo =
                            static_cast<ChiFeature2HWMFFeatureInfo*>(GetFeaturePrivContext(pRequestObject));
                        if (NULL != pHWMFFeatureInfo)
                        {
                            SetNextStageInfoFromStageDescriptor(pRequestObject, pStageDescriptor, 0,
                                pHWMFFeatureInfo->hwmfTotalNumFrames);
                            const ChiFeature2InputDependency* pInputDependency =
                                GetDependencyListFromStageDescriptor(pStageDescriptor, 0, 0, 0);
                            if (NULL != pInputDependency)
                            {
                                for (UINT8 bufferIndex = 0; bufferIndex < pHWMFFeatureInfo->hwmfTotalNumFrames; ++bufferIndex)
                                {
                                    PopulateDependencyPorts(pRequestObject, bufferIndex, pInputDependency);
                                }
                            }
                            CHX_LOG_INFO("Num of RDI's Requested %d ", pHWMFFeatureInfo->hwmfTotalNumFrames);
                            CHX_LOG_INFO("Wait for Initial Dependency stageId %d sequenceId %d",
                                stageId, stageSequenceId);
                        }
                        else
                        {
                            CHX_LOG_ERROR("Invalid Private context EXIT HWMF snapshot");
                        }
                    }
                    break;

                case HWMFStagePrefilter:
                    ATRACE_ASYNC_BEGIN("ChiFeature2HWmultiframe::ProcessMfsrPrefilterStage", HWMFStagePrefilter);
                    result = ProcessHWMFPrefilterStage(pRequestObject, stageId, stageSequenceId, nextStageId);
                    break;

                case HWMFStageBlendInit:
                    ATRACE_ASYNC_BEGIN("ChiFeature2HWmultiframe::ProcessHWMFBlendLoopStage", HWMFStageBlendLoop);
                    result = ProcessHWMFBlendInitStage(pRequestObject, stageId, stageSequenceId, nextStageId);
                    break;

                case HWMFStageBlendLoop:
                    result = ProcessHWMFBlendLoopStage(pRequestObject, stageId, stageSequenceId, nextStageId);
                    break;

                case HWMFStagePostfilter:
                    ATRACE_ASYNC_BEGIN("ChiFeature2HWmultiframe::ProcessHWMFPostfilterStage", HWMFStagePostfilter);
                    result = ProcessHWMFPostfilterStage(pRequestObject, stageId, stageSequenceId, nextStageId);
                    break;

                default:
                    CHX_LOG_ERROR("Unhandled stage id %d", stageId);
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid Request Object");
        result = CDKResultEInvalidState;
    }

    return result;

}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::ProcessHWMFPrefilterStage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2HWmultiframe::ProcessHWMFPrefilterStage(
    ChiFeature2RequestObject * pRequestObject,
    UINT8                      stageId,
    UINT8                      stageSequenceId,
    UINT8                      nextStageId
    )const
{
    CDKResult result                                    = CDKResultSuccess;
    ChiFeature2BufferMetadataInfo inputBufferInfo       = {0};
    ChiFeature2BufferMetadataInfo inputMetaInfo         = {0};
    UINT8      numDependencyLists                       = 0;

    ChiFeature2PortIdList               outputList      = {0};
    ChiFeature2PortIdList               inputList       = {0};

    ChiFeature2HWMFFeatureInfo*         pHWMFFeatureInfo    = NULL;
    const ChiFeature2StageDescriptor*   pStageDescriptor    = NULL;
    CHISTREAMBUFFER*                    pRDIBuffer          = NULL;
    CHIMETAHANDLE                       hRDIMetadata        = NULL;

    ChiFeature2Identifier portidentifier        = { 0, 0, 0, ChiFeature2PortDirectionType::ExternalInput,
    ChiFeature2PortType::ImageBuffer };
    ChiFeature2Identifier metaportidentifier    = { 0, 0, 1, ChiFeature2PortDirectionType::ExternalInput,
    ChiFeature2PortType::MetaData };

    ChiFeature2Identifier preFilterRDIportidentifier  = { 0, 0, 0, ChiFeature2PortDirectionType::ExternalInput,
    ChiFeature2PortType::ImageBuffer };
    ChiFeature2Identifier preFilterMetaportidentifier = { 0, 0, 1, ChiFeature2PortDirectionType::ExternalInput,
    ChiFeature2PortType::MetaData };

    pRDIBuffer = static_cast<CHISTREAMBUFFER*>(CHX_CALLOC(sizeof(CHISTREAMBUFFER)));

    pStageDescriptor = GetStageDescriptor(stageId);

    if (NULL != pStageDescriptor)
    {
        CHX_LOG_INFO("Dependency Met for HWMFStagePrefilter stageId %d sequenceId %d",
            stageId, stageSequenceId);
        pHWMFFeatureInfo =
            static_cast<ChiFeature2HWMFFeatureInfo*>(GetFeaturePrivContext(pRequestObject));

        if (NULL != pHWMFFeatureInfo)
        {
            CHX_LOG_INFO("Num of RDI's Received %d ", pHWMFFeatureInfo->hwmfTotalNumFrames);

            CHAR dumpFileBaseName[128] = { 0 };
            GetDumpFileBaseName(pRequestObject, dumpFileBaseName, 128);

            for (UINT i = 0; i < pHWMFFeatureInfo->hwmfTotalNumFrames; i++)
            {
                pRequestObject->GetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency,
                    &portidentifier, &inputBufferInfo.hBuffer, &inputBufferInfo.key, 0, i);
                pRequestObject->GetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency,
                    &metaportidentifier, &inputMetaInfo.hBuffer, &inputMetaInfo.key, 0, i);

                ChxUtils::Memcpy(&pHWMFFeatureInfo->inputRDIBufferInfo[i], &inputBufferInfo,
                    sizeof(ChiFeature2BufferMetadataInfo));
                ChxUtils::Memcpy(&pHWMFFeatureInfo->inputRDIMetaInfo[i], &inputMetaInfo,
                    sizeof(ChiFeature2BufferMetadataInfo));

                CHX_LOG_INFO("input RDI buffer %pK meta handle %pK",
                    inputBufferInfo.hBuffer, inputMetaInfo.hBuffer);

                if (NULL != pRDIBuffer)
                {
                    result = GetStreamBuffer(inputBufferInfo.hBuffer,
                        inputBufferInfo.key,
                        pRDIBuffer);
                    if (CDKResultSuccess == result)
                    {
                        result = GetMetadataBuffer(inputMetaInfo.hBuffer,
                            inputMetaInfo.key, &hRDIMetadata);

                        if (CDKResultSuccess == result)
                        {
                            if ((NULL == pRDIBuffer->bufferInfo.phBuffer) ||
                                (NULL == hRDIMetadata))
                            {
                                CHX_LOG_ERROR("RDI or Metadata Buffer are NULL RDI buffer %pK  meta buffer %pK",
                                    pRDIBuffer->bufferInfo.phBuffer, hRDIMetadata);
                            }
                        }
                    }
                }

                if (TRUE == ExtensionModule::GetInstance()->EnableFeature2Dump())
                {
                    DumpInputMetaBuffer(&inputMetaInfo, dumpFileBaseName, i);
                    DumpInputImageBuffer(&inputBufferInfo, dumpFileBaseName, i);
                }
            }

            if (NULL != pRDIBuffer)
            {
                CHX_FREE(pRDIBuffer);
                pRDIBuffer = NULL;
            }

            pHWMFFeatureInfo->currentHWMFStage              = HWMFStagePrefilter;
            pHWMFFeatureInfo->tuningMode                    = static_cast<UINT32>(ChiModeFeature2SubModeType::MFNRBlend);
            pHWMFFeatureInfo->finalHWMFDimensions.width     = m_hwmfFinalDimensions.width;
            pHWMFFeatureInfo->finalHWMFDimensions.height    = m_hwmfFinalDimensions.height;
            SetFeaturePrivContext(pRequestObject, pHWMFFeatureInfo);
        }

        result = GetOutputPortsForStage(stageId, &outputList);

        if (CDKResultSuccess == result)
        {
            result = GetInputPortsForStage(stageId, &inputList);
        }
        if (CDKResultSuccess == result)
        {
            if (TRUE == m_disableDS64Port)
            {
                std::vector<ChiFeature2Identifier> filteredList = SelectOutputPorts(&outputList, pRequestObject);

                ChiFeature2PortIdList filtered = {0};

                filtered.numPorts = filteredList.size();
                filtered.pPorts   = filteredList.data();
                PopulatePortConfiguration(pRequestObject, &inputList, &filtered);
                if (NULL != pHWMFFeatureInfo)
                {
                    pHWMFFeatureInfo->preFilterHWMFStageBuffersExpected = filtered.numPorts - 2;
                }
            }
            else
            {
                PopulatePortConfiguration(pRequestObject, &inputList, &outputList);
                if (NULL != pHWMFFeatureInfo)
                {
                    pHWMFFeatureInfo->preFilterHWMFStageBuffersExpected = outputList.numPorts - 1;
                }
            }

            if (NULL != pHWMFFeatureInfo)
            {
                pRequestObject->SetBufferInfo(ChiFeature2RequestObjectOpsType::InputConfiguration,
                                              &preFilterRDIportidentifier,
                                              pHWMFFeatureInfo->inputRDIBufferInfo[stageId + stageSequenceId].hBuffer,
                                              pHWMFFeatureInfo->inputRDIBufferInfo[stageId + stageSequenceId].key,
                                              FALSE,
                                              0,
                                              0);

                pRequestObject->SetBufferInfo(ChiFeature2RequestObjectOpsType::InputConfiguration,
                                              &preFilterMetaportidentifier,
                                              pHWMFFeatureInfo->inputRDIMetaInfo[stageId + stageSequenceId].hBuffer,
                                              pHWMFFeatureInfo->inputRDIMetaInfo[stageId + stageSequenceId].key,
                                              FALSE,
                                              0,
                                              0);

                SetFeaturePrivContext(pRequestObject, pHWMFFeatureInfo);
                CHX_LOG_INFO("Submit HWMFStagePrefilter stageId %d sequenceId %d", stageId, stageSequenceId);
                result = SubmitRequestToSession(pRequestObject);
            }

            if ((CDKResultSuccess == result) && (nextStageId < GetNumStages()))
            {
                pStageDescriptor = GetStageDescriptor(nextStageId);
                if (NULL != pStageDescriptor)
                {
                    numDependencyLists = GetNumDependencyListsFromStageDescriptor(pStageDescriptor, 0, 0);
                    SetNextStageInfoFromStageDescriptor(pRequestObject, pStageDescriptor,
                        0, numDependencyLists);
                    if (TRUE == m_disableDS64Port)
                    {
                        const ChiFeature2InputDependency* pInputDependency =
                            GetDependencyListFromStageDescriptor(pStageDescriptor, 0, 0, 1);
                        if (NULL != pInputDependency)
                        {
                            PopulateDependencyPorts(pRequestObject, 0, pInputDependency);
                        }
                    }
                    else
                    {
                        const ChiFeature2InputDependency* pInputDependency =
                            GetDependencyListFromStageDescriptor(pStageDescriptor, 0, 0, 0);
                        if (NULL != pInputDependency)
                        {
                            PopulateDependencyPorts(pRequestObject, 0, pInputDependency);
                        }
                    }
                    CHX_LOG_INFO("Wait for Dependency  for HWMFStageBlendInit stageId %d sequenceId %d",
                        nextStageId, stageSequenceId);
                }
            }
        }
        else
        {
            CHX_LOG_ERROR("Failure to get I/O port info for HWMFStagePrefilter stage %d stageSequenceId %d",
                stageId, stageSequenceId);
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid stage descriptor for HWMFStagePrefilter stage %d stageSequenceId %d",
            stageId, stageSequenceId);
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::ProcessHWMFBlendInitStage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2HWmultiframe::ProcessHWMFBlendInitStage(
    ChiFeature2RequestObject * pRequestObject,
    UINT8                      stageId,
    UINT8                      stageSequenceId,
    UINT8                      nextStageId
    )const
{

    CDKResult result                                    = CDKResultSuccess;
    UINT8      numDependencyLists                       = 0;
    ChiFeature2PortIdList               outputList      = {0};
    ChiFeature2PortIdList               inputList       = {0};

    ChiFeature2HWMFFeatureInfo*         pHWMFFeatureInfo    = NULL;
    const ChiFeature2StageDescriptor*   pStageDescriptor    = NULL;

    ChiFeature2Identifier blendRDIportidentifier  = { 0, 1, 0, ChiFeature2PortDirectionType::ExternalInput,
    ChiFeature2PortType::ImageBuffer };
    ChiFeature2Identifier blendMetaportidentifier = { 0, 1, 7, ChiFeature2PortDirectionType::ExternalInput,
    ChiFeature2PortType::MetaData };

    pStageDescriptor = GetStageDescriptor(stageId);

    if (NULL != pStageDescriptor)
    {
        CHX_LOG_INFO("Dependency Met for HWMFStageBlendInit stageId %d sequenceId %d",
            stageId, stageSequenceId);
        result = GetOutputPortsForStage(stageId, &outputList);

        if (CDKResultSuccess == result)
        {
            result = GetInputPortsForStage(stageId, &inputList);
        }
        if (CDKResultSuccess == result)
        {
            pHWMFFeatureInfo =
                static_cast<ChiFeature2HWMFFeatureInfo*>(GetFeaturePrivContext(pRequestObject));

            if (NULL != pHWMFFeatureInfo)
            {
                pHWMFFeatureInfo->currentHWMFStage = HWMFStageBlendInit;
                pHWMFFeatureInfo->tuningMode = static_cast<UINT32>(ChiModeFeature2SubModeType::MFNRBlend);
                SetFeaturePrivContext(pRequestObject, pHWMFFeatureInfo);
            }

            if (TRUE == m_disableDS64Port)
            {
                std::vector<ChiFeature2Identifier> filteredList = SelectInputPorts(&inputList, pRequestObject);

                ChiFeature2PortIdList filtered = {0};

                filtered.numPorts = filteredList.size();
                filtered.pPorts   = filteredList.data();
                PopulatePortConfiguration(pRequestObject, &filtered, &outputList);
            }
            else
            {
                PopulatePortConfiguration(pRequestObject, &inputList, &outputList);
            }

            if (NULL != pHWMFFeatureInfo)
            {
                pRequestObject->SetBufferInfo(ChiFeature2RequestObjectOpsType::InputConfiguration,
                                              &blendRDIportidentifier,
                                              pHWMFFeatureInfo->inputRDIBufferInfo[stageId + stageSequenceId].hBuffer,
                                              pHWMFFeatureInfo->inputRDIBufferInfo[stageId + stageSequenceId].key,
                                              FALSE,
                                              0,
                                              0);

                pRequestObject->SetBufferInfo(ChiFeature2RequestObjectOpsType::InputConfiguration,
                                              &blendMetaportidentifier,
                                              pHWMFFeatureInfo->inputRDIMetaInfo[stageId + stageSequenceId].hBuffer,
                                              pHWMFFeatureInfo->inputRDIMetaInfo[stageId + stageSequenceId].key,
                                              FALSE,
                                              0,
                                              0);

                pHWMFFeatureInfo->blendHWMFStageBuffersExpected = outputList.numPorts - 1;

                SetFeaturePrivContext(pRequestObject, pHWMFFeatureInfo);
                CHX_LOG_INFO("Submit HWMFStageBlendInit stageId %d sequenceId %d", stageId, stageSequenceId);
                result = SubmitRequestToSession(pRequestObject);

                if ((CDKResultSuccess == result) && (nextStageId < GetNumStages()))
                {
                    if ((TRUE == pHWMFFeatureInfo->bIsRDIRequiredForPostFilterStage) &&
                        (pHWMFFeatureInfo->hwmfTotalNumFrames <= 3))
                    {
                        nextStageId++;
                        CHX_LOG_VERBOSE("Skip BlendLoop if number of RDI frames <=3 stageId %d sequenceId %d",
                            nextStageId, stageSequenceId);
                    }

                    pStageDescriptor = GetStageDescriptor(nextStageId);
                    if (NULL != pStageDescriptor)
                    {
                        numDependencyLists = GetNumDependencyListsFromStageDescriptor(pStageDescriptor, 0, 0);
                        SetNextStageInfoFromStageDescriptor(pRequestObject, pStageDescriptor,
                            0, numDependencyLists);
                        if (TRUE == m_disableDS64Port)
                        {
                            const ChiFeature2InputDependency* pInputDependency =
                                GetDependencyListFromStageDescriptor(pStageDescriptor, 0, 0, 1);
                            if (NULL != pInputDependency)
                            {
                                PopulateDependencyPorts(pRequestObject, 0, pInputDependency);
                            }
                        }
                        else
                        {
                            const ChiFeature2InputDependency* pInputDependency =
                                GetDependencyListFromStageDescriptor(pStageDescriptor, 0, 0, 0);
                            if (NULL != pInputDependency)
                            {
                                PopulateDependencyPorts(pRequestObject, 0, pInputDependency);
                            }
                        }
                        CHX_LOG_INFO("Wait for Dependency  for HWMFStageBlendLoop stageId %d sequenceId %d",
                            nextStageId, stageSequenceId);
                    }
                }
            }

        }
        else
        {
            CHX_LOG_ERROR("Failure to get I/O port info for HWMFStageBlendInit stage %d stageSequenceId %d",
                stageId, stageSequenceId);
        }

    }
    else
    {
        CHX_LOG_ERROR("Invalid stage descriptor for HWMFStageBlendInit stage %d stageSequenceId %d",
            stageId, stageSequenceId);
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::ProcessHWMFBlendLoopStage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2HWmultiframe::ProcessHWMFBlendLoopStage(
    ChiFeature2RequestObject * pRequestObject,
    UINT8                      stageId,
    UINT8                      stageSequenceId,
    UINT8                      nextStageId
    )const
{

    CDKResult result                                = CDKResultSuccess;
    UINT8      numDependencyLists                   = 0;
    UINT8      numBlendLoops                        = 0;
    ChiFeature2PortIdList               outputList  = {0};
    ChiFeature2PortIdList               inputList   = {0};

    ChiFeature2HWMFFeatureInfo*         pHWMFFeatureInfo    = NULL;
    const ChiFeature2StageDescriptor*   pStageDescriptor    = NULL;

    ChiFeature2Identifier blendRDIportidentifier  = { 0, 1, 0, ChiFeature2PortDirectionType::ExternalInput,
    ChiFeature2PortType::ImageBuffer };
    ChiFeature2Identifier blendMetaportidentifier = { 0, 1, 7, ChiFeature2PortDirectionType::ExternalInput,
    ChiFeature2PortType::MetaData };

    pStageDescriptor = GetStageDescriptor(stageId);

    if (NULL != pStageDescriptor)
    {
        CHX_LOG_INFO("Dependency Met for HWMFStageBlendLoop stageId %d sequenceId %d",
            stageId, stageSequenceId);

        result = GetOutputPortsForStage(stageId, &outputList);

        if (CDKResultSuccess == result)
        {
            result = GetInputPortsForStage(stageId, &inputList);
        }
        if (CDKResultSuccess == result)
        {
            pHWMFFeatureInfo =
                static_cast<ChiFeature2HWMFFeatureInfo*>(GetFeaturePrivContext(pRequestObject));

            if (NULL != pHWMFFeatureInfo)
            {
                pHWMFFeatureInfo->currentHWMFStage = HWMFStageBlendLoop;
                pHWMFFeatureInfo->tuningMode = static_cast<UINT32>(ChiModeFeature2SubModeType::MFNRBlend);
                SetFeaturePrivContext(pRequestObject, pHWMFFeatureInfo);
            }

            if (TRUE == m_disableDS64Port)
            {
                std::vector<ChiFeature2Identifier> filteredList = SelectInputPorts(&inputList, pRequestObject);

                ChiFeature2PortIdList filtered = {0};

                filtered.numPorts = filteredList.size();
                filtered.pPorts   = filteredList.data();
                PopulatePortConfiguration(pRequestObject, &filtered, &outputList);
            }
            else
            {
                PopulatePortConfiguration(pRequestObject, &inputList, &outputList);
            }

            if (NULL != pHWMFFeatureInfo)
            {
                if (TRUE == pHWMFFeatureInfo->bIsRDIRequiredForPostFilterStage)
                {
                    numBlendLoops = pHWMFFeatureInfo->hwmfTotalNumFrames - 4;
                }
                else
                {
                    numBlendLoops = pHWMFFeatureInfo->hwmfTotalNumFrames - 3;
                }

                pRequestObject->SetBufferInfo(ChiFeature2RequestObjectOpsType::InputConfiguration,
                                              &blendRDIportidentifier,
                                              pHWMFFeatureInfo->inputRDIBufferInfo[stageId + stageSequenceId].hBuffer,
                                              pHWMFFeatureInfo->inputRDIBufferInfo[stageId + stageSequenceId].key,
                                              FALSE,
                                              0,
                                              0);

                pRequestObject->SetBufferInfo(ChiFeature2RequestObjectOpsType::InputConfiguration,
                                              &blendMetaportidentifier,
                                              pHWMFFeatureInfo->inputRDIMetaInfo[stageId + stageSequenceId].hBuffer,
                                              pHWMFFeatureInfo->inputRDIMetaInfo[stageId + stageSequenceId].key,
                                              FALSE,
                                              0,
                                              0);

                pHWMFFeatureInfo->blendHWMFStageBuffersExpected = outputList.numPorts - 1;

                SetFeaturePrivContext(pRequestObject, pHWMFFeatureInfo);
                CHX_LOG_INFO("Submit HWMFStageBlendLoop stageId %d sequenceId %d", stageId, stageSequenceId);
                result = SubmitRequestToSession(pRequestObject);
            }

            if ((CDKResultSuccess == result) && (stageSequenceId < numBlendLoops))
            {
                pStageDescriptor = GetStageDescriptor(stageId);
                if (NULL != pStageDescriptor)
                {
                    ++stageSequenceId;
                    numDependencyLists = GetNumDependencyListsFromStageDescriptor(pStageDescriptor, 0, 0);
                    SetNextStageInfoFromStageDescriptor(pRequestObject, pStageDescriptor,
                        stageSequenceId, numDependencyLists);
                    if (TRUE == m_disableDS64Port)
                    {
                        const ChiFeature2InputDependency* pInputDependency =
                            GetDependencyListFromStageDescriptor(pStageDescriptor, 0, 0, 1);
                        if (NULL != pInputDependency)
                        {
                            PopulateDependencyPorts(pRequestObject, 0, pInputDependency);
                        }
                    }
                    else
                    {
                        const ChiFeature2InputDependency* pInputDependency =
                            GetDependencyListFromStageDescriptor(pStageDescriptor, 0, 0, 0);
                        if (NULL != pInputDependency)
                        {
                            PopulateDependencyPorts(pRequestObject, 0, pInputDependency);
                        }
                    }
                    CHX_LOG_INFO("Wait for Loop Dependency  stageId %d sequenceId %d",
                        stageId, stageSequenceId);
                }
            }
            else if (nextStageId < GetNumStages())
            {
                pStageDescriptor = GetStageDescriptor(nextStageId);
                if (NULL != pStageDescriptor)
                {
                    numDependencyLists = GetNumDependencyListsFromStageDescriptor(pStageDescriptor, 0, 0);
                    SetNextStageInfoFromStageDescriptor(pRequestObject, pStageDescriptor,
                        0, numDependencyLists);
                    if (TRUE == m_disableDS64Port)
                    {
                        const ChiFeature2InputDependency* pInputDependency =
                            GetDependencyListFromStageDescriptor(pStageDescriptor, 0, 0, 1);
                        if (NULL != pInputDependency)
                        {
                            PopulateDependencyPorts(pRequestObject, 0, pInputDependency);
                        }
                    }
                    else
                    {
                        const ChiFeature2InputDependency* pInputDependency =
                            GetDependencyListFromStageDescriptor(pStageDescriptor, 0, 0, 0);
                        if (NULL != pInputDependency)
                        {
                            PopulateDependencyPorts(pRequestObject, 0, pInputDependency);
                        }
                    }
                    CHX_LOG_INFO("Wait for Dependency  for HWMFStagePostfilter stageId %d sequenceId 0",
                        nextStageId);
                }
            }
        }
        else
        {
            CHX_LOG_ERROR("Failure to get I/O port info for HWMFStageBlendLoop stage %d stageSequenceId %d",
                stageId, stageSequenceId);
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid stage descriptor for HWMFStageBlendLoop stage %d stageSequenceId %d",
            stageId, stageSequenceId);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::ProcessHWMFPostfilterStage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2HWmultiframe::ProcessHWMFPostfilterStage(
    ChiFeature2RequestObject * pRequestObject,
    UINT8                      stageId,
    UINT8                      stageSequenceId,
    UINT8                      nextStageId
    )const
{
    CDK_UNUSED_PARAM(nextStageId);

    CDKResult result                                    = CDKResultSuccess;
    ChiFeature2BufferMetadataInfo inputBufferInfo       = {0};
    ChiFeature2BufferMetadataInfo inputMetaInfo         = {0};
    UINT8      numDependencyLists                       = 0;

    ChiFeature2PortIdList               outputList      = {0};
    ChiFeature2PortIdList               inputList       = {0};

    ChiFeature2HWMFFeatureInfo*         pHWMFFeatureInfo    = NULL;
    const ChiFeature2StageDescriptor*   pStageDescriptor    = NULL;

    pStageDescriptor = GetStageDescriptor(stageId);

    if (NULL != pStageDescriptor)
    {
        CHX_LOG_INFO("Dependency Met for HWMFStagePostfilter stageId %d sequenceId %d",
            stageId, stageSequenceId);
        result = GetOutputPortsForStage(stageId, &outputList);

        if (CDKResultSuccess == result)
        {
            result = GetInputPortsForStage(stageId, &inputList);
        }
        if (CDKResultSuccess == result)
        {
            pHWMFFeatureInfo =
                static_cast<ChiFeature2HWMFFeatureInfo*>(GetFeaturePrivContext(pRequestObject));

            if (NULL != pHWMFFeatureInfo)
            {
                pHWMFFeatureInfo->currentHWMFStage = HWMFStagePostfilter;
                pHWMFFeatureInfo->tuningMode       = static_cast<UINT32>(ChiModeFeature2SubModeType::MFNRPostFilter);
                SetFeaturePrivContext(pRequestObject, pHWMFFeatureInfo);
            }
            if (TRUE == m_disableDS64Port)
            {
                std::vector<ChiFeature2Identifier> filteredList = SelectInputPorts(&inputList, pRequestObject);

                ChiFeature2PortIdList filtered = {0};

                filtered.numPorts = filteredList.size();
                filtered.pPorts   = filteredList.data();
                PopulatePortConfiguration(pRequestObject, &filtered, &outputList);
            }
            else
            {
                PopulatePortConfiguration(pRequestObject, &inputList, &outputList);
            }

            ChiFeature2Identifier  postfilterRawportidentifier  = {0};
            ChiFeature2Identifier  postfilterMetaportidentifier = {0};

            postfilterRawportidentifier  = inputList.pPorts[0];
            postfilterMetaportidentifier = inputList.pPorts[inputList.numPorts-1];

            if (NULL != pHWMFFeatureInfo)
            {
                if (TRUE == pHWMFFeatureInfo->bIsRDIRequiredForPostFilterStage)
                {
                    pRequestObject->SetBufferInfo(ChiFeature2RequestObjectOpsType::InputConfiguration,
                        &postfilterRawportidentifier,
                        pHWMFFeatureInfo->inputRDIBufferInfo[pHWMFFeatureInfo->hwmfTotalNumFrames-1].hBuffer,
                        pHWMFFeatureInfo->inputRDIBufferInfo[pHWMFFeatureInfo->hwmfTotalNumFrames-1].key,
                        FALSE,
                        0,
                        0);
                }

                pRequestObject->SetBufferInfo(ChiFeature2RequestObjectOpsType::InputConfiguration,
                    &postfilterMetaportidentifier,
                    pHWMFFeatureInfo->inputRDIMetaInfo[pHWMFFeatureInfo->hwmfTotalNumFrames-1].hBuffer,
                    pHWMFFeatureInfo->inputRDIMetaInfo[pHWMFFeatureInfo->hwmfTotalNumFrames-1].key,
                    FALSE,
                    0,
                    0);

                pHWMFFeatureInfo->postFilterHWMFStageBuffersExpected = outputList.numPorts - 1;
            }

            SetFeaturePrivContext(pRequestObject, pHWMFFeatureInfo);
            CHX_LOG_INFO("Submit HWMFStagePostfilter stageId %d sequenceId %d", stageId, stageSequenceId);
            result = SubmitRequestToSession(pRequestObject);
        }
        else
        {
            CHX_LOG_ERROR("Failure to get I/O port info for HWMFStagePostfilter stage %d stageSequenceId %d",
                stageId, stageSequenceId);
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid stage descriptor for HWMFStagePostfilter stage %d stageSequenceId %d",
            stageId, stageSequenceId);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::OnPopulateDependencySettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2HWmultiframe::OnPopulateDependencySettings(
    ChiFeature2RequestObject*     pRequestObject,
    UINT8                         dependencyIndex,
    const ChiFeature2Identifier*  pSettingPortId,
    ChiMetadata*                  pFeatureSettings
    ) const
{
    CDKResult result = ChiFeature2Base::OnPopulateDependencySettings(
        pRequestObject, dependencyIndex, pSettingPortId, pFeatureSettings);
    ChiFeature2Hint* pFeatureHint = pRequestObject->GetFeatureHint();

    if (TRUE == m_disableZoomCrop)
    {
        BOOL bDisableZoomCrop = TRUE;
        if (NULL != pFeatureSettings)
        {
            result = pFeatureSettings->SetTag("org.quic.camera2.ref.cropsize", "DisableZoomCrop",
                                              &bDisableZoomCrop, 1);
            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("Cannot set disable Zoom crop into metadata");
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::OnPopulateConfigurationSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2HWmultiframe::OnPopulateConfigurationSettings(
    ChiFeature2RequestObject*     pRequestObject,
    const ChiFeature2Identifier*  pMetadataPortId,
    ChiMetadata*                  pInputMetadata
    ) const
{

    CDKResult                   result                          = CDKResultSuccess;
    ChiFeature2HWMFFeatureInfo* pHWMFFeatureInfo                = NULL;
    ChiFeature2UsecaseRequestObject*    pUsecaseRequestObject   = NULL;
    UINT32 effectmode                                           = ANDROID_CONTROL_EFFECT_MODE_OFF;
    UINT32 scenemode                                            = ANDROID_CONTROL_SCENE_MODE_DISABLED;
    UINT32 feature1mode                                         = 0;
    UINT32 sensorModeIndex;

    result = ChiFeature2Base::OnPopulateConfigurationSettings(
        pRequestObject, pMetadataPortId, pInputMetadata);

    if (NULL != pInputMetadata)
    {
        pHWMFFeatureInfo =
            static_cast<ChiFeature2HWMFFeatureInfo*>(GetFeaturePrivContext(pRequestObject));

        if (pHWMFFeatureInfo != NULL)
        {
            if (FALSE == pHWMFFeatureInfo->bIsRDIRequiredForPostFilterStage)
            {
                if (pHWMFFeatureInfo->currentHWMFStage == HWMFStageBlendInit)
                {
                    if (NULL != pHWMFFeatureInfo->pHWMFPrefilterStageMeta)
                    {
                        // Copy Prefilter stage Meta in priv context
                        // Prefilter stage Meta takes the priority
                        pHWMFFeatureInfo->pHWMFPrefilterStageMeta->Copy(*pInputMetadata, FALSE);
                        SetFeaturePrivContext(pRequestObject, pHWMFFeatureInfo);
                    }
                }
                else if ((pHWMFFeatureInfo->currentHWMFStage == HWMFStageBlendLoop) ||
                    (pHWMFFeatureInfo->currentHWMFStage == HWMFStagePostfilter))
                {
                    if (NULL != pHWMFFeatureInfo->pHWMFPrefilterStageMeta)
                    {
                        // Copy prefilter stage meta to Blend loop and Postfilter stages
                        // InputMetaData takes the priority
                        pInputMetadata->Copy(*pHWMFFeatureInfo->pHWMFPrefilterStageMeta, TRUE);
                    }
                }
            }

            ChiFeature2StageInfo  stageInfo   = { 0 };
            UINT8                 requestId   = 0;

            requestId = pRequestObject->GetCurRequestId();
            pRequestObject->GetCurrentStageInfo(&stageInfo, requestId);
            UINT8 index = (stageInfo.stageId + stageInfo.stageSequenceId) %
                pHWMFFeatureInfo->hwmfTotalNumFrames;
            BOOL  fetchRdiMetadata = TRUE;

            if ((TRUE == pHWMFFeatureInfo->bIsRDIRequiredForPostFilterStage) &&
                (HWMFStagePostfilter == stageInfo.stageId))
            {
                index = pHWMFFeatureInfo->hwmfTotalNumFrames - 1;
            }

            if ((FALSE == pHWMFFeatureInfo->bIsRDIRequiredForPostFilterStage) &&
                (HWMFStagePostfilter == stageInfo.stageId))
            {
                fetchRdiMetadata = FALSE;
            }

            if (TRUE == fetchRdiMetadata)
            {
                ChiFeature2BufferMetadataInfo* pBufMetaInfo =
                    &pHWMFFeatureInfo->inputRDIMetaInfo[index];
                if (NULL != pBufMetaInfo->hBuffer)
                {
                    CHIMETAHANDLE hMetadataBuffer = NULL;
                    result = GetMetadataBuffer(pBufMetaInfo->hBuffer,
                        pBufMetaInfo->key, &hMetadataBuffer);

                    if (NULL != hMetadataBuffer)
                    {
                        ChiMetadata* pResultMetadata =
                            GetMetadataManager()->GetMetadataFromHandle(hMetadataBuffer);
                        if (NULL != pResultMetadata)
                        {
                            // bring in current metatdata info to pInputMetadata
                            pInputMetadata->Copy(*pResultMetadata, FALSE);
                        }
                    }
                }
            }

            if (FALSE == pHWMFFeatureInfo->bIsRDIRequiredForPostFilterStage)
            {
                pInputMetadata->SetTag("org.quic.camera2.mfsrconfigs", "MFSRTotalNumFrames",
                    &pHWMFFeatureInfo->hwmfTotalNumFrames, 1);

                pInputMetadata->SetTag("org.quic.camera.snapshotOutputDimension", "StillOutputDimension",
                    &pHWMFFeatureInfo->finalHWMFDimensions, sizeof(GeoLibStreamOutput));

                if ((pHWMFFeatureInfo->currentHWMFStage == HWMFStagePostfilter) ||
                    (pHWMFFeatureInfo->currentHWMFStage == HWMFStagePrefilter))
                {
                    // for Prefilter/postfilter value has to be 1
                    pHWMFFeatureInfo->hwmfCurrentStageCount = 1;
                }
                else
                {
                    pHWMFFeatureInfo->hwmfCurrentStageCount = index;
                    CHX_LOG_VERBOSE("tag index %d and stage %d ", pHWMFFeatureInfo->hwmfCurrentStageCount,
                                     pHWMFFeatureInfo->currentHWMFStage);
                }
                pInputMetadata->SetTag("org.quic.camera2.mfsrconfigs", "MFSRBlendFrameNum",
                &pHWMFFeatureInfo->hwmfCurrentStageCount, 1);
            }
            else
            {
                pInputMetadata->SetTag("org.quic.camera2.mfnrconfigs", "MFNRTotalNumFrames",
                    &pHWMFFeatureInfo->hwmfTotalNumFrames, 1);

                if (pHWMFFeatureInfo->currentHWMFStage == HWMFStagePrefilter)
                {
                    // for Prefilter value has to be 1
                    pHWMFFeatureInfo->hwmfCurrentStageCount = 1;
                }
                else if ((pHWMFFeatureInfo->currentHWMFStage == HWMFStageBlendInit) ||
                    (pHWMFFeatureInfo->currentHWMFStage == HWMFStageBlendLoop))
                {
                    pHWMFFeatureInfo->hwmfCurrentStageCount = index;
                }
                else if (pHWMFFeatureInfo->currentHWMFStage == HWMFStagePostfilter)
                {
                    pHWMFFeatureInfo->hwmfCurrentStageCount = pHWMFFeatureInfo->hwmfTotalNumFrames - 1;
                }

                pInputMetadata->SetTag("org.quic.camera2.mfnrconfigs", "MFNRBlendFrameNum",
                &pHWMFFeatureInfo->hwmfCurrentStageCount, 1);
            }

            pUsecaseRequestObject = pRequestObject->GetUsecaseRequestObject();

            sensorModeIndex = GetSensorModeIndex(pMetadataPortId);

            if (NULL != pUsecaseRequestObject)
            {
                ChxUtils::FillTuningModeData(pInputMetadata,
                    pUsecaseRequestObject->GetCaptureRequest(),
                    sensorModeIndex,
                    &effectmode,
                    &scenemode,
                    &feature1mode,
                    &pHWMFFeatureInfo->tuningMode);
            }
        }
        else
        {
            CHX_LOG_ERROR("Failed to get private context");
        }

    }
    else
    {
        CHX_LOG_ERROR("Failed to set Total Number of Vendor Tag");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::OnBufferResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2HWmultiframe::OnBufferResult(
    ChiFeature2RequestObject*  pRequestObject,
    UINT8                      resultId,
    ChiFeature2StageInfo*      pStageInfo,
    ChiFeature2Identifier*     pPortIdentifier,
    const CHISTREAMBUFFER*     pStreamBuffer,
    UINT32                     frameNumber,
    VOID*                      pPrivateData)
{
    BOOL sendToGraph = ChiFeature2Base::OnBufferResult(pRequestObject, resultId, pStageInfo, pPortIdentifier,
        pStreamBuffer, frameNumber, pPrivateData);

    CHX_LOG_INFO("Received buffer callback on port %d %d %d stream %p", pPortIdentifier->session,
        pPortIdentifier->pipeline, pPortIdentifier->port, pStreamBuffer->pStream);

    ChiFeature2HWMFFeatureInfo* pHWMFFeatureInfo = NULL;

    pHWMFFeatureInfo =
        static_cast<ChiFeature2HWMFFeatureInfo*>(GetFeaturePrivContext(pRequestObject));

    if (pHWMFFeatureInfo != NULL)
    {
        if (HWMFStagePrefilter == pStageInfo->stageId)
        {
            if (pHWMFFeatureInfo->preFilterHWMFStageBuffersExpected > 0)
            {
                pHWMFFeatureInfo->preFilterHWMFStageBuffersExpected--;
            }

            if (pHWMFFeatureInfo->preFilterHWMFStageBuffersExpected == 0)
            {
                CHX_LOG_INFO(" MFSR stage %d stageSequenceId %d all buffers received",
                    pStageInfo->stageId, pStageInfo->stageSequenceId);
                DeActivatePipeline(pPortIdentifier, CHIDeactivateModeReleaseBuffer);

                ATRACE_ASYNC_END("ChiFeature2HWmultiframe::ProcessMfsrPrefilterStage",
                    HWMFStagePrefilter);

            }
        }
        else if (((HWMFStageBlendLoop == pStageInfo->stageId) &&
            (pStageInfo->stageSequenceId == pHWMFFeatureInfo->hwmfTotalNumFrames - 3) &&
            (FALSE == pHWMFFeatureInfo->bIsRDIRequiredForPostFilterStage)) ||
            ((HWMFStageBlendLoop == pStageInfo->stageId) &&
            (pStageInfo->stageSequenceId == pHWMFFeatureInfo->hwmfTotalNumFrames - 4) &&
            (TRUE == pHWMFFeatureInfo->bIsRDIRequiredForPostFilterStage)) ||
            ((HWMFStageBlendInit == pStageInfo->stageId) &&
            (TRUE == pHWMFFeatureInfo->bIsRDIRequiredForPostFilterStage) &&
            (pHWMFFeatureInfo->hwmfTotalNumFrames == 3)))
        {
            if (pHWMFFeatureInfo->blendHWMFStageBuffersExpected > 0)
            {
                pHWMFFeatureInfo->blendHWMFStageBuffersExpected--;
            }

            if (pHWMFFeatureInfo->blendHWMFStageBuffersExpected == 0)
            {
                CHX_LOG_INFO(" MFSR stage %d stageSequenceId %d all buffers received",
                    pStageInfo->stageId, pStageInfo->stageSequenceId);
                DeActivatePipeline(pPortIdentifier, CHIDeactivateModeReleaseBuffer);

                ATRACE_ASYNC_END("ChiFeature2HWmultiframe::ProcessHWMFBlendLoopStage",
                        HWMFStageBlendLoop);
            }

        }
        else if (HWMFStagePostfilter == pStageInfo->stageId)
        {
            if (pHWMFFeatureInfo->postFilterHWMFStageBuffersExpected > 0)
            {
                pHWMFFeatureInfo->postFilterHWMFStageBuffersExpected--;
            }

            if (pHWMFFeatureInfo->postFilterHWMFStageBuffersExpected == 0)
            {
                CHX_LOG_INFO(" MFSR stage %d stageSequenceId %d all buffers received",
                    pStageInfo->stageId, pStageInfo->stageSequenceId);
                DeActivatePipeline(pPortIdentifier, CHIDeactivateModeReleaseBuffer);

                ATRACE_ASYNC_END("ChiFeature2HWmultiframe::ProcessHWMFPostfilterStage",
                    HWMFStagePostfilter);
            }
        }
    }

    return sendToGraph;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::OnReleaseDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2HWmultiframe::OnReleaseDependencies(
    const ChiFeature2Identifier*    pPortIdentifier,
    UINT8                           dependencyIndex,
    ChiFeature2StageInfo*           pStageInfo,
    ChiFeature2RequestObject*       pRequestObject
    ) const
{
    ChiFeature2HWMFFeatureInfo* pHWMFFeatureInfo = NULL;
    BOOL                        releasePort       = FALSE;

    pHWMFFeatureInfo =
            static_cast<ChiFeature2HWMFFeatureInfo*>(GetFeaturePrivContext(pRequestObject));

    if ((NULL != pHWMFFeatureInfo) && (NULL != pStageInfo) && (NULL != pPortIdentifier))
    {
        if (ChiFeature2PortDirectionType::ExternalInput == pPortIdentifier->portDirectionType)
        {
            UINT8 releaseIndex = (pStageInfo->stageId + pStageInfo->stageSequenceId) % pHWMFFeatureInfo->hwmfTotalNumFrames;

            if ((TRUE == pHWMFFeatureInfo->bIsRDIRequiredForPostFilterStage) &&
                (HWMFStagePostfilter == pStageInfo->stageId))
            {
                releaseIndex = pHWMFFeatureInfo->hwmfTotalNumFrames - 1;
            }

            if (dependencyIndex == releaseIndex)
            {
                CHX_LOG_INFO("Releasing port %d %d %d dependencyIndex %d", pPortIdentifier->session, pPortIdentifier->pipeline,
                    pPortIdentifier->port, dependencyIndex);
                releasePort = TRUE;
            }
        }
        else if (ChiFeature2PortDirectionType::InternalInput == pPortIdentifier->portDirectionType)
        {
            if ((*pPortIdentifier == HWMFBlendInitInputPortUBWC) ||
                (*pPortIdentifier == HWMFBlendLoopInputPortUBWC))
            {
                CHX_LOG_INFO("Releasing port %d %d %d dependencyIndex %d", pPortIdentifier->session, pPortIdentifier->pipeline,
                    pPortIdentifier->port, dependencyIndex);
                releasePort = TRUE;
            }
            else if ((HWMFStageBlendLoop        == pStageInfo->stageId) &&
                (pStageInfo->stageSequenceId    == pHWMFFeatureInfo->hwmfTotalNumFrames - 3) &&
                (FALSE                          == pHWMFFeatureInfo->bIsRDIRequiredForPostFilterStage))
            {
                CHX_LOG_INFO("Releasing port %d %d %d dependencyIndex %d", pPortIdentifier->session, pPortIdentifier->pipeline,
                    pPortIdentifier->port, dependencyIndex);
                releasePort = TRUE;
            }
            else if (HWMFStagePostfilter == pStageInfo->stageId)
            {
                CHX_LOG_INFO("Releasing port %d %d %d dependencyIndex %d", pPortIdentifier->session, pPortIdentifier->pipeline,
                    pPortIdentifier->port, dependencyIndex);
                releasePort = TRUE;
            }
        }

    }
    else
    {
        CHX_LOG_WARN("NULL parameters pHWMFFeatureInfo %p pStageInfo %p pPortIdentifier %p",
            pHWMFFeatureInfo, pStageInfo, pPortIdentifier);
    }

    return releasePort;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::OnMetadataResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2HWmultiframe::OnMetadataResult(
    ChiFeature2RequestObject*  pRequestObject,
    UINT8                      resultId,
    ChiFeature2StageInfo*      pStageInfo,
    ChiFeature2Identifier*     pPortIdentifier,
    ChiMetadata*               pMetadata,
    UINT32                     frameNumber,
    VOID*                      pPrivateData)
{
    BOOL                sendToGraph         = FALSE;
    ExtensionModule*    pExtensionModule    = ExtensionModule::GetInstance();

    if (DumpDebugDataPerSnapshot <= pExtensionModule->EnableDumpDebugData())
    {
        ProcessDebugData(pRequestObject,
                         pStageInfo,
                         pPortIdentifier,
                         pMetadata,
                         frameNumber);
    }

    sendToGraph = ChiFeature2Base::OnMetadataResult(pRequestObject,
                                                    resultId,
                                                    pStageInfo,
                                                    pPortIdentifier,
                                                    pMetadata,
                                                    frameNumber,
                                                    pPrivateData);

    return sendToGraph;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::DoCleanupRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2HWmultiframe::DoCleanupRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDK_UNUSED_PARAM(pRequestObject);

    ChiFeature2HWMFFeatureInfo* pHWMFFeatureInfo;
    if (NULL != pRequestObject)
    {
        pHWMFFeatureInfo =
            static_cast<ChiFeature2HWMFFeatureInfo*>(GetFeaturePrivContext(pRequestObject));

        if (pHWMFFeatureInfo != NULL)
        {
            if (NULL != pHWMFFeatureInfo->pHWMFPrefilterStageMeta)
            {
                pHWMFFeatureInfo->pHWMFPrefilterStageMeta->Destroy();
                pHWMFFeatureInfo->pHWMFPrefilterStageMeta = NULL;
            }
            CHX_FREE(pHWMFFeatureInfo);
            pHWMFFeatureInfo = NULL;
            SetFeaturePrivContext(pRequestObject, NULL);
        }

        VOID* pPrivateInterData = pRequestObject->GetInterFeatureRequestPrivateData(m_featureId,
                                                                                   m_pInstanceProps->cameraId,
                                                                                   m_pInstanceProps->instanceId);
        if (NULL != pPrivateInterData)
        {
            CHX_FREE(pPrivateInterData);
            pPrivateInterData = NULL;
            pRequestObject->SetInterFeatureRequestPrivateData(m_featureId,
                                                              m_pInstanceProps->cameraId,
                                                              m_pInstanceProps->instanceId,
                                                              pPrivateInterData);
        }
    }
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::SelectOutputPorts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 std::vector<ChiFeature2Identifier> ChiFeature2HWmultiframe::SelectOutputPorts(
    ChiFeature2PortIdList*     pAllOutputPorts,
    ChiFeature2RequestObject*  pRequestObject
    ) const
{
    CDK_UNUSED_PARAM(pRequestObject);
    // Need to Feed Correct DS64 output port for Prefilter
    ChiFeature2Identifier preFilterOutDS64PortIdentifier = { 0, 0, 3, ChiFeature2PortDirectionType::InternalOutput };
    std::vector<ChiFeature2Identifier> selectedList;
    selectedList.reserve(pAllOutputPorts->numPorts);

    for (UINT8 portIndex = 0; portIndex < pAllOutputPorts->numPorts; portIndex++)
    {
        ChiFeature2Identifier portId = pAllOutputPorts->pPorts[portIndex];

        if (portId != preFilterOutDS64PortIdentifier)
        {
            selectedList.push_back(portId);
        }
    }

    return selectedList;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::SelectInputPorts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 std::vector<ChiFeature2Identifier> ChiFeature2HWmultiframe::SelectInputPorts(
    ChiFeature2PortIdList*     pAllInputPorts,
    ChiFeature2RequestObject*  pRequestObject
    ) const
{
    // Need to Feed Correct DS64 Input port for Blend
    ChiFeature2Identifier blendInDS64PortIdentifier = { 0, 1, 4, ChiFeature2PortDirectionType::InternalInput };
    ChiFeature2Identifier postfilterInDS64PortIdentifier = { 0, 2, 4, ChiFeature2PortDirectionType::InternalInput };
    std::vector<ChiFeature2Identifier> selectedList;
    selectedList.reserve(pAllInputPorts->numPorts);

    ChiFeature2HWMFFeatureInfo* pHWMFFeatureInfo;
    if (NULL != pRequestObject)
    {
        pHWMFFeatureInfo =
            static_cast<ChiFeature2HWMFFeatureInfo*>(GetFeaturePrivContext(pRequestObject));

        if (pHWMFFeatureInfo != NULL)
        {

            for (UINT8 portIndex = 0; portIndex < pAllInputPorts->numPorts; portIndex++)
            {
                ChiFeature2Identifier portId = pAllInputPorts->pPorts[portIndex];

                if (portId != blendInDS64PortIdentifier && portId != postfilterInDS64PortIdentifier)
                {
                    selectedList.push_back(portId);
                }
            }
        }
        else
        {
            CHX_LOG_ERROR("Failed to get private context");
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid Request Object");
    }

    return selectedList;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::DoFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2HWmultiframe::DoFlush()
{
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::DumpInputMetaBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2HWmultiframe::DumpInputMetaBuffer(
    ChiFeature2BufferMetadataInfo* pInputMetaInfo,
    CHAR*                          pBaseName,
    UINT                           index
    ) const
{
    CDKResult               result       = CDKResultSuccess;
    ChiMetadata*            pMetadata    = NULL;
    CHITargetBufferManager* pMetadataTBM = NULL;

    if ((NULL == pInputMetaInfo) || (NULL == pBaseName))
    {
        result = CDKResultEInvalidArg;
    }

    // Dump metadata to file
    if (CDKResultSuccess == result)
    {
        pMetadataTBM = CHITargetBufferManager::GetTargetBufferManager(pInputMetaInfo->hBuffer);
    }

    if (NULL != pMetadataTBM)
    {
        pMetadata = static_cast<ChiMetadata*>(pMetadataTBM->GetTarget(
            pInputMetaInfo->hBuffer, pInputMetaInfo->key));
    }
    else
    {
        result = CDKResultEFailed;
    }

    if ((CDKResultSuccess == result) && (NULL != pMetadata))
    {
        CHAR metaName[256] = { 0 };

        // Determine the metadata dump file name
        CdkUtils::SNPrintF(metaName, sizeof(metaName), "%s_%d.bin", pBaseName, index);

        DumpMetadata(pMetadata, metaName);
    }

    if (CDKResultSuccess != result)
    {
        CHX_LOG_ERROR("DumpInputMetaBuffer failed!");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::DumpInputImageBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2HWmultiframe::DumpInputImageBuffer(
    ChiFeature2BufferMetadataInfo* pInputBufferInfo,
    CHAR*                          pBaseName,
    UINT                           index
    ) const
{
    CDKResult               result        = CDKResultSuccess;
    CHITargetBufferManager* pImageTBM     = NULL;
    CHISTREAMBUFFER*        pStreamBuffer = NULL;

    if (NULL == pInputBufferInfo)
    {
        result = CDKResultEInvalidArg;
    }

    // Dump raw image to file
    if (CDKResultSuccess == result)
    {
        pImageTBM = CHITargetBufferManager::GetTargetBufferManager(pInputBufferInfo->hBuffer);
    }

    if (NULL != pImageTBM)
    {
        pStreamBuffer = static_cast<CHISTREAMBUFFER*>(pImageTBM->GetTarget(
            pInputBufferInfo->hBuffer, pInputBufferInfo->key));
    }
    else
    {
        result = CDKResultEFailed;
    }

    if ((CDKResultSuccess == result) && (NULL != pStreamBuffer))
    {
        CHX_LOG_INFO("Dump image format=%d, WxH=%dx%d, planeStride=%d, sliceHeight=%d",
                        pStreamBuffer->pStream->format,
                        pStreamBuffer->pStream->width,
                        pStreamBuffer->pStream->height,
                        pStreamBuffer->pStream->streamParams.planeStride,
                        pStreamBuffer->pStream->streamParams.sliceHeight);

        CHAR imageName[256] = { 0 };

        // Determine the image file dump name
        CdkUtils::SNPrintF(imageName, sizeof(imageName),
                           "/data/vendor/camera/%s_%d.raw",
                           pBaseName, index);

        DumpRawImage(pStreamBuffer, imageName);
    }

    if (CDKResultSuccess != result)
    {
        CHX_LOG_ERROR("DumpInputImageBuffer failed!");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::GetUseCaseString
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2HWmultiframe::GetUseCaseString(
    ChiModeUsecaseSubModeType useCase,
    std::string&              output
    ) const
{
    switch (useCase)
    {
        case ChiModeUsecaseSubModeType::Preview:
        {
            output = "P";

            break;
        }

        case ChiModeUsecaseSubModeType::Snapshot:
        {
            output = "S";

            break;
        }

        case ChiModeUsecaseSubModeType::Video:
        {
            output = "V";

            break;
        }

        case ChiModeUsecaseSubModeType::ZSL:
        {
            output = "Z";

            break;
        }

        case ChiModeUsecaseSubModeType::Liveshot:
        {
            output = "L";

            break;
        }

        default:
        {
            output = "ERR";

            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HWmultiframe::GetDumpFileBaseName
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2HWmultiframe::GetDumpFileBaseName(
    ChiFeature2RequestObject* pRequestObject,
    CHAR*                     pDumpFileBaseName,
    UINT                      size
    ) const
{
    CHIDateTime dateTime              = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    ChiModeUsecaseSubModeType useCase = pRequestObject->GetUsecaseRequestObject()->GetUsecaseMode();
    std::string useCaseString         = "ERR";
    UINT32 requestID                  = pRequestObject->GetUsecaseRequestObject()->GetAppFrameNumber();

    // Get the current data and time
    CdkUtils::GetDateTime(&dateTime);

    // Get the string associated with the current use case
    GetUseCaseString(useCase, useCaseString);

    CdkUtils::SNPrintF(pDumpFileBaseName, sizeof(CHAR) * size, "Cam%d_L%dI%d_%s_RQ%d_%04d%02d%02d%02d%02d%02d",
                       m_pCameraInfo->ppDeviceInfo[m_pInstanceProps->cameraId]->cameraId, m_pCameraInfo->cameraId,
                       m_pInstanceProps->cameraId, useCaseString.c_str(), requestID,
                       dateTime.year + 1900, dateTime.month + 1, dateTime.dayOfMonth, dateTime.hours,
                       dateTime.minutes, dateTime.seconds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateFeature
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2HWmultiframe::OnPruneUsecaseDescriptor(
    const ChiFeature2CreateInputInfo*  pCreateInputInfo,
    std::vector<PruneVariant>&         rPruneVariants
    ) const
{
    BOOL shouldDisableDS64 = FALSE;

    for (UINT i = 0; i < pCreateInputInfo->pStreamConfig->numStreams; i++)
    {
        if (ShouldDisableDS64(pCreateInputInfo->pStreamConfig->pChiStreams[i]))
        {
            shouldDisableDS64 = TRUE;
            break;
        }
    }

    PruneVariant pruneVariant;
    pruneVariant.group = UsecaseSelector::GetVariantGroup("DS64");
    pruneVariant.type  = UsecaseSelector::GetVariantType((FALSE == shouldDisableDS64) ? "Enabled" : "Disabled");

    rPruneVariants.reserve(1);
    rPruneVariants.push_back(pruneVariant);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateFeature
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* CreateFeature(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    ChiFeature2HWmultiframe* pFeatureRealtime = ChiFeature2HWmultiframe::Create(pCreateInputInfo);
    return static_cast<CHIHANDLE>(pFeatureRealtime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DoQueryCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DoQueryCaps(
    VOID*                 pConfig,
    ChiFeature2QueryInfo* pQueryInfo)
{
    CDK_UNUSED_PARAM(pConfig);
    CDKResult result = CDKResultSuccess;

    if (NULL != pQueryInfo)
    {
        pQueryInfo->numCaps        = CHX_ARRAY_SIZE(Feature2HwMultiFrameCaps);
        pQueryInfo->ppCapabilities = &Feature2HwMultiFrameCaps[0];
    }
    else
    {
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetVendorTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID GetVendorTags(
    VOID* pVendorTags)
{
    CDK_UNUSED_PARAM(pVendorTags);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DoStreamNegotiation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DoStreamNegotiation(
    StreamNegotiationInfo*          pNegotiationInfo,
    StreamNegotiationOutput*        pNegotiationOutput)
{
    CDKResult result        = CDKResultSuccess;
    BOOL      isMultiCamera = (pNegotiationInfo->pLogicalCameraInfo->numPhysicalCameras <= 1) ? FALSE : TRUE;
    std::vector<CHISTREAM*> filteredStreams;

    if (NULL == pNegotiationInfo || pNegotiationOutput == NULL)
    {
        CHX_LOG_ERROR("Invalid Arg! pNegotiation=%p, pDesiredStreamConfig=%p",
            pNegotiationInfo, pNegotiationOutput);
        result = CDKResultEInvalidArg;
    }
    else
    {
        // Clone a stream and put it into the list of streams that will be freed by the feature
        auto CloneStream = [&pNegotiationOutput](CHISTREAM* pSrcCameraStream) -> CHISTREAM* {
            ChiStream* pStream = static_cast<CHISTREAM*>(CHX_CALLOC(sizeof(CHISTREAM)));
            ChxUtils::Memcpy(pStream, pSrcCameraStream, sizeof(CHISTREAM));
            pNegotiationOutput->pOwnedStreams->push_back(pStream);
            return pStream;
        };

        ChiFeature2Type featureId    = static_cast<ChiFeature2Type>(pNegotiationInfo->pFeatureInstanceId->featureId);

        const UINT   numStreams         = pNegotiationInfo->pFwkStreamConfig->numStreams;
        CHISTREAM**  ppChiStreams       = pNegotiationInfo->pFwkStreamConfig->pChiStreams;
        UINT8        physicalCamIdx     = pNegotiationInfo->pFeatureInstanceId->cameraId;
        UINT32       physicalCamId      = pNegotiationInfo->pLogicalCameraInfo->ppDeviceInfo[physicalCamIdx]->cameraId;

        ChiFeature2InstanceFlags flags  = pNegotiationInfo->pFeatureInstanceId->flags;

        for (UINT i = 0; i < pNegotiationInfo->pFwkStreamConfig->numStreams; i++)
        {
            INT32 cameraId = -1;
            if ((NULL != pNegotiationInfo->pFwkStreamConfig->pChiStreams[i]->physicalCameraId) &&
                (ChiStreamTypeInput == pNegotiationInfo->pFwkStreamConfig->pChiStreams[i]->streamType))
            {
                cameraId = ChxUtils::GetCameraIdFromStream(pNegotiationInfo->pFwkStreamConfig->pChiStreams[i]);
            }

            if ((NULL != pNegotiationInfo->pFwkStreamConfig->pChiStreams[i]) &&
                (0 != pNegotiationInfo->pFwkStreamConfig->pChiStreams[i]->width) &&
                (0 != pNegotiationInfo->pFwkStreamConfig->pChiStreams[i]->height))
            {
                if ((-1 != cameraId) && (static_cast<UINT32>(cameraId) == physicalCamId))
                {
                    filteredStreams.push_back(pNegotiationInfo->pFwkStreamConfig->pChiStreams[i]);
                }
            }
        }

        // configure based on filtered streams
        if (0 < filteredStreams.size())
        {
            pNegotiationOutput->pDesiredStreamConfig->numStreams       = filteredStreams.size();
            pNegotiationOutput->pDesiredStreamConfig->operationMode    = pNegotiationInfo->pFwkStreamConfig->operationMode;
            pNegotiationOutput->pDesiredStreamConfig->pChiStreams      = &(filteredStreams)[0];
            pNegotiationOutput->pDesiredStreamConfig->pSessionSettings = NULL;

            pNegotiationInfo->pFwkStreamConfig = pNegotiationOutput->pDesiredStreamConfig;
        }

        GetSensorOutputDimension(physicalCamId,
                                 pNegotiationInfo->pFwkStreamConfig,
                                 pNegotiationInfo->pFeatureInstanceId->flags,
                                 &RDIStream.width,
                                 &RDIStream.height);

        pNegotiationOutput->pStreams->clear();

        CHISTREAM* pHWMFInputStream = CloneStream(&MFSRStreamsInput1);

        UINT                 snapshotWidth      = 0;
        UINT                 snapshotHeight     = 0;
        SnapshotStreamConfig snapshotConfig     = {};
        BOOL                 isNativeResolution = FALSE;
        CHISTREAM*           pYUVSnapshotStream = NULL;

        const ChiFeature2InstanceProps* pInstanceProps = pNegotiationInfo->pInstanceProps;

        if (NULL != pInstanceProps)
        {
            for (UINT i = 0; i < pInstanceProps->numFeatureProps; ++i)
            {
                CHX_LOG_INFO("[%d/%d], propId:%d, propName:%s, size:%d, value:%s", i,
                             pInstanceProps->numFeatureProps,
                             pInstanceProps->pFeatureProperties[i].propertyId,
                             pInstanceProps->pFeatureProperties[i].pPropertyName,
                             pInstanceProps->pFeatureProperties[i].size,
                             pInstanceProps->pFeatureProperties[i].pValue);

                if (!CdkUtils::StrCmp(pInstanceProps->pFeatureProperties[i].pPropertyName, "nativeinputresolution"))
                {
                    if (!CdkUtils::StrCmp(static_cast<const CHAR*>(pInstanceProps->pFeatureProperties[i].pValue), "TRUE"))
                    {
                        isNativeResolution = TRUE;
                        break;
                    }
                }
            }
        }

        result = UsecaseSelector::GetSnapshotStreamConfiguration(numStreams, ppChiStreams, snapshotConfig);

        if (NULL != snapshotConfig.pSnapshotStream)
        {
            snapshotWidth  = snapshotConfig.pSnapshotStream->width;
            snapshotHeight = snapshotConfig.pSnapshotStream->height;
        }

        // For multi camera, there is just yuv stream configure
        if (pNegotiationInfo->pLogicalCameraInfo->numPhysicalCameras > 1)
        {
            for (UINT streamIdx = 0; streamIdx < numStreams; streamIdx++)
            {
                if ((FALSE == IsFDStream(ppChiStreams[streamIdx])))
                {
                    if ((ppChiStreams[streamIdx]->streamType == ChiStreamTypeOutput) &&
                        ((NULL == ppChiStreams[streamIdx]->physicalCameraId) ||
                         ((NULL != ppChiStreams[streamIdx]->physicalCameraId) &&
                          ((0 == strlen(ppChiStreams[streamIdx]->physicalCameraId)) ||
                           (ChxUtils::GetCameraIdFromStream(ppChiStreams[streamIdx]) == physicalCamId)))))
                    {
                        snapshotWidth      = ppChiStreams[streamIdx]->width;
                        snapshotHeight     = ppChiStreams[streamIdx]->height;
                        pYUVSnapshotStream = ppChiStreams[streamIdx];
                        break;
                    }
                }
            }
        }
        else
        {
            if (snapshotWidth * snapshotHeight == 0)
            {
                pYUVSnapshotStream            = GetYUVSnapshotStream(pNegotiationInfo->pFwkStreamConfig);
                if (NULL != pYUVSnapshotStream)
                {
                    snapshotWidth      = pYUVSnapshotStream->width;
                    snapshotHeight     = pYUVSnapshotStream->height;
                }
            }
        }

        if (NULL != pHWMFInputStream)
        {
            // configure input stream
            pHWMFInputStream->width  = RDIStream.width;
            pHWMFInputStream->height = RDIStream.height;
            if (flags.isSWRemosaicSnapshot == TRUE)
            {
                pHWMFInputStream->format = ChiStreamFormatRaw16;
            }

            pNegotiationOutput->pStreams->push_back(pHWMFInputStream);
        }

        CHISTREAM* pHWMFOutputStream = CloneStream(&MFSRStreamsOutput1);

        if (NULL != pHWMFOutputStream)
        {
            // configure output stream
            if (TRUE == isNativeResolution)
            {
                pHWMFOutputStream->width = RDIStream.width;
                pHWMFOutputStream->height = RDIStream.height;
                pNegotiationOutput->pStreams->push_back(pHWMFOutputStream);
            }
            else if ((0 < snapshotWidth) && (0 < snapshotHeight))
            {
                if (NULL != pYUVSnapshotStream)
                {
                    pNegotiationOutput->pStreams->push_back(pYUVSnapshotStream);
                }
                else
                {
                    pHWMFOutputStream->width  = snapshotWidth;
                    pHWMFOutputStream->height = snapshotHeight;
                    pNegotiationOutput->pStreams->push_back(pHWMFOutputStream);
                }
            }
            else
            {
                pHWMFOutputStream->width  = RDIStream.width;
                pHWMFOutputStream->height = RDIStream.height;
                pNegotiationOutput->pStreams->push_back(pHWMFOutputStream);
            }
        }

        // configure desired stream
        pNegotiationOutput->pDesiredStreamConfig->numStreams       = pNegotiationOutput->pStreams->size();
        pNegotiationOutput->pDesiredStreamConfig->operationMode    = pNegotiationInfo->pFwkStreamConfig->operationMode;
        pNegotiationOutput->pDesiredStreamConfig->pChiStreams      = &(*(pNegotiationOutput->pStreams))[0];
        pNegotiationOutput->pDesiredStreamConfig->pSessionSettings = NULL;
        pNegotiationOutput->disableZoomCrop                        = isNativeResolution;

    }

    return result;
}

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2OpsEntry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDK_VISIBILITY_PUBLIC VOID ChiFeature2OpsEntry(
    CHIFEATURE2OPS* pChiFeature2Ops)
{
    if (NULL != pChiFeature2Ops)
    {
        pChiFeature2Ops->size                   = sizeof(CHIFEATURE2OPS);
        pChiFeature2Ops->majorVersion           = Feature2MajorVersion;
        pChiFeature2Ops->minorVersion           = Feature2MinorVersion;
        pChiFeature2Ops->pVendorName            = VendorName;
        pChiFeature2Ops->pCreate                = CreateFeature;
        pChiFeature2Ops->pQueryCaps             = DoQueryCaps;
        pChiFeature2Ops->pGetVendorTags         = GetVendorTags;
        pChiFeature2Ops->pStreamNegotiation     = DoStreamNegotiation;
    }
}
#ifdef __cplusplus
}
#endif // __cplusplus
