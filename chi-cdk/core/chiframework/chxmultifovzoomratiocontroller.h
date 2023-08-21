////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxmultifovzoomratiocontroller.h
/// @brief chxmultifovcontroller related class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHXMULTIFOVZOOMRATIOCONTROLLER_H
#define CHXMULTIFOVZOOMRATIOCONTROLLER_H

#include "chxmulticamcontroller.h"
#include "chxzoomtranslator.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

class CDK_VISIBILITY_PUBLIC MultiFovZoomRatioController : public MultiFovController
{

public:
    /// Process the result metadata
    virtual VOID ProcessResultMetadata(
            ChiMetadata* pResultMetadata);

    /// Translate request settings
    virtual CDKResult TranslateRequestSettings(
            MulticamReqSettings* pMultiCamSettings);

    virtual ControllerResult GetResult(
            ChiMetadata* pMetadata, UINT32 snapshotActiveMask);

       /// Function to update the scaler crop for snapshot
    virtual VOID UpdateScalerCropForSnapshot(
        ChiMetadata* pMetadata);

    /// Translate face result metadata
    virtual VOID TranslateFaceRegions(
        VOID* pResultMetadata);

protected:
    FLOAT               m_zoomUser;                                 ///< User zoom value
    FLOAT               m_transitionWideToUltraWideLow;             ///< Variable to cache low transition value for UW
    FLOAT               m_transitionWideToUltraWideHigh;            ///< Variable to cache high transition value for UW
    UINT32              m_camIdxUltraWide;                          ///< Camera index of ultrawide camera
    UINT32              m_camIdxWide;                               ///< Camera index of wide camera
    UINT32              m_camIdUltraWide;                           ///< Camera id of ultrawide camera
    UINT32              m_camIdWide;                                ///< Camera id of wide camera

    virtual ~MultiFovZoomRatioController();
    MultiFovZoomRatioController() = default;

private:

    /// Translate metering region metadata
    WeightedRegion TranslateMeteringRegion(
            WeightedRegion* pRegion,
            UINT32          camId,
            FLOAT           appZoomRatio);

    /// Translate AE region metadata
    VOID TranslateAERegions(
            ChiMetadata* pMetadata,
            ChiMetadata* pTranslatedMetadata ,
            UINT32       primaryCameraId,
            UINT32       auxCameraId);

    /// Translate AF region metadata
    VOID TranslateAFRegions(
            ChiMetadata* pMetadata,
            ChiMetadata* pTranslatedMetadata,
            UINT32       primaryCameraId,
            UINT32       auxCameraId);

    /// Translate Zoom for ultrawide
    VOID TranslateZoomForUltraWide(
            FLOAT appZoomRatio,
            TranslatedZoom* pTranslatedZoom);

    /// Get custom app zoom ratio
    CHX_INLINE FLOAT  GetAppZoomRatio(
        ChiMetadata* pMetadata)
    {
        return GetAppZoomRatio(NULL, pMetadata);
    }

    /// Get custom app zoom ratio
    CHX_INLINE FLOAT  GetAppZoomRatio(
        camera_metadata_t* pMetadata)
    {
        return GetAppZoomRatio(pMetadata, NULL);
    }

    /// Get custom app zoom ratio
    CHX_INLINE FLOAT  GetAppZoomRatio(
        camera_metadata_t* pMetadata,
        ChiMetadata*       pChiMetadata)
    {
        FLOAT appZoomRatio = 1.0F;
        if(NULL == pMetadata && NULL == pChiMetadata)
        {
            CHX_LOG_ERROR("metadata is null");
        }
        else
        {
            MCCGetParamInput  queryList[1]   = {};
            MCCGetParamOutput outPutList[1]  = {};

            if(NULL != pMetadata)
            {
                  queryList[0].pInputData = static_cast<VOID*>(pChiMetadata);
                  queryList[0].type       = MCCGetParamTypePassChiMetadata;
            }
            else
            {
                  queryList[0].pInputData = static_cast<VOID*>(pMetadata);
                  queryList[0].type       = MCCGetParamTypePassCamera3Metadata;
            }

            MCCGetParamOutput pOutput = outPutList[0];
            pOutput.getParamOutputType = MCCGetParamTypeCustomAppZoom;
            PrepareAndGetParam(1, queryList, outPutList);

            VOID* outputData          =   pOutput.pGetParamOutput;
            if(NULL != outputData)
            {
                appZoomRatio    = *(static_cast<BOOL *>(outputData));
            }
        }
        return appZoomRatio;
    }

    /// Translate result metadata
    virtual VOID TranslateResultMetadata(
        camera_metadata_t* pResultMetadata);

    /// Check if SAT results need to be overridden
    VOID CheckOverrideMccResult(
            ChiMetadata* pMetadata);

    /// Translate request settings for UW camera
    CDKResult TranslateRequestSettingsForUltraWide(
            ChiMetadata* pMetadata, ChiMetadata* pTranslatedMetadata);

    /// Update results for initial frame
    VOID UpdateResultsForInitialFrame(
            ChiMetadata* pMetadata);

    /// Translate face region for uw camera
    VOID TranslateFaceRegionsUW(
            VOID*  pResultMetadata,
            UINT32 camId);

    /// Do not allow the copy constructor or assignment operator
    MultiFovZoomRatioController(const MultiFovZoomRatioController& rMultiCamController) = delete;
    MultiFovZoomRatioController& operator= (const MultiFovZoomRatioController& rMultiCamController) = delete;

};

#endif /* CHXMULTIFOVZOOMRATIOCONTROLLER_H */
