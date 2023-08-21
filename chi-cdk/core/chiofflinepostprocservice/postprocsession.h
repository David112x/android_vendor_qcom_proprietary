////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   postprocsession.h
/// @brief  HIDL interface header file for postproc session.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef POSTPROCSESSION_H
#define POSTPROCSESSION_H

#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#include <android/hardware/graphics/mapper/3.0/IMapper.h>
#include <deque>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include "postprocservice.h"
#include <system/camera_metadata.h>
#include <vendor/qti/hardware/camera/postproc/1.0/IPostProcSession.h>

// NOWHINE FILE CF017:
// NOWHINE FILE CP005:  Specifiers used in struct
// NOWHINE FILE CP006:  STL keyword used (deque, vector)
// NOWHINE FILE CP011:  namespace defined directly
// NOWHINE FILE GR004:  Ignore indentation, else ~30 spaces will be added unnecessarily
// NOWHINE FILE GR017:  Standard data types are used (no dependency on CHI)
// NOWHINE FILE GR028:  CF017 whiner error coming and not coming
// NOWHINE FILE NC003a: Variable name error
// NOWHINE FILE NC009:  These are HIDL service files, chi naming convention not followed
// NOWHINE FILE PR007b: Non-library files (HIDL includes are considered as library )
// NOWHINE FILE PR008:  '/' used for includes

namespace vendor {
namespace qti {
namespace hardware {
namespace camera {
namespace postproc {
namespace V1_0 {
namespace implementation {

using IMapper_2_0 = ::android::hardware::graphics::mapper::V2_0::IMapper;
using ::android::hardware::graphics::mapper::V3_0::IMapper;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;
using ::vendor::qti::hardware::camera::postproc::V1_0::BufferParams;
using ::vendor::qti::hardware::camera::postproc::V1_0::CreateParams;
using ::vendor::qti::hardware::camera::postproc::V1_0::Error;
using ::vendor::qti::hardware::camera::postproc::V1_0::HandleParams;
using ::vendor::qti::hardware::camera::postproc::V1_0::IPostProcSession;
using ::vendor::qti::hardware::camera::postproc::V1_0::IPostProcServiceCallBacks;
using ::vendor::qti::hardware::camera::postproc::V1_0::PostProcResult;
using ::vendor::qti::hardware::camera::postproc::V1_0::ProcessRequestParams;

///< Number of parallel streams supported in each session
const uint32_t NumberOfStreamsSupported = 1;

enum PostProcLoopStatus
{
    PROCLOOP_NOTSTARTED  = 0,   ///< Loop not started
    PROCLOOP_INITIALIZED = 1,   ///< Loop initialized
    PROCLOOP_STARTED     = 2,   ///< Thread started
    PROCLOOP_STOP        = 3,   ///< Thread stop request
    PROCLOOP_STOPPED     = 4,   ///< Thread stopped
};

enum PostProcError
{
    PostProcSuccess             = 0,    ///< Success
    PostProcFail                = 1,    ///< Generic failure
    PostProcMallocFail          = 2,    ///< Memory allocation failure
    PostProcIncorrectMetadata   = 3,    ///< Input metadata not correct
    PostProcNullFunctionPtr     = 4,    ///< Null function pointer
    PostProcThreadInitFailed    = 5,    ///< Thread creation failed
    PostProcIncorrectHandle     = 6,    ///< Handle is not proper
    PostProcUnsupportedType     = 7,    ///< Unsupported postproc type
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  HIDL interface Class for postproc session
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct PostProcSession : public IPostProcSession
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostProcSession
    ///
    /// @brief  constructor
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    PostProcSession();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~PostProcSession
    ///
    /// @brief  destructor
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~PostProcSession();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  API to Initialize Post Proc instance
    ///
    /// @param  createParams    Parameters provided by client
    /// @param  callback        Callback object Ptr
    /// @param  pServiceDiedObj Death Recipient object Ptr
    ///
    /// @return PostProcError if successful, Errors specified by PostProcError otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    PostProcError Initialize(
        const CreateParams&                     createParams,
        const sp<IPostProcServiceCallBacks>&    callback,
        PostProcServiceDeathRecipient*          pServiceDiedObj);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ClearCallback
    ///
    /// @brief  API to reset callback pointer. This is needed to handle client process died scenario.
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void ClearCallback();

