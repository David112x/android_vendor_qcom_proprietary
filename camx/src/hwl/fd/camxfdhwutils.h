////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxfdhwutils.h
/// @brief Translation utility class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXFDHWUTILS_H
#define CAMXFDHWUTILS_H

#include "camxfdutils.h"

CAMX_NAMESPACE_BEGIN

struct FDPyramidBufferSize
{
    UINT nVGA;       ///< Pyramid buffer size for VGA
    UINT nQVGA;      ///< Pyramid buffer size for QVGA
    UINT nWQVGAPlus; ///< Pyramid buffer size for WQVGA+
    UINT nWVGAPlus;  ///< Pyramid buffer size for WVGA+
};

enum FDHwMajorVersion
{
    FDHwVersinoInvalid = -1, ///< FD HW version invalid
    FDHwVersion4       =  0, ///< FD HW v4.x
    FDHwVersion5,            ///< FD HW v5.x
    FDHwVersion6,            ///< FD HW v6.x
    FDHwVersionMax           ///< FD HW version max
};

static const FDPyramidBufferSize FDHWPyramidBufferSize[FDHwVersionMax] =
{
    {            ///< FD HW v4.x
        218496,  ///< Pyramid/Work buffer size for 640x480 input image
         55680,  ///< Pyramid/Work buffer size for 320x240 input image
         74176,  ///< Pyramid/Work buffer size for 427x240 input image
        290240,  ///< Pyramid/Work buffer size for 854x480 input image
    },
    {            ///< FD HW v5.x
        222992,  ///< Pyramid/Work buffer size for 640x480 input image
         58416,  ///< Pyramid/Work buffer size for 320x240 input image
         78624,  ///< Pyramid/Work buffer size for 427x240 input image
        300832,  ///< Pyramid/Work buffer size for 854x480 input image
    },
    {            ///< FD HW v6.x
        211920,  ///< Pyramid/Work buffer size for 640x480 input image
        135120,  ///< Pyramid/Work buffer size for 320x240 input image
        154816,  ///< Pyramid/Work buffer size for 427x240 input image
        258496,  ///< Pyramid/Work buffer size for 854x480 input image
    }
};

CAMX_BEGIN_PACKED

/// @brief Structure face threhsold info for v6 hw
struct FDHwThresholdv6Bits
{
    UINT32  fullProfile     : 10;   /* 9:0   */
    UINT32  halfProfile     : 10;   /* 19:10 */
    UINT32  frontProfile    : 10;   /* 29:20 */
    UINT32  unused0         : 2;    /* 31:30 */
} CAMX_PACKED;

union FDHwThresholdv6
{
    FDHwThresholdv6Bits bits;   ///< Threshold register bits
    UINT32              value;  ///< UINT32 value
};

struct FDHwFaceSizeConfV4V5
{
    UINT32  size    : 9;    /* 8:0 */
    UINT32  conf    : 4;    /* 12:9 */
    UINT32  unused0 : 19;   /* 31:13 */
} CAMX_PACKED;

struct FDHwFaceSizeConfV6
{
    UINT32  size    : 9;    /* 8:0 */
    UINT32  unused0 : 1;    /* 9:9 */
    UINT32  conf    : 10;   /* 19:10 */
    UINT32  unused1 : 12;   /* 31:20 */
} CAMX_PACKED;

union FDHwFaceSizeConf
{
    FDHwFaceSizeConfV4V5 bitfieldV4V5;  ///< Bit field on FD HW V4, V5
    FDHwFaceSizeConfV6   bitfieldV6;    ///< Bit field on FD HW V6
    UINT32               value;         ///< UINT32 value
};

