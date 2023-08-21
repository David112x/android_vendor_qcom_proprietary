////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chiiqmodulesettings.h
///
/// @brief Qualcomm Technologies, Inc. IQ modules setting structure definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIIQMODULESETTINGS_H
#define CHIIQMODULESETTINGS_H

#include "chiiqmoduledefines.h"
#include "chipdlibcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/// @brief Data structure that are exposed to OEM must be packed to have the expected layout
#pragma pack(push, CDK_PACK_SIZE)

/// @brief IEF IQ setting struct to associated with vendor tag
struct OEMIFEIQSetting
{
    BOOL                                 PedestalEnable;
    pedestal13_rgn_dataType              PedestalSetting;
    pedestal13_enable_sectionStruct      PedestalEnableSection;
    BOOL                                 LinearizationEnable;
    linearization33_rgn_dataType         LinearizationSetting;
    BOOL                                 PDPCEnable;
    pdpc11_rgn_dataType                  PDPCSetting;
    pdpc11_enable_section                PDPCDataEnable;
    pdpc30_rgn_dataType                  PDPC30Setting;
    pdpc30_enable_sectionStruct          PDPC30EnableSection;
    BOOL                                 DemuxEnable;
    chromatix_demux13_reserveType        DemuxSetting;
    demux13_enable_sectionStruct         DemuxEnableSection;
    BOOL                                 HDREnable;

    union
    {
        // For HDR v2.0
        struct
        {
            hdr20_rgn_dataType          HDRSetting;
            chromatix_hdr20_reserveType HDRReserveType;
        } HDR20;

        // For HDR v2.2
        struct
        {
            hdr22_rgn_dataType          HDRSetting;
            hdr22_enable_sectionStruct  HDREnableSection;
            chromatix_hdr22_reserveType HDRReserveType;
        } HDR22;

        // For HDR v2.3
        struct
        {
            hdr23_rgn_dataType          HDRSetting;
            chromatix_hdr22_reserveType HDRReserveType;
        } HDR23;

        // For HDR v2.3
        struct
        {
            hdr30_rgn_dataType          HDRSetting;
            hdr30_enable_sectionStruct  HDREnableSection;
            chromatix_hdr30_reserveType HDRReserveType;
        } HDR30;
    } HDRData;

    BOOL                                 BPCBCCEnable;
    bpcbcc50_rgn_dataType                BPCBCCSetting;
    BOOL                                 ABFEnable;
    abf34_rgn_dataType                   ABFSetting;
    abf34_enable_sectionStruct           ABFEnableSection;
    chromatix_abf34_reserveType          ABFReserveType;
    BOOL                                 BLSEnable;
    bls12_rgn_dataType                   BLSSetting;
    bls12_enable_sectionStruct           BLSEnableSection;
    BOOL                                 LSCEnable;
    lsc34_rgn_dataType                   LSCSetting;
    lsc34_enable_sectionStruct           LSCEnableSection;
    lsc40_golden_rgn_dataType            LSC40Setting;
    chromatix_lsc40_reserveType          LSC40CCReserveType;
    lsc40_enable_sectionStruct           LSC40EnableSection;
    BOOL                                 DemosaicEnable;
    demosaic36_rgn_dataType              DemosaicSetting;
    demosaic36_enable_sectionStruct      DemosaicEnableSection;
    BOOL                                 CCEnable;
    cc12_rgn_dataType                    CCSetting;
    chromatix_cc12_reserveType           CCReserveType;
    BOOL                                 GTMEnable;
    gtm10_rgn_dataType                   GTMSetting;
    gtm10_enable_sectionStruct           GTMEnableSection;
    chromatix_gtm10_reserveType          GTMReserveType;
    BOOL                                 GammaEnable;
    gamma16_channel_dataType             GammaSetting[3];  // One for each Channel R/G/B
    gamma16_enable_sectionStruct         GammaEnableSection;
    BOOL                                 CSTEnable;
    chromatix_cst12_reserveType          CSTSetting;
    cst12_enable_sectionStruct           CSTEnableSection;
    BOOL                                 BCEnable;
    bincorr10_rgn_dataType               BC10Setting;
    bincorr10_enable_sectionStruct       BC10EnableSection;
    PDHwConfig                           IFEPDHwConfig;
};

