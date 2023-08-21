////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chiofflinepostproctypes.h
/// @brief Common data types used in offline post proc interface.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIOFFLINEPOSTPROCTYPES_H
#define CHIOFFLINEPOSTPROCTYPES_H

#include <cutils/native_handle.h>
#include <system/camera_metadata.h>
#include <vector>

// NOWHINE FILE CP006:  STL keyword used for vector
// NOWHINE FILE NC004c: Structures are used in hidl lib (not kept chi)

#define CHXFORMAT_VISIBILITY_PUBLIC __attribute__ ((visibility ("default")))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief This enumerates input mode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum PostProcMode
{
    YUVToJPEG   = 0,    ///< YUV to JPEG conversion
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief This enumerates encoder status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum PostProcRetStatus
{
    POSTPROCSUCCESS     = 0,    ///< Success
    POSTPROCFAILED      = 1,    ///< Postproc Failed
    POSTPROCBADSTATE    = 2,    ///< Postproc in bad state
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief This structure used to return Encoder result
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct PostProcResultInfo
{
    uint32_t            size;   ///< Postproc output size
    PostProcRetStatus   result; ///< Postproc status
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief This structure provides input and ouput process request buffer parameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct PostProcBufferParams
{
    uint32_t    format;     ///< format
    uint32_t    width;      ///< width
    uint32_t    height;     ///< height
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief This structure used to provide parameters needed for session initialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct PostProcCreateParams
{
    uint32_t                streamId;       ///< Stream Index
    PostProcMode            processMode;    ///< proc mode
    PostProcBufferParams    inBuffer;       ///< Input buffer params
    PostProcBufferParams    outBuffer;      ///< Output buffer params
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief This structure used to provide Handle params in session request
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct PostProcHandleParams
{
    uint32_t                format;     ///< format
    uint32_t                width;      ///< width
    uint32_t                height;     ///< height
    const native_handle_t*  phHandle;   ///< Native handle
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief This structure used to provide process request
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct PostProcSessionParams
{
    std::vector<PostProcHandleParams>   inHandle;   ///< Input buffer handle Vector
    std::vector<PostProcHandleParams>   outHandle;  ///< Output buffer handle Vector
    camera_metadata_t*                  pMetadata;  ///< Metadata
    uint32_t                            frameNum;   ///< Frame number requested
    uint32_t                            streamId;   ///< Stream or session Id
    uint8_t                             valid;      ///< Flag to indicate entry is valid or not

};

#endif // CHIOFFLINEPOSTPROCTYPES_H