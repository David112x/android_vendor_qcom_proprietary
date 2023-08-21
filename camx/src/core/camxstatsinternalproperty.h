////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxstatsinternalproperty.h
/// @brief Define Qualcomm Technologies, Inc. stats proprietary data for holding internal properties/events
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXSTATSINTERNALPROPERTY_H
#define CAMXSTATSINTERNALPROPERTY_H

#include "camxdefs.h"

CAMX_NAMESPACE_BEGIN

// AEC static definitions
static const UINT32 YStatsNum               = 256;  ///< Size of the Y(Luma) Sum array for AEC output

// AF static definitions
static const UINT8  AFVariablesPlaceHolder  = 47;   ///< Place holder for AF parameters

// ASD static declaration
static const UINT8  ASDVariablesPlaceHolder = 0;    ///< Place holder for ASD parameters

// AWB static definitions
static const UINT8  AWBVariablesPlaceHolder = 3;    ///< Place holder for AWB parameters
static const UINT32 AWBDecisionMapSize      = 64;   ///< Defines the size of the AWB decision map.

/// @todo  (CAMX-523): Update with stats internal data.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Stats common structures for output data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Describe AEC Output Data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief APEX values to provide basic parameters in logarithmic form
struct AECAPEXValues
{
    FLOAT   brightness; ///< Brightness Value (BV) - Apex value for brightness
    FLOAT   aperture;   ///< Aperture Value (AV)   - Apex value for aperture
    FLOAT   speed;      ///< Speed Value (SV)      - Apex value for sensitivity (ISO speed)
    FLOAT   time;       ///< Time Value (TV)       - Apex value for exposure time
    FLOAT   exposure;   ///< Exposure Value (EV)   - Apex value for exposure
};

/// @brief Structure describing AEC internal output data
struct AECOutputInternal
{
    AECAPEXValues       APEXValues;                 ///< AEC APEX Values
    FLOAT               asdExtremeGreenRatio;       ///< The extreme color green ratio
    FLOAT               asdExtremeBlueRatio;        ///< The extreme color blue ratio
    BOOL                brightnessSettled;          ///< Indicate if brightness is settled
    FLOAT               LEDInfluenceRatio;          ///< The sensitivity ratio which is calculated from
                                                    ///  sensitivity with no flash / sensitivity with
                                                    ///  preflash
    FLOAT               LEDRGRatio;                 ///< The RG ratio when flash is on
    FLOAT               LEDBGRatio;                 ///< The BG ratio when flash is on
    FLOAT               legacyYStats[YStatsNum];    ///< The Y(luma) statistics of current frame
    AECFlashInfoType    flashInfo;                  ///< Flash information if it is main or preflash
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Describe AWB internal Output Data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief AWB flash estimation state enumeration
enum class AWBFlashEstimationState
{
    AWBEstimationInactive,  ///< AWB is not doing any flash estimation
    AWBEstimationRunning,   ///< Flash is ON and AWB estimation is in progress
    AWBEstimationDone,      ///< AWB has completed the estimation for main flash
};

/// @brief AWB Illuminant output decision
enum class AWBIlluminant
{
    High,                           ///< High CCT
    D75,                            ///< Daylight, 7500K CCT
    D65,                            ///< Daylight, 6500K CCT
    D50,                            ///< Daylight, 5000K CCT
    CoolWhite,                      ///< Cold fluorescent
    Fluorescent,                    ///< fluorescent
    TL84,                           ///< Warm fluorescent
    Incadescnet,                    ///< Incandescent light
    Horizon,                        ///< Horizon light
    Low,                            ///< Low CCT
};

/// @brief Structure describing AWB output data
struct AWBOutputInternal
{
    AWBFlashEstimationState flashEstimationStatus;                  ///< AWB flash estimation status
    AWBIlluminant           AWBDecision;                            ///< AWB final illuminant decision
    UINT8                   AWBSampleDecision[AWBDecisionMapSize];  ///< AWB illuminant decision map
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Describes AF internal Output Data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Structure describing Auto Focus output data
struct AFOutputInternal
{
    UINT32 status;    ///< Auto focus status data
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Describes ASD Output Data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Structure describing ASD output data
struct ASDOutputInternal
{
    UINT32 severity;   ///< Severity array
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Describes AFD Output Data to Internal pool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief AFD mode enumeration
enum class AFDMode
{
    StatisticsAntibandingInvalid = -1,          ///< Antibanding Invalid type
    StatisticsAntibandingOff,                   ///< Antibanding off
    StatisticsAntibanding50Hz,                  ///< Antibanding mode set to 50Hz
    StatisticsAntibanding60Hz,                  ///< Antibanding mode set to 60Hz
    StatisticsAntibandingAuto,                  ///< Antibanding automatic mode
};

/// @brief Structure describing AFD output data
struct AFDOutputInternal
{
    AFDMode detectedAFDMode;   ///< Current AFD Mode detected by Algo
};

CAMX_NAMESPACE_END

#endif // CAMXSTATSINTERNALPROPERTY_H
