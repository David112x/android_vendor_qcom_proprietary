////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   chiofflinepostproccallbacks.h
/// @brief  HIDL Callback object interface header file. This is used by test module
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef VENDOR_QTI_HARDWARE_CAMERA_POSTPROC_V1_0_IPOSTPROCSERVICECALLBACKS_H
#define VENDOR_QTI_HARDWARE_CAMERA_POSTPROC_V1_0_IPOSTPROCSERVICECALLBACKS_H

#include "hidl/MQDescriptor.h"
#include "hidl/Status.h"
#include "vendor/qti/hardware/camera/postproc/1.0/IPostProcServiceCallBacks.h"


namespace vendor {
namespace qti {
namespace hardware {
namespace camera {
namespace postproc {
namespace V1_0 {
namespace implementation {

using ::android::hidl::base::V1_0::DebugInfo;
using ::android::hidl::base::V1_0::IBase;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;
using ::vendor::qti::hardware::camera::postproc::V1_0::Error;
using ::vendor::qti::hardware::camera::postproc::V1_0::PostProcResult;

class CameraPostProcCallBacks : public IPostProcServiceCallBacks
{

  public:
    CameraPostProcCallBacks();
    virtual ~CameraPostProcCallBacks();
    // Methods from ::vendor::qti::::harware::camera::jpegenc::V1_0::IPostProcServiceCallBacks follow.
    Return<void> notifyResult(
        Error                 err,
        const PostProcResult& result) override;


    // Methods from ::android::hidl::base::V1_0::IBase follow.

};

extern "C" IPostProcServiceCallBacks *HIDL_FETCH_IPostProcServiceCallBacks(
  const char *name);

}  // namespace implementation
}  // namespace V1_0
}  // jpegenc
}  // camera
}  // namespace hardware
}  // namespace qti
}  // namespace vendor
#endif  // VENDOR_QTI_HARDWARE_CAMERA_POSTPROC_V1_0_IPOSTPROCSERVICECALLBACKS_H