/// @brief BPS IQ setting struct to associated with vendor tag
struct OEMBPSIQSetting
{
    BOOL                                 PedestalEnable;
    pedestal13_rgn_dataType              PedestalSetting;
    pedestal13_enable_sectionStruct      PedestalEnableSection;
    BOOL                                 LinearizationEnable;
    linearization34_rgn_dataType         LinearizationSetting;
    linearization34_enable_sectionStruct LinearizationEnableSection;
    BOOL                                 PDPCEnable;
    pdpc20_rgn_dataType                  PDPCSetting;
    pdpc20_enable_sectionStruct          PDPCEnableSection;
    pdpc30_rgn_dataType                  PDPC30Setting;
    pdpc30_enable_sectionStruct          PDPC30EnableSection;
    BOOL                                 DemuxEnable;
    chromatix_demux13_reserveType        DemuxSetting;
    demux13_enable_sectionStruct         DemuxEnableSection;
    BOOL                                 HDREnable;
    hdr22_rgn_dataType                   HDRSetting;
    hdr22_enable_sectionStruct           HDREnableSection;
    hdr30_rgn_dataType                   HDR30Setting;
    hdr30_enable_sectionStruct           HDR30EnableSection;
    chromatix_hdr30_reserveType          HDR30ReserveType;
    BOOL                                 BCEnable;
    bincorr10_rgn_dataType               BC10Setting;
    bincorr10_enable_sectionStruct       BC10EnableSection;
    chromatix_hdr22_reserveType          HDRReserveType;
    BOOL                                 ABFEnable;
    abf40_rgn_dataType                   ABFSetting;
    abf40_enable_sectionStruct           ABFEnableSection;
    chromatix_abf40_reserveType          ABFReserveType;
    BOOL                                 BLSEnable;
    bls12_rgn_dataType                   BLSSetting;
    bls12_enable_sectionStruct           BLSEnableSection;
    BOOL                                 GICEnable;
    gic30_rgn_dataType                   GICSetting;
    gic30_enable_sectionStruct           GICEnableSection;
    chromatix_gic30_reserveType          GICReserveType;
    BOOL                                 LSCEnable;
    lsc34_rgn_dataType                   LSCSetting;
    lsc34_enable_sectionStruct           LSCEnableSection;
    lsc40_golden_rgn_dataType            LSC40Setting;
    chromatix_lsc40_reserveType          LSC40CCReserveType;
    lsc40_enable_sectionStruct           LSC40EnableSection;
    BOOL                                 WBEnable;
    BOOL                                 DemosaicEnable;
    demosaic36_rgn_dataType              DemosaicSetting;
    demosaic36_enable_sectionStruct      DemosaicEnableSection;
    BOOL                                 CCEnable;
    cc13_rgn_dataType                    CCSetting;
    cc13_enable_sectionStruct            CCEnableSection;
    chromatix_cc13_reserveType           CCReserveType;
    BOOL                                 GTMEnable;
    gtm10_rgn_dataType                   GTMSetting;
    gtm10_enable_sectionStruct           GTMEnableSection;
    chromatix_gtm10_reserveType          GTMReserveType;
    BOOL                                 GammaEnable;
    gamma16_channel_dataType             GammaSetting[3];  // One for each Channel R/G/B
    gamma16_enable_sectionStruct         GammaEnableSection;
    BOOL                                 CSTEnable;
    chromatix_cst12_reserveType          CSTSetting;
    cst12_enable_sectionStruct           CSTEnableSection;
    BOOL                                 HNREnable;
    hnr10_rgn_dataType                   HNRSetting;
    hnr10_enable_sectionStruct           HNREnableSection;
    chromatix_hnr10_reserveType          HNRReserveType;
    BOOL                                 TMCEnable;
    tmc12_rgn_dataType                   TMC12Setting;
    tmc12_enable_sectionStruct           TMC12EnableSection;
};

