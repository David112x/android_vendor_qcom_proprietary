////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxfdhwutils.cpp
/// @brief FD utility class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdebugprint.h"
#include "camxdefs.h"
#include "camxfdhwutils.h"
#include "camxosutils.h"
#include "camxtypes.h"

#include "titan170_fd_wrapper.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwUtils::GetPoseAngleFromRegValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 FDHwUtils::GetPoseAngleFromRegValue(
    UINT32 poseRegValue)
{
    INT32   pose;

    switch (poseRegValue)
    {
        case FDHwPoseFront:
            pose = 0;
            break;
        case FDHwPoseRightDiagonal:
            pose = 45;
            break;
        case FDHwPoseRight:
            pose = 90;
            break;
        case FDHwPoseLeftDiagonal:
            pose = -45;
            break;
        case FDHwPoseLeft:
            pose = -90;
            break;
        default:
            pose = 0;
            CAMX_LOG_WARN(CamxLogGroupFD, "Invalid pose = %d, return default", poseRegValue);
            break;
    }

    return pose;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwUtils::CalculatePyramidBufferSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FDHwUtils::CalculatePyramidBufferSize(
    FDHwMajorVersion   FDHWVersion,
    UINT32             width,
    UINT32             height,
    UINT32*            pSize)
{
    CamxResult  result  = CamxResultSuccess;

    if ((FDHwVersinoInvalid != FDHWVersion) && (FDHwVersionMax > FDHWVersion))
    {
        if ((FDVGAWidth == width) && (FDVGAHeight == height))
        {
            *pSize = FDHWPyramidBufferSize[FDHWVersion].nVGA;
        }
        else if ((FDQVGAWidth == width) && (FDQVGAHeight == height))
        {
            *pSize = FDHWPyramidBufferSize[FDHWVersion].nQVGA;
        }
        else if ((FDWQVGAPlusWidth == width) && (FDWQVGAPlusHeight == height))
        {
            *pSize = FDHWPyramidBufferSize[FDHWVersion].nWQVGAPlus;
        }
        else if ((FDWVGAPlusWidth == width) && (FDWVGAPlusHeight == height))
        {
            *pSize = FDHWPyramidBufferSize[FDHWVersion].nWVGAPlus;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupFD, "Invalid FD frame dimensions [%d, %d]", width, height);
            result = CamxResultEUnsupported;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupFD, "Invalid FD HW versoin : %d", FDHWVersion);
        result = CamxResultEUnsupported;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwUtils::UpdateFDHwCondValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FDHwUtils::UpdateFDHwCondValue(
    FDHwMajorVersion    FDHwVersion,
    BOOL                enableHwFP,
    UINT32              minFaceSize,
    INT32               referenceOrientation,
    FDSearchAngle       angle,
    UINT32*             pReg)
{
    CamxResult  result          = CamxResultSuccess;
    UINT32      detectionDir    = 0;

    if ((FDHwVersion6 != FDHwVersion) && (FDAngle75 == angle))
    {
        // HW v4, v5 do not support FDAngle75, fallback to All (360 degrees)
        angle = FDAngleAll;
    }

    if ((-1 == referenceOrientation) || (FDAngleAll == angle))
    {
        detectionDir = FDHwDirectionAll;
    }
    else
    {
        if (referenceOrientation < 45)
        {
            switch (angle)
            {
                case FDAngle45:  detectionDir = FDHwUpwardDirection45;  break;
                case FDAngle75:  detectionDir = FDHwUpwardDirection75;  break;
                case FDAngle135: detectionDir = FDHwUpwardDirection135; break;
                default:         detectionDir = FDHwDirectionAll;       break;
            }
        }
        else if (referenceOrientation < 135)
        {
            switch (angle)
            {
                case FDAngle45:  detectionDir = FDHwRightDirection45;  break;
                case FDAngle75:  detectionDir = FDHwRightDirection75;  break;
                case FDAngle135: detectionDir = FDHwRightDirection135; break;
                default:         detectionDir = FDHwDirectionAll;      break;
            }
        }
        else if (referenceOrientation < 225)
        {
            switch (angle)
            {
                case FDAngle45:  detectionDir = FDHwDownwardDirection45;  break;
                case FDAngle75:  detectionDir = FDHwDownwardDirection75;  break;
                case FDAngle135: detectionDir = FDHwDownwardDirection135; break;
                default:         detectionDir = FDHwDirectionAll;         break;
            }
        }
        else
        {
            switch (angle)
            {
                case FDAngle45:  detectionDir = FDHwLeftDirection45;  break;
                case FDAngle75:  detectionDir = FDHwLeftDirection75;  break;
                case FDAngle135: detectionDir = FDHwLeftDirection135; break;
                default:         detectionDir = FDHwDirectionAll;     break;
            }
        }
    }

    *pReg = detectionDir << FD_WRAPPER_FD_A_FD_COND_DIR_SHIFT;

    if (FDMinFacePixels0 >= minFaceSize)
    {
        *pReg |= FDMinFaceSize0;
    }
    else if (FDMinFacePixels1 >= minFaceSize)
    {
        *pReg |= FDMinFaceSize1;
    }
    else if (FDMinFacePixels2 >= minFaceSize)
    {
        *pReg |= FDMinFaceSize2;
    }
    else if (FDMinFacePixels3 >= minFaceSize)
    {
        *pReg |= FDMinFaceSize3;
    }
    else
    {
        *pReg |= FDMinFaceSize3;
        CAMX_LOG_ERROR(CamxLogGroupFD, "Min face size %d not supported by HW, set to default", minFaceSize);
    }

    if ((FDHwVersion5 == FDHwVersion) && (FALSE == enableHwFP))
    {
        *pReg |= FDHWFPSetOffBit;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwUtils::UpdateFDHwImageValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FDHwUtils::UpdateFDHwImageValue(
    UINT32  width,
    UINT32  height,
    UINT32* pReg)
{
    CamxResult  result = CamxResultSuccess;

    if ((FDQVGAWidth == width) && (FDQVGAHeight == height))
    {
        *pReg = FDInputImageQVGA;
    }
    else if ((FDVGAWidth == width) && (FDVGAHeight == height))
    {
        *pReg = FDInputImageVGA;
    }
    else if ((FDWQVGAPlusWidth == width) && (FDWQVGAPlusHeight == height))
    {
        *pReg = FDInputImageWQVGAPlus;
    }
    else if ((FDWVGAPlusWidth == width) && (FDWVGAPlusHeight == height))
    {
        *pReg = FDInputImageWVGAPlus;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupFD, "Invalid FD frame dimensions [%d, %d]", width, height);
        result = CamxResultEUnsupported;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwUtils::GetFDHwThresholdRegVal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 FDHwUtils::GetFDHwThresholdRegVal(
    FDHwMajorVersion    FDHwVersion,
    UINT32              threshold)
{
    UINT32 regValue = 0x0;

    if (FDHwVersion6 == FDHwVersion)
    {
        FDHwThresholdv6 thresholdv6;

        // On HW v6, threshold range is 0 - 1000, whereas existing tuning value range is 0-10
        // If the tuning value is in
        //  10-1000 range : tuning header is properly updated for FD HWv6
        //  1-10    range : tuning header is not properly updated as generally 1-10 is not an intended value,
        //                  have a fallback to multiply with 100
        //  0             : probably intended value. keep it same.
        if (FDHwThresholdTuningMax < threshold)
        {
            // invalid value, have a fallback default value
            threshold = 0;
        }
        else if (FDHwThresholdV4V5Max >= threshold)
        {
            threshold *= (FDHwThresholdV6Max / FDHwThresholdV4V5Max);
        }

        // use same threshold value for all profiles.
        // In case there is a need to use different values, these need to be added in tuning header first.
        thresholdv6.bits.frontProfile = threshold;
        thresholdv6.bits.halfProfile  = threshold;
        thresholdv6.bits.fullProfile  = threshold;

        regValue = thresholdv6.value;
    }
    else
    {
        // On HW v4, v5, HW threahold range is 0 - 9
        // If the tuning value is in
        //  0-9    range : tuning header is properly updated
        //  10-1000 range : tuning header is not properly updated for FD HWv4, v5 (probably using v6 tuning value)
        //                  have a fallback to divided by 100
        //  0             : probably intended value. keep it same.
        if (FDHwThresholdTuningMax < threshold)
        {
            CAMX_LOG_ERROR(CamxLogGroupFD, "Incorrect threshold tuning value, threshold=%d", threshold);
            threshold = 0;
        }
        else if ((FDHwThresholdV4V5Max - 1) < threshold)
        {
            threshold /= (FDHwThresholdV6Max / FDHwThresholdV4V5Max);
        }

        // HW v4, v5 has only one threshold value for all profiles.
        regValue = threshold;
    }

    return regValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwUtils::GetSizeFromRegValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 FDHwUtils::GetSizeFromRegValue(
    FDHwMajorVersion    FDHwVersion,
    UINT32              hwSizeConfidence)
{
    UINT32           size       = 0;
    FDHwFaceSizeConf hwSizeConf;

    hwSizeConf.value = hwSizeConfidence;
    switch (FDHwVersion)
    {
        case FDHwVersion4:
        case FDHwVersion5:
            size = hwSizeConf.bitfieldV4V5.size;
            break;
        case FDHwVersion6:
            size = hwSizeConf.bitfieldV6.size;
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupFD, "Invalid HW version ");
    }

    return size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwUtils::GetConfidenceFromRegValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 FDHwUtils::GetConfidenceFromRegValue(
    FDHwMajorVersion    FDHwVersion,
    UINT32              hwSizeConfidence)
{
    UINT32           confidence = 0;
    FDHwFaceSizeConf hwSizeConf;

    // Expected value for confidence is 0-1000 range from FDHw, FDManager Nodes' perspective.
    // Update based on FD HW behavior

    hwSizeConf.value = hwSizeConfidence;

    switch (FDHwVersion)
    {
        case FDHwVersion4:
        case FDHwVersion5:
            // On HW v4, v5, HW outputs confidence in range 0-9
            confidence = (hwSizeConf.bitfieldV4V5.conf * FDHwConfidenceMax) / FDHwConfidenceV4V5Max;
            break;
        case FDHwVersion6:
            // HW v6 outputs face confidence in 0-1000 range, no need to change
            confidence = (hwSizeConf.bitfieldV6.conf * FDHwConfidenceMax) / FDHwConfidenceV6Max;
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupFD, "Invalid HW version ");
    }

    return confidence;
}

CAMX_NAMESPACE_END