/******************************************************************************
#  Copyright (c) 2015-2017 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
/******************************************************************************
  @file    qcril.c
  @brief   qcril qmi core

  DESCRIPTION


******************************************************************************/

/*===========================================================================

                           INCLUDE FILES

===========================================================================*/

#include "qcril_memory_management.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>

#include <cutils/properties.h>
#include "telephony/ril.h"
#include "IxErrno.h"
#include <framework/qcril_event.h>
#include <translators/android/utils.h>
#include "qcrili.h"
#include "modules/nas/qcril_arb.h"
#include "qcril_log.h"
#include "qcril_reqlist.h"
#include "qcril_other.h"
#include "qcril_qmi_client.h"
#include "modules/nas/qcril_db.h"

#include "qcril_pbm.h"
#include "modules/nas/qcril_qmi_nas.h"
#include "modules/nas/qcril_qmi_prov.h"
#include "modules/nas/qcril_qmi_nas2.h"
#include "modules/sms/qcril_qmi_sms.h"
#include "modules/voice/qcril_qmi_voice.h"

#include "qcril_qmi_oem_events.h"
#include "qcril_qmi_oemhook_agent.h"
#include "qcril_qmi_oem_packing.h"
#include "qcril_qmi_oem_reqlist.h"

extern "C" {
#include "mdm_detect.h"
}
#include "qcril_am.h"
#include "qmi_ril_peripheral_mng.h"

#include "qmi_ril_file_observer.h"

#include "qcril_qmi_radio_config_dispatch_helper.h"
#include "qcril_qmi_radio_config_socket.h"
#include "settings.h"
#include <qtibus/Messenger.h>
#include <qtibus/QtiBusTransportServer.h>

#ifdef QMI_RIL_UTF
#include "modules/mbn/qcril_qmi_pdc.h"
#include "modules/mbn/qcril_mbn_hw_update.h"
#include "modules/mbn/qcril_mbn_sw_update.h"
#include "qcril_qmi_ims.h"
void qcril_qmi_hal_uim_module_cleanup();
void qcril_qmi_hal_gstk_module_cleanup();
void qcril_qmi_hal_lpa_module_cleanup();
void qcril_qmi_hal_android_ims_radio_module_cleanup();
void qcril_qmi_hal_qcril_am_cleanup();
#ifdef FEATURE_QCRIL_LTE_DIRECT
void qcril_qmi_hal_lte_direct_module_cleanup();
#endif
#endif
/*===========================================================================

                   INTERNAL DEFINITIONS AND TYPES

===========================================================================*/
#undef TAG
#define TAG "qcril_qmi_core"

#define QCRIL_PRIMARY_MODEM_INFO_LOCK()   primary_modem_info.mdm_mutex.lock();
#define QCRIL_PRIMARY_MODEM_INFO_UNLOCK() primary_modem_info.mdm_mutex.unlock();;

#define QCRIL_SECONDARY_MODEM_INFO_LOCK()   secondary_modem_info.mdm_mutex.lock();
#define QCRIL_SECONDARY_MODEM_INFO_UNLOCK() secondary_modem_info.mdm_mutex.unlock();

/*===========================================================================

                         LOCAL VARIABLES

===========================================================================*/

/* QCRIL internal info */
qcril_arb_state_struct_type qcril_state;

/* QCRIL timer id */
qtimutex::QtiSharedMutex qcril_timer_id_mutex; /*!< Mutex to control access/update of QCRIL Timer ID*/
static uint16 qcril_timer_id;         /*!< Next QCRIL Timer ID */

/* QCRIL timer list */
qtimutex::QtiSharedMutex qcril_timed_callback_list_mutex;
static qcril_timed_callback_info *qcril_timed_callback_list = NULL;

static qtimutex::QtiSharedMutex qmi_ril_common_critical_section;

/* QCRIL Heap Memory list*/
qcril_heap_list_info *qcril_heap_memory_list = NULL;
uint32 heap_memory_id;

static boolean is_heap_memory_tracked;
static qtimutex::QtiSharedMutex qcril_heap_memory_list_mutex;
#define QCRIL_TRACK_HEAP_MEM             "persist.vendor.radio.track_heap_mem"

/* Time (1 second) to wait for the completion of modem restart before re-initiate QCRIL */
static const struct timeval TIMEVAL_DELAY = {1,0};
static const struct timeval HEAP_MEM_LIST_PRINT_TIMEVAL_DELAY = {60,0};

#define QCRIL_REQUEST_SUPPRESS_MAX_LEN 4
static qmi_ril_suppress_event_type qcril_request_suppress_list[QCRIL_REQUEST_SUPPRESS_MAX_LEN];

/* QCRIL request supress list mutex. */
qtimutex::QtiSharedMutex qcril_request_supress_list_mutex;

#define QMI_RIL_INI_RETRY_GAP_SEC     1

static qcril_instance_id_e_type            qmi_ril_process_instance_id;
static uint32_t                            qmi_ril_sim_slot; ///< sim slot associated w/this ril instance
static pthread_t                           qmi_ril_init_retry_thread_pid;

typedef struct esoc_mdm_info {
    boolean                             pm_feature_supported;
    boolean                             esoc_feature_supported;
    int                                 esoc_fd;
    int                                 voting_state; // 0 - vote released; 1 - vote activated
    char                                link_name[MAX_NAME_LEN];
    char                                modem_name[MAX_NAME_LEN];
    char                                esoc_node[MAX_NAME_LEN];
    qtimutex::QtiSharedMutex              mdm_mutex;
    MdmType                             type;
} qcril_mdm_info;

/*===========================================================================

                    EXTERNAL FUNCTION PROTOTYPES


===========================================================================*/

/*===========================================================================

                    INTERNAL FUNCTION PROTOTYPES


===========================================================================*/

static void onRequest( int request, void *data, size_t datalen, RIL_Token t );

static RIL_Errno qmi_ril_fw_android_request_render_execution( RIL_Token token,
                                                       qcril_evt_e_type android_request_id,
                                                       void * android_request_data,
                                                       int android_request_data_len,
                                                       qcril_instance_id_e_type  instance_id,
                                                       int * is_dedicated_thread);

static RIL_Errno qmi_ril_core_init(void);
static RIL_Errno qmi_ril_initiate_core_init_retry(void);
static void * qmi_ril_core_init_kicker_thread_proc(void* empty_param);
static void qmi_ril_initiate_bootup(void);
static void qmi_ril_bootup_perform_core_or_start_polling(void * params);
static void qmi_ril_core_init_kicker_main_threaded_proc(void* empty_param);
// EVENT REFACTOR cleanup static void qcril_free_request_list_entry_deferred( qcril_timed_callback_handler_params_type * handler_params );

pthread_t qmi_ril_fw_get_main_thread_id();

static void qmi_ril_oem_hook_init();

boolean qcril_qmi_is_pm_voting_feature_supported_for_primary_modem(void);
boolean qcril_qmi_is_pm_voting_feature_supported_for_secondary_modem(void);

static const RIL_RadioFunctions qcril_request_api = {
  QCRIL_RIL_VERSION, onRequest, NULL, NULL, NULL, NULL };

static pthread_t qmi_ril_main_thread_id;

/* modem esoc/subsys info */
static qcril_mdm_info primary_modem_info;
static qcril_mdm_info secondary_modem_info;

extern qmi_ril_oem_hook_overview_type qmi_ril_oem_hook_overview;

#ifdef RIL_SHLIB
struct RIL_Env *qcril_response_api[ QCRIL_MAX_INSTANCE_ID ]; /*!< Functions for ril to call */
#endif /* RIL_SHLIB */

/*===========================================================================

                                FUNCTIONS

===========================================================================*/


//===========================================================================
// qmi_ril_clear_timed_callback_list
//===========================================================================
void qmi_ril_clear_timed_callback_list()
{
  qcril_timed_callback_info *cur = NULL;
  qcril_timed_callback_info *next = NULL;
  QCRIL_MUTEX_LOCK( &qcril_timed_callback_list_mutex, "timed callback list mutex");
  cur = qcril_timed_callback_list;

  while ( NULL != cur )
  {
    if (cur->need_free && NULL != cur->extra_params)
    {
      qcril_free(cur->extra_params);
    }
    next = cur->next;
    qcril_free(cur);
    cur = next;
  }

  qcril_timed_callback_list = NULL;
  QCRIL_MUTEX_UNLOCK( &qcril_timed_callback_list_mutex, "timed callback list mutex");

  // add heap memory print callback back
  if ( 1 == is_heap_memory_tracked )
  {
    qcril_setup_timed_callback(QCRIL_DEFAULT_INSTANCE_ID, QCRIL_DEFAULT_MODEM_ID,
                               qcril_print_heap_memory_list, &HEAP_MEM_LIST_PRINT_TIMEVAL_DELAY, NULL );
  }

  QCRIL_LOG_FUNC_RETURN();
} // qmi_ril_clear_timed_callback_list

//===========================================================================
// qmi_ril_is_multi_sim_feature_supported
//===========================================================================
int qmi_ril_is_multi_sim_feature_supported()
{
   return ( qmi_ril_is_feature_supported(QMI_RIL_FEATURE_DSDS) ||
            qmi_ril_is_feature_supported(QMI_RIL_FEATURE_TSTS) );
} // qmi_ril_is_multi_sim_feature_supported

//===========================================================================
// qmi_ril_is_qcom_ril_version_supported
//===========================================================================
int qmi_ril_is_qcom_ril_version_supported(int version)
{
  int res = FALSE;

  #if defined(RIL_QCOM_VERSION)
  if( version > QMI_RIL_ZERO && RIL_QCOM_VERSION >= version )
  {
    res = TRUE;
  }
  #else
  QCRIL_NOTUSED(version);
  #endif

  return res;
} //qmi_ril_is_qcom_ril_version_supported

//===========================================================================
// ril_to_uim_is_tsts_enabled
//===========================================================================
int ril_to_uim_is_tsts_enabled(void)
{
  return qmi_ril_is_feature_supported( QMI_RIL_FEATURE_TSTS );
} // ril_to_uim_is_tsts_enabled

//===========================================================================
// ril_to_uim_is_dsds_enabled
//===========================================================================
int ril_to_uim_is_dsds_enabled(void)
{
  return (qmi_ril_is_feature_supported( QMI_RIL_FEATURE_DSDS ));
} // ril_to_uim_is_dsds_enabled

qcril_timed_callback_info **_qcril_find_timed_callback_locked(uint32 timer_id)
{
  qcril_timed_callback_info **i;

  for (i = &qcril_timed_callback_list; *i ; i = &((*i)->next)) {
    if ((*i)->timer_id == timer_id) {
      break;
    }
  }

  return i;

}
qcril_timed_callback_info *qcril_find_and_remove_timed_callback(uint32 timer_id)
{
  qcril_timed_callback_info **i = NULL, *ret = NULL;
  QCRIL_MUTEX_LOCK( &qcril_timed_callback_list_mutex, "timed callback list mutex");

  i = _qcril_find_timed_callback_locked(timer_id);

  ret = *i;
  if ( NULL != ret )
  {
    if ( NULL != ret->next )
    {
      ret->next->prev = ret->prev;
    }
    *i = ret->next;

    ret->next = NULL;
    ret->prev = NULL;
  }

  QCRIL_MUTEX_UNLOCK( &qcril_timed_callback_list_mutex, "timed callback list mutex");
  return ret;

}

void qcril_add_timed_callback(qcril_timed_callback_info *info)
{
  qcril_timed_callback_info **i;
  qcril_timed_callback_info *prev = NULL;
  QCRIL_MUTEX_LOCK( &qcril_timed_callback_list_mutex, "timed callback list mutex");

  for (i = &qcril_timed_callback_list; *i; i = &((*i)->next))
  {
    prev = *i;
  }
  *i = info;
  info->next = NULL;
  info->prev = prev;
  QCRIL_MUTEX_UNLOCK( &qcril_timed_callback_list_mutex, "timed callback list mutex");
}

/*=========================================================================
  FUNCTION:  qcril_timed_callback_dispatch

===========================================================================*/
/*!
    @brief
    Dispatch function for all timed callbacks

    @return
    void
*/
/*=========================================================================*/
void qcril_timed_callback_dispatch
(
  void *param
)
{
  uint32            timer_id = (uintptr_t) param;
  RIL_TimedCallback cb;
  qcril_timed_callback_info *info = qcril_find_and_remove_timed_callback(timer_id);

  if (info)
  {
    cb = (RIL_TimedCallback)info->callback;//void* to function pointer
    cb((void *)(uintptr_t)timer_id);
    qcril_free(info);
  }
} /* qcril_timed_callback_dispatch */

/*=========================================================================
  FUNCTION:  qcril_setup_timed_callback

===========================================================================*/
/*!
    @brief
    Setup RIL callback timer

    @return
    0 on success.
*/
/*=========================================================================*/
int qcril_setup_timed_callback
(
  qcril_instance_id_e_type instance_id,
  qcril_modem_id_e_type modem_id,
  RIL_TimedCallback callback,
  const struct timeval *relativeTime,
  uint32 *timer_id
)
{
  qcril_timed_callback_info *tcbinfo = NULL;
  IxErrnoType ret = E_FAILURE;
  uint32 the_timer_id;

  if ((instance_id < QCRIL_MAX_INSTANCE_ID) && (modem_id < QCRIL_MAX_MODEM_ID))
  {
      tcbinfo = qcril_malloc2(tcbinfo);
      if (tcbinfo)
      {

        QCRIL_MUTEX_LOCK( &qcril_timer_id_mutex, "qcril_timer_id_mutex" );

        /* Most Significant 16 bits are the Instance ID + Modem ID and Least Significant 16 bits are the QCRIL Timer ID */
        the_timer_id = ( uint32 ) QCRIL_COMPOSE_USER_DATA( instance_id, modem_id, qcril_timer_id );
        qcril_timer_id++;
        if( 0 == qcril_timer_id )
        {
            qcril_timer_id = 1;
        }

        QCRIL_MUTEX_UNLOCK( &qcril_timer_id_mutex, "qcril_timer_id_mutex" );

        tcbinfo->timer_id = the_timer_id;
        tcbinfo->callback = (void*)callback; //function pointer to void*

        qcril_add_timed_callback(tcbinfo);

        if ( relativeTime != NULL )
        {
          QCRIL_LOG_DEBUG( "Sec - %d usec - %d", relativeTime->tv_sec, relativeTime->tv_usec);
        }
        else
        {
          QCRIL_LOG_DEBUG("Immediate call back");
        }

        qcril_response_api[ instance_id ]->RequestTimedCallback( qcril_timed_callback_dispatch,
                                                                 (void *)(uintptr_t)the_timer_id, relativeTime );

        QCRIL_LOG_DEBUG( "Set timer with ID %d", the_timer_id );

        if (timer_id)
        {
          *timer_id = the_timer_id;
        }
        ret = E_SUCCESS;
      }
      else
      {
        QCRIL_LOG_ERROR("Memory allocation failed..");
        ret = E_NO_MEMORY;
      }
  }
  else
  {
    QCRIL_LOG_ERROR("Internal error(E_FAILURE)..invalid instance_id %d or modem_id %d",
                    instance_id, modem_id);
    ret = E_FAILURE;
  }

  return ret;
} /* qcril_setup_timed_callback */

