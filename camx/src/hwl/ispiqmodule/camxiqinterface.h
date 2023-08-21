////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxiqinterface.h
/// @brief CamX IQ Interface class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXIQINTERFACE_H
#define CAMXIQINTERFACE_H

/// IQ Chromatix Headers
#include "abf_3_4_0.h"
#include "asf_3_0_0.h"
#include "bincorr_1_0_0.h"
#include "bls_1_2_0.h"
#include "bpcbcc_5_0_0.h"
#include "cac_2_3_0.h"
#include "cc_1_2_0.h"
#include "cc_1_3_0.h"
#include "cst_1_2_0.h"
#include "cv_1_2_0.h"
#include "demux_1_3_0.h"
#include "demosaic_3_6_0.h"
#include "demosaic_3_7_0.h"
#include "gamma_1_5_0.h"
#include "gamma_1_6_0.h"
#include "gtm_1_0_0.h"
#include "hdr_3_0_0.h"
#include "linearization_3_3_0.h"
#include "lsc_3_4_0.h"
#include "ltm_1_4_0.h"
#include "lsc_4_0_0.h"
#include "pdpc_1_1_0.h"
#include "pedestal_1_3_0.h"
#include "tintless_2_0_0.h"
#include "upscale_2_0_0.h"
#include "ica_2_0_0.h"
/// Camx Headers
#include "bps_data.h"
#include "camxbpsabf40.h"
#include "camxbpsbpcpdpc20.h"
#include "camxbpsbpcpdpc30.h"
#include "camxbpscc13.h"
#include "camxbpscst12.h"
#include "camxbpsdemosaic36.h"
#include "camxbpsdemux13.h"
#include "camxbpsgic30.h"
#include "camxbpshdr22.h"
#include "camxbpshnr10.h"
#include "camxbpslinearization34.h"
#include "camxbpspedestal13.h"
#include "camxbpswb13.h"
#include "camxbpslsc34.h"
#include "camxbpslsc40.h"
#include "camxformats.h"
#include "camxifeabf34.h"
#include "camxifeabf40.h"
#include "camxifebls12.h"
#include "camxifebpcbcc50.h"
#include "camxifecc12.h"
#include "camxifecc13.h"
#include "camxifecst12.h"
#include "camxifedemosaic36.h"
#include "camxifedemosaic37.h"
#include "camxifedemux13.h"
#include "camxifedsx10.h"
#include "camxipegamma15.h"
#include "camxifegamma16.h"
#include "camxifegtm10.h"
#include "camxifehdr20.h"
#include "camxifehdr22.h"
#include "camxifehdr23.h"
#include "camxifelinearization33.h"
#include "camxifelsc34.h"
#include "camxifelsc40.h"
#include "camxifepdpc11.h"
#include "camxifepedestal13.h"
#include "camxifewb12.h"
#include "camxifewb13.h"
#include "camxipeanr10.h"
#include "camxipeasf30.h"
#include "camxipecac22.h"
#include "camxipechromaenhancement12.h"
#include "camxipechromasuppression20.h"
#include "camxipecolorcorrection13.h"
#include "camxipecolortransform12.h"
#include "camxipegamma15.h"
#include "camxipeica10.h"
#include "camxipeica20.h"
#include "camxipeica20.h"
#include "camxipeltm13.h"
#include "camxipeltm14.h"
#include "camxipesce11.h"
#include "camxipe2dlut10.h"
#include "camxipegrainadder10.h"
#include "camxipeupscaler12.h"
#include "camxipeupscaler20.h"
#include "camxtypes.h"
#include "camxipeanr10.h"

// CHI Headers
#include "chitintlessinterface.h"

CAMX_NAMESPACE_BEGIN
static const UINT32 MAX_NUM_OF_CAMERA                   = 16; // max physical camera id
static const UINT32 MAX_SENSOR_ASPECT_RATIO             = 2; // mostly sensor aspect ratio can be 4:3 or 16:9
enum class PipelineType
{
    IFE,    ///< IFE HW Pipeline
    BPS,    ///< BPS HW Pipeline
    IPE     ///< IPE HW Pipeline
};

/// @brief OEM Trigger Tag Format
struct OEMTriggerListSet
{
    CHAR   tagSessionName[125];  ///< tag Session Name 1
    CHAR   tagName[125];         ///< tag Name
};

/// @brief Function Table of the IQ Modules
struct IQOperationTable
{
    BOOL (*IQModuleInitialize)(IQLibraryData* pInitializeData);
    BOOL (*IQModuleUninitialize)(IQLibraryData* pInitializeData);
    BOOL (*IQFillOEMTuningTriggerData)(const ISPIQTriggerData*  pInput,
                                       ISPIQTuningDataBuffer*   pTriggerOEMData);
    VOID (*IQTriggerDataDump)(ISPIQTriggerData* pTriggerData);

    BOOL (*IQSetHardwareVersion)(UINT32 titanVersion, UINT32 hardwareVersion);
    BOOL (*ASF30TriggerUpdate)(ISPIQTriggerData* pInput, ASF30InputData* pOutput);
    BOOL (*ASF30Interpolation)(const ASF30InputData* pInput, asf_3_0_0::asf30_rgn_dataType* pData);
    BOOL (*ASF30CalculateHWSetting)(const ASF30InputData*                                 pInput,
                                    asf_3_0_0::asf30_rgn_dataType*                        pData,
                                    asf_3_0_0::chromatix_asf30_reserveType*               pReserveType,
                                    asf_3_0_0::chromatix_asf30Type::enable_sectionStruct* pModuleEnable,
                                    VOID*                                                 pUnpackedField);

    BOOL (*BC10TriggerUpdate)(ISPIQTriggerData* pInput, BC10InputData* pTriggerData);
    BOOL (*BC10Interpolation)(const BC10InputData* pInput, bincorr_1_0_0::bincorr10_rgn_dataType* pData);
    BOOL (*BC10CalculateHWSetting)(const BC10InputData*               pInput,
        bincorr_1_0_0::bincorr10_rgn_dataType*                        pData,
        bincorr_1_0_0::chromatix_bincorr10Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                         pOutput);

    BOOL (*BLS12TriggerUpdate)(ISPIQTriggerData* pInput, BLS12InputData* pOutput);
    BOOL (*BLS12Interpolation)(const BLS12InputData* pInput, bls_1_2_0::bls12_rgn_dataType* pData);
    BOOL (*BLS12CalculateHWSetting)(const BLS12InputData*                                 pInput,
                                    bls_1_2_0::bls12_rgn_dataType*                        pData,
                                    bls_1_2_0::chromatix_bls12Type::enable_sectionStruct* pModuleEnable,
                                    VOID*                                                 pUnpackedField);

    BOOL (*ABF40TriggerUpdate)(ISPIQTriggerData* pInput, ABF40InputData* pOutput);
    BOOL (*ABF40Interpolation)(const ABF40InputData* pInput, abf_4_0_0::abf40_rgn_dataType* pData);
    BOOL (*ABF40CalculateHWSetting)(ABF40InputData*                                       pInput,
                                       abf_4_0_0::abf40_rgn_dataType*                        pData,
                                       const abf_4_0_0::chromatix_abf40_reserveType*         pReserveType,
                                       abf_4_0_0::chromatix_abf40Type::enable_sectionStruct* pModuleEnable,
                                       VOID*                                                 pUnpackedField);

    BOOL (*BPSGIC30TriggerUpdate)(ISPIQTriggerData* pInput, GIC30InputData* pOutput);
    BOOL (*BPSGIC30Interpolation)(const GIC30InputData* pInput, gic_3_0_0::gic30_rgn_dataType* pData);
    BOOL (*BPSGIC30CalculateHWSetting)(const GIC30InputData*                                 pInput,
                                       gic_3_0_0::gic30_rgn_dataType*                        pData,
                                       gic_3_0_0::chromatix_gic30_reserveType*               pReserveType,
                                       gic_3_0_0::chromatix_gic30Type::enable_sectionStruct* pModuleEnable,
                                       VOID*                                                 pUnpackedField);

    BOOL (*HDR22TriggerUpdate)(ISPIQTriggerData* pInput, HDR22InputData* pOutput);
    BOOL (*HDR22Interpolation)(const HDR22InputData* pInput, hdr_2_2_0::hdr22_rgn_dataType* pData);
    BOOL (*HDR22CalculateHWSetting)(const HDR22InputData*                                 pInput,
                                       hdr_2_2_0::hdr22_rgn_dataType*                        pData,
                                       hdr_2_2_0::chromatix_hdr22_reserveType*               pReserveType,
                                       hdr_2_2_0::chromatix_hdr22Type::enable_sectionStruct* pModuleEnable,
                                       VOID*                                                 pUnpackedField);

    BOOL(*HDR30TriggerUpdate)(ISPIQTriggerData* pInput, HDR30InputData* pOutput);
    BOOL(*HDR30Interpolation)(const HDR30InputData* pInput, hdr_3_0_0::hdr30_rgn_dataType* pData);
    BOOL(*HDR30CalculateHWSetting)(const HDR30InputData*                                 pInput,
        hdr_3_0_0::hdr30_rgn_dataType*                        pData,
        hdr_3_0_0::chromatix_hdr30_reserveType*               pReserveType,
        hdr_3_0_0::chromatix_hdr30Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                 pUnpackedField);

    BOOL (*Linearization34TriggerUpdate)(ISPIQTriggerData* pInput, Linearization34IQInput* pOutput);
    BOOL (*Linearization34Interpolation)(const Linearization34IQInput*                      pInput,
                                         linearization_3_4_0::linearization34_rgn_dataType* pData);
    BOOL (*Linearization34CalculateHWSetting)(const Linearization34IQInput*       pInput,
        linearization_3_4_0::linearization34_rgn_dataType*                        pData,
        linearization_3_4_0::chromatix_linearization34Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                                     pUnpackedField);

    BOOL (*BPSPDPC20TriggerUpdate)(ISPIQTriggerData* pInput, PDPC20IQInput* pOutput);
    BOOL (*BPSPDPC20Interpolation)(const PDPC20IQInput* pInput, pdpc_2_0_0::pdpc20_rgn_dataType* pData);
    BOOL (*BPSPDPC20CalculateHWSetting)(const PDPC20IQInput*                                    pInput,
                                        pdpc_2_0_0::pdpc20_rgn_dataType*                        pData,
                                        pdpc_2_0_0::chromatix_pdpc20Type::enable_sectionStruct* pModuleEnable,
                                        VOID*                                                   pUnpackedField);

    BOOL(*PDPC30TriggerUpdate)(ISPIQTriggerData* pInput, PDPC30IQInput* pOutput);
    BOOL(*PDPC30Interpolation)(const PDPC30IQInput* pInput, pdpc_3_0_0::pdpc30_rgn_dataType* pData);
    BOOL(*PDPC30CalculateHWSetting)(const PDPC30IQInput*                                    pInput,
        pdpc_3_0_0::pdpc30_rgn_dataType*                        pData,
        pdpc_3_0_0::chromatix_pdpc30Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                   pUnpackedField);

