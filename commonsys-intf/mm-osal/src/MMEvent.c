/*===========================================================================
                          M M   W r a p p e r
                        f o r   E v e nt   S e r v i c e s

*//** @file MMEvent.c
  This file implements the Event primitives for synchronization.

Copyright (c) 2018 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
*//*========================================================================*/

/*===========================================================================
                             Edit History


when       who         what, where, why
--------   ---         -------------------------------------------------------
2/4/18   awaism      Created file.

============================================================================*/

/*===========================================================================
 Include Files
============================================================================*/
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include "MMEvent.h"
#include "MMDebugMsg.h"

/* =======================================================================

                DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains definitions for constants, macros, types, variables
and other items needed by this module.

========================================================================== */
typedef struct _event_t
{
  bool bManualReset;
  volatile int bSet;
  pthread_mutex_t mutex;
  pthread_cond_t condition;
} event_t;


/*
 * Creates an Event
 *
 * @param[out] phEvent - pointer to event handle
 *
 * @return return value 0 is success else failure
 */
int MM_Event_Create(MM_HANDLE* phEvent)
{
  int nResult = 1; // failure
  pthread_condattr_t attr;

  if (phEvent)
  {
    event_t *pEvent = (event_t*) malloc(sizeof(*pEvent));
    if (pEvent)
    {
      memset(pEvent, 0, sizeof(*pEvent));
      nResult = pthread_condattr_init(&attr);
      if (0 != nResult)
      {
        MM_MSG_PRIO1(MM_GENERAL, MM_PRIO_ERROR, "MM_Event_Create:pthread_condattr_init - %s", strerror(nResult) );
        free(pEvent);
      }
      else if (0 != (nResult = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC)))
      {
        MM_MSG_PRIO1(MM_GENERAL, MM_PRIO_ERROR, "MM_Event_Create:pthread_condattr_setclock - %s", strerror(nResult) );
        pthread_condattr_destroy(&attr);
        free(pEvent);
      }
      else if (0 != (nResult = pthread_mutex_init(&pEvent->mutex, NULL)))
      {
        MM_MSG_PRIO1(MM_GENERAL, MM_PRIO_ERROR, "pthread_mutex_init - %s", strerror(nResult) );
        pthread_condattr_destroy(&attr);
        free(pEvent);
      }
      else if (0 != (nResult = pthread_cond_init(&pEvent->condition, &attr)))
      {
        MM_MSG_PRIO1(MM_GENERAL, MM_PRIO_ERROR, "pthread_cond_init - %s", strerror(nResult) );
        pthread_mutex_destroy(&pEvent->mutex);
        pthread_condattr_destroy(&attr);
        free(pEvent);
      }
      else
      {
        /* success */
         *phEvent = pEvent;
          nResult = 0;
      }

      if (nResult) nResult = 1;
    }
  }

  return nResult;
}

/*
 * Destroys an Event
 *
 * @param[in] hEvent - event handle
 *
 * @return return value 0 is success else failure
 */
int MM_Event_Destroy(MM_HANDLE hEvent)
{
  int nResult = 1; //failure

  if (hEvent)
  {
    event_t *pEvent = (event_t*) (hEvent);
    pthread_cond_destroy(&pEvent->condition);
    pthread_mutex_destroy(&pEvent->mutex);
    free(pEvent);
    nResult = 0;
  }

  return nResult;
}

/*
 * Set event state.
 *
 * @param[in] hEvent - event handle
 *
 * @return return value 0 is success else failure
 */
int MM_Event_Set(MM_HANDLE hEvent)
{
  int nResult = 1; //failure

  event_t *pEvent = (event_t*) hEvent;

  if (pEvent)
  {
    pthread_mutex_lock(&pEvent->mutex);
    /* "Setting an event that is already set has no effect." */
    if (!pEvent->bSet)
    {
      pEvent->bSet = 1;
      pthread_cond_signal(&pEvent->condition);
    }
    else
    {
      MM_MSG_PRIO1(MM_GENERAL, MM_PRIO_HIGH, "Event handle = 0x%p already set", hEvent);
    }

    pthread_mutex_unlock(&pEvent->mutex);
    nResult = 0;
  }
  return nResult;
}

