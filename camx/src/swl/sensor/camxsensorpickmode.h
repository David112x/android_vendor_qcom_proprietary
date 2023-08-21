////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2017, 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxsensorpickmode.h
/// @brief SensorPickMode class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXSENSORPICKMODE_H
#define CAMXSENSORPICKMODE_H

#include "camxdefs.h"
#include "camxsensordriver.h"

CAMX_NAMESPACE_BEGIN

/// @brief This enumerates conditions for sensor pick mode
enum class SensorPickModeCondition
{
    FrameRate,        ///< Sensor frame rate
    BoundedFrameRate, ///< Bounded frame rate
    AspectRatio,      ///< Aspect ratio
    Width,            ///< Resolution width
    Height,           ///< Resolution height
    Clock,            ///< Clock
    Quadra,           ///< Quadra
    HFR,              ///< High frame rate
    DEF,              ///< DEF
    IHDR,             ///< Interlaced HDR
    RHDR,             ///< RHDR
    MPIX,             ///< MPIX
    BestResolution,   ///< Best resolution match
    Max               ///< Max
};

static const UINT16 MaxSensorPickModeCondition = static_cast<UINT16>(SensorPickModeCondition::Max);

/// @brief This enumerates usecases for sensor pick mode
enum class SensorPickModeUseCase
{
    FastAEC,      ///< Fast AEC
    Quadra,       ///< Quadra
    HFR,          ///< High frame rate
    IHDR,         ///< Interlaced HDR
    RHDR,         ///< RHDR
    VHDR,         ///< Video HDR
    Snapshot,     ///< Snapshot
    VideoPreview, ///< Video preview
    Max           ///< Max
};

static const UINT16 MaxSensorPickModeUseCase = static_cast<UINT16>(SensorPickModeUseCase::Max);

/// @brief Structure used to pick resolution matching best mode or max mode
struct SensorPickResolution
{
    UINT32  temporaryResolution;  ///< Temporary resolution for matching in pixels (width * height)
    UINT32  lastResolution;       ///< Last picked resolution in pixels (width * height)
};

