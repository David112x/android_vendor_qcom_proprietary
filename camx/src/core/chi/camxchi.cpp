////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchi.cpp
/// @brief Landing methods for CamX implementation of CHI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @todo (CAMX-2491) Avoid having in here
#include <system/camera_metadata.h>
#include "camera_metadata_hidden.h"

#include "camxchi.h"
#include "camxchitypes.h"
#include "camxchicontext.h"
#include "camxdebug.h"
#include "camxhal3defaultrequest.h"
#include "camxhal3metadatautil.h"
#include "camxmetabuffer.h"
#include "camxncsservice.h"
#include "camxosutils.h"
#include "camxpipeline.h"
#include "camxtrace.h"
#include "camxutils.h"
#include "camxvendortags.h"
#include "camxchiversion.h"

#include "chibinarylog.h"

CAMX_NAMESPACE_BEGIN

static ChiContext* g_pChiContext = NULL;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constant Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const UINT32 CMBClientFrameNumberBitCount = 24;

// Number of settings tokens
static const UINT32 NumExtendSettings = static_cast<UINT32>(ChxSettingsToken::CHX_SETTINGS_TOKEN_COUNT);

/// @todo (CAMX-1223) Remove below and set the vendor tag ops in hal3test/Chinativetest
// Global vendor tag ops
vendor_tag_ops_t g_vendorTagOps;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FillSettingTokenList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FillSettingTokenList(
    CHISETTINGTOKEN* pTokens)
{
#define REGISTER_TOKEN(inToken, data)                                        \
    pTokens[static_cast<UINT>(inToken)].id    = static_cast<UINT>(inToken);  \
    pTokens[static_cast<UINT>(inToken)].size  = sizeof(data);

#define REGISTER_BIT_TOKEN(inToken, data)                                        \
    pTokens[static_cast<UINT>(inToken)].id    = static_cast<UINT>(inToken);  \
    pTokens[static_cast<UINT>(inToken)].size  = sizeof(VOID*);

    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

    REGISTER_TOKEN(ChxSettingsToken::OverrideForceUsecaseId,            pStaticSettings->overrideForceUsecaseId);
    REGISTER_TOKEN(ChxSettingsToken::OverrideDisableZSL,                pStaticSettings->overrideDisableZSL);
    REGISTER_TOKEN(ChxSettingsToken::OverrideGPURotationUsecase,        pStaticSettings->overrideGPURotationUsecase);
    REGISTER_TOKEN(ChxSettingsToken::OverrideEnableMFNR,                pStaticSettings->overrideEnableMFNR);
    REGISTER_TOKEN(ChxSettingsToken::AnchorSelectionAlgoForMFNR,        pStaticSettings->anchorSelectionAlgoForMFNR);
    REGISTER_TOKEN(ChxSettingsToken::OverrideHFRNo3AUseCase,            pStaticSettings->overrideHFRNo3AUseCase);
    REGISTER_TOKEN(ChxSettingsToken::OverrideForceSensorMode,           pStaticSettings->overrideForceSensorMode);
    REGISTER_TOKEN(ChxSettingsToken::DefaultMaxFPS,                     pStaticSettings->defaultMaxFPS);
    REGISTER_TOKEN(ChxSettingsToken::FovcEnable,                        pStaticSettings->fovcEnable);
    REGISTER_TOKEN(ChxSettingsToken::OverrideCameraClose,               pStaticSettings->overrideCameraClose);
    REGISTER_TOKEN(ChxSettingsToken::OverrideCameraOpen,                pStaticSettings->overrideCameraOpen);
    REGISTER_TOKEN(ChxSettingsToken::EISV2Enable,                       pStaticSettings->EISV2Enable);
    REGISTER_TOKEN(ChxSettingsToken::EISV3Enable,                       pStaticSettings->EISV3Enable);
    REGISTER_TOKEN(ChxSettingsToken::NumPCRsBeforeStreamOn,             pStaticSettings->numPCRsBeforeStreamOn);
    REGISTER_TOKEN(ChxSettingsToken::StatsProcessingSkipFactor,         pStaticSettings->statsProcessingSkipFactor);
    REGISTER_TOKEN(ChxSettingsToken::DumpDebugDataEveryProcessResult,   pStaticSettings->dumpDebugDataEveryProcessResult);
    REGISTER_BIT_TOKEN(ChxSettingsToken::Enable3ADebugData,             pStaticSettings->enable3ADebugData);
    REGISTER_BIT_TOKEN(ChxSettingsToken::EnableConcise3ADebugData,      pStaticSettings->enableConcise3ADebugData);
    REGISTER_BIT_TOKEN(ChxSettingsToken::EnableTuningMetadata,          pStaticSettings->enableTuningMetadata);
    REGISTER_TOKEN(ChxSettingsToken::DebugDataSizeAEC,                  pStaticSettings->debugDataSizeAEC);
    REGISTER_TOKEN(ChxSettingsToken::DebugDataSizeAWB,                  pStaticSettings->debugDataSizeAWB);
    REGISTER_TOKEN(ChxSettingsToken::DebugDataSizeAF,                   pStaticSettings->debugDataSizeAF);
    REGISTER_TOKEN(ChxSettingsToken::ConciseDebugDataSizeAEC,           pStaticSettings->conciseDebugDataSizeAEC);
    REGISTER_TOKEN(ChxSettingsToken::ConciseDebugDataSizeAWB,           pStaticSettings->conciseDebugDataSizeAWB);
    REGISTER_TOKEN(ChxSettingsToken::ConciseDebugDataSizeAF,            pStaticSettings->conciseDebugDataSizeAF);
    REGISTER_TOKEN(ChxSettingsToken::TuningDumpDataSizeIFE,             pStaticSettings->tuningDumpDataSizeIFE);
    REGISTER_TOKEN(ChxSettingsToken::TuningDumpDataSizeIPE,             pStaticSettings->tuningDumpDataSizeIPE);
    REGISTER_TOKEN(ChxSettingsToken::TuningDumpDataSizeBPS,             pStaticSettings->tuningDumpDataSizeBPS);
    REGISTER_TOKEN(ChxSettingsToken::MultiCameraVREnable,               pStaticSettings->multiCameraVREnable);
    REGISTER_TOKEN(ChxSettingsToken::OverrideGPUDownscaleUsecase,       pStaticSettings->overrideGPUDownscaleUsecase);
    REGISTER_TOKEN(ChxSettingsToken::AdvanceFeatureMask,                pStaticSettings->advanceFeatureMask);
    REGISTER_TOKEN(ChxSettingsToken::DisableASDStatsProcessing,         pStaticSettings->disableASDStatsProcessing);
    REGISTER_TOKEN(ChxSettingsToken::MultiCameraFrameSync,              pStaticSettings->multiCameraFrameSync);
    REGISTER_TOKEN(ChxSettingsToken::OutputFormat,                      pStaticSettings->outputFormat);
    REGISTER_TOKEN(ChxSettingsToken::EnableCHIPartialData,              pStaticSettings->enableCHIPartialData);
    REGISTER_TOKEN(ChxSettingsToken::EnableFDStreamInRealTime,          pStaticSettings->enableFDStreamInRealTime);
    REGISTER_TOKEN(ChxSettingsToken::SelectInSensorHDR3ExpUsecase,      pStaticSettings->selectInSensorHDR3ExpUsecase);
    REGISTER_TOKEN(ChxSettingsToken::EnableUnifiedBufferManager,        pStaticSettings->enableUnifiedBufferManager);
    REGISTER_TOKEN(ChxSettingsToken::EnableCHIImageBufferLateBinding,   pStaticSettings->enableCHIImageBufferLateBinding);
    REGISTER_TOKEN(ChxSettingsToken::EnableCHIPartialDataRecovery,      pStaticSettings->enableCHIPartialDataRecovery);
    REGISTER_TOKEN(ChxSettingsToken::UseFeatureForQCFA,                 pStaticSettings->useFeatureForQCFA);
    REGISTER_TOKEN(ChxSettingsToken::AECGainThresholdForQCFA,           pStaticSettings->AECGainThresholdForQCFA);
    REGISTER_TOKEN(ChxSettingsToken::EnableOfflineNoiseReprocess,       pStaticSettings->enableOfflineNoiseReprocess);
    REGISTER_TOKEN(ChxSettingsToken::EnableAsciilog,                    pStaticSettings->enableAsciiLogging);
    REGISTER_TOKEN(ChxSettingsToken::EnableBinarylog,                   pStaticSettings->enableBinaryLogging);
    REGISTER_TOKEN(ChxSettingsToken::OverrideLogLevels,                 pStaticSettings->overrideLogLevels);
    REGISTER_TOKEN(ChxSettingsToken::EnableFeature2Dump,                pStaticSettings->enableFeature2Dump);
    REGISTER_TOKEN(ChxSettingsToken::ForceHWMFFixedNumOfFrames,         pStaticSettings->forceHWMFFixedNumOfFrames);
    REGISTER_TOKEN(ChxSettingsToken::ForceSWMFFixedNumOfFrames,         pStaticSettings->forceSWMFFixedNumOfFrames);
    REGISTER_TOKEN(ChxSettingsToken::EnableTBMChiFence,                 pStaticSettings->enableTBMChiFence);
    REGISTER_TOKEN(ChxSettingsToken::EnableRawHDR,                      pStaticSettings->enableRawHDR);
    REGISTER_BIT_TOKEN(ChxSettingsToken::EnableRequestMapping,          pStaticSettings->logRequestMapping);
    REGISTER_BIT_TOKEN(ChxSettingsToken::EnableSystemLogging,           pStaticSettings->systemLogEnable);
    REGISTER_TOKEN(ChxSettingsToken::BPSRealtimeSensorId,               pStaticSettings->bpsRealtimeSensorId);
    REGISTER_TOKEN(ChxSettingsToken::EnableMFSRChiFence,                pStaticSettings->enableMFSRChiFence);
    REGISTER_TOKEN(ChxSettingsToken::MultiCameraJPEG,                   pStaticSettings->multiCameraJPEG);
    REGISTER_TOKEN(ChxSettingsToken::MaxHALRequests,                    pStaticSettings->maxHalRequests);
    REGISTER_TOKEN(ChxSettingsToken::MultiCameraHWSyncMask,             pStaticSettings->multiCameraHWSyncMask);
    REGISTER_TOKEN(ChxSettingsToken::AnchorAlgoSelectionType,           pStaticSettings->anchorAlgoSelectionType);
    REGISTER_TOKEN(ChxSettingsToken::EnableBLMClient,                   pStaticSettings->enableBLMClient);
    REGISTER_TOKEN(ChxSettingsToken::OverrideForceBurstShot,            pStaticSettings->overrideForceBurstShot);
    REGISTER_BIT_TOKEN(ChxSettingsToken::ExposeFullSizeForQCFA,         pStaticSettings->exposeFullSizeForQCFA);
    REGISTER_BIT_TOKEN(ChxSettingsToken::EnableScreenGrab,              pStaticSettings->enableScreenGrab);
#undef REGISTER_SETTING
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PopulateSettingsData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PopulateSettingsData(
    CHIMODIFYSETTINGS* pSettings)
{
    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

    // decltype()  returns the declaration type, the right hand side of the assignment in the code below is to let us use a
    // const cast in the macro, otherwise whiner whines and NOWHINEs are being ignored in the macro.
#define ADD_SETTING(inToken, data)                                                    \
    pSettings[static_cast<UINT>(inToken)].pData       = (decltype(data)*)(&(data));   \
    pSettings[static_cast<UINT>(inToken)].token.id    = static_cast<UINT32>(inToken); \
    pSettings[static_cast<UINT>(inToken)].token.size  = sizeof(data);

#define ADD_BIT_SETTING(inToken, data)                                                \
    pSettings[static_cast<UINT>(inToken)].pData       = (VOID*)(data == TRUE);        \
    pSettings[static_cast<UINT>(inToken)].token.id    = static_cast<UINT32>(inToken); \
    pSettings[static_cast<UINT>(inToken)].token.size  = sizeof(VOID*);


    ADD_SETTING(ChxSettingsToken::OverrideForceUsecaseId, pStaticSettings->overrideForceUsecaseId);
    ADD_SETTING(ChxSettingsToken::OverrideDisableZSL, pStaticSettings->overrideDisableZSL);
    ADD_SETTING(ChxSettingsToken::OverrideGPURotationUsecase, pStaticSettings->overrideGPURotationUsecase);
    ADD_SETTING(ChxSettingsToken::OverrideEnableMFNR, pStaticSettings->overrideEnableMFNR);
    ADD_SETTING(ChxSettingsToken::AnchorSelectionAlgoForMFNR, pStaticSettings->anchorSelectionAlgoForMFNR);
    ADD_SETTING(ChxSettingsToken::OverrideHFRNo3AUseCase, pStaticSettings->overrideHFRNo3AUseCase);
    ADD_SETTING(ChxSettingsToken::OverrideForceSensorMode, pStaticSettings->overrideForceSensorMode);
    ADD_SETTING(ChxSettingsToken::DefaultMaxFPS, pStaticSettings->defaultMaxFPS);
    ADD_SETTING(ChxSettingsToken::FovcEnable, pStaticSettings->fovcEnable);
    ADD_SETTING(ChxSettingsToken::OverrideCameraClose, pStaticSettings->overrideCameraClose);
    ADD_SETTING(ChxSettingsToken::OverrideCameraOpen, pStaticSettings->overrideCameraOpen);
    ADD_SETTING(ChxSettingsToken::EISV2Enable, pStaticSettings->EISV2Enable);
    ADD_SETTING(ChxSettingsToken::EISV3Enable, pStaticSettings->EISV3Enable);
    ADD_SETTING(ChxSettingsToken::NumPCRsBeforeStreamOn, pStaticSettings->numPCRsBeforeStreamOn);
    ADD_SETTING(ChxSettingsToken::StatsProcessingSkipFactor, pStaticSettings->statsProcessingSkipFactor);
    ADD_SETTING(ChxSettingsToken::DumpDebugDataEveryProcessResult, pStaticSettings->dumpDebugDataEveryProcessResult);
    ADD_BIT_SETTING(ChxSettingsToken::Enable3ADebugData, pStaticSettings->enable3ADebugData);
    ADD_BIT_SETTING(ChxSettingsToken::EnableConcise3ADebugData, pStaticSettings->enableConcise3ADebugData);
    ADD_BIT_SETTING(ChxSettingsToken::EnableTuningMetadata, pStaticSettings->enableTuningMetadata);
    ADD_SETTING(ChxSettingsToken::DebugDataSizeAEC, pStaticSettings->debugDataSizeAEC);
    ADD_SETTING(ChxSettingsToken::DebugDataSizeAWB, pStaticSettings->debugDataSizeAWB);
    ADD_SETTING(ChxSettingsToken::DebugDataSizeAF, pStaticSettings->debugDataSizeAF);
    ADD_SETTING(ChxSettingsToken::ConciseDebugDataSizeAEC, pStaticSettings->conciseDebugDataSizeAEC);
    ADD_SETTING(ChxSettingsToken::ConciseDebugDataSizeAWB, pStaticSettings->conciseDebugDataSizeAWB);
    ADD_SETTING(ChxSettingsToken::ConciseDebugDataSizeAF, pStaticSettings->conciseDebugDataSizeAF);
    ADD_SETTING(ChxSettingsToken::TuningDumpDataSizeIFE, pStaticSettings->tuningDumpDataSizeIFE);
    ADD_SETTING(ChxSettingsToken::TuningDumpDataSizeIPE, pStaticSettings->tuningDumpDataSizeIPE);
    ADD_SETTING(ChxSettingsToken::TuningDumpDataSizeBPS, pStaticSettings->tuningDumpDataSizeBPS);
    ADD_SETTING(ChxSettingsToken::MultiCameraVREnable, pStaticSettings->multiCameraVREnable);
    ADD_SETTING(ChxSettingsToken::OverrideGPUDownscaleUsecase, pStaticSettings->overrideGPUDownscaleUsecase);
    ADD_SETTING(ChxSettingsToken::AdvanceFeatureMask, pStaticSettings->advanceFeatureMask);
    ADD_SETTING(ChxSettingsToken::DisableASDStatsProcessing, pStaticSettings->disableASDStatsProcessing);
    ADD_SETTING(ChxSettingsToken::MultiCameraFrameSync, pStaticSettings->multiCameraFrameSync);
    ADD_SETTING(ChxSettingsToken::OutputFormat, pStaticSettings->outputFormat);
    ADD_SETTING(ChxSettingsToken::EnableCHIPartialData, pStaticSettings->enableCHIPartialData);
    ADD_SETTING(ChxSettingsToken::EnableFDStreamInRealTime, pStaticSettings->enableFDStreamInRealTime);
    ADD_SETTING(ChxSettingsToken::SelectInSensorHDR3ExpUsecase, pStaticSettings->selectInSensorHDR3ExpUsecase);
    ADD_SETTING(ChxSettingsToken::EnableUnifiedBufferManager, pStaticSettings->enableUnifiedBufferManager);
    ADD_SETTING(ChxSettingsToken::EnableCHIImageBufferLateBinding, pStaticSettings->enableCHIImageBufferLateBinding);
    ADD_SETTING(ChxSettingsToken::EnableCHIPartialDataRecovery, pStaticSettings->enableCHIPartialDataRecovery);
    ADD_SETTING(ChxSettingsToken::UseFeatureForQCFA, pStaticSettings->useFeatureForQCFA);
    ADD_SETTING(ChxSettingsToken::AECGainThresholdForQCFA, pStaticSettings->AECGainThresholdForQCFA);
    ADD_SETTING(ChxSettingsToken::EnableOfflineNoiseReprocess, pStaticSettings->enableOfflineNoiseReprocess);
    ADD_SETTING(ChxSettingsToken::EnableAsciilog, pStaticSettings->enableAsciiLogging);
    ADD_SETTING(ChxSettingsToken::EnableBinarylog, pStaticSettings->enableBinaryLogging);
    ADD_SETTING(ChxSettingsToken::OverrideLogLevels, pStaticSettings->overrideLogLevels);
    ADD_SETTING(ChxSettingsToken::EnableFeature2Dump, pStaticSettings->enableFeature2Dump);
    ADD_SETTING(ChxSettingsToken::ForceHWMFFixedNumOfFrames, pStaticSettings->forceHWMFFixedNumOfFrames);
    ADD_SETTING(ChxSettingsToken::ForceSWMFFixedNumOfFrames, pStaticSettings->forceSWMFFixedNumOfFrames);
    ADD_SETTING(ChxSettingsToken::EnableTBMChiFence, pStaticSettings->enableTBMChiFence);
    ADD_SETTING(ChxSettingsToken::EnableRawHDR, pStaticSettings->enableRawHDR);
    ADD_BIT_SETTING(ChxSettingsToken::EnableRequestMapping, pStaticSettings->logRequestMapping);
    ADD_BIT_SETTING(ChxSettingsToken::EnableSystemLogging,  pStaticSettings->systemLogEnable);
    ADD_SETTING(ChxSettingsToken::BPSRealtimeSensorId, pStaticSettings->bpsRealtimeSensorId);
    ADD_SETTING(ChxSettingsToken::EnableMFSRChiFence, pStaticSettings->enableMFSRChiFence);
    ADD_SETTING(ChxSettingsToken::MultiCameraJPEG, pStaticSettings->multiCameraJPEG);
    ADD_SETTING(ChxSettingsToken::MaxHALRequests, pStaticSettings->maxHalRequests);
    ADD_SETTING(ChxSettingsToken::MultiCameraHWSyncMask, pStaticSettings->multiCameraHWSyncMask);
    ADD_SETTING(ChxSettingsToken::AnchorAlgoSelectionType, pStaticSettings->anchorAlgoSelectionType);
    ADD_SETTING(ChxSettingsToken::EnableBLMClient, pStaticSettings->enableBLMClient);
    ADD_SETTING(ChxSettingsToken::OverrideForceBurstShot, pStaticSettings->overrideForceBurstShot);
    ADD_BIT_SETTING(ChxSettingsToken::ExposeFullSizeForQCFA, pStaticSettings->exposeFullSizeForQCFA);
    ADD_BIT_SETTING(ChxSettingsToken::EnableScreenGrab, pStaticSettings->enableScreenGrab);
#undef ADD_SETTING
#undef ADD_BIT_SETTING

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetChiContext
///
/// @brief  Extract the CHI device pointer from the camera3_device_t.
///         Precondition: pCamera3DeviceAPI has been checked for NULL
///
/// @param  pCamera3DeviceAPI The camera3_device_t pointer passed in from the application framework. Assumed that it has been
///                           checked against NULL.
///
/// @return A pointer to the CHIDevice object held in the opaque point in camera3_device_t.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CAMX_INLINE ChiContext* GetChiContext(
    CHIHANDLE hChiContext)
{
    CAMX_ASSERT(NULL != hChiContext);

    return reinterpret_cast<ChiContext*>(hChiContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetChiSession
///
/// @brief  Get the CHI session pointer
///
/// @param  session session handle
///
/// @return A pointer to the CHISession object held in the opaque point in camera3_device_t.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CAMX_INLINE CHISession* GetChiSession(
    CHIHANDLE hSession)
{
    CAMX_ASSERT(NULL != hSession);

    return reinterpret_cast<CHISession*>(hSession);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetChiPipelineDescriptor
///
/// @brief  Get the CHI Pipeline pointer
///
/// @param  hPipelineDescriptor Pipeline pointer
///
/// @return A pointer to the GetChiPipeline object held in the opaque point in camera3_device_t.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CAMX_INLINE PipelineDescriptor* GetChiPipelineDescriptor(
    CHIPIPELINEHANDLE hPipelineDescriptor)
{
    CAMX_ASSERT(NULL != hPipelineDescriptor);

    return reinterpret_cast<PipelineDescriptor*>(hPipelineDescriptor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiGetTagCount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CAMX_INLINE INT ChiGetTagCount(
    const vendor_tag_ops_t* pVendorTagOpsAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3GetTagCount);

    // Per Google, this parameter is extraneous
    CAMX_UNREFERENCED_PARAM(pVendorTagOpsAPI);

    return static_cast<INT>(VendorTagManager::GetTagCount(TagSectionVisibility::TagSectionVisibleToFramework));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiGetAllTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void CAMX_INLINE ChiGetAllTags(
    const vendor_tag_ops_t* pVendorTagOpsAPI,
    uint32_t*               pTagArrayAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3GetAllTags);

    // Per Google, this parameter is extraneous
    CAMX_UNREFERENCED_PARAM(pVendorTagOpsAPI);

    CAMX_STATIC_ASSERT(sizeof(pTagArrayAPI[0]) == sizeof(VendorTag));

    CAMX_ASSERT(NULL != pTagArrayAPI);
    if (NULL != pTagArrayAPI)
    {
        VendorTag* pVendorTags = static_cast<VendorTag*>(pTagArrayAPI);
        VendorTagManager::GetAllTags(pVendorTags, TagSectionVisibility::TagSectionVisibleToFramework);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid argument 2 for get_all_tags()");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiGetSectionName
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const CHAR* ChiGetSectionName(
    const vendor_tag_ops_t* pVendorTagOpsAPI,
    uint32_t                tagAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3GetSectionName);

    // Per Google, this parameter is extraneous
    CAMX_UNREFERENCED_PARAM(pVendorTagOpsAPI);

    CAMX_STATIC_ASSERT(sizeof(tagAPI) == sizeof(VendorTag));

    return VendorTagManager::GetSectionName(static_cast<VendorTag>(tagAPI));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiGetTagName
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const CHAR* ChiGetTagName(
    const vendor_tag_ops_t* pVendorTagOpsAPI,
    uint32_t                tagAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3GetTagName);

    // Per Google, this parameter is extraneous
    CAMX_UNREFERENCED_PARAM(pVendorTagOpsAPI);

    CAMX_STATIC_ASSERT(sizeof(tagAPI) == sizeof(VendorTag));

    return VendorTagManager::GetTagName(static_cast<VendorTag>(tagAPI));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiGetTagType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static INT ChiGetTagType(
    const vendor_tag_ops_t* pVendorTagOpsAPI,
    uint32_t                tagAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3GetTagType);

    // Per Google, this parameter is extraneous
    CAMX_UNREFERENCED_PARAM(pVendorTagOpsAPI);

    CAMX_STATIC_ASSERT(sizeof(tagAPI) == sizeof(VendorTag));

    VendorTagType vendorTagType = VendorTagManager::GetTagType(static_cast<VendorTag>(tagAPI));
    return static_cast<INT>(vendorTagType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiQueryVendorTagLocation
///
/// @brief  Query vendor tag location assigned by vendor tag manager
///
/// @return CDKResultSuccess if successful otherwise CDKResultNoSuch
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiQueryVendorTagLocation(
    const CHAR* pSectionName,
    const CHAR* pTagName,
    UINT32*     pTagLocation)
{
    return VendorTagManager::QueryVendorTagLocation(pSectionName, pTagName, pTagLocation);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiSetMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiSetMetaData(
    CHIHANDLE handle,
    UINT32    tag,
    VOID*     pData,
    SIZE_T    count)
{
    SIZE_T unit = count / HAL3MetadataUtil::GetSizeByType(HAL3MetadataUtil::GetTypeByTag(tag));
    return HAL3MetadataUtil::SetMetadata(handle, tag, pData, unit);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiGetMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiGetMetaData(
    CHIHANDLE handle,
    UINT32    tag,
    VOID*     pData,
    SIZE_T    count)
{
    CDKResult   result  = CDKResultSuccess;
    VOID*       pBuffer = NULL;

    result = HAL3MetadataUtil::GetMetadata(handle, tag, &pBuffer);
    if ((NULL != pData) && (NULL != pBuffer))
    {
        Utils::Memcpy(pData, pBuffer, count);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiGetTagOps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID ChiGetTagOps(
    CHITAGSOPS* pTagOps)
{
    if (NULL != pTagOps)
    {
        pTagOps->pQueryVendorTagLocation = ChiQueryVendorTagLocation;
        pTagOps->pSetMetaData            = ChiSetMetaData;
        pTagOps->pGetMetaData            = ChiGetMetaData;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiCreateFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiCreateFence(
    CHIHANDLE              hChiContext,
    CHIFENCECREATEPARAMS*  pInfo,
    CHIFENCEHANDLE*        phChiFence)
{
    CDKResult   cdkResult   = CDKResultSuccess;

    if ((NULL == hChiContext) || (NULL == pInfo) || (NULL == phChiFence))
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid input args hChiContext=%p, pInfo=%p, phChiFence=%p",
                       hChiContext, pInfo, phChiFence);

        cdkResult = CDKResultEInvalidArg;
    }
    else if (pInfo->size != sizeof(CHIFENCECREATEPARAMS))
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "CreateFence Info size mismatch expected size %u incoming size %u",
                       static_cast<UINT32>(sizeof(CHIFENCECREATEPARAMS)), pInfo->size);

        cdkResult = CDKResultEInvalidArg;
    }
    else
    {
        CamxResult  result;
        ChiContext* pChiContext = GetChiContext(hChiContext);

        CAMX_ASSERT(NULL != pChiContext);

        result = pChiContext->CreateChiFence(pInfo, phChiFence);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "Create ChiFence failed with error %d, type=%d", result, pInfo->type);

            cdkResult = CDKResultEFailed;
        }
        else
        {
            CHIFENCECREATEPARAMS& rChiFenceCreateParams = *pInfo;
            CHIFENCEHANDLE&       rhChiFence             = *phChiFence;
            BINARY_LOG(LogEvent::CamXChi_CreateFence, hChiContext, rChiFenceCreateParams, rhChiFence, phChiFence);
        }
    }

    return cdkResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiReleaseFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiReleaseFence(
    CHIHANDLE       hChiContext,
    CHIFENCEHANDLE  hChiFence)
{
    CDKResult   cdkResult   = CDKResultSuccess;

    if ((NULL != hChiContext) && (NULL != hChiFence))
    {
        CamxResult  result;
        ChiContext* pChiContext = GetChiContext(hChiContext);

        CAMX_ASSERT(NULL != pChiContext);

        result = pChiContext->ReleaseChiFence(hChiFence);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "Release ChiFence failed with error %d.", result);

            cdkResult = CDKResultEFailed;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid input args hChiContext=%p, hChiFence=%p", hChiContext, hChiFence);
        cdkResult = CDKResultEInvalidArg;
    }

    return cdkResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiWaitFenceAsync
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiWaitFenceAsync(
    CHIHANDLE            hChiContext,
    PFNCHIFENCECALLBACK  pCallbackFn,
    CHIFENCEHANDLE       hChiFence,
    VOID*                pData)
{
    CDKResult   cdkResult   = CDKResultSuccess;

    if ((NULL != hChiContext) && (NULL != pCallbackFn) && (NULL != hChiFence) && (NULL == pData))
    {
        CamxResult  result;
        ChiContext* pChiContext = GetChiContext(hChiContext);

        CAMX_ASSERT(NULL != pChiContext);

        result = pChiContext->WaitChiFence(hChiFence, pCallbackFn, pData, UINT64_MAX);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "Wait ChiFence failed with error %d.", result);

            cdkResult = CDKResultEFailed;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid input args hChiContext=%p, pCallbackFn=%p, hChiFence=%p, pData=%p",
                       hChiContext, pCallbackFn, hChiFence, pData);

        cdkResult = CDKResultEInvalidArg;
    }

    return cdkResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiSignalFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiSignalFence(
    CHIHANDLE       hChiContext,
    CHIFENCEHANDLE  hChiFence,
    CDKResult       statusResult)
{
    CDKResult   cdkResult   = CDKResultSuccess;

    if ((NULL != hChiContext) && (NULL != hChiFence))
    {
        ChiContext* pChiContext = GetChiContext(hChiContext);
        CamxResult  result      = (CDKResultSuccess == statusResult) ? CamxResultSuccess : CamxResultEFailed;

        CAMX_ASSERT(NULL != pChiContext);

        result = pChiContext->SignalChiFence(hChiFence, result);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "Signal ChiFence failed with error %d.", result);
            cdkResult = CDKResultEFailed;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid input args hChiContext=%p, hChiFence=%p", hChiContext, hChiFence);
        cdkResult = CDKResultEInvalidArg;
    }

    return cdkResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiGetFenceStatus
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiGetFenceStatus(
    CHIHANDLE       hChiContext,
    CHIFENCEHANDLE  hChiFence,
    CDKResult*      pFenceResult)
{
    CDKResult   cdkResult   = CDKResultSuccess;
    CamxResult  result      = CamxResultSuccess;

    if ((NULL != hChiContext) && (NULL != hChiFence) && (NULL != pFenceResult))
    {
        ChiContext* pChiContext = GetChiContext(hChiContext);

        CAMX_ASSERT(NULL != pChiContext);

        result = pChiContext->GetChiFenceResult(hChiFence, &cdkResult);

        *pFenceResult = cdkResult;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid input args hChiContext=%p, hChiFence=%p, pFenceResult=%p",
                       hChiContext, hChiFence, pFenceResult);
        cdkResult = CDKResultEInvalidArg;
    }

    return cdkResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiGetFenceOps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID ChiGetFenceOps(
    CHIFENCEOPS*  pFenceOps)
{
    if (NULL != pFenceOps)
    {
        pFenceOps->pCreateFence    = ChiCreateFence;
        pFenceOps->pReleaseFence   = ChiReleaseFence;
        pFenceOps->pWaitFenceAsync = ChiWaitFenceAsync;
        pFenceOps->pSignalFence    = ChiSignalFence;
        pFenceOps->pGetFenceStatus = ChiGetFenceStatus;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid input pointer, Fence Operations aren't available");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferCreate(
    CHIMETAHANDLE*     phMetaHandle,
    CHIMETAPRIVATEDATA pPrivateData)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT(NULL != phMetaHandle);

    if (NULL != phMetaHandle)
    {
        MetaBuffer* pMetaBuffer = MetaBuffer::Create(pPrivateData);
        if (NULL != pMetaBuffer)
        {
            *phMetaHandle = pMetaBuffer;
            result        = CDKResultSuccess;
        }
        else
        {
            result = CDKResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferCreateWithTaglist
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferCreateWithTaglist(
    const UINT32*      pTagList,
    UINT32             tagListCount,
    CHIMETAHANDLE*     phMetaHandle,
    CHIMETAPRIVATEDATA pPrivateData)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT((NULL != phMetaHandle) && (NULL != pTagList) && (0 < tagListCount));

    if ((NULL != phMetaHandle) && (NULL != pTagList) && (0 < tagListCount))
    {
        MetaBuffer* pMetaBuffer = MetaBuffer::Create(pPrivateData);
        if (NULL != pMetaBuffer)
        {
            result = pMetaBuffer->AllocateBuffer(pTagList, tagListCount);
            if (CamxResultSuccess == result)
            {
                *phMetaHandle = pMetaBuffer;
                result        = CDKResultSuccess;
            }
            else
            {
                pMetaBuffer->Destroy();
            }
        }
        else
        {
            result = CDKResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferCreateWithAndroidMeta
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferCreateWithAndroidMeta(
    const VOID*        pAndroidMeta,
    CHIMETAHANDLE*     phMetaHandle,
    CHIMETAPRIVATEDATA pPrivateData)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT((NULL != phMetaHandle) && (NULL != pAndroidMeta));

    if ((NULL != phMetaHandle) && (NULL != pAndroidMeta))
    {
        MetaBuffer* pMetaBuffer = MetaBuffer::Create(pPrivateData);
        if (NULL != pMetaBuffer)
        {
            const camera_metadata* pMetadata = static_cast<const camera_metadata*>(pAndroidMeta);

            result = pMetaBuffer->AllocateBuffer(pMetadata);
            if (CamxResultSuccess == result)
            {
                *phMetaHandle = pMetaBuffer;
            }
            else
            {
                pMetaBuffer->Destroy();
            }
        }
        else
        {
            result = CDKResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferDestroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferDestroy(
    CHIMETAHANDLE hMetaHandle,
    BOOL          force)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT(NULL != hMetaHandle);

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);
    if ((NULL != pMetaBuffer) && (TRUE == MetaBuffer::IsValid(pMetaBuffer)))
    {
        result = pMetaBuffer->Destroy(force);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferEntryCopy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID ChiMetabufferEntryCopy(
    CHIMETADATAENTRY*   pDstEntry,
    MetaBuffer::Entry*  pSrcEntry)
{
    pDstEntry->count    = pSrcEntry->count;
    pDstEntry->size     = pSrcEntry->size;
    pDstEntry->pTagData = pSrcEntry->pTagData;
    pDstEntry->tagID    = pSrcEntry->tagID;
    pDstEntry->type     = static_cast<ChiMetadataTagType>(pSrcEntry->type);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferGetTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferGetTag(
    CHIMETAHANDLE hMetaHandle,
    UINT32        tag,
    VOID**        ppData)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT((NULL != hMetaHandle) && (NULL != ppData));

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);
    if ((NULL != pMetaBuffer) && (NULL != ppData) && (TRUE == MetaBuffer::IsValid(pMetaBuffer)))
    {
        *ppData = pMetaBuffer->GetTag(tag);
        if (NULL != *ppData)
        {
            result = CDKResultSuccess;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferGetTagEntry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferGetTagEntry(
    CHIMETAHANDLE       hMetaHandle,
    UINT32              tag,
    CHIMETADATAENTRY*   pEntry)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT((NULL != hMetaHandle) && (NULL != pEntry));

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);
    if ((NULL != pMetaBuffer) && (NULL != pEntry) && (TRUE == MetaBuffer::IsValid(pMetaBuffer)))
    {
        MetaBuffer::Entry entry;
        result = pMetaBuffer->GetTag(tag, entry);

        if (CamxResultSuccess == result)
        {
            ChiMetabufferEntryCopy(pEntry, &entry);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferGetTagByCameraId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferGetTagByCameraId(
    CHIMETAHANDLE hMetaHandle,
    UINT32        tag,
    UINT32        cameraId,
    VOID**        ppData)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT((NULL != hMetaHandle) && (NULL != ppData));

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);
    if ((NULL != pMetaBuffer) && (NULL != ppData) && (TRUE == MetaBuffer::IsValid(pMetaBuffer)))
    {
        *ppData = pMetaBuffer->GetTagByCameraId(tag, cameraId, FALSE);
        if (NULL != *ppData)
        {
            result = CDKResultSuccess;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferGetVendorTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferGetVendorTag(
    CHIMETAHANDLE hMetaHandle,
    const CHAR*   pTagSectionName,
    const CHAR*   pTagName,
    VOID**        ppData)
{
    CDKResult result = CDKResultEInvalidArg;
    UINT32    tag;

    CAMX_ASSERT((NULL != hMetaHandle) && (NULL != ppData) && (NULL != pTagSectionName) && (NULL != pTagName));

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);

    if ((NULL != pTagSectionName) &&
        (NULL != pTagName) &&
        (NULL != pMetaBuffer) &&
        (NULL != ppData) &&
        (TRUE == MetaBuffer::IsValid(pMetaBuffer)))
    {
        result = VendorTagManager::QueryVendorTagLocation(pTagSectionName, pTagName, &tag);

        if (CamxResultSuccess == result)
        {
            *ppData = pMetaBuffer->GetTag(tag);
            if (NULL != *ppData)
            {
                result = CDKResultSuccess;
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferGetVendorTagEntry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferGetVendorTagEntry(
    CHIMETAHANDLE       hMetaHandle,
    const CHAR*         pTagSectionName,
    const CHAR*         pTagName,
    CHIMETADATAENTRY*   pEntry)
{
    CDKResult result = CDKResultEInvalidArg;
    UINT32    tag;

    CAMX_ASSERT((NULL != hMetaHandle) && (NULL != pEntry) && (NULL != pTagSectionName) && (NULL != pTagName));

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);

    if ((NULL != pTagSectionName) &&
        (NULL != pTagName)        &&
        (NULL != pMetaBuffer)     &&
        (NULL != pEntry)          &&
        (TRUE == MetaBuffer::IsValid(pMetaBuffer)))
    {
        MetaBuffer::Entry entry;

        result = VendorTagManager::QueryVendorTagLocation(pTagSectionName, pTagName, &tag);

        if (CamxResultSuccess == result)
        {
            result = pMetaBuffer->GetTag(tag, entry);

            if (CamxResultSuccess == result)
            {
                ChiMetabufferEntryCopy(pEntry, &entry);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferSetVendorTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferSetVendorTag(
    CHIMETAHANDLE hMetaHandle,
    const CHAR*   pTagSectionName,
    const CHAR*   pTagName,
    const VOID*   pData,
    UINT32        count)
{
    CDKResult result = CDKResultEInvalidArg;
    UINT32   tag;

    CAMX_ASSERT((NULL != hMetaHandle) && (NULL != pData) && (NULL != pTagSectionName) && (NULL != pTagName));

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);

    if ((NULL != pTagSectionName) &&
        (NULL != pTagName)        &&
        (NULL != pMetaBuffer)     &&
        (NULL != pData)           &&
        (TRUE == MetaBuffer::IsValid(pMetaBuffer)))
    {
        result = VendorTagManager::QueryVendorTagLocation(pTagSectionName, pTagName, &tag);

        if (CamxResultSuccess == result)
        {
            const MetadataInfo* pMetadataInfo = HAL3MetadataUtil::GetMetadataInfoByTag(tag);
            UINT32              tagType;

            if (NULL != pMetadataInfo)
            {
                result = pMetaBuffer->SetTag(tag, pData, count, TagSizeByType[pMetadataInfo->type] * count);
            }
            else
            {
                result = CDKResultEFailed;
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferSetTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferSetTag(
    CHIMETAHANDLE hMetaHandle,
    UINT32        tag,
    const VOID*   pData,
    UINT32        count)
{
    const MetadataInfo* pMetadataInfo = HAL3MetadataUtil::GetMetadataInfoByTag(tag);
    CDKResult           result        = CDKResultEInvalidArg;
    UINT32              tagType;

    CAMX_ASSERT((NULL != hMetaHandle) && (NULL != pData));

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);
    if ((NULL != pMetaBuffer) &&
        (NULL != pData) &&
        (TRUE == MetaBuffer::IsValid(pMetaBuffer)) &&
        (NULL != pMetadataInfo))
    {
        result = pMetaBuffer->SetTag(tag, pData, count, TagSizeByType[pMetadataInfo->type] * count);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferSetTagWithAndroidMeta
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferSetTagWithAndroidMeta(
    CHIMETAHANDLE hMetaHandle,
    const VOID*   pAndroidMeta)
{
    CDKResult result = CDKResultEInvalidArg;

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);
    if ((NULL != pMetaBuffer) && (NULL != pAndroidMeta) && (TRUE == MetaBuffer::IsValid(pMetaBuffer)))
    {
        const camera_metadata* pMetadata = static_cast<const camera_metadata*>(pAndroidMeta);

        result = pMetaBuffer->SetTag(pMetadata);
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupChi, "Unable to set tag");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferDeleteTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferDeleteTag(
    CHIMETAHANDLE hMetaHandle,
    UINT32        tag)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT(NULL != hMetaHandle);

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);
    if (NULL != pMetaBuffer)
    {
        result = pMetaBuffer->RemoveTag(tag);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferInvalidate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferInvalidate(
    CHIMETAHANDLE hMetaHandle)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT(NULL != hMetaHandle);

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);
    if (NULL != pMetaBuffer)
    {
        result = pMetaBuffer->Invalidate();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferMerge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferMerge(
    CHIMETAHANDLE hDstMetaHandle,
    CHIMETAHANDLE hSrcMetaHandle,
    BOOL          disjoint)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT(NULL != hDstMetaHandle);
    CAMX_ASSERT(NULL != hSrcMetaHandle);

    MetaBuffer* pSrcMetaBuffer = static_cast<MetaBuffer*>(hSrcMetaHandle);
    MetaBuffer* pDstMetaBuffer = static_cast<MetaBuffer*>(hDstMetaHandle);
    if ((NULL != pSrcMetaBuffer)                      &&
        (NULL != pDstMetaBuffer)                      &&
        (TRUE == MetaBuffer::IsValid(pSrcMetaBuffer)) &&
        (TRUE == MetaBuffer::IsValid(pDstMetaBuffer)))
    {
        result = pDstMetaBuffer->Merge(pSrcMetaBuffer, disjoint);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferMergeMultiCameraMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferMergeMultiCameraMetadata(
    CHIMETAHANDLE  hDstMetaHandle,
    UINT32         srcMetaHandleCount,
    CHIMETAHANDLE* phSrcMetaHandles,
    UINT32*        pCameraIdArray,
    UINT32         primaryCameraId)
{
    CDKResult result = CDKResultSuccess;
    CAMX_ASSERT(NULL != hDstMetaHandle);
    CAMX_ASSERT(NULL != phSrcMetaHandles);
    CAMX_ASSERT(NULL != pCameraIdArray);
    CAMX_ASSERT(0    <  srcMetaHandleCount);

    MetaBuffer* pDstMetaBuffer = static_cast<MetaBuffer*>(hDstMetaHandle);

    if ((NULL != pCameraIdArray)   &&
        (NULL != phSrcMetaHandles) &&
        (0 < srcMetaHandleCount)   &&
        (primaryCameraId < MaxNumCameras))
    {
        // validate source handles
        for (UINT32 index = 0; index < srcMetaHandleCount; ++index)
        {
            MetaBuffer* pSrcMetaBuffer = static_cast<MetaBuffer*>(phSrcMetaHandles[index]);

            if ((NULL != pSrcMetaBuffer) && (FALSE == MetaBuffer::IsValid(pSrcMetaBuffer)))
            {
                result = CDKResultEInvalidPointer;
                CAMX_LOG_ERROR(CamxLogGroupMeta, "Source metadata for index %d invalid %p", index, pSrcMetaBuffer);
                break;
            }
            if (MaxNumCameras <= pCameraIdArray[index])
            {
                CAMX_LOG_ERROR(CamxLogGroupMeta, "CameraId for index %d invalid %u", index, pCameraIdArray[index]);
                result = CDKResultEInvalidArg;
                break;
            }
        }
    }
    else
    {
        result = CDKResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupMeta, "Invalid args srcMetaArray %p cameraIdArray %p srcHandleCount %d"
            "primaryCameraId %u max cameras %u",
            phSrcMetaHandles, pCameraIdArray, srcMetaHandleCount, primaryCameraId, MaxNumCameras);
    }

    if ((CDKResultSuccess == result) &&
        (NULL != pDstMetaBuffer)     &&
        (TRUE == MetaBuffer::IsValid(pDstMetaBuffer)))
    {
        result = pDstMetaBuffer->CombineMultiCameraMetadata(srcMetaHandleCount,
            pCameraIdArray,
            reinterpret_cast<MetaBuffer**>(phSrcMetaHandles),
            primaryCameraId);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta, "Destination buffer invalid %p result %d count %d primCamId %u maxNumcamera %u",
            pDstMetaBuffer, result, srcMetaHandleCount, primaryCameraId, MaxNumCameras);

        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferCopy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferCopy(
    CHIMETAHANDLE hDstMetaHandle,
    CHIMETAHANDLE hSrcMetaHandle,
    BOOL          disjoint)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT(NULL != hDstMetaHandle);
    CAMX_ASSERT(NULL != hSrcMetaHandle);

    MetaBuffer* pSrcMetaBuffer = static_cast<MetaBuffer*>(hSrcMetaHandle);
    MetaBuffer* pDstMetaBuffer = static_cast<MetaBuffer*>(hDstMetaHandle);
    if ((NULL != pSrcMetaBuffer) &&
        (NULL != pDstMetaBuffer) &&
        (TRUE == MetaBuffer::IsValid(pSrcMetaBuffer)) &&
        (TRUE == MetaBuffer::IsValid(pDstMetaBuffer)))
    {
        result = pDstMetaBuffer->Copy(pSrcMetaBuffer, disjoint);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferClone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferClone(
    CHIMETAHANDLE  hSrcMetaHandle,
    CHIMETAHANDLE* phDstMetaHandle)
{
    CDKResult result = CDKResultEInvalidArg;

    CAMX_ASSERT(NULL != hSrcMetaHandle);
    CAMX_ASSERT(NULL != phDstMetaHandle);

    MetaBuffer* pSrcMetaBuffer = static_cast<MetaBuffer*>(hSrcMetaHandle);

    if ((NULL != pSrcMetaBuffer) && (NULL != phDstMetaHandle) && (TRUE == MetaBuffer::IsValid(pSrcMetaBuffer)))
    {
        MetaBuffer* pDstMetaBuffer = pSrcMetaBuffer->Clone();
        if (NULL != pDstMetaBuffer)
        {
            *phDstMetaHandle = static_cast<CHIMETAHANDLE>(pDstMetaBuffer);
            result           = CDKResultSuccess;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferGetCapacity
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferGetCapacity(
    CHIMETAHANDLE hMetaHandle,
    UINT32*       pCapacity)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT((NULL != hMetaHandle) && (NULL != pCapacity));

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);
    if ((NULL != pMetaBuffer) && (NULL != pCapacity) && (TRUE == MetaBuffer::IsValid(pMetaBuffer)))
    {
        *pCapacity = pMetaBuffer->Capacity();
        result     = CDKResultSuccess;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferGetCount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferGetCount(
    CHIMETAHANDLE hMetaHandle,
    UINT32*       pCapacity)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT((NULL != hMetaHandle) && (NULL != pCapacity));

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);
    if ((NULL != pMetaBuffer) && (NULL != pCapacity) && (TRUE == MetaBuffer::IsValid(pMetaBuffer)))
    {
        *pCapacity = pMetaBuffer->Count();
        result     = CDKResultSuccess;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferPrint
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferPrint(
    CHIMETAHANDLE hMetaHandle)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT(NULL != hMetaHandle);

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);
    if (NULL != pMetaBuffer)
    {
        pMetaBuffer->PrintDetails();
        result = CDKResultSuccess;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferDump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferDump(
    CHIMETAHANDLE hMetaHandle,
    const CHAR*   pFilename)
{
    CDKResult result = CDKResultEInvalidArg;

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);
    if ((NULL != pMetaBuffer) && (NULL != pFilename) && (TRUE == MetaBuffer::IsValid(pMetaBuffer)))
    {
        pMetaBuffer->DumpDetailsToFile(pFilename);
        result = CDKResultSuccess;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferBinaryDump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferBinaryDump(
    CHIMETAHANDLE hMetaHandle,
    const CHAR*   pFilename)
{
    CDKResult result = CDKResultEInvalidArg;

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);
    if ((NULL != pMetaBuffer) && (NULL != pFilename) && (TRUE == MetaBuffer::IsValid(pMetaBuffer)))
    {
        pMetaBuffer->BinaryDump(pFilename);
        result = CDKResultSuccess;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferAddReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferAddReference(
    CHIMETAHANDLE       hMetaHandle,
    CHIMETADATACLIENTID clientID,
    UINT32*             pRefCount)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT((NULL != hMetaHandle) && (NULL != pRefCount));

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);
    if ((NULL != pMetaBuffer) && (NULL != pRefCount) && (TRUE == MetaBuffer::IsValid(pMetaBuffer)))
    {
        UINT32 metaBufferClientID = (clientID.clientIndex << CMBClientFrameNumberBitCount) | clientID.frameNumber;

        *pRefCount = pMetaBuffer->AddReference(metaBufferClientID);
        result     = CDKResultSuccess;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferReleaseReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferReleaseReference(
    CHIMETAHANDLE       hMetaHandle,
    CHIMETADATACLIENTID clientID,
    UINT32*             pRefCount)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT((NULL != hMetaHandle) && (NULL != pRefCount));

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);
    if ((NULL != pMetaBuffer) && (NULL != pRefCount) && (TRUE == MetaBuffer::IsValid(pMetaBuffer)))
    {
        UINT32 metaBufferClientID = clientID.clientIndex << CMBClientFrameNumberBitCount | clientID.frameNumber;

        *pRefCount = pMetaBuffer->ReleaseReference(metaBufferClientID);
        result     = CDKResultSuccess;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferReleaseAllReferences
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferReleaseAllReferences(
    CHIMETAHANDLE hMetaHandle,
    BOOL          bCHIAndCAMXReferences)
{
    CDKResult result = CDKResultEInvalidArg;

    CAMX_ASSERT(NULL != hMetaHandle);

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);

    if ((NULL != pMetaBuffer) && (TRUE == MetaBuffer::IsValid(pMetaBuffer)))
    {
        pMetaBuffer->ReleaseAllReferences(bCHIAndCAMXReferences);

        result = CDKResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferReferenceCount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferReferenceCount(
    CHIMETAHANDLE hMetaHandle,
    UINT32*       pRefCount)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT((NULL != hMetaHandle) && (NULL != pRefCount));

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);
    if ((NULL != pMetaBuffer) && (NULL != pRefCount) && (TRUE == MetaBuffer::IsValid(pMetaBuffer)))
    {
        *pRefCount = pMetaBuffer->ReferenceCount();
        result     = CDKResultSuccess;
    }
    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiMetabufferIteratorCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferIteratorCreate(
    CHIMETAHANDLE           hMetaHandle,
    CHIMETADATAITERATOR*    pIterator)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT((NULL != hMetaHandle) && (NULL != pIterator));

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);
    if ((NULL != pMetaBuffer) && (NULL != pIterator) && (TRUE == MetaBuffer::IsValid(pMetaBuffer)))
    {
        MetaBuffer::Iterator* pMetaBufferIterator = pMetaBuffer->CreateIterator();
        if (NULL != pIterator)
        {
            *pIterator = pMetaBufferIterator;
            result     = CDKResultSuccess;
        }
        else
        {
            result = CDKResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiMetabufferIteratorDestroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferIteratorDestroy(
    CHIMETADATAITERATOR hIterator)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT(NULL != hIterator);

    MetaBuffer::Iterator* pIterator = static_cast<MetaBuffer::Iterator*>(hIterator);
    if (NULL != pIterator)
    {
        result = CamxResultSuccess;
        CAMX_DELETE pIterator;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiMetabufferIteratorBegin
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferIteratorBegin(
    CHIMETADATAITERATOR hIterator)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT(NULL != hIterator);

    MetaBuffer::Iterator* pIterator = static_cast<MetaBuffer::Iterator*>(hIterator);
    if (NULL != pIterator)
    {
        result = pIterator->Begin();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiMetabufferIteratorNext
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferIteratorNext(
    CHIMETADATAITERATOR hIterator)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT(NULL != hIterator);

    MetaBuffer::Iterator* pIterator = static_cast<MetaBuffer::Iterator*>(hIterator);
    if (NULL != pIterator)
    {
        result = pIterator->Next();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiMetabufferIteratorGetEntry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferIteratorGetEntry(
    CHIMETADATAITERATOR hIterator,
    CHIMETADATAENTRY*   pEntry)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT((NULL != hIterator) && (NULL != pEntry));

    MetaBuffer::Iterator* pIterator = static_cast<MetaBuffer::Iterator*>(hIterator);
    if ((NULL != pIterator) && (NULL != pEntry))
    {
        MetaBuffer::Entry metadataSrcEntry;
        result = pIterator->GetEntry(metadataSrcEntry);

        if (CamxResultSuccess == result)
        {
            ChiMetabufferEntryCopy(pEntry, &metadataSrcEntry);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferGetTable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferGetTable(
    CHIMETADATAENTRY* pMetadataEntryTable)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT(NULL != pMetadataEntryTable);

    if (NULL != pMetadataEntryTable)
    {
        UINT32 totalCount = HAL3MetadataUtil::GetAllMetadataTagCount();

        result = CDKResultSuccess;

        for (UINT32 tagIndex = 0; tagIndex < totalCount; ++tagIndex)
        {
            CHIMETADATAENTRY* pDstEntry = &pMetadataEntryTable[tagIndex];

            const MetadataInfo* pInfo = HAL3MetadataUtil::GetMetadataInfoByIndex(tagIndex);

            if (NULL != pInfo)
            {
                pDstEntry->count    = pInfo->count;
                pDstEntry->size     = pInfo->size;
                pDstEntry->pTagData = NULL;
                pDstEntry->tagID    = pInfo->tag;
                pDstEntry->type     = static_cast<ChiMetadataTagType>(pInfo->type);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogMeta, "Cannot find tag for index %d, total count %d",
                    tagIndex, totalCount);
                break;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetabufferGetEntryCount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiMetabufferGetEntryCount(
    UINT32* pMetadataCount)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT(NULL != pMetadataCount);

    if (NULL != pMetadataCount)
    {
        *pMetadataCount = HAL3MetadataUtil::GetTotalTagCount();
        result          = CDKResultSuccess;
        CAMX_ASSERT(0 < *pMetadataCount);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiGetDefaultSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiGetDefaultSettings(
    UINT32  cameraId,
    const VOID**  ppAndroidMetadata)
{
    CDKResult result = CDKResultEInvalidArg;
    if (NULL != ppAndroidMetadata)
    {
        const Metadata* pAndroidMeta = HAL3DefaultRequest::ConstructDefaultRequestSettings(cameraId, RequestTemplatePreview);
        if (NULL != pAndroidMeta)
        {
            *ppAndroidMetadata  = pAndroidMeta;
            result              = CDKResultSuccess;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiGetDefaultMetaBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiGetDefaultMetaBuffer(
    UINT32         cameraId,
    CHIMETAHANDLE* phMetaBuffer)
{
    const Metadata* pAndroidMetadata = NULL;

    CDKResult result = ChiGetDefaultSettings(cameraId, &pAndroidMetadata);

    if (CamxResultSuccess == result)
    {
        result = ChiMetabufferCreateWithAndroidMeta(pAndroidMetadata, phMetaBuffer, NULL);

        if (CamxResultSuccess == result)
        {
            result = ChiMetabufferSetTagWithAndroidMeta(*phMetaBuffer, pAndroidMetadata);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetaBufferFilter
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetaBufferFilter(
    CHIMETAHANDLE hMetaHandle,
    VOID*         pAndroidMeta,
    BOOL          frameworkTagsOnly,
    BOOL          filterProperties,
    UINT32        filterTagCount,
    UINT32*       pFilterTagArray)
{
    CDKResult result = CDKResultEInvalidArg;
    CAMX_ASSERT((NULL != hMetaHandle) && (NULL != pAndroidMeta));
    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);
    if ((NULL != pMetaBuffer) && (NULL != pAndroidMeta) && (TRUE == MetaBuffer::IsValid(pMetaBuffer)))
    {
        result = pMetaBuffer->GetAndroidMeta(static_cast<camera_metadata_t *>(pAndroidMeta),
                                             frameworkTagsOnly,
                                             filterProperties,
                                             filterTagCount,
                                             pFilterTagArray);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetaBufferGetPrivateData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetaBufferGetPrivateData(
    CHIMETAHANDLE       hMetaHandle,
    CHIMETAPRIVATEDATA* pPrivateData)
{
    CDKResult result = CDKResultEInvalidArg;

    MetaBuffer* pMetaBuffer = static_cast<MetaBuffer*>(hMetaHandle);

    if ((NULL != pMetaBuffer) && (NULL != pPrivateData) && (TRUE == MetaBuffer::IsValid(pMetaBuffer)))
    {
        *pPrivateData  = pMetaBuffer->GetPrivateUserHandle();
        result         = CDKResultSuccess;
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupChi, "Unable to get private data metaBuffer %p privateData %p",
            pMetaBuffer, pPrivateData);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiGetMetadataOps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID ChiGetMetadataOps(
    CHIMETADATAOPS* pMetadataOps)
{
    if (NULL != pMetadataOps)
    {
        pMetadataOps->pCreate                     = ChiMetabufferCreate;
        pMetadataOps->pCreateWithTagArray         = ChiMetabufferCreateWithTaglist;
        pMetadataOps->pCreateWithAndroidMetadata  = ChiMetabufferCreateWithAndroidMeta;
        pMetadataOps->pDestroy                    = ChiMetabufferDestroy;
        pMetadataOps->pGetTag                     = ChiMetabufferGetTag;
        pMetadataOps->pGetTagEntry                = ChiMetabufferGetTagEntry;
        pMetadataOps->pSetTag                     = ChiMetabufferSetTag;
        pMetadataOps->pGetVendorTag               = ChiMetabufferGetVendorTag;
        pMetadataOps->pGetVendorTagEntry          = ChiMetabufferGetVendorTagEntry;
        pMetadataOps->pSetVendorTag               = ChiMetabufferSetVendorTag;
        pMetadataOps->pSetAndroidMetadata         = ChiMetabufferSetTagWithAndroidMeta;
        pMetadataOps->pDeleteTag                  = ChiMetabufferDeleteTag;
        pMetadataOps->pInvalidate                 = ChiMetabufferInvalidate;
        pMetadataOps->pMerge                      = ChiMetabufferMerge;
        pMetadataOps->pCopy                       = ChiMetabufferCopy;
        pMetadataOps->pClone                      = ChiMetabufferClone;
        pMetadataOps->pCapacity                   = ChiMetabufferGetCapacity;
        pMetadataOps->pCount                      = ChiMetabufferGetCount;
        pMetadataOps->pPrint                      = ChiMetabufferPrint;
        pMetadataOps->pDump                       = ChiMetabufferDump;
        pMetadataOps->pBinaryDump                 = ChiMetabufferBinaryDump;
        pMetadataOps->pAddReference               = ChiMetabufferAddReference;
        pMetadataOps->pReleaseReference           = ChiMetabufferReleaseReference;
        pMetadataOps->pReleaseAllReferences       = ChiMetabufferReleaseAllReferences;
        pMetadataOps->pReferenceCount             = ChiMetabufferReferenceCount;
        pMetadataOps->pIteratorCreate             = ChiMetabufferIteratorCreate;
        pMetadataOps->pIteratorDestroy            = ChiMetabufferIteratorDestroy;
        pMetadataOps->pIteratorBegin              = ChiMetabufferIteratorBegin;
        pMetadataOps->pIteratorNext               = ChiMetabufferIteratorNext;
        pMetadataOps->pIteratorGetEntry           = ChiMetabufferIteratorGetEntry;
        pMetadataOps->pGetMetadataEntryCount      = ChiMetabufferGetEntryCount;
        pMetadataOps->pGetMetadataTable           = ChiMetabufferGetTable;
        pMetadataOps->pGetDefaultAndroidMeta      = ChiGetDefaultSettings;
        pMetadataOps->pGetDefaultMetadata         = ChiGetDefaultMetaBuffer;
        pMetadataOps->pFilter                     = ChiMetaBufferFilter;
        pMetadataOps->pGetPrivateData             = ChiMetaBufferGetPrivateData;
        pMetadataOps->pMergeMultiCameraMeta       = ChiMetabufferMergeMultiCameraMetadata;
        pMetadataOps->pGetTagByCameraId           = ChiMetabufferGetTagByCameraId;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiBufferManagerCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIBUFFERMANAGERHANDLE ChiBufferManagerCreate(
    const CHAR*                 pBufferManagerName,
    CHIBufferManagerCreateData* pCreateData)
{
    CDKResult               result          = CDKResultSuccess;
    ImageBufferManager*     pBufferManager  = NULL;
    const StaticSettings*   pSettings       = g_pChiContext->GetStaticSettings();
    UINT32                  bufferHeap;

    if ((NULL != pBufferManagerName) && (NULL != pCreateData) && (NULL != pCreateData->pChiStream))
    {
        GrallocProperties       grallocProperties;
        BufferManagerCreateData createData      = {};
        FormatParamInfo         formatParamInfo = {};
        ImageFormat*            pImageFormat    = &createData.bufferProperties.imageFormat;
        const ChiStream*        pChiStream      = pCreateData->pChiStream;
        GrallocUsage64          grallocUsage    = ChiContext::GetGrallocUsage(pChiStream)   |
                                                  pCreateData->consumerFlags                |
                                                  pCreateData->producerFlags;

        switch (pChiStream->dataspace)
        {
            case DataspaceUnknown:
                pImageFormat->colorSpace = ColorSpace::Unknown;
                break;
            case DataspaceJFIF:
            case DataspaceV0JFIF:
                pImageFormat->colorSpace = ColorSpace::BT601Full;
                break;
            case DataspaceBT601_625:
                pImageFormat->colorSpace = ColorSpace::BT601625;
                break;
            case DataspaceBT601_525:
                pImageFormat->colorSpace = ColorSpace::BT601525;
                break;
            case DataspaceBT709:
                pImageFormat->colorSpace = ColorSpace::BT709;
                break;
            case DataspaceDepth:
                pImageFormat->colorSpace = ColorSpace::Depth;
                break;
            default:
                /// @todo (CAMX-346) Add more colorspace here.
                pImageFormat->colorSpace = ColorSpace::Unknown;
                break;
        }

        grallocProperties.colorSpace         = static_cast<ColorSpace>(pImageFormat->colorSpace);
        grallocProperties.pixelFormat        = pChiStream->format;
        grallocProperties.grallocUsage       = ChiContext::GetGrallocUsage(pChiStream);
        grallocProperties.isInternalBuffer   = TRUE;
        grallocProperties.isRawFormat        = FALSE;
        grallocProperties.staticFormat       = g_pChiContext->GetStaticSettings()->outputFormat;
        grallocProperties.isMultiLayerFormat = FALSE;
        result = ImageFormatUtils::GetFormat(grallocProperties, pImageFormat->format);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore,
                "GetFormat failed, colorspace %d pixelFormat %d, outputFormat %d, usage %llu",
                grallocProperties.colorSpace, grallocProperties.pixelFormat,
                grallocProperties.staticFormat, grallocProperties.grallocUsage);
        }

        pImageFormat->width           = pCreateData->width;
        pImageFormat->height          = pCreateData->height;
        pImageFormat->alignment       = 1;

        switch (pChiStream->rotation)
        {
            case ChiStreamRotation::StreamRotationCCW0:
                pImageFormat->rotation = Rotation::CW0Degrees;
                break;
            case ChiStreamRotation::StreamRotationCCW90:
                pImageFormat->rotation = Rotation::CW90Degrees;
                break;
            case ChiStreamRotation::StreamRotationCCW180:
                pImageFormat->rotation = Rotation::CW180Degrees;
                break;
            case ChiStreamRotation::StreamRotationCCW270:
                pImageFormat->rotation = Rotation::CW270Degrees;
                break;
            default:
                pImageFormat->rotation = Rotation::CW0Degrees;
                break;
        }

        formatParamInfo.grallocUsage  = grallocUsage;
        formatParamInfo.isHALBuffer   = TRUE;
        formatParamInfo.pixelFormat   = pChiStream->format;
        formatParamInfo.outputFormat  = g_pChiContext->GetStaticSettings()->outputFormat;
        formatParamInfo.isImplDefined = ((pChiStream->format == ChiStreamFormatImplDefined) ||
                                         (pChiStream->format == ChiStreamFormatYCrCb420_SP)) ? TRUE : FALSE;
        formatParamInfo.yuvPlaneAlign = g_pChiContext->GetStaticSettings()->yuvPlaneAlignment;

        if (pImageFormat->format == Format::Jpeg)
        {
            formatParamInfo.debugDataSize = HAL3MetadataUtil::DebugDataSize(DebugDataType::AllTypes);
        }

        if (TRUE == g_pChiContext->GetStaticSettings()->IsStrideSettingEnable)
        {
            formatParamInfo.planeStride = pCreateData->width;
            formatParamInfo.sliceHeight = pCreateData->height;
        }
        else
        {
            formatParamInfo.planeStride = 0;
            formatParamInfo.sliceHeight = 0;
        }

        ImageFormatUtils::InitializeFormatParams(pImageFormat, &formatParamInfo);

        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "Formatparams : "
                         "YUV[0]=(w=%d, h=%d, stride=%d, sliceH=%d, size=%d), "
                         "YUV[1]=(w=%d, h=%d, stride=%d, sliceH=%d, size=%d), "
                         "YUV[2]=(w=%d, h=%d, stride=%d, sliceH=%d, size=%d), "
                         "rawFormat=(stride=%d, sliceH=%d, bitsPerPixel=%d, pattern=%d), "
                         "jpegFormat=(maxJPEGSizeInBytes=%d), ",
                         pImageFormat->formatParams.yuvFormat[0].width,
                         pImageFormat->formatParams.yuvFormat[0].height,
                         pImageFormat->formatParams.yuvFormat[0].planeStride,
                         pImageFormat->formatParams.yuvFormat[0].sliceHeight,
                         pImageFormat->formatParams.yuvFormat[0].planeSize,
                         pImageFormat->formatParams.yuvFormat[1].width,
                         pImageFormat->formatParams.yuvFormat[1].height,
                         pImageFormat->formatParams.yuvFormat[1].planeStride,
                         pImageFormat->formatParams.yuvFormat[1].sliceHeight,
                         pImageFormat->formatParams.yuvFormat[1].planeSize,
                         pImageFormat->formatParams.yuvFormat[2].width,
                         pImageFormat->formatParams.yuvFormat[2].height,
                         pImageFormat->formatParams.yuvFormat[2].planeStride,
                         pImageFormat->formatParams.yuvFormat[2].sliceHeight,
                         pImageFormat->formatParams.yuvFormat[2].planeSize,
                         pImageFormat->formatParams.rawFormat.stride,
                         pImageFormat->formatParams.rawFormat.sliceHeight,
                         pImageFormat->formatParams.rawFormat.bitsPerPixel,
                         pImageFormat->formatParams.rawFormat.colorFilterPattern,
                         pImageFormat->formatParams.jpegFormat.maxJPEGSizeInBytes);

        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "AlignmentInfo : Plane[0]=(stride=%d, scanline=%d), "
                         "Plane[1]=(stride=%d, scanline=%d), Plane[2]=(stride=%d, scanline=%d)",
                         pImageFormat->planeAlignment[0].strideAlignment, pImageFormat->planeAlignment[0].scanlineAlignment,
                         pImageFormat->planeAlignment[1].strideAlignment, pImageFormat->planeAlignment[1].scanlineAlignment,
                         pImageFormat->planeAlignment[2].strideAlignment, pImageFormat->planeAlignment[2].scanlineAlignment);

        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "ImageFormat : w=%d, h=%d, format=%d, colorSpace=%d, rotation=%d, alignment=%zu",
                         pImageFormat->width, pImageFormat->height, pImageFormat->format, pImageFormat->colorSpace,
                         pImageFormat->rotation, pImageFormat->alignment);

        // Populate CSL flags based on Gralloc usage flags
        if (0 != (grallocUsage & GrallocUsageProtected))
        {
            createData.bufferProperties.memFlags |= CSLMemFlagProtected;
        }

        if ((0 != (grallocUsage & GrallocUsageSwReadMask)) || (0 != (grallocUsage & GrallocUsageSwWriteMask)))
        {
            createData.bufferProperties.memFlags |= CSLMemFlagUMDAccess;
            createData.bufferProperties.memFlags |= CSLMemFlagCache;
        }

        // Initialize bufferHeap field
        if (BufferHeapDefault == pCreateData->bufferHeap)
        {
            // If this field is set to default, lets use the override setting.
            bufferHeap = pSettings->forceCHIBufferManagerHeap;
        }
        else
        {
            // Otherwise, use the value explictly set by the caller.
            bufferHeap = pCreateData->bufferHeap;
        }

        switch (bufferHeap)
        {
            case BufferHeapSystem:
                createData.bufferProperties.bufferHeap = CSLBufferHeapSystem;
                break;
            case BufferHeapIon:
                createData.bufferProperties.bufferHeap = CSLBufferHeapIon;
                break;
            case BufferHeapDSP:
                createData.bufferProperties.bufferHeap = CSLBufferHeapDSP;
                break;
            case BufferHeapEGL:
                createData.bufferProperties.bufferHeap = CSLBufferHeapEGL;
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Buffer heap %d not supported!", pCreateData->bufferHeap);
                result = CDKResultEUnsupported;
                break;
        }

        // Initialize remaining buffer manager parameters
        if (CDKResultSuccess == result)
        {
            createData.bufferProperties.grallocFormat               = pCreateData->format;
            createData.bufferProperties.producerFlags               = pCreateData->producerFlags;
            createData.bufferProperties.consumerFlags               = pCreateData->consumerFlags;
            createData.bufferProperties.immediateAllocImageBuffers  = pCreateData->immediateBufferCount;
            createData.bufferProperties.maxImageBuffers             = pCreateData->maxBufferCount;

            createData.allocateBufferMemory         = TRUE;
            createData.deviceCount                  = 0;
            createData.immediateAllocBufferCount    = pCreateData->immediateBufferCount;
            createData.maxBufferCount               = pCreateData->maxBufferCount;
            createData.numBatchedFrames             = 1;
            createData.bufferManagerType            = BufferManagerType::ChiBufferManager;
            createData.bEnableLateBinding           = ((TRUE == pCreateData->bEnableLateBinding) &&
                                                       (TRUE == pSettings->enableImageBufferLateBinding)) ? TRUE : FALSE;
            createData.bDisableSelfShrinking        = FALSE;
            createData.linkProperties.pNode         = NULL;
            createData.bNeedDedicatedBuffers        = (TRUE == createData.bDisableSelfShrinking) ? TRUE : FALSE;

            // Workaround to handle hollow Buffer managers coming from CHI - for which maxBufferCount will be 0
            // If maxBufferCount is 0 - move them to their own dedicated buffers to avoid unnecessary grouping.
            // Unnecessary grouping can cause some buffers to be freed only after idle timeout
            if (0 == createData.maxBufferCount)
            {
                createData.bNeedDedicatedBuffers = TRUE;
            }

            result = ImageBufferManager::Create(pBufferManagerName, &createData, &pBufferManager);
            if (CDKResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                               "[%s] Create BufferManager failed : heap=%d, Gralloc format=0x%x, Flags=0x%llx, 0x%llx, "
                               "Format=%d, width=%d, height=%d",
                               pBufferManagerName,
                               createData.bufferProperties.bufferHeap,
                               createData.bufferProperties.grallocFormat,
                               createData.bufferProperties.producerFlags,
                               createData.bufferProperties.consumerFlags,
                               createData.bufferProperties.imageFormat.format,
                               createData.bufferProperties.imageFormat.width,
                               createData.bufferProperties.imageFormat.height);
            }
        }

        // Fill stride value in createData
        if (CDKResultSuccess == result)
        {
            if (TRUE == ImageFormatUtils::IsYUV(pImageFormat))
            {
                pCreateData->bufferStride = pImageFormat->formatParams.yuvFormat[0].planeStride;
            }
            else if (TRUE == ImageFormatUtils::IsRAW(pImageFormat))
            {
                pCreateData->bufferStride = pImageFormat->formatParams.rawFormat.stride;
            }
            else
            {
                pCreateData->bufferStride = pImageFormat->width;
            }

            CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s] : format=%d, width=%d, stride=%d",
                             pBufferManagerName, pImageFormat->format, pImageFormat->width, pCreateData->bufferStride);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid input! pBufferManagerName=%p pCreateData=%p",
                       pBufferManagerName, pCreateData);
    }

    return static_cast<CHIBUFFERMANAGERHANDLE>(pBufferManager);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiBufferManagerDestroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID ChiBufferManagerDestroy(
    CHIBUFFERMANAGERHANDLE hBufferManager)
{
    if (NULL != hBufferManager)
    {
        ImageBufferManager* pBufferManager = static_cast<ImageBufferManager*>(hBufferManager);
        pBufferManager->Destroy();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid input! hBufferManager is NULL");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiBufferManagerGetImageBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHIBUFFERHANDLE ChiBufferManagerGetImageBuffer(
    CHIBUFFERMANAGERHANDLE hBufferManager)
{
    CHIBUFFERHANDLE hImageBuffer = NULL;

    if (NULL != hBufferManager)
    {
        ImageBufferManager* pBufferManager = static_cast<ImageBufferManager*>(hBufferManager);

        hImageBuffer = reinterpret_cast<CHIBUFFERHANDLE>(pBufferManager->GetImageBuffer());
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid input! hBufferManager is NULL");
    }

    return static_cast<CHIBUFFERHANDLE>(hImageBuffer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiBufferManagerAddReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiBufferManagerAddReference(
    CHIBUFFERMANAGERHANDLE hBufferManager,
    CHIBUFFERHANDLE        hImageBuffer)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL != hBufferManager) && (NULL != hImageBuffer))
    {
        ImageBufferManager* pBufferManager  = static_cast<ImageBufferManager*>(hBufferManager);
        ImageBuffer*        pImageBuffer    = static_cast<ImageBuffer*>(hImageBuffer);

        pBufferManager->AddReference(pImageBuffer);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid input! hBufferManager=%p hImageBuffer=%p", hBufferManager, hImageBuffer);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiBufferManagerReleaseReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiBufferManagerReleaseReference(
    CHIBUFFERMANAGERHANDLE hBufferManager,
    CHIBUFFERHANDLE        hImageBuffer)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL != hBufferManager) && (NULL != hImageBuffer))
    {
        ImageBufferManager* pBufferManager  = static_cast<ImageBufferManager*>(hBufferManager);
        ImageBuffer*        pImageBuffer    = static_cast<ImageBuffer*>(hImageBuffer);

        pBufferManager->ReleaseReference(pImageBuffer);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid input! hBufferManager=%p hImageBuffer=%p", hBufferManager, hImageBuffer);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiBufferManagerGetReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT ChiBufferManagerGetReference(
    CHIBUFFERHANDLE hImageBuffer)
{
    UINT refCount = 0xFFFFFFFF;

    if (NULL != hImageBuffer)
    {
        ImageBuffer* pImageBuffer = static_cast<ImageBuffer*>(hImageBuffer);
        refCount = pImageBuffer->GetReferenceCount();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid input! hImageBuffer is NULL!");
    }

    return refCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiBufferManagerActivate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiBufferManagerActivate(
    CHIBUFFERMANAGERHANDLE hBufferManager)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != hBufferManager)
    {
        ImageBufferManager* pBufferManager = static_cast<ImageBufferManager*>(hBufferManager);
        result = pBufferManager->Activate();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid input! hBufferManager is NULL");
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiBufferManagerDeactivate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiBufferManagerDeactivate(
    CHIBUFFERMANAGERHANDLE hBufferManager,
    BOOL                   isPartialFree)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != hBufferManager)
    {
        ImageBufferManager* pBufferManager = static_cast<ImageBufferManager*>(hBufferManager);
        result = pBufferManager->Deactivate(isPartialFree);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid input! hBufferManager is NULL");
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiBufferManagerBindBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiBufferManagerBindBuffer(
    CHIBUFFERHANDLE hImageBuffer)
{
    CDKResult result = CDKResultSuccess;

    if (NULL != hImageBuffer)
    {
        ImageBuffer* pImageBuffer = static_cast<ImageBuffer*>(hImageBuffer);
        result = pImageBuffer->BindBuffer();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "hImageBuffer is NULL!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiBufferManagerGetCPUAddress
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const VOID* ChiBufferManagerGetCPUAddress(
    CHIBUFFERHANDLE hImageBuffer)
{
    const VOID*  pAddr = NULL;

    if (NULL != hImageBuffer)
    {
        ImageBuffer* pImageBuffer = static_cast<ImageBuffer*>(hImageBuffer);

        pAddr = pImageBuffer->GetCPUAddress();
    }

    if (NULL == pAddr)
    {
        CAMX_LOG_WARN(CamxLogGroupMemMgr, "Unable to get CPU address, hImageBuffer=%p", hImageBuffer);
    }

    return pAddr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiBufferManagerGetFileDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static INT ChiBufferManagerGetFileDescriptor(
    CHIBUFFERHANDLE hImageBuffer)
{
    INT fd = -1;

    if (NULL != hImageBuffer)
    {
        ImageBuffer* pImageBuffer = static_cast<ImageBuffer*>(hImageBuffer);

        fd = pImageBuffer->GetFileDescriptor();
    }

    if (-1 == fd)
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Unable to get File Descriptor, hImageBuffer=%p", hImageBuffer);
    }

    return fd;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiBufferManagerGetGrallocHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID* ChiBufferManagerGetGrallocHandle(
    CHIBUFFERHANDLE hImageBuffer)
{
    VOID* phGrallocHandle = NULL;

    if (NULL != hImageBuffer)
    {
        ImageBuffer* pImageBuffer = static_cast<ImageBuffer*>(hImageBuffer);

        phGrallocHandle = static_cast<VOID*>(pImageBuffer->GetGrallocBufferHandle());
    }

    if (NULL == phGrallocHandle)
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Unable to get gralloc handle, hImageBuffer=%p", hImageBuffer);
    }

    return phGrallocHandle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiBufferManagerCacheOps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiBufferManagerCacheOps(
    CHIBUFFERHANDLE hImageBuffer,
    BOOL            invalidate,
    BOOL            clean)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL != hImageBuffer) &&
        ((TRUE == invalidate) || (TRUE == clean)))
    {
        ImageBuffer* pImageBuffer = static_cast<ImageBuffer*>(hImageBuffer);
        result = pImageBuffer->CacheOps(invalidate, clean);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "InvalidArgs! hImageBuffer=%p, invalidate=%d, clean=%d",
                       hImageBuffer, invalidate, clean);
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiGetBufferManagerOps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID ChiGetBufferManagerOps(
    CHIBUFFERMANAGEROPS* pBufferManagerOps)
{
    if (NULL != pBufferManagerOps)
    {
        pBufferManagerOps->pCreate              = ChiBufferManagerCreate;
        pBufferManagerOps->pDestroy             = ChiBufferManagerDestroy;
        pBufferManagerOps->pGetImageBuffer      = ChiBufferManagerGetImageBuffer;
        pBufferManagerOps->pAddReference        = ChiBufferManagerAddReference;
        pBufferManagerOps->pReleaseReference    = ChiBufferManagerReleaseReference;
        pBufferManagerOps->pGetReference        = ChiBufferManagerGetReference;
        pBufferManagerOps->pActivate            = ChiBufferManagerActivate;
        pBufferManagerOps->pDeactivate          = ChiBufferManagerDeactivate;
        pBufferManagerOps->pBindBuffer          = ChiBufferManagerBindBuffer;
        pBufferManagerOps->pGetCPUAddress       = ChiBufferManagerGetCPUAddress;
        pBufferManagerOps->pGetFileDescriptor   = ChiBufferManagerGetFileDescriptor;
        pBufferManagerOps->pGetGrallocHandle    = ChiBufferManagerGetGrallocHandle;
        pBufferManagerOps->pCacheOps            = ChiBufferManagerCacheOps;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid input! pBufferManagerOps is NULL");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGetSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiGetSettings(
    CHIEXTENDSETTINGS** ppExtendSettings,
    CHIMODIFYSETTINGS** ppModifySettings)
{
    CHIEXTENDSETTINGS* pExtendSettings = static_cast<CHIEXTENDSETTINGS*>(CAMX_CALLOC(sizeof(CHIEXTENDSETTINGS)));
    if (NULL != pExtendSettings)
    {
        pExtendSettings->numTokens = NumExtendSettings;
        pExtendSettings->pTokens   = static_cast<CHISETTINGTOKEN*>(CAMX_CALLOC(sizeof(CHISETTINGTOKEN)*NumExtendSettings));
        *ppExtendSettings          = pExtendSettings;
        if (NULL != pExtendSettings->pTokens)
        {
            FillSettingTokenList(pExtendSettings->pTokens);
        }
    }

    CHIMODIFYSETTINGS* pModifySettings =
        static_cast<CHIMODIFYSETTINGS*>(CAMX_CALLOC(sizeof(CHIMODIFYSETTINGS)*NumExtendSettings));
    *ppModifySettings = pModifySettings;
    if (NULL != pModifySettings)
    {
        PopulateSettingsData(pModifySettings);
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiOpenContext
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHIHANDLE ChiOpenContext()
{
    NCSService*      pNCSServiceObj   = NULL;
    CamxResult       result           = CamxResultSuccess;
    BOOL             isNCSEnabled     = FALSE;

    if (NULL == g_pChiContext)
    {
        g_pChiContext = ChiContext::Create();
    }

    if (NULL != g_pChiContext)
    {
        isNCSEnabled = g_pChiContext->GetStaticSettings()->enableNCSService;
    }

    if (TRUE == isNCSEnabled)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCore, "NCS Setting Enabled");

        if ((NULL == HwEnvironment::GetInstance()->GetNCSObject()))
        {
            pNCSServiceObj = CAMX_NEW NCSService();
            if (NULL != pNCSServiceObj)
            {
                NCSInitializeInfo info;

                info.pThreadManager  = g_pChiContext->GetThreadManager();
                info.pCallbackData   = g_pChiContext;
                info.attachChiFence  = ChiContext::AttachChiFenceCallback;
                info.releaseChiFence = ChiContext::ReleaseChiFenceCallback;
                info.signalChiFence  = ChiContext::SignalChiFenceCallback;

                result = pNCSServiceObj->Initialize(&info);
                if (CamxResultSuccess == result)
                {
                    HwEnvironment::GetInstance()->SetNCSObject(reinterpret_cast<VOID*>(pNCSServiceObj));
                }
                else
                {
                    CAMX_LOG_WARN(CamxLogGroupCore, "Unable to initialize NCS Service object!!");
                    CAMX_DELETE pNCSServiceObj;
                    pNCSServiceObj = NULL;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Unable to create NCS Service object, no mem!!");
            }
        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCore, "Not creating NCS service handle, NCS disabled");
    }


    return static_cast<CHIHANDLE>(g_pChiContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiCloseContext
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID ChiCloseContext(
    CHIHANDLE hChiContext)
{
    NCSService* pNCSServiceObj = NULL;
    BOOL        isNCSEnabled   = FALSE;
    ChiContext* pChiContext    = GetChiContext(hChiContext);

    CAMX_ASSERT(g_pChiContext == pChiContext);

    if (NULL != pChiContext)
    {
        isNCSEnabled = pChiContext->GetStaticSettings()->enableNCSService;

        if (TRUE == isNCSEnabled)
        {
            pNCSServiceObj = reinterpret_cast<NCSService*>(HwEnvironment::GetInstance()->GetNCSObject());
            if (NULL != pNCSServiceObj)
            {
                CAMX_DELETE pNCSServiceObj;
                pNCSServiceObj = NULL;
                HwEnvironment::GetInstance()->SetNCSObject(NULL);
            }
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCore, "NCS Setting Disabled");
        }

        pChiContext->Destroy();
        pChiContext = NULL;
    }


    g_pChiContext = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiGetNumCameras
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT32 ChiGetNumCameras(
    CHIHANDLE hChiContext)
{
    ChiContext* pChiContext = GetChiContext(hChiContext);
    UINT32      numCameras  = 0;

    numCameras = pChiContext->GetNumCameras();

    return numCameras;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiGetCameraInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiGetCameraInfo(
    CHIHANDLE      hChiContext,
    UINT32         cameraId,
    CHICAMERAINFO* pChiCameraInfo)
{
    CDKResult   result      = CDKResultSuccess;
    ChiContext* pChiContext = GetChiContext(hChiContext);

    /// @todo (CAMX-2491) CDKResult to CamxResult
    result = pChiContext->GetCameraInfo(cameraId, pChiCameraInfo);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiEnumerateSensorModes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiEnumerateSensorModes(
    CHIHANDLE          hChiContext,
    UINT32             cameraId,
    UINT32             numSensorModes,
    CHISENSORMODEINFO* pSensorModeInfo)
{
    CamxResult  result      = CamxResultEFailed;
    ChiContext* pChiContext = NULL;

    CAMX_ASSERT(numSensorModes <= MaxSensorModes);

    if (numSensorModes > MaxSensorModes)
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Unsupported Sensor mode");
        return CamxResultENoSuch;
    }

    if (NULL != hChiContext)
    {
        pChiContext = GetChiContext(hChiContext);

        if (NULL != pChiContext)
        {
            result = pChiContext->EnumerateSensorModes(cameraId, numSensorModes, pSensorModeInfo);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "hChiContext is NULL");
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiCreatePipelineDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHIPIPELINEDESCRIPTOR ChiCreatePipelineDescriptor(
    CHIHANDLE                          hChiContext,
    const CHAR*                        pPipelineName,
    const CHIPIPELINECREATEDESCRIPTOR* pCreateDescriptor,
    UINT32                             numOutputs,
    CHIPORTBUFFERDESCRIPTOR*           pOutputBufferDescriptors,
    UINT32                             numInputs,
    CHIPIPELINEINPUTOPTIONS*           pInputBufferOptions)
{
    CDKResult result = CDKResultSuccess;

    CAMX_ASSERT(NULL != hChiContext);
    CAMX_ASSERT(NULL != pCreateDescriptor);
    CAMX_ASSERT(NULL != pOutputBufferDescriptors);
    CAMX_ASSERT(NULL != pInputBufferOptions);

    ChiNode* pChiNode = &pCreateDescriptor->pNodes[0];

    // Number of input can not be Zero for offline case.
    // Ignore this check for Torch widget node.
    /// @todo (CAMX-3119) remove Torch check below and handle this in generic way.
    if ((NULL != pCreateDescriptor && FALSE == pCreateDescriptor->isRealTime && 0 == numInputs) &&
        ((NULL != pChiNode) && (Torch != pChiNode->nodeId)))
    {
        result = CDKResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Number of Input cannot be zero for offline use cases!");
    }

    PipelineDescriptor* pPipelineDescriptor = NULL;

    if ((CDKResultSuccess == result)                    &&
        (NULL             != hChiContext)               &&
        (NULL             != pCreateDescriptor)         &&
        (NULL             != pPipelineName)             &&
        (NULL             != pOutputBufferDescriptors)  &&
        (NULL             != pInputBufferOptions))
    {
        ChiContext* pChiContext = GetChiContext(hChiContext);

        pPipelineDescriptor = pChiContext->CreatePipelineDescriptor(pPipelineName,
                                                                    pCreateDescriptor,
                                                                    numOutputs,
                                                                    pOutputBufferDescriptors,
                                                                    numInputs,
                                                                    pInputBufferOptions);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid input parameters!");
    }

    return reinterpret_cast<CHIPIPELINEDESCRIPTOR>(pPipelineDescriptor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiDestroyPipelineDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID ChiDestroyPipelineDescriptor(
    CHIHANDLE             hChiContext,
    CHIPIPELINEDESCRIPTOR hPipelineDescriptor)
{
    ChiContext*         pChiContext         = GetChiContext(hChiContext);
    PipelineDescriptor* pPipelineDescriptor = GetChiPipelineDescriptor(hPipelineDescriptor);

    pChiContext->DestroyPipelineDescriptor(pPipelineDescriptor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiCreateSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHIHANDLE ChiCreateSession(
    CHIHANDLE        hChiContext,
    UINT             numPipelines,
    CHIPIPELINEINFO* pPipelineInfo,
    CHICALLBACKS*    pCallbacks,
    VOID*            pPrivateCallbackData,
    CHISESSIONFLAGS  sessionCreateflags)
{
    CHISession* pCHISession = NULL;

    if ((NULL != hChiContext) && (NULL != pPipelineInfo) && (NULL != pCallbacks))
    {
        ChiContext* pChiContext = GetChiContext(hChiContext);

        pCHISession = pChiContext->CreateSession(numPipelines,
                                                 pPipelineInfo,
                                                 pCallbacks,
                                                 pPrivateCallbackData,
                                                 sessionCreateflags);
    }

    return reinterpret_cast<CHIHANDLE>(pCHISession);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiDestroySession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID ChiDestroySession(
    CHIHANDLE hChiContext,
    CHIHANDLE hSession,
    BOOL      isForced)
{
    CAMX_UNREFERENCED_PARAM(isForced);

    ChiContext* pChiContext = GetChiContext(hChiContext);
    CHISession* pCHISession = GetChiSession(hSession);

    pChiContext->DestroySession(pCHISession);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFlushSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiFlushSession(
    CHIHANDLE           hChiContext,
    CHISESSIONFLUSHINFO hSessionFlushInfo)
{
    CDKResult result = CDKResultSuccess;

    ChiContext* pChiContext = GetChiContext(hChiContext);
    CHISession* pChiSession = GetChiSession(hSessionFlushInfo.pSessionHandle);

    if ((NULL == pChiContext) || (NULL == pChiSession))
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid Argument - ChiContext: %p ChiSession: %p", pChiContext, pChiSession);

        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        result = pChiContext->FlushSession(pChiSession, hSessionFlushInfo);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiActivatePipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiActivatePipeline(
    CHIHANDLE          hChiContext,
    CHIHANDLE          hSession,
    CHIHANDLE          hPipeline,
    CHISENSORMODEINFO* pModeInfo)
{
    CDKResult           result              = CDKResultSuccess;
    ChiContext*         pChiContext         = GetChiContext(hChiContext);
    CHISession*         pChiSession         = GetChiSession(hSession);
    CHIPIPELINEHANDLE   hPipelineDescriptor = hPipeline;

    CAMX_UNREFERENCED_PARAM(pModeInfo);

    result = pChiContext->ActivatePipeline(pChiSession, hPipelineDescriptor);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiDeactivatePipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiDeactivatePipeline(
    CHIHANDLE                   hChiContext,
    CHIHANDLE                   hSession,
    CHIHANDLE                   hPipeline,
    CHIDEACTIVATEPIPELINEMODE   modeBitmask)
{
    CDKResult           result              = CDKResultSuccess;
    ChiContext*         pChiContext         = GetChiContext(hChiContext);
    CHISession*         pChiSession         = GetChiSession(hSession);
    CHIPIPELINEHANDLE   hPipelineDescriptor = hPipeline;

    result = pChiContext->DeactivatePipeline(pChiSession, hPipelineDescriptor, modeBitmask);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiQueryPipelineMetadataInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiQueryPipelineMetadataInfo(
    CHIHANDLE                   hChiContext,
    CHIHANDLE                   hSession,
    CHIPIPELINEDESCRIPTOR       hPipelineDescriptor,
    CHIPIPELINEMETADATAINFO*    pPipelineMetadataInfo)
{
    CDKResult   result      = CDKResultSuccess;
    ChiContext* pChiContext = GetChiContext(hChiContext);
    CHISession* pChiSession = GetChiSession(hSession);

    if ((NULL != pChiContext) && (NULL != pChiSession) && (NULL != hPipelineDescriptor) && (NULL != pPipelineMetadataInfo))
    {
        result = pChiContext->QueryMetadataInfo(
            pChiSession,
            hPipelineDescriptor,
            CAMX_ARRAY_SIZE(pPipelineMetadataInfo->publishTagArray),
            &pPipelineMetadataInfo->publishTagArray[0],
            &pPipelineMetadataInfo->publishTagCount,
            &pPipelineMetadataInfo->publishPartialTagCount,
            &pPipelineMetadataInfo->maxNumMetaBuffers);
    }
    else
    {
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSubmitPipelineRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ChiSubmitPipelineRequest(
    CHIHANDLE           hChiContext,
    CHIPIPELINEREQUEST* pRequest)
{
    CDKResult   result      = CDKResultSuccess;
    ChiContext* pChiContext = GetChiContext(hChiContext);
    CHISession* pChiSession = GetChiSession(pRequest->pSessionHandle);

    if ((NULL == pChiContext) || (NULL == pChiSession))
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid Argument - ChiContext: %p ChiSession: %p", pChiContext, pChiSession);

        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        result = pChiContext->SubmitRequest(pChiSession, pRequest);

        if (CDKResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "Submit request failed with error %d.", result);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiOverrideBypass
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiOverrideBypass(
    HALCallbacks* pBypassCallbacks)
{
    CAMX_UNREFERENCED_PARAM(pBypassCallbacks);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetSocId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static ChiCameraFamilySoc GetSocId()
{
    switch(HwEnvironment::GetInstance()->GetSocId())
    {
        case CSLCameraTitanSocSDM845:
            return ChiCameraTitanSocSDM845;
        case CSLCameraTitanSocSDM670:
            return ChiCameraTitanSocSDM670;
        case CSLCameraTitanSocSDM855:
            return ChiCameraTitanSocSDM855;
        case CSLCameraTitanSocSDM855P:
            return ChiCameraTitanSocSDM855P;
        case CSLCameraTitanSocQCS605:
            return ChiCameraTitanSocQCS605;
        case CSLCameraTitanSocSM6150:
            return ChiCameraTitanSocSM6150;
        case CSLCameraTitanSocSDM865:
            return ChiCameraTitanSocSDM865;
        case CSLCameraTitanSocSM7150:
            return ChiCameraTitanSocSM7150;
        case CSLCameraTitanSocSM7150P:
            return ChiCameraTitanSocSM7150P;
        case CSLCameraTitanSocSDM710:
            return ChiCameraTitanSocSDM710;
        case CSLCameraTitanSocSXR1120:
            return ChiCameraTitanSocSXR1120;
        case CSLCameraTitanSocSXR1130:
            return ChiCameraTitanSocSXR1130;
        case CSLCameraTitanSocSDM712:
            return ChiCameraTitanSocSDM712;
        case CSLCameraTitanSocSM7250:
            return ChiCameraTitanSocSM7250;
        case CSLCameraTitanSocSM6250:
            return ChiCameraTitanSocSM6250;
        case CSLCameraTitanSocQSM7250:
            return ChiCameraTitanSocQSM7250;
        case CSLCameraTitanSocSM6350:
            return ChiCameraTitanSocSM6350;
        case CSLCameraTitanSocSM7225:
            return ChiCameraTitanSocSM7225;
        case CSLCameraTitanSocInvalid:
        default:
            CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid Camera Soc Id");
            return ChiCameraTitanSocInvalid;
    }
}

CAMX_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEntry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC VOID ChiEntry(
    ChiContextOps* pChiContextOps)
{
    if (NULL != pChiContextOps)
    {
        pChiContextOps->size                       = sizeof(ChiContextOps);

        pChiContextOps->majorVersion               = CHI_API_MAJOR_VERSION;
        pChiContextOps->minorVersion               = CHI_API_MINOR_VERSION;
        pChiContextOps->subVersion                 = CHI_API_SUB_VERSION;
        pChiContextOps->pOpenContext               = CamX::ChiOpenContext;
        pChiContextOps->pCloseContext              = CamX::ChiCloseContext;
        pChiContextOps->pGetNumCameras             = CamX::ChiGetNumCameras;
        pChiContextOps->pGetCameraInfo             = CamX::ChiGetCameraInfo;
        pChiContextOps->pEnumerateSensorModes      = CamX::ChiEnumerateSensorModes;
        pChiContextOps->pCreatePipelineDescriptor  = CamX::ChiCreatePipelineDescriptor;
        pChiContextOps->pDestroyPipelineDescriptor = CamX::ChiDestroyPipelineDescriptor;
        pChiContextOps->pCreateSession             = CamX::ChiCreateSession;
        pChiContextOps->pDestroySession            = CamX::ChiDestroySession;
        pChiContextOps->pFlushSession              = CamX::ChiFlushSession;
        pChiContextOps->pActivatePipeline          = CamX::ChiActivatePipeline;
        pChiContextOps->pDeactivatePipeline        = CamX::ChiDeactivatePipeline;
        pChiContextOps->pSubmitPipelineRequest     = CamX::ChiSubmitPipelineRequest;
        pChiContextOps->pQueryPipelineMetadataInfo = CamX::ChiQueryPipelineMetadataInfo;
        pChiContextOps->pTagOps                    = CamX::ChiGetTagOps;
        pChiContextOps->pGetFenceOps               = CamX::ChiGetFenceOps;
        pChiContextOps->pMetadataOps               = CamX::ChiGetMetadataOps;
        pChiContextOps->pGetBufferManagerOps       = CamX::ChiGetBufferManagerOps;
        pChiContextOps->pGetSettings               = CamX::ChiGetSettings;
        pChiContextOps->pGetSocId                  = CamX::GetSocId;
    }

    // This is the workaround for presil HAL3test on Windows
    // On Device, set_camera_metadata_vendor_ops will be call the set the
    // static vendor tag operation in camera_metadata.c
    //
    // On Windows side, theoretically hal3test should mimic what Android framework
    // does and call the set_camera_metadata_vendor_ops function in libcamxext library
    // However, in Windows, if both hal3test.exe and hal.dll link to libcamxext library,
    // there are two different instance of static varibles sit in different memory location.
    // Even if set_camera_metadata_vendor_ops is called in hal3test, when hal try to
    // access to vendor tag ops, it is still not set.
    //
    // This is also a workaround to call vendor tag ops in Chi at GetNumCameras which happens to get called before
    // GetVendorTagOps
    CamX::g_vendorTagOps.get_all_tags     = CamX::ChiGetAllTags;
    CamX::g_vendorTagOps.get_section_name = CamX::ChiGetSectionName;
    CamX::g_vendorTagOps.get_tag_count    = CamX::ChiGetTagCount;
    CamX::g_vendorTagOps.get_tag_name     = CamX::ChiGetTagName;
    CamX::g_vendorTagOps.get_tag_type     = CamX::ChiGetTagType;

    set_camera_metadata_vendor_ops(&(CamX::g_vendorTagOps));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiDumpState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiDumpState(
    INT fd)
{
    CamX::g_pChiContext->DumpState(fd);
}
