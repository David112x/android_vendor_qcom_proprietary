#ifndef FACE3D_MANAGER_HPP
#define FACE3D_MANAGER_HPP

/******************************************************************************
*
* Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*
******************************************************************************/

#include "scveTypes.hpp"
#include "Face3DManagerTypes.hpp"
#include "face3d_common.h"

using namespace SCVE;

namespace Face3D {

class Face3D_Manager
{
public:

    static const uint32_t MAX_PID_LEN = 20;
    // User ID lenth including the NUL terminating character.

    //-------------------------------------------------------------------------
    /// @brief
    ///    Estimate the scratch buffer size needed to create a Face3D_Manager.
    /// @details
    ///    This static function estimate the secure scratch buffer size as
    ///    needed by an instance of Face3D_Manager. The caller should call
    ///    this function in the first place to estimate the size of a secure
    ///    scratch buffer to be allocated. Then the caller should allocate the
    ///    secure scratch buffer and encapsulate it with a Face3D_Mem_Handle.
    ///    The Face3D_Mem_Handle will be passed as a parameter in the
    ///    newInstance() API call to create the Face3D_Manager. The caller is
    ///    responsible for deallocating the secure scratch buffer after the
    ///    Face3D_Manager is destructed.
    ///
    /// @retval uint32_t
    ///    The required size of a secure scratch buffer needed by
    ///    Face3D_Manager.
    //-------------------------------------------------------------------------
    static uint32_t estimate_scratch_size();

    //-------------------------------------------------------------------------
    /// @brief
    ///    Creates a new instance of Face3D_Manager.
    /// @details
    ///    This is a static factory-function that creates a new instance of
    ///    Face3D_Manager. You will not be able to instanciate the class by
    ///    calling the constructor directly. You will be able to create a new
    ///    instance only through this function.
    ///
    /// @param nirImgCb
    ///    Callback to obtain a NIR image frame.
    /// @param depthImgCb
    ///    Callback to obtain a depth image frame.
    /// @param releaseImgCb
    ///    Callback to release an NIR/depth image frame.
    /// @param switchModeCb
    ///    Callback to configure the image sensor to NIR or depth mode.
    /// @param decryptFileCb
    ///    Callback to load an encypted file into a secure buffer and decrypt it.
    /// @param secNonSecMemCpyCb
    ///    Callback to copy contents between secure and non-secure memory.
    /// @param releaseMemCb
    ///    Callback to release a secure buffer.
    /// @param secureScratch
    ///    A secure scratch buffer. See estimate_scratch_size() above for more
    ///    information.
    /// @param secureHeap
    ///    A secure scratch buffer for DSP heap.
    /// @param producerType
    ///    Frame producer type. See face3d_producer_type for more
    ///    information.
    /// @param caller_data
    ///    The caller can specify customized data to pass to all the callbacks.
    ///
    /// @retval Non-NULL
    ///    If the initialization is successful. The returned value is a
    ///    pointer to the newly created Face3D_Manager instance.
    /// @retval NULL
    ///    If the initialization has failed. Check logs for any error messages
    ///    if this occurs.
    //-------------------------------------------------------------------------
    static Face3D_Manager *newInstance(ReadNirImage_cb nirImgCb,
                                         ReadDepthImage_cb depthImgCb,
                                         ReleaseImage_cb releaseImgCb,
                                         SwitchImageMode_cb switchModeCb,
                                         DecryptFile_cb decryptFileCb,
                                         SecureNonSecureMemCpy_cb secNonSecMemCpyCb,
                                         ReleaseMemHandle_cb releaseMemCb,
                                         Face3D_Mem_Handle secureScratch,
                     Face3D_Mem_Handle secureHeap,
                                         face3d_producer_type producerType,
                                         void* caller_data = NULL);

    //-------------------------------------------------------------------------
    /// @brief
    ///    Initialize Face3D_Manager.
    /// @details
    ///    This is the initialization method that prepares the internal data
    ///    structure based on the given parameter.
    ///
    /// @param option
    ///    Pick an internally hard-coded configuration for supported sensor
    ///    combinations. For the sake of security, valid options will be
    ///    communicated with OEM customers later.
    ///    [COMMENTS BELOW TO BE DELETED BEFORE PUBLIC RELEASE]
    ///    For internal testing:
    ///       0 --> Orbbec single pose (tested)
    ///       1 --> Orbbec multi pose (TBD)
    ///       2 --> HiMax single pose (TBD)
    ///       3 --> HiMax multi pose (tested)
    ///
    /// @retval FACE3D_SUCCESS
    ///    If the initialization is successful.
    /// @retval FACE3D_FAIL
    ///    If any error occured.
    //-------------------------------------------------------------------------
    //virtual Face3D_Status init(int option) = 0;
    virtual Face3D_Status init(int option = 0, Image *nir_image = NULL, Image *depth_map = NULL);