CAMX_END_PACKED

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the FD utility functions for FD core ver 4.0
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FDHwUtils
{
public:
    static const UINT FDHWFPSetOffBit   = 0x100;  ///< Bit to set to disable HW FP in v5 hw

    /// FD HW input dimensions
    static const UINT FDVGAWidth        = 640; ///< VGA width
    static const UINT FDVGAHeight       = 480; ///< VGA height
    static const UINT FDQVGAWidth       = 320; ///< QVGA width
    static const UINT FDQVGAHeight      = 240; ///< QVGA height
    static const UINT FDWVGAPlusWidth   = 854; ///< WVGA+ width
    static const UINT FDWVGAPlusHeight  = 480; ///< WVGA+ height
    static const UINT FDWQVGAPlusWidth  = 427; ///< QWVGA+ width
    static const UINT FDWQVGAPlusHeight = 240; ///< QWVGA+ height

    /// FD HW input images
    static const UINT FDInputImageQVGA      = 0; ///< Input image QVGA
    static const UINT FDInputImageVGA       = 1; ///< Input image VGA
    static const UINT FDInputImageWQVGAPlus = 2; ///< Input image WQVGA+
    static const UINT FDInputImageWVGAPlus  = 3; ///< Input image WVGA+

    /// different pose angles returned by HW FD
    static const UINT FDHwPoseFront         = 1;    ///< pose front
    static const UINT FDHwPoseRightDiagonal = 2;    ///< pose right diagonal
    static const UINT FDHwPoseRight         = 3;    ///< pose right
    static const UINT FDHwPoseLeftDiagonal  = 4;    ///< pose left diagonal
    static const UINT FDHwPoseLeft          = 5;    ///< pose left

    /// different detection angles for HW FD
    static const UINT FDHwUpwardDirection45     = 0;    ///< direction 0 degrees +/- 45 degrees
    static const UINT FDHwRightDirection45      = 1;    ///< direction 90 degrees +/- 45 degrees
    static const UINT FDHwLeftDirection45       = 2;    ///< direction 270 degrees +/- 45 degrees
    static const UINT FDHwDownwardDirection45   = 3;    ///< direction 180 degrees +/- 45 degrees
    static const UINT FDHwUpwardDirection135    = 4;    ///< direction 0 degrees +/- 135 degrees
    static const UINT FDHwRightDirection135     = 5;    ///< direction 90 degrees +/- 135 degrees
    static const UINT FDHwLeftDirection135      = 6;    ///< direction 270 degrees +/- 135 degrees
    static const UINT FDHwDownwardDirection135  = 7;    ///< direction 180 degrees +/- 135 degrees
    static const UINT FDHwDirectionAll          = 8;    ///< direction 180 degrees +/- 135 degrees
    static const UINT FDHwUpwardDirection75     = 9;    ///< direction 0 degrees +/- 75 degrees
    static const UINT FDHwRightDirection75      = 0xA;  ///< direction 90 degrees +/- 75 degrees
    static const UINT FDHwLeftDirection75       = 0xB;  ///< direction 270 degrees +/- 75 degrees
    static const UINT FDHwDownwardDirection75   = 0xC;  ///< direction 180 degrees +/- 75 degrees

    /// FD minimum face sizes in pixels
    static const UINT FDMinFacePixels0 = 20; ///< minimum face size in pixels 20
    static const UINT FDMinFacePixels1 = 25; ///< minimum face size in pixels 25
    static const UINT FDMinFacePixels2 = 32; ///< minimum face size in pixels 32
    static const UINT FDMinFacePixels3 = 40; ///< minimum face size in pixels 40

    /// FD minimum face size register values mapping to min size in pixels
    static const UINT FDMinFaceSize0 = 0; ///< minimum face size register value 0
    static const UINT FDMinFaceSize1 = 1; ///< minimum face size register value 1
    static const UINT FDMinFaceSize2 = 2; ///< minimum face size register value 2
    static const UINT FDMinFaceSize3 = 3; ///< minimum face size register value 3

    /// FD HW threshold, confidence ranges on different hw versions
    static const UINT FDHwThresholdTuningMax    = 1000;     ///< Max threshold value range from tuning header
    static const UINT FDHwThresholdV4V5Max      = 10;       ///< Max threshold value configurable on HW v4, v5
    static const UINT FDHwThresholdV6Max        = 1000;     ///< Max threshold value configurable on HW v6
    static const UINT FDHwConfidenceMax         = 1000;     ///< Max confidence value range that SW works on
    static const UINT FDHwConfidenceV4V5Max     = 9;        ///< Max confidence value from FD Hw v4, v5
    static const UINT FDHwConfidenceV6Max       = 1000;     ///< Max confidence value from FD Hw v6

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFDHwVersion
    ///
    /// @brief  Get FD Hw version from CSL Platform version
    ///
    /// @param  pCSLPlatform    CSL Platform version
    ///
    /// @return FDHw version
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE FDHwMajorVersion GetFDHwVersion(
        CSLCameraPlatform* pCSLPlatform)
    {
        FDHwMajorVersion FDHwVersion = FDHwVersinoInvalid;

        if (NULL != pCSLPlatform)
        {
            if ((1 == pCSLPlatform->platformVersion.majorVersion) &&
                (7 == pCSLPlatform->platformVersion.minorVersion) &&
                (0 == pCSLPlatform->platformVersion.revVersion))
            {
                FDHwVersion = FDHwVersion4;
            }
            else if ((1 == pCSLPlatform->platformVersion.majorVersion) &&
                     (7 == pCSLPlatform->platformVersion.minorVersion) &&
                     (5 == pCSLPlatform->platformVersion.revVersion))
            {
                FDHwVersion = FDHwVersion5;
            }
            else if ((1 == pCSLPlatform->platformVersion.majorVersion) &&
                     (6 == pCSLPlatform->platformVersion.minorVersion) &&
                     (0 == pCSLPlatform->platformVersion.revVersion))
            {
                FDHwVersion = FDHwVersion5;
            }
            else if ((4 == pCSLPlatform->platformVersion.majorVersion) &&
                     (8 == pCSLPlatform->platformVersion.minorVersion) &&
                     (0 == pCSLPlatform->platformVersion.revVersion))
            {
                FDHwVersion = FDHwVersion6;
            }
        }

        return FDHwVersion;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPoseAngleFromRegValue
    ///
    /// @brief  convert results from HW raw format to SW understandable pose
    ///
    /// @param  poseRegValue   FD HW pose
    ///
    /// @return INT32 SW pose value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static INT32 GetPoseAngleFromRegValue(
        UINT32 poseRegValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculatePyramidBufferSize
    ///
    /// @brief  update the pyramid/work buffer needed for HW FD
    ///
    /// @param  FDHWVersion     FD HW version
    /// @param  width           HW FD processing frame width
    /// @param  height          HW FD processing frame height
    /// @param  pSize           Pointer to the pyramid size
    ///
    /// @return CamxResult Success or fail
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult CalculatePyramidBufferSize(
        FDHwMajorVersion   FDHWVersion,
        UINT32             width,
        UINT32             height,
        UINT32*            pSize);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateFDHwCondValue
    ///
    /// @brief  update the COND HW register based on FD min face size and direction
    ///
    /// @param  FDHwVersion          FD Hw major version
    /// @param  enableHwFP           Whether to enable False Positive filter in Hw
    /// @param  minFaceSize          Minimum face size
    /// @param  referenceOrientation Reference device orientation
    /// @param  angle                Angle range
    /// @param  pReg                 pointer to the register
    ///
    /// @return CamxResult Success or fail
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult UpdateFDHwCondValue(
        FDHwMajorVersion    FDHwVersion,
        BOOL                enableHwFP,
        UINT32              minFaceSize,
        INT32               referenceOrientation,
        FDSearchAngle       angle,
        UINT32*             pReg);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateFDHwImageValue
    ///
    /// @brief  update image register based on dimension
    ///
    /// @param  width   HW FD processing frame width
    /// @param  height  HW FD processing frame height
    /// @param  pReg    pointer to the register
    ///
    /// @return CamxResult Success or fail
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult UpdateFDHwImageValue(
        UINT32  width,
        UINT32  height,
        UINT32* pReg);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFDHwThresholdRegVal
    ///
    /// @brief  Return register value for threshold configuration based on HW version
    ///
    /// @param  FDHwVersion     FD Hw major version
    /// @param  threshold       Threshold value from tuning info
    ///
    /// @return Register value for threshold
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 GetFDHwThresholdRegVal(
        FDHwMajorVersion    FDHwVersion,
        UINT32              threshold);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSizeFromRegValue
    ///
    /// @brief  Return face size value from hw output value
    ///
    /// @param  FDHwVersion         FD Hw major version
    /// @param  hwSizeConfidence    Packed size and confidence value from HW
    ///
    /// @return Register value for face size
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 GetSizeFromRegValue(
        FDHwMajorVersion    FDHwVersion,
        UINT32              hwSizeConfidence);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetConfidenceFromRegValue
    ///
    /// @brief  Return face confidence value from hw output value
    ///
    /// @param  FDHwVersion         FD Hw major version
    /// @param  hwSizeConfidence    Packed size and confidence value from HW
    ///
    /// @return Register value for face confidence
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 GetConfidenceFromRegValue(
        FDHwMajorVersion    FDHwVersion,
        UINT32              hwSizeConfidence);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FDHwUtils
    ///
    /// @brief Default constructor
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    FDHwUtils() = default;
private:
    FDHwUtils(const FDHwUtils&) = delete;
    FDHwUtils& operator=(const FDHwUtils&) = delete;
};

CAMX_NAMESPACE_END

#endif // CAMXFDHWUTILS_H
