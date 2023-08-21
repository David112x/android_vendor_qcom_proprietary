////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxzoomtranslator.cpp
/// @brief  CHX Zoom translator class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

#include "chxzoomtranslator.h"
#include <dlfcn.h>

#undef LOG_TAG
#define LOG_TAG "CHIZOOMTRANSLATOR"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ZoomTranslator::ZoomTranslator
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ZoomTranslator::ZoomTranslator()
    : m_libHandle(NULL)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ZoomTranslator::~ZoomTranslator
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ZoomTranslator::~ZoomTranslator()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ZoomTranslator::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ZoomTranslator::Destroy()
{
    // dlclose the lib here. Do not dlclose in Deinit
    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ZoomTranslator::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ZoomTranslator* ZoomTranslator::Create()
{
    ZoomTranslator* pZoomTranslator = CHX_NEW ZoomTranslator;

    if (NULL != pZoomTranslator)
    {
        // dlopen and dlsym here. Set m_libHandle.
    }

    return pZoomTranslator;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ZoomTranslator::Init
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ZoomTranslator::Init(
    const ZoomTranslatorInitData* initData)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == m_libHandle)
    {
        m_numLinkedSessions    = initData->numLinkedSessions;
        m_defaultAppCameraID   = initData->defaultAppCameraID;
        m_maxZoom              = initData->maxZoom;

        // Default implementation for zoom translation
        for (UINT32 j = 0; j < m_numLinkedSessions; j++)
        {
            m_linkedCameraInfo[j] = initData->linkedCameraInfo[j];
        }
        CHX_LOG("Using the default zoom translation implementation");
    }
    else
    {
        /* Use initData to initialize the ZoomTranslator object.
        If the initialization fails set the result code to CDKResultEFailed */
        CHX_LOG("Using the customized zoom translation implementation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ZoomTranslator::Deinit
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ZoomTranslator::Deinit()
{
    CDKResult result = CDKResultSuccess;

    /* If the De-initialization fails set the result code to CDKResultEFailed */

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ZoomTranslator::GetCameraIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 ZoomTranslator::GetCameraIndex(
    UINT32 cameraID)
{
    INT32 index = -1;

    for (UINT32 i = 0; i < m_numLinkedSessions; i++)
    {
        if (cameraID == m_linkedCameraInfo[i].cameraId)
        {
            index = i;
            break;
        }
    }

    return index;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ZoomTranslator::GetTranslatedZoom
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ZoomTranslator::GetTranslatedZoom(
    const CHIRECTINT*  userZoom,
    TranslatedZoom*    translatedZoom)
{
    CDKResult result        = CDKResultSuccess;

    if (NULL == m_libHandle)
    {
        UINT32 defaultCamIdx = GetCameraIndex(m_defaultAppCameraID);
        FLOAT  scaleWidthDef  = 0.0F;
        FLOAT  scaleHeightDef = 0.0F;
        FLOAT  zoom           = 0.0F;
        if ((0 < userZoom->width) && (0 < userZoom->height))
        {
            scaleWidthDef = m_linkedCameraInfo[defaultCamIdx].activeArraySize.width /
                (FLOAT)userZoom->width;
            scaleHeightDef = m_linkedCameraInfo[defaultCamIdx].activeArraySize.height /
                (FLOAT)userZoom->height;
            zoom = scaleWidthDef > scaleHeightDef ? scaleWidthDef : scaleHeightDef;

            if (m_maxZoom < zoom)
            {
                zoom = m_maxZoom;
            }
        }
        else
        {
            CHX_LOG_ERROR("Invalid User zoom Crop with/height. Cannot Translate zoom");
            result = CDKResultEFailed;
        }

        // Default implementation for zoom translation

        if (m_numLinkedSessions <= MaxDevicePerLogicalCamera)
        {
            for (UINT32 idx = 0; idx < m_numLinkedSessions; idx++)
            {
                 if (defaultCamIdx == idx)
                 {
                     CHX_LOG("Default Camera id %d, index %d", m_linkedCameraInfo[idx].cameraId, idx);
                      //Default camera has the same crop window as user zoom
                      CHIRECT* pUserZoom = reinterpret_cast<CHIRECT*>(const_cast<CHIRECTINT*>(userZoom));
                      translatedZoom->zoomPreview.totalZoom[idx]   = *pUserZoom;
                 }
                 else
                 {
                     //Get range of FOV ratios wrt default camera
                     FLOAT minFovRatio = 0.0F;
                     FLOAT maxFovRatio = m_maxZoom;
                     if (idx < m_numLinkedSessions - 1)
                     {
                         maxFovRatio = m_linkedCameraInfo[idx + 1].adjustedFovRatio;
                     }
                     minFovRatio = m_linkedCameraInfo[idx].adjustedFovRatio;

                     /// If the adjustedFOVRatio is the same as primary camera, no translation is required
                     if (m_linkedCameraInfo[idx].adjustedFovRatio == m_linkedCameraInfo[defaultCamIdx].adjustedFovRatio)
                     {
                         CHIRECT* pUserZoom = reinterpret_cast<CHIRECT*>(const_cast<CHIRECTINT*>(userZoom));
                         translatedZoom->zoomPreview.totalZoom[idx] = *pUserZoom;
                     }

                     /// If current zoom is beyon the range of the current camera, no need to translate
                     if (zoom < minFovRatio)
                     {
                         translatedZoom->zoomPreview.totalZoom[idx].top   = 0;
                         translatedZoom->zoomPreview.totalZoom[idx].left  = 0;
                         translatedZoom->zoomPreview.totalZoom[idx].width =
                             m_linkedCameraInfo[idx].activeArraySize.width;
                         translatedZoom->zoomPreview.totalZoom[idx].height =
                             m_linkedCameraInfo[idx].activeArraySize.height;
                     }
                     else
                     {
                         FLOAT scaleWidth  = 0.0F;
                         FLOAT scaleHeight = 0.0F;
                         if (0.0 < minFovRatio)
                         {
                             scaleWidth = scaleWidthDef / minFovRatio;
                             scaleHeight = scaleHeightDef / minFovRatio;
                         }

                         translatedZoom->zoomPreview.totalZoom[idx].width =
                             m_linkedCameraInfo[idx].activeArraySize.width / scaleWidth;
                         translatedZoom->zoomPreview.totalZoom[idx].height =
                             m_linkedCameraInfo[idx].activeArraySize.height / scaleHeight;
                         translatedZoom->zoomPreview.totalZoom[idx].left =
                             (m_linkedCameraInfo[idx].activeArraySize.width -
                              translatedZoom->zoomPreview.totalZoom[idx].width) / 2.0f;
                         translatedZoom->zoomPreview.totalZoom[idx].top =
                             (m_linkedCameraInfo[idx].activeArraySize.height -
                              translatedZoom->zoomPreview.totalZoom[idx].height) / 2.0f;
                    }
                }
                translatedZoom->zoomPreview.ispZoom[idx] =
                     translatedZoom->zoomPreview.totalZoom[idx];
                translatedZoom->zoomPreview.cameraID[idx] =
                    m_linkedCameraInfo[idx].cameraId;
                translatedZoom->zoomSnapshot = translatedZoom->zoomPreview;
            }
        }
    }
    else
    {
        /* Implement the zoom translation logic.
           Set result to CDKResultEFailed in case of error*/
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ZoomTranslator::GetTranslatedRect
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ZoomTranslator::GetTranslatedRect(
    const OutputMetadataOpticalZoom& ozMeta,
    const CHIRECTINT&                userZoom,
    const CHIRECTEXT&                srcRect,
    CHIRECTEXT&                      dstRect)
{
    CDKResult result        = CDKResultSuccess;

    if (NULL == m_libHandle)
    {
        (VOID) userZoom;
         // Acquire mutex to read spatial alignment shifts which are written by other thread
        UINT32     cameraId   = ozMeta.masterCameraId;
        UINT32     camIdx     = GetCameraIndex(cameraId);
        UINT32     primaryIdx = GetCameraIndex(m_defaultAppCameraID);
        PixelShift pixelShift = {ozMeta.outputShiftPreview.horizonalShift,
                                 ozMeta.outputShiftPreview.verticalShift};

        INT32 xMin = srcRect.left;
        INT32 yMin = srcRect.top;
        INT32 xMax = srcRect.right;
        INT32 yMax = srcRect.bottom;

        if (m_defaultAppCameraID == cameraId)
        {
            xMin -= pixelShift.xShift;
            yMin -= pixelShift.yShift;
            xMax -= pixelShift.xShift;
            yMax -= pixelShift.yShift;

            CHX_LOG("Face rectangle on primay: l_t_r_b_(%d, %d, %d, %d), shift(%d, %d).",
                    xMin, yMin, xMax, yMax,
                    pixelShift.xShift, pixelShift.yShift);
        }
        else
        {
            FLOAT xScale = (FLOAT)m_linkedCameraInfo[primaryIdx].activeArraySize.width /
                m_linkedCameraInfo[camIdx].activeArraySize.width;
            FLOAT yScale = (FLOAT)m_linkedCameraInfo[primaryIdx].activeArraySize.height /
                m_linkedCameraInfo[camIdx].activeArraySize.height;

            xMin *= xScale;
            yMin *= yScale;
            xMax *= xScale;
            yMax *= yScale;

            FLOAT fImageRatio = 1.0f / m_linkedCameraInfo[camIdx].adjustedFovRatio;

            pixelShift.xShift *= (xScale * fImageRatio);
            pixelShift.yShift *= (yScale * fImageRatio);
            xMin -= pixelShift.xShift;
            yMin -= pixelShift.yShift;
            xMax -= pixelShift.xShift;
            yMax -= pixelShift.yShift;

            xMin *= fImageRatio;
            yMin *= fImageRatio;
            xMax *= fImageRatio;
            yMax *= fImageRatio;

            UINT32 alignmentWidth = (m_linkedCameraInfo[primaryIdx].activeArraySize.width -
                (m_linkedCameraInfo[camIdx].activeArraySize.width * xScale * fImageRatio)) / 2.0f;
            UINT32 alignmentHeight = (m_linkedCameraInfo[primaryIdx].activeArraySize.height -
                (m_linkedCameraInfo[camIdx].activeArraySize.height * yScale * fImageRatio)) / 2.0f;
            xMin += static_cast<INT32>(alignmentWidth);
            xMax += static_cast<INT32>(alignmentWidth);
            yMin += static_cast<INT32>(alignmentHeight);
            yMax += static_cast<INT32>(alignmentHeight);

            CHX_LOG("Face rectangle on tele: l_t_r_b_(%d, %d, %d, %d), scale(%f, %f), shift(%d, %d),"
                    " fImageRatio = %f .",
                    xMin, yMin, xMax, yMax,
                    xScale, yScale,
                    pixelShift.xShift, pixelShift.yShift,
                    fImageRatio);
        }

        dstRect.left   = xMin;
        dstRect.top    = yMin;
        dstRect.right  = xMax;
        dstRect.bottom = yMax;
    }
    else
    {
        //Implement Face translation logic
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ZoomTranslator::GetTranslatedPoints
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ZoomTranslator::GetTranslatedPoints(
    const OutputMetadataOpticalZoom&   ozMeta,
    const CHIRECTINT&                  userZoom,
    const INT32*                       srcCoordinates,
    const UINT32                       numCoordinates,
    INT32*                             dstCoordinates)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == m_libHandle)
    {
        (VOID) userZoom;
         // Acquire mutex to read spatial alignment shifts which are written by other thread
        UINT32     cameraId   = ozMeta.masterCameraId;
        UINT32     camIdx     = GetCameraIndex(cameraId);
        UINT32     primaryIdx = GetCameraIndex(m_defaultAppCameraID);
        PixelShift pixelShift = {ozMeta.outputShiftPreview.horizonalShift,
                                 ozMeta.outputShiftPreview.verticalShift};

        for (UINT32 i = 0; i < numCoordinates / 2; i++)
        {
            UINT32 xConverted = srcCoordinates[i * 2];
            UINT32 yConverted = srcCoordinates[(i * 2) + 1];

            if (m_defaultAppCameraID == cameraId)
            {
                xConverted -= pixelShift.xShift;
                yConverted -= pixelShift.yShift;

            }
            else
            {
                FLOAT xScale = (FLOAT)m_linkedCameraInfo[primaryIdx].activeArraySize.width /
                    m_linkedCameraInfo[camIdx].activeArraySize.width;
                FLOAT yScale = (FLOAT)m_linkedCameraInfo[primaryIdx].activeArraySize.height /
                    m_linkedCameraInfo[camIdx].activeArraySize.height;

                xConverted *= xScale;
                yConverted *= yScale;

                FLOAT fImageRatio = 1.0f / m_linkedCameraInfo[camIdx].adjustedFovRatio;

                pixelShift.xShift *= (xScale * fImageRatio);
                pixelShift.yShift *= (yScale * fImageRatio);
                xConverted -= pixelShift.xShift;
                yConverted -= pixelShift.yShift;

                xConverted *= fImageRatio;
                yConverted *= fImageRatio;

                UINT32 alignmentWidth = (m_linkedCameraInfo[primaryIdx].activeArraySize.width -
                    (m_linkedCameraInfo[camIdx].activeArraySize.width * xScale * fImageRatio)) / 2.0f;
                UINT32 alignmentHeight = (m_linkedCameraInfo[primaryIdx].activeArraySize.height -
                    (m_linkedCameraInfo[camIdx].activeArraySize.height * yScale * fImageRatio)) / 2.0f;
                xConverted += alignmentWidth;
                yConverted += alignmentHeight;
            }

            dstCoordinates[i * 2]     = xConverted;
            dstCoordinates[(i * 2) + 1] = yConverted;
        }
    }
    else
    {
        //Implement Face translation logic
    }

    return result;
}
