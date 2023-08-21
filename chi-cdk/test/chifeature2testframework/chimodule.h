////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chimodule.h
/// @brief Declarations for chimodule.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIMODULE_H
#define CHIMODULE_H

#include "chifeature2test.h"
#include "camxcdktypes.h"
#include "camera3.h"
#include "chi.h"

#include <vector>

// Remove this once chi.h has this typedef
typedef VOID(*PFNCHIENTRY)(CHICONTEXTOPS* pContextOps);

class ChiModule
{
public:

    static ChiModule*       GetInstance();
    static VOID             DestroyInstance();
    CHIHANDLE               GetContext() const;
    const CHICONTEXTOPS*    GetChiOps() const;
    const CHIFENCEOPS*      GetFenceOps() const;
    VOID*                   GetLibrary() const;

    /* Not needed? */
    int                     GetNumberCameras() const;
    std::vector <int>       GetCameraList() const;
    CHITAGSOPS              GetTagOps() const { return m_chiVendorTagOps; }
    const CHICAMERAINFO*    GetCameraInfo(uint32_t cameraId) const;
    CHISENSORMODEINFO*      GetCameraSensorModeInfo(uint32_t cameraId, uint32_t modeId) const;
    CHISENSORMODEINFO*      GetClosestSensorMode(uint32_t cameraId, Size resolution) const;
    //CDKResult               SubmitPipelineRequest(CHIPIPELINEREQUEST* pRequest) const;



private:

    std::vector <int>   m_camList;              // list of cameras to test
    static ChiModule*   m_pModuleInstance;      // Singleton instance of ChiModule
    CHICAMERAINFO*      m_pInstanceProps;          // pointer to instance properties
    CHICAMERAINFO*      m_pCameraInfo;          // pointer to camera info
    camera_info*        m_pLegacyCameraInfo;    // pointer to legacy camera info
    CHISENSORMODEINFO** m_pSensorInfo;          // pointer to sensor mode info
    int                 m_numOfCameras;         // number of cameras present
    CHIHANDLE           m_hContext;             // Handle to the context
    CHICONTEXTOPS       m_chiOps;               // chi general functions
    CHITAGSOPS          m_chiVendorTagOps;      // vendor tag functions
    vendor_tag_ops_t    m_halVendorTagOps;      // function pointers for finding/enumerating vendor tags
    CHIFENCEOPS         m_fenceOps;             // chi fence operations
    camera_module_t*    m_pCameraModule;        // pointer to camera module functions
    VOID*               hLibrary;               // pointer to the loaded driver dll/.so library

    const UINT32 EXPECT_CHI_API_MAJOR_VERSION = 3;
    const UINT32 EXPECT_CHI_API_MINOR_VERSION = 0;
    const UINT32 EXPECT_CHI_API_SUB_VERSION = 0;


#ifdef ENVIRONMENT64
    const char*         m_libPath = "/vendor/lib64/hw/camera.qcom.so";
#else
    const char*         m_libPath = "/vendor/lib/hw/camera.qcom.so";
#endif

    CDKResult           Initialize();
    CDKResult           LoadLibraries();
    CDKResult           OpenContext();
    CDKResult           CloseContext();

    ChiModule();
    ~ChiModule();

    /// Do not allow the copy constructor or assignment operator
    ChiModule(const ChiModule& ) = delete;
    ChiModule& operator= (const ChiModule& ) = delete;
};

#endif // CHIMODULE_H