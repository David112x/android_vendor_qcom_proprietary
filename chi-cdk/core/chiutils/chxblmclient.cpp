////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxblmclient.cpp
/// @brief BLM Client implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chxblmclient.h"
#include "chxextensionmodule.h"

 const CHAR* pBLMLib = "libthermalclient.so";
 CHAR s_clientName[] = "camera_bw";


#if defined (_LINUX)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHXBLMClient::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHXBLMClient * CHXBLMClient::Create()
{
    CHXBLMClient* pChxBLMClient = CHX_NEW CHXBLMClient();

    if (NULL != pChxBLMClient)
    {
        if (CDKResultSuccess != pChxBLMClient->Initialize())
        {
            CHX_DELETE(pChxBLMClient);
            pChxBLMClient = NULL;
        }
    }

    return pChxBLMClient;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHXBLMClient::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CHXBLMClient::Destroy()
{
    if (NULL != m_pMutex)
    {
        m_pMutex->Destroy();
        m_pMutex = NULL;
    }

    CHX_DELETE(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHXBLMClient::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHXBLMClient::Initialize()
{

    CDKResult result         = CDKResultSuccess;
    OSLIBRARYHANDLE handle   = ChxUtils::LibMap(pBLMLib);

    if (NULL != handle)
    {
        m_pBLMOps.setUsecaseHint    = reinterpret_cast<setUsecaseHintFunc>(
            ChxUtils::LibGetAddr(handle, "thermal_bandwidth_client_request"));
        m_pBLMOps.cancelUsecaseHint = reinterpret_cast<cancelUsecaseHintFunc>(
            ChxUtils::LibGetAddr(handle, "thermal_bandwidth_client_cancel_request"));

        if ((NULL == m_pBLMOps.setUsecaseHint) ||
            (NULL == m_pBLMOps.cancelUsecaseHint))
        {
            CHX_LOG_ERROR("unable to find libthermalclient func");
            result = CDKResultEFailed;
        }

        if (CDKResultSuccess == result)
        {
            m_pMutex = Mutex::Create();
        }
    }
    else
    {
        CHX_LOG_ERROR("Unable to find lib-thermal library");
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHXBLMClient::MapUsecaseFromBwParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 CHXBLMClient::MapUsecaseFromBwParams(
    ChiBLMParams blmparams)
{
    INT32 cameraUsecaseId = ChiBLMClientUseCaseZSLPreview;

    if ((480 == blmparams.FPS)      &&
        (1280 == blmparams.width)   &&
        (720 == blmparams.height))
    {
        cameraUsecaseId = ChiBLMClientUseCaseHFR720p480;
    }

    if ((240 == blmparams.FPS)      &&
        (1920 == blmparams.width)   &&
        (1080 == blmparams.height))
    {
        cameraUsecaseId = ChiBLMClientUseCaseHFR1080p240;
    }

    if ((120 == blmparams.FPS)      &&
        (1920 == blmparams.width)   &&
        (1080 == blmparams.height))
    {
        cameraUsecaseId = ChiBLMClientUseCaseHFR1080p120;
    }

    if ((120 == blmparams.FPS)      &&
        (1280 == blmparams.width)   &&
        (720 == blmparams.height))
    {
        cameraUsecaseId = ChiBLMClientUsecaseHFR720p120;
    }

    if ((60 == blmparams.FPS)      &&
        (1920 == blmparams.width)   &&
        (1080 == blmparams.height))
    {
        cameraUsecaseId = ChiBLMClientUsecaseHFR1080p60;
    }

    if ((60   == blmparams.FPS)     &&
        (1920 == blmparams.width)   &&
        (1080 == blmparams.height)  &&
        (UsecaseId::MultiCamera == blmparams.selectedusecaseId) &&
        (LogicalCameraType_SAT  == blmparams.logicalCameraType) &&
        (TRUE                   == blmparams.isVideoMode))
    {
        cameraUsecaseId = ChiBLMClientUsecaseDualCamHFR1080p60;
    }

    if ((30 == blmparams.FPS)      &&
        (1920 == blmparams.width)   &&
        (1080 == blmparams.height))
    {
        cameraUsecaseId = ChiBLMClientUsecase1080p30;
    }

    if ((30 == blmparams.FPS)      &&
        (3840 == blmparams.width)   &&
        (2160 == blmparams.height))
    {
        cameraUsecaseId = ChiBLMClientUseCaseUHD30;
    }

    if ((30   == blmparams.FPS)     &&
        (1920 == blmparams.width)   &&
        (1080 == blmparams.height)  &&
        (UsecaseId::MultiCamera == blmparams.selectedusecaseId) &&
        (LogicalCameraType_SAT  == blmparams.logicalCameraType) &&
        (TRUE                   == blmparams.isVideoMode))
    {
        cameraUsecaseId = ChiBLMClientUseCaseTriCam1080p30;
    }

    if ((UsecaseId::MultiCamera == blmparams.selectedusecaseId) &&
        (LogicalCameraType_SAT  == blmparams.logicalCameraType) &&
        (FALSE                  == blmparams.isVideoMode))
    {
        cameraUsecaseId = ChiBLMCLientUsecaseSATPreview;
    }

    if ((UsecaseId::MultiCamera == blmparams.selectedusecaseId) &&
        (LogicalCameraType_RTB  == blmparams.logicalCameraType) &&
        (FALSE                  == blmparams.isVideoMode))
    {
        cameraUsecaseId = ChiBLMClientUsecaseRTBSnapshot;
    }

    // 32MP Burst shot
    if ((32000000 <= blmparams.height * blmparams.width) &&
        (FALSE    == blmparams.isVideoMode))
    {
        cameraUsecaseId = ChiBLMClientUsecaseBurstShot;
    }

    return cameraUsecaseId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHXBLMClient::QueryBLMLevel
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CHXBLMClient::QueryBLMLevel(
    INT32 cameraUsecaseId,
    INT   targetIdx)
{
    UINT blmLevel = ChiBLMBWLevelLow;

    for (UINT i = 0; i < MAX_BLMCONFIG; i++)
    {
        if (g_blmClientInfo[targetIdx].chiBLMTarget[i].blmUsecaseID == cameraUsecaseId)
        {
            blmLevel = g_blmClientInfo[targetIdx].chiBLMTarget[i].resourceParam[ResourceTypeBW].value;
            break;
        }
        CHX_LOG_VERBOSE("usecaseID %x xml %x",
            cameraUsecaseId, g_blmClientInfo[targetIdx].chiBLMTarget[i].blmUsecaseID);
    }

    CHX_LOG_INFO("BLM Client usecaseid %x BLMLevel %d", cameraUsecaseId, blmLevel);

    return blmLevel;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHXBLMClient::SetUsecaseBwLevel
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHXBLMClient::SetUsecaseBwLevel(
    ChiBLMParams blmparams)
{
    CDKResult result          = CDKResultSuccess;
    INT32     cameraUsecaseId = 0x0;
    UINT      blmLevel        = 0;
    INT       targetIdx       = 0;

    cameraUsecaseId  = MapUsecaseFromBwParams(blmparams);

    targetIdx        = GetTagetIdx(blmparams.socId);

    blmLevel         = QueryBLMLevel(cameraUsecaseId, targetIdx);

    m_pMutex->Lock();

    m_pBLMOps.setUsecaseHint(s_clientName, blmLevel);

    m_pMutex->Unlock();

    CHX_LOG_INFO("BLMparams logicalcameraType %d FPS %d usecase %d socID %d w %d h %d",
        blmparams.logicalCameraType, blmparams.FPS, blmparams.selectedusecaseId , blmparams.socId,
        blmparams.width, blmparams.height);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHXBLMClient::GetTagetIdx
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT CHXBLMClient::GetTagetIdx(
    INT32 socId)
{
    INT   targetIdx = 0;
    INT32 targetSocId;

    switch (socId)
    {

        case CHISocId::CHISocIdSM7250:
            targetSocId = CHISocId::CHISocIdSM7250;
            break;

        case CHISocId::CHISocIdSM6350:
        case CHISocId::CHISocIdSM7225:
            targetSocId = CHISocId::CHISocIdSM6350;
            break;

        default:
            targetSocId = socId;
    }

    for (INT i = 0; i < MAX_TARGETIDX; i++)
    {
        if (g_blmClientInfo[i].socID == targetSocId)
        {
            targetIdx = i;
            break;
        }
    }

    return targetIdx;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHXBLMClient::CancelUsecaseHint
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHXBLMClient::CancelUsecaseHint()
{

    CDKResult result = CDKResultSuccess;

    m_pMutex->Lock();

    m_pBLMOps.cancelUsecaseHint(s_clientName);

    m_pMutex->Unlock();

    CHX_LOG_INFO("BWL client released %s", s_clientName);

    return result;
}

#else

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHXBLMClient::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHXBLMClient * CHXBLMClient::Create()
{
    CHXBLMClient* pChxBLMClient = CHX_NEW CHXBLMClient();

    if (NULL != pChxBLMClient)
    {
        if (CDKResultSuccess != pChxBLMClient->Initialize())
        {
            CHX_DELETE(pChxBLMClient);
            pChxBLMClient = NULL;
        }
    }

    return pChxBLMClient;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHXBLMClient::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CHXBLMClient::Destroy()
{
    CHX_DELETE(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHXBLMClient::SetUsecaseBwLevel
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHXBLMClient::SetUsecaseBwLevel(
    ChiBLMParams blmparam)
{
    CDK_UNUSED_PARAM(blmparam);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHXBLMClient::CancelUsecaseHint
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHXBLMClient::CancelUsecaseHint()
{
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHXBLMClient::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHXBLMClient::Initialize()
{

    return CDKResultSuccess;
}

#endif // _LINUX