////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxzoomtranslator.h
/// @brief CHX Zoom translator class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHXZOOMTRANSLATOR_H
#define CHXZOOMTRANSLATOR_H

#include "chxmulticamcontroller.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

/// @brief Per camera Info for zoom translator
typedef struct
{
    UINT32          cameraId;                               ///< CameraId
    CalibrationData otpData[MaxDevicePerLogicalCamera];     ///< Calibration Data
    CHIRECT         ifeFOVRect;                             ///< IFE FOV rectangle
    CHIDIMENSION    sensorDimension;                        ///< Sensor Dimension
    CHIRECT         activeArraySize;                        ///< Active Array size
    FLOAT           focalLength;                            ///< Focal length
    FLOAT           adjustedFovRatio;                       ///< Adjusted FOV ratio wtr default camera
    FLOAT           pixelSize;                              ///< Pixel size
    UINT32          horizontalBinning;                      ///< Horizontal binning value
    UINT32          verticalBinning;                        ///< Vertical binning value
} ZoomCameraInfo;

/// @brief Initialization data for zoom translator
typedef struct
{
    CHIDIMENSION     previewDimension;                             ///< Preview dimension
    ZoomCameraInfo   linkedCameraInfo[MaxDevicePerLogicalCamera];  ///< Linked Camera session info
    UINT32           numLinkedSessions;                            ///< Number of enabled linked sessions
    UINT32           defaultAppCameraID;                           ///< Default Camera from the App's perspective
    FLOAT            maxZoom;                                      ///< Max supported user zoom
} ZoomTranslatorInitData;

/// @brief Translated zoom struct
typedef struct
{
    CHIRECT     totalZoom[MaxDevicePerLogicalCamera];  ///< Total zoom on each camera
    CHIRECT     ispZoom[MaxDevicePerLogicalCamera];    ///< ISP zoom to limit per camera
    UINT32      cameraID[MaxDevicePerLogicalCamera];   ///< Camera IDs associated with each zoom struct
} ZoomRegions;

/// @brief Translated zoom data for preview and snapshot
typedef struct
{
    ZoomRegions zoomPreview;             ///< Translated zoom for preview
    ZoomRegions zoomSnapshot;            ///< Translated zoom for snapshot
} TranslatedZoom;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief ZoomTranslator class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ZoomTranslator
{
public:

    /// Create the ZoomTranslator Object
    static ZoomTranslator* Create();

    /// Initialize ZoomTranslator object
    CDKResult Init(
        const ZoomTranslatorInitData* initData);

    /// Deinitialize ZoomTranslator object
    CDKResult Deinit();

    /// Get the translated zoom values
    CDKResult GetTranslatedZoom(
        const CHIRECTINT*   userZoom,
        TranslatedZoom*     translatedZoom);

   /// Get Translated Face Rect
    CDKResult GetTranslatedRect(
        const OutputMetadataOpticalZoom&   ozMeta,
        const CHIRECTINT&                  userZoom,
        const CHIRECTEXT&                  srcRect,
        CHIRECTEXT&                        dstRect);

    /// Get translated points
    CDKResult GetTranslatedPoints(
        const OutputMetadataOpticalZoom&   ozMeta,
        const CHIRECTINT&                  userZoom,
        const INT32*                       srcCoordinates,
        const UINT32                       numCoordinates,
        INT32*                             dstCoordinates);

    /// Destroy ZoomTranslator object
    VOID Destroy();

protected:

    ZoomTranslator();
    virtual ~ZoomTranslator();

private:

    /// Do not allow the copy constructor or assignment operator
    ZoomTranslator(const ZoomTranslator& rZoomTranslator) = delete;
    ZoomTranslator& operator= (const ZoomTranslator& rZoomTranslator) = delete;
    INT32 GetCameraIndex(UINT32 cameraID);

    VOID*            m_libHandle;                                    ///< Custom zoom translator library handle
    ZoomCameraInfo   m_linkedCameraInfo[MaxDevicePerLogicalCamera];  ///< Linked Camera session info
    UINT32           m_numLinkedSessions;                            ///< Number of enabled linked sessions
    UINT32           m_defaultAppCameraID;                           ///< Default Camera from the App's perspective
    FLOAT            m_maxZoom;                                      ///< Max supported user zoom
};

#endif // CHXZOOMTRANSLATOR_H
