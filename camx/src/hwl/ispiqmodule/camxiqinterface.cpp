////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxiqinterface.cpp
/// @brief CamX IQ interface implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Camx Headers
#include "camxnode.h"
#include "camxiqinterface.h"
#include "camxtuningdatamanager.h"
#include "camxutils.h"

/// IQ setting Headers
#include "anr10interpolation.h"
#include "anr10setting.h"
#include "asf30interpolation.h"
#include "asf30setting.h"
#include "bls12interpolation.h"
#include "bls12setting.h"
#include "abf40interpolation.h"
#include "abf40setting.h"
#include "bc10interpolation.h"
#include "bc10setting.h"
#include "bpsgic30interpolation.h"
#include "bpsgic30setting.h"
#include "hdr22interpolation.h"
#include "hdr22setting.h"
#include "hdr23interpolation.h"
#include "hdr23setting.h"
#include "hdr30interpolation.h"
#include "hdr30setting.h"
#include "linearization34interpolation.h"
#include "linearization34setting.h"
#include "bpspdpc20interpolation.h"
#include "bpspdpc20setting.h"
#include "pdpc30interpolation.h"
#include "pdpc30setting.h"
#include "camxhal3module.h"
#include "cac22interpolation.h"
#include "cac22setting.h"
#include "cac23interpolation.h"
#include "cac23setting.h"
#include "cc13interpolation.h"
#include "cc13setting.h"
#include "cst12setting.h"
#include "cv12interpolation.h"
#include "cv12setting.h"
#include "demosaic36interpolation.h"
#include "demosaic37interpolation.h"
#include "demosaic36setting.h"
#include "demosaic37setting.h"
#include "demux13setting.h"
#include "gamma15interpolation.h"
#include "gamma15setting.h"
#include "gamma16interpolation.h"
#include "gamma16setting.h"
#include "gra10interpolation.h"
#include "gra10setting.h"
#include "gtm10interpolation.h"
#include "gtm10setting.h"
#include "hnr10interpolation.h"
#include "hnr10setting.h"
#include "lenr10interpolation.h"
#include "lenr10setting.h"
#include "ica10interpolation.h"
#include "ica20interpolation.h"
#include "ica30interpolation.h"
#include "icasetting.h"
#include "ifeabf34interpolation.h"
#include "ifeabf34setting.h"
#include "ifebpcbcc50interpolation.h"
#include "ifebpcbcc50setting.h"
#include "ifecc12interpolation.h"
#include "ifecc12setting.h"
#include "ifehdr20interpolation.h"
#include "ifehdr20setting.h"
#include "ifelinearization33interpolation.h"
#include "ifelinearization33setting.h"
#include "ifepdpc11interpolation.h"
#include "ifepdpc11setting.h"
#include "iqcommondefs.h"
#include "ipecs20interpolation.h"
#include "ipecs20setting.h"
#include "ipe2dlut10interpolation.h"
#include "ipe2dlut10setting.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"
#include "lsc34interpolation.h"
#include "lsc34setting.h"
#include "lsc40interpolation.h"
#include "lsc40setting.h"
#include "ltm13interpolation.h"
#include "ltm13setting.h"
#include "ltm14interpolation.h"
#include "ltm14setting.h"
#include "pedestal13interpolation.h"
#include "pedestal13setting.h"
#include "sce11interpolation.h"
#include "sce11setting.h"
#include "tf10interpolation.h"
#include "tf10setting.h"
#include "tf20interpolation.h"
#include "tf20setting.h"
#include "tintless20interpolation.h"
#include "tmc10interpolation.h"
#include "tmc11interpolation.h"
#include "tmc12interpolation.h"
#include "upscale12setting.h"
#include "upscale20interpolation.h"
#include "upscale20setting.h"
#include "wb12setting.h"
#include "wb13setting.h"
#include "dsx10interpolation.h"
#include "cvp10interpolation.h"
#include "cvp10setting.h"
#include "dsx10setting.h"
#include "Process_ICA.h"
#include "chiipedefs.h"

#if OEM1IQ
#include "oem1gamma15setting.h"
#include "oem1gamma16setting.h"
#include "oem1gtm10setting.h"
#include "oem1iqsettingutil.h"
#include "oem1asf30setting.h"
#include "oem1ltm13setting.h"
#include "oem1ltm14setting.h"
#endif // OEM1IQ

CAMX_NAMESPACE_BEGIN

// Install IQ Module Function Table
#if OEM1IQ
#include "oem1iqfunctiontable.h"
#else
#include "camxiqfunctiontable.h"
#endif // OEM1IQ

BOOL IQInterface::s_prevTintlessTableValid[MAX_NUM_OF_CAMERA][MAX_SENSOR_ASPECT_RATIO];
UINT16 IQInterface::s_prevTintlessOutput_l[MAX_NUM_OF_CAMERA][MAX_SENSOR_ASPECT_RATIO]
                                          [TotalChannels][ROLLOFF_MESH_PT_V_V34][ROLLOFF_MESH_PT_H_V34];
UINT16 IQInterface::s_prevTintlessOutput_r[MAX_NUM_OF_CAMERA][MAX_SENSOR_ASPECT_RATIO][TotalChannels]
                                          [ROLLOFF_MESH_PT_V_V34][ROLLOFF_MESH_PT_H_V34];

