////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodeeisv3.cpp
/// @brief Chi node for EISV3
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <system/camera_metadata.h>
#include "camxchinodeeisv3.h"
#include "chi.h"
#include "chincsdefs.h"
#include "eis_1_2_0.h"
#include "inttypes.h"
#include "parametertuningtypes.h"
#include "camxipeicatestdata.h"
#include "chistatsproperty.h"

// NOWHINE FILE CP040: Keyword new not allowed. Use the CAMX_NEW/CAMX_DELETE functions insteads

#undef LOG_TAG
#define LOG_TAG         "CHIEISV3"
#define VIDEO4KWIDTH    3840

ChiNodeInterface    g_ChiNodeInterface;         ///< The instance of the CAMX Chi interface
UINT32              g_vendorTagBase     = 0;    ///< Chi assigned runtime vendor tag base for the node

const CHAR* pEIS3LibName = "com.qti.eisv3";

/// @todo (CAMX-1854) the major / minor version shall get from CHI
static const UINT32 ChiNodeMajorVersion = 0;    ///< The major version of CHI interface
static const UINT32 ChiNodeMinorVersion = 0;    ///< The minor version of CHI interface

static const UINT32 ChiNodeCapsEISv3       = 2;                    ///< Supports EIS3.0 algorithm
static const CHAR   EISv3NodeSectionName[] = "com.qti.node.eisv3"; ///< The section name for node
static const UINT8  NumICA10Exterpolate    = 4;                    ///< Num ICA10 exterpolate corners
static const FLOAT  GyroSamplingRate       = 416.0f;               ///< Gyro Sampling rate

///< EIS config
static const UINT32 DefaultEISV3FrameDelay   = 15;      ///< Default Frame Delay
static const UINT32 MaxEISV3FrameDelay       = 30;      ///< Max Frame Delay
static const UINT32 DefaultRequestQueueDepth = 8;       ///< Request queue depth
static const UINT32 GYRO_SAMPLES_BUF_SIZE    = 512;     ///< Max Gyro sample size
static const FLOAT  EISV3Margin              = 0.2F;    ///< Default Stabilization margin

///< max gyro dumps alone per frame * 100 chars per gyro sample line per frame info
static const UINT32 GyroDumpSize           = static_cast<UINT32>(GYRO_SAMPLES_BUF_SIZE * 103 + 150);

///< Buffer size required in order to dump EIS 3.x output matrices to file
static const UINT32 PerspectiveDumpSize    = static_cast<UINT32>(GYRO_SAMPLES_BUF_SIZE * 1.5 * 3);

///< Buffer size required in order to dump EIS 3.x output grids to file
static const UINT32 GridsDumpSize          = 64 * 1024;

///< Buffer size required in order to dump EIS 3.x init input parameters to file
static const UINT32 InitDumpSize           = 32 * 1024;

///< Virtual domain dimensions
static const uint32_t IcaVirtualDomainWidth  = 8192;
static const uint32_t IcaVirtualDomainHeight = 6144;

///< Virtual domain quantization
static const uint32_t IcaVirtualDomainQuantizationV20 = 8;
static const uint32_t IcaVirtualDomainQuantizationV30 = 16;

///< Invalid Index
static const uint32_t InvalidIndex = 0xFFFFFFFF;

///< Invalid sensor mode
static const uint32_t InvalidSensorMode = 0xFFFFFFFF;

///< Default fovc crop factor
static const FLOAT    FOVCFactorDefault = 0.06f;

///< Vendor tag section names
static const CHAR IPEICASectionName[]               = "org.quic.camera2.ipeicaconfigs";
static const CHAR IFECropInfoSectionName[]          = "org.quic.camera.ifecropinfo";
static const CHAR SensorMetaSectionName[]           = "org.codeaurora.qcamera3.sensor_meta_data";
static const CHAR QTimerSectionName[]               = "org.quic.camera.qtimer";
static const CHAR StreamDimensionSectionName[]      = "org.quic.camera.streamDimension";
static const CHAR RecordingSectionName[]            = "org.quic.camera.recording";
static const CHAR EISLookAheadSectionName[]         = "org.quic.camera.eislookahead";
static const CHAR EISRealTimeSectionName[]          = "org.quic.camera.eisrealtime";
static const CHAR MultiCameraInfoSectionName[]      = "com.qti.chi.multicameraoutputmetadata";
static const CHAR CameraConfigurationSectionName[]  = "com.qti.chi.cameraconfiguration";
static const CHAR ChiNodeCropRegions[]              = "com.qti.cropregions";
static const CHAR StatsConfigSectionName[]          = "org.quic.camera2.statsconfigs";

///< This is an array of all vendor tag section data
static CHIVENDORTAGDATA g_VendorTagSectionEISV3Node[] =
{
    { "PerspectiveGridTransform", 0, sizeof(EISV3PerspectiveGridTransforms) },
};

///< This is an array of all vendor tag section data
static CHIVENDORTAGSECTIONDATA g_VendorTagEISV3NodeSection[] =
{
    {
        EISv3NodeSectionName,  0,
        sizeof(g_VendorTagSectionEISV3Node) / sizeof(g_VendorTagSectionEISV3Node[0]), g_VendorTagSectionEISV3Node,
        CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    }
};

///< This is an array of all vendor tag section data
static ChiVendorTagInfo g_VendorTagInfoEISV3Node[] =
{
    {
        &g_VendorTagEISV3NodeSection[0],
        sizeof(g_VendorTagEISV3NodeSection) / sizeof(g_VendorTagEISV3NodeSection[0])
    }
};

///< Vendor tags of interest
static CHIVENDORTAGDATA g_VendorTagSectionICA[] =
{
    { "ICAInPerspectiveTransform",       0, sizeof(IPEICAPerspectiveTransform) },
    { "ICAInGridOut2InTransform",        0, sizeof(IPEICAGridTransform) },
    { "ICAInGridTransformLookahead",     0, sizeof(IPEICAGridTransform) },
    { "ICAReferenceParams",              0, sizeof(IPEICAPerspectiveTransform) },
};

static CHIVENDORTAGDATA g_VendorTagSectionIFECropInfo[] =
{
    { "ResidualCrop", 0, sizeof(IFECropInfo) },
    { "AppliedCrop",  0, sizeof(IFECropInfo) },
    { "SensorIFEAppliedCrop", 0, sizeof(IFECropInfo) }
};

static CHIVENDORTAGDATA g_VendorTagSectionSensorMeta[] =
{
    { "mountAngle",       0, sizeof(INT32) },
    { "cameraPosition",   0, sizeof(INT32) },
    { "sensor_mode_info", 0, sizeof(ChiSensorModeInfo) },
    { "current_mode",     0, sizeof(INT32) },
    { "targetFPS",        0, sizeof(INT32) },
};

static CHIVENDORTAGDATA g_VendorTagSectionQTimer[] =
{
    { "timestamp", 0, sizeof(ChiTimestampInfo) },
};

static CHIVENDORTAGDATA g_VendorTagSectionStreamDimension[] =
{
    { "preview", 0, sizeof(ChiBufferDimension) },
    { "video",   0, sizeof(ChiBufferDimension) },
};

static CHIVENDORTAGDATA g_VendorTagSectionRecording[] =
{
    { "endOfStreamRequestId",  0, sizeof(UINT64) },
    { "requestHasVideoBuffer", 0, 1 }
};

static CHIVENDORTAGDATA g_VendorTagSectionEISLookahead[] =
{
    { "Enabled",              0, 1 },
    { "FrameDelay",           0, sizeof(UINT32) },
    { "RequestedMargin",      0, sizeof(MarginRequest) },
    { "StabilizationMargins", 0, sizeof(StabilizationMargin) },
    { "AdditionalCropOffset", 0, sizeof(ImageDimensions) },
    { "StabilizedOutputDims", 0, sizeof(CHIDimension) },
    { "MinimalTotalMargins",  0, sizeof(MarginRequest) }
};

static CHIVENDORTAGDATA g_VendorTagSectionEISRealTime[] =
{
    { "StabilizedOutputDims", 0, sizeof(CHIDimension) }
};

static CHIVENDORTAGDATA g_VendorTagSectionCameraConfiguration[] =
{
    { "PhysicalCameraConfigs", 0, sizeof(CameraConfigs) }
};

static CHIVENDORTAGDATA g_VendorTagSectionMultiCameraInfo[] =
{
    { "OutputMetadataOpticalZoom", 0, sizeof(OutputMetadataOpticalZoom) }
};

static CHIVENDORTAGDATA g_VendorTagSectionChiNodeCropRegions[] =
{
    { "ChiNodeResidualCrop", 0, sizeof(CHIRectangle) }
};

///< Vendor tags of interest
static CHIVENDORTAGDATA g_VendorTagSectionSAT[] =
{
    { "ICAInPerspectiveTransform", 0, sizeof(IPEICAPerspectiveTransform) },
};

static CHIVENDORTAGDATA g_VendorTagSectionStatsConfig[] =
{
    { "FOVCFrameControl", 0, sizeof(FOVCOutput) },
};

EISV3VendorTags g_vendorTagId = { 0 };

static const UINT32 MaxDimension       = 0xffff; //< Max dimenion for input/output port
static const UINT32 LUTSize            = 32;     //< Look up table size
static const UINT32 MaxPerspMatrixSize = 9;      //< Max size if the perspective matrix

