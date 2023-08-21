////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxextensioninterface.cpp
/// @brief Main landing functions for CHX
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chxincs.h"
#include "chxextensionmodule.h"
#include "chivendortag.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

CDK_VISIBILITY_PUBLIC CHICONTEXTOPS       g_chiContextOps;
CDK_VISIBILITY_PUBLIC CHIHANDLE           g_chiHandle;
CDK_VISIBILITY_PUBLIC CHIBUFFERMANAGEROPS g_chiBufferManagerOps;

// Dummy chi override vender tags for testing purpose
static const CHAR   ChiOverrideNodeSectionName[] = "com.qti.chi.override";             ///< The section name for override module
static CHIVENDORTAGDATA g_VendorTagDataChiOverride[] =
{
    { "override_tag1", TYPE_INT32, 1 },
    { "override_tag2", TYPE_BYTE,  1 },
    { "override_tag3", TYPE_INT64, 1 },
    { "override_tag4", TYPE_INT64, 2 }
};

static CHIVENDORTAGDATA g_VendorTagDataMulticameraSensorConfig[] =
{
    { "sensorsyncmodeconfig", TYPE_BYTE, sizeof(SensorSyncModeMetadata) },
};

static CHIVENDORTAGDATA g_VendorTagRawCbInfo[] =
{
    { "IdealRaw",            TYPE_BYTE,  sizeof(IdealRawInfo) },
};

static CHIVENDORTAGDATA g_VendorTagDataMulticameraInputMetadata[] =
{
    { "InputMetadataOpticalZoom",       TYPE_BYTE,  sizeof(InputMetadataOpticalZoom)    },
    { "InputMetadataBokeh",             TYPE_BYTE,  sizeof(InputMetadataBokeh)          }
};

static CHIVENDORTAGDATA g_VendorTagDataMulticameraOutputMetadata[] =
{
    { "OutputMetadataOpticalZoom",      TYPE_BYTE,  sizeof(OutputMetadataOpticalZoom)   },
    { "OutputMetadataBokeh",            TYPE_BYTE,  sizeof(OutputMetadataBokeh)         }
};

static CHIVENDORTAGDATA g_VendorTagDataMulticameraInfo[] =
{
    { "MultiCameraIds",                 TYPE_BYTE,  sizeof(MultiCameraIds)              },
    { "MasterCamera",                   TYPE_BYTE,  sizeof(BOOL)                        },
    { "LowPowerMode",                   TYPE_BYTE,  sizeof(LowPowerModeInfo)            },
    { "SyncMode",                       TYPE_BYTE,  sizeof(SyncModeInfo)                }
};

static CHIVENDORTAGDATA g_VendorTagDataLogicalCameraInfo[] =
{
    { "NumPhysicalCameras",             TYPE_BYTE,  sizeof(UINT32)                      },
};

static CHIVENDORTAGDATA g_VendorTagCropRegion[] =
{
    { "crop_regions",                   TYPE_BYTE,  sizeof(CaptureRequestCropRegions)   },
    { "ChiNodeResidualCrop",            TYPE_BYTE,  sizeof(CHIRECT)                     }
};

static CHIVENDORTAGDATA g_VendorTagAECLux[] =
{
    { "AecLux",                         TYPE_FLOAT,  1                                  }
};

static CHIVENDORTAGDATA g_VendorTagStatsSkip[] =
{
    { "skipFrame",                      TYPE_INT32,  1                                  }
};

static CHIVENDORTAGDATA g_VendorTagMetaOwnerInfo[] =
{
    { "MetadataOwner",                  TYPE_BYTE,  sizeof(MetadataOwnerInfo) }
};


static CHIVENDORTAGDATA g_VendorTagFeature2RequestInfo[] =
{
    { "Feature2RequestInfo",                  TYPE_BYTE,  sizeof(Feature2InputRequestInfo)},
    { "Feature2MccResult",                    TYPE_BYTE,  sizeof(Feature2ControllerResult)}
};

static CHIVENDORTAGDATA g_VendorTagDataCameraConfiguration[] =
{
    { "PhysicalCameraConfigs",      TYPE_BYTE,  sizeof(CameraConfigs)               },
    { "PhysicalCameraInputConfig",  TYPE_BYTE,  sizeof(ChiPhysicalCameraConfig)     }
};

