////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodeeisv2.cpp
/// @brief Chi node for EISV2
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <system/camera_metadata.h>
#include "camxchinodeeisv2.h"
#include "chi.h"
#include "chincsdefs.h"
#include "eis_1_2_0.h"
#include "inttypes.h"
#include "parametertuningtypes.h"
#include "camxipeicatestdata.h"
#include "chistatsproperty.h"

// NOWHINE FILE CP040: Keyword new not allowed. Use the CAMX_NEW/CAMX_DELETE functions insteads

#undef LOG_TAG
#define LOG_TAG         "CHIEISV2"
#define VIDEO4KWIDTH    3840

ChiNodeInterface    g_ChiNodeInterface;         ///< The instance of the CAMX Chi interface
UINT32              g_vendorTagBase     = 0;    ///< Chi assigned runtime vendor tag base for the node

const CHAR* pEIS2LibName    = "com.qti.eisv2";

/// @todo (CAMX-1854) the major / minor version shall get from CHI
static const UINT32 ChiNodeMajorVersion = 0;    ///< The major version of CHI interface
static const UINT32 ChiNodeMinorVersion = 0;    ///< The minor version of CHI interface

static const UINT32 ChiNodeCapsEISv2       = 2;                       ///< Supports EIS2.0 algorithm
static const CHAR   EISv2NodeSectionName[] = "com.qti.node.eisv2";    ///< The section name for node

static const UINT8  NumICA10Exterpolate    = 4;        ///< Num ICA10 exterpolate corners
static const FLOAT  GyroSamplingRate       = 416.0f;   ///< Gyro Sampling rate

///< EIS config
static const UINT32 EISV2FrameDelay        = 0;     ///< Default Frame Delay
static const FLOAT  EISV2Margin            = 0.2F;  ///< Default Stabilization margin
static const UINT32 GYRO_SAMPLES_BUF_SIZE  = 512;   ///< Max Gyro sample size

///< max gyro dumps alone per frame * 100 chars per gyro sample line per frame info
static const UINT32 GyroDumpSize = static_cast<UINT32>(GYRO_SAMPLES_BUF_SIZE * 103 + 150);

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
static const CHAR MultiCameraInfoSectionName[]      = "com.qti.chi.multicameraoutputmetadata";
static const CHAR EISRealTimeSectionName[]          = "org.quic.camera.eisrealtime";
static const CHAR EISLookAheadSectionName[]         = "org.quic.camera.eislookahead";
static const CHAR CameraConfigurationSectionName[]  = "com.qti.chi.cameraconfiguration";
static const CHAR ChiNodeCropRegions[]              = "com.qti.cropregions";
static const CHAR StatsConfigSectionName[]          = "org.quic.camera2.statsconfigs";

///< This is an array of all vendor tag section data
static CHIVENDORTAGDATA g_VendorTagSectionEISV2Node[] =
{
    { "AlgoComplete",         0, sizeof(BOOL) }
};

///< This is an array of all vendor tag section data
static CHIVENDORTAGSECTIONDATA g_VendorTagEISV2NodeSection[] =
{
    {
        EISv2NodeSectionName,  0,
        sizeof(g_VendorTagSectionEISV2Node) / sizeof(g_VendorTagSectionEISV2Node[0]), g_VendorTagSectionEISV2Node,
        CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    }
};

///< This is an array of all vendor tag section data
static ChiVendorTagInfo g_VendorTagInfoEISV2Node[] =
{
    {
        &g_VendorTagEISV2NodeSection[0],
        sizeof(g_VendorTagEISV2NodeSection) / sizeof(g_VendorTagEISV2NodeSection[0])
    }
};