/// @brief Structure describing the use cases for pick resolution
struct SensorPickModeProperties
{
    UINT32               width;           ///< Width of the frame required
    UINT32               height;          ///< Height of the frame required
    UINT32               maxWidth;        ///< Max Bounded Width of the frame
    UINT32               maxHeight;       ///< Max Bounded Height of the frame
    UINT32               minWidth;        ///< Min Bounded Width of the frame
    UINT32               minHeight;       ///< Min Bounded Height of the frame
    DOUBLE               frameRate;       ///< Frame Rate of the usecase
    UINT32               capabilityCount; ///< Number of capabilities required
    SensorCapabilityList capability;      ///< List of capabilities required
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Static class for picking sensor mode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class SensorPickMode
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetModeIndex
    ///
    /// @brief  Method to validate height
    ///
    /// @param  pModeInfo       Pointer to mode information in sensor driver
    /// @param  pSensorPickMode Pointer to SensorPickMode
    /// @param  pSelectedMode   pointer to selected mode
    ///
    /// @return True if height condition matches else false
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetModeIndex(
        const   ResolutionInformation*    pModeInfo,
        const   SensorPickModeProperties* pSensorPickMode,
        UINT32*                           pSelectedMode);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SensorPickMode
    ///
    /// @brief  Private constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SensorPickMode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SensorPickMode
    ///
    /// @brief  Private copy constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SensorPickMode(const SensorPickMode& rOther) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SensorPickMode
    ///
    /// @brief  Private assignment operator
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SensorPickMode& operator=(const SensorPickMode& rOther) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateHeight
    ///
    /// @brief  Method to validate height
    ///
    /// @param  modeIndex       Mode index in the binary
    /// @param  streamIndex     Index of IMAGE stream type
    /// @param  pModeInfo       Pointer to mode information in sensor driver
    /// @param  pSensorPickMode Pointer to SensorPickMode
    ///
    /// @return True if height condition matches else false
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL ValidateHeight(
        UINT32                           modeIndex,
        UINT32                           streamIndex,
        const  ResolutionInformation*    pModeInfo,
        const  SensorPickModeProperties* pSensorPickMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateWidth
    ///
    /// @brief  Method to validate width
    ///
    /// @param  modeIndex       Mode index in the binary
    /// @param  streamIndex     Index of IMAGE stream type
    /// @param  pModeInfo       Pointer to mode information in sensor driver
    /// @param  pSensorPickMode Pointer to SensorPickMode
    ///
    /// @return True if width condition matches else false
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL ValidateWidth(
        UINT32                           modeIndex,
        UINT32                           streamIndex,
        const  ResolutionInformation*    pModeInfo,
        const  SensorPickModeProperties* pSensorPickMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateAspectRatio
    ///
    /// @brief  Method to validate Aspect ratio
    ///
    /// @param  modeIndex       Mode index in the binary
    /// @param  streamIndex     Index of IMAGE stream type
    /// @param  pModeInfo       Pointer to mode information in sensor driver
    /// @param  pSensorPickMode Pointer to SensorPickMode
    ///
    /// @return True if aspect ratio condition matches else false
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL ValidateAspectRatio(
        UINT32                           modeIndex,
        UINT32                           streamIndex,
        const  ResolutionInformation*    pModeInfo,
        const  SensorPickModeProperties* pSensorPickMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateCapability
    ///
    /// @brief  Method to validate capabilities
    ///
    /// @param  modeIndex       Mode index in the binary
    /// @param  pModeInfo       Pointer to mode information in sensor driver
    /// @param  pSensorPickMode Pointer to SensorPickMode
    ///
    /// @return True if capability condition matches else false
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL ValidateCapability(
        UINT32                           modeIndex,
        const  ResolutionInformation*    pModeInfo,
        const  SensorPickModeProperties* pSensorPickMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateFrameRate
    ///
    /// @brief  Method to validate frame rate
    ///
    /// @param  modeIndex       Mode index in the binary
    /// @param  pModeInfo       Pointer to mode information in sensor driver
    /// @param  pSensorPickMode Pointer to SensorPickMode
    ///
    /// @return True if frame rate condition matches else false
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL ValidateFrameRate(
        UINT32                           modeIndex,
        const  ResolutionInformation*    pModeInfo,
        const  SensorPickModeProperties* pSensorPickMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateBestResolution
    ///
    /// @brief  Method to validate best resolution
    ///         If max resolution and best resolution are selected for a mode
    ///         preference will be given to max resolution selection
    ///
    /// @param  modeIndex         Mode index in the binary
    /// @param  streamIndex       Index of IMAGE stream type
    /// @param  pModeInfo         Pointer to mode information in sensor driver
    /// @param  pSensorPickMode   Pointer to SensorPickMode as requested by application
    /// @param  pPickedResolution Pointer to pick best sensor resolution
    ///
    /// @return True if best resolution condition matches
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL ValidateBestResolution(
        UINT32                           modeIndex,
        UINT32                           streamIndex,
        const  ResolutionInformation*    pModeInfo,
        const  SensorPickModeProperties* pSensorPickMode,
        SensorPickResolution*            pPickedResolution);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateMaxResolution
    ///
    /// @brief  Method to validate max resolution
    ///
    /// @param  modeIndex         Mode index in the binary
    /// @param  streamIndex       Index of IMAGE stream type
    /// @param  pModeInfo         Pointer to mode information in sensor driver
    /// @param  pPickedResolution Pointer to pick best sensor resolution
    ///
    /// @return True if max resolution condition matches
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL ValidateMaxResolution(
        UINT32                         modeIndex,
        UINT32                         streamIndex,
        const  ResolutionInformation*  pModeInfo,
        SensorPickResolution*          pPickedResolution);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateHFR
    ///
    /// @brief  Method to validate HFR mode
    ///
    /// @param  modeIndex       Mode index in the binary
    /// @param  pModeInfo       Pointer to mode information in sensor driver
    /// @param  pSensorPickMode Pointer to SensorPickMode as requested by application
    ///
    /// @return True if the mode is HFR
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL ValidateHFR(
        UINT32                           modeIndex,
        const  ResolutionInformation*    pModeInfo,
        const  SensorPickModeProperties* pSensorPickMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateDefaultMode
    ///
    /// @brief  Method to validate DEF mode
    ///
    /// @param  modeIndex    Mode index in the binary
    /// @param  pModeInfo    Pointer to mode information in sensor driver
    ///
    /// @return True if the mode is Default
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL ValidateDefaultMode(
        UINT32                           modeIndex,
        const  ResolutionInformation*    pModeInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetImageStreamIndex
    ///
    /// @brief  Method to get index for image stream type
    ///
    /// @param  modeIndex    Mode index for validation
    /// @param  pStreamIndex Pointer where index of image stream type will be returned
    /// @param  pModeInfo    Pointer to mode information in sensor driver
    ///
    /// @return Success if IMAGE stream type was found in the configuration
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetImageStreamIndex(
        UINT32                         modeIndex,
        UINT32*                        pStreamIndex,
        const   ResolutionInformation* pModeInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUseCase
    ///
    /// @brief  Method to validate width
    ///
    /// @param  pSensorPickMode Pointer to SensorPickMode
    ///
    /// @return True if aspect ratio condition matches else false
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static SensorPickModeUseCase GetUseCase(
        const SensorPickModeProperties* pSensorPickMode);

    ResolutionInformation* m_pModeInfo; ///< pointer to Mode information
};

CAMX_NAMESPACE_END

#endif // CAMXSENSORPICKMODE_H