static CHIVENDORTAGDATA g_VendorTagSuperSlowMotionFRC[] =
{
    { "CaptureStart",                    TYPE_INT32, 1                                  },
    { "CaptureComplete",                 TYPE_INT32, 1                                  },
    { "ProcessingComplete",              TYPE_INT32, 1                                  },
    { "InterpolationFactor",             TYPE_INT32, 1                                  }
};

static CHIVENDORTAGDATA g_VendorTagBPSRealtimeCam[] =
{
    { "cameraRunningOnBPS", TYPE_BYTE, sizeof(BOOL) }
};

static CHIVENDORTAGDATA g_VendorTagZSLTimeRange[] =
{
    { "ZSLTimeRange", TYPE_INT64, 2 }
};

static CHIVENDORTAGDATA g_VendorTagLivePreview[] =
{
    { "enable", TYPE_INT32, 1 }
};

static CHIVENDORTAGDATA g_VendorTagDebugDumpConfig[] =
{
    { "DebugDumpConfig", TYPE_BYTE, sizeof(DumpFileName) }
};

static CHIVENDORTAGDATA g_VendorTagInduceSleepInChiNode[] =
{
    { "InduceSleep", TYPE_INT32, 1 }
};

static CHIVENDORTAGSECTIONDATA g_VendorTagSectionDataChiOverride[] =
{
    {
        ChiOverrideNodeSectionName, 0,
        sizeof(g_VendorTagDataChiOverride) / sizeof(g_VendorTagDataChiOverride[0]),
        g_VendorTagDataChiOverride, CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    },

    {
        "com.qti.chi.multicamerasensorconfig", 0,
        sizeof(g_VendorTagDataMulticameraSensorConfig) / sizeof(g_VendorTagDataMulticameraSensorConfig[0]),
        g_VendorTagDataMulticameraSensorConfig, CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    },

    {
        "com.qti.chi.multicamerainputmetadata", 0,
        sizeof(g_VendorTagDataMulticameraInputMetadata) / sizeof(g_VendorTagDataMulticameraInputMetadata[0]),
        g_VendorTagDataMulticameraInputMetadata, CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    },

    {
        "com.qti.chi.multicameraoutputmetadata", 0,
        sizeof(g_VendorTagDataMulticameraOutputMetadata) / sizeof(g_VendorTagDataMulticameraOutputMetadata[0]),
        g_VendorTagDataMulticameraOutputMetadata, CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    },

    {
        "com.qti.chi.multicamerainfo", 0,
        sizeof(g_VendorTagDataMulticameraInfo) / sizeof(g_VendorTagDataMulticameraInfo[0]),
        g_VendorTagDataMulticameraInfo, CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    },

    {
        "com.qti.chi.logicalcamerainfo", 0,
        sizeof(g_VendorTagDataLogicalCameraInfo) / sizeof(g_VendorTagDataLogicalCameraInfo[0]),
        g_VendorTagDataLogicalCameraInfo, CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    },

    {
        "com.qti.cropregions",  0,
        sizeof(g_VendorTagCropRegion) / sizeof(g_VendorTagCropRegion[0]),
        g_VendorTagCropRegion, CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    },

    {
        "com.qti.chi.statsaec", 0,
        sizeof(g_VendorTagAECLux) / sizeof(g_VendorTagAECLux[0]),
        g_VendorTagAECLux, CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    },

    {
        "com.qti.chi.statsSkip",  0,
        sizeof(g_VendorTagStatsSkip) / sizeof(g_VendorTagStatsSkip[0]),
        g_VendorTagStatsSkip, CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    },

    {
        "com.qti.chi.metadataOwnerInfo",  0,
        sizeof(g_VendorTagMetaOwnerInfo) / sizeof(g_VendorTagMetaOwnerInfo[0]),
        g_VendorTagMetaOwnerInfo, CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    },

    {
        "com.qti.chi.feature2RequestInfo",  0,
        sizeof(g_VendorTagFeature2RequestInfo) / sizeof(g_VendorTagFeature2RequestInfo[0]),
        g_VendorTagFeature2RequestInfo, CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    },

    {
        "com.qti.chi.cameraconfiguration",  0,
        sizeof(g_VendorTagDataCameraConfiguration) / sizeof(g_VendorTagDataCameraConfiguration[0]),
        g_VendorTagDataCameraConfiguration, CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToOEM
    },

    {
        "com.qti.chi.superslowmotionfrc", 0,
        sizeof(g_VendorTagSuperSlowMotionFRC) / sizeof(g_VendorTagSuperSlowMotionFRC[0]),
        g_VendorTagSuperSlowMotionFRC, CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    },

    {
        "com.qti.chi.bpsrealtimecam", 0,
        sizeof(g_VendorTagBPSRealtimeCam) / sizeof(g_VendorTagBPSRealtimeCam[0]),
        g_VendorTagBPSRealtimeCam, CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    },
    {
        "com.qti.chi.ZSLSettings", 0,
        sizeof(g_VendorTagZSLTimeRange) / sizeof(g_VendorTagZSLTimeRange[0]),
        g_VendorTagZSLTimeRange, CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    },
    {
        "com.qti.chi.livePreview", 0,
        sizeof(g_VendorTagLivePreview) / sizeof(g_VendorTagLivePreview[0]),
        g_VendorTagLivePreview, CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    },
    {
        "org.quic.camera.debugDumpConfig", 0,
        sizeof(g_VendorTagDebugDumpConfig) / sizeof(g_VendorTagDebugDumpConfig[0]),
        g_VendorTagDebugDumpConfig, CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    },
    {
        "org.quic.camera.induceSleepInChiNode", 0,
        sizeof(g_VendorTagInduceSleepInChiNode) / sizeof(g_VendorTagInduceSleepInChiNode[0]),
        g_VendorTagInduceSleepInChiNode, CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToOEM
    },
    {
        "com.qti.chi.rawcbinfo", 0,
        sizeof(g_VendorTagRawCbInfo) / sizeof(g_VendorTagRawCbInfo[0]),
        g_VendorTagRawCbInfo, CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    }
};

