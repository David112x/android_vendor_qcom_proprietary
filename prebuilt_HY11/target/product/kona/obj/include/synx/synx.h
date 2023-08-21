////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  synx.h
/// @brief The SynX API
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SYNX_H
#define SYNX_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

    //////////////////////////////////////////////////////////////////////////
    // Entry Point
    //////////////////////////////////////////////////////////////////////////
    #if defined (_WIN32)
    #define SYNX_DLL_EXPORT            __declspec(dllexport)
    #define SYNX_DLL_IMPORT            __declspec(dllimport)
    #elif defined(__GNUC__)
    #define SYNX_DLL_EXPORT            __attribute((visibility("default")))
    #define SYNX_DLL_IMPORT
    #endif

    #if defined(SYNX_LIB_EXPORT)
    #define SYNX_API                   SYNX_DLL_EXPORT
    #define SYNX_INLINE                inline
    #else
    #define SYNX_INLINE
    #define SYNX_API                   SYNX_DLL_IMPORT
    #endif


typedef int32_t synx_handle_t;
typedef int32_t synx_result_t;

#define SYNX_DEVICE_NAME "synx_device"

static const synx_handle_t  SYNX_INVALID_HANDLE = 0;
#define SYNX_MAX_ATTRIBS 16

enum
{
    SYNX_ATTRIB_INVALID = 0,
    SYNX_ATTRIB_TYPE,
    SYNX_ATTRIB_STATUS
};

enum
{
    SYNX_TYPE_SYNX_SYNC = 0,
    SYNX_TYPE_NATIVE_FENCE,
    SYNX_TYPE_EGL_SYNC,
    SYNX_TYPE_CSL_FENCE,
    SYNX_TYPE_CHI_FENCE
};

typedef enum synx_status_t
{
    SYNX_STATUS_INVALID = 0,
    SYNX_STATUS_UNSIGNALED,
    SYNX_STATUS_SIGNALED_SUCCESS,
    SYNX_STATUS_SIGNALED_ERROR
} synx_status_t;


enum
{
    SYNX_SUCCESS        = 0,
    SYNX_ERROR_FAILED,
    SYNX_CANCELED,
    SYNX_TIMEOUT
};

typedef void (*PFPSYNXCALLBACK)(
    synx_handle_t   hSynx,
    void*           pUserData,
    synx_result_t   result);

typedef struct synx_external_desc_t
{
    uint32_t type;
    uint32_t reserved;
    int32_t  id[2];
} synx_external_desc_t;

typedef struct synx_attrib_t
{
    uint32_t attrib_id;
    uint32_t attrib_val[2];
} synx_attrib_t;

typedef struct synx_create_params_t
{
    size_t          size;
    uint32_t        numAttribs;
    synx_attrib_t   attribs[SYNX_MAX_ATTRIBS];
} synx_create_params_t;

typedef struct synx_initialization_params_t
{
    size_t size;
} synx_initialization_params_t;

typedef struct synx_export_params_t
{
    size_t          size;
    synx_handle_t   hSynx;
    uint32_t        flags;
} synx_export_params_t;

typedef struct synx_import_params_t
{
    size_t      size;
    synx_handle_t id;
    uint32_t    key;
    uint32_t    flags;
} synx_import_params_t;

SYNX_API synx_result_t synx_initialize(
    const synx_initialization_params_t*);

SYNX_API synx_result_t synx_uninitialize();

SYNX_API synx_result_t synx_create(
    const synx_create_params_t* pParams,
    synx_handle_t*              pSynxHandle);

SYNX_API synx_status_t synx_get_status(
    synx_handle_t hSynx);

SYNX_API synx_result_t synx_add_refcount(
    synx_handle_t hSynx,
    const uint32_t n);

SYNX_API synx_result_t synx_release(
    synx_handle_t hSynx);

SYNX_API synx_result_t synx_signal(
    synx_handle_t hSynx,
    synx_result_t res);

SYNX_API synx_result_t synx_wait(
    synx_handle_t   hSynx,
    uint64_t        timeout_milliseconds);

SYNX_API synx_result_t synx_wait_async(
    synx_handle_t   hSynx,
    PFPSYNXCALLBACK pCallback,
    void*           pUserData);

SYNX_API synx_result_t synx_cancel_async_wait(
    synx_handle_t   hSynx,
    PFPSYNXCALLBACK pCallback,
    void*           pUserData,
    PFPSYNXCALLBACK pCancelCB);

SYNX_API synx_result_t synx_merge(
    uint32_t                numSynxs,
    const synx_handle_t*    pSynxHandles,
    synx_handle_t*          pSynxHandle);

SYNX_API synx_result_t synx_bind(
    synx_handle_t       hSynx1,
    const synx_external_desc_t*  pExternalSync);

SYNX_API synx_result_t synx_export(
    const synx_export_params_t* pParams,
    uint32_t*                   pKey);

SYNX_API synx_result_t synx_import(
    const synx_import_params_t* pParams,
    synx_handle_t*              phSynx);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // SYNX_H