/// @brief IPE IQ setting struct to associated with vendor tag
struct OEMIPEIQSetting
{
    BOOL                                 ASFEnable;
    asf30_rgn_dataType                   ASFSetting;
    asf30_enable_sectionStruct           ASFEnableSection;
    chromatix_asf30_reserveType          ASFReserveType;
    BOOL                                 CACEnable;
    cac22_rgn_dataType                   CACSetting;
    cac22_enable_sectionStruct           CACEnableSection;
    cac23_rgn_dataType                   CAC23Setting;
    cac23_enable_sectionStruct           CAC23EnableSection;
    BOOL                                 CSTEnable;
    cst12_enable_sectionStruct           CSTEnableSection;
    chromatix_cst12_reserveType          CSTSetting;
    BOOL                                 LTMEnable;
    ltm13_rgn_dataType                   LTMSetting;
    ltm13_enable_sectionStruct           LTMEnableSection;
    chromatix_ltm13_reserveType          LTMReserveType;
    ltm14_rgn_dataType                   LTM14Setting;
    ltm14_enable_sectionStruct           LTM14EnableSection;
    chromatix_ltm14_reserveType          LTM14ReserveType;
    BOOL                                 CCEnable;
    cc13_rgn_dataType                    CCSetting;
    cc13_enable_sectionStruct            CCEnableSection;
    chromatix_cc13_reserveType           CCReserveType;
    BOOL                                 GammaEnable;
    gamma15_enable_sectionStruct         GammaEnableSection;
    gamma15_cct_dataType                 GammaSetting;
    BOOL                                 ChromaEnhancementEnable;
    cv12_rgn_dataType                    ChromaEnhancementSetting;
    cv12_enable_sectionStruct            ChromaEnhancementEnableSection;
    BOOL                                 ChromaSuppressionEnable;
    cs20_rgn_dataType                    ChromaSuppressionSetting;
    cs20_enable_sectionStruct            ChromaSuppressionEnableSection;
    chromatix_cs20_reserveType           ChromaSuppressionReserveType;
    BOOL                                 SkinEnhancementEnable;
    sce11_rgn_dataType                   SkinEnhancementSetting;
    sce11_enable_sectionStruct           SkinEnhancementEnableSection;
    chromatix_sce11_reserveType          SkinEnhancementReserveType;
    BOOL                                 GrainAdderEnable;
    gra10_rgn_dataType                   GrainAdderSetting;
    gra10_enable_sectionStruct           GrainAdderEnableSection;
    chromatix_gra10_reserveType          GrainAdderReserveType;
    BOOL                                 TDL10Enable;
    tdl10_rgn_dataType                   TDL10Setting;
    tdl10_enable_sectionStruct           TDL10EnableSection;
    chromatix_tdl10_reserveType          TDL10ReserveType;
    BOOL                                 TF10Enable;
    tf10_cct_dataType                    TF10Setting;
    tf10_enable_sectionStruct            TF10EnableSection;
    chromatix_tf10_reserveType           TF10ReserveType;
    BOOL                                 TF20Enable;
    tf20_cct_dataType                    TF20Setting;
    tf20_enable_sectionStruct            TF20EnableSection;
    chromatix_tf20_reserveType           TF20ReserveType;
    BOOL                                 ANR10Enable;
    anr10_cct_dataType                   ANR10Setting;
    anr10_enable_sectionStruct           ANR10EnableSection;
    chromatix_anr10_reserveType          ANR10ReserveType;
    BOOL                                 US20Enable;
    upscale20_rgn_dataType               US20Setting;
    upscale20_enable_sectionStruct       US20EnableSection;
    chromatix_upscale20_reserveType      US20ReserveType;
    BOOL                                 US12Enable;
    hnr10_rgn_dataType                   HNRSetting;
    hnr10_enable_sectionStruct           HNREnableSection;
    chromatix_hnr10_reserveType          HNRReserveType;
    BOOL                                 HNREnable;
    lenr10_rgn_dataType                  LENRSetting;
    lenr10_enable_sectionStruct          LENREnableSection;
    chromatix_lenr10_reserveType         LENRReserveType;
    BOOL                                 LENREnable;

};

#pragma pack(pop)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CHIIQMODULESETTINGS_H
