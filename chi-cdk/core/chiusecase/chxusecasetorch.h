////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxusecasetorch.h
/// @brief CHX basic camcorder declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHXUSECASETORCH_H
#define CHXUSECASETORCH_H

#include <assert.h>

#include "chxincs.h"
#include "chxpipeline.h"
#include "chxsession.h"
#include "chxusecase.h"
#include "chxusecaseutils.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

/// Forward declarations
struct ChiPipelineTargetCreateDescriptor;
class  Session;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Torch widget usecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class UsecaseTorch : public Usecase
{
public:
    /// Static create function to create an instance of the object
    static UsecaseTorch* Create(
        LogicalCameraInfo*              pCameraInfo,    ///< Camera Info
        camera3_stream_configuration_t* pStreamConfig); ///< Stream configuration;
protected:
    // Parallel flush is excessive for the Torch Usecase
    virtual CHX_INLINE BOOL ShouldUseParallelFlush() { return FALSE; }
    /// Destroy/Cleanup the object
    virtual VOID Destroy(BOOL isForced);

private:
    UsecaseTorch() = default;
    virtual ~UsecaseTorch() = default;

    // Do not allow the copy constructor or assignment operator
    UsecaseTorch(const UsecaseTorch&) = delete;
    UsecaseTorch& operator= (const UsecaseTorch&) = delete;

    static VOID SessionCbCaptureResult(
        ChiCaptureResult* pCaptureResult,                       ///< Capture result
        VOID*             pPrivateCallbackData);                ///< Private callback data

    static VOID SessionCbNotifyMessage(
        const ChiMessageDescriptor* pMessageDescriptor,         ///< Message Descriptor
        VOID*                       pPrivateCallbackData);      ///< Private callback data

    static VOID SessionCbPartialCaptureResult(
        CHIPARTIALCAPTURERESULT* pCaptureResult,                ///< Capture result
        VOID*                    pPrivateCallbackData);         ///< Private callback data

    /// Execute capture request
    CDKResult ExecuteCaptureRequest(
        camera3_capture_request_t* pRequest);                   ///< Request parameters

    /// Generate a Chi request from HAL request
    CDKResult SubmitChiRequest(
        camera3_capture_request_t* pRequest);                   ///< Request parameters

    /// To flush the session
    CDKResult ExecuteFlush();

    // Implemented by the derived class to process the saved results
    virtual VOID ProcessResults() { }

    /// Does one time initialization of the created object
    CDKResult Initialize(
        LogicalCameraInfo*              pCameraInfo,    ///< Camera Info
        camera3_stream_configuration_t* pStreamConfig); ///< Stream configuration

    struct SessionPrivateData
    {
        Usecase* pUsecase;  ///< Per usecase class
        UINT32   sessionId; ///< Session Id that is meaningful to the usecase in which the session belongs
    };

    SessionPrivateData m_torchSessionPvtData;    ///< Per session private data
    Session*           m_pTorchSession;          ///< torch Session handle
    Pipeline*          m_pTorchPipeline;         ///< Pipeline handle
    camera3_stream_t*  m_pTorchStream;           ///< Stream for torch
    CHIBufferManager*  m_pTorchBufferManager;    ///< buffer manager for torch pipeline output
    Mutex*             m_pTorchResultMutex;      ///< Torch process capture Result availability mutex
    Condition*         m_pTorchResultAvailable;  ///< Wait for availablability of result.
};

#endif // CHXUSECASETORCH_H