static const NodeProperty NodePropertyEIS3InputPortType = NodePropertyVendorStart;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EISV3NodeGetCaps
///
/// @brief  Implementation of PFNNODEGETCAPS defined in chinode.h
///
/// @param  pCapsInfo   Pointer to a structure that defines the capabilities supported by the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult EISV3NodeGetCaps(
    CHINODECAPSINFO* pCapsInfo)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pCapsInfo)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pCapsInfo is NULL");
    }

    if (CDKResultSuccess == result)
    {
        if (pCapsInfo->size >= sizeof(CHINODECAPSINFO))
        {
            pCapsInfo->nodeCapsMask = ChiNodeCapsEISv3;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODECAPSINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EISV3NodeQueryVendorTag
///
/// @brief  Implementation of PFNCHIQUERYVENDORTAG defined in chinode.h
///
/// @param  pQueryVendorTag Pointer to a structure that returns the exported vendor tag
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult EISV3NodeQueryVendorTag(
    CHIQUERYVENDORTAG* pQueryVendorTag)
{
    CDKResult result = CDKResultSuccess;
    result = ChiNodeUtils::QueryVendorTag(pQueryVendorTag, g_VendorTagInfoEISV3Node);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EISV3GetOverrideSettings
///
/// @brief  Function to fetch EISv3 overridesetting
///
/// @param  pOverrideSettings Pointer to overridesettings.
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EISV3GetOverrideSettings(
    EISV3OverrideSettings* pOverrideSettings)
{
    CHAR    scratchString[128]      = { 0 };
    FILE*   pEISSettingsTextFile    = NULL;

    snprintf(scratchString,
             sizeof(scratchString),
             "%s",
             "/vendor/etc/camera/eisoverridesettings.txt");
    pEISSettingsTextFile = ChiNodeUtils::FOpen(scratchString, "r");

    if (NULL == pEISSettingsTextFile)
    {
        // We didn't find an override settings text file
        LOG_ERROR(CamxLogGroupChi, "Could not find EIS override settings text file at: %s", scratchString);
    }
    else
    {
        // We found an override settings text file.
        LOG_VERBOSE(CamxLogGroupCore, "Opening override settings text file: %s", scratchString);

        CHAR*   pSettingString      = NULL;
        CHAR*   pValueString        = NULL;
        CHAR*   pContext            = NULL;
        CHAR    strippedLine[128];

        // Parse the settings file one line at a time
        while (NULL != fgets(scratchString, sizeof(scratchString), pEISSettingsTextFile))
        {
            // First strip off all whitespace from the line to make it easier to handle enum type settings with
            // combined values (e.g. A = B | C | D). After removing the whitespace, we only need to use '=' as the
            // delimiter to extract the setting/value string pair (e.g. setting string = "A", value string =
            // "B|C|D").
            memset(strippedLine, 0x0, sizeof(strippedLine));
            ChiNodeUtils::StrStrip(strippedLine, scratchString, sizeof(strippedLine));

            // Extract a setting/value string pair.
            pSettingString  = ChiNodeUtils::StrTokReentrant(strippedLine, "=", &pContext);
            pValueString    = ChiNodeUtils::StrTokReentrant(NULL,         "=", &pContext);

            // Check for invalid lines
            if ((NULL == pSettingString) || (NULL == pValueString) || ('\0' == pValueString[0]))
            {
                continue;
            }

            // Discard this line if the setting string starts with a semicolon, indicating a comment
            if (';' == pSettingString[0])
            {
                continue;
            }

            if (0 == strcmp("EISWidthMargin", pSettingString))
            {
                FLOAT widthMargin                       = atof(pValueString);
                pOverrideSettings->margins.widthMargin  = widthMargin;
                LOG_INFO(CamxLogGroupChi, "EISv3 width Margin %f", pOverrideSettings->margins.widthMargin);
            }
            else if (0 == strcmp("EISHeightMargin", pSettingString))
            {
                FLOAT heightMargin                      = atof(pValueString);
                pOverrideSettings->margins.heightMargin = heightMargin;
                LOG_INFO(CamxLogGroupChi, "EISv3 height Margin %f", pOverrideSettings->margins.heightMargin);
            }
            else if (0 == strcmp("EISFrameDelay", pSettingString))
            {
                pOverrideSettings->frameDelay = atoi(pValueString);
                LOG_INFO(CamxLogGroupChi, "EISv3 Frame delay %d", pOverrideSettings->frameDelay);
            }
            else if (0 == strcmp("EISLDCGridEnabled", pSettingString))
            {
                pOverrideSettings->isLDCGridEnabled = (atoi(pValueString) == 1) ? 1 : 0;
                LOG_INFO(CamxLogGroupChi, "EISv3 LDC grid enabled %d", pOverrideSettings->isLDCGridEnabled);
            }
            else if (0 == strcmp("EISv3GyroDumpEnabled", pSettingString))
            {
                pOverrideSettings->isEnabledDumpInputFile = (atoi(pValueString) == 1) ? 1 : 0;
                LOG_INFO(CamxLogGroupChi, "EISv3 gyro dump to file enabled %d", pOverrideSettings->isEnabledDumpInputFile);
            }
            else if (0 == strcmp("EISv3InputDumpLogcatEnabled", pSettingString))
            {
                pOverrideSettings->isEnabledDumpInputLogcat = (atoi(pValueString) == 1) ? 1 : 0;
                LOG_INFO(CamxLogGroupChi, "EISv3 gyro dump to log enabled %d", pOverrideSettings->isEnabledDumpInputLogcat);
            }
            else if (0 == strcmp("EISv3OutputDumpEnabled", pSettingString))
            {
                pOverrideSettings->isEnabledDumpOutputFile = (atoi(pValueString) == 1) ? 1 : 0;
                LOG_INFO(CamxLogGroupChi, "EISv3 output dump to file enabled %d", pOverrideSettings->isEnabledDumpOutputFile);
            }
            else if (0 == strcmp("EISv3OutputDumpLogcatEnabled", pSettingString))
            {
                pOverrideSettings->isEnabledDumpOutputLogcat = (atoi(pValueString) == 1) ? 1 : 0;
                LOG_INFO(CamxLogGroupChi, "EISv3 output dump to log enabled %d", pOverrideSettings->isEnabledDumpOutputLogcat);
            }
            else if (0 == strcmp("EISv3OperationMode", pSettingString))
            {
                INT opMode = atoi(pValueString);
                pOverrideSettings->algoOperationMode = (cam_is_operation_mode_t)opMode;
                LOG_INFO(CamxLogGroupChi, "EISv3 operation mode %d", pOverrideSettings->algoOperationMode);
            }
            else if (0 == strcmp("EISDefaultGridTransformEnable", pSettingString))
            {
                pOverrideSettings->isDefaultGridTransformEnabled = (atoi(pValueString) == 1) ? 1 : 0;
                LOG_INFO(CamxLogGroupChi, "EIS default grid/transform enabled %d",
                         pOverrideSettings->isDefaultGridTransformEnabled);
            }
            else if (0 == strcmp("EISv3DumpForceFlush", pSettingString))
            {
                pOverrideSettings->isEnabledDumpForceFlush = (atoi(pValueString) == 1) ? 1 : 0;
                LOG_INFO(CamxLogGroupChi, "EISv3 output dump force flush enabled %d",
                    pOverrideSettings->isEnabledDumpForceFlush);
            }
            else if (0 == strcmp("EisOisMode", pSettingString))
            {
                INT oisMode = atoi(pValueString);
                pOverrideSettings->overrideOisMode = (cam_is_ois_mode)oisMode;
                LOG_INFO(CamxLogGroupChi, "EIS override OIS mode %d", oisMode);
            }
        }

        if (0 != pOverrideSettings->isEnabledDumpForceFlush)
        {
            LOG_WARN(CamxLogGroupChi, "EISv3 output dump force flush is enabled, performance degradation could occur");
        }

        ChiNodeUtils::FClose(pEISSettingsTextFile);
        pEISSettingsTextFile = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EISV3NodeCreate
///
/// @brief  Implementation of PFNNODECREATE defined in chinode.h
///
/// @param  pCreateInfo Pointer to a structure that defines create session information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult EISV3NodeCreate(
    CHINODECREATEINFO* pCreateInfo)
{
    CDKResult             result            = CDKResultSuccess;
    ChiEISV3Node*         pNode             = NULL;
    EISV3OverrideSettings overrideSettings  = { { 0 } };

    if (NULL == pCreateInfo)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pTagTypeInfo is NULL");
    }

    if (CDKResultSuccess == result)
    {
        if (pCreateInfo->size < sizeof(CHINODECREATEINFO))
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODECREATEINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    if (CDKResultSuccess == result)
    {
        overrideSettings.margins.widthMargin                  = -1;
        overrideSettings.margins.heightMargin                 = -1;
        overrideSettings.frameDelay                           = 0;
        overrideSettings.algoOperationMode                    = IS_OPT_REGULAR;
        overrideSettings.isLDCGridEnabled                     = TRUE;
        overrideSettings.isDefaultGridTransformEnabled        = FALSE;
        overrideSettings.isEnabledDumpInputFile               = FALSE;
        overrideSettings.isEnabledDumpOutputFile              = FALSE;
        overrideSettings.isEnabledDumpInputLogcat             = FALSE;
        overrideSettings.isEnabledDumpOutputLogcat            = FALSE;
        overrideSettings.isEnabledDumpForceFlush              = FALSE;

#if !_WINDOWS
        EISV3GetOverrideSettings(&overrideSettings);
#endif

        pNode = new ChiEISV3Node(overrideSettings);
        if (NULL == pNode)
        {
            result = CDKResultENoMemory;
        }
    }

    if (CDKResultSuccess == result)
    {
        result = pNode->Initialize(pCreateInfo);
    }

    if (CDKResultSuccess != result)
    {
        if (NULL != pNode)
        {
            delete pNode;
            pNode = NULL;
        }
    }

    if (CDKResultSuccess == result)
    {
        pCreateInfo->phNodeSession = reinterpret_cast<CHIHANDLE*>(pNode);
        VOID*              pData            = NULL;
        CHIMETADATAINFO    metadataInfo     = { 0 };
        const UINT32       tagSize          = 4;
        UINT32             index            = 0;
        CHITAGDATA         tagData[tagSize] = { {0} };
        UINT32             tagList[tagSize];
        BOOL               enabled          = TRUE;
        UINT32             frameDelay       = overrideSettings.frameDelay;
        MarginRequest      requestedMargin  = overrideSettings.margins;
        MarginRequest      minTotalMargin;

        pNode->GetTotalMarginsAndFrameDelay(&overrideSettings,
                                            &requestedMargin.widthMargin,
                                            &requestedMargin.heightMargin,
                                            &frameDelay);

        pNode->GetMinTotalMargins(&minTotalMargin.widthMargin, &minTotalMargin.heightMargin);

        pNode->SetFrameDelay(frameDelay);

        metadataInfo.size       = sizeof(CHIMETADATAINFO);
        metadataInfo.chiSession = pCreateInfo->hChiSession;
        metadataInfo.tagNum     = tagSize;
        metadataInfo.pTagList   = &tagList[0];
        metadataInfo.pTagData   = &tagData[0];

        tagList[index]           = (g_vendorTagId.EISV3EnabledTagId | UsecaseMetadataSectionMask);
        tagData[index].size      = sizeof(CHITAGDATA);
        tagData[index].requestId = 0;
        tagData[index].pData     = &enabled;
        tagData[index].dataSize  = g_VendorTagSectionEISLookahead[0].numUnits;
        index++;

        tagList[index]           = (g_vendorTagId.EISV3FrameDelayTagId | UsecaseMetadataSectionMask);
        tagData[index].size      = sizeof(CHITAGDATA);
        tagData[index].requestId = 0;
        tagData[index].pData     = &frameDelay;
        tagData[index].dataSize  = g_VendorTagSectionEISLookahead[1].numUnits;
        index++;

        tagList[index]           = (g_vendorTagId.EISV3RequestedMarginTagId | UsecaseMetadataSectionMask);
        tagData[index].size      = sizeof(CHITAGDATA);
        tagData[index].requestId = 0;
        tagData[index].pData     = &requestedMargin;
        tagData[index].dataSize  = g_VendorTagSectionEISLookahead[2].numUnits;
        index++;

        tagList[index]           = (g_vendorTagId.EISV3MinimalTotalMarginTagId | UsecaseMetadataSectionMask);
        tagData[index].size      = sizeof(CHITAGDATA);
        tagData[index].requestId = 0;
        tagData[index].pData     = &minTotalMargin;
        tagData[index].dataSize  = g_VendorTagSectionEISLookahead[6].numUnits;
        index++;

        LOG_INFO(CamxLogGroupChi,
                    "Publishing EISv3 enabled flag %d, Frame delay %u, requested margin w %f, h %f, "
                    "min total margin w %f h %f",
                    enabled,
                    frameDelay,
                    requestedMargin.widthMargin,
                    requestedMargin.heightMargin,
                    minTotalMargin.widthMargin,
                    minTotalMargin.heightMargin);
        g_ChiNodeInterface.pSetMetadata(&metadataInfo);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EISV3NodeDestroy
///
/// @brief  Implementation of PFNNODEDESTROY defined in chinode.h
///
/// @param  pDestroyInfo    Pointer to a structure that defines the session destroy information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult EISV3NodeDestroy(
    CHINODEDESTROYINFO* pDestroyInfo)
{
    CDKResult     result       = CDKResultSuccess;
    ChiEISV3Node* pNode        = NULL;
    CHIDATASOURCE* pDataSource = NULL;

    if ((NULL == pDestroyInfo) || (NULL == pDestroyInfo->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pDestroyInfo is NULL");
    }

    if (CDKResultSuccess == result)
    {
        if (pDestroyInfo->size >= sizeof(CHINODEDESTROYINFO))
        {
            pNode       = static_cast<ChiEISV3Node*>(pDestroyInfo->hNodeSession);

            pDataSource = pNode->GetDataSource();
            if (NULL != pDataSource)
            {
                LOG_VERBOSE(CamxLogGroupChi, "DataSource %p", pNode->GetDataSource());
                g_ChiNodeInterface.pPutDataSource(pDestroyInfo->hNodeSession, pDataSource);
            }
            delete pNode;

            pNode = NULL;
            pDestroyInfo->hNodeSession = NULL;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODEDESTROYINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    LOG_INFO(CamxLogGroupChi, "EISv3 node destroy %d", result);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EISV3NodeProcRequest
///
/// @brief  Implementation of PFNNODEPROCREQUEST defined in chinode.h
///
/// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult EISV3NodeProcRequest(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pProcessRequestInfo) || (NULL == pProcessRequestInfo->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument");
    }

    if (CDKResultSuccess == result)
    {
        if (pProcessRequestInfo->size >= sizeof(CHINODEPROCESSREQUESTINFO))
        {
            ChiEISV3Node* pNode = static_cast<ChiEISV3Node*>(pProcessRequestInfo->hNodeSession);
            result = pNode->ProcessRequest(pProcessRequestInfo);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODEPROCESSREQUESTINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EISV3NodePostPipelineCreate
///
/// @brief  Implementation of PFNPOSTPIPELINECREATE defined in chinode.h
///
/// @param  hChiHandle Pointer to a CHI handle
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult EISV3NodePostPipelineCreate(
    CHIHANDLE hChiHandle)
{
    CDKResult              result              = CDKResultSuccess;
    ChiEISV3Node*          pNode               = NULL;
    if (NULL == hChiHandle)
    {
        LOG_ERROR(CamxLogGroupChi, "Invalid Chi Handle. Cannot get NCS Data Source");
        result = CDKResultEInvalidPointer;
    }
    else
    {
        pNode  = static_cast<ChiEISV3Node*>(hChiHandle);
        result = pNode->PostPipelineCreate();
        if (CDKResultSuccess != result)
        {
            LOG_ERROR(CamxLogGroupChi, "Failed in PostPipelineCreate, result=%d", result);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EISV3NodePrepareStreamOn
///
/// @brief  On stream on
///
/// @param  pPrepareStreamOnInfo    Pointer to a structure that defines the session stream on information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult EISV3NodePrepareStreamOn(CHINODEPREPARESTREAMONINFO* pPrepareStreamOnInfo)
{
    CDKResult result = CDKResultSuccess;
    CDK_UNUSED_PARAM(pPrepareStreamOnInfo);

    LOG_VERBOSE(CamxLogGroupChi, "EISv3 prepare stream on");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EISV3NodeOnStreamOn
///
/// @brief  On stream on
///
/// @param  pOnStreamOnInfo    Pointer to a structure that defines the session stream on information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult EISV3NodeOnStreamOn(CHINODEONSTREAMONINFO* pOnStreamOnInfo)
{
    CDKResult result = CDKResultSuccess;
    CDK_UNUSED_PARAM(pOnStreamOnInfo);

    LOG_VERBOSE(CamxLogGroupChi, "EISv3 stream on : pOnStreamOnInfo: %p", pOnStreamOnInfo);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EISV3NodeOnStreamOff
///
/// @brief  On stream off
///
/// @param  pOnStreamOffInfo    Pointer to a structure that defines the session stream off information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult EISV3NodeOnStreamOff(CHINODEONSTREAMOFFINFO* pOnStreamOffInfo)
{
    CDKResult result = CDKResultSuccess;
    CDK_UNUSED_PARAM(pOnStreamOffInfo);

    LOG_VERBOSE(CamxLogGroupChi, "EISv3 stream off: pOnStreamOffInfo: %p", pOnStreamOffInfo);

    return result;
}

/// EISV3NodeSetNodeInterface
///
/// @brief  Implementation of PFCHINODESETNODEINTERFACE defined in chinode.h
///
/// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID EISV3NodeSetNodeInterface(
    ChiNodeInterface* pNodeInterface)
{
    CDKResult result = CDKResultSuccess;

    result = ChiNodeUtils::SetNodeInterface(pNodeInterface, EISv3NodeSectionName,
        &g_ChiNodeInterface, &g_vendorTagBase);

    if (CDKResultSuccess != result)
    {
        LOG_ERROR(CamxLogGroupChi, "Set Node Interface Failed");
    }
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EISV3NodeSetBufferInfo
///
/// @brief  Implementation of PFNNODESETBUFFERINFO defined in chinode.h
///
/// @param  pSetBufferInfo  Pointer to a structure with information to set the output buffer resolution and type.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult EISV3NodeSetBufferInfo(
    CHINODESETBUFFERPROPERTIESINFO* pSetBufferInfo)
{
    CDKResult     result = CDKResultSuccess;
    ChiEISV3Node* pNode  = NULL;

    if ((NULL == pSetBufferInfo) || (NULL == pSetBufferInfo->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument");
    }

    if (CDKResultSuccess == result)
    {
        if (pSetBufferInfo->size >= sizeof(CHINODESETBUFFERPROPERTIESINFO))
        {
            if (FullPath == pSetBufferInfo->portId)
            {
                pNode  = static_cast<ChiEISV3Node*>(pSetBufferInfo->hNodeSession);
                result = pNode->SetBufferInfo(pSetBufferInfo);
            }
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODESETBUFFERPROPERTIESINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EISV3NodeQueryBufferInfo
///
/// @brief  Implementation of PFNNODEQUERYBUFFERINFO defined in chinode.h
///
/// @param  pQueryBufferInfo    Pointer to a structure to query the input buffer resolution and type.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult EISV3NodeQueryBufferInfo(
    CHINODEQUERYBUFFERINFO* pQueryBufferInfo)
{
    CDKResult       result  = CDKResultSuccess;
    ChiEISV3Node*   pNode   = NULL;

    if ((NULL == pQueryBufferInfo) || (NULL == pQueryBufferInfo->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument");
    }

    if (CDKResultSuccess == result)
    {
        if (pQueryBufferInfo->numInputPorts != pQueryBufferInfo->numOutputPorts)
        {
            result = CDKResultEFailed;
            LOG_ERROR(CamxLogGroupChi, "EISv3 is a pure bypass, num inputs should be equalt to num outputs");
        }
    }

    if (CDKResultSuccess == result)
    {
        if (pQueryBufferInfo->size >= sizeof(CHINODEQUERYBUFFERINFO))
        {
            pNode  = static_cast<ChiEISV3Node*>(pQueryBufferInfo->hNodeSession);
            result = pNode->QueryBufferInfo(pQueryBufferInfo);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODEQUERYBUFFERINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EISV3NodeQueryMetadataPublishList
///
/// @brief  Implementation of PFNNODEQUERYMETADATAPUBLISHLIST defined in chinode.h
///
/// @param  pMetadataPublishlist    Pointer to a structure to query the metadata list
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult EISV3NodeQueryMetadataPublishList(
    CHINODEMETADATALIST* pMetadataPublishlist)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pMetadataPublishlist) || (NULL == pMetadataPublishlist->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument");
    }

    if (CDKResultSuccess == result)
    {
        if (pMetadataPublishlist->size == sizeof(CHINODEMETADATALIST))
        {
            ChiEISV3Node* pNode = static_cast<ChiEISV3Node*>(pMetadataPublishlist->hNodeSession);
            result = pNode->QueryMetadataPublishList(pMetadataPublishlist);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODEMETADATALIST is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeEntry
///
/// @brief  Entry point called by the Chi driver to initialize the IS node.
///
/// @param pNodeCallbacks  Pointer to a structure that defines callbacks that the Chi driver sends to the node.
///                        The node must fill in these function pointers.
///
/// @return VOID.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDK_VISIBILITY_PUBLIC VOID ChiNodeEntry(
    CHINODECALLBACKS* pNodeCallbacks)
{
    if (NULL != pNodeCallbacks)
    {
        if (pNodeCallbacks->majorVersion == ChiNodeMajorVersion &&
            pNodeCallbacks->size >= sizeof(CHINODECALLBACKS))
        {
            pNodeCallbacks->majorVersion              = ChiNodeMajorVersion;
            pNodeCallbacks->minorVersion              = ChiNodeMinorVersion;
            pNodeCallbacks->pGetCapabilities          = EISV3NodeGetCaps;
            pNodeCallbacks->pQueryVendorTag           = EISV3NodeQueryVendorTag;
            pNodeCallbacks->pCreate                   = EISV3NodeCreate;
            pNodeCallbacks->pDestroy                  = EISV3NodeDestroy;
            pNodeCallbacks->pQueryBufferInfo          = EISV3NodeQueryBufferInfo;
            pNodeCallbacks->pSetBufferInfo            = EISV3NodeSetBufferInfo;
            pNodeCallbacks->pProcessRequest           = EISV3NodeProcRequest;
            pNodeCallbacks->pChiNodeSetNodeInterface  = EISV3NodeSetNodeInterface;
            pNodeCallbacks->pPostPipelineCreate       = EISV3NodePostPipelineCreate;
            pNodeCallbacks->pPrepareStreamOn          = EISV3NodePrepareStreamOn;
            pNodeCallbacks->pOnStreamOn               = EISV3NodeOnStreamOn;
            pNodeCallbacks->pOnStreamOff              = EISV3NodeOnStreamOff;
            pNodeCallbacks->pQueryMetadataPublishList = EISV3NodeQueryMetadataPublishList;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Chi API major version doesn't match (%d:%d) vs (%d:%d)",
                pNodeCallbacks->majorVersion, pNodeCallbacks->minorVersion,
                ChiNodeMajorVersion, ChiNodeMinorVersion);
        }
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Invalid Argument: %p", pNodeCallbacks);
    }
}
#ifdef __cplusplus
}
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::LoadLib
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV3Node::LoadLib()
{
    CDKResult   result          = CDKResultSuccess;
    INT         numCharWritten  = 0;
    CHAR        libFilePath[FILENAME_MAX];

    numCharWritten = ChiNodeUtils::SNPrintF(libFilePath,
        FILENAME_MAX,
        "%s%s%s.%s",
        CameraComponentLibPath,
        PathSeparator,
        pEIS3LibName,
        SharedLibraryExtension);
    LOG_INFO(CamxLogGroupChi, "loading EIS lib %s", pEIS3LibName);

    m_hEISv3Lib = ChiNodeUtils::LibMapFullName(libFilePath);

    if (NULL == m_hEISv3Lib)
    {
        LOG_ERROR(CamxLogGroupChi, "Error loading lib %s", libFilePath);
        result = CDKResultEUnableToLoad;
    }

    ///< Map function pointers
    if (CDKResultSuccess == result)
    {
        m_eis3Initialize                    = reinterpret_cast<EIS3_INITIALIZE>(
                                              ChiNodeUtils::LibGetAddr(m_hEISv3Lib,
                                                                       "eis3_initialize"));

        m_eis3Process                       = reinterpret_cast<EIS3_PROCESS>(
                                              ChiNodeUtils::LibGetAddr(m_hEISv3Lib,
                                                                       "eis3_process"));

        m_eis3Deinitialize                  = reinterpret_cast<EIS3_DEINITIALIZE>(
                                              ChiNodeUtils::LibGetAddr(m_hEISv3Lib,
                                                                       "eis3_deinitialize"));

        m_eis3GetGyroTimeInterval           = reinterpret_cast<EIS3_GET_GYRO_INTERVAL>(
                                              ChiNodeUtils::LibGetAddr(m_hEISv3Lib,
                                                                       "eis3_get_gyro_time_interval"));

        m_eis3GetTotalMargin                = reinterpret_cast<EIS3_GET_TOTAL_MARGIN>(
                                              ChiNodeUtils::LibGetAddr(m_hEISv3Lib,
                                                                       "eis3_get_stabilization_margin"));

        m_eis3GetTotalMarginEx              = reinterpret_cast<EIS3_GET_TOTAL_MARGIN_EX>(
                                              ChiNodeUtils::LibGetAddr(m_hEISv3Lib,
                                                                       "eis3_get_stabilization_margin_ex"));

        m_eis3GetStabilizationCropRatioEx   = reinterpret_cast<EIS3_GET_STABILIZATION_CROP_RATIO_EX>(
                                              ChiNodeUtils::LibGetAddr(m_hEISv3Lib,
                                                                       "eis3_get_stabilization_crop_ratio_ex"));

        m_eisUtilsConvertToWindowRegions    = reinterpret_cast<EIS_UTILS_CONVERT_TO_WINDOW_REGIONS>(
                                              ChiNodeUtils::LibGetAddr(m_hEISv3Lib,
                                                                       "eis_utility_convert_to_window_regions"));

        m_eisUtilsLogInit                   = reinterpret_cast<EIS_UTILS_LOG_INIT>(
                                              ChiNodeUtils::LibGetAddr(m_hEISv3Lib,
                                                                       "eis_utils_log_init"));

        m_eisUtilsLogWrite                  = reinterpret_cast<EIS_UTILS_LOG_WRITE>(
                                              ChiNodeUtils::LibGetAddr(m_hEISv3Lib,
                                                                       "eis_utils_log_write"));

        m_eisUtilsLogOpen                   = reinterpret_cast<EIS_UTILS_LOG_OPEN>(
                                              ChiNodeUtils::LibGetAddr(m_hEISv3Lib,
                                                                       "eis_utils_log_open"));

        m_eisUtilsLogIsOpened               = reinterpret_cast<EIS_UTILS_LOG_IS_OPENED>(
                                              ChiNodeUtils::LibGetAddr(m_hEISv3Lib,
                                                                       "eis_utils_log_is_opened"));

        m_eisUtilsLogClose                  = reinterpret_cast<EIS_UTILS_LOG_CLOSE>(
                                              ChiNodeUtils::LibGetAddr(m_hEISv3Lib,
                                                                       "eis_utils_log_close"));

        m_eisUtilsLogFlush                  = reinterpret_cast<EIS_UTILS_LOG_CLOSE>(
                                              ChiNodeUtils::LibGetAddr(m_hEISv3Lib,
                                                                       "eis_utils_log_flush"));

        m_eisUtilsLogDestroy                = reinterpret_cast<EIS_UTILS_LOG_DESTROY>(
                                              ChiNodeUtils::LibGetAddr(m_hEISv3Lib,
                                                                       "eis_utils_log_destroy"));

        if ((NULL == m_eis3Initialize)                  ||
            (NULL == m_eis3Process)                     ||
            (NULL == m_eis3Deinitialize)                ||
            (NULL == m_eis3GetGyroTimeInterval)         ||
            (NULL == m_eis3GetTotalMargin)              ||
            (NULL == m_eis3GetTotalMarginEx)            ||
            (NULL == m_eis3GetStabilizationCropRatioEx) ||
            (NULL == m_eisUtilsConvertToWindowRegions)  ||
            (NULL == m_eisUtilsLogInit)                 ||
            (NULL == m_eisUtilsLogWrite)                ||
            (NULL == m_eisUtilsLogOpen)                 ||
            (NULL == m_eisUtilsLogIsOpened)             ||
            (NULL == m_eisUtilsLogClose)                ||
            (NULL == m_eisUtilsLogFlush)                ||
            (NULL == m_eisUtilsLogDestroy))
        {
            LOG_ERROR(CamxLogGroupChi,
                      "Error Initializing one or more function pointers in Library: %s"
                      "m_eis3Initialize %p, "
                      "m_eis3Process %p, "
                      "m_eis3Deinitialize %p"
                      "m_eis3GetGyroTimeInterval %p, "
                      "m_eis3GetTotalMargin %p, "
                      "m_eis3GetTotalMarginEx %p,"
                      "m_eis3GetStabilizationCropRatioEx %p,"
                      "m_eisUtilsConvertToWindowRegions %p, "
                      "m_eisUtilsLogInit %p, "
                      "m_eisUtilsLogWrite %p, "
                      "m_eisUtilsLogOpen %p, "
                      "m_eisUtilsLogIsOpened %p, "
                      "m_eisUtilsLogClose %p, "
                      "m_eisUtilsLogFlush %p, "
                      "m_eisUtilsLogDestroy %p, ",
                      libFilePath,
                      m_eis3Initialize,
                      m_eis3Process,
                      m_eis3Deinitialize,
                      m_eis3GetGyroTimeInterval,
                      m_eis3GetTotalMargin,
                      m_eis3GetTotalMarginEx,
                      m_eis3GetStabilizationCropRatioEx,
                      m_eisUtilsConvertToWindowRegions,
                      m_eisUtilsLogInit,
                      m_eisUtilsLogWrite,
                      m_eisUtilsLogOpen,
                      m_eisUtilsLogIsOpened,
                      m_eisUtilsLogClose,
                      m_eisUtilsLogFlush,
                      m_eisUtilsLogDestroy);

            result = CDKResultENoSuch;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::UnLoadLib
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV3Node::UnLoadLib()
{
    CDKResult result = CDKResultSuccess;

    if (NULL != m_hEISv3Lib)
    {
        result = ChiNodeUtils::LibUnmap(m_hEISv3Lib);
        m_hEISv3Lib = NULL;
    }

    if (CDKResultSuccess != result)
    {
        LOG_ERROR(CamxLogGroupChi, "Uninitialize Failed to unmap lib %s: %d", pEIS3LibName, result);
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::InitializeLib
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV3Node::InitializeLib()
{
    CDKResult           result = CDKResultSuccess;
    VOID*               pData  = NULL;
    is_init_data_common isInitDataCommon;
    is_init_data_sensor isInitDataSensors[MaxMulticamSensors];

    if (1 == m_numOfLinkedCameras)
    {
        EISV3PerSensorData* primarySensorData = &m_perSensorData[m_primarySensorIdx];

        // Get Sensor Mount Angle
        pData = ChiNodeUtils::GetMetaData(0,
                                          ANDROID_SENSOR_ORIENTATION,
                                          ChiMetadataStatic,
                                          &g_ChiNodeInterface,
                                          m_hChiSession);
        if (NULL != pData)
        {
            LOG_VERBOSE(CamxLogGroupChi, "sensor mount angle %d", *reinterpret_cast<UINT32*>(pData));
            primarySensorData->mountAngle = *reinterpret_cast<UINT32*>(pData);
        }

        // Get Camera Position
        pData = ChiNodeUtils::GetMetaData(0,
                                          g_vendorTagId.cameraPositionTagId,
                                          ChiMetadataStatic,
                                          &g_ChiNodeInterface,
                                          m_hChiSession);
        if (NULL != pData)
        {
            LOG_VERBOSE(CamxLogGroupChi, "Sensor camera position %d", *reinterpret_cast<UINT32*>(pData));
            primarySensorData->cameraPosition = *reinterpret_cast<INT32 *>(pData);
        }

        // Get active array dimension
        pData = ChiNodeUtils::GetMetaData(0,
                                          ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE,
                                          ChiMetadataStatic,
                                          &g_ChiNodeInterface,
                                          m_hChiSession);
        if (NULL != pData)
        {
            INT32* pActiveArray = reinterpret_cast<INT32*>(pData);

            primarySensorData->activeArraySize.width  = pActiveArray[2];
            primarySensorData->activeArraySize.height = pActiveArray[3];
            LOG_INFO(CamxLogGroupChi, "Sensor Active array dim [%d %d]",
                      primarySensorData->activeArraySize.width,
                      primarySensorData->activeArraySize.height);
        }

        primarySensorData->sensorDimension.width  = primarySensorData->cameraConfig.sensorModeInfo.frameDimension.width;
        primarySensorData->sensorDimension.height = primarySensorData->cameraConfig.sensorModeInfo.frameDimension.height;
        primarySensorData->targetFPS              = primarySensorData->cameraConfig.sensorModeInfo.frameRate;
    }
    else
    {
        for (UINT32 i = 0; i < m_numOfLinkedCameras; i++)
        {
            // Get Sensor Mount Angle
            pData = ChiNodeUtils::GetStaticMetaData(m_perSensorData[i].cameraConfig.cameraId,
                                                    ANDROID_SENSOR_ORIENTATION,
                                                    &g_ChiNodeInterface,
                                                    m_hChiSession);
            if (NULL != pData)
            {
                LOG_INFO(CamxLogGroupChi, "sensor mount angle %d", *reinterpret_cast<UINT32*>(pData));
                m_perSensorData[i].mountAngle = *reinterpret_cast<UINT32*>(pData);
            }

            // Get Camera Position
            pData = ChiNodeUtils::GetStaticMetaData(m_perSensorData[i].cameraConfig.cameraId,
                                                    g_vendorTagId.cameraPositionTagId,
                                                    &g_ChiNodeInterface,
                                                    m_hChiSession);
            if (NULL != pData)
            {
                LOG_INFO(CamxLogGroupChi, "Sensor camera position %d", *reinterpret_cast<UINT32*>(pData));
                m_perSensorData[i].cameraPosition = *reinterpret_cast<INT32 *>(pData);
            }

            // Get active array dimension
            pData = ChiNodeUtils::GetStaticMetaData(m_perSensorData[i].cameraConfig.cameraId,
                                              ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE,
                                              &g_ChiNodeInterface,
                                              m_hChiSession);
            if (NULL != pData)
            {
                INT32* pActiveArray = reinterpret_cast<INT32*>(pData);
                m_perSensorData[i].activeArraySize.width  = pActiveArray[2];
                m_perSensorData[i].activeArraySize.height = pActiveArray[3];
                LOG_INFO(CamxLogGroupChi, "Sensor%d Active array dim [%d %d]",
                          i,
                          m_perSensorData[i].activeArraySize.width,
                          m_perSensorData[i].activeArraySize.height);
            }

            // Get Sensor Mode Info
            m_perSensorData[i].sensorDimension.width  = m_perSensorData[i].cameraConfig.sensorModeInfo.frameDimension.width;
            m_perSensorData[i].sensorDimension.height = m_perSensorData[i].cameraConfig.sensorModeInfo.frameDimension.height;
            m_perSensorData[i].targetFPS              = m_perSensorData[i].cameraConfig.sensorModeInfo.frameRate;
        }
    }

    memset(&isInitDataCommon, 0, sizeof(isInitDataCommon));
    memset(isInitDataSensors, 0, MaxMulticamSensors * sizeof(*isInitDataSensors));

    isInitDataCommon.is_type                            = IS_TYPE_EIS_3_0;
    //TODO: handle IS_OPT_UNDISTORTION_ONLY operation mode case, FOR LDC + ERS only
    isInitDataCommon.operation_mode                     = m_algoOperationMode;
    isInitDataCommon.deployment_type                    = GetDeploymentType();
    isInitDataCommon.is_output_frame_width              = m_perSensorData[m_primarySensorIdx].outputSize.width;
    isInitDataCommon.is_output_frame_height             = m_perSensorData[m_primarySensorIdx].outputSize.height;
    isInitDataCommon.frame_fps                          = m_perSensorData[m_primarySensorIdx].targetFPS;
    isInitDataCommon.do_virtual_upscale_in_transform    = FALSE;
    isInitDataCommon.force_split_grid_matrix            = FALSE;
    isInitDataCommon.unify_undistortion_grid            = FALSE;
    isInitDataCommon.is_sat_enabled                     = FALSE;
    isInitDataCommon.is_mag_enabled                     = FALSE;
    isInitDataCommon.is_acc_enabled                     = FALSE;
    isInitDataCommon.is_ois_enabled                     = FALSE;
    isInitDataCommon.is_linear_acc_enabled              = FALSE;
    isInitDataCommon.is_abs_orientation_enabled         = FALSE;
    isInitDataCommon.is_rel_orientation_enabled         = FALSE;
    isInitDataCommon.buffer_delay                       = static_cast<uint16_t>(m_lookahead + 1);

    for (UINT32 i = 0; i < m_numOfLinkedCameras; i++)
    {
        isInitDataSensors[i].ife_input_frame_width              = m_perSensorData[i].sensorDimension.width;
        isInitDataSensors[i].ife_input_frame_height             = m_perSensorData[i].sensorDimension.height;
        isInitDataSensors[i].ife_input_frame_undistorted_width  = m_perSensorData[i].sensorDimension.width;
        isInitDataSensors[i].ife_input_frame_undistorted_height = m_perSensorData[i].sensorDimension.height;
        isInitDataSensors[i].image_sensor_crop                  = GetSensorAppliedCrop(i);

        LOG_INFO(CamxLogGroupChi,
                  "Sensor id %d, sensor dim [%d %d] frame dim [%d %d %d %d], crop info [%d %d %d %d], binning H %d V %d"
                  " applied sensor crop [%f %f %f %f %f %f]",
                  i,
                  m_perSensorData[i].sensorDimension.width,
                  m_perSensorData[i].sensorDimension.height,
                  m_perSensorData[i].cameraConfig.sensorModeInfo.frameDimension.left,
                  m_perSensorData[i].cameraConfig.sensorModeInfo.frameDimension.top,
                  m_perSensorData[i].cameraConfig.sensorModeInfo.frameDimension.width,
                  m_perSensorData[i].cameraConfig.sensorModeInfo.frameDimension.height,
                  m_perSensorData[i].cameraConfig.sensorModeInfo.cropInfo.left,
                  m_perSensorData[i].cameraConfig.sensorModeInfo.cropInfo.top,
                  m_perSensorData[i].cameraConfig.sensorModeInfo.cropInfo.width,
                  m_perSensorData[i].cameraConfig.sensorModeInfo.cropInfo.height,
                  m_perSensorData[i].cameraConfig.sensorModeInfo.horizontalBinning,
                  m_perSensorData[i].cameraConfig.sensorModeInfo.verticalBinning,
                  isInitDataSensors[i].image_sensor_crop.fullWidth,
                  isInitDataSensors[i].image_sensor_crop.fullHeight,
                  isInitDataSensors[i].image_sensor_crop.windowLeft,
                  isInitDataSensors[i].image_sensor_crop.windowTop,
                  isInitDataSensors[i].image_sensor_crop.windowWidth,
                  isInitDataSensors[i].image_sensor_crop.windowHeight);

        isInitDataSensors[i].is_input_frame_width   = m_perSensorData[i].inputSize.width;
        isInitDataSensors[i].is_input_frame_height  = m_perSensorData[i].inputSize.height;
        isInitDataSensors[i].sensor_mount_angle     = m_perSensorData[i].mountAngle;
        isInitDataSensors[i].camera_position        = static_cast <CameraPosition>(
                                                      m_perSensorData[i].cameraConfig.sensorCaps.facing);

        // TODO: update optical center from sensor data
        isInitDataSensors[i].optical_center_x = 0;
        isInitDataSensors[i].optical_center_y = 0;

        result = GetChromatixData(&isInitDataSensors[i], i);
    }

    if (1 < m_numOfLinkedCameras)
    {
        isInitDataCommon.is_sat_enabled = TRUE;
    }

    if (CDKResultSuccess == result)
    {
        int32_t isResult = m_eis3Initialize(&m_phEIS3Handle, &isInitDataCommon, &isInitDataSensors[0], m_numOfLinkedCameras);
        if (IS_RET_SUCCESS != isResult)
        {
            LOG_ERROR(CamxLogGroupChi, "EISV3 Algorithm Init Failed");
            m_phEIS3Handle    = NULL;
            result            = CDKResultEFailed;
        }
    }

    if (IS_UTILS_FLAG_NONE != m_isUtilsLogFlags)
    {
        is_utils_log_init logInit;

        logInit.init_common     = &isInitDataCommon;
        logInit.init_sensors    = isInitDataSensors;
        logInit.num_sensors     = m_numOfLinkedCameras;
        logInit.flags           = m_isUtilsLogFlags;

        CHIDateTime dateTime    = { 0 };
        ChiNodeUtils::GetDateTime(&dateTime);
        ChiNodeUtils::SNPrintF(logInit.file_prefix,
                               sizeof(logInit.file_prefix),
                               "VID_%04d%02d%02d_%02d%02d%02d_%dx%d",
                               dateTime.year + 1900,
                               dateTime.month + 1,
                               dateTime.dayOfMonth,
                               dateTime.hours,
                               dateTime.minutes,
                               dateTime.seconds,
                               m_perSensorData[m_primarySensorIdx].outputSize.width,
                               m_perSensorData[m_primarySensorIdx].outputSize.height);

        CAMX_ASSERT(NULL == m_pEisUtilsLogContext);

        CDKResult logResult = m_eisUtilsLogInit(&m_pEisUtilsLogContext, &logInit);
        if (IS_RET_SUCCESS != logResult)
        {
            result = CDKResultEFailed;
            LOG_ERROR(CamxLogGroupChi, "EISV3 Algorithm Init log utility Failed");
        }
    }
    if (false != m_isEnabledDumpForceFlush)
    {
        if ((NULL != m_pEisUtilsLogContext) && (NULL != m_eisUtilsLogOpen) && (NULL != m_eisUtilsLogIsOpened))
        {
            bool      isLogOpened = FALSE;
            CDKResult logResult   = m_eisUtilsLogIsOpened(m_pEisUtilsLogContext, &isLogOpened);

            if ((IS_RET_SUCCESS == logResult) && (FALSE == isLogOpened))
            {
                char file_prefix[EIS_UTIL_MAX_FILE_PREFIX_LENGTH];
                CHIDateTime dateTime = { 0 };
                ChiNodeUtils::GetDateTime(&dateTime);
                ChiNodeUtils::SNPrintF(file_prefix,
                                       sizeof(file_prefix),
                                       "VID_%04d%02d%02d_%02d%02d%02d_%dx%d",
                                       dateTime.year + 1900,
                                       dateTime.month + 1,
                                       dateTime.dayOfMonth,
                                       dateTime.hours,
                                       dateTime.minutes,
                                       dateTime.seconds,
                                       m_perSensorData[m_primarySensorIdx].outputSize.width,
                                       m_perSensorData[m_primarySensorIdx].outputSize.height);
                // Reopen new log with new prefix
                CDKResult logResult = m_eisUtilsLogOpen(m_pEisUtilsLogContext, file_prefix);

                if (IS_RET_SUCCESS != logResult)
                {
                    result = CDKResultEFailed;
                    LOG_ERROR(CamxLogGroupChi, "EISV3 Algorithm re-open log utility Failed");
                }
            }
        }
    }


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::FillVendorTagIds
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV3Node::FillVendorTagIds()
{
    CDKResult            result        = CDKResultSuccess;
    CHIVENDORTAGBASEINFO vendorTagBase = { 0 };

    // Get ICA In Perspective Transform Vendor Tag Id
    result = ChiNodeUtils::GetVendorTagBase(IPEICASectionName,
                                            g_VendorTagSectionICA[0].pVendorTagName,
                                            &g_ChiNodeInterface,
                                            &vendorTagBase);
    if (CDKResultSuccess == result)
    {
        // Save ICA In Perspective Transform Vendor Tag Id
        g_vendorTagId.ICAInPerspectiveTransformTagId = vendorTagBase.vendorTagBase;
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Unable to get ICA In Perspective Transform Vendor Tag Id");
    }

    // Get ICA In Grid out2in Transform Vendor Tag Id
    result = ChiNodeUtils::GetVendorTagBase(IPEICASectionName,
                                            g_VendorTagSectionICA[1].pVendorTagName,
                                            &g_ChiNodeInterface,
                                            &vendorTagBase);
    if (CDKResultSuccess == result)
    {
        // Save ICA In Grid out2in Transform Vendor Tag Id
        g_vendorTagId.ICAInGridOut2InTransformTagId = vendorTagBase.vendorTagBase;
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Unable to get ICA In Grid out2in Transform Vendor Tag Id");
    }

    // Get ICA In Grid Transform lookahead Vendor Tag Id
    result = ChiNodeUtils::GetVendorTagBase(IPEICASectionName,
                                            g_VendorTagSectionICA[2].pVendorTagName,
                                            &g_ChiNodeInterface,
                                            &vendorTagBase);
    if (CDKResultSuccess == result)
    {
        // Save ICA In Grid Transform lookahead Vendor Tag Id
        g_vendorTagId.ICAInGridTransformLookAheadTagId = vendorTagBase.vendorTagBase;
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Unable to get ICA In Grid Transform lookahead Vendor Tag Id");
    }

    if (CDKResultSuccess == result)
    {
        // Get IFE Residual Crop Info Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(IFECropInfoSectionName,
                                                g_VendorTagSectionIFECropInfo[0].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save IFE Residual Crop Info Vendor Tag Id
            g_vendorTagId.residualCropTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get IFE Residual Crop Info Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get IFE Applied Crop Info Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(IFECropInfoSectionName,
                                                g_VendorTagSectionIFECropInfo[1].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save IFE Applied Crop Info Vendor Tag Id
            g_vendorTagId.appliedCropTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get IFE Applied Crop Info Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get Sensor Mount Angle Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(SensorMetaSectionName,
                                                g_VendorTagSectionSensorMeta[0].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save Sensor Mount Angle Vendor Tag Id
            g_vendorTagId.mountAngleTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get Sensor Mount Angle Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get Camera Sensor Position Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(SensorMetaSectionName,
                                                g_VendorTagSectionSensorMeta[1].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save Camera Sensor Position Vendor Tag Id
            g_vendorTagId.cameraPositionTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get Camera Sensor Position Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get Sensor Mode Info Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(SensorMetaSectionName,
                                                g_VendorTagSectionSensorMeta[2].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save Sensor Mode Info Vendor Tag Id
            g_vendorTagId.sensorInfoTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get Sensor Mode Info Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get QTimer SOF Timestamp Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(QTimerSectionName,
                                                g_VendorTagSectionQTimer[0].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save QTimer SOF Timestamp Vendor Tag Id
            g_vendorTagId.QTimerTimestampTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get QTimer SOF Timestamp Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get Preview Stream Dimension Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(StreamDimensionSectionName,
                                                g_VendorTagSectionStreamDimension[0].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save Preview Stream Dimension Vendor Tag Id
            g_vendorTagId.previewStreamDimensionsTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get Preview Stream Dimension Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get Video Stream Dimension Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(StreamDimensionSectionName,
                                                g_VendorTagSectionStreamDimension[1].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save Video Stream Dimension Vendor Tag Id
            g_vendorTagId.videoStreamDimensionsTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get Video Stream Dimension Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get EISv3 enabled flag Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(EISLookAheadSectionName,
                                                g_VendorTagSectionEISLookahead[0].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save EISv3 enabled flag Vendor Tag Id
            g_vendorTagId.EISV3EnabledTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get EISLookahead enabled flag Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get EISv3 frame delay Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(EISLookAheadSectionName,
                                                g_VendorTagSectionEISLookahead[1].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save EISv3 frame delay Vendor Tag Id
            g_vendorTagId.EISV3FrameDelayTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get EISv3 frame delay Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get EISv3 Requested Margin Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(EISLookAheadSectionName,
                                                g_VendorTagSectionEISLookahead[2].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save EISv3 Requested Margin Vendor Tag Id
            g_vendorTagId.EISV3RequestedMarginTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get EISLookahead Requested Margin Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get EISv3 actual stabilization margins Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(EISLookAheadSectionName,
                                                g_VendorTagSectionEISLookahead[3].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save EISv3 actual stabilization margins Vendor Tag Id
            g_vendorTagId.EISV3StabilizationMarginsTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get EISLookahead actual stabilization margins Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get EISv3 additional crop offset Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(EISLookAheadSectionName,
                                                g_VendorTagSectionEISLookahead[4].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save EISv3 additional crop offset Vendor Tag Id
            g_vendorTagId.EISV3AdditionalCropOffsetTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get EISLookahead additional crop offset Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get EISv3 Stabilized Output Dimensions offset Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(EISLookAheadSectionName,
                                                g_VendorTagSectionEISLookahead[5].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save EISv3 Stabilized Output Dimensions Vendor Tag Id
            g_vendorTagId.EISV3StabilizedOutputDimsTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get EISLookahead Stabilized Output Dimensions Vendor Tag Id");
        }
        // Get EISv3 Stabilized Output Dimensions Vendor Tag Id from real time peer
        result = ChiNodeUtils::GetVendorTagBase(EISRealTimeSectionName,
                                                g_VendorTagSectionEISRealTime[0].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            g_vendorTagId.EISV3OutputDimsRealTimeTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            g_vendorTagId.EISV3OutputDimsRealTimeTagId = 0;
            LOG_WARN(CamxLogGroupChi, "Unable to get EISRealtime Stabilized Output Dimensions Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get EISv3 minimal total margin Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(EISLookAheadSectionName,
                                                g_VendorTagSectionEISLookahead[6].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save EISv3 minimal toatl margins Vendor Tag Id
            g_vendorTagId.EISV3MinimalTotalMarginTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get EISV3 minimal total margins Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get EISv3 Perspective Transform Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(EISv3NodeSectionName,
                                                g_VendorTagSectionEISV3Node[0].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save EISv3 Perspective Transform Vendor Tag Id
            g_vendorTagId.EISV3PerspectiveGridTransformTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get EISv3 Perspective Transform Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get Current Sensor Mode Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(SensorMetaSectionName,
                                                g_VendorTagSectionSensorMeta[3].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save Current Sensor Mode Vendor Tag Id
            g_vendorTagId.currentSensorModeTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get Current Sensor Mode Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get Target FPS for the usecase Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(SensorMetaSectionName,
                                                g_VendorTagSectionSensorMeta[4].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save Target FPS for the usecase Vendor Tag Id
            g_vendorTagId.targetFPSTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get Target FPS Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get end of stream request Tag Id
        result = ChiNodeUtils::GetVendorTagBase(RecordingSectionName,
                                                g_VendorTagSectionRecording[0].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save Current endo of stream request Tag Id
            g_vendorTagId.endOfStreamRequestTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get end of stream request Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get end of stream request Tag Id
        result = ChiNodeUtils::GetVendorTagBase(RecordingSectionName,
                                                g_VendorTagSectionRecording[1].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save request has video buffer Tag Id
            g_vendorTagId.requestHasVideoBufferTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get request has video buffer tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get Current Sensor Mode Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(CameraConfigurationSectionName,
                                                g_VendorTagSectionCameraConfiguration[0].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save Current Sensor Mode Vendor Tag Id
            g_vendorTagId.physicalCameraConfigsTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get physical camera configs Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get Current camera id Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(MultiCameraInfoSectionName,
                                                g_VendorTagSectionMultiCameraInfo[0].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save Current multi camera role Tag Id
            g_vendorTagId.multiCameraIdTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get physical camera configs Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get Chi node residual crop Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(ChiNodeCropRegions,
                                                g_VendorTagSectionChiNodeCropRegions[0].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save Chi node residual crop Vendor Tag Id
            g_vendorTagId.chiNodeResidualCropTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get Chi node residual crop Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get SAT transform Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(IPEICASectionName,
                                                g_VendorTagSectionSAT[0].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save SAT transform Vendor Tag Id
            g_vendorTagId.SATPerspectiveTransformTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get Chi node residual crop Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get ICA reference params Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(IPEICASectionName,
                                                g_VendorTagSectionICA[3].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save ICA reference params Vendor Tag Id
            g_vendorTagId.ICAReferenceParamsTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get ICA reference params Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get FOVC Control Info Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(StatsConfigSectionName,
                                                g_VendorTagSectionStatsConfig[0].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save FOVC Control Info Vendor Tag Id
            g_vendorTagId.FOVCFactorTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get FOVC Control Info Vendor Tag Id");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV3Node::Initialize(
    CHINODECREATEINFO* pCreateInfo)
{
    CDKResult result = CDKResultSuccess;

    /// @todo (CAMX-1854) Check for Node Capabilities using NodeCapsMask
    m_hChiSession = pCreateInfo->hChiSession;
    m_nodeId      = pCreateInfo->nodeId;
    m_nodeCaps    = pCreateInfo->nodeCaps.nodeCapsMask;
    m_ICAVersion  = pCreateInfo->chiICAVersion;
    m_isRecording = FALSE;

    // Set flag to indicate DRQ that it can preempt this node on recording stop
    pCreateInfo->nodeFlags.canDRQPreemptOnStopRecording = TRUE;

    // Set flag to indicate chi node wrapper that this node can set its own input buffer dependencies
    pCreateInfo->nodeFlags.canSetInputBufferDependency  = TRUE;

    m_nodeFlags = pCreateInfo->nodeFlags;

    //Fill Vendor Tag IDs
    if (CDKResultSuccess == result)
    {
        result = FillVendorTagIds();
        if (CDKResultSuccess != result)
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to query required vendor tag locations");
        }
    }

    // Load EISv3 lib
    if (CDKResultSuccess == result)
    {
        result = LoadLib();
        if (CDKResultSuccess != result)
        {
            LOG_ERROR(CamxLogGroupChi, "Load EISv3 algo lib failed");
        }
    }

    // Get physical camera config
    if (CDKResultSuccess == result)
    {
        result = GetPerCameraConfig();
        if (CDKResultSuccess != result)
        {
            LOG_ERROR(CamxLogGroupChi, "Get per camera config failed");
        }
    }

    // Get default tuning handle
    if (CDKResultSuccess == result)
    {
        for (UINT32 i = 0; i < m_numOfLinkedCameras; i++)
        {
            result = GetChromatixTuningHandle(i, InvalidSensorMode);
            if (CDKResultSuccess != result)
            {
                LOG_ERROR(CamxLogGroupChi, "Unable to get chromatix tunng handle");
                break;
            }
        }
    }

    if (CDKResultSuccess == result)
    {
        LOG_INFO(CamxLogGroupChi, "Num node props %d", pCreateInfo->nodePropertyCount);
        for (UINT i = 0; i < pCreateInfo->nodePropertyCount; i++)
        {
            if (NodePropertyEIS3InputPortType == pCreateInfo->pNodeProperties[i].id)
            {
                const CHAR* temp = reinterpret_cast<const CHAR*>(pCreateInfo->pNodeProperties[i].pValue);
                m_inputPortPathType = static_cast<EISV3PATHTYPE>(atoi(temp));
                LOG_INFO(CamxLogGroupChi, "Input port path type %d", m_inputPortPathType);
            }
            else if (NodePropertyEnableFOVC == pCreateInfo->pNodeProperties[i].id)
            {
                m_isFOVCEnabled = *reinterpret_cast<const UINT*>(pCreateInfo->pNodeProperties[i].pValue);
                LOG_INFO(CamxLogGroupChi, "Is FOVC enabled %d", m_isFOVCEnabled);
            }
        }

        m_pEndOfStreamLock        = CamX::Mutex::Create("EISV3EndOfStreamLock");

        for (UINT32 i = 0; i < m_numOfLinkedCameras; i++)
        {
            m_pLDCIn2OutGrid[i] = static_cast<NcLibWarpGridCoord*>(CHI_CALLOC(
                                  ICA30GridTransformWidth * ICA30GridTransformHeight, sizeof(NcLibWarpGridCoord)));
            m_pLDCOut2InGrid[i] = static_cast<NcLibWarpGridCoord*>(CHI_CALLOC(
                                  ICA30GridTransformWidth * ICA30GridTransformHeight, sizeof(NcLibWarpGridCoord)));

            if ((NULL == m_pLDCIn2OutGrid[i]) || (NULL == m_pLDCOut2InGrid[i]))
            {
                LOG_ERROR(CamxLogGroupChi, "Falied to allocate memory for LDC grids");
                result = CDKResultENoMemory;
                break;
            }
        }

        if (NULL == m_pEndOfStreamLock)
        {
            LOG_ERROR(CamxLogGroupChi, "Falied to allocate memory m_pEndOfStreamLock %p", m_pEndOfStreamLock);
            result = CDKResultENoMemory;
        }
    }

    if (CDKResultSuccess != result)
    {
        if (NULL != m_pEndOfStreamLock)
        {
            m_pEndOfStreamLock->Destroy();
            m_pEndOfStreamLock = NULL;
        }

        for (UINT32 i = 0; i < m_numOfLinkedCameras; i++)
        {
            if (NULL != m_pLDCIn2OutGrid[i])
            {
                CHI_FREE(m_pLDCIn2OutGrid[i]);
                m_pLDCIn2OutGrid[i] = NULL;
            }

            if (NULL != m_pLDCOut2InGrid[i])
            {
                CHI_FREE(m_pLDCOut2InGrid[i]);
                m_pLDCOut2InGrid[i] = NULL;
            }
        }

        UnLoadLib();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::SetDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiEISV3Node::SetDependencies(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo)
{
    CHIDEPENDENCYINFO*   pDependencyInfo  = pProcessRequestInfo->pDependency;
    UINT16               depCount         = 0;

    if ((TRUE == IsRealTimeNode()) && (1 == m_numOfLinkedCameras))
    {
        pDependencyInfo->properties[depCount]   = g_vendorTagId.residualCropTagId;
        pDependencyInfo->offsets[depCount]      = 0;
        pDependencyInfo->count                  = ++depCount;

        pDependencyInfo->properties[depCount]   = g_vendorTagId.appliedCropTagId;
        pDependencyInfo->offsets[depCount]      = 0;
        pDependencyInfo->count                  = ++depCount;
    }
    else if (1 < m_numOfLinkedCameras)
    {
        pDependencyInfo->properties[depCount]   = g_vendorTagId.chiNodeResidualCropTagId;
        pDependencyInfo->offsets[depCount]      = 0;
        pDependencyInfo->count                  = ++depCount;

        // set SAT ICA In perspective transform dependency
        pDependencyInfo->properties[depCount]   = g_vendorTagId.SATPerspectiveTransformTagId;
        pDependencyInfo->offsets[depCount]      = 0;
        pDependencyInfo->inputPortId[depCount]  = FullPath;
        pDependencyInfo->count                  = ++depCount;
    }

    pDependencyInfo->hNodeSession = m_hChiSession;

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::GetGyroInterval
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV3Node::GetGyroInterval(
    UINT64          frameNum,
    UINT32          sensorIndex,
    gyro_times_t*   pGyroInterval,
    frame_times_t*  pFrameTimesIn)
{
    VOID*                pData                          = NULL;
    CHITIMESTAMPINFO*    timestampInfo                  = NULL;
    INT64                frameDuration                  = 0;
    INT64                sensorExpTime                  = 0;
    INT64                sensorRollingShutterSkewNano   = 0;
    CDKResult            result                         = CDKResultSuccess;
    frame_times_t        frameTimes                     = { 0 };
    UINT32               cameraId                       = InvalidCameraId;
    UINT32               SOFTimestampTagId              = g_vendorTagId.QTimerTimestampTagId;
    UINT32               frameDurationTagId             = ANDROID_SENSOR_FRAME_DURATION;
    UINT32               ExposureTimeTagId              = ANDROID_SENSOR_EXPOSURE_TIME;
    UINT32               rollingShutterSkew             = ANDROID_SENSOR_ROLLING_SHUTTER_SKEW;

    if (1 < m_numOfLinkedCameras)
    {
        cameraId = m_perSensorData[sensorIndex].cameraConfig.cameraId;
    }

    if (FALSE == IsRealTimeNode())
    {
        SOFTimestampTagId   |= InputMetadataSectionMask;
        frameDurationTagId  |= InputMetadataSectionMask;
        ExposureTimeTagId   |= InputMetadataSectionMask;
        rollingShutterSkew  |= InputMetadataSectionMask;
    }

    ///< Get SOF Timestamp from QTimer
    pData = ChiNodeUtils::GetMulticamDynamicMetaByCamId(frameNum,
                                                        SOFTimestampTagId,
                                                        ChiMetadataDynamic,
                                                        &g_ChiNodeInterface,
                                                        m_hChiSession,
                                                        cameraId);
    if (NULL != pData)
    {
        timestampInfo = static_cast<CHITIMESTAMPINFO*>(pData);
        LOG_VERBOSE(CamxLogGroupChi, "QTimer timestamp(ns) %" PRIu64, timestampInfo->timestamp);
    }

    ///< Get Sensor frame Duration
    pData = ChiNodeUtils::GetMulticamDynamicMetaByCamId(frameNum,
                                                        frameDurationTagId,
                                                        ChiMetadataDynamic,
                                                        &g_ChiNodeInterface,
                                                        m_hChiSession,
                                                        cameraId);
    if (NULL != pData)
    {
        frameDuration = *static_cast<INT64 *>(pData);
        LOG_VERBOSE(CamxLogGroupChi, "Frame Duration(ns) %" PRId64, frameDuration);
    }

    // Sensor exposure time
    pData = ChiNodeUtils::GetMulticamDynamicMetaByCamId(frameNum,
                                                        ExposureTimeTagId,
                                                        ChiMetadataDynamic,
                                                        &g_ChiNodeInterface,
                                                        m_hChiSession,
                                                        cameraId);
    if (NULL != pData)
    {
        sensorExpTime = *(static_cast<INT64 *>(pData));
        LOG_VERBOSE(CamxLogGroupChi, "Sensor Exposure time(ns) %" PRId64, sensorExpTime);
    }

    ///< Get rolling shutter skew aka sensor readout duration
    pData = ChiNodeUtils::GetMulticamDynamicMetaByCamId(frameNum,
                                                        rollingShutterSkew,
                                                        ChiMetadataDynamic,
                                                        &g_ChiNodeInterface,
                                                        m_hChiSession,
                                                        cameraId);
    if (NULL != pData)
    {
        sensorRollingShutterSkewNano = *static_cast<INT64*>(pData);
        LOG_VERBOSE(CamxLogGroupChi, "Rolling shutter skew in ms %d",
                    static_cast<UINT32>(NanoToMicro(sensorRollingShutterSkewNano)));
    }

    // Check timestampInfo before dereferencing it.
    if ((CDKResultSuccess) == result && (NULL != timestampInfo))
    {
        frameTimes.sof                          = NanoToMicro(timestampInfo->timestamp);
        frameTimes.frame_time                   = static_cast<UINT32>(NanoToMicro(frameDuration));
        frameTimes.exposure_time                = static_cast<UINT32>(NanoToMicro(sensorExpTime));
        frameTimes.sensor_rolling_shutter_skew  = static_cast<UINT32>(NanoToMicro(sensorRollingShutterSkewNano));

        int32_t isResult = m_eis3GetGyroTimeInterval(m_phEIS3Handle, &frameTimes, sensorIndex, pGyroInterval);
        LOG_VERBOSE(CamxLogGroupChi,
                    "Requested gyro time window (us) %" PRIu64 " %" PRIu64
                    ", inputs sof %" PRIu64
                    ", frame time %d"
                    ", exp time %d"
                    ", sensor_rolling_shutter_skew %d",
                    pGyroInterval->first_ts,
                    pGyroInterval->last_ts,
                    frameTimes.sof,
                    frameTimes.frame_time,
                    frameTimes.exposure_time,
                    frameTimes.sensor_rolling_shutter_skew);

        if (IS_RET_SUCCESS != isResult)
        {
            LOG_ERROR(CamxLogGroupChi, "Cannot get Gyro interval");
            result = CDKResultEFailed;
        }

        if (NULL != pFrameTimesIn)
        {
            *pFrameTimesIn = frameTimes;
        }
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Error Initializing EIS3 Algorithm");
    }

    if (CDKResultSuccess == result)
    {
        result = ValidateEISGyroInterval(pGyroInterval);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::SetGyroDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiEISV3Node::SetGyroDependency(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo,
    UINT32                     sensorIndex)
{
    CHIDATAREQUEST    dataRequest;
    gyro_times_t      gyroInterval     = { 0 };
    CDKResult         result           = CDKResultSuccess;
    ChiNCSDataRequest ncsRequest       = { 0, {0}, NULL, 0};

    CHAR              chiFenceName[MaxStringLength64] = { 0 };

    memset(&dataRequest, 0x0, sizeof(CHIDATAREQUEST));

    result = GetGyroInterval(pProcessRequestInfo->frameNum, sensorIndex, &gyroInterval, NULL);

    if (CDKResultSuccess == result)
    {
        //< Create fence
        CHIFENCECREATEPARAMS chiFenceParams = { 0 };
        chiFenceParams.type                 = ChiFenceTypeInternal;
        chiFenceParams.size                 = sizeof(CHIFENCECREATEPARAMS);
        CamX::OsUtils::SNPrintF(chiFenceName, sizeof(chiFenceName), "ChiInternalFence_EISV3");
        chiFenceParams.pName                = chiFenceName;
        CHIFENCEHANDLE hFence               = NULL;
        result                              = g_ChiNodeInterface.pCreateFence(m_hChiSession, &chiFenceParams, &hFence);
        if (CDKResultSuccess == result)
        {

            dataRequest.requestType         = ChiFetchData;

            ncsRequest.size                 = sizeof(ChiNCSDataRequest);
            ncsRequest.numSamples           = 0;
            ncsRequest.windowRequest.tStart = QtimerNanoToQtimerTicks(MicroToNano(gyroInterval.first_ts));
            ncsRequest.windowRequest.tEnd   = QtimerNanoToQtimerTicks(MicroToNano(gyroInterval.last_ts));
            ncsRequest.hChiFence            = hFence;

            LOG_VERBOSE(CamxLogGroupChi, "Getting gyro interval t1 %" PRIu64 " t2 %" PRIu64 " fence %p",
                      ncsRequest.windowRequest.tStart,
                      ncsRequest.windowRequest.tEnd,
                      ncsRequest.hChiFence);

            dataRequest.hRequestPd = &ncsRequest;

            ///< Request Data in Async mode and go into DRQ
            pProcessRequestInfo->pDependency->pChiFences[0]  = hFence;
            pProcessRequestInfo->pDependency->chiFenceCount  = 1;
            g_ChiNodeInterface.pGetData(m_hChiSession, GetDataSource(), &dataRequest, NULL);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Create fence Failed on creation %p", hFence);
        }
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Cannot get Gyro interval. Not setting dependency");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::GetChromatixTuningHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV3Node::GetChromatixTuningHandle(
    UINT32 sensorIndex,
    UINT32 sensorMode)
{
    TuningMode    tuningMode[2];
    VOID*         pData             = NULL;
    UINT32        tuningModeIndex   = 0;
    CDKResult     result            = CDKResultSuccess;

    if (NULL == m_perSensorData[sensorIndex].pTuningManager)
    {
        CHIDATASOURCE chiDataSource;
        chiDataSource.dataSourceType  = ChiTuningManager;
        chiDataSource.pHandle         = NULL;

        CHIDATAREQUEST TuningMangerForCameraId;
        CHIDATAREQUEST* pTuningMangerForCameraId = NULL;

        TuningMangerForCameraId.index = m_perSensorData[sensorIndex].cameraConfig.cameraId;
        if (1 < m_numOfLinkedCameras)
        {
            pTuningMangerForCameraId = &TuningMangerForCameraId;
        }

        m_perSensorData[sensorIndex].pTuningManager = static_cast<TuningSetManager*>(g_ChiNodeInterface.pGetData(m_hChiSession,
                                                                                     &chiDataSource,
                                                                                     pTuningMangerForCameraId,
                                                                                     NULL));
    }

    if (NULL != m_perSensorData[sensorIndex].pTuningManager)
    {
        TuningSetManager*                pTuningManager = m_perSensorData[sensorIndex].pTuningManager;
        eis_1_2_0::chromatix_eis12Type** ppEISChromatix = &m_perSensorData[sensorIndex].pEISChromatix;

        tuningMode[0].mode      = ModeType::Default;
        tuningMode[0].subMode   = { 0 };

        tuningModeIndex++;
        if (InvalidSensorMode != sensorMode)
        {
            tuningMode[tuningModeIndex].mode            = ModeType::Sensor;
            tuningMode[tuningModeIndex].subMode.value   = static_cast<UINT16>(sensorMode);
            tuningModeIndex++;
        }

        if (CDKResultSuccess == result)
        {
            *ppEISChromatix = pTuningManager->GetModule_eis12_sw(tuningMode, tuningModeIndex);

            if (NULL == m_perSensorData[sensorIndex].pEISChromatix)
            {
                result = CDKResultEFailed;
                LOG_ERROR(CamxLogGroupChi, "Failed to get EIS 1_2_0 chromatix handle");
            }
        }

        if (CDKResultSuccess == result)
        {
            switch (m_ICAVersion)
            {
                case ChiICAVersion::ChiICA20:
                    m_perSensorData[sensorIndex].pICAChromatix = pTuningManager->GetModule_ica20_ipe_module1(
                                                                 tuningMode, tuningModeIndex);
                    if (NULL == m_perSensorData[sensorIndex].pICAChromatix)
                    {
                        result = CDKResultEFailed;
                        LOG_ERROR(CamxLogGroupChi, "Failed to get ICA20 chromatix handle");
                    }
                    break;
                case ChiICAVersion::ChiICA30:
                    m_perSensorData[sensorIndex].pICAChromatix = pTuningManager->GetModule_ica30_ipe_module1(
                                                                 tuningMode, tuningModeIndex);
                    if (NULL == m_perSensorData[sensorIndex].pICAChromatix)
                    {
                        result = CDKResultEFailed;
                        LOG_ERROR(CamxLogGroupChi, "Failed to get ICA30 chromatix handle");
                    }
                    break;
                default:
                    LOG_ERROR(CamxLogGroupChi, "LDC grid not supported");
                    break;
            }
        }
    }
    else
    {
        result = CDKResultEFailed;
        LOG_ERROR(CamxLogGroupChi, "Failed to get chromatix handle");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::GetChromatixData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV3Node::GetChromatixData(
    is_init_data_sensor* pEISInitData,
    UINT32               sensorIndex)
{
    CDKResult                                                                   result              = CDKResultSuccess;
    eis_1_2_0::chromatix_eis12Type*                                             pEISChromatix       = NULL;
    eis_1_2_0::chromatix_eis12_reserveType::topStruct*                          pTopStruct          = NULL;
    eis_1_2_0::chromatix_eis12_reserveType::generalStruct*                      pGeneralStruct      = NULL;
    eis_1_2_0::chromatix_eis12_reserveType::timingStruct*                       pTimingStruct       = NULL;
    eis_1_2_0::chromatix_eis12_reserveType::blur_maskingStruct*                 pBlurMaskingStruct  = NULL;
    eis_1_2_0::chromatix_eis12_reserveType::lens_distortion_correctionStruct*   pLdcStruct          = NULL;

    if ((NULL != m_perSensorData[sensorIndex].pEISChromatix) && (NULL != pEISInitData))
    {
        pEISChromatix       = m_perSensorData[sensorIndex].pEISChromatix;
        pTopStruct          = &pEISChromatix->chromatix_eis12_reserve.top;
        pGeneralStruct      = &pEISChromatix->chromatix_eis12_reserve.general;
        pTimingStruct       = &pEISChromatix->chromatix_eis12_reserve.timing;
        pBlurMaskingStruct  = &pEISChromatix->chromatix_eis12_reserve.blur_masking;
        pLdcStruct          = &pEISChromatix->chromatix_eis12_reserve.lens_distortion_correction;

        // Fill top tuning section
        pEISInitData->is_chromatix_info.top.requested_total_margins_y_fhd_30  = pTopStruct->requested_total_margins_y_fhd_30;
        pEISInitData->is_chromatix_info.top.requested_total_margins_y_fhd_60  = pTopStruct->requested_total_margins_y_fhd_60;
        pEISInitData->is_chromatix_info.top.requested_total_margins_y_fhd_120 = pTopStruct->requested_total_margins_y_fhd_120;
        pEISInitData->is_chromatix_info.top.requested_total_margins_y_4k_30   = pTopStruct->requested_total_margins_y_4k_30;
        pEISInitData->is_chromatix_info.top.requested_total_margins_y_4k_60   = pTopStruct->requested_total_margins_y_4k_60;
        pEISInitData->is_chromatix_info.top.requested_total_margins_y_4k_120  = pTopStruct->requested_total_margins_y_4k_120;
        pEISInitData->is_chromatix_info.top.requested_total_margins_y_8k_30   = pTopStruct->requested_total_margins_y_8k_30;
        pEISInitData->is_chromatix_info.top.requested_total_margins_y_8k_60   = pTopStruct->requested_total_margins_y_8k_60;
        pEISInitData->is_chromatix_info.top.requested_total_margins_y_8k_120  = pTopStruct->requested_total_margins_y_8k_120;

        pEISInitData->is_chromatix_info.top.future_buffer_size_fhd_30   = pTopStruct->future_buffer_size_fhd_30;
        pEISInitData->is_chromatix_info.top.future_buffer_size_fhd_60   = pTopStruct->future_buffer_size_fhd_60;
        pEISInitData->is_chromatix_info.top.future_buffer_size_fhd_120  = pTopStruct->future_buffer_size_fhd_120;
        pEISInitData->is_chromatix_info.top.future_buffer_size_4k_30    = pTopStruct->future_buffer_size_4k_30;
        pEISInitData->is_chromatix_info.top.future_buffer_size_4k_60    = pTopStruct->future_buffer_size_4k_60;
        pEISInitData->is_chromatix_info.top.future_buffer_size_4k_120   = pTopStruct->future_buffer_size_4k_120;
        pEISInitData->is_chromatix_info.top.future_buffer_size_8k_30    = pTopStruct->future_buffer_size_8k_30;
        pEISInitData->is_chromatix_info.top.future_buffer_size_8k_60    = pTopStruct->future_buffer_size_8k_60;
        pEISInitData->is_chromatix_info.top.future_buffer_size_8k_120   = pTopStruct->future_buffer_size_8k_120;

        // TODO: Get min total margin from top section once chromatix 1_2_0_is updated
        pEISInitData->is_chromatix_info.top.minimal_total_margin        = pTopStruct->minimal_total_margin;

        // TODO: Get sensors freq from top section once chromatix 1_2_0_is updated
        pEISInitData->is_chromatix_info.top.gyro_frequency              = (pTopStruct->gyro_frequency == 0) ?
                                                                          static_cast<uint32_t>(GyroSamplingRate) :
                                                                          pTopStruct->gyro_frequency;
        pEISInitData->is_chromatix_info.top.acc_frequency               = pTopStruct->acc_frequency;
        pEISInitData->is_chromatix_info.top.mag_frequency               = pTopStruct->mag_frequency;


        // Fill General tuning section
        pEISInitData->is_chromatix_info.general.focal_length            = pGeneralStruct->focal_length;
        pEISInitData->is_chromatix_info.general.gyro_noise_floor        = pGeneralStruct->gyro_noise_floor;
        // TODO: acc_noise_floor needs to be added to EIS chromatix 1_2_0
        pEISInitData->is_chromatix_info.general.acc_noise_floor         = 0;
        // TODO: mag_noise_floor needs to be added to EIS chromatix 1_2_0
        pEISInitData->is_chromatix_info.general.mag_noise_floor         = 0;
        pEISInitData->is_chromatix_info.general.output_grid_precision   = pGeneralStruct->output_grid_precision;
        pEISInitData->is_chromatix_info.general.res_param_1             = pGeneralStruct->res_param_1;
        pEISInitData->is_chromatix_info.general.res_param_2             = pGeneralStruct->res_param_2;
        pEISInitData->is_chromatix_info.general.res_param_3             = pGeneralStruct->res_param_3;
        pEISInitData->is_chromatix_info.general.res_param_4             = pGeneralStruct->res_param_4;
        pEISInitData->is_chromatix_info.general.res_param_5             = pGeneralStruct->res_param_5;
        pEISInitData->is_chromatix_info.general.res_param_6             = pGeneralStruct->res_param_6;
        pEISInitData->is_chromatix_info.general.res_param_7             = pGeneralStruct->res_param_7;
        pEISInitData->is_chromatix_info.general.res_param_8             = pGeneralStruct->res_param_8;
        pEISInitData->is_chromatix_info.general.res_param_9             = pGeneralStruct->res_param_9;
        pEISInitData->is_chromatix_info.general.res_param_10            = pGeneralStruct->res_param_10;

        for (UINT32 i = 0; i < CAMX_ARRAY_SIZE(pEISInitData->is_chromatix_info.general.res_lut_param_1); i++)
        {
            pEISInitData->is_chromatix_info.general.res_lut_param_1[i] =
                pGeneralStruct->res_lut_param_1_tab.res_lut_param_1[i];
        }

        for (UINT32 i = 0; i < CAMX_ARRAY_SIZE(pEISInitData->is_chromatix_info.general.res_lut_param_2); i++)
        {
            pEISInitData->is_chromatix_info.general.res_lut_param_2[i] =
                pGeneralStruct->res_lut_param_2_tab.res_lut_param_2[i];
        }

        for (UINT32 i = 0; i < CAMX_ARRAY_SIZE(pEISInitData->is_chromatix_info.general.res_lut_param_3); i++)
        {
            pEISInitData->is_chromatix_info.general.res_lut_param_3[i] =
                pGeneralStruct->res_lut_param_3_tab.res_lut_param_3[i];
        }

        // Fill timing tuning section
        pEISInitData->is_chromatix_info.timing.s3d_offset                   = pTimingStruct->s3d_offset;
        pEISInitData->is_chromatix_info.timing.ois_offset                   = pTimingStruct->ois_offset;
        pEISInitData->is_chromatix_info.timing.acc_offset                   = pTimingStruct->acc_offset;
        pEISInitData->is_chromatix_info.timing.mag_offset                   = pTimingStruct->mag_offset;

        // Fill blur masking tuning section
        pEISInitData->is_chromatix_info.blur_masking.enable                 = !!pBlurMaskingStruct->enable;
        pEISInitData->is_chromatix_info.blur_masking.min_strength           = pBlurMaskingStruct->min_strength;
        pEISInitData->is_chromatix_info.blur_masking.exposure_time_th       = pBlurMaskingStruct->exposure_time_th;
        pEISInitData->is_chromatix_info.blur_masking.start_decrease_at_blur = pBlurMaskingStruct->start_decrease_at_blur;
        pEISInitData->is_chromatix_info.blur_masking.end_decrease_at_blur   = pBlurMaskingStruct->end_decrease_at_blur;
        pEISInitData->is_chromatix_info.blur_masking.pan_min_threshold      = pBlurMaskingStruct->pan_min_threshold;
        pEISInitData->is_chromatix_info.blur_masking.pan_max_threshold      = pBlurMaskingStruct->pan_max_threshold;
        pEISInitData->is_chromatix_info.blur_masking.blur_masking_res1      = pBlurMaskingStruct->blur_masking_res1;
        pEISInitData->is_chromatix_info.blur_masking.blur_masking_res2      = pBlurMaskingStruct->blur_masking_res2;
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "EIS Chromatix ptr %p for cameraId %d, EIS Init Data pointer %p",
                  m_perSensorData[sensorIndex].pEISChromatix,
                  m_perSensorData[sensorIndex].cameraConfig.cameraId,
                  pEISInitData);
        result = CDKResultEInvalidPointer;
    }

    if ((CDKResultSuccess == result) && (TRUE == m_bIsLDCGridEnabled))
    {
        eis_lens_distortion_correction* pEISLDCInitData = &pEISInitData->is_chromatix_info.lens_distortion_correction;

        pEISLDCInitData->ldc_grid_source = pLdcStruct->ldc_grid_source;
        pEISLDCInitData->ldc_grid_enable = pLdcStruct->ldc_grid_enable;

        if (0 == pLdcStruct->ldc_grid_source)
        {
            GetLDCGridFromEISChromatix(pEISLDCInitData, pLdcStruct);
        }
        else
        {
            switch (m_ICAVersion)
            {
            case ChiICAVersion::ChiICA20:
                GetLDCGridFromICA20Chromatix(pEISInitData, sensorIndex);
                break;
            case ChiICAVersion::ChiICA30:
                GetLDCGridFromICA30Chromatix(pEISInitData, sensorIndex);
                break;
            default:
                LOG_ERROR(CamxLogGroupChi, "LDC not supported");
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV2Node::GetLDCGridFromICA20Chromatix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV3Node::GetLDCGridFromICA20Chromatix(
    is_init_data_sensor* pEISInitData,
    UINT32               sensorIndex)
{
    CDKResult result                                = CDKResultSuccess;
    ica_2_0_0::chromatix_ica20Type* pICA20Chromatix = reinterpret_cast<ica_2_0_0::chromatix_ica20Type*>
                                                      (m_perSensorData[sensorIndex].pICAChromatix);
    ica_2_0_0::ica20_rgn_dataType*  pICA20RgnData   = NULL;

    if (ChiICAVersion::ChiICA20 == m_ICAVersion)
    {
        if ((NULL != pICA20Chromatix)               &&
            (NULL != pEISInitData)                  &&
            (NULL != m_pLDCIn2OutGrid[sensorIndex]) &&
            (NULL != m_pLDCOut2InGrid[sensorIndex]))
        {
            if ((TRUE == !!pICA20Chromatix->chromatix_ica20_reserve.distorted_input_to_undistorted_ldc_grid_valid) &&
                (TRUE == !!pICA20Chromatix->chromatix_ica20_reserve.undistorted_to_lens_distorted_output_ld_grid_valid))
            {
                pICA20RgnData = &pICA20Chromatix->chromatix_ica20_core.mod_ica20_lens_posn_data[0].
                                lens_posn_data.mod_ica20_lens_zoom_data[0].
                                lens_zoom_data.mod_ica20_aec_data[0].
                                ica20_rgn_data;

                m_LDCIn2OutWarpGrid[sensorIndex].enable          = TRUE;
                m_LDCIn2OutWarpGrid[sensorIndex].extrapolateType = EXTRAPOLATION_TYPE_EXTRA_POINT_ALONG_PERIMETER;
                m_LDCIn2OutWarpGrid[sensorIndex].gridExtrapolate = NULL;
                m_LDCIn2OutWarpGrid[sensorIndex].numRows         = ICA20GridTransformHeight;
                m_LDCIn2OutWarpGrid[sensorIndex].numColumns      = ICA20GridTransformWidth;
                m_LDCIn2OutWarpGrid[sensorIndex].grid            = m_pLDCIn2OutGrid[sensorIndex];
                m_LDCIn2OutWarpGrid[sensorIndex].
                    transformDefinedOn.widthPixels               = IcaVirtualDomainWidth  * IcaVirtualDomainQuantizationV20;
                m_LDCIn2OutWarpGrid[sensorIndex].
                    transformDefinedOn.heightLines               = IcaVirtualDomainHeight * IcaVirtualDomainQuantizationV20;

                m_LDCOut2InWarpGrid[sensorIndex].enable          = TRUE;
                m_LDCOut2InWarpGrid[sensorIndex].extrapolateType = EXTRAPOLATION_TYPE_EXTRA_POINT_ALONG_PERIMETER;
                m_LDCOut2InWarpGrid[sensorIndex].gridExtrapolate = NULL;
                m_LDCOut2InWarpGrid[sensorIndex].numRows         = ICA20GridTransformHeight;
                m_LDCOut2InWarpGrid[sensorIndex].numColumns      = ICA20GridTransformWidth;
                m_LDCOut2InWarpGrid[sensorIndex].grid            = m_pLDCOut2InGrid[sensorIndex];
                m_LDCOut2InWarpGrid[sensorIndex].
                    transformDefinedOn.widthPixels               = IcaVirtualDomainWidth  * IcaVirtualDomainQuantizationV20;
                m_LDCOut2InWarpGrid[sensorIndex].
                    transformDefinedOn.heightLines               = IcaVirtualDomainHeight * IcaVirtualDomainQuantizationV20;


                for (UINT32 i = 0; i < (ICA20GridTransformWidth * ICA20GridTransformHeight); i++)
                {
                    // Fill m_pLDCIn2OutGrid from chromatix
                    m_pLDCIn2OutGrid[sensorIndex][i].x = pICA20RgnData->distorted_input_to_undistorted_ldc_grid_x_tab.
                                                         distorted_input_to_undistorted_ldc_grid_x[i];
                    m_pLDCIn2OutGrid[sensorIndex][i].y = pICA20RgnData->distorted_input_to_undistorted_ldc_grid_y_tab.
                                                         distorted_input_to_undistorted_ldc_grid_y[i];

                    // Fill m_pLDCOut2InGrid from chromatix
                    m_pLDCOut2InGrid[sensorIndex][i].x = pICA20RgnData->ctc_grid_x_tab.ctc_grid_x[i];
                    m_pLDCOut2InGrid[sensorIndex][i].y = pICA20RgnData->ctc_grid_y_tab.ctc_grid_y[i];
                }

                pEISInitData->ldc_in2out        = &m_LDCIn2OutWarpGrid[sensorIndex];
                pEISInitData->ldc_out2in        = &m_LDCOut2InWarpGrid[sensorIndex];
                pEISInitData->ldc_calib_domain  = DOMAIN_IFE_OUTPUT_DZX1;
            }
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi,
                      "For camId %d ICA Chromatix ptr %p, EIS Init Data pointer %p, m_pLDCIn2OutGrid %p, m_pLDCOut2InGrid %p",
                      m_perSensorData[sensorIndex].cameraConfig.cameraId,
                      pICA20Chromatix,
                      pEISInitData,
                      m_pLDCIn2OutGrid[sensorIndex],
                      m_pLDCOut2InGrid[sensorIndex]);
            result = CDKResultEInvalidPointer;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV2Node::GetLDCGridFromICA30Chromatix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV3Node::GetLDCGridFromICA30Chromatix(
    is_init_data_sensor* pEISInitData,
    UINT32               sensorIndex)
{
    CDKResult result = CDKResultSuccess;
    ica_3_0_0::chromatix_ica30Type* pICA30Chromatix = reinterpret_cast<ica_3_0_0::chromatix_ica30Type*>
                                                      (m_perSensorData[sensorIndex].pICAChromatix);
    ica_3_0_0::ica30_rgn_dataType*  pICA30RgnData   = NULL;

    if (ChiICAVersion::ChiICA30 == m_ICAVersion)
    {
        if ((NULL != pICA30Chromatix)               &&
            (NULL != pEISInitData)                  &&
            (NULL != m_pLDCIn2OutGrid[sensorIndex]) &&
            (NULL != m_pLDCOut2InGrid[sensorIndex]))
        {
            if ((TRUE == !!pICA30Chromatix->chromatix_ica30_reserve.ld_i2u_grid_valid) &&
                (TRUE == !!pICA30Chromatix->chromatix_ica30_reserve.ld_u2i_grid_valid))
            {
                pICA30RgnData = &pICA30Chromatix->chromatix_ica30_core.mod_ica30_lens_posn_data[0].
                                lens_posn_data.mod_ica30_lens_zoom_data[0].
                                lens_zoom_data.mod_ica30_aec_data[0].
                                ica30_rgn_data;

                UINT32 numGridRows    = (1 == pICA30Chromatix->chromatix_ica30_reserve.ld_i2u_grid_geometry) ?
                                        ICA30GridTransformHeight : ICA20GridTransformHeight;
                UINT32 numGridColumns = (1 == pICA30Chromatix->chromatix_ica30_reserve.ld_i2u_grid_geometry) ?
                                        ICA30GridTransformWidth : ICA20GridTransformWidth;

                m_LDCIn2OutWarpGrid[sensorIndex].enable          = TRUE;
                m_LDCIn2OutWarpGrid[sensorIndex].extrapolateType = EXTRAPOLATION_TYPE_EXTRA_POINT_ALONG_PERIMETER;
                m_LDCIn2OutWarpGrid[sensorIndex].gridExtrapolate = NULL;
                m_LDCIn2OutWarpGrid[sensorIndex].numRows         = numGridRows;
                m_LDCIn2OutWarpGrid[sensorIndex].numColumns      = numGridColumns;
                m_LDCIn2OutWarpGrid[sensorIndex].grid            = m_pLDCIn2OutGrid[sensorIndex];
                m_LDCIn2OutWarpGrid[sensorIndex].
                    transformDefinedOn.widthPixels               = IcaVirtualDomainWidth  * IcaVirtualDomainQuantizationV30;
                m_LDCIn2OutWarpGrid[sensorIndex].
                    transformDefinedOn.heightLines               = IcaVirtualDomainHeight * IcaVirtualDomainQuantizationV30;

                m_LDCOut2InWarpGrid[sensorIndex].enable          = TRUE;
                m_LDCOut2InWarpGrid[sensorIndex].extrapolateType = EXTRAPOLATION_TYPE_EXTRA_POINT_ALONG_PERIMETER;
                m_LDCOut2InWarpGrid[sensorIndex].gridExtrapolate = NULL;
                m_LDCOut2InWarpGrid[sensorIndex].numRows         = numGridRows;
                m_LDCOut2InWarpGrid[sensorIndex].numColumns      = numGridColumns;
                m_LDCOut2InWarpGrid[sensorIndex].grid            = m_pLDCOut2InGrid[sensorIndex];
                m_LDCOut2InWarpGrid[sensorIndex].
                    transformDefinedOn.widthPixels               = IcaVirtualDomainWidth  * IcaVirtualDomainQuantizationV30;
                m_LDCOut2InWarpGrid[sensorIndex].
                    transformDefinedOn.heightLines               = IcaVirtualDomainHeight * IcaVirtualDomainQuantizationV30;


                for (UINT32 i = 0; i < (numGridRows * numGridColumns); i++)
                {
                    // Fill m_pLDCIn2OutGrid from chromatix
                    m_pLDCIn2OutGrid[sensorIndex][i].x = pICA30RgnData->ld_i2u_grid_x_tab.ld_i2u_grid_x[i];
                    m_pLDCIn2OutGrid[sensorIndex][i].y = pICA30RgnData->ld_i2u_grid_y_tab.ld_i2u_grid_y[i];

                    // Fill m_pLDCOut2InGrid from chromatix
                    m_pLDCOut2InGrid[sensorIndex][i].x = pICA30RgnData->ctc_grid_x_tab.ctc_grid_x[i];
                    m_pLDCOut2InGrid[sensorIndex][i].y = pICA30RgnData->ctc_grid_y_tab.ctc_grid_y[i];
                }

                pEISInitData->ldc_in2out        = &m_LDCIn2OutWarpGrid[sensorIndex];
                pEISInitData->ldc_out2in        = &m_LDCOut2InWarpGrid[sensorIndex];
                pEISInitData->ldc_calib_domain  = static_cast<LDCCalibDomian>(pICA30RgnData->ldc_calib_domain);
            }
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi,
                      "For camId %d ICA Chromatix ptr %p, EIS Init Data pointer %p, m_pLDCIn2OutGrid %p, m_pLDCOut2InGrid %p",
                      m_perSensorData[sensorIndex].cameraConfig.cameraId,
                      pICA30Chromatix,
                      pEISInitData,
                      m_pLDCIn2OutGrid[sensorIndex],
                      m_pLDCOut2InGrid[sensorIndex]);
            result = CDKResultEInvalidPointer;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::GetLDCGridFromEISChromatix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV3Node::GetLDCGridFromEISChromatix(
    eis_lens_distortion_correction*                                             pEISLDCInitData,
    eis_1_2_0::chromatix_eis12_reserveType::lens_distortion_correctionStruct*   pEISLDCChromatix)
{
    CDKResult result = CDKResultSuccess;
    UINT32    numLDCGridPoints = 0;

    if (TRUE == !!pEISLDCChromatix->ldc_grid_enable)
    {
        pEISLDCInitData->ldc_calib_domain = pEISLDCChromatix->ldc_calib_domain;

        pEISLDCInitData->distorted_input_to_undistorted_ldc_grid_geometry =
            pEISLDCChromatix->distorted_input_to_undistorted_ldc_grid_geometry;

        numLDCGridPoints = (0 == pEISLDCChromatix->distorted_input_to_undistorted_ldc_grid_geometry) ?
                           (ICA20GridTransformWidth * ICA20GridTransformHeight) :
                           (ICA30GridTransformWidth * ICA30GridTransformHeight);

        memcpy(pEISLDCInitData->distorted_input_to_undistorted_ldc_grid_x,
               pEISLDCChromatix->distorted_input_to_undistorted_ldc_grid_x_tab.distorted_input_to_undistorted_ldc_grid_x,
               sizeof(INT32) * numLDCGridPoints);

        memcpy(pEISLDCInitData->distorted_input_to_undistorted_ldc_grid_y,
               pEISLDCChromatix->distorted_input_to_undistorted_ldc_grid_y_tab.distorted_input_to_undistorted_ldc_grid_y,
               sizeof(INT32) * numLDCGridPoints);

        pEISLDCInitData->undistorted_to_distorted_input_ldc_grid_geometry =
            pEISLDCChromatix->undistorted_to_distorted_input_ldc_grid_geometry;

        numLDCGridPoints = (0 == pEISLDCChromatix->undistorted_to_distorted_input_ldc_grid_geometry) ?
                           (ICA20GridTransformWidth * ICA20GridTransformHeight) :
                           (ICA30GridTransformWidth * ICA30GridTransformHeight);

        memcpy(pEISLDCInitData->undistorted_to_distorted_input_ldc_grid_x,
               pEISLDCChromatix->undistorted_to_distorted_input_ldc_grid_x_tab.undistorted_to_distorted_input_ldc_grid_x,
               sizeof(INT32) * numLDCGridPoints);

        memcpy(pEISLDCInitData->undistorted_to_distorted_input_ldc_grid_y,
               pEISLDCChromatix->undistorted_to_distorted_input_ldc_grid_y_tab.undistorted_to_distorted_input_ldc_grid_y,
               sizeof(INT32) * numLDCGridPoints);

        pEISLDCInitData->ldc_model_type = pEISLDCChromatix->ldc_model_type;
        memcpy(pEISLDCInitData->model_parameters, pEISLDCChromatix->model_parameters_tab.model_parameters, sizeof(FLOAT) * 32);

        pEISLDCInitData->focal_length_x             = pEISLDCChromatix->focal_length_x;
        pEISLDCInitData->focal_length_y             = pEISLDCChromatix->focal_length_y;
        pEISLDCInitData->optical_center_x           = pEISLDCChromatix->optical_center_x;
        pEISLDCInitData->optical_center_y           = pEISLDCChromatix->optical_center_y;
        pEISLDCInitData->image_size_distorted_x     = pEISLDCChromatix->image_size_distorted_x;
        pEISLDCInitData->image_size_distorted_y     = pEISLDCChromatix->image_size_distorted_y;
        pEISLDCInitData->image_size_undistorted_x   = pEISLDCChromatix->image_size_undistorted_x;
        pEISLDCInitData->image_size_undistorted_y   = pEISLDCChromatix->image_size_undistorted_y;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::FillGyroData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV3Node::FillGyroData(
    UINT64          requestId,
    gyro_data_t*    pGyroData,
    frame_times_t*  pFrameTimes,
    UINT32          sensorIndex)
{
    CHIDATAHANDLE     hNCSDataHandle  = NULL;
    CHIDATAREQUEST    gyroDataRequest;
    CHIDATAREQUEST    gyroDataIterator;
    gyro_times_t      gyroInterval    = { 0 };
    CDKResult         result          = CDKResultSuccess;
    UINT              dataSize        = 0;
    CHIDATASOURCE*    pGyroDataSource = GetDataSource();
    CHIDATASOURCE     accessorObject;
    ChiNCSDataGyro*   pNCSGyroData    = NULL;
    ChiNCSDataRequest ncsRequest      = {0, {0}, NULL, 0};

    memset(&accessorObject, 0x0, sizeof(CHIDATASOURCE));
    memset(&gyroDataRequest, 0x0, sizeof(CHIDATAREQUEST));
    memset(&gyroDataIterator, 0x0, sizeof(CHIDATAREQUEST));

    result = GetGyroInterval(requestId, sensorIndex, &gyroInterval, pFrameTimes);

    if (CDKResultSuccess == result)
    {

        gyroDataRequest.requestType     = ChiFetchData;
        ncsRequest.size                 = sizeof(ChiNCSDataRequest);
        ncsRequest.numSamples           = 0;
        ncsRequest.windowRequest.tStart = QtimerNanoToQtimerTicks(
                                                                MicroToNano(gyroInterval.first_ts));
        ncsRequest.windowRequest.tEnd   = QtimerNanoToQtimerTicks(
                                                                MicroToNano(gyroInterval.last_ts));
        gyroDataRequest.hRequestPd      = &ncsRequest;

        // get data accessor handle
        hNCSDataHandle = reinterpret_cast<CHIDATAHANDLE*> (g_ChiNodeInterface.pGetData(m_hChiSession,
                                                                                       pGyroDataSource,
                                                                                       &gyroDataRequest,
                                                                                       &dataSize));
        if (GYRO_SAMPLES_BUF_SIZE < dataSize)
        {
            dataSize = GYRO_SAMPLES_BUF_SIZE;
        }

        if ((NULL != pGyroDataSource) && (NULL != hNCSDataHandle))
        {
            accessorObject.dataSourceType = pGyroDataSource->dataSourceType;
            accessorObject.pHandle        = hNCSDataHandle;

            gyroDataIterator.requestType = ChiIterateData;
            // iterate over the samples
            for (UINT i = 0; i < dataSize; i++)
            {
                gyroDataIterator.index = i;
                pNCSGyroData = NULL;

                pNCSGyroData = static_cast<ChiNCSDataGyro*>(g_ChiNodeInterface.pGetData(m_hChiSession,
                                                                                     &accessorObject,
                                                                                     &gyroDataIterator,
                                                                                     NULL));
                if (NULL != pNCSGyroData)
                {
                    pGyroData->samples[i].data[0] = pNCSGyroData->x;
                    pGyroData->samples[i].data[1] = pNCSGyroData->y;
                    pGyroData->samples[i].data[2] = pNCSGyroData->z;
                    pGyroData->samples[i].data[3] = 0;
                    pGyroData->samples[i].ts      = NanoToMicro(QtimerTicksToQtimerNano(
                                                                pNCSGyroData->timestamp));
                    LOG_VERBOSE(CamxLogGroupChi,
                                "x %f y %f z %f ts %" PRIu64 "(tick %" PRIu64 ")",
                                pGyroData->samples[i].data[0],
                                pGyroData->samples[i].data[1],
                                pGyroData->samples[i].data[2],
                                pGyroData->samples[i].ts,
                                pNCSGyroData->timestamp);
                }
                else
                {
                    LOG_ERROR(CamxLogGroupChi, "Unable to get gyro sample from iterator");
                }
            }
            pGyroData->num_elements = dataSize;
            g_ChiNodeInterface.pPutData(m_hChiSession, pGyroDataSource, hNCSDataHandle);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get data %" PRIu64, requestId);
            result = CDKResultEFailed;
        }
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Unable to get gyro time interval request %" PRIu64, requestId);
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::UpdateZoomWindow
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiEISV3Node::UpdateZoomWindow(
    CHIRectangle* pCropRect,
    UINT32        sensorIndex,
    UINT64        requestId)
{
    UINT32 adjustedFullWidth;
    UINT32 adjustedFullHeight;
    FLOAT  cropFactor           = 1.0f;
    FLOAT  cropFactorOffsetLeft = 1.0f;
    FLOAT  cropFactorOffsetTop  = 1.0f;
    FLOAT  applyFOVCFactor      = 0.0f;

    LOG_VERBOSE(CamxLogGroupChi,
                "Input Zoom Window [%d, %d, %d, %d] sensor size %d x %d margin %d x%d",
                pCropRect->left,
                pCropRect->top,
                pCropRect->width,
                pCropRect->height,
                m_perSensorData[sensorIndex].inputSize.width,
                m_perSensorData[sensorIndex].inputSize.height,
                m_stabilizationMargins.widthPixels,
                m_stabilizationMargins.heightLines);

    adjustedFullWidth  = m_perSensorData[sensorIndex].inputSize.width  - m_stabilizationMargins.widthPixels;
    adjustedFullHeight = m_perSensorData[sensorIndex].inputSize.height - m_stabilizationMargins.heightLines;

    cropFactor           = static_cast<FLOAT>(pCropRect->height) / m_perSensorData[sensorIndex].inputSize.height;
    cropFactorOffsetLeft = static_cast<FLOAT>(pCropRect->left)   / m_perSensorData[sensorIndex].inputSize.width;
    cropFactorOffsetTop  = static_cast<FLOAT>(pCropRect->top)    / m_perSensorData[sensorIndex].inputSize.height;

    pCropRect->width  = static_cast<INT32>(adjustedFullWidth  * cropFactor);
    pCropRect->height = static_cast<INT32>(adjustedFullHeight * cropFactor);
    pCropRect->left   = static_cast<INT32>(adjustedFullWidth  * cropFactorOffsetLeft);
    pCropRect->top    = static_cast<INT32>(adjustedFullHeight * cropFactorOffsetTop);

    if (TRUE == m_isFOVCEnabled)
    {
        if (0.0f < m_cropFactorFOVC)
        {
            applyFOVCFactor = 1 - m_cropFactorFOVC;

            // Update zoom crop based on FOVC crop factor
            INT32 adjustedFOVCWidth  = pCropRect->width;
            INT32 adjustedFOVCHeight = pCropRect->height;

            // Calculate total change in width or height
            adjustedFOVCWidth   -= static_cast<INT32>(adjustedFOVCWidth  * applyFOVCFactor);
            adjustedFOVCHeight  -= static_cast<INT32>(adjustedFOVCHeight * applyFOVCFactor);

            INT32 adjustWidthBy  = adjustedFOVCWidth  / 2;
            INT32 adjustHeightBy = adjustedFOVCHeight / 2;

            pCropRect->left      += adjustWidthBy;
            pCropRect->top       += adjustHeightBy;
            pCropRect->width     -= adjustWidthBy  * 2;
            pCropRect->height    -= adjustHeightBy * 2;
        }
    }

    LOG_VERBOSE(CamxLogGroupChi,
                "Updated Zoom Window [%d, %d, %d, %d] full %d x %d "
                "cropFactor %f, left cropFactor %f, top cropFactor %f, fovc factor applied %f request %"
                PRIu64,
                pCropRect->left,
                pCropRect->top,
                pCropRect->width,
                pCropRect->height,
                adjustedFullWidth,
                adjustedFullHeight,
                cropFactor,
                cropFactorOffsetLeft,
                cropFactorOffsetTop,
                applyFOVCFactor,
                requestId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::GetCropRectfromCropInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiEISV3Node::GetCropRectfromCropInfo(
    IFECropInfo*  cropInfo,
    CHIRectangle* cropRect)
{
    switch (m_inputPortPathType)
    {
    case EISV3PATHTYPE::FullPath:
        *cropRect = cropInfo->fullPath;
        break;
    case EISV3PATHTYPE::DS4Path:
        *cropRect = cropInfo->DS4Path;
        break;
    case EISV3PATHTYPE::DS16Path:
        *cropRect = cropInfo->DS16Path;
        break;
    case EISV3PATHTYPE::FDPath:
        *cropRect = cropInfo->FDPath;
        break;
    case EISV3PATHTYPE::DisplayFullPath:
        *cropRect = cropInfo->displayFullPath;
        break;
    case EISV3PATHTYPE::DisplayDS4Path:
        *cropRect = cropInfo->displayDS4Path;
        break;
    case EISV3PATHTYPE::DisplayDS16Path:
        *cropRect = cropInfo->displayDS16Path;
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::ExecuteAlgo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV3Node::ExecuteAlgo(
    UINT64           requestId,
    is_output_type*  pEIS3Output,
    UINT32           sensorIndex)
{
    is_input_t                eis3Input;
    VOID*                     pData                                     = NULL;
    CHIRectangle*             pIFEResidualCropRect                      = NULL;
    CHIRectangle*             pIFEAppliedCropRect                       = NULL;
    CHIRectangle              IFEResidualcropRect                       = { 0 };
    CHIRectangle              IFEAppliedcropRect                        = { 0 };
    UINT64                    timestamp                                 = 0;
    CDKResult                 result                                    = CDKResultSuccess;
    UINT32                    cameraId                                  = InvalidCameraId;
    sample_data_t             input_gyro_data_t[GYRO_SAMPLES_BUF_SIZE]  = { { { 0 } } };
    WindowRegion              ifeCrop                                   = { 0 };
    WindowRegion              ipeZoom                                   = { 0 };

    NcLibWarp                 satTransformWarp;
    NcLibPerspTransformSingle satTransformMatrix[ICAParametersPerPerspectiveTransform];
    satTransformWarp.matrices.perspMatrices = satTransformMatrix;

    memset(&eis3Input, 0, sizeof(is_input_t));

    UINT32 residualCropTagId            = g_vendorTagId.residualCropTagId;
    UINT32 appliedCropTagId             = g_vendorTagId.appliedCropTagId;
    UINT32 lensFocusDistance            = ANDROID_LENS_FOCUS_DISTANCE;
    UINT32 SATPerspectiveTransformTagId = g_vendorTagId.SATPerspectiveTransformTagId;
    UINT32 fovcFactorTagId              = g_vendorTagId.FOVCFactorTagId;

    if (FALSE == IsRealTimeNode())
    {
        residualCropTagId |= InputMetadataSectionMask;
        appliedCropTagId  |= InputMetadataSectionMask;
        lensFocusDistance |= InputMetadataSectionMask;
        fovcFactorTagId   |= InputMetadataSectionMask;
    }

    if (1 < m_numOfLinkedCameras)
    {
        cameraId          = m_perSensorData[sensorIndex].cameraConfig.cameraId;
        residualCropTagId = g_vendorTagId.chiNodeResidualCropTagId;
    }

    ///< Get fovc factor
    if (TRUE == m_isFOVCEnabled)
    {
        pData = ChiNodeUtils::GetMulticamDynamicMetaByCamId(requestId,
                                                            fovcFactorTagId,
                                                            ChiMetadataDynamic,
                                                            &g_ChiNodeInterface,
                                                            m_hChiSession,
                                                            cameraId);
        if (NULL != pData)
        {
            FOVCOutput* pFOVCOutput = reinterpret_cast<FOVCOutput*>(pData);
            if (0.0f < pFOVCOutput->fieldOfViewCompensationFactor)
            {
                m_cropFactorFOVC = pFOVCOutput->fieldOfViewCompensationFactor;
            }
        }
    }

    ///< Get Residual Crop Size
    pData = ChiNodeUtils::GetMulticamDynamicMetaByCamId(requestId,
                                                        residualCropTagId,
                                                        ChiMetadataDynamic,
                                                        &g_ChiNodeInterface,
                                                        m_hChiSession,
                                                        cameraId);
    if (NULL != pData)
    {
        pIFEResidualCropRect = &IFEResidualcropRect;

        if (1 < m_numOfLinkedCameras)
        {
            *pIFEResidualCropRect = *static_cast<CHIRectangle*>(pData);
        }
        else
        {
            IFECropInfo* pIFEResidualCropInfo = static_cast<IFECropInfo*>(pData);
            GetCropRectfromCropInfo(pIFEResidualCropInfo, pIFEResidualCropRect);
        }

        LOG_VERBOSE(CamxLogGroupChi,
                    "Before update residual Crop info %d, %d, %d, %d",
                    pIFEResidualCropRect->left,
                    pIFEResidualCropRect->top,
                    pIFEResidualCropRect->width,
                    pIFEResidualCropRect->height);

        UpdateZoomWindow(pIFEResidualCropRect, sensorIndex, requestId);

        LOG_VERBOSE(CamxLogGroupChi,
                    "After update residual Crop info %d, %d, %d, %d",
                    pIFEResidualCropRect->left,
                    pIFEResidualCropRect->top,
                    pIFEResidualCropRect->width,
                    pIFEResidualCropRect->height);
    }

    ///< Get Applied Crop Size
    pData = ChiNodeUtils::GetMulticamDynamicMetaByCamId(requestId,
                                                        appliedCropTagId,
                                                        ChiMetadataDynamic,
                                                        &g_ChiNodeInterface,
                                                        m_hChiSession,
                                                        cameraId);
    if (NULL != pData)
    {
        IFECropInfo* pIFEAppliedCropInfo = static_cast<IFECropInfo*>(pData);
        pIFEAppliedCropRect              = &IFEAppliedcropRect;
        GetCropRectfromCropInfo(pIFEAppliedCropInfo, pIFEAppliedCropRect);

        LOG_VERBOSE(CamxLogGroupChi,
                    "Applied Crop info %d, %d, %d, %d",
                    pIFEAppliedCropRect->left,
                    pIFEAppliedCropRect->top,
                    pIFEAppliedCropRect->width,
                    pIFEAppliedCropRect->height);
    }

    ///< Get Lens focus distance
    pData = ChiNodeUtils::GetMulticamDynamicMetaByCamId(requestId,
                                                        lensFocusDistance,
                                                        ChiMetadataDynamic,
                                                        &g_ChiNodeInterface,
                                                        m_hChiSession,
                                                        cameraId);
    if (NULL != pData)
    {
        eis3Input.focus_distance = *static_cast<FLOAT*>(pData);
        LOG_VERBOSE(CamxLogGroupChi, "Lens Focus distance %f", eis3Input.focus_distance);
    }

    eis3Input.frame_id              = static_cast<uint32_t>(requestId);
    eis3Input.active_sensor_idx     = sensorIndex;
    eis3Input.sat                   = NULL;

    ifeCrop.fullWidth    = m_perSensorData[sensorIndex].sensorDimension.width;
    ifeCrop.fullHeight   = m_perSensorData[sensorIndex].sensorDimension.height;
    ipeZoom.fullWidth    = m_perSensorData[sensorIndex].inputSize.width  - m_stabilizationMargins.widthPixels;
    ipeZoom.fullHeight   = m_perSensorData[sensorIndex].inputSize.height - m_stabilizationMargins.heightLines;


    if (NULL != pIFEAppliedCropRect)
    {
        ifeCrop.windowWidth     = pIFEAppliedCropRect->width;
        ifeCrop.windowHeight    = pIFEAppliedCropRect->height;
        ifeCrop.windowLeft      = pIFEAppliedCropRect->left;
        ifeCrop.windowTop       = pIFEAppliedCropRect->top;
    }
    else
    {
        ifeCrop.windowWidth  = m_perSensorData[sensorIndex].sensorDimension.width;
        ifeCrop.windowHeight = m_perSensorData[sensorIndex].sensorDimension.height;
        ifeCrop.windowLeft   = 0;
        ifeCrop.windowTop    = 0;
    }

    if (NULL != pIFEResidualCropRect)
    {
        ipeZoom.windowWidth     = pIFEResidualCropRect->width;
        ipeZoom.windowHeight    = pIFEResidualCropRect->height;
        ipeZoom.windowLeft      = pIFEResidualCropRect->left;
        ipeZoom.windowTop       = pIFEResidualCropRect->top;
    }
    else
    {
        ipeZoom.windowWidth  = ipeZoom.fullWidth;
        ipeZoom.windowHeight = ipeZoom.fullHeight;
        ipeZoom.windowLeft   = 0;
        ipeZoom.windowTop    = 0;
    }

    //< Get Gyro Data
    eis3Input.gyro_data.samples      = &input_gyro_data_t[0];
    eis3Input.gyro_data.num_elements = 0;
    result                           = FillGyroData(requestId, &eis3Input.gyro_data, &eis3Input.frame_times, sensorIndex);
    //TODO: Fill accelerometer and magnetometer data when available

    if (CDKResultSuccess == result)
    {
        int32_t tmpResult = m_eisUtilsConvertToWindowRegions(&ifeCrop,
                                                             &ipeZoom,
                                                             m_stabilizationCropRatioX[sensorIndex],
                                                             m_stabilizationCropRatioY[sensorIndex],
                                                             m_perSensorData[sensorIndex].inputSize.width,
                                                             m_perSensorData[sensorIndex].inputSize.height,
                                                             &eis3Input.window_regions);

        if (IS_RET_SUCCESS != tmpResult)
        {
            LOG_ERROR(CamxLogGroupChi, "eis_utility_convert_to_window_regions failed");
            result = CDKResultEFailed;
        }
        else
        {
            LOG_INFO(CamxLogGroupChi,
                     "IFE crop [%d %d, %d %d %d %d], IPE zoom [%d %d, %d %d %d %d], window_Region: eis_pre_crop_vIN "
                     "[%f %f, %f %f %f %f], eis_pre_crop_vOUT [%f %f, %f %f %f %f], output_crop_fov [%f %f, %f %f %f %f]",
                     ifeCrop.fullWidth, ifeCrop.fullHeight, ifeCrop.windowLeft,
                     ifeCrop.windowTop, ifeCrop.windowWidth, ifeCrop.windowHeight,
                     ipeZoom.fullWidth, ipeZoom.fullHeight, ipeZoom.windowLeft,
                     ipeZoom.windowTop, ipeZoom.windowWidth, ipeZoom.windowHeight,
                     eis3Input.window_regions.eis_pre_crop_vIN.fullWidth,
                     eis3Input.window_regions.eis_pre_crop_vIN.fullHeight,
                     eis3Input.window_regions.eis_pre_crop_vIN.windowLeft,
                     eis3Input.window_regions.eis_pre_crop_vIN.windowTop,
                     eis3Input.window_regions.eis_pre_crop_vIN.windowWidth,
                     eis3Input.window_regions.eis_pre_crop_vIN.windowHeight,
                     eis3Input.window_regions.eis_pre_crop_vOUT.fullWidth,
                     eis3Input.window_regions.eis_pre_crop_vOUT.fullHeight,
                     eis3Input.window_regions.eis_pre_crop_vOUT.windowLeft,
                     eis3Input.window_regions.eis_pre_crop_vOUT.windowTop,
                     eis3Input.window_regions.eis_pre_crop_vOUT.windowWidth,
                     eis3Input.window_regions.eis_pre_crop_vOUT.windowHeight,
                     eis3Input.window_regions.output_crop_fov.fullWidth,
                     eis3Input.window_regions.output_crop_fov.fullHeight,
                     eis3Input.window_regions.output_crop_fov.windowLeft,
                     eis3Input.window_regions.output_crop_fov.windowTop,
                     eis3Input.window_regions.output_crop_fov.windowWidth,
                     eis3Input.window_regions.output_crop_fov.windowHeight);
        }
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Cannot get Gyro Data. Not executing EISv2 algo for %" PRIu64, requestId);
        result = CDKResultEFailed;
    }

    if ((CDKResultSuccess == result) || (1 < m_numOfLinkedCameras))
    {
        pData = ChiNodeUtils::GetPSMetaData(requestId,
                                            SATPerspectiveTransformTagId,
                                            FullPath,
                                            &g_ChiNodeInterface,
                                            m_hChiSession);

        if (NULL != pData)
        {
            IPEICAPerspectiveTransform* pSATPerspectiveTransform     = static_cast<IPEICAPerspectiveTransform*>(pData);
            satTransformWarp.direction                               = OUT_2_IN;
            satTransformWarp.matrices.enable                         = pSATPerspectiveTransform->perspectiveTransformEnable;
            satTransformWarp.matrices.numColumns                     = pSATPerspectiveTransform->perspetiveGeometryNumColumns;
            satTransformWarp.matrices.numRows                        = pSATPerspectiveTransform->perspectiveGeometryNumRows;
            satTransformWarp.matrices.transformDefinedOn.widthPixels = pSATPerspectiveTransform->transformDefinedOnWidth;
            satTransformWarp.matrices.transformDefinedOn.heightLines = pSATPerspectiveTransform->transformDefinedOnHeight;
            satTransformWarp.matrices.confidence                     = pSATPerspectiveTransform->perspectiveConfidence;
            satTransformWarp.matrices.centerType                     = CENTERED;
            memcpy(&satTransformWarp.matrices.perspMatrices[0].T[0], &pSATPerspectiveTransform->perspectiveTransformArray,
                   sizeof(NcLibPerspTransformSingle));

            satTransformWarp.grid.enable = FALSE;
            eis3Input.sat                = &satTransformWarp;
        }
    }

    if (CDKResultSuccess == result)
    {
        int32_t isResult = m_eis3Process(m_phEIS3Handle, &eis3Input, pEIS3Output);

        ///< The first buffer_delay frames are supposed to result in FRAME_NOT_PROCESSES. No video shuld be produced.
        if ((IS_RET_FRAME_NOT_PROCESSES == isResult) && (requestId < m_lookahead))
        {
            LOG_VERBOSE(CamxLogGroupChi, "EISv3 request id %" PRIu64 " < Frame delay %d", requestId, m_lookahead);
            result = CDKResultSuccess;
        }
        else if ((IS_RET_SUCCESS != isResult) && (IS_RET_FRAME_NOT_PROCESSES != isResult))
        {
            LOG_ERROR(CamxLogGroupChi, "EISv3 algo execution Failed for request %" PRIu64 ", rc 0x%x", requestId, isResult);
            result = CDKResultEFailed;
        }

        if (IS_OIS_MODE_INVALID != m_overrideOisMode)
        {
            pEIS3Output->ois_mode = m_overrideOisMode;
        }

        if (NULL != m_pEisUtilsLogContext && TRUE == m_isRecording)
        {
            is_utils_log_write_data data;

            data.is_input  = &eis3Input;
            data.is_output = pEIS3Output;
            data.buffer    = NULL;

            if (IS_RET_SUCCESS != m_eisUtilsLogWrite(m_pEisUtilsLogContext, &data))
            {
                LOG_ERROR(CamxLogGroupChi, "m_eisUtilsLogWrite() failed");
                result = CDKResultEFailed;
            }

            if (false != m_isEnabledDumpForceFlush)
            {
                if (IS_RET_SUCCESS != m_eisUtilsLogFlush(m_pEisUtilsLogContext))
                {
                    LOG_ERROR(CamxLogGroupChi, "m_eisUtilsLogFlush() failed");
                    result = CDKResultEFailed;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::IsEISv3Disabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiEISV3Node::IsEISv3Disabled(
    UINT64 requestId,
    INT64  offset)
{
    VOID* pData                    = NULL;
    BOOL eisDisabledFromApp        = FALSE;
    BOOL eisDisabledNoVideoRequest = FALSE;
    camera_metadata_enum_android_control_video_stabilization_mode eisMode;

    LOG_VERBOSE(CamxLogGroupChi, "Check if Eisv3 Disabled for req %" PRIu64 " offset %" PRId64, requestId, offset);

    requestId = ((static_cast<INT64>(requestId) + offset) <= 0) ? 1 : (static_cast<INT64>(requestId) + offset);
    pData     = ChiNodeUtils::GetMetaData(requestId,
                                          ANDROID_CONTROL_VIDEO_STABILIZATION_MODE | InputMetadataSectionMask,
                                          ChiMetadataDynamic,
                                          &g_ChiNodeInterface,
                                          m_hChiSession);
    if (NULL != pData)
    {
        eisMode = *(static_cast<camera_metadata_enum_android_control_video_stabilization_mode *>(pData));
        if (eisMode == ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF)
        {
            eisDisabledFromApp = TRUE;
            LOG_VERBOSE(CamxLogGroupChi, "Eisv3 Disabled from App %d", eisDisabledFromApp);
        }
    }

    pData = ChiNodeUtils::GetMetaData(requestId,
                                      g_vendorTagId.requestHasVideoBufferTagId,
                                      ChiMetadataDynamic,
                                      &g_ChiNodeInterface,
                                      m_hChiSession);

    if (NULL != pData)
    {
        UINT8 requestHasVideoBuffer = *(static_cast<UINT8*>(pData));
        LOG_VERBOSE(CamxLogGroupChi, "Eisv3 request %" PRIu64 " has video buffer %d", requestId, requestHasVideoBuffer);
        if (0 == requestHasVideoBuffer)
        {
            eisDisabledNoVideoRequest = TRUE;
        }
    }

    return (eisDisabledFromApp || eisDisabledNoVideoRequest);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::GetCaptureIntent
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 ChiEISV3Node::GetCaptureIntent(
    UINT64 requestId,
    INT64  offset)
{
    VOID* pData      = NULL;
    camera_metadata_enum_android_control_capture_intent_t captureIntent = ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM;

    requestId = ((static_cast<INT64>(requestId) + offset) <= 0) ? 1 : (static_cast<INT64>(requestId) + offset);
    pData     = ChiNodeUtils::GetMetaData(requestId, ANDROID_CONTROL_CAPTURE_INTENT | InputMetadataSectionMask,
                                          ChiMetadataDynamic, &g_ChiNodeInterface, m_hChiSession);
    if (NULL != pData)
    {
        captureIntent = *(static_cast<camera_metadata_enum_android_control_capture_intent_t *>(pData));
        LOG_VERBOSE(CamxLogGroupChi, "Eisv3 captureIntent %d", captureIntent);
    }
    return captureIntent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::ProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV3Node::ProcessRequest(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo)
{
    CDKResult          result               = CDKResultSuccess;
    BOOL               hasDependencies      = TRUE;
    INT32              sequenceNumber       = pProcessRequestInfo->pDependency->processSequenceId;
    CHIDEPENDENCYINFO* pDependencyInfo      = pProcessRequestInfo->pDependency;
    UINT16             depCount             = 0;
    UINT32             currentCameraId      = 0;
    UINT32             currentCameraIdIndex = 0;
    is_output_type     eis3Output;

    NcLibPerspTransformSingle perspectiveMatrix[MaxPerspMatrixSize]                               = { { { 0 } } };
    NcLibWarpGridCoord        perspectiveGrid[ICA30GridTransformWidth * ICA30GridTransformHeight] = { { 0 } };
    NcLibWarpGridCoord        gridExtrapolateICA10[NumICA10Exterpolate]                           = { { 0 } };
    NcLibPerspTransformSingle alignmentMatrixDomainUndistorted                                    = { { 0 } };
    NcLibPerspTransformSingle alignmentMatrixDomainStabilized                                     = { { 0 } };

    BOOL   isEISv3DisabledCurrentRequest    = IsEISv3Disabled(pProcessRequestInfo->frameNum, 0);
    BOOL   isEISv3DisabledDependentRequest  = FALSE;
    UINT64 endOfStreamReqId                 = 0;

    // Check if EISv3 is disabled
    if ((0 <= sequenceNumber) && (3 > sequenceNumber))
    {
        isEISv3DisabledDependentRequest = IsEISv3Disabled(pProcessRequestInfo->frameNum,
                                                         (-1 * static_cast<INT64>(m_lookahead)));
    }

    memset(&eis3Output, 0, sizeof(is_output_type));

    // if multi camera usecase get current camera id
    if (1 < m_numOfLinkedCameras)
    {
        VOID* pData = ChiNodeUtils::GetMetaData(pProcessRequestInfo->frameNum,
                                                g_vendorTagId.multiCameraIdTagId,
                                                ChiMetadataDynamic,
                                                &g_ChiNodeInterface,
                                                m_hChiSession);
        if (NULL != pData)
        {
            OutputMetadataOpticalZoom* pMetadataOpticalZoom = static_cast<OutputMetadataOpticalZoom*>(pData);
            currentCameraId         = pMetadataOpticalZoom->masterCameraId;
            currentCameraIdIndex    = GetCameraIndexFromID(currentCameraId);
            if( MaxMulticamSensors <= currentCameraIdIndex)
            {
                LOG_ERROR(CamxLogGroupChi, "camera %d, exceeds max supported linked cameras %d",
                          currentCameraIdIndex, MaxLinkedCameras);
                result = CDKResultEFailed;
            }
        }
    }

    // IMPORTANT appication requirments and assumptions when seting endOfStream:
    // 1) If application is not going to reconfigure stream on stop record, it should not set endOfStream
    // vendor tag.
    // 2) We assume that once endOfStream is set to true on a video request, all subsequent requests if any will be
    // preview only requests and will have endOfStream set to true.
    // 3) EndOfStream requests should always be followed by stream re config.

    VOID* pEndOfStreamRequestIdData = ChiNodeUtils::GetMetaData(0,
                                      g_vendorTagId.endOfStreamRequestTagId | UsecaseMetadataSectionMask,
                                      ChiMetadataDynamic,
                                      &g_ChiNodeInterface,
                                      m_hChiSession);
    if (NULL != pEndOfStreamRequestIdData)
    {
        endOfStreamReqId = *reinterpret_cast<UINT64*>(pEndOfStreamRequestIdData);
        LOG_INFO(CamxLogGroupChi, "End of stream req id %" PRIu64, endOfStreamReqId);
    }

    if (pProcessRequestInfo->frameNum <= endOfStreamReqId)
    {
        sequenceNumber = -1;
    }

    LOG_INFO(CamxLogGroupChi, "E.. request id %" PRIu64 ", seq num %d", pProcessRequestInfo->frameNum, sequenceNumber);

    // Sequence ID -1 is received for all the pending requests that are stuck in DRQ during stop recording.
    // This is needed for EISv3 as it has forward looking request dependency.
    // Publish EIS results for pending requests to IPE/GPU for warping.
    if ((CDKResultSuccess == result) && (-1 == sequenceNumber) && (NULL != m_pEndOfStreamOutputArray))
    {
        ///< Get Input Buffer size
        if (pProcessRequestInfo->inputNum > 0)
        {
            m_perSensorData[currentCameraIdIndex].inputSize.width  = pProcessRequestInfo->phInputBuffer[0]->format.width;
            m_perSensorData[currentCameraIdIndex].inputSize.height = pProcessRequestInfo->phInputBuffer[0]->format.height;
        }

        VOID* pData = ChiNodeUtils::GetMetaData(pProcessRequestInfo->frameNum,
                                                g_vendorTagId.EISV3PerspectiveGridTransformTagId,
                                                ChiMetadataDynamic,
                                                &g_ChiNodeInterface,
                                                m_hChiSession);

        if (NULL != pData)
        {
            LOG_INFO(CamxLogGroupChi, "Metadatadone already handled for request id %" PRIu64 ", seq num %d",
                      pProcessRequestInfo->frameNum, sequenceNumber);

            // metadata Done has been handled already, the wrapper does not need to handle this for us
            pProcessRequestInfo->isEarlyMetadataDone  = TRUE;
        }

        for (UINT32 i = 0; i < pProcessRequestInfo->inputNum; i++)
        {
            if (TRUE == IsBypassableNode())
            {
                pProcessRequestInfo->pBypassData[i].isBypassNeeded          = TRUE;
                pProcessRequestInfo->pBypassData[i].selectedInputPortIndex  = i;
            }
        }

        m_pEndOfStreamLock->Lock();

        if ((m_lastEIS3publishedRequest > m_lookahead) && (FALSE == m_endOfStreamOutputArrayFilled))
        {
            is_input_t                      eis3Input                = { 0 };
            UINT64                          skipAlgoFromRequestId    = m_lastEIS3publishedRequest + 1 + m_lookahead;
            EISV3PerspectiveGridTransforms* pPerspectiveAndGridTrans = NULL;

            while (m_lastEIS3publishedRequest < endOfStreamReqId)
            {
                m_lastEIS3publishedRequest++;
                eis3Input.frame_id                                           = static_cast<uint32_t>(
                                                                               m_lastEIS3publishedRequest + m_lookahead);
                eis3Output.stabilizationTransform.matrices.perspMatrices     = &perspectiveMatrix[0];
                eis3Output.stabilizationTransform.grid.grid                  = &perspectiveGrid[0];
                eis3Output.stabilizationTransform.grid.gridExtrapolate       = &gridExtrapolateICA10[0];
                eis3Output.alignment_matrix_domain_undistorted.perspMatrices = &alignmentMatrixDomainUndistorted;
                eis3Output.alignment_matrix_domain_stabilized.perspMatrices  = &alignmentMatrixDomainStabilized;

                if (m_lastEIS3publishedRequest < skipAlgoFromRequestId)
                {
                    if ((TRUE == m_gyroNCSServiceAvailable) && (FALSE == m_bIsDefaultGridTransformEnabled))
                    {
                        m_eis3Process(m_phEIS3Handle, &eis3Input, &eis3Output);

                        LOG_VERBOSE(CamxLogGroupChi, "Fill last set of requests reqid %" PRIu64 ", index %" PRIu64
                                    ", lookahead req %d",
                                    m_lastEIS3publishedRequest,
                                    m_lastEIS3publishedRequest % m_EISV3RequestQueueDepth,
                                    eis3Input.frame_id);

                        if (IS_OIS_MODE_INVALID != m_overrideOisMode)
                        {
                            eis3Output.ois_mode = m_overrideOisMode;
                        }

                        if (ChiICAVersion::ChiICA10 == m_ICAVersion)
                        {
                            // converting grid inplace
                            CDKResult conversionResult = ConvertICA20GridToICA10Grid(&eis3Output.stabilizationTransform.grid,
                                                                                     &eis3Output.stabilizationTransform.grid);

                            if (CDKResultSuccess != conversionResult)
                            {
                                LOG_ERROR(CamxLogGroupChi, "Grid conversion failed for request %" PRIu64,
                                          pProcessRequestInfo->frameNum);
                                eis3Output.stabilizationTransform.grid.enable = FALSE;
                            }
                        }

                        if (NULL != m_pEisUtilsLogContext && TRUE == m_isRecording)
                        {
                            is_utils_log_write_data data;

                            data.is_input       = &eis3Input;
                            data.is_output      = &eis3Output;
                            data.buffer         = NULL;

                            if (IS_RET_SUCCESS != m_eisUtilsLogWrite(m_pEisUtilsLogContext, &data))
                            {
                                LOG_ERROR(CamxLogGroupChi, "m_eisUtilsLogWrite() failed");
                                result = CDKResultEFailed;
                            }

                            if (false != m_isEnabledDumpForceFlush)
                            {
                                if (IS_RET_SUCCESS != m_eisUtilsLogFlush(m_pEisUtilsLogContext))
                                {
                                    LOG_ERROR(CamxLogGroupChi, "m_eisUtilsLogFlush() failed");
                                    result = CDKResultEFailed;
                                }
                            }
                        }
                    }
                    else
                    {
                        eis3Output.has_output = FALSE;
                    }

                    pPerspectiveAndGridTrans = &m_pEndOfStreamOutputArray[m_lastEIS3publishedRequest % m_EISV3RequestQueueDepth];

                    if (TRUE == eis3Output.has_output)
                    {
                        if (TRUE == eis3Output.stabilizationTransform.matrices.enable)
                        {
                            IPEICAPerspectiveTransform* pPerspectiveTrans   = &pPerspectiveAndGridTrans->perspective;
                            NcLibWarpMatrices           resultMatrices      = eis3Output.stabilizationTransform.matrices;

                            pPerspectiveTrans->perspectiveTransformEnable       = resultMatrices.enable;
                            pPerspectiveTrans->perspectiveConfidence            = resultMatrices.confidence;
                            pPerspectiveTrans->byPassAlignmentMatrixAdjustement = FALSE;
                            pPerspectiveTrans->perspetiveGeometryNumColumns     = resultMatrices.numColumns;
                            pPerspectiveTrans->perspectiveGeometryNumRows       = static_cast<UINT8>(resultMatrices.numRows);

                            //TBD: Change from active sensor to what was passed to the algo
                            pPerspectiveTrans->transformDefinedOnWidth   =
                                m_perSensorData[currentCameraIdIndex].inputSize.width;
                            pPerspectiveTrans->transformDefinedOnHeight  =
                                m_perSensorData[currentCameraIdIndex].inputSize.height;
                            pPerspectiveTrans->ReusePerspectiveTransform = 0;

                            memcpy(&pPerspectiveTrans->perspectiveTransformArray,
                                   &resultMatrices.perspMatrices[0],
                                   sizeof(NcLibPerspTransformSingle)* pPerspectiveTrans->perspectiveGeometryNumRows);
                        }
                        else
                        {
                            LOG_INFO(CamxLogGroupChi, "Algo has perspective transform disabled for req id %u",
                                     eis3Input.frame_id);
                        }

                        if (TRUE == eis3Output.stabilizationTransform.grid.enable)
                        {
                            IPEICAGridTransform* pGridTrans = &pPerspectiveAndGridTrans->grid;
                            NcLibWarpGrid resultGrid        = eis3Output.stabilizationTransform.grid;

                            pGridTrans->gridTransformEnable                   = resultGrid.enable;
                            pGridTrans->gridTransformArrayExtrapolatedCorners = (resultGrid.extrapolateType ==
                                 NcLibWarpGridExtrapolationType::EXTRAPOLATION_TYPE_FOUR_CORNERS) ? 1 : 0;

                            if (1 == pGridTrans->gridTransformArrayExtrapolatedCorners)
                            {
                                memcpy(&pGridTrans->gridTransformArrayCorners[0],
                                       resultGrid.gridExtrapolate,
                                       sizeof(pGridTrans->gridTransformArrayCorners));
                            }

                            //TBD: Change from active sensor to what was passed to the algo
                            pGridTrans->transformDefinedOnWidth  = m_perSensorData[currentCameraIdIndex].inputSize.width;
                            pGridTrans->transformDefinedOnHeight = m_perSensorData[currentCameraIdIndex].inputSize.height;
                            pGridTrans->reuseGridTransform       = 0;
                            getICAGridGeometryVersion(resultGrid.numRows, resultGrid.numColumns, &pGridTrans->geometry);
                            memcpy(&pGridTrans->gridTransformArray[0], resultGrid.grid,
                                   sizeof(ICAGridArray) * resultGrid.numRows * resultGrid.numColumns);
                        }
                        else
                        {
                            LOG_INFO(CamxLogGroupChi, "Algo has grid transform disabled for req id %u", eis3Input.frame_id);
                        }

                        if (TRUE == eis3Output.alignment_matrix_domain_stabilized.enable)
                        {
                            IPEICAPerspectiveTransform* pAlignmentMatrix = &pPerspectiveAndGridTrans->alignmentMatrix;
                            NcLibWarpMatrices           resultMatrices   = eis3Output.alignment_matrix_domain_stabilized;

                            pAlignmentMatrix->perspectiveTransformEnable        = resultMatrices.enable;
                            pAlignmentMatrix->perspectiveConfidence             = resultMatrices.confidence;
                            pAlignmentMatrix->byPassAlignmentMatrixAdjustement  = FALSE;
                            pAlignmentMatrix->perspetiveGeometryNumColumns      = resultMatrices.numColumns;
                            pAlignmentMatrix->perspectiveGeometryNumRows        = static_cast<UINT8>(resultMatrices.numRows);
                            pAlignmentMatrix->ReusePerspectiveTransform         = 0;

                            //TBD: Change from active sensor to what was passed to the algo
                            pAlignmentMatrix->transformDefinedOnWidth  =
                                m_perSensorData[currentCameraIdIndex].inputSize.width;
                            pAlignmentMatrix->transformDefinedOnHeight =
                                m_perSensorData[currentCameraIdIndex].inputSize.height;

                            memcpy(&pAlignmentMatrix->perspectiveTransformArray,
                                   &resultMatrices.perspMatrices[0],
                                   sizeof(NcLibPerspTransformSingle));

                            LOG_VERBOSE(CamxLogGroupChi, "gyro alignment rows %d, columns %d",
                                        pAlignmentMatrix->perspectiveGeometryNumRows,
                                        pAlignmentMatrix->perspetiveGeometryNumColumns);
                        }
                        else
                        {
                            LOG_INFO(CamxLogGroupChi, "Algo has gyro alignment disabled for req id %u",
                                     eis3Input.frame_id);
                        }

                        if ((FALSE == eis3Output.stabilizationTransform.matrices.enable) &&
                            (FALSE == eis3Output.stabilizationTransform.grid.enable))
                        {
                            LOG_ERROR(CamxLogGroupChi, "ERROR: Algo has no perspective or grid transform for req id %u",
                                      eis3Input.frame_id);
                        }
                    }
                    else
                    {
                        LOG_ERROR(CamxLogGroupChi, "Algo has no output for request id %u", eis3Input.frame_id);
                        FillDefaultGridTransform(&pPerspectiveAndGridTrans->perspective,
                                                 &pPerspectiveAndGridTrans->grid,
                                                 &pPerspectiveAndGridTrans->alignmentMatrix,
                                                 currentCameraIdIndex);
                    }
                }
                else
                {
                    // copy results from last but one to last frame
                    UINT64 from = (m_lastEIS3publishedRequest - 1) % m_EISV3RequestQueueDepth;
                    UINT64 to   = m_lastEIS3publishedRequest % m_EISV3RequestQueueDepth;

                    LOG_VERBOSE(CamxLogGroupChi, "Copy transform from idx %" PRIu64 " to idx %" PRIu64, from, to);
                    memcpy(&m_pEndOfStreamOutputArray[to],
                           &m_pEndOfStreamOutputArray[from],
                           sizeof(EISV3PerspectiveGridTransforms));
                }
            }

            m_endOfStreamOutputArrayFilled = TRUE;
            LOG_VERBOSE(CamxLogGroupChi, "Publish reqid %" PRIu64 ", index %" PRIu64,
                   pProcessRequestInfo->frameNum,
                   pProcessRequestInfo->frameNum % m_EISV3RequestQueueDepth);
        }
        else
        {
            isEISv3DisabledCurrentRequest = TRUE;
            LOG_VERBOSE(CamxLogGroupChi, "Forcefully disabling for reqid %" PRIu64 ,
                   pProcessRequestInfo->frameNum);
        }

        PublishICAMatrix(pProcessRequestInfo->frameNum, TRUE, isEISv3DisabledCurrentRequest, currentCameraIdIndex);
        m_pEndOfStreamLock->Unlock();
    }

    // In this sequence we set dependency for SOF time stamp, exposure time and frame duration,
    // needed to calculate gyro sample interval for the frame
    if ((CDKResultSuccess == result) && (0 == sequenceNumber) && (TRUE == hasDependencies))
    {
        m_endOfStreamOutputArrayFilled = FALSE;
        if ((FALSE == isEISv3DisabledCurrentRequest) && (FALSE == m_isRecording))
        {
            m_isRecording = TRUE;
            if (NULL != m_pEisUtilsLogContext && NULL != m_eisUtilsLogOpen && NULL != m_eisUtilsLogIsOpened)
            {
                bool      isLogOpened = FALSE;
                CDKResult logResult   = m_eisUtilsLogIsOpened(m_pEisUtilsLogContext, &isLogOpened);

                if (IS_RET_SUCCESS == logResult && FALSE == isLogOpened && TRUE == m_isRecording)
                {
                    char file_prefix[EIS_UTIL_MAX_FILE_PREFIX_LENGTH];
                    CHIDateTime dateTime = { 0 };
                    ChiNodeUtils::GetDateTime(&dateTime);
                    ChiNodeUtils::SNPrintF(file_prefix,
                                           sizeof(file_prefix),
                                           "VID_%04d%02d%02d_%02d%02d%02d_%dx%d",
                                           dateTime.year + 1900,
                                           dateTime.month + 1,
                                           dateTime.dayOfMonth,
                                           dateTime.hours,
                                           dateTime.minutes,
                                           dateTime.seconds,
                                           m_perSensorData[m_primarySensorIdx].outputSize.width,
                                           m_perSensorData[m_primarySensorIdx].outputSize.height);
                    // Reopen new log with new prefix
                    CDKResult logResult = m_eisUtilsLogOpen(m_pEisUtilsLogContext, file_prefix);
                    if (IS_RET_SUCCESS != logResult)
                    {
                        result = CDKResultEFailed;
                        LOG_ERROR(CamxLogGroupChi, "EISV3 Algorithm re-open log utility Failed");
                    }
                }
            }
        }
        else if ((TRUE == isEISv3DisabledCurrentRequest) && (TRUE == m_isRecording))
        {
            m_isRecording = FALSE;
            // Close current log files
            if (NULL != m_pEisUtilsLogContext && NULL != m_eisUtilsLogClose)
            {
                // Close current log files
                CDKResult logResult = m_eisUtilsLogClose(m_pEisUtilsLogContext);
                if (IS_RET_SUCCESS != logResult)
                {
                    result = CDKResultEFailed;
                    LOG_ERROR(CamxLogGroupChi, "EISV3 Algorithm close log utility Failed");
                }
            }
        }

        // Set Buffer input dependency
        pDependencyInfo->inputBufferFenceCount = 0;
        for (UINT32 inputIdx = 0; inputIdx < pProcessRequestInfo->inputNum; inputIdx++)
        {
            if ((NULL != pProcessRequestInfo->phInputBuffer) &&
                (NULL != pProcessRequestInfo->phInputBuffer[inputIdx]) &&
                (NULL != pProcessRequestInfo->phInputBuffer[inputIdx]->pfenceHandle) &&
                (NULL != pProcessRequestInfo->phInputBuffer[inputIdx]->pIsFenceSignaled))
            {
                pDependencyInfo->pInputBufferFence[pDependencyInfo->inputBufferFenceCount] =
                    pProcessRequestInfo->phInputBuffer[inputIdx]->pfenceHandle;
                pDependencyInfo->pInputBufferFenceIsSignaled[pDependencyInfo->inputBufferFenceCount] =
                    pProcessRequestInfo->phInputBuffer[inputIdx]->pIsFenceSignaled;

                pDependencyInfo->inputBufferFenceCount++;
            }
        }
        depCount = 0;
        if (1 < m_numOfLinkedCameras)
        {
            pDependencyInfo->properties[depCount]   = g_vendorTagId.multiCameraIdTagId;
            pDependencyInfo->offsets[depCount]      = 0;
            pDependencyInfo->count                  = ++depCount;
        }
        // Set the following dependencies on sequence 0 only for realtime pipeline
        if (TRUE == IsRealTimeNode())
        {
            LOG_VERBOSE(CamxLogGroupChi, "Seq number %d", sequenceNumber);
            pDependencyInfo->properties[depCount]   = g_vendorTagId.QTimerTimestampTagId;
            pDependencyInfo->offsets[depCount]      = 0;
            pDependencyInfo->count                  = ++depCount;

            pDependencyInfo->properties[depCount]   = ANDROID_SENSOR_EXPOSURE_TIME;
            pDependencyInfo->offsets[depCount]      = 0;
            pDependencyInfo->count                  = ++depCount;

            pDependencyInfo->properties[depCount]   = ANDROID_SENSOR_FRAME_DURATION;
            pDependencyInfo->offsets[depCount]      = 0;
            pDependencyInfo->count                  = ++depCount;

            if (TRUE == m_isFOVCEnabled)
            {
                pDependencyInfo->properties[depCount] = g_vendorTagId.FOVCFactorTagId;
                pDependencyInfo->offsets[depCount]    = 0;
                pDependencyInfo->count                = ++depCount;
            }
        }

        CHIFLUSHINFO chiFlushInfo;
        UINT64       lastFlushedRequestId = 0;
        UINT64       requestIdOffset      = 0;

        g_ChiNodeInterface.pGetFlushInfo(m_hChiSession, &chiFlushInfo);
        lastFlushedRequestId = chiFlushInfo.lastFlushedRequestId;
        requestIdOffset      = ChiNodeUtils::GetRequestIdOffset(pProcessRequestInfo->frameNum, lastFlushedRequestId);

        // Ensure that algo execution for frame N - 1 has happened before we execute tha algo for frame N
        if (1 < requestIdOffset)
        {
            pProcessRequestInfo->pDependency->sequentialExecutionNeeded = TRUE;
        }

        //< Set the dep property for the sequenece ID, used by the node to determine what state it is in.
        pProcessRequestInfo->pDependency->processSequenceId = 1;
        pDependencyInfo->hNodeSession                       = m_hChiSession;
    }

    // In this sequence we set gyro denpendency based on the SOF time stamp, exposure time and frame duration
    if ((CDKResultSuccess == result) && (1 == sequenceNumber) && (TRUE == hasDependencies))
    {
        ///< Get Input Buffer size
        if (pProcessRequestInfo->inputNum > 0)
        {
            m_perSensorData[currentCameraIdIndex].inputSize.width  = pProcessRequestInfo->phInputBuffer[0]->format.width;
            m_perSensorData[currentCameraIdIndex].inputSize.height = pProcessRequestInfo->phInputBuffer[0]->format.height;
        }

        LOG_VERBOSE(CamxLogGroupChi, "Seq number %d", sequenceNumber);
        if ((TRUE == m_gyroNCSServiceAvailable) &&
            ((FALSE == isEISv3DisabledDependentRequest) ||
             (FALSE == isEISv3DisabledCurrentRequest)))
        {
            SetGyroDependency(pProcessRequestInfo, currentCameraIdIndex);
            SetDependencies(pProcessRequestInfo);
            pProcessRequestInfo->pDependency->processSequenceId = 2;
        }
        else
        {
            // Gyro NCS service not available, move to sequence 2
            sequenceNumber = 2;
        }
    }

    // In this sequence we execute the EISv3 Algo and publish the results to tag EISV3PerspectiveGridTransformTagId,
    // We then set dependency on EISV3PerspectiveGridTransformTagId for request (n + num forwardlooking frame).
    if ((CDKResultSuccess == result) && (2 == sequenceNumber) && (TRUE == hasDependencies))
    {
        LOG_VERBOSE(CamxLogGroupChi, "Seq number %d", sequenceNumber);
        if ((TRUE   == m_gyroNCSServiceAvailable)        &&
            (FALSE  == m_bIsDefaultGridTransformEnabled) &&
            ((FALSE == isEISv3DisabledDependentRequest)  ||
            (FALSE  == isEISv3DisabledCurrentRequest)))
        {
            //< Initialize output to default matrix
            eis3Output.stabilizationTransform.matrices.perspMatrices     = &perspectiveMatrix[0];
            eis3Output.stabilizationTransform.grid.grid                  = &perspectiveGrid[0];
            eis3Output.stabilizationTransform.grid.gridExtrapolate       = &gridExtrapolateICA10[0];
            eis3Output.alignment_matrix_domain_undistorted.perspMatrices = &alignmentMatrixDomainUndistorted;
            eis3Output.alignment_matrix_domain_stabilized.perspMatrices  = &alignmentMatrixDomainStabilized;

             ///< Execute Algo
             result = ExecuteAlgo(pProcessRequestInfo->frameNum, &eis3Output, currentCameraIdIndex);

            if (CDKResultSuccess != result)
            {
                LOG_ERROR(CamxLogGroupChi, "EISv3 algo execution failed for request %" PRIu64, pProcessRequestInfo->frameNum);
                ///< Update metadata with default result
                eis3Output.has_output = FALSE;
                result                = CDKResultSuccess;
            }
            else
            {
                // convert ICA20 grid to ICA10 if ICA version is 10
                if (ChiICAVersion::ChiICA10 == m_ICAVersion)
                {
                    // converting grid inplace
                    result = ConvertICA20GridToICA10Grid(&eis3Output.stabilizationTransform.grid,
                                                         &eis3Output.stabilizationTransform.grid);

                    if (CDKResultSuccess != result)
                    {
                        LOG_ERROR(CamxLogGroupChi, "Grid conversion failed for request %" PRIu64,
                                  pProcessRequestInfo->frameNum);
                        eis3Output.stabilizationTransform.grid.enable = FALSE;
                    }
                }
            }
        }
        else
        {
            LOG_VERBOSE(CamxLogGroupChi,
                      "For request %"  PRIu64
                      " EISv3 disabled due to NCSServiceAvailable %d IsDefaultGridTransformEnabled %d "
                      " EISv3DisabledDependentRequest %d, EISv3DisabledCurrentRequest %d",
                      pProcessRequestInfo->frameNum,
                      m_gyroNCSServiceAvailable,
                      m_bIsDefaultGridTransformEnabled,
                      isEISv3DisabledDependentRequest,
                      isEISv3DisabledCurrentRequest);
            ///< Update metadata with default result
            eis3Output.has_output = FALSE;
        }

        ///< Update metadata with result
        UpdateMetaData(pProcessRequestInfo->frameNum, currentCameraIdIndex, &eis3Output);
        pProcessRequestInfo->pDependency->satisfySequentialExecutionDependency = TRUE;

        // Send the Request Done for this node in order to not delay the preview buffer
        CHINODEPROCESSMETADATADONEINFO metadataDoneInfo;
        metadataDoneInfo.size        = sizeof(metadataDoneInfo);
        metadataDoneInfo.hChiSession = m_hChiSession;
        metadataDoneInfo.frameNum    = pProcessRequestInfo->frameNum;
        metadataDoneInfo.result      = CDKResultSuccess;
        g_ChiNodeInterface.pProcessMetadataDone(&metadataDoneInfo);

        // Check if current request enabled before setting future dependency
        if (FALSE == isEISv3DisabledCurrentRequest)
        {
            LOG_VERBOSE(CamxLogGroupChi,
                "For request %"  PRIu64
                " As EISv3 is enabled for current, set future dep",
                pProcessRequestInfo->frameNum);
            pDependencyInfo = pProcessRequestInfo->pDependency;
            depCount        = 0;

            pDependencyInfo->properties[depCount] = g_vendorTagId.EISV3PerspectiveGridTransformTagId;
            pDependencyInfo->offsets[depCount]    = m_lookahead;
            pDependencyInfo->negate[depCount]     = TRUE;
            depCount++;

            pDependencyInfo->count                = depCount;
            pDependencyInfo->hNodeSession         = m_hChiSession;

            pProcessRequestInfo->pDependency->processSequenceId = 3;
        }
        else
        {
            LOG_VERBOSE(CamxLogGroupChi,
                      "For request %"  PRIu64
                      " As EISv3 is disabled for current, satify dep immediatly",
                      pProcessRequestInfo->frameNum);
            sequenceNumber = 3;
        }
    }

    // This is the final sequence, which is triggerd when EISV3PerspectiveGridTransformTagId
    // is published for (n + num forwardlooking frames) request.
    // In this sequence we will publish the results to IPE/GPU for warping
    if ((CDKResultSuccess == result) && (3 == sequenceNumber) && (TRUE == hasDependencies))
    {
        for (UINT32 i = 0; i < pProcessRequestInfo->inputNum; i++)
        {
            if (TRUE == IsBypassableNode())
            {
                pProcessRequestInfo->pBypassData[i].isBypassNeeded          = TRUE;
                pProcessRequestInfo->pBypassData[i].selectedInputPortIndex  = i;
            }
        }

        //< Properties dependencies should be met by now. Fill data.
        hasDependencies = FALSE;
        LOG_VERBOSE(CamxLogGroupChi, "Seq number %d", sequenceNumber);

        // Do not increment m_lastEIS3publishedRequest if current request is disabled
        if (FALSE == isEISv3DisabledCurrentRequest)
        {
            m_pEndOfStreamLock->Lock();
            m_lastEIS3publishedRequest = pProcessRequestInfo->frameNum;
            LOG_VERBOSE(CamxLogGroupChi, "Last video request published to ICA %" PRIu64, m_lastEIS3publishedRequest);
            m_pEndOfStreamLock->Unlock();
        }

        PublishICAMatrix(pProcessRequestInfo->frameNum, FALSE, isEISv3DisabledCurrentRequest, currentCameraIdIndex);

        // Metadata Done is being handled already, the wrapper does not need to handle this for us
        pProcessRequestInfo->isEarlyMetadataDone  = TRUE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV3Node::PostPipelineCreate()
{
    CDKResult       result     = CDKResultSuccess;
    ImageDimensions marginDims = { 0 };

    CHIDATASOURCECONFIG    CHIDataSourceConfig;
    CHINCSDATASOURCECONFIG NCSDataSourceCfg;

    memset(&NCSDataSourceCfg, 0, sizeof(CHINCSDATASOURCECONFIG));
    NCSDataSourceCfg.sourceType     = ChiDataGyro;
    NCSDataSourceCfg.samplingRate   = GetGyroFrequency(m_primarySensorIdx);
    NCSDataSourceCfg.operationMode  = 0;
    NCSDataSourceCfg.reportRate     = 10000;
    NCSDataSourceCfg.size           = sizeof(CHINCSDATASOURCECONFIG);

    memset(&CHIDataSourceConfig, 0, sizeof(CHIDATASOURCECONFIG));
    CHIDataSourceConfig.sourceType  = ChiDataGyro;
    CHIDataSourceConfig.pConfig     = &NCSDataSourceCfg;

    if (CDKResultSuccess == g_ChiNodeInterface.pGetDataSource(m_hChiSession, GetDataSource(), &CHIDataSourceConfig))
    {
        m_gyroNCSServiceAvailable = TRUE;
    }
    else
    {
        m_gyroNCSServiceAvailable = FALSE;
        LOG_ERROR(CamxLogGroupChi, "NCS service for gyro not available");
    }

    m_pEndOfStreamOutputArray = static_cast<EISV3PerspectiveGridTransforms*>(CHI_CALLOC(
                                    m_EISV3RequestQueueDepth,
                                    sizeof(EISV3PerspectiveGridTransforms)));

    if (NULL == m_pEndOfStreamOutputArray)
    {
        LOG_ERROR(CamxLogGroupChi, "Calloc failed for end of stream output array");
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        // Initialize EISv3 lib
        result = InitializeLib();
    }

    if (CDKResultSuccess == result)
    {
        m_eis3GetTotalMargin(m_phEIS3Handle, m_primarySensorIdx, &marginDims);
        if ((marginDims.widthPixels != m_stabilizationMargins.widthPixels) ||
            (marginDims.heightLines != m_stabilizationMargins.heightLines))
        {
            LOG_ERROR(CamxLogGroupChi,
                      "Unexpected EISv3 margin values. Using %ux%u, calculated %ux%u",
                      m_stabilizationMargins.widthPixels, m_stabilizationMargins.heightLines,
                      marginDims.widthPixels, marginDims.heightLines);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::ChiEISV3Node
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiEISV3Node::ChiEISV3Node(
    EISV3OverrideSettings overrideSettings)
    : m_hChiSession(NULL)
    , m_nodeId(0)
    , m_nodeCaps(0)
    , m_pEndOfStreamOutputArray(NULL)
    , m_endOfStreamOutputArrayFilled(FALSE)
    , m_pEndOfStreamLock(NULL)
    , m_lastEIS3publishedRequest(0)
    , m_gyroNCSServiceAvailable(FALSE)
    , m_numOfLinkedCameras(1)
    , m_primaryCameraId(0)
    , m_pEisUtilsLogContext(NULL)
    , m_isUtilsLogFlags(IS_UTILS_FLAG_NONE)
    , m_isFOVCEnabled(0)
    , m_cropFactorFOVC(FOVCFactorDefault)
{
    m_lookahead                         = DefaultEISV3FrameDelay;
    m_phEIS3Handle                      = NULL;
    m_primarySensorIdx                  = 0;
    m_algoOperationMode                 = overrideSettings.algoOperationMode;
    m_inputPortPathType                 = EISV3PathType::FullPath;
    m_bIsLDCGridEnabled                 = overrideSettings.isLDCGridEnabled;
    m_isEnabledDumpForceFlush           = overrideSettings.isEnabledDumpForceFlush;
    m_bIsDefaultGridTransformEnabled    = overrideSettings.isDefaultGridTransformEnabled;
    m_EISV3RequestQueueDepth            = DefaultEISV3FrameDelay + DefaultRequestQueueDepth;
    m_overrideOisMode                   = overrideSettings.overrideOisMode;
    memset(&m_hChiDataSource, 0, sizeof(CHIDATASOURCE));
    memset(&m_perSensorData,  0, sizeof(m_perSensorData));

    for (UINT i = 0; i < MaxMulticamSensors; i++)
    {
        m_pLDCIn2OutGrid[i] = NULL;
        m_pLDCOut2InGrid[i] = NULL;
        m_LDCIn2OutWarpGrid[i] = { 0 };
        m_LDCOut2InWarpGrid[i] = { 0 };
    }

    UINT32 tmpFlags = static_cast<UINT32>(IS_UTILS_FLAG_NONE);

    if (TRUE == overrideSettings.isEnabledDumpInputFile)
    {
        tmpFlags |= static_cast<UINT32>(IS_UTILS_FLAG_WRITE_INPUT);
    }

    if (TRUE == overrideSettings.isEnabledDumpOutputFile)
    {
        tmpFlags |= static_cast<UINT32>(IS_UTILS_FLAG_WRITE_OUTPUT);
    }

    if (TRUE == overrideSettings.isEnabledDumpInputLogcat)
    {
        tmpFlags |= static_cast<UINT32>(IS_UTILS_FLAG_LOGCAT_INPUT);
    }

    if (TRUE == overrideSettings.isEnabledDumpOutputLogcat)
    {
        tmpFlags |= static_cast<UINT32>(IS_UTILS_FLAG_LOGCAT_OUTPUT);
    }

    m_isUtilsLogFlags = static_cast<is_utils_log_flags>(tmpFlags);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::~ChiEISV3Node
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiEISV3Node::~ChiEISV3Node()
{
    CDKResult result = CDKResultSuccess;

    ///< Destroy EIS log util
    if (NULL != m_pEisUtilsLogContext)
    {
        m_eisUtilsLogDestroy(&m_pEisUtilsLogContext);
        m_pEisUtilsLogContext = NULL;
    }

    ///< Deinitialize algo
    if (NULL != m_phEIS3Handle)
    {
        int32_t isResult = m_eis3Deinitialize(&m_phEIS3Handle);
        if (IS_RET_SUCCESS != isResult)
        {
            LOG_ERROR(CamxLogGroupChi, "EISv3 Algo Deinit failed");
            result = CDKResultEFailed;
        }
        m_phEIS3Handle = NULL;
    }

    //< Unload Lib
    if (NULL != m_hEISv3Lib)
    {
        result = UnLoadLib();
        if (CDKResultSuccess != result)
        {
            LOG_ERROR(CamxLogGroupChi, "EISv3 Lib Unload failed");
        }
    }

    m_hEISv3Lib   = NULL;
    m_hChiSession = NULL;

    if (NULL != m_pEndOfStreamOutputArray)
    {
        CHI_FREE(m_pEndOfStreamOutputArray);
        m_pEndOfStreamOutputArray = NULL;
    }

    if (NULL != m_pEndOfStreamLock)
    {
        m_pEndOfStreamLock->Destroy();
        m_pEndOfStreamLock = NULL;
    }

    for (UINT32 i = 0; i < m_numOfLinkedCameras; i++)
    {
        if (NULL != m_pLDCIn2OutGrid[i])
        {
            CHI_FREE(m_pLDCIn2OutGrid[i]);
            m_pLDCIn2OutGrid[i] = NULL;
        }

        if (NULL != m_pLDCOut2InGrid[i])
        {
            CHI_FREE(m_pLDCOut2InGrid[i]);
            m_pLDCOut2InGrid[i] = NULL;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV3Node::QueryMetadataPublishList(
    CHINODEMETADATALIST* pMetadataPublishlist)
{
    UINT count = 0;

    if (NULL != pMetadataPublishlist)
    {
        if (ChiICAMax == m_ICAVersion)
        {
            // deployment type gpu
            pMetadataPublishlist->tagArray[count] = g_vendorTagId.ICAInGridTransformLookAheadTagId;
            pMetadataPublishlist->tagCount        = ++count;
            pMetadataPublishlist->partialTagCount = 0;
        }
        else
        {
            // deployment type ica
            pMetadataPublishlist->tagArray[count] = g_vendorTagId.ICAInPerspectiveTransformTagId;
            pMetadataPublishlist->tagCount        = ++count;
            pMetadataPublishlist->partialTagCount = 0;

            pMetadataPublishlist->tagArray[count] = g_vendorTagId.ICAInGridOut2InTransformTagId;
            pMetadataPublishlist->tagCount        = ++count;
            pMetadataPublishlist->partialTagCount = 0;

            pMetadataPublishlist->tagArray[count] = g_vendorTagId.ICAReferenceParamsTagId;
            pMetadataPublishlist->tagCount        = ++count;
            pMetadataPublishlist->partialTagCount = 0;
        }
    }

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::SetBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV3Node::SetBufferInfo(
    CHINODESETBUFFERPROPERTIESINFO* pSetBufferInfo)
{
    CDKResult                   result          = CDKResultSuccess;
    ImageDimensions             marginDims      = { 0 };
    is_get_stabilization_margin inputDims       = { 0 };
    FLOAT                       minTotalMarginX = 0.0F;
    FLOAT                       minTotalMarginY = 0.0F;
    CHIDimension                outDimension    = { 0 };
    CHIDimension                outDimRealtime  = { 0 };
    VOID*                       pData           = NULL;

    // Get per camera config
    GetPerCameraConfig();

    // If single camera usecase get sensor mode using current sensor mode tag
    if (1 == m_numOfLinkedCameras)
    {
        pData = ChiNodeUtils::GetMetaData(0,
                                          g_vendorTagId.sensorInfoTagId | UsecaseMetadataSectionMask,
                                          ChiMetadataDynamic,
                                          &g_ChiNodeInterface,
                                          m_hChiSession);
        if (NULL != pData)
        {
            ChiSensorModeInfo* pSensorInfo                  = reinterpret_cast<ChiSensorModeInfo*>(pData);
            m_perSensorData[0].cameraConfig.sensorModeInfo  = *pSensorInfo;
        }
    }

    for (UINT i = 0; i < m_numOfLinkedCameras; i++)
    {
        m_perSensorData[i].inputSize.width  = pSetBufferInfo->pFormat->width;
        m_perSensorData[i].inputSize.height = pSetBufferInfo->pFormat->height;

        // Update tuning handles based on updated sensor mode after buffer negotiation
        GetChromatixTuningHandle(i, m_perSensorData[i].cameraConfig.sensorModeInfo.modeIndex);
    }

    GetMinTotalMargins(&minTotalMarginX, &minTotalMarginY);

    pData = ChiNodeUtils::GetMetaData(0,
                                      (g_vendorTagId.EISV3StabilizedOutputDimsTagId | UsecaseMetadataSectionMask),
                                      ChiMetadataDynamic,
                                      &g_ChiNodeInterface,
                                      m_hChiSession);
    if (NULL != pData)
    {
        outDimension = *static_cast<CHIDimension*>(pData);
    }
    // TODO: Assume that 4K video has different sources, so skip largest dim pick. Make it generic in the future.
    if ((0 != g_vendorTagId.EISV3OutputDimsRealTimeTagId) && (outDimension.width < VIDEO4KWIDTH))
    {
        pData = ChiNodeUtils::GetMetaData(0,
                                          (g_vendorTagId.EISV3OutputDimsRealTimeTagId | UsecaseMetadataSectionMask),
                                          ChiMetadataDynamic,
                                          &g_ChiNodeInterface,
                                          m_hChiSession);
        if (NULL != pData)
        {
            outDimRealtime = *static_cast<CHIDimension*>(pData);
            // Use the largest dim for output
            if ((outDimRealtime.height * outDimRealtime.width) > (outDimension.height * outDimension.width))
            {
                outDimension = outDimRealtime;
            }
            // Sanity check for final dim
            if ((outDimension.height * outDimension.width) >
                (pSetBufferInfo->pFormat->width * pSetBufferInfo->pFormat->height))
            {
                outDimension.width  = pSetBufferInfo->pFormat->width;
                outDimension.height = pSetBufferInfo->pFormat->height;
            }
        }
    }

    for (UINT i = 0; i < m_numOfLinkedCameras; i++)
    {
        m_perSensorData[i].outputSize.width  = outDimension.width;
        m_perSensorData[i].outputSize.height = outDimension.height;
    }

    inputDims.sensor_is_input_frame_width               = pSetBufferInfo->pFormat->width;
    inputDims.sensor_is_input_frame_height              = pSetBufferInfo->pFormat->height;
    inputDims.sensor_minimal_total_margin_x             = minTotalMarginX;
    inputDims.sensor_minimal_total_margin_y             = minTotalMarginY;
    inputDims.common_is_output_frame_width              = outDimension.width;
    inputDims.common_is_output_frame_height             = outDimension.height;
    inputDims.common_do_virtual_upscale_in_transform    = FALSE;

    UINT32 isResult = m_eis3GetTotalMarginEx(&inputDims, &marginDims);
    if (IS_RET_SUCCESS != isResult)
    {
        result = CDKResultEFailed;
        LOG_ERROR(CamxLogGroupChi, "m_eis3GetTotalMarginEx failed - %d", isResult);
    }

    m_stabilizationMargins.widthPixels = marginDims.widthPixels;
    m_stabilizationMargins.heightLines = marginDims.heightLines;

    // Until Geo-Lib integration is done, use utility function to connect old ROIs to new EIS API.
    // Remove this once integration is done
    for (UINT32 i = 0; i < m_numOfLinkedCameras; i++)
    {
        isResult = m_eis3GetStabilizationCropRatioEx(&inputDims,
                                                     &m_stabilizationCropRatioX[i],
                                                     &m_stabilizationCropRatioY[i]);
        if (IS_RET_SUCCESS != isResult)
        {
            result = CDKResultEFailed;
            LOG_ERROR(CamxLogGroupChi, "m_eis3_get_stabilization_crop_ratio_ex failed - %d", isResult);
        }
    }

    LOG_INFO(CamxLogGroupChi, "From in %ux%u and out %ux%u using stabilization margins %ux%u with virtual margins %fx%f"
              "and crop ratio (x,y): (%lf, %lf)",
              pSetBufferInfo->pFormat->width,
              pSetBufferInfo->pFormat->height,
              outDimension.width,
              outDimension.height,
              m_stabilizationMargins.widthPixels,
              m_stabilizationMargins.heightLines,
              minTotalMarginX,
              minTotalMarginY,
              m_stabilizationCropRatioX[m_primarySensorIdx],
              m_stabilizationCropRatioY[m_primarySensorIdx]);

    GetAdditionalCropOffset();

    StabilizationMargin actualMargin;
    memset(&actualMargin, 0, sizeof(StabilizationMargin));
    if ((m_stabilizationMargins.widthPixels > m_additionalCropOffset.widthPixels) &&
        (m_stabilizationMargins.heightLines > m_additionalCropOffset.heightLines))
    {
        actualMargin.widthPixels = m_stabilizationMargins.widthPixels - m_additionalCropOffset.widthPixels;
        actualMargin.heightLines = m_stabilizationMargins.heightLines - m_additionalCropOffset.heightLines;
    }

    CHIMETADATAINFO      metadataInfo     = { 0 };
    const UINT32         tagSize          = 2;
    UINT32               index            = 0;
    CHITAGDATA           tagData[tagSize] = { {0} };
    UINT32               tagList[tagSize];

    metadataInfo.size       = sizeof(CHIMETADATAINFO);
    metadataInfo.chiSession = m_hChiSession;
    metadataInfo.tagNum     = tagSize;
    metadataInfo.pTagList   = &tagList[index];
    metadataInfo.pTagData   = &tagData[index];

    tagList[index]           = (g_vendorTagId.EISV3StabilizationMarginsTagId | UsecaseMetadataSectionMask);
    tagData[index].size      = sizeof(CHITAGDATA);
    tagData[index].requestId = 0;
    tagData[index].pData     = &actualMargin;
    tagData[index].dataSize  = g_VendorTagSectionEISLookahead[3].numUnits;
    index++;

    tagList[index]           = (g_vendorTagId.EISV3AdditionalCropOffsetTagId | UsecaseMetadataSectionMask);
    tagData[index].size      = sizeof(CHITAGDATA);
    tagData[index].requestId = 0;
    tagData[index].pData     = &m_additionalCropOffset;
    tagData[index].dataSize  = g_VendorTagSectionEISLookahead[4].numUnits;

    LOG_VERBOSE(CamxLogGroupChi, "Publishing actual margins %ux%u and additional crop offset %ux%u from eisv3",
                actualMargin.widthPixels, actualMargin.heightLines,
                m_additionalCropOffset.widthPixels, m_additionalCropOffset.heightLines);

    g_ChiNodeInterface.pSetMetadata(&metadataInfo);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV2Node::QueryBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV3Node::QueryBufferInfo(
    CHINODEQUERYBUFFERINFO* pQueryBufferInfo)
{
    CDKResult   result                      = CDKResultSuccess;
    UINT        perOutputPortOptimalWidth   = 0;
    UINT        perOutputPortOptimalHeight  = 0;
    UINT32      perOutputPortMinWidth       = 0;
    UINT32      perOutputPortMinHeight      = 0;
    UINT32      perOutputPortMaxWidth       = MaxDimension;
    UINT32      perOutputPortMaxHeight      = MaxDimension;

    // For EISV2 node there is no real buffer connected to its output port.
    // But for the purpose of buffer negotiation, we fill buffer query info with default buffer data.
    // IFE needs these info to calculate correct buffer input requirement
    for (UINT outputIndex = 0; outputIndex < pQueryBufferInfo->numOutputPorts; outputIndex++)
    {
        ChiOutputPortQueryBufferInfo* pOutputPort    = &pQueryBufferInfo->pOutputPortQueryInfo[outputIndex];
        CHINODEBUFFERREQUIREMENT*     pOutputOptions = &pOutputPort->outputBufferOption;

        // If EISv2 node is bypassable, need to get buffer requirments from downstream node
        if (TRUE == IsBypassableNode())
        {
            for (UINT inputIndex = 0; inputIndex < pOutputPort->numConnectedInputPorts; inputIndex++)
            {
                CHINODEBUFFERREQUIREMENT* pOutputRequirement = &pOutputPort->pBufferRequirement[inputIndex];

                perOutputPortOptimalWidth  = ChiNodeUtils::MaxUINT32(pOutputRequirement->optimalW, perOutputPortOptimalWidth);
                perOutputPortOptimalHeight = ChiNodeUtils::MaxUINT32(pOutputRequirement->optimalH, perOutputPortOptimalHeight);
                perOutputPortMinWidth      = ChiNodeUtils::MaxUINT32(pOutputRequirement->minW, perOutputPortMinWidth);
                perOutputPortMinHeight     = ChiNodeUtils::MaxUINT32(pOutputRequirement->minH, perOutputPortMinHeight);
                perOutputPortMaxWidth      = ChiNodeUtils::MinUINT32(pOutputRequirement->maxW, perOutputPortMaxWidth);
                perOutputPortMaxHeight     = ChiNodeUtils::MinUINT32(pOutputRequirement->maxH, perOutputPortMaxHeight);
            }
        }

        pOutputOptions->minW        = perOutputPortMinWidth;
        pOutputOptions->minH        = perOutputPortMinHeight;
        pOutputOptions->maxW        = perOutputPortMaxWidth;
        pOutputOptions->maxH        = perOutputPortMaxHeight;
        pOutputOptions->optimalW    = perOutputPortOptimalWidth;
        pOutputOptions->optimalH    = perOutputPortOptimalHeight;
    }

    for (UINT inputIndex = 0; inputIndex < pQueryBufferInfo->numInputPorts; inputIndex++)
    {
        ChiInputPortQueryBufferInfo* pInputOptions  = &pQueryBufferInfo->pInputOptions[inputIndex];
        CHINODEBUFFERREQUIREMENT*    pOutputOptions = &pQueryBufferInfo->pOutputPortQueryInfo[inputIndex].outputBufferOption;

        pInputOptions->inputBufferOption.minW       = pOutputOptions->minW;
        pInputOptions->inputBufferOption.minH       = pOutputOptions->minH;
        pInputOptions->inputBufferOption.maxW       = pOutputOptions->maxW;
        pInputOptions->inputBufferOption.maxH       = pOutputOptions->maxH;
        pInputOptions->inputBufferOption.optimalW   = pOutputOptions->optimalW;
        pInputOptions->inputBufferOption.optimalH   = pOutputOptions->optimalH;
    }

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::UpdateMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiEISV3Node::UpdateMetaData(
    UINT64          requestId,
    UINT32          sensorIndex,
    is_output_type* pAlgoResult)
{
    CHIMETADATAINFO      metadataInfo     = { 0 };
    const UINT32         tagSize          = 1;
    UINT32               index            = 0;
    CHITAGDATA           tagData[tagSize] = { {0} };
    UINT32               tagList[tagSize];

    metadataInfo.size       = sizeof(CHIMETADATAINFO);
    metadataInfo.chiSession = m_hChiSession;
    metadataInfo.tagNum     = tagSize;
    metadataInfo.pTagList   = &tagList[0];
    metadataInfo.pTagData   = &tagData[0];

    ///< Update resulting perspective matrix and grid
    EISV3PerspectiveGridTransforms perspectiveAndGridTrans;
    memset(&perspectiveAndGridTrans, 0, sizeof(EISV3PerspectiveGridTransforms));

    if (TRUE == pAlgoResult->has_output)
    {
        if (TRUE == pAlgoResult->stabilizationTransform.matrices.enable)
        {
            NcLibWarpMatrices resultMatrices                                     = pAlgoResult->stabilizationTransform.matrices;
            perspectiveAndGridTrans.perspective.perspectiveTransformEnable       = resultMatrices.enable;
            perspectiveAndGridTrans.perspective.perspectiveConfidence            = resultMatrices.confidence;
            perspectiveAndGridTrans.perspective.byPassAlignmentMatrixAdjustement = FALSE;
            perspectiveAndGridTrans.perspective.perspetiveGeometryNumColumns     = resultMatrices.numColumns;
            perspectiveAndGridTrans.perspective.perspectiveGeometryNumRows       = static_cast<UINT8>(resultMatrices.numRows);
            perspectiveAndGridTrans.perspective.ReusePerspectiveTransform        = 0;

            //TBD: Change from active sensor to what was passed to the algo
            perspectiveAndGridTrans.perspective.transformDefinedOnWidth  = m_perSensorData[sensorIndex].inputSize.width;
            perspectiveAndGridTrans.perspective.transformDefinedOnHeight = m_perSensorData[sensorIndex].inputSize.height;

            memcpy(&perspectiveAndGridTrans.perspective.perspectiveTransformArray,
                   &resultMatrices.perspMatrices[0],
                   sizeof(NcLibPerspTransformSingle)* perspectiveAndGridTrans.perspective.perspectiveGeometryNumRows);

            LOG_VERBOSE(CamxLogGroupChi, "rows %d, columns %d", perspectiveAndGridTrans.perspective.perspectiveGeometryNumRows,
                        perspectiveAndGridTrans.perspective.perspetiveGeometryNumColumns);
        }
        else
        {
            LOG_INFO(CamxLogGroupChi, "Algo has perspective transform disabled for req id %" PRId64, requestId);
        }

        if (TRUE == pAlgoResult->stabilizationTransform.grid.enable)
        {
            NcLibWarpGrid resultGrid = pAlgoResult->stabilizationTransform.grid;
            perspectiveAndGridTrans.grid.gridTransformEnable                   = resultGrid.enable;
            perspectiveAndGridTrans.grid.gridTransformArrayExtrapolatedCorners =
                (resultGrid.extrapolateType == NcLibWarpGridExtrapolationType::EXTRAPOLATION_TYPE_FOUR_CORNERS) ? 1 : 0;

            if (1 == perspectiveAndGridTrans.grid.gridTransformArrayExtrapolatedCorners)
            {
                memcpy(&perspectiveAndGridTrans.grid.gridTransformArrayCorners[0],
                       resultGrid.gridExtrapolate,
                       sizeof(perspectiveAndGridTrans.grid.gridTransformArrayCorners));
            }

            //TBD: Change from active sensor to what was passed to the algo
            perspectiveAndGridTrans.grid.transformDefinedOnWidth  = m_perSensorData[sensorIndex].inputSize.width;
            perspectiveAndGridTrans.grid.transformDefinedOnHeight = m_perSensorData[sensorIndex].inputSize.height;
            perspectiveAndGridTrans.grid.reuseGridTransform       = 0;
            getICAGridGeometryVersion(resultGrid.numRows, resultGrid.numColumns, &perspectiveAndGridTrans.grid.geometry);
            memcpy(&perspectiveAndGridTrans.grid.gridTransformArray[0], resultGrid.grid,
                   sizeof(ICAGridArray) * resultGrid.numRows * resultGrid.numColumns);
        }
        else
        {
            LOG_INFO(CamxLogGroupChi, "Algo has grid transform disabled for req id %" PRId64, requestId);
        }

        if (TRUE == pAlgoResult->alignment_matrix_domain_stabilized.enable)
        {
            NcLibWarpMatrices resultMatrices = pAlgoResult->alignment_matrix_domain_stabilized;

            perspectiveAndGridTrans.alignmentMatrix.perspectiveTransformEnable       = resultMatrices.enable;
            perspectiveAndGridTrans.alignmentMatrix.perspectiveConfidence            = resultMatrices.confidence;
            perspectiveAndGridTrans.alignmentMatrix.byPassAlignmentMatrixAdjustement = FALSE;
            perspectiveAndGridTrans.alignmentMatrix.perspetiveGeometryNumColumns     = resultMatrices.numColumns;
            perspectiveAndGridTrans.alignmentMatrix.perspectiveGeometryNumRows       = static_cast<UINT8>(
                                                                                       resultMatrices.numRows);
            perspectiveAndGridTrans.alignmentMatrix.ReusePerspectiveTransform        = 0;

            //TBD: Change from active sensor to what was passed to the algo
            perspectiveAndGridTrans.alignmentMatrix.transformDefinedOnWidth  = m_perSensorData[sensorIndex].inputSize.width;
            perspectiveAndGridTrans.alignmentMatrix.transformDefinedOnHeight = m_perSensorData[sensorIndex].inputSize.height;

            memcpy(&perspectiveAndGridTrans.alignmentMatrix.perspectiveTransformArray,
                   &resultMatrices.perspMatrices[0],
                   sizeof(NcLibPerspTransformSingle));

            LOG_VERBOSE(CamxLogGroupChi, "gyro alignment rows %d, columns %d",
                        perspectiveAndGridTrans.alignmentMatrix.perspectiveGeometryNumRows,
                        perspectiveAndGridTrans.alignmentMatrix.perspetiveGeometryNumColumns);
        }
        else
        {
            LOG_INFO(CamxLogGroupChi, "Algo has gyro alignment matrix disabled for req id %" PRId64, requestId);
        }

        if ((FALSE == pAlgoResult->stabilizationTransform.matrices.enable) &&
            (FALSE == pAlgoResult->stabilizationTransform.grid.enable) &&
            pAlgoResult->frame_id > m_lookahead)
        {
            LOG_ERROR(CamxLogGroupChi, "ERROR: Algo has no perspective or grid transform for req id %" PRId64, requestId);
        }
    }
    else
    {
        // Fill Identity transform
        LOG_ERROR(CamxLogGroupChi, "Algo has no output for request id %" PRId64, requestId);
        FillDefaultGridTransform(&perspectiveAndGridTrans.perspective,
                                 &perspectiveAndGridTrans.grid,
                                 &perspectiveAndGridTrans.alignmentMatrix,
                                 sensorIndex);
    }

    // Publish EISv3 Perspective transform
    tagList[index]           = g_vendorTagId.EISV3PerspectiveGridTransformTagId;
    tagData[index].size      = sizeof(CHITAGDATA);
    tagData[index].requestId = requestId;
    tagData[index].pData     = &perspectiveAndGridTrans;
    tagData[index].dataSize  = g_VendorTagSectionEISV3Node[0].numUnits;

    g_ChiNodeInterface.pSetMetadata(&metadataInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::PublishICAMatrix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiEISV3Node::PublishICAMatrix(
    UINT64 requestId,
    BOOL   stopRecording,
    BOOL   isEISv3Disabled,
    UINT32 sensorIndex)
{
    EISV3PerspectiveGridTransforms perspectiveAndGridTransforms = { { 0 } };
    VOID*                          pData                        = NULL;
    UINT32                         index                        = 0;
    BOOL                           publishTag                   = TRUE;

    if (TRUE == isEISv3Disabled)
    {
        // set default perspective transform when EISv3 disabled
        FillDefaultGridTransform(&perspectiveAndGridTransforms.perspective,
                                 &perspectiveAndGridTransforms.grid,
                                 &perspectiveAndGridTransforms.alignmentMatrix,
                                 sensorIndex);
    }
    else if (TRUE == stopRecording)
    {
        ///< Stop recording - Publish last good matrix
        LOG_VERBOSE(CamxLogGroupChi, "EISv3 stop recording at req id %" PRIu64, requestId);
        memcpy(&perspectiveAndGridTransforms,
               &m_pEndOfStreamOutputArray[requestId % m_EISV3RequestQueueDepth],
               sizeof(EISV3PerspectiveGridTransforms));
    }
    else
    {
        // Get the matrix from future request
        pData = ChiNodeUtils::GetMetaData(requestId + m_lookahead,
                                          g_vendorTagId.EISV3PerspectiveGridTransformTagId,
                                          ChiMetadataDynamic,
                                          &g_ChiNodeInterface,
                                          m_hChiSession);

        // Publish the matrix on the current request
        if (NULL != pData)
        {
            memcpy(&perspectiveAndGridTransforms,
                   static_cast<EISV3PerspectiveGridTransforms*>(pData),
                   sizeof(EISV3PerspectiveGridTransforms));
        }
        else
        {
            publishTag = FALSE;
        }
    }

    if (TRUE == publishTag)
    {
        if (ChiICAMax == m_ICAVersion)
        {
            CHIMETADATAINFO      metadataInfo           = { 0 };
            const UINT32         gpuTagSize             = 1;
            CHITAGDATA           gpuTagData[gpuTagSize] = { {0} };
            UINT32               gpuTagList[gpuTagSize];

            gpuTagList[0]           = g_vendorTagId.ICAInGridTransformLookAheadTagId;
            gpuTagData[0].size      = sizeof(CHITAGDATA);
            gpuTagData[0].requestId = requestId;
            gpuTagData[0].pData     = &perspectiveAndGridTransforms.grid;
            gpuTagData[0].dataSize  = sizeof(IPEICAGridTransform);

            metadataInfo.size       = sizeof(CHIMETADATAINFO);
            metadataInfo.chiSession = m_hChiSession;
            metadataInfo.tagNum     = gpuTagSize;
            metadataInfo.pTagList   = &gpuTagList[0];
            metadataInfo.pTagData   = &gpuTagData[0];

            // publish for dewarp gpu deployment type
            g_ChiNodeInterface.pSetMetadata(&metadataInfo);
        }
        else
        {
            UINT psTagList[] =
            {
                g_vendorTagId.ICAInPerspectiveTransformTagId,
                g_vendorTagId.ICAInGridOut2InTransformTagId,
                g_vendorTagId.ICAReferenceParamsTagId,
            };

            const UINT32     psTagSize = sizeof(psTagList) / sizeof(psTagList[0]);
            CHITAGDATA       psTagData[psTagSize] = { { 0 } };

            // Publish EISv3 Perspective transform to ICA using PS metadata
            psTagData[index].size      = sizeof(CHITAGDATA);
            psTagData[index].requestId = requestId;
            psTagData[index].pData     = &perspectiveAndGridTransforms.perspective;
            psTagData[index].dataSize  = sizeof(IPEICAPerspectiveTransform);
            index++;

            // Publish EISv3 grid out2in transform to ICA using PS metadata
            psTagData[index].size      = sizeof(CHITAGDATA);
            psTagData[index].requestId = requestId;
            psTagData[index].pData     = &perspectiveAndGridTransforms.grid;
            psTagData[index].dataSize  = sizeof(IPEICAGridTransform);
            index++;

            // Publish EISv3 gyro alignment matrix to IPE
            psTagData[index].size      = sizeof(CHITAGDATA);
            psTagData[index].requestId = requestId;
            psTagData[index].pData     = &perspectiveAndGridTransforms.alignmentMatrix;
            psTagData[index].dataSize  = sizeof(IPEICAPerspectiveTransform);
            index++;

            CHIPSMETADATA psmetadataInfo;
            memset(&psmetadataInfo, 0x0, sizeof(CHIPSMETADATA));

            psmetadataInfo.size       = sizeof(CHIPSMETADATA);
            psmetadataInfo.chiSession = m_hChiSession;
            psmetadataInfo.tagCount   = psTagSize;
            psmetadataInfo.pTagList   = &psTagList[0];
            psmetadataInfo.pTagData   = &psTagData[0];
            psmetadataInfo.portId     = FullPath;

            CHIPSMETADATABYPASSINFO psbypassinfo;
            memset(&psbypassinfo, 0x0, sizeof(CHIPSMETADATABYPASSINFO));

            psbypassinfo.size       = sizeof(CHIPSMETADATABYPASSINFO);
            psbypassinfo.chiSession = m_hChiSession;

            g_ChiNodeInterface.pSetPSMetadata(&psmetadataInfo);
            for (UINT32 i = 0; i < psTagSize; i++)
            {
                g_ChiNodeInterface.pPublishPSMetadata(psTagList[i], &psbypassinfo);
            }
        }
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Unable to publish ICA matrix from EISv3");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV2Node::GetVirtualMargin
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiEISV3Node::GetMinTotalMargins(
    FLOAT* minTotalMarginX,
    FLOAT* minTotalMarginY)
{
    eis_1_2_0::chromatix_eis12Type*                        pEISChromatix    = NULL;
    eis_1_2_0::chromatix_eis12_reserveType::topStruct*     pTopStruct       = NULL;

    if (NULL != m_perSensorData[m_primarySensorIdx].pEISChromatix)
    {
        pEISChromatix               = m_perSensorData[m_primarySensorIdx].pEISChromatix;
        pTopStruct                  = &pEISChromatix->chromatix_eis12_reserve.top;
        *minTotalMarginX            = pTopStruct->minimal_total_margin;
        *minTotalMarginY            = pTopStruct->minimal_total_margin;
    }
    else
    {
        *minTotalMarginX = EISV3Margin;
        *minTotalMarginY = EISV3Margin;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::GetTotalMarginsAndFrameDelay
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiEISV3Node::GetTotalMarginsAndFrameDelay(
    const EISV3OverrideSettings* overrideSettings,
    FLOAT*  pTotalMarginX,
    FLOAT*  pTotalMarginY,
    UINT32* pFrameDelay)
{
    UINT32                                              targetFPS       = 30;
    ChiBufferDimension                                  outDimension    = { 0 };
    VOID*                                               pData           = NULL;
    eis_1_2_0::chromatix_eis12Type*                     pEISChromatix   = NULL;
    eis_1_2_0::chromatix_eis12_reserveType::topStruct*  pTopStruct      = NULL;

    pData = ChiNodeUtils::GetMetaData(0,
                                      (g_vendorTagId.targetFPSTagId | UsecaseMetadataSectionMask),
                                      ChiMetadataDynamic,
                                      &g_ChiNodeInterface,
                                      m_hChiSession);
    if (NULL != pData)
    {
        LOG_ERROR(CamxLogGroupChi, "Target fps is %d", *reinterpret_cast<UINT32*>(pData));
        targetFPS   = *reinterpret_cast<UINT32*>(pData);
        pData       = NULL;
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "No target fps, use default 30");
    }

    pData = ChiNodeUtils::GetMetaData(0,
                                      (g_vendorTagId.previewStreamDimensionsTagId | UsecaseMetadataSectionMask),
                                      ChiMetadataDynamic,
                                      &g_ChiNodeInterface,
                                      m_hChiSession);
    if (NULL != pData)
    {
        outDimension    = *static_cast<ChiBufferDimension*>(pData);
        pData           = NULL;

        LOG_INFO(CamxLogGroupChi, "Preview dimension %d, %d", outDimension.width, outDimension.height);
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Get preview dimension failed");
    }

    pData = ChiNodeUtils::GetMetaData(0,
                                      (g_vendorTagId.videoStreamDimensionsTagId | UsecaseMetadataSectionMask),
                                      ChiMetadataDynamic,
                                      &g_ChiNodeInterface,
                                      m_hChiSession);
    if (NULL != pData)
    {
        ChiBufferDimension videoDimension   = *static_cast<ChiBufferDimension*>(pData);
        pData                               = NULL;

        LOG_INFO(CamxLogGroupChi, "Video dimension %d, %d", videoDimension.width, videoDimension.height);

        if (outDimension.width * outDimension.height < videoDimension.width * videoDimension.height)
        {
            outDimension = videoDimension;
        }
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Get video dimension failed");
    }

    if (NULL != m_perSensorData[m_primarySensorIdx].pEISChromatix)
    {
        pEISChromatix   = m_perSensorData[m_primarySensorIdx].pEISChromatix;
        pTopStruct      = &pEISChromatix->chromatix_eis12_reserve.top;

        if (1080 >= outDimension.height)
        {
            if (30 >= targetFPS)
            {
                *pTotalMarginY  = pTopStruct->requested_total_margins_y_fhd_30;
                *pFrameDelay    = pTopStruct->future_buffer_size_fhd_30;
            }
            else if (60 >= targetFPS)
            {
                *pTotalMarginY  = pTopStruct->requested_total_margins_y_fhd_60;
                *pFrameDelay    = pTopStruct->future_buffer_size_fhd_60;
            }
            else
            {
                *pTotalMarginY  = pTopStruct->requested_total_margins_y_fhd_120;
                *pFrameDelay    = pTopStruct->future_buffer_size_fhd_120;
            }
        }
        else if (2160 >= outDimension.height)
        {
            if (30 >= targetFPS)
            {
                *pTotalMarginY  = pTopStruct->requested_total_margins_y_4k_30;
                *pFrameDelay    = pTopStruct->future_buffer_size_4k_30;
            }
            else if (60 >= targetFPS)
            {
                *pTotalMarginY  = pTopStruct->requested_total_margins_y_4k_60;
                *pFrameDelay    = pTopStruct->future_buffer_size_4k_60;
            }
            else
            {
                *pTotalMarginY  = pTopStruct->requested_total_margins_y_4k_120;
                *pFrameDelay    = pTopStruct->future_buffer_size_4k_120;
            }
        }
        else
        {
            if (30 >= targetFPS)
            {
                *pTotalMarginY  = pTopStruct->requested_total_margins_y_8k_30;
                *pFrameDelay    = pTopStruct->future_buffer_size_8k_30;
            }
            else if (60 >= targetFPS)
            {
                *pTotalMarginY  = pTopStruct->requested_total_margins_y_8k_60;
                *pFrameDelay    = pTopStruct->future_buffer_size_8k_60;
            }
            else
            {
                *pTotalMarginY  = pTopStruct->requested_total_margins_y_8k_120;
                *pFrameDelay    = pTopStruct->future_buffer_size_8k_120;
            }
        }

        *pTotalMarginX = ChiNodeUtils::MinFLOAT(pTopStruct->minimal_total_margin, *pTotalMarginY);
    }
    else
    {
        // Use default values in case opening Chromatix failed
        LOG_INFO(CamxLogGroupChi, "Failed to open EIS Chromatix, using default frame delay and margins values");
        *pFrameDelay        = DefaultEISV3FrameDelay;
        *pTotalMarginX      = EISV3Margin;
        *pTotalMarginY      = EISV3Margin;
    }

    //TODO: remove this constraint once memory issues is solved
    if (MaxEISV3FrameDelay < *pFrameDelay)
    {
        *pFrameDelay = MaxEISV3FrameDelay;
    }

    // Override Chromatix values by using the overrideSettings file
    if (-1 != overrideSettings->margins.widthMargin)    // If margins were read from overrideSettings file use it instead
    {
        *pTotalMarginX = overrideSettings->margins.widthMargin;
    }

    if (-1 != overrideSettings->margins.heightMargin)   // If margins were read from overrideSettings file use it instead
    {
        *pTotalMarginY = overrideSettings->margins.heightMargin;
    }

    if (0 != overrideSettings->frameDelay)  //If frame delay was read from overrideSettings file use it instead
    {
        *pFrameDelay = overrideSettings->frameDelay;
    }

    // Convert requested margins from input to margins from output ( send as a margins request tag )
    LOG_INFO(CamxLogGroupChi, "Chromatix total margins from input: (%f, %f) from output: (%f, %f), frame delay %d",
             *pTotalMarginX,
             *pTotalMarginY,
             (1 / (1 - *pTotalMarginX) - 1),
             (1 / (1 - *pTotalMarginY) - 1),
             *pFrameDelay);

    *pTotalMarginX = 1 / (1 - *pTotalMarginX) - 1;
    *pTotalMarginY = 1 / (1 - *pTotalMarginY) - 1;

    m_lookahead              = *pFrameDelay;
    m_EISV3RequestQueueDepth = m_lookahead + DefaultRequestQueueDepth;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::GetGyroFrequency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FLOAT ChiEISV3Node::GetGyroFrequency(
    UINT32 sensorIndex)
{
    FLOAT                                                  gyroFrequency    = GyroSamplingRate;
    eis_1_2_0::chromatix_eis12Type*                        pEISChromatix    = NULL;
    eis_1_2_0::chromatix_eis12_reserveType::topStruct*     pTopStruct       = NULL;

    if (NULL != m_perSensorData[sensorIndex].pEISChromatix)
    {
        pEISChromatix   = m_perSensorData[sensorIndex].pEISChromatix;
        pTopStruct      = &pEISChromatix->chromatix_eis12_reserve.top;
        gyroFrequency   = static_cast<FLOAT>(pTopStruct->gyro_frequency);
    }

    return ((gyroFrequency == 0) || (gyroFrequency > GyroSamplingRate)) ? GyroSamplingRate : gyroFrequency;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::GetAdditionalCropOffset
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiEISV3Node::GetAdditionalCropOffset()
{
    UINT32 width  = 0;
    UINT32 height = 0;
    m_additionalCropOffset.widthPixels = 0;
    m_additionalCropOffset.heightLines = 0;

    // get the physical margin
    width  = m_perSensorData[m_primarySensorIdx].inputSize.width  - m_perSensorData[m_primarySensorIdx].outputSize.width;
    height = m_perSensorData[m_primarySensorIdx].inputSize.height - m_perSensorData[m_primarySensorIdx].outputSize.height;

    width  = ChiNodeUtils::AlignGeneric32(width, 2);
    height = ChiNodeUtils::AlignGeneric32(height, 2);

    if ((m_stabilizationMargins.widthPixels >= width) && (m_stabilizationMargins.heightLines >= height))
    {
        m_additionalCropOffset.widthPixels = m_stabilizationMargins.widthPixels - width;
        m_additionalCropOffset.heightLines = m_stabilizationMargins.heightLines - height;
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Unexpected actual margin: %ux%u", width, height);
    }
    LOG_VERBOSE(CamxLogGroupChi, "Additional Crop Offset: %ux%u",
                m_additionalCropOffset.widthPixels, m_additionalCropOffset.heightLines);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::ConvertICA20GridToICA10Grid
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV3Node::ConvertICA20GridToICA10Grid(
    NcLibWarpGrid *pInICA20Grid,
    NcLibWarpGrid *pOutICA10Grid)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pInICA20Grid) || (NULL == pOutICA10Grid))
    {
        LOG_ERROR(CamxLogGroupChi, "Grid conversion failed %p, %p", pInICA20Grid, pOutICA10Grid);
        result = CDKResultEInvalidPointer;
    }

    if ((CDKResultSuccess == result) && (TRUE == pInICA20Grid->enable))
    {
        if ((NULL == pInICA20Grid->grid) ||
            (NULL == pOutICA10Grid->grid) ||
            (ICA20GridTransformHeight != pInICA20Grid->numRows) ||
            (ICA20GridTransformWidth != pInICA20Grid->numColumns) ||
            (EXTRAPOLATION_TYPE_EXTRA_POINT_ALONG_PERIMETER != pInICA20Grid->extrapolateType))
        {
            LOG_ERROR(CamxLogGroupChi, "Grid conversion invalid input, ICA20 grid %p, ICA10 Grid %p, rows %d, columns %d, %d",
                      pInICA20Grid->grid,
                      pOutICA10Grid->grid,
                      pInICA20Grid->numRows,
                      pInICA20Grid->numColumns,
                      pInICA20Grid->extrapolateType);
            result = CDKResultEInvalidArg;
        }

        if (CDKResultSuccess == result)
        {
            pOutICA10Grid->enable               = pInICA20Grid->enable;
            pOutICA10Grid->transformDefinedOn   = pInICA20Grid->transformDefinedOn;
            pOutICA10Grid->numRows              = ICA10GridTransformHeight;
            pOutICA10Grid->numColumns           = ICA10GridTransformWidth;

            if (NULL != pOutICA10Grid->gridExtrapolate)
            {
                pOutICA10Grid->extrapolateType = EXTRAPOLATION_TYPE_FOUR_CORNERS;

                // Top-left corner
                pOutICA10Grid->gridExtrapolate[0] = pInICA20Grid->grid[0];

                // Top-right corner
                pOutICA10Grid->gridExtrapolate[1] = pInICA20Grid->grid[ICA20GridTransformWidth * 1 - 1];

                // Bottom-left corner
                pOutICA10Grid->gridExtrapolate[2] = pInICA20Grid->grid[ICA20GridTransformWidth * (ICA20GridTransformHeight - 1)];

                // Bottom-right corner
                pOutICA10Grid->gridExtrapolate[3] = pInICA20Grid->grid[ICA20GridTransformWidth * ICA20GridTransformHeight - 1];
            }
            else
            {
                pOutICA10Grid->extrapolateType = EXTRAPOLATION_TYPE_NONE;
            }

            UINT32 indexICA10 = 0;
            UINT32 indexICA20 = ICA20GridTransformWidth + 1;

            for (UINT32 row = 0; row < ICA10GridTransformHeight; row++)
            {
                for (UINT32 column = 0; column < ICA10GridTransformWidth; column++)
                {
                    // optimized indexing
                    pOutICA10Grid->grid[indexICA10] = pInICA20Grid->grid[indexICA20];
                    indexICA10++;
                    indexICA20++;
                }
                indexICA20 += 2;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::setICAGridGeometryVersion
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiEISV3Node::getICAGridGeometryVersion(
    uint32_t         gridNumRow,
    uint32_t         gridNumColumn,
    ICAGridGeometry* pICAGridGeometry)
{
    if ((ICA10GridTransformWidth == gridNumColumn) && (ICA10GridTransformHeight == gridNumRow))
    {
        *pICAGridGeometry = ICAGeometryCol33Row25;
    }
    else if ((ICA20GridTransformWidth == gridNumColumn) && (ICA20GridTransformHeight == gridNumRow))
    {
        *pICAGridGeometry = ICAGeometryCol35Row27;
    }
    else if ((ICA30GridTransformWidth == gridNumColumn) && (ICA30GridTransformHeight == gridNumRow))
    {
        *pICAGridGeometry = ICAGeometryCol67Row51;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::FillDefaultGridTransform
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiEISV3Node::FillDefaultGridTransform(
    IPEICAPerspectiveTransform* pPerspectiveTrans,
    IPEICAGridTransform*        pPerspectiveGrid,
    IPEICAPerspectiveTransform* pAlignmentMatrix,
    UINT32                      sensorIndex)
{
    pPerspectiveTrans->perspectiveTransformEnable       = TRUE;
    pPerspectiveTrans->perspectiveConfidence            = 1;
    pPerspectiveTrans->byPassAlignmentMatrixAdjustement = TRUE;
    pPerspectiveTrans->perspetiveGeometryNumColumns     = 1;
    pPerspectiveTrans->perspectiveGeometryNumRows       = 1;

    //TBD: Change from active sensor to what was passed to the algo
    pPerspectiveTrans->transformDefinedOnWidth      = m_perSensorData[sensorIndex].inputSize.width;
    pPerspectiveTrans->transformDefinedOnHeight     = m_perSensorData[sensorIndex].inputSize.height;
    pPerspectiveTrans->ReusePerspectiveTransform    = 0;
    memcpy(&pPerspectiveTrans->perspectiveTransformArray, &perspArray,
           sizeof(NcLibPerspTransformSingle));

    pAlignmentMatrix->perspectiveTransformEnable        = TRUE;
    pAlignmentMatrix->perspectiveConfidence             = 1;
    pAlignmentMatrix->byPassAlignmentMatrixAdjustement  = TRUE;
    pAlignmentMatrix->perspetiveGeometryNumColumns      = 1;
    pAlignmentMatrix->perspectiveGeometryNumRows        = 1;

    //TBD: Change from active sensor to what was passed to the algo
    pAlignmentMatrix->transformDefinedOnWidth       = m_perSensorData[sensorIndex].inputSize.width;
    pAlignmentMatrix->transformDefinedOnHeight      = m_perSensorData[sensorIndex].inputSize.height;
    pAlignmentMatrix->ReusePerspectiveTransform     = 0;
    memcpy(&pAlignmentMatrix->perspectiveTransformArray, &perspArray,
           sizeof(NcLibPerspTransformSingle));

    // Fill identity grid
    pPerspectiveGrid->gridTransformEnable = TRUE;
    pPerspectiveGrid->reuseGridTransform  = 0;

    //TBD: Change from active sensor to what was passed to the algo
    pPerspectiveGrid->transformDefinedOnWidth               = IcaVirtualDomainWidth  * IcaVirtualDomainQuantizationV20;
    pPerspectiveGrid->transformDefinedOnHeight              = IcaVirtualDomainHeight * IcaVirtualDomainQuantizationV20;
    pPerspectiveGrid->gridTransformArrayExtrapolatedCorners = FALSE;
    switch (m_ICAVersion)
    {
        case ChiICA30:
            pPerspectiveGrid->geometry = ICAGeometryCol67Row51;
            for (UINT idx = 0; idx < (ICA30GridTransformWidth * ICA30GridTransformHeight); idx++)
            {
                pPerspectiveGrid->gridTransformArray[idx].x = gridArrayX30[idx];
                pPerspectiveGrid->gridTransformArray[idx].y = gridArrayY30[idx];
            }
            break;
        case ChiICA20:
            pPerspectiveGrid->geometry = ICAGeometryCol35Row27;
            for (UINT idx = 0; idx < (ICA20GridTransformWidth * ICA20GridTransformHeight); idx++)
            {
                pPerspectiveGrid->gridTransformArray[idx].x = gridArrayX20[idx];
                pPerspectiveGrid->gridTransformArray[idx].y = gridArrayY20[idx];
            }
            break;
        case ChiICA10:
            pPerspectiveGrid->geometry = ICAGeometryCol33Row25;
            for (UINT idx = 0; idx < (ICA10GridTransformWidth * ICA10GridTransformHeight); idx++)
            {
                pPerspectiveGrid->gridTransformArray[idx].x = gridArrayX[idx];
                pPerspectiveGrid->gridTransformArray[idx].y = gridArrayY[idx];
            }
            break;
        case ChiICAMax:
            pPerspectiveGrid->gridTransformEnable = FALSE;
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::GetCameraIndexFromID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiEISV3Node::GetCameraIndexFromID(UINT32 cameraId)
{
    UINT32 camIndex = InvalidIndex;

    for (UINT32 i = 0; i < m_numOfLinkedCameras; i++)
    {
        if (m_perSensorData[i].cameraConfig.cameraId == cameraId)
        {
            camIndex = i;
            break;
        }
    }
    return camIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::GetPerCameraConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV3Node::GetPerCameraConfig()
{
    CDKResult result = CDKResultSuccess;
    void*     pData = ChiNodeUtils::GetMetaData(0,
                                                g_vendorTagId.physicalCameraConfigsTagId | UsecaseMetadataSectionMask,
                                                ChiMetadataDynamic,
                                                &g_ChiNodeInterface,
                                                m_hChiSession);
    if (NULL != pData)
    {
        CameraConfigs* pPhysicalCameraConfigs = static_cast<CameraConfigs*>(pData);
        LOG_VERBOSE(CamxLogGroupChi, "number of physical cameras %d", pPhysicalCameraConfigs->numPhysicalCameras);

        if (MaxMulticamSensors >= pPhysicalCameraConfigs->numPhysicalCameras)
        {
            if (1 != m_numOfLinkedCameras)
            {
                CAMX_ASSERT(m_numOfLinkedCameras == pPhysicalCameraConfigs->numPhysicalCameras);
            }

            m_numOfLinkedCameras    = pPhysicalCameraConfigs->numPhysicalCameras;
            m_primaryCameraId       = pPhysicalCameraConfigs->primaryCameraId;

            for (UINT32 i = 0; i < m_numOfLinkedCameras; i++)
            {
                m_perSensorData[i].cameraConfig = pPhysicalCameraConfigs->cameraConfigs[i];
            }

            m_primarySensorIdx = GetCameraIndexFromID(m_primaryCameraId);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi,
                      "Number of linked cameras %d, exceeds max supported linked cameras %d ",
                      pPhysicalCameraConfigs->numPhysicalCameras, MaxLinkedCameras);
            result = CDKResultEFailed;
        }
    }
    else
    {
        m_numOfLinkedCameras                        = 1;
        m_perSensorData[0].cameraConfig.cameraId    = 0;
        m_primaryCameraId                           = m_perSensorData[0].cameraConfig.cameraId;
        m_primarySensorIdx                          = GetCameraIndexFromID(m_primaryCameraId);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::GetDeploymentType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
cam_is_deployment_type_t ChiEISV3Node::GetDeploymentType()
{
    cam_is_deployment_type_t deploymentType = DEP_TYPE_ICA_V20;
    switch (m_ICAVersion)
    {
    case ChiICA10:
        deploymentType = DEP_TYPE_ICA_V20;
        break;
    case ChiICA20:
        deploymentType = DEP_TYPE_ICA_V20;
        break;
    case ChiICA30:
        deploymentType = DEP_TYPE_ICA_V30;
        break;
    case ChiICAMax:
        deploymentType = DEP_TYPE_GPU_PRE;
        break;
        //TODO: handle Post Ipe case
    }

    return deploymentType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV3Node::GetSensorAppliedCrop
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
WindowRegionF ChiEISV3Node::GetSensorAppliedCrop(UINT32 sensorIdx)
{
    WindowRegionF sensorAppliedCropWindow;

    sensorAppliedCropWindow.fullWidth    = m_perSensorData[sensorIdx].activeArraySize.width;
    sensorAppliedCropWindow.fullHeight   = m_perSensorData[sensorIdx].activeArraySize.height;
    sensorAppliedCropWindow.windowLeft   = m_perSensorData[sensorIdx].cameraConfig.sensorModeInfo.activeArrayCropWindow.left;
    sensorAppliedCropWindow.windowTop    = m_perSensorData[sensorIdx].cameraConfig.sensorModeInfo.activeArrayCropWindow.top;
    sensorAppliedCropWindow.windowWidth  = m_perSensorData[sensorIdx].cameraConfig.sensorModeInfo.activeArrayCropWindow.width;
    sensorAppliedCropWindow.windowHeight = m_perSensorData[sensorIdx].cameraConfig.sensorModeInfo.activeArrayCropWindow.height;

    return sensorAppliedCropWindow;
}