//===========================================================================
// qcril_timed_callback_dispatch_expra_params
//===========================================================================
static void qcril_timed_callback_dispatch_expra_params
(
  void *param
)
{
  uint32                                    timer_id = (uint32)(uintptr_t) param;
  qcril_timed_callback_type                 cb;
  qcril_timed_callback_info *               info = qcril_find_and_remove_timed_callback(timer_id);
  qcril_timed_callback_handler_params_type  handler_params;

  if (info)
  {
    memset( &handler_params, 0, sizeof( handler_params ) );
    handler_params.timer_id     = timer_id;
    handler_params.custom_param = info->extra_params;

    cb = (qcril_timed_callback_type)info->callback; //void* to fun ptr

    cb( &handler_params );

    qcril_free( info );
  }
} // qcril_timed_callback_dispatch_expra_params

//===========================================================================
// qcril_setup_timed_callback_ex_params
//===========================================================================
int qcril_setup_timed_callback_ex_params
(
  qcril_instance_id_e_type      instance_id,
  qcril_modem_id_e_type         modem_id,
  qcril_timed_callback_type     callback,
  void*                         extra_params,
  const struct timeval *        relativeTime,
  uint32 *                      timer_id
)
{
  return qcril_setup_timed_callback_ex_params_adv(instance_id, modem_id, callback, extra_params, FALSE, relativeTime, timer_id);
}

//===========================================================================
// qcril_setup_timed_callback_ex_params_adv
//===========================================================================
int qcril_setup_timed_callback_ex_params_adv
(
  qcril_instance_id_e_type      instance_id,
  qcril_modem_id_e_type         modem_id,
  qcril_timed_callback_type     callback,
  void*                         extra_params,
  boolean                       need_free,
  const struct timeval *        relativeTime,
  uint32 *                      timer_id
)
{
  qcril_timed_callback_info *tcbinfo = NULL;
  int ret = -1;
  uint32 the_timer_id;


  if ((instance_id < QCRIL_MAX_INSTANCE_ID) && (modem_id < QCRIL_MAX_MODEM_ID))
  {
      tcbinfo = qcril_malloc2(tcbinfo);
      if (tcbinfo)
      {

        QCRIL_MUTEX_LOCK( &qcril_timer_id_mutex, "qcril_timer_id_mutex" );

        /* Most Significant 16 bits are the Instance ID + Modem ID and Least Significant 16 bits are the QCRIL Timer ID */
        the_timer_id = ( uint32 ) QCRIL_COMPOSE_USER_DATA( instance_id, modem_id, qcril_timer_id );
        qcril_timer_id++;
        if( 0 == qcril_timer_id )
        {
            qcril_timer_id = 1;
        }

        QCRIL_MUTEX_UNLOCK( &qcril_timer_id_mutex, "qcril_timer_id_mutex" );

        tcbinfo->timer_id     = the_timer_id;
        tcbinfo->callback     = (void*)callback;
        tcbinfo->extra_params = extra_params;
        tcbinfo->need_free    = need_free;

        qcril_add_timed_callback(tcbinfo);

        qcril_response_api[ instance_id ]->RequestTimedCallback( qcril_timed_callback_dispatch_expra_params,
                                                                 (void *)(uintptr_t)the_timer_id,
                                                                 relativeTime );

        QCRIL_LOG_DEBUG( "Set timer with ID %d. extra_params: %p. need_free: %s", the_timer_id, extra_params, need_free ? "true" : "false" );

        if (timer_id)
        {
          *timer_id = the_timer_id;
        }
        ret = 0;
      }
  }

  return ret;
} // qcril_setup_timed_callback_ex_params_adv


/*=========================================================================
  FUNCTION:  qcril_cancel_timed_callback

===========================================================================*/
/*!
    @brief
    Cancel RIL callback timer

    @return
    0 on success.
*/
/*=========================================================================*/
int qcril_cancel_timed_callback
(
  void *param
)
{
  uint32 timer_id = (uint32)(uintptr_t) param;
  qcril_timed_callback_info *info = qcril_find_and_remove_timed_callback(timer_id);
  int ret = -1;
  /*-----------------------------------------------------------------------*/

  if (info)
  {
    ret = 0;

    if (info->need_free && NULL != info->extra_params)
    {
      qcril_free(info->extra_params);
    }

    QCRIL_LOG_DEBUG( "Cancel timer with ID %d", info->timer_id );
    qcril_free(info);
  }

  return ret;
} /* qcril_cancel_timed_callback */

/*=========================================================================
  FUNCTION:  qcril_timed_callback_active

===========================================================================*/
/*!
    @brief
    Query state of the timed callback

    @return
    0 if timer is inactive. Non-zero Otherwise
*/
/*=========================================================================*/
int qcril_timed_callback_active
(
  uint32 timer_id
)
{
  /*-----------------------------------------------------------------------*/
  qcril_timed_callback_info **info = NULL;

  QCRIL_ASSERT( info );

  QCRIL_MUTEX_LOCK( &qcril_timed_callback_list_mutex, "timed callback list mutex" );

  info = _qcril_find_timed_callback_locked(timer_id);

  QCRIL_MUTEX_UNLOCK( &qcril_timed_callback_list_mutex, "timed callback list mutex" );

  QCRIL_ASSERT( info != NULL );

  return *info !=NULL;
} /* qcril_timed_callback_active */


// EVENT REFACTOR - cleanup
#if 1
//===========================================================================
// qcril_free_request_and_dispatch_follower_request_cb
//===========================================================================
void qcril_free_request_and_dispatch_follower_request_cb(qcril_timed_callback_handler_params_type * handler_params)
{
  qcril_free_req_and_dispatch_follower_req_payload_type *payload = NULL;
  qcril_request_resp_params_type                         resp_local;
  RIL_Token                                              follower_token;

  QCRIL_LOG_FUNC_ENTRY();

  if ( NULL != handler_params)
  {
    payload = static_cast<
    qcril_free_req_and_dispatch_follower_req_payload_type *>(
        handler_params->custom_param);

    if ( NULL != payload )
    {
      follower_token = qcril_reqlist_get_follower_token( payload->instance_id, payload->t );
      if ( QMI_RIL_ZERO != follower_token && qcril_reqlist_is_auto_respond_duplicate( payload->instance_id, follower_token ) )
      {
        // drop org
        qcril_reqlist_free( payload->instance_id, payload->t );
        // auto respond for duplicate
        resp_local          = *payload->data;
        resp_local.t        = follower_token; // substitute token
        resp_local.logstr   = NULL;
        qcril_send_request_response( &resp_local );
      }
      else
      {
        qcril_reqlist_free_and_dispatch_follower_req(payload->t, payload->token_id, payload->instance_id, NULL, QMI_RIL_ZERO );
      }

      if ( NULL != payload->data)
      {
        if (NULL != payload->data->resp_pkt)
        {
          qcril_free(payload->data->resp_pkt);
        }
        qcril_free(payload->data);
      }
      qcril_free(payload);
    }
  }
  QCRIL_LOG_FUNC_RETURN();
} // qcril_free_request_and_dispatch_follower_request_cb

//===========================================================================
// qcril_free_request_list_entry_deferred
//===========================================================================
void qcril_free_request_list_entry_deferred( qcril_timed_callback_handler_params_type * handler_params )
{
  QCRIL_LOG_FUNC_ENTRY();

  if ( NULL != handler_params )
  {
    qcril_deferred_free_req_payload_type* payload = (qcril_deferred_free_req_payload_type*)handler_params;
    qcril_reqlist_free_deferred( payload->instance_id, payload->t, payload->token_id );
    qcril_free(handler_params);
  }
  QCRIL_LOG_FUNC_RETURN();
} // qcril_free_request_list_entry_deferred
#endif

void qcril_send_empty_payload_request_response(qcril_instance_id_e_type instance_id, RIL_Token t, qcril_evt_e_type request_id, RIL_Errno ril_err_no)
{
  qcril_request_resp_params_type resp_param;

  {
    qcril_default_request_resp_params( instance_id, t, request_id, ril_err_no, &resp_param );
    qcril_send_request_response( &resp_param );
  }
}

//===========================================================================
// qcril_is_internal_token
//===========================================================================
boolean qcril_is_internal_token(RIL_Token token)
{
    boolean result = FALSE;

    if ((token >= (RIL_Token)QCRIL_INTERNAL_REQ_TOKEN_BASE) &&
        (token <= (RIL_Token)QCRIL_INTERNAL_REQ_TOKEN_END))
    {
        result = TRUE;
    }

    return result;
}
/*===========================================================================

  FUNCTION: QCRIL_LOG_GET_TOKEN_ID

===========================================================================*/
/*!
    @brief
    Return the value of the Token ID.

    @return
    The value of Token ID
*/
/*=========================================================================*/
int32_t qcril_log_get_token_id
(
  RIL_Token t
)
{
  int32_t token_id = 0;

  /*-----------------------------------------------------------------------*/

  if ( t == NULL )
  {
    token_id = 0xFFFE;
  }
  else if ( t == ( void * ) QCRIL_TOKEN_ID_INTERNAL )
  {
    token_id = 0xFFFF;
  }
  else if (qcril_is_internal_token(t))
  {
    // only show low 32bits of token for 64 bit platform
    token_id = (int32_t)(long)t;
  }
  else
  {
    token_id =  (int32_t)((((long)*( (int32_t *) t )) - INT_MIN) % 10000);
  }

  return token_id;

} /* qcril_log_get_token_id */

//===========================================================================
//qcril_send_request_ack
//===========================================================================
void qcril_send_request_ack
(
  qcril_instance_id_e_type instance_id,
  RIL_Token token
)
{
  QCRIL_LOG_FUNC_ENTRY();
  if(QCRIL_RIL_VERSION >= 13)
  {
    if (token && !qcril_is_internal_token(token))
    {
       if(qcril_response_api[instance_id]->OnRequestAck != NULL)
       {
         qcril_response_api[instance_id]->OnRequestAck(token);
       }
       else
       {
         QCRIL_LOG_ERROR("No information received during init to send back the ack");
       }
    }
    else
    {
       QCRIL_LOG_DEBUG("invalid token");
    }
  }
  else
  {
    QCRIL_LOG_DEBUG("current RIL VERSION doesnt support this feature");
  }
  QCRIL_LOG_FUNC_RETURN();
}

/*=========================================================================
  FUNCTION:  qcril_default_unsol_resp_params

===========================================================================*/
/*!
    @brief
    Set default values for unsolicted response parameters.

    @return
    None
*/
/*=========================================================================*/
void qcril_default_unsol_resp_params
(
  qcril_instance_id_e_type instance_id,
  int response_id,
  qcril_unsol_resp_params_type *param_ptr
)
{
  if(instance_id < QCRIL_MAX_INSTANCE_ID && param_ptr != NULL)
  {
      param_ptr->instance_id = instance_id;
      param_ptr->response_id = response_id;
      param_ptr->resp_pkt = NULL;
      param_ptr->resp_len = 0;
      param_ptr->logstr = NULL;
  }
  else
  {
    QCRIL_LOG_FATAL("CHECK FAILED");
  }
} /* qcril_default_unsol_resp_params */

/*=========================================================================
  FUNCTION:  qcril_send_unsol_response_epilog
===========================================================================*/
static void qcril_send_unsol_response_epilog(qcril_unsol_resp_params_type *param_ptr)
{
  QCRIL_LOG_FUNC_ENTRY();
  qcril_instance_id_e_type instance_id = param_ptr->instance_id;
  char label[ 512 ];

  /* Log event packet for Unsolicited response */
  if ( param_ptr->logstr != NULL)
  {
    /* EVENT REFACTOR REVIEW: UNSOL handling */
    QCRIL_SNPRINTF( label, sizeof( label ), "%s, %s", qcril_log_lookup_event_name( qcril_android_request_get_internal_event(param_ptr->response_id) ), param_ptr->logstr );
  }
  else
  {
    QCRIL_SNPRINTF( label, sizeof( label ), "%s", qcril_log_lookup_event_name( qcril_android_request_get_internal_event(param_ptr->response_id) ) );
  }

  QCRIL_LOG_CF_PKT_RIL_UNSOL_RES( instance_id, label );

  /* Send Unsolicted RIL response */
  QCRIL_LOG_DEBUG( "UI <--- %s (%p) --- RIL [RID %d, Len %d, %s]",
                   qcril_log_lookup_event_name( qcril_android_request_get_internal_event(param_ptr->response_id) ), param_ptr->response_id,
                   param_ptr->instance_id, param_ptr->resp_len, param_ptr->logstr );


  /* Print RIL Message */
  qcril_log_print_ril_message(qcril_android_request_get_internal_event(param_ptr->response_id), RIL__MSG_TYPE__UNSOL_RESPONSE,
                               param_ptr->resp_pkt, param_ptr->resp_len, RIL_E_SUCCESS);

  if ( param_ptr->instance_id < QCRIL_MAX_INSTANCE_ID )
  {
    qcril_response_api[ param_ptr->instance_id ]->OnUnsolicitedResponse( param_ptr->response_id, param_ptr->resp_pkt,
                                                                       param_ptr->resp_len );
  }

  QCRIL_LOG_FUNC_RETURN();
} /* qcril_send_unsol_response_epilog */