    // Methods from ::vendor::qti::hardware::camera::postproc::V1_0::IPostProcSession follow.

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// process
    ///
    /// @brief  API to postproc
    ///
    /// @param  rProcParam   post process parameters structure
    /// @param  hidlCb       callback to provide result
    ///
    /// @return Void() Hidl function call
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Return<void> process(
        const ProcessRequestParams& rProcParam,
        process_cb                  hidlCb) override;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// abort
    ///
    /// @brief  API to abort all the requests in queue
    ///
    ///
    /// @return Error enum as defined in types.hal
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Return<Error> abort() override;

private:

    PostProcLoopStatus  m_postProcLoop;     ///< Enum to control post proc loop
    pthread_t           m_procLoopThread;   ///< Thread creation object
    pthread_cond_t      m_bufferSignal;     ///< Signal to control when buffer is empty or not
    pthread_cond_t      m_abortSignal;      ///< Signal to control when buffer is empty or not
    pthread_mutex_t     m_bufferMutex;      ///< Mutex to protect queue access
    uint32_t            m_validEntries;     ///< Number of valid entries given by client
    uint32_t            m_frameNum;         ///< Frame number
    uint8_t             m_abortPostProc;    ///< Flag to indicate abort API is waiting
    uint8_t             m_releaseResources; ///< Flag to indicate whether resources released or not

    uint8_t                 m_initFlag[NumberOfStreamsSupported];     ///< Flag to indicate postproc inited or not
    void*                   m_pPostProc[NumberOfStreamsSupported];    ///< PostProc pointer array
    PostProcCreateParams    m_createParams[NumberOfStreamsSupported]; ///< PostProc Create Params

    sp<IMapper_2_0>                         m_spMapper2_0;      ///< Mapper service pointer
    sp<IMapper>                             m_spMapper;         ///< Mapper service pointer
    sp<IPostProcServiceCallBacks>           m_spClientcallback; ///< Client callback pointer
    sp<PostProcServiceDeathRecipient>       m_spDeathRecipient; ///< Death Recipient object created
    std::deque<PostProcSessionParams*>      m_sessionQueue;     ///< Queue to maintain postproc requests

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeSession
    ///
    /// @brief  API to initialize particular index postproc session
    ///
    /// @param  index index to refer m_createParams
    ///
    /// @return PostProcError if successful, Errors specified by PostProcError otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    PostProcError InitializeSession(
        uint32_t index);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FreePostProcParams
    ///
    /// @brief  API to free postproc parameters pointer
    ///
    /// @param  pPostProcParam pointer to structure
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void FreePostProcParams(
        PostProcSessionParams* pPostProcParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateHandlePtr
    ///
    /// @brief  API to conver HIDL handle params to native handle params
    ///
    /// @param  rHidlParams     Vector of HIDL entries
    /// @param  rNativeHandle   Vector of native handle entries
    ///
    /// @return PostProcError enum
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    PostProcError UpdateHandlePtr(
        const hidl_vec<HandleParams>&       rHidlParams,
        std::vector<PostProcHandleParams>&  rNativeHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostProcLoop
    ///
    /// @brief  API to do postproc in separate thread loop. This runs till m_ePostProcLoop is updated to STOP.
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void PostProcLoop();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostProcLoopThreadFunction
    ///
    /// @brief  API to invoke postproc Loop thread. This will be initiated during Initialize.
    ///
    /// @param  pThis pointer to the class
    ///
    /// @return NULL pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void* PostProcLoopThreadFunction(
        void* pThis)
    {
        if (NULL != pThis)
        {
            /* Postproc enum will be set to STARTED inside thread */
            (static_cast<PostProcSession *>(pThis))->PostProcLoop();
        }

        return NULL;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckAbortStatus
    ///
    /// @brief  API to signal abort after postproc is completed.
    ///
    ///
    /// @return NULL pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline void CheckAbortStatus()
    {
        pthread_mutex_lock(&m_bufferMutex);

        /* Check if Abort function is waiting for signal or not.  */
        if (true == m_abortPostProc)
        {
            pthread_cond_signal(&m_abortSignal);
        }

        pthread_mutex_unlock(&m_bufferMutex);
    }
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace postpro
}  // namespace camera
}  // namespace hardware
}  // namespace qti
}  // namespace vendor

#endif  // POSTPROCSESSION_H
