////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016,2018-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   camximageformatutils.h
///
/// @brief  Utility functions for camera image formats.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXIMAGEFORMATUTILS_H
#define CAMXIMAGEFORMATUTILS_H

#include "camxcommontypes.h"
#include "camxformats.h"
#include "camxutils.h"
#include "camxdebugprint.h"


CAMX_NAMESPACE_BEGIN

/// @brief UBWC tile info structure for a UBWC format
struct UBWCTileInfo
{
    UINT widthPixels;       ///< Tile width in pixels
    UINT widthBytes;        ///< Tile width in pixels
    UINT height;            ///< Tile height
    UINT widthMacroTile;    ///< Macro tile width
    UINT heightMacroTile;   ///< Macro tile height
    UINT BPPNumerator;      ///< Bytes per pixel (numerator)
    UINT BPPDenominator;    ///< Bytes per pixel (denominator)
};

/// @brief UBWC Paartial tile info structure for a UBWC format
struct UBWCPartialTileInfo
{
    UINT partialTileBytesLeft;     ///< partial tile residual bytes on left
    UINT partialTileBytesRight;    ///< partial tile residual bytes on right
    UINT horizontalInit;           ///< Init / start on horizontal direction
    UINT verticalInit;             ///< Init / start on vertical direction
};

/// @brief UBWC plane mode for a UBWC format
struct UBWCPlaneModeConfig
{
    UINT highestValueBit;     ///< highest bank value bit
    UINT highestBankL1Enable; ///< highest bank l1 enable
    UINT highestBankEnable;   ///< highest bank enable
    UINT bankSpreadEnable;    ///< bank spread enable
    UINT eightChannelEnable;  ///< eight channel enable
};

