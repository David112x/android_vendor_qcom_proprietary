////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxmultifovzoomratiocontroller.cpp
/// @brief chxmultifovzoomratiocontroller class definition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "chxmultifovzoomratiocontroller.h"
#include "chivendortag.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MultiFovZoomRatioController::TranslateRequestSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult MultiFovZoomRatioController::TranslateRequestSettings(
    MulticamReqSettings* pMultiCamSettings)
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pMultiCamSettings)
    {
        BOOL isUltraWideMaster = (m_camIdUltraWide == m_result.masterCameraId) ? TRUE : FALSE;

        if (TRUE == isUltraWideMaster)
        {
            CameraSettings primarySettings;
            CameraSettings ultrawideSettings;
            UINT32         primaryCamIdx       = 0;
            UINT32         translatedMetacount = 0;

            ChxUtils::Memset(&primarySettings, 0, sizeof(primarySettings));
            ChxUtils::Memset(&ultrawideSettings, 0, sizeof(ultrawideSettings));

            for (UINT32 i = 0; i < pMultiCamSettings->numSettingsBuffers; i++)
            {
                ChiMetadata* pMetadata = pMultiCamSettings->ppSettings[i];

                // Get CameraID from metadata
                UINT32* pCameraId = static_cast <UINT32*>(pMetadata->GetTag(
                        MultiCamControllerManager::m_vendorTagMetadataOwner));

                if (NULL != pCameraId)
                {
                    UINT32 cameraId = *pCameraId;
                    if (m_primaryCamId == cameraId)
                    {
                        primarySettings.cameraId = *pCameraId;
                        primarySettings.pMetadata = pMetadata;
                        primaryCamIdx = GetCameraIndex(primarySettings.cameraId);
                        CHX_ASSERT(INVALID_INDEX != primaryCamIdx);
                    }
                    else if (m_camIdUltraWide == cameraId)
                    {
                        ultrawideSettings.cameraId = *pCameraId;
                        ultrawideSettings.pMetadata = pMetadata;
                    }
                }
                else
                {
                    CHX_LOG_ERROR("Metadata tag MetadataOwner is NULL. Cannot get cameraId");
                    result = CDKResultEFailed;
                }
            }

            if (NULL == primarySettings.pMetadata || NULL == ultrawideSettings.pMetadata)
            {
                CHX_LOG_ERROR("Primary setting metadata is NULL");
                result = CDKResultEFailed;
            }

            if (CDKResultSuccess == result)
            {
                ChiMetadata* pMetadata           = primarySettings.pMetadata;
                ChiMetadata* pTranslatedMetadata = ultrawideSettings.pMetadata;
                if ((NULL != pMetadata) && (NULL != pTranslatedMetadata))
                {
                    result = TranslateRequestSettingsForUltraWide(pMetadata, pTranslatedMetadata);
                    TranslateAFRegions(pMetadata, pTranslatedMetadata, m_camIdWide, m_camIdUltraWide);
                    TranslateAERegions(pMetadata, pTranslatedMetadata, m_camIdWide, m_camIdUltraWide);
                    UpdateVendorTags(pMultiCamSettings);
                }
                else
                {
                    CHX_LOG_ERROR("primary setting metadata is %p UW %p", pMetadata, pTranslatedMetadata);
                }
            }
        }
        else
        {
            result = MultiFovController::TranslateRequestSettings(pMultiCamSettings);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MultiFovZoomRatioController::ProcessResultMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MultiFovZoomRatioController::ProcessResultMetadata(
    ChiMetadata* pResultMetadata)
{
    if (FALSE == m_overrideMode)
    {
        MultiFovController::ProcessResultMetadata(pResultMetadata);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MultiFovZoomRatioController::TranslateResultMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MultiFovZoomRatioController::TranslateResultMetadata(
    camera_metadata_t* pResultMetadata)
{
    if (FALSE == m_overrideMode)
    {
        MultiFovController::TranslateResultMetadata(pResultMetadata);
    }
    else
    {
        // Send realtime result to derived class so it can take actions if any.
        MCCSetParam setParamArray[1]      = {};
        setParamArray[0].setParamType     = MCCSetParamTypeRealtimeResultMeta;
        setParamArray[0].pMCCSetParamData = static_cast<VOID*>(pResultMetadata);
        PrepareAndSetParam(1, setParamArray);

        // Translate the FD ROI
        TranslateFaceRegions(pResultMetadata);

        MCCGetParamInput  queryList[1]   = {};
        MCCGetParamOutput outPutList[1]  = {};
        queryList[0].pInputData          = static_cast<VOID*>(&pResultMetadata);
        queryList[0].type                = MCCGetParamTypePassRealtimeResultMeta;
        outPutList[0].getParamOutputType = MCCGetParamTypeOverrideScalerCrop;

        PrepareAndGetParam(1, queryList, outPutList);

        CHIRECT cropRect;
        camera_metadata_entry_t entry = { 0 };
        if (1 == outPutList->sizeOfGetParamOutput &&
            MCCGetParamTypeOverrideScalerCrop == outPutList->getParamOutputType &&
            NULL != outPutList[0].pGetParamOutput)
        {
            cropRect = *(static_cast<CHIRECT *>(outPutList[0].pGetParamOutput));
            if (0 == find_camera_metadata_entry(pResultMetadata, ANDROID_SCALER_CROP_REGION, &entry))
            {
                UpdateMetadata(pResultMetadata, entry.index, &cropRect, sizeof(CHIRECT) / sizeof(INT32), NULL);
            }
            else
            {
                add_camera_metadata_entry(pResultMetadata, ANDROID_SCALER_CROP_REGION, &cropRect, 0);
            }
        }
        else
        {
            // remove it when derived class is ready;
            cropRect = {0, 0, m_camInfo[m_camIdxWide].activeArraySize.width, m_camInfo[m_camIdxWide].activeArraySize.height};
            if (0 == find_camera_metadata_entry(pResultMetadata, ANDROID_SCALER_CROP_REGION, &entry))
            {
                UpdateMetadata(pResultMetadata, entry.index, &cropRect, sizeof(CHIRECT) / sizeof(INT32), NULL);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MultiFovZoomRatioController::TranslateRequestSettingsForUltraWide
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult MultiFovZoomRatioController::TranslateRequestSettingsForUltraWide(
    ChiMetadata* pMetadata,
    ChiMetadata* pTranslatedMetadata)
{
    // Translate the zoom crop window
    camera_metadata_entry_t entryCropRegion      = { 0 };
    camera_metadata_entry_t entryCropRegionTrans = { 0 };

    FLOAT                       zoomWide;
    FLOAT                       zoomUltraWide;
    TranslatedZoom              translatedZoom;
    CHIRECTINT                  userZoom;
    CaptureRequestCropRegions   cropRegionsWide;
    CaptureRequestCropRegions   cropRegionsUltraWide;

    FLOAT appZoomRatio = GetAppZoomRatio(pMetadata);
    m_zoomUser         = appZoomRatio;

    TranslateZoomForUltraWide(appZoomRatio, &translatedZoom);

    // Update the zoom value based on output of Zoom Translator
    ZoomRegions zoomPreview = translatedZoom.zoomPreview;
    cropRegionsWide.userCropRegion     = userZoom;
    cropRegionsWide.pipelineCropRegion = zoomPreview.totalZoom[m_camIdxWide];
    cropRegionsWide.ifeLimitCropRegion = zoomPreview.ispZoom[m_camIdxWide];

    cropRegionsUltraWide.userCropRegion     = userZoom;
    cropRegionsUltraWide.pipelineCropRegion = zoomPreview.totalZoom[m_camIdxUltraWide];
    cropRegionsUltraWide.ifeLimitCropRegion = zoomPreview.ispZoom[m_camIdxUltraWide];

    pMetadata->SetTag(ANDROID_SCALER_CROP_REGION, &cropRegionsWide.pipelineCropRegion, 4);

    if (0 == pTranslatedMetadata->FindTag(ANDROID_SCALER_CROP_REGION, &entryCropRegionTrans))
    {
        CHX_LOG_INFO("User Zoom Ratio : %f - CropRegion [%d, %d, %d, %d] \n",
                            m_zoomUser, zoomPreview.totalZoom[m_camIdxUltraWide].left,
                            zoomPreview.totalZoom[m_camIdxUltraWide].top,
                            zoomPreview.totalZoom[m_camIdxUltraWide].width,
                            zoomPreview.totalZoom[m_camIdxUltraWide].height);
        pTranslatedMetadata->SetTag(ANDROID_SCALER_CROP_REGION, &zoomPreview.totalZoom[m_camIdxUltraWide], 4);
    }

    /* Update the vendor tag for the CropRegions containing
    user crop, pipeline crop and IFE crop limit */
    ChxUtils::SetVendorTagValue(pMetadata, VendorTag::CropRegions,
        sizeof(CaptureRequestCropRegions), &cropRegionsWide);
    ChxUtils::SetVendorTagValue(pTranslatedMetadata, VendorTag::CropRegions,
        sizeof(CaptureRequestCropRegions), &cropRegionsUltraWide);

    translatedZoom.zoomSnapshot.totalZoom[m_camIdxWide].width = m_camInfo[m_camIdxWide].activeArraySize.width/appZoomRatio;
    zoomWide      = m_camInfo[m_camIdxWide].activeArraySize.width /
                    static_cast<FLOAT>(translatedZoom.zoomSnapshot.totalZoom[m_camIdxWide].width);
    zoomUltraWide = m_camInfo[m_camIdxUltraWide].activeArraySize.width /
                    static_cast<FLOAT>(translatedZoom.zoomSnapshot.totalZoom[m_camIdxUltraWide].width);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MultiFovZoomRatioController::TranslateAFRegions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MultiFovZoomRatioController::TranslateAFRegions(
    ChiMetadata*   pMetadata,
    ChiMetadata*   pTranslatedMetadata,
    UINT32         primaryCameraId,
    UINT32         auxCameraId)
{
    // Translate the Focus ROI
    camera_metadata_entry_t entryAFRegionMain = { 0 };
    camera_metadata_entry_t entryAFRegionAux  = { 0 };

    if ((0 == pMetadata->FindTag( ANDROID_CONTROL_AF_REGIONS, &entryAFRegionMain)) &&
        (0 == pTranslatedMetadata->FindTag(ANDROID_CONTROL_AF_REGIONS, &entryAFRegionAux)))
    {
        // AF_REGION tag tuple has 5 elements [xMin, yMin, xMax, yMax, weight]
        UINT32 tupleSize        = 5;
        FLOAT  appZoomRatioUW   = 1.0F;
        for (UINT32 i = 0; i < entryAFRegionMain.count / tupleSize; i++)
        {
            WeightedRegion afRegion = { 0 };
            UINT32 index = (i * tupleSize);

            afRegion.xMin   = entryAFRegionMain.data.i32[index];
            afRegion.yMin   = entryAFRegionMain.data.i32[index + 1];
            afRegion.xMax   = entryAFRegionMain.data.i32[index + 2];
            afRegion.yMax   = entryAFRegionMain.data.i32[index + 3];
            afRegion.weight = entryAFRegionMain.data.i32[index + 4];

            // Get the translated AF ROI for the wide camera
            WeightedRegion afRegionTrans = TranslateMeteringRegion(&afRegion, primaryCameraId, appZoomRatioUW);
            // Update the metadata
            pMetadata->SetTag(ANDROID_CONTROL_AF_REGIONS, &afRegionTrans, 5);

            // Get the translated AF ROI for the aux camera
            if (m_camIdUltraWide == auxCameraId)
            {
                appZoomRatioUW = GetAppZoomRatio(pMetadata);
            }
            afRegionTrans = TranslateMeteringRegion(&afRegion, auxCameraId, appZoomRatioUW);
            // Update the metadata
            pTranslatedMetadata->SetTag(ANDROID_CONTROL_AF_REGIONS, &afRegionTrans, 5);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MultiFovZoomRatioController::TranslateAERegions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MultiFovZoomRatioController::TranslateAERegions(
    ChiMetadata*   pMetadata,
    ChiMetadata*   pTranslatedMetadata,
    UINT32         primaryCameraId,
    UINT32         auxCameraId)
{
    // Translate the Metering ROI
    camera_metadata_entry_t entryAERegionMain = { 0 };
    camera_metadata_entry_t entryAERegionAux  = { 0 };

    if ((0 == pMetadata->FindTag(ANDROID_CONTROL_AE_REGIONS, &entryAERegionMain)) &&
        (0 == pTranslatedMetadata->FindTag(ANDROID_CONTROL_AE_REGIONS, &entryAERegionAux)))
    {
        // AE_REGION tag tuple has 5 elements [xMin, yMin, xMax, yMax, weight]
        UINT32 tupleSize         = 5;
        FLOAT  appZoomRatioUW    = 1.0F;

        for (UINT32 i = 0; i < entryAERegionMain.count / tupleSize; i++)
        {
            WeightedRegion aeRegion = { 0 };
            UINT32 index = i * tupleSize;

            aeRegion.xMin   = entryAERegionMain.data.i32[index];
            aeRegion.yMin   = entryAERegionMain.data.i32[index + 1];
            aeRegion.xMax   = entryAERegionMain.data.i32[index + 2];
            aeRegion.yMax   = entryAERegionMain.data.i32[index + 3];
            aeRegion.weight = entryAERegionMain.data.i32[index + 4];

            // Get the translated AE ROI for the wide camera
            WeightedRegion aeRegionTrans = TranslateMeteringRegion(&aeRegion, primaryCameraId, appZoomRatioUW);

            // Update the metadata
            pMetadata->SetTag(ANDROID_CONTROL_AE_REGIONS,
                &aeRegionTrans, 5);

            // Get the translated AE ROI for the aux camera
            if (m_camIdUltraWide == auxCameraId)
            {
                appZoomRatioUW = GetAppZoomRatio(pMetadata);
            }
            aeRegionTrans = TranslateMeteringRegion(&aeRegion, auxCameraId, appZoomRatioUW);

            // Update the metadata
            pTranslatedMetadata->SetTag(ANDROID_CONTROL_AE_REGIONS,
                &aeRegionTrans, 5);
        }
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MultiFovZoomRatioController::TranslateMeteringRegion
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
WeightedRegion MultiFovZoomRatioController::TranslateMeteringRegion(
    WeightedRegion* pRegion,
    UINT32          camId,
    FLOAT           appZoomRatio)
{
    FLOAT             fovRatio    = 1.0f;
    WeightedRegion    regionInput = *pRegion;
    WeightedRegion    regionTrans = regionInput;
    CHIRECT           activeArray = { 0 };

    if (regionInput.xMax <= 0 || regionInput.yMax <= 0)
    {
        CHX_LOG_INFO("Skip translation due to width %d or height %d smaller than 0",
            regionInput.xMax, regionInput.yMax);
        return regionTrans;
    }

    if (m_camIdUltraWide == camId)
    {
        TranslatedZoom translatedZoom;
        TranslateZoomForUltraWide(appZoomRatio, &translatedZoom);

        // AF/AE ROIs are with respect to Wide FOV,
        // Translate it first to UW FOV and then change reference to Active array of ultrawide
        regionTrans.xMin = (regionInput.xMin * ((translatedZoom.zoomPreview.totalZoom[m_camIdxUltraWide].width * 1.0f)/
                            m_camInfo[m_camIdxWide].activeArraySize.width)) +
                            (translatedZoom.zoomPreview.totalZoom[m_camIdxUltraWide].left);
        regionTrans.yMin = (regionInput.yMin * ((translatedZoom.zoomPreview.totalZoom[m_camIdxUltraWide].width * 1.0f)/
                             m_camInfo[m_camIdxWide].activeArraySize.width)) +
                             (translatedZoom.zoomPreview.totalZoom[m_camIdxUltraWide].top);
        regionTrans.xMax = (regionInput.xMax * ((translatedZoom.zoomPreview.totalZoom[m_camIdxUltraWide].width * 1.0f)/
                             m_camInfo[m_camIdxWide].activeArraySize.width)) +
                             (translatedZoom.zoomPreview.totalZoom[m_camIdxUltraWide].left);
        regionTrans.yMax = (regionInput.yMax * ((translatedZoom.zoomPreview.totalZoom[m_camIdxUltraWide].width * 1.0f)/
                             m_camInfo[m_camIdxWide].activeArraySize.width)) +
                             (translatedZoom.zoomPreview.totalZoom[m_camIdxUltraWide].top);

        activeArray = m_camInfo[m_camIdxUltraWide].activeArraySize;

        CHX_LOG_INFO("Translated region UW = %d %d %d %d", regionTrans.xMin,
                      regionTrans.yMin,
                      regionTrans.xMax,
                      regionTrans.yMax);
    }

    INT32 activeArrayWidth  = activeArray.width;
    INT32 activeArrayHeight = activeArray.height;

    // Check ROI bounds and correct it if necessary
    if ((regionTrans.xMin < 0) ||
        (regionTrans.yMin < 0) ||
        (regionTrans.xMax > activeArrayWidth) ||
        (regionTrans.yMax > activeArrayHeight))
    {
        if (regionTrans.xMin < 0)
        {
            regionTrans.xMin = 0;
        }
        if (regionTrans.yMin < 0)
        {
            regionTrans.yMin = 0;
        }
        if (regionTrans.xMax >= activeArrayWidth)
        {
            regionTrans.xMax = activeArrayWidth - 1;
        }
        if (regionTrans.yMax >= activeArrayHeight)
        {
            regionTrans.yMax = activeArrayHeight - 1;
        }
    }

    return regionTrans;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MultiFovZoomRatioController::TranslateZoomForUltraWide
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MultiFovZoomRatioController::TranslateZoomForUltraWide(
    FLOAT           appZoomRatio,
    TranslatedZoom* pTranslatedZoom)
{

    CHIRECT relativeZoom;
    // Ultrawide's FOV is twice of Wide find relative FOV = 1.0x FOV of primary/wide
    relativeZoom.left   = (m_camInfo[m_camIdxUltraWide].activeArraySize.width/4);
    relativeZoom.top    = (m_camInfo[m_camIdxUltraWide].activeArraySize.height/4);
    relativeZoom.height = (m_camInfo[m_camIdxUltraWide].activeArraySize.height/2);
    relativeZoom.width  = (m_camInfo[m_camIdxUltraWide].activeArraySize.width/2);

    UINT32 ultrawideCamIdx = GetCameraIndex(m_camIdUltraWide);
    UINT32 wideCamIdx      = GetCameraIndex(m_camIdWide);

    if (appZoomRatio < m_transitionWideToUltraWideHigh && appZoomRatio >= m_transitionWideToUltraWideLow)
    {
        // update when derived class is ready
        // Check if derived class wants to overrise the zoom
        MCCGetParamOutput outPutList[1]  = {};

        MCCGetParamOutput pOutput        = outPutList[0];
        pOutput.getParamOutputType      = MCCGetParamTypeOverrideZoom;
        PrepareAndGetParam(1, NULL, outPutList);

        VOID* pOutputdata = pOutput.pGetParamOutput;
        if (0 /* NULL != outputdata */)
        {
            pTranslatedZoom  = static_cast<TranslatedZoom *>(pOutputdata);
        }
        else
        {
            // QC Formula
            // zoom ratio 0.5x is full fov of ultrawide and 1.0x is relative roi calculated above
            // which matches 1.0x FOV of wide.
            pTranslatedZoom->zoomPreview.totalZoom[ultrawideCamIdx].left =
                (appZoomRatio - m_transitionWideToUltraWideLow) * 2.0F * relativeZoom.left;
            pTranslatedZoom->zoomPreview.totalZoom[ultrawideCamIdx].top  =
                (appZoomRatio - m_transitionWideToUltraWideLow) * 2.0F * relativeZoom.top;

            // While calculating width and heigh, consider relative roi as reference and add both the sides =
            // 2*(difference between relative left and calculated left/top)
            // to calculate width and height
            pTranslatedZoom->zoomPreview.totalZoom[ultrawideCamIdx].width  =
                relativeZoom.width + ((relativeZoom.left - pTranslatedZoom->zoomPreview.totalZoom[ultrawideCamIdx].left)
                *2.0F);
            pTranslatedZoom->zoomPreview.totalZoom[ultrawideCamIdx].height =
                relativeZoom.height + ((relativeZoom.top- pTranslatedZoom->zoomPreview.totalZoom[ultrawideCamIdx].top)
                *2.0F);
        }
        CHX_LOG_INFO("translatedZoom l_t_w_H : [%d %d %d %d] -",
        pTranslatedZoom->zoomPreview.totalZoom[ultrawideCamIdx].left,
        pTranslatedZoom->zoomPreview.totalZoom[ultrawideCamIdx].top,
        pTranslatedZoom->zoomPreview.totalZoom[ultrawideCamIdx].width,
        pTranslatedZoom->zoomPreview.totalZoom[ultrawideCamIdx].height);
    }
    else
    {
        pTranslatedZoom->zoomPreview.totalZoom[ultrawideCamIdx].top    = 0;
        pTranslatedZoom->zoomPreview.totalZoom[ultrawideCamIdx].left   = 0;
        pTranslatedZoom->zoomPreview.totalZoom[ultrawideCamIdx].width  = m_camInfo[m_camIdxUltraWide].activeArraySize.width;
        pTranslatedZoom->zoomPreview.totalZoom[ultrawideCamIdx].height = m_camInfo[m_camIdxUltraWide].activeArraySize.height;
    }

    pTranslatedZoom->zoomPreview.ispZoom[ultrawideCamIdx] = pTranslatedZoom->zoomPreview.totalZoom[ultrawideCamIdx];
    pTranslatedZoom->zoomSnapshot = pTranslatedZoom->zoomPreview;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MultiFovZoomRatioController::GetResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ControllerResult MultiFovZoomRatioController::GetResult(
    ChiMetadata* pMetadata,
    UINT32 snapshotActiveMask)
{
    (void)snapshotActiveMask;
    m_pLock->Lock();
    CheckOverrideMccResult(pMetadata);
    MCCGetParamInput  queryList[1]   = {};
    MCCGetParamOutput outPutList[1]  = {};

    queryList[0].pInputData          = static_cast<VOID*>(&m_result);
    queryList[0].type                = MCCGetParamTypePassControllerResult;
    outPutList[0].getParamOutputType = MCCGetParamTypeOverrideResult;

    PrepareAndGetParam(1, queryList, outPutList);
    ControllerResult result;

    if (1 == outPutList->sizeOfGetParamOutput &&
        MCCGetParamTypeOverrideResult == outPutList->getParamOutputType &&
        NULL != outPutList[0].pGetParamOutput)
    {
        m_result = *(static_cast<ControllerResult *>(outPutList[0].pGetParamOutput));
    }

    m_result.activeMap = 0;
    for (UINT32 i = 0; i < m_result.numOfActiveCameras; i++)
    {
        if (TRUE == m_result.activeCameras[i].isActive)
        {
            m_result.activeMap |= 1 << i;
        }
    }
    result = m_result;
    m_pLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MultiFovZoomRatioController::CheckOverrideMccResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MultiFovZoomRatioController::CheckOverrideMccResult(
    ChiMetadata* pMetadata)
{
    if (NULL != pMetadata)
    {
        // Get Zoom ratio from derived class
        FLOAT     appZoomRatio     = GetAppZoomRatio(pMetadata);
        UINT32    currentMasterId  = m_result.masterCameraId;
        BOOL      masterUpdated    = FALSE;

        if (appZoomRatio < m_transitionWideToUltraWideHigh && appZoomRatio >= m_transitionWideToUltraWideLow)
        {
            m_result.masterCameraId       = m_camIdUltraWide;
            for (UINT32 i = 0; i < m_result.numOfActiveCameras; i++)
            {
                if (m_result.activeCameras[i].cameraId == m_result.masterCameraId)
                {
                    m_result.activeCameras[i].isActive = TRUE;
                }
                else
                {
                    m_result.activeCameras[i].isActive = FALSE;
                }
            }

            // Override SAT results for ultrawide
            m_overrideMode                = TRUE;
            CHX_LOG("SETTING UW camid0=%d camid1=%d camid2=%d",
                    m_result.activeCameras[0].cameraId,
                    m_result.activeCameras[1].cameraId,
                    m_result.activeCameras[2].cameraId);
        }
        else if (m_camIdUltraWide == m_result.masterCameraId)
        {
            // we have crossed UW limit and it is still master
            // update results asap to replicate correct master
            // Don't handle fallback for this transition
            UpdateResultsForInitialFrame(pMetadata);
        }
    }

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MultiFovZoomRatioController::UpdateResultsForInitialFrame
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MultiFovZoomRatioController::UpdateResultsForInitialFrame(
    ChiMetadata* pMetadata)
{
    if (NULL != pMetadata)
    {
        m_overrideMode   = FALSE;
        m_result.activeCameras[2].isActive = FALSE;

        if (FALSE == ENABLE_LPM)
        {
            m_result.activeCameras[0].isActive = TRUE;
            m_result.activeCameras[1].isActive = TRUE;
        }
        else
        {
            camera_metadata_entry_t entryCropRegion = { 0 };
            if (0 == pMetadata->FindTag(ANDROID_SCALER_CROP_REGION, &entryCropRegion))
            {
                FLOAT userZoomRatio = 0.0F;
                CHIRECT userZoom;
                userZoom.left = entryCropRegion.data.i32[0];
                userZoom.top = entryCropRegion.data.i32[1];
                userZoom.width = entryCropRegion.data.i32[2];
                userZoom.height = entryCropRegion.data.i32[3];

                userZoomRatio = m_camInfo[m_camIdxWide].activeArraySize.width / static_cast<FLOAT>(userZoom.width);
                CHX_LOG("UpdateResultsForInitialFrame userZoomRatio=%f , m_zoomUser = %f, width : %d",
                        userZoomRatio, m_zoomUser, userZoom.width);
                m_pLock->Lock();

                // check if transition ratio is used by SS.
                for (UINT32 i = 0; i < m_numOfLinkedSessions; i++)
                {
                    FLOAT minFovRatio = m_camInfo[i].transitionLeft.transitionRatio;
                    FLOAT maxFovRatio = m_camInfo[i].transitionRight.transitionRatio;
                    if ((minFovRatio <= userZoomRatio) && (maxFovRatio > userZoomRatio))
                    {
                        CHX_LOG_INFO("MinFov %f Max Fov %f, userzoom %f, camera %d, current master %d",
                                minFovRatio, maxFovRatio, userZoomRatio, m_camInfo[i].cameraId,
                                m_result.masterCameraId);
                        m_result.masterCameraId            = m_camInfo[i].cameraId;
                    }
                }

                for (UINT32 i = 0; i < m_result.numOfActiveCameras; i++)
                {
                    if (m_result.activeCameras[i].cameraId == m_result.masterCameraId)
                    {
                        m_result.activeCameras[i].isActive = TRUE;
                    }
                    else
                    {
                        m_result.activeCameras[i].isActive = FALSE;
                    }
                }
                m_pLock->Unlock();
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MultiFovZoomRatioController::TranslateFaceRegions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MultiFovZoomRatioController::TranslateFaceRegions(
    VOID* pResultMetadata)
{
    MultiCameraIds multiCameraId;
    camera_metadata_t* pMetadata         = static_cast<camera_metadata_t*>(pResultMetadata);
    if (CDKResultSuccess == MultiCamControllerManager::s_vendorTagOps.pGetMetaData(pMetadata,
            MultiCamControllerManager::m_vendorTagMultiCameraId, &multiCameraId, sizeof(MultiCameraIds)))
    {

        if (m_camIdUltraWide == multiCameraId.currentCameraId)
        {
            TranslateFaceRegionsUW(pResultMetadata, multiCameraId.currentCameraId);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MultiFovZoomRatioController::TranslateFaceRegionsUW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MultiFovZoomRatioController::TranslateFaceRegionsUW(
    VOID*           pResultMetadata,
    UINT32          camId)
{
    if (m_camIdUltraWide != camId)
    {
        CHX_LOG_WARN("cannot translate non uw faces");
    }
    else
    {
        CDKResult result = CDKResultSuccess;
        PGETMETADATA              pGetMetaData             = MultiCamControllerManager::s_vendorTagOps.pGetMetaData;
        camera_metadata_t*        pMetadata                = static_cast<camera_metadata_t*>(pResultMetadata);
        camera_metadata_entry_t entry = { 0 };

        if ((0 == find_camera_metadata_entry(pMetadata, ANDROID_STATISTICS_FACE_RECTANGLES, &entry)) &&
            (entry.count > 0))
        {
            UINT32 numElemsRect = sizeof(CHIRECT) / sizeof(UINT32);
            INT32 numFaces = entry.count / numElemsRect;
            INT32 numFacesValid = 0;

            CHIRECT faceRegions[FDMaxFaces];
            UINT32 dataIndex = 0;

            // Get the user zoom to remove the faces which are outside of preview FOV
            // CaptureRequestCropRegions* pCropRegions = static_cast<CaptureRequestCropRegions*>(pResultMetadata->GetTag(
             //       MultiCamControllerManager::m_vendorTagCropRegions));
            CaptureRequestCropRegions cropRegions;
              // Get the user zoom to remove the faces which are outside of preview FOV
            result = pGetMetaData(pMetadata,
                                  MultiCamControllerManager::m_vendorTagCropRegions,
                                  &cropRegions,
                                  sizeof(CaptureRequestCropRegions));

            for (INT32 i = 0; i < numFaces; ++i)
            {
                UINT32 xMin = entry.data.i32[dataIndex++];
                UINT32 yMin = entry.data.i32[dataIndex++];
                UINT32 xMax = entry.data.i32[dataIndex++];
                UINT32 yMax = entry.data.i32[dataIndex++];

                CHX_LOG("face rect from camera_metadata UW, l_t_r_b_(%d, %d, %d, %d), center(%d, %d)",
                        xMin, yMin, xMax, yMax,
                        (xMin + xMax) / 2, (yMin + yMax) / 2);

                FLOAT appZoomRatio = GetAppZoomRatio(static_cast<camera_metadata_t*>(pResultMetadata));

                TranslatedZoom translatedZoom;
                TranslateZoomForUltraWide(appZoomRatio, &translatedZoom);

                // Values coming from UW pipeline are related to Active Array of UW.
                // first Move the reference of Face ROI from Active array to Crop region FOV of ultrawide and then translate
                // this ROI from UW crop FOV to Wide FOV
                xMin = ((xMin -(translatedZoom.zoomPreview.totalZoom[m_camIdxUltraWide].left)) * (
                        (m_camInfo[m_camIdxWide].activeArraySize.width * 1.0f)/translatedZoom.zoomPreview.totalZoom[0].width));
                yMin = ((yMin - (translatedZoom.zoomPreview.totalZoom[m_camIdxUltraWide].top)) * (
                        (m_camInfo[m_camIdxWide].activeArraySize.width * 1.0f)/translatedZoom.zoomPreview.totalZoom[0].width));
                xMax =  ((xMax-(translatedZoom.zoomPreview.totalZoom[m_camIdxUltraWide].left))  * (
                         (m_camInfo[m_camIdxWide].activeArraySize.width * 1.0f)/translatedZoom.zoomPreview.totalZoom[0].width));
                yMax = ((yMax- (translatedZoom.zoomPreview.totalZoom[m_camIdxUltraWide].top))  * (
                         (m_camInfo[m_camIdxWide].activeArraySize.width * 1.0f)/translatedZoom.zoomPreview.totalZoom[0].width));

                CHX_LOG_INFO("FACE roi UW = %d %d %d %d", xMin, yMin, xMax, yMax);

                // Reject FD ROIs which are outside of the user crop window
                if (xMin > m_camInfo[m_camIdxWide].activeArraySize.left   &&
                    xMin <  m_camInfo[m_camIdxWide].activeArraySize.left + m_camInfo[m_camIdxWide].activeArraySize.width &&
                    yMin > m_camInfo[m_camIdxWide].activeArraySize.top   &&
                    yMin <  m_camInfo[m_camIdxWide].activeArraySize.top + m_camInfo[m_camIdxWide].activeArraySize.height &&
                    xMax > m_camInfo[m_camIdxWide].activeArraySize.left &&
                    xMax <  m_camInfo[m_camIdxWide].activeArraySize.left + m_camInfo[m_camIdxWide].activeArraySize.width &&
                    yMax > m_camInfo[m_camIdxWide].activeArraySize.top   &&
                    yMax <  m_camInfo[m_camIdxWide].activeArraySize.top + m_camInfo[m_camIdxWide].activeArraySize.height)
                {
                    faceRegions[numFacesValid].left   = xMin;
                    faceRegions[numFacesValid].top    = yMin;
                    faceRegions[numFacesValid].width  = xMax;
                    faceRegions[numFacesValid].height = yMax;
                    numFacesValid++;
                    CHX_LOG_INFO("FACE roi SET for UW = %d %d %d %d", xMin, yMin, xMax, yMax);
                }
            }

            if (numFacesValid > 0)
            {
                update_camera_metadata_entry(pMetadata,
                                             entry.index,
                                             faceRegions,
                                             numFacesValid * sizeof(CHIRECT) / sizeof(INT32),
                                             NULL);
            }
            else
            {
                INT32 status = delete_camera_metadata_entry(pMetadata, entry.index);

                if (CDKResultSuccess == status)
                {
                    status = add_camera_metadata_entry(pMetadata, ANDROID_STATISTICS_FACE_RECTANGLES, faceRegions, 0);
                }

                if (CDKResultSuccess != status)
                {
                    CHX_LOG_ERROR("Failed to delete/ add metadata entry for no face case");
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MultiFovZoomRatioController::UpdateScalerCropForSnapshot
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MultiFovZoomRatioController::UpdateScalerCropForSnapshot(
    ChiMetadata* pMetadata)
{
    CDKResult result = CDKResultSuccess;
    MultiCameraIds* pMultiCameraId;
    UINT32         currentCameraId = m_camIdWide;

    pMultiCameraId = static_cast <MultiCameraIds*>(pMetadata->GetTag(
                                                        MultiCamControllerManager::m_vendorTagMultiCameraId));

    if (NULL != pMultiCameraId)
    {
        currentCameraId = pMultiCameraId->currentCameraId;
    }

    if (m_camIdUltraWide != currentCameraId)
    {
        MultiFovController::UpdateScalerCropForSnapshot(pMetadata);

    }
    else
    {
        CaptureRequestCropRegions* pCropRegions;
        pCropRegions = static_cast <CaptureRequestCropRegions *>(pMetadata->GetTag(
            MultiCamControllerManager::m_vendorTagCropRegions));
        if (NULL != pCropRegions)
        {
            TranslatedZoom translatedZoom;
            m_pZoomTranslator->GetTranslatedZoom(&pCropRegions->userCropRegion, &translatedZoom);
            ZoomRegions zoomSnapshot = translatedZoom.zoomSnapshot;

            CHIRECT zoomTotal;
            CHIRECT zoomIspLimit;
            CHIRECT activeArraySize;

            ChxUtils::Memset(&zoomTotal, 0, sizeof(zoomTotal));
            ChxUtils::Memset(&zoomIspLimit, 0, sizeof(zoomIspLimit));
            ChxUtils::Memset(&activeArraySize, 0, sizeof(activeArraySize));

            if (m_camIdUltraWide == currentCameraId)
            {
                // Set the reference crop window for both Wide and Tele
                RefCropWindowSize refCropWindowUltraWide;
                UINT32 refCropSizeTag = 0;
                result = MultiCamControllerManager::s_vendorTagOps.pQueryVendorTagLocation(
                        "org.quic.camera2.ref.cropsize", "RefCropSize", &refCropSizeTag);
                if (CDKResultSuccess == result)
                {
                    refCropWindowUltraWide.width = m_camInfo[m_camIdxUltraWide].activeArraySize.width;
                    refCropWindowUltraWide.height = m_camInfo[m_camIdxUltraWide].activeArraySize.height;
                    result = pMetadata->SetTag(refCropSizeTag,
                            &refCropWindowUltraWide, sizeof(refCropWindowUltraWide));
                }

                activeArraySize    = m_camInfo[m_camIdxUltraWide].activeArraySize;

                FLOAT appZoomRatio = GetAppZoomRatio(pMetadata);
                TranslateZoomForUltraWide(appZoomRatio, &translatedZoom);
                zoomTotal          = translatedZoom.zoomPreview.totalZoom[0];
                zoomIspLimit       = zoomSnapshot.ispZoom[0];
            }

            /* Update the vendor tag for the CropRegions containing
        user crop, pipeline crop and IFE crop limit */

            CHIRECTINT* pUserZoom = reinterpret_cast<CHIRECTINT*>(&activeArraySize);

            pCropRegions->userCropRegion     = *pUserZoom;
            pCropRegions->pipelineCropRegion = activeArraySize;
            pCropRegions->ifeLimitCropRegion = activeArraySize;

            pMetadata->SetTag(MultiCamControllerManager::m_vendorTagCropRegions, pCropRegions,
                    sizeof(CaptureRequestCropRegions));

            // Update the scaler crop region
            camera_metadata_entry_t entryCropRegion = { 0 };
            if (0 == pMetadata->FindTag(ANDROID_SCALER_CROP_REGION, &entryCropRegion))
            {
                CHIRECT cropRect;
                if (m_camIdUltraWide == currentCameraId)
                {
                    cropRect = {zoomTotal.left, zoomTotal.top, zoomTotal.width, zoomTotal.height};
                }
                else
                {
                    cropRect = {0, 0, activeArraySize.width, activeArraySize.height};
                }
                pMetadata->SetTag(ANDROID_SCALER_CROP_REGION, &cropRect, 4);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MultiFovZoomRatioController::~MultiFovZoomRatioController
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MultiFovZoomRatioController::~MultiFovZoomRatioController()
{

}