    //-------------------------------------------------------------------------
    /// @brief
    ///    Enroll a user
    /// @details
    ///    This function adds a user registration. If the user is registered
    ///    successfully, the enroll operation will be completed by returning
    ///    FACE3D_SUCCESS. The enrollment will be aborted if any of the
    ///    callbacks designated in the newInstance() experiences failure. As
    ///    a result, the caller can terminate the enrollment by returning
    ///    failure from the nirImgCb or depthImgCb or releaseImgCb. Returning
    ///    failure from releaseImgCb is the most straight-forward way to abort,
    ///    since this callback is exercised in both NIR and depth mode.
    ///
    /// @param pid
    ///    The personal ID string of the user to be added. It will be
    ///    trunctated if it is too long.
    /// @param poseCb
    ///    [Optional] A callback so that the Face3D library has a way to
    ///    communicate with the caller about the face pose informatioin during
    ///    enrollment. Please see definition of EnrollPose_cb above.
    ///
    /// @retval FACE3D_SUCCESS
    ///    If the user is succefully added.
    /// @retval FACE3D_FAIL
    ///    If the user cannot be registered.
    //-------------------------------------------------------------------------
    virtual Face3D_Status enroll(const char* pid,
                                 EnrollPose_cb poseCb = NULL) = 0;

    //-------------------------------------------------------------------------
    /// @brief
    ///    Try to recognize and authenticate a user.
    /// @details
    ///    This function checks if there is a live face matching an enrolled
    ///    user. If the image stream matches a registered face successfully,
    ///    the authentication operation will be completed by returning
    ///    FACE3D_SUCCESS. The authentication will be aborted if any of the
    ///    callbacks designated in the newInstance() experiences failure. As
    ///    a result, the caller can terminate the authentication by returning
    ///    failure from the nirImgCb or depthImgCb or releaseImgCb. Returning
    ///    failure from releaseImgCb is the most straight-forward way to abort,
    ///    since this callback is exercised in both NIR and depth mode.
    ///
    /// @param operation_id
    ///    [Input] a 64-bit unsigned integer that is set by the client and expected
    ///    to be returned in the hardware auth token generated after successful
    ///    authentication
    /// @param authen_pid
    ///    [Output] A buffer to receive the user ID string if the
    ///    authentication is successful. If the authentication fails, it
    ///    will not be updated. The pid string is for information display
    ///    purposes such as UI development. It is not meant to be used for
    ///    secure authentication. Secure authentication should only rely on
    ///    the encrypted Face3D_Token.
    ///    The caller pass in a pointer to a buffer for the library
    ///    to fill out the user ID string (NUL terminated).
    /// @param len
    ///    Size of the buffer passed in by the caller.
    ///    If the buffer provided by the caller is not large enough to store
    ///    the version string, it will be truncated to return the first
    ///    (len - 1) characters. The last character will be NUL.
    ///    See MAX_PID_LEN above for suggested value.
    ///
    /// @retval Face3D_Token
    ///    Returns a valid authentication token if it is a live face matching
    ///    a previously enrolled user. Otherwise returns an invalid token.
    ///    The authentication token can be used in downstream applications
    ///    for secure applications. The Face3D_Token must be able to
    ///    simultaneously distinguish: a) a stranger from registered users
    ///    AND b) a registered user from other registered users.
    //-------------------------------------------------------------------------
    virtual Face3D_Token authenticate(uint64_t operation_id, char* authen_pid, uint32_t len) = 0;

    //-------------------------------------------------------------------------
    /// @brief
    ///    Remove a user from the list of enrolled users.
    /// @details
    ///    Only the last authenticated user can remove itself from the list of
    ///    enrolled users. This API must be immediately preceded by an
    ///    authenticate() API call first.
    ///
    /// @param pid
    ///    The personal ID string of the user to be deleted.
    /// @param token
    ///    The authentication result from the last authenticate() API call.
    ///    To delete a user, this API must be called immediately after the same
    ///    user has been successfully authenticated.
    ///
    /// @retval FACE3D_SUCCESS
    ///    If the user is succefully deleted.
    /// @retval FACE3D_FAIL
    ///    If the user to be deleted does not exist, or there is no
    ///    authorization to delete the user.
    //-------------------------------------------------------------------------
    virtual Face3D_Status delete_user(const char *pid, Face3D_Token token) = 0;

    //-------------------------------------------------------------------------
    /// @brief
    ///    Get the personal ID strings of all enrolled users.
    /// @details
    ///    This function returns the ID strings of all enrolled users.
    ///    If the buffer is not large enough to store all the user IDs, it will
    ///    fill in as many as possible user IDs until no more room left in the
    ///    buffer.
    ///
    /// @param pids
    ///    The caller pass in a pointer to a buffer for the library to fill out
    ///    the user pid strings (NUL terminated).
    ///    pids                   --- pid of the first user
    ///    pids + MAX_PID_LEN     --- pid of the second user
    ///    pids + MAX_PID_LEN * 2 --- pid of the third user
    ///     ...
    /// @param len
    ///    Size of the buffer passed in by the caller.
    ///
    /// @retval uint32_t
    ///    Number of user pids that can be fit into the buffer.
    ///    retval * MAX_PID_LEN <= len.
    //-------------------------------------------------------------------------
    virtual uint32_t get_pids(char* pids, uint32_t len) = 0;