/*
 * Clears event state.
 *
 * @param[in] hEvent - event handle
 *
 * @return return value 0 is success else failure
 */
int MM_Event_Reset(MM_HANDLE hEvent)
{
  int nResult = 1; //failure

  event_t *pEvent = (event_t*) hEvent;

  if (pEvent)
  {
    pthread_mutex_lock(&pEvent->mutex);
    pEvent->bSet = 0;
    pthread_mutex_unlock(&pEvent->mutex);
    nResult = 0;
  }
  return nResult;
}

static void unlock_mutex(void* handle)
{
  if (handle)
  {
    pthread_mutex_unlock((pthread_mutex_t*) handle);
  }
}

/*
 * Configure Event to Manual Reset mode.
 *
 * @param[in] hEvent - event handle
 * @param[in] bRest - TRUE or FALSE
 *
 * @return return value 0 is success else failure
 */
int MM_Event_ManualReset(MM_HANDLE hEvent, bool bReset)
{
   int nResult = 1;
   event_t *pEvent = (event_t*) hEvent;

   if (pEvent)
   {
     pthread_mutex_lock(&pEvent->mutex);
     pEvent->bManualReset = bReset;
     pthread_mutex_unlock(&pEvent->mutex);
     nResult = 0;
   }

   return nResult;
}


/*
 * Wait on event state.
 *
 * @param[in] hEvent - event handle
 * @param[in] uTimeoutInMs - timeout in milliseconds
 *
 * @return return value 0 is success else failure
 */
int MM_Event_Wait(MM_HANDLE hEvent, unsigned int uTimeoutInMs)
{
  int nResult = 1; //failure
  event_t *pEvent = (event_t*) hEvent;

  if (pEvent)
  {
    int status = 0;

    pthread_mutex_lock(&pEvent->mutex);
    pthread_cleanup_push(unlock_mutex, &pEvent->mutex);

    if (0 == uTimeoutInMs)
    {
       status = pEvent->bSet ? 0:ETIMEDOUT;
    }
    else if (0xFFFFFFFF == uTimeoutInMs)
    {
      while (!pEvent->bSet && (0 == status))
      {
        status = pthread_cond_wait(&pEvent->condition,&pEvent->mutex);
      }
    }
    else
    {
      struct timespec timeout;
      long nsec;

      clock_gettime(CLOCK_MONOTONIC, &timeout);
      timeout.tv_sec = timeout.tv_sec + uTimeoutInMs / 1000;
      nsec = timeout.tv_nsec + ((uTimeoutInMs % 1000) * 1000) * 1000;
      if (nsec >= (1000 * 1000 * 1000))
      {
        nsec -= (1000 * 1000 * 1000);
        timeout.tv_sec += 1;
      }
      timeout.tv_nsec = nsec;
      if (!pEvent->bSet)
      {
        status = pthread_cond_timedwait(&pEvent->condition, &pEvent->mutex, &timeout);
        if (EINVAL == status)
        {
          MM_MSG_PRIO1(MM_GENERAL, MM_PRIO_ERROR, "MM_Event_Wait %s", strerror(status) );
        }
      }
    }
    pthread_cleanup_pop(0);

    switch (status)
    {
      case 0:
        nResult = 0;
        /* handle auto event */
        if (!pEvent->bManualReset)
        {
          pEvent->bSet = 0; /* auto clear */
        }
        break;
      case ETIMEDOUT:
        break;
      default:
        /* some error occured - don't care what it is */
        MM_MSG_PRIO1( MM_GENERAL, MM_PRIO_ERROR, "pthread_cond_timedwait: %s", strerror(status) );
    }
    pthread_mutex_unlock(&pEvent->mutex);
  }
  return nResult;
}