BOOL IQInterface::s_prevALSCTableValid[MAX_NUM_OF_CAMERA][MAX_SENSOR_ASPECT_RATIO];
UINT16 IQInterface::s_prevALSCGainOutput[MAX_NUM_OF_CAMERA][MAX_SENSOR_ASPECT_RATIO][LSC_MESH_PT_V_V40][LSC_MESH_PT_H_V40];
UINT16 IQInterface::s_prevALSCMeanOutput[MAX_NUM_OF_CAMERA][MAX_SENSOR_ASPECT_RATIO][LSC_MESH_PT_V_V40][LSC_MESH_PT_H_V40];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IQSettingModuleInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IQSettingModuleInitialize(
    IQLibInitialData* pLibData)
{
    CamxResult    result    = CamxResultSuccess;
    BOOL          isSucceed = FALSE;
    IQLibraryData libData;

    if (NULL == pLibData->pLibData)
    {
        isSucceed = IQInterface::s_interpolationTable.IQModuleInitialize(&libData);

        if (FALSE == isSucceed)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "IQ Setting Library Initialization Failed");
            result = CamxResultEFailed;
        }
        else
        {
            pLibData->pLibData = libData.pCustomticData;
        }

        pLibData->isSucceed = isSucceed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IQSettingModuleUninitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IQSettingModuleUninitialize(
    IQLibInitialData* pData)
{
    CamxResult result    = CamxResultSuccess;
    BOOL       isSucceed = FALSE;
    IQLibraryData libData;

    if (NULL != pData->pLibData)
    {
        libData.pCustomticData = pData->pLibData;

        isSucceed = IQInterface::s_interpolationTable.IQModuleUninitialize(&libData);

        if (FALSE == isSucceed)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "IQ Setting Library Deinitialization Failed");
            result = CamxResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IQSetupTriggerData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IQInterface::IQSetupTriggerData(
    ISPInputData*           pInputData,
    Node*                   pNode,
    BOOL                    isRealTime,
    ISPIQTuningDataBuffer*  pIQOEMTriggerData)
{
    UINT32     metaTag          = 0;
    VOID*      pOEMData[1]      = { 0 };
    UINT64     pOEMDataOffset[] = { 0 };
    CamxResult result           = CamxResultSuccess;

    CAMX_ASSERT(NULL != pInputData);

    FLOAT shortSensitivity = pInputData->pAECUpdateData->exposureInfo[ExposureIndexShort].sensitivity;
    FLOAT scalingFactor = 1.0f;

    if (0 != pInputData->pAECUpdateData->exposureInfo[ExposureIndexShort].exposureTime)
    {
        pInputData->triggerData.AECexposureTime     =
            static_cast<FLOAT>(pInputData->pAECUpdateData->exposureInfo[ExposureIndexLong].exposureTime) /
            static_cast<FLOAT>(pInputData->pAECUpdateData->exposureInfo[ExposureIndexShort].exposureTime);
    }
    else
    {
        pInputData->triggerData.AECexposureTime     = 1.0f;
    }

    if (FALSE == IQSettingUtils::FEqual(pInputData->pAECUpdateData->exposureInfo[ExposureIndexShort].linearGain, 0.0f))
    {
        pInputData->triggerData.AECexposureGainRatio = pInputData->pAECUpdateData->exposureInfo[ExposureIndexLong].linearGain
            / pInputData->pAECUpdateData->exposureInfo[ExposureIndexShort].linearGain;
    }
    else
    {
        pInputData->triggerData.AECexposureGainRatio = 1.0f;
    }

    if (FALSE == IQSettingUtils::FEqual(pInputData->pAECUpdateData->exposureInfo[ExposureIndexShort].sensitivity, 0.0f))
    {
        pInputData->triggerData.AECSensitivity      = pInputData->pAECUpdateData->exposureInfo[ExposureIndexLong].sensitivity
            / pInputData->pAECUpdateData->exposureInfo[ExposureIndexShort].sensitivity;
    }
    else
    {
        pInputData->triggerData.AECSensitivity     = 1.0f;
    }

    if (TRUE == pInputData->sensorData.isIHDR)
    {
        pInputData->triggerData.AECGain             = pInputData->pAECUpdateData->exposureInfo[ExposureIndexLong].linearGain;
    }
    else
    {
        pInputData->triggerData.AECGain             = pInputData->pAECUpdateData->exposureInfo[ExposureIndexSafe].linearGain;
    }
    pInputData->triggerData.AECShortGain
        = pInputData->pAECUpdateData->exposureInfo[ExposureIndexShort].linearGain;
    pInputData->triggerData.AECLuxIndex                 = pInputData->pAECUpdateData->luxIndex;
    pInputData->triggerData.AECPrevLuxIndex             = pInputData->pAECUpdateData->prevLuxIndex;
    pInputData->triggerData.AWBColorTemperature         = static_cast<FLOAT>(pInputData->pAWBUpdateData->colorTemperature);
    pInputData->triggerData.lensZoom                    = pInputData->lensZoom;
    pInputData->triggerData.postScaleRatio              = pInputData->postScaleRatio;
    pInputData->triggerData.preScaleRatio               = pInputData->preScaleRatio;
    pInputData->triggerData.totalScaleRatio             = pInputData->preScaleRatio * pInputData->postScaleRatio;
    pInputData->triggerData.enableAECYHistStretching    = pInputData->pAECUpdateData->stretchControl.enable;
    pInputData->triggerData.AECYHistStretchClampOffset  = pInputData->pAECUpdateData->stretchControl.clamp;
    pInputData->triggerData.AECYHistStretchScaleFactor  = pInputData->pAECUpdateData->stretchControl.scaling;

    ISPHALTagsData*     pHALTagsData             = pInputData->pHALTagsData;

    if ((NULL                               != pHALTagsData)                      &&
        (ColorCorrectionModeTransformMatrix == pHALTagsData->colorCorrectionMode) &&
        (((ControlAWBModeOff                == pHALTagsData->controlAWBMode)      &&
        (ControlModeAuto                    == pHALTagsData->controlMode))        ||
        (ControlModeOff                     == pHALTagsData->controlMode)))
    {
        pInputData->triggerData.AWBleftGGainWB = pHALTagsData->colorCorrectionGains.greenEven;
        pInputData->triggerData.AWBleftBGainWB = pHALTagsData->colorCorrectionGains.blue;
        pInputData->triggerData.AWBleftRGainWB = pHALTagsData->colorCorrectionGains.red;
    }
    else
    {
        pInputData->triggerData.AWBleftGGainWB  = pInputData->pAWBUpdateData->AWBGains.gGain;
        pInputData->triggerData.AWBleftBGainWB  = pInputData->pAWBUpdateData->AWBGains.bGain;
        pInputData->triggerData.AWBleftRGainWB  = pInputData->pAWBUpdateData->AWBGains.rGain;
        pInputData->triggerData.predictiveGain  = Utils::MaxFLOAT(pInputData->pAECUpdateData->predictiveGain, 1.0f);
    }

    if (NULL != pInputData->pAFUpdateData)
    {
        pInputData->triggerData.lensPosition =
            static_cast<FLOAT>(pInputData->pAFUpdateData->moveLensOutput.targetLensPosition);
    }

    if (pNode->Type() == BPS || pNode->Type() == IPE)
    {
        CAMX_LOG_INFO(CamxLogGroupPProc,
                      "NodeType = %d, preScaleRatio = %f, postScaleRatio = %f, totalScaleRatio = %f",
                      pNode->Type(),
                      pInputData->triggerData.preScaleRatio,
                      pInputData->triggerData.postScaleRatio,
                      pInputData->triggerData.totalScaleRatio);
    }

    if (NULL != pInputData->pCalculatedMetadata)
    {
        pInputData->triggerData.blackLevelOffset = pInputData->pCalculatedMetadata->BLSblackLevelOffset;
    }

    if (FALSE == IQSettingUtils::FEqual(shortSensitivity, 0.0f))
    {
        if (TRUE == pInputData->sensorData.isIHDR)
        {
            pInputData->triggerData.DRCGain = pInputData->pAECUpdateData->compenADRCGain;
        }
        else
        {
            pInputData->triggerData.DRCGain = pInputData->pAECUpdateData->exposureInfo[ExposureIndexSafe].sensitivity /
                pInputData->pAECUpdateData->exposureInfo[ExposureIndexShort].sensitivity;
        }

    }
    else
    {
        pInputData->triggerData.DRCGain = 0.0f;
    }

    if (FALSE == IQSettingUtils::FEqual(pInputData->pAECUpdateData->exposureInfo[ExposureIndexSafe].sensitivity, 0.0f))
    {
        if (TRUE == pInputData->sensorData.isIHDR)
        {
            pInputData->triggerData.DRCGainDark = 1.0f;
        }
        else
        {
            pInputData->triggerData.DRCGainDark = pInputData->pAECUpdateData->exposureInfo[ExposureIndexLong].sensitivity /
                pInputData->pAECUpdateData->exposureInfo[ExposureIndexSafe].sensitivity;
        }
    }
    else
    {
        pInputData->triggerData.DRCGainDark = 0.0f;
    }

    pInputData->triggerData.sensorImageWidth = pInputData->sensorData.CAMIFCrop.lastPixel -
        pInputData->sensorData.CAMIFCrop.firstPixel + 1;
    pInputData->triggerData.sensorImageHeight = pInputData->sensorData.CAMIFCrop.lastLine -
        pInputData->sensorData.CAMIFCrop.firstLine + 1;

    if (NULL != pInputData->pStripeConfig)
    {
        pInputData->triggerData.CAMIFWidth        =
            pInputData->pStripeConfig->CAMIFCrop.lastPixel - pInputData->pStripeConfig->CAMIFCrop.firstPixel + 1;
        pInputData->triggerData.CAMIFHeight       =
            pInputData->pStripeConfig->CAMIFCrop.lastLine - pInputData->pStripeConfig->CAMIFCrop.firstLine + 1;
        if (NULL != pInputData->pStripeConfig->statsDataForISP.pParsedBHISTStats)
        {
            pInputData->triggerData.pParsedBHISTStats = pInputData->pStripeConfig->statsDataForISP.pParsedBHISTStats;
            pInputData->triggerData.maxPipelineDelay  = pInputData->maximumPipelineDelay;
        }
    }
    else
    {
        pInputData->triggerData.CAMIFWidth        = pInputData->sensorData.CAMIFCrop.lastPixel;
        pInputData->triggerData.CAMIFHeight       = pInputData->sensorData.CAMIFCrop.lastLine;
    }

    pInputData->triggerData.LEDSensitivity      = pInputData->pAECUpdateData->LEDInfluenceRatio;
    pInputData->triggerData.LEDFirstEntryRatio  = pInputData->pAECUpdateData->LEDFirstEntryRatio;
    pInputData->triggerData.numberOfLED         = pInputData->numberOfLED;

    result = IQInterface::GetPixelFormat(&pInputData->sensorData.format,
                                         &pInputData->triggerData.bayerPattern);

    scalingFactor                               =
        (pInputData->sensorData.sensorScalingFactor) * (pInputData->sensorData.sensorBinningFactor);
    pInputData->triggerData.sensorOffsetX       =
        static_cast<UINT32>((pInputData->sensorData.CAMIFCrop.firstPixel) * scalingFactor);
    pInputData->triggerData.sensorOffsetY       =
        static_cast<UINT32>((pInputData->sensorData.CAMIFCrop.firstLine) * scalingFactor);

    for (UINT16 count = 0; count < g_customaticTriggerNumber; count++)
    {
        result =
            VendorTagManager::QueryVendorTagLocation(g_pOEM1TriggerTagList[count].tagSessionName,
                                                     g_pOEM1TriggerTagList[count].tagName,
                                                     &metaTag);
        UINT OEMProperty[] = { 0 };

        if (FALSE == isRealTime)
        {
            OEMProperty[0] = metaTag | InputMetadataSectionMask;
        }
        else
        {
            OEMProperty[0] = metaTag;
        }

        CAMX_ASSERT(CamxResultSuccess == result);

        pNode->GetDataList(OEMProperty, pOEMData, pOEMDataOffset, 1);

        pInputData->triggerData.pOEMTrigger[count] = pOEMData[0];
    }
    if (0 < g_customaticTriggerNumber)
    {
        CAMX_ASSERT(NULL != IQInterface::s_interpolationTable.IQFillOEMTuningTriggerData);
        // Fill OEM custom trigger data
        IQInterface::s_interpolationTable.IQFillOEMTuningTriggerData(&pInputData->triggerData, pIQOEMTriggerData);
    }

    pInputData->triggerData.pLibInitialData = pInputData->pLibInitialData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::GetADRCParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IQInterface::GetADRCParams(
    const ISPInputData* pInputData,
    BOOL*               pAdrcEnabled,
    FLOAT*              pGtmPercentage,
    const SWTMCVersion  tmcVersion)
{
    TuningDataManager*   pTuningManager = NULL;

    *pAdrcEnabled        = FALSE;
    *pGtmPercentage      = 0.0f;
    pTuningManager       = pInputData->pTuningDataManager;

    if (NULL != pInputData->pTuningData)
    {
        switch (tmcVersion)
        {
            case SWTMCVersion::TMC10:
            {
                tmc_1_0_0::chromatix_tmc10Type* ptmcChromatix = NULL;

                if (TRUE == pTuningManager->IsValidChromatix())
                {
                    ptmcChromatix = pTuningManager->GetChromatix()->GetModule_tmc10_sw(
                        reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                        pInputData->pTuningData->noOfSelectionParameter);
                }

                if (NULL != ptmcChromatix)
                {
                    if ((TRUE == ptmcChromatix->enable_section.adrc_isp_enable) &&
                        (TRUE == ptmcChromatix->chromatix_tmc10_reserve.use_gtm) &&
                        (FALSE == pInputData->sensorData.isIHDR))
                    {
                        *pAdrcEnabled = TRUE;
                        *pGtmPercentage = ptmcChromatix->chromatix_tmc10_core.mod_tmc10_aec_data->tmc10_rgn_data.gtm_percentage;
                    }
                }
                break;
            }

            case SWTMCVersion::TMC11:
            {
                tmc_1_1_0::chromatix_tmc11Type* ptmcChromatix = NULL;
                if (TRUE == pTuningManager->IsValidChromatix())
                {
                    ptmcChromatix = pTuningManager->GetChromatix()->GetModule_tmc11_sw(
                        reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                        pInputData->pTuningData->noOfSelectionParameter);
                }

                if (NULL != ptmcChromatix)
                {
                    if ((TRUE == ptmcChromatix->enable_section.tmc_enable) &&
                        (TRUE == ptmcChromatix->chromatix_tmc11_reserve.use_gtm) &&
                        (FALSE == pInputData->sensorData.isIHDR))
                    {
                        *pAdrcEnabled = TRUE;
                        *pGtmPercentage = ptmcChromatix->chromatix_tmc11_core.mod_tmc11_drc_gain_data->
                            drc_gain_data.mod_tmc11_hdr_aec_data->hdr_aec_data.mod_tmc11_aec_data->
                            tmc11_rgn_data.gtm_percentage;
                    }
                }
                break;
            }

            case SWTMCVersion::TMC12:
            {
                tmc_1_2_0::chromatix_tmc12Type* ptmcChromatix = NULL;
                if (TRUE == pTuningManager->IsValidChromatix())
                {
                    ptmcChromatix = pTuningManager->GetChromatix()->GetModule_tmc12_sw(
                        reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                        pInputData->pTuningData->noOfSelectionParameter);
                }

                if (NULL != ptmcChromatix)
                {
                    if ((TRUE == ptmcChromatix->enable_section.tmc_enable) &&
                        (TRUE == ptmcChromatix->chromatix_tmc12_reserve.use_gtm) &&
                        (FALSE == pInputData->sensorData.isIHDR))
                    {
                        *pAdrcEnabled = TRUE;
                        *pGtmPercentage = ptmcChromatix->chromatix_tmc12_core.mod_tmc12_drc_gain_data->
                            drc_gain_data.mod_tmc12_hdr_aec_data->hdr_aec_data.mod_tmc12_aec_data->
                            tmc12_rgn_data.gtm_percentage;
                    }
                }
                break;
            }

            default:
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Unsupported TMC Version = %u", static_cast<UINT>(tmcVersion));
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid tuning data %p", pInputData->pTuningData);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::GetADRCData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IQInterface::GetADRCData(
    TMC10InputData*   pTMCInput)
{
    BOOL                           commonIQResult = FALSE;
    tmc_1_0_0::tmc10_rgn_dataType  interpolationDataTMC;

    // Call the Interpolation Calculation
    commonIQResult = IQInterface::s_interpolationTable.TMC10Interpolation(pTMCInput, &interpolationDataTMC);

    return commonIQResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQInterface::GetADRCData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IQInterface::GetADRCData(
    TMC11InputData*   pTMCInput)
{
    // This might be a redundant function, can be removed: only need for the TMC10 version
    BOOL                           commonIQResult = FALSE;
    tmc_1_1_0::tmc11_rgn_dataType  interpolationDataTMC;

    // Call the Interpolation Calculation
    commonIQResult = IQInterface::s_interpolationTable.TMC11Interpolation(pTMCInput, &interpolationDataTMC);

    return commonIQResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQInterface::GetADRCData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IQInterface::GetADRCData(
    TMC12InputData*   pTMCInput)
{
    // This might be a redundant function, can be removed: only need for the TMC10 version
    BOOL                           commonIQResult = FALSE;
    tmc_1_2_0::tmc12_rgn_dataType  interpolationDataTMC;

    // Call the Interpolation Calculation
    commonIQResult = IQInterface::s_interpolationTable.TMC12Interpolation(pTMCInput, &interpolationDataTMC);

    return commonIQResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQInterface::UpdateAECGain()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IQInterface::UpdateAECGain(
    ISPIQModuleType mType,
    ISPInputData*   pInputData,
    FLOAT           gtmPercentage)
{
    FLOAT gain     = pInputData->triggerData.AECShortGain;
    FLOAT drc_gain = pInputData->triggerData.DRCGain;

    // Gain before GTM module is triggered by short gain
    pInputData->triggerData.AECGain = gain;

    if (mType == ISPIQModuleType::IFEGTM   ||
        mType == ISPIQModuleType::IFEGamma ||
        mType == ISPIQModuleType::IFECST   ||
        mType == ISPIQModuleType::BPSGTM   ||
        mType == ISPIQModuleType::BPSGamma ||
        mType == ISPIQModuleType::BPSCST   ||
        mType == ISPIQModuleType::BPSHNR   ||
        mType == ISPIQModuleType::IPECAC   ||
        mType == ISPIQModuleType::IPEICA   ||
        mType == ISPIQModuleType::IPEANR   ||
        mType == ISPIQModuleType::IPETF    ||
        mType == ISPIQModuleType::IPECST   ||
        mType == ISPIQModuleType::IPELTM   ||
        mType == ISPIQModuleType::IPEHNR   ||
        mType == ISPIQModuleType::IPELENR)
    {
        // Gain betweem GTM & LTM ( includes ) will be triggered by shortGain*power(DRCGain,gtm_perc)
        pInputData->triggerData.AECGain = gain * static_cast<FLOAT>(CamX::Utils::Power(drc_gain, gtmPercentage));
    }

    if (mType == ISPIQModuleType::IPEColorCorrection   ||
        mType == ISPIQModuleType::IPEGamma             ||
        mType == ISPIQModuleType::IPE2DLUT             ||
        mType == ISPIQModuleType::IPEChromaEnhancement ||
        mType == ISPIQModuleType::IPEChromaSuppression ||
        mType == ISPIQModuleType::IPESCE               ||
        mType == ISPIQModuleType::IPEASF               ||
        mType == ISPIQModuleType::IPEUpscaler          ||
        mType == ISPIQModuleType::IPEGrainAdder)
    {
        // Gain post LTM will be triggered by shortGain*DRCGain
        pInputData->triggerData.AECGain = gain * drc_gain;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IQSetHardwareVersion
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IQInterface::IQSetHardwareVersion(
    UINT32 titanVersion,
    UINT32 hardwareVersion)
{
    BOOL  commonIQResult    = FALSE;
    commonIQResult          = IQInterface::s_interpolationTable.IQSetHardwareVersion(titanVersion, hardwareVersion);

    return commonIQResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IFEGetSensorMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IFEGetSensorMode(
    const PixelFormat*  pPixelFormat,
    SensorType*         pSensorType)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pPixelFormat) && (NULL != pSensorType))
    {
        switch (*pPixelFormat)
        {
            case PixelFormat::BayerRGGB:
            case PixelFormat::BayerBGGR:
            case PixelFormat::BayerGBRG:
            case PixelFormat::BayerGRBG:
                *pSensorType = SensorType::BAYER_RGGB;
                break;

            case PixelFormat::YUVFormatUYVY:
            case PixelFormat::YUVFormatYUYV:
            case PixelFormat::YUVFormatY:
                *pSensorType = SensorType::BAYER_RCCB;
                break;

            case PixelFormat::MetaStatsHDR:
                *pSensorType = SensorType::BAYER_HDR;
                break;

            case PixelFormat::MetaStatsPDAF:
                *pSensorType = SensorType::BAYER_PDAF;
                break;

            default:
                CAMX_ASSERT_ALWAYS_MESSAGE("No Corresponding Format. Format = %d", *pPixelFormat);
                result = CamxResultEUnsupported;
                break;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Null data ");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::GetPixelFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::GetPixelFormat(
    const PixelFormat*  pPixelFormat,
    UINT8*              pBayerPattern)
{
    CamxResult result = CamxResultSuccess;

    switch (*pPixelFormat)
    {
        case PixelFormat::BayerRGGB:
            *pBayerPattern = RGGB_PATTERN;
            break;

        case PixelFormat::BayerBGGR:
            *pBayerPattern = BGGR_PATTERN;
            break;

        case PixelFormat::BayerGBRG:
            *pBayerPattern = GBRG_PATTERN;
            break;

        case PixelFormat::BayerGRBG:
            *pBayerPattern = GRBG_PATTERN;
            break;

        case PixelFormat::YUVFormatUYVY:
            *pBayerPattern = CBYCRY422_PATTERN;
            break;

        case PixelFormat::YUVFormatYUYV:
            *pBayerPattern = YCBYCR422_PATTERN;
            break;

        case PixelFormat::YUVFormatY:
            *pBayerPattern = Y_PATTERN;
            break;

        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("No Corresponding Format. Format = %d", *pPixelFormat);
            result = CamxResultEUnsupported;
            break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQInterface::DSX10CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::DSX10CalculateSetting(
    DSX10InputData*  pInput,
    VOID*            pOEMIQData,
    DSX10OutputData* pOutput)
{
    CamxResult                              result             = CamxResultSuccess;
    BOOL                                    commonIQResult     = FALSE;
    dsx_1_0_0::dsx10_rgn_dataType*          pInterpolationData = NULL;
    ISPHWSetting*                           pHWSetting         = NULL;
    dsx_1_0_0::chromatix_dsx10_reserveType* pReserveType       = NULL;
    DSX10UnpackedField                      unpackedRegData;

    if ((NULL != pInput) && (NULL != pOutput))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        Utils::Memset(&unpackedRegData, 0, sizeof(DSX10UnpackedField));

        if (NULL == pOEMIQData)
        {
            pInterpolationData = static_cast<dsx_1_0_0::dsx10_rgn_dataType*>(pInput->pInterpolationData);
            // Call the Interpolation Calculation
            commonIQResult     = IQInterface::s_interpolationTable.DSX10Interpolation(pInput, pInterpolationData);
            pReserveType       = &(pInput->pChromatix->chromatix_dsx10_reserve);
        }

        if (TRUE == commonIQResult)
        {
            commonIQResult = IQInterface::s_interpolationTable.DSX10CalculateHWSetting(
                pInput,
                pInterpolationData,
                pReserveType,
                &pInput->pDS4to1Chromatix->chromatix_ds4to1v11_reserve.mod_ds4to1v11_pass_reserve_data[1].pass_data,
                static_cast<VOID*>(&unpackedRegData));

            if (TRUE == commonIQResult)
            {
                pOutput->lumaStartingLocation   = unpackedRegData.dsxData.luma_start_location_x;
                pOutput->lumaInputImageWidth    = unpackedRegData.dsxData.luma_input_width;
                pOutput->lumaOutputImageWidth   = unpackedRegData.dsxData.luma_out_width;
                pOutput->lumaScaleRatio         = unpackedRegData.dsxData.luma_scale_ratio_x;
                pOutput->chromaStartingLocation = unpackedRegData.dsxData.chroma_start_location_x;
                pOutput->chromaInputImageWidth  = unpackedRegData.dsxData.chroma_input_width;
                pOutput->chromaOutputImageWidth = unpackedRegData.dsxData.chroma_out_width;
                pOutput->chromaScaleRatio       = unpackedRegData.dsxData.chroma_scale_ratio_x;
                if (FALSE == pInput->isPrepareStripeInputContext)
                {
                    result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegData),
                                                               static_cast<VOID*>(pOutput));
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupISP, "DSX10 Pack register failed ", result);
                    }
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupISP, "DSX10 Calculate HW setting failed.");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupISP, "DSX10 interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Input data is pInput %p pOutput %p ", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::Demux13CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::Demux13CalculateSetting(
    Demux13InputData*  pInput,
    VOID*              pOEMIQData,
    Demux13OutputData* pOutput,
    PixelFormat        pixelFormat)
{
    CamxResult                                                result         = CamxResultSuccess;
    BOOL                                                      commonIQResult = FALSE;
    ISPHWSetting*                                             pHWSetting     = NULL;
    demux_1_3_0::chromatix_demux13_reserveType*               pReserveType   = NULL;
    demux_1_3_0::chromatix_demux13Type::enable_sectionStruct* pModuleEnable  = NULL;

    if ((NULL != pInput) && (NULL != pOutput) && (NULL != pInput->pHWSetting))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);

        result      = GetPixelFormat(&pixelFormat, &(pInput->bayerPattern));

        if (NULL == pOEMIQData)
        {
            if (NULL != pInput->pChromatixInput)
            {
                pReserveType  = &(pInput->pChromatixInput->chromatix_demux13_reserve);
                pModuleEnable = &(pInput->pChromatixInput->enable_section);
            }
            else
            {
                result = CamxResultEFailed;
            }
        }
        else
        {
            if (PipelineType::IFE == pOutput->type)
            {
                OEMIFEIQSetting* pOEMInput = static_cast<OEMIFEIQSetting*>(pOEMIQData);

                // Use OEM defined interpolation data
                pReserveType  = reinterpret_cast<demux_1_3_0::chromatix_demux13_reserveType*>(
                                     &(pOEMInput->DemuxSetting));
                pModuleEnable = reinterpret_cast<demux_1_3_0::chromatix_demux13Type::enable_sectionStruct*>(
                                     &(pOEMInput->DemuxEnableSection));
            }
            else if (PipelineType::BPS == pOutput->type)
            {
                OEMBPSIQSetting* pOEMInput = static_cast<OEMBPSIQSetting*>(pOEMIQData);

                // Use OEM defined interpolation data
                pReserveType   = reinterpret_cast<demux_1_3_0::chromatix_demux13_reserveType*>(
                                     &(pOEMInput->DemuxSetting));
                pModuleEnable  = reinterpret_cast<demux_1_3_0::chromatix_demux13Type::enable_sectionStruct*>(
                                     &(pOEMInput->DemuxEnableSection));
            }
            else
            {
                result = CamxResultEInvalidArg;
                CAMX_ASSERT_ALWAYS_MESSAGE("Pipeline Type %d not supported for Demux13", pOutput->type);
            }
        }

        if (NULL != pReserveType)
        {
            commonIQResult = TRUE;
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("No Reserve Data");
        }


        if (TRUE == commonIQResult)
        {
            Demux13UnpackedField unpackedData;

            if (CamxResultSuccess == result)
            {
                // Call the Interpolation Calculation
                commonIQResult =
                    IQInterface::s_interpolationTable.demux13CalculateHWSetting(pInput,
                                                                                pReserveType,
                                                                                pModuleEnable,
                                                                                static_cast<VOID*>(&unpackedData));
            }

            if (TRUE == commonIQResult)
            {
                result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedData), pOutput);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("Demux Calculate HW settings failed.");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("Demux interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Null input/output parameter to Demux13 module");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::Demosaic36CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::Demosaic36CalculateSetting(
    const Demosaic36InputData*  pInput,
    VOID*                       pOEMIQData,
    Demosaic36OutputData*       pOutput)
{
    CamxResult                                                      result             = CamxResultSuccess;
    BOOL                                                            commonIQResult     = FALSE;
    demosaic_3_6_0::demosaic36_rgn_dataType*                        pInterpolationData = NULL;
    demosaic_3_6_0::chromatix_demosaic36Type::enable_sectionStruct* pModuleEnable      = NULL;
    ISPHWSetting*                                                   pHWSetting         = NULL;

    if ((NULL != pInput) && (NULL != pInput->pHWSetting) && (NULL != pOutput))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pInterpolationData = static_cast<demosaic_3_6_0::demosaic36_rgn_dataType*>(pInput->pInterpolationData);
            commonIQResult     = IQInterface::s_interpolationTable.demosaic36Interpolation(pInput, pInterpolationData);
            pModuleEnable      = &(pInput->pChromatix->enable_section);
        }
        else
        {
            if (PipelineType::IFE == pOutput->type)
            {
                OEMIFEIQSetting* pOEMInput = static_cast<OEMIFEIQSetting*>(pOEMIQData);

                // Use OEM defined interpolation data
                pInterpolationData         =
                    reinterpret_cast<demosaic_3_6_0::demosaic36_rgn_dataType*>(&(pOEMInput->DemosaicSetting));
                pModuleEnable              =
                    reinterpret_cast<demosaic_3_6_0::chromatix_demosaic36Type::enable_sectionStruct*>(
                    &(pOEMInput->DemosaicEnableSection));
            }
            else if (PipelineType::BPS == pOutput->type)
            {
                OEMBPSIQSetting* pOEMInput = static_cast<OEMBPSIQSetting*>(pOEMIQData);

                // Use OEM defined interpolation data
                pInterpolationData         =
                    reinterpret_cast<demosaic_3_6_0::demosaic36_rgn_dataType*>(&(pOEMInput->DemosaicSetting));
                pModuleEnable              =
                    reinterpret_cast<demosaic_3_6_0::chromatix_demosaic36Type::enable_sectionStruct*>(
                        &(pOEMInput->DemosaicEnableSection));
            }
            else
            {
                result = CamxResultEInvalidArg;
                CAMX_ASSERT_ALWAYS_MESSAGE("Pipeline Type %d not supported for Demosaic36", pOutput->type);
            }

            if ((NULL != pInterpolationData) && (NULL != pModuleEnable))
            {
                commonIQResult = TRUE;
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            Demosaic36UnpackedField unpackedRegData;

            commonIQResult =
                IQInterface::s_interpolationTable.demosaic36CalculateHWSetting(pInput,
                                                                               pInterpolationData,
                                                                               pModuleEnable,
                                                                               static_cast<VOID*>(&unpackedRegData));
            if (TRUE == commonIQResult)
            {
                if ((PipelineType::IFE == pOutput->type) || (PipelineType::BPS == pOutput->type))
                {
                    pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegData), pOutput);
                }
                else
                {
                    result = CamxResultEInvalidArg;
                    CAMX_ASSERT_ALWAYS_MESSAGE("invalid Pipeline");
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE(" Demosaic CalculateHWSetting failed.");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("Demosaic interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL ");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::Demosaic37CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::Demosaic37CalculateSetting(
    const Demosaic37InputData*  pInput,
    VOID*                       pOEMIQData,
    Demosaic37OutputData*       pOutput)
{
    CamxResult                                                      result             = CamxResultSuccess;
    BOOL                                                            commonIQResult     = FALSE;
    demosaic_3_7_0::demosaic37_rgn_dataType*                        pInterpolationData = NULL;
    demosaic_3_7_0::chromatix_demosaic37Type::enable_sectionStruct* pModuleEnable      = NULL;
    ISPHWSetting*                                                   pHWSetting         = NULL;

    if ((NULL != pInput) && (NULL != pInput->pHWSetting) && (NULL != pOutput))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pInterpolationData = static_cast<demosaic_3_7_0::demosaic37_rgn_dataType*>(pInput->pInterpolationData);
            commonIQResult     = IQInterface::s_interpolationTable.demosaic37Interpolation(pInput, pInterpolationData);
            pModuleEnable      = &(pInput->pChromatix->enable_section);
        }
        else
        {
            if (PipelineType::IFE == pOutput->type)
            {
                OEMIFEIQSetting* pOEMInput = static_cast<OEMIFEIQSetting*>(pOEMIQData);

                // Use OEM defined interpolation data
                pInterpolationData =
                    reinterpret_cast<demosaic_3_7_0::demosaic37_rgn_dataType*>(&(pOEMInput->DemosaicSetting));
                pModuleEnable      =
                    reinterpret_cast<demosaic_3_7_0::chromatix_demosaic37Type::enable_sectionStruct*>(
                        &(pOEMInput->DemosaicEnableSection));
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Pipeline Type %d not supported for Demosaic37", pOutput->type);
            }

            if ((NULL != pInterpolationData) && (NULL != pModuleEnable))
            {
                commonIQResult = TRUE;
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            Demosaic37UnpackedField unpackedRegData;

            commonIQResult =
                IQInterface::s_interpolationTable.demosaic37CalculateHWSetting(pInput,
                    pInterpolationData,
                    pModuleEnable,
                    static_cast<VOID*>(&unpackedRegData));
            if (TRUE == commonIQResult)
            {
                if (PipelineType::IFE == pOutput->type)
                {
                    pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegData), NULL);
                }
                else
                {
                    result = CamxResultEInvalidArg;
                    CAMX_ASSERT_ALWAYS_MESSAGE("invalid Pipeline");
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE(" Demosaic CalculateHWSetting failed.");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("Demosaic interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL ");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IFEBPCBCC50CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IFEBPCBCC50CalculateSetting(
    const BPCBCC50InputData*  pInput,
    VOID*                     pOEMIQData)
{
    CamxResult                           result             = CamxResultSuccess;
    BOOL                                 commonIQResult     = FALSE;
    ISPHWSetting*                        pHWSetting         = NULL;
    bpcbcc_5_0_0::bpcbcc50_rgn_dataType* pInterpolationData = NULL;
    OEMIFEIQSetting*                     pOEMInput          = NULL;
    globalelements::enable_flag_type     moduleEnable       = 0;
    BPCBCC50UnpackedField                unpackedRegData;

    if ((NULL != pInput) && (NULL != pInput->pHWSetting))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        if (NULL == pOEMIQData)
        {
            pInterpolationData = static_cast<bpcbcc_5_0_0::bpcbcc50_rgn_dataType*>(pInput->pInterpolationData);
            // Call the Interpolation Calculation
            commonIQResult = IQInterface::s_interpolationTable.IFEBPCBCC50Interpolation(pInput, pInterpolationData);
            moduleEnable       = pInput->pChromatix->enable_section.bpcbcc_enable;
        }
        else
        {
            pOEMInput    = static_cast<OEMIFEIQSetting*>(pOEMIQData);
            moduleEnable = static_cast<globalelements::enable_flag_type>(pOEMInput->BPCBCCEnable);

            // Use OEM defined interpolation data
            pInterpolationData =
                reinterpret_cast<bpcbcc_5_0_0::bpcbcc50_rgn_dataType*>(&(pOEMInput->BPCBCCSetting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            commonIQResult = IQInterface::s_interpolationTable.IFEBPCBCC50CalculateHWSetting(
                pInput, pInterpolationData, moduleEnable, static_cast<VOID*>(&unpackedRegData));

            if (TRUE == commonIQResult)
            {
                result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegData), NULL);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("BPCBCC interpolation failed.");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("BPCBCC interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is pInput %p", pInput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::CopyLSCMapData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IQInterface::CopyLSCMapData(
    const ISPInputData*    pInputData,
    LSC34UnpackedField*    pUnpackedField)
{
    UINT16 bankS        = pUnpackedField->bank_sel;
    UINT32 k            = 0;
    UINT32 totalHorMesh = 0;
    UINT32 toatlVerMesh = 0;
    UINT32 dmiCount     = 0;


    // (NUM_MESHGAIN_H+2) * (NUM_MESHGAIN_V+2) LUT entries for a frame.
    totalHorMesh = pUnpackedField->num_meshgain_h + 2;
    toatlVerMesh = pUnpackedField->num_meshgain_v + 2;

    for (UINT16 i = 0; i < toatlVerMesh; i++)
    {
        for (UINT16 j = 0; j < totalHorMesh; j++)
        {
            // 0->R
            pInputData->pCalculatedData->lensShadingInfo.lensShadingMap[k++]     =
                ((static_cast<FLOAT> (pUnpackedField->mesh_table_l[bankS][0][i][j])) / (1 << 10));

            if (pInputData->sensorData.format == PixelFormat::BayerBGGR)
            {
                // 1->Gr
                pInputData->pCalculatedData->lensShadingInfo.lensShadingMap[k++] =
                    ((static_cast<FLOAT> (pUnpackedField->mesh_table_l[bankS][1][i][j])) / (1 << 10));
                pInputData->pCalculatedData->lensShadingInfo.lensShadingMap[k++] =
                    ((static_cast<FLOAT> (pUnpackedField->mesh_table_l[bankS][2][i][j])) / (1 << 10));
            }
            else
            {
                // 2->Gb
                pInputData->pCalculatedData->lensShadingInfo.lensShadingMap[k++] =
                    ((static_cast<FLOAT> (pUnpackedField->mesh_table_l[bankS][2][i][j])) / (1 << 10));
                // 1->Gr
                pInputData->pCalculatedData->lensShadingInfo.lensShadingMap[k++] =
                    ((static_cast<FLOAT> (pUnpackedField->mesh_table_l[bankS][1][i][j])) / (1 << 10));
            }

            // 3->B
            pInputData->pCalculatedData->lensShadingInfo.lensShadingMap[k++]     =
                ((static_cast<FLOAT> (pUnpackedField->mesh_table_l[bankS][3][i][j])) / (1 << 10));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::CopyLSCMapDataV40
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IQInterface::CopyLSCMapDataV40(
    const ISPInputData*    pInputData,
    LSC40UnpackedField*    pUnpackedField)
{
    UINT16 bankS = pUnpackedField->bank_sel;
    UINT32 k = 0;
    UINT32 totalHorMesh = 0;
    UINT32 toatlVerMesh = 0;
    UINT32 dmiCount = 0;


    // (NUM_MESHGAIN_H+2) * (NUM_MESHGAIN_V+2) LUT entries for a frame.
    totalHorMesh = pUnpackedField->num_meshgain_h + 2;
    toatlVerMesh = pUnpackedField->num_meshgain_v + 2;

    for (UINT16 i = 0; i < toatlVerMesh; i++)
    {
        for (UINT16 j = 0; j < totalHorMesh; j++)
        {
            // 0->R
            pInputData->pCalculatedData->lensShadingInfo.lensShadingMap[k++] =
                ((static_cast<FLOAT> (pUnpackedField->mesh_table[bankS][0][i][j])) / (1 << 10));

            if (pInputData->sensorData.format == PixelFormat::BayerBGGR)
            {
                // 1->Gr
                pInputData->pCalculatedData->lensShadingInfo.lensShadingMap[k++] =
                    ((static_cast<FLOAT> (pUnpackedField->mesh_table[bankS][1][i][j])) / (1 << 10));
                pInputData->pCalculatedData->lensShadingInfo.lensShadingMap[k++] =
                    ((static_cast<FLOAT> (pUnpackedField->mesh_table[bankS][2][i][j])) / (1 << 10));
            }
            else
            {
                // 2->Gb
                pInputData->pCalculatedData->lensShadingInfo.lensShadingMap[k++] =
                    ((static_cast<FLOAT> (pUnpackedField->mesh_table[bankS][2][i][j])) / (1 << 10));
                // 1->Gr
                pInputData->pCalculatedData->lensShadingInfo.lensShadingMap[k++] =
                    ((static_cast<FLOAT> (pUnpackedField->mesh_table[bankS][1][i][j])) / (1 << 10));
            }

            // 3->B
            pInputData->pCalculatedData->lensShadingInfo.lensShadingMap[k++] =
                ((static_cast<FLOAT> (pUnpackedField->mesh_table[bankS][3][i][j])) / (1 << 10));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::LSC34CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::LSC34CalculateSetting(
    LSC34InputData*              pInput,
    VOID*                        pOEMIQData,
    const ISPInputData*          pInputData,
    LSC34OutputData*             pOutput)
{
    CamxResult                                            result             = CamxResultSuccess;
    BOOL                                                  commonIQResult     = FALSE;
    ISPHWSetting*                                         pHWSetting         = NULL;
    lsc_3_4_0::lsc34_rgn_dataType*                        pInterpolationData = NULL;
    tintless_2_0_0::tintless20_rgn_dataType               tintlessInterpolationData;
    lsc_3_4_0::chromatix_lsc34Type::enable_sectionStruct* pModuleEnable      = NULL;
    UINT32 cameraID;
    UINT32 sensorAR;

    if ((NULL != pInput) && (NULL != pOutput) && (NULL != pInputData) && (NULL != pInput->pHWSetting))
    {
        cameraID = pInputData->sensorID;
        sensorAR = pInputData->sensorData.sensorAspectRatioMode;
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        pInterpolationData = static_cast<lsc_3_4_0::lsc34_rgn_dataType*>(pInput->pInterpolationData);

        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            if (NULL != pInterpolationData)
            {
                commonIQResult = IQInterface::s_interpolationTable.LSC34Interpolation(pInput, pInterpolationData);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid InterpolationData pointer");
            }

            pModuleEnable      = &(pInput->pChromatix->enable_section);

            if (NULL != pInput->pTintlessConfig &&
                NULL != pInput->pTintlessStats)
            {
                struct Tintless20InterpolationInput tintlessInput;

                tintlessInput.pTintlessChromatix = pInput->pTintlessChromatix;
                tintlessInput.AECSensitivity     = pInput->AECSensitivity;
                tintlessInput.exposureTime       = pInput->exposureTime;
                tintlessInput.luxIndex           = pInput->luxIndex;
                tintlessInput.realGain           = pInput->realGain;

                commonIQResult = IQInterface::s_interpolationTable.TINTLESS20Interpolation(
                    &tintlessInput, &tintlessInterpolationData);
            }
        }
        else
        {
            if (PipelineType::IFE == pOutput->type)
            {
                OEMIFEIQSetting* pOEMInput = static_cast<OEMIFEIQSetting*>(pOEMIQData);
                pModuleEnable              = reinterpret_cast<lsc_3_4_0::chromatix_lsc34Type::enable_sectionStruct*>(
                                                &(pOEMInput->LSCEnableSection));

                if (NULL != pInterpolationData)
                {
                    // Copy OEM defined interpolation data
                    Utils::Memcpy(pInterpolationData, &pOEMInput->LSCSetting, sizeof(lsc_3_4_0::lsc34_rgn_dataType));
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid InterpolationData pointer");
                }
            }
            else if (PipelineType::BPS == pOutput->type)
            {
                OEMBPSIQSetting* pOEMInput = static_cast<OEMBPSIQSetting*>(pOEMIQData);
                pModuleEnable              = reinterpret_cast<lsc_3_4_0::chromatix_lsc34Type::enable_sectionStruct*>(
                                                &(pOEMInput->LSCEnableSection));

                if (NULL != pInterpolationData)
                {
                    // Copy OEM defined interpolation data
                    Utils::Memcpy(pInterpolationData, &pOEMInput->LSCSetting, sizeof(lsc_3_4_0::lsc34_rgn_dataType));
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid InterpolationData pointer");
                }
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Pipeline Type %d not supported for Demux13", pOutput->type);
            }

            if (NULL != pModuleEnable)
            {
                commonIQResult = TRUE;
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            if (PipelineType::IFE == pOutput->type)
            {
                pOutput->pUnpackedField->bank_sel = pInput->bankSelect;
                commonIQResult = IQInterface::s_interpolationTable.LSC34CalculateHWSetting(
                    pInput,
                    pInterpolationData,
                    pModuleEnable,
                    &tintlessInterpolationData,
                    static_cast<VOID*>(pOutput->pUnpackedField));

                if (TRUE == commonIQResult)
                {
                    if (NULL != pInput->pTintlessStats)
                    {
                        memcpy(s_prevTintlessOutput_l[cameraID][sensorAR],
                            pOutput->pUnpackedField->mesh_table_l[pInput->bankSelect],
                            sizeof(s_prevTintlessOutput_l[cameraID][sensorAR]));
                        memcpy(s_prevTintlessOutput_r[cameraID][sensorAR],
                            pOutput->pUnpackedField->mesh_table_r[pInput->bankSelect],
                            sizeof(s_prevTintlessOutput_r[cameraID][sensorAR]));
                        s_prevTintlessTableValid[cameraID][sensorAR] = TRUE;
                    }
                    if (TRUE == pInput->fetchSettingOnly)
                    {
                        pOutput->lscState.bwidth_l         = static_cast<uint16_t>(pOutput->pUnpackedField->bwidth_l);
                        pOutput->lscState.bx_d1_l          = static_cast<uint16_t>(pOutput->pUnpackedField->bx_d1_l);
                        pOutput->lscState.bx_start_l       = static_cast<uint16_t>(pOutput->pUnpackedField->bx_start_l);
                        pOutput->lscState.lx_start_l       = static_cast<uint16_t>(pOutput->pUnpackedField->lx_start_l);
                        pOutput->lscState.meshGridBwidth_l =
                            static_cast<uint16_t>(pOutput->pUnpackedField->meshGridBwidth_l);
                        pOutput->lscState.num_meshgain_h   =
                            static_cast<uint16_t>(pOutput->pUnpackedField->num_meshgain_h);
                    }
                    else
                    {
                        result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(pOutput->pUnpackedField), pOutput);
                        if (StatisticsLensShadingMapModeOn == pInputData->pHALTagsData->statisticsLensShadingMapMode)
                        {
                            CopyLSCMapData(pInputData, pOutput->pUnpackedField);
                        }
                    }
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_ASSERT_ALWAYS_MESSAGE("LSC CalculateHWSetting failed.");
                }

            }
            else if (PipelineType::BPS == pOutput->type)
            {
                pOutput->pUnpackedField->bank_sel = pInput->bankSelect;
                commonIQResult           = IQInterface::s_interpolationTable.LSC34CalculateHWSetting(
                    pInput,
                    pInterpolationData,
                    pModuleEnable,
                    &tintlessInterpolationData,
                    static_cast<VOID*>(pOutput->pUnpackedField));
                if (TRUE == commonIQResult)
                {
                    if (TRUE == s_prevTintlessTableValid[cameraID][sensorAR] && NULL == pInput->pTintlessStats)
                    {
                        memcpy(pOutput->pUnpackedField->mesh_table_l[pInput->bankSelect],
                            s_prevTintlessOutput_l[cameraID][sensorAR],
                            sizeof(pOutput->pUnpackedField->mesh_table_l[pInput->bankSelect]));
                        memcpy(pOutput->pUnpackedField->mesh_table_r[pInput->bankSelect],
                            s_prevTintlessOutput_r[cameraID][sensorAR],
                            sizeof(pOutput->pUnpackedField->mesh_table_r[pInput->bankSelect]));
                        CAMX_LOG_INFO(CamxLogGroupISP, "BPS UsePrevious Tintless camera %d sensorAR %d"
                                                       "bankS:%d LSC Table B0 0x%x 0x%x B1 0x%x 0x%x",
                            cameraID, sensorAR, pInput->bankSelect,
                            pOutput->pUnpackedField->mesh_table_l[0][0][0][0],
                            pOutput->pUnpackedField->mesh_table_l[0][0][0][1],
                            pOutput->pUnpackedField->mesh_table_l[1][0][0][0],
                            pOutput->pUnpackedField->mesh_table_l[1][0][0][1]);
                    }
                    result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(pOutput->pUnpackedField), pOutput);
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_ASSERT_ALWAYS_MESSAGE("LSC CalculateHWSetting failed.");
                }

            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("invalid Pipeline");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("LSC intepolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL ");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::LSCMeshTableInit
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::LSCMeshTableInit(
    UINT32                    cameraID,
    UINT32                    sensorAR)
{
    CamxResult         result = CamxResultSuccess;

    s_prevTintlessTableValid[cameraID][sensorAR]  = FALSE;
    s_prevALSCTableValid[cameraID][sensorAR]      = FALSE;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::LSC40CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::LSC40CalculateSetting(
    LSC40InputData*              pInput,
    VOID*                        pOEMIQData,
    const ISPInputData*          pInputData,
    LSC40OutputData*             pOutput)
{
    CamxResult                                            result             = CamxResultSuccess;
    BOOL                                                  commonIQResult     = FALSE;
    ISPHWSetting*                                         pHWSetting         = NULL;
    lsc_4_0_0::lsc40_rgn_dataType*                        pInterpolationData = NULL;
    lsc_4_0_0::chromatix_lsc40_reserveType*               pReserveType       = NULL;
    tintless_2_0_0::tintless20_rgn_dataType               tintlessInterpolationData;
    lsc_4_0_0::chromatix_lsc40Type::enable_sectionStruct* pModuleEnable      = NULL;
    UINT32                                                cameraID           = 0;
    UINT32                                                sensorAR           = 0;

    if ((NULL != pInput)             &&
        (NULL != pOutput)            &&
        (NULL != pInput->pChromatix) &&
        (NULL != pInputData)         &&
        (NULL != pInput->pHWSetting))
    {
        cameraID           = pInputData->sensorID;
        sensorAR           = pInputData->sensorData.sensorAspectRatioMode;
        pHWSetting         = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        pInterpolationData = static_cast<lsc_4_0_0::lsc40_rgn_dataType*>(pInput->pInterpolationData);

        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            if (NULL != pInterpolationData)
            {
                commonIQResult = IQInterface::s_interpolationTable.LSC40Interpolation(pInput, pInterpolationData);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid InterpolationData pointer");
            }

            pReserveType = &(pInput->pChromatix->chromatix_lsc40_reserve);
            pModuleEnable      = &(pInput->pChromatix->enable_section);

            if (NULL != pInput->pTintlessConfig &&
                NULL != pInput->pTintlessStats)
            {
                struct Tintless20InterpolationInput  tintlessInput;

                tintlessInput.pTintlessChromatix = pInput->pTintlessChromatix;
                tintlessInput.AECSensitivity     = pInput->AECSensitivity;
                tintlessInput.exposureTime       = pInput->exposureTime;
                tintlessInput.luxIndex           = pInput->luxIndex;
                tintlessInput.realGain           = pInput->realGain;

                commonIQResult = IQInterface::s_interpolationTable.TINTLESS20Interpolation(
                    &tintlessInput, &tintlessInterpolationData);
            }
        }
        else
        {
            if (PipelineType::IFE == pOutput->type)
            {
                OEMIFEIQSetting* pOEMInput = static_cast<OEMIFEIQSetting*>(pOEMIQData);
                // Use OEM defined interpolation data
                pInterpolationData =
                    reinterpret_cast<lsc_4_0_0::lsc40_rgn_dataType*>(&(pOEMInput->LSC40Setting));

                if (NULL != pInterpolationData)
                {
                    commonIQResult = TRUE;
                    pReserveType = reinterpret_cast<lsc_4_0_0::chromatix_lsc40_reserveType*>(&(pOEMInput->LSC40CCReserveType));
                }
                else
                {
                    CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
                }

                pModuleEnable = reinterpret_cast<lsc_4_0_0::chromatix_lsc40Type::enable_sectionStruct*>
                    (&(pOEMInput->LSC40EnableSection));
            }
            else if (PipelineType::BPS == pOutput->type)
            {
                OEMBPSIQSetting* pOEMInput = static_cast<OEMBPSIQSetting*>(pOEMIQData);
                // Use OEM defined interpolation data
                pInterpolationData =
                    reinterpret_cast<lsc_4_0_0::lsc40_rgn_dataType*>(&(pOEMInput->LSC40Setting));

                if (NULL != pInterpolationData)
                {
                    commonIQResult = TRUE;
                    pReserveType = reinterpret_cast<lsc_4_0_0::chromatix_lsc40_reserveType*>(&(pOEMInput->LSC40CCReserveType));
                }
                else
                {
                    CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
                }

                pModuleEnable = reinterpret_cast<lsc_4_0_0::chromatix_lsc40Type::enable_sectionStruct*>
                    (&(pOEMInput->LSC40EnableSection));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Pipeline %d", pOutput->type);
            }
        }

        if (TRUE == commonIQResult)
        {
            if (PipelineType::IFE == pOutput->type)
            {
                pOutput->pUnpackedField->bank_sel = pInput->bankSelect;
                commonIQResult = IQInterface::s_interpolationTable.LSC40CalculateHWSetting(
                    pInput,
                    pInterpolationData,
                    pReserveType,
                    pModuleEnable,
                    &tintlessInterpolationData,
                    static_cast<VOID*>(pOutput->pUnpackedField));

                if (TRUE != commonIQResult)
                {
                    result = CamxResultEFailed;
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "LSC CalculateHWSetting failed.");
                }

                if (NULL != pInput->pTintlessStats)
                {
                    Utils::Memcpy(s_prevTintlessOutput_r[cameraID][sensorAR],
                                  pOutput->pUnpackedField->mesh_table[pInput->bankSelect],
                                  sizeof(s_prevTintlessOutput_r[cameraID][sensorAR]));

                    s_prevTintlessTableValid[cameraID][sensorAR] = TRUE;

                    CAMX_LOG_INFO(CamxLogGroupISP, "IFE Store Tintless camera %d sensorAR %d"
                        "bankS:%d LSC Table GR 0x%x, 0x%x BG 0x%x 0x%x",
                        cameraID, sensorAR, pInput->bankSelect,
                        pOutput->pUnpackedField->mesh_table[pInput->bankSelect][0][0][0],
                        pOutput->pUnpackedField->mesh_table[pInput->bankSelect][1][0][0],
                        pOutput->pUnpackedField->mesh_table[pInput->bankSelect][2][0][0],
                        pOutput->pUnpackedField->mesh_table[pInput->bankSelect][3][0][0]);
                }

                if (NULL != pInput->pAWBBGStats)
                {
                    Utils::Memcpy(s_prevALSCGainOutput[cameraID][sensorAR],
                                  pOutput->pUnpackedField->grids_gain[pInput->bankSelect],
                                  sizeof(s_prevALSCGainOutput[cameraID][sensorAR]));

                    Utils::Memcpy(s_prevALSCMeanOutput[cameraID][sensorAR],
                                  pOutput->pUnpackedField->grids_mean[pInput->bankSelect],
                                  sizeof(s_prevALSCMeanOutput[cameraID][sensorAR]));

                    CAMX_LOG_INFO(CamxLogGroupISP, "IFE Store ALSC camera %d sensorAR %d"
                        "bankS:%d ALSC grid gain Table [0][0] 0x%x, [0][1] 0x%x, mean Table [0][0] 0x%x, [0][1] 0x%x",
                        cameraID, sensorAR, pInput->bankSelect,
                        pOutput->pUnpackedField->grids_gain[pInput->bankSelect][0][0],
                        pOutput->pUnpackedField->grids_gain[pInput->bankSelect][0][1],
                        pOutput->pUnpackedField->grids_mean[pInput->bankSelect][0][0],
                        pOutput->pUnpackedField->grids_mean[pInput->bankSelect][0][1]);

                    s_prevALSCTableValid[cameraID][sensorAR] = TRUE;
                }

                if (TRUE == commonIQResult)
                {
                    if (TRUE == pInput->fetchSettingOnly)
                    {
                        pOutput->lscState.bwidth_l          = static_cast<uint16_t>(pOutput->pUnpackedField->Bwidth);
                        pOutput->lscState.bx_d1_l           = static_cast<uint16_t>(pOutput->pUnpackedField->Bx_d1);
                        pOutput->lscState.bx_start_l        = static_cast<uint16_t>(pOutput->pUnpackedField->Bx_start);
                        pOutput->lscState.lx_start_l        = static_cast<uint16_t>(pOutput->pUnpackedField->Lx_start);
                        pOutput->lscState.meshGridBwidth_l  =
                            static_cast<uint16_t>(pOutput->pUnpackedField->MeshGridBwidth);
                        pOutput->lscState.num_meshgain_h    =
                            static_cast<uint16_t>(pOutput->pUnpackedField->num_meshgain_h);
                    }
                    else
                    {
                        result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(pOutput->pUnpackedField), pOutput);
                    }
                }
            }
            else if (PipelineType::BPS == pOutput->type)
            {
                pOutput->pUnpackedField->bank_sel = pInput->bankSelect;
                commonIQResult = IQInterface::s_interpolationTable.LSC40CalculateHWSetting(
                    pInput,
                    pInterpolationData,
                    pReserveType,
                    pModuleEnable,
                    &tintlessInterpolationData,
                    static_cast<VOID*>(pOutput->pUnpackedField));
                if (TRUE == commonIQResult)
                {
                    if (TRUE == s_prevTintlessTableValid[cameraID][sensorAR] && NULL == pInput->pTintlessStats)
                    {
                        Utils::Memcpy(pOutput->pUnpackedField->mesh_table[pInput->bankSelect],
                                      s_prevTintlessOutput_r[cameraID][sensorAR],
                                      sizeof(pOutput->pUnpackedField->mesh_table[pInput->bankSelect]));

                        CAMX_LOG_INFO(CamxLogGroupISP, "BPS UsePrevious Tintless camera %d sensorAR %d"
                            "bankS:%d LSC Table GR 0x%x, 0x%x BG 0x%x 0x%x",
                            cameraID, sensorAR, pInput->bankSelect,
                            pOutput->pUnpackedField->mesh_table[pInput->bankSelect][0][0][0],
                            pOutput->pUnpackedField->mesh_table[pInput->bankSelect][1][0][0],
                            pOutput->pUnpackedField->mesh_table[pInput->bankSelect][2][0][0],
                            pOutput->pUnpackedField->mesh_table[pInput->bankSelect][3][0][0]);
                    }

                    if (TRUE == s_prevALSCTableValid[cameraID][sensorAR] && NULL == pInput->pAWBBGStats)
                    {
                        Utils::Memcpy(pOutput->pUnpackedField->grids_gain[pInput->bankSelect],
                                      s_prevALSCGainOutput[cameraID][sensorAR],
                                      sizeof(pOutput->pUnpackedField->grids_gain[pInput->bankSelect]));

                        Utils::Memcpy(pOutput->pUnpackedField->grids_mean[pInput->bankSelect],
                                      s_prevALSCMeanOutput[cameraID][sensorAR],
                                      sizeof(pOutput->pUnpackedField->grids_mean[pInput->bankSelect]));

                        pOutput->pUnpackedField->ALSC_enable = 1;

                        CAMX_LOG_INFO(CamxLogGroupISP, "BPS UsePrevious ALSC camera %d sensorAR %d"
                            "bankS:%d ALSC grid gain Table [0][0] 0x%x, [0][1] 0x%x, mean Table [0][0] 0x%x, [0][1] 0x%x",
                            cameraID, sensorAR, pInput->bankSelect,
                            pOutput->pUnpackedField->grids_gain[pInput->bankSelect][0][0],
                            pOutput->pUnpackedField->grids_gain[pInput->bankSelect][0][1],
                            pOutput->pUnpackedField->grids_mean[pInput->bankSelect][0][0],
                            pOutput->pUnpackedField->grids_mean[pInput->bankSelect][0][1]);
                    }
                    else if (NULL == pInput->pAWBBGStats)
                    {
                        pOutput->pUnpackedField->ALSC_enable = 0;
                    }
                    result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(pOutput->pUnpackedField), pOutput);
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "LSC CalculateHWSetting failed.");
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Pipeline %d", pOutput->type);
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "LSC intepolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Input data is NULL ");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IFECC12CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IFECC12CalculateSetting(
    const CC12InputData*  pInput,
    VOID*                 pOEMIQData)
{
    CamxResult                            result             = CamxResultSuccess;
    BOOL                                  commonIQResult     = FALSE;
    ISPHWSetting*                         pHWSetting         = NULL;
    cc_1_2_0::cc12_rgn_dataType*          pInterpolationData = NULL;
    OEMIFEIQSetting*                      pOEMInput          = NULL;
    cc_1_2_0::chromatix_cc12_reserveType* pReserveType       = NULL;
    CC12UnpackedField                     unpackedRegData;

    if ((NULL != pInput) && (NULL != pInput->pHWSetting))
    {
        Utils::Memset(&unpackedRegData, 0, sizeof(CC12UnpackedField));
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        if (NULL == pOEMIQData)
        {
            pInterpolationData = static_cast<cc_1_2_0::cc12_rgn_dataType*>(pInput->pInterpolationData);
            // Call the Interpolation Calculation
            commonIQResult     = IQInterface::s_interpolationTable.IFECC12Interpolation(pInput, pInterpolationData);
            pReserveType       = &(pInput->pChromatix->chromatix_cc12_reserve);
        }
        else
        {
            pOEMInput = static_cast<OEMIFEIQSetting*>(pOEMIQData);
            // Use OEM defined interpolation data
            pInterpolationData =
                reinterpret_cast<cc_1_2_0::cc12_rgn_dataType*>(&(pOEMInput->CCSetting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
                pReserveType   = reinterpret_cast<cc_1_2_0::chromatix_cc12_reserveType*>(&(pOEMInput->CCReserveType));
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            commonIQResult = IQInterface::s_interpolationTable.IFECC12CalculateHWSetting(
                pInput, pInterpolationData, pReserveType, static_cast<VOID*>(&unpackedRegData));
            if (TRUE == commonIQResult)
            {
                result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegData), NULL);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("CC1_2 Calculate HW setting failed.");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("CC1_2 interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is pInput %p", pInput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::CST12CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::CST12CalculateSetting(
    const CST12InputData* pInput,
    VOID*                 pOEMIQData,
    CST12OutputData*      pOutput)
{
    CamxResult                                            result         = CamxResultSuccess;
    BOOL                                                  commonIQResult = FALSE;
    ISPHWSetting*                                         pHWSetting     = NULL;
    cst_1_2_0::chromatix_cst12_reserveType*               pReserveType   = NULL;
    cst_1_2_0::chromatix_cst12Type::enable_sectionStruct* pModuleEnable  = NULL;

    // if ((NULL != pInput) && (NULL != pInput->pHWSetting) && (NULL != pOutput))
    if ((NULL != pInput) && (NULL != pOutput))
    {
        // pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        if (NULL == pOEMIQData)
        {
            pReserveType  = &(pInput->pChromatix->chromatix_cst12_reserve);
            pModuleEnable = &(pInput->pChromatix->enable_section);
        }
        else
        {
            if (PipelineType::IFE == pOutput->type)
            {
                OEMIFEIQSetting* pOEMInput = static_cast<OEMIFEIQSetting*>(pOEMIQData);

                // Use OEM defined interpolation data
                pReserveType               = reinterpret_cast<cst_1_2_0::chromatix_cst12_reserveType*>(
                                                &(pOEMInput->CSTSetting));
                pModuleEnable              = reinterpret_cast<cst_1_2_0::chromatix_cst12Type::enable_sectionStruct*>(
                                                &(pOEMInput->CSTEnableSection));
            }
            else if (PipelineType::BPS == pOutput->type)
            {
                OEMBPSIQSetting* pOEMInput = static_cast<OEMBPSIQSetting*>(pOEMIQData);

                // Use OEM defined interpolation data
                pReserveType               = reinterpret_cast<cst_1_2_0::chromatix_cst12_reserveType*>(
                                                &(pOEMInput->CSTSetting));
                pModuleEnable              = reinterpret_cast<cst_1_2_0::chromatix_cst12Type::enable_sectionStruct*>(
                                                &(pOEMInput->CSTEnableSection));
            }
            else if (PipelineType::IPE == pOutput->type)
            {
                OEMIPEIQSetting* pOEMInput = static_cast<OEMIPEIQSetting*>(pOEMIQData);

                // Use OEM defined interpolation data
                pReserveType               = reinterpret_cast<cst_1_2_0::chromatix_cst12_reserveType*>(
                    &(pOEMInput->CSTSetting));
                pModuleEnable              = reinterpret_cast<cst_1_2_0::chromatix_cst12Type::enable_sectionStruct*>(
                    &(pOEMInput->CSTEnableSection));
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Pipeline Type %d not supported for Demux13", pOutput->type);
            }
        }

        if (NULL != pReserveType)
        {
            commonIQResult = TRUE;
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
        }

        if (TRUE == commonIQResult)
        {
            CST12UnpackedField unpackedRegData;

            commonIQResult =
                IQInterface::s_interpolationTable.CST12CalculateHWSetting(pInput,
                                                                          pReserveType,
                                                                          pModuleEnable,
                                                                          static_cast<VOID*>(&unpackedRegData));

            if (TRUE == commonIQResult)
            {
                // if ((PipelineType::IFE == pOutput->type)|| (PipelineType::BPS == pOutput->type))
                if (((PipelineType::IFE == pOutput->type)||
                    (PipelineType::BPS == pOutput->type)) &&
                    (NULL != pInput->pHWSetting))
                {
                    pHWSetting  = static_cast<ISPHWSetting*>(pInput->pHWSetting);
                    result      = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegData), pOutput);
                }
                else if (PipelineType::IPE == pOutput->type)
                {
                    pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
                    result = pHWSetting->PackIQRegisterSetting(&unpackedRegData, pOutput);
                }
                else
                {
                    result = CamxResultEInvalidArg;
                    CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Pipeline");
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("CST12 HW calculate Settings failed");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("CST12 interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is pInput %p, pOutput %p ", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IFELinearization33CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IFELinearization33CalculateSetting(
    const Linearization33InputData*  pInput,
    VOID*                            pOEMIQData,
    Linearization33OutputData*       pOutput)
{
    CamxResult                                         result             = CamxResultSuccess;
    BOOL                                               commonIQResult     = FALSE;
    linearization_3_3_0::linearization33_rgn_dataType* pInterpolationData = NULL;
    OEMIFEIQSetting*                                   pOEMInput          = NULL;
    ISPHWSetting*                                      pHWSetting         = NULL;
    Linearization33UnpackedField                       unpackedRegData;

    if ((NULL != pInput) && (NULL != pInput->pHWSetting) && (NULL != pOutput))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        if (NULL == pOEMIQData)
        {
            pInterpolationData = static_cast<linearization_3_3_0::linearization33_rgn_dataType*>(pInput->pInterpolationData);

            // Call the Interpolation Calculation
            commonIQResult     =
                IQInterface::s_interpolationTable.IFELinearization33Interpolation(pInput, pInterpolationData);
        }
        else
        {
            pOEMInput = static_cast<OEMIFEIQSetting*>(pOEMIQData);
            // Use OEM defined interpolation data
            pInterpolationData =
                reinterpret_cast<linearization_3_3_0::linearization33_rgn_dataType*>(&(pOEMInput->LinearizationSetting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            // Calculate the remaining Gain to apply in Demux
            if (FALSE == pInput->pedestalEnable)
            {
                FLOAT maxLUTValue = static_cast<FLOAT>(Linearization33MaximumLUTVal);

                pOutput->stretchGainRed =
                    static_cast<FLOAT>(maxLUTValue / (maxLUTValue - pInterpolationData->r_lut_p_tab.r_lut_p[0]));
                pOutput->stretchGainBlue =
                    static_cast<FLOAT>(maxLUTValue / (maxLUTValue - pInterpolationData->b_lut_p_tab.b_lut_p[0]));
                if (PixelFormat::BayerRGGB == static_cast<PixelFormat>(pInput->bayerPattern) ||
                    PixelFormat::BayerGRBG == static_cast<PixelFormat>(pInput->bayerPattern))
                {
                    pOutput->stretchGainGreenEven =
                        static_cast<FLOAT>(maxLUTValue / (maxLUTValue - pInterpolationData->gr_lut_p_tab.gr_lut_p[0]));
                    pOutput->stretchGainGreenOdd =
                        static_cast<FLOAT>(maxLUTValue / (maxLUTValue - pInterpolationData->gb_lut_p_tab.gb_lut_p[0]));
                }
                else if (PixelFormat::BayerBGGR == static_cast<PixelFormat>(pInput->bayerPattern) ||
                    PixelFormat::BayerGBRG == static_cast<PixelFormat>(pInput->bayerPattern))
                {
                    pOutput->stretchGainGreenEven =
                        static_cast<FLOAT>(maxLUTValue / (maxLUTValue - pInterpolationData->gb_lut_p_tab.gb_lut_p[0]));
                    pOutput->stretchGainGreenOdd =
                        static_cast<FLOAT>(maxLUTValue / (maxLUTValue - pInterpolationData->gr_lut_p_tab.gr_lut_p[0]));
                }
            }

            commonIQResult = IQInterface::s_interpolationTable.IFELinearization33CalculateHWSetting(
                                 pInput, pInterpolationData, static_cast<VOID*>(&unpackedRegData));
            if (TRUE == commonIQResult)
            {
                result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegData), pOutput);

                pOutput->dynamicBlackLevel[0] = unpackedRegData.r_lut_p_l[0];
                pOutput->dynamicBlackLevel[1] = unpackedRegData.gr_lut_p_l[0];
                pOutput->dynamicBlackLevel[2] = unpackedRegData.gb_lut_p_l[0];
                pOutput->dynamicBlackLevel[3] = unpackedRegData.b_lut_p_l[0];
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("Linearization calculate HW settings failed.");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("Linearization interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("data is pInput %p pOutput %p", pInput, pOutput);
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IFEPDPC11CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IFEPDPC11CalculateSetting(
    const PDPC11InputData*  pInput,
    VOID*                   pOEMIQData,
    PDPC11OutputData*       pOutput)
{
    CamxResult                       result                               = CamxResultSuccess;
    BOOL                             commonIQResult                       = FALSE;
    ISPHWSetting*                    pHWSetting                           = NULL;
    pdpc_1_1_0::pdpc11_rgn_dataType* pInterpolationData                   = NULL;
    OEMIFEIQSetting*                 pOEMInput                            = NULL;
    pdpc_1_1_0::chromatix_pdpc11Type::enable_sectionStruct* pModuleEnable = NULL;
    PDPC11UnpackedField              unpackedRegData;

    if ((NULL != pInput) && (NULL != pInput->pHWSetting) && (NULL != pOutput))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        if (NULL == pOEMIQData)
        {
            pInterpolationData = static_cast<pdpc_1_1_0::pdpc11_rgn_dataType*>(pInput->pInterpolationData);
            // Call the Interpolation Calculation
            commonIQResult     = IQInterface::s_interpolationTable.IFEPDPC11Interpolation(pInput, pInterpolationData);
            pModuleEnable      = &pInput->pChromatix->enable_section;
        }
        else
        {
            pOEMInput = static_cast<OEMIFEIQSetting*>(pOEMIQData);

            pModuleEnable =
               reinterpret_cast<pdpc_1_1_0::chromatix_pdpc11Type::enable_sectionStruct*>(&(pOEMInput->PDPCDataEnable));

            // Use OEM defined interpolation data
            pInterpolationData =
                reinterpret_cast<pdpc_1_1_0::pdpc11_rgn_dataType*>(&(pOEMInput->PDPCSetting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            commonIQResult = IQInterface::s_interpolationTable.IFEPDPC11CalculateHWSetting(
                pInput, pInterpolationData, pModuleEnable, static_cast<VOID*>(&unpackedRegData));

            if (TRUE == commonIQResult)
            {
                // store pdpc state
                pOutput->pdpcState.pdaf_global_offset_x    = unpackedRegData.pdaf_global_offset_x;
                pOutput->pdpcState.pdaf_x_end              = unpackedRegData.pdaf_x_end;
                pOutput->pdpcState.pdaf_zzHDR_first_rb_exp = unpackedRegData.pdaf_zzHDR_first_rb_exp;
                Utils::Memcpy(pOutput->pdpcState.PDAF_PD_Mask,
                              unpackedRegData.PDAF_PD_Mask,
                              sizeof(unpackedRegData.PDAF_PD_Mask));

                // Avoid PackIQRegisterSetting() for prepare striping input context.
                // This also prevents DMI buffer address NULL pointer check failure in case of dual IFE.
                if (FALSE == pInput->isPrepareStripeInputContext)
                {
                    result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegData), pOutput);
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("IFEPDPC11 calculate HW settings failed.");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("PDPC interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is pInput %p pOutput %p ", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::BLS12CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::BLS12CalculateSetting(
    const BLS12InputData* pInput,
    VOID*                 pOEMIQData,
    BLS12OutputData*      pOutput)
{
    CamxResult                                            result             = CamxResultSuccess;
    ISPHWSetting*                                         pHWSetting         = NULL;
    BOOL                                                  commonIQResult     = FALSE;
    bls_1_2_0::bls12_rgn_dataType*                        pInterpolationData = NULL;
    bls_1_2_0::chromatix_bls12Type::enable_sectionStruct* pModuleEnable      = NULL;

    if ((NULL != pInput) && (NULL != pInput->pHWSetting) && (NULL != pOutput))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pInterpolationData = static_cast<bls_1_2_0::bls12_rgn_dataType*>(pInput->pInterpolationData);
            commonIQResult     = IQInterface::s_interpolationTable.BLS12Interpolation(pInput, pInterpolationData);
            pModuleEnable      = &(pInput->pChromatix->enable_section);
        }
        else
        {
            OEMIFEIQSetting* pOEMInput = static_cast<OEMIFEIQSetting*>(pOEMIQData);

            // Use OEM defined interpolation data
            pInterpolationData = reinterpret_cast<bls_1_2_0::bls12_rgn_dataType*>(&(pOEMInput->BLSSetting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
                pModuleEnable  = reinterpret_cast<bls_1_2_0::chromatix_bls12Type::enable_sectionStruct*>(
                                     &(pOEMInput->BLSEnableSection));
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            BLS12UnpackedField unpackedRegisterData;

            commonIQResult = IQInterface::s_interpolationTable.BLS12CalculateHWSetting(
                                 pInput,
                                 pInterpolationData,
                                 pModuleEnable,
                                 static_cast<VOID*>(&unpackedRegisterData));

            if (TRUE == commonIQResult)
            {
                result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegisterData), pOutput);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("BLS12 calculate hardware setting failed");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("BLS12 interpolation failed");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is pInput %p pOutput %p ", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::Gamma16CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::Gamma16CalculateSetting(
    const Gamma16InputData*  pInput,
    VOID*                    pOEMIQData,
    Gamma16OutputData*       pOutput,
    VOID*                    pDebugBuffer)
{
    CamxResult                                                result             = CamxResultSuccess;
    BOOL                                                      commonIQResult     = FALSE;
    ISPHWSetting*                                             pHWSetting         = NULL;
    gamma_1_6_0::mod_gamma16_channel_dataType*                pInterpolationData = NULL;
    gamma_1_6_0::mod_gamma16_channel_dataType                 pChanneldata[GammaLUTMax];
    Gamma16UnpackedField                                      unpackedRegData;
    gamma_1_6_0::chromatix_gamma16Type::enable_sectionStruct* pModuleEnable      = NULL;

    Utils::Memset(&unpackedRegData, 0, sizeof(unpackedRegData));
    Utils::Memset(&pChanneldata, 0, sizeof(pChanneldata));
    if ((NULL != pInput) && (NULL != pOutput) && (NULL != pInput->pHWSetting))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);

        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pInterpolationData = &pChanneldata[0];
            commonIQResult     = IQInterface::s_interpolationTable.gamma16Interpolation(pInput, pInterpolationData);
            pModuleEnable      = &(pInput->pChromatix->enable_section);
        }
        else
        {
            if (PipelineType::IFE == pOutput->type)
            {
                OEMIFEIQSetting* pOEMInput = static_cast<OEMIFEIQSetting*>(pOEMIQData);
                pModuleEnable              = reinterpret_cast<gamma_1_6_0::chromatix_gamma16Type::enable_sectionStruct*>(
                    &(pOEMInput->GammaEnableSection));
                // Use OEM defined interpolation data
                pInterpolationData         =
                    reinterpret_cast<gamma_1_6_0::mod_gamma16_channel_dataType*>(&(pOEMInput->GammaSetting[0]));
            }
            else if (PipelineType::BPS == pOutput->type)
            {
                OEMBPSIQSetting* pOEMInput = static_cast<OEMBPSIQSetting*>(pOEMIQData);
                pModuleEnable              = reinterpret_cast<gamma_1_6_0::chromatix_gamma16Type::enable_sectionStruct*>(
                    &(pOEMInput->GammaEnableSection));
                // Use OEM defined interpolation data
                pInterpolationData         =
                    reinterpret_cast<gamma_1_6_0::mod_gamma16_channel_dataType*>(&(pOEMInput->GammaSetting[0]));
            }
            else
            {
                result = CamxResultEInvalidArg;
                CAMX_ASSERT_ALWAYS_MESSAGE("Pipeline Type %d not supported for Gamma16", pOutput->type);
            }

            if ((NULL != pInterpolationData) && (NULL != pModuleEnable))
            {
                commonIQResult = TRUE;
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            unpackedRegData.r_lut_in_cfg.pGammaTable = pOutput->pRDMIDataPtr;
            unpackedRegData.g_lut_in_cfg.pGammaTable = pOutput->pGDMIDataPtr;
            unpackedRegData.b_lut_in_cfg.pGammaTable = pOutput->pBDMIDataPtr;

            commonIQResult =
                IQInterface::s_interpolationTable.gamma16CalculateHWSetting(pInput,
                                                                            pInterpolationData,
                                                                            pModuleEnable,
                                                                            static_cast<VOID*>(&unpackedRegData));

            if (TRUE == commonIQResult)
            {
                if (PipelineType::IFE == pOutput->type)
                {
                    result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegData), pOutput);
                }
                else if (PipelineType::BPS != pOutput->type)
                {
                    result = CamxResultEInvalidArg;
                    CAMX_ASSERT_ALWAYS_MESSAGE("Pipeline Type %d not supported for Gamma16", pOutput->type);
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("Gamma16 calculate HW settings failed.");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("Gamma16 interpolation failed");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Gamma Input data is pInput %p pOutput %p", pInput, pOutput);
    }

    if (NULL != pDebugBuffer && CamxResultSuccess == result)
    {
        Gamma16DebugBuffer* pBuffer = static_cast<Gamma16DebugBuffer*>(pDebugBuffer);

        Utils::Memcpy(&(pBuffer->interpolationResult),
                      pInterpolationData,
                      sizeof(gamma_1_6_0::mod_gamma16_channel_dataType) * GammaLUTMax);
        Utils::Memcpy(&(pBuffer->unpackedData), &unpackedRegData, sizeof(unpackedRegData));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::BPSBPCPDPC20CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::BPSBPCPDPC20CalculateSetting(
    PDPC20IQInput*          pInput,
    VOID*                   pOEMIQData,
    BPSBPCPDPC20OutputData* pOutput,
    PixelFormat             pixelFormat)
{
    CamxResult                                              result             = CamxResultSuccess;
    BOOL                                                    commonIQResult     = FALSE;
    ISPHWSetting*                                           pHWSetting         = NULL;
    pdpc_2_0_0::pdpc20_rgn_dataType*                        pInterpolationData = NULL;
    pdpc_2_0_0::chromatix_pdpc20Type::enable_sectionStruct* pModuleEnable      = NULL;

    if ((NULL != pInput) && (NULL != pOutput) && (NULL != pInput->pHWSetting))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        result = GetPixelFormat(&pixelFormat, &(pInput->bayerPattern));

        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pInterpolationData = static_cast<pdpc_2_0_0::pdpc20_rgn_dataType*>(pInput->pInterpolationData);
            commonIQResult     = IQInterface::s_interpolationTable.BPSPDPC20Interpolation(pInput, pInterpolationData);
            pModuleEnable      = &(pInput->pChromatix->enable_section);
        }
        else
        {
            OEMBPSIQSetting* pOEMInput = static_cast<OEMBPSIQSetting*>(pOEMIQData);

            // Use OEM defined interpolation data
            pInterpolationData = reinterpret_cast<pdpc_2_0_0::pdpc20_rgn_dataType*>(&(pOEMInput->PDPCSetting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
                pModuleEnable  = reinterpret_cast<pdpc_2_0_0::chromatix_pdpc20Type::enable_sectionStruct*>(
                                     &(pOEMInput->PDPCEnableSection));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            PDPC20UnpackedField unpackedRegisterData = { 0 };

            commonIQResult =
                IQInterface::s_interpolationTable.BPSPDPC20CalculateHWSetting(pInput,
                                                                              pInterpolationData,
                                                                              pModuleEnable,
                                                                              static_cast<VOID*>(&unpackedRegisterData));

            if (TRUE == commonIQResult)
            {
                result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegisterData), pOutput);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("BPCPDP Calculate HW setting failed");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("BPCPDP intepolation failed.");
        }
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL %p %p", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQInterface::BPSPDPC30CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::BPSPDPC30CalculateSetting(
    PDPC30IQInput*          pInput,
    VOID*                   pOEMIQData,
    BPSPDPC30OutputData*    pOutput,
    PixelFormat             pixelFormat)
{
    CamxResult                                              result              = CamxResultSuccess;
    BOOL                                                    commonIQResult      = FALSE;
    pdpc_3_0_0::pdpc30_rgn_dataType*                        pInterpolationData  = NULL;
    pdpc_3_0_0::chromatix_pdpc30Type::enable_sectionStruct* pModuleEnable       = NULL;
    ISPHWSetting*                                           pHWSetting          = NULL;

    if ((NULL != pInput) && (NULL != pOutput) && (NULL != pInput->pHWSetting))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        result = GetPixelFormat(&pixelFormat, &(pInput->bayerPattern));

        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pInterpolationData = static_cast<pdpc_3_0_0::pdpc30_rgn_dataType*>(pInput->pInterpolationData);
            commonIQResult = IQInterface::s_interpolationTable.PDPC30Interpolation(pInput, pInterpolationData);
            pModuleEnable = &(pInput->pChromatix->enable_section);
        }
        else
        {
            OEMBPSIQSetting* pOEMInput = static_cast<OEMBPSIQSetting*>(pOEMIQData);

            // Use OEM defined interpolation data
            pInterpolationData = reinterpret_cast<pdpc_3_0_0::pdpc30_rgn_dataType*>(&(pOEMInput->PDPC30Setting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
                pModuleEnable = reinterpret_cast<pdpc_3_0_0::chromatix_pdpc30Type::enable_sectionStruct*>(
                    &(pOEMInput->PDPC30EnableSection));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            PDPC30UnpackedField unpackedRegisterData = { 0 };

            commonIQResult =
                IQInterface::s_interpolationTable.PDPC30CalculateHWSetting(pInput,
                    pInterpolationData,
                    pModuleEnable,
                    static_cast<VOID*>(&unpackedRegisterData));
            if (TRUE == commonIQResult)
            {
                /* setup unpacked info */
                result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegisterData), pOutput);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("BPCPDP Calculate HW setting failed");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("BPCPDP intepolation failed.");
        }
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL %p %p", pInput, pOutput);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IFEPDPC30CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IFEPDPC30CalculateSetting(
    const PDPC30IQInput*    pInput,
    VOID*                   pOEMIQData,
    IFEPDPC30OutputData*    pOutput)
{
    CamxResult                       result                                 = CamxResultSuccess;
    BOOL                             commonIQResult                         = FALSE;
    pdpc_3_0_0::pdpc30_rgn_dataType* pInterpolationData                     = NULL;
    OEMIFEIQSetting*                 pOEMInput                              = NULL;
    ISPHWSetting*                    pHWSetting                             = NULL;
    pdpc_3_0_0::chromatix_pdpc30Type::enable_sectionStruct* pModuleEnable   = NULL;
    PDPC30UnpackedField              unpackedRegData;
    if ((NULL != pInput) && (NULL != pOutput))
    {
        if (NULL == pOEMIQData)
        {
            pInterpolationData = static_cast<pdpc_3_0_0::pdpc30_rgn_dataType*>(pInput->pInterpolationData);
            // Call the Interpolation Calculation
            commonIQResult = IQInterface::s_interpolationTable.PDPC30Interpolation(pInput, pInterpolationData);
            pModuleEnable = &pInput->pChromatix->enable_section;
        }
        else
        {
            pOEMInput = static_cast<OEMIFEIQSetting*>(pOEMIQData);

            pModuleEnable =
                reinterpret_cast<pdpc_3_0_0::chromatix_pdpc30Type::enable_sectionStruct*>(&(pOEMInput->PDPC30EnableSection));

            // Use OEM defined interpolation data
            pInterpolationData =
                reinterpret_cast<pdpc_3_0_0::pdpc30_rgn_dataType*>(&(pOEMInput->PDPC30Setting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            pHWSetting      = static_cast<ISPHWSetting*>(pInput->pHWSetting);
            commonIQResult  = IQInterface::s_interpolationTable.PDPC30CalculateHWSetting(pInput,
                                                                                     pInterpolationData,
                                                                                     pModuleEnable,
                                                                                     static_cast<VOID*>(&unpackedRegData));

            if (TRUE == commonIQResult)
            {
                // store pdpc state
                pOutput->pdpcState.PDAFGlobalOffsetX    = unpackedRegData.PDAFGlobalOffsetX;
                pOutput->pdpcState.PDAFEndX             = unpackedRegData.PDAFXend;
                pOutput->pdpcState.PDAFzzHDRFirstRBExp  = unpackedRegData.PDAFzzHDRFirstrbExposure;
                pOutput->pdpcState.PDAFTableoffsetX     = unpackedRegData.PDAFTableXOffset;
                pOutput->pdpcState.enable               = unpackedRegData.enable;
                pOutput->pdpcState.PDAFPDPCEnable       = unpackedRegData.PDAFPDPCEnable;
                if (FALSE == pInput->isPrepareStripeInputContext)
                {
                    result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegData), pOutput);
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("IFEPDPC30 calculate HW settings failed.");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("PDPC interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is pInput %p pOutput %p ", pInput, pOutput);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::BPSLinearization34CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::BPSLinearization34CalculateSetting(
    const Linearization34IQInput* pInput,
    VOID*                         pOEMIQData,
    Linearization34OutputData*    pOutput)
{
    CamxResult                                                                result             = CamxResultSuccess;
    BOOL                                                                      commonIQResult     = FALSE;
    ISPHWSetting*                                                             pHWSetting         = NULL;
    linearization_3_4_0::linearization34_rgn_dataType*                        pInterpolationData = NULL;
    linearization_3_4_0::chromatix_linearization34Type::enable_sectionStruct* pModuleEnable      = NULL;

    if ((NULL != pInput) && (NULL != pOutput) && (NULL != pInput->pHWSetting))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pInterpolationData = static_cast<linearization_3_4_0::linearization34_rgn_dataType*>(pInput->pInterpolationData);
            commonIQResult     = IQInterface::s_interpolationTable.Linearization34Interpolation(pInput, pInterpolationData);
            pModuleEnable      = &(pInput->pChromatix->enable_section);
        }
        else
        {
            OEMBPSIQSetting* pOEMInput = static_cast<OEMBPSIQSetting*>(pOEMIQData);

            // Use OEM defined interpolation data
            pInterpolationData =
                reinterpret_cast<linearization_3_4_0::linearization34_rgn_dataType*>(&(pOEMInput->LinearizationSetting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
                pModuleEnable  = reinterpret_cast<linearization_3_4_0::chromatix_linearization34Type::enable_sectionStruct*>(
                                     &(pOEMInput->LinearizationEnableSection));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            Linearization34UnpackedField unpackedRegisterData = { 0 };

            // Calculate the remaining Gain to apply in Demux
            if (FALSE == pInput->pedestalEnable)
            {
                FLOAT maxLUTValue = static_cast<FLOAT>(Linearization34MaximumLUTVal);

                pOutput->stretchGainRed  =
                    static_cast<FLOAT>(maxLUTValue / (maxLUTValue - pInterpolationData->r_lut_p_tab.r_lut_p[0]));
                pOutput->stretchGainBlue =
                    static_cast<FLOAT>(maxLUTValue / (maxLUTValue - pInterpolationData->b_lut_p_tab.b_lut_p[0]));
                if (PixelFormat::BayerRGGB == static_cast<PixelFormat>(pInput->bayerPattern) ||
                    PixelFormat::BayerGRBG == static_cast<PixelFormat>(pInput->bayerPattern))
                {
                    pOutput->stretchGainGreenEven =
                        static_cast<FLOAT>(maxLUTValue / (maxLUTValue - pInterpolationData->gr_lut_p_tab.gr_lut_p[0]));
                    pOutput->stretchGainGreenOdd  =
                        static_cast<FLOAT>(maxLUTValue / (maxLUTValue - pInterpolationData->gb_lut_p_tab.gb_lut_p[0]));
                }
                else if (PixelFormat::BayerBGGR == static_cast<PixelFormat>(pInput->bayerPattern) ||
                    PixelFormat::BayerGBRG == static_cast<PixelFormat>(pInput->bayerPattern))
                {
                    pOutput->stretchGainGreenEven =
                        static_cast<FLOAT>(maxLUTValue / (maxLUTValue - pInterpolationData->gb_lut_p_tab.gb_lut_p[0]));
                    pOutput->stretchGainGreenOdd  =
                        static_cast<FLOAT>(maxLUTValue / (maxLUTValue - pInterpolationData->gr_lut_p_tab.gr_lut_p[0]));
                }
            }

            commonIQResult = IQInterface::s_interpolationTable.Linearization34CalculateHWSetting(
                                 pInput,
                                 pInterpolationData,
                                 pModuleEnable,
                                 static_cast<VOID*>(&unpackedRegisterData));


            if (TRUE == commonIQResult)
            {
                pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegisterData), pOutput);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("Linearization34 calculate HW settings failed.");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("Linearization34 interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is pInput %p pOutput %p", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQInterface::IFELinearization34CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IFELinearization34CalculateSetting(
    const Linearization34IQInput* pInput,
    VOID*                         pOEMIQData,
    Linearization34OutputData*    pOutput)
{
    CamxResult                                                                result             = CamxResultSuccess;
    ISPHWSetting*                                                             pHWSetting         = NULL;
    BOOL                                                                      commonIQResult     = FALSE;
    linearization_3_4_0::linearization34_rgn_dataType*                        pInterpolationData = NULL;
    linearization_3_4_0::chromatix_linearization34Type::enable_sectionStruct* pModuleEnable      = NULL;

    if ((NULL != pInput) && (NULL != pInput->pHWSetting) && (NULL != pOutput))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pInterpolationData = static_cast<linearization_3_4_0::linearization34_rgn_dataType*>(pInput->pInterpolationData);
            commonIQResult     = IQInterface::s_interpolationTable.Linearization34Interpolation(pInput, pInterpolationData);
            pModuleEnable      = &(pInput->pChromatix->enable_section);
        }
        else
        {
            OEMIFEIQSetting* pOEMInput = static_cast<OEMIFEIQSetting*>(pOEMIQData);

            // Use OEM defined interpolation data
            pInterpolationData =
                reinterpret_cast<linearization_3_4_0::linearization34_rgn_dataType*>(&(pOEMInput->LinearizationSetting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
                pModuleEnable = reinterpret_cast<linearization_3_4_0::chromatix_linearization34Type::enable_sectionStruct*>(
                    &(pOEMInput->LinearizationEnable));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            Linearization34UnpackedField unpackedRegisterData = { 0 };

            // Calculate the remaining Gain to apply in Demux
            if (FALSE == pInput->pedestalEnable)
            {
                FLOAT maxLUTValue = static_cast<FLOAT>(Linearization34MaximumLUTVal);

                pOutput->stretchGainRed =
                    static_cast<FLOAT>(maxLUTValue / (maxLUTValue - pInterpolationData->r_lut_p_tab.r_lut_p[0]));
                pOutput->stretchGainBlue =
                    static_cast<FLOAT>(maxLUTValue / (maxLUTValue - pInterpolationData->b_lut_p_tab.b_lut_p[0]));
                if (PixelFormat::BayerRGGB == static_cast<PixelFormat>(pInput->bayerPattern) ||
                    PixelFormat::BayerGRBG == static_cast<PixelFormat>(pInput->bayerPattern))
                {
                    pOutput->stretchGainGreenEven =
                        static_cast<FLOAT>(maxLUTValue / (maxLUTValue - pInterpolationData->gr_lut_p_tab.gr_lut_p[0]));
                    pOutput->stretchGainGreenOdd =
                        static_cast<FLOAT>(maxLUTValue / (maxLUTValue - pInterpolationData->gb_lut_p_tab.gb_lut_p[0]));
                }
                else if (PixelFormat::BayerBGGR == static_cast<PixelFormat>(pInput->bayerPattern) ||
                    PixelFormat::BayerGBRG == static_cast<PixelFormat>(pInput->bayerPattern))
                {
                    pOutput->stretchGainGreenEven =
                        static_cast<FLOAT>(maxLUTValue / (maxLUTValue - pInterpolationData->gb_lut_p_tab.gb_lut_p[0]));
                    pOutput->stretchGainGreenOdd =
                        static_cast<FLOAT>(maxLUTValue / (maxLUTValue - pInterpolationData->gr_lut_p_tab.gr_lut_p[0]));
                }
            }

            commonIQResult = IQInterface::s_interpolationTable.Linearization34CalculateHWSetting(
                pInput,
                pInterpolationData,
                pModuleEnable,
                static_cast<VOID*>(&unpackedRegisterData));


            if (TRUE == commonIQResult)
            {
                pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegisterData), pOutput);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("Linearization34 calculate HW settings failed.");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("Linearization34 interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is pInput %p pOutput %p", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IFEABF34CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IFEABF34CalculateSetting(
    const ABF34InputData* pInput,
    VOID*                 pOEMIQData,
    ABF34OutputData*      pOutput)
{
    CamxResult                                            result              = CamxResultSuccess;
    BOOL                                                  commonIQResult      = FALSE;
    ISPHWSetting*                                         pHWSetting          = NULL;
    abf_3_4_0::abf34_rgn_dataType*                        pInterpolationData  = NULL;
    OEMIFEIQSetting*                                      pOEMInput           = NULL;
    abf_3_4_0::chromatix_abf34Type::enable_sectionStruct* pModuleEnable       = NULL;
    abf_3_4_0::chromatix_abf34_reserveType*               pReserveType        = NULL;
    ABF34UnpackedField                                    unpackedRegData;

    if ((NULL != pInput) && (NULL != pInput->pHWSetting) && (NULL != pOutput))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pInterpolationData = static_cast<abf_3_4_0::abf34_rgn_dataType*>(pInput->pInterpolationData);

            commonIQResult     = IQInterface::s_interpolationTable.IFEABF34Interpolation(pInput, pInterpolationData);
            pModuleEnable      = &(pInput->pChromatix->enable_section);
            pReserveType       = &(pInput->pChromatix->chromatix_abf34_reserve);
        }
        else
        {
            pOEMInput = static_cast<OEMIFEIQSetting*>(pOEMIQData);
            // Use OEM defined interpolation data
            pInterpolationData =
                reinterpret_cast<abf_3_4_0::abf34_rgn_dataType*>(&(pOEMInput->ABFSetting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
                pModuleEnable  =
                    reinterpret_cast<abf_3_4_0::chromatix_abf34Type::enable_sectionStruct*>(&(pOEMInput->ABFEnableSection));
                pReserveType   =
                    reinterpret_cast<abf_3_4_0::chromatix_abf34_reserveType*>(&(pOEMInput->ABFReserveType));
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            commonIQResult = IQInterface::s_interpolationTable.IFEABF34CalculateHWSetting(
                pInput, pInterpolationData, pModuleEnable, pReserveType, static_cast<VOID*>(&unpackedRegData));

            if (TRUE == commonIQResult)
            {
                result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegData), pOutput);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupPProc, "ABF calculate HW setting failed.");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "ABF interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Input data is  pInput %p pOutput %p ", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::GTM10CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::GTM10CalculateSetting(
    GTM10InputData*   pInput,
    VOID*             pOEMIQData,
    GTM10OutputData*  pOutput,
    TMC10InputData*   pTMCInput)
{

    CamxResult                                            result             = CamxResultSuccess;
    BOOL                                                  commonIQResult     = FALSE;
    ISPHWSetting*                                         pHWSetting         = NULL;
    gtm_1_0_0::gtm10_rgn_dataType*                        pInterpolationData = NULL;
    gtm_1_0_0::chromatix_gtm10_reserveType*               pReserveType       = NULL;
    gtm_1_0_0::chromatix_gtm10Type::enable_sectionStruct* pModuleEnable      = NULL;

    if ((NULL != pInput) && (NULL != pOutput) && (NULL != pInput->pHWSetting))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        if (NULL == pOEMIQData)
        {
            pInterpolationData = static_cast<gtm_1_0_0::gtm10_rgn_dataType*>(pInput->pInterpolationData);
            // Call the Interpolation Calculation
            commonIQResult = IQInterface::s_interpolationTable.GTM10Interpolation(pInput, pInterpolationData);

            if ((TRUE == commonIQResult) &&
                (TRUE == pTMCInput->adrcGTMEnable))
            {
                if (SWTMCVersion::TMC10 == pTMCInput->pAdrcOutputData->version)
                {
                    commonIQResult = IQInterface::GetADRCData(pTMCInput);
                }
                pInput->pAdrcInputData = pTMCInput->pAdrcOutputData;
            }

            pReserveType       = &(pInput->pChromatix->chromatix_gtm10_reserve);
            pModuleEnable      = &(pInput->pChromatix->enable_section);
        }
        else
        {
            if (PipelineType::IFE == pOutput->type)
            {
                OEMIFEIQSetting* pOEMInput = static_cast<OEMIFEIQSetting*>(pOEMIQData);
                // Use OEM defined interpolation data
                pInterpolationData = reinterpret_cast<gtm_1_0_0::gtm10_rgn_dataType*>(&(pOEMInput->GTMSetting));
                pReserveType = reinterpret_cast<gtm_1_0_0::chromatix_gtm10_reserveType*>(
                    &(pOEMInput->GTMReserveType));
                pModuleEnable = reinterpret_cast<gtm_1_0_0::chromatix_gtm10Type::enable_sectionStruct*>(
                    &(pOEMInput->GTMEnableSection));
            }
            else if (PipelineType::BPS == pOutput->type)
            {
                OEMBPSIQSetting* pOEMInput = static_cast<OEMBPSIQSetting*>(pOEMIQData);
                // Use OEM defined interpolation data
                pInterpolationData = reinterpret_cast<gtm_1_0_0::gtm10_rgn_dataType*>(&(pOEMInput->GTMSetting));
                pReserveType = reinterpret_cast<gtm_1_0_0::chromatix_gtm10_reserveType*>(
                    &(pOEMInput->GTMReserveType));
                pModuleEnable = reinterpret_cast<gtm_1_0_0::chromatix_gtm10Type::enable_sectionStruct*>(
                    &(pOEMInput->GTMEnableSection));
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Pipeline Type %d not supported for Demux13", pOutput->type);
            }

            if ((NULL != pInterpolationData) && (NULL != pReserveType) && (NULL !=  pModuleEnable))
            {
                commonIQResult = TRUE;
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            GTM10UnpackedField unpackedRegisterData;

            commonIQResult = IQInterface::s_interpolationTable.GTM10CalculateHWSetting(
                                 pInput,
                                 pInterpolationData,
                                 pReserveType,
                                 pModuleEnable,
                                 static_cast<VOID*>(&unpackedRegisterData));

            if (TRUE == commonIQResult)
            {
                result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegisterData), pOutput);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupISP, "GTM10 calculate hardware setting failed");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupISP, "GTM10 interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Data is pInput %p pOutput %p", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CC13ApplyEffect
///
/// @brief  Apply effects if any to the interpolated CC values
///
/// @param  pInput   Pointer to the input data
/// @param  pData    Pointer to the interpolated data to be updated
///
/// @return VOID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID CC13ApplyEffect(
    const CC13InputData*         pInput,
    cc_1_3_0::cc13_rgn_dataType* pData)
{
    UINT8 matrixSize = 3;
    FLOAT out[3][3];
    FLOAT inputMatrix[3][3];

    // Copy interpolated data into a 3x3 matrix
    inputMatrix[0][0] = pData->c_tab.c[0];
    inputMatrix[0][1] = pData->c_tab.c[1];
    inputMatrix[0][2] = pData->c_tab.c[2];
    inputMatrix[1][0] = pData->c_tab.c[3];
    inputMatrix[1][1] = pData->c_tab.c[4];
    inputMatrix[1][2] = pData->c_tab.c[5];
    inputMatrix[2][0] = pData->c_tab.c[6];
    inputMatrix[2][1] = pData->c_tab.c[7];
    inputMatrix[2][2] = pData->c_tab.c[8];

    // Matrix multiplication of effects and interpolated data
    for (UINT8 i = 0; i < matrixSize; i++)
    {
        for (UINT8 j = 0; j < matrixSize; j++)
        {
            out[i][j] = 0;
            for (UINT8 k = 0; k < matrixSize; k++)
            {
                out[i][j] += (pInput->effectsMatrix[i][k] * inputMatrix[k][j]);
            }
        }
    }

    // Copy the updated co-efficients with applied effect into interpolated data
    pData->c_tab.c[0] = out[0][0];
    pData->c_tab.c[1] = out[0][1];
    pData->c_tab.c[2] = out[0][2];
    pData->c_tab.c[3] = out[1][0];
    pData->c_tab.c[4] = out[1][1];
    pData->c_tab.c[5] = out[1][2];
    pData->c_tab.c[6] = out[2][0];
    pData->c_tab.c[7] = out[2][1];
    pData->c_tab.c[8] = out[2][2];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::CC13CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::CC13CalculateSetting(
    const CC13InputData* pInput,
    VOID*                pOEMIQData,
    CC13OutputData*      pOutput)
{
    CamxResult                                          result             = CamxResultSuccess;
    BOOL                                                commonIQResult     = FALSE;
    ISPHWSetting*                                       pHWSetting         = NULL;
    cc_1_3_0::cc13_rgn_dataType*                        pInterpolationData = NULL;
    cc_1_3_0::chromatix_cc13_reserveType*               pReserveType       = NULL;
    cc_1_3_0::chromatix_cc13Type::enable_sectionStruct* pModuleEnable      = NULL;

    if ((NULL != pInput) && (NULL != pOutput))
    {
        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pInterpolationData = static_cast<cc_1_3_0::cc13_rgn_dataType*>(pInput->pInterpolationData);
            commonIQResult     = IQInterface::s_interpolationTable.CC13Interpolation(pInput, pInterpolationData);
            pReserveType       = &(pInput->pChromatix->chromatix_cc13_reserve);
            pModuleEnable      = &(pInput->pChromatix->enable_section);

            // Apply effect only for non-OEM case since interpolation data is modified by effect matrix.
            if ((PipelineType::IPE == pOutput->type) && (DefaultSaturation != pInput->saturation))
            {
                CC13ApplyEffect(pInput, pInterpolationData);
            }
        }
        else
        {
            if (PipelineType::BPS == pOutput->type)
            {
                OEMBPSIQSetting* pOEMInput  = static_cast<OEMBPSIQSetting*>(pOEMIQData);
                pInterpolationData          = reinterpret_cast<cc_1_3_0::cc13_rgn_dataType*>(&(pOEMInput->CCSetting));
                if (NULL != pInterpolationData)
                {
                    commonIQResult = TRUE;
                    pReserveType   = reinterpret_cast<cc_1_3_0::chromatix_cc13_reserveType*>(
                                        &(pOEMInput->CCReserveType));
                    pModuleEnable  = reinterpret_cast<cc_1_3_0::chromatix_cc13Type::enable_sectionStruct*>(
                                        &(pOEMInput->CCEnableSection));
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Interpolation Data");
                }
            }
            else if (PipelineType::IPE == pOutput->type)
            {
                OEMIPEIQSetting* pOEMInput  = static_cast<OEMIPEIQSetting*>(pOEMIQData);
                pInterpolationData          = reinterpret_cast<cc_1_3_0::cc13_rgn_dataType*>(&(pOEMInput->CCSetting));
                if (NULL != pInterpolationData)
                {
                    commonIQResult = TRUE;
                    pReserveType   = reinterpret_cast<cc_1_3_0::chromatix_cc13_reserveType*>(
                                        &(pOEMInput->CCReserveType));
                    pModuleEnable  = reinterpret_cast<cc_1_3_0::chromatix_cc13Type::enable_sectionStruct*>(
                                        &(pOEMInput->CCEnableSection));
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Interpolation Data");
                }
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Pipeline Type %d not supported for CC13", pOutput->type);
            }
        }

        if (TRUE == commonIQResult)
        {
            CC13UnpackedField unpackedRegData = { 0 };

            commonIQResult =
                IQInterface::s_interpolationTable.CC13CalculateHWSetting(pInput,
                                                                         pInterpolationData,
                                                                         pReserveType,
                                                                         pModuleEnable,
                                                                         static_cast<VOID*>(&unpackedRegData));
            if (TRUE == commonIQResult)
            {
                if (((PipelineType::IFE == pOutput->type)  ||
                     (PipelineType::IPE == pOutput->type)  ||
                     (PipelineType::BPS == pOutput->type)) &&
                     (NULL != pInput->pHWSetting))
                {
                    pHWSetting  = static_cast<ISPHWSetting*>(pInput->pHWSetting);
                    result      = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegData), pOutput);
                }
                else
                {
                    CAMX_ASSERT_ALWAYS_MESSAGE("Pipeline Type %d not supported for CC1_3", pOutput->type);
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("CC13 calculate hardware setting failed");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("CC13 interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is pInput %p pOutput  %p  ", pInput, pOutput);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IPEGamma15CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IPEGamma15CalculateSetting(
    const Gamma15InputData* pInput,
    VOID*                   pOEMIQData,
    Gamma15OutputData*      pOutput)
{
    CamxResult                                                result                = CamxResultSuccess;
    BOOL                                                      commonIQResult        = FALSE;
    gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct*    pInterpolationData    = NULL;
    gamma_1_5_0::mod_gamma15_channel_dataType                 channelData[GammaLUTMax];
    gamma_1_5_0::chromatix_gamma15Type::enable_sectionStruct* pModuleEnable         = NULL;

    // Check pInput->pInterpolationData also before dereferencing it.
    if ((NULL != pInput) && (NULL != pOutput) && (NULL != pInput->pInterpolationData))
    {
        Gamma15UnpackedField unpackedRegisterData = { 0 };

        // set the unpacked register data LUT pointers to DMI memory buffer
        unpackedRegisterData.gLUTInConfig.pGammaTable = pOutput->pLUT[GammaLUTChannel0];
        unpackedRegisterData.gLUTInConfig.numEntries  = Gamma15LUTNumEntriesPerChannel;
        unpackedRegisterData.bLUTInConfig.pGammaTable = pOutput->pLUT[GammaLUTChannel1];
        unpackedRegisterData.bLUTInConfig.numEntries  = Gamma15LUTNumEntriesPerChannel;
        unpackedRegisterData.rLUTInConfig.pGammaTable = pOutput->pLUT[GammaLUTChannel2];
        unpackedRegisterData.rLUTInConfig.numEntries  = Gamma15LUTNumEntriesPerChannel;

        pInterpolationData =
            static_cast<gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct*>(pInput->pInterpolationData);
        if (NULL != pInterpolationData)
        {
            if (NULL == pOEMIQData)
            {
                // allocate memory for channel data
                pInterpolationData->mod_gamma15_channel_data  = &channelData[0];
                pModuleEnable      = &(pInput->pChromatix->enable_section);

                // Call the Interpolation Calculation
                commonIQResult = IQInterface::s_interpolationTable.gamma15Interpolation(pInput, pInterpolationData);
            }
            else
            {
                OEMIPEIQSetting* pOEMInput = static_cast<OEMIPEIQSetting*>(pOEMIQData);

                pInterpolationData->mod_gamma15_channel_dataID    =
                    pOEMInput->GammaSetting.cct_data.mod_gamma15_channel_dataID;
                pInterpolationData->mod_gamma15_channel_dataCount =
                    pOEMInput->GammaSetting.cct_data.mod_gamma15_channel_dataCount;
                // Allocate memory
                pInterpolationData->mod_gamma15_channel_data = &channelData[0];

                Utils::Memcpy(channelData,
                    pOEMInput->GammaSetting.cct_data.mod_gamma15_channel_data,
                    sizeof(pOEMInput->GammaSetting.cct_data.mod_gamma15_channel_data));

                commonIQResult = TRUE;
                pModuleEnable  = reinterpret_cast<gamma_1_5_0::chromatix_gamma15Type::enable_sectionStruct*>(
                    &(pOEMInput->GammaEnableSection));
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("Null Interpolation data");
        }

        if (TRUE == commonIQResult)
        {
            if (pOutput->type == PipelineType::IPE)
            {
                IQInterface::s_interpolationTable.gamma15CalculateHWSetting(pInput,
                                                                            pInterpolationData,
                                                                            pModuleEnable,
                                                                            static_cast<VOID*>(&unpackedRegisterData));
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Pipeline Type %d not supported for Gamma1_5", pOutput->type);
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("Gamma1_5 interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL %p %p ", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::HNR10CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::HNR10CalculateSetting(
    const HNR10InputData* pInput,
    VOID*                 pOEMIQData,
    HNR10OutputData*      pOutput)
{
    CamxResult                                            result             = CamxResultSuccess;
    BOOL                                                  commonIQResult     = FALSE;
    hnr_1_0_0::hnr10_rgn_dataType*                        pInterpolationData = NULL;
    ISPHWSetting*                                         pHWSetting         = NULL;
    hnr_1_0_0::chromatix_hnr10_reserveType*               pReserveType       = NULL;
    hnr_1_0_0::chromatix_hnr10Type::enable_sectionStruct* pModuleEnable      = NULL;

    if ((NULL != pInput) && (NULL != pOutput) && (NULL != pInput->pHWSetting))
    {
        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pInterpolationData = static_cast<hnr_1_0_0::hnr10_rgn_dataType*>(pInput->pInterpolationData);
            commonIQResult     = IQInterface::s_interpolationTable.HNR10Interpolation(pInput, pInterpolationData);
            pReserveType       = &(pInput->pChromatix->chromatix_hnr10_reserve);
            pModuleEnable      = &(pInput->pChromatix->enable_section);
        }
        else
        {
            OEMBPSIQSetting* pOEMInput = static_cast<OEMBPSIQSetting*>(pOEMIQData);

            // Use OEM defined interpolation data
            pInterpolationData = reinterpret_cast<hnr_1_0_0::hnr10_rgn_dataType*>(&(pOEMInput->HNRSetting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
                pReserveType   = reinterpret_cast<hnr_1_0_0::chromatix_hnr10_reserveType*>(
                                     &(pOEMInput->HNRReserveType));
                pModuleEnable  = reinterpret_cast<hnr_1_0_0::chromatix_hnr10Type::enable_sectionStruct*>(
                                     &(pOEMInput->HNREnableSection));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            HNR10UnpackedField unpackedRegData;
            Utils::Memset(&unpackedRegData, 0, sizeof(unpackedRegData));

            commonIQResult =
                IQInterface::s_interpolationTable.HNR10CalculateHWSetting(pInput,
                                                                          pInterpolationData,
                                                                          pReserveType,
                                                                          pModuleEnable,
                                                                          static_cast<VOID*>(&unpackedRegData));

            if (TRUE == commonIQResult)
            {
                pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
                result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegData), pOutput);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("HNR10 Calculate HW setting Failed");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("HNR10 interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is pInput %p pOutput %p", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::BPSGIC30CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::BPSGIC30CalculateSetting(
    const GIC30InputData*  pInput,
    VOID*                  pOEMIQData,
    GIC30OutputData*       pOutput)
{
    CamxResult                                            result             = CamxResultSuccess;
    BOOL                                                  commonIQResult     = FALSE;
    ISPHWSetting*                                         pHWSetting         = NULL;
    gic_3_0_0::gic30_rgn_dataType*                        pInterpolationData = NULL;
    gic_3_0_0::chromatix_gic30_reserveType*               pReserveType       = NULL;
    gic_3_0_0::chromatix_gic30Type::enable_sectionStruct* pModuleEnable      = NULL;

    if ((NULL != pInput) && (NULL != pOutput) && (NULL != pInput->pHWSetting))
    {
        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pInterpolationData = static_cast<gic_3_0_0::gic30_rgn_dataType*>(pInput->pInterpolationData);
            commonIQResult     = IQInterface::s_interpolationTable.BPSGIC30Interpolation(pInput, pInterpolationData);
            pReserveType       = &(pInput->pChromatix->chromatix_gic30_reserve);
            pModuleEnable      = &(pInput->pChromatix->enable_section);
        }
        else
        {
            OEMBPSIQSetting* pOEMInput = static_cast<OEMBPSIQSetting*>(pOEMIQData);

            // Use OEM defined interpolation data
            pInterpolationData = reinterpret_cast<gic_3_0_0::gic30_rgn_dataType*>(&(pOEMInput->GICSetting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
                pReserveType   = reinterpret_cast<gic_3_0_0::chromatix_gic30_reserveType*>(
                                     &(pOEMInput->GICReserveType));
                pModuleEnable  = reinterpret_cast<gic_3_0_0::chromatix_gic30Type::enable_sectionStruct*>(
                                     &(pOEMInput->GICEnableSection));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            GIC30UnpackedField unpackedRegisterData;

            commonIQResult =
                IQInterface::s_interpolationTable.BPSGIC30CalculateHWSetting(pInput,
                                                                             pInterpolationData,
                                                                             pReserveType,
                                                                             pModuleEnable,
                                                                             static_cast<VOID*>(&unpackedRegisterData));

            if (TRUE == commonIQResult)
            {
                pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
                result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegisterData), pOutput);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("GIC30 calculate HW setting failed.");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("GIC30 intepolation failed");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("GIC Input data is pInput %p pOutput  %p ", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IFEHDR20CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IFEHDR20CalculateSetting(
    const HDR20InputData*  pInput,
    VOID*                  pOEMIQData,
    HDR20OutputData*       pOutput)
{
    CamxResult                     result                = CamxResultSuccess;
    BOOL                           commonIQResult        = FALSE;
    ISPHWSetting*                  pHWSetting            = NULL;
    hdr_2_0_0::hdr20_rgn_dataType* pInterpolationData    = NULL;
    OEMIFEIQSetting*               pOEMInput             = NULL;
    hdr_2_0_0::chromatix_hdr20_reserveType* pReserveType = NULL;
    // hdr_2_0_0::hdr20_rgn_dataType  interpolationData;
    HDR20UnpackedField             unpackedRegData;

    if ((NULL != pInput) && (NULL != pInput->pHWSetting) && (NULL != pOutput))
    {
        if (NULL == pOEMIQData)
        {
            pInterpolationData = static_cast<hdr_2_0_0::hdr20_rgn_dataType*>(pInput->pInterpolationData);
            // Call the Interpolation Calculation
            commonIQResult     = IQInterface::s_interpolationTable.IFEHDR20Interpolation(pInput, pInterpolationData);
            pReserveType       = &(pInput->pChromatix->chromatix_hdr20_reserve);
        }
        else
        {
            pOEMInput = static_cast<OEMIFEIQSetting*>(pOEMIQData);
            // Use OEM defined interpolation data
            pInterpolationData =
                reinterpret_cast<hdr_2_0_0::hdr20_rgn_dataType*>(&(pOEMInput->HDRData.HDR20.HDRSetting));
            pReserveType       =
                reinterpret_cast<hdr_2_0_0::chromatix_hdr20_reserveType*>(&(pOEMInput->HDRData.HDR20.HDRReserveType));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            commonIQResult = IQInterface::s_interpolationTable.IFEHDR20CalculateHWSetting(
                pInput, pInterpolationData, pReserveType, static_cast<VOID*>(&unpackedRegData));

            if (TRUE == commonIQResult)
            {
                pOutput->hdr20State.hdr_zrec_first_rb_exp = unpackedRegData.hdr_zrec_first_rb_exp;

                pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
                result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegData), NULL);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("HDR20 calculate hardware setting failed");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("HDR20 intepolation failed");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is pInput %p pOutput  %p ", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::HDR23CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::HDR23CalculateSetting(
    const HDR23InputData*  pInput,
    VOID*                  pOEMIQData,
    HDR23OutputData*       pOutput)
{
    CamxResult                     result                = CamxResultSuccess;
    BOOL                           commonIQResult        = FALSE;
    ISPHWSetting*                  pHWSetting            = NULL;
    hdr_2_3_0::hdr23_rgn_dataType* pInterpolationData    = NULL;
    OEMIFEIQSetting*               pOEMInput             = NULL;
    hdr_2_3_0::chromatix_hdr23_reserveType* pReserveType = NULL;
    // hdr_2_3_0::hdr30_rgn_dataType  interpolationData;
    HDR23UnpackedField             unpackedRegData;

    if ((NULL != pInput) && (NULL != pInput->pHWSetting) && (NULL != pOutput))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        if (NULL == pOEMIQData)
        {
            pInterpolationData = static_cast<hdr_2_3_0::hdr23_rgn_dataType*>(pInput->pInterpolationData);
            // Call the Interpolation Calculation
            commonIQResult     = IQInterface::s_interpolationTable.IFEHDR23Interpolation(pInput, pInterpolationData);
            pReserveType       = &(pInput->pChromatix->chromatix_hdr23_reserve);
        }
        else
        {
            pOEMInput = static_cast<OEMIFEIQSetting*>(pOEMIQData);
            // Use OEM defined interpolation data
            pInterpolationData =
                reinterpret_cast<hdr_2_3_0::hdr23_rgn_dataType*>(&(pOEMInput->HDRData.HDR23.HDRSetting));
            pReserveType =
                reinterpret_cast<hdr_2_3_0::chromatix_hdr23_reserveType*>(&(pOEMInput->HDRData.HDR23.HDRReserveType));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            commonIQResult = IQInterface::s_interpolationTable.IFEHDR23CalculateHWSetting(
                pInput, pInterpolationData, pReserveType, static_cast<VOID*>(&unpackedRegData));

            if (TRUE == commonIQResult)
            {
                pOutput->hdr23State.hdr_zrec_first_rb_exp = unpackedRegData.hdr_zrec_first_rb_exp;
                result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegData), NULL);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("HDR23 calculate hardware setting failed");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("HDR23 intepolation failed");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is pInput %p pOutput  %p ", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::Pedestal13CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::Pedestal13CalculateSetting(
    const Pedestal13InputData* pInput,
    VOID*                      pOEMIQData,
    Pedestal13OutputData*      pOutput)
{
    CamxResult                               result                               = CamxResultSuccess;
    BOOL                                     commonIQResult                       = FALSE;
    ISPHWSetting*                            pHWSetting                           = NULL;
    pedestal_1_3_0::pedestal13_rgn_dataType* pInterpolationData                   = NULL;
    pedestal_1_3_0::chromatix_pedestal13Type::enable_sectionStruct* pModuleEnable = NULL;

    if ((NULL != pInput) && (NULL != pInput->pHWSetting) && (NULL != pOutput))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pInterpolationData = static_cast<pedestal_1_3_0::pedestal13_rgn_dataType*>(pInput->pInterpolationData);;
            commonIQResult     = IQInterface::s_interpolationTable.pedestal13Interpolation(pInput, pInterpolationData);
            pModuleEnable      = &(pInput->pChromatix->enable_section);
        }
        else
        {
            if (PipelineType::IFE == pOutput->type)
            {
                OEMIFEIQSetting* pOEMInput = static_cast<OEMIFEIQSetting*>(pOEMIQData);
                // Use OEM defined interpolation data
                pInterpolationData         = reinterpret_cast<pedestal_1_3_0::pedestal13_rgn_dataType*>(
                                                &(pOEMInput->PedestalSetting));
                pModuleEnable              = reinterpret_cast<pedestal_1_3_0::chromatix_pedestal13Type::enable_sectionStruct*>(
                                                &(pOEMInput->PedestalEnableSection));
            }
            else if (PipelineType::BPS == pOutput->type)
            {
                OEMBPSIQSetting* pOEMInput = static_cast<OEMBPSIQSetting*>(pOEMIQData);
                // Use OEM defined interpolation data
                pInterpolationData         = reinterpret_cast<pedestal_1_3_0::pedestal13_rgn_dataType*>(
                                                &(pOEMInput->PedestalSetting));
                pModuleEnable              = reinterpret_cast<pedestal_1_3_0::chromatix_pedestal13Type::enable_sectionStruct*>(
                                                &(pOEMInput->PedestalEnableSection));
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Pipeline Type %d not supported for Demux13", pOutput->type);
            }

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            Pedestal13UnpackedField unpackedRegisterData;

            commonIQResult =
                IQInterface::s_interpolationTable.pedestal13CalculateHWSetting(pInput,
                                                                               pInterpolationData,
                                                                               pModuleEnable,
                                                                               static_cast<VOID*>(&unpackedRegisterData));

            if (TRUE == commonIQResult)
            {
                if (PipelineType::IFE == pOutput->type)
                {
                    pOutput->pedState.bwidth_l         = static_cast<uint16_t>(unpackedRegisterData.bWidthL);
                    pOutput->pedState.bx_d1_l          = static_cast<uint16_t>(unpackedRegisterData.bxD1L);
                    pOutput->pedState.bx_start_l       = static_cast<uint16_t>(unpackedRegisterData.bxStartL);
                    pOutput->pedState.lx_start_l       = static_cast<uint16_t>(unpackedRegisterData.lxStartL);
                    pOutput->pedState.meshGridBwidth_l = static_cast<uint16_t>(unpackedRegisterData.meshGridbWidthL);

                    result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegisterData), pOutput);
                }
                else if (PipelineType::BPS == pOutput->type)
                {
                    pHWSetting          = static_cast<ISPHWSetting*>(pInput->pHWSetting);
                    result              = pHWSetting->PackIQRegisterSetting(
                                            static_cast<VOID*>(&unpackedRegisterData), pOutput);
                }
                else
                {
                    result = CamxResultEInvalidArg;
                    CAMX_ASSERT_ALWAYS_MESSAGE("invalid Pipeline type.  Type is %d ", pOutput->type);
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("Pedestal13 intepolation failed");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("Pedestal13 interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is pInput %p pOutput  %p ", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IFEWB12CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IFEWB12CalculateSetting(
    const WB12InputData*   pInput,
    WB12OutputData*        pOutput)
{
    BOOL               commonIQResult = FALSE;
    CamxResult         result         = CamxResultSuccess;
    ISPHWSetting*      pHWSetting     = NULL;
    WB12UnpackedField  unpackedRegData;

    if ((NULL != pInput) && (NULL != pInput->pHWSetting) && (NULL != pOutput))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        commonIQResult = IQInterface::s_interpolationTable.WB12CalculateHWSetting(pInput, static_cast<VOID*>(&unpackedRegData));
        if (TRUE == commonIQResult)
        {
            result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegData), pOutput);
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("WB calculate HW setting failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL %p %p ", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::WB13CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::WB13CalculateSetting(
    const WB13InputData*  pInput,
    VOID*                 pOEMIQData,
    WB13OutputData*       pOutput)
{
    BOOL               commonIQResult = FALSE;
    CamxResult         result         = CamxResultSuccess;
    ISPHWSetting*      pHWSetting     = NULL;

    if ((NULL != pInput) && (NULL != pOutput) && (NULL != pInput->pHWSetting))
    {
        WB13UnpackedField                unpackedRegData;
        globalelements::enable_flag_type moduleEnable = TRUE;
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);

        if (NULL != pOEMIQData)
        {
            OEMBPSIQSetting* pOEMInput = static_cast<OEMBPSIQSetting*>(pOEMIQData);
            moduleEnable = pOEMInput->WBEnable;
        }

        commonIQResult = IQInterface::s_interpolationTable.WB13CalculateHWSetting(pInput,
                                                                                  moduleEnable,
                                                                                  static_cast<VOID*>(&unpackedRegData));
        if (TRUE == commonIQResult)
        {
            result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegData), pOutput);
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("WB calculate HW setting failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL %p %p ", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::HDR22CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::HDR22CalculateSetting(
    const HDR22InputData* pInput,
    VOID*                 pOEMIQData,
    HDR22OutputData*      pOutput)
{
    CamxResult                                            result             = CamxResultSuccess;
    BOOL                                                  commonIQResult     = FALSE;
    ISPHWSetting*                                         pHWSetting         = NULL;
    hdr_2_2_0::hdr22_rgn_dataType*                        pInterpolationData = NULL;
    hdr_2_2_0::chromatix_hdr22_reserveType*               pReserveType       = NULL;
    hdr_2_2_0::chromatix_hdr22Type::enable_sectionStruct* pModuleEnable      = NULL;

    if ((NULL != pInput) && (NULL != pOutput))
    {
        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pInterpolationData = static_cast<hdr_2_2_0::hdr22_rgn_dataType*>(pInput->pInterpolationData);
            commonIQResult     = IQInterface::s_interpolationTable.HDR22Interpolation(pInput, pInterpolationData);
            pReserveType       = &(pInput->pChromatix->chromatix_hdr22_reserve);
            pModuleEnable      = &(pInput->pChromatix->enable_section);
        }
        else
        {
            if (PipelineType::IFE == pOutput->type)
            {
                OEMIFEIQSetting* pOEMInput = static_cast<OEMIFEIQSetting*>(pOEMIQData);

                // Use OEM defined interpolation data
                pInterpolationData = reinterpret_cast<hdr_2_2_0::hdr22_rgn_dataType*>(&(pOEMInput->HDRData.HDR22.HDRSetting));

                if (NULL != pInterpolationData)
                {
                    commonIQResult = TRUE;
                    pReserveType   = reinterpret_cast<hdr_2_2_0::chromatix_hdr22_reserveType*>(
                                        &(pOEMInput->HDRData.HDR22.HDRReserveType));
                    pModuleEnable  = reinterpret_cast<hdr_2_2_0::chromatix_hdr22Type::enable_sectionStruct*>(
                                        &(pOEMInput->HDRData.HDR22.HDREnableSection));
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
                }
            }
            else if (PipelineType::BPS == pOutput->type)
            {
                OEMBPSIQSetting* pOEMInput = static_cast<OEMBPSIQSetting*>(pOEMIQData);

                // Use OEM defined interpolation data
                pInterpolationData = reinterpret_cast<hdr_2_2_0::hdr22_rgn_dataType*>(&(pOEMInput->HDRSetting));

                if (NULL != pInterpolationData)
                {
                    commonIQResult = TRUE;
                    pReserveType   = reinterpret_cast<hdr_2_2_0::chromatix_hdr22_reserveType*>(
                                        &(pOEMInput->HDRReserveType));
                    pModuleEnable  = reinterpret_cast<hdr_2_2_0::chromatix_hdr22Type::enable_sectionStruct*>(
                                        &(pOEMInput->HDREnableSection));
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
                }
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Pipeline Type %d not supported for HDR22", pOutput->type);
            }
        }

        if (TRUE == commonIQResult)
        {
            HDR22UnpackedField  unpackedRegisterData;

            commonIQResult =
                IQInterface::s_interpolationTable.HDR22CalculateHWSetting(pInput,
                                                                          pInterpolationData,
                                                                          pReserveType,
                                                                          pModuleEnable,
                                                                          static_cast<VOID*>(&unpackedRegisterData));

            if (TRUE == commonIQResult)
            {
                if (NULL != pInput->pHWSetting)
                {
                    pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);

                    result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegisterData), pOutput);
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("BPSHDR22 calculate HW settings failed.");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("BPSHDR22 intepolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is pInput %p pOutput %p", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::HDR30CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::HDR30CalculateSetting(
    const HDR30InputData* pInput,
    VOID*                 pOEMIQData,
    HDR30OutputData*      pOutput)
{
    CamxResult                                                    result                    = CamxResultSuccess;
    BOOL                                                          commonIQResultHDR30       = FALSE;
    BOOL                                                          commonIQResultBC10        = FALSE;
    ISPHWSetting*                                                 pHWSetting                = NULL;
    hdr_3_0_0::chromatix_hdr30_reserveType*                       pReserveType              = NULL;
    hdr_3_0_0::chromatix_hdr30Type::enable_sectionStruct*         pModuleEnable             = NULL;
    hdr_3_0_0::hdr30_rgn_dataType*                                pHDR30InterpolationData   = NULL;
    bincorr_1_0_0::bincorr10_rgn_dataType*                        pBC10InterpolationData    = NULL;
    bincorr_1_0_0::chromatix_bincorr10Type::enable_sectionStruct* pBC10ModuleEnable         = NULL;
    HDR30UnpackedField                                            unpackedRegisterDataHdr30 = {};
    BC10UnpackedField                                             unpackedRegisterDataBc10  = {};
    HDR30BC10UnpackedField                                        unpackedRegisterData      = {};

    if ((NULL != pInput) && (NULL != pOutput))
    {
        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pHDR30InterpolationData  = static_cast<hdr_3_0_0::hdr30_rgn_dataType*>(pInput->pInterpolationData);
            commonIQResultHDR30      = IQInterface::s_interpolationTable.HDR30Interpolation(pInput, pHDR30InterpolationData);
            pReserveType        = &(pInput->pChromatix->chromatix_hdr30_reserve);
            pModuleEnable       = &(pInput->pChromatix->enable_section);

            pBC10InterpolationData = static_cast<bincorr_1_0_0::bincorr10_rgn_dataType*>(
                pInput->bc10Data.pBC10InterpolationData);
            commonIQResultBC10 = IQInterface::s_interpolationTable.BC10Interpolation(
                &(pInput->bc10Data), pBC10InterpolationData);
            pBC10ModuleEnable = &(pInput->bc10Data.pChromatixBC->enable_section);
        }
        else
        {
            if (PipelineType::BPS == pOutput->type)
            {
                OEMBPSIQSetting* pOEMInput = static_cast<OEMBPSIQSetting*>(pOEMIQData);

                // Use OEM defined interpolation data
                pHDR30InterpolationData =
                    reinterpret_cast<hdr_3_0_0::hdr30_rgn_dataType*>(&(pOEMInput->HDR30Setting));

                if (NULL != pHDR30InterpolationData)
                {
                    commonIQResultHDR30 = TRUE;
                    pModuleEnable = reinterpret_cast<hdr_3_0_0::chromatix_hdr30Type::enable_sectionStruct*>(
                        &(pOEMInput->HDR30EnableSection));
                    pReserveType =
                        reinterpret_cast<hdr_3_0_0::chromatix_hdr30_reserveType*>(&(pOEMInput->HDR30ReserveType));
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_ASSERT_ALWAYS_MESSAGE("No hdr OEM Input Data");
                }

                pBC10InterpolationData =
                    reinterpret_cast<bincorr_1_0_0::bincorr10_rgn_dataType*>(&(pOEMInput->BC10Setting));

                if (NULL != pBC10InterpolationData)
                {
                    commonIQResultBC10 = TRUE;
                    pBC10ModuleEnable = reinterpret_cast<bincorr_1_0_0::chromatix_bincorr10Type::enable_sectionStruct*>(
                        &(pOEMInput->BC10EnableSection));
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_ASSERT_ALWAYS_MESSAGE("No bincorr OEM Input Data");
                }
            }
            else if (PipelineType::IFE == pOutput->type)
            {
                OEMIFEIQSetting* pOEMInput = static_cast<OEMIFEIQSetting*>(pOEMIQData);

                // Use OEM defined interpolation data
                pHDR30InterpolationData =
                    reinterpret_cast<hdr_3_0_0::hdr30_rgn_dataType*>(&(pOEMInput->HDRData.HDR30.HDRSetting));
                pReserveType =
                    reinterpret_cast<hdr_3_0_0::chromatix_hdr30_reserveType*>(&(pOEMInput->HDRData.HDR30.HDRReserveType));

                if (NULL != pHDR30InterpolationData)
                {
                    commonIQResultHDR30 = TRUE;
                    pModuleEnable = reinterpret_cast<hdr_3_0_0::chromatix_hdr30Type::enable_sectionStruct*>(
                        &(pOEMInput->HDRData.HDR30.HDREnableSection));
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_ASSERT_ALWAYS_MESSAGE("No hdr OEM Input Data");
                }

                pBC10InterpolationData =
                    reinterpret_cast<bincorr_1_0_0::bincorr10_rgn_dataType*>(&(pOEMInput->BC10Setting));

                if (NULL != pBC10InterpolationData)
                {
                    commonIQResultBC10 = TRUE;
                    pBC10ModuleEnable = reinterpret_cast<bincorr_1_0_0::chromatix_bincorr10Type::enable_sectionStruct*>(
                        &(pOEMInput->BC10EnableSection));
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_ASSERT_ALWAYS_MESSAGE("No bincorr OEM Input Data");
                }
            }
            else
            {
                result = CamxResultEInvalidArg;
                CAMX_ASSERT_ALWAYS_MESSAGE("Pipeline Type %d not supported ", pOutput->type);
            }
        }

        if (TRUE == commonIQResultHDR30)
        {
            unpackedRegisterData.pUnpackedRegisterDataHDR30 = &unpackedRegisterDataHdr30;

            commonIQResultHDR30 = IQInterface::s_interpolationTable.HDR30CalculateHWSetting(pInput,
                pHDR30InterpolationData,
                pReserveType,
                pModuleEnable,
                static_cast<VOID*>(&unpackedRegisterDataHdr30));

            if (TRUE != commonIQResultHDR30)
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupPProc, "HDR30 Hardware setting failed.");
            }

        }
        else
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "BPSHDR30 intepolation failed.");
        }

        if ((CamxResultSuccess == result) && (commonIQResultBC10 = TRUE))
        {
            unpackedRegisterData.pUnpackedRegisterDataBC10  = &unpackedRegisterDataBc10;

            commonIQResultBC10 = IQInterface::s_interpolationTable.BC10CalculateHWSetting(&pInput->bc10Data,
                pBC10InterpolationData,
                pBC10ModuleEnable,
                static_cast<VOID*>(&unpackedRegisterDataBc10));

            if (TRUE != commonIQResultBC10)
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupPProc, "BC10 Hardware setting failed.");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "BC10 intepolation failed.");
        }


        if (((TRUE == commonIQResultHDR30) || (TRUE == commonIQResultBC10)) && NULL != pInput->pHWSetting &&
            (CamxResultSuccess == result))
        {
            pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
            result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegisterData), pOutput);
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "BC10 calculate HW settings failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Input data is pInput %p pOutput %p", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::SCE11CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::SCE11CalculateSetting(
    const SCE11InputData* pInput,
    VOID*                 pOEMIQData,
    SCE11OutputData*      pOutput)
{
    CamxResult                                            result                = CamxResultSuccess;
    BOOL                                                  commonIQResult        = FALSE;
    sce_1_1_0::sce11_rgn_dataType*                        pInterpolationDataSCE = NULL;
    sce_1_1_0::chromatix_sce11_reserveType*               pReserveType          = NULL;
    sce_1_1_0::chromatix_sce11Type::enable_sectionStruct* pModuleEnable         = NULL;
    ISPHWSetting*                                         pHWSetting            = NULL;

    if ((NULL != pInput) && (NULL != pOutput) && (NULL != pInput->pChromatix) && (NULL != pInput->pHWSetting))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        if (NULL == pOEMIQData)
        {
            pInterpolationDataSCE = static_cast<sce_1_1_0::sce11_rgn_dataType*>(pInput->pInterpolationData);;
            pReserveType          = &(pInput->pChromatix->chromatix_sce11_reserve);
            pModuleEnable         = &(pInput->pChromatix->enable_section);

            // Call the Interpolation Calculation
            commonIQResult = IQInterface::s_interpolationTable.SCE11Interpolation(pInput, pInterpolationDataSCE);
        }
        else
        {
            OEMIPEIQSetting* pOEMInput = static_cast<OEMIPEIQSetting*>(pOEMIQData);

            // Use OEM defined interpolation data
            pInterpolationDataSCE = reinterpret_cast<sce_1_1_0::sce11_rgn_dataType* >(&(pOEMInput->SkinEnhancementSetting));

            if (NULL != pInterpolationDataSCE)
            {
                commonIQResult = TRUE;
                pReserveType   = reinterpret_cast<sce_1_1_0::chromatix_sce11_reserveType*>(
                                     &(pOEMInput->SkinEnhancementReserveType));
                pModuleEnable  = reinterpret_cast<sce_1_1_0::chromatix_sce11Type::enable_sectionStruct*>(
                                     &(pOEMInput->SkinEnhancementEnableSection));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            SCE11UnpackedField unpackedRegisterDataSCE;

            commonIQResult = IQInterface::s_interpolationTable.SCE11CalculateHWSetting(
                                 pInput,
                                 pInterpolationDataSCE,
                                 pReserveType,
                                 pModuleEnable,
                                 static_cast<VOID*>(&unpackedRegisterDataSCE));

            if (TRUE == commonIQResult)
            {
                VOID* pInputData = static_cast<VOID*>(&unpackedRegisterDataSCE);

                pHWSetting->PackIQRegisterSetting(pInputData, NULL);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("SCE CalculateHWSetting failed.");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("SCE intepolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL ");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::ABF40CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::ABF40CalculateSetting(
    ABF40InputData*  pInput,
    VOID*            pOEMIQData,
    ABF40OutputData* pOutput)
{
    CamxResult                                            result                    = CamxResultSuccess;
    BOOL                                                  commonIQResultABF         = FALSE;
    BOOL                                                  commonIQResultBLS         = FALSE;
    abf_4_0_0::abf40_rgn_dataType*                        pInterpolationDataABF     = NULL;
    ABF40UnpackedField                                    unpackedRegisterDataABF   = { 0, };
    BLS12UnpackedField                                    unpackedRegisterDataBLS   = { 0, };
    bls_1_2_0::bls12_rgn_dataType*                        pInterpolationDataBLS     = NULL;
    abf_4_0_0::chromatix_abf40_reserveType*               pReserveType              = NULL;
    abf_4_0_0::chromatix_abf40Type::enable_sectionStruct* pABFModuleEnable          = NULL;
    bls_1_2_0::chromatix_bls12Type::enable_sectionStruct* pBLSModuleEnable          = NULL;
    ISPHWSetting*                                         pHWSetting                = NULL;

    bls_1_2_0::bls12_rgn_dataType   interpolationDataBLS[BLS12MaxmiumNonLeafNode];
    ABF40BLS12UnpackedField         unpackedRegisterData;

    unpackedRegisterData.pUnpackedRegisterDataABF = static_cast<ABF40UnpackedField*>(&unpackedRegisterDataABF);
    unpackedRegisterData.pUnpackedRegisterDataBLS = static_cast<BLS12UnpackedField*>(&unpackedRegisterDataBLS);

    if ((NULL != pInput) && (NULL != pInput->pHWSetting) && (NULL != pOutput))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pInterpolationDataABF = static_cast<abf_4_0_0::abf40_rgn_dataType*>(pInput->pInterpolationData);
            commonIQResultABF     = IQInterface::s_interpolationTable.ABF40Interpolation(pInput,
                                                                                         pInterpolationDataABF);
            pReserveType          = &(pInput->pChromatix->chromatix_abf40_reserve);
            pABFModuleEnable      = &(pInput->pChromatix->enable_section);
            pBLSModuleEnable      = &(pInput->BLSData.pChromatix->enable_section);

            pInterpolationDataBLS = &interpolationDataBLS[0];
            commonIQResultBLS     = IQInterface::s_interpolationTable.BLS12Interpolation(&pInput->BLSData,
                                                                                         pInterpolationDataBLS);
        }
        else
        {
            if (pOutput->type == PipelineType::BPS)
            {
                OEMBPSIQSetting* pOEMInputBPS = static_cast<OEMBPSIQSetting*>(pOEMIQData);

                // Use OEM defined interpolation data for BPS::ABF
                pInterpolationDataABF = reinterpret_cast<abf_4_0_0::abf40_rgn_dataType*>(&(pOEMInputBPS->ABFSetting));

                if (NULL != pInterpolationDataABF)
                {
                    commonIQResultABF = TRUE;
                    pReserveType = reinterpret_cast<abf_4_0_0::chromatix_abf40_reserveType*>(
                        &(pOEMInputBPS->ABFReserveType));
                    pABFModuleEnable = reinterpret_cast<abf_4_0_0::chromatix_abf40Type::enable_sectionStruct*>(
                        &(pOEMInputBPS->ABFEnableSection));
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "No ABF40 OEM Input Data");
                }

                // Use OEM defined interpolation data for BPS::BLS
                pInterpolationDataBLS = reinterpret_cast<bls_1_2_0::bls12_rgn_dataType*>(&(pOEMInputBPS->BLSSetting));

                if (NULL != pInterpolationDataBLS)
                {
                    commonIQResultBLS = TRUE;
                    pBLSModuleEnable = reinterpret_cast<bls_1_2_0::chromatix_bls12Type::enable_sectionStruct*>(
                        &(pOEMInputBPS->BLSEnableSection));
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "No BLS12 OEM Input Data");
                }
            }
            else // IFE
            {
                OEMIFEIQSetting* pOEMInputIFE = static_cast<OEMIFEIQSetting*>(pOEMIQData);

                // Use OEM defined interpolation data for IFE::ABF
                pInterpolationDataABF = reinterpret_cast<abf_4_0_0::abf40_rgn_dataType*>(&(pOEMInputIFE->ABFSetting));

                if (NULL != pInterpolationDataABF)
                {
                    commonIQResultABF = TRUE;
                    pReserveType = reinterpret_cast<abf_4_0_0::chromatix_abf40_reserveType*>(
                        &(pOEMInputIFE->ABFReserveType));
                    pABFModuleEnable = reinterpret_cast<abf_4_0_0::chromatix_abf40Type::enable_sectionStruct*>(
                        &(pOEMInputIFE->ABFEnableSection));
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "No ABF40 OEM Input Data");
                }

                // Use OEM defined interpolation data for IFE::BLS
                pInterpolationDataBLS = reinterpret_cast<bls_1_2_0::bls12_rgn_dataType*>(&(pOEMInputIFE->BLSSetting));

                if (NULL != pInterpolationDataBLS)
                {
                    commonIQResultBLS = TRUE;
                    pBLSModuleEnable = reinterpret_cast<bls_1_2_0::chromatix_bls12Type::enable_sectionStruct*>(
                        &(pOEMInputIFE->BLSEnableSection));
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "No BLS12 OEM Input Data");
                }
            }
        }

        if ((TRUE == commonIQResultABF) || (TRUE == commonIQResultBLS))
        {
            commonIQResultABF =
                IQInterface::s_interpolationTable.ABF40CalculateHWSetting(pInput,
                                                                          pInterpolationDataABF,
                                                                          pReserveType,
                                                                          pABFModuleEnable,
                                                                          static_cast<VOID*>(
                                                                          unpackedRegisterData.pUnpackedRegisterDataABF));
            if (TRUE == commonIQResultABF)
            {
                ABF40UnpackedField* pABFUnpackedField  = unpackedRegisterData.pUnpackedRegisterDataABF;

                pABFUnpackedField->enable              &= pInput->moduleEnable;
                pABFUnpackedField->bilateralEnable     &= pInput->bilateralEnable;
                pABFUnpackedField->crossProcessEnable  &= pInput->crossPlaneEnable;
                pABFUnpackedField->darkDesatEnable     &= pABFUnpackedField->bilateralEnable;
                pABFUnpackedField->darkSmoothEnable    &= pABFUnpackedField->bilateralEnable;
                pABFUnpackedField->directSmoothEnable  &= pInput->directionalSmoothEnable;
                pABFUnpackedField->actAdjEnable        &= static_cast<UINT16>(pInput->activityAdjustEnable                 &
                                                                          pABFUnpackedField->bilateralEnable  &
                                                                          pABFUnpackedField->directSmoothEnable);
                pABFUnpackedField->minmaxEnable        &= pInput->minmaxEnable;
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupPProc, "ABF40 Hardware setting failed.");
            }

            commonIQResultBLS =
                IQInterface::s_interpolationTable.BLS12CalculateHWSetting(&pInput->BLSData,
                                                                          pInterpolationDataBLS,
                                                                          pBLSModuleEnable,
                                                                          static_cast<VOID*>(
                                                                          unpackedRegisterData.pUnpackedRegisterDataBLS));
            if (TRUE == commonIQResultBLS)
            {
                BLS12UnpackedField* pBLSUnpackedField   = unpackedRegisterData.pUnpackedRegisterDataBLS;
                pBLSUnpackedField->enable               &= pInput->BLSEnable;
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupPProc, "BLS12 Hardware setting failed.");
            }
        }

        if ((TRUE == commonIQResultABF) || (TRUE == commonIQResultBLS))
        {
            result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&unpackedRegisterData), pOutput);
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("ABF40 intepolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is pInput %p pOutput  %p ", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IPECS20CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IPECS20CalculateSetting(
    const CS20InputData* pInput,
    VOID*                pOEMIQData,
    VOID*                pOutput)
{
    CamxResult                                          result             = CamxResultSuccess;
    BOOL                                                commonIQResult     = FALSE;
    ISPHWSetting*                                       pHWSetting         = NULL;
    cs_2_0_0::cs20_rgn_dataType*                        pInterpolationData = NULL;
    cs_2_0_0::chromatix_cs20_reserveType*               pReserveType       = NULL;
    cs_2_0_0::chromatix_cs20Type::enable_sectionStruct* pModuleEnable      = NULL;

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pInput)
    {
        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pInterpolationData = static_cast<cs_2_0_0::cs20_rgn_dataType*>(pInput->pInterpolationData);
            pReserveType       = &(pInput->pChromatix->chromatix_cs20_reserve);
            pModuleEnable      = &(pInput->pChromatix->enable_section);

            // Call the Interpolation Calculation
            commonIQResult = IQInterface::s_interpolationTable.IPECS20Interpolation(pInput, pInterpolationData);
        }
        else
        {
            OEMIPEIQSetting* pOEMInput = static_cast<OEMIPEIQSetting*>(pOEMIQData);

            // Use OEM defined interpolation data
            pInterpolationData = reinterpret_cast<cs_2_0_0::cs20_rgn_dataType*>(&(pOEMInput->ChromaSuppressionSetting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
                pReserveType   = reinterpret_cast<cs_2_0_0::chromatix_cs20_reserveType*>(
                                     &(pOEMInput->ChromaSuppressionReserveType));
                pModuleEnable  = reinterpret_cast<cs_2_0_0::chromatix_cs20Type::enable_sectionStruct*>(
                                     &(pOEMInput->ChromaSuppressionEnableSection));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            CS20UnpackedField unpackedRegisterData;

            commonIQResult = IQInterface::s_interpolationTable.IPECS20CalculateHWSetting(
                                 pInput,
                                 pInterpolationData,
                                 pReserveType,
                                 pModuleEnable,
                                 static_cast<VOID*>(&unpackedRegisterData));

            if (TRUE == commonIQResult)
            {
                pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
                result = pHWSetting->PackIQRegisterSetting(&unpackedRegisterData, NULL);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("CS20 calculate hardwware setting failed");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("CS20 interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Data is NULL %p", pInput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPESetupASF30Register
///
/// @brief  Setup ASF30 Register based on common library output
///
/// @param  pInput     Pointer to the Common Library Calculation Input
/// @param  pData      Pointer to the Common Library Calculation Result
/// @param  pOutput    Pointer to the ASF30 output data (register set, DMI data pointer, etc.)
///
/// @return VOID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID IPESetupASF30Register(
    const ASF30InputData*   pInput,
    ASF30UnpackedField*     pData,
    ASF30OutputData*        pOutput)
{
    CAMX_UNREFERENCED_PARAM(pInput);

    if ((NULL != pData) && (NULL != pOutput) && (NULL != pOutput->pDMIDataPtr))
    {
        {
            AsfParameters*  pASFParameters     = pOutput->pAsfParameters;
            ASF_MODULE_CFG* pASFModuleConfig   = &(pASFParameters->moduleCfg);

            pASFModuleConfig->EN               = pData->enable;
            pASFModuleConfig->SP_EFF_EN        = pData->specialEffectEnable;
            pASFModuleConfig->SP_EFF_ABS_EN    = pData->specialEffectAbsoluteEnable;
            pASFModuleConfig->LAYER_1_EN       = pData->layer1Enable;
            pASFModuleConfig->LAYER_2_EN       = pData->layer2Enable;
            pASFModuleConfig->CONTRAST_EN      = pData->contrastEnable;
            pASFModuleConfig->EDGE_ALIGN_EN    = pData->edgeAlignmentEnable;
            pASFModuleConfig->RNR_ENABLE       = pData->radialEnable;
            pASFModuleConfig->NEG_ABS_Y1       = pData->negateAbsoluteY1;
            pASFModuleConfig->SP               = pData->smoothPercentage;
            pASFModuleConfig->CHROMA_GRAD_EN   = pData->chromaGradientEnable;
            pASFModuleConfig->SKIN_EN          = pData->skinEnable;

            pASFParameters->faceVerticalOffset = pData->faceVerticalOffset;
        }

        // Fill up DMI Buffers
        UINT32*  pDMIDataPtr = pOutput->pDMIDataPtr;
        UINT32   data;
        UINT     pos = 0;
        UINT     i;

        for (i = 0; i < DMI_GAINPOSNEG_SIZE; i++)
        {
            data               = static_cast<UINT32>(pData->layer1SoftThresholdLut[i])                  & 0xFF;
            data              |= ((static_cast<UINT32>(pData->layer1GainNegativeLut[i]) << 8)           & 0xFF00);
            data              |= ((static_cast<UINT32>(pData->layer1GainPositiveLut[i]) << 16)          & 0xFF0000);
            data              |= ((static_cast<UINT32>(pData->layer1ActivityNormalizationLut[i]) << 24) & 0xFF000000);
            pDMIDataPtr[pos++] = data;
        }

        for (i = 0; i < DMI_GAINWEIGHT_SIZE; i++)
        {
            data               = static_cast<UINT32>(pData->layer1GainWeightLut[i]) & 0xFF;
            pDMIDataPtr[pos++] = data;
        }

        for (i = 0; i < DMI_SOFT_SIZE; i++)
        {
            data               = static_cast<UINT32>(pData->layer1SoftThresholdWeightLut[i]) & 0xFF;
            pDMIDataPtr[pos++] = data;
        }

        for (i = 0; i < DMI_GAINPOSNEG_SIZE; i++)
        {
            data               = static_cast<UINT32>(pData->layer2SoftThresholdLut[i])                  & 0xFF;
            data              |= ((static_cast<UINT32>(pData->layer2GainNegativeLut[i]) << 8 )          & 0xFF00);
            data              |= ((static_cast<UINT32>(pData->layer2GainPositiveLut[i]) << 16)          & 0xFF0000);
            data              |= ((static_cast<UINT32>(pData->layer2ActivityNormalizationLut[i]) << 24) & 0xFF000000);
            pDMIDataPtr[pos++] = data;
        }

        for (i = 0; i < DMI_GAINWEIGHT_SIZE; i++)
        {
            data               = static_cast<UINT32>(pData->layer2GainWeightLut[i]) & 0xFF;
            pDMIDataPtr[pos++] = data;
        }

        for (i = 0; i < DMI_SOFT_SIZE; i++)
        {
            data               = static_cast<UINT32>(pData->layer2SoftThresholdWeightLut[i]) & 0xFF;
            pDMIDataPtr[pos++] = data;
        }

        for (i = 0; i < DMI_GAINCHROMA_SIZE; i++)
        {
            data               = static_cast<UINT32>(pData->chromaGradientNegativeLut[i])          & 0xFF;
            data              |= ((static_cast<UINT32>(pData->chromaGradientPositiveLut[i]) << 8 ) & 0xFF00);
            pDMIDataPtr[pos++] = data;
        }

        for (i = 0; i < DMI_GAINCONTRAST_SIZE; i++)
        {
            data               = static_cast<UINT32>(pData->gainContrastNegativeLut[i])          & 0xFF;
            data              |= ((static_cast<UINT32>(pData->gainContrastPositiveLut[i]) << 8 ) & 0xFF00);
            pDMIDataPtr[pos++] = data;
        }

        for (i = 0; i < DMI_SKIN_SIZE; i++)
        {
            data               = static_cast<UINT32>(pData->skinGainLut[i])             & 0xFF;
            data              |= ((static_cast<UINT32>(pData->skinActivityLut[i]) << 8) & 0xFF00);
            pDMIDataPtr[pos++] = data;
        }
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL ");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IPEASF30CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IPEASF30CalculateSetting(
    const ASF30InputData* pInput,
    VOID*                 pOEMIQData,
    ASF30OutputData*      pOutput)
{
    CamxResult                                            result             = CamxResultSuccess;
    BOOL                                                  commonIQResult     = FALSE;
    asf_3_0_0::asf30_rgn_dataType*                        pInterpolationData = NULL;
    asf_3_0_0::chromatix_asf30_reserveType*               pReserveType       = NULL;
    asf_3_0_0::chromatix_asf30Type::enable_sectionStruct* pModuleEnable      = NULL;
    ISPHWSetting*                                         pHWSetting         = NULL;

    if ((NULL != pInput) && (NULL != pOutput) && (NULL != pInput->pHWSetting))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pInterpolationData = static_cast<asf_3_0_0::asf30_rgn_dataType*>(pInput->pInterpolationData);
            pReserveType       = &(pInput->pChromatix->chromatix_asf30_reserve);
            pModuleEnable      = &(pInput->pChromatix->enable_section);

            commonIQResult     = IQInterface::s_interpolationTable.ASF30Interpolation(pInput, pInterpolationData);
        }
        else
        {
            OEMIPEIQSetting* pOEMInput = static_cast<OEMIPEIQSetting*>(pOEMIQData);

            // Use OEM defined interpolation data
            pInterpolationData = reinterpret_cast<asf_3_0_0::asf30_rgn_dataType*>(&(pOEMInput->ASFSetting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
                pReserveType   = reinterpret_cast<asf_3_0_0::chromatix_asf30_reserveType*>(
                                     &(pOEMInput->ASFReserveType));
                pModuleEnable  = reinterpret_cast<asf_3_0_0::chromatix_asf30Type::enable_sectionStruct*>(
                                     &(pOEMInput->ASFEnableSection));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            ASF30UnpackedField unpackedRegisterData = { 0 };

            if (NULL == pOEMIQData)
            {
                // @todo (CAMX-2933) IQ vendor tags settings testing:
                //
                // Seems like the following control flags are sp_read across multiple places,
                // perhaps due to older datastructures. Hence, populating the reserved field
                // contents from Input field.
                //
                pReserveType->edge_alignment_enable = pInput->edgeAlignEnable;
                pReserveType->layer_1_enable        = pInput->layer1Enable;
                pReserveType->layer_2_enable        = pInput->layer2Enable;
                pReserveType->radial_enable         = pInput->radialEnable;
                pReserveType->contrast_enable       = pInput->contrastEnable;
            }

            commonIQResult = IQInterface::s_interpolationTable.ASF30CalculateHWSetting(
                                 pInput,
                                 pInterpolationData,
                                 pReserveType,
                                 pModuleEnable,
                                 static_cast<VOID*>(&unpackedRegisterData));

            if (TRUE == commonIQResult)
            {
                pHWSetting->PackIQRegisterSetting(&unpackedRegisterData, NULL);
                IPESetupASF30Register(pInput, &unpackedRegisterData, pOutput);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("IPEASF30 calculate hardwware setting failed");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("ASF30 interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL %p %p ", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SetupTDL10Register
///
/// @brief  Setup TDL10 Register based on common library output
///
/// @param  pData   Pointer to the Common Library Calculation Result
/// @param  pRegCmd Pointer to the TDL10 Register Set
///
/// @return VOID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID SetupTDL10Register(
    const TDL10UnpackedField* pData,
    TDL10OutputData*          pOutput)
{
    UINT hueCount            = 0;
    UINT satCount            = 0;
    UINT index               = 0;

    if (NULL != pData)
    {
        UINT index0 = 0;
        UINT index1 = 0;
        UINT index2 = 0;
        UINT index3 = 0;
        for (hueCount = 0; hueCount < H_GRID; hueCount++)
        {
            for (satCount = 0; satCount < S_GRID; satCount++)
            {
                if (TRUE == (Utils::IsUINT32Odd(hueCount)))
                {
                    if (TRUE == (Utils::IsUINT32Odd(satCount)))
                    {
                        // (odd, odd)
                        pOutput->pD2H3LUT[index3] = pData->hs_lut.lut_2d_h[index];
                        pOutput->pD2S3LUT[index3] = pData->hs_lut.lut_2d_s[index];
                        index3++;
                        index++;
                    }
                    else
                    {
                        // (odd, even)
                        pOutput->pD2H2LUT[index2] = pData->hs_lut.lut_2d_h[index];
                        pOutput->pD2S2LUT[index2] = pData->hs_lut.lut_2d_s[index];
                        index2++;
                        index++;
                    }
                }
                else
                {
                    if (TRUE == (Utils::IsUINT32Odd(satCount)))
                    {
                        // even, odd
                        pOutput->pD2H1LUT[index1] = pData->hs_lut.lut_2d_h[index];
                        pOutput->pD2S1LUT[index1] = pData->hs_lut.lut_2d_s[index];
                        index1++;
                        index++;
                    }
                    else
                    {
                        pOutput->pD2H0LUT[index0] = pData->hs_lut.lut_2d_h[index];
                        pOutput->pD2S0LUT[index0] = pData->hs_lut.lut_2d_s[index];
                        index0++;
                        index++;
                    }
                }
            }
        }

        Utils::Memcpy(pOutput->pD1HLUT, pData->hs_lut.lut_1d_h, sizeof(UINT32) * H_GRID);
        Utils::Memcpy(pOutput->pD1SLUT, pData->hs_lut.lut_1d_s, sizeof(UINT32) * S_GRID);
        Utils::Memcpy(pOutput->pD1IHLUT, pData->hs_lut.lut_1d_h_inv, sizeof(UINT32) * (H_GRID-1));
        Utils::Memcpy(pOutput->pD1ISLUT, pData->hs_lut.lut_1d_s_inv, sizeof(UINT32) * (S_GRID - 1));

    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "data is NULL pData:%p pOutput:%p", pData, pOutput);
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::TDL10CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::TDL10CalculateSetting(
    const TDL10InputData* pInput,
    VOID*                 pOEMIQData,
    TDL10OutputData*      pOutput)
{
    CamxResult                                            result             = CamxResultSuccess;
    BOOL                                                  commonIQResult     = FALSE;
    ISPHWSetting*                                         pHWSetting         = NULL;
    tdl_1_0_0::tdl10_rgn_dataType*                        pInterpolationData = NULL;
    tdl_1_0_0::chromatix_tdl10_reserveType*               pReserveType       = NULL;
    tdl_1_0_0::chromatix_tdl10Type::enable_sectionStruct* pModuleEnable      = NULL;

    if ((NULL != pInput) && (NULL != pOutput))
    {
        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pInterpolationData = static_cast<tdl_1_0_0::tdl10_rgn_dataType*>(pInput->pInterpolationData);
            pReserveType       = &(pInput->pChromatix->chromatix_tdl10_reserve);
            pModuleEnable      = &(pInput->pChromatix->enable_section);

            commonIQResult = IQInterface::s_interpolationTable.IPETDL10Interpolation(pInput, pInterpolationData);
        }
        else
        {
            OEMIPEIQSetting* pOEMInput = static_cast<OEMIPEIQSetting*>(pOEMIQData);
            // Use OEM defined interpolation data
            pInterpolationData = reinterpret_cast<tdl_1_0_0::tdl10_rgn_dataType*>(&(pOEMInput->TDL10Setting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
                pReserveType   = reinterpret_cast<tdl_1_0_0::chromatix_tdl10_reserveType*>(
                                     &(pOEMInput->TDL10ReserveType));
                pModuleEnable  = reinterpret_cast<tdl_1_0_0::chromatix_tdl10Type::enable_sectionStruct*>(
                                     &(pOEMInput->TDL10EnableSection));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            TDL10UnpackedField unpackedRegData;

            commonIQResult =
                IQInterface::s_interpolationTable.IPETDL10CalculateHWSetting(pInput,
                                                                             pInterpolationData,
                                                                             pReserveType,
                                                                             pModuleEnable,
                                                                             &unpackedRegData);

            if (TRUE == commonIQResult)
            {
                pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
                result     = pHWSetting->PackIQRegisterSetting(&unpackedRegData, NULL);
                SetupTDL10Register(&unpackedRegData, pOutput);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("TDL10 calculate hardware setting failed");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("TDL10 interpolation failed");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL %p %p ", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IPECV12CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IPECV12CalculateSetting(
    const CV12InputData* pInput,
    VOID*                pOEMIQData,
    CV12OutputData*      pOutput)
{
    CamxResult                                          result                = CamxResultSuccess;
    BOOL                                                commonIQResult        = FALSE;
    cv_1_2_0::cv12_rgn_dataType*                        pInterpolationData    = NULL;
    cv_1_2_0::chromatix_cv12Type::enable_sectionStruct* pModuleEnable         = NULL;
    ISPHWSetting*                                       pHWSetting            = NULL;

    if ((NULL != pInput) && (NULL != pInput->pHWSetting) && (NULL != pOutput))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);

        if (NULL == pOEMIQData)
        {
            pInterpolationData = static_cast<cv_1_2_0::cv12_rgn_dataType*>(pInput->pInterpolationData);
            pModuleEnable      = &(pInput->pChromatix->enable_section);

            // Call the Interpolation Calculation
            commonIQResult = IQInterface::s_interpolationTable.CV12Interpolation(pInput, pInterpolationData);
        }
        else
        {
            OEMIPEIQSetting* pOEMInput = static_cast<OEMIPEIQSetting*>(pOEMIQData);

            // Use OEM defined interpolation data
            pInterpolationData = reinterpret_cast<cv_1_2_0::cv12_rgn_dataType*>(&(pOEMInput->ChromaEnhancementSetting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
                pModuleEnable  = reinterpret_cast<cv_1_2_0::chromatix_cv12Type::enable_sectionStruct*>(
                                     &(pOEMInput->ChromaEnhancementEnableSection));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            CV12UnpackedField unpackedRegisterData = { 0 };
            commonIQResult = IQInterface::s_interpolationTable.CV12CalculateHWSetting(
                                 pInput,
                                 pInterpolationData,
                                 pModuleEnable,
                                 static_cast<VOID*>(&unpackedRegisterData));


            if (TRUE == commonIQResult)
            {
                VOID* pInputData = static_cast<VOID*>(&unpackedRegisterData);
                pHWSetting->PackIQRegisterSetting(pInputData, NULL);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("IPECV12 calculate hardwware setting failed");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("CV12 interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Data is NULL. pInput=%p, pOutput=%p", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IPECAC22CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IPECAC22CalculateSetting(
    const CAC22InputData* pInput,
    VOID*                 pOEMIQData,
    CAC22OutputData*      pOutput)
{
    CamxResult                                            result               = CamxResultSuccess;
    BOOL                                                  commonIQResult       = FALSE;
    cac_2_2_0::cac22_rgn_dataType*                        pInterpolationData   = NULL;
    cac_2_2_0::chromatix_cac22Type::enable_sectionStruct* pModuleEnable        = NULL;
    ISPHWSetting*                                         pHWSetting           = NULL;

    if ((NULL != pInput) && (NULL != pInput->pHWSetting) && (NULL != pOutput))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pInterpolationData = static_cast<cac_2_2_0::cac22_rgn_dataType*>(pInput->pInterpolationData);
            pModuleEnable      = &(pInput->pChromatix->enable_section);

            commonIQResult = IQInterface::s_interpolationTable.CAC22Interpolation(pInput, pInterpolationData);
        }
        else
        {
            OEMIPEIQSetting* pOEMInput = static_cast<OEMIPEIQSetting*>(pOEMIQData);

            // Use OEM defined interpolation data
            pInterpolationData = reinterpret_cast<cac_2_2_0::cac22_rgn_dataType*>(&(pOEMInput->CACSetting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
                pModuleEnable  = reinterpret_cast<cac_2_2_0::chromatix_cac22Type::enable_sectionStruct*>(
                                     &(pOEMInput->CACEnableSection));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            CAC22UnpackedField unpackedRegisterData = { 0 };

            commonIQResult =
                IQInterface::s_interpolationTable.CAC22CalculateHWSetting(
                    pInput,
                    pInterpolationData,
                    pModuleEnable,
                    static_cast<VOID*>(&unpackedRegisterData));

            if (TRUE == commonIQResult)
            {
                VOID* pInputData = static_cast<VOID*>(&unpackedRegisterData);

                pHWSetting->PackIQRegisterSetting(pInputData, NULL);

                pOutput->enableCAC2 = unpackedRegisterData.enableCAC2;
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("CAC2 calculate hardwware setting failed");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("CAC22 intepolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL pInput = %p, pOutput = %p", pInput, pOutput);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IPECAC23CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IPECAC23CalculateSetting(
    const CAC23InputData* pInput,
    VOID*                 pOEMIQData,
    CAC23OutputData*      pOutput)
{
    CamxResult                                            result               = CamxResultSuccess;
    BOOL                                                  commonIQResult       = FALSE;
    cac_2_3_0::cac23_rgn_dataType*                        pInterpolationData   = NULL;
    cac_2_3_0::chromatix_cac23Type::enable_sectionStruct* pModuleEnable        = NULL;
    ISPHWSetting*                                         pHWSetting           = NULL;

    if ((NULL != pInput) && (NULL != pInput->pHWSetting) && (NULL != pOutput))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            pInterpolationData = static_cast<cac_2_3_0::cac23_rgn_dataType*>(pInput->pInterpolationData);
            pModuleEnable      = &(pInput->pChromatix->enable_section);

            commonIQResult = IQInterface::s_interpolationTable.CAC23Interpolation(pInput, pInterpolationData);
        }
        else
        {
            OEMIPEIQSetting* pOEMInput = static_cast<OEMIPEIQSetting*>(pOEMIQData);

            // Use OEM defined interpolation data
            pInterpolationData = reinterpret_cast<cac_2_3_0::cac23_rgn_dataType*>(&(pOEMInput->CAC23Setting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
                pModuleEnable  = reinterpret_cast<cac_2_3_0::chromatix_cac23Type::enable_sectionStruct*>(
                                     &(pOEMInput->CAC23EnableSection));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            CAC23UnpackedField unpackedRegisterData = { 0 };

            commonIQResult =
                IQInterface::s_interpolationTable.CAC23CalculateHWSetting(
                    pInput,
                    pInterpolationData,
                    pModuleEnable,
                    static_cast<VOID*>(&unpackedRegisterData));

            if (TRUE == commonIQResult)
            {
                VOID* pInputData = static_cast<VOID*>(&unpackedRegisterData);

                pHWSetting->PackIQRegisterSetting(pInputData, NULL);

                pOutput->enableCAC2 = unpackedRegisterData.enableCAC2;
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("CAC2 calculate hardwware setting failed");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("CAC23 intepolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL pInput = %p, pOutput = %p", pInput, pOutput);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IPETF10CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IPETF10CalculateSetting(
    TF10InputData* pInput,
    VOID*                pOEMIQData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pInput) && (NULL != pInput->pHWSetting))
    {
        BOOL                                                commonIQResult     = FALSE;
        tf_1_0_0::chromatix_tf10_reserveType*               pReserveData       = NULL;
        tf_1_0_0::mod_tf10_cct_dataType::cct_dataStruct*    pInterpolationData = NULL;
        tf_1_0_0::chromatix_tf10Type::enable_sectionStruct* pModuleEnable      = NULL;
        ISPHWSetting*                                       pHWSetting         = NULL;

        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);

        if (NULL == pOEMIQData)
        {
            pInterpolationData =
                static_cast<tf_1_0_0::mod_tf10_cct_dataType::cct_dataStruct*>(pInput->pInterpolationData);

            pReserveData       = &(pInput->pChromatix->chromatix_tf10_reserve);
            pModuleEnable      = &(pInput->pChromatix->enable_section);

            // Call the Interpolation Calculation
            commonIQResult = IQInterface::s_interpolationTable.TF10Interpolation(pInput, pInterpolationData);
        }
        else
        {
            OEMIPEIQSetting* pOEMInput = static_cast<OEMIPEIQSetting*>(pOEMIQData);

            // Use OEM defined interpolation data
            pInterpolationData = reinterpret_cast<tf_1_0_0::mod_tf10_cct_dataType::cct_dataStruct*>(
                                     &(pOEMInput->TF10Setting.cct_data));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
                pReserveData   = reinterpret_cast<tf_1_0_0::chromatix_tf10_reserveType*>(
                &(pOEMInput->TF10ReserveType));
                pModuleEnable  = reinterpret_cast<tf_1_0_0::chromatix_tf10Type::enable_sectionStruct*>(
                                     &(pOEMInput->TF10EnableSection));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {

            commonIQResult = IQInterface::s_interpolationTable.TF10CalculateHWSetting(
                                 pInput,
                                 pInterpolationData,
                                 pReserveData,
                                 pModuleEnable,
                                 NULL);

            if (TRUE != commonIQResult)
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("TF10 HW Calculation failed.");
            }
            else
            {
                pHWSetting->PackIQRegisterSetting(pInput, NULL);
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("TF10 intepolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL pInput = %p", pInput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IPETF10GetInitializationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IPETF10GetInitializationData(
    struct TFNcLibOutputData* pData)
{
    CamxResult result         = CamxResultSuccess;
    BOOL       commonIQResult = FALSE;

    commonIQResult = IQInterface::s_interpolationTable.GetTF10InitializationData(pData);
    if (FALSE == commonIQResult)
    {
        result = CamxResultEFailed;
        CAMX_ASSERT_ALWAYS_MESSAGE("Get TF initialization datafailed.");
    }
    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPESetupICA10InterpolationParams
///
/// @brief  Setup ICA30 Register based on common library output
///
/// @param  pData                 Pointer to the unpacked ICA interpolation input data
/// @param  pReserveData          Pointer to reserve data structure
/// @param  pinterpolationData    Pointer to region data
///
/// @return BOOL TRUE if success
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL IPESetupICA30InterpolationParams(
    VOID*                                    pData,
    ica_3_0_0::chromatix_ica30_reserveType*  pReserveData,
    ica_3_0_0::ica30_rgn_dataType*           pinterpolationData)
{
    IPEICAInterpolationParams*  pInterpolationParams     = static_cast<IPEICAInterpolationParams*>(pData);
    pReserveData->opg_invalid_output_treatment_cb        = pInterpolationParams->outOfFramePixelFillConsCb;
    pReserveData->opg_invalid_output_treatment_cr        = pInterpolationParams->outOfFramePixelFillConsCr;
    pReserveData->opg_invalid_output_treatment_y         = pInterpolationParams->outOfFramePixelFillConstY;
    pReserveData->opg_invalid_output_treatment_calculate = pInterpolationParams->outOfFramePixelFillMode;

    for (UINT i = 0; i < ICAInterpolationCoeffSets; i++)
    {
        pinterpolationData->opg_interpolation_lut_0_tab.opg_interpolation_lut_0[i] =
            pInterpolationParams->customInterpolationCoefficients0[i];
        pinterpolationData->opg_interpolation_lut_1_tab.opg_interpolation_lut_1[i] =
            pInterpolationParams->customInterpolationCoefficients1[i];
        pinterpolationData->opg_interpolation_lut_2_tab.opg_interpolation_lut_2[i] =
            pInterpolationParams->customInterpolationCoefficients2[i];
    }
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEDumpICA30Register
///
/// @brief  Dump ICA30 Register based on common library output
///
/// @param  pInput   Pointer to Input data
/// @param  pData    Pointer to the unpacked ICA data
/// @param  pOutput  Pointer to the ICA output data structure
///
/// @return VOID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID IPEDumpICA30Register(
    const ICAInputData*  pInput,
    ICAUnpackedField*    pData,
    ICAOutputData*       pOutput)
{
    UINT32*  pPerspectivetransform =
        reinterpret_cast<UINT32*>(pOutput->pLUT[ICA30IndexPerspective]);
    UINT64*  pGrid0 =
        reinterpret_cast<UINT64*>(pOutput->pLUT[ICA30IndexGrid0]);
    UINT64*  pGrid1 =
        reinterpret_cast<UINT64*>(pOutput->pLUT[ICA30IndexGrid1]);
    ICA_REG_v30* pRegData =
        static_cast<ICA_REG_v30*>(pData->pRegData);
    IcaParameters*    pIcaParams =
        static_cast<IcaParameters*>(pData->pIcaParameter);

    CHAR  dumpFilename[256];
    FILE* pFile = NULL;
    UINT  j = 0;
    UINT  k = 0;

    CamX::OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename),
        "%s/ica30grid_Out_%d_path_%d.txt", ConfigFileDirectory,
        pInput->frameNum, pInput->IPEPath);

    pFile = CamX::OsUtils::FOpen(dumpFilename, "w");

    if (NULL != pFile)
    {
        CamX::OsUtils::FPrintF(pFile, "Perspective Enable %d\n", pIcaParams->isPerspectiveEnable);
        CamX::OsUtils::FPrintF(pFile, "Num of rows %d\n", pIcaParams->perspGeomN);
        CamX::OsUtils::FPrintF(pFile, "Num of columns %d\n", pIcaParams->perspGeomM);
        for (UINT i = 0; i < ICAPerspectiveSize; i++)
        {
            CamX::OsUtils::FPrintF(pFile, "transform 0x%x, perspectiveM 0x%x, perspectiveE 0x%x\n",
                pPerspectivetransform[i],
                pRegData->CTC_PERSPECTIVE_PARAMS_M[i],
                pRegData->CTC_PERSPECTIVE_PARAMS_E[i]);
        }
        CamX::OsUtils::FPrintF(pFile, "--------------------------------\n");
        CamX::OsUtils::FPrintF(pFile, " Grid Enable %d\n", pIcaParams->isGridEnable);

        for (UINT i = 0; i < ICA30GridRegSize; i++)
        {
            if (0 == (i % 2))
            {
                CamX::OsUtils::FPrintF(pFile, "i %d,j %d,gridX 0x%x,grid Y 0x%x,LUT 0x%llx\n",
                    i, j, pRegData->CTC_GRID_X[i], pRegData->CTC_GRID_Y[i], pGrid0[j]);
                j++;
            }
            else
            {
                CamX::OsUtils::FPrintF(pFile, "i %d,k %d,gridRegX 0x%x,RegY 0x%x,LUT 0x%llx\n",
                    i, k, pRegData->CTC_GRID_X[i], pRegData->CTC_GRID_Y[i], pGrid1[k]);
                k++;
            }
        }
        CamX::OsUtils::FPrintF(pFile, "--------------------------------\n");
        CamX::OsUtils::FClose(pFile);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IPETF20CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IPETF20CalculateSetting(
    TF20InputData* pInput,
    VOID*                pOEMIQData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pInput) && (NULL != pInput->pHWSetting))
    {
        BOOL                                                commonIQResult     = FALSE;
        tf_2_0_0::chromatix_tf20_reserveType*               pReserveData       = NULL;
        tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct*    pInterpolationData = NULL;
        tf_2_0_0::chromatix_tf20Type::enable_sectionStruct* pModuleEnable      = NULL;
        ISPHWSetting*                                       pHWSetting         = NULL;

        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);

        if (NULL == pOEMIQData)
        {
            pInterpolationData =
                static_cast<tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct*>(pInput->pInterpolationData);

            pReserveData       = &(pInput->pChromatix->chromatix_tf20_reserve);
            pModuleEnable      = &(pInput->pChromatix->enable_section);

            // Call the Interpolation Calculation
            commonIQResult = IQInterface::s_interpolationTable.TF20Interpolation(pInput, pInterpolationData);
        }
        else
        {
            OEMIPEIQSetting* pOEMInput = static_cast<OEMIPEIQSetting*>(pOEMIQData);

            // Use OEM defined interpolation data
            pInterpolationData = reinterpret_cast<tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct*>(
                                     &(pOEMInput->TF20Setting.cct_data));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
                pReserveData   = reinterpret_cast<tf_2_0_0::chromatix_tf20_reserveType*>(
                &(pOEMInput->TF20ReserveType));
                pModuleEnable  = reinterpret_cast<tf_2_0_0::chromatix_tf20Type::enable_sectionStruct*>(
                                     &(pOEMInput->TF20EnableSection));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {

            commonIQResult = IQInterface::s_interpolationTable.TF20CalculateHWSetting(
                                 pInput,
                                 pInterpolationData,
                                 pReserveData,
                                 pModuleEnable,
                                 NULL);

            if (TRUE != commonIQResult)
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("TF20 HW Calculation failed.");
                CAMX_LOG_ERROR(CamxLogGroupPProc, "TF20 HW Calculation failed.");
            }
            else
            {
                pHWSetting->PackIQRegisterSetting(pInput, NULL);
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("TF20 intepolation failed.");
            CAMX_LOG_ERROR(CamxLogGroupPProc, "IPE TF20 Calculation Failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL pInput = %p", pInput);
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Input data is NULL pInput = %p", pInput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IPETF20GetInitializationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IPETF20GetInitializationData(
    struct TFNcLibOutputData* pData)
{
    CamxResult result         = CamxResultSuccess;
    BOOL       commonIQResult = FALSE;

    commonIQResult = IQInterface::s_interpolationTable.GetTF20InitializationData(pData);
    if (FALSE == commonIQResult)
    {
        result = CamxResultEFailed;
        CAMX_ASSERT_ALWAYS_MESSAGE("Get TF initialization datafailed.");
    }
    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPESetupICA10InterpolationParams
///
/// @brief  Setup ICA10 Register based on common library output
///
/// @param  pData                 Pointer to the unpacked ICA interpolation input data
/// @param  pReserveData          Pointer to reserve data structure
/// @param  pinterpolationData    Pointer to region data
///
/// @return BOOL TRUE if success
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL IPESetupICA10InterpolationParams(
    VOID*                                    pData,
    ica_1_0_0::chromatix_ica10_reserveType*  pReserveData,
    ica_1_0_0::ica10_rgn_dataType*           pinterpolationData)
{
    IPEICAInterpolationParams*  pInterpolationParams     = static_cast<IPEICAInterpolationParams*>(pData);
    pReserveData->opg_invalid_output_treatment_cb        = pInterpolationParams->outOfFramePixelFillConsCb;
    pReserveData->opg_invalid_output_treatment_cr        = pInterpolationParams->outOfFramePixelFillConsCr;
    pReserveData->opg_invalid_output_treatment_y         = pInterpolationParams->outOfFramePixelFillConstY;
    pReserveData->opg_invalid_output_treatment_calculate = pInterpolationParams->outOfFramePixelFillMode;

    for (UINT i = 0; i < ICAInterpolationCoeffSets; i++)
    {
        pinterpolationData->opg_interpolation_lut_0_tab.opg_interpolation_lut_0[i] =
            pInterpolationParams->customInterpolationCoefficients0[i];
        pinterpolationData->opg_interpolation_lut_1_tab.opg_interpolation_lut_1[i] =
            pInterpolationParams->customInterpolationCoefficients1[i];
        pinterpolationData->opg_interpolation_lut_2_tab.opg_interpolation_lut_2[i] =
            pInterpolationParams->customInterpolationCoefficients2[i];
    }
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEDumpICA10Register
///
/// @brief  Dump ICA10 Register based on common library output
///
/// @param  pInput   Pointer to Input data
/// @param  pData    Pointer to the unpacked ICA data
/// @param  pOutput  Pointer to the ICA output data structure
///
/// @return VOID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID IPEDumpICA10Register(
    const ICAInputData*  pInput,
    ICAUnpackedField*    pData,
    ICAOutputData*       pOutput)
{
    UINT32*  pPerspectivetransform =
        reinterpret_cast<UINT32*>(pOutput->pLUT[ICA10IndexPerspective]);
    UINT64*  pGrid0                =
        reinterpret_cast<UINT64*>(pOutput->pLUT[ICA10IndexGrid0]);
    UINT64*  pGrid1                =
        reinterpret_cast<UINT64*>(pOutput->pLUT[ICA10IndexGrid1]);
    ICA_REG_v30* pRegData              =
        static_cast<ICA_REG_v30*>(pData->pRegData);
    IcaParameters*    pIcaParams   =
        static_cast<IcaParameters*>(pData->pIcaParameter);

    CHAR  dumpFilename[256];
    FILE* pFile = NULL;
    UINT  j     = 0;
    UINT  k     = 0;

    CamX::OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename),
                            "%s/ica10grid_Out_%d_path_%d.txt", ConfigFileDirectory,
                            pInput->frameNum, pInput->IPEPath);

    pFile = CamX::OsUtils::FOpen(dumpFilename, "w");

    if (NULL != pFile)
    {
        CamX::OsUtils::FPrintF(pFile, "Perspective Enable %d\n", pIcaParams->isPerspectiveEnable);
        CamX::OsUtils::FPrintF(pFile, "Num of rows %d\n", pIcaParams->perspGeomN);
        CamX::OsUtils::FPrintF(pFile, "Num of columns %d\n", pIcaParams->perspGeomM);
        for (UINT i = 0; i < ICAPerspectiveSize; i++)
        {
            CamX::OsUtils::FPrintF(pFile, "transform 0x%x, perspectiveM 0x%x, perspectiveE 0x%x\n",
                pPerspectivetransform[i],
                pRegData->CTC_PERSPECTIVE_PARAMS_M[i],
                pRegData->CTC_PERSPECTIVE_PARAMS_E[i]);
        }
        CamX::OsUtils::FPrintF(pFile, "--------------------------------\n");
        CamX::OsUtils::FPrintF(pFile, " Grid Enable %d\n", pIcaParams->isGridEnable);

        for (UINT i = 0; i < ICA10GridRegSize; i++)
        {
            if (0 == (i % 2))
            {
                CamX::OsUtils::FPrintF(pFile, "i %d,j %d,gridX 0x%x,grid Y 0x%x,LUT 0x%llx\n",
                    i, j, pRegData->CTC_GRID_X[i], pRegData->CTC_GRID_Y[i], pGrid0[j]);
                j++;
            }
            else
            {
                CamX::OsUtils::FPrintF(pFile, "i %d,k %d,gridRegX 0x%x,RegY 0x%x,LUT 0x%llx\n",
                    i, k, pRegData->CTC_GRID_X[i], pRegData->CTC_GRID_Y[i], pGrid1[k]);
                k++;
            }
        }
        CamX::OsUtils::FPrintF(pFile, "--------------------------------\n");
        CamX::OsUtils::FClose(pFile);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPESetupICA20InterpolationParams
///
/// @brief  Setup ICA20 Register based on common library output
///
/// @param  pData                 Pointer to the unpacked ICA interpolation input data
/// @param  pReserveData          Pointer to reserve data structure
/// @param  pinterpolationData    Pointer to region data
///
/// @return BOOL TRUE if success
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL IPESetupICA20InterpolationParams(
    VOID*                                    pData,
    ica_2_0_0::chromatix_ica20_reserveType*  pReserveData,
    ica_2_0_0::ica20_rgn_dataType*           pinterpolationData)
{
    IPEICAInterpolationParams*  pInterpolationParams     = static_cast<IPEICAInterpolationParams*>(pData);
    pReserveData->opg_invalid_output_treatment_cb        = pInterpolationParams->outOfFramePixelFillConsCb;
    pReserveData->opg_invalid_output_treatment_cr        = pInterpolationParams->outOfFramePixelFillConsCr;
    pReserveData->opg_invalid_output_treatment_y         = pInterpolationParams->outOfFramePixelFillConstY;
    pReserveData->opg_invalid_output_treatment_calculate = pInterpolationParams->outOfFramePixelFillMode;

    for (UINT i = 0; i < ICAInterpolationCoeffSets; i++)
    {
        pinterpolationData->opg_interpolation_lut_0_tab.opg_interpolation_lut_0[i] =
            pInterpolationParams->customInterpolationCoefficients0[i];
        pinterpolationData->opg_interpolation_lut_1_tab.opg_interpolation_lut_1[i] =
            pInterpolationParams->customInterpolationCoefficients1[i];
        pinterpolationData->opg_interpolation_lut_2_tab.opg_interpolation_lut_2[i] =
            pInterpolationParams->customInterpolationCoefficients2[i];
    }
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEDumpICA20Register
///
/// @brief  Dump ICA10 Register based on common library output
///
/// @param  pInput   Pointer to Input data
/// @param  pData    Pointer to the unpacked ICA data
/// @param  pOutput  Pointer to the ICA output data structure
///
/// @return VOID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID IPEDumpICA20Register(
    const ICAInputData*  pInput,
    ICAUnpackedField*    pData,
    ICAOutputData*       pOutput)
{
    UINT32*           pPerspectivetransform =
        reinterpret_cast<UINT32*>(pOutput->pLUT[ICA20IndexPerspective]);
    UINT64*           pGrid0                =
        reinterpret_cast<UINT64*>(pOutput->pLUT[ICA20IndexGrid]);
    ICA_REG_v30*      pRegData              =
        static_cast<ICA_REG_v30*>(pData->pRegData);
    IcaParameters*    pIcaParams            =
        static_cast<IcaParameters*>(pData->pIcaParameter);

    CHAR  dumpFilename[256];
    FILE* pFile = NULL;

    CamX::OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename),
        "%s/ica20grid_Out_%d_path_%d.txt", ConfigFileDirectory,
        pInput->frameNum, pInput->IPEPath);
    pFile = CamX::OsUtils::FOpen(dumpFilename, "w");

    if (NULL != pFile)
    {
        CamX::OsUtils::FPrintF(pFile, "Perspective Enable %d\n", pIcaParams->isPerspectiveEnable);
        CamX::OsUtils::FPrintF(pFile, "Num of rows %d\n", pIcaParams->perspGeomN);
        CamX::OsUtils::FPrintF(pFile, "Num of columns %d\n", pIcaParams->perspGeomM);
        for (UINT i = 0; i < ICAPerspectiveSize; i++)
        {
            CamX::OsUtils::FPrintF(pFile, "perspectiveM 0x%x, perspectiveE 0x%x, transform 0x%x\n",
                pPerspectivetransform[i],
                pRegData->CTC_PERSPECTIVE_PARAMS_M[i],
                pRegData->CTC_PERSPECTIVE_PARAMS_E[i]);
        }
        CamX::OsUtils::FPrintF(pFile, "--------------------------------\n");
        CamX::OsUtils::FPrintF(pFile, " Grid Enable %d\n", pIcaParams->isGridEnable);

        for (UINT i = 0; i < ICA20GridRegSize; i++)
        {
            CamX::OsUtils::FPrintF(pFile, "i %d,gridX 0x%x,grid Y 0x%x,LUT 0x%llx\n",
                i, pRegData->CTC_GRID_X[i], pRegData->CTC_GRID_Y[i], pGrid0[i]);
        }
        CamX::OsUtils::FPrintF(pFile, "--------------------------------\n");
        CamX::OsUtils::FClose(pFile);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::ICA10CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::ICA10CalculateSetting(
    const ICAInputData*  pInput,
    ICAOutputData*       pOutput)
{
    IPEICAInterpolationParams*               pInterpolationParams = NULL;
    CamxResult                               result               = CamxResultSuccess;
    BOOL                                     commonIQResult       = FALSE;
    ica_1_0_0::chromatix_ica10_reserveType*  pReserveData         = NULL;
    ica_1_0_0::ica10_rgn_dataType*           pInterpolationData   = NULL;
    ISPHWSetting*                            pHWSetting           = NULL;
    ica_1_0_0::chromatix_ica10_reserveType   reserveData;
    ica_1_0_0::ica10_rgn_dataType            interpolationData;
    ICAUnpackedField                         unpackedRegData;

    if ((NULL != pInput) && (NULL != pOutput))
    {
        pHWSetting           = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        // Call the Interpolation Calculation
        pInterpolationParams = static_cast<IPEICAInterpolationParams*>(pInput->pInterpolationParamters);
        if (TRUE == pInterpolationParams->customInterpolationEnabled)
        {
            commonIQResult      = IPESetupICA10InterpolationParams(pInterpolationParams, &reserveData, &interpolationData);
            pReserveData        = &reserveData;
            pInterpolationData  = &interpolationData;
        }
        else
        {
            pInterpolationData  = static_cast<ica_1_0_0::ica10_rgn_dataType*>(pInput->pInterpolationData);
            commonIQResult      = IQInterface::s_interpolationTable.ICA10Interpolation(pInput, pInterpolationData);
            pReserveData        =
                &((static_cast <ica_1_0_0::chromatix_ica10Type*>(pInput->pChromatix))->chromatix_ica10_reserve);
        }

        // Fill Unpacked register data. // Update this if this needs to be ICA Registers.
        unpackedRegData.pIcaParameter = pOutput->pICAParameter;
        unpackedRegData.pRegData      = pInput->pNCRegData;


        if (TRUE == commonIQResult)
        {
            commonIQResult = IQInterface::s_interpolationTable.ICACalculateHWSetting(
                pInput, pInterpolationData, pReserveData, static_cast<VOID*>(&unpackedRegData));

            // not required if unpacked accepts this pointer. if not copy from registers to DMI buffer
            if (FALSE == commonIQResult)
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("ICA10 calculate hardware setting failed");
            }
            else
            {
                pHWSetting->PackIQRegisterSetting(&unpackedRegData, pOutput);
                if (TRUE == pInput->dumpICAOut)
                {
                    IPEDumpICA10Register(pInput, &unpackedRegData, pOutput);
                }
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("ICA10 interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL pInput = %p, pOutput = %p", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::ICA20CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::ICA20CalculateSetting(
    const ICAInputData*  pInput,
    ICAOutputData*       pOutput)
{
    IPEICAInterpolationParams*               pInterpolationParams = NULL;
    CamxResult                               result               = CamxResultSuccess;
    BOOL                                     commonIQResult       = FALSE;
    ica_2_0_0::chromatix_ica20_reserveType*  pReserveData         = NULL;
    ica_2_0_0::ica20_rgn_dataType*           pInterpolationData   = NULL;
    ISPHWSetting*                            pHWSetting           = NULL;
    ica_2_0_0::chromatix_ica20_reserveType   reserveData;
    ica_2_0_0::ica20_rgn_dataType            interpolationData;
    ICAUnpackedField                         unpackedRegData;

    if ((NULL != pInput) && (NULL != pOutput))
    {
        pHWSetting           = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        // Call the Interpolation Calculation
        pInterpolationParams = static_cast<IPEICAInterpolationParams*>(pInput->pInterpolationParamters);
        if (TRUE == pInterpolationParams->customInterpolationEnabled)
        {
            commonIQResult      = IPESetupICA20InterpolationParams(pInterpolationParams, &reserveData, &interpolationData);
            pReserveData        = &reserveData;
            pInterpolationData  = &interpolationData;
        }
        else
        {
            pInterpolationData  = static_cast<ica_2_0_0::ica20_rgn_dataType*>(pInput->pInterpolationData);
            commonIQResult      = IQInterface::s_interpolationTable.ICA20Interpolation(pInput, pInterpolationData);
            pReserveData        =
                &((static_cast <ica_2_0_0::chromatix_ica20Type*>(pInput->pChromatix))->chromatix_ica20_reserve);
        }

        // Fill Unpacked register data.
        unpackedRegData.pIcaParameter       = pOutput->pICAParameter;
        unpackedRegData.pRegData            = pInput->pNCRegData;
        unpackedRegData.pCVPICAFrameCfgData = pOutput->pCVPICAFrameCfgData;
        if (TRUE == commonIQResult)
        {
            commonIQResult = IQInterface::s_interpolationTable.ICACalculateHWSetting(
                pInput, pInterpolationData, pReserveData, static_cast<VOID*>(&unpackedRegData));

            if (FALSE == commonIQResult)
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("IC20 calculate hardware setting failed");
            }
            else
            {
                pHWSetting->PackIQRegisterSetting(&unpackedRegData, pOutput);
                if (TRUE == pInput->dumpICAOut)
                {
                    IPEDumpICA20Register(pInput, &unpackedRegData, pOutput);
                }
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("ICA20 interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL pInput = %p, pOutput = %p", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::ICA30CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::ICA30CalculateSetting(
    const ICAInputData*  pInput,
    ICAOutputData*       pOutput)
{
    IPEICAInterpolationParams*               pInterpolationParams = NULL;
    CamxResult                               result = CamxResultSuccess;
    BOOL                                     commonIQResult       = FALSE;
    ica_3_0_0::chromatix_ica30_reserveType*  pReserveData         = NULL;
    ica_3_0_0::ica30_rgn_dataType*           pInterpolationData   = NULL;
    ISPHWSetting*                            pHWSetting           = NULL;
    ica_3_0_0::chromatix_ica30_reserveType   reserveData;
    ica_3_0_0::ica30_rgn_dataType            interpolationData;
    ICAUnpackedField                         unpackedRegData;

    if ((NULL != pInput) && (NULL != pOutput))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        // Call the Interpolation Calculation
        pInterpolationParams = static_cast<IPEICAInterpolationParams*>(pInput->pInterpolationParamters);
        pInterpolationData = static_cast<ica_3_0_0::ica30_rgn_dataType*>(pInput->pInterpolationData);
        if (TRUE == pInterpolationParams->customInterpolationEnabled)
        {
            commonIQResult = IPESetupICA30InterpolationParams(pInterpolationParams, &reserveData, pInterpolationData);
            pReserveData = &reserveData;
        }
        else
        {
            commonIQResult = IQInterface::s_interpolationTable.ICA30Interpolation(pInput, pInterpolationData);
            pReserveData =
                &((static_cast <ica_3_0_0::chromatix_ica30Type*>(pInput->pChromatix))->chromatix_ica30_reserve);
        }

        // Fill Unpacked register data.
        unpackedRegData.pIcaParameter     = pOutput->pICAParameter;
        unpackedRegData.pRegData          = pInput->pNCRegData;
        unpackedRegData.pIcaGeoParameters = pOutput->pIcaGeoParameters;

        if (TRUE == commonIQResult)
        {
            commonIQResult = IQInterface::s_interpolationTable.ICACalculateHWSetting(
                pInput, pInterpolationData, pReserveData, static_cast<VOID*>(&unpackedRegData));

            if (FALSE == commonIQResult)
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("IC30 calculate hardware setting failed");
            }
            else
            {
                pHWSetting->PackIQRegisterSetting(&unpackedRegData, pOutput);
                if (TRUE == pInput->dumpICAOut)
                {
                    IPEDumpICA30Register(pInput, &unpackedRegData, pOutput);
                }
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("ICA30 interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL pInput = %p, pOutput = %p", pInput, pOutput);
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IPEICAGetInitializationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IPEICAGetInitializationData(
    struct ICANcLibOutputData* pData)
{
    CamxResult result         = CamxResultSuccess;
    BOOL       commonIQResult = FALSE;

    commonIQResult = IQInterface::s_interpolationTable.GetICAInitializationData(pData);
    if (FALSE == commonIQResult)
    {
        result = CamxResultEFailed;
        CAMX_ASSERT_ALWAYS_MESSAGE("Get ICA initialization datafailed.");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IPEANR10CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IPEANR10CalculateSetting(
    ANR10InputData*       pInput,
    VOID*                 pOEMIQData)
{
    CamxResult    result         = CamxResultSuccess;
    BOOL          commonIQResult = FALSE;
    ISPHWSetting* pHWSetting     = NULL;

    if (NULL != pInput)
    {
        anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct*    pInterpolationData = NULL;
        anr_1_0_0::chromatix_anr10Type::enable_sectionStruct* pModuleEnable      = NULL;
        anr_1_0_0::chromatix_anr10_reserveType*               pReserveData       = NULL;

        if (NULL == pOEMIQData)
        {
            pInterpolationData =
                static_cast<anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct*>(pInput->pInterpolationData);
            pReserveData       = &(pInput->pChromatix->chromatix_anr10_reserve);
            pModuleEnable      = &(pInput->pChromatix->enable_section);

            // Call the Interpolation Calculation
            commonIQResult = IQInterface::s_interpolationTable.ANR10Interpolation(pInput, pInterpolationData);
        }
        else
        {
            OEMIPEIQSetting* pOEMInput = static_cast<OEMIPEIQSetting*>(pOEMIQData);

            // Use OEM defined interpolation data
            pInterpolationData = reinterpret_cast<anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct*>(
                                     &(pOEMInput->ANR10Setting.cct_data));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
                pReserveData   = reinterpret_cast<anr_1_0_0::chromatix_anr10_reserveType*>(
                                     &(pOEMInput->ANR10ReserveType));
                pModuleEnable  = reinterpret_cast<anr_1_0_0::chromatix_anr10Type::enable_sectionStruct*>(
                                     &(pOEMInput->ANR10EnableSection));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            commonIQResult = IQInterface::s_interpolationTable.ANR10CalculateHWSetting(
                                 pInput,
                                 pInterpolationData,
                                 pReserveData,
                                 pModuleEnable,
                                 NULL);

            if (TRUE == commonIQResult)
            {
                pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);
                if (NULL != pHWSetting)
                {
                    result = pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(pInput), NULL);
                }
                else
                {
                    CAMX_ASSERT_ALWAYS_MESSAGE("HW Setting pointer is NULL");
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("ANR10 HW Calculation failed.");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("ANR10 interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL pInput = %p", pInput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IPEANR10GetInitializationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IPEANR10GetInitializationData(
    struct ANRNcLibOutputData* pData)
{
    CamxResult result         = CamxResultSuccess;
    BOOL       commonIQResult = FALSE;

    commonIQResult = IQInterface::s_interpolationTable.GetANR10InitializationData(pData);
    if (FALSE == commonIQResult)
    {
        result = CamxResultEFailed;
        CAMX_ASSERT_ALWAYS_MESSAGE("Get ANR initialization datafailed.");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IFEDSX10GetInitializationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IFEDSX10GetInitializationData(
    struct DSXNcLibOutputData* pData)
{
    CamxResult result         = CamxResultSuccess;
    BOOL       commonIQResult = FALSE;

    commonIQResult = IQInterface::s_interpolationTable.GetDSX10InitializationData(pData);
    if (FALSE == commonIQResult)
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Get DSX initialization data failed.");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPELTM13CalculateVendorSetting
///
/// @brief  Calculate and apply LTM vendor settings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID IPELTM13CalculateVendorSetting(
    LTM13InputData* pInput,
    ltm_1_3_0::ltm13_rgn_dataType* pInterpolationData)
{
    if ((NULL != pInput) && (NULL != pInterpolationData))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupPProc,
            "ltmDarkBoostStrength = %f "
            "ltmBrightSupressStrength = %f "
            "ltmLceStrength = %f "
            "ltm_old_dark_boost = %f "
            "ltm_old_bright_suppress = %f "
            "ltm_old_lce_strength = %f ",
            pInput->ltmDarkBoostStrength,
            pInput->ltmBrightSupressStrength,
            pInput->ltmLceStrength,
            pInterpolationData->dark_boost,
            pInterpolationData->bright_suppress,
            pInterpolationData->lce_strength);

        if (pInput->ltmDarkBoostStrength >= 0.0f)
        {
            pInterpolationData->dark_boost += (4.0f - pInterpolationData->dark_boost) * pInput->ltmDarkBoostStrength / 100.0f;
        }
        else
        {
            pInterpolationData->dark_boost *= (pInput->ltmDarkBoostStrength + 100.0f) / 100.0f;
        }

        if (pInput->ltmBrightSupressStrength >= 0.0f)
        {
            pInterpolationData->bright_suppress +=
                (4.0f - pInterpolationData->bright_suppress) * pInput->ltmBrightSupressStrength / 100.0f;
        }
        else
        {
            pInterpolationData->bright_suppress *= (pInput->ltmBrightSupressStrength + 100.0f) / 100.0f;
        }

        if (pInput->ltmLceStrength >= 0.0f)
        {
            pInterpolationData->lce_strength += (4.0f - pInterpolationData->lce_strength) * pInput->ltmLceStrength / 100.0f;
        }
        else
        {
            pInterpolationData->lce_strength *= (pInput->ltmLceStrength + 100.0f) / 100.0f;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupPProc,
            "ltm_new_dark_boost = %f "
            "ltm_new_bright_suppress = %f "
            "ltm_new_lce_strength = %f ",
            pInterpolationData->dark_boost,
            pInterpolationData->bright_suppress,
            pInterpolationData->lce_strength);
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid data is NULL pInput %p pInterpolationData %p", pInput, pInterpolationData);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQInterface::IPELTM13CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IPELTM13CalculateSetting(
    LTM13InputData*       pInput,
    VOID*                 pOEMIQData,
    LTM13OutputData*      pOutput,
    TMC10InputData*       pTMCInput)
{
    CamxResult                                            result             = CamxResultSuccess;
    BOOL                                                  commonIQResult     = FALSE;
    ltm_1_3_0::chromatix_ltm13Type::enable_sectionStruct* pModuleEnable      = NULL;
    ltm_1_3_0::chromatix_ltm13_reserveType*               pReserveData       = NULL;
    ltm_1_3_0::ltm13_rgn_dataType*                        pInterpolationData = NULL;
    ISPHWSetting*                                         pHWSetting         = NULL;

    if ((NULL != pInput)           &&
        (NULL != pOutput)          &&
        (NULL != pOEMIQData || NULL != pInput->pChromatix) &&
        (NULL != pInput->pHWSetting))
    {
        pHWSetting         = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        pInterpolationData = static_cast<ltm_1_3_0::ltm13_rgn_dataType*>(pInput->pInterpolationData);

        if (NULL == pOEMIQData)
        {
            pModuleEnable      = &(pInput->pChromatix->enable_section);
            pReserveData       = &(pInput->pChromatix->chromatix_ltm13_reserve);

            if (NULL != pInterpolationData)
            {
                // Call the Interpolation Calculation
                commonIQResult = IQInterface::s_interpolationTable.LTM13Interpolation(pInput, pInterpolationData);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid InterpolationData pointer");
            }

            // If ADRC is enable for LTM module, update ADRC input data.
            if ((TRUE == commonIQResult) &&
                (TRUE == pTMCInput->adrcLTMEnable))
            {
                if (SWTMCVersion::TMC10 == pTMCInput->pAdrcOutputData->version)
                {
                    commonIQResult = IQInterface::GetADRCData(pTMCInput);
                }
                pInput->pAdrcInputData = pTMCInput->pAdrcOutputData;
            }
        }
        else
        {
            OEMIPEIQSetting* pOEMInput = static_cast<OEMIPEIQSetting*>(pOEMIQData);

            pReserveData       = reinterpret_cast<ltm_1_3_0::chromatix_ltm13_reserveType*>(
                                     &(pOEMInput->LTMReserveType));

            if (NULL != pInterpolationData)
            {
                // Copy OEM defined interpolation data
                Utils::Memcpy(pInterpolationData, &(pOEMInput->LTMSetting), sizeof(ltm_1_3_0::ltm13_rgn_dataType));

                commonIQResult = TRUE;
                pModuleEnable  = reinterpret_cast<ltm_1_3_0::chromatix_ltm13Type::enable_sectionStruct*>(
                                     &(pOEMInput->LTMEnableSection));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            // make the lut table in unpackedRegisterData point to DMI Table
            UINT32             offset               = 0;
            LTM13UnpackedField unpackedRegisterData = { 0 };

            unpackedRegisterData.wt.numEntries = LTM_WEIGHT_LUT_SIZE;
            unpackedRegisterData.wt.pLUTTable = reinterpret_cast<INT32*>(pOutput->pDMIDataPtr + offset);

            offset += IPELTMLUTNumEntries[LTMIndexWeight];
            unpackedRegisterData.la_curve.numEntries = LTM_CURVE_LUT_SIZE - 1;
            unpackedRegisterData.la_curve.pLUTTable = reinterpret_cast<INT32*>(pOutput->pDMIDataPtr + offset);

            offset += IPELTMLUTNumEntries[LTMIndexLA0] + IPELTMLUTNumEntries[LTMIndexLA1];
            unpackedRegisterData.ltm_curve.numEntries = LTM_CURVE_LUT_SIZE - 1;
            unpackedRegisterData.ltm_curve.pLUTTable = reinterpret_cast<INT32*>(pOutput->pDMIDataPtr + offset);

            offset += IPELTMLUTNumEntries[LTMIndexCurve];
            unpackedRegisterData.ltm_scale.numEntries = LTM_SCALE_LUT_SIZE - 1;
            unpackedRegisterData.ltm_scale.pLUTTable = reinterpret_cast<INT32*>(pOutput->pDMIDataPtr + offset);

            offset += IPELTMLUTNumEntries[LTMIndexScale];
            unpackedRegisterData.mask_rect_curve.numEntries = LTM_SCALE_LUT_SIZE - 1;
            unpackedRegisterData.mask_rect_curve.pLUTTable = reinterpret_cast<INT32*>(pOutput->pDMIDataPtr + offset);

            offset += IPELTMLUTNumEntries[LTMIndexMask];
            unpackedRegisterData.lce_scale_pos.numEntries = LTM_SCALE_LUT_SIZE - 1;
            unpackedRegisterData.lce_scale_pos.pLUTTable = reinterpret_cast<INT32*>(pOutput->pDMIDataPtr + offset);

            offset += IPELTMLUTNumEntries[LTMIndexLCEPositive];
            unpackedRegisterData.lce_scale_neg.numEntries = LTM_SCALE_LUT_SIZE - 1;
            unpackedRegisterData.lce_scale_neg.pLUTTable = reinterpret_cast<INT32*>(pOutput->pDMIDataPtr + offset);

            offset += IPELTMLUTNumEntries[LTMIndexLCENegative];
            unpackedRegisterData.igamma64.numEntries = LTM_GAMMA_LUT_SIZE - 1;
            unpackedRegisterData.igamma64.pLUTTable = reinterpret_cast<INT32*>(pOutput->pDMIDataPtr + offset);

            IPELTM13CalculateVendorSetting(pInput, pInterpolationData);

            commonIQResult = IQInterface::s_interpolationTable.LTM13CalculateHWSetting(
                                 pInput,
                                 pInterpolationData,
                                 pReserveData,
                                 pModuleEnable,
                                 static_cast<VOID*>(&unpackedRegisterData));

            if (TRUE == commonIQResult)
            {
                pHWSetting->PackIQRegisterSetting(&unpackedRegisterData, pOutput->pDMIDataPtr);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("LTM13 calculate HW settings failed.");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("LTM13 intepolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL pInput %p pOutput %p", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQInterface::IPELTM14CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IPELTM14CalculateSetting(
    LTM14InputData*       pInput,
    VOID*                 pOEMIQData,
    LTM14OutputData*      pOutput,
    TMC10InputData*       pTMCInput)
{
    CamxResult                                            result             = CamxResultSuccess;
    BOOL                                                  commonIQResult     = FALSE;
    ltm_1_4_0::chromatix_ltm14Type::enable_sectionStruct* pModuleEnable      = NULL;
    ltm_1_4_0::chromatix_ltm14_reserveType*               pReserveData       = NULL;
    ltm_1_4_0::ltm14_rgn_dataType*                        pInterpolationData = NULL;
    ISPHWSetting*                                         pHWSetting         = NULL;

    if ((NULL != pInput)                                   &&
        (NULL != pOutput)                                  &&
        (NULL != pOEMIQData || NULL != pInput->pChromatix) &&
        (NULL != pInput->pHWSetting))
    {
        pHWSetting         = static_cast<ISPHWSetting*>(pInput->pHWSetting);
        pInterpolationData = static_cast<ltm_1_4_0::ltm14_rgn_dataType*>(pInput->pInterpolationData);

        if (NULL == pOEMIQData)
        {
            pModuleEnable      = &(pInput->pChromatix->enable_section);
            pReserveData       = &(pInput->pChromatix->chromatix_ltm14_reserve);

            if (NULL != pInterpolationData)
            {
                // Call the Interpolation Calculation
                commonIQResult = IQInterface::s_interpolationTable.LTM14Interpolation(pInput, pInterpolationData);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid InterpolationData pointer");
            }

            if ((TRUE == commonIQResult) &&
                (TRUE == pTMCInput->adrcLTMEnable))
            {
                if (SWTMCVersion::TMC10 == pTMCInput->pAdrcOutputData->version)
                {
                    commonIQResult = IQInterface::GetADRCData(pTMCInput);
                }
                pInput->pAdrcInputData = pTMCInput->pAdrcOutputData;
            }
        }
        else
        {
            OEMIPEIQSetting* pOEMInput = static_cast<OEMIPEIQSetting*>(pOEMIQData);

            pReserveData               = reinterpret_cast<ltm_1_4_0::chromatix_ltm14_reserveType*>(
                                             &(pOEMInput->LTM14ReserveType));

            if (NULL != pInterpolationData)
            {
                // Copy OEM defined interpolation data
                Utils::Memcpy(pInterpolationData, &(pOEMInput->LTM14Setting), sizeof(ltm_1_4_0::ltm14_rgn_dataType));
                commonIQResult = TRUE;
                pModuleEnable  = reinterpret_cast<ltm_1_4_0::chromatix_ltm14Type::enable_sectionStruct*>(
                                     &(pOEMInput->LTM14EnableSection));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            // make the lut table in unpackedRegisterData point to DMI Table
            UINT32             offset               = 0;
            LTM14UnpackedField unpackedRegisterData = { 0 };
            unpackedRegisterData.wt.numEntries              = LTM14_WEIGHT_LUT_SIZE;
            unpackedRegisterData.wt.pLUTTable               = reinterpret_cast<INT32*>(pOutput->pDMIDataPtr + offset);

            offset += IPELTM14LUTNumEntries[LTM14IndexWeight];
            unpackedRegisterData.la_curve.numEntries        = LTM14_CURVE_LUT_SIZE - 1;
            unpackedRegisterData.la_curve.pLUTTable         = reinterpret_cast<INT32*>(pOutput->pDMIDataPtr + offset);

            offset += IPELTM14LUTNumEntries[LTM14IndexLA];
            unpackedRegisterData.ltm_curve.numEntries       = LTM14_CURVE_LUT_SIZE - 1;
            unpackedRegisterData.ltm_curve.pLUTTable        = reinterpret_cast<INT32*>(pOutput->pDMIDataPtr + offset);

            offset += IPELTM14LUTNumEntries[LTM14IndexCurve];
            unpackedRegisterData.ltm_scale.numEntries       = LTM14_SCALE_LUT_SIZE - 1;
            unpackedRegisterData.ltm_scale.pLUTTable        = reinterpret_cast<INT32*>(pOutput->pDMIDataPtr + offset);

            offset += IPELTM14LUTNumEntries[LTM14IndexScale];
            unpackedRegisterData.mask_rect_curve.numEntries = LTM14_SCALE_LUT_SIZE - 1;
            unpackedRegisterData.mask_rect_curve.pLUTTable  = reinterpret_cast<INT32*>(pOutput->pDMIDataPtr + offset);

            offset += IPELTM14LUTNumEntries[LTM14IndexMask];
            unpackedRegisterData.lce_scale_pos.numEntries   = LTM14_LCE_SCALE_LUT_SIZE - 1;
            unpackedRegisterData.lce_scale_pos.pLUTTable    = reinterpret_cast<INT32*>(pOutput->pDMIDataPtr + offset);

            offset += IPELTM14LUTNumEntries[LTM14IndexLCEPositive];
            unpackedRegisterData.lce_scale_neg.numEntries   = LTM14_LCE_SCALE_LUT_SIZE - 1;
            unpackedRegisterData.lce_scale_neg.pLUTTable    = reinterpret_cast<INT32*>(pOutput->pDMIDataPtr + offset);

            offset += IPELTM14LUTNumEntries[LTM14IndexLCENegative];
            unpackedRegisterData.igamma64.numEntries        = LTM14_GAMMA_LUT_SIZE - 1;
            unpackedRegisterData.igamma64.pLUTTable         = reinterpret_cast<INT32*>(pOutput->pDMIDataPtr + offset);

            commonIQResult = IQInterface::s_interpolationTable.LTM14CalculateHWSetting(
                                 pInput,
                                 pInterpolationData,
                                 pReserveData,
                                 pModuleEnable,
                                 static_cast<VOID*>(&unpackedRegisterData));

            if (TRUE == commonIQResult)
            {
                pHWSetting->PackIQRegisterSetting(&unpackedRegisterData, pOutput->pDMIDataPtr);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("LTM14 calculate HW settings failed.");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("LTM14 intepolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL pInput %p pOutput %p", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CalculateUpscale20LUT
///
/// @brief  Calculate Upscale20 LUT tables
///
/// @param  offset       Offset to the LUT table
/// @param  ppTwoDFilter Pointer to the Common Library Calculation Result
/// @param  numRows      size of row for ppTwoDFilter
/// @param  numColumns   size of column for ppTwoDFilter
/// @param  sizeLUT      size of LUT in this calculation
/// @param  pLUT         Pointer to the LUT table
///
/// @return VOID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID CalculateUpscale20LUT(
    UINT32* pOffset,
    UINT32* pTwoDFilter,
    UINT32  numRows,
    UINT32  numColumns,
    UINT32  sizeLUT,
    UINT32* pLUT)
{
    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 k = 0;
    for (i = 0; i < sizeLUT; i++)
    {
        // twoDFilter[j][k];
        pLUT[i + *pOffset] = *((pTwoDFilter + (j * numColumns)) + k);
        k++;
        if (k >= numColumns && j < numRows)
        {
            j++;
            k = 0;
        }
    }

    *pOffset += sizeLUT;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPESetupUpscale20Register
///
/// @brief  Setup Upscale20 Register based on common library output
///
/// @param  pUnpackedRegisterData    Pointer to the Common Library Calculation Result
/// @param  pOutput                  Pointer to the Upscale20 module Output
///
/// @return VOID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID IPESetupUpscale20Register(
    Upscale20UnpackedField* pUnpackedRegisterData,
    Upscale20OutputData*    pOutput)
{
    UpScaleHwConfig*  pHwConfig      = &(pUnpackedRegisterData->upscaleHwConfig);
    UINT32*           pLUT           = pOutput->pDMIPtr;

    // DMI LUTs
    UINT32* pTwoDFilterA = reinterpret_cast<UINT32*>(pHwConfig->twoDFilterA);
    UINT32* pTwoDFilterB = reinterpret_cast<UINT32*>(pHwConfig->twoDFilterB);
    UINT32* pTwoDFilterC = reinterpret_cast<UINT32*>(pHwConfig->twoDFilterC);
    UINT32* pTwoDFilterD = reinterpret_cast<UINT32*>(pHwConfig->twoDFilterD);
    UINT32 offset = 0;
    CalculateUpscale20LUT(&offset, pTwoDFilterA, 4, 42, IPEUpscaleLUTNumEntries[DMI_LUT_A], pLUT);
    CalculateUpscale20LUT(&offset, pTwoDFilterB, 4, 24, IPEUpscaleLUTNumEntries[DMI_LUT_B], pLUT);
    CalculateUpscale20LUT(&offset, pTwoDFilterC, 4, 24, IPEUpscaleLUTNumEntries[DMI_LUT_C], pLUT);
    CalculateUpscale20LUT(&offset, pTwoDFilterD, 4, 20, IPEUpscaleLUTNumEntries[DMI_LUT_D], pLUT);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IPEUpscale20CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IPEUpscale20CalculateSetting(
    const Upscale20InputData* pInput,
    VOID*                     pOEMIQData,
    Upscale20OutputData*      pOutput)
{
    CamxResult    result     = CamxResultSuccess;
    ISPHWSetting* pHWSetting = NULL;

    if ((NULL != pInput) && (NULL != pOutput) && (NULL != pInput->pHWSetting))
    {
        BOOL                                                          commonIQResult     = FALSE;
        upscale_2_0_0::chromatix_upscale20_reserveType*               pReserveData       = NULL;
        upscale_2_0_0::chromatix_upscale20Type::enable_sectionStruct* pModuleEnable      = NULL;
        upscale_2_0_0::upscale20_rgn_dataType*                        pInterpolationData = NULL;

        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);

        if (NULL == pOEMIQData)
        {
            pInterpolationData = static_cast<upscale_2_0_0::upscale20_rgn_dataType*>(pInput->pInterpolationData);;
            pReserveData       = &(pInput->pChromatix->chromatix_upscale20_reserve);
            pModuleEnable      = &(pInput->pChromatix->enable_section);

            // Call the Interpolation Calculation
            commonIQResult     = IQInterface::s_interpolationTable.upscale20Interpolation(pInput, pInterpolationData);
        }
        else
        {
            OEMIPEIQSetting* pOEMInput = static_cast<OEMIPEIQSetting*>(pOEMIQData);

            // Use OEM defined interpolation data
            pInterpolationData = reinterpret_cast<upscale_2_0_0::upscale20_rgn_dataType*>(
                                     &(pOEMInput->US20Setting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
                pReserveData   = reinterpret_cast<upscale_2_0_0::chromatix_upscale20_reserveType*>(
                                     &(pOEMInput->US20ReserveType));
                pModuleEnable  = reinterpret_cast<upscale_2_0_0::chromatix_upscale20Type::enable_sectionStruct*>(
                                     &(pOEMInput->US20EnableSection));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            Upscale20UnpackedField unpackedRegDataUpscale = { 0 };

            commonIQResult = IQInterface::s_interpolationTable.upscale20CalculateHWSetting(
                                 pInput,
                                 pInterpolationData,
                                 pReserveData,
                                 pModuleEnable,
                                 &unpackedRegDataUpscale);

            if (TRUE == commonIQResult)
            {
                pHWSetting->PackIQRegisterSetting(&unpackedRegDataUpscale, NULL);
                IPESetupUpscale20Register(&unpackedRegDataUpscale, pOutput);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("IPEUpscale20 calculate hardware setting failed");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("Upscale2_0 interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL pInput = %p, pOutput = %p ", pInput, pOutput);
    }

    CAMX_ASSERT(result == CamxResultSuccess);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQInterface::IPEUpscale12CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IPEUpscale12CalculateSetting(
    const Upscale12InputData* pInput,
    VOID*                     pOEMIQData)
{
    CamxResult            result           = CamxResultSuccess;
    BOOL                  commonIQResult   = FALSE;
    ISPHWSetting*         pHWSetting       = NULL;

    if ((NULL != pInput) && (NULL != pInput->pHWSetting))
    {
        commonIQResult = TRUE;
        pHWSetting     = static_cast<ISPHWSetting*>(pInput->pHWSetting);

        if (NULL != pOEMIQData)
        {
            OEMIPEIQSetting* pOEMInput = static_cast<OEMIPEIQSetting*>(pOEMIQData);
            // Use OEM defined interpolation data
            CAMX_UNREFERENCED_PARAM(pOEMIQData);
        }
    }

    if (TRUE == commonIQResult)
    {
        Upscale12UnpackedField unpackedRegDataUpscale = { 0 };

        commonIQResult = IQInterface::s_interpolationTable.upscale12CalculateHWSetting(
                             pInput,
                             &unpackedRegDataUpscale);

        if (TRUE == commonIQResult)
        {
            pHWSetting->PackIQRegisterSetting(&unpackedRegDataUpscale, NULL);
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("IPEUpscale12 calculate hardware setting failed");
        }

    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL pInput = %p", pInput);
    }

    CAMX_ASSERT(result == CamxResultSuccess);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPESetupGRA10Register
///
/// @brief  Setup GrainAdder10 Register based on common library output
///
/// @param  pUnpackedRegisterData    Pointer to the Common Library Calculation Result
/// @param  pOutput                  Pointer to the GrainAdder10 module Output
///
/// @return VOID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID IPESetupGRA10Register(
    GRA10UnpackedField* pUnpackedRegisterData,
    GRA10OutputData*    pOutput)
{
    CAMX_ASSERT(NULL != pUnpackedRegisterData);
    CAMX_ASSERT(NULL != pOutput);

    pOutput->enableDitheringC = pUnpackedRegisterData->enable_dithering_C;
    pOutput->enableDitheringY = pUnpackedRegisterData->enable_dithering_Y;
    pOutput->grainSeed        = pUnpackedRegisterData->grain_seed;
    pOutput->grainStrength    = pUnpackedRegisterData->grain_strength;
    pOutput->mcgA             = pUnpackedRegisterData->mcg_a;
    pOutput->skiAheadAJump    = pUnpackedRegisterData->skip_ahead_a_jump;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::IPEGRA10CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::IPEGRA10CalculateSetting(
    const GRA10IQInput* pInput,
    VOID*               pOEMIQData,
    GRA10OutputData*    pOutput)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pInput) && (NULL != pOutput))
    {
        BOOL                                                  commonIQResult     = FALSE;
        gra_1_0_0::chromatix_gra10_reserveType*               pReserveData       = NULL;
        gra_1_0_0::chromatix_gra10Type::enable_sectionStruct* pModuleEnable      = NULL;
        gra_1_0_0::gra10_rgn_dataType*                        pInterpolationData = NULL;

        if (NULL == pOEMIQData)
        {
            pInterpolationData = static_cast<gra_1_0_0::gra10_rgn_dataType*>(pInput->pInterpolationData);
            pReserveData       = &(pInput->pChromatix->chromatix_gra10_reserve);
            pModuleEnable      = &(pInput->pChromatix->enable_section);

            // Call the Interpolation Calculation
            commonIQResult     = IQInterface::s_interpolationTable.gra10Interpolation(pInput, pInterpolationData);
        }
        else
        {
            OEMIPEIQSetting* pOEMInput = static_cast<OEMIPEIQSetting*>(pOEMIQData);

            // Use OEM defined interpolation data
            pInterpolationData = reinterpret_cast<gra_1_0_0::gra10_rgn_dataType*>(
                                     &(pOEMInput->GrainAdderSetting));

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;
                pReserveData   = reinterpret_cast<gra_1_0_0::chromatix_gra10_reserveType*>(
                                     &(pOEMInput->GrainAdderReserveType));
                pModuleEnable  = reinterpret_cast<gra_1_0_0::chromatix_gra10Type::enable_sectionStruct*>(
                                     &(pOEMInput->GrainAdderEnableSection));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            GRA10UnpackedField unpackedRegisterData = { 0 };

            // set the unpacked register data LUT pointers to DMI memory buffer
            unpackedRegisterData.ch0_LUT.pGRATable  = pOutput->pLUT[GRALUTChannel0];
            unpackedRegisterData.ch1_LUT.numEntries = GRA10LUTNumEntriesPerChannelSize;
            unpackedRegisterData.ch1_LUT.pGRATable  = pOutput->pLUT[GRALUTChannel1];
            unpackedRegisterData.ch1_LUT.numEntries = GRA10LUTNumEntriesPerChannelSize;
            unpackedRegisterData.ch2_LUT.pGRATable  = pOutput->pLUT[GRALUTChannel2];
            unpackedRegisterData.ch2_LUT.numEntries = GRA10LUTNumEntriesPerChannelSize;

            commonIQResult = IQInterface::s_interpolationTable.gra10CalculateHWSetting(
                                 pInput,
                                 pInterpolationData,
                                 pReserveData,
                                 pModuleEnable,
                                 &unpackedRegisterData);

            if (TRUE == commonIQResult)
            {
                IPESetupGRA10Register(&unpackedRegisterData, pOutput);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("IPEGRA10 calculate hardware setting failed");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("GRA1_0 interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is NULL pInput: %p, pOutput: %p ", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::TMC11CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::TMC11CalculateSetting(
    TMC11InputData*  pInput)
{
    CamxResult                    result         = CamxResultSuccess;
    BOOL                          commonIQResult = FALSE;
    tmc_1_1_0::tmc11_rgn_dataType interpolationDataTMC[15];

    if (NULL != pInput)
    {
        commonIQResult = IQInterface::s_interpolationTable.TMC11Interpolation(pInput, interpolationDataTMC);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IQInterface::TMC12CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::TMC12CalculateSetting(
    TMC12InputData*  pInput)
{
    CamxResult                    result         = CamxResultSuccess;
    BOOL                          commonIQResult = FALSE;
    tmc_1_2_0::tmc12_rgn_dataType interpolationDataTMC[15];

    if (NULL != pInput)
    {
        commonIQResult = IQInterface::s_interpolationTable.TMC12Interpolation(pInput, interpolationDataTMC);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQInterface::LENR10CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::LENR10CalculateSetting(
    const LENR10InputData* pInput,
    VOID*                 pOEMIQData,
    LENR10OutputData*     pOutput)
{
    CamxResult                                              result             = CamxResultSuccess;
    BOOL                                                    commonIQResult     = FALSE;
    lenr_1_0_0::lenr10_rgn_dataType*                        pInterpolationData = NULL;
    lenr_1_0_0::chromatix_lenr10_reserveType                reserveType        = { 0 };
    lenr_1_0_0::chromatix_lenr10_reserveType*               pReserveType       = &reserveType;
    lenr_1_0_0::chromatix_lenr10Type::enable_sectionStruct  moduleEnable       = { 0 };
    lenr_1_0_0::chromatix_lenr10Type::enable_sectionStruct* pModuleEnable      = &moduleEnable;
    ISPHWSetting*                                           pHWSetting         = NULL;
    lenr_1_0_0::lenr10_rgn_dataType                         interpolationData;

    if ((NULL != pInput) && (NULL != pInput->pHWSetting) &&
        (NULL != pOutput) && (NULL != pInput->pInterpolationData))
    {
        pHWSetting = static_cast<ISPHWSetting*>(pInput->pHWSetting);

        pInterpolationData = static_cast<lenr_1_0_0::lenr10_rgn_dataType*>(pInput->pInterpolationData);
        if (NULL == pOEMIQData)
        {
            // Call the Interpolation Calculation
            if (NULL != pInterpolationData)
            {
                commonIQResult = IQInterface::s_interpolationTable.LENR10Interpolation(pInput, pInterpolationData);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid InterpolationData pointer");
            }
            Utils::Memcpy(pReserveType,
                          &(pInput->pChromatix->chromatix_lenr10_reserve),
                          sizeof(lenr_1_0_0::chromatix_lenr10_reserveType));
            Utils::Memcpy(pModuleEnable,
                          &(pInput->pChromatix->enable_section),
                          sizeof(lenr_1_0_0::chromatix_lenr10Type::enable_sectionStruct));
        }
        else
        {

            OEMIPEIQSetting* pOEMInput = static_cast<OEMIPEIQSetting*>(pOEMIQData);

            if (NULL != pInterpolationData)
            {
                commonIQResult = TRUE;

                // Copy OEM defined interpolation data
                Utils::Memcpy(pInterpolationData, &(pOEMInput->LENRSetting), sizeof(lenr_1_0_0::lenr10_rgn_dataType));

                Utils::Memcpy(pReserveType,
                              &(pOEMInput->LENRReserveType),
                              sizeof(lenr_1_0_0::chromatix_lenr10_reserveType));
                Utils::Memcpy(pModuleEnable,
                              &(pOEMInput->LENREnableSection),
                              sizeof(lenr_1_0_0::chromatix_lenr10Type::enable_sectionStruct));
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_ASSERT_ALWAYS_MESSAGE("No OEM Input Data");
            }
        }

        if (TRUE == commonIQResult)
        {
            LENR10UnpackedField unpackedRegData;
            Utils::Memset(&unpackedRegData, 0, sizeof(unpackedRegData));
            commonIQResult =
                IQInterface::s_interpolationTable.LENR10CalculateHWSetting(pInput,
                    pInterpolationData,
                    pReserveType,
                    pModuleEnable,
                    static_cast<VOID*>(&unpackedRegData));
            if (TRUE == commonIQResult)
            {
                VOID* pInputData = static_cast<VOID*>(&unpackedRegData);
                pHWSetting->PackIQRegisterSetting(pInputData, pOutput);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupISP, "LENR10 calculate hardwware setting failed");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("LENR10 interpolation failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is pInput %p pOutput %p", pInput, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQInterface::CVP10CalculateSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IQInterface::CVP10CalculateSetting(
    CVP10InputData*    pInput,
    ISPInputData*      pInputData,
    CVP10OutputData*   pOutput)
{
    CamxResult                               result             = CamxResultSuccess;
    BOOL                                     commonIQResult     = FALSE;
    cvp_1_0_0::cvp10_rgn_dataType*           pInterpolationData = NULL;
    cvp_1_0_0::chromatix_cvp10_reserveType*  pReserveType       = NULL;

    if ((NULL != pInput) && (NULL != pInput->pInterpolationData) &&
        (NULL != pOutput) && (NULL != pInput->pChromatix))
    {
        commonIQResult = IQInterface::s_interpolationTable.CVP10TriggerUpdate(&(pInputData->triggerData), pInput);

        if (TRUE == commonIQResult)
        {
            pInterpolationData = static_cast<cvp_1_0_0::cvp10_rgn_dataType*>(pInput->pInterpolationData);

            // Call the Interpolation Calculation
            commonIQResult = IQInterface::s_interpolationTable.CVP10Interpolation(pInput, pInterpolationData);
            pReserveType = &(pInput->pChromatix->chromatix_cvp10_reserve);

            if (TRUE == commonIQResult)
            {
                CVP10UnpackedField unpackedRegData;
                Utils::Memset(&unpackedRegData, 0, sizeof(unpackedRegData));

                commonIQResult =
                    IQInterface::s_interpolationTable.CVP10CalculateHWSetting(pInput,
                        pInterpolationData,
                        pReserveType,
                        static_cast<VOID*>(&unpackedRegData));

                if (TRUE == commonIQResult)
                {
                    Utils::Memcpy(pOutput, &unpackedRegData, sizeof(CVP10OutputData));
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_LOG_ERROR(CamxLogGroupCVP, "CVP10 calculate hardwware setting failed");
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupCVP, "CVP10 interpolation failed");
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCVP, "Input data is NULL");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQInterface::DumpICAGrid
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IQInterface::DumpICAGrid(
    FILE* pFile,
    const NcLibIcaGrid* pGrid)
{
    if (NULL != pGrid)
    {
        UINT   ICAGridGeometry = pGrid->geometry;
        UINT32 columns         = 0;
        UINT32 rows            = 0;

        if (TRUE == pGrid->enable)
        {
            CamX::OsUtils::FPrintF(pFile, "enable = %d\n", pGrid->enable);
            CamX::OsUtils::FPrintF(pFile, "transform width = %d\n", pGrid->transformDefinedOnWidth);
            CamX::OsUtils::FPrintF(pFile, "transform height = %d\n", pGrid->transformDefinedOnHeight);
            CamX::OsUtils::FPrintF(pFile, "transform geometry = %d\n", ICAGridGeometry);

            switch (ICAGridGeometry)
            {
                case ICAGeometryCol67Row51:
                    columns = ICA30GridTransformWidth;
                    rows    = ICA30GridTransformHeight;
                    break;
                case ICAGeometryCol35Row27:
                    columns = ICA20GridTransformWidth;
                    rows    = ICA20GridTransformHeight;
                    break;
                case ICAGeometryCol33Row25:
                    columns = ICA10GridTransformWidth;
                    rows    = ICA10GridTransformHeight;
                    break;
                default:
                    CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid ica grid geometry");
                    break;
            }

            for (UINT i = 0; i < (columns * rows); i++)
            {
                CamX::OsUtils::FPrintF(pFile, "i %d, gridx %f\n", i, pGrid->grid[i].x);
                CamX::OsUtils::FPrintF(pFile, "i %d  gridy %f\n", i, pGrid->grid[i].y);
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid grid ptr");
    }
}

CAMX_NAMESPACE_END
