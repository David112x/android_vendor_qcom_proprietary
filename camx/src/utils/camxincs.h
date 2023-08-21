////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 - 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxincs.h
/// @brief CamX common header file
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXINCS_H
#define CAMXINCS_H

#ifdef _WINDOWS
// NOWHINE FILE PR006:
#pragma warning (disable : 4100) // unreferenced formal parameter
#pragma warning (disable:  4189) // local variable is initialized but not referenced
#pragma warning (disable : 4201) // nonstandard extension used : nameless struct/union
#pragma warning (disable : 4505) // unreferenced local function has been removed
#pragma warning (disable : 4366) // The result of the unary '&' operator may be unaligned
#endif // _WINDOWS

#include "camxatomic.h"
#include "camxdebug.h"
#include "camxdebugprint.h"
#include "camxdefs.h"
#include "camxosutils.h"
#include "camxtrace.h"
#include "camxtypes.h"
#include "camxutils.h"

#endif // CAMXINCS_H