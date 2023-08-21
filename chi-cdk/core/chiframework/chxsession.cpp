////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxsession.cpp
/// @brief CHX session class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <assert.h>

#include "chxextensionmodule.h"
#include "chxincs.h"
#include "chxpipeline.h"
#include "chxsession.h"
#include "chxusecaseutils.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Session* Session::Create(
    Pipeline**    ppPipelines,
    UINT32        numPipelines,
    ChiCallBacks* pChiCallbacks,
    VOID*         pPrivateData)
{
    CDKResult result = CDKResultSuccess;

    Session* pSession = CHX_NEW Session();

    if (NULL != pSession)
    {
        result = pSession->Initialize(ppPipelines, numPipelines, pChiCallbacks, pPrivateData);

        if (CDKResultSuccess != result)
        {
            CHX_DELETE pSession;
            pSession = NULL;
        }
    }

    return pSession;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::Destroy(
    BOOL isForced)
{
    if (NULL != m_hSession)
    {
        ExtensionModule::GetInstance()->DestroySession(m_hSession, isForced);

        m_hSession = NULL;

        Pipeline* pSCPipeline = GetSCRealTimePipeline();

        if (NULL != pSCPipeline)
        {
            CHX_LOG_INFO("[CAMX_RC_DEBUG] Update resource cost here for cameraId %d, numIFEs %d",
                pSCPipeline->GetCameraId(), pSCPipeline->GetNumberOfIFEsUsed());
            ExtensionModule::GetInstance()->ResetResourceCost(pSCPipeline->GetCameraId());
        }

    }

    if (NULL != m_pPipelineInfo)
    {
        CHX_DELETE m_pPipelineInfo;
        m_pPipelineInfo = NULL;
    }

    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Session::Initialize(
    Pipeline**          ppPipelines,
    UINT32              numPipelines,
    ChiCallBacks*       pCallbacks,
    VOID*               pPrivateData,
    CHISESSIONFLAGS     flags)
{
    CDKResult         result = CDKResultSuccess;
    SessionCreateData sessionCreateData;

    ChxUtils::Memset(&sessionCreateData, 0, sizeof(sessionCreateData));

    m_numPipelines                             = numPipelines;
    sessionCreateData.numPipelines             = numPipelines;
    sessionCreateData.pPipelineInfo            = CHX_NEW CHIPIPELINEINFO[numPipelines];

    if (NULL != sessionCreateData.pPipelineInfo)
    {
        m_pPipelineInfo                        = sessionCreateData.pPipelineInfo;
        sessionCreateData.pCallbacks           = pCallbacks;
        sessionCreateData.pPrivateCallbackData = pPrivateData;
        sessionCreateData.flags                = flags;

        for (UINT i = 0; i < numPipelines; i++)
        {
            m_pPipelines[i]                    = ppPipelines[i];
            sessionCreateData.pPipelineInfo[i] = ppPipelines[i]->GetPipelineInfo();
        }

        m_hSession = ExtensionModule::GetInstance()->CreateSession(&sessionCreateData);

        if (NULL == m_hSession)
        {
            result = CDKResultEFailed;
        }

        for (UINT index = 0; index < numPipelines; ++index)
        {
            m_pPipelines[index]->QueryMetadataTags(m_hSession);
        }

        // Update the ISP resource utilized by this session to extension module
        if (CDKResultSuccess == result)
        {
            Pipeline* pSCPipeline = GetSCRealTimePipeline();

            if (NULL != pSCPipeline)
            {
                CHX_LOG_INFO("[CAMX_RC_DEBUG] Update resource cost here for cameraId %d, numIFEs %d",
                    pSCPipeline->GetCameraId(), pSCPipeline->GetNumberOfIFEsUsed());
                ExtensionModule::GetInstance()->SetResourceCost(pSCPipeline->GetCameraId(),
                    pSCPipeline->GetNumberOfIFEsUsed());
            }
        }
    }
    else
    {
        result = CDKResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Session::Finalize()
{
    CDKResult result = CDKResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::GetPipelineIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 Session::GetPipelineIndex(UINT32 cameraId)
{
    UINT32 pipelineIndex = CDKInvalidId;

    for (UINT32 i = 0; i < m_numPipelines; i++)
    {
        if (cameraId == m_pPipelines[i]->GetCameraId())
        {
            pipelineIndex = i;
        }
    }
    return pipelineIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::GetPipelineIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 Session::GetPipelineIndex(CHIPIPELINEHANDLE pipelineHandle)
{
    UINT32 pipelineIndex = CDKInvalidId;

    for (UINT32 i = 0; i < m_numPipelines; i++)
    {
        if (pipelineHandle == GetPipelineHandle(i))
        {
            pipelineIndex = i;
        }
    }
    return pipelineIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::GetCameraId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 Session::GetCameraId(UINT32 pipelineIndex)
{
    return m_pPipelines[pipelineIndex]->GetCameraId();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::~Session
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Session::~Session()
{
}