/*=========================================================================
  FUNCTION:  qcril_send_unsol_response

===========================================================================*/
/*!
    @brief
    Send RIL_onUnsolicitedResponse.

    @return
    None
*/
/*=========================================================================*/
void qcril_send_unsol_response
(
  qcril_unsol_resp_params_type *param_ptr
)
{
  qmi_ril_gen_operational_status_type cur_status;

  QCRIL_LOG_FUNC_ENTRY();
  do
  {
    if ( param_ptr == NULL || param_ptr->instance_id >= QCRIL_MAX_INSTANCE_ID )
    {
      QCRIL_LOG_FATAL("invalid param");
      break;
    }

    // handling RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED differently
    if ( RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED!= param_ptr->response_id)
    {
       cur_status = qmi_ril_get_operational_status();
       if ( cur_status == QMI_RIL_GEN_OPERATIONAL_STATUS_INIT_ONGOING ||
             cur_status == QMI_RIL_GEN_OPERATIONAL_STATUS_UNRESTRICTED ||
             cur_status == QMI_RIL_GEN_OPERATIONAL_STATUS_RESUMING )
       {
         qcril_send_unsol_response_epilog(param_ptr);
       }
#ifndef QMI_RIL_UTF
       else if(RIL_UNSOL_DATA_CALL_LIST_CHANGED == param_ptr->response_id)
       {
         qcril_send_unsol_response_epilog(param_ptr);
       }
#endif
       else
       {
         QCRIL_LOG_INFO("Invalid state (%d), Blocking unsol resp %d", cur_status, param_ptr->response_id);
       }
    }

    // send RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED after unsol_resp_unlock
    if ( (RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED== param_ptr->response_id) )
    {
       qcril_send_unsol_response_epilog(param_ptr);
    }

  } while (FALSE);

  QCRIL_LOG_FUNC_RETURN();

} /* qcril_send_unsol_response */

//===========================================================================
//qcril_hook_unsol_response
//===========================================================================
void qcril_hook_unsol_response
(
  qcril_instance_id_e_type instance_id,
  qcril_evt_e_type unsol_evt,
  // Move the unsol events outof eventlist.h and into oem.h, include oem.h in qcrili.h (temp)
  // Change them into an enum, change type of this function.
  void *data,
  uint32 data_len
)
{
  char *payload = NULL;
  uint32 index = 0;
  qcril_unsol_resp_params_type unsol_resp;

  int                                       is_qmi_idl_tunelling;
  uint16                                    message_id;
  qmi_ril_oem_hook_qmi_tunneling_service_id_type     service_id;
  uint32_t                                  tlv_stream_len;
  qmi_idl_service_object_type               qmi_idl_tunneling_service_object;
  qmi_client_error_type                     idl_err;
  uint32_t                                  encoded_fact;
  uint32_t *                                int32_param;
  uint16_t *                                int16_param;

  int unsol_event = qcril_qmi_oem_get_oemhook_msg(unsol_evt);
  // EVENT REFACTOR TEST
  payload = NULL;
  do
  {
    service_id = QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_NONE;
    message_id = QMI_RIL_ZERO;

    switch ( unsol_event )
    {
      // * VT section
      case QCRIL_REQ_HOOK_VT_UNSOL_CALL_STATUS_IND:
        is_qmi_idl_tunelling = TRUE;
        service_id = QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_VT;
        message_id  = IMS_VT_CALL_STATUS_IND_V01;
        break;

      // * eMBMS section
      case QCRIL_REQ_HOOK_EMBMS_UNSOL_RSSI_IND:
        is_qmi_idl_tunelling = TRUE;
        service_id  = QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_EMBMS;
        message_id  = QMI_EMBMS_UNSOL_RSSI_IND_V01;
        break;

      case QCRIL_REQ_HOOK_EMBMS_UNSOL_SVC_STATE:
        is_qmi_idl_tunelling = TRUE;
        service_id           = QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_EMBMS;
        message_id           = QMI_EMBMS_UNSOL_EMBMS_SERVICE_STATE_IND_V01;
        break;

      case QCRIL_REQ_HOOK_EMBMS_UNSOL_ACTIVE_TMGI:
        is_qmi_idl_tunelling = TRUE;
        service_id           = QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_EMBMS;
        message_id           = QMI_EMBMS_ACTIVE_TMGI_IND_V01;
        break;

      case QCRIL_REQ_HOOK_EMBMS_UNSOL_COVERAGE:
        is_qmi_idl_tunelling = TRUE;
        service_id           = QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_EMBMS;
        message_id           = QMI_EMBMS_UNSOL_BROADCAST_COVERAGE_IND_V01;
        break;

      case QCRIL_REQ_HOOK_EMBMS_UNSOL_OSS_WARNING:
        is_qmi_idl_tunelling = TRUE;
        service_id           = QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_EMBMS;
        message_id           = QMI_EMBMS_OOS_WARNING_IND_V01;
        break;

      case QCRIL_REQ_HOOK_EMBMS_UNSOL_AVAILABLE_TMGI:
        is_qmi_idl_tunelling = TRUE;
        service_id           = QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_EMBMS;
        message_id           = QMI_EMBMS_AVAILABLE_TMGI_IND_V01;
        break;

      case QCRIL_REQ_HOOK_EMBMS_UNSOL_CELL_INFO_CHANGED:
        is_qmi_idl_tunelling = TRUE;
        service_id           = QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_EMBMS;
        message_id           = QMI_EMBMS_CELL_INFO_CHANGED_IND_V01;
        break;

      case QCRIL_REQ_HOOK_EMBMS_UNSOL_RADIO_STATE_CHANGED:
        is_qmi_idl_tunelling = TRUE;
        service_id           = QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_EMBMS;
        message_id           = QMI_EMBMS_RADIO_STATE_IND_V01;
        break;

      case QCRIL_REQ_HOOK_EMBMS_UNSOL_SAI_LIST:
        is_qmi_idl_tunelling = TRUE;
        service_id           = QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_EMBMS;
        message_id           = QMI_EMBMS_SAI_IND_V01;
        break;

      case QCRIL_REQ_HOOK_EMBMS_UNSOL_SIB16_COVERAGE:
        is_qmi_idl_tunelling = TRUE;
        service_id           = QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_EMBMS;
        message_id           = QMI_EMBMS_UNSOL_SIB16_COVERAGE_IND_V01;
        break;

      case QCRIL_REQ_HOOK_EMBMS_UNSOL_E911_STATE_CHANGED:
        is_qmi_idl_tunelling = TRUE;
        service_id           = QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_EMBMS;
        message_id           = QMI_EMBMS_E911_STATE_IND_V01;
        break;

      case QCRIL_REQ_HOOK_EMBMS_UNSOL_CONTENT_DESC_CONTROL:
        is_qmi_idl_tunelling = TRUE;
        service_id           = QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_EMBMS;
        message_id           = QMI_EMBMS_UNSOL_CONTENT_DESC_UPDATE_PER_OBJ_IND_V01;
        break;

      case QCRIL_REQ_HOOK_EMBMS_UNSOL_EMBMS_STATUS:
        is_qmi_idl_tunelling = TRUE;
        service_id           = QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_EMBMS;
        message_id           = QMI_EMBMS_UNSOL_EMBMS_STATUS_IND_V01;
        break;

      case QCRIL_REQ_HOOK_EMBMS_UNSOL_GET_INTERESTED_TMGI_LIST:
        is_qmi_idl_tunelling = TRUE;
        service_id           = QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_EMBMS;
        message_id           = QMI_EMBMS_UNSOL_GET_INTERESTED_TMGI_LIST_REQ_V01;
        break;

      //presence
      case QCRIL_REQ_HOOK_IMS_PUBLISH_TRIGGER_IND_V01:
        is_qmi_idl_tunelling = TRUE;
        service_id  = QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_PRESENCE;
        message_id  = QMI_IMSP_PUBLISH_TRIGGER_IND_V01;
        break;
      case QCRIL_REQ_HOOK_IMS_NOTIFY_XML_IND_V01:
        is_qmi_idl_tunelling = TRUE;
        service_id  = QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_PRESENCE;
        message_id  = QMI_IMSP_NOTIFY_XML_IND_V01;
        break;

      case QCRIL_REQ_HOOK_IMS_NOTIFY_IND_V01:
        is_qmi_idl_tunelling = TRUE;
        service_id  = QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_PRESENCE;
        message_id  = QMI_IMSP_NOTIFY_IND_V01;
        break;

      case QCRIL_REQ_HOOK_IMS_ENABLER_STATUS_IND:
        is_qmi_idl_tunelling = TRUE;
        service_id  = QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_PRESENCE;
        message_id  = QMI_IMSP_ENABLER_STATE_IND_V01;
        break;

      default:
        is_qmi_idl_tunelling = FALSE;
        break;
    }


    if ( is_qmi_idl_tunelling )
    {
      qmi_idl_tunneling_service_object = qmi_ril_oem_hook_qmi_idl_tunneling_get_service_object( service_id );

      if ( NULL != qmi_idl_tunneling_service_object )
      {
        idl_err = qmi_idl_get_max_message_len( qmi_idl_tunneling_service_object, QMI_IDL_INDICATION, message_id, &tlv_stream_len  );

        if ( QMI_NO_ERR == idl_err )
        {
          payload = (char *) qcril_malloc( tlv_stream_len + OEM_HOOK_QMI_TUNNELING_IND_OVERHEAD_SIZE );

          if ( NULL != payload )
          {
            QCRIL_LOG_DEBUG("max length = %d, rcvd struc len = %d, msg_id = %d", tlv_stream_len, data_len, message_id );

            encoded_fact = QMI_RIL_ZERO;
            idl_err = qmi_idl_message_encode( qmi_idl_tunneling_service_object,
                                    QMI_IDL_INDICATION,
                                    message_id,
                                    data,
                                    data_len,
                                    payload + OEM_HOOK_QMI_TUNNELING_IND_OVERHEAD_SIZE,
                                    tlv_stream_len,
                                    &encoded_fact );

            if ( QMI_NO_ERR == idl_err )
            {
              // complete the oem hook tunneling header

              // signature
              memcpy( payload, QCRIL_HOOK_OEM_NAME, OEM_HOOK_QMI_TUNNELING_IND_Q_STR_LEN_SIZE );

              // event id
              int32_param = (uint32_t*) ( payload + OEM_HOOK_QMI_TUNNELING_IND_Q_STR_LEN_SIZE );
              *int32_param = QCRIL_REQ_HOOK_UNSOL_GENERIC;

              // payload length
              int32_param = (uint32_t*) ( payload + OEM_HOOK_QMI_TUNNELING_IND_Q_STR_LEN_SIZE + OEM_HOOK_QMI_TUNNELING_IND_EVENT_ID_SIZE );
              *int32_param = encoded_fact + OEM_HOOK_QMI_TUNNELING_SVC_ID_SIZE + OEM_HOOK_QMI_TUNNELING_MSG_ID_SIZE;

              // service id
              int16_param = (uint16_t*) ( payload + OEM_HOOK_QMI_TUNNELING_IND_Q_STR_LEN_SIZE + OEM_HOOK_QMI_TUNNELING_IND_EVENT_ID_SIZE + OEM_HOOK_QMI_TUNNELING_IND_PLD_LEN_SIZE );
              *int16_param = service_id;

              // message id
              int16_param = (uint16_t*) ( payload + OEM_HOOK_QMI_TUNNELING_IND_Q_STR_LEN_SIZE + OEM_HOOK_QMI_TUNNELING_IND_EVENT_ID_SIZE +
                                         OEM_HOOK_QMI_TUNNELING_IND_PLD_LEN_SIZE + OEM_HOOK_QMI_TUNNELING_SVC_ID_SIZE );
              *int16_param = message_id;

              // finally
              qcril_default_unsol_resp_params( instance_id,
                                               (int) RIL_UNSOL_OEM_HOOK_RAW,
                                               &unsol_resp );

              unsol_resp.resp_pkt                   = ( void * ) payload;
              unsol_resp.resp_len                   = encoded_fact + OEM_HOOK_QMI_TUNNELING_IND_OVERHEAD_SIZE;

              if (!qmi_ril_is_multi_sim_oem_hook_request(unsol_evt) &&
                   qmi_ril_is_feature_supported(QMI_RIL_FEATURE_OEM_SOCKET))
              {
                qcril_qmi_oemhook_agent_send_unsol(instance_id, unsol_resp.resp_pkt,
                                                unsol_resp.resp_len);
              }
              else
              {
                qcril_send_unsol_response( &unsol_resp );
              }
            }
            else
            {
              QCRIL_LOG_ERROR( "QMI IDL - failed to compose tlv stream err %d, actually encoded len %d ", (int) idl_err, (int) encoded_fact  );
              break;
            }

          }
          else
          {
            QCRIL_LOG_ERROR( "QMI IDL - failed to allocate payload tlv stream buf, size %d ", (int) tlv_stream_len  );
            break;
          }
        }
        else
        {
          QCRIL_LOG_ERROR( "QMI IDL - unsol event decode failed to obtain message len for msg id %d, idl err %d", (int) message_id, (int) idl_err  );
          break;
        }
      }
      else
      {
        QCRIL_LOG_ERROR( "QMI IDL - unsol event decode failed to obtain svc object for svc id %d ", (int) service_id   );
        break;
      }
    }
    else
    { // legacy stream
      payload = (char *) qcril_malloc( QCRIL_OTHER_OEM_NAME_LENGTH + sizeof(unsol_event) + sizeof(data_len) + data_len );
      if ( NULL != payload )
      {
        memcpy( payload, QCRIL_HOOK_OEM_NAME, QCRIL_OTHER_OEM_NAME_LENGTH );
        index += QCRIL_OTHER_OEM_NAME_LENGTH;

        memcpy( &payload[index], &unsol_event, sizeof(unsol_event) );
        index += sizeof(unsol_event);

        memcpy( &payload[index], &data_len, sizeof(data_len) );
        index += sizeof(data_len);

        memcpy( &payload[index], data, data_len );
        index += data_len;

        qcril_default_unsol_resp_params( instance_id, (int) RIL_UNSOL_OEM_HOOK_RAW, &unsol_resp );
        unsol_resp.resp_pkt = ( void * ) payload;
        unsol_resp.resp_len = index;

        if (qmi_ril_is_feature_supported(QMI_RIL_FEATURE_OEM_IND_TO_BOTH))
        {
          qcril_qmi_oemhook_agent_send_unsol(instance_id, unsol_resp.resp_pkt,
                                          unsol_resp.resp_len);
          qcril_send_unsol_response( &unsol_resp );
        }
        else if (!qmi_ril_is_multi_sim_oem_hook_request(unsol_evt) &&
             qmi_ril_is_feature_supported(QMI_RIL_FEATURE_OEM_SOCKET))
        {
          qcril_qmi_oemhook_agent_send_unsol(instance_id, unsol_resp.resp_pkt,
                                          unsol_resp.resp_len);
        }
        else
        {
          qcril_send_unsol_response( &unsol_resp );
        }
      }
      else
      {
        QCRIL_LOG_ERROR( "qcril_malloc returned NULL" );
        break;
      }
    }

  } while ( FALSE );

  if ( NULL != payload )
  {
    qcril_free( payload );
  }
} // qcril_hook_unsol_response

