/**=============================================================================

@file
   cvpSession.h

@brief
   Computer Vision Processor API Definition for preparing session related.

Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================**/


/**=============================================================================
@mainpage CVP Public API Documentation

@version 2.0.0

@section Overview Overview

CVP provides hardware acceleration computer vision APIs.

CVP 2.0.0 supports Android mobile developers.

Releases are generally motivated for the following reasons:
   - Changes to previously released APIs
   - Addition of new functions
   - Performance improvements and/or bug fixes - also known as implementation
     modifications

    Each motivation has a varying degree of impact on the user of the library.
    The general release numbering scheme captures this variety of motivations.

    Given release ID: Major.Minor.Revision

    An increase in "Major" indicates that a previously released API has changed,
    so a developer may encounter compilation issues which require modification
    of their code in order to adhere to the modified API.  Qualcomm will make
    every effort to minimize these changes.  Additionally, new functions and
    implementation modifications may be present.

    An increase in "Minor" indicates that new functions have been added to the
    library, so additional functionality is available, however existing APIs
    have not changed.  Additionally, implementation modifications may be
    present.

    An increase in "Revision" indicates that implementation modifications only have
    been made.

@defgroup cvp_types CVP types
@details CVP common types

@defgroup cvp_image_transform Image transformation
@details Image transformation functions such as scale down, pyramid, etc.

@defgroup cvp_feature_detection Feature detection
@details Feature detection functions such as harris corner detection, etc.

@defgroup cvp_object_detection Object detection
@details Object detection functions such as NCC template match, Motion Estimation, HOG, etc.

@defgroup cvp_3D_reconstruction 3D reconstruction
@details 3D reconstruction functions such as DFS, etc

@defgroup cvp_machine_learning Machine Learning
@details  Machine learning functions such as SVM prediction, etc.

@defgroup cvp_mem_management Memory Management
@details Functions to allocate and deallocate memory for use with CVP.

@defgroup cvp_utils Utilities
@details Utility functions

**/


#ifndef CVP_SESSION_H
#define CVP_SESSION_H

#include "cvpTypes.h"