    BOOL(*CAC22Interpolation)(const CAC22InputData* pInput, cac_2_2_0::cac22_rgn_dataType* pData);
    BOOL(*CAC22CalculateHWSetting)(const CAC22InputData*                                 pInput,
                                   cac_2_2_0::cac22_rgn_dataType*                        pData,
                                   cac_2_2_0::chromatix_cac22Type::enable_sectionStruct* pModuleEnable,
                                   VOID*                                                 pUnpackedField);

    BOOL(*CAC23Interpolation)(const CAC23InputData* pInput, cac_2_3_0::cac23_rgn_dataType* pData);
    BOOL(*CAC23CalculateHWSetting)(const CAC23InputData*                                 pInput,
                                   cac_2_3_0::cac23_rgn_dataType*                        pData,
                                   cac_2_3_0::chromatix_cac23Type::enable_sectionStruct* pModuleEnable,
                                   VOID*                                                 pUnpackedField);

    BOOL (*CC13TriggerUpdate)(ISPIQTriggerData* pInput, CC13InputData* pOutput);
    BOOL (*CC13Interpolation)(const CC13InputData* pInput, cc_1_3_0::cc13_rgn_dataType* pData);
    BOOL (*CC13CalculateHWSetting)(const CC13InputData*                                pInput,
                                   cc_1_3_0::cc13_rgn_dataType*                        pData,
                                   cc_1_3_0::chromatix_cc13_reserveType*               pReserveType,
                                   cc_1_3_0::chromatix_cc13Type::enable_sectionStruct* pModuleEnable,
                                   VOID*                                               pUnpackedField);

    BOOL (*CST12CalculateHWSetting)(const CST12InputData*                                 pInput,
                                    cst_1_2_0::chromatix_cst12_reserveType*               pReserveType,
                                    cst_1_2_0::chromatix_cst12Type::enable_sectionStruct* pModuleEnable,
                                    VOID*                                                 pUnpackedField);

    BOOL (*CV12TriggerUpdate)(ISPIQTriggerData* pInput, CV12InputData* pOutput);
    BOOL (*CV12Interpolation)(const CV12InputData* pInput, cv_1_2_0::cv12_rgn_dataType* pData);
    BOOL (*CV12CalculateHWSetting)(const CV12InputData*                                pInput,
                                   cv_1_2_0::cv12_rgn_dataType*                        pData,
                                   cv_1_2_0::chromatix_cv12Type::enable_sectionStruct* pModuleEnable,
                                   VOID*                                               pUnpackedField);

    BOOL (*demosaic36TriggerUpdate)(ISPIQTriggerData* pInput, Demosaic36InputData* pOutput);
    BOOL (*demosaic36Interpolation)(const Demosaic36InputData* pInput, demosaic_3_6_0::demosaic36_rgn_dataType* pData);
    BOOL (*demosaic36CalculateHWSetting)(const Demosaic36InputData*                                      pInput,
                                         demosaic_3_6_0::demosaic36_rgn_dataType*                        pData,
                                         demosaic_3_6_0::chromatix_demosaic36Type::enable_sectionStruct* pModuleEnable,
                                         VOID*                                                           pUnpackedField);

    BOOL(*demosaic37TriggerUpdate)(ISPIQTriggerData* pInput, Demosaic37InputData* pOutput);
    BOOL(*demosaic37Interpolation)(const Demosaic37InputData* pInput, demosaic_3_7_0::demosaic37_rgn_dataType* pData);
    BOOL(*demosaic37CalculateHWSetting)(const Demosaic37InputData*                                      pInput,
                                        demosaic_3_7_0::demosaic37_rgn_dataType*                        pData,
                                        demosaic_3_7_0::chromatix_demosaic37Type::enable_sectionStruct* pModuleEnable,
                                        VOID*                                                           pUnpackedField);

    BOOL (*demux13CalculateHWSetting)(const Demux13InputData*                                   pInput,
                                      const demux_1_3_0::chromatix_demux13_reserveType*         pReserveType,
                                      demux_1_3_0::chromatix_demux13Type::enable_sectionStruct* pModuleEnable,
                                      VOID*                                                     pUnpackedField);

    BOOL (*gamma15TriggerUpdate)(ISPIQTriggerData* pInput, Gamma15InputData* pOutput);
    BOOL (*gamma15Interpolation)(const Gamma15InputData*                                pInput,
                                 gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct* pData);
    BOOL (*gamma15CalculateHWSetting)(const Gamma15InputData*                                   pInput,
                                      gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct*    pData,
                                      gamma_1_5_0::chromatix_gamma15Type::enable_sectionStruct* pModuleEnable,
                                      VOID*                                                     pUnpackedField);

    BOOL (*gamma16TriggerUpdate)(ISPIQTriggerData* pInput, Gamma16InputData* pOutput);
    BOOL (*gamma16Interpolation)(const Gamma16InputData* pInput, gamma_1_6_0::mod_gamma16_channel_dataType* pData);
    BOOL (*gamma16CalculateHWSetting)(const Gamma16InputData*                                   pInput,
                                      gamma_1_6_0::mod_gamma16_channel_dataType*                pChannelData,
                                      gamma_1_6_0::chromatix_gamma16Type::enable_sectionStruct* pModuleEnable,
                                      VOID*                                                     pUnpackedField);

    BOOL (*gra10TriggerUpdate)(const ISPIQTriggerData* pInput, GRA10IQInput* pOutput);
    BOOL (*gra10Interpolation)(const GRA10IQInput* pInput, gra_1_0_0::gra10_rgn_dataType* pData);
    BOOL (*gra10CalculateHWSetting)(const GRA10IQInput*                                   pInput,
                                    gra_1_0_0::gra10_rgn_dataType*                        pData,
                                    gra_1_0_0::chromatix_gra10_reserveType*               pReserveData,
                                    gra_1_0_0::chromatix_gra10Type::enable_sectionStruct* pModuleEnable,
                                    VOID*                                                 pUnpackedField);

    BOOL (*GTM10TriggerUpdate)(ISPIQTriggerData* pInput, GTM10InputData* pOutput);
    BOOL (*GTM10Interpolation)(const GTM10InputData* pInput, gtm_1_0_0::gtm10_rgn_dataType* pData);
    BOOL (*GTM10CalculateHWSetting)(GTM10InputData*                                       pInput,
                                    const gtm_1_0_0::gtm10_rgn_dataType*                  pData,
                                    const gtm_1_0_0::chromatix_gtm10_reserveType*         pReserveType,
                                    gtm_1_0_0::chromatix_gtm10Type::enable_sectionStruct* pModuleEnable,
                                    VOID*                                                 pUnpackedField);

    BOOL (*HNR10TriggerUpdate)(ISPIQTriggerData* pInput, HNR10InputData* pOutput);
    BOOL (*HNR10Interpolation)(const HNR10InputData* pInput, hnr_1_0_0::hnr10_rgn_dataType* pData);
    BOOL (*HNR10CalculateHWSetting)(const HNR10InputData*                                 pInput,
                                    hnr_1_0_0::hnr10_rgn_dataType*                        pData,
                                    hnr_1_0_0::chromatix_hnr10_reserveType*               pReserveType,
                                    hnr_1_0_0::chromatix_hnr10Type::enable_sectionStruct* pModuleEnable,
                                    VOID*                                                 pUnpackedField);

    BOOL (*ICA10Interpolation)(const ICAInputData* pInput, ica_1_0_0::ica10_rgn_dataType* pData);
    BOOL (*ICA20Interpolation)(const ICAInputData* pInput, ica_2_0_0::ica20_rgn_dataType* pData);
    BOOL (*ICA30Interpolation)(const ICAInputData* pInput, ica_3_0_0::ica30_rgn_dataType* pData);
    BOOL (*ICACalculateHWSetting)(const ICAInputData*                     pInput,
                                   VOID*                                   pData,
                                   VOID*                                   pReserveData,
                                   VOID*                                   pUnpackedField);
    BOOL (*GetICAInitializationData)(ICANcLibOutputData* pData);

    BOOL (*IFEABF34TriggerUpdate)(ISPIQTriggerData* pInput, ABF34InputData* pOutput);
    BOOL (*IFEABF34Interpolation)(const ABF34InputData* pInput, abf_3_4_0::abf34_rgn_dataType* pData);
    BOOL (*IFEABF34CalculateHWSetting)(const ABF34InputData*                                 pInput,
                                       abf_3_4_0::abf34_rgn_dataType*                        pData,
                                       abf_3_4_0::chromatix_abf34Type::enable_sectionStruct* pModuleEnable,
                                       abf_3_4_0::chromatix_abf34_reserveType*               pReserveType,
                                       VOID*                                                 pUnpackedField);

    BOOL (*IFEBPCBCC50TriggerUpdate)(ISPIQTriggerData* pInput, BPCBCC50InputData* pOutput);
    BOOL (*IFEBPCBCC50Interpolation)(const BPCBCC50InputData* pInput, bpcbcc_5_0_0::bpcbcc50_rgn_dataType* pData);
    BOOL (*IFEBPCBCC50CalculateHWSetting)(const BPCBCC50InputData*              pInput,
                                          bpcbcc_5_0_0::bpcbcc50_rgn_dataType*  pData,
                                          globalelements::enable_flag_type      moduleEnable,
                                          VOID*                                 pUnpackedField);

    BOOL (*IFECC12TriggerUpdate)(ISPIQTriggerData* pInput, CC12InputData* pOutput);
    BOOL (*IFECC12Interpolation)(const CC12InputData* pInput, cc_1_2_0::cc12_rgn_dataType* pData);
    BOOL (*IFECC12CalculateHWSetting)(const CC12InputData*                  pInput,
                                      cc_1_2_0::cc12_rgn_dataType*          pData,
                                      cc_1_2_0::chromatix_cc12_reserveType* pReserveType,
                                      VOID*                                 pUnpackedField);

    BOOL (*IFEHDR20TriggerUpdate)(ISPIQTriggerData* pInput, HDR20InputData* pOutput);
    BOOL (*IFEHDR20Interpolation)(const HDR20InputData* pInput, hdr_2_0_0::hdr20_rgn_dataType* pData);
    BOOL (*IFEHDR20CalculateHWSetting)(const HDR20InputData*                   pInput,
                                       hdr_2_0_0::hdr20_rgn_dataType*          pData,
                                       hdr_2_0_0::chromatix_hdr20_reserveType* pReserveType,
                                       VOID*                                   pUnpackedField);

