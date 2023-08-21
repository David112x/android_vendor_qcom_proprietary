#ifndef __DEPTHMAP_WRAPPER_H__
#define __DEPTHMAP_WRAPPER_H__

/**=============================================================================

Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
=============================================================================**/


//==============================================================================
// Included modules
//==============================================================================
#include <stdint.h>

//==============================================================================
// MACROS
//==============================================================================
#ifdef _MSC_VER
#define CP_DLL_PUBLIC __declspec(dllexport)
#else
#define CP_DLL_PUBLIC __attribute__ ((visibility ("default")))
#endif

#define IMG_UNUSED(x) (void)(x)

//==============================================================================
// DECLARATIONS
//==============================================================================

typedef struct pc_pack {
    float X;
    float Y;
    float Z;
    float c;
} pc_pkt_t;

typedef enum {
    WRAPPER_CAM_FORMAT_NONE,
    WRAPPER_CAM_FORMAT_IR,
    WRAPPER_CAM_FORMAT_DEPTH16,
    WRAPPER_CAM_FORMAT_DEPTH_POINT_CLOUD,
} WrapperDepthFormat;

typedef enum {
    WRAPPER_EXP_MODE_ME,
    WRAPPER_EXP_MODE_AE,
    WRAPPER_EXP_MODE_NONE
} WrapperDepthExpModeType;

typedef struct {
    float min_fps;
    float max_fps;
    float video_min_fps;
    float video_max_fps;
} WrapperDepthCamFpsRange;

typedef struct {
    int64_t in_depth_map_me_val;
    int64_t in_out_depth_map_ae_val;
    int64_t in_depth_map_frame_num;
    int64_t in_out_depth_map_timestamp;
    int64_t out_depth_map_laser_strength_val;
    WrapperDepthExpModeType in_depth_map_exp_mode;
    WrapperDepthCamFpsRange in_depth_map_fps_range;
} WrapperDepthCamConfig;

typedef struct {
    WrapperDepthFormat format;
    union {
        uint8_t*  out_depth_image;
        pc_pkt_t* out_pcloud_image;
        int32_t out_image_fd;
    };
    union {
        uint32_t  out_image_length;
        uint32_t* count_pt_cloud;
    };
} WrapperDepthOutput;

typedef struct {
    union {
        const int8_t* in_image;
        int32_t in_image_fd;
    };
} WrapperDepthInput;

//==============================================================================
// API CLASS
//==============================================================================
class CP_DLL_PUBLIC DepthMapWrapper
{
public:
    DepthMapWrapper(
        int32_t width,
        int32_t height,
        int32_t dm_width,
        int32_t dm_height,
        uint64_t* exposure_time,
        int32_t* sensitivity,
        uint8_t is_secure);

    bool processFrame(
        WrapperDepthInput in_image,
        WrapperDepthCamConfig *wrapper_depth_map_config,
        uint32_t num_outputs,
        WrapperDepthOutput outputs[]);

    ~DepthMapWrapper() ;

    // For future use
    static unsigned int getWrapperVersion();

private:
    struct DepthWrapperData;
    DepthWrapperData *pDepthData;

    DepthMapWrapper(); // No default constructor
    DepthMapWrapper( const DepthMapWrapper& ); // No copy constructor
    DepthMapWrapper& operator=( const DepthMapWrapper& ); // No copy assignment operator
};


// Class factories
#ifdef __cplusplus
extern "C" {
#endif
CP_DLL_PUBLIC
DepthMapWrapper* DepthMapWrapperCreate(
    int32_t width,
    int32_t height,
    int32_t dm_width,
    int32_t dm_height,
    uint64_t* exposure_time,
    int32_t* sensitivity,
    uint8_t is_secure);

CP_DLL_PUBLIC
bool DepthMapWrapperProcessFrame(
    DepthMapWrapper* pDepthMapWrapper,
    WrapperDepthInput in_image,
    WrapperDepthCamConfig *wrapper_depth_map_config,
    uint32_t num_outputs,
    WrapperDepthOutput outputs[]);

CP_DLL_PUBLIC
void DepthMapWrapperDestroy(DepthMapWrapper* pDepthMapWrapper);
#ifdef __cplusplus
}
#endif

#endif //__DEPTHMAP_WRAPPER_H__

