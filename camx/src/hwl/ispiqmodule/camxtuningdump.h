////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxispiqmodule.h
/// @brief TITAN IQ Module base class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXTUNINGDUMP_H
#define CAMXTUNINGDUMP_H

#include "camxstatsdebugdatawriter.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief BPS Firmware Version
const UINT32 BPSFirmwareAPIVersion            = 0x2000088; // Version should match FW_API_VERSION @ icpdefs.h
                                                           // IF BpsIQSettings size changes
/// @brief BPS settings that FW configure (size)
const UINT BPSFirmwareSettingSize             = 259;       // Update with size of BpsIQSettings

/// @brief IPE Firmware Version
const UINT32 IPEFirmwareAPIVersion            = 0x2000088; // Version should match FW_API_VERSION @ icpdefs.h
                                                           // IF IpeIQSettings size changes
/// @brief IPE settings that FW configure
const UINT IPEFirmwareSettingSize             = 919;       // Update with size of IpeIQSettings

/// @brief Tuning face data
const UINT TuningMaxFaceNumber                = 10;

/// @brief IFE ABF register and DMI sizes
const UINT IFEABFRegisters1Titan17x           = 1;
const UINT IFEABFRegisters2Titan17x           = 22;
const UINT IFEABFRegisters1Titan480           = 3;
const UINT IFEABFRegisters2Titan480           = 46;
const UINT IFEABFLUTEntriesNoise              = 64;
const UINT IFEABFLUTEntriesActivity           = 32;
const UINT IFEABFLUTEntriesDark               = 42;

/// @brief IFE BLS register sizes
const UINT IFEBLSRegistersTitan17x            = 3;
const UINT IFEBLSRegisters1Titan480           = 1;
const UINT IFEBLSRegisters2Titan480           = 4;

/// @brief IFE BPCBCC register sizes
const UINT IFEBPCBCCRegister                  = 8;

/// @brief IFE CC register sizes
const UINT IFECCRegisters1Titan17x            = 13;
const UINT IFECCRegisters1Titan480            = 1;
const UINT IFECCRegisters2Titan480            = 9;

/// @brief IFE CST register sizes
const UINT IFECSTRegistersTitan17x            = 12;
const UINT IFECSTRegistersTitan480            = 13;

/// @brief IFE Demosaic register sizes
const UINT IFEDemosaicRegisters               = 3;

/// @brief IFE Demux register sizes
const UINT IFEDemuxRegisters                  = 7;

/// @brief IFE Gamma register sizes
const UINT IFEGammaRegistersTitan17x          = 1;
const UINT IFEGammaRegistersTitan480          = 3;

/// @brief IFE GTM register sizes
const UINT IFEGTMRegistersTitan17x            = 1;
const UINT IFEGTMRegistersTitan480            = 3;

/// @brief IFE HDR register sizes
const UINT IFEHDRRegisters1Titan17x           = 18;
const UINT IFEHDRRegisters2Titan17x           = 1;
const UINT IFEHDRRegisters3Titan17x           = 2;
const UINT IFEHDRRegisters1Titan480           = 1;
const UINT IFEHDRRegisters2Titan480           = 19;

/// @brief IFE Linearization register sizes
const UINT IFELinearizationRegistersTitan17x  = 33;
const UINT IFELinearizationRegisters1Titan480 = 3;
const UINT IFELinearizationRegisters2Titan480 = 16;
const UINT IFELinearizationLUTEntries         = 36;

/// @brief IFE LSC register sizes
const UINT IFELSCRegistersTitan17x            = 11;
const UINT IFELSCRegisters1Titan480           = 3;
const UINT IFELSCRegisters2Titan480           = 11;

/// @brief IFE PDPC register sizes
const UINT IFEPDPCRegistersTitan17x           = 11;
const UINT IFEPDPCRegistersTitan480           = 23;
const UINT IFEPDPCLUTEntries                  = 64;

/// @brief IFE Pedestal register sizes
const UINT IFEPedestalRegistersTitan17x       = 11;
const UINT IFEPedestalRegisters1Titan480      = 2;
const UINT IFEPedestalRegisters2Titan480      = 5;

/// @brief IFE WB register sizes
const UINT IFEWBRegistersTitan17x             = 4;
const UINT IFEWBRegistersTitan480             = 4;

/// @brief Gamma IFE & BPS v1.6 LUT details
const UINT GammaLUTEntries              = 64;

/// @brief Gamma IPE v1.5 LUT details
const UINT IPEGamma15LUTEntries         = 256;

/// @brief GTM v1.0 LUT details
const UINT GTMLUTEntries                = 64;

/// @brief BPS LSC tuning data
const UINT      LSCHorizontalMeshPoint  = 17;
const UINT      LSCVerticalMeshPoint    = 13;
const UINT32    LSCMeshSize             = LSCHorizontalMeshPoint * LSCVerticalMeshPoint;

/// @brief LSC34 tuning data
const UINT      LSC34ConfigRegisters      = 7;

/// @brief BPS LSC40 tuning data
const UINT      LSC40ConfigRegisters    = 11;

/// @brief Pedestal v1.3 tuning data
const UINT      PedestalConfigRegisters = 5;
const UINT32    PedestalDMISetSizeDword = 130;

/// @brief BPS Linearization tuning data
const UINT      LinearizationConfigKneepoints   = 16;
const UINT32    BPSLinearizationLUTEntries      = 36;

/// @brief BPS: BPC & PDPC 20 tuning data
const UINT      BPCPDPC20ConfigRegister0    = 10;
const UINT      BPCPDPC20ConfigRegister1    = 3;

/// @brief BPS: BPC & PDPC 30 tuning data
const UINT      BPCPDPC30ConfigRegister0  = 10;
const UINT      BPCPDPC30ConfigRegister1  = 10;

/// @brief BPS: BPC & PDPC tuning data
const UINT32    BPSPDPCLUTEntries         = 64;

/// @brief BPS HDR 22 tuning data
const UINT      BPSHDR22ReconRegisters    = 9;
const UINT      BPSHDR22MACRegisters      = 7;

/// @brief BPS HDR 30 tuning data
const UINT      BPSHDR30Registers       = 19;

/// @brief GIC tuning data
const UINT      BPSGICRegisters         = 22;
const UINT32    BPSGICLUTEntries        = 64;

/// @brief ABF tuning data
const UINT      BPSABFRegisters             = 46;
const UINT32    BPSABFLUTEntriesNoise       = 64;
const UINT32    BPSABFLUTEntriesActivity    = 32;
const UINT32    BPSABFLUTEntriesDark        = 42;

/// @brief WB tuning data
const UINT      BPSWBConfigRegisters    = 4;

/// @brief Demosaic tuning data
const UINT      BPSDemosaicConfigRegisters  = 2;

/// @brief Demux tuning data
const UINT      BPSDemuxConfigRegisters = 7;

/// @brief Color Correction data
const UINT      BPSCCConfigRegisters    = 9;

/// @brief Color Space Transform 12 data
const UINT      CST12ConfigRegisters      = 12;

/// @brief HNR 10 data
const UINT      HNR10ConfigRegisters       = 48;
const UINT32    HNR10LUTEntriesLNR         = 33;
const UINT32    HNR10LUTEntriesFNR         = 17;
const UINT32    HNR10LUTEntriesFNRAC       = 17;
const UINT32    HNR10LUTEntriesSNR         = 17;
const UINT32    HNR10LUTEntriesBlendLNR    = 17;
const UINT32    HNR10LUTEntriesBlendSNR    = 17;

/// @brief IPE ICA 10 data
const UINT32    IPEICA10LUTEntriesPerspective = 72;
const UINT32    IPEICA10LUTEntriesGrid0       = 832;
const UINT32    IPEICA10LUTEntriesGrid1       = 832;

// @brief IPE ICA 20 data
const UINT32    IPEICA20LUTEntriesPerspective = 72;
const UINT32    IPEICA20LUTEntriesGrid        = 1965;

// @brief IPE ICA 30 data
const UINT32    IPEICA30LUTEntiresPerspective = 72;
const UINT32    IPEICA30LUTEntiresGrid0       = 1712; // ICA30 LUT initialized as 32 bit buffer
const UINT32    IPEICA30LUTEntiresGrid1       = 1712; // of size 3424 but used as 64 bit buffer

/// @brief IPE ANR 10 data
const UINT32 IPEANR10DCBlend1ConfigRegisters          = 1;
const UINT32 IPEANR10RNFConfigRegisters               = 32;
const UINT32 IPEANR10DCUSConfigRegisters              = 1;
const UINT32 IPEANR10CFilter2ConfigRegisters          = 12;
const UINT32 IPEANR10DCBlend2ConfigRegisters          = 22;
const UINT32 IPEANR10CYLPFPreLensGainConfigRegisters  = 14;
const UINT32 IPEANR10CYLPFLensGainConfigRegisters     = 45;
const UINT32 IPEANR10CYLPFPostLensGainConfigRegisters = 125;
const UINT32 IPEANR10CNRConfigRegisters               = 75;

/// @brief IPE TF 10 data
const UINT32 IPETF10ConfigRegisters     = 94;
/// @brief IPE TF 20 data
const UINT32 IPETF20ConfigRegisters     = 99;

/// @brief IPE CAC 2x data
const UINT32 IPECAC2xConfigRegisters      = 2;

/// @brief IPE LTM data
const UINT   IPELTMExposureIndexPrevious    = 0;
const UINT   IPELTMExposureIndexCurrent     = 1;
const UINT   IPELTMExposureIndexCount       = 2;

/// @brief IPE LTM13 data
const UINT32 IPELTM13LUTEntriesWeight         = 12;
const UINT32 IPELTM13LUTEntriesLA0            = 64;
const UINT32 IPELTM13LUTEntriesLA1            = 64;
const UINT32 IPELTM13LUTEntriesCurve          = 64;
const UINT32 IPELTM13LUTEntriesScale          = 64;
const UINT32 IPELTM13LUTEntriesMask           = 64;
const UINT32 IPELTM13LUTEntriesLCEPositive    = 16;
const UINT32 IPELTM13LUTEntriesLCENegative    = 16;
const UINT32 IPELTM13LUTEntriesRGamma         = 64;
const UINT32 IPELTM13LUTEntriesRGamma0        = 64;
const UINT32 IPELTM13LUTEntriesRGamma1        = 64;
const UINT32 IPELTM13LUTEntriesRGamma2        = 64;
const UINT32 IPELTM13LUTEntriesRGamma3        = 64;
const UINT32 IPELTM13LUTEntriesRGamma4        = 64;
const UINT32 IPELTM13LUTEntriesAverage        = 140;

