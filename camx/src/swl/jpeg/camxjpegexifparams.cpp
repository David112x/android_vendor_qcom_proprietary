////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxjpegexifparams.cpp
/// @brief EXIF params class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE ENTIRE FILE - legacy code
/// @todo (CAMX-1980) Clean up JPEG legacy code
/// @todo (CAMX-1988) Code clean up apply NC001 coding standard on entire project
/// @todo (CAMX-1989) Code clean up apply GR030 coding standard on entire project

#include "camxtypes.h"
#include "camxosutils.h"
#include "camxpropertyblob.h"
#include "camxvendortags.h"
#include "camxhwenvironment.h"
#include "camxsettingsmanager.h"
#include "camximagebuffer.h"
#include "camximagebuffermanager.h"
#include "camxnode.h"
#include "camxjpegexifdefaults.h"
#include "camxjpegexifparams.h"

CAMX_NAMESPACE_BEGIN

static EXIFTagInfo default_tag_exif_version =
{
    {
        EXIFTagType::EXIF_UNDEFINED,            // type
        1,                                      // copy
        4,                                      // count
        { (char*)default_exif_version },        // data._ascii (initialization applies
                                                // to first member of the union)
    },                                          // entry
    EXIFTAGID_EXIF_VERSION
};

static EXIFTagInfo default_tag_flash_pix_version =
{
    {
        EXIFTagType::EXIF_UNDEFINED,            // type
        1,                                      // copy
        4,                                      // count
        { (char*)default_flash_pix_version },   // data._ascii (initialization applies
                                                // to first member of the union)
    },                                          // entry
    EXIFTAGID_EXIF_FLASHPIX_VERSION
};

static EXIFTagInfo default_tag_components_config =
{
    {
        EXIFTagType::EXIF_UNDEFINED,            // type
        1,                                      // copy
        4,                                      // count
        { (char*)default_components_config },   // data._ascii (initialization applies
                                                // to first member of the union)
    },                                          // entry
    EXIFTAGID_EXIF_COMPONENTS_CONFIG
};

static EXIFTagInfo default_tag_resolution_unit =
{
    {
        EXIFTagType::EXIF_SHORT,                // type
        0,                                      // copy
        1,                                      // count
        { (char*)0 },                           // data._ascii (initialization applies
                                                // to first member of the union)
                                                // needs to be properly initialized by code
    },                                          // entry
    EXIFTAGID_RESOLUTION_UNIT
};

static EXIFTagInfo default_tag_tn_resolution_unit =
{
    {
        EXIFTagType::EXIF_SHORT,                // type
        0,                                      // copy
        1,                                      // count
        { (char*)0 },                           // data._ascii (initialization applies
                                                // to first member of the union)
                                                // needs to be properly initialized by code
    },                                          // entry
    EXIFTAGID_TN_RESOLUTION_UNIT
};

static EXIFTagInfo default_tag_tn_compression =
{
    {
        EXIFTagType::EXIF_SHORT,                // type
        0,                                      // copy
        1,                                      // count
        { (char*)0 },                           // data._ascii (initialization applies
                                                // to first member of the union)
                                                // needs to be properly initialized by code
    },                                          // entry
    EXIFTAGID_TN_COMPRESSION
};

static EXIFTagInfo default_tag_exif_x_resolution =
{
    {
        EXIFTagType::EXIF_RATIONAL,             // type
        0,                                      // copy
        1,                                      // count
        { (char*)0 },                           // data._ascii (initialization applies
                                                // to first member of the union)
                                                // needs to be properly initialized by code
    },                                          // entry
    EXIFTAGID_X_RESOLUTION
};

static EXIFTagInfo default_tag_exif_y_resolution =
{
    {
        EXIFTagType::EXIF_RATIONAL,             // type
        0,                                      // copy
        1,                                      // count
        { (char*)0 },                           // data._ascii (initialization applies
                                                // to first member of the union)
                                                // needs to be properly initialized by code
    },                                          // entry
    EXIFTAGID_Y_RESOLUTION
};

static EXIFTagInfo default_tag_tn_exif_x_resolution =
{
    {
        EXIFTagType::EXIF_RATIONAL,             // type
        0,                                      // copy
        1,                                      // count
        { (char*)0 },                           // data._ascii (initialization applies
                                                // to first member of the union)
                                                // needs to be properly initialized by code
    },                                          // entry
    EXIFTAGID_TN_X_RESOLUTION
};

static EXIFTagInfo default_tag_tn_exif_y_resolution =
{
    {
        EXIFTagType::EXIF_RATIONAL,             // type
        0,                                      // copy
        1,                                      // count
        { (char*)0 },                           // data._ascii (initialization applies
                                                // to first member of the union)
                                                // needs to be properly initialized by code
    },                                          // entry
    EXIFTAGID_TN_Y_RESOLUTION
};

static EXIFTagInfo default_tag_ycbcr_positioning =
{
    {
        EXIFTagType::EXIF_SHORT,                // type
        0,                                      // copy
        1,                                      // count
        { (char*)0 },                           // data._ascii (initialization applies
                                                // to first member of the union)
                                                // needs to be properly initialized by code
    },                                          // entry
    EXIFTAGID_YCBCR_POSITIONING
};

static EXIFTagInfo default_tag_color_space =
{
    {
        EXIFTagType::EXIF_SHORT,                // type
        0,                                      // copy
        1,                                      // count
        { (char*)0 },                           // data._ascii (initialization applies
                                                // to first member of the union)
                                                // needs to be properly initialized by code
    },                                          // entry
    EXIFTAGID_EXIF_COLOR_SPACE
};

