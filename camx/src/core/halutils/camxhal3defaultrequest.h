////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxhal3defaultrequest.h
/// @brief Declarations for HAL3DefaultRequest class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXHAL3DEFAULTREQUEST_H
#define CAMXHAL3DEFAULTREQUEST_H

#include "camxdefs.h"
#include "camxhal3metadatatags.h"
#include "camxhal3metadatatagtypes.h"
#include "camxhal3metadatautil.h"
#include "camxhal3module.h"
#include "camxhwenvironment.h"
#include "camxtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  HAL3DefaultRequest class that contains HAL API specific information
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class HAL3DefaultRequest
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInstance
    ///
    /// @brief  This function returns the singleton instance of the HAL3DefaultRequest.
    ///
    /// @param  cameraId        The id of the camera for which defaults are being fetched.
    ///
    /// @return A pointer to the singleton instance of the HAL3DefaultRequest
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static HAL3DefaultRequest* GetInstance(
        UINT32 cameraId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConstructDefaultRequestSettings
    ///
    /// @brief  This method is used by the application framework to create capture settings for standard camera use cases. A
    ///         settings buffer that is configured to meet the requested use case will be returned. This method is an
    ///         abstraction of the camera3_device_ops_t::construct_default_request_settings() API.
    ///
    /// @param  cameraId        The id of the camera for which defaults are being fetched.
    /// @param  requestTemplate The type of request to provide default settings for.
    ///
    /// @return Metadata pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static const Metadata* ConstructDefaultRequestSettings(
        UINT32                 cameraId,
        Camera3RequestTemplate requestTemplate);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDefaultRequest
    ///
    /// @brief  Get the default request setting base on request template type
    ///
    /// @param  pDefaultRequest The request to be filled in
    /// @param  requestTemplate Request template
    /// @param  pCameraInfo     Camera information and capabilities used to determine the proper request template
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GetDefaultRequest(
        DefaultRequest*              pDefaultRequest,
        const Camera3RequestTemplate requestTemplate,
        const HwCameraInfo*          pCameraInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDefaultColorCorrectionRequest
    ///
    /// @brief  Get the default color correction request setting base on request template type
    ///
    /// @param  pDefaultRequest The request to be filled in
    /// @param  requestTemplate Request template
    /// @param  pCameraInfo     Camera information and capabilities used to determine the proper request template
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GetDefaultColorCorrectionRequest(
        DefaultRequest*              pDefaultRequest,
        const Camera3RequestTemplate requestTemplate,
        const HwCameraInfo*          pCameraInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDefaultControlRequest
    ///
    /// @brief  Get the default control request setting base on request template type
    ///
    /// @param  pDefaultRequest The request to be filled in
    /// @param  requestTemplate Request template
    /// @param  pCameraInfo     Camera information and capabilities used to determine the proper request template
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GetDefaultControlRequest(
        DefaultRequest*              pDefaultRequest,
        const Camera3RequestTemplate requestTemplate,
        const HwCameraInfo*          pCameraInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDefaultEdgeRequest
    ///
    /// @brief  Get the default edge request setting base on request template type
    ///
    /// @param  pDefaultRequest The request to be filled in
    /// @param  requestTemplate Request template
    /// @param  pCameraInfo     Camera information and capabilities used to determine the proper request template
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GetDefaultEdgeRequest(
        DefaultRequest*              pDefaultRequest,
        const Camera3RequestTemplate requestTemplate,
        const HwCameraInfo*          pCameraInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDefaultFlashRequest
    ///
    /// @brief  Get the default flash request setting base on request template type
    ///
    /// @param  pDefaultRequest The request to be filled in
    /// @param  requestTemplate Request template
    /// @param  pCameraInfo     Camera information and capabilities used to determine the proper request template
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE VOID GetDefaultFlashRequest(
        DefaultRequest*              pDefaultRequest,
        const Camera3RequestTemplate requestTemplate,
        const HwCameraInfo*          pCameraInfo)
    {
        CAMX_UNREFERENCED_PARAM(requestTemplate);
        CAMX_UNREFERENCED_PARAM(pCameraInfo);
        CAMX_ASSERT(NULL != pDefaultRequest);

        pDefaultRequest->flashMode = FlashModeOff;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDefaultHotPixelRequest
    ///
    /// @brief  Get the default flash request setting base on request template type
    ///
    /// @param  pDefaultRequest The request to be filled in
    /// @param  requestTemplate Request template
    /// @param  pCameraInfo     Camera information and capabilities used to determine the proper request template
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE VOID GetDefaultHotPixelRequest(
        DefaultRequest*              pDefaultRequest,
        const Camera3RequestTemplate requestTemplate,
        const HwCameraInfo*          pCameraInfo)
    {
        CAMX_UNREFERENCED_PARAM(requestTemplate);
        CAMX_UNREFERENCED_PARAM(pCameraInfo);
        CAMX_ASSERT(NULL != pDefaultRequest);
        if (requestTemplate == RequestTemplateStillCapture)
        {
            pDefaultRequest->hotPixelMode = HotPixelModeHighQuality;
        }
        else
        {
            pDefaultRequest->hotPixelMode = HotPixelModeFast;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDefaultLensRequest
    ///
    /// @brief  Get the default lens request setting base on request template type
    ///
    /// @param  pDefaultRequest The request to be filled in
    /// @param  requestTemplate Request template
    /// @param  pCameraInfo     Camera information and capabilities used to determine the proper request template
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GetDefaultLensRequest(
        DefaultRequest*              pDefaultRequest,
        const Camera3RequestTemplate requestTemplate,
        const HwCameraInfo*          pCameraInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDefaultNoiseReductionRequest
    ///
    /// @brief  Get the default noise reduction request setting base on request template type
    ///
    /// @param  pDefaultRequest The request to be filled in
    /// @param  requestTemplate Request template
    /// @param  pCameraInfo     Camera information and capabilities used to determine the proper request template
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GetDefaultNoiseReductionRequest(
        DefaultRequest*              pDefaultRequest,
        const Camera3RequestTemplate requestTemplate,
        const HwCameraInfo*          pCameraInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDefaultRequestId
    ///
    /// @brief  Get the default request id setting base on request template type
    ///
    /// @param  pDefaultRequest The request to be filled in
    /// @param  requestTemplate Request template
    /// @param  pCameraInfo     Camera information and capabilities used to determine the proper request template
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE VOID GetDefaultRequestId(
        DefaultRequest*              pDefaultRequest,
        const Camera3RequestTemplate requestTemplate,
        const HwCameraInfo*          pCameraInfo)
    {
        CAMX_UNREFERENCED_PARAM(requestTemplate);
        CAMX_UNREFERENCED_PARAM(pCameraInfo);
        CAMX_ASSERT(NULL != pDefaultRequest);

        pDefaultRequest->requestId = 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDefaultScalerRequest
    ///
    /// @brief  Get the default scaler request setting base on request template type
    ///
    /// @param  pDefaultRequest The request to be filled in
    /// @param  requestTemplate Request template
    /// @param  pCameraInfo     Camera information and capabilities used to determine the proper request template
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE VOID GetDefaultScalerRequest(
        DefaultRequest*              pDefaultRequest,
        const Camera3RequestTemplate requestTemplate,
        const HwCameraInfo*          pCameraInfo)
    {
        CAMX_UNREFERENCED_PARAM(requestTemplate);
        CAMX_ASSERT(NULL != pDefaultRequest);

        Region                activeArraySize = pCameraInfo->pSensorCaps->activeArraySize;
        const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

        if ((TRUE  == pCameraInfo->pSensorCaps->isQuadCFASensor) &&
            (FALSE == pStaticSettings->exposeFullSizeForQCFA))
        {
            activeArraySize = pCameraInfo->pSensorCaps->QuadCFAActiveArraySize;
        }

        pDefaultRequest->scalerCropRegion.xMin      = 0;
        pDefaultRequest->scalerCropRegion.yMin      = 0;
        pDefaultRequest->scalerCropRegion.width     = activeArraySize.width;
        pDefaultRequest->scalerCropRegion.height    = activeArraySize.height;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDefaultSensorRequest
    ///
    /// @brief  Get the default sensor request setting base on request template type
    ///
    /// @param  pDefaultRequest The request to be filled in
    /// @param  requestTemplate Request template
    /// @param  pCameraInfo     Camera information and capabilities used to determine the proper request template
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GetDefaultSensorRequest(
        DefaultRequest*              pDefaultRequest,
        const Camera3RequestTemplate requestTemplate,
        const HwCameraInfo*          pCameraInfo)
    {
        CAMX_UNREFERENCED_PARAM(requestTemplate);
        CAMX_UNREFERENCED_PARAM(pCameraInfo);
        CAMX_ASSERT(NULL != pDefaultRequest);

        pDefaultRequest->sensorExposureTime     = pCameraInfo->pSensorCaps->minExposureTime;
        pDefaultRequest->sensorFrameDuration    = pCameraInfo->pSensorCaps->maxFrameDuration;
        pDefaultRequest->sensorSensitivity      = pCameraInfo->pSensorCaps->minISOSensitivity;
        pDefaultRequest->sensorTestPatternMode  = SensorTestPatternModeOff;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDefaultShadingRequest
    ///
    /// @brief  Get the default shading request setting base on request template type
    ///
    /// @param  pDefaultRequest The request to be filled in
    /// @param  requestTemplate Request template
    /// @param  pCameraInfo     Camera information and capabilities used to determine the proper request template
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GetDefaultShadingRequest(
        DefaultRequest*              pDefaultRequest,
        const Camera3RequestTemplate requestTemplate,
        const HwCameraInfo*          pCameraInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDefaultStatisticsRequest
    ///
    /// @brief  Get the default statistics request setting base on request template type
    ///
    /// @param  pDefaultRequest The request to be filled in
    /// @param  requestTemplate Request template
    /// @param  pCameraInfo     Camera information and capabilities used to determine the proper request template
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE VOID GetDefaultStatisticsRequest(
        DefaultRequest*              pDefaultRequest,
        const Camera3RequestTemplate requestTemplate,
        const HwCameraInfo*          pCameraInfo)
    {
        CAMX_UNREFERENCED_PARAM(requestTemplate);
        CAMX_UNREFERENCED_PARAM(pCameraInfo);
        CAMX_ASSERT(NULL != pDefaultRequest);

        pDefaultRequest->faceDetectMode     = StatisticsFaceDetectModeOff;
        pDefaultRequest->hotPixelMapMode    = StatisticsHotPixelMapModeOff;
        if (requestTemplate == RequestTemplateStillCapture)
        {
            pDefaultRequest->lensShadingMapMode = StatisticsLensShadingMapModeOn;
        }
        else
        {
            pDefaultRequest->lensShadingMapMode = StatisticsLensShadingMapModeOff;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDefaultTonemapRequest
    ///
    /// @brief  Get the default tonemap request setting base on request template type
    ///
    /// @param  pDefaultRequest The request to be filled in
    /// @param  requestTemplate Request template
    /// @param  pCameraInfo     Camera information and capabilities used to determine the proper request template
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GetDefaultTonemapRequest(
        DefaultRequest*              pDefaultRequest,
        const Camera3RequestTemplate requestTemplate,
        const HwCameraInfo*          pCameraInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDefaultBlackLevelLockRequest
    ///
    /// @brief  Get the default black level lock request setting base on request template type
    ///
    /// @param  pDefaultRequest The request to be filled in
    /// @param  requestTemplate Request template
    /// @param  pCameraInfo     Camera information and capabilities used to determine the proper request template
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE VOID GetDefaultBlackLevelLockRequest(
        DefaultRequest*              pDefaultRequest,
        const Camera3RequestTemplate requestTemplate,
        const HwCameraInfo*          pCameraInfo)
    {
        CAMX_UNREFERENCED_PARAM(requestTemplate);
        CAMX_UNREFERENCED_PARAM(pCameraInfo);
        CAMX_ASSERT(NULL != pDefaultRequest);

        pDefaultRequest->blackLevelLock = BlackLevelLockOff;
    }

    // Do not implement the copy constructor or assignment operator
    HAL3DefaultRequest(const HAL3DefaultRequest& rHAL3DefaultRequest)             = delete;
    HAL3DefaultRequest& operator= (const HAL3DefaultRequest& rHAL3DefaultRequest) = delete;

    HAL3DefaultRequest() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~HAL3DefaultRequest
    ///
    /// @brief  Default destructor for the HAL3DefaultRequest class
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~HAL3DefaultRequest();

    Metadata* m_pMetadataTemplates[RequestTemplateCount];       ///< A list of templates for ConstructDefaultRequestSettings
};

CAMX_NAMESPACE_END

#endif // CAMXHAL3DEFAULTREQUEST_H
