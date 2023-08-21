////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipepipelinetitan150.cpp
/// @brief IPE Pipeline for Titan 150
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxipepipelinetitan150.h"
#include "camxtitan17xcontext.h"
#include "camxipe2dlut10.h"
#include "camxipeanr10.h"
#include "camxipeasf30.h"
#include "camxipecac22.h"
#include "camxipechromaenhancement12.h"
#include "camxipechromasuppression20.h"
#include "camxipecolorcorrection13.h"
#include "camxipecolortransform12.h"
#include "camxipegamma15.h"
#include "camxipegrainadder10.h"
#include "camxipeica10.h"
#include "camxipeltm13.h"
#include "camxipesce11.h"
#include "camxipetf10.h"
#include "camxipeupscaler12.h"

CAMX_NAMESPACE_BEGIN

// This list will follow order of modules in real hardware
static IPEIQModuleInfo IQModulesList[] =
{
    { ISPIQModuleType::IPEICA,               IPEPath::INPUT,     TRUE,    IPEICA10::Create },
    { ISPIQModuleType::IPEANR,               IPEPath::INPUT,     TRUE,    IPEANR10::Create },
    { ISPIQModuleType::IPEICA,               IPEPath::REFERENCE, TRUE,    IPEICA10::Create },
    { ISPIQModuleType::IPETF,                IPEPath::INPUT,     TRUE,    IPETF10::Create },
    // No ISPIQModuleType::IPECAC
    { ISPIQModuleType::IPECST,               IPEPath::INPUT,     TRUE,    IPEColorTransform12::Create },
    { ISPIQModuleType::IPELTM,               IPEPath::INPUT,     TRUE,    IPELTM13::Create },
    { ISPIQModuleType::IPEColorCorrection,   IPEPath::INPUT,     TRUE,    IPEColorCorrection13::Create },
    { ISPIQModuleType::IPEGamma,             IPEPath::INPUT,     TRUE,    IPEGamma15::Create },
    { ISPIQModuleType::IPEChromaEnhancement, IPEPath::INPUT,     TRUE,    IPEChromaEnhancement12::Create },
    { ISPIQModuleType::IPE2DLUT,             IPEPath::INPUT,     TRUE,    IPE2DLUT10::Create },
    { ISPIQModuleType::IPEChromaSuppression, IPEPath::INPUT,     TRUE,    IPEChromaSuppression20::Create },
    // No ISPIQModuleType::IPESCE
    { ISPIQModuleType::IPEASF,               IPEPath::INPUT,     TRUE,    IPEASF30::Create },
    { ISPIQModuleType::IPEUpscaler,          IPEPath::INPUT,     TRUE,    IPEUpscaler12::Create },
    { ISPIQModuleType::IPEGrainAdder,        IPEPath::INPUT,     TRUE,    IPEGrainAdder10::Create },
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEPipelineTitan150::GetCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEPipelineTitan150::GetCapability(
    VOID* pCapabilityInfo)
{
    IPECapabilityInfo* pIPECapabilityInfo  = static_cast<IPECapabilityInfo*>(pCapabilityInfo);
    CamxResult         result              = CamxResultSuccess;
    BOOL               UBWCScaleRatioLimit = TRUE;

    if (NULL != pIPECapabilityInfo)
    {
        /// @todo (CAMX-727) Implement query capability from hardware/firmware.
        pIPECapabilityInfo->numIPEIQModules                      = sizeof(IQModulesList) / sizeof(IPEIQModuleInfo);
        pIPECapabilityInfo->pIPEIQModuleList                     = IQModulesList;

        // Striping library in firmware will be removed in future. Remove this setting once striping in FW is removed.
        pIPECapabilityInfo->swStriping                           = static_cast<Titan17xContext *>(
            m_pHwContext)->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->IPESwStriping;

        pIPECapabilityInfo->maxInputWidth[ICA_MODE_DISABLED]     = IPEMaxInputWidthICADisabled;  // ICA Disabled
        pIPECapabilityInfo->maxInputHeight[ICA_MODE_DISABLED]    = IPEMaxInputHeightICADisabled; // ICA Disabled
        pIPECapabilityInfo->maxInputWidth[ICA_MODE_ENABLED]      = IPEMaxInputWidthICAEnabled;   // ICA Enabled
        pIPECapabilityInfo->maxInputHeight[ICA_MODE_ENABLED]     = IPEMaxInputHeightICAEnabled;  // ICA Enabled

        UBWCScaleRatioLimit                                      = ImageFormatUtils::GetUBWCScaleRatioLimitationFlag();

        pIPECapabilityInfo->minInputWidth                        = IPEMinInputWidth;
        pIPECapabilityInfo->minInputHeight                       = IPEMinInputHeight;

        pIPECapabilityInfo->maxDownscale[UBWC_MODE_DISABLED]     = IPEMaxDownscaleLinear;               // LINEAR Format
        pIPECapabilityInfo->maxDownscale[UBWC_MODE_ENABLED]      =
            (UBWCScaleRatioLimit == FALSE) ? IPEMaxDownscaleLinear : IPEMaxDownscaleUBWC;               // UBWC   Format
        pIPECapabilityInfo->maxUpscale[UBWC_MODE_DISABLED]       = TITAN1xxIPEMaxUpscaleLinear;         // LINEAR Format
        pIPECapabilityInfo->maxUpscale[UBWC_MODE_ENABLED]        =
            (UBWCScaleRatioLimit == FALSE) ? TITAN1xxIPEMaxUpscaleLinear : TITAN1xxIPEMaxUpscaleUBWC;   // UBWC   Format

        pIPECapabilityInfo->numIPE                               = m_pHwContext->GetNumberOfIPE();

        pIPECapabilityInfo->LENRSupport                          = FALSE;
        pIPECapabilityInfo->referenceLookaheadTransformSupported = FALSE;
        pIPECapabilityInfo->minOutputWidthUBWC                   = TITAN150IPEMinOutputWidthUBWC;
        pIPECapabilityInfo->minOutputHeightUBWC                  = TITAN150IPEMinOutputHeightUBWC;
        pIPECapabilityInfo->UBWCSupportedVersionMask             = UBWCVersion2Mask;
        pIPECapabilityInfo->UBWCLossySupport                     = UBWCLossless;
        pIPECapabilityInfo->lossy10bitWidth                      = UINT32_MAX;
        pIPECapabilityInfo->lossy10bitHeight                     = UINT32_MAX;
        pIPECapabilityInfo->lossy8bitWidth                       = UINT32_MAX;
        pIPECapabilityInfo->lossy8bitHeight                      = UINT32_MAX;
        pIPECapabilityInfo->realtimeClockEfficiency              = IPE17xClockEfficiency;
        pIPECapabilityInfo->nonrealtimeClockEfficiency           = IPE17xClockEfficiency;
        pIPECapabilityInfo->ICAVersion                           = ICAVersion10;
        pIPECapabilityInfo->LDCSupport                           = FALSE;
        pIPECapabilityInfo->isPostfilterWithBlend                = TRUE;
        pIPECapabilityInfo->hasMFlimit                           = TRUE;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Pointer");
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEPipelineTitan150::IPEPipelineTitan150
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEPipelineTitan150::IPEPipelineTitan150(
    HwContext* pHwContext)
{
    m_pHwContext = pHwContext;
}

CAMX_NAMESPACE_END