/// @brief IPE LTM 14 data
const UINT32 IPELTM14LUTEntriesWeight         = 12;
const UINT32 IPELTM14LUTEntriesLA             = 64;
const UINT32 IPELTM14LUTEntriesCurve          = 64;
const UINT32 IPELTM14LUTEntriesScale          = 64;
const UINT32 IPELTM14LUTEntriesMask           = 64;
const UINT32 IPELTM14LUTEntriesLCEPositive    = 16;
const UINT32 IPELTM14LUTEntriesLCENegative    = 16;
const UINT32 IPELTM14LUTEntriesRGamma         = 64;
const UINT32 IPELTM14LUTEntriesAverage        = 140;

/// @brief IPE Color correction 13 data
const UINT32 IPECC13ConfigRegisters       = 9;

/// @brief IPE 2D LUT 10 data
const UINT32 IPE2DLUT10ConfigRegisters            = 11;
const UINT32 IPE2DLUT10EntriesHue0                = 112;
const UINT32 IPE2DLUT10EntriesHue1                = 112;
const UINT32 IPE2DLUT10EntriesHue2                = 112;
const UINT32 IPE2DLUT10EntriesHue3                = 112;
const UINT32 IPE2DLUT10EntriesSaturation0         = 112;
const UINT32 IPE2DLUT10EntriesSaturation1         = 112;
const UINT32 IPE2DLUT10EntriesSaturation2         = 112;
const UINT32 IPE2DLUT10EntriesSaturation3         = 112;
const UINT32 IPE2DLUT10EntriesInverseHue          = 24;
const UINT32 IPE2DLUT10EntriesInverseSaturation   = 15;
const UINT32 IPE2DLUT10Entries1DHue               = 25;
const UINT32 IPE2DLUT10Entries1DSaturation        = 16;

/// @brief IPE Chroma Enhancement 12 data
const UINT32 IPEChromaEnhancement12ConfigRegisters    = 8;

/// @brief IPE Chroma Suppression 20 data
const UINT32 IPEChromaSuppression20ConfigRegisters    = 42;

/// @brief IPE Skin Color Enhancement 11 data
const UINT32 IPESCE11ConfigRegisters                  = 39;

/// @todo (CAMX-2846) Tuning data has been keep enough for the 68 registers, but writing only 65, data for others,
///                   should be obtain from FW.
///                   Change IPEASF30ConfigRegisters back to 68 when this is fixed

/// @brief IPE Adaptive Spatial Filter (ASF) 30 data
const UINT32 IPEASF30ConfigRegisters                                    = 65;
const UINT32 IPEASF30EntriesLayer1ActivityNormalGainPosGainNegSoftThLUT = 256;
const UINT32 IPEASF30EntriesLayer1GainWeightLUT                         = 256;
const UINT32 IPEASF30EntriesLayer1SoftThresholdWeightLUT                = 256;
const UINT32 IPEASF30EntriesLayer2ActivityNormalGainPosGainNegSoftThLUT = 256;
const UINT32 IPEASF30EntriesLayer2GainWeightLUT                         = 256;
const UINT32 IPEASF30EntriesLayer2SoftThresholdWeightLUT                = 256;
const UINT32 IPEASF30EntriesChromaGradientPosNegLUT                     = 256;
const UINT32 IPEASF30EntriesContrastPosNegLUT                           = 256;
const UINT32 IPEASF30EntriesSkinActivityGainLUT                         = 17;

/// @brief IPE Upscaler 20 data
const UINT32 IPEUpscaler20Registers       = 19;
const UINT32 IPEUpscaler20EntriesALUT     = 168;
const UINT32 IPEUpscaler20EntriesBLUT     = 96;
const UINT32 IPEUpscaler20EntriesCLUT     = 96;
const UINT32 IPEUpscaler20EntriesDLUT     = 80;

/// @brief IPE Grain Adder 10 (GRA) data
const UINT32 IPEGrainAdder10EntriesLUT  = 32;

/// @brief IQ OEM Costom tuning data
const UINT32 IQOEMTuningData            = 256;

/// @brief LENR 10 registers configuration
const UINT32 LENR10ConfigRegisters      = 151;

/// @brief: Gamma LUT channels, place holder for tuning-metadata only
enum GammaLUTChannel
{
    GammaLUTChannelR = 0,   ///< R channel
    GammaLUTChannelG = 1,   ///< G channel
    GammaLUTChannelB = 2,   ///< B channel
    GammaMaxChannel  = 3,   ///< Max LUTs
};

/// @brief Structure to encapsulate ABF34 LUTs data
struct IFEABF34LUT
{
    UINT32 noiseLUT[IFEABFLUTEntriesNoise];
} CAMX_PACKED;

/// @brief Structure to encapsulate ABF40 LUTs data
struct IFEABF40LUT
{
    UINT32 noiseLUT[IFEABFLUTEntriesNoise];
    UINT32 activityLUT[IFEABFLUTEntriesActivity];
    UINT32 darkLUT[IFEABFLUTEntriesDark];
} CAMX_PACKED;

/// @brief Structure to encapsulate Linearization 33 LUT data
struct IFELinearization33LUT
{
    UINT64 linearizationLUT[IFELinearizationLUTEntries];
} CAMX_PACKED;

/// @brief Structure to encapsulate Linearization 34 LUT data
struct IFELinearization34LUT
{
    UINT32 linearizationLUT[IFELinearizationLUTEntries];
} CAMX_PACKED;

/// @brief Structure to encapsulate PDPC11 LUT data
struct IFEPDPC11LUT
{
    UINT32 IFEPDPCMaskLUT[IFEPDPCLUTEntries];
} CAMX_PACKED;

/// @brief Structure to encapsulate PDPC30 LUT data
struct IFEPDPC30LUT
{
    UINT64 IFEPDPCMaskLUT[IFEPDPCLUTEntries];
    UINT32 IFEPDPCNoiseLUT[IFEPDPCLUTEntries];
} CAMX_PACKED;

/// @brief Structure to encapsulate IFE & BPS Gamma curve data
struct GammaCurve
{
    UINT32 curve[GammaLUTEntries];
} CAMX_PACKED;

/// @brief Structure to encapsulate GTM curve data
struct GTMLUT
{
    UINT64  LUT[GTMLUTEntries];  ///< GTM LUT
} CAMX_PACKED;

/// @brief Structure to encapsulate LSC 34 LUT data
struct LSC34MeshLUT
{
    /// register info also from register dump
    UINT32 GRRLUT[LSCMeshSize];
    UINT32 GBBLUT[LSCMeshSize];
} CAMX_PACKED;

/// @brief Structure to encapsulate LSC 40 LUT data
struct LSC40MeshLUT
{
    /// register info also from register dump
    UINT32 GRRLUT[LSCMeshSize];
    UINT32 GBBLUT[LSCMeshSize];
    UINT32 gridLUT[LSCMeshSize];
} CAMX_PACKED;

/// @brief Structure to encapsulate Pedestal LUT data
struct PedestalLUT
{
    /// register info also from register dump
    UINT32 GRRLUT[PedestalDMISetSizeDword];
    UINT32 GBBLUT[PedestalDMISetSizeDword];
} CAMX_PACKED;

/// @brief Structure to encapsulate Linearization LUT data
struct BPSLinearizationLUT
{
    UINT32 linearizationLUT[BPSLinearizationLUTEntries];
} CAMX_PACKED;

/// @brief Structure to encapsulate PDPC LUT data
struct BPSPDPC20LUT
{
    UINT32 BPSPDPCMaskLUT[BPSPDPCLUTEntries];
} CAMX_PACKED;

/// @brief Structure to encapsulate PDPC30  LUT data
struct BPSPDPC30LUT
{
    UINT64 BPSPDPCMaskLUT[BPSPDPCLUTEntries];
    UINT32 BPSPDPCNoiseLUT[BPSPDPCLUTEntries];
} CAMX_PACKED;
/// @brief Structure to encapsulate GIC LUT data
struct BPSGICLUT
{
    UINT32 BPSGICNoiseLUT[BPSGICLUTEntries];
} CAMX_PACKED;

/// @brief Structure to encapsulate ABF LUTs data
struct BPSABFLUT
{
    UINT32 noiseLUT[BPSABFLUTEntriesNoise];
    UINT32 noise1LUT[BPSABFLUTEntriesNoise];
    UINT32 activityLUT[BPSABFLUTEntriesActivity];
    UINT32 darkLUT[BPSABFLUTEntriesDark];
} CAMX_PACKED;

/// @brief Structure to encapsulate HNR LUTs data
struct HNR10LUT
{
    UINT32 LNRLUT[HNR10LUTEntriesLNR];
    UINT32 FNRAndClampLUT[HNR10LUTEntriesFNR];
    UINT32 FNRAcLUT[HNR10LUTEntriesFNRAC];
    UINT32 SNRLUT[HNR10LUTEntriesSNR];
    UINT32 blendLNRLUT[HNR10LUTEntriesBlendLNR];
    UINT32 blendSNRLUT[HNR10LUTEntriesBlendSNR];
} CAMX_PACKED;

/// @brief Structure to encapsulate IPE Gamma curve data
struct IPEGamma15Curve
{
    UINT32 curve[IPEGamma15LUTEntries];
} CAMX_PACKED;

/// @brief enumeration defining IPE path
enum IPETuningICAPath
{
    TuningICAInput      = 0,    ///< Input
    TuningICAReference,         ///< Reference
    TuningICAMaxPaths,          ///< Total number of ICE paths
};

/// @brief Structure to encapsulate ICA LUTs data
struct IPEICA10LUT
{
    UINT32 PerspectiveLUT[IPEICA10LUTEntriesPerspective];
    UINT32 grid0LUT[IPEICA10LUTEntriesGrid0];
    UINT32 grid1LUT[IPEICA10LUTEntriesGrid1];
} CAMX_PACKED;

struct IPEICA30LUT
{
    UINT32 PerspectiveLUT[IPEICA30LUTEntiresPerspective];
    UINT64 grid0LUT[IPEICA30LUTEntiresGrid0];
    UINT64 grid1LUT[IPEICA30LUTEntiresGrid1];
} CAMX_PACKED;

