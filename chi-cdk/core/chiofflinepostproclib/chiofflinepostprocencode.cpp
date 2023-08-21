////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chiofflinepostprocencode.cpp
/// @brief Generic encode functions are implemented in this source file.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chi.h"
#include "chiofflinepostprocencode.h"
#include "chxextensionmodule.h"

// NOWHINE FILE CP036a: Const cast is used

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Callback function to get Context
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID(*PFNCHIENTRY)(CHICONTEXTOPS* pContextOps);

const UINT32 ProcessRequestThreadSleepTime  = 50;      ///< Sleep time for thread

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiOfflinePostprocEncode::Setup
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiOfflinePostprocEncode::Setup()
{
    // Initialize member variables
    m_pFeature2ObjectMutex          = NULL;
    m_pFeature2RequestStateMutex    = NULL;
    m_pFeature2RequestStateComplete = NULL;
    m_pFeature2RequestObject        = NULL;
    m_pFeature2Base                 = NULL;
    m_pThreadManager                = NULL;
    m_pMetadataManager              = NULL;
    m_pCameraMetadata               = NULL;
    m_pDebugDataPtr                 = NULL;
    m_selectCam                     = 0;
    m_sensorSubModeValue            = 0;

    CDKResult result = LoadChiOps();

    if (CDKResultSuccess == result)
    {
        // Get metadata manager
        result = CreateMetadataManager();

        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Metadata manager creation failed!");
        }
    }

    if (CDKResultSuccess == result)
    {
        result = CHIThreadManager::Create(&m_pThreadManager, "FeatureThreadManager");

        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("FeatureThreadManager Create failed");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Setup mutex and conditions
        m_pFeature2ObjectMutex          = Mutex::Create();
        m_pFeature2RequestStateMutex    = Mutex::Create();
        m_pFeature2RequestStateComplete = Condition::Create();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiOfflinePostprocEncode::Teardown
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiOfflinePostprocEncode::Teardown()
{
    if (NULL != m_pFeature2RequestObject)
    {
        m_pFeature2RequestObject->Destroy();
        m_pFeature2RequestObject = NULL;
    }

    DestroyFeature2Object();
    FreeCameraMetadata();

    if (NULL != m_pThreadManager)
    {
        m_pThreadManager->Destroy();
        m_pThreadManager = NULL;
    }

    if (NULL != m_pMetadataManager)
    {
        m_pMetadataManager->Destroy();
        m_pMetadataManager = NULL;
    }

    // Destroy mutex and conditions
    if (NULL != m_pFeature2ObjectMutex)
    {
        m_pFeature2ObjectMutex->Destroy();
        m_pFeature2ObjectMutex = NULL;
    }

    if (NULL != m_pFeature2RequestStateMutex)
    {
        m_pFeature2RequestStateMutex->Destroy();
        m_pFeature2RequestStateMutex = NULL;
    }

    if (NULL != m_pFeature2RequestStateComplete)
    {
        m_pFeature2RequestStateComplete->Destroy();
        m_pFeature2RequestStateComplete = NULL;
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiOfflinePostprocEncode::ReleasePipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiOfflinePostprocEncode::ReleasePipeline()
{
    CHX_LOG_INFO("Resource release request came from service");

    if (NULL != m_pFeature2RequestObject)
    {
        m_pFeature2RequestObject->Destroy();
        m_pFeature2RequestObject = NULL;
    }

    DestroyFeature2Object();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiOfflinePostprocEncode::LoadChiOps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiOfflinePostprocEncode::LoadChiOps()
{
    CDKResult   result = CDKResultSuccess;
    PFNCHIENTRY pChiHalOpen;

    VOID* pLibrary = ChxUtils::LibMap("/vendor/lib64/hw/camera.qcom.so");

    if (NULL == pLibrary)
    {
        CHX_LOG_ERROR("Failed to load android library");
        return CDKResultEUnableToLoad;
    }

    pChiHalOpen = reinterpret_cast<PFNCHIENTRY>(ChxUtils::LibGetAddr(pLibrary, "ChiEntry"));

    if (NULL != pChiHalOpen)
    {
        (*pChiHalOpen)(&m_chiOps);
        m_chiOps.pMetadataOps(&m_metadataOps);
        m_chiOps.pGetFenceOps(&m_fenceOps);
    }
    else
    {
        CHX_LOG_ERROR("ChiEntry missing in library");
        result = CDKResultEUnableToLoad;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiOfflinePostprocEncode::SetupCamera
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiOfflinePostprocEncode::SetupCamera()
{
    // Get number of cameras
    UINT32 numFwCameras;
    UINT32 numLogicalCameras;
    ExtensionModule::GetInstance()->GetNumCameras(&numFwCameras, &numLogicalCameras);

    // Get camera info
    if (numLogicalCameras > 0)
    {
        ExtensionModule::GetInstance()->GetCameraInfo(m_selectCam, &m_cameraInfo);

        // Initialize extension module settings
        const UINT32 numExtendSettings      = static_cast<UINT32>(ChxSettingsToken::CHX_SETTINGS_TOKEN_COUNT);
        CHIEXTENDSETTINGS* pExtendSettings  = NULL;
        CHIMODIFYSETTINGS* pModifySettings  = NULL;
        m_chiOps.pGetSettings(&pExtendSettings, &pModifySettings);

        // Open camera
        ExtensionModule::GetInstance()->ExtendOpen(m_selectCam, pExtendSettings);

        // Modify settings for each override setting
        for (UINT32 i = 0; i < numExtendSettings; i++)
        {
            ExtensionModule::GetInstance()->ModifySettings(&pModifySettings[i]);
        }
    }

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiOfflinePostprocEncode::CreateMetadataManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiOfflinePostprocEncode::CreateMetadataManager()
{
    CDKResult result = CDKResultENoMemory;

    m_pMetadataManager = ChiMetadataManager::Create();

    if (NULL != m_pMetadataManager)
    {
        result                    = CDKResultSuccess;
        m_genericMetadataClientId = m_pMetadataManager->RegisterClient(TRUE, NULL, 0, 0,
                                                                       DefaultNumMetadataBuffers,
                                                                       ChiMetadataUsage::Generic);
        CHX_LOG_INFO("m_genericMetadataClientId for pThreadManager is %u",
                     m_genericMetadataClientId);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiOfflinePostprocEncode::ProcessMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiOfflinePostprocEncode::ProcessMessage(
    ChiFeature2RequestObject*   pFeatureRequestObj,
    ChiFeature2Messages*        pMessages)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pMessages)
    {
        CHX_LOG_ERROR("Invalid argument: pMessages is NULL");
        result = CDKResultEInvalidArg;
    }
    else if (NULL == pFeatureRequestObj)
    {
        CHX_LOG_VERBOSE("Invalid argument: pFeatureRequestObj is NULL, pMessages %p, pFeatureMessages %p",
                        pMessages, pMessages->pFeatureMessages);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        VOID* pGraphPrivateData = NULL;
        pFeatureRequestObj->GetGraphPrivateData(&pGraphPrivateData);

        ChiOfflinePostprocEncode* pFeature2Inf = reinterpret_cast<ChiOfflinePostprocEncode*>(pGraphPrivateData);

        if (NULL != pFeature2Inf)
        {
            // Acquire mutex to avoid other thread to release resources
            pFeature2Inf->LockFeature2Object();
            // Irrespective of the result of this API, we need to continue and complete the request.
            pFeature2Inf->ProcessFeature2Message(pFeatureRequestObj, pMessages);

            ChiFeature2RequestObject*   pFeature2ReqObj = pFeature2Inf->GetRequestObject();
            ChiFeature2RequestState     reqState        = pFeature2ReqObj->GetCurRequestState(0);

            CHX_LOG_VERBOSE("current state %d", reqState);

            if ((ChiFeature2RequestState::InputResourcePending == reqState) ||
                (ChiFeature2RequestState::OutputNotificationPending == reqState))
            {
                (pFeature2Inf->GetBasePtr())->ProcessRequest(pFeature2ReqObj);
            }

            if (NULL != pMessages->pFeatureMessages)
            {
                // This needs to be reviisted later.
                // Since initial design is creating Feature2Base class, this shortcut is used.
                if (ChiFeature2MessageType::SubmitRequestNotification == pMessages->pFeatureMessages->messageType)
                {
                    result = ExtensionModule::GetInstance()->SubmitRequest(const_cast<CHIPIPELINEREQUEST*>
                        (&(pMessages->pFeatureMessages->message.submitRequest)));

                    if (CDKResultECancelledRequest == result)
                    {
                        CHX_LOG_WARN("Session returned that flush was in progress. Rewriting result as success.");
                        result = CDKResultSuccess;
                    }

                    CHX_LOG_VERBOSE("ChiOfflineEncode: SubmitRequest API completed, result %d", result);
                }
                else if ((ChiFeature2MessageType::ResultNotification == pMessages->pFeatureMessages->messageType) &&
                         (ChiFeature2RequestState::OutputNotificationPending == reqState))
                {
                    pFeature2Inf->OutputImageReceived();
                }
            }

            pFeature2Inf->UnlockFeature2Object();
        }
        else
        {
            CHX_LOG_ERROR("GraphPrivateData is NULL, cannot ProcessMessage");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiOfflinePostprocEncode::FillDefaultMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiOfflinePostprocEncode::FillDefaultMetadata(
    ChiMetadata* pMetaData)
{
    if (pMetaData != NULL)
    {
        ChiTuningModeParameter chiTuningModeParameter = { 0 };
        chiTuningModeParameter.noOfSelectionParameter = MaxTuningMode;

        chiTuningModeParameter.TuningMode[0].mode               = ChiModeType::Default;
        chiTuningModeParameter.TuningMode[0].subMode.value      = 0;
        chiTuningModeParameter.TuningMode[1].mode               = ChiModeType::Sensor;
        chiTuningModeParameter.TuningMode[1].subMode.value      = m_sensorSubModeValue;
        chiTuningModeParameter.TuningMode[2].mode               = ChiModeType::Usecase;
        chiTuningModeParameter.TuningMode[2].subMode.usecase    = ChiModeUsecaseSubModeType::Preview;
        chiTuningModeParameter.TuningMode[3].mode               = ChiModeType::Feature1;
        chiTuningModeParameter.TuningMode[3].subMode.feature1   = ChiModeFeature1SubModeType::None;
        chiTuningModeParameter.TuningMode[4].mode               = ChiModeType::Feature2;
        chiTuningModeParameter.TuningMode[4].subMode.feature2   = ChiModeFeature2SubModeType::None;
        chiTuningModeParameter.TuningMode[5].mode               = ChiModeType::Scene;
        chiTuningModeParameter.TuningMode[5].subMode.scene      = ChiModeSceneSubModeType::None;
        chiTuningModeParameter.TuningMode[6].mode               = ChiModeType::Effect;
        chiTuningModeParameter.TuningMode[6].subMode.effect     = ChiModeEffectSubModeType::None;

        ChxUtils::SetVendorTagValue(pMetaData, VendorTag::TuningMode,
                                    sizeof(ChiTuningModeParameter), &chiTuningModeParameter);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiOfflinePostprocEncode::DoEncode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiOfflinePostprocEncode::DoEncode()
{
    ChiFeature2CreateInputInfo feature2CreateInputInfo = { 0 };
    CDKResult                  result                  = CDKResultENoMemory;
    ChiFeature2RequestState    requestState            = ChiFeature2RequestState::Initialized;

    // Check if base obj needs to be created or not
    if (NULL == m_pFeature2Base)
    {
        // Get feature descriptor
        result = GetGenericFeature2Descriptor(&feature2CreateInputInfo);

        if (CDKResultSuccess == result)
        {
            // Add metadata manager
            feature2CreateInputInfo.pMetadataManager    = m_pMetadataManager;

            // "ProcessMessage" static callback function will be used
            ChiFeature2GraphCallbacks graphCallbacks    = {0};
            graphCallbacks.ChiFeature2ProcessMessage    = ProcessMessage;
            feature2CreateInputInfo.pClientCallbacks    = &graphCallbacks;
            feature2CreateInputInfo.pThreadManager      = m_pThreadManager;

            ChiFeature2GraphManagerCallbacks graphMgrCallbacks      = {0};
            feature2CreateInputInfo.pFeatureGraphManagerCallbacks   = &graphMgrCallbacks;

            // Create feature 2 base
            m_pFeature2Base = CreateFeature2(&feature2CreateInputInfo);
        }
    }

    // Destroy earlier Request FRO
    if (NULL != m_pFeature2RequestObject)
    {
        // Acquire lock before freeing object
        LockFeature2Object();
        CHX_LOG_VERBOSE("Destroy feature2 object %p", m_pFeature2RequestObject);
        m_pFeature2RequestObject->Destroy();
        m_pFeature2RequestObject = NULL;
        UnlockFeature2Object();
    }

    if (NULL != m_pFeature2Base)
    {
        ChiMetadata* pMetadata = m_pMetadataManager->Get(m_genericMetadataClientId, 0);

        result = GetInputFeature2RequestObject(m_pFeature2Base, pMetadata,
                                               &m_pFeature2RequestObject, this);

        if (NULL != m_pFeature2RequestObject)
        {
            ChiFeature2RequestState currentState = ChiFeature2RequestState::Initialized;
            UINT32                  sleepTime    = ProcessRequestThreadSleepTime;
            // Call ProcessRequest for first time, subsequent calls are done from CB function
            m_pFeature2Base->ProcessRequest(m_pFeature2RequestObject);

            do
            {
                // This runs in postproc service thread, instead of checking for status continuously.
                // We check once for every ~50ms. If the image is available, ProcessMessage CB function
                // will signal by checking current State of FRO. In such case, we will not wait for ~50ms.
                m_pFeature2RequestStateMutex->Lock();
                CHX_LOG_INFO("Waiting for output notifiation, current state %d, framenum %u",
                             currentState, m_frameNumber);
                m_pFeature2RequestStateComplete->TimedWait(m_pFeature2RequestStateMutex->GetNativeHandle(), sleepTime);
                CHX_LOG_VERBOSE("Received for output notifiation");
                m_pFeature2RequestStateMutex->Unlock();
                currentState = m_pFeature2RequestObject->GetCurRequestState(0);
            } while (ChiFeature2RequestState::Complete != currentState);
        }
    }
    else
    {
        CHX_LOG_ERROR("m_pFeature2Base is NULL");
    }


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiOfflinePostprocEncode::DestroyFeature2Object
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiOfflinePostprocEncode::DestroyFeature2Object()
{
    if (NULL != m_pFeature2Base)
    {
        m_pFeature2Base->Destroy();
        m_pFeature2Base = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiOfflinePostprocEncode::OutputImageReceived
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiOfflinePostprocEncode::OutputImageReceived()
{
    CHX_LOG_INFO("Output notifiation is received for framenum %u", m_frameNumber);
    m_pFeature2RequestStateComplete->Signal();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiOfflinePostprocEncode::UpdateDebugMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiOfflinePostprocEncode::UpdateDebugMetadata(
    ChiMetadata* pMetadata)
{
    CDKResult result            = CDKResultSuccess;
    UINT32    isEnable3A        = ExtensionModule::GetInstance()->Enable3ADebugData();
    UINT32    isEnableTuning    = ExtensionModule::GetInstance()->EnableTuningMetadata();

    if ((NULL != m_pCameraMetadata) && ((TRUE == isEnable3A) || (TRUE == isEnableTuning)))
    {
        DebugData debugData = {0};

        if (NULL != m_pDebugDataPtr)
        {
            // Overwrite with empty Debug data first
            result = ChxUtils::AndroidMetadata::SetVendorTagValue(static_cast<VOID*>(m_pCameraMetadata),
                                                                  VendorTag::DebugDataTag,
                                                                  sizeof(DebugData),
                                                                  reinterpret_cast<VOID*>(&debugData));
            CHX_DELETE[] m_pDebugDataPtr;
            m_pDebugDataPtr = NULL;
        }

        // Function that needs to be modified based on OEM need
        result = ProcessVendorDebugMetadata(pMetadata);

        if (CDKResultSuccess == result)
        {
            debugData.pData = m_pDebugDataPtr;
            debugData.size  = m_debugDataSize;

            result = ChxUtils::AndroidMetadata::SetVendorTagValue(static_cast<VOID*>(m_pCameraMetadata),
                                                                  VendorTag::DebugDataTag,
                                                                  sizeof(DebugData),
                                                                  reinterpret_cast<VOID*>(&debugData));

            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("DebugDataTag set is failed");
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiOfflinePostprocEncode::FreeCameraMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiOfflinePostprocEncode::FreeCameraMetadata()
{
    if (NULL != m_pCameraMetadata)
    {
        free_camera_metadata(m_pCameraMetadata);
        m_pCameraMetadata = NULL;
    }

    if (NULL != m_pDebugDataPtr)
    {
        CHX_DELETE[] m_pDebugDataPtr;
        m_pDebugDataPtr = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiOfflinePostprocEncode::ProcessVendorDebugMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiOfflinePostprocEncode::ProcessVendorDebugMetadata(
    ChiMetadata* pMetadata)
{
    CDKResult  result             = CDKResultEInvalidArg;
    UINT32     vendorDebugMetaTag = 0;
    CHITAGSOPS vendorTagOps;

    // Needs to be modified per OEM need basis
    const CHAR sectionName[] = "org.quic.camera.debugdata";
    const CHAR tagName[]     = "DebugDataAll";

    ExtensionModule::GetInstance()->GetVendorTagOps(&vendorTagOps);

    result = vendorTagOps.pQueryVendorTagLocation(sectionName, tagName, &vendorDebugMetaTag);

    VOID* pMetadataTag = pMetadata->GetTag(vendorDebugMetaTag);

    if (NULL != pMetadataTag)
    {
        UCHAR* pSrcMetadata = static_cast<UCHAR*>(pMetadataTag) + sizeof(UINT32);
        UINT32 metadataSize = *(static_cast<UINT32*>(pMetadataTag));

        m_debugDataSize = metadataSize * 4;
        m_pDebugDataPtr = CHX_NEW UCHAR[m_debugDataSize];

        // Add further logic if anything specific needs to be done to convert this
        // OEM specific Metadata tag into QC Specific DebugDataTag
        ChxUtils::Memcpy(m_pDebugDataPtr, pSrcMetadata, metadataSize);

        result = CDKResultSuccess;
    }
    else
    {
        result = CDKResultENoSuch;
        CHX_LOG_VERBOSE("vendorTag %s, vendorDebugMetaTag %u is not found!",
                        tagName, vendorDebugMetaTag);
    }

    return result;
}
