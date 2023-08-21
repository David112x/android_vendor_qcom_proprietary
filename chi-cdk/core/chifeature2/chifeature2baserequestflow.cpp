////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2baserequestflow.cpp
/// @brief Base provided request flow implementations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2base.h"
#include "chifeature2utils.h"

// NOWHINE FILE CP006:  Need vector to read in variable number of input buffer info

static const UINT NumberOfFrames = 3;
ChiFeature2RequestFlow ChiFeature2Base::m_requestFlows[] =
{
    { ChiFeature2RequestFlowType::Type0, &ChiFeature2Base::ExecuteFlowType0 },
    { ChiFeature2RequestFlowType::Type1, &ChiFeature2Base::ExecuteFlowType1 },
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Base::ExecuteBaseRequestFlow
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::ExecuteBaseRequestFlow(
    ChiFeature2RequestObject*   pRequestObject,
    ChiFeature2RequestFlowType  flowType
    ) const
{
    CDKResult result = CDKResultSuccess;

    UINT8 flow = static_cast<INT8>(flowType);

    if ((flow < CHX_ARRAY_SIZE(m_requestFlows)) && (flowType == m_requestFlows[flow].flowType))
    {
        PFNBASEREQUESTFLOWFUNCTION pExecuteFunc = m_requestFlows[flow].pFlowFunc;
        result = (this->*pExecuteFunc)(pRequestObject);
    }
    else
    {
        CHX_LOG_ERROR("Invalid parameter: flow=%u, flowType=%d", flow, flowType);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Base::ExecuteFlowType0
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::ExecuteFlowType0(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult result             = CDKResultSuccess;
    UINT8     stageId            = InvalidStageId;
    UINT8     nextStageId        = InvalidStageId;
    INT8      stageSequenceId    = InvalidStageSequenceId;
    UINT8     numDependencyLists = 0;
    UINT8     numStages          = GetNumStages();

    const ChiFeature2StageDescriptor*   pStageDescriptor = NULL;
    ChiFeature2StageInfo                stageInfo        = { 0 };
    ChiFeature2PortIdList               outputList       = { 0 };
    ChiFeature2PortIdList               inputList        = { 0 };

    result = GetCurrentStageInfo(pRequestObject, &stageInfo);

    if (CDKResultSuccess == result)
    {
        stageId         = stageInfo.stageId;
        stageSequenceId = stageInfo.stageSequenceId;
        nextStageId     = stageId + 1;

        if (InvalidStageId == stageId)
        {
            // Setting this tells the request object how many overall sequences we will run
            SetConfigInfo(pRequestObject, numStages);
            pStageDescriptor = GetStageDescriptor(nextStageId);

            if (NULL != pStageDescriptor)
            {
                numDependencyLists = GetNumDependencyListsFromStageDescriptor(pStageDescriptor, 0, 0);
                SetNextStageInfoFromStageDescriptor(pRequestObject, pStageDescriptor, 0, numDependencyLists);
                PopulateDependency(pRequestObject);
            }
        }
        else
        {
            if ((TRUE == ExtensionModule::GetInstance()->EnableFeature2Dump()) &&
                (static_cast<UINT32>(ChiFeature2Type::B2Y) == GetFeatureId()))
            {
                ChiFeature2BufferMetadataInfo inputBufferInfo = { 0 };
                ChiFeature2BufferMetadataInfo inputMetaInfo   = { 0 };

                ChiFeature2Identifier portidentifier     =
                { 0, 0, 0, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer };
                ChiFeature2Identifier metaportidentifier =
                { 0, 0, 1, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData };

                pRequestObject->GetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency,
                    &portidentifier, &inputBufferInfo.hBuffer, &inputBufferInfo.key, 0, 0);
                pRequestObject->GetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency,
                    &metaportidentifier, &inputMetaInfo.hBuffer, &inputMetaInfo.key, 0, 0);

                CHAR dumpFileBaseName[128] = { 0 };
                GetDumpFileBaseName(pRequestObject, dumpFileBaseName, 128);

                DumpInputMetaBuffer(&inputMetaInfo, dumpFileBaseName, 0);
                DumpInputImageBuffer(&inputBufferInfo, dumpFileBaseName, 0);
            }

            result = PopulateConfiguration(pRequestObject);

            if (CDKResultSuccess == result)
            {
                result = SubmitRequestToSession(pRequestObject);
                if (CDKResultSuccess == result)
                {
                    if (nextStageId < numStages)
                    {
                        pStageDescriptor = GetStageDescriptor(nextStageId);

                        if (NULL != pStageDescriptor)
                        {
                            numDependencyLists = GetNumDependencyListsFromStageDescriptor(pStageDescriptor, 0, 0);
                            SetNextStageInfoFromStageDescriptor(pRequestObject, pStageDescriptor, 0, 1);
                            PopulateDependency(pRequestObject);
                        }
                    }
                }
                else
                {
                    CHX_LOG_ERROR("Failed to submit request %d to session %d", pRequestObject->GetCurRequestId(), result);
                }
            }
            else
            {
                CHX_LOG_ERROR("Failed to populate configuration");
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Failed to get current stage info");
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Base::ExecuteFlowType1
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::ExecuteFlowType1(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult           result                = CDKResultSuccess;
    UINT8               stageId               = InvalidStageId;
    UINT8               nextStageId           = InvalidStageId;
    INT8                stageSequenceId       = InvalidStageSequenceId;
    UINT8               numFrames             = NumberOfFrames;
    UINT8               requestId             = pRequestObject->GetCurRequestId();;
    UINT8               numTimesExecuteStage  = 1;
    std::vector<UINT8>  framesPerStage;

    ChiFeature2StageDescriptor*         pStageDescriptor    = NULL;
    ChiFeature2StageInfo                stageInfo           = { 0 };
    ChiFeature2PortIdList               outputList          = { 0 };
    ChiFeature2PortIdList               inputList           = { 0 };
    ChiFeature2Hint*                    pHint               = pRequestObject->GetFeatureHint();
    const ChiFeature2InputDependency*   pInputDependency    = NULL;
    ChiFeatureSequenceData*             pRequestData        = NULL;

    if (NULL != pHint)
    {
        numFrames               = pHint->numFrames;
        numTimesExecuteStage    = pHint->stageSequenceInfo.size();
        framesPerStage          = pHint->stageSequenceInfo;
    }
    else
    {
        // If there is no hint, set default values which are to run the stage once with numFrames frames
        numTimesExecuteStage = 1;
        framesPerStage.push_back(numFrames);
    }

    std::vector<ChiFeature2BufferMetadataInfo> inputBufferInfo;
    std::vector<ChiFeature2Identifier>         filteredPortIds;

    result = GetCurrentStageInfo(pRequestObject, &stageInfo);

    if (CDKResultSuccess == result)
    {
        stageId         = stageInfo.stageId;
        stageSequenceId = stageInfo.stageSequenceId;
        nextStageId     = stageId + 1;

        CHX_LOG_VERBOSE("Total frames:%d, split between:%d stage sequences, current stage:%d, stageSequence:%d",
                        numFrames,
                        numTimesExecuteStage,
                        stageId,
                        stageSequenceId);

        if (stageId == InvalidStageId)
        {
            SetConfigInfo(pRequestObject, (numTimesExecuteStage + 1));
            pStageDescriptor = GetStageDescriptor(nextStageId);
            if (NULL != pStageDescriptor)
            {
                CHX_LOG_VERBOSE("%s Setting up stage:%d stageSequenceId:0 requesting:%d frames",
                                pRequestObject->IdentifierString(),
                                nextStageId,
                                framesPerStage[0]);

                SetNextStageInfoFromStageDescriptor(pRequestObject, pStageDescriptor, 0, framesPerStage[0]);
                pInputDependency = GetDependencyListFromStageDescriptor(pStageDescriptor, 0, 0, 0);
                if (NULL != pInputDependency)
                {
                    for (UINT8 bufferIndex = 0; bufferIndex < framesPerStage[0]; ++bufferIndex)
                    {
                        PopulateDependencyPorts(pRequestObject, bufferIndex, pInputDependency);
                    }
                }
            }
            else
            {
                CHX_LOG_ERROR("pStageDescriptor is NULL");
            }
        }
        else if (stageId == 0)
        {
            pStageDescriptor    = GetStageDescriptor(stageId);
            numFrames           = framesPerStage[stageSequenceId];

            inputBufferInfo.resize(numFrames);

            result = GetOutputPortsForStage(stageId, &outputList);

            // If this sequence stage is not the final stage, we want to skip the output of this
            // request and only send back the result from the final sequence stage
            ChiFeatureSequenceData* pRequestData = static_cast<ChiFeatureSequenceData*>(
                GetSequencePrivContext(pRequestObject));
            if (NULL != pRequestData)
            {
                if (stageSequenceId < (numTimesExecuteStage - 1))
                {
                    pRequestData->skipSequence = TRUE;
                }
            }

            if (CDKResultSuccess == result)
            {
                result = GetInputPortsForStage(stageId, &inputList);
                filteredPortIds.reserve(inputList.numPorts);
            }

            if (CDKResultSuccess == result)
            {
                const ChiFeature2InputDependency* pInputDependency =
                    GetDependencyListFromStageDescriptor(pStageDescriptor, 0, 0, 0);

                if (NULL != pInputDependency)
                {
                    const ChiFeature2Identifier* pImagePort = &pInputDependency->pInputDependency[0].globalId;
                    const ChiFeature2Identifier* pMetaPort  = &pInputDependency->pInputDependency[1].globalId;

                    // This flow requires all image buffers to be requested on the first port and the last port to be metadata
                    // port. In between we need to have internal input ports which will be used for submission.
                    // Validate that here

                    if (pImagePort->portType != ChiFeature2PortType::ImageBuffer)
                    {
                        CHX_LOG_ERROR("First dependency port must be of type image Buffer");
                        result = CDKResultEInvalidArg;
                    }

                    if (CDKResultSuccess == result)
                    {
                        // Only select the number of ports required for the number of frames, that is if we are running 3-frame
                        // feature, we need to enable 3 internal input ports and the metadata port

                        for (UINT i = 0; i < numFrames; i++)
                        {
                            pRequestObject->GetBufferInfo(
                                ChiFeature2RequestObjectOpsType::InputDependency,
                                pImagePort,
                                &inputBufferInfo[i].hBuffer,
                                &inputBufferInfo[i].key,
                                requestId,
                                i);

                            filteredPortIds.push_back(inputList.pPorts[i]);

                            CHX_LOG_INFO("GetBufferinfo RDI%d buffer %p", i, inputBufferInfo[i].hBuffer);
                        }

                        filteredPortIds.push_back(*pMetaPort);

                        ChiFeature2PortIdList filteredPortIdList = { 0 };

                        filteredPortIdList.numPorts = filteredPortIds.size();
                        filteredPortIdList.pPorts   = filteredPortIds.data();

                        PopulatePortConfiguration(pRequestObject, &filteredPortIdList, &outputList);

                        for (UINT i = 0; i < numFrames; i++)
                        {
                            pRequestObject->SetBufferInfo(
                                ChiFeature2RequestObjectOpsType::InputConfiguration,
                                &inputList.pPorts[i],
                                inputBufferInfo[i].hBuffer,
                                inputBufferInfo[i].key,
                                FALSE,
                                0,
                                0);

                            CHX_LOG_INFO("SetBufferinfo RDI%d buffer %p", i, inputBufferInfo[i].hBuffer);
                        }

                        result = SubmitRequestToSession(pRequestObject);

                        if (CDKResultSuccess == result)
                        {
                            // If this stage sequence isn't the final sequence for the stage, wait for the result
                            // of the current sequence, then set the stage info and populate the dependencies for the
                            // next stage sequence
                            if (stageSequenceId < (numTimesExecuteStage - 1))
                            {
                                // Wait for every stage result before proceeding
                                pRequestObject->WaitOnResult(requestId);

                                pStageDescriptor = GetStageDescriptor(stageId);
                                if (NULL != pStageDescriptor)
                                {
                                    ++stageSequenceId;
                                    numFrames = framesPerStage[stageSequenceId];

                                    CHX_LOG_VERBOSE("%s stage:%d stageSequenceId:%d requesting:%d frames",
                                                    pRequestObject->IdentifierString(),
                                                    pStageDescriptor->stageId,
                                                    stageSequenceId,
                                                    numFrames);

                                    SetNextStageInfoFromStageDescriptor(pRequestObject,
                                                                        pStageDescriptor,
                                                                        stageSequenceId,
                                                                        numFrames);
                                    pInputDependency = GetDependencyListFromStageDescriptor(pStageDescriptor, 0, 0, 0);
                                    if (NULL != pInputDependency)
                                    {
                                        for (UINT8 bufferIndex = 0; bufferIndex < numFrames; ++bufferIndex)
                                        {
                                            if (ChiFeature2RequestState::Executing !=
                                                pRequestObject->GetCurRequestState(requestId))
                                            {
                                                pRequestObject->SetCurRequestState(ChiFeature2RequestState::Executing,
                                                                                   requestId);
                                            }
                                            PopulateDependencyPorts(pRequestObject, bufferIndex, pInputDependency);
                                        }
                                    }
                                }
                                else
                                {
                                    CHX_LOG_ERROR("pStageDescriptor is NULL");
                                }
                            }
                        }
                        else
                        {
                            CHX_LOG_ERROR("Failed to submit request %d to session %d", requestId, result);
                        }
                    }
                }
            }
        }
        else
        {
            CHX_LOG_ERROR("Unhandled stage id %d", stageId);
        }
    }
    else
    {
        CHX_LOG_ERROR(" Invalid Request Object");
        result = CDKResultEInvalidState;
    }

    return result;
}