/// @brief Structure to encapsulate LTM13 LUTs data
struct IPELTM13LUT
{
    UINT32 WeightLUT[IPELTM13LUTEntriesWeight];
    UINT32 LA0LUT[IPELTM13LUTEntriesLA0];
    UINT32 LA1LUT[IPELTM13LUTEntriesLA1];
    UINT32 CurveLUT[IPELTM13LUTEntriesCurve];
    UINT32 ScaleLUT[IPELTM13LUTEntriesScale];
    UINT32 MaskLUT[IPELTM13LUTEntriesMask];
    UINT32 LCEPositiveLUT[IPELTM13LUTEntriesLCEPositive];
    UINT32 LCENegativeLUT[IPELTM13LUTEntriesLCENegative];
    UINT32 RGamma0LUT[IPELTM13LUTEntriesRGamma0];
    UINT32 RGamma1LUT[IPELTM13LUTEntriesRGamma1];
    UINT32 RGamma2LUT[IPELTM13LUTEntriesRGamma2];
    UINT32 RGamma3LUT[IPELTM13LUTEntriesRGamma3];
    UINT32 RGamma4LUT[IPELTM13LUTEntriesRGamma4];
    UINT32 AverageLUT[IPELTM13LUTEntriesAverage];
} CAMX_PACKED;

/// @brief Structure to encapsulate LTM14 LUTs data
struct IPELTM14LUT
{
    UINT32 WeightLUT[IPELTM14LUTEntriesWeight];
    UINT32 LALUT[IPELTM14LUTEntriesLA];
    UINT32 CurveLUT[IPELTM14LUTEntriesCurve];
    UINT32 ScaleLUT[IPELTM14LUTEntriesScale];
    UINT32 MaskLUT[IPELTM14LUTEntriesMask];
    UINT32 LCEPositiveLUT[IPELTM14LUTEntriesLCEPositive];
    UINT32 LCENegativeLUT[IPELTM14LUTEntriesLCENegative];
    UINT32 RGammaLUT[IPELTM14LUTEntriesRGamma];
} CAMX_PACKED;

/// @brief Structure to encapsulate 2DLUT LUTs
struct IPE2DLUT10LUT
{
    UINT32 Hue0LUT[IPE2DLUT10EntriesHue0];
    UINT32 Hue1LUT[IPE2DLUT10EntriesHue1];
    UINT32 Hue2LUT[IPE2DLUT10EntriesHue2];
    UINT32 Hue3LUT[IPE2DLUT10EntriesHue3];
    UINT32 Saturation0LUT[IPE2DLUT10EntriesSaturation0];
    UINT32 Saturation1LUT[IPE2DLUT10EntriesSaturation1];
    UINT32 Saturation2LUT[IPE2DLUT10EntriesSaturation2];
    UINT32 Saturation3LUT[IPE2DLUT10EntriesSaturation3];
    UINT32 InverseHueLUT[IPE2DLUT10EntriesInverseHue];
    UINT32 InverseSaturationLUT[IPE2DLUT10EntriesInverseSaturation];
    UINT32 Hue1DLUT[IPE2DLUT10Entries1DHue];
    UINT32 Saturation1DLUT[IPE2DLUT10Entries1DSaturation];
} CAMX_PACKED;

/// @brief Structure to encapsulate ASF LUTs
struct IPEASF30LUT
{
    UINT32 Layer1ActivityNormalGainPosGainNegSoftThLUT[IPEASF30EntriesLayer1ActivityNormalGainPosGainNegSoftThLUT];
    UINT32 Layer1GainWeightLUT[IPEASF30EntriesLayer1GainWeightLUT];
    UINT32 Layer1SoftThresholdWeightLUT[IPEASF30EntriesLayer1SoftThresholdWeightLUT];
    UINT32 Layer2ActivityNormalGainPosGainNegSoftThLUT[IPEASF30EntriesLayer2ActivityNormalGainPosGainNegSoftThLUT];
    UINT32 Layer2GainWeightLUT[IPEASF30EntriesLayer2GainWeightLUT];
    UINT32 Layer2SoftThresholdWeightLUT[IPEASF30EntriesLayer2SoftThresholdWeightLUT];
    UINT32 ChromaGradientPosNegLUT[IPEASF30EntriesChromaGradientPosNegLUT];
    UINT32 ContrastPosNegLUT[IPEASF30EntriesContrastPosNegLUT];
    UINT32 SkinActivityGainLUT[IPEASF30EntriesSkinActivityGainLUT];
} CAMX_PACKED;

/// @brief Structure to encapsulate Upscaler 20 LUTs
struct IPEUpscaler20LUT
{
    UINT32 ALUT[IPEUpscaler20EntriesALUT];
    UINT32 BLUT[IPEUpscaler20EntriesBLUT];
    UINT32 CLUT[IPEUpscaler20EntriesCLUT];
    UINT32 DLUT[IPEUpscaler20EntriesDLUT];
} CAMX_PACKED;

/// @brief: CAMX node information, useful for multi-pass usecase
struct IQNodeInformation
{
    UINT32  instanceId;
    UINT64  requestId;
    UINT32  isRealTime;
    UINT32  profileId;
    UINT32  processingType;
} CAMX_PACKED;

/// @brief Structure to encapsulate Grain Adder 10 (GRA)
struct IPEGrainAdder10LUT
{
    UINT32 Channel1LUT[IPEGrainAdder10EntriesLUT];
    UINT32 Channel2LUT[IPEGrainAdder10EntriesLUT];
    UINT32 Channel3LUT[IPEGrainAdder10EntriesLUT];
} CAMX_PACKED;

/// @brief: CAMX Tuning mode data
struct TuningModeDebugData
{
    UINT32  base;       ///< Default tuning data
    UINT32  sensor;     ///< Sensor mode
    UINT32  usecase;    ///< Usecase mode
    UINT32  feature1;   ///< Feature 1 mode
    UINT32  feature2;   ///< Feature 2 mode
    UINT32  scene;      ///< Scene mode
    UINT32  effect;     ///< Effect mode
} CAMX_PACKED;

/// @brief Structure to encapsulate IFE DMI LUT packed data for titan17x
struct IFEDMILUTData17x
{
    GammaCurve              gamma[GammaMaxChannel]; ///< Gamma LUT curve
    GTMLUT                  GTM;                    ///< GTM LUT
    IFEABF34LUT             ABFLUT;                 ///< ABF noise, activity & dark LUTs
    IFEPDPC11LUT            PDPCLUT;                ///< BPC & PDPC LUT
    IFELinearization33LUT   linearizationLUT;       ///< Linarization IFE LUT
    LSC34MeshLUT            LSCMesh;                ///< LSC mesh LUT
    PedestalLUT             pedestalLUT;            ///< Pedestal LUT
} CAMX_PACKED;

/// @brief Structure to encapsulate IFE DMI LUT packed data for titan480
struct IFEDMILUTData480
{
    GammaCurve              gamma[GammaMaxChannel]; ///< Gamma LUT curve
    GTMLUT                  GTM;                    ///< GTM LUT
    IFEABF40LUT             ABFLUT;                 ///< ABF noise, activity & dark LUTs
    IFEPDPC30LUT            PDPCLUT;                ///< BPC & PDPC LUT
    IFELinearization34LUT   linearizationLUT;       ///< Linarization IFE LUT
    LSC40MeshLUT            LSCMesh;                ///< LSC mesh LUT
    PedestalLUT             pedestalLUT;            ///< Pedestal LUT
} CAMX_PACKED;

/// @brief Structure to encapsulate BPS DMI data for titan17x
struct BPSDMILUTData17x
{
    GTMLUT              GTM;                    ///< GTM LUT
    LSC34MeshLUT        LSCMesh;                ///< LSC34 mesh LUT
    PedestalLUT         pedestalLUT;            ///< Pedestal LUT
    BPSLinearizationLUT linearizationLUT;       ///< Linarization BPS LUT
    BPSPDPC20LUT        PDPCLUT;                ///< BPC & PDPC LUT
    BPSGICLUT           GICLUT;                 ///< GIC noise LUT
    BPSABFLUT           ABFLUT;                 ///< ABF noise, activity & dark LUTs
    GammaCurve          gamma[GammaMaxChannel]; ///< Gamma LUT curve
    HNR10LUT            HNRLUT;                 ///< HNR LUTs
} CAMX_PACKED;

/// @brief Structure to encapsulate BPS DMI data for titan480
struct BPSDMILUTData480
{
    GTMLUT              GTM;                    ///< GTM LUT
    LSC40MeshLUT        LSCMesh;                ///< LSC 40 mesh LUT
    PedestalLUT         pedestalLUT;            ///< Pedestal LUT
    BPSLinearizationLUT linearizationLUT;       ///< Linarization BPS LUT
    BPSPDPC30LUT        PDPCLUT;                ///< BPS PDPC LUTs
    BPSGICLUT           GICLUT;                 ///< GIC noise LUT
    BPSABFLUT           ABFLUT;                 ///< ABF noise, activity & dark LUTs
    GammaCurve          gamma[GammaMaxChannel]; ///< Gamma LUT curve
} CAMX_PACKED;

/// @brief Structure to encapsulate IPE DMI data for titan17x
struct IPEDMILUTData17x
{
    IPEGamma15Curve     gamma[GammaMaxChannel];     ///< Gamma 15 LUT curve
    IPEICA10LUT         ICALUT[TuningICAMaxPaths];  ///< ICA 10 LUTs for input & reference
    IPELTM13LUT         LTMLUT;                     ///< LTM 13 LUTs
    IPE2DLUT10LUT       LUT2D;                      ///< 2D LUT 10 LUTs
    IPEASF30LUT         ASFLUT;                     ///< Adaptive Spatial Filter 30 LUTs
    IPEUpscaler20LUT    upscalerLUT;                ///< Upscaler 20 LUTs
    IPEGrainAdder10LUT  grainAdderLUT;              ///< Grain Adder 10 LUTs
} CAMX_PACKED;

/// @brief Structure to encapsulate IPE DMI data for titan480
struct IPEDMILUTData480
{
    IPEGamma15Curve     gamma[GammaMaxChannel];     ///< Gamma 15 LUT curve
    IPEICA30LUT         ICALUT[TuningICAMaxPaths];  ///< ICA 30 LUTs for input & reference
    IPELTM14LUT         LTMLUT;                     ///< LTM 14 LUTs
    IPE2DLUT10LUT       LUT2D;                      ///< 2D LUT 10 LUTs
    IPEASF30LUT         ASFLUT;                     ///< Adaptive Spatial Filter 30 LUTs
    IPEUpscaler20LUT    upscalerLUT;                ///< Upscaler 20 LUTs
    IPEGrainAdder10LUT  grainAdderLUT;              ///< Grain Adder 10 LUTs
    HNR10LUT            HNRLUT;                     ///< HNR 10 LUTs
};

