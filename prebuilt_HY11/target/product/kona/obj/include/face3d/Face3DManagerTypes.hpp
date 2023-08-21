/******************************************************************************
*
* Copyright (c) 2018 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*
******************************************************************************/

#ifndef FACE3D_MANAGER_TYPES_HPP
#define FACE3D_MANAGER_TYPES_HPP

using namespace SCVE;

namespace Face3D {

enum Face3D_Status {
   FACE3D_SUCCESS = 0,
   FACE3D_FAIL,
   FACE3D_ERROR_FACE_OFFCENTER,
   FACE3D_ERROR_DEPTH_LARGE,
   FACE3D_ERROR_DEPTH_SMALL,
   FACE3D_ERROR_GALLERY_FULL,
   FACE3D_ERROR_ENGINE_UNINIT,
   FACE3D_ERROR_NONE_FRONTAL
};

enum Face3DImageType {
   FACE3D_NIR_IMAGE = 0,
   FACE3D_DEPTH_IMAGE,
};

enum Face3DmemCpyType {
   SECURE_TO_NONSECURE = 0,
   NONSECURE_TO_SECURE
};
struct Face3DEnrollPose {
   int32_t pitch;
   int32_t yaw;
   int32_t roll;
   int32_t current_pose_idx;
   Face3D_Status error_code;
};

//-------------------------------------------------------------------------
/// @brief
///    Abstract authentication token.
/// @details
///    To encapsulate the face authentication result.
///    For the time being with a non-secure implementation, simply
///    represent it with a boolean.
///    For the final secure implementation, it is going to be re-defined
///    with more secure data structures.
//-------------------------------------------------------------------------
//typedef bool Face3D_Token;

struct Face3D_Token{
   //hw_auth_token_t authToken;
   Face3D_Status status;
};
//-------------------------------------------------------------------------
/// @brief
///    Abstract secure memory handle
/// @details
///    Abstraction of a secure buffer.
///    For the time being with a non-secure implementation, simply
///    represent it with a file descriptor or memory address.
///    For the final secure implementation, it is going to be re-defined
///    with more secaure data structures.
//-------------------------------------------------------------------------
struct Face3D_Mem_Handle {
   int fd;
   uint32_t size;
   uint64_t addr;
   bool cached;
   int ion_fd;
   int handle_data;

