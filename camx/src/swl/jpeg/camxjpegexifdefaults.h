////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxjpegexifdefaults.h
/// @brief JPEG EXIF defaults
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXJPEGEXIFDEFAULTS_H
#define CAMXJPEGEXIFDEFAULTS_H

// NOWHINE ENTIRE FILE - legacy code
/// @todo (CAMX-1980) Clean up JPEG legacy code
/// @todo (CAMX-1988) Code clean up apply NC001 coding standard on entire project
/// @todo (CAMX-1989) Code clean up apply GR030 coding standard on entire project

#include "chistatspropertydefines.h"
#include "camxmem.h"
#include "camxutils.h"
#include "camxjpegexifdefs.h"

CAMX_NAMESPACE_BEGIN

static const UINT8   default_exif_version[] = { 0x30, 0x32, 0x32, 0x30 };
static const UINT8   default_flash_pix_version[] = { 0x30, 0x31, 0x30, 0x30 };
static const UINT8   default_r98_version[] = { 0x30, 0x31, 0x30, 0x30 };
static const UINT8   default_components_config[] = { 1, 2, 3, 0 };
static const UINT16  default_resolution_unit = 2;
static const UINT16  default_compression = 6; // 1 for uncompressed, 6 for Jpeg compression
static const UINT16  default_ycbcr_positioning = 1; // 1 for CENTER
static const UINT16  default_color_space = 1; // 1 for SRGB
static const URAT32  default_exif_resolution = { 0x48, 1 };
static const CHAR*   default_make = "QTI-AA";
static const CHAR*   default_model = "QCAM-AA";
static const CHAR*   default_datetime_original = "2002:12:08 12:00:00";
static const CHAR*   default_datetime_digitized = "2002:12:08 12:00:00";
static const CHAR*   default_interopindexstr = "R98";

static EXIFTagInfo default_tag_make =
{
    {
        EXIFTagType::EXIF_ASCII,    // type
        1,                          // copy
        7,                          // count
        { (CHAR*)default_make },    // data._ascii (initialization applies
                                    // to first member of the union)
    },                              // entry
    EXIFTAGID_MAKE,
};

static EXIFTagInfo default_tag_model =
{
    {
        EXIFTagType::EXIF_ASCII,    // type
        1,                          // copy
        8,                          // count
        { (CHAR*)default_model },   // data._ascii (initialization applies
                                    // to first member of the union)
    },                              // entry
    EXIFTAGID_MODEL
};

static EXIFTagInfo default_tag_datetime_original =
{
    {
        EXIFTagType::EXIF_ASCII,                // type
        1,                                      // copy
        20,                                     // count
        { (CHAR*)default_datetime_original },   // data._ascii (initialization applies
                                                // to first member of the union)
    },                                          // entry
    EXIFTAGID_EXIF_DATE_TIME_ORIGINAL
};

static EXIFTagInfo default_tag_datetime_digitized =
{
    {
        EXIFTagType::EXIF_ASCII,                // type
        1,                                      // copy
        20,                                     // count
        { (CHAR*)default_datetime_digitized },  // data._ascii (initialization applies
                                                // to first member of the union)
    },                                          // entry
    EXIFTAGID_EXIF_DATE_TIME_DIGITIZED
};

static EXIFTagInfo default_tag_interopindexstr =
{
    {
        EXIFTagType::EXIF_ASCII,                // type
        1,                                      // copy
        4,                                      // count
        { (CHAR*)default_interopindexstr },     // data._ascii (initialization applies
                                                // to first member of the union)
    },                                          // entry
    CONSTRUCT_TAGID(EXIF_TAG_MAX_OFFSET, 0x0001)
};

CAMX_NAMESPACE_END

#endif // CAMXJPEGEXIFDEFAULTS_H
