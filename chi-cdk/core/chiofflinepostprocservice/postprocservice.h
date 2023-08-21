////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   postprocservice.h
/// @brief  HIDL interface header file for postproc service.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef POSTPROCSERVICE_H
#define POSTPROCSERVICE_H

#include <vendor/qti/hardware/camera/postproc/1.0/IPostProcService.h>
#include <vendor/qti/hardware/camera/postproc/1.0/IPostProcSession.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <set>


// NOWHINE FILE CF017:
// NOWHINE FILE CP004:  Struct contains public/private specifier
// NOWHINE FILE CP005:  Specifiers used in struct
// NOWHINE FILE CP006:  STL keyword used (set)
// NOWHINE FILE CP011:  namespace defined directly
// NOWHINE FILE GR004:  Ignore indentation, else ~30 lines will be added unnecessarily
// NOWHINE FILE GR017:  Standard data types are used (no dependency on CHI)
// NOWHINE FILE GR028:  CF017 whiner error coming and not coming
// NOWHINE FILE NC009:  These are HIDL service files, chi naming convention not followed
// NOWHINE FILE NC010:  '_' used in function signature (HIDL standard function)
// NOWHINE FILE PR007b: Non-library files (HIDL includes are considered as library )

namespace vendor {
namespace qti {
namespace hardware {
namespace camera {
namespace postproc {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_death_recipient;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hidl::base::V1_0::DebugInfo;
using ::android::hidl::base::V1_0::IBase;
using ::android::sp;
using ::android::wp;
using ::vendor::qti::hardware::camera::postproc::V1_0::CreateParams;
using ::vendor::qti::hardware::camera::postproc::V1_0::Error;
using ::vendor::qti::hardware::camera::postproc::V1_0::IPostProcSession;
using ::vendor::qti::hardware::camera::postproc::V1_0::IPostProcServiceCallBacks;
using ::vendor::qti::hardware::camera::postproc::V1_0::PostProcCapabilities;
using ::vendor::qti::hardware::camera::postproc::V1_0::PostProcType;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  HIDL interface Class for postproc service
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct PostProcService : public IPostProcService
{
public:
    // Methods from ::vendor::qti::hardware::camera::postproc::V1_0::IPostProcService follow.

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostProcService
    ///
    /// @brief  PostProcService constructor
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    PostProcService();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// getPostprocTypes
    ///
    /// @brief  API to get PostprocTypes supported by service
    ///
    /// @param  hidlCB Hidl callback to update PostprocTypes
    ///
    /// @return Void() Hidl function call
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Return<void> getPostprocTypes(
        getPostprocTypes_cb hidlCb) override;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// getCapabilities
    ///
    /// @brief  API to get capabilities for a given postproc enum type
    ///
    /// @param  postprocenum PostProc enum that client is interested
    /// @param  hidlCB Hidl  callback to update capabilities
    ///
    /// @return Void() Hidl function call
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Return<void> getCapabilities(
        PostProcType        postprocenum,
        getCapabilities_cb  hidlCB) override;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// createPostProcessor
    ///
    /// @brief  API to create postproc instance
    ///
    /// @param  rCreateParams Params needed to create postproc instance
    /// @param  rCallback     Callback object instance
    ///
    /// @return IPostProcSession instance pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Return<sp<IPostProcSession>> createPostProcessor(
        const CreateParams&                     rCreateParams,
        const sp<IPostProcServiceCallBacks>&    rCallback) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// serviceDied
    ///
    /// @brief  API to manage client sudden death situation
    ///
    /// @param  cookie  cookie parameter provided during linktoDeath
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void serviceDied(
        uint64_t cookie);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// eraseEntry
    ///
    /// @brief  API to delete corresponding propstproc entry from the list
    ///
    /// @param  pPostProc   Ptr to be deleted from sest
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void eraseEntry(
        void* pPostProc)
    {
        // NOWHINE CP006:
        std::set<sp<IPostProcSession>>::iterator it = m_pPostProcInstance.begin();
        for (; it != m_pPostProcInstance.end(); it++)
        {
            if (*it == reinterpret_cast<IPostProcSession*>(pPostProc))
            {
                m_pPostProcInstance.erase(it);
                break;
            }
        }
    };

private:
    std::set<sp<IPostProcSession>> m_pPostProcInstance;    ///< PostProc Instance pointers list
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  Death Recipient class for PostProc Service
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct PostProcServiceDeathRecipient : hidl_death_recipient
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostProcServiceDeathRecipient
    ///
    /// @brief  PostProcServiceDeathRecipient constructor
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    PostProcServiceDeathRecipient(
        PostProcService* pService)
    {
        m_spPostProcService = pService;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~PostProcServiceDeathRecipient
    ///
    /// @brief  PostProcServiceDeathRecipient Destructor
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~PostProcServiceDeathRecipient()
    {
        m_spPostProcService.clear();
        m_spPostProcService = NULL;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// serviceDied
    ///
    /// @brief  Function to handle scenario when client dies
    ///
    /// @param  cookie  Cookie value setup during linkToDeath
    /// @param  who     IBase Class Pointer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual void serviceDied(
        uint64_t cookie,
        const wp<IBase>& /* who */)
    {
        m_spPostProcService->serviceDied(cookie);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// eraseEntry
    ///
    /// @brief  Function to erase entry from set in service class
    ///
    /// @param  pPtr  Pointer to be removed from service set
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void eraseEntry(
        void* pPtr)
    {
        m_spPostProcService->eraseEntry(pPtr);
    };

private:
    sp<PostProcService> m_spPostProcService;    ///< Service pointer
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HIDL_FETCH_IPostProcService
///
/// @brief  API to initiate PostProc Service
///
/// @param  pName  Service name string
///
/// @return IPostProcService Pointer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" IPostProcService* HIDL_FETCH_IPostProcService(
    const char* pName);

}  // namespace implementation
}  // namespace V1_0
}  // namespace postproc
}  // namespace camera
}  // namespace hardware
}  // namespace qti
}  // namespace vendor

#endif  // POSTPROCSERVICE_H
