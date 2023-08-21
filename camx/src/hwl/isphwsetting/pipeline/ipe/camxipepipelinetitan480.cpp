////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipepipelinetitan480.cpp
/// @brief IPE Pipeline for Titan 480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxipepipelinetitan480.h"
#include "camxtitan17xcontext.h"
#include "camxipe2dlut10.h"
#include "camxipeanr10.h"
#include "camxipeasf30.h"
#include "camxipecac23.h"
#include "camxipechromaenhancement12.h"
#include "camxipechromasuppression20.h"
#include "camxipecolorcorrection13.h"
#include "camxipecolortransform12.h"
#include "camxipegamma15.h"
#include "camxipegrainadder10.h"
#include "camxipehnr10.h"
#include "camxipeica30.h"
#include "camxipeltm14.h"
#include "camxipesce11.h"
#include "camxipetf20.h"
#include "camxipeupscaler20.h"
#include "camxipelenr10.h"

CAMX_NAMESPACE_BEGIN

// This list will follow order of modules in real hardware
static IPEIQModuleInfo IQModulesList[] =
{
    { ISPIQModuleType::IPEICA,               IPEPath::INPUT,     TRUE,    IPEICA30::Create },
    { ISPIQModuleType::IPE2DLUT,             IPEPath::INPUT,     TRUE,    IPE2DLUT10::Create },
    { ISPIQModuleType::IPEANR,               IPEPath::INPUT,     TRUE,    IPEANR10::Create },
    { ISPIQModuleType::IPEICA,               IPEPath::REFERENCE, TRUE,    IPEICA30::Create },
    { ISPIQModuleType::IPETF,                IPEPath::INPUT,     TRUE,    IPETF20::Create },
    { ISPIQModuleType::IPEHNR,               IPEPath::INPUT,     TRUE,    IPEHNR10::Create },
    { ISPIQModuleType::IPELENR,              IPEPath::INPUT,     TRUE,    IPELENR10::Create },
    { ISPIQModuleType::IPECAC,               IPEPath::INPUT,     TRUE,    IPECAC23::Create },
    { ISPIQModuleType::IPECST,               IPEPath::INPUT,     TRUE,    IPEColorTransform12::Create },
    { ISPIQModuleType::IPELTM,               IPEPath::INPUT,     TRUE,    IPELTM14::Create },
    { ISPIQModuleType::IPEColorCorrection,   IPEPath::INPUT,     TRUE,    IPEColorCorrection13::Create },
    { ISPIQModuleType::IPEGamma,             IPEPath::INPUT,     TRUE,    IPEGamma15::Create },
    { ISPIQModuleType::IPEChromaEnhancement, IPEPath::INPUT,     TRUE,    IPEChromaEnhancement12::Create },
    { ISPIQModuleType::IPEChromaSuppression, IPEPath::INPUT,     TRUE,    IPEChromaSuppression20::Create },
    // No ISPIQModuleType::IPESCE
    { ISPIQModuleType::IPEASF,               IPEPath::INPUT,     TRUE,    IPEASF30::Create },
    { ISPIQModuleType::IPEUpscaler,          IPEPath::INPUT,     TRUE,    IPEUpscaler20::Create },
    { ISPIQModuleType::IPEGrainAdder,        IPEPath::INPUT,     TRUE,    IPEGrainAdder10::Create },
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEPipelineTitan480::GetCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEPipelineTitan480::GetCapability(
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
        pIPECapabilityInfo->minInputHeight                        = IPEMinInputHeight;

        pIPECapabilityInfo->maxDownscale[UBWC_MODE_DISABLED]     = IPEMaxDownscaleLinear;       // LINEAR Format
        pIPECapabilityInfo->maxDownscale[UBWC_MODE_ENABLED]      =
            (UBWCScaleRatioLimit == FALSE) ? IPEMaxDownscaleLinear : IPEMaxDownscaleUBWC;   // UBWC   Format
        pIPECapabilityInfo->maxUpscale[UBWC_MODE_DISABLED]       = IPEMaxUpscaleLinear;         // LINEAR Format
        pIPECapabilityInfo->maxUpscale[UBWC_MODE_ENABLED]        =
            (UBWCScaleRatioLimit == FALSE) ? IPEMaxUpscaleLinear : IPEMaxUpscaleUBWC;       // UBWC   Format

        pIPECapabilityInfo->numIPE                               = m_pHwContext->GetNumberOfIPE();

        pIPECapabilityInfo->minOutputWidthUBWC                   = TITAN480IPEMinOutputWidthUBWC;
        pIPECapabilityInfo->minOutputHeightUBWC                  = TITAN480IPEMinOutputHeightUBWC;

        pIPECapabilityInfo->UBWCSupportedVersionMask             = UBWCVersion2Mask | UBWCVersion3Mask | UBWCVersion4Mask;
        pIPECapabilityInfo->UBWCLossySupport                     = UBWCLossy;
        pIPECapabilityInfo->lossy10bitWidth                      = 1280;
        pIPECapabilityInfo->lossy10bitHeight                     = 720;
        pIPECapabilityInfo->lossy8bitWidth                       = 3840;
        pIPECapabilityInfo->lossy8bitHeight                      = 2160;

        pIPECapabilityInfo->LENRSupport                          = TRUE;
        pIPECapabilityInfo->referenceLookaheadTransformSupported = TRUE;
        pIPECapabilityInfo->disableLENRDS4Buffer                 = static_cast<Titan17xContext *>(
            m_pHwContext)->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->disableLENRDS4Buffer;
        pIPECapabilityInfo->ICAVersion                           = ICAVersion30;
        pIPECapabilityInfo->ICAGridGeometryRows                  = ICA30GridTransformHeight;
        pIPECapabilityInfo->ICAGridGeometryColumns               = ICA30GridTransformWidth;
        pIPECapabilityInfo->realtimeClockEfficiency              = IPE480RealtimeClockEfficiency;
        pIPECapabilityInfo->nonrealtimeClockEfficiency           = IPE480NonRealtimeClockEfficiency;
        pIPECapabilityInfo->LDCSupport                           = TRUE;
        pIPECapabilityInfo->isPostfilterWithBlend                = FALSE;
        pIPECapabilityInfo->hasMFlimit                           = FALSE;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Pointer");
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEPipelineTitan480::IPEPipelineTitan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEPipelineTitan480::IPEPipelineTitan480(
    HwContext* pHwContext)
{
    m_pHwContext = pHwContext;
}

CAMX_NAMESPACE_END