static ChiVendorTagInfo g_VendorTagInfoChiOverride[] =
{
    {
        & g_VendorTagSectionDataChiOverride[0],
        sizeof(g_VendorTagSectionDataChiOverride) / sizeof(g_VendorTagSectionDataChiOverride[0])
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Allows additional modification during HAL open
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult chi_extend_open(
    uint32_t    cameraId,
    void*       priv)
{
    ExtensionModule* pExtensionModule = ExtensionModule::GetInstance();

    return pExtensionModule->ExtendOpen(cameraId, priv);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Allows additional modification during HAL close
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void chi_extend_close(
    uint32_t    cameraId,
    void*       priv)
{
    ExtensionModule* pExtensionModule = ExtensionModule::GetInstance();

    pExtensionModule->ExtendClose(cameraId, priv);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Get the number of cameras
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void chi_get_num_cameras(
    uint32_t* numFwCameras,
    uint32_t* numLogicalCameras)
{
    ExtensionModule* pExtensionModule = ExtensionModule::GetInstance();

    pExtensionModule->GetNumCameras(numFwCameras, numLogicalCameras);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Allow remapping of framework ID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static uint32_t chi_remap_camera_id(
    uint32_t            frameworkCameraId,
    CameraIdRemapMode   mode)
{
    ExtensionModule* pExtensionModule = ExtensionModule::GetInstance();

    return pExtensionModule->RemapCameraId(frameworkCameraId, mode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Get camera information
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult chi_get_camera_info(
    int             camera_id,
    camera_info*    camera_info)
{
    ExtensionModule* pExtensionModule = ExtensionModule::GetInstance();

    return pExtensionModule->GetCameraInfo(camera_id, camera_info);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Get info from override
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult chi_get_info(
    CDKGetInfoCmd       infoCmd,
    void*               inputParams,
    void*               outputParams)
{
    ExtensionModule* pExtensionModule = ExtensionModule::GetInstance();

    return pExtensionModule->GetInfo(infoCmd, inputParams, outputParams);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Main entry point
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult chi_initialize_override_session(
    uint32_t                        cameraId,
    const camera3_device_t*         camera3_device,
    const chi_hal_ops_t*            chiHalOps,
    camera3_stream_configuration_t* stream_config,
    int*                            override_config,
    void**                          priv)
{
    ExtensionModule* pExtensionModule = ExtensionModule::GetInstance();

    pExtensionModule->InitializeOverrideSession(cameraId, camera3_device, chiHalOps, stream_config, override_config, priv);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Finalize the override session
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult chi_finalize_override_session(
    const camera3_device_t* camera3_device,
    void*                   priv)
{
    ExtensionModule* pExtensionModule = ExtensionModule::GetInstance();

    return pExtensionModule->FinalizeOverrideSession(camera3_device, priv);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Destroy the session
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void chi_teardown_override_session(
    const camera3_device_t* camera3_device,
    uint64_t                session,
    void*                   priv)
{
    ExtensionModule* pExtensionModule = ExtensionModule::GetInstance();

    return pExtensionModule->TeardownOverrideSession(camera3_device, session, priv);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Process request call
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult chi_override_process_request(
    const camera3_device_t*     camera3_device,
    camera3_capture_request_t*  capture_request,
    void*                       priv)
{
    ExtensionModule* pExtensionModule = ExtensionModule::GetInstance();

    return pExtensionModule->OverrideProcessRequest(camera3_device, capture_request, priv);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Allows implementation-specific settings to be toggled in the override at runtime
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void chi_modify_settings(
    void*       priv)
{
    ExtensionModule* pExtensionModule = ExtensionModule::GetInstance();

    pExtensionModule->ModifySettings(priv);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Allows implementation-specific settings to be added to the default request template settings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID chi_get_default_request_settings(
    uint32_t                  cameraId,
    int                       requestTemplate,
    const camera_metadata_t** settings)
{
    ExtensionModule* pExtensionModule = ExtensionModule::GetInstance();

    pExtensionModule->DefaultRequestSettings(cameraId, requestTemplate, settings);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Allows implementation-specific settings to be added to the default flush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult chi_override_flush(
    const camera3_device_t*     camera3_device)
{
    ExtensionModule* pExtensionModule = ExtensionModule::GetInstance();
    return pExtensionModule->OverrideFlush(camera3_device);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Allows implementation-specific settings to be added to the default dump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult chi_override_dump(
    const camera3_device_t*     camera3_device,
    int                         fd)
{
    ExtensionModule* pExtensionModule = ExtensionModule::GetInstance();
    return pExtensionModule->OverrideDump(camera3_device, fd);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief HAL Override entry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void chi_hal_override_entry(
    chi_hal_callback_ops_t* callbacks)
{
    ExtensionModule* pExtensionModule = ExtensionModule::GetInstance();

    CHX_ASSERT(NULL != callbacks);

    if (NULL != pExtensionModule)
    {
        callbacks->chi_get_num_cameras              = chi_get_num_cameras;
        callbacks->chi_get_camera_info              = chi_get_camera_info;
        callbacks->chi_get_info                     = chi_get_info;
        callbacks->chi_initialize_override_session  = chi_initialize_override_session;
        callbacks->chi_finalize_override_session    = chi_finalize_override_session;
        callbacks->chi_override_process_request     = chi_override_process_request;
        callbacks->chi_teardown_override_session    = chi_teardown_override_session;
        callbacks->chi_extend_open                  = chi_extend_open;
        callbacks->chi_extend_close                 = chi_extend_close;
        callbacks->chi_remap_camera_id              = chi_remap_camera_id;
        callbacks->chi_modify_settings              = chi_modify_settings;
        callbacks->chi_get_default_request_settings = chi_get_default_request_settings;
        callbacks->chi_override_flush               = chi_override_flush;
        callbacks->chi_override_dump                = chi_override_dump;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief chi_hal_query_vendertag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult chi_hal_query_vendertag(
    CHIQUERYVENDORTAG* pQueryVendorTag)
{
    CDKResult result = CDKResultSuccess;
    if (NULL == pQueryVendorTag)
    {
        result = CDKResultEInvalidPointer;
    }

    if (CDKResultSuccess == result)
    {
        if (pQueryVendorTag->size >= sizeof(CHIQUERYVENDORTAG))
        {
            pQueryVendorTag->pVendorTagInfo = g_VendorTagInfoChiOverride;
        }
        else
        {
            result = CDKResultEFailed;
        }
    }
    return result;
}
