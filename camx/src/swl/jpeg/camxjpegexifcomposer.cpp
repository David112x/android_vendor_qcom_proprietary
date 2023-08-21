////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxjpegexifcomposer.cpp
/// @brief EXIF composer class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE ENTIRE FILE - legacy code
/// @todo (CAMX-1980) Clean up JPEG legacy code
/// @todo (CAMX-1988) Code clean up apply NC001 coding standard on entire project
/// @todo (CAMX-1989) Code clean up apply GR030 coding standard on entire project

#include "camxjpegexifcomposer.h"
#include "camxjpegexifdefaults.h"
#include "camxhal3module.h"

CAMX_NAMESPACE_BEGIN

static const UINT32 MaxApp1Length         = 0xFFFF;      ///< Max App1 length
static const UINT32 EXIFString            = 0x45786966;  ///< "EXIF" string
static const UINT32 MaxApp2Length         = 0xFFFF;      ///< Max App2 length
static const UINT32 FPXRString            = 0x46505852;  ///< "FPXR" string
static const UINT16 BigEndian             = 0x4D4D;      ///< Big endian marker
static const UINT16 TIFFIdentifier        = 0x002A;      ///< TIFF identifier
static const UINT32 FieldInteropSize      = 12;          ///< 12bytes for fiel interoperbility
static const UINT32 NextIfdOffset         = 4;           ///< Next IFD offset

static const char StatsDebugMagicString[] = "QTI Debug Metadata";                ///< Stats debug data string
static const char MobicatMagicString[]    = "Qualcomm Camera Attributes v2";     ///< Mobicat data string
static const char MIMagicString[]         = "Qualcomm Dual Camera Attributes";   ///< Dual camera string
static const char OEMAppDataMagicString[] = "OEM Camera Image Data Attributes";  ///< OEM EXIF App Data string

/// Jpeg natual zigzag order, extra entries for safety
static const UINT16 jpegNaturalOrder[64 + 16] =
{
    0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63,
    63, 63, 63, 63, 63, 63, 63, 63,
    63, 63, 63, 63, 63, 63, 63, 63
};