    BOOL(*IFEHDR23TriggerUpdate)(ISPIQTriggerData* pInput, HDR23InputData* pOutput);
    BOOL(*IFEHDR23Interpolation)(const HDR23InputData* pInput, hdr_2_3_0::hdr23_rgn_dataType* pData);
    BOOL(*IFEHDR23CalculateHWSetting)(const HDR23InputData*                                  pInput,
                                    hdr_2_3_0::hdr23_rgn_dataType*                        pData,
                                    hdr_2_3_0::chromatix_hdr23_reserveType*               pReserveType,
                                    VOID*                                                 pUnpackedField);

    BOOL (*IFELinearization33TriggerUpdate)(ISPIQTriggerData* pInput, Linearization33InputData* pOutput);
    BOOL (*IFELinearization33Interpolation)(const Linearization33InputData* pInput,
                                            linearization_3_3_0::linearization33_rgn_dataType* pData);
    BOOL (*IFELinearization33CalculateHWSetting)(const Linearization33InputData*                    pInput,
                                                  linearization_3_3_0::linearization33_rgn_dataType* pData,
                                                  VOID*                                              pUnpackedField);

    BOOL (*LSC34TriggerUpdate)(ISPIQTriggerData* pInput, LSC34InputData* pOutput);
    BOOL (*LSC34Interpolation)(const LSC34InputData* pInput, lsc_3_4_0::lsc34_rgn_dataType* pData);
    BOOL (*LSC34CalculateHWSetting)(LSC34InputData*                                       pInput,
                                    lsc_3_4_0::lsc34_rgn_dataType*                        pData,
                                    lsc_3_4_0::chromatix_lsc34Type::enable_sectionStruct* pModuleEnable,
                                    tintless_2_0_0::tintless20_rgn_dataType*              pTintlessData,
                                    VOID*                                                 pUnpackedField);

    BOOL(*LSC40TriggerUpdate)(ISPIQTriggerData* pInput, LSC40InputData* pOutput);
    BOOL(*LSC40Interpolation)(const LSC40InputData* pInput, lsc_4_0_0::lsc40_rgn_dataType* pData);
    BOOL(*LSC40CalculateHWSetting)(LSC40InputData*                                       pInput,
                                   lsc_4_0_0::lsc40_rgn_dataType*                        pData,
                                   lsc_4_0_0::chromatix_lsc40_reserveType*               pReserveType,
                                   lsc_4_0_0::chromatix_lsc40Type::enable_sectionStruct* pModuleEnable,
                                   tintless_2_0_0::tintless20_rgn_dataType*              pTintlessData,
                                   VOID*                                                 pUnpackedField);

    BOOL (*IFEPDPC11TriggerUpdate)(ISPIQTriggerData* pInput, PDPC11InputData* pOutput);
    BOOL (*IFEPDPC11Interpolation)(const PDPC11InputData* pInput, pdpc_1_1_0::pdpc11_rgn_dataType*  pData);
    BOOL (*IFEPDPC11CalculateHWSetting)(const PDPC11InputData*                                  pInput,
                                        pdpc_1_1_0::pdpc11_rgn_dataType*                        pData,
                                        pdpc_1_1_0::chromatix_pdpc11Type::enable_sectionStruct* pModuleEnable,
                                        VOID*                                                   pUnpackedField);

    BOOL (*IPECS20TriggerUpdate)(ISPIQTriggerData* pInput, CS20InputData* pOutput);
    BOOL (*IPECS20Interpolation)(const CS20InputData* pInput, cs_2_0_0::cs20_rgn_dataType* pData);
    BOOL (*IPECS20CalculateHWSetting)(const CS20InputData*                                pInput,
                                      cs_2_0_0::cs20_rgn_dataType*                        pData,
                                      cs_2_0_0::chromatix_cs20_reserveType*               pReserveType,
                                      cs_2_0_0::chromatix_cs20Type::enable_sectionStruct* pModuleEnable,
                                      VOID*                                               pUnpackedField);

    BOOL(*IPETDL10TriggerUpdate)(ISPIQTriggerData* pInput, TDL10InputData* pOutput);
    BOOL(*IPETDL10Interpolation)(const TDL10InputData* pInput, tdl_1_0_0::tdl10_rgn_dataType* pData);
    BOOL(*IPETDL10CalculateHWSetting)(const TDL10InputData*                                 pInput,
                                      tdl_1_0_0::tdl10_rgn_dataType*                        pData,
                                      tdl_1_0_0::chromatix_tdl10_reserveType*               pReserveType,
                                      tdl_1_0_0::chromatix_tdl10Type::enable_sectionStruct* pModuleEnable,
                                      VOID*                                                 pUnpackedField);

    BOOL (*pedestal13TriggerUpdate)(ISPIQTriggerData* pInput, Pedestal13InputData* pOutput);
    BOOL (*pedestal13Interpolation)(const Pedestal13InputData* pInput, pedestal_1_3_0::pedestal13_rgn_dataType* pData);
    BOOL (*pedestal13CalculateHWSetting)(const Pedestal13InputData*                                      pInput,
                                         pedestal_1_3_0::pedestal13_rgn_dataType*                        pData,
                                         pedestal_1_3_0::chromatix_pedestal13Type::enable_sectionStruct* pModuleEnable,
                                         VOID*                                                           pRegCmd);

    BOOL (*WB12CalculateHWSetting)(const WB12InputData* pInput,
                                   VOID*                pUnpackedField);

    BOOL (*WB13CalculateHWSetting)(const WB13InputData*             pInput,
                                   globalelements::enable_flag_type moduleEnable,
                                   VOID*                            pUnpackedField);

    BOOL (*SCE11Interpolation)(const SCE11InputData* pInput, sce_1_1_0::sce11_rgn_dataType*  pData);
    BOOL (*SCE11CalculateHWSetting)(const SCE11InputData*                                 pInput,
                                    sce_1_1_0::sce11_rgn_dataType*                        pData,
                                    sce_1_1_0::chromatix_sce11_reserveType*               pReserveType,
                                    sce_1_1_0::chromatix_sce11Type::enable_sectionStruct* pModuleEnable,
                                    VOID*                                                 pUnpackedField);

    BOOL (*TF10TriggerUpdate)(ISPIQTriggerData* pInput, TF10InputData* pOutput);
    BOOL (*TF10Interpolation)(const TF10InputData* pInput, tf_1_0_0::mod_tf10_cct_dataType::cct_dataStruct* pData);
    BOOL (*TF10CalculateHWSetting)(const TF10InputData*                                pInput,
                                  tf_1_0_0::mod_tf10_cct_dataType::cct_dataStruct*    pData,
                                  tf_1_0_0::chromatix_tf10_reserveType*               pReserveData,
                                  tf_1_0_0::chromatix_tf10Type::enable_sectionStruct* pModuleEnable,
                                  VOID*                                               pUnpackedField);
    BOOL (*GetTF10InitializationData)(TFNcLibOutputData* pData);

    BOOL (*TF20TriggerUpdate)(ISPIQTriggerData* pInput, TF20InputData* pOutput);
    BOOL (*TF20Interpolation)(const TF20InputData* pInput, tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct* pData);
    BOOL (*TF20CalculateHWSetting)(const TF20InputData*                                pInput,
                                  tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct*    pData,
                                  tf_2_0_0::chromatix_tf20_reserveType*               pReserveData,
                                  tf_2_0_0::chromatix_tf20Type::enable_sectionStruct* pModuleEnable,
                                  VOID*                                               pUnpackedField);
    BOOL (*GetTF20InitializationData)(TFNcLibOutputData* pData);

    BOOL (*ANR10TriggerUpdate)(ISPIQTriggerData* pInput, ANR10InputData* pOutput);
    BOOL (*ANR10Interpolation)(const ANR10InputData* pInput, anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct* pData);
    BOOL (*ANR10CalculateHWSetting)(const ANR10InputData*                                 pInput,
                                   anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct*    pData,
                                   anr_1_0_0::chromatix_anr10_reserveType*               pReserveData,
                                   anr_1_0_0::chromatix_anr10Type::enable_sectionStruct* pModuleEnable,
                                   VOID*                                                 pUnpackedField);
    BOOL (*GetANR10InitializationData)(ANRNcLibOutputData* pData);

    BOOL(*LTM13TriggerUpdate)(ISPIQTriggerData* pInput, LTM13InputData* pOutput);
    BOOL(*LTM13Interpolation)(const LTM13InputData* pInput, ltm_1_3_0::ltm13_rgn_dataType* pData);
    BOOL(*LTM13CalculateHWSetting)(LTM13InputData*                                       pInput,
                                   ltm_1_3_0::ltm13_rgn_dataType*                        pData,
                                   ltm_1_3_0::chromatix_ltm13_reserveType*               pReserveData,
                                   ltm_1_3_0::chromatix_ltm13Type::enable_sectionStruct* pModuleEnable,
                                   VOID*                                                 pUnpackedField);

    BOOL(*LTM14TriggerUpdate)(ISPIQTriggerData* pInput, LTM14InputData* pOutput);
    BOOL(*LTM14Interpolation)(const LTM14InputData* pInput, ltm_1_4_0::ltm14_rgn_dataType* pData);
    BOOL(*LTM14CalculateHWSetting)(LTM14InputData*                                       pInput,
                                   ltm_1_4_0::ltm14_rgn_dataType*                        pData,
                                   ltm_1_4_0::chromatix_ltm14_reserveType*               pReserveData,
                                   ltm_1_4_0::chromatix_ltm14Type::enable_sectionStruct* pModuleEnable,
                                   VOID*                                                 pUnpackedField);

    BOOL (*upscale20Interpolation)(const Upscale20InputData* pInput, upscale_2_0_0::upscale20_rgn_dataType* pData);
    BOOL (*upscale20CalculateHWSetting)(const Upscale20InputData*                                     pInput,
                                        upscale_2_0_0::upscale20_rgn_dataType*                        pData,
                                        upscale_2_0_0::chromatix_upscale20_reserveType*               pReserveData,
                                        upscale_2_0_0::chromatix_upscale20Type::enable_sectionStruct* pModuleEnable,
                                        VOID*                                                         pUnpackedField);

    BOOL(*upscale12CalculateHWSetting)(const Upscale12InputData*                                      pInput,
                                        VOID*                                                         pUnpackedField);

    BOOL(*TMC10TriggerUpdate)(ISPIQTriggerData* pInput, TMC10InputData* pOutput);
    BOOL(*TMC10Interpolation)(const TMC10InputData* pInput, tmc_1_0_0::tmc10_rgn_dataType*  pData);

    BOOL(*TINTLESS20Interpolation)(const Tintless20InterpolationInput* pInput, tintless_2_0_0::tintless20_rgn_dataType* pData);

    BOOL(*TMC11TriggerUpdate)(ISPIQTriggerData* pInput, TMC11InputData* pOutput);
    BOOL(*TMC11Interpolation)(const TMC11InputData* pInput, tmc_1_1_0::tmc11_rgn_dataType* pData);