   //Face3D_Mem_Handle() : fd(0), size(0), addr(0), cached(true), ion_fd(0), handle_data(0) {};
   Face3D_Mem_Handle() : fd(0), size(0), addr(0), cached(true), ion_fd(0), handle_data(0) {};
};

//-------------------------------------------------------------------------
/// @brief
///    Callback to obtain a NIR image.
/// @details
///    Caller of the Face3D APIs provides its implementation of this
///    callback to obtain a NIR image frame. The Face3D library will use
///    the callback during its operation to consume the NIR frames.
///
/// @param Face3D_Mem_Handle&
///    Secure buffer handle corresponding to the raw image data in the
///    Image.
/// @param void*
///    Caller can use it to pass any customized data to the callback.
///    The caller_data is specified optionally in the Face3D::newInstance().
///
/// @retval true
///    If the NIR frame is successfully obtained.
/// @retval false
///    If the NIR frame cannot be obtained.
//-------------------------------------------------------------------------
typedef bool (*ReadNirImage_cb)(Face3D_Mem_Handle&, void* caller_data);

//-------------------------------------------------------------------------
/// @brief
///    Callback to obtain a depth image.
/// @details
///    Caller of the Face3D APIs provides its implementation of this
///    callback to obtain a depth image frame. The Face3D library will
///    use the callback during its operation to consume the depth frames.
///
/// @param Face3D_Mem_Handle&
///    Secure buffer handle corresponding to the raw image data in the
///    Image.
/// @param void*
///    Caller can use it to pass any customized data to the callback.
///    The caller_data is specified optionally in the Face3D::newInstance().
///
/// @retval true
///    If the depth frame is successfully obtained.
/// @retval false
///    If the depth frame cannot be obtained.
//-------------------------------------------------------------------------
typedef bool (*ReadDepthImage_cb)(Face3D_Mem_Handle&, void* caller_data);

//-------------------------------------------------------------------------
/// @brief
///    Callback to release an NIR/depth image.
/// @details
///    Caller of the Face3D APIs provides its implementation of this
///    callback to release an NIR or dpeth image frame. The Face3D library
///    will use the callback during its operation to inform the caller
///    that the image frame is no longer needed. It is used to release the
///    secure buffers allocated by ReadNirImage_cb() and ReadDepthImage_cb().
///
/// @param Face3D_Mem_Handle&
///    Secure buffer handle corresponding to the raw image data in the
///    Image.
/// @param void*
///    Caller can use it to pass any customized data to the callback.
///    The caller_data is specified optionally in the Face3D::newInstance().
///
/// @retval true
///    If the image frame is successfully released.
/// @retval false
///    If the image frame cannot be released.
//-------------------------------------------------------------------------
typedef bool (*ReleaseImage_cb)(Face3D_Mem_Handle, void* caller_data);

//-------------------------------------------------------------------------
/// @brief
///    Callback to switch the image mode to NIR or depth.
/// @details
///    Caller of the Face3D APIs provides its implementation of this
///    callback to change the sensor image mode. The Face3D library will
///    use the callback during its operation to switch the image mode.
///
/// @param Face3DImageType
///    Desired image mode to switch to:
///    FACE3D_NIR_IMAGE   --> changed to NIR image mode.
///    FACE3D_DEPTH_IMAGE --> changed to depth image mode.
/// @param void*
///    Caller can use it to pass any customized data to the callback.
///    The caller_data is specified optionally in the Face3D::newInstance().
///
/// @retval true
///    If the sensor is configured to the desired mode successfully.
/// @retval false
///    If the sensor cannot be configured to the desired mode.
//-------------------------------------------------------------------------
typedef bool (*SwitchImageMode_cb)(Face3DImageType, void* caller_data);

//-------------------------------------------------------------------------
/// @brief
///    Callback to decrypt a file and load it into a secure buffer.
/// @details
///    Caller of the Face3D APIs provides its implementation of this
///    callback to decrypt an encrypted file and load it into a secuare
///    buffer. The Face3D library will use the callback during its
///    operation to load the network model files, enrolled user data files,
///    etc.
///
/// @param const char*
///    Name of the encrypted file to be loaded.
/// @param uint32_t&
///    Returns the bytes decrypted.
/// @param void*
///    Caller can use it to pass any customized data to the callback.
///    The caller_data is specified optionally in the Face3D::newInstance().
/// @param Face3D_Mem_Handle*
///    A valid non-secure buffer that would contain the encrypted model buffer.
///    Otherwise return an invalid buffer.
/// @param Face3D_Mem_Handle*
///    A valid secure buffer if the file is successfully decrypted.
///    Otherwise return an invalid buffer.
///
/// @retval true
///    If the model decryption was done successfully.
/// @retval false
///    If the model loading or decryption failed.
//-------------------------------------------------------------------------
typedef bool (*DecryptFile_cb)(const char*, uint32_t&, void* caller_data, Face3D_Mem_Handle *, Face3D_Mem_Handle *);

//-------------------------------------------------------------------------
/// @brief
///    Callback to copy contents between secure and non-secure memory.
/// @details
///    Caller of the Face3D APIs provides its implementation of this
///    callback to either copy contents from a secure to non-secure buffer
///    or viceversa.
///
/// @param Face3D_Mem_Handle*
///    A valid non-secure buffer.
/// @param Face3D_Mem_Handle*
///    A valid secure buffer.
/// @param Face3DmemCpyType
///    Parameter to specify if it is secure to non-secure copy or the other way.
/// @param void*
///    Caller can use it to pass any customized data to the callback.
///    The caller_data is specified optionally in the Face3D::newInstance().
///
/// @retval true
///    If the copy was done successfully.
/// @retval false
///    If the copy failed.
//-------------------------------------------------------------------------
typedef bool (*SecureNonSecureMemCpy_cb)(Face3D_Mem_Handle *, Face3D_Mem_Handle *, Face3DmemCpyType, void* caller_data);

//-------------------------------------------------------------------------
/// @brief
///    Callback to release a secure buffer handle
/// @details
///    Caller of the Face3D APIs provides its implementation of this
///    callback to release a secure buffer handle. The Face3D library
///    will use the callback during its operation to inform the caller
///    that the secure buffer is no longer needed. It is used to release
///    the secure buffer allocated by DecryptFile_cb().
///
/// @param Face3D_Mem_Handle
///    The secure buffer handle to be released
/// @param void*
///    Caller can use it to pass any customized data to the callback.
///    The caller_data is specified optionally in the Face3D::newInstance().
///
/// @retval true
///    If the secure buffer is successfully released.
/// @retval false
///    If the secure buffer cannot be released.
//-------------------------------------------------------------------------
typedef bool (*ReleaseMemHandle_cb)(Face3D_Mem_Handle, void* caller_data);

//-------------------------------------------------------------------------
/// @brief
///    Feed back the face pose to the caller during enrollment.
/// @details
///    Caller of the Face3D APIs provides its implementation of this
///    callback to save a copy of the face pose information during
///    enrollment. The caller can use pose information to guide the end
///    user through UI for enrollment alignment. The Face3D library
///    will use the callback during its operation to provide the face pose
///    to the caller. It is a blocking call for the Face3D library. The
///    caller should avoid any time consuming computation in the callback
///    implementation.
///
/// @param Face3DEnrollPose
///    The face pose information passed by the Face3D library during
///    enrollment
/// @param void*
///    Caller can use it to pass any customized data to the callback.
///    The caller_data is specified optionally in the Face3D::newInstance().
///
/// @retval true
///    If the caller receives and copies the pose information successfully.
/// @retval false
///    If the caller fails to receive or copy the pose information.
//-------------------------------------------------------------------------
typedef bool (*EnrollPose_cb)(Face3DEnrollPose, void* caller_data);

}


#endif
