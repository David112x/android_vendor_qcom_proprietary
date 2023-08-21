////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifedspinterface.cpp
/// @brief ife hvx implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifedspinterface.h"
#include "camxincs.h"


CAMX_NAMESPACE_BEGIN

remote_handle64 IFEDSPInterface::s_hFastRpcHandle = 0;
struct HVXStubSetCallbackFunc IFEDSPInterface::s_callback = { 0, 0 };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSPInterface::DSPOpen
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSPInterface::DSPOpen(
    INT*             phDSPHandle,
    HVXStubOpen*     pHvxOpen)
{
    CamxResult result = CamxResultSuccess;
    INT        HVXResult;
    hvx_open_t hvxOpen;
    VOID*      pData;

    if (NULL != pHvxOpen)
    {
        OsUtils::StrLCpy(hvxOpen.name, pHvxOpen->name, sizeof(pHvxOpen->name));

        hvxOpen.bus_clock   = pHvxOpen->busClock;
        hvxOpen.dsp_clock   = pHvxOpen->dspClock;
        hvxOpen.dsp_latency = pHvxOpen->dspLatency;
        hvxOpen.IFE_mode    = static_cast<hvx_IFE_mode_t>(pHvxOpen->IFEType);

        pData     = static_cast<VOID *>(&hvxOpen);
        HVXResult = dsp_streamer_event(s_hFastRpcHandle,
                                       phDSPHandle,
                                       HVX_EVENT_OPEN,
                                       static_cast<CHAR *>(pData),
                                       sizeof(hvx_open_t));

        if (HVX_SUCCESS != HVXResult)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupISP, "HVX_EVENT_OPEN failed result %d", HVXResult);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSPInterface::DSPClose
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSPInterface::DSPClose(
    INT* phDSPHandle)
{
    CamxResult result = CamxResultSuccess;
    INT        HVXResult;

    HVXResult = dsp_streamer_event(s_hFastRpcHandle, phDSPHandle, HVX_EVENT_CLOSE, NULL, 0);
    if (HVX_SUCCESS != HVXResult)
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupISP, "HVX_EVENT_CLOSE failed result %d", HVXResult);
    }

    dsp_streamer_close(s_hFastRpcHandle);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSPInterface::DSPReset
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSPInterface::DSPReset(
    INT* phDSPHandle)
{
    CamxResult result = CamxResultSuccess;
    INT        HVXResult;

    HVXResult = dsp_streamer_event(s_hFastRpcHandle, phDSPHandle, HVX_EVENT_RESET, NULL, 0);
    if (HVX_SUCCESS != HVXResult)
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupISP, "HVX_EVENT_RESET failed result %d", HVXResult);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSPInterface::DSPStart
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSPInterface::DSPStart(
    INT* phDSPHandle)
{
    CamxResult result = CamxResultSuccess;
    INT        HVXResult;

    HVXResult = dsp_streamer_event(s_hFastRpcHandle, phDSPHandle, HVX_EVENT_START, NULL, 0);
    if (HVX_SUCCESS != HVXResult)
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupISP, "HVX_EVENT_START failed result %d", HVXResult);
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSPInterface::DSPStop
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSPInterface::DSPStop(
    INT* phDSPHandle)
{
    CamxResult result = CamxResultSuccess;
    INT  HVXResult;

    HVXResult = dsp_streamer_event(s_hFastRpcHandle, phDSPHandle, HVX_EVENT_STOP, NULL, 0);
    if (HVX_SUCCESS != HVXResult)
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupISP, "HVX_EVENT_STOP failed result %d", HVXResult);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSPInterface::QueryCapabilities
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSPInterface::QueryCapabilities(
    INT               handle,
    HVXStubQueryCaps* pHvxQueryCaps)
{
    CamxResult       result = CamxResultSuccess;
    INT              HVXResult;
    hvx_query_caps_t queryCaps;
    VOID*            pData;
    const CHAR*      pUri = dsp_streamer_URI "&_dom=cdsp";

    Utils::Memset(&queryCaps, 0, sizeof(queryCaps));

    HVXResult = dsp_streamer_open(pUri, &s_hFastRpcHandle);
    if (0 != HVXResult)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "failed %llx", s_hFastRpcHandle);
        result = CamxResultEFailed;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "s_hFastRpcHandle %llx", s_hFastRpcHandle);

    if ((CamxResultSuccess == result) && (NULL != pHvxQueryCaps))
    {
        pData     = static_cast<VOID*>(&queryCaps);
        HVXResult = dsp_streamer_event(s_hFastRpcHandle,
                                       &handle,
                                       HVX_EVENT_QUERY_CAPS,
                                       static_cast<CHAR*>(pData),
                                       sizeof(hvx_query_caps_t));

        pHvxQueryCaps->hvxStubVectorMode = static_cast<HVXStubVectorMode>(queryCaps.hvx_vector_mode);
        pHvxQueryCaps->maxHvxUnit        = queryCaps.max_hvx_unit;

        CAMX_LOG_INFO(CamxLogGroupISP, "hvx_vector_mode %d maxHvxUnit %d ",
            pHvxQueryCaps->hvxStubVectorMode, pHvxQueryCaps->maxHvxUnit);

        if (HVX_SUCCESS != HVXResult)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupISP, "HVX_EVENT_QUERY_CAPS failed result %d", HVXResult);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSPInterface::StaticConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSPInterface::StaticConfig(
    INT*                 phDSPHandle,
    HVXStubStaticConfig* pStaticConfig)
{
    CamxResult result = CamxResultSuccess;
    INT        HVXResult;
    hvx_static_config_t dspstaticconfig;
    VOID* pData;

    if (NULL != pStaticConfig)
    {
        dspstaticconfig.bits_per_pixel     = pStaticConfig->bitsPerPixel;
        dspstaticconfig.tapping_point      = static_cast<tapping_point_select_t>(pStaticConfig->tappingPoint);
        dspstaticconfig.in_width           = pStaticConfig->width;
        dspstaticconfig.in_height          = pStaticConfig->height;
        dspstaticconfig.out_width          = pStaticConfig->OutWidth;
        dspstaticconfig.out_height         = pStaticConfig->OutHeight;
        dspstaticconfig.hvx_unit_no[0]     = pStaticConfig->hvxUnitNo[0];
        dspstaticconfig.hvx_unit_no[1]     = pStaticConfig->hvxUnitNo[1];
        dspstaticconfig.hvx_vector_mode    = static_cast<hvx_vector_mode_t>(pStaticConfig->hvxStubVectorMode);
        dspstaticconfig.IFE_mode           = static_cast<hvx_IFE_mode_t> (pStaticConfig->IFEid);
        dspstaticconfig.buf_request_mode   = static_cast<buf_request_mode_t>(pStaticConfig->dataMode);
        dspstaticconfig.image_overlap      = pStaticConfig->imageOverlap;
        dspstaticconfig.right_image_offset = pStaticConfig->rightImageOffset;
        dspstaticconfig.pixel_format       = static_cast<hvx_pixel_format_t>(pStaticConfig->pixelFormat);

        Utils::Memcpy(static_cast<VOID*>(dspstaticconfig.req_buf),
                      static_cast<VOID*>(pStaticConfig->data),
                      sizeof(request_buffer_t) * 2);

        pData = static_cast<VOID *>(&dspstaticconfig);

        HVXResult = dsp_streamer_event(s_hFastRpcHandle, phDSPHandle,
                                       HVX_EVENT_STATIC_CONFIG,
                                       static_cast<CHAR *>(pData),
                                       sizeof(hvx_static_config_t));
        if (HVX_SUCCESS != HVXResult)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupISP, "HVX_EVENT_STATIC_CONFIG failed result %d", HVXResult);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSPInterface::DynamicConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSPInterface::DynamicConfig(
    INT*  phDSPHandle,
    VOID* pData,
    UINT  dataLength)
{
    CamxResult result = CamxResultSuccess;
    INT        HVXResult;

    if (NULL != pData)
    {
        HVXResult = dsp_streamer_event(s_hFastRpcHandle,
                                       phDSPHandle,
                                       HVX_EVENT_DYNAMIC_CONFIG,
                                       static_cast<CHAR *>(pData),
                                       dataLength);

        if (HVX_SUCCESS != HVXResult)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupISP, "HVX_EVENT_DYNAMIC_CONFIG failed result %d", HVXResult);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSPInterface::SetCallbackFunctions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSPInterface::SetCallbackFunctions(
    VOID* pData)
{
    HVXStubSetCallbackFunc* pSetCallBack = NULL;
    CamxResult              result = CamxResultSuccess;

    if (NULL != pData)
    {
        pSetCallBack = static_cast<HVXStubSetCallbackFunc*>(pData);
        s_callback   = *pSetCallBack;
    }
    else
    {
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSPInterface::IFEDSPInterface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDSPInterface::IFEDSPInterface()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSPInterface::~IFEDSPInterface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDSPInterface::~IFEDSPInterface()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSPInterface::HVXStubCallbackData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSPInterface::HVXStubCallbackData(
    INT    handle,
    INT    vfeType,
    UCHAR  bufLabel)
{
    CamxResult result = CamxResultSuccess;
    HVXIFEType stubVfeType;

    stubVfeType = static_cast<HVXIFEType>(vfeType);
    if (NULL != s_callback.callbackdata)
    {
        s_callback.callbackdata(handle, stubVfeType, bufLabel);
    }
    else
    {
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSPInterface::HVXStubCallbackError
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSPInterface::HVXStubCallbackError(
    INT         handle,
    UINT32      vfeType,
    const CHAR* pErrorMsg,
    INT         errorMsgLen,
    INT         frameId)
{
    CamxResult result = CamxResultSuccess;
    HVXStubVFEType stubVfeType;

    stubVfeType = static_cast<HVXStubVFEType>(vfeType);
    if (NULL != s_callback.ErrorCallback)
    {
        s_callback.ErrorCallback(handle,
                                 stubVfeType,
                                 pErrorMsg,
                                 errorMsgLen,
                                 frameId);
    }
    else
    {
        result = CamxResultEFailed;
    }

    return result;

}


#ifdef __cplusplus
extern "C" {
#endif // _cplusplus
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// dsp_streamer_callback_data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT dsp_streamer_callback_data(
    INT handle,
    INT ifeId,
    INT bufLabel)
{
    CamxResult result = CamxResultSuccess;
    UCHAR      label;

    if (handle < 0 || !bufLabel)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "invalid handle %d bufLabel %d ", handle, bufLabel);
        result = CamxResultEFailed;
    }
    if (CamxResultSuccess == result)
    {
        label  = static_cast<UCHAR>(bufLabel);
        result = IFEDSPInterface::HVXStubCallbackData(handle, ifeId, label);
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// dsp_streamer_callback_error
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT dsp_streamer_callback_error(
    INT handle,
    INT ife_id,
    INT frame_id,
    hvx_cb_error_type_t error,
    const CHAR* pErrorMsg,
    INT error_msgLen)
{
    CAMX_LOG_ERROR(CamxLogGroupISP, "handle %d vfe %d errorType %d msg %s error_msgLen %d frame_id %d \n",  handle,
        ife_id, error, pErrorMsg, error_msgLen, frame_id);
    return 0;
}
#ifdef __cplusplus
}
#endif  // _cplusplus

CAMX_NAMESPACE_END
