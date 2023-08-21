////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxerrorinducer.cpp
/// @brief ContingencyInducer class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxchicontext.h"
#include "camxsettingsmanager.h"
#include "camxerrorinducer.h"

#include "chi.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ContingencyInducer::ContingencyInducer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ContingencyInducer::ContingencyInducer()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ContingencyInducer::~ContingencyInducer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ContingencyInducer::~ContingencyInducer()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ContingencyInducer::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ContingencyInducer::Initialize(
    ChiContext* pChiContext,
    const CHAR* pPipelineName,
    const CHAR* pNodeName)
{
    CamxResult result = CamxResultSuccess;

    m_pChiContext          = pChiContext;
    m_pCurrentPipelineName = pPipelineName;
    m_pCurrentNodeName     = pNodeName;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ContingencyInducer::CheckFenceDropNeeded
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FenceDropActionReturnType ContingencyInducer::CheckFenceDropNeeded(
    BOOL   isRealTime,
    UINT64 requestId,
    UINT   portId)
{
    const StaticSettings*     pStaticSettings   = m_pChiContext->GetStaticSettings();
    FenceDropActionReturnType fenceAction       = FenceDropActionReturnType::ACTION_NONE;
    BOOL                      dropConditionMask = FALSE;

    // Check enable inducer
    if ((NULL == pStaticSettings) || (FALSE == pStaticSettings->enableFenceDrop))
    {
        return FenceDropActionReturnType::ACTION_NONE;
    }

    // Check the real time usecase
    if (isRealTime == static_cast<INT32>(pStaticSettings->inducerIsRealTime))
    {
        dropConditionMask = TRUE;
    }

    // Check the pipeline name
    if (0 == OsUtils::StrLen(pStaticSettings->inducerPipelineName))
    {
        dropConditionMask &= TRUE;
    }
    else if (0 == OsUtils::StrCmp(pStaticSettings->inducerPipelineName, m_pCurrentPipelineName))
    {
        dropConditionMask &= TRUE;
    }
    else
    {
        dropConditionMask = FALSE;
    }

    // Check the node name
    if (0 == OsUtils::StrLen(pStaticSettings->inducerNodeName))
    {
        dropConditionMask &= TRUE;
    }
    else if (0 == OsUtils::StrCmp(pStaticSettings->inducerNodeName, m_pCurrentNodeName))
    {
        dropConditionMask &= TRUE;
    }
    else
    {
        dropConditionMask = FALSE;
    }

    // Check the output port equal the default value
    if (-1 == (pStaticSettings->inducerPortId))
    {
        dropConditionMask &= TRUE;
    }
    else if (portId == static_cast<UINT>(pStaticSettings->inducerPortId))
    {
        dropConditionMask &= TRUE;
    }
    else
    {
        dropConditionMask = FALSE;
    }

    // Check RequestId
    if (pStaticSettings->inducerRequestId < requestId)
    {
        dropConditionMask &= TRUE;
    }
    else
    {
        dropConditionMask = FALSE;
    }

    // Check mark as error
    if (TRUE == dropConditionMask)
    {
        if (TRUE == pStaticSettings->inducerFenceMarkError)
        {
            fenceAction = FenceDropActionReturnType::ACTION_ERROR;
        }
        else
        {
            fenceAction = FenceDropActionReturnType::ACTION_DROP;
        }
        CAMX_LOG_INFO(CamxLogGroupCore,
                "inducer inducerRequestId:%d, requestId:%d, fenceAction:%d, "
                "currentNode:%s, currentPipelineName:%s, currentPortId:%d   ",
                pStaticSettings->inducerRequestId,
                requestId,
                fenceAction,
                m_pCurrentNodeName,
                m_pCurrentPipelineName,
                portId);
    }
    return fenceAction;
}

CAMX_NAMESPACE_END