    //-------------------------------------------------------------------------
    /// @brief
    ///    Get the version string of the Face3D library.
    /// @details
    ///    This function returns the version string of the Face3D library.
    ///    If the buffer provided by the caller is not large enough to store
    ///    the version string, it will be truncated to return the first
    ///    (len - 1) characters.
    ///
    /// @param version_str
    ///    The caller pass in a pointer to a buffer for the library to fill out
    ///    the version string (NUL terminated).
    /// @param len
    ///    Size of the buffer passed in by the caller.
    ///
    /// @retval uint32_t
    ///    Number of bytes written to the buffer excluding the terminating NUL
    ///    character. This is guaranteed to be < len.
    //-------------------------------------------------------------------------
    virtual uint32_t get_version(char* version_str, uint32_t len) = 0;

    //-------------------------------------------------------------------------
    /// @brief
    ///    Save the currently enrolled users to the encrypted file.
    /// @details
    ///    The enroll() and delete_user() above modify registered users in
    ///    memory. This API will save the update information to the encrypted
    ///    file of users.
    ///
    /// @retval FACE3D_SUCCESS
    ///    If the operation is succeful.
    /// @retval FACE3D_FAIL
    ///    If the operation fails.
    //-------------------------------------------------------------------------
    virtual Face3D_Status save_users() = 0;

    //-------------------------------------------------------------------------
    /// @brief
    ///    Remove the database that containing enrolled users.
    ///
    /// @param db_full_path
    ///    database full path. This should be match with the database path
    ///    currently used to avoid user removing random files or attacking
    ///    the system.
    ///
    /// @retval FACE3D_SUCCESS
    ///    If the operation is succeful.
    /// @retval FACE3D_FAIL
    ///    If the operation fails.
    //-------------------------------------------------------------------------
    virtual Face3D_Status remove_database(const char *db_full_path) = 0;

    //-------------------------------------------------------------------------
    /// @brief
    ///    Generate a 64-bit challenge/nonce for enrollment
    ///
    /// @retval uint64_t
    ///    The 64-bit challenge (zero if it was not successful)
    //-------------------------------------------------------------------------
    virtual uint64_t generate_challenge() = 0;

    //-------------------------------------------------------------------------
    /// @brief
    ///    Verifies hardware auth token sent with enroll API
    ///
    /// @param token
    ///    hardware auth token to be verified
    ///
    /// @retval FACE3D_SUCCESS
    ///    If the operation is succeful.
    /// @retval FACE3D_FAIL
    ///    If the operation fails.
    //-------------------------------------------------------------------------
    virtual Face3D_Status verify_token(const hw_auth_token_t *token) = 0;

    //-------------------------------------------------------------------------
    /// @brief
    ///    Return hardware auth token for the authenticated  user
    ///
    /// @param token
    ///    hardware auth token for the autheticated user
    ///
    /// @retval FACE3D_SUCCESS
    ///    If the operation is succeful.
    /// @retval FACE3D_FAIL
    ///    If the operation fails.
    //-------------------------------------------------------------------------
    virtual Face3D_Status get_auth_token(hw_auth_token_t *token) = 0;

    //-------------------------------------------------------------------------
    /// @brief
    ///    Set active user with trustzone
    ///
    /// @param gid
    ///    User ID
    ///
    /// @retval FACE3D_SUCCESS
    ///    If the operation is succeful.
    /// @retval FACE3D_FAIL
    ///    If the operation fails.
    //-------------------------------------------------------------------------
    virtual Face3D_Status set_active_user(int32_t gid) = 0;

    //-------------------------------------------------------------------------
    /// @brief
    ///    Get authenticator for enrolled user from trustzone
    ///
    /// @retval uint64_t
    ///    64-bit Authenticator ID
    //-------------------------------------------------------------------------
    virtual uint64_t get_authenticator_id() = 0;

    //-------------------------------------------------------------------------
    /// @brief
    ///    Initialize secure camera
    ///
    /// @retval FACE3D_SUCCESS
    ///    If the operation is succeful.
    /// @retval FACE3D_FAIL
    ///    If the operation fails.
    //-------------------------------------------------------------------------
    virtual Face3D_Status seccam_init() = 0;

    //-------------------------------------------------------------------------
    /// @brief
    ///    De-initialize secure camera
    ///
    /// @retval FACE3D_SUCCESS
    ///    If the operation is succeful.
    /// @retval FACE3D_FAIL
    ///    If the operation fails.
    //-------------------------------------------------------------------------
    virtual Face3D_Status seccam_deinit() = 0;

    virtual ~Face3D_Manager() = 0;

};

}


#endif