/// @brief UBWC mode for UBWC version for given wm
struct UBWCModeConfig1
{
    UINT lossyMode;           ///< UBWC Lossy/Loss-less; 0:Lossless; 1: Lossy
    UINT UBWCVersion;         ///< UBWC Version; 0: ubwc2.0; 1: ubwc3.0
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Static utility class for image format information.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAMX_VISIBILITY_PUBLIC ImageFormatUtils
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeFormatParams
    ///
    /// @brief  Initialize format params
    ///
    /// @param  pFormat          The image format
    /// @param  pFormatParamInfo Format param info
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID InitializeFormatParams(
        ImageFormat*       pFormat,
        FormatParamInfo*   pFormatParamInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRawSize
    ///
    /// @brief  Get the size of a RAW buffer given a specific format.
    ///
    /// @param  pRawFormat  The RAW image format.
    ///
    /// @return The size of the RAW buffer in bytes.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static SIZE_T GetRawSize(
        const ImageFormat* pRawFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTotalSize
    ///
    /// @brief  Get the total number of bytes required to hold a complete image buffer of a given format.
    ///
    /// @param  pFormat The image format.
    ///
    /// @return The total number of bytes required to hold a complete image buffer of a given format.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static SIZE_T GetTotalSize(
        const ImageFormat* pFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumberOfPlanes
    ///
    /// @brief  Get the number of planes in an image of a given format.
    ///
    /// @param  pFormat The image format.
    ///
    /// @Return The number of planes in an image of a given format.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT GetNumberOfPlanes(
        const ImageFormat* pFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPlaneSize
    ///
    /// @brief  Get the number of bytes required to hold a given plane of an image buffer of a given format.
    ///
    /// @param  pFormat     The image format.
    /// @param  planeIndex  The plane index.
    ///
    /// @return The number of bytes required to hold a given plane of an image buffer of a given format.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static SIZE_T GetPlaneSize(
        const ImageFormat* pFormat,
        UINT planeIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetBytesPerPixel
    ///
    /// @brief  Get the number of bytes per pixel based on the image format
    ///
    /// @param  pFormat     The image format.
    ///
    /// @return bytes per pixel; 0 if invalid/unsupported format
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static FLOAT GetBytesPerPixel(
        const ImageFormat* const pImageFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupUBWCPlanes
    ///
    /// @brief  Setup the UBWC plane elements as per UBWC specification
    ///
    /// @param  pFormat The UBWC image format
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID SetupUBWCPlanes(
        ImageFormat* pFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsYUV
    ///
    /// @brief  Test if an image format is a YUV format.
    ///
    /// @param  pFormat The image format.
    ///
    /// @return TRUE if a YUV Format.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL IsYUV(
        const ImageFormat* pFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsRAW
    ///
    /// @brief  Test if an image format is a RAW format.
    ///
    /// @param  pFormat The image format.
    ///
    /// @return TRUE if a RAW Format.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL IsRAW(
        const ImageFormat* pFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsUBWC
    ///
    /// @brief  Test if an image format is a UBWC format.
    ///
    /// @param  format The image format.
    ///
    /// @return TRUE if a UBWC Format.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL IsUBWC(
        Format format);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Is10BitFormat
    ///
    /// @brief  Test if an image format is a 10 bit format.
    ///
    /// @param  format The image format.
    ///
    /// @return TRUE if a 10 bit format.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL Is10BitFormat(
        Format format);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Is8BitFormat
    ///
    /// @brief  Test if an image format is a 8 bit format.
    ///
    /// @param  format The image format.
    ///
    /// @return TRUE if a 8 bit Format.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL Is8BitFormat(
        Format format);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Is10BitUBWCFormat
    ///
    /// @brief  Test if an image format is a 10 bit UBWC ormat.
    ///
    /// @param  format The image format.
    ///
    /// @return TRUE if a 10 bit UBWC Format.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL Is10BitUBWCFormat(
        Format format);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Is8BitUBWCFormat
    ///
    /// @brief  Test if an image format is a 8 bit UBWC format.
    ///
    /// @param  format The image format.
    ///
    /// @return TRUE if a 8 bit UBWC Format.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL Is8BitUBWCFormat(
        Format format);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsUHDResolution
    ///
    /// @brief  Tests if passed resolution is greater than or equal to UHD resolution
    ///
    /// @param  width   Width of resolution
    /// @param  height  Height of resolution
    ///
    /// @return TRUE if resolution is greater than equal to UHDresolution
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL IsUHDResolution(
        UINT width,
        UINT height);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUBWCTileInfo
    ///
    /// @brief  Return the UBWC tile info
    ///
    /// @param  pFormat The image format.
    ///
    /// @return UBWC tile info pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static const UBWCTileInfo* GetUBWCTileInfo(
        const ImageFormat* pFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUBWCPartialTileInfo
    ///
    /// @brief  Calculate the UBWC partial tile information
    ///
    /// @param  pTileInfo               UBWC tile info pointer.
    /// @param  pUBWCPartialTileParam   UBWC partial tile parameter.
    /// @param  startOffsetPixel        start offset of frame or stripe in pixels.
    /// @param  widthPixels             width in pixels for the stripe.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GetUBWCPartialTileInfo(
        const  UBWCTileInfo*           pTileInfo,
        struct UBWCPartialTileInfo*    pUBWCPartialTileParam,
        UINT                           startOffsetPixel,
        UINT                           widthPixels);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalcPlaneOffset
    ///
    /// @brief  Calculates the base offset for a given plane and batchId
    ///
    /// @param  pFormat             Pointer to an ImageFormat structure
    /// @param  numBatchedFrames    Total number of batched frames...will be 1 if not batching
    /// @param  batchFrameIndex     Batch index...will be 0 if not batching
    /// @param  planeIndex          Plane index (for planar formats)
    ///
    /// @return Offset from the base address for the specified plane and image within the batch.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE static SIZE_T CalcPlaneOffset(
        const ImageFormat*  pFormat,
        UINT                numBatchedFrames,
        UINT                batchFrameIndex,
        UINT                planeIndex)
    {
        SIZE_T offset = 0;
        if (IsYUV(pFormat) || IsRAW(pFormat) || IsUBWC(pFormat->format))
        {
            SIZE_T planeOffset = (planeIndex == 0) ? 0 :
                (pFormat->formatParams.yuvFormat[planeIndex - 1].planeSize *  numBatchedFrames);
            offset = planeOffset + (batchFrameIndex * pFormat->formatParams.yuvFormat[planeIndex].planeSize);
        }
        return offset;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUBWCModeConfig
    ///
    /// @brief  Calculate the UBWC partial tile information
    ///
    /// @param  pFormat         UBWC tile info pointer.
    /// @param  planeIndex      Y or UV plane index
    ///
    /// @return UBWC Mode config value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 GetUBWCModeConfig(
        const ImageFormat* pFormat,
        UINT               planeIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUBWCVersion
    ///
    /// @brief  Get UBWC version
    ///
    /// @return Supported UBWC version
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 GetUBWCVersion();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUBWCHWVersion
    ///
    /// @brief  Get UBWC version
    ///
    /// @return Supported UBWC version
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 GetUBWCHWVersion();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUBWCHWVersionExternal
    ///
    /// @brief  Get UBWC version for external outputs
    ///
    /// @return Supported UBWC version
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 GetUBWCHWVersionExternal();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUBWCScaleRatioLimitationFlag
    ///
    /// @brief  Get UBWC Scale ratio limitations flag (TRUE or FALSE)
    ///
    /// @return Supported UBWC Scale ratio limitations flag (TRUE or FALSE)
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL GetUBWCScaleRatioLimitationFlag();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUBWCModeConfig1
    ///
    /// @brief  Evaluate the UBWC version and mode of compression (Lossy or Lossless)
    ///
    /// @param  pFormat    Image Format pointer
    ///
    /// @return UBWC Mode config1 value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 GetUBWCModeConfig1(
        const ImageFormat* pFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetUBWCModeConfig
    ///
    /// @brief  Set the UBWC UBWC plane mode information
    ///
    /// @param  pPlaneMode     Point to UBWC plane mode information.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE static VOID SetUBWCModeConfig(
        struct UBWCPlaneModeConfig*    pPlaneMode)
    {
        s_UBWCPlaneConfig = *pPlaneMode;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetUBWCModeConfig1
    ///
    /// @brief  Set the UBWC mode config1 information
    ///
    /// @param  pModeConfig1     Point to UBWC mode config1 information.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE static VOID SetUBWCModeConfig1(
        struct UBWCModeConfig1* pModeConfig1)
    {
        if (NULL != pModeConfig1)
        {
            s_UBWCModeConfig1 = *pModeConfig1;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetUBWCModeConfig2
    ///
    /// @brief  Set the UBWC mode config information. Used for display/video outputs where UBWC version could be different
    ///         from the version used internally
    ///
    /// @param  pModeConfig2     Pointer to UBWC mode config information.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE static VOID SetUBWCModeConfig2(
        struct UBWCModeConfig1* pModeConfig2)
    {
        if (NULL != pModeConfig2)
        {
            s_UBWCModeConfig2 = *pModeConfig2;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUBWCLossyMode
    ///
    /// @brief  Set the lossy UBWC mode
    ///
    /// @param  pFormatParamInfo     Pointer to Format info.
    ///
    /// @return UBWC lossy mode
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE  static UBWCLossyMode GetUBWCLossyMode(
        FormatParamInfo*   pFormatParamInfo)
    {
        UBWCLossyMode UBWClossyFlag = UBWCLossless;
        if (TRUE == pFormatParamInfo->isHALBuffer)
        {
            UBWClossyFlag = (TRUE == Utils::IsBitMaskSet64(pFormatParamInfo->grallocUsage, GrallocUsagePrivateAllocUBWCPI)) ?
                UBWCLossy : UBWCLossless;

        }
        else
        {
            UBWClossyFlag = (0 != (pFormatParamInfo->LossyUBWCProducerUsage & pFormatParamInfo->LossyUBWCConsumerUsage)) ?
                UBWCLossy : UBWCLossless;
        }
        return UBWClossyFlag;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUBWCVersionForLink
    ///
    /// @brief  Get the UBWC version from the producer consumer flags
    ///
    /// @param  pFormatParamInfo     Pointer to Format info.
    ///
    /// @return return the version
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UBWCVersion GetUBWCVersionForLink(
        FormatParamInfo*   pFormatParamInfo)
    {
        UINT32 highestSetBit = 0;
        if (TRUE == pFormatParamInfo->isHALBuffer)
        {
            highestSetBit = GetUBWCVersion();
            CAMX_LOG_VERBOSE(CamxLogGroupUtils, " isHALBuffer %d, highestSetBit %d",
                             pFormatParamInfo->isHALBuffer, highestSetBit);
        }
        else
        {
            UINT32 flag = (pFormatParamInfo->UBWCVersionConsumerUsage & UBWCVersionMask) &
                (pFormatParamInfo->UBWCVersionProducerUsage & UBWCVersionMask);
            while (flag >>= 1)
            {
                highestSetBit++;
            }
            CAMX_LOG_VERBOSE(CamxLogGroupUtils, "UBWCVersionConsumerUsage %d, UBWCVersionProducerUsage %d,"
                          "flag %d, highestSetBit %d",
                          pFormatParamInfo->UBWCVersionConsumerUsage, pFormatParamInfo->UBWCVersionProducerUsage,
                          flag, highestSetBit);
        }
        return static_cast<UBWCVersion>(highestSetBit);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetUBWCLimitation
    ///
    /// @brief  Set the supported UBWC limitation (2x) on down/up scale ratio (TRUE/FALSE)
    ///
    /// @param  UBWCLimitationInScaleRatio flag determing max down/up scale as 2x / 2x respectively .
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE static VOID SetUBWCLimitation(
        BOOL UBWCLimitationInScaleRatio)
    {
        s_UBWCLimitationOnScaleRatio = UBWCLimitationInScaleRatio;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFormat
    ///
    /// @brief  Function to get format type, given the gralloc properties
    ///
    /// @param  rGrallocProperties Gralloc usage flags and other properties required for buffer allocation
    /// @param  rFormat            Unique Image format structure
    ///
    /// @return CamxResultSuccess if successful, Errors specified by CamxResults otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetFormat(
       GrallocProperties&   rGrallocProperties,
       Format&              rFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateBufferSize
    ///
    /// @brief  Function to validate buffer size with gralloc size
    ///
    /// @param  phBufferHandle          Buffer Handle
    /// @param  bufferSize              Buffer Size value
    ///
    /// @return CamxResultSuccess if successful, Errors specified by CamxResults otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult ValidateBufferSize(
        VOID*   phBufferHandle,
        SIZE_T  bufferSize);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUnalignedBufferSize
    ///
    /// @brief  Function to get buffer size from gralloc
    ///
    /// @param  phBufferHandle          Buffer Handle
    /// @param  bufferSize              Buffer Size value
    /// @param  format                  format of the Image buffer
    /// @param  pGrallocBufferSize      Pointer to read Gralloc Buffer Size value
    ///
    /// @return CamxResultSuccess if successful, Errors specified by CamxResults otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetUnalignedBufferSize(
        VOID*   phBufferHandle,
        SIZE_T  bufferSize,
        Format  format,
        SIZE_T* pGrallocBufferSize);

private:
    ImageFormatUtils()                                          = delete;
    ImageFormatUtils(const ImageFormatUtils& rOther)             = delete;
    ImageFormatUtils& operator=(const ImageFormatUtils& rOther)  = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeRawFormatParams
    ///
    /// @brief  Function to initialize params for Raw Format
    ///
    /// @param  pImageFormat     The image format
    /// @param  pFormatParamInfo Format param info
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID InitializeRawFormatParams(
        ImageFormat*       pImageFormat,
        FormatParamInfo*   pFormatParamInfo);

    static UBWCPlaneModeConfig s_UBWCPlaneConfig; ///< UBWC plane config
    static UBWCModeConfig1     s_UBWCModeConfig1; ///< UBWC version config
    static UBWCModeConfig1     s_UBWCModeConfig2; ///< UBWC version config for external outputs
    static BOOL    s_UBWCLimitationOnScaleRatio;  ///< UBWC Supported size and scale are limited
};

CAMX_NAMESPACE_END

#endif // CAMXIMAGEFORMATUTILS_H