/// @brief IFE ABF Tuning Metadata
struct IFEABFTuningMetadata17x
{
    UINT32 ABFConfig1[IFEABFRegisters1Titan17x];
    UINT32 ABFConfig2[IFEABFRegisters2Titan17x];
} CAMX_PACKED;

/// @brief IFE ABF Tuning Metadata
struct IFEABFTuningMetadata480
{
    UINT32 ABFConfig1[IFEABFRegisters1Titan480];
    UINT32 ABFConfig2[IFEABFRegisters2Titan480];
} CAMX_PACKED;

/// @brief IFE BLS Tuning Metadata
struct IFEBLSTuningMetadata17x
{
    UINT32 BLSConfig[IFEBLSRegistersTitan17x];
} CAMX_PACKED;

/// @brief IFE BLS Tuning Metadata
struct IFEBLSTuningMetadata480
{
    UINT32 BLSConfig1[IFEBLSRegisters1Titan480];
    UINT32 BLSConfig2[IFEBLSRegisters2Titan480];
} CAMX_PACKED;

/// @brief IFE BPCBCC Tuning Metadata
struct IFEBPCBCCMetadata
{
    UINT32 BPCBCCConfig[IFEBPCBCCRegister];
} CAMX_PACKED;

/// @brief IFE CC Tuning Metadata
struct IFECCMetadata17x
{
    UINT32 CCConfig1[IFECCRegisters1Titan17x];
} CAMX_PACKED;

/// @brief IFE CC Tuning Metadata
struct IFECCMetadata480
{
    UINT32 CCConfig1[IFECCRegisters1Titan480];
    UINT32 CCConfig2[IFECCRegisters2Titan480];
} CAMX_PACKED;

/// @brief IFE CST Tuning Metadata
struct IFECSTMetadata17x
{
    UINT32 CSTConfig[IFECSTRegistersTitan17x];
} CAMX_PACKED;


/// @brief IFE CST Tuning Metadata
struct IFECSTMetadata480
{
    UINT32 CSTConfig[IFECSTRegistersTitan480];
} CAMX_PACKED;

/// @brief IFE Demosaic Tuning Metadata
struct IFEDemosaicMetadata
{
    UINT32 demosaicConfig[IFEDemosaicRegisters];
} CAMX_PACKED;

/// @brief IFE Demux Tuning Metadata
struct IFEDemuxMetadata
{
    UINT32 demuxConfig[IFEDemuxRegisters];
} CAMX_PACKED;

/// @brief IFE Gamma Tuning Metadata
struct IFEGammaMetadata17x
{
    UINT32 gammaConfig[IFEGammaRegistersTitan17x];
} CAMX_PACKED;

/// @brief IFE Gamma Tuning Metadata
struct IFEGammaMetadata480
{
    UINT32 gammaConfig[IFEGammaRegistersTitan480];
} CAMX_PACKED;

/// @brief IFE GTM Tuning Metadata
struct IFEGTMMetadata17x
{
    UINT32 GTMConfig[IFEGTMRegistersTitan17x];
} CAMX_PACKED;

/// @brief IFE GTM Tuning Metadata
struct IFEGTMMetadata480
{
    UINT32 GTMConfig[IFEGTMRegistersTitan480];
} CAMX_PACKED;

/// @brief IFE HDR 17x Tuning Metadata
struct IFEHDRMetadata17x
{
    UINT32 HDRConfig1[IFEHDRRegisters1Titan17x];
    UINT32 HDRConfig2[IFEHDRRegisters2Titan17x];
    UINT32 HDRConfig3[IFEHDRRegisters3Titan17x];
} CAMX_PACKED;

/// @brief IFE HDR 480 Tuning Metadata
struct IFEHDRMetadata480
{
    UINT32 HDRConfig1[IFEHDRRegisters1Titan480];
    UINT32 HDRConfig2[IFEHDRRegisters2Titan480];
} CAMX_PACKED;

/// @brief IFE Linearization Tuning Metadata
struct IFESLinearizationMetadata17x
{
    UINT32 linearizationConfig1[IFELinearizationRegistersTitan17x];
} CAMX_PACKED;


/// @brief IFE Linearization Tuning Metadata
struct IFESLinearizationMetadata480
{
    UINT32 linearizationConfig1[IFELinearizationRegisters1Titan480];
    UINT32 linearizationConfig2[IFELinearizationRegisters2Titan480];
} CAMX_PACKED;

/// @brief IFE LSC Tuning Metadata
struct IFELSCMetadata17x
{
    UINT32 LSCConfig1[IFELSCRegistersTitan17x];
} CAMX_PACKED;

/// @brief IFE LSC Tuning Metadata
struct IFELSCMetadata480
{
    UINT32 LSCConfig1[IFELSCRegisters1Titan480];
    UINT32 LSCConfig2[IFELSCRegisters2Titan480];
} CAMX_PACKED;

/// @brief IFE PDPC Tuning Metadata
struct IFEPDPCMetadata17x
{
   UINT32 PDPCConfig[IFEPDPCRegistersTitan17x];
} CAMX_PACKED;

/// @brief IFE PDPC Tuning Metadata
struct IFEPDPCMetadata480
{
    UINT32 PDPCConfig[IFEPDPCRegistersTitan480];
} CAMX_PACKED;

/// @brief IFE Pedestal Tuning Metadata
struct IFEPedestalMetadata17x
{
    UINT32 pedestalConfig[IFEPedestalRegistersTitan17x];
} CAMX_PACKED;

/// @brief IFE Pedestal Tuning Metadata
struct IFEPedestalMetadata480
{
    UINT32 pedestalConfig1[IFEPedestalRegisters1Titan480];
    UINT32 pedestalConfig2[IFEPedestalRegisters2Titan480];
} CAMX_PACKED;

/// @brief IFE WB Tuning Metadata
struct IFEWBMetadata17x
{
    UINT32 WBconfig[IFEWBRegistersTitan17x];
} CAMX_PACKED;

/// @brief IFE WB Tuning Metadata
struct IFEWBMetadata480
{
    UINT32 WBconfig[IFEWBRegistersTitan480];
} CAMX_PACKED;

/// @brief Pedestal Tuning Metadata
struct BPSPedestalTuningMetadata
{
    UINT32  pedestalConfig[PedestalConfigRegisters];
} CAMX_PACKED;

/// @brief Linearization kneepoint configuration
struct BPSLinearizationTuningMetadata
{
    UINT32 linearizationConfig[LinearizationConfigKneepoints];
} CAMX_PACKED;

/// @brief BPC & PDPC 20 register configuration
struct BPSBPCPDPC20TuningMetadata
{
    UINT32  BPCPDPConfig0[BPCPDPC20ConfigRegister0];
    UINT32  BPCPDPConfig1[BPCPDPC20ConfigRegister1];
} CAMX_PACKED;

/// @brief BPC & PDPC 30 register configuration
struct BPSBPCPDPC30TuningMetadata
{
    UINT32  BPCPDPConfig0[BPCPDPC30ConfigRegister0];
    UINT32  BPCPDPConfig1[BPCPDPC30ConfigRegister1];
} CAMX_PACKED;

/// @brief HDR 22 register configuration
struct BPSHDR22TuningMetadata
{
    UINT32  HDRReconConfig[BPSHDR22ReconRegisters];
    UINT32  HDRMACConfig[BPSHDR22MACRegisters];
} CAMX_PACKED;

/// @brief HDR 30 register configuration
struct BPSHDR30TuningMetadata
{
    UINT32  HDRConfig[BPSHDR30Registers];
} CAMX_PACKED;

/// @brief GIC register configuration
struct BPSGICTuningMetadata
{
    UINT32  filterConfig[BPSGICRegisters];
} CAMX_PACKED;

/// @brief ABF register configuration
struct BPSABFTuningMetadata
{
    UINT32  config[BPSABFRegisters];
} CAMX_PACKED;

/// @brief LSC 34 registers configuration
struct BPSLSC34TuningMetadata
{
    UINT32  rolloffConfig[LSC34ConfigRegisters];
} CAMX_PACKED;

/// @brief LSC 40 registers configuration
struct BPSLSC40TuningMetadata
{
    UINT32  rolloffConfig[LSC40ConfigRegisters];
} CAMX_PACKED;

/// @brief WB registers configuration
struct BPSWBTuningMetadata
{
    UINT32  WBConfig[BPSWBConfigRegisters];
} CAMX_PACKED;

/// @brief Demosaic registers configuration
struct BPSDemosaicTuningMetadata
{
    UINT32  demosaicConfig[BPSDemosaicConfigRegisters];
} CAMX_PACKED;

/// @brief Demux registers configuration
struct BPSDemuxTuningMetadata
{
    UINT32  demuxConfig[BPSDemuxConfigRegisters];
} CAMX_PACKED;

/// @brief Color correction registers configuration
struct BPSCCTuningMetadata
{
    UINT32  colorCorrectionConfig[BPSCCConfigRegisters];
} CAMX_PACKED;

/// @brief Color space transform 12 registers configuration
struct CST12TuningMetadata
{
    UINT32  CSTConfig[CST12ConfigRegisters];
} CAMX_PACKED;

/// @brief Tuning Face Data configuration
struct TuningFaceData
{
    INT32 numberOfFace;                     ///< Number of faces
    INT32 faceCenterX[TuningMaxFaceNumber]; ///< Face cordinates X
    INT32 faceCenterY[TuningMaxFaceNumber]; ///< Face coordinates Y
    INT32 faceRadius[TuningMaxFaceNumber];  ///< Face Radius
} CAMX_PACKED;

/// @brief Hybrid Noise Reduction 10 (HNR) registers configuration
struct HNR10TuningMetadata
{
    UINT32  HNRConfig[HNR10ConfigRegisters];
} CAMX_PACKED;

/// @brief BPS settings that FW configure
struct BPSFirmwareHeader
{
    UINT32 BPSFWAPIVersion;
    UINT32 BPSFirmwareSetting[BPSFirmwareSettingSize];
} CAMX_PACKED;

/// @brief IPE pass names
enum TuningIPEPassName
{
    TuningIPEPassFull   = 0,                ///< FULL
    TuningIPEPassDC4    = 1,                ///< ANR/TF 1:4
    TuningIPEPassDC16   = 2,                ///< ANR/TF 1:16
    TuningIPEPassDC64   = 3,                ///< ANR/TF 1:64