static EXIFTagInfo default_tag_r98_version =
{
    {
        EXIFTagType::EXIF_UNDEFINED,        // type
        0,                     // copy
        4,                     // count
        { (char*)default_r98_version }, // data._ascii (initialization applies
                                        // to first member of the union)
    }, // entry
    CONSTRUCT_TAGID(EXIF_TAG_MAX_OFFSET, 0x0002)
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::Initialize()
{
    CamxResult result = CamxResultSuccess;

    m_pAheadBuf = CAMX_NEW UINT8[MaxApp1Length];

    if (NULL == m_pAheadBuf)
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Failed to alloc mem for ahead buffer");
        result = CamxResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::Cleanup
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFComposer::Cleanup()
{
    m_pInputMainBuf             = NULL;
    m_pInputThumbBuffer         = NULL;
    m_pOutputBuf                = NULL;
    m_pEXIFParams               = NULL;
    m_pOutputBufAddr            = NULL;
    m_pInputBufAddr             = NULL;
    m_pInputThumbBufferAddr     = NULL;
    m_outputBufSize             = 0;
    m_currentOffset             = 0;
    m_app1LengthOffset          = 0;
    m_app2LengthOffset          = 0;
    m_TIFFHeaderOffset          = 0;
    m_fieldCount                = 0;
    m_fieldCountOffset          = 0;
    m_thumbnailIFDPointerOffset = 0;
    m_thumbnailOffset           = 0;
    m_thumbnailStreamOffset     = 0;
    m_aheadBufOffset            = 0;
    m_app1Present               = 0;
    m_app2Present               = 0;
    m_pStatsDebugDataAddr       = NULL;
    m_statsDebugDataSize        = 0;
    m_pOEMAppDataAddr           = NULL;
    m_OEMAppDataSize            = 0;
    m_exitAppMarkerInc          = 0;
    m_pStatsDebugHeaderString   = NULL;
    m_JPEGInterChangeLOffset    = 0;
    m_skipThumbnailIfLarge      = FALSE;
    Utils::Memset(m_pAheadBuf, 0x0, MaxApp1Length * sizeof(UINT8));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::AddInputMainBuf
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFComposer::AddInputMainBuf(
    ImageBuffer* pImageBuffer)
{
    m_pInputMainBuf = pImageBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::AddInputThumbBuf
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFComposer::AddInputThumbBuffer(
    ImageBuffer* pImageBuffer)
{
    m_pInputThumbBuffer = pImageBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::AddOutputBuf
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFComposer::AddOutputBuf(
    ImageBuffer* pImageBuffer)
{
    m_pOutputBuf = pImageBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::SetParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFComposer::SetParams(
    JPEGEXIFParams* pParams)
{
    m_pEXIFParams = pParams;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::SetStatsDebugData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFComposer::SetStatsDebugData(
    UINT8*      pStatsDebugDataAddr,
    UINT32      statsDebugDataSize,
    const CHAR* pStatsDebugHeaderString)
{
    m_pStatsDebugDataAddr       = pStatsDebugDataAddr;
    m_statsDebugDataSize        = statsDebugDataSize;
    m_pStatsDebugHeaderString   = pStatsDebugHeaderString;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::SetOEMAppData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFComposer::SetOEMAppData(
    UINT8*      pOEMAppDataAddr,
    UINT32      OEMAppDataSize)
{
    m_pOEMAppDataAddr       = pOEMAppDataAddr;
    m_OEMAppDataSize        = OEMAppDataSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::StartComposition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::StartComposition()
{
    CAMX_ENTRYEXIT(CamxLogGroupJPEG);

    CamxResult result              = CamxResultSuccess;
    UINT32     encoderBufferFilled = 0;
    Camera3JPEGBlob jpegBlob;

    if ((NULL == m_pOutputBuf) || (NULL == m_pAheadBuf) || (NULL == m_pEXIFParams))
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params m_pOutputBuf%p m_pAheadBuf %p m_pEXIFParams%p",
                       m_pOutputBuf, m_pAheadBuf, m_pEXIFParams);
        result = CamxResultEInvalidState;
    }

    if (CamxResultSuccess == result)
    {
        m_pOutputBufAddr = m_pOutputBuf->GetPlaneVirtualAddr(0, 0);

        if (NULL == m_pOutputBufAddr)
        {
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Output buffer Null!");
            result = CamxResultEInvalidPointer;
        }
    }

    if (CamxResultSuccess == result)
    {
        if (NULL != m_pInputMainBuf)
        {
            m_pInputBufAddr = m_pInputMainBuf->GetPlaneVirtualAddr(0, 0);

            if (NULL == m_pInputBufAddr)
            {
                CAMX_LOG_ERROR(CamxLogGroupJPEG, "Input buffer Null!");
                result = CamxResultEInvalidPointer;
            }
        }
        else if (FALSE == m_pEXIFParams->IsMainAvailable())
        {
            CAMX_LOG_INFO(CamxLogGroupJPEG, "Main encoder buffer not set, not needed");
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Fail to get main encoder buffer, not set!!");
        }
    }

    if (CamxResultSuccess == result)
    {
        if (NULL != m_pInputThumbBuffer)
        {
            m_pInputThumbBufferAddr = m_pInputThumbBuffer->GetPlaneVirtualAddr(0, 0);

            if (NULL == m_pInputThumbBufferAddr)
            {
                CAMX_LOG_ERROR(CamxLogGroupJPEG, "Input Thumbnail buffer Null!");
                result = CamxResultEInvalidPointer;
            }
            else
            {
                // cache invalidate thumb input
                m_pInputThumbBuffer->CacheOps(true, false);
            }
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupJPEG, "Thumbnail buffer not set thumbnail, not needed");
        }
    }

    if (CamxResultSuccess == result)
    {
        m_outputBufSize = static_cast<UINT32>(m_pOutputBuf->GetPlaneSize(0));
        if (DataspaceJPEGAPPSegments == m_pEXIFParams->GetDataSpaceThumb())
        {
            SIZE_T requiredSize = m_pEXIFParams->GetMaxRequiredSize();
            if (static_cast<UINT32>(requiredSize) < m_outputBufSize)
            {
                // For DataspaceJPEGAPPSegments consider only the required size
                m_outputBufSize = static_cast<UINT32>(requiredSize);
            }
        }

        /// Emit exif data
        result = EmitEXIF();
    }

    if (CamxResultSuccess == result)
    {
        if (TRUE == m_pEXIFParams->IsMainAvailable())
        {
            encoderBufferFilled = m_pEXIFParams->GetEncodeOutParams()->bufferFilledSize;
            CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "EXIF header done, encoded size %d offset %x",
                             encoderBufferFilled,
                             m_currentOffset);
            if ((m_currentOffset + encoderBufferFilled) > m_outputBufSize)
            {
                CAMX_LOG_ERROR(CamxLogGroupJPEG, "Insufficient output buffer size %d required size %d",
                               m_outputBufSize,
                               m_currentOffset + encoderBufferFilled);
                result = CamxResultEOutOfBounds;
            }
            else if (static_cast<INT32>(encoderBufferFilled) <= 0)
            {
                CAMX_LOG_ERROR(CamxLogGroupJPEG, "Invalid encoded buffer! Encoded size %d",
                               encoderBufferFilled);
                result = CamxResultEOutOfBounds;
            }
        }
    }
    if (CamxResultSuccess == result)
    {
        if (TRUE == m_pEXIFParams->IsMainAvailable())
        {
            // Invalidate the input buffer before doing copy.
            // Since the buffer is configured as cached and it is filled by encoder HW directly.
            // Therefore, it is guranteed the cached data is invalid.
            if (NULL != m_pInputMainBuf)
            {
                result = m_pInputMainBuf->CacheOps(true, false);
            }

            if (CamxResultSuccess == result)
            {
                if (encoderBufferFilled != 0x03FFFFFF)
                {
                    Utils::Memcpy(m_pOutputBufAddr + m_currentOffset, m_pInputBufAddr, encoderBufferFilled);
                }
                else
                {

                    UINT32 skipBytes        = (1 * 1024 * 1024);
                    UINT32 skipBytesAtStart = 0x03FFFFFE;
                    UINT32 totalSize        = 0;
                    UINT8* prevPtr          = m_pInputBufAddr + skipBytesAtStart;
                    UINT8* currPtr          = m_pInputBufAddr + skipBytesAtStart + skipBytes;
                    UINT8* endPtr           = m_pInputBufAddr + m_outputBufSize;

                    while (endPtr >= currPtr)
                    {
                        while ((*(currPtr) == 0x00 && *(currPtr + 1) == 0x00) == FALSE)
                        {
                            prevPtr = currPtr;
                            currPtr += skipBytes;
                        }

                        while ((prevPtr < currPtr) && ((*(currPtr) == 0xD9 && *(currPtr - 1) == 0xFF) == FALSE))
                        {
                            currPtr--;
                        }

                        if (prevPtr == currPtr)
                        {
                            prevPtr += skipBytes;
                            currPtr = prevPtr + skipBytes;
                        }
                        else
                        {
                            break;
                        }
                    }

                    if (endPtr < currPtr)
                    {
                        currPtr = endPtr - 1;

                        while ((prevPtr < currPtr) && ((*(currPtr) == 0xD9 && *(currPtr - 1) == 0xFF) == FALSE))
                        {
                            currPtr--;
                        }

                        if (prevPtr == currPtr)
                        {
                            currPtr = endPtr - 1;
                            CAMX_LOG_ERROR(CamxLogGroupJPEG, "EOI marker not found in encoded data");
                        }
                    }

                    totalSize = currPtr - m_pInputBufAddr + 1;

                    encoderBufferFilled = totalSize;

                    CAMX_LOG_INFO(CamxLogGroupJPEG, " SW-1MB---END find Num of bytes extra 0x%0lx Total size %lx",
                                   totalSize - 0x03FFFFFE, totalSize);

                    Utils::Memcpy(m_pOutputBufAddr + m_currentOffset, m_pInputBufAddr, totalSize);

                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupJPEG, "Failed to invalidate the cache");
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        if (DataspaceJPEGAPPSegments == m_pEXIFParams->GetDataSpaceThumb())
        {
            jpegBlob.JPEGBlobId = 0x0100;
        }
        else
        {
            jpegBlob.JPEGBlobId = 0x00FF;
        }

        jpegBlob.JPEGBlobSize = m_currentOffset + encoderBufferFilled;

        size_t jpeg_eof_offset = (size_t)(m_outputBufSize - (size_t)sizeof(jpegBlob));
        UINT8* jpeg_eof = &m_pOutputBufAddr[jpeg_eof_offset];

        CAMX_LOG_VERBOSE(CamxLogGroupJPEG,
                      "Write transport header %d %d %d JPEGBlob: ID 0x%x Size %d dataspace: 0x%x",
                      m_outputBufSize,
                      m_currentOffset,
                      encoderBufferFilled,
                      jpegBlob.JPEGBlobId,
                      jpegBlob.JPEGBlobSize,
                      m_pEXIFParams->GetDataSpaceThumb());

        Utils::Memcpy(jpeg_eof, &jpegBlob, sizeof(Camera3JPEGBlob));
    }

#if JPEG_DUMP_TO_FILE
    CHAR file_name[64];
    CamX::OsUtils::SNPrintF(file_name, sizeof(file_name), "%s/jpegaggr.jpg", ConfigFileDirectory);
    CamX::OsUtils::FPrintF(stderr, "Output file for client = %s\n", "/data/misc/camera/jpegaggr.jpeg");

    FILE* pFp = CamX::OsUtils::FOpen(file_name, "w+");
    if (NULL != pFp)
    {
        CamX::OsUtils::FWrite(m_pOutputBufAddr, m_currentOffset + encoderBufferFilled, 1, pFp);
        CamX::OsUtils::FClose(pFp);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "cannot dump image");
    }
#endif // JPEG_DUMP_TO_FILE

    if (CamxResultSuccess == result)
    {
        // Cache flush output buffer
        m_pOutputBuf->CacheOps(false, true);
    }

    Cleanup();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::EmitEXIF
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  JPEGEXIFComposer::EmitEXIF()
{
    CAMX_ENTRYEXIT(CamxLogGroupJPEG);

    CamxResult result = CamxResultSuccess;

    /// Start of image
    if (CamxResultSuccess == result)
    {
        if ((NULL != m_pInputMainBuf) && (TRUE == m_pEXIFParams->IsMainAvailable()))
        {
            result = EmitShort(0xFF00 | M_SOI, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupJPEG, "No main image");
        }
    }

    /// Fill App1
    if (CamxResultSuccess == result)
    {
        result = EmitApp1();
    }

    /// Fill App2
    if ((CamxResultSuccess == result) && (TRUE == HwEnvironment::GetInstance()->GetStaticSettings()->enableJPEGEXIFApp2))
    {
        result = EmitApp2();
    }


    /// flush file header
    if (CamxResultSuccess == result)
    {
        result = FlushFileHeader();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::EmitApp1
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::EmitApp1()
{
    CamxResult result                   = CamxResultSuccess;
    UINT32     lEXIFIfdPointerOffset    = 0;
    UINT32     lGPSIfdPointerOffset     = 0;
    UINT32     lInteropIfdPointerOffset = 0;

    if (CamxResultSuccess == result)
    {
        result = EmitShort(0xFF00 | M_APP1, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        /// length will be calculated after all operations. bypass for now
        m_app1LengthOffset = m_currentOffset;
        m_currentOffset += 2;

        /// Write "Exif" app string, followed by 0x0 for padding
        result = EmitLong(EXIFString, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        result = EmitShort(0x0, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        m_TIFFHeaderOffset = m_currentOffset;
        result = EmitShort(BigEndian, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    /// TIFF header
    if (CamxResultSuccess == result)
    {
        result = EmitShort(TIFFIdentifier, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    /// set 0x00000008 if TIFF is immediately followed by 0thIFD
    if (CamxResultSuccess == result)
    {
        result = EmitLong(0x00000008, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    /// Write 0th Ifd
    if (CamxResultSuccess == result)
    {
        result = Emit0thIFD(&lEXIFIfdPointerOffset, &lGPSIfdPointerOffset);
    }

    /// Make sure the starting offset is at 2-byte boundary.
    if (CamxResultSuccess == result)
    {
        if (m_currentOffset & 1)
        {
            result = EmitByte(0x0, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
        }
    }

    /// Go back and update EXIF_IFD pointer
    if (CamxResultSuccess == result)
    {
        lEXIFIfdPointerOffset += 8;
        result = EmitLong(m_currentOffset - m_TIFFHeaderOffset, m_pOutputBufAddr, &lEXIFIfdPointerOffset, m_outputBufSize,
            TRUE, __LINE__);
    }

    // Write Exif Ifd
    if (CamxResultSuccess == result)
    {
        result = EmitExifIFD(&lInteropIfdPointerOffset);
    }

    /// Go back and update INTEROP_IFD pointer
    if (CamxResultSuccess == result)
    {
        lInteropIfdPointerOffset += 8;
        result = EmitLong(m_currentOffset - m_TIFFHeaderOffset, m_pOutputBufAddr, &lInteropIfdPointerOffset, m_outputBufSize,
            TRUE, __LINE__);
    }

    /// Write Interoperability Ifd
    if (CamxResultSuccess == result)
    {
        result = EmitInteropIFD();
    }

    /// Make sure the starting offset is at 2-byte boundary.
    if (CamxResultSuccess == result)
    {
        if (m_currentOffset & 1)
        {
            result = EmitByte(0, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
        }
    }

    /// Go back and update GPS_IFD pointer
    if (CamxResultSuccess == result)
    {
        lGPSIfdPointerOffset += 8;
        result = EmitLong(m_currentOffset - m_TIFFHeaderOffset, m_pOutputBufAddr, &lGPSIfdPointerOffset, m_outputBufSize,
            TRUE, __LINE__);
    }

    /// Write GPS Ifd
    if (CamxResultSuccess == result)
    {
        result = EmitGPSIFD();
    }

    if (CamxResultSuccess == result)
    {
        if (NULL != m_pInputThumbBuffer)
        {
            /// Make sure the starting offset is at 2-byte boundary.
            if (m_currentOffset & 1)
            {
                result = EmitByte(0x0, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
            }

            /// Save the thumbnail starting offset
            if (CamxResultSuccess == result)
            {
                m_thumbnailOffset = m_currentOffset;
                /// Checking if thumbnail size + remaining APP1 size does not exceed app marker size, if so drop thumbnail
                if (((m_currentOffset - m_app1LengthOffset) +
                     m_pEXIFParams->GetEncodeOutParamsThumbnail()->bufferFilledSize) <= 0xFFFF)
                {
                    result = EmitThumbnailIFD();
                    if (CamxResultSuccess == result)
                    {
                        if (((m_currentOffset - m_app1LengthOffset) +
                             m_pEXIFParams->GetEncodeOutParamsThumbnail()->bufferFilledSize) > 0xFFFF)
                        {
                            m_currentOffset = m_thumbnailOffset;
                            m_skipThumbnailIfLarge = TRUE;
                            CAMX_LOG_WARN(CamxLogGroupJPEG, "Skip Large Thumbnail");
                        }
                    }
                }
                else
                {
                    m_skipThumbnailIfLarge = TRUE;
                    CAMX_LOG_WARN(CamxLogGroupJPEG, "Skip Large Thumbnail");
                }
            }
        }
    }

    /// Update App1 length field
    if (CamxResultSuccess == result)
    {
        UINT32 app1LengthOffset = m_app1LengthOffset;
        result = EmitShort(static_cast<UINT16>(m_currentOffset - m_app1LengthOffset), m_pOutputBufAddr, &app1LengthOffset,
            m_outputBufSize, TRUE, __LINE__);
    }

    // Updating Thumbnail related EXIF params if Thumbnail is present
    if ((CamxResultSuccess == result) && (NULL != m_pInputThumbBufferAddr) && (0 != m_thumbnailStreamOffset) &&
        (FALSE == m_skipThumbnailIfLarge))
    {
        result = EmitLong((m_currentOffset - m_thumbnailStreamOffset) +
            m_pEXIFParams->GetEncodeOutParamsThumbnail()->bufferFilledSize,
            m_pOutputBufAddr, &m_JPEGInterChangeLOffset,
            m_outputBufSize, TRUE, __LINE__);

        result = EmitShort(static_cast<UINT16>((m_currentOffset - m_app1LengthOffset) +
            m_pEXIFParams->GetEncodeOutParamsThumbnail()->bufferFilledSize),
            m_pOutputBufAddr, &m_app1LengthOffset,
            m_outputBufSize, TRUE, __LINE__);

        // Invalidate the input buffer before doing copy.
        // Since the buffer is configured as cached and it is filled by encoder HW directly.
        // Therefore, it is guaranteed the cached data is invalid.
        if (NULL != m_pInputThumbBuffer)
        {
            result = m_pInputThumbBuffer->CacheOps(true, false);
        }

        Utils::Memcpy(m_pOutputBufAddr + m_currentOffset, m_pInputThumbBufferAddr,
            m_pEXIFParams->GetEncodeOutParamsThumbnail()->bufferFilledSize);
        m_currentOffset += m_pEXIFParams->GetEncodeOutParamsThumbnail()->bufferFilledSize;
    }

    if (CamxResultSuccess == result)
    {
        m_app1Present = TRUE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::EmitApp2
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::EmitApp2()
{
    CamxResult result                   = CamxResultSuccess;

    if (CamxResultSuccess == result)
    {
        result = EmitShort(0xFF00 | M_APP2, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        /// length will be calculated after all operations. bypass for now
        m_app2LengthOffset = m_currentOffset;
        m_currentOffset += 2;

        /// Write "FPXR" string, followed by 0x0 for padding
        result = EmitLong(FPXRString, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        result = EmitShort(0x0, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        // Write 0x0 for version number
        result = EmitShort(0x0, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    /// Update App2 length field
    if (CamxResultSuccess == result)
    {
        UINT32 app2LengthOffset = m_app2LengthOffset;
        result = EmitShort(static_cast<UINT16>(m_currentOffset - m_app2LengthOffset), m_pOutputBufAddr, &app2LengthOffset,
            m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        m_app2Present = TRUE;
    }

    return result;

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::Emit0thIFD
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::Emit0thIFD(
    UINT32* pEXIFIfdPointerOffset,
    UINT32* pGPSIfdPointerOffset)
{
    CamxResult  result      = CamxResultSuccess;
    EXIFTagInfo dummyEntry  = {};

    if ((NULL == pEXIFIfdPointerOffset) || (NULL == pGPSIfdPointerOffset))
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params pEXIFIfdPointerOffset %p pGPSIfdPointerOffset %p", pEXIFIfdPointerOffset,
            pGPSIfdPointerOffset);
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        StartIFD();

        for (UINT32 tagOffset = NEW_SUBFILE_TYPE; tagOffset <= GPS_IFD; tagOffset++)
        {

            result = EmitEXIFTags(m_pEXIFParams->GetEXIFTagInfo() + tagOffset);
            if (result != CamxResultSuccess)
            {
                break;
            }
        }

        /// Save location for later update and emit Exif IFD pointer
        if (CamxResultSuccess == result)
        {
            *pEXIFIfdPointerOffset = m_currentOffset;

            // Craft Exif IFD pointer tag
            dummyEntry.entry.count      = 1;
            dummyEntry.entry.type       = EXIFTAGTYPE_EXIF_IFD_PTR;
            dummyEntry.entry.data._long = 0;
            dummyEntry.id               = EXIFTAGID_EXIF_IFD_PTR;
            dummyEntry.isTagSet         = TRUE;

            result = EmitEXIFTags(&dummyEntry);
        }

        /// Save location for later use and emit GPD Ifd pointer
        if (CamxResultSuccess == result)
        {
            *pGPSIfdPointerOffset = m_currentOffset;
            dummyEntry.entry.count      = 1;
            dummyEntry.entry.type       = EXIFTAGTYPE_EXIF_IFD_PTR;
            dummyEntry.entry.data._long = 0;
            dummyEntry.id               = EXIFTAGID_GPS_IFD_PTR;
            dummyEntry.isTagSet         = TRUE;
            result = EmitEXIFTags(&dummyEntry);
        }

        /// Save location for thumbnail pointer IFD
        if (CamxResultSuccess == result)
        {
            m_thumbnailIFDPointerOffset = m_currentOffset;
            result = FinishIFD();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::EmitExifIFD
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::EmitExifIFD(
    UINT32* pInteropIFDPointerOffset)
{
    EXIFTagInfo dummyEntry      = {};
    UINT32      outputWidth     = 0;
    UINT32      outputHeight    = 0;

    CamxResult result = CamxResultSuccess;

    if (NULL == pInteropIFDPointerOffset)
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params pInteropIFDPointerOffset %p", pInteropIFDPointerOffset);
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        StartIFD();

        /// Emit EXIF IFD
        for (UINT32 tagOffset = EXPOSURE_TIME; tagOffset <= EXIF_COLOR_SPACE; tagOffset++)
        {
            result = EmitEXIFTags(m_pEXIFParams->GetEXIFTagInfo() + tagOffset);
            if (CamxResultSuccess != result)
            {
                break;
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        if (TRUE == m_pEXIFParams->IsMainAvailable())
        {
            outputWidth  = m_pEXIFParams->GetEXIFImageParams()->imgFormat.width;
            outputHeight = m_pEXIFParams->GetEXIFImageParams()->imgFormat.height;

            /// Emit Pixel X and Y Dimension
            dummyEntry.entry.count = 1;
            dummyEntry.entry.type  = EXIF_LONG;
            if ((static_cast<UINT32>(m_pEXIFParams->GetEXIFImageParams()->imgFormat.rotation) * 90) % 180)
            {
                dummyEntry.entry.data._long = outputHeight;
            }
            else
            {
                dummyEntry.entry.data._long = outputWidth;
            }
            dummyEntry.id       = EXIFTAGID_EXIF_PIXEL_X_DIMENSION;
            dummyEntry.isTagSet = TRUE;
            result = EmitEXIFTags(&dummyEntry);
        }
    }

    if (CamxResultSuccess == result)
    {
        if (TRUE == m_pEXIFParams->IsMainAvailable())
        {
            dummyEntry.entry.count = 1;
            dummyEntry.entry.type  = EXIF_LONG;
            if ((static_cast<UINT32>(m_pEXIFParams->GetEXIFImageParams()->imgFormat.rotation) * 90) % 180)
            {
                dummyEntry.entry.data._long = outputWidth;
            }
            else
            {
                dummyEntry.entry.data._long = outputHeight;
            }
            dummyEntry.id       = EXIFTAGID_EXIF_PIXEL_Y_DIMENSION;
            dummyEntry.isTagSet = TRUE;
            result = EmitEXIFTags(&dummyEntry);
        }
    }

    /// Save offset and Emit Interoperability Ifd Pointer
    if (CamxResultSuccess == result)
    {
        *pInteropIFDPointerOffset   = m_currentOffset;
        dummyEntry.entry.count      = 1;
        dummyEntry.entry.type       = EXIF_LONG;
        dummyEntry.entry.data._long = 0;
        dummyEntry.id               = EXIFTAGID_INTEROP_IFD_PTR;
        dummyEntry.isTagSet         = TRUE;

        result = EmitEXIFTags(&dummyEntry);
    }

    /// Continue with the rest of the IFD
    if (CamxResultSuccess == result)
    {
        for (UINT32 tagOffset = RELATED_SOUND_FILE; tagOffset <= PIM; tagOffset++)
        {
            result = EmitEXIFTags(m_pEXIFParams->GetEXIFTagInfo() + tagOffset);
            if (CamxResultSuccess != result)
            {
                break;
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        result = FinishIFD();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::EmitInteropIFD
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::EmitInteropIFD()
{
    CamxResult result = CamxResultSuccess;

    if (CamxResultSuccess == result)
    {
        StartIFD();

        /// Emit Interoperability Index
        default_tag_interopindexstr.isTagSet = TRUE;

        result = EmitEXIFTags(&default_tag_interopindexstr);
    }

    /// Emit Exif R98 Version
    if (CamxResultSuccess == result)
    {
        default_tag_r98_version.isTagSet = TRUE;

        result = EmitEXIFTags(&default_tag_r98_version);
    }

    if (CamxResultSuccess == result)
    {
        result = FinishIFD();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::EmitGPSIFD
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::EmitGPSIFD()
{
    CamxResult result = CamxResultSuccess;

    if (CamxResultSuccess == result)
    {
        StartIFD();

        // Emit tags in GPS IFD
        for (UINT32 tagOffset = GPS_VERSION_ID; tagOffset <= GPS_DIFFERENTIAL; tagOffset++)
        {
            result = EmitEXIFTags(m_pEXIFParams->GetEXIFTagInfo() + tagOffset);
            if (CamxResultSuccess != result)
            {
                break;
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        result = FinishIFD();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::EmitThumbnailIFD
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::EmitThumbnailIFD()
{
    CamxResult  result                  = CamxResultSuccess;
    EXIFTagInfo dummyEntry              = {};
    UINT32      JPEGInterChangeOffset   = 0;
    UINT32      outputWidth             = 0;
    UINT32      outputHeight            = 0;

    /// Update Thumbnail_IFD pointer
    if (CamxResultSuccess == result)
    {
        result = EmitLong(m_currentOffset - m_TIFFHeaderOffset, m_pOutputBufAddr, &m_thumbnailIFDPointerOffset,
            m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        StartIFD();

        /// Emit Pixel X and Y Dimension
        dummyEntry.entry.count      = 1;
        dummyEntry.entry.type       = EXIF_LONG;
        dummyEntry.entry.data._long = 0;
        dummyEntry.isTagSet         = TRUE;

        outputWidth  = m_pEXIFParams->GetEXIFImageParamsThumb()->imgFormat.width;
        outputHeight = m_pEXIFParams->GetEXIFImageParamsThumb()->imgFormat.height;

        // Emit tags in TIFF IFD for thumbnail
        for (UINT32 tagOffset = TN_IMAGE_WIDTH; tagOffset <= TN_COPYRIGHT; tagOffset++)
        {
            // Save offset to 'Offset to JPEG SOI'
            if (TN_JPEGINTERCHANGE_FORMAT == tagOffset)
            {
                dummyEntry.entry.count      = 1;
                dummyEntry.entry.type       = EXIF_LONG;
                dummyEntry.entry.data._long = 0;
                dummyEntry.isTagSet         = TRUE;

                JPEGInterChangeOffset = m_currentOffset + 8;
                dummyEntry.id         = EXIFTAGID_TN_JPEGINTERCHANGE_FORMAT;

                result = EmitEXIFTags(&dummyEntry);
            }
            else if (TN_JPEGINTERCHANGE_FORMAT_L == tagOffset)
            {
                dummyEntry.entry.count      = 1;
                dummyEntry.entry.type       = EXIF_LONG;
                dummyEntry.entry.data._long = 0;
                dummyEntry.isTagSet         = TRUE;

                m_JPEGInterChangeLOffset = m_currentOffset + 8;
                dummyEntry.id            = EXIFTAGID_TN_JPEGINTERCHANGE_FORMAT_L;

                result = EmitEXIFTags(&dummyEntry);
            }
            else if (TN_IMAGE_WIDTH == tagOffset)
            {
                if (TRUE == m_pEXIFParams->IsMainAvailable())
                {
                    /// Emit Pixel X and Y Dimension
                    dummyEntry.entry.count = 1;
                    dummyEntry.entry.type  = EXIF_LONG;
                    if ((static_cast<UINT32>(m_pEXIFParams->GetEXIFImageParams()->imgFormat.rotation) * 90) % 180)
                    {
                        dummyEntry.entry.data._long = outputHeight;
                    }
                    else
                    {
                        dummyEntry.entry.data._long = outputWidth;
                    }
                    dummyEntry.id       = EXIFTAGID_EXIF_PIXEL_X_DIMENSION;
                    dummyEntry.isTagSet = TRUE;
                    result = EmitEXIFTags(&dummyEntry);
                }
            }
            else if (TN_IMAGE_LENGTH == tagOffset)
            {
                if (TRUE == m_pEXIFParams->IsMainAvailable())
                {
                    dummyEntry.entry.count = 1;
                    dummyEntry.entry.type  = EXIF_LONG;
                    if ((static_cast<UINT32>(m_pEXIFParams->GetEXIFImageParams()->imgFormat.rotation) * 90) % 180)
                    {
                        dummyEntry.entry.data._long = outputWidth;
                    }
                    else
                    {
                        dummyEntry.entry.data._long = outputHeight;
                    }
                    dummyEntry.id       = EXIFTAGID_EXIF_PIXEL_Y_DIMENSION;
                    dummyEntry.isTagSet = TRUE;
                    result = EmitEXIFTags(&dummyEntry);
                }
            }
            else
            {
                result = EmitEXIFTags(m_pEXIFParams->GetEXIFTagInfo() + tagOffset);
            }

            if (CamxResultSuccess != result)
            {
                break;
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        result = FinishIFD();
    }

    /// Save Thumbnail Stream Offset
    if (CamxResultSuccess == result)
    {
        m_thumbnailStreamOffset = m_currentOffset;
        /// Emit SOI
        result = EmitShort(0xFF00 | M_SOI, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    /// Emit Frame Header
    if (CamxResultSuccess == result)
    {
        result = EmitFrameHeader(m_pEXIFParams->GetEXIFImageParamsThumb(), TRUE);
    }

    /// Emit Scan Header
    if (CamxResultSuccess == result)
    {
        result = EmitScanHeader(m_pEXIFParams->GetEXIFImageParamsThumb());
    }

    /// Update Jpeg Interchange Format
    if (CamxResultSuccess == result)
    {
        result = EmitLong(m_thumbnailStreamOffset - m_TIFFHeaderOffset, m_pOutputBufAddr, &JPEGInterChangeOffset,
            m_outputBufSize, TRUE, __LINE__);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::EmitFrameHeader
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::EmitFrameHeader(
    EXIFImageParams* pImgParams,
    BOOL bThumbnail)
{
    CamxResult result = CamxResultSuccess;

    if (NULL == pImgParams)
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params pImgParams %p", pImgParams);
        result = CamxResultEInvalidPointer;
    }

    /// Emit DQT for each quantization table.Note that emit_dqt() suppresses any duplicate tables.
    if (CamxResultSuccess == result)
    {
        result = EmitShort(0xFF00 | M_DQT, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        result = EmitShort(64 * 2 + 2 + 2, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        result = EmitByte(0x0, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        result = EmitDQT(m_pEXIFParams->GetQuantTable(QuantTableType::QuantTableLuma, bThumbnail)->GetTable());
    }

    if (CamxResultSuccess == result)
    {
        result = EmitByte(1, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        result = EmitDQT(m_pEXIFParams->GetQuantTable(QuantTableType::QuantTableChroma, bThumbnail)->GetTable());
    }

    /// SOF code for baseline implementation
    if (CamxResultSuccess == result)
    {
        result = EmitSOF(M_SOF0, pImgParams);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::EmitScanHeader
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::EmitScanHeader(
    EXIFImageParams* pImgParams)
{
    UINT16      length;
    CamxResult  result = CamxResultSuccess;

    if (NULL == pImgParams)
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params pImgParams %p", pImgParams);
        result = CamxResultEInvalidPointer;
    }

    /// Emit Huffman tables.
    if (CamxResultSuccess == result)
    {
        result = EmitShort(0xFF00 | M_DHT, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    /// Compute length
    if (CamxResultSuccess == result)
    {
        length = 0;

        for (UINT32 i = 1; i <= 16; i++)
        {
            length = length + m_pEXIFParams->GetHuffTable(HuffTableType::HuffTableLumaDC)->GetTable()->bits[i];
            length = length + m_pEXIFParams->GetHuffTable(HuffTableType::HuffTableLumaAC)->GetTable()->bits[i];
            length = length + m_pEXIFParams->GetHuffTable(HuffTableType::HuffTableChromaDC)->GetTable()->bits[i];
            length = length + m_pEXIFParams->GetHuffTable(HuffTableType::HuffTableChromaAC)->GetTable()->bits[i];
        }

        /// length + 17 * 4 + 2 will never overflow 16 bits
        result = EmitShort(2 + length + 17 * 4, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        result = EmitDHT(m_pEXIFParams->GetHuffTable(HuffTableType::HuffTableLumaDC)->GetTable(), 0);
    }

    if (CamxResultSuccess == result)
    {
        result = EmitDHT(m_pEXIFParams->GetHuffTable(HuffTableType::HuffTableLumaAC)->GetTable(), 0 | 0x10);
    }

    if (CamxResultSuccess == result)
    {
        result = EmitDHT(m_pEXIFParams->GetHuffTable(HuffTableType::HuffTableChromaDC)->GetTable(), 1);
    }

    if (CamxResultSuccess == result)
    {
        result = EmitDHT(m_pEXIFParams->GetHuffTable(HuffTableType::HuffTableChromaAC)->GetTable(), 1 | 0x10);
    }

    if (CamxResultSuccess == result)
    {
        result = EmitSOS(pImgParams);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::FlushFileHeader
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::FlushFileHeader()
{
    CamxResult  result = CamxResultSuccess;
    const StaticSettings* pStaticSettings    = HwEnvironment::GetInstance()->GetStaticSettings();
    UINT32 neededSize = 0;

    // Emit Stats debug data
    neededSize = m_currentOffset + m_statsDebugDataSize;
    if (TRUE == m_pEXIFParams->IsMainAvailable())
    {
        neededSize += m_pEXIFParams->GetEncodeOutParams()->bufferFilledSize;
    }

    if (neededSize < static_cast<UINT32>(m_pOutputBuf->GetPlaneSize(0))) {

        if (NULL != m_pStatsDebugDataAddr)
        {
            CAMX_LOG_CONFIG(CamxLogGroupDebugData,
                          "DebugDataAll: EXIF writing total: %p, size: %u, 3A enable:  %u (%s), Tuning enable: %u "
                          "Per-module sizes: AEC= %u AWB= %u AF= %u IFE= %u IPE= %u BPS= %u",
                          m_pStatsDebugDataAddr,
                          m_statsDebugDataSize,
                          ((TRUE == pStaticSettings->enable3ADebugData) ||
                           (TRUE == pStaticSettings->enableConcise3ADebugData)),
                          ((TRUE == pStaticSettings->enable3ADebugData) ? "Default"
                          : ((TRUE == pStaticSettings->enableConcise3ADebugData) ? "Concise" : "None")),
                          pStaticSettings->enableTuningMetadata,
                          ((TRUE == pStaticSettings->enable3ADebugData) ? pStaticSettings->debugDataSizeAEC
                                                                        : pStaticSettings->conciseDebugDataSizeAEC),
                          ((TRUE == pStaticSettings->enable3ADebugData) ? pStaticSettings->debugDataSizeAWB
                                                                        : pStaticSettings->conciseDebugDataSizeAWB),
                          ((TRUE == pStaticSettings->enable3ADebugData) ? pStaticSettings->debugDataSizeAF
                                                                        : pStaticSettings->conciseDebugDataSizeAF),
                          pStaticSettings->tuningDumpDataSizeIFE,
                          pStaticSettings->tuningDumpDataSizeIPE,
                          pStaticSettings->tuningDumpDataSizeBPS);
            EmitAppByType(ExifPayloadType::ExifStatsDebugData, m_pStatsDebugDataAddr, m_statsDebugDataSize, 0, 0);
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupDebugData, "DebugDataAll: Not writing 3A debug data or Tuning metadata. 3A enable: %s "
                          "Tuning enable: %u",
                          ((TRUE == pStaticSettings->enable3ADebugData) ? "Default"
                          : ((TRUE == pStaticSettings->enableConcise3ADebugData) ? "Concise" : "None")),
                          pStaticSettings->enableTuningMetadata);
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupDebugData, "Skip Large Debug Data");
    }

    // Emit OEM EXIF App data
    if ((NULL != m_pOEMAppDataAddr) && (0 != m_OEMAppDataSize)) {
        CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "OEM EXIF App Data: EXIF writing total size: %u %p",
            m_OEMAppDataSize, m_pOEMAppDataAddr);
        EmitAppByType(ExifPayloadType::ExifOEMAppData, m_pOEMAppDataAddr, m_OEMAppDataSize, 0, 0);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "OEM EXIF App Data: Not writing");
    }

    /// Emit Frame Header
    if (CamxResultSuccess == result)
    {
        if (TRUE == m_pEXIFParams->IsMainAvailable())
        {
            result = EmitFrameHeader(m_pEXIFParams->GetEXIFImageParams(), FALSE);
        }
    }

    /// Emit Scan Header
    if (CamxResultSuccess == result)
    {
        if (TRUE == m_pEXIFParams->IsMainAvailable())
        {
            result = EmitScanHeader(m_pEXIFParams->GetEXIFImageParams());
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::EmitDQT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::EmitDQT(
    UINT16* pQuantTable)
{
    UINT16      qval;
    CamxResult  result = CamxResultSuccess;

    if (NULL == pQuantTable)
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params pQuantTable %p", pQuantTable);
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        for (UINT32 i = 0; i < 64; i++)
        {
            /* The table entries must be emitted in zigzag order. */
            qval = pQuantTable[jpegNaturalOrder[i]];

            result = EmitByte(qval & 0xFF, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
            if (CamxResultSuccess != result)
            {
                break;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::EmitDHT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::EmitDHT(
    HuffTable* pHuffTable,
    UINT32     index)
{
    UINT32      length = 0;
    CamxResult  result = CamxResultSuccess;

    if (NULL == pHuffTable)
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params pHuffTable %p", pHuffTable);
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        for (UINT32 i = 1; i <= 16; i++)
        {
            length = length + pHuffTable->bits[i];
        }

        result = EmitByte(index & 0xFF, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        for (UINT32 i = 1; i <= 16; i++)
        {
            result = EmitByte(pHuffTable->bits[i], m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
            if (CamxResultSuccess != result)
            {
                break;
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        for (UINT32 i = 0; i < length; i++)
        {
            result = EmitByte(pHuffTable->values[i], m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
            if (CamxResultSuccess != result)
            {
                break;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::EmitSOF
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::EmitSOF(
    UINT8            code,
    EXIFImageParams* pImgParams)
{
    UINT16       numComponents  = 0;
    UINT8        precision      = 0;
    UINT32       outputWidth    = 0;
    UINT32       outputHeight   = 0;
    UINT32       rotation       = 0;
    CamxResult   result         = CamxResultSuccess;

    if (NULL == pImgParams)
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params pImgParams %p", pImgParams);
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        numComponents = pImgParams->numComponents;
        rotation      = static_cast<UINT32>(pImgParams->imgFormat.rotation) * 90;
        precision     = 8;

        /// code
        result = EmitShort(0xFF00 | code, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    /// length
    if (CamxResultSuccess == result)
    {
        result = EmitShort(3 * numComponents + 2 + 5 + 1, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    /// data precision
    if (CamxResultSuccess == result)
    {
        result = EmitByte(precision, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        outputWidth = pImgParams->imgFormat.width;
        outputHeight = pImgParams->imgFormat.height;

        if (rotation % 180)
        {
            result = EmitShort(static_cast<UINT16>(outputWidth), m_pOutputBufAddr, &m_currentOffset, m_outputBufSize,
                TRUE, __LINE__);
            if (CamxResultSuccess == result)
            {
                result = EmitShort(static_cast<UINT16>(outputHeight), m_pOutputBufAddr, &m_currentOffset, m_outputBufSize,
                    TRUE, __LINE__);
            }
        }
        else
        {
            result = EmitShort(static_cast<UINT16>(outputHeight), m_pOutputBufAddr, &m_currentOffset, m_outputBufSize,
                TRUE, __LINE__);
            if (CamxResultSuccess == result)
            {
                result = EmitShort(static_cast<UINT16>(outputWidth), m_pOutputBufAddr, &m_currentOffset, m_outputBufSize,
                    TRUE, __LINE__);
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        result = EmitByte(static_cast<UINT8>(numComponents), m_pOutputBufAddr, &m_currentOffset, m_outputBufSize,
            TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        for (UINT32 i = 0; i < numComponents; i++)
        {
            /// Write component ID
            result = EmitByte(static_cast<UINT8>(i + 1), m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);

            /// Luma
            if (CamxResultSuccess == result)
            {
                if (0 == i)
                {
                    /// Write sampling factors
                    switch (pImgParams->subsampling)
                    {
                        case JPEGSubsampling::H2V2:
                            result = EmitByte(0x22, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
                            break;
                        case JPEGSubsampling::H2V1:
                            if (rotation % 180)
                            {
                                result = EmitByte(0x12, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
                            }
                            else
                            {
                                result = EmitByte(0x21, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
                            }
                            break;
                        case JPEGSubsampling::H1V2:
                            if (rotation % 180)
                            {
                                result = EmitByte(0x21, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
                            }
                            else
                            {
                                result = EmitByte(0x12, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
                            }
                            break;
                        case JPEGSubsampling::H1V1:
                            result = EmitByte(0x11, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
                            break;
                        default:
                            break;
                    }

                    /// Write quantization table selector
                    if (CamxResultSuccess == result)
                    {
                        result = EmitByte(0x0, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
                    }
                }
                else
                {
                    /// Chroma write sampling factors
                    result = EmitByte(0x11, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);

                    /// Write quantization table selector
                    if (CamxResultSuccess == result)
                    {
                        result = EmitByte(0x1, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
                    }
                }
            }

            if (CamxResultSuccess != result)
            {
                break;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::EmitSOS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::EmitSOS(
    EXIFImageParams* pImgParams)
{
    CamxResult   result        = CamxResultSuccess;
    UINT8        numComponents = 0;

    if (NULL == pImgParams)
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params pImgParams %p", pImgParams);
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        numComponents = pImgParams->numComponents;
        result = EmitShort(0xFF00 | M_SOS, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    /// length
    if (CamxResultSuccess == result)
    {
        result = EmitShort(2 * numComponents + 2 + 1 + 3, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    /// number of components in scan
    if (CamxResultSuccess == result)
    {
        result = EmitByte(numComponents, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    /// Y DC AC Table Indices
    if (CamxResultSuccess == result)
    {
        result = EmitShort((1 << 8) | (0 << 4) | 0, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        if (numComponents > 1)
        {
            /// Cb  DC AC Table Indices
            result = EmitShort((2 << 8) | (1 << 4) | 1, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);

            /// Cr  DC AC Table Indices
            if (CamxResultSuccess == result)
            {
                result = EmitShort((3 << 8) | (1 << 4) | 1, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE,
                    __LINE__);
            }
        }
    }

    /// Sub sampling
    if (CamxResultSuccess == result)
    {
        result = EmitShort((0 << 8) | 63, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        result = EmitByte(0, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::EmitEXIFTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::EmitEXIFTags(
    EXIFTagInfo* pTagInfo)
{
    UINT16      tagId;
    UINT32      toWriteLen;
    UINT32      bytesWritten = 0;
    CamxResult  result       = CamxResultSuccess;

    if (NULL == pTagInfo)
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params pTagInfo %p", pTagInfo);
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        if (pTagInfo->isTagSet == FALSE)
        {
            return result;
        }
    }

    if (CamxResultSuccess == result)
    {
        tagId = pTagInfo->id & 0xFFFF;

        /// Write Tag ID
        result = EmitShort(tagId, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    // Write Tag Type
    if (CamxResultSuccess == result)
    {
        result = EmitShort(static_cast<UINT16>(pTagInfo->entry.type), m_pOutputBufAddr, &m_currentOffset, m_outputBufSize,
            TRUE, __LINE__);
    }

    // Write Tag count
    if (CamxResultSuccess == result)
    {
        result = EmitLong(pTagInfo->entry.count, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        // Compute the length that needs to be written
        toWriteLen = EXIFTagTypeSizes[pTagInfo->entry.type] * pTagInfo->entry.count;

        /// If to_write_len <= 4, tag value written along-side taglength and other parameters in the Scratch Buffer.
        /// Else, a pointer to the tag value is written along-side taglength etc in the Scratch Buffer, and the actual
        /// value written in the ahead buffer. Refer to EXIF std for details.
        if (toWriteLen <= 4)
        {
            if ((EXIF_ASCII == pTagInfo->entry.type) || (EXIF_UNDEFINED == pTagInfo->entry.type))
            {
                result = EmitNBytes(reinterpret_cast<UINT8*>(pTagInfo->entry.data._ascii), toWriteLen, m_pOutputBufAddr,
                    &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
                bytesWritten = toWriteLen;
            }
            else
            {
                if (pTagInfo->entry.count > 1)
                {
                    for (UINT32 i = 0; i < pTagInfo->entry.count; i++)
                    {
                        switch (pTagInfo->entry.type)
                        {
                            case EXIF_BYTE:
                                result = EmitByte(pTagInfo->entry.data._bytes[i], m_pOutputBufAddr, &m_currentOffset,
                                    m_outputBufSize, TRUE, __LINE__);
                                bytesWritten++;
                                break;
                            case EXIF_SHORT:
                                result = EmitShort(pTagInfo->entry.data._shorts[i], m_pOutputBufAddr, &m_currentOffset,
                                    m_outputBufSize, TRUE, __LINE__);
                                bytesWritten += 2;
                                break;
                            default:
                                CAMX_LOG_ERROR(CamxLogGroupJPEG, "Invalid entry type %d", pTagInfo->entry.type);
                                result = CamxResultEInvalidArg;
                        }

                        if (CamxResultSuccess != result)
                        {
                            break;
                        }
                    }
                }
                else
                {
                    switch (pTagInfo->entry.type)
                    {
                        case EXIF_BYTE:
                            result = EmitByte(pTagInfo->entry.data._byte, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize,
                                TRUE, __LINE__);
                            bytesWritten = 1;
                            break;
                        case EXIF_SHORT:
                            result = EmitShort(pTagInfo->entry.data._short, m_pOutputBufAddr, &m_currentOffset,
                                m_outputBufSize, TRUE, __LINE__);
                            bytesWritten = 2;
                            break;
                        case EXIF_LONG:
                            result = EmitLong(pTagInfo->entry.data._long, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize,
                                TRUE, __LINE__);
                            bytesWritten = 4;
                            break;
                        case EXIF_SLONG:
                            result = EmitLong(pTagInfo->entry.data._slong, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize,
                                TRUE, __LINE__);
                            bytesWritten = 4;
                            break;
                        default:
                            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Invalid entry type %d", pTagInfo->entry.type);
                            result = CamxResultEInvalidArg;
                    }
                }
            }

            /// Fill up 0's till there are totally 4 bytes written
            if (CamxResultSuccess == result)
            {
                for (UINT32 i = bytesWritten; i < 4; i++)
                {
                    result = EmitByte(0x0, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
                    if (CamxResultSuccess != result)
                    {
                        break;
                    }
                }
            }
        }
        else
        {
            /// if (to_write_len <= 4)
            if (m_aheadBufOffset & 1)
            {
                m_aheadBufOffset++;
            }

            /// Write the temporary offset (to be updated later)
            result = EmitLong(m_aheadBufOffset, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);

            if (CamxResultSuccess == result)
            {
                if ((EXIF_ASCII == pTagInfo->entry.type) || (EXIF_UNDEFINED == pTagInfo->entry.type))
                {
                    result = EmitNBytes(reinterpret_cast<UINT8*>(pTagInfo->entry.data._ascii), pTagInfo->entry.count,
                        m_pAheadBuf, &m_aheadBufOffset, MaxApp1Length, TRUE, __LINE__);
                }
                else if (pTagInfo->entry.count > 1)
                {
                    // Multiple data to write
                    for (UINT32 i = 0; i < pTagInfo->entry.count; i++)
                    {
                        switch (pTagInfo->entry.type)
                        {
                            case EXIF_BYTE:
                                result = EmitByte(pTagInfo->entry.data._bytes[i], m_pAheadBuf, &m_aheadBufOffset,
                                    MaxApp1Length, TRUE, __LINE__);
                                break;
                            case EXIF_SHORT:
                                result = EmitShort(pTagInfo->entry.data._shorts[i], m_pAheadBuf, &m_aheadBufOffset,
                                    MaxApp1Length, TRUE, __LINE__);
                                break;
                            case EXIF_LONG:
                                result = EmitLong(pTagInfo->entry.data._longs[i], m_pAheadBuf, &m_aheadBufOffset,
                                    MaxApp1Length, TRUE, __LINE__);
                                break;
                            case EXIF_SLONG:
                                result = EmitLong(pTagInfo->entry.data._slongs[i], m_pAheadBuf, &m_aheadBufOffset,
                                    MaxApp1Length, TRUE, __LINE__);
                                break;
                            case EXIF_RATIONAL:
                                result = EmitLong(pTagInfo->entry.data._rats[i].numerator, m_pAheadBuf, &m_aheadBufOffset,
                                    MaxApp1Length, TRUE, __LINE__);
                                if (result == CamxResultSuccess)
                                {
                                    result = EmitLong(pTagInfo->entry.data._rats[i].denominator, m_pAheadBuf,
                                        &m_aheadBufOffset, MaxApp1Length, TRUE, __LINE__);
                                }
                                break;
                            case EXIF_SRATIONAL:
                                result = EmitLong(pTagInfo->entry.data._srats[i].numerator, m_pAheadBuf, &m_aheadBufOffset,
                                    MaxApp1Length, TRUE, __LINE__);
                                if (result == CamxResultSuccess)
                                {
                                    result = EmitLong(pTagInfo->entry.data._srats[i].denominator, m_pAheadBuf,
                                        &m_aheadBufOffset, MaxApp1Length, TRUE, __LINE__);
                                }
                                break;
                            default:
                                break;
                        }

                        if (CamxResultSuccess != result)
                        {
                            break;
                        }
                    }
                }
                else
                {
                    switch (pTagInfo->entry.type)
                    {
                        case EXIF_BYTE:
                            result = EmitByte(pTagInfo->entry.data._byte, m_pAheadBuf, &m_aheadBufOffset, MaxApp1Length, TRUE,
                                __LINE__);
                            break;
                        case EXIF_SHORT:
                            result = EmitShort(pTagInfo->entry.data._short, m_pAheadBuf, &m_aheadBufOffset, MaxApp1Length,
                                TRUE, __LINE__);
                            break;
                        case EXIF_LONG:
                            result = EmitLong(pTagInfo->entry.data._long, m_pAheadBuf, &m_aheadBufOffset, MaxApp1Length, TRUE,
                                __LINE__);
                            break;
                        case EXIF_SLONG:
                            result = EmitLong(pTagInfo->entry.data._slong, m_pAheadBuf, &m_aheadBufOffset, MaxApp1Length, TRUE,
                                __LINE__);
                            break;
                        case EXIF_RATIONAL:
                            result = EmitLong(pTagInfo->entry.data._rat.numerator, m_pAheadBuf, &m_aheadBufOffset,
                                MaxApp1Length, TRUE, __LINE__);
                            if (CamxResultSuccess == result)
                            {
                                result = EmitLong(pTagInfo->entry.data._rat.denominator, m_pAheadBuf, &m_aheadBufOffset,
                                    MaxApp1Length, TRUE, __LINE__);
                            }
                            break;
                        case EXIF_SRATIONAL:
                            result = EmitLong(pTagInfo->entry.data._srat.numerator, m_pAheadBuf, &m_aheadBufOffset,
                                MaxApp1Length, TRUE, __LINE__);
                            if (CamxResultSuccess == result)
                            {
                                EmitLong(pTagInfo->entry.data._srat.denominator, m_pAheadBuf, &m_aheadBufOffset, MaxApp1Length,
                                    TRUE, __LINE__);
                            }
                            break;
                        default:
                            break;
                    }
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            m_fieldCount++;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::StartIFD
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEXIFComposer::StartIFD()
{
    m_fieldCount = 0;
    m_fieldCountOffset = m_currentOffset;

    m_currentOffset += 2;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::FinishIFD
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::FinishIFD()
{
    CamxResult  result                = CamxResultSuccess;
    UINT32      aheadBufDestination   = 0;         ///< the offset in scratch buffer where ahead buffer should be copied to

    /// Emit Next IFD pointer
    if (CamxResultSuccess == result)
    {
        result = EmitLong(0, m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    /// Update the number of fields written and flush the ahead buffer to scratch buffer
    if (CamxResultSuccess == result)
    {
        result = EmitShort(static_cast<UINT16>(m_fieldCount), m_pOutputBufAddr, &m_fieldCountOffset, m_outputBufSize, TRUE,
            __LINE__);
    }

    /// extra 4 bytes for NextIfdOffset
    if (CamxResultSuccess == result)
    {
        aheadBufDestination = m_fieldCountOffset + m_fieldCount * FieldInteropSize + NextIfdOffset;

        for (UINT32 i = 0; i < m_fieldCount; i++)
        {
            UINT32 aheadOffset;
            UINT32 writeOffset = m_fieldCountOffset + i * FieldInteropSize + 8;

            /// update offset if dataLen is greater than 4
            const UINT32 type  = ReadShort(m_pOutputBufAddr, writeOffset - 6);
            const UINT32 count = ReadLong(m_pOutputBufAddr, writeOffset - 4);

            /// tag_type_sizes is with size 11
            /// valid type is 0 to 10
            if ((type < EXIF_MAX_TYPE) && count * EXIFTagTypeSizes[type] > 4)
            {
                aheadOffset = ReadLong(m_pOutputBufAddr, writeOffset);

                /// adjust offset by adding
                result = EmitLong(aheadOffset + aheadBufDestination - m_TIFFHeaderOffset, m_pOutputBufAddr, &writeOffset,
                    m_outputBufSize, TRUE, __LINE__);
            }

            if (CamxResultSuccess != result)
            {
                break;
            }
        }
    }

    /// copy ahead buffer to scratch buffer
    if (CamxResultSuccess == result)
    {
        Utils::Memcpy(m_pOutputBufAddr + aheadBufDestination, m_pAheadBuf, m_aheadBufOffset);
        m_currentOffset     = aheadBufDestination + m_aheadBufOffset;
        m_aheadBufOffset    = 0;
        m_fieldCount        = 0;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::EmitByte
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::EmitByte(
    UINT8    value,
    UINT8*   pBuffer,
    UINT32*  pOffset,
    UINT32   bufferSize,
    BOOL     endian,
    UINT32   lineNum)
{
    UINT32     shift      = sizeof(UINT8) * 8;
    UINT32     offset     = 0;
    UINT32     endOffset  = 0;
    CamxResult result     = CamxResultSuccess;

    if ((NULL == pBuffer) || (NULL == pOffset))
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params pBuffer %p pOffset %p", pBuffer, pOffset);
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        offset    = *pOffset;
        endOffset = offset + sizeof(UINT8);

        if (endOffset - 1 >= bufferSize)
        {
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Line %d: Buffer overflow buffersize %d, endOffset-1 = %d", lineNum,
                bufferSize, endOffset - 1);
            result = CamxResultEOutOfBounds;
        }
    }

    if (CamxResultSuccess == result)
    {
        if (TRUE == endian)
        {
            do
            {
                shift = shift - 8;
                pBuffer[offset] = static_cast <UINT8>((value >> shift) & 0xFF);
                offset++;
            } while (shift);
        }
        else
        {
            offset = endOffset - 1;
            do
            {
                shift = shift - 8;
                pBuffer[offset] = static_cast <UINT8>((value >> shift) & 0xFF);
                offset--;
            } while (shift);
        }

        *pOffset = endOffset;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::EmitShort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::EmitShort(
    UINT16    value,
    UINT8*    pBuffer,
    UINT32*   pOffset,
    UINT32    bufferSize,
    BOOL      endian,
    UINT32    lineNum)
{
    UINT32     shift      = sizeof(UINT16) * 8;
    UINT32     offset     = 0;
    UINT32     endOffset  = 0;
    CamxResult result     = CamxResultSuccess;

    if ((NULL == pBuffer) || (NULL == pOffset))
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params pBuffer %p pOffset %p", pBuffer, pOffset);
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        offset    = *pOffset;
        endOffset = offset + sizeof(UINT16);

        if (endOffset - 1 >= bufferSize)
        {
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Line %d: Buffer overflow buffersize %d, endOffset-1 = %d", lineNum,
                bufferSize, endOffset - 1);
            result = CamxResultEOutOfBounds;
        }
    }

    if (CamxResultSuccess == result)
    {
        if (TRUE == endian)
        {
            do
            {
                shift = shift - 8;
                pBuffer[offset] = static_cast <UINT8>((value >> shift) & 0xFF);
                offset++;
            } while (shift);
        }
        else
        {
            offset = endOffset - 1;
            do
            {
                shift = shift - 8;
                pBuffer[offset] = static_cast <UINT8>((value >> shift) & 0xFF);
                offset--;
            } while (shift);
        }

        *pOffset = endOffset;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::EmitLong
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::EmitLong(
    UINT32    value,
    UINT8*    pBuffer,
    UINT32*   pOffset,
    UINT32    bufferSize,
    BOOL      endian,
    UINT32    lineNum)
{
    UINT32     shift      = sizeof(UINT32) * 8;
    UINT32     offset     = 0;
    UINT32     endOffset  = 0;
    CamxResult result     = CamxResultSuccess;

    if ((NULL == pBuffer) || (NULL == pOffset))
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params pBuffer %p pOffset %p", pBuffer, pOffset);
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        offset    = *pOffset;
        endOffset = offset + sizeof(UINT32);

        if (endOffset - 1 >= bufferSize)
        {
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Line %d: Buffer overflow buffersize %d, endOffset-1 = %d", lineNum,
                bufferSize, endOffset - 1);
            result = CamxResultEOutOfBounds;
        }
    }

    if (CamxResultSuccess == result)
    {
        if (TRUE == endian)
        {
            do
            {
                shift = shift - 8;
                pBuffer[offset] = static_cast <UINT8>((value >> shift) & 0xFF);
                offset++;
            } while (shift);
        }
        else
        {
            offset = endOffset - 1;
            do
            {
                shift = shift - 8;
                pBuffer[offset] = static_cast <UINT8>((value >> shift) & 0xFF);
                offset--;
            } while (shift);
        }

        *pOffset = endOffset;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::EmitNBytes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::EmitNBytes(
    const UINT8*   pData,
    UINT32         count,
    UINT8*         pBuffer,
    UINT32*        pOffset,
    UINT32         bufferSize,
    BOOL           endian,
    UINT32         lineNum)
{
    UINT32     offset    = 0;
    UINT32     endOffset = 0;
    CamxResult result    = CamxResultSuccess;

    if ((NULL == pBuffer) || (NULL == pOffset) || (NULL == pData))
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params pBuffer %p pOffset %p pData %p", pBuffer, pOffset, pData);
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        offset    = *pOffset;
        endOffset = offset + count;

        if (endOffset - 1 >= bufferSize)
        {
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Line %d: Buffer overflow buffersize %d, endOffset-1 = %d", lineNum,
                bufferSize, endOffset - 1);
            result = CamxResultEOutOfBounds;
        }
    }

    if (CamxResultSuccess == result)
    {
        if (TRUE == endian)
        {
            Utils::Memcpy(pBuffer + offset, pData, count);
        }
        else
        {
            while (count--)
            {
                pBuffer[offset + count] = *pData++;
            }
        }

        *pOffset = endOffset;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::ReadShort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT16 JPEGEXIFComposer::ReadShort(
    const UINT8* pBuffer,
    UINT32       offset)
{
    return static_cast<UINT16>((static_cast<UINT16>(pBuffer[offset] << 8)) + static_cast<UINT16>(pBuffer[offset + 1]));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::ReadLong
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 JPEGEXIFComposer::ReadLong(
    const UINT8* pBuffer,
    UINT32       offset)
{
    return static_cast<UINT32>((static_cast<UINT32>(pBuffer[offset] << 24)) +
        (static_cast<UINT32>(pBuffer[offset + 1] << 16)) +
        (static_cast<UINT32>(pBuffer[offset + 2] << 8)) +
        static_cast<UINT32>(pBuffer[offset + 3]));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::JPEGEXIFComposer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JPEGEXIFComposer::JPEGEXIFComposer()
{
    m_pInputMainBuf           = NULL;
    m_pInputThumbBuffer       = NULL;
    m_pOutputBuf              = NULL;
    m_pAheadBuf               = NULL;
    m_pEXIFParams             = NULL;
    m_currentOffset           = 0;
    m_exitAppMarkerInc        = 0;
    m_pStatsDebugDataAddr     = NULL;
    m_statsDebugDataSize      = 0;
    m_pStatsDebugHeaderString = NULL;
    m_pOEMAppDataAddr         = NULL;
    m_OEMAppDataSize          = 0;
    m_skipThumbnailIfLarge    = FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::~JPEGEXIFComposer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JPEGEXIFComposer::~JPEGEXIFComposer()
{
    if (NULL != m_pAheadBuf)
    {
        CAMX_DELETE[] m_pAheadBuf;
        m_pAheadBuf = NULL;
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEXIFComposer::EmitAppByType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEXIFComposer::EmitAppByType(
    ExifPayloadType payloadType,
    const UINT8*    pPayload,
    UINT32          payloadLength,
    UINT32          payloadWritten,
    UINT8           recursionCount)
{
    CamxResult                  result            = CamxResultSuccess;
    UINT32                      overflowLength    = 0;
    UINT32                      appHeaderSize     = 0;
    const CHAR*                 pMagicStr         = NULL;
    UINT32                      magicStrLen       = 0;
    DebugDataStartHeader        statsHeader;

    if (pPayload == NULL)
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Exif payload NULL for type %d", payloadType);
        return CamxResultEInvalidArg;
    }

    switch (payloadType)
    {
    case ExifPayloadType::ExifStatsDebugData:
        if ((NULL != m_pStatsDebugHeaderString) &&
            (0 < OsUtils::StrLen(m_pStatsDebugHeaderString)))
        {
            pMagicStr = m_pStatsDebugHeaderString;
            magicStrLen = static_cast<UINT32>(OsUtils::StrLen(m_pStatsDebugHeaderString));

        }
        else
        {
            pMagicStr = StatsDebugMagicString;
            magicStrLen = sizeof(StatsDebugMagicString);
        }

        if (recursionCount == 0) {
            appHeaderSize += sizeof(statsHeader);
        }
        break;
    case ExifPayloadType::ExifMobicatData:
        pMagicStr = MobicatMagicString;
        magicStrLen = sizeof(MobicatMagicString) - 1;
        break;
    case ExifPayloadType::ExifDualCamData:
        pMagicStr = MIMagicString;
        magicStrLen = sizeof(MIMagicString) - 1;
        break;
    case ExifPayloadType::ExifOEMAppData:
        pMagicStr = OEMAppDataMagicString;
        magicStrLen = sizeof(OEMAppDataMagicString) - 1;
        break;
    default:
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Invalid payload type");
        return CamxResultEInvalidArg;
    }

    // 2 bytes(next app length) + magic_str_len + NULL terminator
    appHeaderSize += 2 + magicStrLen + 1;

    // Check overflow
    if ((payloadLength + appHeaderSize) > 0xFFFF)
    {
        overflowLength = payloadLength + appHeaderSize - 0xFFFF;
        payloadLength = 0xFFFF - appHeaderSize;
        CAMX_LOG_INFO(CamxLogGroupJPEG, "large payload, setting payload to %d overflow %d",payloadLength, overflowLength);
    }

    // add payload length
    appHeaderSize += payloadLength;

    if (CamxResultSuccess == result)
    {
        // Write APP marker incrementally starting with APP4, 2 bytes
        result = EmitShort(0xFF00 | (0xE4 + m_exitAppMarkerInc), m_pOutputBufAddr,
                           &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        // Write Size of Data inside the App header, 2 bytes
        result = EmitShort(static_cast<UINT16>(appHeaderSize), m_pOutputBufAddr,
                           &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        // Write magic string
        result = EmitNBytes(reinterpret_cast<const UINT8*>(pMagicStr), magicStrLen, m_pOutputBufAddr,
            &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    if (CamxResultSuccess == result)
    {
        result = EmitByte('\0', m_pOutputBufAddr, &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    if ((payloadType == ExifPayloadType::ExifStatsDebugData)    &&
        (recursionCount == 0)                                   &&
        (0 == OsUtils::StrCmp(StatsDebugMagicString, pMagicStr)))
    {
        statsHeader.dataSize                = (magicStrLen + 1) + sizeof(statsHeader) + m_statsDebugDataSize;
        statsHeader.majorRevision           = 1;
        statsHeader.minorRevision           = 1;
        statsHeader.patchRevision           = 0;
        statsHeader.SWMajorRevision         = 1;
        statsHeader.SWMinorRevision         = 0;
        statsHeader.SWPatchRevision         = 0;
        statsHeader.featureDesignator[0]    = 'R';
        statsHeader.featureDesignator[1]    = 'C';

        result = EmitNBytes(reinterpret_cast<const UINT8*>(&statsHeader), sizeof(statsHeader), m_pOutputBufAddr,
                            &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    // Write stats payload
    if (CamxResultSuccess == result)
    {
        result = EmitNBytes(reinterpret_cast<const UINT8*>(pPayload + payloadWritten), payloadLength, m_pOutputBufAddr,
            &m_currentOffset, m_outputBufSize, TRUE, __LINE__);
    }

    // If overflow data exists recurse till no overflow left
    if (overflowLength > 0)
    {
        EmitAppByType(payloadType, pPayload, overflowLength,
            (payloadWritten + payloadLength), (recursionCount + 1));
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupJPEG, "%s:%d] EXIF payload type %d write successful to App%d",
            __func__, __LINE__, payloadType, (4 + m_exitAppMarkerInc));
    }

    // Incriment App marker
    if (recursionCount == 0)
    {
        m_exitAppMarkerInc++;
    }

    return result;
}

CAMX_NAMESPACE_END
