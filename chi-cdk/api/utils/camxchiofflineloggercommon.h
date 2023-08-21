////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchiofflineloggercommon.h
/// @brief Debug Print related defines
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXOFFLINELOGGERCOMMON_H
#define CAMXOFFLINELOGGERCOMMON_H

#define OFFLINELOG_MAX_TYPE (static_cast<UINT>(OfflineLoggerType::NUM_OF_TYPE))

enum class OfflineLoggerStatus
{
    ACTIVE,          ///< Offlinelogger is active mode
    CAMERA_CLOSING,  ///< Offlinelogger is a special mode while camera close.
    DISABLE          ///< Offlinelogger is disable
};

enum class OfflineLoggerType
{
    ASCII,           ///< Offlinelogger type for ASCII
    BINARY,          ///< Offlinelogger type for Binary
    NUM_OF_TYPE      ///< Calculate total number of offlinelogger type, do not remove it
};

#endif // CAMXOFFLINELOGGERCOMMON_H