    TuningIPEPassMax,

    TuningIPEPassDCBase = TuningIPEPassDC4,
};

/// @brief IPE all modules config data from registers that FW configure
struct IPEFirmwareHeader
{
    UINT32 IPEFWAPIVersion;
    UINT32 IPEFirmwareSetting[IPEFirmwareSettingSize];
} CAMX_PACKED;

/// @brief IPE ANR 10 register DCBlend1 reg cmds
struct IPEANR10TuningDCBlend1RegCmd
{
    UINT32 DCBlend1[IPEANR10DCBlend1ConfigRegisters];
} CAMX_PACKED;

/// @brief IPE ANR 10 register RNF reg cmds
struct IPEANR10TuningRNFRegCmd
{
    UINT32 RNF[IPEANR10RNFConfigRegisters];
} CAMX_PACKED;

/// @brief IPE ANR 10 register DCUS reg cmds
struct IPEANR10TuningDCUSRegCmd
{
    UINT32 DCUS[IPEANR10DCUSConfigRegisters];
} CAMX_PACKED;
/// @brief IPE ANR 10 register Filter2 cmds
struct IPEANR10TuningCFilter2RegCmd
{
    UINT32 CFilter2[IPEANR10CFilter2ConfigRegisters];
} CAMX_PACKED;
/// @brief IPE ANR 10 register DCBlend2 reg cmds
struct IPEANR10TuningDCBlend2RegCmd
{
    UINT32 DCBlend2[IPEANR10DCBlend2ConfigRegisters];
} CAMX_PACKED;

/// @brief IPE ANR 10 register CYLPF pre Lens Gain reg cmds
struct IPEANR10TuningCYLPFPreLensGainRegCmd
{
    UINT32 CYLPFPreLensGain[IPEANR10CYLPFPreLensGainConfigRegisters];
} CAMX_PACKED;

/// @brief IPE ANR 10 register CYLPF Lens Gain reg cmds
struct IPEANR10TuningCYLPFLensGainRegCmd
{
    UINT32 CYLPFLensGain[IPEANR10CYLPFLensGainConfigRegisters];
} CAMX_PACKED;

/// @brief IPE ANR 10 register CYLPF post Lens Gain cmds
struct IPEANR10TuningCYLPFPostLensGainRegCmd
{
    UINT32 CYLPFPostLensGain[IPEANR10CYLPFPostLensGainConfigRegisters];
} CAMX_PACKED;
/// @brief IPE ANR 10 register CNR reg cmds
struct IPEANR10TuningCNRRegCmd
{
    UINT32 CNR[IPEANR10CNRConfigRegisters];
} CAMX_PACKED;

/// @brief LENR 10 registers configuration
struct IPELENR10TuningMetadata
{
    UINT32  LENRConfig[LENR10ConfigRegisters];
} CAMX_PACKED;

/// @brief IPE ANR 10 registers configuration per pass
struct IPEANR10TuningPassData
{
    IPEANR10TuningDCBlend1RegCmd          dcBlend1Cmd;            ///< ANR 10 DCBlend1 reg cmds
    IPEANR10TuningRNFRegCmd               rnfCmd;                 ///< ANR 10 RNF reg cmds
    IPEANR10TuningDCUSRegCmd              dcusCmd;                ///< ANR 10 DCUS reg cmds
    IPEANR10TuningCFilter2RegCmd          filter2Cmd;             ///< ANR 10 Filter2 cmds
    IPEANR10TuningDCBlend2RegCmd          dcBlend2Cmd;            ///< ANR 10 DCBlend2 reg cmds
    IPEANR10TuningCYLPFPreLensGainRegCmd  cylpfPreLensGainCmd;    ///< ANR 10 CYLPF pre Lens Gain reg cmds
    IPEANR10TuningCYLPFLensGainRegCmd     cylpfLensGainCmd;       ///< ANR 10 CYLPF Lens Gain reg cmds
    IPEANR10TuningCYLPFPostLensGainRegCmd cylpfPostLensGainCmd;   ///< ANR 10 CYLPF post Lens Gain cmds
    IPEANR10TuningCNRRegCmd               cnrCmd;                 ///< ANR 10 CNR reg cmds
} CAMX_PACKED;

/// @brief IPE ANR 10 all registers configuration
struct IPEANR10TuningMetadata
{
    IPEANR10TuningPassData    ANRData[TuningIPEPassMax];
} CAMX_PACKED;

/// @brief IPE TF 10 registers configuration per pass
struct IPETF10TuningPassData
{
    UINT32  TFRegister[IPETF10ConfigRegisters];
} CAMX_PACKED;

/// @brief IPE TF 20 registers configuration per pass
struct IPETF20TuningPassData
{
    UINT32  TFRegister[IPETF20ConfigRegisters];
} CAMX_PACKED;


/// @brief IPE TF 10 all registers configuration
struct IPETF10TuningMetadata
{
    IPETF10TuningPassData    TFData[TuningIPEPassMax];
} CAMX_PACKED;

/// @brief IPE TF 20 all registers configuration
struct IPETF20TuningMetadata
{
    IPETF20TuningPassData    TFData[TuningIPEPassMax];
} CAMX_PACKED;

/// @brief IPE Upscaler 20 all registers configuration
struct IPEUpscaler20TuningMetadata
{
    UINT32 UpscalerConfig[IPEUpscaler20Registers];
} CAMX_PACKED;

/// @brief IPE CAC 2x all registers configuration
struct IPECAC2xTuningMetadata
{
    UINT32  CACConfig[IPECAC2xConfigRegisters];
} CAMX_PACKED;

/// @brief IPE LTM exposure index data, previous and current (13/14)
struct IPELTMTuningExpData
{
    FLOAT  exposureIndex[IPELTMExposureIndexCount];
} CAMX_PACKED;

/// @brief IPE Color correction 13 registers configuration
struct IPECC13TuningMetadata
{
    UINT32  colorCorrectionConfig[IPECC13ConfigRegisters];
} CAMX_PACKED;

/// @brief IPE 2D LUT 10 registers configuration
struct IPE2DLUT10TuningMetadata
{
    UINT32  LUT2DConfig[IPE2DLUT10ConfigRegisters];
} CAMX_PACKED;

/// @brief IPE Advance Chroma Enhancement 12 registers configuration
struct IPEChromaEnhancement12Metadata
{
    UINT32  CEConfig[IPEChromaEnhancement12ConfigRegisters];
} CAMX_PACKED;

/// @brief IPE Chroma Suppression 20 registers configuration
struct IPEChromaSuppression20Metadata
{
    UINT32  CSConfig[IPEChromaSuppression20ConfigRegisters];
} CAMX_PACKED;

/// @brief IPE Skin Color Enhancement 11 registers configuration
struct IPESCE11Metadata
{
    UINT32  SCEConfig[IPESCE11ConfigRegisters];
} CAMX_PACKED;

/// @brief IPE Adaptive Spatial Filter (ASF) 30 registers configuration
struct IPEASF30Metadata
{
    UINT32  ASFConfig[IPEASF30ConfigRegisters];
} CAMX_PACKED;

/// @brief IFE Structure to hold Crop, Round and Clamp module enable registers
struct IFETuningModuleEnableConfig
{
    UINT32  lensProcessingModuleConfig;             ///< Lens processing Module enable
    UINT32  colorProcessingModuleConfig;            ///< Color processing Module enable
    UINT32  frameProcessingModuleConfig;            ///< Frame processing Module enable
    UINT32  FDLumaCropRoundClampConfig;             ///< FD Luma path Module enable
    UINT32  FDChromaCropRoundClampConfig;           ///< FD Chroma path Module enable
    UINT32  fullLumaCropRoundClampConfig;           ///< Full Luma path Module enable
    UINT32  fullChromaCropRoundClampConfig;         ///< Full Chroma path Module enable
    UINT32  DS4LumaCropRoundClampConfig;            ///< DS4 Luma path Module enable
    UINT32  DS4ChromaCropRoundClampConfig;          ///< DS4 Chroma path Module enable
    UINT32  DS16LumaCropRoundClampConfig;           ///< DS16 Luma path Module enable
    UINT32  DS16ChromaCropRoundClampConfig;         ///< DS16 Chroma path Module enable
    UINT32  statsEnable;                            ///< Stats Module Enable
    UINT32  statsConfig;                            ///< Stats config
    UINT32  dspConfig;                              ///< Dsp Config
    UINT32  frameProcessingDisplayModuleConfig;     ///< Frame processing Disp module enable
    UINT32  displayFullLumaCropRoundClampConfig;    ///< Full Disp Luma path Module enable
    UINT32  displayFullChromaCropRoundClampConfig;  ///< Full Disp Chroma path Module enable
    UINT32  displayDS4LumaCropRoundClampConfig;     ///< DS4 Disp Luma path Module enable
    UINT32  displayDS4ChromaCropRoundClampConfig;   ///< DS4 Disp Chroma path Module enable
    UINT32  displayDS16LumaCropRoundClampConfig;    ///< DS16 Disp Luma path Module enable
    UINT32  displayDS16ChromaCropRoundClampConfig;  ///< DS16 Disp Chroma path Module enable
} CAMX_PACKED;

/// @brief Defines the exposure parameters to control the frame exposure setting to sensor and ISP
struct TuningAECExposureData
{
    UINT64  exposureTime;         ///< The exposure time in nanoseconds used by sensor
    FLOAT   linearGain;           ///< The total gain consumed by sensor only
    FLOAT   sensitivity;          ///< The total exposure including exposure time and gain
    FLOAT   deltaEVFromTarget;    ///< The current exposure delta compared with the desired target
} CAMX_PACKED;

/// @brief Structure to encapsulate AEC Frame Control
struct TuningAECFrameControl
{
    TuningAECExposureData   exposureInfo[ExposureIndexCount];    ///< Exposure info including gain and exposure time
    FLOAT                   luxIndex;                            ///< Future frame lux index,  consumed by ISP
    UINT8                   flashInfo;                           ///< Flash information if it is main or preflash
    UINT8                   preFlashState;                       ///< Preflash state from AEC, consumed by Sensor
    UINT32                  LEDCurrents[LEDSettingCount];        ///< The LED currents value for the use case of LED snapshot
} CAMX_PACKED;

/// @brief Structure to encapsulate AWB Frame Control
struct TuningAWBFrameControl
{
    FLOAT   AWBGains[RGBChannelCount];          ///< R/G/B gains
    UINT32  colorTemperature;                   ///< Color temperature
    UINT8   isCCMOverrideEnabled;               ///< Flag indicates if CCM override is enabled.
    FLOAT   CCMOffset[AWBNumCCMRows];           ///< The offsets for color correction matrix
    FLOAT   CCM[AWBNumCCMRows][AWBNumCCMCols]; ///< The color correction matrix
} CAMX_PACKED;