    BOOL(*LENR10TriggerUpdate)(ISPIQTriggerData* pInput, LENR10InputData* pOutput);
    BOOL(*LENR10Interpolation)(const LENR10InputData* pInput, lenr_1_0_0::lenr10_rgn_dataType* pData);
    BOOL(*LENR10CalculateHWSetting)(const LENR10InputData*                                 pInput,
        lenr_1_0_0::lenr10_rgn_dataType*                        pData,
        lenr_1_0_0::chromatix_lenr10_reserveType*               pReserveType,
        lenr_1_0_0::chromatix_lenr10Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                   pUnpackedField);

    BOOL(*DSX10TriggerUpdate)(ISPIQTriggerData* pInput, DSX10InputData* pOutput);
    BOOL(*DSX10Interpolation)(const DSX10InputData* pInput, dsx_1_0_0::dsx10_rgn_dataType* pData);
    BOOL(*DSX10CalculateHWSetting)(const DSX10InputData*                          pInput,
        dsx_1_0_0::dsx10_rgn_dataType*                                            pData,
        const dsx_1_0_0::chromatix_dsx10_reserveType*                             pReserveData,
        const ds4to1_1_1_0::mod_ds4to1v11_pass_reserve_dataType::pass_dataStruct* pDS4to1Data,
        VOID*                                                                     pOutput);
    BOOL(*GetDSX10InitializationData)(DSXNcLibOutputData* pData);

    BOOL(*TMC12TriggerUpdate)(ISPIQTriggerData* pInput, TMC12InputData* pOutput);
    BOOL(*TMC12Interpolation)(const TMC12InputData* pInput, tmc_1_2_0::tmc12_rgn_dataType* pData);

    BOOL(*CVP10TriggerUpdate)(ISPIQTriggerData* pInput, CVP10InputData* pOutput);
    BOOL(*CVP10Interpolation)(const CVP10InputData* pInput, cvp_1_0_0::cvp10_rgn_dataType* pData);
    BOOL(*CVP10CalculateHWSetting)(CVP10InputData* pInput,
        cvp_1_0_0::cvp10_rgn_dataType*             pData,
        cvp_1_0_0::chromatix_cvp10_reserveType*    pReserveType,
        VOID*                                      pOutput);
};

/// @brief Output Data to Demux13 IQ Algorithm
struct Demux13OutputData
{
    PipelineType type;                           ///< Identifies the pipeline type
};

/// @brief Output Data to Demosaic36 IQ Algorithm
struct Demosaic36OutputData
{
    PipelineType type;                          ///< Identifies the pipeline type
};

/// @brief Output Data to Demosaic37 IQ Algorithm
struct Demosaic37OutputData
{
    PipelineType type;                          ///< Identifies the pipeline type
};

/// @brief Output Data to ABF34 IQ Algorithm
struct ABF34OutputData
{
    UINT32*           pDMIData;     ///< Pointer to the ABF DMI data
};

/// @brief Output Data to DSX10
struct DSX10OutputData
{
    UINT64*  pLumaKernelWeightsHoriz;   ///< Pointer to Luma Kernel Weights Horizontal
    UINT64*  pLumaKernelWeightsVert;    ///< Pointer to Luma Kernel Weights Vertical
    UINT64*  pChromaKernelWeightsHoriz; ///< Pointer to Chroma Kernel Weights Horizontal
    UINT64*  pChromaKernelWeightsVert;  ///< Pointer to Chroma Kernel Weights Vertical
    UINT32   lumaStartingLocation;      ///< Luma Starting location
    UINT32   lumaInputImageWidth;       ///< Luma Input Image Width
    UINT32   lumaOutputImageWidth;      ///< Luma Output Image Width
    UINT32   lumaScaleRatio;            ///< Luma Scale Ratio
    UINT32   chromaStartingLocation;    ///< Chroma Starting Location
    UINT32   chromaInputImageWidth;     ///< Chroma Input Image Witdh
    UINT32   chromaOutputImageWidth;    ///< Chroma Output Imagw Width
    UINT32   chromaScaleRatio;          ///< Chroma Scale Ratio
    DSState* pState;                    ///< Pointer to the current DS State
};

/// @brief Output Data to CST12
struct CST12OutputData
{
    PipelineType type;                  ///< Identifies the pipeline type
};

/// @brief Output Data to Crop
struct CropOutputData
{
    CropState*         pCropState;      ///< Crop State
    IFEPipelinePath    modulePath;      ///< IFE pipeline path for module
    UINT32             ifeOutputPath;   ///< IPE piepline output
};

/// @brief Output Data to PreCrop10
struct PreCrop10OutputData
{
    IFEPipelinePath    modulePath;      ///< IFE pipeline path for module
};

/// @brief Output Data to PreCrop10
struct R2PD10OutputData
{
    IFEPipelinePath    modulePath;      ///< IFE pipeline path for module
    UINT32             packMode;        ///< Pack mode based on the number of bit
};

/// @brief Output Data to DualPD10
struct DualPD10OutputData
{
    UINT32*            pDMIDataPtr;     ///< Pointer to the DMI table
};

/// @brief Output Data to PDAF20
struct PDAF20OutputData
{
    VOID*   pDMIDataPtr;    ///< Pointer to the DMI data
    UINT32  width;          ///< PDAF Output Width
    UINT32  height;         ///< PDAF Output Height
};

/// @brief Output Data of LCR
struct IFELCR10OutputData
{
    UINT32 width;   ///< LCR Output Width
    UINT32 height;  ///< LCR Output Height
};

/// @brief Output Data to MNDS
struct MNDSOutputData
{
    MNDSState*      pMNDSState;      ///< MNDS State
    Format          pixelFormat;     ///< pixel Format from the sensor
    UINT32          ifeOutputPath;   ///< IPE piepline output
};

/// @brief Output Data to Linearization33 IQ Algorithm
struct Linearization33OutputData
{
    UINT64*                    pDMIDataPtr;           ///< Pointer to the DMI table
    FLOAT                      stretchGainRed;        ///< Stretch Gain Red
    FLOAT                      stretchGainGreenEven;  ///< Stretch Gain Green Even
    FLOAT                      stretchGainGreenOdd;   ///< Stretch Gain Green Odd
    FLOAT                      stretchGainBlue;       ///< Stretch Gain Blue
    UINT32                     dynamicBlackLevel[4];  ///< Previous Dynamic Black Level
    BOOL                       registerBETEn;         ///< Register BET test enabled
};

/// @brief Output Data to BLS12 IQ Algorithm
struct BLS12OutputData
{
    UINT32 blackLevelOffset;    ///< BLS black level offset
};

/// @brief Output Data to CC13 IQ Algorithm
struct CC13OutputData
{
    PipelineType type;  ///< Identifies the pipeline type: IFE, BPS, IPE
};

struct LSCStripeState
{
    UINT16 bwidth_l;                      ///< Subgrid width, 9u, 2*Bwidth is the real width, n mean n+1
    UINT16 meshGridBwidth_l;              ///< Meshgrid width, 9u, 2*MeshGridBwidth is the real width, n mean n+1
                                          ///< not used in rolloff implementation, only as HW counters
    UINT16 lx_start_l;                    ///< Start block x index, 6u
    UINT16 bx_start_l;                    ///< Start subgrid x index within start block, 3u
    UINT16 bx_d1_l;                       ///< x coordinate of top left pixel in start block/subgrid, 9u
    UINT16 num_meshgain_h;                ///< Number of horizontal mesh gains, n mean n+2
};

/// @brief Output Data of the LSC34 IQ Algorithm
struct LSC34OutputData
{
    PipelineType type;                            ///< Identifies the pipeline type

    LSC34UnpackedField* pUnpackedField;           ///< Pointer to the LSC unpacked field
    UINT32*             pGRRLUTDMIBuffer;         ///< Pointer to GRR DMI buffer
    UINT32*             pGBBLUTDMIBuffer;         ///< Pointer to GBB DMI buffer
    LSCStripeState      lscState;                 ///< LSC state, needed for striping lib
};

struct LSC40OutputData
{
    PipelineType type;                            ///< Identifies the pipeline type

    LSC40UnpackedField* pUnpackedField;           ///< Pointer to the LSC unpacked field
    UINT32*             pGRRLUTDMIBuffer;         ///< Pointer to GRR DMI buffer
    UINT32*             pGBBLUTDMIBuffer;         ///< Pointer to GBB DMI buffer
    UINT32*             pGridLUTDMIBuffer;        ///< Pointer to Grid DMI buffer
    LSCStripeState      lscState;                 ///< LSC state, needed for striping lib
};

/// @brief Output Data to Gamma16 IQ Algorithm
struct Gamma16OutputData
{
    PipelineType type;            ///< Identifies the pipeline type
    UINT32*      pRDMIDataPtr;    ///< Pointer to the R DMI table
    UINT32*      pGDMIDataPtr;    ///< Pointer to the G DMI table
    UINT32*      pBDMIDataPtr;    ///< Pointer to the B DMI table
};

struct PDPCState
{
    int32_t  pdaf_global_offset_x;        ///< 14u; PD pattern start global offset x
    int32_t  pdaf_x_end;                  ///< 14u; horizontal PDAF pixel end location (0 means first  pixel from left)
    int32_t  pdaf_zzHDR_first_rb_exp;     ///< 1u; 0x0: T1 (long exp), 0x1: T2 (short exp)
    uint32_t PDAF_PD_Mask[64];            ///< PD location mask for 64 32-bit words;
                                          ///< for each bit 0: not PD pixel; 1: PD pixel
};

struct PDPC30State
{
    BOOL  enable;              ///< enable;
    INT32 PDAFGlobalOffsetX;   ///< pdaf_global_offset_x;
    INT32 PDAFTableoffsetX;    ///< pdaf_table_x_offset;
    INT32 PDAFEndX;            ///< pdaf_x_end;
    INT32 PDAFzzHDRFirstRBExp; ///< pdaf_zzHDR_first_rb_exp;
    INT16 PDAFPDPCEnable;      ///< pdaf_pdpc_en;
};

/// @brief Output Data to PDPC11 IQ Algorithm
struct PDPC11OutputData
{
    UINT32*           pDMIDataPtr;   ///< Pointer to the DMI data
    PDPCState         pdpcState;     ///< PDPC state, needed for striping lib
};

/// @brief Output Data to PDPC30 IQ Algorithm
struct IFEPDPC30OutputData
{
    UINT32*     pDMINoiseStdLUTDataPtr;   ///< Pointer to the Noise Std LUT DMI table
    UINT32*     pDMIPDAFMaskLUTDataPtr;   ///< Pointer to the DMI data
    PDPC30State pdpcState;                ///< PDPC state, needed for striping lib
    VOID*       pSettingsData;            ///< Pointer to Settings Data
};

