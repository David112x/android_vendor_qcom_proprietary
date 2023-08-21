////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxhal3entry.cpp
/// @brief Dispatch entry points for HAL3
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxchi.h"
#include "camxentry.h"
#include "camxhal3defs.h"
#include "camxhal3entry.h"
#include "camxincs.h"
#include "camxlist.h"
#include "camxmem.h"

#include "chi.h"

CAMX_NAMESPACE_BEGIN

// Global dispatch
static Dispatch g_dispatchHAL3(&g_jumpTableHAL3);

// NOWHINE FILE GR017: Google types
// NOWHINE FILE NC010: Google types
// NOWHINE FILE NC011: Google types

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Type Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Wrapper for redirecting camera_module_callbacks. NOTE: The ordering of the fields must remain unchanged as we cast a
///        pointer to the first field back to the entire structure.
struct CameraModuleCbsRedirect
{
    camera_module_callbacks_t           moduleCbs;      ///< The module callback functions defined in the entry implementation
                                                        ///  and passed down to the HAL3Module instance
    const camera_module_callbacks_t*    pModuleCbsAPI;  ///< The module callback functions defined by the application framework
};

/// @brief Wrapper for redirecting camera3_callback_ops. NOTE: The ordering of the fields must remain unchanged as we cast a
///        pointer to the first field back to the entire structure.
struct Camera3CbOpsRedirect
{
    camera3_callback_ops_t          cbOps;          ///< The camera3 callback functions defined in the entry implementation and
                                                    ///  passed down to the HAL3Device instance
    const camera3_device_t*         pCamera3Device; ///< The camera3 device instance from the application framework
    const camera3_callback_ops_t*   pCbOpsAPI;      ///< The camera3 callback functions defined by the application framework
};

/// @brief Describe close operation for HAL3 Device
struct HwDeviceCloseOps
{
    HwDeviceCloseFunc hwDeviceCloseFunc;            ///< Hardware close structure
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief HAL3 Entry class to wrap the callback ops list
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class HAL3Entry
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HAL3Entry
    ///
    /// @brief  Default constructor for HAL3Entry class
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HAL3Entry();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~HAL3Entry
    ///
    /// @brief  Destructor for HAL3Entry class.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~HAL3Entry();