/// @brief Structure to encapsulate Sensor data
struct SensorTuningData
{
    PixelFormat         format;            ///< Pixel format
    UINT                numPixelsPerLine;  ///< Number of pixels per line (lineLengthPclk)
    UINT                numLinesPerFrame;  ///< Number of lines per frame (frameLengthLines)
    UINT                maxLineCount;      ///< Max line count
    SensorResolution    resolution;        ///< Sensor output resolution for this mode
    SensorCropInfo      cropInfo;          ///< Crop info for this modes
    UINT                binningTypeH;      ///< Binning horizontal
    UINT                binningTypeV;      ///< Binning vertical
} CAMX_PACKED;

/// @brief Structure encapsulating Trigger data
struct IFETuningTriggerData
{
    FLOAT       AECexposureGainRatio;       ///< Input trigger value:  AEC exposure/gain ratio
    FLOAT       AECexposureTime;            ///< Input trigger value:  AEC exposure time
    FLOAT       AECSensitivity;             ///< Input trigger value:  AEC sensitivity data
    FLOAT       AECGain;                    ///< Input trigger value:  AEC Gain Value
    FLOAT       AECLuxIndex;                ///< Input trigger value:  Lux index
    FLOAT       AWBleftGGainWB;             ///< Input trigger value:  AWB G channel gain
    FLOAT       AWBleftBGainWB;             ///< Input trigger value:  AWB B channel gain
    FLOAT       AWBleftRGainWB;             ///< Input trigger value:  AWB R channel gain
    FLOAT       AWBColorTemperature;        ///< Input Trigger value:  AWB CCT value
    FLOAT       DRCGain;                    ///< Input trigger value:  DRCGain
    FLOAT       DRCGainDark;                ///< Input trigger value:  DRCGainDark
    FLOAT       lensPosition;               ///< Input trigger value:  Lens position
    FLOAT       lensZoom;                   ///< Input trigger value:  Lens Zoom
    FLOAT       postScaleRatio;             ///< Input trigger value:  Post scale ratio
    FLOAT       preScaleRatio;              ///< Input trigger value:  Pre scale ratio
    UINT32      sensorImageWidth;           ///< Current Sensor Image Output Width
    UINT32      sensorImageHeight;          ///< Current Sensor Image Output Height
    UINT32      CAMIFWidth;                 ///< Width of CAMIF Output
    UINT32      CAMIFHeight;                ///< Height of CAMIF Output
    UINT16      numberOfLED;                ///< Number of LED
    INT32       LEDSensitivity;             ///< LED Sensitivity
    UINT32      bayerPattern;               ///< Bayer pattern
    UINT32      sensorOffsetX;              ///< Current Sensor Image Output horizontal offset
    UINT32      sensorOffsetY;              ///< Current Sensor Image Output vertical offset
    UINT32      blackLevelOffset;           ///< Black level offset
} CAMX_PACKED;

/// @brief Structure encapsulating Sensor configuration data
struct IFEBPSSensorConfigData
{
    INT32                           isBayer;                    ///< Flag to indicate Bayer sensor
    INT32                           format;                     ///< Image Format
    FLOAT                           digitalGain;                ///< Digital Gain Value
    UINT32                          ZZHDRColorPattern;          ///< ZZHDR Color Pattern Information
    ZZHDRFirstExposurePattern       ZZHDRFirstExposure;  ///< ZZHDR First Exposure Pattern
} CAMX_PACKED;

/// @brief Structure to encapsulate custom OEM data from IQ modules
struct IQOEMTuningMetadata
{
    FLOAT   IQOEMTuningData[IQOEMTuningData];
} CAMX_PACKED;

/// @brief Structure to encapsulate metadata from sensor, 3A and IFE
struct IFETuningMetadata17x
{
    // IQ Module's Metadata
    IFEABFTuningMetadata17x         IFEABFData;             ///< IFE ABF data
    IFEBLSTuningMetadata17x         IFEBLSData;             ///< IFE BLS data
    IFEBPCBCCMetadata               IFEBPCBCCData;          ///< IFE BPCBCC data
    IFECCMetadata17x                IFECCData;              ///< IFE CC data
    IFECSTMetadata17x               IFECSTData;             ///< IFE CST data
    IFEDemosaicMetadata             IFEDemosaicData;        ///< IFE Demosaic data
    IFEDemuxMetadata                IFEDemuxData;           ///< IFE Demux data
    IFEGammaMetadata17x             IFEGammaData;           ///< IFE Gamma data
    IFEGTMMetadata17x               IFEGTMData;             ///< IFE GTM data
    IFEHDRMetadata17x               IFEHDRData;             ///< IFE HDR data
    IFESLinearizationMetadata17x    IFELinearizationData;   ///< IFE Linearization data
    IFELSCMetadata17x               IFELSCData;             ///< IFE LSC data
    IFEPDPCMetadata17x              IFEPDPCData;            ///< IFE PDPC data
    IFEPedestalMetadata17x          IFEPedestalData;        ///< IFE Pedestal data
    IFEWBMetadata17x                IFEWBData;              ///< IFE WB data
    IFEDMILUTData17x                IFEDMIPacked;           ///< IFE DMI LUT packed data
} CAMX_PACKED;

/// @brief Structure to encapsulate metadata from sensor, 3A and IFE
struct IFETuningMetadata480
{
    // IQ Module's Metadata
    IFEABFTuningMetadata480        IFEABFData;             ///< IFE ABF data
    IFEBLSTuningMetadata480        IFEBLSData;             ///< IFE BLS data
    IFEBPCBCCMetadata              IFEBPCBCCData;          ///< IFE BPCBCC data
    IFECCMetadata480               IFECCData;              ///< IFE CC data
    IFECSTMetadata480              IFECSTData;             ///< IFE CST data
    IFEDemosaicMetadata            IFEDemosaicData;        ///< IFE Demosaic data
    IFEDemuxMetadata               IFEDemuxData;           ///< IFE Demux data
    IFEGammaMetadata480            IFEGammaData;           ///< IFE Gamma data
    IFEGTMMetadata480              IFEGTMData;             ///< IFE GTM data
    IFEHDRMetadata480              IFEHDRData;             ///< IFE HDR data
    IFESLinearizationMetadata480   IFELinearizationData;   ///< IFE Linearization data
    IFELSCMetadata480              IFELSCData;             ///< IFE LSC data
    IFEPDPCMetadata480             IFEPDPCData;            ///< IFE PDPC data
    IFEPedestalMetadata480         IFEPedestalData;        ///< IFE Pedestal data
    IFEWBMetadata480               IFEWBData;              ///< IFE WB data
    IFEDMILUTData480               IFEDMIPacked;           ///< IFE DMI LUT packed data
} CAMX_PACKED;

/// @brief Structure to encapsulate metadata from sensor, 3A and IFE
struct IFETuningMetadata
{
    // Configurations
    SensorTuningData            sensorData;             ///< Sensor tuning data information
    IFETuningModuleEnableConfig IFEEnableConfig;        ///< IFE Structure to hold Crop, Round and Clamp enable registers
    IFEBPSSensorConfigData      IFESensorConfig;        ///< Sensor configuration data
    IFETuningTriggerData        IFETuningTriggers;      ///< IFE input trigger data
    IQOEMTuningMetadata         oemTuningData;          ///< Structure to encapsulate custom OEM data from IQ modules

    union
    {
        IFETuningMetadata17x metadata17x;               ///< Tuning metadata
        IFETuningMetadata480 metadata480;               ///< Tuning metadata
    };
} CAMX_PACKED;

/// @brief Structure encapsulating Trigger data
struct BPSTuningTriggerData
{
    FLOAT       AECexposureTime;            ///< Input trigger value:  exposure time
    FLOAT       AECSensitivity;             ///< Input trigger value:  AEC sensitivity data
    FLOAT       AECGain;                    ///< Input trigger value:  AEC Gain Value
    FLOAT       AECLuxIndex;                ///< Input trigger value:  Lux index
    FLOAT       AWBleftGGainWB;             ///< Input trigger value:  AWB G channel gain
    FLOAT       AWBleftBGainWB;             ///< Input trigger value:  AWB B channel gain
    FLOAT       AWBleftRGainWB;             ///< Input trigger value:  AWB R channel gain
    FLOAT       AWBColorTemperature;        ///< Input Trigger value:  AWB CCT value
    FLOAT       DRCGain;                    ///< Input trigger value:  DRCGain
    FLOAT       DRCGainDark;                ///< Input trigger value:  DRCGainDark
    FLOAT       lensPosition;               ///< Input trigger value:  Lens position
    FLOAT       lensZoom;                   ///< Input trigger value:  Lens Zoom
    FLOAT       postScaleRatio;             ///< Input trigger value:  Post scale ratio
    FLOAT       preScaleRatio;              ///< Input trigger value:  Pre scale ratio
    UINT32      sensorImageWidth;           ///< Current Sensor Image Output Width
    UINT32      sensorImageHeight;          ///< Current Sensor Image Output Height
    UINT32      CAMIFWidth;                 ///< Width of CAMIF Output
    UINT32      CAMIFHeight;                ///< Height of CAMIF Output
    UINT16      numberOfLED;                ///< Number of LED
    INT32       LEDSensitivity;             ///< LED Sensitivity
    UINT32      bayerPattern;               ///< Bayer pattern
    UINT32      sensorOffsetX;              ///< Current Sensor Image Output horizontal offset
    UINT32      sensorOffsetY;              ///< Current Sensor Image Output vertical offset
    UINT32      blackLevelOffset;           ///< Black level offset
} CAMX_PACKED;

/// @brief Structure to encapsulate unique metadata from BPS titan17x
struct BPSTuningMetadata17x
{
    BPSPedestalTuningMetadata           BPSPedestalData;
    BPSLinearizationTuningMetadata      BPSLinearizationData;
    BPSGICTuningMetadata                BPSGICData;
    BPSABFTuningMetadata                BPSABFData;
    BPSWBTuningMetadata                 BPSWBData;
    BPSDemosaicTuningMetadata           BPSDemosaicData;
    BPSDemuxTuningMetadata              BPSDemuxData;
    BPSCCTuningMetadata                 BPSCCData;
    CST12TuningMetadata                 BPSCSTData;
    BPSBPCPDPC20TuningMetadata          BPSBBPCPDPCData;
    BPSHDR22TuningMetadata              BPSHDRData;
    BPSLSC34TuningMetadata              BPSLSCData;
    HNR10TuningMetadata                 BPSHNRData;
    TuningFaceData                      BPSHNRFaceDetection;
    BPSDMILUTData17x                    BPSDMIData;
} CAMX_PACKED;

