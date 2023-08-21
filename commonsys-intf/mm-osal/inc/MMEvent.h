#ifndef MMEVENT_H
#define MMEVENT_H

/*===========================================================================
                          M M    W r a p p e r
                        f o r   F i l e   S e r v i c e s

*//** @file MMEvent.h
  This file defines the interfaces the support event operations such as create,
  destroy, set, wait and reset.

Copyright (c) 2017 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
*//*========================================================================*/

/*===========================================================================
                             Edit History

$Header:

when       who         what, where, why
--------   ---         -------------------------------------------------------
2/4/18   awaism      Created file.

============================================================================*/

/* =======================================================================

                DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains definitions for constants, macros, types, variables
and other items needed by this module.

========================================================================== */

#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>

/* -----------------------------------------------------------------------
** Type Declarations
** ----------------------------------------------------------------------- */
#ifndef _MM_HANDLE
typedef void* MM_HANDLE;
#define _MM_HANDLE
#endif

#include <pthread.h>
#include "MMDebugMsg.h"

/*
 * Creates an Event
 *
 * @param[out] phEvent - pointer to event handle
 *
 * @return return value 0 is success else failure
 */
int MM_Event_Create(MM_HANDLE* phEvent);

/*
 * Destroys an Event
 *
 * @param[in] hEvent - event handle
 *
 * @return return value 0 is success else failure
 */
int MM_Event_Destroy(MM_HANDLE hEvent);

/*
 * Set event state.
 *
 * @param[in] hEvent - event handle
 *
 * @return return value 0 is success else failure
 */
int MM_Event_Set(MM_HANDLE hEvent);

/*
 * Clears event state.
 *
 * @param[in] hEvent - event handle
 *
 * @return return value 0 is success else failure
 */
int MM_Event_Reset(MM_HANDLE hEvent);

/*
 * Configure Event to Manual Reset mode.
 *
 * @param[in] hEvent - event handle
 * @param[in] bRest - TRUE or FALSE
 *
 * @return return value 0 is success else failure
 */
int MM_Event_ManualReset(MM_HANDLE hEvent, bool bReset);

/*
 * Wait on event state.
 *
 * @param[in] hEvent - event handle
 * @param[in] uTimeoutInMs - timeout in milliseconds
 *
 * @return return value 0 is success else failure
 */
int MM_Event_Wait(MM_HANDLE hEvent, unsigned int uTimeoutInMs);

#endif