/*===========================================================================

  FUNCTION:  onRequest

===========================================================================*/
/*!
    @brief
    Call from RIL to us to make a RIL_REQUEST
    Must be completed with a call to RIL_onRequestComplete()
    RIL_onRequestComplete() may be called from any thread, before or after
    this function returns.
    Returning from this routine implies the radio is ready to process another
    command (whether or not the previous command has completed).

    @return
    None.
*/
/*=========================================================================*/
void onRequest
(
  int                       request,
  void                      *data,
  size_t                    datalen,
  RIL_Token                 t
)
{
  qcril_request_params_type param{};
  qcril_request_resp_params_type resp{};
  RIL_Errno audit_result = RIL_E_GENERIC_FAILURE;
  qmi_ril_oem_hook_request_details_type oem_hook_req_details{};
  int log_dispatch_dedicated_thrd = FALSE;

  /*-----------------------------------------------------------------------*/
  QCRIL_ASSERT( t != (void *) QCRIL_TOKEN_ID_INTERNAL );
  /*-----------------------------------------------------------------------*/

  memset( &oem_hook_req_details, 0, sizeof(oem_hook_req_details) );

  do
  {
    param.event_id_android  = request;
    param.event_id          = qcril_android_request_get_internal_event(request);
    param.data              = data;
    param.datalen           = datalen;
    param.t                 = t;
    param.instance_id       = QCRIL_MAX_INSTANCE_ID;
    param.modem_id          = QCRIL_DEFAULT_MODEM_ID;
    if (!param.event_id) {
        QCRIL_LOG_DEBUG("Unable to find internal event for request %d", request);
        audit_result = RIL_E_REQUEST_NOT_SUPPORTED;
        break;
    }

    if (param.event_id_android == RIL_REQUEST_OEM_HOOK_RAW)
    {
      if (!qmi_ril_get_req_details_from_oem_req(&oem_hook_req_details,
                                                &audit_result,
                                                static_cast<unsigned char*>(data),
                                                &param,
                                                datalen))
      {
        QCRIL_LOG_DEBUG("OEM HOOK RAW request %d not supported.",
                                     oem_hook_req_details.hook_req);
        break;
      }
      if (!qmi_ril_is_feature_supported(QMI_RIL_FEATURE_OEM_SOCKET) ||
           qmi_ril_is_multi_sim_oem_hook_request(oem_hook_req_details.hook_req_event))
      {
        // oem hook qmi idl tunneling
        if (oem_hook_req_details.is_qmi_tunneling)
        {
          if (!qmi_ril_parse_oem_req_tunnelled_message(&oem_hook_req_details,
                                                       &audit_result,
                                                       &param))
          {
            break;
          }
        }
      }
      else
      {
        QCRIL_LOG_DEBUG("OEM HOOK RAW messages are supported through oem socket, "
                        "not through rild socket");
        audit_result = RIL_E_REQUEST_NOT_SUPPORTED;
        break;
      }
    }
    else
    {
      audit_result = RIL_E_REQUEST_NOT_SUPPORTED;
    }

    if (RIL_E_SUCCESS == audit_result)
    {
      audit_result = qmi_ril_fw_android_request_render_execution(param.t,
                                                                 param.event_id,
                                                                 param.data,
                                                                 param.datalen,
                                                                 param.instance_id,
                                                                 &log_dispatch_dedicated_thrd);
    }
  } while ( FALSE );

  if (RIL_E_SUCCESS != audit_result)
  {
    qcril_default_request_resp_params_ex(param.instance_id, param.t, request, audit_result, &resp);
    if ((oem_hook_req_details.hook_req > QCRIL_REQ_HOOK_BASE) &&
        (oem_hook_req_details.hook_req < QCRIL_REQ_HOOK_MAX)) {
      resp.rild_sock_oem_req = TRUE;
    }
    qcril_send_request_response(&resp);
  }
} // onRequest

//===========================================================================
//qmi_ril_ssr_in_progress
//===========================================================================
int qmi_ril_ssr_in_progress( )
{
  int                                 res = FALSE;
  qmi_ril_gen_operational_status_type status;

  status = qmi_ril_get_operational_status();
  if ( ( status == QMI_RIL_GEN_OPERATIONAL_STATUS_SUSPENDING ) ||
       ( status == QMI_RIL_GEN_OPERATIONAL_STATUS_SUSPENDED ) ||
       ( status == QMI_RIL_GEN_OPERATIONAL_STATUS_RESUME_PENDING ) ||
       ( status == QMI_RIL_GEN_OPERATIONAL_STATUS_RESUME_RETRY ) )
  {
    res = TRUE;
  }

  QCRIL_LOG_INFO( "ssr in progress %d", res );
  return res;
} // qmi_ril_ssr_in_progress


/*===========================================================================

  FUNCTION:  qcril_get_current_radio_state

===========================================================================*/
/*!
    @brief
    Return current radio state.

    @return
    The current state of the RIL
*/
/*=========================================================================*/
RIL_RadioState qcril_get_current_radio_state()
{
  RIL_RadioState radio_state = RADIO_STATE_UNAVAILABLE;
  qmi_ril_gen_operational_status_type current_state;

  QCRIL_LOG_FUNC_ENTRY();

  current_state = qmi_ril_get_operational_status();
  switch (current_state)
  {
    case QMI_RIL_GEN_OPERATIONAL_STATUS_INIT_ONGOING: // fallthrough
    case QMI_RIL_GEN_OPERATIONAL_STATUS_UNRESTRICTED: // fallthrough
    case QMI_RIL_GEN_OPERATIONAL_STATUS_RESUMING:
    case QMI_RIL_GEN_OPERATIONAL_STATUS_UNBIND:
      radio_state = qcril_qmi_nas_dms_get_current_power_state();
      break;

    default:
      radio_state = RADIO_STATE_UNAVAILABLE;
      break;
  }

  if (qcril_qmi_nas_modem_power_is_mdm_shdn_in_apm() &&
      0 == qcril_qmi_modem_power_voting_state_primary_modem() &&
      qcril_qmi_modem_power_is_voting_feature_supported_for_primary_modem())
  {
      radio_state = RADIO_STATE_OFF;
      QCRIL_LOG_INFO("setting RADIO STATE OFF");
  }

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_state);
  return radio_state;
}

/*===========================================================================

  FUNCTION:  qcril_delay_timed_cb

===========================================================================*/
/*!
    @brief
    Handle delay timer expiration.

    @return
    None.
*/
/*=========================================================================*/
static void qcril_delay_timed_cb
(
  void *param
)
{
  QCRIL_LOG_DEBUG( "Delay Timer expired with ID %d", (uint32)(uintptr_t) param );

}; /* qcril_delay_timed_cb */


/*===========================================================================

  FUNCTION:  qcril_init_state

===========================================================================*/
/*!
    @brief
    Initialize states of QCRIL.

    @return
    None.
*/
/*=========================================================================*/
static void qcril_init_state
(
  void
)
{
  qcril_arb_state_info_struct_type *s_ptr = NULL;
  int len;
  char args[ PROPERTY_VALUE_MAX ];
  char *end_ptr = NULL;
  unsigned long ret_val;

  /*-----------------------------------------------------------------------*/

  /* Initialize TIMER ID */
  qcril_timer_id = 1;
  /* initialize Timed Callback list */
  qcril_timed_callback_list = NULL;

  /* conditionally initialize heap memory tracker list*/
  is_heap_memory_tracked = 0;
  property_get( QCRIL_TRACK_HEAP_MEM, args, "" );
  len = strlen( args );
  if ( len > 0 )
  {
    ret_val = strtoul( args, &end_ptr, 0 );
    if ( ( errno == ERANGE ) && ( ret_val == ULONG_MAX ) )
    {
      QCRIL_LOG_ERROR( "Fail to convert QCRIL_TRACK_HEAP_MEM %s", QCRIL_TRACK_HEAP_MEM );
    }
    else if ( ret_val > 1 )
    {
      QCRIL_LOG_ERROR( "Invalid saved QCRIL_TRACK_HEAP_MEM %ld, use default", ret_val );
    }
    else
    {
      is_heap_memory_tracked = ( uint8 ) ret_val;
    }
  }
  if ( 1 == is_heap_memory_tracked )
  {
    qcril_heap_memory_list = NULL;
    heap_memory_id = 0;
    qcril_setup_timed_callback(QCRIL_DEFAULT_INSTANCE_ID, QCRIL_DEFAULT_MODEM_ID,
                               qcril_print_heap_memory_list, &HEAP_MEM_LIST_PRINT_TIMEVAL_DELAY, NULL );
  }

  memset( &qmi_ril_common_critical_section, 0, sizeof( qmi_ril_common_critical_section ) );

  /* Initialize internal data */
  for ( unsigned instance_id = 0; instance_id < QCRIL_MAX_INSTANCE_ID; instance_id++ )
  {
    s_ptr = &qcril_state.info[ instance_id ];


    /* Stay in GW SIM Not Ready State till Modem reports an update on GW SIM State */
    s_ptr->pri_gw_sim_state = QCRIL_SIM_STATE_NOT_READY;
    s_ptr->sec_gw_sim_state = QCRIL_SIM_STATE_NOT_READY;

    /* Stay in CDMA SIM Not Ready State till Modem reports an update on CDMA SIM State */
    s_ptr->pri_cdma_sim_state = QCRIL_SIM_STATE_NOT_READY;
    s_ptr->sec_cdma_sim_state = QCRIL_SIM_STATE_NOT_READY;

    /* No RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED needed to be reported at power-up.
    ** RILD itself will invoke the unsol event when it's done register and query
    ** QCRIL for the Radio State. */
  }

  /* This is a workaround for a bug in ril.cpp where it starts ril_event_loop()
     before adding s_fdlisten to readFds.  When ril_event_loop first calls
     select() it is only waiting on s_fdWakeupRead.  Setting this timer wakes up
     the select, and when it blocks again s_fdlisten is in the fd_set.  Otherwise
     ril_event_loop() is blocked forever, even if Java connects to the socket. */
  qcril_setup_timed_callback( QCRIL_DEFAULT_INSTANCE_ID, QCRIL_DEFAULT_MODEM_ID, qcril_delay_timed_cb,
                                         &TIMEVAL_DELAY, NULL );
} /* qcril_init_state */