static EXIFTagInfo* default_tags[] =
{
    &default_tag_make,
    &default_tag_model,
    &default_tag_datetime_original,
    &default_tag_datetime_digitized,
    &default_tag_exif_version,
    &default_tag_flash_pix_version,
    &default_tag_components_config,
    &default_tag_resolution_unit,
    &default_tag_tn_resolution_unit,
    &default_tag_tn_compression,
    &default_tag_tn_exif_x_resolution,
    &default_tag_tn_exif_y_resolution,
    &default_tag_exif_x_resolution,
    &default_tag_exif_y_resolution,
    &default_tag_ycbcr_positioning,
    &default_tag_color_space
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFParams::Initialize(
    Node* pNode,
    BOOL  bIsRealTime,
    BOOL  bIsGpuNodePresent)
{
    CamxResult result   = CamxResultSuccess;

    m_bIsRealTime       = bIsRealTime;
    m_bIsGpuNodePresent = bIsGpuNodePresent;

    if (NULL != pNode)
    {
        m_pNode = pNode;
    }
    else
    {
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        result = SetDefaultEXIFTagInfo();
    }

    if (CamxResultSuccess == result)
    {
        m_pQuantTables = CAMX_NEW JPEGQuantTable[NumQuantTables];
        if (NULL != m_pQuantTables)
        {
            result = JPEGUtil::SetDefaultQuantizationTables(m_pQuantTables);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Failed to alloc mem for Exif data Quantization table %p", m_pQuantTables);
            result = CamxResultENoMemory;
        }
    }

    if (CamxResultSuccess == result)
    {
        m_pQuantTablesThumbnail = CAMX_NEW JPEGQuantTable[NumQuantTables];
        if (NULL != m_pQuantTablesThumbnail)
        {
            result = JPEGUtil::SetDefaultQuantizationTables(m_pQuantTablesThumbnail);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Failed to alloc mem for Exif data Quantization table %p",
                m_pQuantTablesThumbnail);
            result = CamxResultENoMemory;
        }
    }

    if (CamxResultSuccess == result)
    {
        m_pHuffmanTables = CAMX_NEW JPEGHuffTable[NumHuffTables];
        if (NULL != m_pHuffmanTables)
        {
            result = JPEGUtil::SetDefaultHuffmanTables(m_pHuffmanTables);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Failed to alloc mem for Exif data Huffman table %p", m_pHuffmanTables);
            result = CamxResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::SetDefaultEXIFTagInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFParams::SetDefaultEXIFTagInfo()
{
    UINT32      dataSize     = static_cast<UINT32>(EXIFTagOffset::EXIF_TAG_MAX_OFFSET);
    CamxResult  result       = CamxResultSuccess;

    if (NULL == m_pEXIFTagData)
    {
        m_pEXIFTagData = CAMX_NEW EXIFTagInfo[dataSize];
        if (NULL == m_pEXIFTagData)
        {
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Failed to alloc mem for Exif data m_pEXIFTagData %p", m_pEXIFTagData);
            result = CamxResultENoMemory;
        }

        if (CamxResultSuccess == result)
        {
            FlushEXIFTagCopyData(m_pEXIFTagData, static_cast<UINT32>(EXIFTagOffset::EXIF_TAG_MAX_OFFSET));
            Utils::Memset(m_pEXIFTagData, 0x0, dataSize * sizeof(EXIFTagInfo));
        }
    }

    // Get product manufacturer and model name
    if (CamxResultSuccess == result)
    {
        if (CamX::OsUtils::GetPropertyString("ro.product.manufacturer", m_makeEXIF, "QTI-AA") > 0)
        {
            default_tag_make.entry.count        = static_cast<UINT32>(CamX::OsUtils::StrLen(m_makeEXIF)) + 1;
            default_tag_make.entry.data._ascii  = m_makeEXIF;
        }

        if (CamX::OsUtils::GetPropertyString("ro.product.model", m_modelEXIF, "QCAM-AA") > 0)
        {
            default_tag_model.entry.count       = static_cast<UINT32>(CamX::OsUtils::StrLen(m_modelEXIF)) + 1;
            default_tag_model.entry.data._ascii = m_modelEXIF;
        }
    }

    if (CamxResultSuccess == result)
    {
        /// @todo (CAMX-1980) Clean up JPEG legacy code
        default_tag_resolution_unit.entry.data._short = default_resolution_unit;
        default_tag_tn_resolution_unit.entry.data._short = default_resolution_unit;
        default_tag_tn_compression.entry.data._short = default_compression;
        default_tag_exif_x_resolution.entry.data._rat = default_exif_resolution;
        default_tag_exif_y_resolution.entry.data._rat = default_exif_resolution;
        default_tag_tn_exif_x_resolution.entry.data._rat = default_exif_resolution;
        default_tag_tn_exif_y_resolution.entry.data._rat = default_exif_resolution;
        default_tag_ycbcr_positioning.entry.data._short = default_ycbcr_positioning;
        default_tag_color_space.entry.data._short = default_color_space;

        for (UINT32 i = 0; i < sizeof(default_tags) / sizeof(EXIFTagInfo*); i++)
        {
            result = SetEXIFTagInfo(*default_tags[i]);
            if (CamxResultSuccess != result)
            {
                break;
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::SetEXIFTagInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFParams::SetEXIFTagInfo(
    EXIFTagInfo tagInfo)
{
    CAMX_ENTRYEXIT(CamxLogGroupJPEG);

    CamxResult    result     = CamxResultSuccess;
    EXIFTagInfo*  pTagLoc    = NULL;
    UINT32        tagOffset  = 0;
    UINT32        sizeToCopy = 0;

    if (NULL == m_pEXIFTagData)
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Pointer to EXIF tag data is null m_pEXIFTagData %p", m_pEXIFTagData);
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        if ((!tagInfo.entry.count) || (tagInfo.entry.type > EXIFTagType::EXIF_SRATIONAL))
        {
            result = CamxResultEInvalidArg;
        }
    }

    if (CamxResultSuccess == result)
    {
        tagOffset = (tagInfo.id >> 16) & 0xFFFF;

        if (tagOffset >= ExifMaxTag)
        {
            result = CamxResultEOutOfBounds;
        }
    }

    if (CamxResultSuccess == result)
    {
        pTagLoc                 = m_pEXIFTagData + tagOffset;
        pTagLoc->entry.copy     = tagInfo.entry.copy;
        pTagLoc->entry.count    = tagInfo.entry.count;
        pTagLoc->entry.type     = tagInfo.entry.type;
        pTagLoc->id             = tagInfo.id;
        pTagLoc->isTagSet       = TRUE;

        if (tagInfo.entry.copy)
        {
            switch (tagInfo.entry.type)
            {
                case EXIFTagType::EXIF_ASCII:
                    if (NULL != pTagLoc->entry.data._ascii)
                    {
                        CAMX_DELETE[] pTagLoc->entry.data._ascii;
                        pTagLoc->entry.data._ascii = NULL;
                    }

                    sizeToCopy                  = pTagLoc->entry.count * sizeof(CHAR);
                    pTagLoc->entry.data._ascii  = CAMX_NEW CHAR[sizeToCopy];
                    if (NULL == pTagLoc->entry.data._ascii)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Memory allocation failed %p", pTagLoc->entry.data._ascii);
                        result = CamxResultENoMemory;
                    }
                    else
                    {
                        Utils::Memcpy(pTagLoc->entry.data._ascii, tagInfo.entry.data._ascii, sizeToCopy);
                    }
                    break;
                case EXIFTagType::EXIF_UNDEFINED:
                    if (NULL != pTagLoc->entry.data._undefined)
                    {
                        CAMX_DELETE[] pTagLoc->entry.data._undefined;
                        pTagLoc->entry.data._undefined = NULL;
                    }

                    sizeToCopy                      = pTagLoc->entry.count * sizeof(UINT8);
                    pTagLoc->entry.data._undefined  = CAMX_NEW UINT8[sizeToCopy];
                    if (NULL == pTagLoc->entry.data._undefined)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Memory allocation failed %p", pTagLoc->entry.data._undefined);
                        result = CamxResultENoMemory;
                    }
                    else
                    {
                        Utils::Memcpy(pTagLoc->entry.data._undefined, tagInfo.entry.data._undefined, sizeToCopy);
                    }
                    break;
                case EXIFTagType::EXIF_BYTE:
                    if (tagInfo.entry.count > 1)
                    {
                        if (NULL != pTagLoc->entry.data._undefined)
                        {
                            CAMX_DELETE[] pTagLoc->entry.data._undefined;
                            pTagLoc->entry.data._undefined = NULL;
                        }

                        sizeToCopy                   = pTagLoc->entry.count * sizeof(UINT8);
                        pTagLoc->entry.data._bytes   = CAMX_NEW UINT8[sizeToCopy];
                        if (NULL == pTagLoc->entry.data._bytes)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Memory allocation failed %p", pTagLoc->entry.data._bytes);
                            result = CamxResultENoMemory;
                        }
                        else
                        {
                            Utils::Memcpy(pTagLoc->entry.data._bytes, tagInfo.entry.data._bytes, sizeToCopy);
                        }
                    }
                    break;
                case EXIFTagType::EXIF_SHORT:
                    if (tagInfo.entry.count > 1)
                    {
                        if (NULL != pTagLoc->entry.data._shorts)
                        {
                            CAMX_DELETE[] pTagLoc->entry.data._shorts;
                            pTagLoc->entry.data._shorts = NULL;
                        }

                        sizeToCopy                    = pTagLoc->entry.count * sizeof(UINT16);
                        pTagLoc->entry.data._shorts   = CAMX_NEW UINT16[sizeToCopy];
                        if (NULL == pTagLoc->entry.data._shorts)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Memory allocation failed %p", pTagLoc->entry.data._shorts);
                            result = CamxResultENoMemory;
                        }
                        else
                        {
                            Utils::Memcpy(pTagLoc->entry.data._shorts, tagInfo.entry.data._shorts, sizeToCopy);
                        }
                    }
                    break;
                case EXIFTagType::EXIF_LONG:
                    if (tagInfo.entry.count > 1)
                    {
                        if (NULL != pTagLoc->entry.data._longs)
                        {
                            CAMX_DELETE[] pTagLoc->entry.data._longs;
                            pTagLoc->entry.data._longs = NULL;
                        }

                        sizeToCopy                  = pTagLoc->entry.count * sizeof(UINT32);
                        pTagLoc->entry.data._longs  = CAMX_NEW UINT32[sizeToCopy];
                        if (NULL == pTagLoc->entry.data._longs)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Memory allocation failed %p", pTagLoc->entry.data._longs);
                            result = CamxResultENoMemory;
                        }
                        else
                        {
                            Utils::Memcpy(pTagLoc->entry.data._longs, tagInfo.entry.data._longs, sizeToCopy);
                        }
                    }
                    break;
                case EXIFTagType::EXIF_RATIONAL:
                    if (tagInfo.entry.count > 1)
                    {
                        if (NULL != pTagLoc->entry.data._rats)
                        {
                            CAMX_DELETE[] pTagLoc->entry.data._rats;
                            pTagLoc->entry.data._rats = NULL;
                        }

                        sizeToCopy                  = pTagLoc->entry.count * sizeof(URAT32);
                        pTagLoc->entry.data._rats   = CAMX_NEW URAT32[sizeToCopy];
                        if (NULL == pTagLoc->entry.data._rats)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Memory allocation failed %p", pTagLoc->entry.data._rats);
                            result = CamxResultENoMemory;
                        }
                        else
                        {
                            Utils::Memcpy(pTagLoc->entry.data._rats, tagInfo.entry.data._rats, sizeToCopy);
                        }
                    }
                    break;
                case EXIFTagType::EXIF_SLONG:
                    if (tagInfo.entry.count > 1)
                    {
                        if (NULL != pTagLoc->entry.data._slongs)
                        {
                            CAMX_DELETE[] pTagLoc->entry.data._slongs;
                            pTagLoc->entry.data._slongs = NULL;
                        }

                        sizeToCopy                  = pTagLoc->entry.count * sizeof(INT32);
                        pTagLoc->entry.data._slongs = CAMX_NEW INT32[sizeToCopy];
                        if (NULL == pTagLoc->entry.data._slongs)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Memory allocation failed %p", pTagLoc->entry.data._slongs);
                            result = CamxResultENoMemory;
                        }
                        else
                        {
                            Utils::Memcpy(pTagLoc->entry.data._slongs, tagInfo.entry.data._slongs, sizeToCopy);
                        }
                    }
                    break;
                case EXIFTagType::EXIF_SRATIONAL:
                    if (tagInfo.entry.count > 1)
                    {
                        if (NULL != pTagLoc->entry.data._srats)
                        {
                            CAMX_DELETE[] pTagLoc->entry.data._srats;
                            pTagLoc->entry.data._srats = NULL;
                        }

                        sizeToCopy                  = pTagLoc->entry.count * sizeof(RAT32);
                        pTagLoc->entry.data._srats  = CAMX_NEW RAT32[sizeToCopy];
                        if (NULL == pTagLoc->entry.data._srats)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Memory allocation failed %p", pTagLoc->entry.data._srats);
                            result = CamxResultENoMemory;
                        }
                        else
                        {
                            Utils::Memcpy(pTagLoc->entry.data._srats, tagInfo.entry.data._srats, sizeToCopy);
                        }
                    }
                    break;
                default:
                    CAMX_LOG_ERROR(CamxLogGroupJPEG, "Invalid tag type %d", tagInfo.entry.type);
                    result = CamxResultEInvalidArg;
            }
        }
        else
        {
            pTagLoc->entry.data = tagInfo.entry.data;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::SetEXIFImageParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFParams::SetEXIFImageParams(
    ImageFormat imageFormatInfo)
{
    m_imageParams.imgFormat = imageFormatInfo;
    return GetSubsampleCompCount(imageFormatInfo.format, &m_imageParams.subsampling, &m_imageParams.numComponents);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::SetEXIFImageParamsThumb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFParams::SetEXIFImageParamsThumb(
    ImageFormat imageFormatInfo)
{
    m_imageParamsThumb.imgFormat = imageFormatInfo;
    return GetSubsampleCompCount(imageFormatInfo.format, &m_imageParamsThumb.subsampling, &m_imageParamsThumb.numComponents);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFDateTime
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetEXIFDateTime()
{
    CAMX_ENTRYEXIT(CamxLogGroupJPEG);

    CHAR dateTime[20];
    CHAR subsecTime[8];

    CamxDateTime systemDateTime;
    EXIFTagInfo  tagInfo;

    CamX::OsUtils::GetDateTime(&systemDateTime);

    OsUtils::SNPrintF(dateTime,
             sizeof(dateTime),
             "%04d:%02d:%02d %02d:%02d:%02d",
             systemDateTime.year + 1900,
             systemDateTime.month + 1,
             systemDateTime.dayOfMonth,
             systemDateTime.hours,
             systemDateTime.minutes,
             systemDateTime.seconds);

    CAMX_LOG_INFO(CamxLogGroupJPEG, "Date time %s", dateTime);
    OsUtils::SNPrintF(subsecTime, sizeof(subsecTime), "%06ld", static_cast<long>(systemDateTime.microseconds));

    Utils::Memset(&tagInfo, 0x0, sizeof(EXIFTagInfo));
    tagInfo.entry.type          = EXIF_ASCII;
    tagInfo.entry.count         = static_cast<UINT32>(CamX::OsUtils::StrLen(dateTime)) + 1;
    tagInfo.entry.copy          = 1;
    tagInfo.entry.data._ascii   = reinterpret_cast<CHAR*>(dateTime);
    tagInfo.id                  = EXIFTAGID_DATE_TIME;
    SetEXIFTagInfo(tagInfo);

    Utils::Memset(&tagInfo, 0x0, sizeof(EXIFTagInfo));
    tagInfo.entry.type          = EXIF_ASCII;
    tagInfo.entry.count         = static_cast<UINT32>(CamX::OsUtils::StrLen(dateTime)) + 1;
    tagInfo.entry.copy          = 1;
    tagInfo.entry.data._ascii   = reinterpret_cast<CHAR*>(dateTime);
    tagInfo.id                  = EXIFTAGID_EXIF_DATE_TIME_ORIGINAL;
    SetEXIFTagInfo(tagInfo);

    Utils::Memset(&tagInfo, 0x0, sizeof(EXIFTagInfo));
    tagInfo.entry.type          = EXIF_ASCII;
    tagInfo.entry.count         = static_cast<UINT32>(CamX::OsUtils::StrLen(dateTime)) + 1;
    tagInfo.entry.copy          = 1;
    tagInfo.entry.data._ascii   = reinterpret_cast<CHAR*>(dateTime);
    tagInfo.id                  = EXIFTAGID_EXIF_DATE_TIME_DIGITIZED;
    SetEXIFTagInfo(tagInfo);

    Utils::Memset(&tagInfo, 0x0, sizeof(EXIFTagInfo));
    tagInfo.entry.type          = EXIF_ASCII;
    tagInfo.entry.count         = static_cast<UINT32>(CamX::OsUtils::StrLen(subsecTime)) + 1;
    tagInfo.entry.copy          = 1;
    tagInfo.entry.data._ascii   = reinterpret_cast<CHAR*>(subsecTime);
    tagInfo.id                  = EXIFTAGID_SUBSEC_TIME;
    SetEXIFTagInfo(tagInfo);

    Utils::Memset(&tagInfo, 0x0, sizeof(EXIFTagInfo));
    tagInfo.entry.type          = EXIF_ASCII;
    tagInfo.entry.count         = static_cast<UINT32>(CamX::OsUtils::StrLen(subsecTime)) + 1;
    tagInfo.entry.copy          = 1;
    tagInfo.entry.data._ascii   = reinterpret_cast<CHAR*>(subsecTime);
    tagInfo.id                  = EXIFTAGID_SUBSEC_TIME_ORIGINAL;
    SetEXIFTagInfo(tagInfo);

    Utils::Memset(&tagInfo, 0x0, sizeof(EXIFTagInfo));
    tagInfo.entry.type          = EXIF_ASCII;
    tagInfo.entry.count         = static_cast<UINT32>(CamX::OsUtils::StrLen(subsecTime)) + 1;
    tagInfo.entry.copy          = 1;
    tagInfo.entry.data._ascii   = reinterpret_cast<CHAR*>(subsecTime);
    tagInfo.id                  = EXIFTAGID_SUBSEC_TIME_DIGITIZED;
    SetEXIFTagInfo(tagInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFOrientation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetEXIFOrientation()
{
    CAMX_ENTRYEXIT(CamxLogGroupJPEG);

    EXIFRotationType    EXIFRotation     = EXIFRotationType::EXIF0;
    EXIFTagInfo         tagInfo;

    static const UINT JpegTag[] =
    {
        InputJPEGOrientation
    };
    VOID* pData[] = { 0 };
    UINT length = CAMX_ARRAY_SIZE(JpegTag);
    UINT64 JpegDataOffset[1] = { 0 };

    m_pNode->GetDataList(JpegTag, pData, JpegDataOffset, length);

    if (NULL != pData[0])
    {
        m_dataEXIF.orientation = *reinterpret_cast<INT32*>(pData[0]);
        CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "Rotation received %d", m_dataEXIF.orientation);
    }
    else
    {
        m_dataEXIF.orientation = 0;
        CAMX_LOG_WARN(CamxLogGroupJPEG, "Exif orientation is not received");
    }

    switch (m_dataEXIF.orientation)
    {
        case 0:
            EXIFRotation = EXIFRotationType::EXIF0;
            break;
        case 90:
            EXIFRotation = EXIFRotationType::EXIF90;
            break;
        case 180:
            EXIFRotation = EXIFRotationType::EXIF180;
            break;
        case 270:
            EXIFRotation = EXIFRotationType::EXIF270;
            break;
        default:
            EXIFRotation = EXIFRotationType::EXIF0;
            break;
    }

    tagInfo.entry.type          = EXIFTagType::EXIF_SHORT;
    tagInfo.entry.count         = 1;
    tagInfo.entry.copy          = 0;
    tagInfo.entry.data._short   = static_cast<UINT16>(EXIFRotation);
    tagInfo.id                  = EXIFTAGID_ORIENTATION;
    SetEXIFTagInfo(tagInfo);

    Utils::Memset(&tagInfo, 0x0, sizeof(EXIFTagInfo));
    tagInfo.entry.type          = EXIFTagType::EXIF_SHORT;
    tagInfo.entry.count         = 1;
    tagInfo.entry.copy          = 0;
    tagInfo.entry.data._short   = static_cast<UINT16>(EXIFRotation);
    tagInfo.id                  = EXIFTAGID_TN_ORIENTATION;
    SetEXIFTagInfo(tagInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFFocalLengthRational
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFParams::GetEXIFFocalLengthRational(
    URAT32* pFocalLength,
    FLOAT   value)
{
    UINT32 focalLengthValue = static_cast<UINT32>((value * FocalLengthDecimalPrecision));
    return JPEGUtil::GetUnsignedRational(pFocalLength, focalLengthValue, FocalLengthDecimalPrecision);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFAperture
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetEXIFAperture()
{
    URAT32            aperture               = {};
    URAT32            fNumber                = {};
    URAT32            maxAperture            = {};
    EXIFTagInfo       tagInfo                = {};
    static const UINT JpegTag[]              = { InputLensAperture, LensInfoAvailableApertures | StaticMetadataSectionMask };
    const UINT        length                 = CAMX_ARRAY_SIZE(JpegTag);
    VOID*             pData[length]          = { 0 };
    UINT64            jpegDataOffset[length] = { 0 };

    m_pNode->GetDataList(JpegTag, pData, jpegDataOffset, length);
    m_dataEXIF.data3ASensor.aperture = *reinterpret_cast<FLOAT*>(pData[0]);
    FLOAT apertureVal = m_dataEXIF.data3ASensor.aperture;

    CAMX_LOG_INFO(CamxLogGroupJPEG, "Aperture received %f", apertureVal);
    if (1.0 <= apertureVal)
    {
        DOUBLE apexValue        = 2.0 * log(apertureVal) / log(2.0);
        aperture.numerator      = static_cast<UINT32>(apexValue * 100);
        aperture.denominator    = 100;

        tagInfo.entry.type          = EXIFTagType::EXIF_RATIONAL;
        tagInfo.entry.count         = 1;
        tagInfo.entry.copy          = 0;
        tagInfo.entry.data._rat     = aperture;
        tagInfo.id                  = EXIFTAGID_APERTURE;
        SetEXIFTagInfo(tagInfo);

        fNumber.numerator   = static_cast<UINT32>((apertureVal) * 100);
        fNumber.denominator = 100;

        Utils::Memset(&tagInfo, 0x0, sizeof(tagInfo));
        tagInfo.entry.type          = EXIFTagType::EXIF_RATIONAL;
        tagInfo.entry.count         = 1;
        tagInfo.entry.copy          = 0;
        tagInfo.entry.data._rat     = fNumber;
        tagInfo.id                  = EXIFTAGID_F_NUMBER;
        SetEXIFTagInfo(tagInfo);
    }

    FLOAT maxApertureVal    = *reinterpret_cast<FLOAT*>(pData[1]);
    CAMX_LOG_INFO(CamxLogGroupJPEG, "Max aperture received %f", maxApertureVal);

    DOUBLE maxApexValue     = 2.0 * log(apertureVal) / log(2.0);
    maxAperture.numerator   = static_cast<UINT32>(maxApexValue * 100);
    maxAperture.denominator = 100;

    Utils::Memset(&tagInfo, 0x0, sizeof(tagInfo));
    tagInfo.entry.type      = EXIFTagType::EXIF_RATIONAL;
    tagInfo.entry.count     = 1;
    tagInfo.entry.copy      = 0;
    tagInfo.entry.data._rat = maxAperture;
    tagInfo.id              = EXIFTAGID_MAX_APERTURE;
    SetEXIFTagInfo(tagInfo);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFFocalLength
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetEXIFFocalLength()
{
    CamxResult        result      = CamxResultSuccess;
    SensorProperties  sensorProp  = { 0 };
    URAT32            focalLength = {};
    EXIFTagInfo       tagInfo     = {};
    static const UINT JpegTag[]   =
    {
        InputLensFocalLength,
        PropertyIDSensorProperties | InputMetadataSectionMask
    };
    static const UINT length                 = CAMX_ARRAY_SIZE(JpegTag);
    VOID*             pData[length]          = { 0 };
    UINT64            jpegDataOffset[length] = { 0 };

    m_pNode->GetDataList(JpegTag, pData, jpegDataOffset, length);
    m_dataEXIF.data3ASensor.focalLength = *reinterpret_cast<FLOAT*>(pData[0]);
    FLOAT focalLengthVal = m_dataEXIF.data3ASensor.focalLength;

    CAMX_LOG_INFO(CamxLogGroupJPEG, "Focal length received %f", focalLengthVal);
    result = GetEXIFFocalLengthRational(&focalLength, focalLengthVal);
    if (CamxResultSuccess == result)
    {
        tagInfo.entry.type          = EXIFTagType::EXIF_RATIONAL;
        tagInfo.entry.count         = 1;
        tagInfo.entry.copy          = 0;
        tagInfo.entry.data._rat     = focalLength;
        tagInfo.id                  = EXIFTAGID_FOCAL_LENGTH;
        SetEXIFTagInfo(tagInfo);
    }

    if (NULL != pData[1])
    {
        sensorProp = *reinterpret_cast<SensorProperties*>(pData[1]);

        CAMX_LOG_INFO(CamxLogGroupJPEG, "focalLengthIn35mm %f", sensorProp.focalLengthIn35mm);
        Utils::Memset(&tagInfo, 0x0, sizeof(tagInfo));
        tagInfo.entry.type        = EXIFTagType::EXIF_SHORT;
        tagInfo.entry.count       = 1;
        tagInfo.entry.copy        = 0;
        tagInfo.entry.data._short = static_cast<INT16>(sensorProp.focalLengthIn35mm + 0.5f);
        tagInfo.id                = EXIFTAGID_FOCAL_LENGTH_35MM;
        SetEXIFTagInfo(tagInfo);

        CAMX_LOG_INFO(CamxLogGroupJPEG, "sensingMethod %d", sensorProp.sensingMethod);
        Utils::Memset(&tagInfo, 0x0, sizeof(tagInfo));
        tagInfo.entry.type          = EXIFTagType::EXIF_SHORT;
        tagInfo.entry.count         = 1;
        tagInfo.entry.copy          = 0;
        tagInfo.entry.data._short   = static_cast<INT16>(sensorProp.sensingMethod);
        tagInfo.id                  = EXIFTAGID_SENSING_METHOD;
        SetEXIFTagInfo(tagInfo);
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupJPEG, "PropertyIDSensorProperties not published");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetExposureCompensation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetEXIFExposureCompensation()
{
    RAT32             exposureCompensationValue = {};
    EXIFTagInfo       tagInfo                   = {};
    static const UINT JpegTag[]                 = { ControlAEExposureCompensation, InputControlAEExposureCompensation,
                                                    ControlAECompensationStep, InputControlAECompensationStep };
    const UINT        length                    = CAMX_ARRAY_SIZE(JpegTag);
    VOID*             pData[length]             = { 0 };
    UINT64            jpegDataOffset[length]    = { 0 };

    m_pNode->GetDataList(JpegTag, pData, jpegDataOffset, length);
    if (NULL != pData[0])
    {
        m_dataEXIF.data3ASensor.exposureCompensation = *reinterpret_cast<INT32*>(pData[0]);
    }
    else if (NULL != pData[1])
    {
        m_dataEXIF.data3ASensor.exposureCompensation = *reinterpret_cast<INT32*>(pData[1]);
    }

    if (NULL != pData[2])
    {
        m_dataEXIF.data3ASensor.exposureCompensationStep = *reinterpret_cast<RAT32*>(pData[2]);
    }
    else if (NULL != pData[3])
    {
        m_dataEXIF.data3ASensor.exposureCompensationStep = *reinterpret_cast<RAT32*>(pData[3]);
    }

    CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "Exposure compensation received %d", m_dataEXIF.data3ASensor.exposureCompensation);

    CAMX_LOG_VERBOSE(CamxLogGroupJPEG,
        "Exposure compensation step received numerator %d denominator %d",
        m_dataEXIF.data3ASensor.exposureCompensationStep.numerator,
        m_dataEXIF.data3ASensor.exposureCompensationStep.denominator);

    exposureCompensationValue.numerator =
        (m_dataEXIF.data3ASensor.exposureCompensation) * (m_dataEXIF.data3ASensor.exposureCompensationStep.numerator);
    exposureCompensationValue.denominator = m_dataEXIF.data3ASensor.exposureCompensationStep.denominator;

    tagInfo.entry.type       = EXIFTagType::EXIF_SRATIONAL;
    tagInfo.entry.count      = 1;
    tagInfo.entry.copy       = 0;
    tagInfo.entry.data._srat = exposureCompensationValue;
    tagInfo.id               = EXIFTAGID_EXPOSURE_BIAS_VALUE;
    SetEXIFTagInfo(tagInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFExposure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetEXIFExposure()
{
    URAT32               exposureTime = {};
    RAT32                shutterSpeed = {};
    EXIFTagInfo          tagInfo      = {};
    AECFrameInformation* pFrameInfo   = NULL;
    ASDOutput*           pASDOutput   = NULL;
    PropertyID           aecPropertyIdValue;
    PropertyID           asdPropertyIdValue;

    if (TRUE == m_bIsRealTime)
    {
        aecPropertyIdValue = PropertyIDAECFrameInfo;
        asdPropertyIdValue = PropertyIDASD;
    }
    else
    {
        aecPropertyIdValue = PropertyIDAECFrameInfo | InputMetadataSectionMask;
        asdPropertyIdValue = PropertyIDASD | InputMetadataSectionMask;
    }

    static const UINT JpegTag[] =
    {
        InputSensorExposureTime,
        SensorExposureTime,
        aecPropertyIdValue,
        asdPropertyIdValue
    };

    static const UINT length                 = CAMX_ARRAY_SIZE(JpegTag);
    VOID*             pData[length]          = { 0 };
    UINT64            jpegDataOffset[length] = { 0 };

    m_pNode->GetDataList(JpegTag, pData, jpegDataOffset, length);
    if (NULL != pData[1])
    {
        m_dataEXIF.data3ASensor.exposureTime = *reinterpret_cast<INT64*>(pData[1]);
        CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "Exposure time received %lld", m_dataEXIF.data3ASensor.exposureTime);
    }
    else
    {
        m_dataEXIF.data3ASensor.exposureTime = *reinterpret_cast<INT64*>(pData[0]);
        CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "Input Exposure time received %lld", m_dataEXIF.data3ASensor.exposureTime);
    }

    if (NULL != pData[2])
    {
        pFrameInfo = reinterpret_cast<AECFrameInformation*>(pData[2]);
        CAMX_LOG_VERBOSE(CamxLogGroupJPEG,
                         "AEC Exposure Info: BV=%f MeteringMode=%d ExpProgram=%d SceneType=%d ExposureMode=%d",
                         pFrameInfo->brightnessValue,
                         pFrameInfo->meteringMode,
                         pFrameInfo->exposureProgram,
                         pFrameInfo->sceneType,
                         pFrameInfo->exposureMode);
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupJPEG, "AEC Exposure Info:NULL");
    }

    if (NULL != pData[3])
    {
        pASDOutput = reinterpret_cast<ASDOutput*>(pData[3]);
        CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "Detected (PT:%d, LS:%d)",
                         pASDOutput->detected[ASDPortrait],
                         pASDOutput->detected[ASDLandscape]);
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupJPEG, "ASD Output:NULL");
    }

    FLOAT exposureTimeSec = static_cast<FLOAT>(static_cast<DOUBLE>(m_dataEXIF.data3ASensor.exposureTime) / 1000000000.0);
    if (0.0f >= exposureTimeSec)
    {
        exposureTime.numerator      = 0;
        exposureTime.denominator    = 0;
    }
    else if (1.0f > exposureTimeSec)
    {
        exposureTime.numerator      = 1;
        exposureTime.denominator    = static_cast<UINT32>(Utils::RoundFLOAT(static_cast<FLOAT>(1.0) / exposureTimeSec));
    }
    else
    {
        exposureTime.numerator      = static_cast<UINT32>(Utils::RoundFLOAT(exposureTimeSec));
        exposureTime.denominator    = 1;
    }
    CAMX_LOG_VERBOSE(CamxLogGroupJPEG,
                     "Exposure time numerator %d denominator %d",
                     exposureTime.numerator,
                     exposureTime.denominator);

    tagInfo.entry.type          = EXIFTagType::EXIF_RATIONAL;
    tagInfo.entry.count         = 1;
    tagInfo.entry.copy          = 0;
    tagInfo.entry.data._rat     = exposureTime;
    tagInfo.id                  = EXIFTAGID_EXPOSURE_TIME;
    SetEXIFTagInfo(tagInfo);

    if (0.0f < exposureTimeSec)
    {
        DOUBLE shutterSpeedValue    = log10(1 / exposureTimeSec) / log10(2);
        shutterSpeed.numerator      = static_cast<INT32>(shutterSpeedValue * 1000);
        shutterSpeed.denominator    = 1000;
    }
    else
    {
        shutterSpeed.numerator      = 0;
        shutterSpeed.denominator    = 0;
    }

    Utils::Memset(&tagInfo, 0x0, sizeof(tagInfo));
    tagInfo.entry.type          = EXIFTagType::EXIF_SRATIONAL;
    tagInfo.entry.count         = 1;
    tagInfo.entry.copy          = 0;
    tagInfo.entry.data._srat    = shutterSpeed;
    tagInfo.id                  = EXIFTAGID_SHUTTER_SPEED;
    SetEXIFTagInfo(tagInfo);

    if (NULL != pFrameInfo)
    {
        // Set exposure metering mode
        Utils::Memset(&tagInfo, 0x0, sizeof(tagInfo));
        tagInfo.entry.type          = EXIFTagType::EXIF_SHORT;
        tagInfo.entry.count         = 1;
        tagInfo.entry.copy          = 0;
        tagInfo.entry.data._short   = pFrameInfo->meteringMode;
        tagInfo.id                  = EXIFTAGID_METERING_MODE;
        SetEXIFTagInfo(tagInfo);

        // Set Exposure mode
        Utils::Memset(&tagInfo, 0x0, sizeof(tagInfo));
        tagInfo.entry.type          = EXIFTagType::EXIF_SHORT;
        tagInfo.entry.count         = 1;
        tagInfo.entry.copy          = 0;
        tagInfo.entry.data._short   = pFrameInfo->exposureMode;
        tagInfo.id                  = EXIFTAGID_EXPOSURE_MODE;
        SetEXIFTagInfo(tagInfo);

        // set exposure program
        Utils::Memset(&tagInfo, 0x0, sizeof(tagInfo));
        tagInfo.entry.type          = EXIFTagType::EXIF_SHORT;
        tagInfo.entry.count         = 1;
        tagInfo.entry.copy          = 0;
        tagInfo.entry.data._short   = pFrameInfo->exposureProgram;
        tagInfo.id                  = EXIFTAGID_EXPOSURE_PROGRAM;
        SetEXIFTagInfo(tagInfo);

        // set brightness
        Utils::Memset(&tagInfo, 0x0, sizeof(tagInfo));
        tagInfo.entry.type          = EXIFTagType::EXIF_SRATIONAL;
        tagInfo.entry.count         = 1;
        tagInfo.entry.copy          = 0;
        tagInfo.entry.data._srat.numerator   = (INT32)(pFrameInfo->brightnessValue * 100.0f);
        tagInfo.entry.data._srat.denominator = 100;
        tagInfo.id                  = EXIFTAGID_BRIGHTNESS;
        SetEXIFTagInfo(tagInfo);

        // set Scene type
        Utils::Memset(&tagInfo, 0x0, sizeof(tagInfo));
        tagInfo.entry.type       = EXIFTagType::EXIF_BYTE;
        tagInfo.entry.count      = 1;
        tagInfo.entry.copy       = 0;
        tagInfo.entry.data._byte = static_cast<BYTE>(pFrameInfo->sceneType);
        tagInfo.id               = EXIFTAGID_SCENE_TYPE;
        SetEXIFTagInfo(tagInfo);
    }

    if (NULL != pASDOutput)
    {
        // set scene capture type
        INT16 sceneCaptureType = 0;
        if (TRUE == pASDOutput->detected[ASDPortrait])
        {
            sceneCaptureType = 2;
        }
        else if (TRUE == pASDOutput->detected[ASDLandscape])
        {
            sceneCaptureType = 1;
        }
        else
        {
            sceneCaptureType = 0;
        }

        Utils::Memset(&tagInfo, 0x0, sizeof(tagInfo));
        tagInfo.entry.type          = EXIFTagType::EXIF_SHORT;
        tagInfo.entry.count         = 1;
        tagInfo.entry.copy          = 0;
        tagInfo.entry.data._short   = sceneCaptureType;
        tagInfo.id                  = EXIFTAGID_SCENE_CAPTURE_TYPE;
        SetEXIFTagInfo(tagInfo);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFISO
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetEXIFISO()
{
    INT16             sensorISOSpeedRate     = {};
    EXIFTagInfo       tagInfo                = {};
    static const UINT JpegTag[]              = { SensorSensitivity, InputSensorSensitivity,
                                                 ControlPostRawSensitivityBoost, InputControlPostRawSensitivityBoost };
    const UINT        length                 = CAMX_ARRAY_SIZE(JpegTag);
    VOID*             pData[length]          = { 0 };
    UINT64            jpegDataOffset[length] = { 0 };

    m_pNode->GetDataList(JpegTag, pData, jpegDataOffset, length);
    if (NULL != pData[0])
    {
        m_dataEXIF.data3ASensor.sensitivity = *reinterpret_cast<INT32*>(pData[0]);
        CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "Sensitivity received %d", m_dataEXIF.data3ASensor.sensitivity);
    }
    else
    {
        m_dataEXIF.data3ASensor.sensitivity = *reinterpret_cast<INT32*>(pData[1]);
        CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "Input Sensitivity received %d", m_dataEXIF.data3ASensor.sensitivity);
    }

    if (NULL != pData[2])
    {
        m_dataEXIF.data3ASensor.sensitivityBoost = *reinterpret_cast<INT32*>(pData[2]);
        CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "SensitivityBoost received %d", m_dataEXIF.data3ASensor.sensitivityBoost);

        sensorISOSpeedRate = static_cast<INT16>((m_dataEXIF.data3ASensor.sensitivity *
            m_dataEXIF.data3ASensor.sensitivityBoost) / 100);
    }
    else if (NULL != pData[3])
    {
        m_dataEXIF.data3ASensor.sensitivityBoost = *reinterpret_cast<INT32*>(pData[3]);
        CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "Input SensitivityBoost received %d", m_dataEXIF.data3ASensor.sensitivityBoost);

        sensorISOSpeedRate = static_cast<INT16>((m_dataEXIF.data3ASensor.sensitivity *
            m_dataEXIF.data3ASensor.sensitivityBoost) / 100);
    }
    else
    {
        sensorISOSpeedRate = static_cast<INT16>(m_dataEXIF.data3ASensor.sensitivity);
    }

    tagInfo.entry.type          = EXIFTagType::EXIF_SHORT;
    tagInfo.entry.count         = 1;
    tagInfo.entry.copy          = 0;
    tagInfo.entry.data._short   = sensorISOSpeedRate;
    tagInfo.id                  = EXIFTAGID_ISO_SPEED_RATING;
    SetEXIFTagInfo(tagInfo);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFWhiteBalance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetEXIFWhiteBalance()
{
    INT16             whiteBalance           = {};
    INT16             lightSource            = {};
    EXIFTagInfo       tagInfo                = {};
    static const UINT JpegTag[]              = { InputControlAWBMode, SensorReferenceIlluminant1 | StaticMetadataSectionMask };
    const UINT        length                 = CAMX_ARRAY_SIZE(JpegTag);
    VOID*             pData[length]          = { 0 };
    UINT64            jpegDataOffset[length] = { 0 };

    m_pNode->GetDataList(JpegTag, pData, jpegDataOffset, length);
    m_dataEXIF.data3ASensor.whiteBalanceMode = *reinterpret_cast<BYTE*>(pData[0]);

    CAMX_LOG_INFO(CamxLogGroupJPEG, "White balance mode received %d", m_dataEXIF.data3ASensor.whiteBalanceMode);
    if (ControlAWBModeValues::ControlAWBModeAuto == m_dataEXIF.data3ASensor.whiteBalanceMode)
    {
        whiteBalance = 0;
    }
    else
    {
        whiteBalance = 1;
    }

    tagInfo.entry.type          = EXIFTagType::EXIF_SHORT;
    tagInfo.entry.count         = 1;
    tagInfo.entry.copy          = 0;
    tagInfo.entry.data._short   = whiteBalance;
    tagInfo.id                  = EXIFTAGID_WHITE_BALANCE;
    SetEXIFTagInfo(tagInfo);

    lightSource = *reinterpret_cast<BYTE*>(pData[1]);
    CAMX_LOG_INFO(CamxLogGroupJPEG, "Source light %d", lightSource);

    Utils::Memset(&tagInfo, 0x0, sizeof(tagInfo));
    tagInfo.entry.type          = EXIFTagType::EXIF_SHORT;
    tagInfo.entry.count         = 1;
    tagInfo.entry.copy          = 0;
    tagInfo.entry.data._short   = lightSource;
    tagInfo.id                  = EXIFTAGID_LIGHT_SOURCE;
    SetEXIFTagInfo(tagInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFMeteringMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetEXIFMeteringMode()
{
    INT16        meteringmode            = {};
    EXIFTagInfo  tagInfo                 = {};
    UINT32       metaTagExposureMetering = 0;
    INT16        meteringmodevalue       = {};
    UINT         jpegTag[1]              = {0};
    const UINT   length                  = CAMX_ARRAY_SIZE(jpegTag);
    VOID*        pData[length]           = { 0 };
    UINT64       jpegDataOffset[length]  = { 0 };

    VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.exposure_metering",
        "exposure_metering_mode",
        &metaTagExposureMetering);
    jpegTag[0] = metaTagExposureMetering | InputMetadataSectionMask;
    m_pNode->GetDataList(jpegTag, pData, jpegDataOffset, length);

    if (NULL != pData[0])
    {
        meteringmode = *reinterpret_cast<BYTE*>(pData[0]);

        switch (meteringmode)
        {
            case ExposureMeteringFrameAverage:
                meteringmodevalue = 1;
                break;
            case ExposureMeteringCenterWeighted:
                meteringmodevalue = 2;
                break;
            case ExposureMeteringSpotMetering:
                meteringmodevalue = 3;
                break;
            default:
                meteringmodevalue = 0;
                break;
        }

        tagInfo.entry.type        = EXIFTagType::EXIF_SHORT;
        tagInfo.entry.count       = 1;
        tagInfo.entry.copy        = 0;
        tagInfo.entry.data._short = meteringmodevalue;
        tagInfo.id                = EXIFTAGID_METERING_MODE;
        SetEXIFTagInfo(tagInfo);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFSensorData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetEXIF3ASensorData()
{
    CAMX_ENTRYEXIT(CamxLogGroupJPEG);

    GetEXIFFocalLength();
    GetEXIFExposureCompensation();
    GetEXIFAperture();
    GetEXIFExposure();
    GetEXIFISO();
    GetEXIFWhiteBalance();
    GetEXIFMeteringMode();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFFlashData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetEXIFFlashData()
{
    CAMX_ENTRYEXIT(CamxLogGroupJPEG);

    INT16               flashValue              = 0;
    INT16               flashedFired            = 0;
    EXIFFlashMode       flashEXIFMode           = EXIFFlashMode::FLASHAUTO;
    EXIFTagInfo         tagInfo                 = {};
    static const UINT   JpegTag[]               = { InputFlashMode, InputFlashState, FlashState, InputControlAEMode };
    const UINT          length                  = CAMX_ARRAY_SIZE(JpegTag);
    VOID*               pData[length]           = { 0 };
    UINT64              jpegDataOffset[length]  = { 0 };
    BOOL                isEnoughDataForFlash    = TRUE;

    m_pNode->GetDataList(JpegTag, pData, jpegDataOffset, length);

    if (NULL != pData[0])
    {
        m_dataEXIF.dataFlash.flashMode        = *reinterpret_cast<BYTE*>(pData[0]);
    }
    else
    {
        isEnoughDataForFlash = FALSE;
    }

    if (NULL != pData[2])
    {
        m_dataEXIF.dataFlash.flashState = *reinterpret_cast<BYTE*>(pData[2]);
    }
    else if (NULL != pData[1])
    {
        m_dataEXIF.dataFlash.flashState = *reinterpret_cast<BYTE*>(pData[1]);
    }
    else
    {
        m_dataEXIF.dataFlash.flashState = FlashStateValues::FlashStateUnavailable;
    }

    if (NULL != pData[3])
    {
        m_dataEXIF.dataFlash.autoExposureMode = *reinterpret_cast<BYTE*>(pData[3]);
    }
    else
    {
        isEnoughDataForFlash = FALSE;
    }

    switch(m_dataEXIF.dataFlash.autoExposureMode)
    {
        case ControlAEModeValues::ControlAEModeOff:
        case ControlAEModeValues::ControlAEModeOn:
            if (FlashModeValues::FlashModeOff == m_dataEXIF.dataFlash.flashMode)
            {
                flashEXIFMode = EXIFFlashMode::FLASHOFF;
            }
            else
            {
                flashEXIFMode = EXIFFlashMode::FLASHON;
            }
            break;

        case ControlAEModeValues::ControlAEModeOnAutoFlash:
        case ControlAEModeValues::ControlAEModeOnAutoFlashRedeye:
            flashEXIFMode = EXIFFlashMode::FLASHAUTO;
            break;

        case ControlAEModeValues::ControlAEModeOnAlwaysFlash:
            flashEXIFMode = EXIFFlashMode::FLASHON;
            break;

        default:
            flashEXIFMode = EXIFFlashMode::FLASHAUTO;
    }

    if (FlashStateValues::FlashStateFired == static_cast<FlashStateValues>(m_dataEXIF.dataFlash.flashState))
    {
        flashedFired = 1;
    }
    else
    {
        flashedFired = 0;
    }

    if (TRUE == isEnoughDataForFlash)
    {

        flashValue                  = flashedFired | (static_cast<INT16>(flashEXIFMode) << 3);
        tagInfo.entry.type          = EXIFTagType::EXIF_SHORT;
        tagInfo.entry.count         = 1;
        tagInfo.entry.copy          = 0;
        tagInfo.entry.data._short   = flashValue;
        tagInfo.id                  = EXIFTAGID_FLASH;

        CAMX_LOG_VERBOSE(CamxLogGroupJPEG,
                         "FLASH_DEBUG  AutoExposureMode %d  m_dataEXIF.dataFlash.flashMode :%d flashValue: 0x%x",
                         m_dataEXIF.dataFlash.autoExposureMode,
                         m_dataEXIF.dataFlash.flashMode,
                         flashValue);

        SetEXIFTagInfo(tagInfo);
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupJPEG, "FLASH_DEBUG: Not enough data to set EXIF Flash data, skip");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFCustomQuantizationtables
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetEXIFCustomQuantizationtables()
{
    CAMX_ENTRYEXIT(CamxLogGroupJPEG);

    UINT32      metaTag = 0;
    CamxResult  result  = CamxResultSuccess;

    UINT JpegTag[2] = { 0 };
    VOID* pData[2] = { 0 };
    UINT length = CAMX_ARRAY_SIZE(JpegTag);
    UINT64 jpegDataOffset[2] = { 0 };

    result = VendorTagManager::QueryVendorTagLocation(QuantizationTableVendorTagSection,
        QuantizationTableLumaVendorTagName,
        &metaTag);
    CAMX_ASSERT(CamxResultSuccess == result);
    JpegTag[0] = (metaTag | InputMetadataSectionMask);

    result = VendorTagManager::QueryVendorTagLocation(QuantizationTableVendorTagSection,
        QuantizationTableChromaVendorTagName,
        &metaTag);
    JpegTag[1] = (metaTag | InputMetadataSectionMask);
    CAMX_ASSERT(CamxResultSuccess == result);

    m_pNode->GetDataList(JpegTag, pData, jpegDataOffset, length);

    if (CamxResultSuccess == result)
    {
        if (NULL != pData[0])
        {
            if (TRUE == IsMainAvailable())
            {
                m_pQuantTables[static_cast<INT>(QuantTableType::QuantTableLuma)].SetTable(reinterpret_cast<UINT16*>(pData[0]));
            }
            if (TRUE == IsThumbnailAvailable())
            {
                m_pQuantTablesThumbnail[static_cast<INT>(QuantTableType::QuantTableLuma)].SetTable(
                    reinterpret_cast<UINT16*>(pData[0]));
            }
        }
        if (NULL != pData[1])
        {
            if (TRUE == IsMainAvailable())
            {
                m_pQuantTables[static_cast<INT>(QuantTableType::QuantTableChroma)].SetTable(reinterpret_cast<UINT16*>(pData[1]));
            }
            if (TRUE == IsThumbnailAvailable())
            {
                m_pQuantTablesThumbnail[static_cast<INT>(QuantTableType::QuantTableChroma)].SetTable(
                    reinterpret_cast<UINT16*>(pData[1]));
            }
        }

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFGPSProcessingMethod
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetEXIFGPSProcessingMethod()
{
    if (0 != m_pNode->GetDataCountFromPipeline(InputJPEGGpsProcessingMethod, 0, m_pNode->GetPipelineId(), FALSE))
    {
        CHAR              gpsProcessingMethod[EXIFASCIIPrefixSize + GPSProcessingMethodSize] = {};
        EXIFTagInfo       tagInfo                                                            = {};
        static const UINT JpegTag[]                                                          = { InputJPEGGpsProcessingMethod };
        const UINT        length                                                             = CAMX_ARRAY_SIZE(JpegTag);
        VOID*             pData[length]                                                      = { 0 };
        UINT64            jpegDataOffset[length]                                             = { 0 };

        m_pNode->GetDataList(JpegTag, pData, jpegDataOffset, length);

        // sizeof(CHAR) * (GPSProcessingMethodSize - 1) because gpsProcessingMethod is designated with fixed size of 32
        Utils::Memcpy(m_dataEXIF.dataGPS.gpsProcessingMethod,
                      reinterpret_cast<CHAR*>(pData[0]),
                      sizeof(CHAR) * (GPSProcessingMethodSize - 1));
        // add NULL terminated char to end of gpsProcessingMethod
        m_dataEXIF.dataGPS.gpsProcessingMethod[GPSProcessingMethodSize - 1] = '\0';
        CHAR*       pGPSProcessingMethod = reinterpret_cast<CHAR*>(m_dataEXIF.dataGPS.gpsProcessingMethod);

        if (0 < CamX::OsUtils::StrLen(pGPSProcessingMethod))
        {
            UINT32 count = 0;
            Utils::Memcpy(gpsProcessingMethod, EXIFASCIIPrefix, EXIFASCIIPrefixSize);
            count = EXIFASCIIPrefixSize;

            CamX::OsUtils::StrLCpy(gpsProcessingMethod + EXIFASCIIPrefixSize,
                                pGPSProcessingMethod,
                                sizeof(gpsProcessingMethod) - EXIFASCIIPrefixSize);

            // increase 1 to include null character
            count += (static_cast<UINT32>(CamX::OsUtils::StrLen(pGPSProcessingMethod)) + 1);

            tagInfo.entry.type              = EXIFTAGTYPE_GPS_PROCESSINGMETHOD;
            tagInfo.entry.count             = count;
            tagInfo.entry.copy              = 1;
            tagInfo.entry.data._undefined   = reinterpret_cast<UINT8*>(gpsProcessingMethod);
            tagInfo.id                      = EXIFTAGID_GPS_PROCESSINGMETHOD;
            SetEXIFTagInfo(tagInfo);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::ParseGPSCoordinate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFParams::ParseGPSCoordinate(
    const CHAR* pCoordinatesStr,
    URAT32*     pCoordinates)
{
    CamxResult  result = CamxResultSuccess;

    if ((NULL != pCoordinates) && (NULL != pCoordinatesStr))
    {
        DOUBLE degree = atof(pCoordinatesStr);
        if (degree < 0)
        {
            degree = -degree;
        }
        DOUBLE minutes = (degree - static_cast<INT32>(degree)) * 60;
        DOUBLE seconds = (minutes - static_cast<INT32>(minutes)) * 60;

        JPEGUtil::GetUnsignedRational(&pCoordinates[0], static_cast<INT32>(degree), 1);
        JPEGUtil::GetUnsignedRational(&pCoordinates[1], static_cast<INT32>(minutes), 1);
        JPEGUtil::GetUnsignedRational(&pCoordinates[2], static_cast<INT32>(seconds * 10000), 10000);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "invalid argument coordinate = NULL");
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFLatitude
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetEXIFLatitude(
    DOUBLE value)
{
    CHAR            str[30]                                     = {0};
    URAT32          latitude[GPSLatitudeSize]                   = {};
    CHAR            latitudeReference[GPSLatitudeReferenceSize] = {};
    EXIFTagInfo     tagInfo                                     = {};

    OsUtils::SNPrintF(str, sizeof(str), "%f", value);
    if ('\0' != str[0])
    {
        ParseGPSCoordinate(str, latitude);

        //set Latitude Ref
        FLOAT latitudeValue = strtof(str, 0);
        if (latitudeValue < 0.0f)
        {
            latitudeReference[0] = 'S';
        }
        else
        {
            latitudeReference[0] = 'N';
        }
        latitudeReference[1] = '\0';

        tagInfo.entry.type          = EXIF_RATIONAL;
        tagInfo.entry.count         = GPSLatitudeSize;
        tagInfo.entry.copy          = 1;
        tagInfo.entry.data._rats    = reinterpret_cast<URAT32*>(latitude);
        tagInfo.id                  = EXIFTAGID_GPS_LATITUDE;
        SetEXIFTagInfo(tagInfo);

        Utils::Memset(&tagInfo, 0x0, sizeof(EXIFTagInfo));
        tagInfo.entry.type          = EXIF_ASCII;
        tagInfo.entry.count         = GPSLatitudeReferenceSize;
        tagInfo.entry.copy          = 1;
        tagInfo.entry.data._ascii   = reinterpret_cast<CHAR*>(latitudeReference);
        tagInfo.id                  = EXIFTAGID_GPS_LATITUDE_REF;
        SetEXIFTagInfo(tagInfo);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFLongitude
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetEXIFLongitude(
    DOUBLE value)
{
    CHAR            str[30]                                       = {0};
    URAT32          longitude[GPSLongitudeSize]                   = {};
    CHAR            longitudeReference[GPSLongitudeReferenceSize] = {};
    EXIFTagInfo     tagInfo                                       = {};

    OsUtils::SNPrintF(str, sizeof(str), "%f", value);
    if ('\0' != str[0])
    {
        ParseGPSCoordinate(str, longitude);

        //set Latitude Ref
        FLOAT latitudeValue = strtof(str, 0);
        if (latitudeValue < 0.0f)
        {
            longitudeReference[0] = 'W';
        }
        else
        {
            longitudeReference[0] = 'E';
        }
        longitudeReference[1] = '\0';

        tagInfo.entry.type          = EXIF_RATIONAL;
        tagInfo.entry.count         = GPSLongitudeSize;
        tagInfo.entry.copy          = 1;
        tagInfo.entry.data._rats    = reinterpret_cast<URAT32*>(longitude);
        tagInfo.id                  = EXIFTAGID_GPS_LONGITUDE;
        SetEXIFTagInfo(tagInfo);

        Utils::Memset(&tagInfo, 0x0, sizeof(EXIFTagInfo));
        tagInfo.entry.type          = EXIF_ASCII;
        tagInfo.entry.count         = GPSLongitudeReferenceSize;
        tagInfo.entry.copy          = 1;
        tagInfo.entry.data._ascii   = reinterpret_cast<CHAR*>(longitudeReference);
        tagInfo.id                  = EXIFTAGID_GPS_LONGITUDE_REF;
        SetEXIFTagInfo(tagInfo);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFAltitude
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetEXIFAltitude(
    DOUBLE value)
{
    CHAR            str[30]             = {0};
    EXIFTagInfo     tagInfo             = {};
    URAT32          altitude            = {};
    BYTE            altitudeReference   = 0;
    CamxResult      result              = CamxResultSuccess;

    OsUtils::SNPrintF(str, sizeof(str), "%f", value);
    if ('\0' != str[0])
    {
        DOUBLE floatValue = atof(str);
        altitudeReference = 0;
        if (floatValue < 0)
        {
            altitudeReference    = 1;
            floatValue           = -floatValue;
        }

        result = JPEGUtil::GetUnsignedRational(&altitude, static_cast<INT32>(floatValue * 1000), 1000);
        if (CamxResultSuccess == result)
        {
            tagInfo.entry.type          = EXIF_RATIONAL;
            tagInfo.entry.count         = GPSAltitudeSize;
            tagInfo.entry.copy          = 0;
            tagInfo.entry.data._rat     = altitude;
            tagInfo.id                  = EXIFTAGID_GPS_ALTITUDE;
            SetEXIFTagInfo(tagInfo);

            Utils::Memset(&tagInfo, 0x0, sizeof(EXIFTagInfo));
            tagInfo.entry.type          = EXIF_BYTE;
            tagInfo.entry.count         = GPSAltitudeReferenceSize;
            tagInfo.entry.copy          = 0;
            tagInfo.entry.data._byte    = altitudeReference;
            tagInfo.id                  = EXIFTAGID_GPS_ALTITUDE_REF;
            SetEXIFTagInfo(tagInfo);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFGPSCoordinates
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetEXIFGPSCoordinates()
{
    if (0 != m_pNode->GetDataCountFromPipeline(InputJPEGGpsCoordinates, 0, m_pNode->GetPipelineId(), FALSE))
    {
        EXIFTagInfo       tagInfo                = {};
        static const UINT JpegTag[]              = { InputJPEGGpsCoordinates };
        const UINT        length                 = CAMX_ARRAY_SIZE(JpegTag);
        VOID*             pData[length]          = { 0 };
        UINT64            jpegDataOffset[length] = { 0 };

        m_pNode->GetDataList(JpegTag, pData, jpegDataOffset, length);
        Utils::Memcpy(m_dataEXIF.dataGPS.gpsCoordinates, pData[0],
                      sizeof(DOUBLE) * static_cast<UINT32>(GPSCoordinateType::GPSMAX));
        DOUBLE* pGPSCoordinates = m_dataEXIF.dataGPS.gpsCoordinates;

        // Get GPS Latitude
        GetEXIFLatitude(pGPSCoordinates[static_cast<UINT32>(GPSCoordinateType::GPSLATITUDE)]);

        // Get GPS Longitude
        GetEXIFLongitude(pGPSCoordinates[static_cast<UINT32>(GPSCoordinateType::GPSLONGITUDE)]);

        // Get GPS Altitude
        GetEXIFAltitude(pGPSCoordinates[static_cast<UINT32>(GPSCoordinateType::GPSALTITUDE)]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFGPSDateTimeStamp
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetEXIFGPSDateTimeStamp()
{
    if (0 != m_pNode->GetDataCountFromPipeline(InputJPEGGpsTimestamp, 0, m_pNode->GetPipelineId(), FALSE))
    {
        CHAR              GPSDateStamp[20]       = {0};
        URAT32            GPSTimeStamp[3]        = {};
        CHAR              str[30]                = {0};
        EXIFTagInfo       tagInfo                = {};
        static const UINT JpegTag[]              = { InputJPEGGpsTimestamp };
        const UINT        length                 = CAMX_ARRAY_SIZE(JpegTag);
        VOID*             pData[length]          = { 0 };
        UINT64            jpegDataOffset[length] = { 0 };

        m_pNode->GetDataList(JpegTag, pData, jpegDataOffset, length);
        m_dataEXIF.dataGPS.gpsTimeStamp = *reinterpret_cast<INT64*>(pData[0]);

        OsUtils::SNPrintF(str, sizeof(str), "%lld", static_cast<long long int>(m_dataEXIF.dataGPS.gpsTimeStamp));

        if ('\0' != str[0])
        {
            struct tm   TimestampUTCResult    = {};
            time_t      unixTime              = (time_t)atol(str);

            if (0 == CamX::OsUtils::GetGMTime(&unixTime, &TimestampUTCResult))
            {
                strftime(GPSDateStamp, 20, "%Y:%m:%d", &TimestampUTCResult);

                JPEGUtil::GetUnsignedRational(&GPSTimeStamp[0], TimestampUTCResult.tm_hour, 1);
                JPEGUtil::GetUnsignedRational(&GPSTimeStamp[1], TimestampUTCResult.tm_min, 1);
                JPEGUtil::GetUnsignedRational(&GPSTimeStamp[2], TimestampUTCResult.tm_sec, 1);

                tagInfo.entry.type          = EXIF_ASCII;
                tagInfo.entry.count         = static_cast<UINT32>(CamX::OsUtils::StrLen(GPSDateStamp) + 1);
                tagInfo.entry.copy          = 1;
                tagInfo.entry.data._ascii   = GPSDateStamp;
                tagInfo.id                  = EXIFTAGID_GPS_DATESTAMP;
                SetEXIFTagInfo(tagInfo);

                Utils::Memset(&tagInfo, 0x0, sizeof(EXIFTagInfo));
                tagInfo.entry.type          = EXIF_RATIONAL;
                tagInfo.entry.count         = 3;
                tagInfo.entry.copy          = 1;
                tagInfo.entry.data._rats    = GPSTimeStamp;
                tagInfo.id                  = EXIFTAGID_GPS_TIMESTAMP;
                SetEXIFTagInfo(tagInfo);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFGPSInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetEXIFGPSInfo()
{
    CAMX_ENTRYEXIT(CamxLogGroupJPEG);

    // Get GPS processing method
    GetEXIFGPSProcessingMethod();

    // Get GPS coordinates
    GetEXIFGPSCoordinates();

    // Get GPS time stamp
    GetEXIFGPSDateTimeStamp();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGUtil::GetJPEGQualityAndUpdateQuantizationTable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetJPEGQualityAndUpdateQuantizationTable()
{
    static const UINT JpegTag[] = { InputJPEGQuality };
    VOID* pData[1] = { 0 };
    UINT length = CAMX_ARRAY_SIZE(JpegTag);
    UINT64 jpegDataOffset[1] = { 0 };

    if (NULL != m_pQuantTables)
    {
        m_pNode->GetDataList(JpegTag, pData, jpegDataOffset, length);
        if (NULL != pData[0])
        {
            CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "Quality received in exif %d", *reinterpret_cast<BYTE*>(pData[0]));
            UINT32 quality = *reinterpret_cast<BYTE*>(pData[0]);
            if ((1 <= quality) && (100 >= quality))
            {
                m_quality = quality;
            }
            else
            {
                m_quality = DefaultQuality;
            }
        }
        else
        {
            m_quality = DefaultQuality;
        }

        JPEGUtil::UpdateQuantizationTableQuality(m_pQuantTables, m_quality);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Invalid QuantizationTables %p", m_pQuantTables);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGUtil::GetJPEGThumbnailQualityAndUpdateQuantizationTable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetJPEGThumbnailQualityAndUpdateQuantizationTable()
{
    static const UINT JPEGTag[] = { InputJPEGThumbnailQuality };
    VOID* pData[1] = { 0 };
    UINT length = CAMX_ARRAY_SIZE(JPEGTag);
    UINT64 dataOffset[1] = { 0 };

    m_pNode->GetDataList(JPEGTag, pData, dataOffset, length);
    if (NULL != m_pQuantTablesThumbnail)
    {
        UINT32 quality = 0;

        if (NULL != pData[0])
        {
            CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "Thumb Quality received in exif %d", *reinterpret_cast<BYTE*>(pData[0]));
            quality = *reinterpret_cast<BYTE*>(pData[0]);
            if ((1 <= quality) && (100 >= quality))
            {
                m_qualityThumbnail = quality;
            }
            else
            {
                m_qualityThumbnail = DefaultQuality;
                quality = DefaultQuality;
            }
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupJPEG, "Thumb Quality not received, assigning default thumbnail quality");
            m_qualityThumbnail = DefaultQuality;
            quality = DefaultQuality;
        }

        quality = (quality > MaxJpegThumbnailQuality) ? MaxJpegThumbnailQuality : quality; // Clamping to max
        JPEGUtil::UpdateQuantizationTableQuality(m_pQuantTablesThumbnail, quality);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Invalid QuantizationTables %p", m_pQuantTablesThumbnail);
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFImageDimensions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::GetEXIFImageDimensions()
{
    UINT16      outputWidth     = 0;
    UINT16      outputHeight    = 0;
    EXIFTagInfo tagInfo         = {};

    if ((static_cast<UINT32>(GetEXIFImageParams()->imgFormat.rotation) * 90) % 180)
    {
        outputWidth  = static_cast<UINT16>(GetEXIFImageParams()->imgFormat.height);
        outputHeight = static_cast<UINT16>(GetEXIFImageParams()->imgFormat.width);
    }
    else
    {
        outputWidth  = static_cast<UINT16>(GetEXIFImageParams()->imgFormat.width);
        outputHeight = static_cast<UINT16>(GetEXIFImageParams()->imgFormat.height);
    }

    tagInfo.entry.type          = EXIFTagType::EXIF_SHORT;
    tagInfo.entry.count         = 1;
    tagInfo.entry.copy          = 0;
    tagInfo.entry.data._short   = outputWidth;
    tagInfo.id                  = EXIFTAGID_IMAGE_WIDTH;
    SetEXIFTagInfo(tagInfo);

    Utils::Memset(&tagInfo, 0x0, sizeof(EXIFTagInfo));
    tagInfo.entry.type          = EXIFTagType::EXIF_SHORT;
    tagInfo.entry.count         = 1;
    tagInfo.entry.copy          = 0;
    tagInfo.entry.data._short   = outputHeight;
    tagInfo.id                  = EXIFTAGID_IMAGE_LENGTH;
    SetEXIFTagInfo(tagInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFParams::GetEXIFData()
{
    CAMX_ENTRYEXIT(CamxLogGroupJPEG);

    CamxResult result   = CamxResultSuccess;
    UINT32     dataSize = static_cast<UINT32>(EXIFTagOffset::EXIF_TAG_MAX_OFFSET);

    if (m_pEXIFTagData != NULL)
    {
        FlushEXIFTagCopyData(m_pEXIFTagData, static_cast<UINT32>(EXIFTagOffset::EXIF_TAG_MAX_OFFSET));
        Utils::Memset(m_pEXIFTagData, 0x0, dataSize * sizeof(EXIFTagInfo));
    }
    SetDefaultEXIFTagInfo();
    GetEXIFDateTime();

    // If rotation has been handled for the incomming jpeg image, do not write exif orientation data
    if (FALSE == m_bIsGpuNodePresent)
    {
        GetEXIFOrientation();
    }

    GetEXIFCustomQuantizationtables();

    if (TRUE == IsMainAvailable())
    {
        GetJPEGQualityAndUpdateQuantizationTable();
    }

    if (TRUE == IsThumbnailAvailable())
    {
        GetJPEGThumbnailQualityAndUpdateQuantizationTable();
    }
    GetEXIF3ASensorData();
    GetEXIFFlashData();
    GetEXIFGPSInfo();
    GetEXIFImageDimensions();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::SetEXIFVendorTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFParams::SetEXIFVendorTags()
{
    CAMX_ENTRYEXIT(CamxLogGroupJPEG);

    CamxResult result = CamxResultSuccess;

    // Set JPEGQuality, ThumbnailQuality, ThumbnailDimensions
    DimensionCap thumbnailDimensions;

    const VOID*         pData[1]                   = { 0 };
    UINT                pDataCount[1]              = { 0 };
    UINT                outJPEGMetaDataTag[1]      = { 0 };

    if (TRUE == IsMainAvailable())
    {
        // Main JPEG Quality
        outJPEGMetaDataTag[0] = JPEGQuality;
        pData[0]              = &m_quality;
        pDataCount[0]         = 1;
        m_pNode->WriteDataList(outJPEGMetaDataTag, pData, pDataCount, 1);
    }

    if (TRUE == IsThumbnailAvailable())
    {
        // Thumbnail Dimensions
        thumbnailDimensions.width  = m_imageParamsThumb.imgFormat.width;
        thumbnailDimensions.height = m_imageParamsThumb.imgFormat.height;
        outJPEGMetaDataTag[0]      = JPEGThumbnailSize;
        pData[0]                   = &thumbnailDimensions;
        pDataCount[0]              = 2;
        m_pNode->WriteDataList(outJPEGMetaDataTag, pData, pDataCount, 1);

        // JPEG Thumbnail Quality
        outJPEGMetaDataTag[0] = JPEGThumbnailQuality;
        pData[0]              = &m_qualityThumbnail;
        pDataCount[0]         = 1;
        m_pNode->WriteDataList(outJPEGMetaDataTag, pData, pDataCount, 1);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFTagInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EXIFTagInfo* JPEGEXIFParams::GetEXIFTagInfo()
{
    return m_pEXIFTagData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetQuantTable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JPEGQuantTable* JPEGEXIFParams::GetQuantTable(
    QuantTableType type,
    BOOL bThumbnail)
{
    if ((type < QuantTableType::QuantTableMin) || (type >= QuantTableType::QuantTableMax))
    {
        return NULL;
    }
    if (FALSE == bThumbnail)
    {
        return &m_pQuantTables[static_cast<UINT32>(type)];
    }
    else
    {
        return &m_pQuantTablesThumbnail[static_cast<UINT32>(type)];
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetHuffTable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JPEGHuffTable* JPEGEXIFParams::GetHuffTable(
    HuffTableType type)
{
    if ((type < HuffTableType::HuffTableMin) || (type >= HuffTableType::HuffTableMax))
    {
        return NULL;
    }

    return &m_pHuffmanTables[static_cast<UINT32>(type)];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFImageParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EXIFImageParams* JPEGEXIFParams::GetEXIFImageParams()
{
    return &m_imageParams;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetEXIFImageParamsThumb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EXIFImageParams* JPEGEXIFParams::GetEXIFImageParamsThumb()
{
    return &m_imageParamsThumb;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::GetSubsampleCompCount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFParams::GetSubsampleCompCount(
    Format            format,
    JPEGSubsampling*  pSS,
    UINT8*            pCompCount)
{
    CamxResult result = CamxResultSuccess;

    switch (format)
    {
        case Format::Y8:
        case Format::Y16:
            *pCompCount = 1;
            *pSS        = JPEGSubsampling::H2V2;
            break;
        case Format::YUV420NV12:
        case Format::YUV420NV12TP10:
            *pCompCount = 3;
            *pSS        = JPEGSubsampling::H2V2;
            break;
        case Format::YUV420NV21:
        case Format::YUV420NV21TP10:
            *pCompCount = 3;
            *pSS        = JPEGSubsampling::H2V2;
            break;
        case Format::YUV422NV16:
        case Format::YUV422NV16TP10:
            *pCompCount = 3;
            *pSS        = JPEGSubsampling::H2V1;
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Invalid format %d", format);
            result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::FlushEXIFTagCopyData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFParams::FlushEXIFTagCopyData(EXIFTagInfo* pEXIFTagData, UINT32 count)
{
    for (UINT32 i = 0; i < count; i++)
    {
        if (pEXIFTagData[i].isTagSet &&  pEXIFTagData[i].entry.copy)
        {
            switch (pEXIFTagData[i].entry.type)
            {
                case EXIFTagType::EXIF_ASCII:
                    if (NULL != pEXIFTagData[i].entry.data._ascii)
                    {
                        CAMX_DELETE[] pEXIFTagData[i].entry.data._ascii;
                        pEXIFTagData[i].entry.data._ascii = NULL;
                    }
                    break;
                case EXIFTagType::EXIF_UNDEFINED:
                    if (NULL != pEXIFTagData[i].entry.data._undefined)
                    {
                        CAMX_DELETE[] pEXIFTagData[i].entry.data._undefined;
                        pEXIFTagData[i].entry.data._undefined = NULL;
                    }
                    break;
                case EXIFTagType::EXIF_BYTE:
                    if (pEXIFTagData[i].entry.count > 1)
                    {
                        if (NULL != pEXIFTagData[i].entry.data._undefined)
                        {
                            CAMX_DELETE[] pEXIFTagData[i].entry.data._undefined;
                            pEXIFTagData[i].entry.data._undefined = NULL;
                        }
                    }
                    break;
                case EXIFTagType::EXIF_SHORT:
                    if (pEXIFTagData[i].entry.count > 1)
                    {
                        if (NULL != pEXIFTagData[i].entry.data._shorts)
                        {
                            CAMX_DELETE[] pEXIFTagData[i].entry.data._shorts;
                            pEXIFTagData[i].entry.data._shorts = NULL;
                        }
                    }
                    break;
                case EXIFTagType::EXIF_LONG:
                    if (pEXIFTagData[i].entry.count > 1)
                    {
                        if (NULL != pEXIFTagData[i].entry.data._longs)
                        {
                            CAMX_DELETE[] pEXIFTagData[i].entry.data._longs;
                            pEXIFTagData[i].entry.data._longs = NULL;
                        }
                    }
                    break;
                case EXIFTagType::EXIF_RATIONAL:
                    if (pEXIFTagData[i].entry.count > 1)
                    {
                        if (NULL != pEXIFTagData[i].entry.data._rats)
                        {
                            CAMX_DELETE[] pEXIFTagData[i].entry.data._rats;
                            pEXIFTagData[i].entry.data._rats = NULL;
                        }
                    }
                    break;
                case EXIFTagType::EXIF_SLONG:
                    if (pEXIFTagData[i].entry.count > 1)
                    {
                        if (NULL != pEXIFTagData[i].entry.data._slongs)
                        {
                            CAMX_DELETE[] pEXIFTagData[i].entry.data._slongs;
                            pEXIFTagData[i].entry.data._slongs = NULL;
                        }
                    }
                    break;
                case EXIFTagType::EXIF_SRATIONAL:
                    if (pEXIFTagData[i].entry.count > 1)
                    {
                        if (NULL != pEXIFTagData[i].entry.data._srats)
                        {
                            CAMX_DELETE[] pEXIFTagData[i].entry.data._srats;
                            pEXIFTagData[i].entry.data._srats = NULL;
                        }
                    }
                    break;
                default:
                    CAMX_LOG_ERROR(CamxLogGroupJPEG, "Invalid format");
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::JPEGEXIFParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JPEGEXIFParams::JPEGEXIFParams()
{
    m_quality                 = DefaultQuality;
    m_qualityThumbnail        = DefaultQuality;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFParams::~JPEGEXIFParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JPEGEXIFParams::~JPEGEXIFParams()
{
    if (NULL != m_pEXIFTagData)
    {
        FlushEXIFTagCopyData(m_pEXIFTagData, static_cast<UINT32>(EXIFTagOffset::EXIF_TAG_MAX_OFFSET));
        CAMX_DELETE[] m_pEXIFTagData;
        m_pEXIFTagData = NULL;
    }

    if (NULL != m_pQuantTables)
    {
        CAMX_DELETE[] m_pQuantTables;
        m_pQuantTables = NULL;
    }

    if (NULL != m_pQuantTablesThumbnail)
    {
        CAMX_DELETE[] m_pQuantTablesThumbnail;
        m_pQuantTablesThumbnail = NULL;
    }

    if (NULL != m_pHuffmanTables)
    {
        CAMX_DELETE[] m_pHuffmanTables;
        m_pHuffmanTables = NULL;
    }
}

CAMX_NAMESPACE_END