/// @brief Output Data to BPSHNR10 IQ Algorithm
struct HNR10OutputData
{
    HnrParameters*      pHNRParameters;          ///< Pointer to the HNR parameters
    UINT32*             pLNRDMIBuffer;           ///< Pointer to the LNR Dmi data
    UINT32*             pFNRAndClampDMIBuffer;   ///< Pointer to the Merged FNR and Gain Clamp data
    UINT32*             pFNRAcDMIBuffer;         ///< Pointer to the FNR ac data
    UINT32*             pSNRDMIBuffer;           ///< Pointer to the SNR data
    UINT32*             pBlendLNRDMIBuffer;      ///< Pointer to the Blend LNR data
    UINT32*             pBlendSNRDMIBuffer;      ///< Pointer to the Blend SNR data
};

/// @brief Output Data to GTM10 IQ Algorithm
struct GTM10OutputData
{
    PipelineType type;              ///< Identifies the pipeline type

    union
    {
        struct
        {
            UINT64* pDMIDataPtr;    ///< Pointer to the DMI table
        } IFE;

        struct
        {
            UINT64* pDMIDataPtr;    ///< Pointer to the DMI table
        } BPS;
    } regCmd;

    BOOL    registerBETEn;          ///< Register BET test enabled
    UINT32  bankSelect;             ///< Bank Slect
    BOOL    bIsBankUpdateValid;     ///< Is Bank Update Valid
};

/// @brief Output Data to SCE11 IQ Algorithem
struct SCE11OutputData
{
};

/// @brief Output Data to GIC30 IQ Algorithm
struct GIC30OutputData
{
    UINT32*               pDMIData;         ///< Pointer to the DMI table
};

struct HDR20State
{
    UINT16 hdr_zrec_first_rb_exp;           ///< 0~1, first R/B exposure 0=T1, 1=T2
};

/// @brief Output Data to HDR20 IQ Algorithm
struct HDR20OutputData
{
    HDR20State       hdr20State;    ///< HDR state, for striping lib
};

/// @brief Output Data to HDR IQ Algorithem
struct HDR22OutputData
{
    PipelineType type;                  ///< Identifies the pipeline type
};

struct HDR23State
{
    UINT16 hdr_zrec_first_rb_exp;           ///< 0~1, first R/B exposure 0=T1, 1=T2
};

/// @brief Output Data to HDR20 IQ Algorithm
struct HDR23OutputData
{
    HDR23State   hdr23State;                ///< HDR state, for striping lib
};

/// @brief Output Data to HDR30 IQ Algorithm
struct HDR30OutputData
{
    PipelineType type;                                       ///< Identifies the pipeline type
};

struct Ped13State
{
    uint16_t bwidth_l;                      ///< Subgrid width, 11u, 2*Bwidth is the real width   (n means n+1)
    uint16_t meshGridBwidth_l;              ///< Meshgrid width, 11u, 2*MeshGridBwidth is the real width  (n means n+1)
    uint16_t lx_start_l;                    ///< Start block x index, 4u
    uint16_t bx_start_l;                    ///< Start subgrid x index within start block, 3u
    uint16_t bx_d1_l;                       ///< x coordinate of top left pixel in start block/subgrid, 9u
};

/// @brief Output Data to Pedestal13 IQ Algorithm
struct Pedestal13OutputData
{
    PipelineType type;                                     ///< Identifies the pipeline type

    UINT32* pGRRLUTDMIBuffer;                              ///< GRR LUT DMI Buffer Pointer
    UINT32* pGBBLUTDMIBuffer;                              ///< GBB LUT DMI Buffer Pointer

    Ped13State pedState;                                   ///< Pedestal state, needed for striping lib
};

/// @brief Output Data to WhiteBalance12 IQ Algorithem
struct WB12OutputData
{
    UINT32 rGain;                       ///< WB12 R Gain
    UINT32 gGain;                       ///< WB12 G Gain
    UINT32 bGain;                       ///< WB12 B Gain
};

/// @brief Output Data to WhiteBalance13 IQ Algorithem
struct WB13OutputData
{
    AWBGainParams manualControl;      ///< AWB CC gain from app
    BOOL          manualGainOverride; ///< Manual Control override flag
};

/// @brief Output Data to ABF40
struct ABF40OutputData
{
    PipelineType type;                          ///< Identifies the pipeline type
    UINT32*               pNoiseLUT;            ///< Point to the ABF40 Noise DMI data
    UINT32*               pNoiseLUT1;           ///< Point to the ABF40 Noise DMI data LUT1
    UINT32*               pActivityLUT;         ///< Point to the ABF40 Activity DMI data
    UINT32*               pDarkLUT;             ///< Point to the ABF40 Dark DMI data
    UINT32                blackLevelOffset;     ///< BLS black level offset
    UINT32                filterEnable;         ///< Filter EN
    UINT32                actEnable;            ///< Act EN
    UINT32                darkSmoothEnable;     ///< Dark Smooth EN
    UINT32                darkDesatEnable;      ///< Dark Desat EN
    UINT32                dirSmoothEnable;      ///< Dir Smooth EN
    UINT32                minmaxEnable;         ///< MinMax EN
    UINT32                crossPlaneEnable;     ///< Cross Panel EN
    UINT32                BLSEnable;            ///< BLS EN
    UINT32                pixMatchLevelRB;      ///< Pix RB EN
    UINT32                pixMatchLevelG;       ///< Pix G EN
    UINT32                blockMatchPatternRB;  ///< Block Match pattern RB
    UINT32                enable;               ///< Module Enable
    ABF40InputData        dependenceData;       ///< Dependence Data for this Module
    CmdBuffer*            pLUTDMICmdBuffer;     ///< Command buffer for holding all LUTs
};

/// @brief Output Data to ASF30 IQ Algorithm
struct ASF30OutputData
{
    UINT32*        pDMIDataPtr;       ///< ASF30 LUT DMI Buffer Pointer
    AsfParameters* pAsfParameters;    ///< Pointer to ASF30 Parameters for firmware
};

/// @brief Output Data to TDL10 IQ Algorithm
struct TDL10OutputData
{
    UINT32*          pD2H0LUT;     ///< Pointer to D2_H_0 data
    UINT32*          pD2H1LUT;     ///< Pointer to D2_H_1 data
    UINT32*          pD2H2LUT;     ///< Pointer to D2_H_2 data
    UINT32*          pD2H3LUT;     ///< Pointer to D2_H_3 data
    UINT32*          pD2S0LUT;     ///< Pointer to D2_S_0 data
    UINT32*          pD2S1LUT;     ///< Pointer to D2_S_1 data
    UINT32*          pD2S2LUT;     ///< Pointer to D2_S_2 data
    UINT32*          pD2S3LUT;     ///< Pointer to D2_S_3 data
    UINT32*          pD1IHLUT;     ///< Pointer to D1_IH data
    UINT32*          pD1ISLUT;     ///< Pointer to D1_IS data
    UINT32*          pD1HLUT;      ///< Pointer to D1_H data
    UINT32*          pD1SLUT;      ///< Pointer to D1_S data
};

/// @brief Output Data to CV12 IQ Algorithem
struct CV12OutputData
{
};

/// @brief Output Data to Gamma15 IQ Algorithem
struct Gamma15OutputData
{
    PipelineType type;                  ///< Pipeline type
    UINT32*      pLUT[MaxGammaLUTNum];  ///< Pointer to the DMI table
};

/// @brief Output Data to LTM13 IQ Algorithem
struct LTM13OutputData
{
    UINT32*               pDMIDataPtr;      ///< Pointer to the DMI table
};

/// @brief Output Data to LTM14 IQ Algorithem
struct LTM14OutputData
{
    UINT32*               pDMIDataPtr;      ///< Pointer to the DMI table
};

/// @brief Output Data to Upscale20 and ChromaUp20 IQ Algorithm
struct Upscale20OutputData
{
    UINT32*           pDMIPtr;               ///< Point to the Upscale20 DMI data
};

/// @brief Output Data to CAC22
struct CAC22OutputData
{
    UINT32        enableCAC2; ///< a flag after interpolation
};

/// @brief Output Data to CAC23
struct CAC23OutputData
{
    UINT32        enableCAC2; ///< a flag after interpolation
};

/// @brief Output Data to ICA10 /ICA20 IQ Algorithm
struct ICAOutputData
{
    VOID*    pICAParameter;          ///< ICA parameter
    VOID*    pLUT[ICA10Indexmax];      ///< Pointer to the DMI table
    // Output of ICA1 consumed by ICA2 only
    VOID*    pCurrICAInData;         ///< ICA input current frame Data
    VOID*    pPrevICAInData;         ///< ICA reference current frame Data
    VOID*    pCurrWarpAssistData;    ///< Current Warp assist data
    VOID*    pPrevWarpAssistData;    ///< Current Warp assist data
    // Warp geometry data rrequired by ANR/TF
    VOID*    pWarpGeometryData;      ///< Warp Geometry data
    VOID*    pCVPICAFrameCfgData;    ///< CVP ica frame config data
    VOID*    pIcaGeoParameters;      ///< ICA geo pass parameters;
};

// @brief Output Data to GRA10 IQ Algorithem
struct GRA10OutputData
{
    PipelineType       type;               ///< Pipeline type
    UINT32*            pLUT[GRALUTMax];    ///< Pointer to the DMI table
    UINT16             enableDitheringY;   ///< Dithering Y Enable
    UINT16             enableDitheringC;   ///< Dithering C Enable
    UINT32             grainSeed;          ///< Grain Seed Value
    UINT32             mcgA;               ///< Multiplier parameter for MCG calculation
    UINT32             skiAheadAJump;      ///< It will be equal to mcg_an mod m.
    UINT16             grainStrength;      ///< Grain Strength Value
};

/// @brief BPC PDPC output data
struct BPSBPCPDPC20OutputData
{
    UINT32*                   pDMIDataPtr;      ///< Pointer to the DMI table
};

/// @brief BPC PDPC output data
struct BPSPDPC30OutputData
{
    UINT64*                 pDMIDataPtrPDAF;    ///< Pointer to the PDAF DMI table
    UINT32*                 pDMIDataPtrPNoise;  ///< Pointer to the Noise DMI table
};

/// @brief Output Data to Linearization34
struct Linearization34OutputData
{
    UINT32*           pDMIDataPtr;           ///< Pointer to the DMI table
    FLOAT             stretchGainRed;        ///< Stretch Gain Red
    FLOAT             stretchGainGreenEven;  ///< Stretch Gain Green Even
    FLOAT             stretchGainGreenOdd;   ///< Stretch Gain Green Odd
    FLOAT             stretchGainBlue;       ///< Stretch Gain Blue
    UINT32            dynamicBlackLevel[4];  ///< Previous Dynamic Black Level
};

/// @brief Output Data to LENR10
struct LENR10OutputData
{
    LenrParameters* pLENRParams;  ///< LENR IQ setting
};