/// @brief Structure to encapsulate unique metadata from BPS titan480
struct BPSTuningMetadata480
{
    BPSPedestalTuningMetadata           BPSPedestalData;
    BPSLinearizationTuningMetadata      BPSLinearizationData;
    BPSGICTuningMetadata                BPSGICData;
    BPSABFTuningMetadata                BPSABFData;
    BPSWBTuningMetadata                 BPSWBData;
    BPSDemosaicTuningMetadata           BPSDemosaicData;
    BPSDemuxTuningMetadata              BPSDemuxData;
    BPSCCTuningMetadata                 BPSCCData;
    CST12TuningMetadata                 BPSCSTData;
    BPSBPCPDPC30TuningMetadata          BPSBBPCPDPCData;
    BPSHDR30TuningMetadata              BPSHDRData;
    BPSLSC40TuningMetadata              BPSLSCData;
    BPSDMILUTData480                    BPSDMIData;
} CAMX_PACKED;

/// @brief Structure to encapsulate BPS metadata
struct BPSTuningMetadata
{
    UINT32                          BPSChipsetVersion;
    BPSFirmwareHeader               BPSFWHeader;
    BPSTuningTriggerData            BPSTuningTriggers;
    IFEBPSSensorConfigData          BPSSensorConfig;
    FLOAT                           BPSExposureGainRatio;
    IQNodeInformation               BPSNodeInformation;
    IQOEMTuningMetadata             oemTuningData;

    union
    {
        BPSTuningMetadata17x       BPSTuningMetadata17x;
        BPSTuningMetadata480       BPSTuningMetadata480;
    };
} CAMX_PACKED;

/// @brief Structure encapsulating Trigger data
struct IPETuningTriggerData
{
    FLOAT       predictiveGain;                        ///< digital gain from predictive convergence algorithm
    FLOAT       AECYHistStretchClampOffset;            ///< Dark Clamp Value for Hist Stretch
    FLOAT       AECYHistStretchScaleFactor;            ///< Scaling Value for Hist Stretch
} CAMX_PACKED;

struct IPETuningMetadata17x
{
    IPEDMILUTData17x                IPEDMIData;
    IPE2DLUT10TuningMetadata        IPE2DLUTData;
    IPEANR10TuningMetadata          IPEANRData;
    TuningFaceData                  IPEANRFaceDetection;
    IPEASF30Metadata                IPEASFData;
    TuningFaceData                  IPEASFFaceDetection;
    IPECAC2xTuningMetadata          IPECACData;
    IPEChromaEnhancement12Metadata  IPEChromaEnhancementData;
    IPEChromaSuppression20Metadata  IPEChromaSuppressionData;
    IPECC13TuningMetadata           IPECCData;
    CST12TuningMetadata             IPECSTData;
    IPELTMTuningExpData             IPELTMExposureData;
    IPESCE11Metadata                IPESCEData;
    IPETF10TuningMetadata           IPETFData;
} CAMX_PACKED;

struct IPETuningMetadata480
{
    IPEDMILUTData480                IPEDMIData;
    IPE2DLUT10TuningMetadata        IPE2DLUTData;
    IPEANR10TuningMetadata          IPEANRData;
    TuningFaceData                  IPEANRFaceDetection;
    IPEASF30Metadata                IPEASFData;
    TuningFaceData                  IPEASFFaceDetection;
    IPECAC2xTuningMetadata          IPECACData;
    IPEChromaEnhancement12Metadata  IPEChromaEnhancementData;
    IPEChromaSuppression20Metadata  IPEChromaSuppressionData;
    IPECC13TuningMetadata           IPECCData;
    CST12TuningMetadata             IPECSTData;
    HNR10TuningMetadata             IPEHNRData;
    TuningFaceData                  IPEHNRFaceDetection;
    IPELENR10TuningMetadata         IPELENRData;
    TuningFaceData                  IPELENRFaceDetection;
    IPELTMTuningExpData             IPELTMExposureData;
    IPESCE11Metadata                IPESCEData;
    IPETF20TuningMetadata           IPETFData;
    IPEUpscaler20TuningMetadata     IPEUpscalerData;
} CAMX_PACKED;

/// @brief Structure to encapsulate metadata from IPE
struct IPETuningMetadata
{
    UINT32                          IPEChipsetVersion;
    IPEFirmwareHeader               IPEFWHeader;
    IPETuningTriggerData            IPETuningTriggers;
    IQNodeInformation               IPENodeInformation;
    TuningModeDebugData             IPETuningModeDebugData;

    union
    {
        IPETuningMetadata17x       IPETuningMetadata17x;
        IPETuningMetadata480       IPETuningMetadata480;
    };
} CAMX_PACKED;

CAMX_END_PACKED