    LightweightDoublyLinkedList m_cbOpsList;  ///< The list of per-device callback ops
    Mutex*                      m_pCbOpsLock; ///< Lock for protecting callback ops List

private:
    // Do not implement the copy constructor or assignment operator
    HAL3Entry(const HAL3Entry&) = delete;
    HAL3Entry& operator=(const HAL3Entry&) = delete;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Static Data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static CameraModuleCbsRedirect  g_cameraModuleCb;   ///< Global container for module callback redirection

static HAL3Entry                g_HAL3Entry;        ///< Global object to wrap list for per-device callback ops redirection

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3 Entry Class Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Entry::HAL3Entry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HAL3Entry::HAL3Entry()
{
    m_pCbOpsLock = Mutex::Create("Hal3CbOpsList");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Entry::~HAL3Entry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HAL3Entry::~HAL3Entry()
{
    LDLLNode* pNode = m_cbOpsList.RemoveFromHead();

    while (NULL != pNode)
    {
        if (NULL != pNode->pData)
        {
            CAMX_FREE(pNode->pData);
        }
        CAMX_FREE(pNode);

        pNode = m_cbOpsList.RemoveFromHead();
    }

    if (NULL != m_pCbOpsLock)
    {
        m_pCbOpsLock->Destroy();
        m_pCbOpsLock = NULL;
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// camera_module_callbacks_t Exit Points
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// camera_device_status_change
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void camera_device_status_change(
    const struct camera_module_callbacks*   pModuleCbsAPI,
    int                                     cameraIdAPI,
    int                                     newStatusAPI)
{
    const CameraModuleCbsRedirect* pModuleCbs = reinterpret_cast<const CameraModuleCbsRedirect*>(pModuleCbsAPI);

    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->camera_device_status_change);

    pHAL3->camera_device_status_change(pModuleCbs->pModuleCbsAPI, cameraIdAPI, newStatusAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// torch_mode_status_change
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void torch_mode_status_change(
    const struct camera_module_callbacks*   pModuleCbsAPI,
    const char*                             pCameraIdAPI,
    int                                     newStatusAPI)
{
    const CameraModuleCbsRedirect* pModuleCbs = reinterpret_cast<const CameraModuleCbsRedirect*>(pModuleCbsAPI);

    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->torch_mode_status_change);

    pHAL3->torch_mode_status_change(pModuleCbs->pModuleCbsAPI, pCameraIdAPI, newStatusAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// camera3_callback_ops_t Exit Points
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// process_capture_result
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void process_capture_result(
    const struct camera3_callback_ops*  pCamera3CbOpsAPI,
    const camera3_capture_result_t*     pCaptureResultAPI)
{
    const Camera3CbOpsRedirect* pCamera3CbOps = reinterpret_cast<const Camera3CbOpsRedirect*>(pCamera3CbOpsAPI);

    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->process_capture_result);

    pHAL3->process_capture_result(pCamera3CbOps->pCbOpsAPI, pCaptureResultAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// notify
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void notify(
    const struct camera3_callback_ops*  pCamera3CbOpsAPI,
    const camera3_notify_msg_t*         pNotifyMessageAPI)
{
    const Camera3CbOpsRedirect* pCamera3CbOps = reinterpret_cast<const Camera3CbOpsRedirect*>(pCamera3CbOpsAPI);

    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->notify);

    pHAL3->notify(pCamera3CbOps->pCbOpsAPI, pNotifyMessageAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// hw_module_methods_t Entry Points
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// open
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int open(
    const struct hw_module_t*   pHwModuleAPI,
    const char*                 pCameraIdAPI,
    struct hw_device_t**        ppHwDeviceAPI)
{
    /// @todo (CAMX-43) - Reload Jumptable from settings
    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->open);

    return pHAL3->open(pHwModuleAPI, pCameraIdAPI, ppHwDeviceAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// camera_module_t Entry Points
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_number_of_cameras
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int get_number_of_cameras(void)
{
    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->get_number_of_cameras);

    return pHAL3->get_number_of_cameras();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_camera_info
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int get_camera_info(
    int                 cameraIdAPI,
    struct camera_info* pCameraInfoAPI)
{
    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->get_camera_info);

    return pHAL3->get_camera_info(cameraIdAPI, pCameraInfoAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set_callbacks
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int set_callbacks(
    const camera_module_callbacks_t* pModuleCbsAPI)
{
    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->set_callbacks);

    // Intercept callbacks
    g_cameraModuleCb.moduleCbs.camera_device_status_change = camera_device_status_change;
    g_cameraModuleCb.moduleCbs.torch_mode_status_change = torch_mode_status_change;
    g_cameraModuleCb.pModuleCbsAPI = pModuleCbsAPI;

    return pHAL3->set_callbacks(&(g_cameraModuleCb.moduleCbs));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_vendor_tag_ops
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void get_vendor_tag_ops(
    vendor_tag_ops_t* pVendorTagOpsAPI)
{
    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->get_vendor_tag_ops);

    return pHAL3->get_vendor_tag_ops(pVendorTagOpsAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// open_legacy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int open_legacy(
    const struct hw_module_t*   pHwModuleAPI,
    const char*                 pCameraIdAPI,
    uint32_t                    halVersionAPI,
    struct hw_device_t**        ppHwDeviceAPI)
{
    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->open_legacy);

    return pHAL3->open_legacy(pHwModuleAPI, pCameraIdAPI, halVersionAPI, ppHwDeviceAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set_torch_mode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int set_torch_mode(
    const char* pCameraIdAPI,
    bool        enabledAPI)
{
    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->set_torch_mode);

    return pHAL3->set_torch_mode(pCameraIdAPI, enabledAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// init
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int init()
{
    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->init);

    return pHAL3->init();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// vendor_tag_ops_t Entry Points
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_tag_count
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int get_tag_count(
    const vendor_tag_ops_t* pVendorTagOpsAPI)
{
    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->get_tag_count);

    return pHAL3->get_tag_count(pVendorTagOpsAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_all_tags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void get_all_tags(
    const vendor_tag_ops_t* pVendorTagOpsAPI,
    uint32_t*               pTagArrayAPI)
{
    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->get_all_tags);

    return pHAL3->get_all_tags(pVendorTagOpsAPI, pTagArrayAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_section_name
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* get_section_name(
    const vendor_tag_ops_t* pVendorTagOpsAPI,
    uint32_t                tagAPI)
{
    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->get_section_name);

    return pHAL3->get_section_name(pVendorTagOpsAPI, tagAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_tag_name
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* get_tag_name(
    const vendor_tag_ops_t* pVendorTagOpsAPI,
    uint32_t                tagAPI)
{
    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->get_tag_name);

    return pHAL3->get_tag_name(pVendorTagOpsAPI, tagAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_tag_type
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int get_tag_type(
    const vendor_tag_ops_t* pVendorTagOpsAPI,
    uint32_t                tagAPI)
{
    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->get_tag_type);

    return pHAL3->get_tag_type(pVendorTagOpsAPI, tagAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// hw_device_t Entry Points
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// close
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int close(
    struct hw_device_t* pHwDeviceAPI)
{
    int            errnoResult = -EINVAL;
    JumpTableHAL3* pHAL3       = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->close);

    g_HAL3Entry.m_pCbOpsLock->Lock();
    LDLLNode* pNode = g_HAL3Entry.m_cbOpsList.Head();
    while (NULL != pNode)
    {
        if (reinterpret_cast<struct camera3_device*>(pHwDeviceAPI) ==
                static_cast<Camera3CbOpsRedirect*>(pNode->pData)->pCamera3Device)
        {
            errnoResult = 0;
            break;
        }
        pNode = LightweightDoublyLinkedList::NextNode(pNode);
    }
    g_HAL3Entry.m_pCbOpsLock->Unlock();


    if (0 == errnoResult)
    {
        errnoResult = pHAL3->close(pHwDeviceAPI);

        g_HAL3Entry.m_pCbOpsLock->Lock();
        g_HAL3Entry.m_cbOpsList.RemoveNode(pNode);
        CAMX_FREE(pNode->pData);
        pNode->pData = NULL;
        CAMX_FREE(pNode);
        pNode       = NULL;
        g_HAL3Entry.m_pCbOpsLock->Unlock();
    }

    return errnoResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// camera3_device_ops_t Entry Points
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int initialize(
    const struct camera3_device*    pCamera3DeviceAPI,
    const camera3_callback_ops_t*   pCamera3CbOpsAPI)
{
    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->initialize);

    g_HAL3Entry.m_pCbOpsLock->Lock();

    // See if there is already an entry for this device
    Camera3CbOpsRedirect* pCamera3CbOps = NULL;
    LDLLNode*             pNode         = g_HAL3Entry.m_cbOpsList.Head();

    while (NULL != pNode)
    {
        if (pCamera3DeviceAPI == static_cast<Camera3CbOpsRedirect*>(pNode->pData)->pCamera3Device)
        {
            pCamera3CbOps = static_cast<Camera3CbOpsRedirect*>(pNode->pData);
            break;
        }
        pNode = LightweightDoublyLinkedList::NextNode(pNode);
    }

    // Else create and add to list
    if (NULL == pCamera3CbOps)
    {
        pNode = reinterpret_cast<LDLLNode*>(CAMX_CALLOC(sizeof(LDLLNode)));

        if (NULL != pNode)
        {
            pCamera3CbOps = reinterpret_cast<Camera3CbOpsRedirect*>(CAMX_CALLOC(sizeof(Camera3CbOpsRedirect)));

            if (NULL != pCamera3CbOps)
            {
                pNode->pData = pCamera3CbOps;
                g_HAL3Entry.m_cbOpsList.InsertToTail(pNode);
            }
        }
    }

    // List management may have failed, skip override on failure
    if (NULL != pCamera3CbOps)
    {
        pCamera3CbOps->cbOps.process_capture_result = process_capture_result;
        pCamera3CbOps->cbOps.notify = notify;
        pCamera3CbOps->pCamera3Device = pCamera3DeviceAPI;
        pCamera3CbOps->pCbOpsAPI = pCamera3CbOpsAPI;
        pCamera3CbOpsAPI = &(pCamera3CbOps->cbOps);
    }
    g_HAL3Entry.m_pCbOpsLock->Unlock();

    return pHAL3->initialize(pCamera3DeviceAPI, pCamera3CbOpsAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// configure_streams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int configure_streams(
    const struct camera3_device*    pCamera3DeviceAPI,
    camera3_stream_configuration_t* pStreamConfigsAPI)
{
    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->configure_streams);

    return pHAL3->configure_streams(pCamera3DeviceAPI, pStreamConfigsAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// construct_default_request_settings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const camera_metadata_t* construct_default_request_settings(
    const struct camera3_device*    pCamera3DeviceAPI,
    int                             requestTemplateAPI)
{
    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->construct_default_request_settings);

    return pHAL3->construct_default_request_settings(pCamera3DeviceAPI, requestTemplateAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// process_capture_request
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int process_capture_request(
    const struct camera3_device*    pCamera3DeviceAPI,
    camera3_capture_request_t*      pCaptureRequestAPI)
{
    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->process_capture_request);

    return pHAL3->process_capture_request(pCamera3DeviceAPI, pCaptureRequestAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// dump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void dump(
    const struct camera3_device*    pCamera3DeviceAPI,
    int                             fdAPI)
{
    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->dump);

    return pHAL3->dump(pCamera3DeviceAPI, fdAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// flush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int flush(
    const struct camera3_device* pCamera3DeviceAPI)
{
    JumpTableHAL3* pHAL3 = static_cast<JumpTableHAL3*>(g_dispatchHAL3.GetJumpTable());

    CAMX_ASSERT(pHAL3);
    CAMX_ASSERT(pHAL3->flush);

    return pHAL3->flush(pCamera3DeviceAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Static Data with Dependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Array containing hw_module_methods_t methods
static hw_module_methods_t g_hwModuleMethods =
{
    CamX::open
};

/// Array containing camera3_device_ops_t methods
#if defined (_LINUX)
static camera3_device_ops_t g_camera3DeviceOps =
{
    .initialize                         = CamX::initialize,
    .configure_streams                  = CamX::configure_streams,
    .construct_default_request_settings = CamX::construct_default_request_settings,
    .process_capture_request            = CamX::process_capture_request,
    .dump                               = CamX::dump,
    .flush                              = CamX::flush,
};
#else // _LINUX
static camera3_device_ops_t g_camera3DeviceOps =
{
    CamX::initialize,
    CamX::configure_streams,
    NULL,
    CamX::construct_default_request_settings,
    CamX::process_capture_request,
    NULL,
    CamX::dump,
    CamX::flush,
    NULL,
    {0},
};
#endif // _LINUX

/// Array of HwDeviceCloseOps to hold the close method
static HwDeviceCloseOps g_hwDeviceCloseOps =
{
    close
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Definitions with Dependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetCamera3DeviceOps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
camera3_device_ops_t* GetCamera3DeviceOps()
{
    return &g_camera3DeviceOps;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetHwDeviceCloseFunc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HwDeviceCloseFunc GetHwDeviceCloseFunc()
{
    return g_hwDeviceCloseOps.hwDeviceCloseFunc;
}

CAMX_NAMESPACE_END

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Exported Data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Array to populate the HAL_MODULE_INFO_SYM export required by the HAL client (camera service framework) to access the HAL
/// module implementation.
#if defined (_LINUX)
CAMX_VISIBILITY_PUBLIC camera_module_t HAL_MODULE_INFO_SYM =
{
    .common =
    {
        .tag                = HARDWARE_MODULE_TAG,
        .module_api_version = CAMERA_MODULE_API_VERSION_CURRENT,
        .hal_api_version    = HARDWARE_HAL_API_VERSION,
        .id                 = CAMERA_HARDWARE_MODULE_ID,
        .name               = "QTI Camera HAL",
        .author             = "Qualcomm Technologies, Inc.",
        .methods            = &CamX::g_hwModuleMethods
    },
    .get_number_of_cameras  = CamX::get_number_of_cameras,
    .get_camera_info        = CamX::get_camera_info,
    .set_callbacks          = CamX::set_callbacks,
    .get_vendor_tag_ops     = CamX::get_vendor_tag_ops,
    .open_legacy            = CamX::open_legacy,
    .set_torch_mode         = CamX::set_torch_mode,
    .init                   = CamX::init
};
#else // _LINUX
CAMX_VISIBILITY_PUBLIC camera_module_t HAL_MODULE_INFO_SYM =
{
    {
        HARDWARE_MODULE_TAG,
        CAMERA_MODULE_API_VERSION_CURRENT,
        HARDWARE_HAL_API_VERSION,
        CAMERA_HARDWARE_MODULE_ID,
        "QTI Camera HAL",
        "Qualcomm Technologies, Inc.",
        &CamX::g_hwModuleMethods,
        NULL,
        {0}
    },
    CamX::get_number_of_cameras,
    CamX::get_camera_info,
    CamX::set_callbacks,
    CamX::get_vendor_tag_ops,
    CamX::open_legacy,
    CamX::set_torch_mode,
    CamX::init,
    NULL,
    NULL,
    { NULL, NULL, NULL }
};
#endif // _LINUX
// Global vendor tag ops
vendor_tag_ops_t g_vendorTagOps;

#ifdef __cplusplus
}
#endif // __cplusplus