/*===========================================================================

  FUNCTION:  qcril_init

===========================================================================*/
/*!
    @brief
    Initializes all QCRIL modules

    @return
    None.
*/
/*=========================================================================*/
void qcril_init
(
  int argc,
  char **argv
)
{
  (void) argc;
  (void) argv;
  /*-----------------------------------------------------------------------*/


  if (qmi_ril_is_multi_sim_feature_supported()) {
    // Start the QtiBusTransportServer only once from primary RIL instance
    if (qmi_ril_get_process_instance_id() == QCRIL_DEFAULT_INSTANCE_ID) {
      QCRIL_LOG_DEBUG("Starting QtiBusTransportServer.");
      QtiBusTransportServer::get().start();
    }
    QCRIL_LOG_DEBUG("Starting Messenger.");
    Messenger::get().start();
  }

  /* Initialize the Arbitration module. Should be done before any other initialization */
  qcril_arb_init();

  /* Initialize QCRIL states */
  qcril_init_state();

  // init oem handling fw
  qmi_ril_oem_hook_init();

  qcril_db_init();

  /* ###############################################################################################
        !!!IMPORTANT!!!
  ##################################################################################################

   (1) Use the state mutex to block QCRIL states update that could possibily triggered by any AMSS
       command callback, or AMSS event before the completion of radio state initialization.

   (2) Don't call qcril_process_event inside this block. Doing so, will end up in mutex deadlock.

  >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                                            QCRIL STATES INITIALIZATION BEGIN
  >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */


  QCRIL_MUTEX_LOCK( &qcril_state.mutex, "qcril_state_mutex" );

  qcril_reqlist_init();

  QCRIL_MUTEX_UNLOCK( &qcril_state.mutex, "qcril_state_mutex" );

  /* <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                                         QCRIL STATES INITIALIZATION END
     <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

  qmi_ril_qmi_client_pre_initialization_init();
  qmi_ril_qmi_client_pre_initialization_acquire();

#ifndef QMI_RIL_UTF
  qmi_ril_file_observer_init();
#endif

  // per technology pre-init
  qcril_am_pre_init();

#ifndef QMI_RIL_UTF
#ifdef FEATURE_QCRIL_RADIO_CONFIG_SOCKET
  qcril_qmi_radio_config_socket_init();
#endif
  settingsd_client_start();
#endif

#ifdef QMI_RIL_UTF
  qmi_ril_reset_baseband_rat_option();
  qcril_qmi_nas_pre_init();
#endif
#ifdef FEATURE_QCRIL_MBN  // TODO move to MBN module
  qcril_qmi_pdc_pre_init();
#endif

  QCRIL_LOG_FUNC_RETURN();

} /* qcril_init() */

//===========================================================================
//qmi_ril_initiate_bootup
//===========================================================================
void qmi_ril_initiate_bootup(void)
{
#ifndef QMI_RIL_UTF
  qmi_ril_bootup_perform_core_or_start_polling(NULL);
#else
  qcril_setup_timed_callback( QCRIL_DEFAULT_INSTANCE_ID, QCRIL_DEFAULT_MODEM_ID, qmi_ril_bootup_perform_core_or_start_polling, NULL, NULL );
#endif
} // qmi_ril_initiate_bootup

//===========================================================================
// qmi_ril_oem_hook_init
//===========================================================================
void qmi_ril_oem_hook_init()
{
  memset( &qmi_ril_oem_hook_overview, 0, sizeof( qmi_ril_oem_hook_overview ) );
  qmi_ril_oem_hook_overview.qmi_ril_oem_hook_qmi_tunneling_reqs_root = NULL;
} // qmi_ril_oem_hook_init

//===========================================================================
//qmi_ril_bootup_perform_core_or_start_polling
//===========================================================================
void qmi_ril_bootup_perform_core_or_start_polling(void * params)
{
  RIL_Errno init_res;

  qmi_ril_main_thread_id = pthread_self();

  qmi_ril_set_thread_name( qmi_ril_fw_get_main_thread_id(), QMI_RIL_QMI_MAIN_THREAD_NAME);
  QCRIL_LOG_FUNC_ENTRY();

  QCRIL_NOTUSED( params );

  qmi_ril_wave_modem_status(); // this should result in "modem unavailble" report

  qmi_ril_set_operational_status( QMI_RIL_GEN_OPERATIONAL_STATUS_INIT_PENDING ); // for consistency

  qmi_ril_set_operational_status( QMI_RIL_GEN_OPERATIONAL_STATUS_INIT_ONGOING );
  init_res = qmi_ril_core_init();
  QCRIL_LOG_INFO("sees %d from qmi_ril_core_init()", (int)init_res );
  if ( RIL_E_SUCCESS == init_res )
  {
    qmi_ril_wave_modem_status(); // this should trigger reporting of modem state to Android
    qmi_ril_core_init_enter_warp();
  }
  else
  {
    qmi_ril_set_operational_status( QMI_RIL_GEN_OPERATIONAL_STATUS_INIT_PENDING );
    qmi_ril_initiate_core_init_retry();
  }

  QCRIL_LOG_FUNC_RETURN();
} // qmi_ril_bootup_perform_core_or_start_polling
//qmi_ril_core_init
//===========================================================================
RIL_Errno qmi_ril_core_init(void)
{
  RIL_Errno res = RIL_E_GENERIC_FAILURE;

  QCRIL_LOG_FUNC_ENTRY();

  qcril_event_suspend(); // to ensure atomic init flow cross sub domains
  do
  {
    if (qmi_ril_is_feature_supported(QMI_RIL_FEATURE_OEM_SOCKET))
    {
      QCRIL_LOG_INFO( "%s Init OEM socket thread", __FUNCTION__ );
      qcril_qmi_oemhook_agent_init();
    }

    res = qcril_qmi_client_init();
    if ( RIL_E_SUCCESS != res )
      break;

    qcril_other_init();

#ifndef QMI_RIL_UTF
    // DATA STUB qcril_data_init();
#endif

    qcril_qmi_nas_dms_commmon_post_init();
    // after legacy qmi_client_init_complete, check early radio power
    qcril_qmi_nas_start_timer_if_early_radio_power_req_came();

    // disable data dormancy indication
    qcril_qmi_nas_disable_data_dormancy_indication();
  } while (FALSE);
  qcril_event_resume();

  QCRIL_LOG_FUNC_RETURN_WITH_RET(res);

  return res;
} // qmi_ril_core_init
//===========================================================================
//qmi_ril_core_init_enter_warp
//===========================================================================
void qmi_ril_core_init_enter_warp()
{
    qmi_ril_gen_operational_status_type cur_status;

    QCRIL_LOG_FUNC_ENTRY();
    cur_status = qmi_ril_get_operational_status();

    if( !( cur_status == QMI_RIL_GEN_OPERATIONAL_STATUS_SUSPENDING ||
         cur_status == QMI_RIL_GEN_OPERATIONAL_STATUS_SUSPENDED )
      )
    {
        // SMS REFACTOR (void)qcril_sms_perform_initial_configuration(); // SMS initialization done right away
        QCRIL_LOG_INFO( "QMI RIL! SMS init right away for both Non DSDS or DSDS" );

        if ( !qmi_ril_is_multi_sim_feature_supported() || qcril_qmi_nas_is_sim_provisioned() )
        {
            QCRIL_LOG_INFO( "!QMI RIL! 2nd phase init for NON - DSDS" );
            qmi_ril_set_operational_status( QMI_RIL_GEN_OPERATIONAL_STATUS_UNRESTRICTED );
        }
    }
    QCRIL_LOG_FUNC_RETURN();
} // qmi_ril_core_init_enter_warp

//===========================================================================
//qmi_ril_initiate_core_init_retry
//===========================================================================
RIL_Errno qmi_ril_initiate_core_init_retry(void)
{
  RIL_Errno res = RIL_E_GENERIC_FAILURE;
  pthread_attr_t attr;
  int conf;

#ifdef QMI_RIL_UTF
  pthread_attr_init (&attr);
  conf = utf_pthread_create_handler(&qmi_ril_init_retry_thread_pid, &attr, qmi_ril_core_init_kicker_thread_proc, NULL);
#else
  pthread_attr_init (&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  conf = pthread_create(&qmi_ril_init_retry_thread_pid, &attr, qmi_ril_core_init_kicker_thread_proc, NULL);
#endif
  qmi_ril_set_thread_name(qmi_ril_init_retry_thread_pid, QMI_RIL_CORE_INIT_KICKER_THREAD_NAME);

  pthread_attr_destroy(&attr);

  res =  (conf < 0 ) ? RIL_E_GENERIC_FAILURE : RIL_E_SUCCESS;
  QCRIL_LOG_FUNC_RETURN_WITH_RET(res);

  return res;
} // qmi_ril_initiate_core_init_retry

//===========================================================================
//qmi_ril_core_init_kicker_main_threaded_proc
//===========================================================================
void qmi_ril_core_init_kicker_main_threaded_proc(void* empty_param)
{
  RIL_Errno core_init_res = RIL_E_GENERIC_FAILURE;
  QCRIL_LOG_FUNC_ENTRY();

  QCRIL_NOTUSED( empty_param );
  qmi_ril_set_operational_status( QMI_RIL_GEN_OPERATIONAL_STATUS_INIT_ONGOING );
  core_init_res = qmi_ril_core_init();
  QCRIL_LOG_INFO( "iteration - %d", (int) core_init_res );

  if ( RIL_E_SUCCESS == core_init_res )
  {
    qmi_ril_core_init_enter_warp();
    qmi_ril_wave_modem_status(); // this should trigger reporting of modem state to Android
  }
  else
  {
    qmi_ril_set_operational_status( QMI_RIL_GEN_OPERATIONAL_STATUS_INIT_PENDING );
    qmi_ril_initiate_core_init_retry();
  }

  QCRIL_LOG_FUNC_RETURN();
} // qmi_ril_core_init_kicker_main_threaded_proc
//===========================================================================
//qmi_ril_core_init_kicker_thread_proc
//===========================================================================
void * qmi_ril_core_init_kicker_thread_proc(void* empty_param)
{
  QCRIL_LOG_FUNC_ENTRY();

  QCRIL_NOTUSED( empty_param );
  sleep( QMI_RIL_INI_RETRY_GAP_SEC );
  qcril_setup_timed_callback( QCRIL_DEFAULT_INSTANCE_ID, QCRIL_DEFAULT_MODEM_ID, qmi_ril_core_init_kicker_main_threaded_proc, NULL, NULL );

  QCRIL_LOG_FUNC_RETURN();

  qmi_ril_clear_thread_name(pthread_self());
  return NULL;
}  // qmi_ril_core_init_kicker_thread_proc

/*===========================================================================

  FUNCTION:  qcril_release

===========================================================================*/
/*!
    @brief
    Release AMSS client objects.

    @return
    None.
*/
/*=========================================================================*/
void qcril_release
(
  void
)
{
  /*-----------------------------------------------------------------------*/

  QCRIL_LOG_FUNC_ENTRY();

  /* For QMI_VOICE, NAS/DMS, WMS */
  qcril_qmi_client_release();

  qcril_log_cleanup();
#ifndef QMI_RIL_UTF
  if (qcril_qmi_is_pm_voting_feature_supported_for_primary_modem())
  {
    qmi_ril_peripheral_mng_deregister_pm_client_for_primary_modem();
  }

  if (qcril_qmi_is_pm_voting_feature_supported_for_secondary_modem())
  {
    // TODO: Re-evaluate this
    qmi_ril_peripheral_mng_deregister_pm_client_for_secondary_modem();
  }
#endif
} /* qcril_release()*/


//===========================================================================
// qcril_get_empty_binary_data_type
//===========================================================================
qcril_binary_data_type qcril_get_empty_binary_data_type()
{
    qcril_binary_data_type res;
    res.len = 0;
    res.data = NULL;
    return res;
} // qcril_get_empty_binary_data_type

//===========================================================================
// qcril_is_binary_data_empty
//===========================================================================
boolean qcril_is_binary_data_empty(qcril_binary_data_type bin_data)
{
    return !bin_data.len || !bin_data.data;
} // qcril_is_binary_data_empty

//===========================================================================
// qcril_find_pattern
//===========================================================================
qcril_binary_data_type qcril_find_pattern(qcril_binary_data_type bin_data, const char *pattern)
{
    size_t i;
    for (i = 0; i < bin_data.len - strlen(pattern); ++i)
    {
        boolean match = TRUE;
        size_t j;
        for (j = 0; j < strlen(pattern); ++j)
        {
            if (bin_data.data[i + j] != pattern[j])
            {
                match = FALSE;
                break;
            }
        }
        if (match)
        {
            bin_data.data += i;
            bin_data.len -= i;
            return bin_data;
        }
    }
    return qcril_get_empty_binary_data_type();
} // qcril_find_pattern

void qcril_qmi_copy_modem_info(struct mdm_info *src, qcril_mdm_info *dst)
{
    if (!src || !dst)
    {
        return;
    }

    /* Read esoc node, to be used if
     * peripheral manager is not supported */
    strlcpy(dst->esoc_node,
            src->powerup_node,
            sizeof(dst->esoc_node));

    /* Read modem name, to be used to register with
     * peripheral manager */
    strlcpy(dst->modem_name,
            src->mdm_name,
            sizeof(dst->modem_name));

    /* Read link name to find out the transport medium
     * to decide on qmi port */
    strlcpy(dst->link_name,
            src->mdm_link,
            sizeof(dst->link_name));

    dst->type = src->type;
}

/*===========================================================================

  FUNCTION: qcril_qmi_load_esoc_info

===========================================================================*/
/*!
    @brief
    Loads esoc info

    @return
    None
*/
/*=========================================================================*/
void qcril_qmi_load_esoc_info(void)
{
    struct dev_info devinfo;

    if (get_system_info(&devinfo) != RET_SUCCESS)
    {
        QCRIL_LOG_ERROR("Could not retrieve esoc info");
        return;
    }

    if (devinfo.num_modems > 2)
    {
        QCRIL_LOG_ERROR("Unexpected number of modems %d",
                         devinfo.num_modems);
        return;
    }

    unsigned short int primary_modem_idx = 0;
    unsigned short int secondary_modem_idx = 1;

    /* In the dual modem scenario, an internal modem paired with an
     * external modem is the only supported configuration. The internal
     * modem is considered the "primary" modem and the external modem
     * is considered the "secondary" modem. For example, in 4G-5G fusion
     * setup, the 4G modem is internal and primary, and the 5G modem is
     * external and secondary. */
    if (devinfo.num_modems == 2)
    {
        if (devinfo.mdm_list[0].type == devinfo.mdm_list[1].type)
        {
            QCRIL_LOG_ERROR("Invalid configuration. For dual modems, "
                            "an internal and an external modem is the "
                            "only supported configuration.");
            return;
        }

        if (devinfo.mdm_list[0].type == MDM_TYPE_EXTERNAL) {
            secondary_modem_idx = 0;
            primary_modem_idx = 1;
        }

        qcril_qmi_copy_modem_info(&devinfo.mdm_list[secondary_modem_idx], &secondary_modem_info);

        QCRIL_LOG_INFO("secondary modem name: %s, "
                       "secondary modem link name: %s, "
                       "secondary modem esoc_node: %s",
                       secondary_modem_info.modem_name,
                       secondary_modem_info.link_name,
                       secondary_modem_info.esoc_node);
        QCRIL_LOG_INFO("secondary modem type: %s",
                       secondary_modem_info.type ? "internal" : "external");
    }

    qcril_qmi_copy_modem_info(&devinfo.mdm_list[primary_modem_idx], &primary_modem_info);

    QCRIL_LOG_INFO("primary modem name: %s, primary modem link name: %s, "
                   "primary modem esoc_node: %s",
                   primary_modem_info.modem_name, primary_modem_info.link_name,
                   primary_modem_info.esoc_node);
    QCRIL_LOG_INFO("primary modem type: %s",
                   primary_modem_info.type ? "internal" : "external");
}

/*===========================================================================
  FUNCTION: qcril_qmi_get_primary_modem_name
===========================================================================*/
/*!
    @brief
    Returns the name of the primary modem.

    @return
    Primary modem name
*/
/*=========================================================================*/
char *qcril_qmi_get_primary_modem_name(void)
{
    if (strlen(primary_modem_info.modem_name) > 0)
    {
        return primary_modem_info.modem_name;
    }

    return NULL;
}

/*===========================================================================
  FUNCTION: qcril_qmi_get_secondary_modem_name
===========================================================================*/
/*!
    @brief
    Returns the name of the secondary modem.

    @return
    Secondary modem name
*/
/*=========================================================================*/
char *qcril_qmi_get_secondary_modem_name(void)
{
    if (strlen(secondary_modem_info.modem_name) > 0)
    {
        return secondary_modem_info.modem_name;
    }

    return NULL;
}

bool qcril_qmi_is_secondary_modem_present(void)
{
    return (bool) strlen(secondary_modem_info.modem_name) > 0;
}

/*===========================================================================

  FUNCTION: qcril_qmi_load_esoc_and_register_with_pm

===========================================================================*/
/*!
    @brief
    Loads esoc info

    @return
    None
*/
/*=========================================================================*/
void qcril_qmi_load_esoc_and_register_with_pm(void)
{
    char *modem_name;
    qcril_qmi_load_esoc_info();

    modem_name = qcril_qmi_get_primary_modem_name();
#ifndef QMI_RIL_UTF
    if (modem_name &&
        !qmi_ril_peripheral_mng_register_pm_client_for_primary_modem(modem_name))
    {
        QCRIL_LOG_INFO("Registered peripheral manager client for "
                       "the primary modem [%s].", modem_name);
        primary_modem_info.pm_feature_supported = TRUE;
    }

    modem_name = qcril_qmi_get_secondary_modem_name();
    if (modem_name &&
        !qmi_ril_peripheral_mng_register_pm_client_for_secondary_modem(modem_name))
    {
        QCRIL_LOG_INFO("Registered peripheral manager client for "
                       "the secondary modem [%s].", modem_name);
        secondary_modem_info.pm_feature_supported = TRUE;
    }
#endif
}

//=============================================================================
// FUNCTION: qmi_ril_get_stack_id
//
// DESCRIPTION:
// returns the modem stack id associated with current RIL instance
//
// RETURN: 0 | 1 | 2 - primary | secondary | tertiary stack id
//=============================================================================
qcril_modem_stack_id_e_type qmi_ril_get_stack_id
(
  qcril_instance_id_e_type instance_id
)
{
  QCRIL_NOTUSED(instance_id);
  return qcril_qmi_get_modem_stack_id();
}

int qcril_qmi_modem_power_voting_state_primary_modem(void)
{
    int ret;
    QCRIL_PRIMARY_MODEM_INFO_LOCK();
    ret = primary_modem_info.voting_state;
    QCRIL_PRIMARY_MODEM_INFO_UNLOCK();
    QCRIL_LOG_FUNC_RETURN_WITH_RET(ret);
    return ret;
}

int qcril_qmi_modem_power_voting_state_secondary_modem(void)
{
    int ret;
    QCRIL_SECONDARY_MODEM_INFO_LOCK();
    ret = secondary_modem_info.voting_state;
    QCRIL_SECONDARY_MODEM_INFO_UNLOCK();
    QCRIL_LOG_FUNC_RETURN_WITH_RET(ret);
    return ret;
}

void qcril_qmi_modem_power_set_voting_state_primary_modem(int state)
{
    QCRIL_PRIMARY_MODEM_INFO_LOCK();
    primary_modem_info.voting_state = state;
    QCRIL_PRIMARY_MODEM_INFO_UNLOCK();
    QCRIL_LOG_INFO("Voting state of primary modem has been set to %d",
                   state);
}

void qcril_qmi_modem_power_set_voting_state_secondary_modem(int state)
{
    QCRIL_SECONDARY_MODEM_INFO_LOCK();
    secondary_modem_info.voting_state = state;
    QCRIL_SECONDARY_MODEM_INFO_UNLOCK();
    QCRIL_LOG_INFO("Voting state set of secondary modem has been set to %d",
                   state);
}

/*===========================================================================

  function: qcril_qmi_get_esoc_link_name

===========================================================================*/
/*!
    @brief
    returns esoc mdm link name

    @return
    esoc device node link name
*/
/*=========================================================================*/
// TODO: Rename this function to "qcril_qmi_get_primary_modem_link_name"
char *qcril_qmi_get_esoc_link_name(void)
{
    if (strlen(primary_modem_info.link_name) > 0)
    {
        return primary_modem_info.link_name;
    }

    return NULL;
}

/*===========================================================================

  FUNCTION: qcril_qmi_vote_for_modem_up_using_esoc

===========================================================================*/
/*!
    @brief
        adds vote for modem using esoc , so that pil is loaded.

    @return
        None
*/
/*=========================================================================*/
void qcril_qmi_vote_for_modem_up_using_esoc(qcril_mdm_info *modem_info)
{
    char *esoc_node_name = NULL;

    QCRIL_LOG_FUNC_ENTRY();

    if (!modem_info)
    {
        QCRIL_LOG_FUNC_RETURN();
        return;
    }

    // TODO: Acquire modem info mutex

    if (strlen(modem_info->esoc_node) > 0)
    {
        esoc_node_name = modem_info->esoc_node;
    }

    if (esoc_node_name)
    {
        if (!access(esoc_node_name, F_OK))
        {
            QCRIL_LOG_INFO("esoc feature is enabled");
            modem_info->esoc_feature_supported = TRUE;
            if(modem_info->type == MDM_TYPE_EXTERNAL)
            {
                modem_info->esoc_fd = open(esoc_node_name, O_RDONLY);

                if (RIL_VALID_FILE_HANDLE > modem_info->esoc_fd)
                {
                    modem_info->esoc_feature_supported = FALSE;
                    QCRIL_LOG_ERROR("Cannot open file %s", esoc_node_name);
                }
                else
                {
                    modem_info->voting_state = 1;
                    QCRIL_LOG_INFO("Vote activated for node %s, fd %d",
                                    esoc_node_name, modem_info->esoc_fd);
                }
            }
            else
            {
                modem_info->esoc_feature_supported = FALSE;
                QCRIL_LOG_INFO("Internal modem - esoc file open not required");
            }
        }
        else
        {
            QCRIL_LOG_ERROR("ESOC node %s not accessible", esoc_node_name);
        }
    }
    else
    {
        QCRIL_LOG_ERROR("ESOC node is not available");
    }

    QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_vote_for_modem_up_using_esoc

void qcril_qmi_vote_up_primary_modem(void)
{
    QCRIL_LOG_FUNC_ENTRY();

    char *modem_name = qcril_qmi_get_primary_modem_name();
    if (!modem_name)
    {
        QCRIL_LOG_ERROR("Primary modem is not present.");
        QCRIL_LOG_FUNC_RETURN();
        return;
    }
#ifndef QMI_RIL_UTF
    QCRIL_PRIMARY_MODEM_INFO_LOCK();
    if (primary_modem_info.pm_feature_supported)
    {
        if (qmi_ril_peripheral_mng_vote_up_primary_modem())
        {
            QCRIL_LOG_ERROR("Failed to vote for primary modem [%s] "
                            "using peripheral manager.", modem_name);
        }
        else
        {
            primary_modem_info.voting_state = 1;
            QCRIL_LOG_INFO("Successfully voted for primary modem [%s] "
                           "using peripheral manager.", modem_name);
        }
    }
    else
    {
        qcril_qmi_vote_for_modem_up_using_esoc(&primary_modem_info);
        if (primary_modem_info.voting_state)
        {
            QCRIL_LOG_INFO("Successfully voted for primary modem [%s] "
                           "using esoc node.", modem_name);
        }
        else
        {
            QCRIL_LOG_ERROR("Failed to vote for primary modem [%s] "
                            "using esoc node.", modem_name);
        }
    }
    QCRIL_PRIMARY_MODEM_INFO_UNLOCK();
#endif
    QCRIL_LOG_FUNC_RETURN();
}

void qcril_qmi_vote_up_secondary_modem(void)
{
    QCRIL_LOG_FUNC_ENTRY();

    char *modem_name = qcril_qmi_get_secondary_modem_name();
    if (!modem_name)
    {
        QCRIL_LOG_ERROR("Secondary modem is not present.");
        QCRIL_LOG_FUNC_RETURN();
        return; // no-op if the secondary modem is not present
    }
#ifndef QMI_RIL_UTF
    QCRIL_SECONDARY_MODEM_INFO_LOCK();
    if (secondary_modem_info.pm_feature_supported)
    {
        if (qmi_ril_peripheral_mng_vote_up_secondary_modem())
        {
            QCRIL_LOG_ERROR("Failed to vote for secondary modem [%s] "
                            "using peripheral manager.", modem_name);
        }
        else
        {
            secondary_modem_info.voting_state = 1;
            QCRIL_LOG_INFO("Successfully voted for secondary modem [%s] "
                           "using peripheral manager.", modem_name);
        }
    }
    else
    {
        qcril_qmi_vote_for_modem_up_using_esoc(&secondary_modem_info);
        if (secondary_modem_info.voting_state)
        {
            QCRIL_LOG_INFO("Successfully voted for secondary modem [%s] "
                           "using esoc node.", modem_name);
        }
        else
        {
            QCRIL_LOG_ERROR("Failed to vote for secondary modem [%s] "
                            "using esoc node.", modem_name);
        }
    }
    QCRIL_SECONDARY_MODEM_INFO_UNLOCK();
#endif
    QCRIL_LOG_FUNC_RETURN();
}

void qcril_qmi_vote_down_primary_modem(void)
{
    QCRIL_LOG_FUNC_ENTRY();
#ifndef QMI_RIL_UTF
    QCRIL_PRIMARY_MODEM_INFO_LOCK();
    if (primary_modem_info.pm_feature_supported)
    {
        qmi_ril_peripheral_mng_vote_down_primary_modem();
        QCRIL_LOG_INFO("Successfully released vote for primary modem [%s] "
                       "using peripheral manager.",
                       qcril_qmi_get_primary_modem_name() ?
                           qcril_qmi_get_primary_modem_name() : "null");
    }
    else if (primary_modem_info.esoc_feature_supported)
    {
        // TODO: Check if close() was successful
        close(primary_modem_info.esoc_fd);
        primary_modem_info.esoc_fd = -1;
        QCRIL_LOG_INFO("Successfully released vote for primary modem [%s] "
                       "using esoc node.", primary_modem_info.esoc_node);
    }
    QCRIL_PRIMARY_MODEM_INFO_UNLOCK();
#endif
    QCRIL_LOG_FUNC_RETURN();
}

void qcril_qmi_vote_down_secondary_modem(void)
{
    QCRIL_LOG_FUNC_ENTRY();
#ifndef QMI_RIL_UTF
    QCRIL_SECONDARY_MODEM_INFO_LOCK();
    if (secondary_modem_info.pm_feature_supported)
    {
        qmi_ril_peripheral_mng_vote_down_secondary_modem();
        QCRIL_LOG_INFO("Successfully released vote for secondary modem [%s] "
                       "using peripheral manager.",
                       qcril_qmi_get_secondary_modem_name() ?
                           qcril_qmi_get_secondary_modem_name() : "null");
    }
    else if (secondary_modem_info.esoc_feature_supported)
    {
        // TODO: Check if close() was successful
        close(secondary_modem_info.esoc_fd);
        secondary_modem_info.esoc_fd = -1;
        QCRIL_LOG_INFO("Successfully released vote for secondary modem [%s] "
                       "using esoc node.", secondary_modem_info.esoc_node);
    }
    QCRIL_SECONDARY_MODEM_INFO_UNLOCK();
#endif
    QCRIL_LOG_FUNC_RETURN();
}

/*===========================================================================

  FUNCTION: qcril_qmi_modem_power_process_bootup

===========================================================================*/
/*!
    @brief
        adds vote for modem, so that pil is loaded.

    @return
        None
*/
/*=========================================================================*/
void qcril_qmi_modem_power_process_bootup(void)
{
    QCRIL_LOG_FUNC_ENTRY();
    qcril_qmi_nas_modem_power_load_apm_mdm_not_pwdn();
    if (0 == qcril_qmi_modem_power_voting_state_primary_modem())
    {
        qcril_qmi_vote_up_primary_modem();
    }
    if (0 == qcril_qmi_modem_power_voting_state_secondary_modem())
    {
        qcril_qmi_vote_up_secondary_modem();
    }
    QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_modem_power_process_bootup

/*===========================================================================

  FUNCTION: qcril_qmi_modem_power_process_apm_off

===========================================================================*/
/*!
    @brief
    vote to power up modem

    @return
    none
*/
/*=========================================================================*/
void qcril_qmi_modem_power_process_apm_off(void)
{
    QCRIL_LOG_FUNC_ENTRY();
    qcril_qmi_vote_up_primary_modem();
    qcril_qmi_vote_up_secondary_modem();
    QCRIL_LOG_FUNC_RETURN();
}

/*===========================================================================

  FUNCTION: qcril_qmi_is_pm_voting_feature_supported

===========================================================================*/
/*!
    @brief
    Retrieve if peripheral manager voting feature is supported

    @return
    TRUE or FALSE
*/
/*=========================================================================*/
boolean qcril_qmi_is_pm_voting_feature_supported_for_primary_modem(void)
{
    boolean ret = primary_modem_info.pm_feature_supported;
    QCRIL_LOG_FUNC_RETURN_WITH_RET(ret);
    return ret;
} // qcril_qmi_is_pm_voting_feature_supported

boolean qcril_qmi_is_pm_voting_feature_supported_for_secondary_modem(void)
{
    boolean ret = secondary_modem_info.pm_feature_supported;
    QCRIL_LOG_FUNC_RETURN_WITH_RET(ret);
    return ret;
} // qcril_qmi_is_pm_voting_feature_supported

/*===========================================================================

  FUNCTION: qcril_qmi_is_esoc_voting_feature_supported

===========================================================================*/
/*!
    @brief
    Retrieve if esoc voting feature is supported

    @return
    TRUE or FALSE
*/
/*=========================================================================*/
boolean qcril_qmi_is_esoc_voting_feature_supported_for_primary_modem(void)
{
    boolean ret = primary_modem_info.esoc_feature_supported;
    QCRIL_LOG_FUNC_RETURN_WITH_RET(ret);
    return ret;
} // qcril_qmi_is_esoc_voting_feature_supported_for_primary_modem

/*===========================================================================

    FUNCTION: qcril_qmi_modem_power_is_voting_feature_supported

===========================================================================*/
/*!
    @brief
    Retrieve if voting feature is supported

    @return
    TRUE or FALSE
*/
/*=========================================================================*/
boolean qcril_qmi_modem_power_is_voting_feature_supported_for_primary_modem(void)
{
    boolean ret = (qcril_qmi_is_esoc_voting_feature_supported_for_primary_modem() ||
            qcril_qmi_is_pm_voting_feature_supported_for_primary_modem());
    QCRIL_LOG_FUNC_RETURN_WITH_RET(ret);
    return ret;
} // qcril_qmi_modem_power_is_voting_feature_supported_for_primary_modem

#ifdef RIL_SHLIB
/*===========================================================================

  FUNCTION:  RIL_Init

===========================================================================*/
/*!
    @brief
    Returns the current state of the RIL

    @return
    The current state of the RIL
*/
/*=========================================================================*/
const RIL_RadioFunctions *legacy_RIL_Init
(
  const struct RIL_Env *env,
  qcril_instance_id_e_type instance_id,
  int argc,
  char **argv
)
{
  qmi_ril_set_thread_name( pthread_self() , QMI_RIL_QMI_RILD_THREAD_NAME);

  /* Ignore SIGPIPE signal so that rild does not crash
     even if the other end of the socket is closed abruptly. */
  signal(SIGPIPE, SIG_IGN);

  qcril_qmi_nas_initiate_radio_power_process();
  qcril_log_init();

  /* Load eSOC info and register with peripheral manager */
  qcril_qmi_load_esoc_and_register_with_pm();

  qmi_ril_process_instance_id = instance_id;
  qmi_ril_sim_slot = 0; // use 1st slot as default

  if (qmi_ril_process_instance_id == QCRIL_SECOND_INSTANCE_ID)
  { // 2nd RIL instance - 1, only for DSDS or DSDA
    if ( qmi_ril_is_multi_sim_feature_supported() )
    {
       qmi_ril_sim_slot = QCRIL_SECOND_INSTANCE_ID; // use 2nd slot for DSDS/TSTS
    }
    else if ( qmi_ril_is_feature_supported(QMI_RIL_FEATURE_DSDA)  ||
              qmi_ril_is_feature_supported(QMI_RIL_FEATURE_DSDA2) )
    {
       qmi_ril_sim_slot = 0; // use 1st slot for DSDA
    }
    else
    {
       QCRIL_LOG_ERROR("Unsupported configuration, can't determine sim slot");
    }
  }
  else if (qmi_ril_process_instance_id == QCRIL_THIRD_INSTANCE_ID)
  { // 3rd RIL instance, only for TSTS
    if ( qmi_ril_is_feature_supported(QMI_RIL_FEATURE_TSTS) )
    {
       qmi_ril_sim_slot = QCRIL_THIRD_INSTANCE_ID; // use 3rd slot for TSTS
    }
    else
    {
       QCRIL_LOG_ERROR("Unsupported configuration, can't determine sim slot");
    }
  }

  qmi_ril_set_sim_slot(qmi_ril_sim_slot);
  qmi_ril_set_process_instance_id(qmi_ril_process_instance_id);

  QCRIL_LOG_DEBUG( "RIL %d, running RIL_Init()", qmi_ril_process_instance_id );

  // o maintain compatibility with data and UIM code which use instance_id and may respond on "second instance" context
  qcril_response_api[ QCRIL_DEFAULT_INSTANCE_ID ] = (struct RIL_Env *) env;
  qcril_response_api[ QCRIL_SECOND_INSTANCE_ID ] = (struct RIL_Env *) env;
  // TODO_TSTS: Check if this is required. Seems not required.
  qcril_response_api[ QCRIL_THIRD_INSTANCE_ID ] = (struct RIL_Env *) env;

  // Initialize QCRIL
  qcril_init(argc, argv);

  // Start event thread
  qcril_event_start();

  qcril_log_timer_init();

  // start bootup if applicable
  qmi_ril_initiate_bootup();

  return &qcril_request_api;

} /* RIL_Init() */
#endif /* RIL_SHLIB */

//===========================================================================
// qmi_ril_fw_android_request_render_execution
//===========================================================================
RIL_Errno qmi_ril_fw_android_request_render_execution( RIL_Token token,
                                                       qcril_evt_e_type request_id,
                                                       void * android_request_data,
                                                       int android_request_data_len,
                                                       qcril_instance_id_e_type  instance_id,
                                                       int * is_dedicated_thread )
{

  RIL_Errno                                audit_result;
  IxErrnoType                              event_execute_error;
  qcril_request_params_type                param;

  QCRIL_LOG_FUNC_ENTRY();

  memset( &param, 0, sizeof( param ) );
  param.event_id          = request_id;
  param.data              = android_request_data;
  param.datalen           = android_request_data_len;
  param.t                 = token;
  param.instance_id       = instance_id;
  param.modem_id          = QCRIL_DEFAULT_MODEM_ID;

  audit_result = RIL_E_GENERIC_FAILURE;

  QCRIL_LOG_DEBUG( "rendering exec for token id %" PRId32 "", qcril_log_get_token_id( token ) );

  event_execute_error = qcril_execute_event(&param, is_dedicated_thread);

  QCRIL_LOG_DEBUG("called the function");
  switch(event_execute_error)
  {
    case E_SUCCESS:
      audit_result = RIL_E_SUCCESS;
      break;
    case E_NO_MEMORY: //fallthrough
    case E_FAILURE:
      audit_result = RIL_E_GENERIC_FAILURE;
      break;
    case E_NOT_ALLOWED:
      audit_result = RIL_E_RADIO_NOT_AVAILABLE;
      break;
    case E_NOT_SUPPORTED:
      audit_result = RIL_E_REQUEST_NOT_SUPPORTED;
      break;
    default:
      audit_result = RIL_E_INTERNAL_ERR;
      break;
  }

  QCRIL_LOG_FUNC_RETURN_WITH_RET( (int)audit_result );

  return audit_result;
} // qmi_ril_fw_android_request_render_execution
//===========================================================================
// qmi_ril_fw_get_main_thread_id
//===========================================================================
pthread_t qmi_ril_fw_get_main_thread_id()
{
  return qmi_ril_main_thread_id;
} // qmi_ril_fw_get_main_thread_id

//===========================================================================
// qmi_ril_fw_destroy_android_live_params_copy
//===========================================================================
void qmi_ril_fw_destroy_android_live_params_copy(qmi_ril_fw_android_param_copy_approach_type used_approach,
                                               qcril_evt_e_type event_id,
                                               void* four_byte_storage,
                                               void* sub_created_custom_storage)
{
  RIL_IMS_SMS_Message*  copied_android_send_ims_msg_params;
  RIL_CDMA_SMS_Message* copied_android_cdma_send_ims_param;
  char *copied_android_gw_smsc_address = NULL;
  char *copied_android_gw_pdu = NULL;
  char ** copied_android_gw_sms_ims_params;

  char** copied_cdma_dtmf_holder;
  char* copied_cdma_dtmf_str;
  char* copied_cdma_dtmf_on;
  char* copied_cdma_dtmf_off;

  RIL_SMS_WriteArgs*  copied_android_write_sms_to_sim_msg_params;
  char *copied_android_write_sms_to_sim_msg_smsc_address = NULL;
  char *copied_android_write_sms_to_sim_msg_pdu = NULL;

#if (QCRIL_RIL_VERSION < 15)
  RIL_InitialAttachApn* copied_android_initial_attach_apn_params;
#else
  RIL_InitialAttachApn_v15* copied_android_initial_attach_apn_params;
  char* copied_android_initial_attach_apn_roaming_protocol;
  char* copied_android_initial_attach_apn_mvno_type;
  char* copied_android_initial_attach_apn_mvno_match_data;
#endif
  char* copied_android_initial_attach_apn_apn;
  char* copied_android_initial_attach_apn_protocol;
  char* copied_android_initial_attach_apn_username;
  char* copied_android_initial_attach_apn_password;

  char *copied_android_manual_selection_mcc_mnc;
  char *copied_android_manual_selection_rat;
  char ** copied_android_manual_selection_params;

  RIL_CallForwardInfo* copied_android_query_call_fwd_status_params;
  char *copied_android_query_call_fwd_status_number = NULL;

  char** copied_change_barring_pwd_params;
  char *copied_ch_bar_pwd_faclity = NULL;
  char *copied_ch_bar_pwd_old_pwd = NULL;
  char *copied_ch_bar_pwd_new_pwd = NULL;

  QCRIL_LOG_FUNC_ENTRY();
  QCRIL_LOG_INFO("action to destroy cloned Android request parameters, a-r-id %s, appron %d", qcril_log_lookup_event_name(event_id), (int)used_approach );

  int android_request_id = qcril_event_get_android_request(event_id);
  switch( android_request_id )
  {
    case RIL_REQUEST_IMS_SEND_SMS:
      if ( NULL != sub_created_custom_storage && QMI_RIL_ANDROID_PARAM_CPY_APPRON_DYNAMIC_COPY == used_approach )
      {
        copied_android_send_ims_msg_params = (RIL_IMS_SMS_Message*)sub_created_custom_storage;
        if ( RADIO_TECH_3GPP2 == copied_android_send_ims_msg_params->tech )
        {
          copied_android_cdma_send_ims_param = copied_android_send_ims_msg_params->message.cdmaMessage;
          if ( NULL != copied_android_cdma_send_ims_param )
          {
            qcril_free( copied_android_cdma_send_ims_param );
            copied_android_cdma_send_ims_param = NULL;
          }
        }
        else
        {
          copied_android_gw_sms_ims_params = copied_android_send_ims_msg_params->message.gsmMessage;

          if ( NULL != copied_android_gw_sms_ims_params )
          {
            copied_android_gw_smsc_address = copied_android_gw_sms_ims_params[0];
            copied_android_gw_pdu          = copied_android_gw_sms_ims_params[1];

            if ( NULL != copied_android_gw_smsc_address )
            {
              qcril_free( copied_android_gw_smsc_address );
              copied_android_gw_smsc_address = NULL;
            }
            if ( NULL != copied_android_gw_pdu )
            {
              qcril_free( copied_android_gw_pdu );
              copied_android_gw_pdu = NULL;
            }

            qcril_free( copied_android_gw_sms_ims_params );
            copied_android_gw_sms_ims_params = NULL;
          }


        }

        qcril_free( copied_android_send_ims_msg_params );
        copied_android_send_ims_msg_params = NULL;
      }
      break;

    case RIL_REQUEST_SEND_SMS:              // fallthrough
    case RIL_REQUEST_SEND_SMS_EXPECT_MORE:
      if ( NULL != sub_created_custom_storage && QMI_RIL_ANDROID_PARAM_CPY_APPRON_DYNAMIC_COPY == used_approach )
      {
        copied_android_gw_sms_ims_params = (char**)sub_created_custom_storage;
        copied_android_gw_smsc_address   = copied_android_gw_sms_ims_params[0];
        copied_android_gw_pdu            = copied_android_gw_sms_ims_params[1];

        QCRIL_LOG_DEBUG("sms allo 0x%x, 0x%x, 0x%x", copied_android_gw_sms_ims_params, copied_android_gw_smsc_address, copied_android_gw_pdu);

        if( NULL != copied_android_gw_pdu )
        {
            qcril_free( copied_android_gw_pdu );
            copied_android_gw_pdu = NULL;
        }

        if( NULL != copied_android_gw_smsc_address )
        {
            qcril_free( copied_android_gw_smsc_address );
            copied_android_gw_smsc_address = NULL;
        }

        qcril_free( copied_android_gw_sms_ims_params );
        copied_android_gw_sms_ims_params = NULL;
      }
      break;

    case RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL:
      /* only legacy format with RAT needs */
      if ( qmi_ril_is_feature_supported(QMI_RIL_FEATURE_LEGACY_RAT) )
      {
        if ( NULL != sub_created_custom_storage && QMI_RIL_ANDROID_PARAM_CPY_APPRON_DYNAMIC_COPY == used_approach )
        {
          copied_android_manual_selection_params    = (char**)sub_created_custom_storage;
          copied_android_manual_selection_mcc_mnc   = copied_android_manual_selection_params[0];
          copied_android_manual_selection_rat       = copied_android_manual_selection_params[1];

          QCRIL_LOG_DEBUG("manual sel allo 0x%x, 0x%x, 0x%x", copied_android_manual_selection_params, copied_android_manual_selection_mcc_mnc, copied_android_manual_selection_rat);

          if( NULL != copied_android_manual_selection_mcc_mnc )
          {
              qcril_free( copied_android_manual_selection_mcc_mnc );
              copied_android_manual_selection_mcc_mnc = NULL;
          }

          if( NULL != copied_android_manual_selection_rat )
          {
              qcril_free( copied_android_manual_selection_rat );
              copied_android_manual_selection_rat = NULL;
          }

          qcril_free( copied_android_manual_selection_params );
          copied_android_manual_selection_params = NULL;
        }
      }
      else
      {
        if ( NULL != sub_created_custom_storage && QMI_RIL_ANDROID_PARAM_CPY_APPRON_DYNAMIC_COPY == used_approach )
        {
          qcril_free( sub_created_custom_storage );
          sub_created_custom_storage = NULL;
        }
      }
      break;

    case RIL_REQUEST_CDMA_BURST_DTMF:
      if ( NULL != sub_created_custom_storage && QMI_RIL_ANDROID_PARAM_CPY_APPRON_DYNAMIC_COPY == used_approach )
      {
        copied_cdma_dtmf_holder = (char**)sub_created_custom_storage;

        copied_cdma_dtmf_str = copied_cdma_dtmf_holder[0];
        copied_cdma_dtmf_on  = copied_cdma_dtmf_holder[1];
        copied_cdma_dtmf_off = copied_cdma_dtmf_holder[2];

        if( NULL != copied_cdma_dtmf_off )
        {
            qcril_free( copied_cdma_dtmf_off );
            copied_cdma_dtmf_off = NULL;
        }

        if( NULL != copied_cdma_dtmf_on )
        {
            qcril_free( copied_cdma_dtmf_on );
            copied_cdma_dtmf_on = NULL;
        }

        if( NULL != copied_cdma_dtmf_str )
        {
            qcril_free( copied_cdma_dtmf_str );
            copied_cdma_dtmf_str = NULL;
        }

        qcril_free( copied_cdma_dtmf_holder );
        copied_cdma_dtmf_holder = NULL;
      }
      break;

    case RIL_REQUEST_SET_INITIAL_ATTACH_APN:
      if ( NULL != sub_created_custom_storage && QMI_RIL_ANDROID_PARAM_CPY_APPRON_DYNAMIC_COPY == used_approach )
      {
#if (QCRIL_RIL_VERSION < 15)
        copied_android_initial_attach_apn_params = (RIL_InitialAttachApn*)sub_created_custom_storage;
#else
        copied_android_initial_attach_apn_params = (RIL_InitialAttachApn_v15*)sub_created_custom_storage;
#endif

        copied_android_initial_attach_apn_apn = copied_android_initial_attach_apn_params->apn;
        copied_android_initial_attach_apn_protocol = copied_android_initial_attach_apn_params->protocol;
        copied_android_initial_attach_apn_username = copied_android_initial_attach_apn_params->username;
        copied_android_initial_attach_apn_password = copied_android_initial_attach_apn_params->password;
#if (QCRIL_RIL_VERSION >= 15)
        copied_android_initial_attach_apn_roaming_protocol = copied_android_initial_attach_apn_params->roamingProtocol;
        copied_android_initial_attach_apn_mvno_type = copied_android_initial_attach_apn_params->mvnoType;
        copied_android_initial_attach_apn_mvno_match_data = copied_android_initial_attach_apn_params->mvnoMatchData;
#endif

        if ( NULL != copied_android_initial_attach_apn_apn )
        {
          qcril_free( copied_android_initial_attach_apn_apn );
          copied_android_initial_attach_apn_apn = NULL;
        }

        if ( NULL != copied_android_initial_attach_apn_protocol )
        {
          qcril_free( copied_android_initial_attach_apn_protocol );
          copied_android_initial_attach_apn_protocol = NULL;
        }

        if ( NULL != copied_android_initial_attach_apn_username )
        {
          qcril_free( copied_android_initial_attach_apn_username );
          copied_android_initial_attach_apn_username = NULL;
        }


        if ( NULL != copied_android_initial_attach_apn_password )
        {
          qcril_free( copied_android_initial_attach_apn_password );
          copied_android_initial_attach_apn_password = NULL;
        }

#if (QCRIL_RIL_VERSION >= 15)
        if ( NULL != copied_android_initial_attach_apn_roaming_protocol )
        {
          qcril_free( copied_android_initial_attach_apn_roaming_protocol );
          copied_android_initial_attach_apn_roaming_protocol = NULL;
        }

        if ( NULL != copied_android_initial_attach_apn_mvno_type )
        {
          qcril_free( copied_android_initial_attach_apn_mvno_type );
          copied_android_initial_attach_apn_mvno_type = NULL;
        }

        if ( NULL != copied_android_initial_attach_apn_mvno_match_data )
        {
          qcril_free( copied_android_initial_attach_apn_mvno_match_data );
          copied_android_initial_attach_apn_mvno_match_data = NULL;
        }
#endif

        qcril_free( copied_android_initial_attach_apn_params );
        copied_android_initial_attach_apn_params = NULL;
      }
      break;

    case RIL_REQUEST_WRITE_SMS_TO_SIM:
      if ( NULL != sub_created_custom_storage && QMI_RIL_ANDROID_PARAM_CPY_APPRON_DYNAMIC_COPY == used_approach )
      {
        copied_android_write_sms_to_sim_msg_params = (RIL_SMS_WriteArgs*)sub_created_custom_storage;

        copied_android_write_sms_to_sim_msg_smsc_address = copied_android_write_sms_to_sim_msg_params->smsc;
        copied_android_write_sms_to_sim_msg_pdu = copied_android_write_sms_to_sim_msg_params->pdu;

        if ( NULL != copied_android_write_sms_to_sim_msg_smsc_address )
        {
          qcril_free( copied_android_write_sms_to_sim_msg_smsc_address );
          copied_android_write_sms_to_sim_msg_smsc_address = NULL;
        }

        if ( NULL != copied_android_write_sms_to_sim_msg_pdu )
        {
          qcril_free( copied_android_write_sms_to_sim_msg_pdu );
          copied_android_write_sms_to_sim_msg_pdu = NULL;
        }

        qcril_free( copied_android_write_sms_to_sim_msg_params );
        copied_android_write_sms_to_sim_msg_params = NULL;
      }
      break;

    case RIL_REQUEST_SET_CALL_FORWARD:            // fallthrough
    case RIL_REQUEST_QUERY_CALL_FORWARD_STATUS:
      if ( NULL != sub_created_custom_storage && QMI_RIL_ANDROID_PARAM_CPY_APPRON_DYNAMIC_COPY == used_approach )
      {
        copied_android_query_call_fwd_status_params = (RIL_CallForwardInfo*)sub_created_custom_storage;

        if ( NULL != copied_android_query_call_fwd_status_params )
        {
          copied_android_query_call_fwd_status_number = copied_android_query_call_fwd_status_params->number;

          if ( NULL != copied_android_query_call_fwd_status_number )
          {
            qcril_free( copied_android_query_call_fwd_status_number );
            copied_android_query_call_fwd_status_number = NULL;
          }
          qcril_free( copied_android_query_call_fwd_status_params );
          copied_android_query_call_fwd_status_params = NULL;
        }
      }
      break;

    case RIL_REQUEST_CHANGE_BARRING_PASSWORD:
      if ( NULL != sub_created_custom_storage && QMI_RIL_ANDROID_PARAM_CPY_APPRON_DYNAMIC_COPY == used_approach )
      {
        copied_change_barring_pwd_params = (char**)sub_created_custom_storage;

        if ( NULL != copied_change_barring_pwd_params )
        {
          copied_ch_bar_pwd_faclity = copied_change_barring_pwd_params[0];
          copied_ch_bar_pwd_old_pwd = copied_change_barring_pwd_params[1];
          copied_ch_bar_pwd_new_pwd = copied_change_barring_pwd_params[2];

          if ( NULL != copied_ch_bar_pwd_faclity )
          {
            qcril_free( copied_ch_bar_pwd_faclity );
            copied_ch_bar_pwd_faclity = NULL;
          }

          if ( NULL != copied_ch_bar_pwd_old_pwd )
          {
            qcril_free( copied_ch_bar_pwd_old_pwd );
            copied_ch_bar_pwd_old_pwd = NULL;
          }

          if ( NULL != copied_ch_bar_pwd_new_pwd )
          {
            qcril_free( copied_ch_bar_pwd_new_pwd );
            copied_ch_bar_pwd_new_pwd = NULL;
          }

          qcril_free( copied_change_barring_pwd_params );
          copied_change_barring_pwd_params = NULL;
        }
      }
      break;

    default:
      switch ( used_approach )
      {
        case QMI_RIL_ANDROID_PARAM_CPY_APPRON_4_BYTES_SNAPSHOT:
          if ( NULL != four_byte_storage )
          {
            memset( four_byte_storage, 0, 4 );
          }
          break;

        case QMI_RIL_ANDROID_PARAM_CPY_APPRON_DYNAMIC_COPY:
          if ( NULL != sub_created_custom_storage )
          {
            qcril_free( sub_created_custom_storage );
            sub_created_custom_storage = NULL;
          }
          break;

        case QMI_RIL_ANDROID_PARAM_CPY_APPRON_EMPTY_NO_ACTION: // no action
          break;

        default: // no action
          break;
      }
      break;
  }

  QCRIL_LOG_FUNC_RETURN();

} // qmi_ril_fw_destroy_android_live_params_copy

//===========================================================================
// qcril_qmi_mgr_voice_technology_updated
//===========================================================================
void qcril_qmi_mgr_voice_technology_updated(qcril_radio_tech_e_type new_voice_technology)
{
  qcril_arb_state_info_struct_type *s_ptr;

  QCRIL_LOG_FUNC_ENTRY();

  qmi_ril_enter_critical_section();
  s_ptr = &qcril_state.info[ QCRIL_DEFAULT_INSTANCE_ID ];
  s_ptr->voice_radio_tech = new_voice_technology;

  s_ptr = &qcril_state.info[ QCRIL_SECOND_INSTANCE_ID ];
  s_ptr->voice_radio_tech = new_voice_technology;

  // TODO_TSTS: This code seems not required. This info is used no where.
  s_ptr = &qcril_state.info[ QCRIL_THIRD_INSTANCE_ID ];
  s_ptr->voice_radio_tech = new_voice_technology;
  qmi_ril_leave_critical_section();

  QCRIL_LOG_FUNC_RETURN_WITH_RET(new_voice_technology);
} // qcril_qmi_mgr_modem_state_updated

//============================================================================
// FUNCTION: qmi_ril_retrieve_number_of_rilds
//
// DESCRIPTION:
// Returns the number of rilds supported on a target that supports mutiple rild scenario
//
// RETURN: number of rilds supported on a target that supports mutiple rild scenario
//============================================================================
int qmi_ril_retrieve_number_of_rilds()
{
    int num_of_rilds = 1;
    if ( qmi_ril_is_feature_supported(QMI_RIL_FEATURE_DSDS) )
    {
        num_of_rilds = 2;
    }
    else if( qmi_ril_is_feature_supported(QMI_RIL_FEATURE_TSTS) )
    {
        num_of_rilds = 3;
    }

    return num_of_rilds;
} //qmi_ril_retrieve_number_of_rilds

/*===========================================================================

  FUNCTION  qcril_request_check_if_suppressed

===========================================================================*/
/*!
    @brief
    Check if a request id is in suppressed list.

    @return
    TRUE is request is suppressed.
*/
/*=========================================================================*/
boolean qcril_request_check_if_suppressed
(
    uint32_t event_id,
    qcril_req_handler_type **event_handler
)
{
    boolean ret = FALSE;
    int     i;

    QCRIL_MUTEX_LOCK(&qcril_request_supress_list_mutex, "supress list mutex");

    do
    {
        for (i = 0; ((i < QCRIL_REQUEST_SUPPRESS_MAX_LEN) &&
                    (qcril_request_suppress_list[i].event_id != 0)); i++)
        {
            if (event_id == qcril_request_suppress_list[i].event_id)
            {
                if (event_handler)
                {
                    *event_handler = qcril_request_suppress_list[i].event_handler;
                }

                ret = TRUE;
                break;
            }
        }
    } while (FALSE);

    QCRIL_MUTEX_UNLOCK(&qcril_request_supress_list_mutex, "supress list mutex");
    QCRIL_LOG_FUNC_RETURN_WITH_RET(ret);
    return ret;
}

/*===========================================================================

  FUNCTION  qcril_request_suppress_request

===========================================================================*/
/*!
    @brief
    Add request id to suppressed list.

    @return
    E_SUCCESS if success.
    E_FAILURE if failure.
*/
/*=========================================================================*/
int qcril_request_suppress_request
(
    uint32_t                     event_id,
    qcril_req_handler_type *event_handler
)
{
    boolean ret = E_FAILURE;
    int i;

    QCRIL_MUTEX_LOCK(&qcril_request_supress_list_mutex, "supress list mutex");

    do
    {
        for (i = 0; i < QCRIL_REQUEST_SUPPRESS_MAX_LEN; i++)
        {
            if (0 == qcril_request_suppress_list[i].event_id)
            {
                QCRIL_LOG_DEBUG("Supress %d", event_id);
                qcril_request_suppress_list[i].event_id      = event_id;
                qcril_request_suppress_list[i].event_handler = event_handler;
                ret = E_SUCCESS;
                break;
            }
        }

    } while (FALSE);

    QCRIL_MUTEX_UNLOCK(&qcril_request_supress_list_mutex, "supress list mutex");
    QCRIL_LOG_FUNC_RETURN_WITH_RET(ret);
    return ret;
}

/*===========================================================================

  FUNCTION  qcril_request_unsuppress_request

===========================================================================*/
/*!
    @brief
    Remove request id from suppressed list.

    @return
    E_SUCCESS if success.
    E_FAILURE if failure.
*/
/*=========================================================================*/
int qcril_request_unsuppress_request
(
    uint32_t                     event_id,
    qcril_req_handler_type *event_handler
)
{
    boolean ret = E_FAILURE;
    int i;

    QCRIL_MUTEX_LOCK(&qcril_request_supress_list_mutex, "supress list mutex");

    do
    {
        for (i = 0; i < QCRIL_REQUEST_SUPPRESS_MAX_LEN; i++)
        {
            if ((event_id == qcril_request_suppress_list[i].event_id) &&
                 (event_handler == qcril_request_suppress_list[i].event_handler))
            {
                QCRIL_LOG_DEBUG("Unsupress %d", event_id);
                qcril_request_suppress_list[i].event_id      = 0;
                qcril_request_suppress_list[i].event_handler = NULL;
                ret = E_SUCCESS;
                break;
            }
        }

    } while (FALSE);

    QCRIL_MUTEX_UNLOCK(&qcril_request_supress_list_mutex, "supress list mutex");
    QCRIL_LOG_FUNC_RETURN_WITH_RET(ret);
    return ret;
}

/*===========================================================================

  FUNCTION qcril_request_clean_up_suppress_list

===========================================================================*/
/*!
    @brief
    Clean up suppress list.

    @return
    None
*/
/*=========================================================================*/
void qcril_request_clean_up_suppress_list
(
    void
)
{
    int i;
    QCRIL_LOG_FUNC_ENTRY();

    QCRIL_MUTEX_LOCK(&qcril_request_supress_list_mutex, "supress list mutex");
    for (i = 0; i < QCRIL_REQUEST_SUPPRESS_MAX_LEN; i++)
    {
        qcril_request_suppress_list[i].event_id      = 0;
        qcril_request_suppress_list[i].event_handler = NULL;
    }

    QCRIL_MUTEX_UNLOCK(&qcril_request_supress_list_mutex, "supress list mutex");
    QCRIL_LOG_FUNC_RETURN();
}

void qcril_unsol_oem_socket_connected()
{
    QCRIL_LOG_INFO( "Report unsol oem socket connected");
    qcril_hook_unsol_response( QCRIL_DEFAULT_INSTANCE_ID,
                               QCRIL_EVT_HOOK_UNSOL_OEM_SOCKET_CONNECTED,
                               NULL,
                               0);
}

#ifdef QMI_RIL_UTF
//============================================================================
// FUNCTION: qmi_ril_thread_shutdown
//
// DESCRIPTION:
// clears all global variables and releases all shared resources for reboot
//
// RETURN:
//============================================================================
int qmi_ril_threads_shutdown()
{
  //if (core_shutdown_for_reboot() != 0)
  //{
  //  QCRIL_LOG_ERROR("Could not successfully shutdown thread in core_handler.c");
  //}

  return 0;
}

//============================================================================
// FUNCTION: qmi_ril_reboot_cleanup
//
// DESCRIPTION:
// clears all global variables and releases all shared resources for reboot
//
// RETURN:
//============================================================================

int qmi_ril_reboot_cleanup()
{
  // Begin shutdown process
  qmi_ril_clear_timed_callback_list();
  // clean up core clients
  qcril_qmi_client_release();

  qcril_qmi_hal_ims_module_cleanup();
  qcril_qmi_hal_pdc_module_cleanup();
  qcril_qmi_hal_nas_module_cleanup();
  qcril_qmi_hal_pbm_module_cleanup();
  qcril_qmi_hal_sms_module_cleanup();
  qcril_qmi_hal_uim_module_cleanup();
  qcril_qmi_hal_gstk_module_cleanup();
  qcril_qmi_hal_lpa_module_cleanup();
  qcril_qmi_hal_voice_module_cleanup();
  qcril_qmi_hal_dms_module_cleanup();
  qcril_qmi_hal_android_ims_radio_module_cleanup();
  qcril_qmi_hal_qcril_am_cleanup();
#ifdef FEATURE_QCRIL_LTE_DIRECT
  qcril_qmi_hal_lte_direct_module_cleanup();
#endif

  qmi_ril_reset_multi_sim_ftr_info();

  if (qcril_db_reset_cleanup() != 0)
  {
    QCRIL_LOG_ERROR("Could not successfully reset resources in qcril_db.c");
  }
  qcril_qmi_nas_reboot_cleanup();
  qcril_qmi_hw_mbn_reboot_cleanup();
  qcril_qmi_sw_mbn_reboot_cleanup();

  return 0;
}

#endif

boolean qcril_is_heap_memory_tracked()
{
    return is_heap_memory_tracked;
}