class TDDebugDataWriter : public DebugDataWriter
{
public:
    TDDebugDataWriter()
        : DebugDataWriter(WriterId::OtherWriter)
    {
    }

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateTypeDefinitionTag
    ///
    /// @brief  Adds all fields for this specific type of tag. This is an implementation of a pure virtual function.
    ///
    /// @param  type    The tag type to be added
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PopulateTypeDefinitionTag(
        DebugDataTagType type)
    {
        BOOL bSuccess = FALSE;

        switch (type)
        {
            case DebugDataTagType::TuningIFEEnableConfig:
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                break;
            case DebugDataTagType::TuningIFETriggerData:
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt16);
                bSuccess |= AddTypedefField(DebugDataTagType::Int32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                break;
            case DebugDataTagType::TuningAECExposureData:
                bSuccess |= AddTypedefField(DebugDataTagType::UInt64);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                break;
            case DebugDataTagType::TuningAECFrameControl:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::TuningAECExposureData,
                                                 CAMX_ARRAY_SIZE(TuningAECFrameControl::exposureInfo));
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt8);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt8);
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(TuningAECFrameControl::LEDCurrents));
                break;
           case DebugDataTagType::TuningAWBFrameControl:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::Float,
                                                 CAMX_ARRAY_SIZE(TuningAWBFrameControl::AWBGains));
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::Bool);
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::Float,
                                                 CAMX_ARRAY_SIZE(TuningAWBFrameControl::CCMOffset));
                bSuccess |= AddTypedefField2DArray(DebugDataTagType::Float,
                                                   CAMX_ARRAY_ROWS(TuningAWBFrameControl::CCM),
                                                   CAMX_ARRAY_COLS(TuningAWBFrameControl::CCM));
                break;
            case DebugDataTagType::TuningSensorResolution:
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                break;
            case DebugDataTagType::TuningSensorCropInfo:
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                break;
            case DebugDataTagType::TuningLSC34MeshLUT:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(LSC34MeshLUT::GRRLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(LSC34MeshLUT::GBBLUT));
                break;
            case DebugDataTagType::TuningLSC40MeshLUT:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(LSC40MeshLUT::GRRLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(LSC40MeshLUT::GBBLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(LSC40MeshLUT::gridLUT));
                break;
            case DebugDataTagType::TuningGammaCurve:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(GammaCurve::curve));
                break;
            case DebugDataTagType::TuningGTMLUT:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt64,
                                                 CAMX_ARRAY_SIZE(GTMLUT::LUT));
                break;
            case DebugDataTagType::TuningPedestalLUT:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(PedestalLUT::GRRLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(PedestalLUT::GBBLUT));
                break;
            case DebugDataTagType::TuningIFE2xHDR:
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                break;
            case DebugDataTagType::TuningIFE30HDR:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IFEHDRMetadata480::HDRConfig1));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IFEHDRMetadata480::HDRConfig2));
                break;
            case DebugDataTagType::TuningBPSBPCPDPC20Config:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(BPSBPCPDPC20TuningMetadata::BPCPDPConfig0));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(BPSBPCPDPC20TuningMetadata::BPCPDPConfig1));
                break;
            case DebugDataTagType::TuningBPSHDR22Config:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(BPSHDR22TuningMetadata::HDRReconConfig));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(BPSHDR22TuningMetadata::HDRMACConfig));
                break;
            case DebugDataTagType::TuningABFLUT:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(BPSABFLUT::noiseLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(BPSABFLUT::noise1LUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(BPSABFLUT::activityLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(BPSABFLUT::darkLUT));
                break;
            case DebugDataTagType::TuningHNR10LUT:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(HNR10LUT::LNRLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(HNR10LUT::FNRAndClampLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(HNR10LUT::FNRAcLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(HNR10LUT::SNRLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(HNR10LUT::blendLNRLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(HNR10LUT::blendSNRLUT));
                break;
            case DebugDataTagType::TuningBPSTriggerData:
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt16);
                bSuccess |= AddTypedefField(DebugDataTagType::Int32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                break;
            case DebugDataTagType::TuningIFESensorConfig:
            case DebugDataTagType::TuningBPSSensorConfig:
                bSuccess |= AddTypedefField(DebugDataTagType::Int32);
                bSuccess |= AddTypedefField(DebugDataTagType::Int32);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                break;
            case DebugDataTagType::TuningICA10LUT:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEICA10LUT::PerspectiveLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEICA10LUT::grid0LUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEICA10LUT::grid1LUT));
                break;
            case DebugDataTagType::TuningANR10Config:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEANR10TuningDCBlend1RegCmd::DCBlend1));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEANR10TuningRNFRegCmd::RNF));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEANR10TuningDCUSRegCmd::DCUS));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEANR10TuningCFilter2RegCmd::CFilter2));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEANR10TuningDCBlend2RegCmd::DCBlend2));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEANR10TuningCYLPFPreLensGainRegCmd::CYLPFPreLensGain));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEANR10TuningCYLPFLensGainRegCmd::CYLPFLensGain));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEANR10TuningCYLPFPostLensGainRegCmd::CYLPFPostLensGain));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEANR10TuningCNRRegCmd::CNR));
                break;
            case DebugDataTagType::TuningTF10Config:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPETF10TuningPassData::TFRegister));
                break;
            case DebugDataTagType::TuningLTM13LUT:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM13LUT::WeightLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM13LUT::LA0LUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM13LUT::LA1LUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM13LUT::CurveLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM13LUT::ScaleLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM13LUT::MaskLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM13LUT::LCEPositiveLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM13LUT::LCENegativeLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM13LUT::RGamma0LUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM13LUT::RGamma1LUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM13LUT::RGamma2LUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM13LUT::RGamma3LUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM13LUT::RGamma4LUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM13LUT::AverageLUT));
                break;
            case DebugDataTagType::Tuning2DLUT10LUT:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPE2DLUT10LUT::Hue0LUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPE2DLUT10LUT::Hue1LUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPE2DLUT10LUT::Hue2LUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPE2DLUT10LUT::Hue3LUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPE2DLUT10LUT::Saturation0LUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPE2DLUT10LUT::Saturation1LUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPE2DLUT10LUT::Saturation2LUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPE2DLUT10LUT::Saturation3LUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPE2DLUT10LUT::InverseHueLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPE2DLUT10LUT::InverseSaturationLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPE2DLUT10LUT::Hue1DLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPE2DLUT10LUT::Saturation1DLUT));
                break;
            case DebugDataTagType::TuningASF30LUT:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEASF30LUT::Layer1ActivityNormalGainPosGainNegSoftThLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEASF30LUT::Layer1GainWeightLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEASF30LUT::Layer1SoftThresholdWeightLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEASF30LUT::Layer2ActivityNormalGainPosGainNegSoftThLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEASF30LUT::Layer2GainWeightLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEASF30LUT::Layer2SoftThresholdWeightLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEASF30LUT::ChromaGradientPosNegLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEASF30LUT::ContrastPosNegLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEASF30LUT::SkinActivityGainLUT));
                break;
            case DebugDataTagType::TuningUpscaler20LUT:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEUpscaler20LUT::ALUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEUpscaler20LUT::BLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEUpscaler20LUT::CLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEUpscaler20LUT::DLUT));
                break;
            case DebugDataTagType::TuningGrainAdder10LUT:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEGrainAdder10LUT::Channel1LUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEGrainAdder10LUT::Channel2LUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEGrainAdder10LUT::Channel3LUT));
                break;
            case DebugDataTagType::TuningFaceData:
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(TuningFaceData::faceCenterX));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(TuningFaceData::faceCenterY));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(TuningFaceData::faceRadius));
                break;
            case DebugDataTagType::TuningIPEGamma15Curve:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEGamma15Curve::curve));
                break;
            case DebugDataTagType::TuningIQNodeInfo:
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt64);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                break;
            case DebugDataTagType::TuningModeInfo:
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                break;
            case DebugDataTagType::TuningBPSBPCPDPC30LUT:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt64,
                                                 CAMX_ARRAY_SIZE(BPSPDPC30LUT::BPSPDPCMaskLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(BPSPDPC30LUT::BPSPDPCNoiseLUT));
                break;
            case DebugDataTagType::TuningBPSBPCPDPC30Config:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(BPSBPCPDPC30TuningMetadata::BPCPDPConfig0));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(BPSBPCPDPC30TuningMetadata::BPCPDPConfig1));
                break;
            case DebugDataTagType::TuningBPSFirmwareHeader:
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(BPSFirmwareHeader::BPSFirmwareSetting));
                break;
            case DebugDataTagType::TuningIFELSC40Config:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IFELSCMetadata480::LSCConfig1));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IFELSCMetadata480::LSCConfig2));
                break;
            case DebugDataTagType::TuningICA30LUT:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEICA30LUT::PerspectiveLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt64,
                                                 CAMX_ARRAY_SIZE(IPEICA30LUT::grid0LUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt64,
                                                 CAMX_ARRAY_SIZE(IPEICA30LUT::grid1LUT));
                break;
            case DebugDataTagType::TuningLTM14LUT:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM14LUT::WeightLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM14LUT::LALUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM14LUT::CurveLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM14LUT::ScaleLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM14LUT::MaskLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM14LUT::LCEPositiveLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM14LUT::LCENegativeLUT));
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPELTM14LUT::RGammaLUT));
                break;
            case DebugDataTagType::TuningTF20Config:
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPETF20TuningPassData::TFRegister));
                break;
            case DebugDataTagType::TuningIPEFirmwareHeader:
                bSuccess |= AddTypedefField(DebugDataTagType::UInt32);
                bSuccess |= AddTypedefFieldArray(DebugDataTagType::UInt32,
                                                 CAMX_ARRAY_SIZE(IPEFirmwareHeader::IPEFirmwareSetting));
                break;
            case DebugDataTagType::TuningIPETriggerData:
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                bSuccess |= AddTypedefField(DebugDataTagType::Float);
                break;
            default:
                return CamxResultEUnsupported;
        }

        if (FALSE == bSuccess)
        {
            return CamxResultENoMemory;
        }

        return CamxResultSuccess;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSizeOfType
    ///
    /// @brief  Get the size of customer type
    ///
    /// @param  type    The data type to get the size of
    ///
    /// @return Size of data
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SIZE_T GetSizeOfType(
        DebugDataTagType type)
    {
        SIZE_T size;

        // Short circuit this method if its a built-in type
        size = GetSizeOfBuiltInType(type);
        if (0 != size)
        {
            return size;
        }

        switch (type)
        {
            case DebugDataTagType::TuningIFEEnableConfig:
                size = sizeof(IFETuningModuleEnableConfig);
                break;
            case DebugDataTagType::TuningIFETriggerData:
                size = sizeof(IFETuningTriggerData);
                break;
            case DebugDataTagType::TuningAECExposureData:
                size = sizeof(TuningAECExposureData);
                break;
            case DebugDataTagType::TuningAECFrameControl:
                size = sizeof(TuningAECFrameControl);
                break;
            case DebugDataTagType::TuningAWBFrameControl:
                size = sizeof(TuningAWBFrameControl);
                break;
            case DebugDataTagType::TuningSensorResolution:
                size = sizeof(SensorResolution);
                break;
            case DebugDataTagType::TuningSensorCropInfo:
                size = sizeof(SensorCropInfo);
                break;
            case DebugDataTagType::TuningLSC34MeshLUT:
                size = sizeof(LSC34MeshLUT);
                break;
            case DebugDataTagType::TuningIFELSC40Config:
                size = sizeof(IFELSCMetadata480);
                break;
            case DebugDataTagType::TuningLSC40MeshLUT:
                size = sizeof(LSC40MeshLUT);
                break;
            case DebugDataTagType::TuningGammaCurve:
                size = sizeof(GammaCurve);
                break;
            case DebugDataTagType::TuningGTMLUT:
                size = sizeof(GTMLUT);
                break;
            case DebugDataTagType::TuningPedestalLUT:
                size = sizeof(PedestalLUT);
                break;
            case DebugDataTagType::TuningIFE2xHDR:
                size = sizeof(IFEHDRMetadata17x);
                break;
            case DebugDataTagType::TuningIFE30HDR:
                size = sizeof(IFEHDRMetadata480);
                break;
            case DebugDataTagType::TuningBPSBPCPDPC20Config:
                size = sizeof(BPSBPCPDPC20TuningMetadata);
                break;
            case DebugDataTagType::TuningBPSHDR22Config:
                size = sizeof(BPSHDR22TuningMetadata);
                break;
            case DebugDataTagType::TuningABFLUT:
                size = sizeof(BPSABFLUT);
                break;
            case DebugDataTagType::TuningHNR10LUT:
                size = sizeof(HNR10LUT);
                break;
            case DebugDataTagType::TuningBPSTriggerData:
                size = sizeof(BPSTuningTriggerData);
                break;
            case DebugDataTagType::TuningIFESensorConfig:
            case DebugDataTagType::TuningBPSSensorConfig:
                size = sizeof(IFEBPSSensorConfigData);
                break;
            case DebugDataTagType::TuningICA10LUT:
                size = sizeof(IPEICA10LUT);
                break;
            case DebugDataTagType::TuningANR10Config:
                size = sizeof(IPEANR10TuningPassData);
                break;
            case DebugDataTagType::TuningTF10Config:
                size = sizeof(IPETF10TuningPassData);
                break;
            case DebugDataTagType::TuningLTM13LUT:
                size = sizeof(IPELTM13LUT);
                break;
            case DebugDataTagType::Tuning2DLUT10LUT:
                size = sizeof(IPE2DLUT10LUT);
                break;
            case DebugDataTagType::TuningASF30LUT:
                size = sizeof(IPEASF30LUT);
                break;
            case DebugDataTagType::TuningUpscaler20LUT:
                size = sizeof(IPEUpscaler20LUT);
                break;
            case DebugDataTagType::TuningGrainAdder10LUT:
                size = sizeof(IPEGrainAdder10LUT);
                break;
            case DebugDataTagType::TuningFaceData:
                size = sizeof(TuningFaceData);
                break;
            case DebugDataTagType::TuningIPEGamma15Curve:
                size = sizeof(IPEGamma15Curve);
                break;
            case DebugDataTagType::TuningIQNodeInfo:
                size = sizeof(IQNodeInformation);
                break;
            case DebugDataTagType::TuningModeInfo:
                size = sizeof(TuningModeDebugData);
                break;
            case DebugDataTagType::TuningBPSBPCPDPC30LUT:
                size = sizeof(BPSPDPC30LUT);
                break;
            case DebugDataTagType::TuningBPSBPCPDPC30Config:
                size = sizeof(BPSBPCPDPC30TuningMetadata);
                break;
            case DebugDataTagType::TuningBPSFirmwareHeader:
                size = sizeof(BPSFirmwareHeader);
                break;
            case DebugDataTagType::TuningICA30LUT:
                size = sizeof(IPEICA30LUT);
                break;
            case DebugDataTagType::TuningLTM14LUT:
                size = sizeof(IPELTM14LUT);
                break;
            case DebugDataTagType::TuningTF20Config:
                size = sizeof(IPETF20TuningPassData);
                break;
            case DebugDataTagType::TuningIPEFirmwareHeader:
                size = sizeof(IPEFirmwareHeader);
                break;
            case DebugDataTagType::TuningIPETriggerData:
                size = sizeof(IPETuningTriggerData);
                break;
            default:
                break;
        }

        return size;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddTypeDefinitionTags
    ///
    /// @brief  Add a set of custom type definition tags based on the implementation requirements
    ///
    /// @return  CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AddTypeDefinitionTags()
    {
        return DebugDataWriter::AddTypeDefinitionTags(DebugDataTagType::TuningTypeBegin, DebugDataTagType::TuningTypeEnd);
    }
};


CAMX_NAMESPACE_END
#endif // CAMXTUNINGDUMP_H