///< Vendor tags of interest
static CHIVENDORTAGDATA g_VendorTagSectionICA[] =
{
    { "ICAInPerspectiveTransform" ,   0, sizeof(IPEICAPerspectiveTransform) }, ///< ICA Ref perspective transform
    { "ICAInGridOut2InTransform"   ,  0, sizeof(IPEICAGridTransform) },        ///< ICA Grid out2in transform
    { "ICAInGridIn2OutTransform"   ,  0, sizeof(IPEICAGridTransform) },        ///< ICA Grid in2out transform
    { "ICAReferenceParams"         ,  0, sizeof(IPEICAPerspectiveTransform) }, ///< ICA reference params for gyro alignment
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

static CHIVENDORTAGDATA g_VendorTagSectionEISRealTime[] =
{
    { "Enabled",              0, 1 },
    { "RequestedMargin",      0, sizeof(MarginRequest) },
    { "StabilizationMargins", 0, sizeof(StabilizationMargin) },
    { "AdditionalCropOffset", 0, sizeof(ImageDimensions) },
    { "StabilizedOutputDims", 0, sizeof(CHIDimension) },
    { "MinimalTotalMargins",  0, sizeof(MarginRequest) }
};

static CHIVENDORTAGDATA g_VendorTagSectionEISLookahead[] =
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

static CHIVENDORTAGDATA g_VendorTagSectionStatsConfig[] =
{
    { "FOVCFrameControl", 0, sizeof(FOVCOutput) },
};

EISV2VendorTags g_vendorTagId = { 0 };

static const UINT32 MaxDimension       = 0xffff;  //< Max dimenion for input/output port
static const UINT32 LUTSize            = 32;      //< Look up table size
static const UINT32 MaxPerspMatrixSize = 9;       //< Max size if the perspective matrix

static const NodeProperty NodePropertyEIS2InputPortType = NodePropertyVendorStart;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EISV2NodeGetCaps
///
/// @brief  Implementation of PFNNODEGETCAPS defined in chinode.h
///
/// @param  pCapsInfo   Pointer to a structure that defines the capabilities supported by the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult EISV2NodeGetCaps(
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
            pCapsInfo->nodeCapsMask = ChiNodeCapsEISv2;
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
/// EISV2NodeQueryVendorTag
///
/// @brief  Implementation of PFNCHIQUERYVENDORTAG defined in chinode.h
///
/// @param  pQueryVendorTag Pointer to a structure that returns the exported vendor tag
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult EISV2NodeQueryVendorTag(
    CHIQUERYVENDORTAG* pQueryVendorTag)
{
    CDKResult result = CDKResultSuccess;
    result = ChiNodeUtils::QueryVendorTag(pQueryVendorTag, g_VendorTagInfoEISV2Node);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EISV2GetOverrideSettings
///
/// @brief  Function to fetch EISv2 overridesetting
///
/// @param  pOverrideSettings Pointer to overridesettings.
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EISV2GetOverrideSettings(
    EISV2OverrideSettings* pOverrideSettings)
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
                LOG_INFO(CamxLogGroupChi, "EISv2 width Margin %f", pOverrideSettings->margins.widthMargin);
            }
            else if (0 == strcmp("EISHeightMargin", pSettingString))
            {
                FLOAT heightMargin                      = atof(pValueString);
                pOverrideSettings->margins.heightMargin = heightMargin;
                LOG_INFO(CamxLogGroupChi, "EISv2 height Margin %f", pOverrideSettings->margins.heightMargin);
            }
            else if (0 == strcmp("EISLDCGridEnabled", pSettingString))
            {
                pOverrideSettings->isLDCGridEnabled = (atoi(pValueString) == 1) ? 1 : 0;
                LOG_INFO(CamxLogGroupChi, "EISv2 LDC grid enabled %d", pOverrideSettings->isLDCGridEnabled);
            }
            else if (0 == strcmp("EISv2GyroDumpEnabled", pSettingString))
            {
                pOverrideSettings->isEnabledDumpInputFile = (atoi(pValueString) == 1) ? 1 : 0;
                LOG_INFO(CamxLogGroupChi, "EISv2 gyro dump to file enabled %d", pOverrideSettings->isEnabledDumpInputFile);
            }
            else if (0 == strcmp("EISv2InputDumpLogcatEnabled", pSettingString))
            {
                pOverrideSettings->isEnabledDumpInputLogcat = (atoi(pValueString) == 1) ? 1 : 0;
                LOG_INFO(CamxLogGroupChi, "EISv2 gyro dump to log enabled %d", pOverrideSettings->isEnabledDumpInputLogcat);
            }
            else if (0 == strcmp("EISv2OutputDumpEnabled", pSettingString))
            {
                pOverrideSettings->isEnabledDumpOutputFile = (atoi(pValueString) == 1) ? 1 : 0;
                LOG_INFO(CamxLogGroupChi, "EISv2 output dump to file enabled %d", pOverrideSettings->isEnabledDumpOutputFile);
            }
            else if (0 == strcmp("EISv2OutputDumpLogcatEnabled", pSettingString))
            {
                pOverrideSettings->isEnabledDumpOutputLogcat = (atoi(pValueString) == 1) ? 1 : 0;
                LOG_INFO(CamxLogGroupChi, "EISv2 output dump to log enabled %d", pOverrideSettings->isEnabledDumpOutputLogcat);
            }
            else if (0 == strcmp("EISv2OperationMode", pSettingString))
            {
                INT opMode = atoi(pValueString);
                pOverrideSettings->algoOperationMode = (cam_is_operation_mode_t)opMode;
                LOG_INFO(CamxLogGroupChi, "EISv2 operation mode %d", pOverrideSettings->algoOperationMode);
            }
            else if (0 == strcmp("EISDefaultGridTransformEnable", pSettingString))
            {
                pOverrideSettings->isDefaultGridTransformEnabled = (atoi(pValueString) == 1) ? 1 : 0;
                LOG_INFO(CamxLogGroupChi, "EIS default grid/transform enabled %d",
                         pOverrideSettings->isDefaultGridTransformEnabled);
            }
            else if (0 == strcmp("EISv2DumpForceFlush", pSettingString))
            {
                pOverrideSettings->isEnabledDumpForceFlush = (atoi(pValueString) == 1) ? 1 : 0;
                LOG_INFO(CamxLogGroupChi, "EISv2 output dump force flush enabled %d",
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
            LOG_WARN(CamxLogGroupChi, "EISv2 output dump force flush is enabled, performance degradation could occur");
        }

        ChiNodeUtils::FClose(pEISSettingsTextFile);
        pEISSettingsTextFile = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EISV2NodeCreate
///
/// @brief  Implementation of PFNNODECREATE defined in chinode.h
///
/// @param  pCreateInfo Pointer to a structure that defines create session information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult EISV2NodeCreate(
    CHINODECREATEINFO* pCreateInfo)
{
    CDKResult             result             = CDKResultSuccess;
    ChiEISV2Node*         pNode              = NULL;
    EISV2OverrideSettings overrideSettings   = { { 0 } };

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
        overrideSettings.algoOperationMode                    = IS_OPT_REGULAR;
        overrideSettings.isLDCGridEnabled                     = TRUE;
        overrideSettings.isDefaultGridTransformEnabled        = FALSE;
        overrideSettings.isEnabledDumpInputFile               = FALSE;
        overrideSettings.isEnabledDumpOutputFile              = FALSE;
        overrideSettings.isEnabledDumpInputLogcat             = FALSE;
        overrideSettings.isEnabledDumpOutputLogcat            = FALSE;
        overrideSettings.isEnabledDumpForceFlush              = FALSE;

#if !_WINDOWS
        EISV2GetOverrideSettings(&overrideSettings);
#endif

        pNode = new ChiEISV2Node(overrideSettings);
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

        CHIMETADATAINFO      metadataInfo     = { 0 };
        const UINT32         tagSize          = 3;
        UINT32               index            = 0;
        CHITAGDATA           tagData[tagSize] = { {0} };
        UINT32               tagList[tagSize];
        BOOL                 enabled          = TRUE;
        MarginRequest        requestedMargin  = overrideSettings.margins;
        MarginRequest        minTotalMargin;

        pNode->GetTotalMargins(&overrideSettings, &requestedMargin.widthMargin, &requestedMargin.heightMargin);
        pNode->GetMinTotalMargins(&minTotalMargin.widthMargin, &minTotalMargin.heightMargin);

        metadataInfo.size       = sizeof(CHIMETADATAINFO);
        metadataInfo.chiSession = pCreateInfo->hChiSession;
        metadataInfo.tagNum     = tagSize;
        metadataInfo.pTagList   = &tagList[0];
        metadataInfo.pTagData   = &tagData[0];

        tagList[index]           = (g_vendorTagId.EISV2EnabledTagId | UsecaseMetadataSectionMask);
        tagData[index].size      = sizeof(CHITAGDATA);
        tagData[index].requestId = 0;
        tagData[index].pData     = &enabled;
        tagData[index].dataSize  = g_VendorTagSectionEISRealTime[0].numUnits;
        index++;

        tagList[index]           = (g_vendorTagId.EISV2RequestedMarginTagId | UsecaseMetadataSectionMask);
        tagData[index].size      = sizeof(CHITAGDATA);
        tagData[index].requestId = 0;
        tagData[index].pData     = &requestedMargin;
        tagData[index].dataSize  = g_VendorTagSectionEISRealTime[1].numUnits;
        index++;

        tagList[index]           = (g_vendorTagId.EISV2MinimalTotalMarginTagId | UsecaseMetadataSectionMask);
        tagData[index].size      = sizeof(CHITAGDATA);
        tagData[index].requestId = 0;
        tagData[index].pData     = &minTotalMargin;
        tagData[index].dataSize  = g_VendorTagSectionEISRealTime[5].numUnits;
        index++;

        LOG_VERBOSE(CamxLogGroupChi,
                    "Publishing EISv2 enabled flag %d, requested margin x = %f, y = %f, "
                    "min total margin w %f h %f",
                    enabled,
                    requestedMargin.widthMargin,
                    requestedMargin.heightMargin,
                    minTotalMargin.widthMargin,
                    minTotalMargin.heightMargin);
        g_ChiNodeInterface.pSetMetadata(&metadataInfo);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EISV2NodeDestroy
///
/// @brief  Implementation of PFNNODEDESTROY defined in chinode.h
///
/// @param  pDestroyInfo    Pointer to a structure that defines the session destroy information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult EISV2NodeDestroy(
    CHINODEDESTROYINFO* pDestroyInfo)
{
    CDKResult      result       = CDKResultSuccess;
    ChiEISV2Node*  pNode        = NULL;
    CHIDATASOURCE* pDataSource  = NULL;

    if ((NULL == pDestroyInfo) || (NULL == pDestroyInfo->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pDestroyInfo is NULL");
    }

    if (CDKResultSuccess == result)
    {
        if (pDestroyInfo->size >= sizeof(CHINODEDESTROYINFO))
        {
            pNode = static_cast<ChiEISV2Node*>(pDestroyInfo->hNodeSession);
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

    LOG_INFO(CamxLogGroupChi, "EISv2 node destroy %d", result);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EISV2NodeProcRequest
///
/// @brief  Implementation of PFNNODEPROCREQUEST defined in chinode.h
///
/// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult EISV2NodeProcRequest(
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
            ChiEISV2Node* pNode = static_cast<ChiEISV2Node*>(pProcessRequestInfo->hNodeSession);
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
/// EISV2NodePostPipelineCreate
///
/// @brief  Implementation of PFNPOSTPIPELINECREATE defined in chinode.h
///
/// @param  hChiHandle Pointer to a CHI handle
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult EISV2NodePostPipelineCreate(
    CHIHANDLE hChiHandle)
{
    CDKResult              result              = CDKResultSuccess;
    ChiEISV2Node*          pNode               = NULL;

    if (NULL == hChiHandle)
    {
        LOG_ERROR(CamxLogGroupChi, "Invalid Chi Handle");
        result = CDKResultEInvalidPointer;
    }
    else
    {
        pNode  = static_cast<ChiEISV2Node*>(hChiHandle);
        result = pNode->PostPipelineCreate();
        if (CDKResultSuccess != result)
        {
            LOG_ERROR(CamxLogGroupChi, "Failed in PostPipelineCreate, result=%d", result);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EISV2NodeSetNodeInterface
///
/// @brief  Implementation of PFCHINODESETNODEINTERFACE defined in chinode.h
///
/// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID EISV2NodeSetNodeInterface(
    ChiNodeInterface* pNodeInterface)
{
    CDKResult result = CDKResultSuccess;

    result = ChiNodeUtils::SetNodeInterface(pNodeInterface, EISv2NodeSectionName,
        &g_ChiNodeInterface, &g_vendorTagBase);

    if (CDKResultSuccess != result)
    {
        LOG_ERROR(CamxLogGroupChi, "Set Node Interface Failed");
    }
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EISV2NodeSetBufferInfo
///
/// @brief  Implementation of PFNNODESETBUFFERINFO defined in chinode.h
///
/// @param  pSetBufferInfo  Pointer to a structure with information to set the output buffer resolution and type.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult EISV2NodeSetBufferInfo(
    CHINODESETBUFFERPROPERTIESINFO* pSetBufferInfo)
{
    CDKResult     result = CDKResultSuccess;
    ChiEISV2Node* pNode  = NULL;

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
                pNode  = static_cast<ChiEISV2Node*>(pSetBufferInfo->hNodeSession);
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
/// EISV2NodeQueryBufferInfo
///
/// @brief  Implementation of PFNNODEQUERYBUFFERINFO defined in chinode.h
///
/// @param  pQueryBufferInfo    Pointer to a structure to query the input buffer resolution and type.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult EISV2NodeQueryBufferInfo(
    CHINODEQUERYBUFFERINFO* pQueryBufferInfo)
{
    CDKResult     result    = CDKResultSuccess;
    ChiEISV2Node* pNode     = NULL;

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
            LOG_ERROR(CamxLogGroupChi, "EISv2 is a pure bypass, num inputs should be equalt to num outputs");
        }
    }

    if (CDKResultSuccess == result)
    {
        if (pQueryBufferInfo->size >= sizeof(CHINODEQUERYBUFFERINFO))
        {
            pNode   = static_cast<ChiEISV2Node*>(pQueryBufferInfo->hNodeSession);
            result  = pNode->QueryBufferInfo(pQueryBufferInfo);
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
/// EISV2NodeQueryMetadataPublishList
///
/// @brief  Implementation of PFNNODEQUERYMETADATAPUBLISHLIST defined in chinode.h
///
/// @param  pMetadataPublishlist    Pointer to a structure to query the metadata list
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult EISV2NodeQueryMetadataPublishList(
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
            ChiEISV2Node* pNode = static_cast<ChiEISV2Node*>(pMetadataPublishlist->hNodeSession);
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
            pNodeCallbacks->pGetCapabilities          = EISV2NodeGetCaps;
            pNodeCallbacks->pQueryVendorTag           = EISV2NodeQueryVendorTag;
            pNodeCallbacks->pCreate                   = EISV2NodeCreate;
            pNodeCallbacks->pDestroy                  = EISV2NodeDestroy;
            pNodeCallbacks->pQueryBufferInfo          = EISV2NodeQueryBufferInfo;
            pNodeCallbacks->pSetBufferInfo            = EISV2NodeSetBufferInfo;
            pNodeCallbacks->pProcessRequest           = EISV2NodeProcRequest;
            pNodeCallbacks->pChiNodeSetNodeInterface  = EISV2NodeSetNodeInterface;
            pNodeCallbacks->pPostPipelineCreate       = EISV2NodePostPipelineCreate;
            pNodeCallbacks->pQueryMetadataPublishList = EISV2NodeQueryMetadataPublishList;
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
/// ChiEISV2Node::LoadLib
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV2Node::LoadLib()
{
    CDKResult   result          = CDKResultSuccess;
    INT         numCharWritten  = 0;
    CHAR        libFilePath[FILENAME_MAX];

    numCharWritten = ChiNodeUtils::SNPrintF(libFilePath,
        FILENAME_MAX,
        "%s%s%s.%s",
        CameraComponentLibPath,
        PathSeparator,
        pEIS2LibName,
        SharedLibraryExtension);
    LOG_INFO(CamxLogGroupChi, "loading EIS lib %s", pEIS2LibName);

    m_hEISv2Lib = ChiNodeUtils::LibMapFullName(libFilePath);

    if (NULL == m_hEISv2Lib)
    {
        LOG_ERROR(CamxLogGroupChi, "Error loading lib %s", libFilePath);
        result = CDKResultEUnableToLoad;
    }

    ///< Map function pointers
    if (CDKResultSuccess == result)
    {
        m_eis2Initialize                  = reinterpret_cast<EIS2_INITIALIZE>(
                                            ChiNodeUtils::LibGetAddr(m_hEISv2Lib,
                                                                     "eis2_initialize"));

        m_eis2Process                     = reinterpret_cast<EIS2_PROCESS>(
                                            ChiNodeUtils::LibGetAddr(m_hEISv2Lib,
                                                                     "eis2_process"));

        m_eis2Deinitialize                = reinterpret_cast<EIS2_DEINITIALIZE>(
                                            ChiNodeUtils::LibGetAddr(m_hEISv2Lib,
                                                                     "eis2_deinitialize"));

        m_eis2GetGyroTimeInterval         = reinterpret_cast<EIS2_GET_GYRO_INTERVAL>(
                                            ChiNodeUtils::LibGetAddr(m_hEISv2Lib,
                                                                     "eis2_get_gyro_time_interval"));

        m_eis2GetTotalMargin              = reinterpret_cast<EIS2_GET_TOTAL_MARGIN>(
                                            ChiNodeUtils::LibGetAddr(m_hEISv2Lib,
                                                                     "eis2_get_stabilization_margin"));

        m_eis2GetTotalMarginEx            = reinterpret_cast<EIS2_GET_TOTAL_MARGIN_EX>(
                                            ChiNodeUtils::LibGetAddr(m_hEISv2Lib,
                                                                     "eis2_get_stabilization_margin_ex"));

        m_eis2GetStabilizationCropRatioEx = reinterpret_cast<EIS2_GET_STABILIZATION_CROP_RATIO_EX>(
                                            ChiNodeUtils::LibGetAddr(m_hEISv2Lib,
                                                                     "eis2_get_stabilization_crop_ratio_ex"));

        m_eisUtilsConvertToWindowRegions  = reinterpret_cast<EIS_UTILS_CONVERT_TO_WINDOW_REGIONS>(
                                            ChiNodeUtils::LibGetAddr(m_hEISv2Lib,
                                                                     "eis_utility_convert_to_window_regions"));

        m_eisUtilsLogInit                 = reinterpret_cast<EIS_UTILS_LOG_INIT>(
                                            ChiNodeUtils::LibGetAddr(m_hEISv2Lib,
                                                                     "eis_utils_log_init"));

        m_eisUtilsLogWrite                = reinterpret_cast<EIS_UTILS_LOG_WRITE>(
                                            ChiNodeUtils::LibGetAddr(m_hEISv2Lib,
                                                                     "eis_utils_log_write"));

        m_eisUtilsLogOpen                 = reinterpret_cast<EIS_UTILS_LOG_OPEN>(
                                            ChiNodeUtils::LibGetAddr(m_hEISv2Lib,
                                                                     "eis_utils_log_open"));

        m_eisUtilsLogIsOpened             = reinterpret_cast<EIS_UTILS_LOG_IS_OPENED>(
                                            ChiNodeUtils::LibGetAddr(m_hEISv2Lib,
                                                                     "eis_utils_log_is_opened"));

        m_eisUtilsLogClose                = reinterpret_cast<EIS_UTILS_LOG_CLOSE>(
                                            ChiNodeUtils::LibGetAddr(m_hEISv2Lib,
                                                                     "eis_utils_log_close"));

        m_eisUtilsLogFlush                = reinterpret_cast<EIS_UTILS_LOG_FLUSH>(
                                            ChiNodeUtils::LibGetAddr(m_hEISv2Lib,
                                                                     "eis_utils_log_flush"));

        m_eisUtilsLogDestroy              = reinterpret_cast<EIS_UTILS_LOG_DESTROY>(
                                            ChiNodeUtils::LibGetAddr(m_hEISv2Lib,
                                                                     "eis_utils_log_destroy"));

        if ((NULL == m_eis2Initialize)                  ||
            (NULL == m_eis2Process)                     ||
            (NULL == m_eis2Deinitialize)                ||
            (NULL == m_eis2GetGyroTimeInterval)         ||
            (NULL == m_eis2GetTotalMargin)              ||
            (NULL == m_eis2GetTotalMarginEx)            ||
            (NULL == m_eis2GetStabilizationCropRatioEx) ||
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
                      "m_eis2Initialize %p, "
                      "m_eis2Process %p, "
                      "m_eis2Deinitialize %p"
                      "m_eis2GetGyroTimeInterval %p, "
                      "m_eis2GetTotalMargin %p, "
                      "m_eis2GetTotalMarginEx %p "
                      "m_eis2GetStabilizationCropRatioEx %p, "
                      "m_eisUtilsConvertToWindowRegions %p "
                      "m_eisUtilsLogInit %p, "
                      "m_eisUtilsLogWrite %p, "
                      "m_eisUtilsLogOpen %p, "
                      "m_eisUtilsLogIsOpened %p, "
                      "m_eisUtilsLogClose %p, "
                      "m_eisUtilsLogFlush %p, "
                      "m_eisUtilsLogDestroy %p ",
                      libFilePath,
                      m_eis2Initialize,
                      m_eis2Process,
                      m_eis2Deinitialize,
                      m_eis2GetGyroTimeInterval,
                      m_eis2GetTotalMargin,
                      m_eis2GetTotalMarginEx,
                      m_eis2GetStabilizationCropRatioEx,
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
/// ChiEISV2Node::UnLoadLib
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV2Node::UnLoadLib()
{
    CDKResult result = CDKResultSuccess;

    if (NULL != m_hEISv2Lib)
    {
        result = ChiNodeUtils::LibUnmap(m_hEISv2Lib);
        m_hEISv2Lib = NULL;
    }

    if (CDKResultSuccess != result)
    {
        LOG_ERROR(CamxLogGroupChi, "Uninitialize Failed to unmap lib %s: %d", pEIS2LibName, result);
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV2Node::InitializeLib
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV2Node::InitializeLib()
{
    CDKResult            result = CDKResultSuccess;
    VOID*                pData  = NULL;
    is_init_data_common  isInitDataCommon;
    is_init_data_sensor  isInitDataSensors[MaxMulticamSensors];

    if (1 == m_numOfLinkedCameras)
    {
        EISV2PerSensorData* primarySensorData = &m_perSensorData[m_primarySensorIdx];

        // Get Sensor Mount Angle
        pData = ChiNodeUtils::GetMetaData(0,
                                          ANDROID_SENSOR_ORIENTATION,
                                          ChiMetadataStatic,
                                          &g_ChiNodeInterface,
                                          m_hChiSession);
        if (NULL != pData)
        {
            LOG_INFO(CamxLogGroupChi, "sensor mount angle %d", *reinterpret_cast<UINT32*>(pData));
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
            LOG_INFO(CamxLogGroupChi, "Sensor camera position %d", *reinterpret_cast<UINT32*>(pData));
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

    isInitDataCommon.is_type                            = IS_TYPE_EIS_2_0;
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
        isInitDataSensors[i].optical_center_x       = 0;
        isInitDataSensors[i].optical_center_y       = 0;

        result = GetChromatixData(&isInitDataSensors[i], i);
    }

    if (1 < m_numOfLinkedCameras)
    {
        isInitDataCommon.is_sat_enabled = TRUE;
    }

    if (CDKResultSuccess == result)
    {
        int32_t isResult = m_eis2Initialize(&m_phEIS2Handle, &isInitDataCommon, &isInitDataSensors[0], m_numOfLinkedCameras);
        if (IS_RET_SUCCESS != isResult)
        {
            LOG_ERROR(CamxLogGroupChi, "EISV2 Algorithm Init Failed");
            m_phEIS2Handle    = NULL;
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
            LOG_ERROR(CamxLogGroupChi, "EISV2 Algorithm Init log utility Failed");
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
                    LOG_ERROR(CamxLogGroupChi, "EISV2 Algorithm re-open log utility Failed");
                }
            }
       }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV2Node::FillVendorTagIds
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV2Node::FillVendorTagIds()
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

    // Get ICA In Grid Transform Vendor Tag Id
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

    // Get ICA In Grid in2out Transform Vendor Tag Id
    result = ChiNodeUtils::GetVendorTagBase(IPEICASectionName,
                                            g_VendorTagSectionICA[2].pVendorTagName,
                                            &g_ChiNodeInterface,
                                            &vendorTagBase);
    if (CDKResultSuccess == result)
    {
        // Save ICA In Grid in2out Transform Vendor Tag Id
        g_vendorTagId.ICAInGridIn2OutTransformTagId = vendorTagBase.vendorTagBase;
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Unable to get ICA In Grid in2out Transform Vendor Tag Id");
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
        // Get EISv2 enabled flag Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(EISRealTimeSectionName,
                                                g_VendorTagSectionEISRealTime[0].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save EISv2 enabled flag Vendor Tag Id
            g_vendorTagId.EISV2EnabledTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get EISRealTime enabled flag Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get EISRealTimeSectionName Requested Margin Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(EISRealTimeSectionName,
                                                g_VendorTagSectionEISRealTime[1].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save EISv2 Requested Margin Vendor Tag Id
            g_vendorTagId.EISV2RequestedMarginTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get EISRealTime Requested Margin Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get EISRealTimeSectionName actual stabilization margins Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(EISRealTimeSectionName,
                                                g_VendorTagSectionEISRealTime[2].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save EISRealTimeSectionName actual stabilization margins Vendor Tag Id
            g_vendorTagId.EISV2StabilizationMarginsTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get EISRealTimeSectionName actual stabilization margins Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get EISRealTimeSectionName additional crop offset Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(EISRealTimeSectionName,
                                                g_VendorTagSectionEISRealTime[3].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save EISRealTimeSectionName additional crop offset Vendor Tag Id
            g_vendorTagId.EISV2AdditionalCropOffsetTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get EISRealTimeSectionName additional crop offset Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get EISRealTimeSectionName Stabilized Output Dimensions Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(EISRealTimeSectionName,
                                                g_VendorTagSectionEISRealTime[4].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save EISRealTimeSectionName Stabilized Output Dimensions Vendor Tag Id
            g_vendorTagId.EISV2StabilizedOutputDimsTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get EISRealTimeSectionName Stabilized Output Dimensions Vendor Tag Id");
        }
        // Get EISLookAheadSectionName Stabilized Output Dimensions Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(EISLookAheadSectionName,
                                                g_VendorTagSectionEISLookahead[0].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save EISLookAheadSectionName Stabilized Output Dimensions Vendor Tag Id
            g_vendorTagId.EISV2OutputDimsLookAheadTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            g_vendorTagId.EISV2OutputDimsLookAheadTagId = 0;
            LOG_WARN(CamxLogGroupChi, "Unable to get EISLookAhead Requested Margin Vendor Tag Id");
        }
    }

    if (CDKResultSuccess == result)
    {
        // Get minimal total margin Vendor Tag Id
        result = ChiNodeUtils::GetVendorTagBase(EISRealTimeSectionName,
                                                g_VendorTagSectionEISRealTime[5].pVendorTagName,
                                                &g_ChiNodeInterface,
                                                &vendorTagBase);
        if (CDKResultSuccess == result)
        {
            // Save minimal total margins Vendor Tag Id
            g_vendorTagId.EISV2MinimalTotalMarginTagId = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get minimal total margins Vendor Tag Id");
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
            g_vendorTagId.chiNodeResidualCrop = vendorTagBase.vendorTagBase;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to get Chi node residual crop Vendor Tag Id");
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
/// ChiEISV2Node::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV2Node::Initialize(
    CHINODECREATEINFO* pCreateInfo)
{
    CDKResult result = CDKResultSuccess;

    /// @todo (CAMX-1854) Check for Node Capabilities using NodeCapsMask
    m_hChiSession = pCreateInfo->hChiSession;
    m_nodeId      = pCreateInfo->nodeId;
    m_nodeCaps    = pCreateInfo->nodeCaps.nodeCapsMask;
    m_ICAVersion  = pCreateInfo->chiICAVersion;

    // Set flag to indicate chi node wrapper that this node can set its own input buffer dependencies
    pCreateInfo->nodeFlags.canSetInputBufferDependency = TRUE;

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

    // Load EISv2 lib
    if (CDKResultSuccess == result)
    {
        result = LoadLib();
        if (CDKResultSuccess != result)
        {
            LOG_ERROR(CamxLogGroupChi, "Load EISv2 algo lib failed");
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
            if (NodePropertyEIS2InputPortType == pCreateInfo->pNodeProperties[i].id)
            {
                const CHAR* temp    = reinterpret_cast<const CHAR*>(pCreateInfo->pNodeProperties[i].pValue);
                m_inputPortPathType = static_cast<EISV2PATHTYPE>(atoi(temp));
                LOG_INFO(CamxLogGroupChi, "Input port path type %d", m_inputPortPathType);
            }
            else if (NodePropertyEnableFOVC == pCreateInfo->pNodeProperties[i].id)
            {
                m_isFOVCEnabled = *reinterpret_cast<const UINT*>(pCreateInfo->pNodeProperties[i].pValue);
                LOG_INFO(CamxLogGroupChi, "Is FOVC enabled %d", m_isFOVCEnabled);
            }
        }

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
    }

    if (CDKResultSuccess != result)
    {
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
/// ChiEISV2Node::SetDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiEISV2Node::SetDependencies(
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
        pDependencyInfo->properties[depCount]   = g_vendorTagId.chiNodeResidualCrop;
        pDependencyInfo->offsets[depCount]      = 0;
        pDependencyInfo->count                  = ++depCount;

        // set SAT ICA In perspective transform dependency
        pDependencyInfo->properties[depCount]   = g_vendorTagId.ICAInPerspectiveTransformTagId;
        pDependencyInfo->offsets[depCount]      = 0;
        pDependencyInfo->inputPortId[depCount]  = FullPath;
        pDependencyInfo->count                  = ++depCount;
    }

    pDependencyInfo->hNodeSession         = m_hChiSession;

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV2Node::GetGyroInterval
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV2Node::GetGyroInterval(
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
        SOFTimestampTagId  |= InputMetadataSectionMask;
        frameDurationTagId |= InputMetadataSectionMask;
        ExposureTimeTagId  |= InputMetadataSectionMask;
        rollingShutterSkew |= InputMetadataSectionMask;
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
    if ((CDKResultSuccess == result) && (NULL != timestampInfo))
    {
        frameTimes.sof                          = NanoToMicro(timestampInfo->timestamp);
        frameTimes.frame_time                   = static_cast<UINT32>(NanoToMicro(frameDuration));
        frameTimes.exposure_time                = static_cast<UINT32>(NanoToMicro(sensorExpTime));
        frameTimes.sensor_rolling_shutter_skew  = static_cast<UINT32>(NanoToMicro(sensorRollingShutterSkewNano));

        UINT32 isResult = m_eis2GetGyroTimeInterval(m_phEIS2Handle, &frameTimes, sensorIndex, pGyroInterval);
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
        LOG_ERROR(CamxLogGroupChi, "Error Initializing EIS2 Algorithm");
    }

    if (CDKResultSuccess == result)
    {
        result = ValidateEISGyroInterval(pGyroInterval);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV2Node::SetGyroDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiEISV2Node::SetGyroDependency(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo,
    UINT32                     sensorIndex)
{
    CHIDATAREQUEST    dataRequest;
    gyro_times_t      gyroInterval     = { 0 };
    CDKResult         result           = CDKResultSuccess;
    ChiNCSDataRequest ncsRequest       = {0, {0}, NULL, 0};

    CHAR              chiFenceName[MaxStringLength64] = { 0 };

    memset(&dataRequest, 0x0, sizeof(CHIDATAREQUEST));

    result = GetGyroInterval(pProcessRequestInfo->frameNum, sensorIndex, &gyroInterval, NULL);

    if (CDKResultSuccess == result)
    {
        //< Create fence
        CHIFENCECREATEPARAMS chiFenceParams = { 0 };
        chiFenceParams.type                 = ChiFenceTypeInternal;
        chiFenceParams.size                 = sizeof(CHIFENCECREATEPARAMS);
        CamX::OsUtils::SNPrintF(chiFenceName, sizeof(chiFenceName), "ChiInternalFence_EISV2");
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
/// ChiEISV2Node::GetChromatixTuningHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV2Node::GetChromatixTuningHandle(
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
        TuningSetManager*                pTuningManager  = m_perSensorData[sensorIndex].pTuningManager;
        eis_1_2_0::chromatix_eis12Type** ppEISChromatix  = &m_perSensorData[sensorIndex].pEISChromatix;

        tuningMode[tuningModeIndex].mode    = ModeType::Default;
        tuningMode[tuningModeIndex].subMode = { 0 };

        tuningModeIndex++;
        if (InvalidSensorMode != sensorMode)
        {
            tuningMode[tuningModeIndex].mode          = ModeType::Sensor;
            tuningMode[tuningModeIndex].subMode.value = static_cast<UINT16>(sensorMode);
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
/// ChiEISV2Node::GetChromatixData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV2Node::GetChromatixData(
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
        pEISInitData->is_chromatix_info.general.focal_length           = pGeneralStruct->focal_length;
        pEISInitData->is_chromatix_info.general.gyro_noise_floor       = pGeneralStruct->gyro_noise_floor;
        // TODO: acc_noise_floor needs to be added to EIS chromatix 1_2_0
        pEISInitData->is_chromatix_info.general.acc_noise_floor        = 0;
        // TODO: mag_noise_floor needs to be added to EIS chromatix 1_2_0
        pEISInitData->is_chromatix_info.general.mag_noise_floor        = 0;
        pEISInitData->is_chromatix_info.general.output_grid_precision  = pGeneralStruct->output_grid_precision;
        pEISInitData->is_chromatix_info.general.res_param_1            = pGeneralStruct->res_param_1;
        pEISInitData->is_chromatix_info.general.res_param_2            = pGeneralStruct->res_param_2;
        pEISInitData->is_chromatix_info.general.res_param_3            = pGeneralStruct->res_param_3;
        pEISInitData->is_chromatix_info.general.res_param_4            = pGeneralStruct->res_param_4;
        pEISInitData->is_chromatix_info.general.res_param_5            = pGeneralStruct->res_param_5;
        pEISInitData->is_chromatix_info.general.res_param_6            = pGeneralStruct->res_param_6;
        pEISInitData->is_chromatix_info.general.res_param_7            = pGeneralStruct->res_param_7;
        pEISInitData->is_chromatix_info.general.res_param_8            = pGeneralStruct->res_param_8;
        pEISInitData->is_chromatix_info.general.res_param_9            = pGeneralStruct->res_param_9;
        pEISInitData->is_chromatix_info.general.res_param_10           = pGeneralStruct->res_param_10;

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
CDKResult ChiEISV2Node::GetLDCGridFromICA20Chromatix(
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
CDKResult ChiEISV2Node::GetLDCGridFromICA30Chromatix(
    is_init_data_sensor* pEISInitData,
    UINT32               sensorIndex)
{
    CDKResult result                                = CDKResultSuccess;
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
/// ChiEISV2Node::GetLDCGridFromEISChromatix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV2Node::GetLDCGridFromEISChromatix(
    eis_lens_distortion_correction*                                             pEISLDCInitData,
    eis_1_2_0::chromatix_eis12_reserveType::lens_distortion_correctionStruct*   pEISLDCChromatix)
{
    CDKResult result            = CDKResultSuccess;
    UINT32    numLDCGridPoints  = 0;

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
/// ChiEISV2Node::FillGyroData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV2Node::FillGyroData(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo,
    gyro_data_t*               pGyroData,
    frame_times_t*             pFrameTimes,
    UINT32                     sensorIndex)
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

    result = GetGyroInterval(pProcessRequestInfo->frameNum, sensorIndex, &gyroInterval, pFrameTimes);

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
            LOG_ERROR(CamxLogGroupChi, "Unable to get data");
            result = CDKResultEFailed;
        }
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Unable to get gyro time interval");
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV2Node::UpdateZoomWindow
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiEISV2Node::UpdateZoomWindow(
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

            pCropRect->left     += adjustWidthBy;
            pCropRect->top      += adjustHeightBy;
            pCropRect->width    -= adjustWidthBy  * 2;
            pCropRect->height   -= adjustHeightBy * 2;
        }
    }

    LOG_VERBOSE(CamxLogGroupChi,
                "Updated Zoom Window [%d, %d, %d, %d] full %d x %d "
                "cropFactor %f, left cropFactor %f, top cropFactor %f, fovc factor allyed %f request %"
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
/// ChiEISV2Node::GetCropRectfromCropInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiEISV2Node::GetCropRectfromCropInfo(
    IFECropInfo* cropInfo,
    CHIRectangle* cropRect)
{
    switch (m_inputPortPathType)
    {
    case EISV2PATHTYPE::FullPath:
        *cropRect = cropInfo->fullPath;
        break;
    case EISV2PATHTYPE::DS4Path:
        *cropRect = cropInfo->DS4Path;
        break;
    case EISV2PATHTYPE::DS16Path:
        *cropRect = cropInfo->DS16Path;
        break;
    case EISV2PATHTYPE::FDPath:
        *cropRect = cropInfo->FDPath;
        break;
    case EISV2PATHTYPE::DisplayFullPath:
        *cropRect = cropInfo->displayFullPath;
        break;
    case EISV2PATHTYPE::DisplayDS4Path:
        *cropRect = cropInfo->displayDS4Path;
        break;
    case EISV2PATHTYPE::DisplayDS16Path:
        *cropRect = cropInfo->displayDS16Path;
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV2Node::ExecuteAlgo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV2Node::ExecuteAlgo(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo,
    is_output_type*            pEIS2Output,
    UINT32                     sensorIndex)
{
    UINT64                    requestId;
    is_input_t                eis2Input                                 = { 0 };
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

    requestId = pProcessRequestInfo->frameNum;

    UINT32 residualCropTagId            = g_vendorTagId.residualCropTagId;
    UINT32 appliedCropTagId             = g_vendorTagId.appliedCropTagId;
    UINT32 lensFocusDistance            = ANDROID_LENS_FOCUS_DISTANCE;
    UINT32 SATPerspectiveTransformTagId = g_vendorTagId.ICAInPerspectiveTransformTagId;
    UINT32 fovcFactorTagId              = g_vendorTagId.FOVCFactorTagId;

    if (1 < m_numOfLinkedCameras)
    {
        cameraId = m_perSensorData[sensorIndex].cameraConfig.cameraId;
    }

    if (FALSE == IsRealTimeNode())
    {
        residualCropTagId   |= InputMetadataSectionMask;
        appliedCropTagId    |= InputMetadataSectionMask;
        lensFocusDistance   |= InputMetadataSectionMask;
        fovcFactorTagId     |= InputMetadataSectionMask;
    }

    if (1 < m_numOfLinkedCameras)
    {
        cameraId            = m_perSensorData[sensorIndex].cameraConfig.cameraId;
        residualCropTagId   = g_vendorTagId.chiNodeResidualCrop;
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
            IFECropInfo* pIFERes1dualCropInfo = static_cast<IFECropInfo*>(pData);
            GetCropRectfromCropInfo(pIFERes1dualCropInfo, pIFEResidualCropRect);
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
        eis2Input.focus_distance = *static_cast<FLOAT*>(pData);
        LOG_VERBOSE(CamxLogGroupChi, "Lens Focus distance %f", eis2Input.focus_distance);
    }

    eis2Input.frame_id                  = static_cast<uint32_t>(requestId);
    eis2Input.active_sensor_idx         = sensorIndex;
    eis2Input.sat                       = NULL;

    ifeCrop.fullWidth        = m_perSensorData[sensorIndex].sensorDimension.width;
    ifeCrop.fullHeight       = m_perSensorData[sensorIndex].sensorDimension.height;
    ipeZoom.fullWidth        = m_perSensorData[sensorIndex].inputSize.width  - m_stabilizationMargins.widthPixels;
    ipeZoom.fullHeight       = m_perSensorData[sensorIndex].inputSize.height - m_stabilizationMargins.heightLines;

    if (NULL != pIFEAppliedCropRect)
    {
        ifeCrop.windowWidth  = pIFEAppliedCropRect->width;
        ifeCrop.windowHeight = pIFEAppliedCropRect->height;
        ifeCrop.windowLeft   = pIFEAppliedCropRect->left;
        ifeCrop.windowTop    = pIFEAppliedCropRect->top;
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
        ipeZoom.windowWidth  = pIFEResidualCropRect->width;
        ipeZoom.windowHeight = pIFEResidualCropRect->height;
        ipeZoom.windowLeft   = pIFEResidualCropRect->left;
        ipeZoom.windowTop    = pIFEResidualCropRect->top;
    }
    else
    {
        ipeZoom.windowWidth  = ipeZoom.fullWidth;
        ipeZoom.windowHeight = ipeZoom.fullHeight;
        ipeZoom.windowLeft   = 0;
        ipeZoom.windowTop    = 0;
    }

    //< Get Gyro Data
    eis2Input.gyro_data.samples      = &input_gyro_data_t[0];
    eis2Input.gyro_data.num_elements = 0;
    result = FillGyroData(pProcessRequestInfo, &eis2Input.gyro_data, &eis2Input.frame_times, sensorIndex);
    //TODO: Fill accelerometer and magnetometer data when available

    if (CDKResultSuccess == result)
    {
        int32_t tmpResult = m_eisUtilsConvertToWindowRegions(&ifeCrop,
                                                             &ipeZoom,
                                                             m_stabilizationCropRatioX[sensorIndex],
                                                             m_stabilizationCropRatioY[sensorIndex],
                                                             m_perSensorData[sensorIndex].inputSize.width,
                                                             m_perSensorData[sensorIndex].inputSize.height,
                                                             &eis2Input.window_regions);

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
                     eis2Input.window_regions.eis_pre_crop_vIN.fullWidth,
                     eis2Input.window_regions.eis_pre_crop_vIN.fullHeight,
                     eis2Input.window_regions.eis_pre_crop_vIN.windowLeft,
                     eis2Input.window_regions.eis_pre_crop_vIN.windowTop,
                     eis2Input.window_regions.eis_pre_crop_vIN.windowWidth,
                     eis2Input.window_regions.eis_pre_crop_vIN.windowHeight,
                     eis2Input.window_regions.eis_pre_crop_vOUT.fullWidth,
                     eis2Input.window_regions.eis_pre_crop_vOUT.fullHeight,
                     eis2Input.window_regions.eis_pre_crop_vOUT.windowLeft,
                     eis2Input.window_regions.eis_pre_crop_vOUT.windowTop,
                     eis2Input.window_regions.eis_pre_crop_vOUT.windowWidth,
                     eis2Input.window_regions.eis_pre_crop_vOUT.windowHeight,
                     eis2Input.window_regions.output_crop_fov.fullWidth,
                     eis2Input.window_regions.output_crop_fov.fullHeight,
                     eis2Input.window_regions.output_crop_fov.windowLeft,
                     eis2Input.window_regions.output_crop_fov.windowTop,
                     eis2Input.window_regions.output_crop_fov.windowWidth,
                     eis2Input.window_regions.output_crop_fov.windowHeight);
        }
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Cannot get Gyro Data. Not executing EISv2 algo for %" PRIu64, requestId);
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
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

            eis2Input.sat = &satTransformWarp;
        }
    }

    if (CDKResultSuccess == result)
    {
        int32_t isResult = m_eis2Process(m_phEIS2Handle, &eis2Input, pEIS2Output);

        if (IS_RET_SUCCESS != isResult)
        {
            LOG_ERROR(CamxLogGroupChi, "EISv2 algo execution Failed (%x) for request %" PRIu64, result, requestId);
            result = CDKResultEFailed;
        }
        else
        {
            if (false == pEIS2Output->has_output)
            {
                LOG_ERROR(CamxLogGroupChi, "Send default matrix for request %" PRIu64, requestId);
                result = CDKResultEFailed;
            }
        }

        if (IS_OIS_MODE_INVALID != m_overrideOisMode)
        {
            pEIS2Output->ois_mode = m_overrideOisMode;
        }

        if (NULL != m_pEisUtilsLogContext)
        {
            is_utils_log_write_data data;

            data.is_input   = &eis2Input;
            data.is_output  = pEIS2Output;
            data.buffer     = NULL;

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
/// ChiEISV2Node::IsEISv2Disabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiEISV2Node::IsEISv2Disabled(
    UINT64 requestId)
{
    VOID* pData      = NULL;
    BOOL eisDisabled = FALSE;
    camera_metadata_enum_android_control_video_stabilization_mode eisMode;

    pData = ChiNodeUtils::GetMetaData(requestId, ANDROID_CONTROL_VIDEO_STABILIZATION_MODE | InputMetadataSectionMask,
                                      ChiMetadataDynamic, &g_ChiNodeInterface, m_hChiSession);
    if (NULL != pData)
    {
        eisMode = *(static_cast<camera_metadata_enum_android_control_video_stabilization_mode *>(pData));
        if (eisMode == ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF)
        {
            eisDisabled = TRUE;
            LOG_VERBOSE(CamxLogGroupChi, "Eisv2 Disabled %d", eisDisabled);
        }
    }
    return eisDisabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV2Node::ProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV2Node::ProcessRequest(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo)
{
    CDKResult          result               = CDKResultSuccess;
    BOOL               hasDependencies      = TRUE;
    BOOL               isDisabled           = FALSE;
    INT32              sequenceNumber       = pProcessRequestInfo->pDependency->processSequenceId;
    CHIDEPENDENCYINFO* pDependencyInfo      = pProcessRequestInfo->pDependency;
    UINT16             depCount             = 0;
    UINT32             currentCameraId      = 0;
    UINT32             currentCameraIdIndex = 0;
    is_output_type     eis2Output;

    NcLibPerspTransformSingle perspectiveMatrix                                                   = { { 0 } };
    NcLibWarpGridCoord        perspectiveGrid[ICA30GridTransformWidth * ICA30GridTransformHeight] = { { 0 } };
    NcLibWarpGridCoord        gridExtrapolateICA10[NumICA10Exterpolate]                           = { { 0 } };
    NcLibPerspTransformSingle alignmentMatrixDomainUndistorted                                    = { { 0 } };
    NcLibPerspTransformSingle alignmentMatrixDomainStabilized                                     = { { 0 } };

    LOG_INFO(CamxLogGroupChi, "E.. request id %" PRIu64 ", seq id %d", pProcessRequestInfo->frameNum, sequenceNumber);
    memset(&eis2Output, 0, sizeof(is_output_type));

    // if multi camera usecase get current camera id
    if ((1 < m_numOfLinkedCameras) && (0 < sequenceNumber))
    {
        UINT32 cameraIDmetaTag = 0;
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
    if (NULL != m_pEisUtilsLogContext && NULL != m_eisUtilsLogOpen && NULL != m_eisUtilsLogIsOpened)
    {
        bool      isLogOpened = FALSE;
        CDKResult logResult   = m_eisUtilsLogIsOpened(m_pEisUtilsLogContext, &isLogOpened);

        if (IS_RET_SUCCESS == logResult && FALSE == isLogOpened)
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
                LOG_ERROR(CamxLogGroupChi, "EISV2 Algorithm re-open log utility Failed");
            }
        }
    }

    // In this sequence we set dependency for SOF time stamp, exposure time and frame duration,
    // needed to calculate gyro sample interval for the frame.
    if ((CDKResultSuccess == result) && (0 == sequenceNumber) && (TRUE == hasDependencies))
    {
        ///< Check if EISv2 is disabled
        isDisabled = IsEISv2Disabled(pProcessRequestInfo->frameNum);
        depCount = 0;
        if (1 < m_numOfLinkedCameras)
        {
            pDependencyInfo->properties[depCount]   = g_vendorTagId.multiCameraIdTagId;
            pDependencyInfo->offsets[depCount]      = 0;
            pDependencyInfo->count                  = ++depCount;
        }
        // Set the following dependencies on sequence 0 only for single camera
        if (TRUE == IsRealTimeNode())
        {
            LOG_VERBOSE(CamxLogGroupChi, "Seq number %d", sequenceNumber);
            if (FALSE == isDisabled)
            {
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
                pDependencyInfo->hNodeSession             = m_hChiSession;
            }
            else if (1 == pProcessRequestInfo->frameNum)
            {
                pDependencyInfo->properties[depCount]   = ANDROID_SENSOR_EXPOSURE_TIME;
                pDependencyInfo->offsets[depCount]      = 0;
                pDependencyInfo->count                  = ++depCount;
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
    }

    // In this sequence we set gyro denpendency based on the SOF time stamp, exposure time and frame duration
    if ((CDKResultSuccess == result) && (1 == sequenceNumber) && (TRUE == hasDependencies))
    {
        LOG_VERBOSE(CamxLogGroupChi, "Seq number %d", sequenceNumber);
        ///< Get Input Buffer size
        if (pProcessRequestInfo->inputNum > 0)
        {
            m_perSensorData[currentCameraIdIndex].inputSize.width  = pProcessRequestInfo->phInputBuffer[0]->format.width;
            m_perSensorData[currentCameraIdIndex].inputSize.height = pProcessRequestInfo->phInputBuffer[0]->format.height;
        }
        ///< Check if EISv2 is disabled
        isDisabled = IsEISv2Disabled(pProcessRequestInfo->frameNum);
        if (isDisabled)
        {
            hasDependencies       = FALSE;
            eis2Output.has_output = 0;
            eis2Output.frame_id   = static_cast<uint32_t>(pProcessRequestInfo->frameNum);

            for (UINT32 i = 0; i < pProcessRequestInfo->inputNum; i++)
            {
                if (TRUE == IsBypassableNode())
                {
                    pProcessRequestInfo->pBypassData[i].isBypassNeeded         = TRUE;
                    pProcessRequestInfo->pBypassData[i].selectedInputPortIndex = i;
                    LOG_VERBOSE(CamxLogGroupChi, "is disabled-bypass true port %d,frame %" PRIu64 ", ",
                                i,  pProcessRequestInfo->frameNum);
                }
            }

            UpdateMetaData(pProcessRequestInfo->frameNum, currentCameraIdIndex, &eis2Output);
            pProcessRequestInfo->pDependency->satisfySequentialExecutionDependency = TRUE;
        }
        else
        {
            if (TRUE == m_gyroNCSServiceAvailable)
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
    }

    // In this sequence we execute the EISv3 Algo and publish the results to IPE/GPU for warping
    if ((CDKResultSuccess == result) && (2 == sequenceNumber) && (TRUE == hasDependencies))
    {
        //< Properties dependencies should be met by now. Fill data.
        hasDependencies = FALSE;
        LOG_VERBOSE(CamxLogGroupChi, "Seq number %d", sequenceNumber);

        //< Initialize output matrix
        eis2Output.stabilizationTransform.matrices.perspMatrices        = &perspectiveMatrix;
        eis2Output.stabilizationTransform.grid.grid                     = &perspectiveGrid[0];
        eis2Output.stabilizationTransform.grid.gridExtrapolate          = &gridExtrapolateICA10[0];
        eis2Output.alignment_matrix_domain_undistorted.perspMatrices    = &alignmentMatrixDomainUndistorted;
        eis2Output.alignment_matrix_domain_stabilized.perspMatrices     = &alignmentMatrixDomainStabilized;

        ///< Execute Algo
        if ((TRUE == m_gyroNCSServiceAvailable) && (FALSE == m_bIsDefaultGridTransformEnabled))
        {
            result = ExecuteAlgo(pProcessRequestInfo, &eis2Output, currentCameraIdIndex);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi,
                      "For request %"  PRIu64
                      " EISv2 disabled due to NCSServiceAvailable %d IsDefaultGridTransformEnabled %d",
                      pProcessRequestInfo->frameNum,
                      m_gyroNCSServiceAvailable,
                      m_bIsDefaultGridTransformEnabled);
            result = CDKResultEUnsupported;
        }

        if (CDKResultSuccess != result)
        {
            LOG_ERROR(CamxLogGroupChi, "EISv2 algo execution failed for request %" PRIu64, pProcessRequestInfo->frameNum);
            ///< Update metadata with default result
            eis2Output.has_output = FALSE;
            result                = CDKResultSuccess;
        }
        else
        {
            LOG_VERBOSE(CamxLogGroupChi, "ICA version is %d", m_ICAVersion);

            // convert ICA20 grid to ICA10 if ICA version is 10
            if (ChiICAVersion::ChiICA10 == m_ICAVersion)
            {
                // converting grid inplace
                result = ConvertICA20GridToICA10Grid(&eis2Output.stabilizationTransform.grid,
                                                     &eis2Output.stabilizationTransform.grid);

                if (CDKResultSuccess != result)
                {
                    LOG_ERROR(CamxLogGroupChi, "Grid conversion failed for request %" PRIu64, pProcessRequestInfo->frameNum);
                    eis2Output.stabilizationTransform.grid.enable = FALSE;
                }
            }
        }

        UpdateMetaData(pProcessRequestInfo->frameNum, currentCameraIdIndex, &eis2Output);
        pProcessRequestInfo->pDependency->satisfySequentialExecutionDependency = TRUE;

        // Set Buffer input dependency to avoid CVP failure due to IFE delayed buffer done
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
        pProcessRequestInfo->pDependency->processSequenceId = 3;
    }

    if ((CDKResultSuccess == result) && (3 == sequenceNumber))
    {
        for (UINT32 i = 0; i < pProcessRequestInfo->inputNum; i++)
        {
            if (TRUE == IsBypassableNode())
            {
                pProcessRequestInfo->pBypassData[i].isBypassNeeded         = TRUE;
                pProcessRequestInfo->pBypassData[i].selectedInputPortIndex = i;
                LOG_VERBOSE(CamxLogGroupChi, "eis on-bypass true port %d,frame %" PRIu64 ", ",
                            i,  pProcessRequestInfo->frameNum);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV2Node::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV2Node::PostPipelineCreate()
{
    CDKResult       result     = CDKResultSuccess;
    ImageDimensions marginDims = { 0 };

    CHIDATASOURCECONFIG    CHIDataSourceConfig;
    CHINCSDATASOURCECONFIG NCSDataSourceCfg;

    memset(&NCSDataSourceCfg, 0, sizeof(CHINCSDATASOURCECONFIG));
    NCSDataSourceCfg.sourceType    = ChiDataGyro;
    NCSDataSourceCfg.samplingRate  = GetGyroFrequency(m_primarySensorIdx);
    NCSDataSourceCfg.operationMode = 0;
    NCSDataSourceCfg.reportRate    = 10000;
    NCSDataSourceCfg.size          = sizeof(CHINCSDATASOURCECONFIG);

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

    // Initialize EISv2 lib
    result = InitializeLib();

    if (CDKResultSuccess == result)
    {
        m_eis2GetTotalMargin(m_phEIS2Handle, m_primarySensorIdx, &marginDims);
        if ((marginDims.widthPixels != m_stabilizationMargins.widthPixels) ||
            (marginDims.heightLines != m_stabilizationMargins.heightLines))
        {
            LOG_ERROR(CamxLogGroupChi,
                      "Unexpected EISv2 margin values. Using %ux%u, calculated %ux%u",
                      m_stabilizationMargins.widthPixels, m_stabilizationMargins.heightLines,
                      marginDims.widthPixels, marginDims.heightLines);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV2Node::ChiEISV2Node
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiEISV2Node::ChiEISV2Node(
    EISV2OverrideSettings overrideSettings)
    : m_hChiSession(NULL)
    , m_nodeId(0)
    , m_nodeCaps(0)
    , m_gyroNCSServiceAvailable(FALSE)
    , m_numOfLinkedCameras(1)
    , m_primaryCameraId(0)
    , m_pEisUtilsLogContext(NULL)
    , m_isUtilsLogFlags(IS_UTILS_FLAG_NONE)
    , m_isFOVCEnabled(0)
    , m_cropFactorFOVC(FOVCFactorDefault)
{
    m_lookahead                         = 0;
    m_primarySensorIdx                  = 0;
    m_hChiDataSource.pHandle            = NULL;
    m_phEIS2Handle                      = NULL;
    m_hChiDataSource.dataSourceType     = ChiDataMax;
    m_algoOperationMode                 = overrideSettings.algoOperationMode;
    m_inputPortPathType                 = EISV2PathType::FullPath;
    m_bIsLDCGridEnabled                 = overrideSettings.isLDCGridEnabled;
    m_isEnabledDumpForceFlush           = overrideSettings.isEnabledDumpForceFlush;
    m_bIsDefaultGridTransformEnabled    = overrideSettings.isDefaultGridTransformEnabled;
    m_overrideOisMode                   = overrideSettings.overrideOisMode;
    memset(&m_hChiDataSource, 0, sizeof(CHIDATASOURCE));
    memset(&m_perSensorData,  0, sizeof(m_perSensorData));

    for (UINT i = 0; i < MaxMulticamSensors; i++)
    {
        m_pLDCIn2OutGrid[i]     = NULL;
        m_pLDCOut2InGrid[i]     = NULL;
        m_LDCIn2OutWarpGrid[i]  = { 0 };
        m_LDCOut2InWarpGrid[i]  = { 0 };
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
/// ChiEISV2Node::~ChiEISV2Node
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiEISV2Node::~ChiEISV2Node()
{
    CDKResult result = CDKResultSuccess;

    ///< Destroy EIS log util
    if (NULL != m_pEisUtilsLogContext)
    {
        m_eisUtilsLogDestroy(&m_pEisUtilsLogContext);
        m_pEisUtilsLogContext = NULL;
    }

    ///< Deinitialize algo
    if (NULL != m_phEIS2Handle)
    {
        int32_t isResult = m_eis2Deinitialize(&m_phEIS2Handle);
        if (IS_RET_SUCCESS != isResult)
        {
            LOG_ERROR(CamxLogGroupChi, "EISv2 Algo Deinit failed");
            result = CDKResultEFailed;
        }
        m_phEIS2Handle    = NULL;
    }

    //< Unload Lib
    if (NULL != m_hEISv2Lib)
    {
        result = UnLoadLib();
        if (CDKResultSuccess != result)
        {
            LOG_ERROR(CamxLogGroupChi, "EISv2 Lib Unload failed");
        }
    }

    m_hEISv2Lib      = NULL;
    m_hChiSession    = NULL;

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
/// ChiEISV2Node::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV2Node::QueryMetadataPublishList(
    CHINODEMETADATALIST* pMetadataPublishlist)
{
    UINT count = 0;

    if (NULL != pMetadataPublishlist)
    {
        if (ChiICAMax == m_ICAVersion)
        {
            // deployment type gpu
            pMetadataPublishlist->tagArray[count] = g_vendorTagId.ICAInGridIn2OutTransformTagId;
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
/// ChiEISV2Node::SetBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV2Node::SetBufferInfo(
    CHINODESETBUFFERPROPERTIESINFO* pSetBufferInfo)
{
    CDKResult                   result          = CDKResultSuccess;
    ImageDimensions             marginDims      = { 0 };
    is_get_stabilization_margin inputDims       = { 0 };
    FLOAT                       minTotalMarginX = 0.0F;
    FLOAT                       minTotalMarginY = 0.0F;
    CHIDimension                outDimension    = { 0 };
    CHIDimension                outDimLookAhead = { 0 };
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
                                      (g_vendorTagId.EISV2StabilizedOutputDimsTagId | UsecaseMetadataSectionMask),
                                      ChiMetadataDynamic,
                                      &g_ChiNodeInterface,
                                      m_hChiSession);
    if (NULL != pData)
    {
        outDimension = *static_cast<CHIDimension*>(pData);
    }
    if (0 != g_vendorTagId.EISV2OutputDimsLookAheadTagId)
    {
        pData = ChiNodeUtils::GetMetaData(0,
                      (g_vendorTagId.EISV2OutputDimsLookAheadTagId | UsecaseMetadataSectionMask),
                      ChiMetadataDynamic,
                      &g_ChiNodeInterface,
                      m_hChiSession);
        if (NULL != pData)
        {
            outDimLookAhead = *static_cast<CHIDimension*>(pData);
            // TODO: Assume that 4K video has different sources, so skip largest dim pick. Make it generic in the future.
            if (FALSE == (outDimLookAhead.width >= VIDEO4KWIDTH && m_numOfLinkedCameras == 1))
            {
                // Use the largest dim for output
                if ((outDimLookAhead.height * outDimLookAhead.width) > (outDimension.height * outDimension.width))
                {
                    outDimension = outDimLookAhead;
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

    UINT32 isResult = m_eis2GetTotalMarginEx(&inputDims, &marginDims);
    if (IS_RET_SUCCESS != isResult)
    {
        result = CDKResultEFailed;
        LOG_ERROR(CamxLogGroupChi, "m_eis2GetTotalMarginEx failed - %d", isResult);
    }

    m_stabilizationMargins.widthPixels = marginDims.widthPixels;
    m_stabilizationMargins.heightLines = marginDims.heightLines;

    // Until Geo-Lib integration is done, use utility function to connect old ROIs to new EIS API.
    // Remove this once integration is done
    for (UINT32 i = 0; i < m_numOfLinkedCameras; i++)
    {
        isResult = m_eis2GetStabilizationCropRatioEx(&inputDims,
                                                     &m_stabilizationCropRatioX[i],
                                                     &m_stabilizationCropRatioY[i]);
        if (IS_RET_SUCCESS != isResult)
        {
            result = CDKResultEFailed;
            LOG_ERROR(CamxLogGroupChi, "m_eis2_get_stabilization_crop_ratio_ex failed - %d", isResult);
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

    tagList[index]           = (g_vendorTagId.EISV2StabilizationMarginsTagId | UsecaseMetadataSectionMask);
    tagData[index].size      = sizeof(CHITAGDATA);
    tagData[index].requestId = 0;
    tagData[index].pData     = &actualMargin;
    tagData[index].dataSize  = g_VendorTagSectionEISRealTime[2].numUnits;
    index++;

    tagList[index]           = (g_vendorTagId.EISV2AdditionalCropOffsetTagId | UsecaseMetadataSectionMask);
    tagData[index].size      = sizeof(CHITAGDATA);
    tagData[index].requestId = 0;
    tagData[index].pData     = &m_additionalCropOffset;
    tagData[index].dataSize  = g_VendorTagSectionEISRealTime[3].numUnits;

    LOG_VERBOSE(CamxLogGroupChi, "Publishing actual margins %ux%u and additional crop offset %ux%u from eisv2",
                actualMargin.widthPixels, actualMargin.heightLines,
                m_additionalCropOffset.widthPixels, m_additionalCropOffset.heightLines);

    g_ChiNodeInterface.pSetMetadata(&metadataInfo);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV2Node::QueryBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV2Node::QueryBufferInfo(
    CHINODEQUERYBUFFERINFO* pQueryBufferInfo)
{
    CDKResult   result = CDKResultSuccess;
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

                perOutputPortOptimalWidth   = ChiNodeUtils::MaxUINT32(pOutputRequirement->optimalW, perOutputPortOptimalWidth);
                perOutputPortOptimalHeight  = ChiNodeUtils::MaxUINT32(pOutputRequirement->optimalH, perOutputPortOptimalHeight);
                perOutputPortMinWidth       = ChiNodeUtils::MaxUINT32(pOutputRequirement->minW, perOutputPortMinWidth);
                perOutputPortMinHeight      = ChiNodeUtils::MaxUINT32(pOutputRequirement->minH, perOutputPortMinHeight);
                perOutputPortMaxWidth       = ChiNodeUtils::MinUINT32(pOutputRequirement->maxW, perOutputPortMaxWidth);
                perOutputPortMaxHeight      = ChiNodeUtils::MinUINT32(pOutputRequirement->maxH, perOutputPortMaxHeight);
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
/// ChiEISV2Node::UpdateMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiEISV2Node::UpdateMetaData(
    UINT64          requestId,
    UINT32          sensorIndex,
    is_output_type* pAlgoResult)
{
    UINT32 tagIdList[] =
    {
        g_vendorTagId.ICAInGridIn2OutTransformTagId
    };

    CHIMETADATAINFO  metadataInfo          = { 0 };
    const UINT32     tagSize               = sizeof(tagIdList) / sizeof(tagIdList[0]);
    UINT32           index                 = 0;
    CHITAGDATA       tagData[tagSize]      = { { 0 } };
    UINT32           tagList[tagSize]      = { 0 };

    metadataInfo.size       = sizeof(CHIMETADATAINFO);
    metadataInfo.chiSession = m_hChiSession;
    metadataInfo.tagNum     = tagSize;
    metadataInfo.pTagList   = &tagList[0];
    metadataInfo.pTagData   = &tagData[0];

    ///< Update resulting perspective matrix
    IPEICAPerspectiveTransform perspectiveTrans;
    memset(&perspectiveTrans, 0, sizeof(IPEICAPerspectiveTransform));

    IPEICAGridTransform gridTrans;
    memset(&gridTrans, 0, sizeof(IPEICAGridTransform));

    IPEICAPerspectiveTransform gyroAlignmentTrans;
    memset(&gyroAlignmentTrans, 0, sizeof(IPEICAPerspectiveTransform));

    if (TRUE == pAlgoResult->has_output)
    {
        if (TRUE == pAlgoResult->stabilizationTransform.matrices.enable)
        {
            NcLibWarpMatrices resultMatrices                  = pAlgoResult->stabilizationTransform.matrices;
            perspectiveTrans.perspectiveTransformEnable       = resultMatrices.enable;
            perspectiveTrans.perspectiveConfidence            = resultMatrices.confidence;
            perspectiveTrans.byPassAlignmentMatrixAdjustement = FALSE;
            perspectiveTrans.perspetiveGeometryNumColumns     = resultMatrices.numColumns;
            perspectiveTrans.perspectiveGeometryNumRows       = static_cast<UINT8>(resultMatrices.numRows);

            //TBD: Change from active sensor to what was passed to the algo
            perspectiveTrans.transformDefinedOnWidth     = m_perSensorData[sensorIndex].inputSize.width;
            perspectiveTrans.transformDefinedOnHeight    = m_perSensorData[sensorIndex].inputSize.height;
            perspectiveTrans.ReusePerspectiveTransform   = 0;

            memcpy(&perspectiveTrans.perspectiveTransformArray, &resultMatrices.perspMatrices[0].T[0],
                   sizeof(NcLibPerspTransformSingle));
        }
        else
        {
            LOG_INFO(CamxLogGroupChi, "Algo has perspective transform disabled for req id %" PRId64, requestId);
        }

        if (TRUE == pAlgoResult->stabilizationTransform.grid.enable)
        {
            NcLibWarpGrid resultGrid                        = pAlgoResult->stabilizationTransform.grid;
            gridTrans.gridTransformEnable                   = resultGrid.enable;
            gridTrans.gridTransformArrayExtrapolatedCorners =
                (resultGrid.extrapolateType == NcLibWarpGridExtrapolationType::EXTRAPOLATION_TYPE_FOUR_CORNERS) ? 1 : 0;

            if (1 == gridTrans.gridTransformArrayExtrapolatedCorners)
            {
                memcpy(&gridTrans.gridTransformArrayCorners[0],
                       resultGrid.gridExtrapolate,
                       sizeof(gridTrans.gridTransformArrayCorners));
            }

            //TBD: Change from active sensor to what was passed to the algo
            gridTrans.transformDefinedOnWidth   = m_perSensorData[sensorIndex].inputSize.width;
            gridTrans.transformDefinedOnHeight  = m_perSensorData[sensorIndex].inputSize.height;
            gridTrans.reuseGridTransform        = 0;
            getICAGridGeometryVersion(resultGrid.numRows, resultGrid.numColumns, &gridTrans.geometry);
            memcpy(&gridTrans.gridTransformArray[0], resultGrid.grid,
                   sizeof(ICAGridArray) * resultGrid.numRows * resultGrid.numColumns);
        }
        else
        {
            LOG_INFO(CamxLogGroupChi, "Algo has grid transform disabled for req id %" PRId64, requestId);
        }

        if (TRUE == pAlgoResult->alignment_matrix_domain_stabilized.enable)
        {
            NcLibWarpMatrices resultMatrices                    = pAlgoResult->alignment_matrix_domain_stabilized;
            gyroAlignmentTrans.perspectiveTransformEnable       = resultMatrices.enable;
            gyroAlignmentTrans.perspectiveConfidence            = resultMatrices.confidence;
            gyroAlignmentTrans.byPassAlignmentMatrixAdjustement = FALSE;
            gyroAlignmentTrans.perspetiveGeometryNumColumns     = resultMatrices.numColumns;
            gyroAlignmentTrans.perspectiveGeometryNumRows       = static_cast<UINT8>(resultMatrices.numRows);

            //TBD: Change from active sensor to what was passed to the algo
            gyroAlignmentTrans.transformDefinedOnWidth          = m_perSensorData[sensorIndex].inputSize.width;
            gyroAlignmentTrans.transformDefinedOnHeight         = m_perSensorData[sensorIndex].inputSize.height;
            gyroAlignmentTrans.ReusePerspectiveTransform        = 0;

            memcpy(&gyroAlignmentTrans.perspectiveTransformArray, &resultMatrices.perspMatrices[0].T[0],
                   sizeof(NcLibPerspTransformSingle));
        }
        else
        {
            LOG_INFO(CamxLogGroupChi, "Algo has alignment matrix disabled for req id %" PRId64, requestId);
        }

        if ((FALSE == pAlgoResult->stabilizationTransform.matrices.enable) &&
            (FALSE == pAlgoResult->stabilizationTransform.grid.enable))
        {
            LOG_ERROR(CamxLogGroupChi, "ERROR: Algo has no perspective or grid transform for req id %" PRId64, requestId);
        }
    }
    else
    {
        // Fill Identity transform
        LOG_ERROR(CamxLogGroupChi, "Algo has no output for request id %" PRId64, requestId);
        perspectiveTrans.perspectiveTransformEnable       = TRUE;
        perspectiveTrans.perspectiveConfidence            = 128;
        perspectiveTrans.byPassAlignmentMatrixAdjustement = TRUE;
        perspectiveTrans.perspetiveGeometryNumColumns     = 1;
        perspectiveTrans.perspectiveGeometryNumRows       = 1;

        //TBD: Change from active sensor to what was passed to the algo
        perspectiveTrans.transformDefinedOnWidth   = m_perSensorData[sensorIndex].inputSize.width;
        perspectiveTrans.transformDefinedOnHeight  = m_perSensorData[sensorIndex].inputSize.height;
        perspectiveTrans.ReusePerspectiveTransform = 0;
        memcpy(&perspectiveTrans.perspectiveTransformArray, &perspArray, sizeof(NcLibPerspTransformSingle));

        gyroAlignmentTrans.perspectiveTransformEnable       = FALSE;
        gyroAlignmentTrans.perspectiveConfidence            = 128;
        gyroAlignmentTrans.byPassAlignmentMatrixAdjustement = TRUE;
        gyroAlignmentTrans.perspetiveGeometryNumColumns     = 1;
        gyroAlignmentTrans.perspectiveGeometryNumRows       = 1;

        //TBD: Change from active sensor to what was passed to the algo
        gyroAlignmentTrans.transformDefinedOnWidth      = m_perSensorData[sensorIndex].inputSize.width;
        gyroAlignmentTrans.transformDefinedOnHeight     = m_perSensorData[sensorIndex].inputSize.height;
        gyroAlignmentTrans.ReusePerspectiveTransform    = 0;
        memcpy(&gyroAlignmentTrans.perspectiveTransformArray, &perspArray, sizeof(NcLibPerspTransformSingle));

        // Fill identity grid
        gridTrans.gridTransformEnable = TRUE;
        gridTrans.reuseGridTransform  = 0;

        //TBD: Change from active sensor to what was passed to the algo
        gridTrans.transformDefinedOnWidth               = IcaVirtualDomainWidth  * IcaVirtualDomainQuantizationV20;
        gridTrans.transformDefinedOnHeight              = IcaVirtualDomainHeight * IcaVirtualDomainQuantizationV20;
        gridTrans.gridTransformArrayExtrapolatedCorners = FALSE;

        switch (m_ICAVersion)
        {
            case ChiICA30:
                gridTrans.geometry = ICAGeometryCol67Row51;
                for (UINT idx = 0; idx < (ICA30GridTransformWidth * ICA30GridTransformHeight); idx++)
                {
                    gridTrans.gridTransformArray[idx].x = gridArrayX30[idx];
                    gridTrans.gridTransformArray[idx].y = gridArrayY30[idx];
                }
                break;
            case ChiICA20:
                gridTrans.geometry = ICAGeometryCol35Row27;
                for (UINT idx = 0; idx < (ICA20GridTransformWidth * ICA20GridTransformHeight); idx++)
                {
                    gridTrans.gridTransformArray[idx].x = gridArrayX20[idx];
                    gridTrans.gridTransformArray[idx].y = gridArrayY20[idx];
                }
                break;
            case ChiICA10:
                gridTrans.geometry = ICAGeometryCol33Row25;
                for (UINT idx = 0; idx < (ICA10GridTransformWidth * ICA10GridTransformHeight); idx++)
                {
                    gridTrans.gridTransformArray[idx].x = gridArrayX[idx];
                    gridTrans.gridTransformArray[idx].y = gridArrayY[idx];
                }
                break;
            case ChiICAMax:
                gridTrans.gridTransformEnable = FALSE;
                break;
        }
    }

    if (ChiICAMax == m_ICAVersion)
    {
        // for gpu deployment type
        tagList[index]              = tagIdList[index];
        tagData[index].size         = sizeof(CHITAGDATA);
        tagData[index].requestId    = requestId;
        tagData[index].pData        = &gridTrans;
        tagData[index].dataSize     = sizeof(IPEICAGridTransform);
        index++;
    }

    metadataInfo.tagNum = index;
    g_ChiNodeInterface.pSetMetadata(&metadataInfo);

    if (ChiICAMax != m_ICAVersion)
    {
        UINT32 psTagList[] =
        {
            g_vendorTagId.ICAInPerspectiveTransformTagId,
            g_vendorTagId.ICAInGridOut2InTransformTagId,
            g_vendorTagId.ICAReferenceParamsTagId,
        };

        const UINT32     psTagSize              = sizeof(psTagList) / sizeof(psTagList[0]);
        CHITAGDATA       psTagData[psTagSize]   = { { 0 } };

        index = 0;
        // Publish matrix tag list idx 0
        psTagData[index].size      = sizeof(CHITAGDATA);
        psTagData[index].requestId = requestId;
        psTagData[index].pData     = &perspectiveTrans;
        psTagData[index].dataSize  = sizeof(IPEICAPerspectiveTransform);
        index++;

        // Publish ldc grid out2in tag list idx 1
        psTagData[index].size      = sizeof(CHITAGDATA);
        psTagData[index].requestId = requestId;
        psTagData[index].pData     = &gridTrans;
        psTagData[index].dataSize  = sizeof(IPEICAGridTransform);
        index++;

        // Publish ICA reference params for gyro alignment
        psTagData[index].size      = sizeof(CHITAGDATA);
        psTagData[index].requestId = requestId;
        psTagData[index].pData     = &gyroAlignmentTrans;
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV2Node::GetVirtualMargin
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiEISV2Node::GetMinTotalMargins(
    FLOAT* minTotalMarginX,
    FLOAT* minTotalMarginY)
{
    eis_1_2_0::chromatix_eis12Type*                        pEISChromatix  = NULL;
    eis_1_2_0::chromatix_eis12_reserveType::topStruct*     pTopStruct     = NULL;

    if (NULL != m_perSensorData[m_primarySensorIdx].pEISChromatix)
    {
        pEISChromatix               = m_perSensorData[m_primarySensorIdx].pEISChromatix;
        pTopStruct                  = &pEISChromatix->chromatix_eis12_reserve.top;
        *minTotalMarginX            = pTopStruct->minimal_total_margin;
        *minTotalMarginY            = pTopStruct->minimal_total_margin;
    }
    else
    {
        *minTotalMarginX = EISV2Margin;
        *minTotalMarginY = EISV2Margin;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV2Node::GetTotalMargins
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiEISV2Node::GetTotalMargins(
    const EISV2OverrideSettings*    overrideSettings,
    FLOAT*                          pTotalMarginX,
    FLOAT*                          pTotalMarginY)
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
        LOG_INFO(CamxLogGroupChi, "Target fps is %d", *reinterpret_cast<UINT32*>(pData));
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
        ChiBufferDimension videoDimension = *static_cast<ChiBufferDimension*>(pData);
        pData                             = NULL;

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
                *pTotalMarginY = pTopStruct->requested_total_margins_y_fhd_30;
            }
            else if (60 >= targetFPS)
            {
                *pTotalMarginY = pTopStruct->requested_total_margins_y_fhd_60;
            }
            else
            {
                *pTotalMarginY = pTopStruct->requested_total_margins_y_fhd_120;
            }
        }
        else if (2160 >= outDimension.height)
        {
            if (30 >= targetFPS)
            {
                *pTotalMarginY = pTopStruct->requested_total_margins_y_4k_30;
            }
            else if (60 >= targetFPS)
            {
                *pTotalMarginY = pTopStruct->requested_total_margins_y_4k_60;
            }
            else
            {
                *pTotalMarginY = pTopStruct->requested_total_margins_y_4k_120;
            }
        }
        else
        {
            if (30 >= targetFPS)
            {
                *pTotalMarginY = pTopStruct->requested_total_margins_y_8k_30;
            }
            else if (60 >= targetFPS)
            {
                *pTotalMarginY = pTopStruct->requested_total_margins_y_8k_60;
            }
            else
            {
                *pTotalMarginY = pTopStruct->requested_total_margins_y_8k_120;
            }
        }

        *pTotalMarginX = ChiNodeUtils::MinFLOAT(pTopStruct->minimal_total_margin, *pTotalMarginY);
    }
    else
    {
        // Use default values in case opening Chromatix failed
        LOG_INFO(CamxLogGroupChi, "Failed to open EIS Chromatix, using default margins values");
        *pTotalMarginX = EISV2Margin;
        *pTotalMarginY = EISV2Margin;
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

    // Convert requested margins from input to margins from output ( send as a margins request tag )
    LOG_INFO(CamxLogGroupChi, "Chromatix total margins from input: (%f, %f) from output: (%f, %f)",
        *pTotalMarginX,
        *pTotalMarginY,
        (1 / (1 - *pTotalMarginX) - 1),
        (1 / (1 - *pTotalMarginY) - 1));

    *pTotalMarginX = 1 / (1 - *pTotalMarginX) - 1;
    *pTotalMarginY = 1 / (1 - *pTotalMarginY) - 1;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV2Node::GetGyroFrequency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FLOAT ChiEISV2Node::GetGyroFrequency(
    UINT32 sensorIndex)
{
    FLOAT                                                  gyroFrequency  = GyroSamplingRate;
    eis_1_2_0::chromatix_eis12Type*                        pEISChromatix  = NULL;
    eis_1_2_0::chromatix_eis12_reserveType::topStruct*     pTopStruct     = NULL;

    if (NULL != m_perSensorData[sensorIndex].pEISChromatix)
    {
        pEISChromatix   = m_perSensorData[sensorIndex].pEISChromatix;
        pTopStruct      = &pEISChromatix->chromatix_eis12_reserve.top;
        gyroFrequency   = static_cast<FLOAT>(pTopStruct->gyro_frequency);
    }

    return ((gyroFrequency == 0) || (gyroFrequency > GyroSamplingRate)) ? GyroSamplingRate : gyroFrequency;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV2Node::GetAdditionalCropOffset
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiEISV2Node::GetAdditionalCropOffset()
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
        LOG_ERROR(CamxLogGroupChi, "Unexpected physical margin: %ux%u", width, height);
    }
    LOG_VERBOSE(CamxLogGroupChi, "Additional Crop Offset: %ux%u",
                m_additionalCropOffset.widthPixels, m_additionalCropOffset.heightLines);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEISV2Node::ConvertICA20GridToICA10Grid
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV2Node::ConvertICA20GridToICA10Grid(
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
/// ChiEISV2Node::getICAGridGeometryVersion
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiEISV2Node::getICAGridGeometryVersion(
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
/// ChiEISV2Node::GetCameraIndexFromID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiEISV2Node::GetCameraIndexFromID(UINT32 cameraId)
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
/// ChiEISV2Node::GetPerCameraConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiEISV2Node::GetPerCameraConfig()
{
    CDKResult result = CDKResultSuccess;
    void*     pData  = ChiNodeUtils::GetMetaData(0,
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

            m_numOfLinkedCameras = pPhysicalCameraConfigs->numPhysicalCameras;
            m_primaryCameraId    = pPhysicalCameraConfigs->primaryCameraId;

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
/// ChiEISV2Node::GetDeploymentType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
cam_is_deployment_type_t ChiEISV2Node::GetDeploymentType()
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
/// ChiEISV2Node::GetSensorAppliedCrop
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
WindowRegionF ChiEISV2Node::GetSensorAppliedCrop(UINT32 sensorIdx)
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