#ifdef __cplusplus
extern "C"
{
#endif

//------------------------------------------------------------------------------
/// @brief
///   Retrieves version of CVP library.
/// @param pVersion
///   [Output] Pointer to CVP version structure.
///
/// @retval CVP_SUCCESS
///    If successful.
/// @retval CVP_EBADPARAM
///    If pVersion is NULL
///
/// @ingroup cvp_types
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpGetVersion(cvpVersion *pVersion);

//------------------------------------------------------------------------------
/// @brief
///    CVP Hardware has encountered an irrecoverable error
///    CVP Session must be terminated. This callback is used to notify the
///    application.
/// @param hSession
///    CVP session handle.
/// @param eEvent
///    Error Event Generated.
/// @param pSessionUserData
///    User-data that was set in the cvpCreateSession call.
///
/// @ingroup cvp_types
//------------------------------------------------------------------------------
typedef void (*cvpEventHandlerCb) (cvpSession hSession,
                                   cvpEvent   eEvent,
                                   void       *pSessionUserData);

//------------------------------------------------------------------------------
/// @brief
///    CVP session type options.
/// @param CVP_SESSION_CV
///    Session type for general CV.
/// @param CVP_SESSION_ODT
///    Session type for Object Detection and Tracking.
///
/// @ingroup cvp_types
//------------------------------------------------------------------------------
typedef enum _cvpSessionType
{
   CVP_SESSION_CV = 0,
   CVP_SESSION_ODT
} cvpSessionType;

//------------------------------------------------------------------------------
/// @brief
///    CVP session prioriy options.
/// @param CVP_PRIO_NRT_LOW
///    Session priority non real time low.
/// @param CVP_PRIO_NRT_HIGH
///    Session priority non real time high.
/// @param CVP_PRIO_RT_LOW
///    Session priority real time low.
/// @param CVP_PRIO_RT_MED
///    Session priority real time medium.
/// @param CVP_PRIO_RT_HIGH
///    Session priority real time high.
///
/// @ingroup cvp_types
//------------------------------------------------------------------------------
typedef enum _cvpSessionPriority
{
   CVP_PRIO_NRT_LOW = 0,
   CVP_PRIO_NRT_HIGH,
   CVP_PRIO_RT_LOW,
   CVP_PRIO_RT_MED,
   CVP_PRIO_RT_HIGH
} cvpSessionPriority;

//------------------------------------------------------------------------------
/// @brief
///    Structure for configuration of CVP session
/// @param eType
///    Session type.
/// @param nPriority
///    Session priority. Range 0-4. 0 is the lowest and 4 is the highest.
///    Default: 0. 0-3 is non-real time. 4 is for real time.
/// @param bSecure
///    Flag to indicate whether session is secured. True means secure. False
///    means non-secure. Default is non-secure.
/// @param nDspAccessMask
///     Secured bitmask for indicating if output of operation is consumed
///     by FW or DSP. This value will be ignored if bSecure is false.
///     0 means FW access, 0xFFFFFFFF means DSP access.
///     Currently all kernel outputs will be consumed by either FW or DSP.
///
/// @ingroup cvp_types
//------------------------------------------------------------------------------
typedef struct _cvpConfigSession
{
   cvpSessionType     eType;
   cvpSessionPriority nPriority;
   bool               bSecure;
   uint32_t           nDspAccessMask;

   #ifdef __cplusplus
   INLINE _cvpConfigSession() : eType(CVP_SESSION_CV),
                                nPriority(CVP_PRIO_NRT_LOW),
                                bSecure(false),
                                nDspAccessMask(0) {}
   #endif
} cvpConfigSession;

//------------------------------------------------------------------------------
/// @brief
///    Create CVP session
/// @param fnCb
///    [Input] Callback Function for Asynchronous Error Event.
/// @param pSessionUserData
///    [Input] A private pointer that can be set by user, with this session, and this
///    pointer will be provided as parameter to all Asynchronous Error Event callback
///    functions originating from the current session. This could be used to associate a
///    callback to this session. This can only be set once while creating
///    the session. This value will not/cannot-be changed for life
///    of a session.
/// @param pConfig
///    [Input] Pointer to the session configuration. If NULL, session will be created
///    based on default configuration.
///
/// @retval CVP session handle.
///    If successful.
/// @retval NULL
///    If creation failed.
///
/// @ingroup cvp_types
//------------------------------------------------------------------------------
CVP_API cvpSession cvpCreateSession(cvpEventHandlerCb fnCb,
                                    void             *pSessionUserData,
                                    cvpConfigSession *pConfig=NULL);

//------------------------------------------------------------------------------
/// @brief
///    Delete CVP session
/// @param hSession
///    [Input] CVP session handle.
///
/// @retval CVP_SUCCESS
///    If successful.
///
/// @ingroup cvp_types
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpDeleteSession(cvpSession hSession);

//------------------------------------------------------------------------------
/// @brief
///    Start CVP session
/// @details
///    CVP will set firmware clock based on all initialization CVP API.
///    After starting the session, client can start calling the synchronous or
///    asynchronous CVP API.
/// @param hSession
///    [Input] CVP session handle.
///
/// @retval CVP_SUCCESS
///    If successful.
///
/// @ingroup cvp_types
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpStartSession(cvpSession hSession);

//------------------------------------------------------------------------------
/// @brief
///    Stop CVP session
/// @details
///    After stopping the session, client can start reinitialize (deinit and init)
///    the CVP API with new configuration.
/// @param hSession
///    [Input] CVP session handle.
///
/// @retval CVP_SUCCESS
///    If successful.
///
/// @ingroup cvp_types
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpStopSession(cvpSession hSession);

#ifdef __cplusplus
}//extern "C"
#endif

#endif