/// @brief Output Data to CVP10
struct CVP10OutputData
{
    FLOAT             robustness_max_allowed_NCC;                           ///< robustness max allowed NCC
    FLOAT             robustness_min_allowed_tar_var;                       ///< robustness min allowed tar var
    FLOAT             robustness_meaningful_ncc_diff;                       ///< robustness meaningful ncc diff
    FLOAT             fpx_threshold;                                        ///< fpx threshold
    FLOAT             desc_match_threshold;                                 ///< desc match threshold
    FLOAT             enable_transform_confidence;                          ///< enable transform confidence
    FLOAT             transform_confidence_mapping_base;                    ///< transform confidence mapping_base
    FLOAT             transform_confidence_mapping_c1;                      ///< transform confidence mapping c1
    FLOAT             transform_confidence_mapping_c2;                      ///< transform confidence mapping c2
    FLOAT             transform_confidence_thr_to_force_identity_transform; ///< thr to force identity transform
    UINT32            robustness_measure_dist_map[8];                       ///< robustness measure dist map
    UINT32            descriptor_Lpf;                                       ///< descriptor Lpf
    UINT32            fpx_score_shift;                                      ///< fpx score shift
    UINT32            inlier_track_enable;                                  ///< inlier track enable
    UINT32            transform_model;                                      ///< transform model
    UINT32            multi_frame_input_resolution;                         ///< multi frame input resolution
    FLOAT             video_registration_down_scale_ratio;                  ///< video registration downscale ratio
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the CAMX IQ common library interface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IQInterface
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IQSetupTriggerData
    ///
    /// @brief  Setup Trigger Data for current frame
    ///
    /// @param  pInputData          Pointer to the input data from Node
    /// @param  pNode               Pointer to the Node which calls this function
    /// @param  isRealTime          Realtime or offline
    /// @param  pIQOEMTriggerData   Pointer to tuning metadata, to be use by OEM to fill trigger data from vendor tags.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID IQSetupTriggerData(
        ISPInputData*           pInputData,
        Node*                   pNode,
        BOOL                    isRealTime,
        ISPIQTuningDataBuffer*  pIQOEMTriggerData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetADRCParams
    ///
    /// @brief  get ADRC params.
    ///
    /// @param  pInputData          Pointer to the input data from Node
    /// @param  pAdrcEnabled        Pointer to update ADRC Enablement status
    /// @param  pGtmPercentage      Pointer to update ADRC GTM tuning percentage
    /// @param  tmcVersion          TMC version
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GetADRCParams(
        const ISPInputData* pInputData,
        BOOL*               pAdrcEnabled,
        FLOAT*              pGtmPercentage,
        const SWTMCVersion  tmcVersion);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateAECGain
    ///
    /// @brief  update AEC Gain
    ///
    /// @param  mType          ISP IQModule Type
    /// @param  pInputData     Pointer to the input data from Node
    /// @param  gtmPercentage  ADRC GTM tuning percentage
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID UpdateAECGain(
        ISPIQModuleType mType,
        ISPInputData*   pInputData,
        FLOAT           gtmPercentage);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IQSettingModuleInitialize
    ///
    /// @brief  Initiate IQ Setting Module
    ///
    /// @param  pData Pointer to the IQ Library Data
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IQSettingModuleInitialize(
        IQLibInitialData* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IQSettingModuleUninitialize
    ///
    /// @brief  Uninitialize IQ Setting Module
    ///
    /// @param  pData Pointer to the IQ Library Data
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IQSettingModuleUninitialize(
        IQLibInitialData* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEGetSensorMode
    ///
    /// @brief  Convert Camx sensor pixel format based on common lib definition
    ///
    /// @param  pPixelFormat Pointer to the Camx Pixel Format
    /// @param  pSensorType  Pointer to the Sensor type in common library
    ///
    /// @return return       CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IFEGetSensorMode(
        const PixelFormat*  pPixelFormat,
        SensorType*         pSensorType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPixelFormat
    ///
    /// @brief  Convert Camx sensor pixel format based on common lib definition
    ///
    /// @param  pPixelFormat  Point to the Camx Pixel Format
    /// @param  pBayerPattern Point to the Bayer Pattern of the Sensor Input
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetPixelFormat(
        const PixelFormat*  pPixelFormat,
        UINT8*              pBayerPattern);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetADRCData
    ///
    /// @brief  get adrc specific parsed data from sw chromatix.
    ///
    /// @param  pTMCInput Pointer to the TMC10 parsed output.
    ///
    /// @return return  BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL GetADRCData(
        TMC10InputData*   pTMCInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetADRCData
    ///
    /// @brief  get adrc specific parsed data from sw chromatix.
    ///
    /// @param  pTMCInput Pointer to the TMC11 parsed output.
    ///
    /// @return return  BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOWHINE CP028: Imported code from system team
    static BOOL GetADRCData(
        TMC11InputData*   pTMCInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LSCMeshTableInit
    ///
    /// @brief  API function.  Call LSC module in common lib to initialize Previously calculated
    ///         tintless table.
    ///
    /// @param  cameraID denoting validate Tintless Table.
    /// @param  sensorAR sensor aspect ratio.
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult LSCMeshTableInit(
        UINT32                    cameraID,
        UINT32                    sensorAR);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetADRCData
    ///
    /// @brief  get adrc specific parsed data from sw chromatix.
    ///
    /// @param  pTMCInput Pointer to the TMC12 parsed output.
    ///
    /// @return return  BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOWHINE CP028: Imported code from system team
    static BOOL GetADRCData(
        TMC12InputData*   pTMCInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IQSetHardwareVersion
    ///
    /// @brief  get adrc specific parsed data from sw chromatix.
    ///
    /// @param  titanVersion    variable holding titan version.
    /// @param  hardwareVersion variable holding hardware version.
    ///
    /// @return return  BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL IQSetHardwareVersion(
        UINT32 titanVersion, UINT32 hardwareVersion);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEABF34CalculateSetting
    ///
    /// @brief  API function.  Call ABF34 module in common lib for calculation
    ///         For error code: CamxResultEUnsupported, Node should disable ABF34 operation.
    ///
    /// @param  pInput       Pointer to the Input data to the ABF34 module for calculation
    /// @param  pOEMIQData   Pointer to the OEM Input IQ Setting
    /// @param  pOutput      Pointer to the Calculation output from the ABF34 module
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IFEABF34CalculateSetting(
        const ABF34InputData*  pInput,
        VOID*                  pOEMIQData,
        ABF34OutputData*       pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Demux13CalculateSetting
    ///
    /// @brief  API function. Call IFEDemux13 Module for calculation
    ///         For error code: CamxResultEUnsupported, Node should disable Demux operation.
    ///
    /// @param  pInput      Pointer to the Input data to the Demux13 module for calculation
    /// @param  pOEMIQData  Pointer to the OEM Input IQ Setting
    /// @param  pOutput     Pointer to the Calculation output from the Demux13 module
    /// @param  pixelFormat Pixel Format indicating the sensor output
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Demux13CalculateSetting(
        Demux13InputData*  pInput,
        VOID*              pOEMIQData,
        Demux13OutputData* pOutput,
        PixelFormat        pixelFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DSX10CalculateSetting
    ///
    /// @brief  API function. Call DSX10 Module for calculation
    ///         For error code: CamxResultEUnsupported, Node should disable DSX10 operation.
    ///
    /// @param  pInput      Pointer to the Input data to the DSX10 module for calculation
    /// @param  pOEMIQData  Pointer to the OEM Input IQ Setting
    /// @param  pOutput     Pointer to the Calculation output from the DSX10 module
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult DSX10CalculateSetting(
        DSX10InputData*  pInput,
        VOID*            pOEMIQData,
        DSX10OutputData* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Demosaic36CalculateSetting
    ///
    /// @brief  API function.  Call Demosaic36 module in common lib for calculation
    ///         For error code: CamxResultEUnsupported, Node should disable Demosaic operation.
    ///
    /// @param  pInput       Pointer to the Input data to the Demosaic36 module for calculation
    /// @param  pOEMIQData   Pointer to the OEM Input IQ Setting
    /// @param  pOutput      Pointer to the Calculation output from the Demosaic36 module
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Demosaic36CalculateSetting(
        const Demosaic36InputData*  pInput,
        VOID*                       pOEMIQData,
        Demosaic36OutputData*       pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Demosaic37CalculateSetting
    ///
    /// @brief  API function.  Call Demosaic36 module in common lib for calculation
    ///         For error code: CamxResultEUnsupported, Node should disable Demosaic operation.
    ///
    /// @param  pInput       Pointer to the Input data to the Demosaic37 module for calculation
    /// @param  pOEMIQData   Pointer to the OEM Input IQ Setting
    /// @param  pOutput      Pointer to the Calculation output from the Demosaic37 module
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Demosaic37CalculateSetting(
        const Demosaic37InputData*  pInput,
        VOID*                       pOEMIQData,
        Demosaic37OutputData*       pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEBPCBCC50CalculateSetting
    ///
    /// @brief  API function Call BPCBCC50 module in common lib for calculation
    ///         For error code: CamxResultEUnsupported, Node should disable BPCBCC operation.
    ///
    /// @param  pInput     Pointer to the Input data to the BPCBCC50 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IFEBPCBCC50CalculateSetting(
        const BPCBCC50InputData*  pInput,
        VOID*                     pOEMIQData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyLSCMapData
    ///
    /// @brief  Copy LSC map data based on LSC34 module unpacked data
    ///
    /// @param  pInputData      Pointer to the ISP input data
    /// @param  pUnpackedField  Pointer to the LSC34 module unpacked data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CopyLSCMapData(
        const ISPInputData*    pInputData,
        LSC34UnpackedField*    pUnpackedField);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyLSCMapDataV40
    ///
    /// @brief  Copy LSC map data based on LSC40 module unpacked data
    ///
    /// @param  pInputData      Pointer to the ISP input data
    /// @param  pUnpackedField  Pointer to the LSC34 module unpacked data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CopyLSCMapDataV40(
        const ISPInputData*    pInputData,
        LSC40UnpackedField*    pUnpackedField);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFECC12CalculateSetting
    ///
    /// @brief  API function. Call CC12 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the CC12 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IFECC12CalculateSetting(
        const CC12InputData* pInput,
        VOID*                pOEMIQData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CST12CalculateSetting
    ///
    /// @brief  API function. Call CST12 module in common lib for calculation
    ///         For error code: CamxResultEUnsupported, Node should disable CST12 operation.
    ///
    /// @param  pInput     Pointer to the Input data to the CST12 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the CST12 module
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult CST12CalculateSetting(
        const CST12InputData* pInput,
        VOID*                 pOEMIQData,
        CST12OutputData*      pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFELinearization33CalculateSetting
    ///
    /// @brief  API function. Call Linearization33 module in common lib for calculation
    ///         For error code: CamxResultEUnsupported, Node should disable Linearization operation.
    ///
    /// @param  pInput     Pointer to the Input data to the Linearization33 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the Linearization33 module
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IFELinearization33CalculateSetting(
        const Linearization33InputData*  pInput,
        VOID*                            pOEMIQData,
        Linearization33OutputData*       pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFELinearization34CalculateSetting
    ///
    /// @brief  API function.  Call Linearization34 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the Linearization34 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the Linearization34 module
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IFELinearization34CalculateSetting(
        const Linearization34IQInput* pInput,
        VOID*                         pOEMIQData,
        Linearization34OutputData*    pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEPDPC11CalculateSetting
    ///
    /// @brief  API function.  Call PDPC11 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the PDPC11 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the PDPC11 module
    ///
    /// @return Return CamxResult. For error code: CamxResultEUnsupported, Node should
    ///         disable PDPC11 operation.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IFEPDPC11CalculateSetting(
        const PDPC11InputData*    pInput,
        VOID*                     pOEMIQData,
        PDPC11OutputData*         pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BLS12CalculateSetting
    ///
    /// @brief  API function. Call BLS12 module in common lib for calculation
    ///         For error code: CamxResultEUnsupported, Node should disable BLS12 operation.
    ///
    /// @param  pInput     Pointer to the Input data to the BLS12 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the BLS12 module
    ///
    /// @return Return  CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult BLS12CalculateSetting(
        const BLS12InputData* pInput,
        VOID*                 pOEMIQData,
        BLS12OutputData*      pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LSC34CalculateSetting
    ///
    /// @brief  API function.  Call LSC34 module in common lib for calculation
    ///         For error code: CamxResultEUnsupported, Node should disable LSC34 operation.
    ///
    /// @param  pInput           Pointer to the Input data to the LSC34 module for calculation
    /// @param  pOEMIQData       Pointer to the OEM Input IQ Setting
    /// @param  pInputData       Pointer to the ISP input data
    /// @param  pOutput          Pointer to the Calculation output from the LSC34 module
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult LSC34CalculateSetting(
        LSC34InputData*              pInput,
        VOID*                        pOEMIQData,
        const ISPInputData*          pInputData,
        LSC34OutputData*             pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LSC40CalculateSetting
    ///
    /// @brief  API function.  Call LSC34 module in common lib for calculation
    ///         For error code: CamxResultEUnsupported, Node should disable LSC34 operation.
    ///
    /// @param  pInput           Pointer to the Input data to the LSC34 module for calculation
    /// @param  pOEMIQData       Pointer to the OEM Input IQ Setting
    /// @param  pInputData       Pointer to ISP Input data
    /// @param  pOutput          Pointer to the Calculation output from the LSC34 module
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult LSC40CalculateSetting(
        LSC40InputData*              pInput,
        VOID*                        pOEMIQData,
        const ISPInputData*          pInputData,
        LSC40OutputData*             pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Gamma16CalculateSetting
    ///
    /// @brief  API function.  Call Gamma16 module in common lib for calculation
    ///
    /// @param  pInput       Pointer to the Input data to the Gamma module for calculation
    /// @param  pOEMIQData   Pointer to the OEM Input IQ Setting
    /// @param  pOutput      Pointer to the Calculated output from the Gamma module
    /// @param  pDebugBuffer Pointer to the Debugging Buffer. In Non-debugging mode, this pointer is NULL
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Gamma16CalculateSetting(
        const Gamma16InputData*  pInput,
        VOID*                    pOEMIQData,
        Gamma16OutputData*       pOutput,
        VOID*                    pDebugBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HNR10CalculateSetting
    ///
    /// @brief  API function.  Call HNR10 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the HNR10 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the HNR10 module
    ///
    /// @return Return  CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult HNR10CalculateSetting(
        const HNR10InputData*  pInput,
        VOID*                  pOEMIQData,
        HNR10OutputData*       pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GTM10CalculateSetting
    ///
    /// @brief  API function.  Call GTM10 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the GTM10 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the GTM10 module
    /// @param  pTMCInput  Pointer to the Calculation output from the TMC10 module
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GTM10CalculateSetting(
        GTM10InputData*   pInput,
        VOID*             pOEMIQData,
        GTM10OutputData*  pOutput,
        TMC10InputData*   pTMCInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SCE11CalculateSetting
    ///
    /// @brief  API function.  Call SCE11 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the SCE11 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the SCE11 module
    ///
    /// @return Return CamxResult
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult SCE11CalculateSetting(
        const SCE11InputData* pInput,
        VOID*                 pOEMIQData,
        SCE11OutputData*      pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CC13CalculateSetting
    ///
    /// @brief  API function. Call CC13 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the CC13 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the CC13 module
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult CC13CalculateSetting(
        const CC13InputData* pInput,
        VOID*                pOEMIQData,
        CC13OutputData*      pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BPSGIC30CalculateSetting
    ///
    /// @brief  API function.  Call GIC30 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the input data to the GIC30 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the calculated output from the GIC30 module
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult BPSGIC30CalculateSetting(
        const GIC30InputData* pInput,
        VOID*                 pOEMIQData,
        GIC30OutputData*      pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEHDR20CalculateSetting
    ///
    /// @brief  API function.  Call HDR20 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the HDR20 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the HDR20 module
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IFEHDR20CalculateSetting(
        const HDR20InputData* pInput,
        VOID*                 pOEMIQData,
        HDR20OutputData*      pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPECS20CalculateSetting
    ///
    /// @brief  API function.  Call CS20 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the CS20 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the CS20 module
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IPECS20CalculateSetting(
        const CS20InputData* pInput,
        VOID*                pOEMIQData,
        VOID*                pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEWB12CalculateSetting
    ///
    /// @brief  API function.  Call WB12 module in common lib for calculation
    ///
    /// @param  pInput  Pointer to the Input data to the WB12 module for calculation
    /// @param  pOutput Pointer to the Calculation output from the WB12 module
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IFEWB12CalculateSetting(
        const WB12InputData*  pInput,
        WB12OutputData*       pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WB13CalculateSetting
    ///
    /// @brief  API function.  Call WB13 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the WB13 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the WB13 module
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult WB13CalculateSetting(
        const WB13InputData*  pInput,
        VOID*                 pOEMIQData,
        WB13OutputData*       pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Pedestal13CalculateSetting
    ///
    /// @brief  API function.  Call Pedestal36 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the Pedestal13 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the Pedestal13 module
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Pedestal13CalculateSetting(
        const Pedestal13InputData* pInput,
        VOID*                      pOEMIQData,
        Pedestal13OutputData*      pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HDR22CalculateSetting
    ///
    /// @brief  API function.  Call HDR22 module in common lib for calculation
    ///
    /// @param  pInput     Point to the Input data to the HDR22 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Point to the Calculation output from the BPSHDR22 module
    ///
    /// @return Return CamxResult. For error code: CamxResultEUnsupported, Node should
    ///         disable BPSHDR22 operation.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult HDR22CalculateSetting(
        const HDR22InputData* pInput,
        VOID*                 pOEMIQData,
        HDR22OutputData*      pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HDR23CalculateSetting
    ///
    /// @brief  API function.  Call HDR23 module in common lib for calculation
    ///
    /// @param  pInput     Point to the Input data to the HDR23 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Point to the Calculation output from the HDR23 module
    ///
    /// @return Return CamxResult. For error code: CamxResultEUnsupported, Node should
    ///         disable HDR23 operation.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult HDR23CalculateSetting(
        const HDR23InputData* pInput,
        VOID*                 pOEMIQData,
        HDR23OutputData*      pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HDR30CalculateSetting
    ///
    /// @brief  API function.  Call HDR22 module in common lib for calculation
    ///
    /// @param  pInput     Point to the Input data to the HDR22 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Point to the Calculation output from the BPSHDR22 module
    ///
    /// @return Return CamxResult. For error code: CamxResultEUnsupported, Node should
    ///         disable BPSHDR22 operation.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult HDR30CalculateSetting(
        const HDR30InputData* pInput,
        VOID*                 pOEMIQData,
        HDR30OutputData*      pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BPSLinearization34CalculateSetting
    ///
    /// @brief  API function.  Call Linearization34 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the Linearization34 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the Linearization34 module
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult BPSLinearization34CalculateSetting(
        const Linearization34IQInput* pInput,
        VOID*                         pOEMIQData,
        Linearization34OutputData*    pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ABF40CalculateSetting
    ///
    /// @brief  API function.  Call ABF40 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the input data to the ABF40 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the calculated output from the ABF40 module
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult ABF40CalculateSetting(
        ABF40InputData*  pInput,
        VOID*            pOEMIQData,
        ABF40OutputData* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BPSBPCPDPC20CalculateSetting
    ///
    /// @brief  API function. Call BPCPDPC20 module in common lib for calculation
    ///
    /// @param  pInput      Pointer to the Input data to the BPCPDPC module for calculation
    /// @param  pOEMIQData  Pointer to the OEM Input IQ Setting
    /// @param  pOutput     Pointer to the Calculation output from the BPCPDPC20 module
    /// @param  pixelFormat Pixel format from the sensor
    ///
    /// @return Return CamxResult. For error code: CamxResultEUnsupported, Node should
    ///         disable BPCPDPC operation.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult BPSBPCPDPC20CalculateSetting(
        PDPC20IQInput*          pInput,
        VOID*                   pOEMIQData,
        BPSBPCPDPC20OutputData* pOutput,
        PixelFormat             pixelFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BPSPDPC30CalculateSetting
    ///
    /// @brief  API function. Call BPCPDPC30 module in common lib for calculation
    ///
    /// @param  pInput      Pointer to the Input data to the BPCPDPC module for calculation
    /// @param  pOEMIQData  Pointer to the OEM Input IQ Setting
    /// @param  pOutput     Pointer to the Calculation output from the BPCPDPC20 module
    /// @param  pixelFormat Pixel format from the sensor
    ///
    /// @return Return CamxResult. For error code: CamxResultEUnsupported, Node should
    ///         disable BPCPDPC operation.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult BPSPDPC30CalculateSetting(
        PDPC30IQInput*          pInput,
        VOID*                   pOEMIQData,
        BPSPDPC30OutputData*    pOutput,
        PixelFormat             pixelFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEPDPC30CalculateSetting
    ///
    /// @brief  API function. Call BPCPDPC30 module in common lib for calculation
    ///
    /// @param  pInput      Pointer to the Input data to the BPCPDPC module for calculation
    /// @param  pOEMIQData  Pointer to the OEM Input IQ Setting
    /// @param  pOutput     Pointer to the Calculation output from the BPCPDPC20 module
    ///
    /// @return Return CamxResult. For error code: CamxResultEUnsupported, Node should
    ///         disable BPCPDPC operation.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IFEPDPC30CalculateSetting(
        const PDPC30IQInput*    pInput,
        VOID*                   pOEMIQData,
        IFEPDPC30OutputData*    pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPEASF30CalculateSetting
    ///
    /// @brief  API function.  Call ASF30 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the ASF30 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the Linearization34 module
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IPEASF30CalculateSetting(
        const ASF30InputData* pInput,
        VOID*                 pOEMIQData,
        ASF30OutputData*      pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TDL10CalculateSetting
    ///
    /// @brief  API function. Call TDL10 module in common lib for calculation
    ///         For error code: CamxResultEUnsupported, Node should disable TDL10 operation.
    ///
    /// @param  pInput     Pointer to the Input data to the TDL10 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the TDL10 module
    ///
    /// @return Return  CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult TDL10CalculateSetting(
        const TDL10InputData* pInput,
        VOID*                 pOEMIQData,
        TDL10OutputData*      pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPECV12CalculateSetting
    ///
    /// @brief  API function. Call CV12 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the CV12 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the CV12 module
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IPECV12CalculateSetting(
        const CV12InputData* pInput,
        VOID*                pOEMIQData,
        CV12OutputData*      pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPEGamma15CalculateSetting
    ///
    /// @brief  API function. Call Gamma15 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the Gamma15 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the Gamma15 module
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IPEGamma15CalculateSetting(
        const Gamma15InputData* pInput,
        VOID*                   pOEMIQData,
        Gamma15OutputData*      pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPECAC22CalculateSetting
    ///
    /// @brief  API function.  Call CAC22 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the CAC22 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the CAC22 module
    ///
    /// @return Return CamxResult. For error code: CamxResultEUnsupported, Node should
    ///         disable CAC22 operation.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IPECAC22CalculateSetting(
        const CAC22InputData* pInput,
        VOID*                 pOEMIQData,
        CAC22OutputData*      pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPECAC23CalculateSetting
    ///
    /// @brief  API function.  Call CAC23 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the CAC23 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the CAC23 module
    ///
    /// @return Return CamxResult. For error code: CamxResultEUnsupported, Node should
    ///         disable CAC23 operation.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IPECAC23CalculateSetting(
        const CAC23InputData* pInput,
        VOID*                 pOEMIQData,
        CAC23OutputData*      pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPETF10CalculateSetting
    ///
    /// @brief  API function.  Call TF10 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the TF10 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    ///
    /// @return Return CamxResult. For error code: CamxResultEUnsupported, Node should
    ///         disable TF10 operation.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IPETF10CalculateSetting(
        TF10InputData* pInput,
        VOID*                pOEMIQData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPETF10GetInitializationData
    ///
    /// @brief  API function.  Call TF10 module in common lib to get Initialization Data
    ///
    /// @param  pData     Pointer to variable that holds initialization data from common library
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IPETF10GetInitializationData(
        struct TFNcLibOutputData* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPETF20CalculateSetting
    ///
    /// @brief  API function.  Call TF10 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the TF20 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    ///
    /// @return Return CamxResult. For error code: CamxResultEUnsupported, Node should
    ///         disable TF20 operation.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IPETF20CalculateSetting(
        TF20InputData* pInput,
        VOID*                pOEMIQData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPETF20GetInitializationData
    ///
    /// @brief  API function.  Call TF20 module in common lib to get Initialization Data
    ///
    /// @param  pData     Pointer to variable that holds initialization data from common library
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IPETF20GetInitializationData(
        struct TFNcLibOutputData* pData);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ICA10CalculateSetting
    ///
    /// @brief  API function. Call ICA module in common lib for calculation
    ///
    /// @param  pInput         Pointer to the Input data to the ICA module for calculation
    /// @param  pOutput        Pointer to the Calculation output from the ICA module
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult ICA10CalculateSetting(
        const ICAInputData*  pInput,
        ICAOutputData*       pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ICA20CalculateSetting
    ///
    /// @brief  API function. Call ICA20 module in common lib for calculation
    ///
    /// @param  pInput         Pointer to the Input data to the ICA module for calculation
    /// @param  pOutput        Pointer to the Calculation output from the ICA module
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult ICA20CalculateSetting(
        const ICAInputData*   pInput,
        ICAOutputData*      pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ICA30CalculateSetting
    ///
    /// @brief  API function. Call ICA30 module in common lib for calculation
    ///
    /// @param  pInput         Pointer to the Input data to the ICA module for calculation
    /// @param  pOutput        Pointer to the Calculation output from the ICA module
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult ICA30CalculateSetting(
        const ICAInputData*   pInput,
        ICAOutputData*      pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPEICAGetInitializationData
    ///
    /// @brief  API function.  Call ICA module in common lib to get Initialization Data
    ///
    /// @param  pData     Pointer to variable that holds initialization data from common library
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IPEICAGetInitializationData(
        struct ICANcLibOutputData* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPEANR10CalculateSetting
    ///
    /// @brief  API function. Call ANR10 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the ANR10 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IPEANR10CalculateSetting(
        ANR10InputData*       pInput,
        VOID*                 pOEMIQData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPEANR10GetInitializationData
    ///
    /// @brief  API function.  Call ANR10 module in common lib to get Initialization Data
    ///
    /// @param  pData     Pointer to variable that holds initialization data from common library
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IPEANR10GetInitializationData(
        struct ANRNcLibOutputData* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEDSX10GetInitializationData
    ///
    /// @brief  API function.  Call DSX10 module in common lib to get Initialization Data
    ///
    /// @param  pData     Pointer to variable that holds initialization data from common library
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IFEDSX10GetInitializationData(
        struct DSXNcLibOutputData* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPELTM13CalculateSetting
    ///
    /// @brief  API function. Call IPELTM13 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the IPELTM13 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the IPELTM13 module
    /// @param  pTMCInput  Pointer to the Calculation output from the TMC10 module
    ///
    /// @return Return CamxResult. For error code: CamxResultEUnsupported, Node should
    ///         disable IPELTM13 operation.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IPELTM13CalculateSetting(
        LTM13InputData*       pInput,
        VOID*                 pOEMIQData,
        LTM13OutputData*      pOutput,
        TMC10InputData*       pTMCInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPELTM14CalculateSetting
    ///
    /// @brief  API function. Call IPELTM14 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the IPELTM14 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the IPELTM14 module
    /// @param  pTMCInput  Pointer to the Calculation output from the TMC10 module
    ///
    /// @return Return CamxResult. For error code: CamxResultEUnsupported, Node should
    ///         disable IPELTM14 operation.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IPELTM14CalculateSetting(
        LTM14InputData*       pInput,
        VOID*                 pOEMIQData,
        LTM14OutputData*      pOutput,
        TMC10InputData*       pTMCInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPEUpscale12CalculateSetting
    ///
    /// @brief  API function.  Call Upscale12/ChromaUp12 modules in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the Upscale12/ChromaUp12 modules for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IPEUpscale12CalculateSetting(
        const Upscale12InputData* pInput,
        VOID*                     pOEMIQData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TMC11CalculateSetting
    ///
    /// @brief  API function. Call TMC11 module in common lib for calculation
    ///
    /// @param  pInput  Pointer to the Input data to the TMC11 module for calculation
    ///
    /// @return Return CamxResult. For error code: CamxResultEUnsupported, Node should disable TMC11 operation.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult TMC11CalculateSetting(
        TMC11InputData*  pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TMC12CalculateSetting
    ///
    /// @brief  API function. Call TMC12 module in common lib for calculation
    ///
    /// @param  pInput  Pointer to the Input data to the TMC12 module for calculation
    ///
    /// @return Return CamxResult. For error code: CamxResultEUnsupported, Node should disable TMC12 operation.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult TMC12CalculateSetting(
        TMC12InputData*  pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPEUpscale20CalculateSetting
    ///
    /// @brief  API function.  Call Upscale20/ChromaUp20 modules in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the Upscale20/ChromaUp20 modules for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the Upscale20/ChromaUp20 modules
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IPEUpscale20CalculateSetting(
        const Upscale20InputData* pInput,
        VOID*                     pOEMIQData,
        Upscale20OutputData*      pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPEGRA10CalculateSetting
    ///
    /// @brief  API function.  Call GRA10 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the GRA module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculated output from the GRA module
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult IPEGRA10CalculateSetting(
        const GRA10IQInput* pInput,
        VOID*               pOEMIQData,
        GRA10OutputData*    pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LENR10CalculateSetting
    ///
    /// @brief  API function.  Call LENR10 module in common lib for calculation
    ///
    /// @param  pInput     Pointer to the Input data to the LENR10 module for calculation
    /// @param  pOEMIQData Pointer to the OEM Input IQ Setting
    /// @param  pOutput    Pointer to the Calculation output from the HNR10 module
    ///
    /// @return Return  CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult LENR10CalculateSetting(
        const LENR10InputData*  pInput,
        VOID*                   pOEMIQData,
        LENR10OutputData*       pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CVP10CalculateSetting
    ///
    /// @brief  API function.  Call CVP10 module in common lib for calculation
    ///
    /// @param  pInput         Pointer to the Input data to the CVP10 module for calculation
    /// @param  pInputData     Pointer to the ISP input data
    /// @param  pOutput        Pointer to the Calculation output from the CVP10 modulez
    ///
    /// @return Return  CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult CVP10CalculateSetting(
        CVP10InputData*    pInput,
        ISPInputData*      pInputData,
        CVP10OutputData*   pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpICAGrid
    ///
    /// @brief  API function.  Dump ICA Grid
    ///
    /// @param  pFile      Pointer to the dump file
    /// @param  pGrid      Pointer to the grid to dump
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID DumpICAGrid(
        FILE* pFile,
        const NcLibIcaGrid* pGrid);

    // Data members
    static IQOperationTable s_interpolationTable;    ///< IQ Module Function Table
    ///< Store prev Tintless
    static UINT16 s_prevTintlessOutput_l[MAX_NUM_OF_CAMERA][MAX_SENSOR_ASPECT_RATIO][TotalChannels]
                                                     [ROLLOFF_MESH_PT_V_V34][ROLLOFF_MESH_PT_H_V34];
    static UINT16 s_prevTintlessOutput_r[MAX_NUM_OF_CAMERA][MAX_SENSOR_ASPECT_RATIO][TotalChannels]
                                                     [ROLLOFF_MESH_PT_V_V34][ROLLOFF_MESH_PT_H_V34];
    static BOOL   s_prevTintlessTableValid[MAX_NUM_OF_CAMERA][MAX_SENSOR_ASPECT_RATIO];
    ///< Store previous ALSC tables
    static UINT16 s_prevALSCGainOutput[MAX_NUM_OF_CAMERA][MAX_SENSOR_ASPECT_RATIO]
                                                   [LSC_MESH_PT_V_V40][LSC_MESH_PT_H_V40];
    static UINT16 s_prevALSCMeanOutput[MAX_NUM_OF_CAMERA][MAX_SENSOR_ASPECT_RATIO]
                                                   [LSC_MESH_PT_V_V40][LSC_MESH_PT_H_V40];
    static BOOL   s_prevALSCTableValid[MAX_NUM_OF_CAMERA][MAX_SENSOR_ASPECT_RATIO];
};

CAMX_NAMESPACE_END

#endif // CAMXIQINTERFACE_H
