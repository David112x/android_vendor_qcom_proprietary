////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcustomization.cpp
/// @brief CamX Customization Source File
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcustomization.h"
#include "camxdebugprint.h"
#include "camxhwenvironment.h"
#include "camxsettingsmanager.h"

CAMX_NAMESPACE_BEGIN

static CAMXCustomizeOEMInterface   g_oemInterface;
static CAMXCustomizeCAMXInterface  g_camxInterface;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// InitializeExtendedPlatformStaticCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID InitializeExtendedPlatformStaticCaps(
    PlatformStaticCaps* pCaps, UINT32 numberSensors)
{
    CAMX_ASSERT(pCaps != NULL);
    CAMX_UNREFERENCED_PARAM(numberSensors);

/*  Sample code for OEM to customize the PlatformStaticCaps

    static const DimensionCap SupportedImageSizes[] =
    {
        // This is the pre-defined table, and available image sizes capability
        // can be modified base on sensor capability
        { 4032, 3024 },  // 12M 1.33
        { 3840, 2160 },  // 1.77
        { 3264, 2448 },  // 8M 1.33
        { 3264, 1836 },  // 1.77
        { 3008, 2256 },
        { 2976, 2976 },  // 1.0
        { 2688, 1512 },  // 4MP  292 1.77
        { 2560, 1920 },  // 1.33
        { 2048, 1536 },  // 3MP 1.33
        { 2048, 1152 },  // 2.4M
        { 1920, 1080 },  // 1080p
        { 1280, 960  },  // SXGA
        { 1280, 720  },  // 720p
        {  800,  600 },
        {  720,  480 },  // 480p
        {  640,  480 },  // VGA
        {  640,  360 },
        {  512,  288 },  // 1.777777
        {  352,  288 },  // CIF
        {  320,  240 },  // QVGA
        {  256,  154 },  // 1.66233
        {  176,  144 },
    };
    static const SIZE_T NumSupportedImageSizes                  = sizeof(SupportedImageSizes) / sizeof(DimensionCap);

    for (UINT sensorIndex = 0; sensorIndex < numberSensors; sensorIndex++)
    {
        CAMX_ASSERT(pCaps[sensorIndex].pExtended == NULL);

        // allocate the memory for pExtend only if PlatformStaticCapsExtended is used
        pCaps[sensorIndex].pExtended =
            static_cast<PlatformStaticCapsExtended*>(CAMX_CALLOC(sizeof(PlatformStaticCapsExtended)));
        CAMX_ASSERT(pCaps[sensorIndex].pExtended != NULL);

        // Update the existing static caps with customized supported image size
        pCaps[sensorIndex].numDefaultImageSizes = NumSupportedImageSizes;
        for (UINT8 i = 0; i < NumSupportedImageSizes; i++)
        {
            pCaps[sensorIndex].defaultImageSizes[i].width   = SupportedImageSizes[i].width;
            pCaps[sensorIndex].defaultImageSizes[i].height  = SupportedImageSizes[i].height;
        }

        // AE compensation range.
        pCaps[sensorIndex].minAECompensationValue = -20;
        pCaps[sensorIndex].maxAECompensationValue = 20;

        // AE compensation step
        pCaps[sensorIndex].AECompensationSteps.numerator   = 1;
        pCaps[sensorIndex].AECompensationSteps.denominator = 10;

        pCaps[sensorIndex].minPostRawSensitivityBoost = 100;

        // sensor specific change
        if (sensorIndex == 1)
        {
            UINT size = 0;
            pCaps[sensorIndex].abberationModes[size++]  = ColorCorrectionAberrationModeOff;
            pCaps[sensorIndex].numAbberationsModes      = size;

            size                                        = 0;
            pCaps[sensorIndex].edgeModes[size++]        = EdgeModeFast;
            pCaps[sensorIndex].edgeModes[size++]        = EdgeModeHighQuality;
            pCaps[sensorIndex].edgeModes[size++]        = EdgeModeOff;
            pCaps[sensorIndex].edgeModes[size++]        = EdgeModeZeroShutterLag;
            CAMX_ASSERT(size <= EdgeModeEnd);
            pCaps[sensorIndex].numEdgeModes             = size;
        }
    }
*/
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ReleaseExtendedPlatformStaticCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID ReleaseExtendedPlatformStaticCaps(
    PlatformStaticCaps* pCaps, UINT32 numberSensors)
{
    if (NULL != pCaps)
    {
        for (UINT sensorIndex = 0; sensorIndex < numberSensors; sensorIndex++)
        {
            if (NULL != pCaps[sensorIndex].pExtended)
            {
                CAMX_FREE(pCaps[sensorIndex].pExtended);
                pCaps[sensorIndex].pExtended = NULL;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// InitializeExtendedHwEnvironmentStaticCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID InitializeExtendedHwEnvironmentStaticCaps(
    HwEnvironmentStaticCaps* pCaps, UINT32 numberSensors)
{
    CAMX_ASSERT(pCaps != NULL);
    CAMX_UNREFERENCED_PARAM(numberSensors);

    // OEM to customize the HwEnvironmentStaticCaps

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ReleaseExtendedHwEnvironmentStaticCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID ReleaseExtendedHwEnvironmentStaticCaps(
    HwEnvironmentStaticCaps* pCaps, UINT32 numberSensors)
{
    if (NULL != pCaps)
    {
        for (UINT sensorIndex = 0; sensorIndex < numberSensors; sensorIndex++)
        {
            if (NULL != pCaps[sensorIndex].pExtended)
            {
                CAMX_FREE(pCaps[sensorIndex].pExtended);
                pCaps[sensorIndex].pExtended = NULL;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAMXCustomizeEntry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAMXCustomizeEntry(
    CAMXCustomizeOEMInterface** ppOEMInterface,
    CAMXCustomizeCAMXInterface* pCAMXInterface)
{
    g_oemInterface.pInitializeExtendedPlatformStaticCaps       = InitializeExtendedPlatformStaticCaps;
    g_oemInterface.pReleaseExtendedPlatformStaticCaps          = ReleaseExtendedPlatformStaticCaps;
    g_oemInterface.pInitializeExtendedHWEnvironmentStaticCaps  = InitializeExtendedHwEnvironmentStaticCaps;
    g_oemInterface.pReleaseExtendedHWEnvironmentStaticCaps     = ReleaseExtendedHwEnvironmentStaticCaps;

    *ppOEMInterface     = &g_oemInterface;
    g_camxInterface     = *pCAMXInterface;

    return;
}

CAMX_NAMESPACE_END