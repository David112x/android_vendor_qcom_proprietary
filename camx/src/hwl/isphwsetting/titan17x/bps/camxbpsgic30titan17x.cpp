////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpsgic30titan17x.cpp
/// @brief CAMXBPSGIC30TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "bpsgic30setting.h"
#include "camxbpsgic30titan17x.h"
#include "camxiqinterface.h"
#include "titan170_bps.h"

CAMX_NAMESPACE_BEGIN

static const UINT32 GICNoiseLUT = BPS_BPS_0_CLC_GIC_DMI_LUT_CFG_LUT_SEL_NOISE_LUT;
static const UINT32 GICLUTBank0 = BPS_BPS_0_CLC_GIC_DMI_LUT_BANK_CFG_BANK_SEL_LUT_BANK0;
static const UINT32 GICLUTBank1 = BPS_BPS_0_CLC_GIC_DMI_LUT_BANK_CFG_BANK_SEL_LUT_BANK1;

CAMX_BEGIN_PACKED

/// @brief GIC register Configuration
struct BPSGIC30RegConfig
{
    BPS_BPS_0_CLC_GIC_GIC_FILTER_CFG          filterConfig;             ///< GIC Filter config
    BPS_BPS_0_CLC_GIC_THIN_LINE_NOISE_OFFSET  lineNoiseOffset;          ///< GIC thin line noise offset
    BPS_BPS_0_CLC_GIC_ANCHOR_BASE_SETTINGS_0  anchorBaseSetting0;       ///< GIC anchor and base setting 0
    BPS_BPS_0_CLC_GIC_ANCHOR_BASE_SETTINGS_1  anchorBaseSetting1;       ///< GIC anchor and base setting 1
    BPS_BPS_0_CLC_GIC_ANCHOR_BASE_SETTINGS_2  anchorBaseSetting2;       ///< GIC anchor and base setting 2
    BPS_BPS_0_CLC_GIC_ANCHOR_BASE_SETTINGS_3  anchorBaseSetting3;       ///< GIC anchor and base setting 3
    BPS_BPS_0_CLC_GIC_ANCHOR_BASE_SETTINGS_4  anchorBaseSetting4;       ///< GIC anchor and base setting 4
    BPS_BPS_0_CLC_GIC_ANCHOR_BASE_SETTINGS_5  anchorBaseSetting5;       ///< GIC anchor and base setting 5
    BPS_BPS_0_CLC_GIC_SLOPE_SHIFT_SETTINGS_0  slopeShiftSetting0;       ///< GIC slope shift setting 0
    BPS_BPS_0_CLC_GIC_SLOPE_SHIFT_SETTINGS_1  slopeShiftSetting1;       ///< GIC slope shift setting 1
    BPS_BPS_0_CLC_GIC_SLOPE_SHIFT_SETTINGS_2  slopeShiftSetting2;       ///< GIC slope shift setting 2
    BPS_BPS_0_CLC_GIC_SLOPE_SHIFT_SETTINGS_3  slopeShiftSetting3;       ///< GIC slope shift setting 3
    BPS_BPS_0_CLC_GIC_SLOPE_SHIFT_SETTINGS_4  slopeShiftSetting4;       ///< GIC slope shift setting 4
    BPS_BPS_0_CLC_GIC_SLOPE_SHIFT_SETTINGS_5  slopeShiftSetting5;       ///< GIC slope shift setting 5
    BPS_BPS_0_CLC_GIC_INIT_HV_OFFSET          horizontalVerticalOffset; ///< GIC horizontal and vertical offset
    BPS_BPS_0_CLC_GIC_R_SQUARE_INIT           squareInit;               ///< GIC R square init
    BPS_BPS_0_CLC_GIC_R_SCALE_SHIFT           scaleShift;               ///< GIC scale shift
    BPS_BPS_0_CLC_GIC_PNR_NOISE_SCALE_0       noiseRatioScale0;         ///< GIC PNR noise scale 0
    BPS_BPS_0_CLC_GIC_PNR_NOISE_SCALE_1       noiseRatioScale1;         ///< GIC PNR noise scale 1
    BPS_BPS_0_CLC_GIC_PNR_NOISE_SCALE_2       noiseRatioScale2;         ///< GIC PNR noise scale 2
    BPS_BPS_0_CLC_GIC_PNR_NOISE_SCALE_3       noiseRatioScale3;         ///< GIC PNR noise scale 3
    BPS_BPS_0_CLC_GIC_PNR_FILTER_STRENGTH     noiseRatioFilterStrength; ///< GIC noise filter strength
} CAMX_PACKED;

/// @brief GIC module Configuration
struct BPSGIC30ModuleConfig
{
    BPS_BPS_0_CLC_GIC_MODULE_CFG  moduleConfig;    ///< Module Configuration
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 BPSGIC30RegLengthDword = sizeof(BPSGIC30RegConfig) / RegisterWidthInBytes;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30Titan17x::BPSGIC30Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSGIC30Titan17x::BPSGIC30Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30Titan17x::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGIC30Titan17x::Create(
    ISPHWSetting** ppHWSetting)
{
    CamxResult result = CamxResultSuccess;

    BPSGIC30Titan17x* pHWSetting = CAMX_NEW BPSGIC30Titan17x;

    if (NULL != pHWSetting)
    {
        result = pHWSetting->Initialize();
        if (CamxResultSuccess == result)
        {
            (*ppHWSetting) = pHWSetting;
        }
        else
        {
            CAMX_DELETE pHWSetting;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to initialize in %s, no memory", __FUNCTION__);
        }
    }
    else
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "%s failed, no memory", __FUNCTION__);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSGIC30Titan17x::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGIC30Titan17x::Initialize()
{
    CamxResult result = CamxResultSuccess;

    m_pModuleConfig = CAMX_CALLOC(sizeof(BPSGIC30ModuleConfig));

    if (NULL == m_pModuleConfig)
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "No memory for module configuration!");
    }
    else
    {
        m_pRegCmd = CAMX_CALLOC(sizeof(BPSGIC30RegConfig));

        if (NULL != m_pRegCmd)
        {
            SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(BPSGIC30RegLengthDword));
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "No memory for register configuration!");

            CAMX_FREE(m_pModuleConfig);
            m_pModuleConfig = NULL;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30Titan17x::WriteLUTtoDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGIC30Titan17x::WriteLUTtoDMI(
    VOID* pInput)
{
    CamxResult    result     = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pInput);

    // CDM pack the DMI buffer and patch the Red and Blue LUT DMI buffer into CDM pack
    if ((NULL != pInputData) && (NULL != pInputData->pDMICmdBuffer))
    {
        result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                         regBPS_BPS_0_CLC_GIC_DMI_CFG,
                                         GICNoiseLUT,
                                         pInputData->p64bitDMIBuffer,
                                         0,
                                         BPSGICNoiseLUTBufferSize);
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGIC30Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    CamxResult         result     = CamxResultSuccess;
    ISPInputData*      pInputData = static_cast<ISPInputData*>(pSettingData);
    BPSGIC30RegConfig* pRegCmd    = static_cast<BPSGIC30RegConfig*>(m_pRegCmd);

    result = WriteLUTtoDMI(pInputData);

    if ((CamxResultSuccess == result) && (NULL != pInputData->pCmdBuffer))
    {
        CmdBuffer* pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regBPS_BPS_0_CLC_GIC_GIC_FILTER_CFG,
                                              BPSGIC30RegLengthDword,
                                              reinterpret_cast<UINT32*>(pRegCmd));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to write command buffer");
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30Titan17x::UpdateFirmwareData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGIC30Titan17x::UpdateFirmwareData(
    VOID*   pSettingData,
    BOOL    moduleEnable)
{
    ISPInputData*         pInputData     = static_cast<ISPInputData*>(pSettingData);
    BpsIQSettings*        pBPSIQSettings = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    BPSGIC30ModuleConfig* pModuleCfg     = static_cast<BPSGIC30ModuleConfig*>(m_pModuleConfig);
    CAMX_ASSERT(NULL != pBPSIQSettings);

    pBPSIQSettings->gicParameters.moduleCfg.EN     = moduleEnable;

    pBPSIQSettings->gicParameters.moduleCfg.PNR_EN = pModuleCfg->moduleConfig.bitfields.PNR_EN;
    pBPSIQSettings->gicParameters.moduleCfg.GIC_EN = pModuleCfg->moduleConfig.bitfields.GIC_EN;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGIC30Titan17x::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult          result             = CamxResultSuccess;
    ISPInputData*       pInputData         = static_cast<ISPInputData*>(pSettingData);
    BPSTuningMetadata*  pBPSTuningMetadata = static_cast<BPSTuningMetadata*>(pInputData->pBPSTuningMetadata);
    BpsIQSettings*      pBPSIQSettings     = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    BPSGIC30RegConfig*  pRegCmd            = static_cast<BPSGIC30RegConfig*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if (NULL != pBPSTuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(BPSGIC30RegConfig) <=
            sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSGICData.filterConfig));

        Utils::Memcpy(&pBPSTuningMetadata->BPSTuningMetadata17x.BPSGICData.filterConfig,
                        pRegCmd,
                        sizeof(BPSGIC30RegConfig));

        if (TRUE == pBPSIQSettings->gicParameters.moduleCfg.EN)
        {
            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                DebugDataTagID::TuningBPSGICRegister,
                DebugDataTagType::UInt32,
                CAMX_ARRAY_SIZE(pBPSTuningMetadata->BPSTuningMetadata17x.BPSGICData.filterConfig),
                &pBPSTuningMetadata->BPSTuningMetadata17x.BPSGICData.filterConfig,
                sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSGICData.filterConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGIC30Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result      = CamxResultSuccess;
    GIC30UnpackedField* pData       = static_cast<GIC30UnpackedField*>(pInput);
    GIC30OutputData*    pOutputData = static_cast<GIC30OutputData*>(pOutput);

    BPSGIC30RegConfig*    pRegCmd    = static_cast<BPSGIC30RegConfig*>(m_pRegCmd);
    BPSGIC30ModuleConfig* pModuleCfg = static_cast<BPSGIC30ModuleConfig*>(m_pModuleConfig);


    if ((NULL != pData) && (NULL != pOutputData))
    {
        pRegCmd->filterConfig.bitfields.GIC_FILTER_STRENGTH = pData->GICStrength;
        pRegCmd->filterConfig.bitfields.GIC_NOISE_SCALE     = pData->GICNoiseScale;

        pRegCmd->horizontalVerticalOffset.bitfields.BX = pData->bx;
        pRegCmd->horizontalVerticalOffset.bitfields.BY = pData->by;

        pRegCmd->lineNoiseOffset.bitfields.VALUE = pData->thinLineNoiseOffset;

        pRegCmd->anchorBaseSetting0.bitfields.ANCHOR_TABLE      = pData->RNRAnchor[0];
        pRegCmd->anchorBaseSetting0.bitfields.BASE_TABLE        = pData->RNRBase[0];
        pRegCmd->anchorBaseSetting1.bitfields.ANCHOR_TABLE      = pData->RNRAnchor[1];
        pRegCmd->anchorBaseSetting1.bitfields.BASE_TABLE        = pData->RNRBase[1];
        pRegCmd->anchorBaseSetting2.bitfields.ANCHOR_TABLE      = pData->RNRAnchor[2];
        pRegCmd->anchorBaseSetting2.bitfields.BASE_TABLE        = pData->RNRBase[2];
        pRegCmd->anchorBaseSetting3.bitfields.ANCHOR_TABLE      = pData->RNRAnchor[3];
        pRegCmd->anchorBaseSetting3.bitfields.BASE_TABLE        = pData->RNRBase[3];
        pRegCmd->anchorBaseSetting4.bitfields.ANCHOR_TABLE      = pData->RNRAnchor[4];
        pRegCmd->anchorBaseSetting4.bitfields.BASE_TABLE        = pData->RNRBase[4];
        pRegCmd->anchorBaseSetting5.bitfields.ANCHOR_TABLE      = pData->RNRAnchor[5];
        pRegCmd->anchorBaseSetting5.bitfields.BASE_TABLE        = pData->RNRBase[5];

        pRegCmd->slopeShiftSetting0.bitfields.SHIFT_TABLE       = pData->RNRShift[0];
        pRegCmd->slopeShiftSetting0.bitfields.SLOPE_TABLE       = pData->RNRSlope[0];
        pRegCmd->slopeShiftSetting1.bitfields.SHIFT_TABLE       = pData->RNRShift[1];
        pRegCmd->slopeShiftSetting1.bitfields.SLOPE_TABLE       = pData->RNRSlope[1];
        pRegCmd->slopeShiftSetting2.bitfields.SHIFT_TABLE       = pData->RNRShift[2];
        pRegCmd->slopeShiftSetting2.bitfields.SLOPE_TABLE       = pData->RNRSlope[2];
        pRegCmd->slopeShiftSetting3.bitfields.SHIFT_TABLE       = pData->RNRShift[3];
        pRegCmd->slopeShiftSetting3.bitfields.SLOPE_TABLE       = pData->RNRSlope[3];
        pRegCmd->slopeShiftSetting4.bitfields.SHIFT_TABLE       = pData->RNRShift[4];
        pRegCmd->slopeShiftSetting4.bitfields.SLOPE_TABLE       = pData->RNRSlope[4];
        pRegCmd->slopeShiftSetting5.bitfields.SHIFT_TABLE       = pData->RNRShift[5];
        pRegCmd->slopeShiftSetting5.bitfields.SLOPE_TABLE       = pData->RNRSlope[5];

        pRegCmd->noiseRatioFilterStrength.bitfields.VALUE       = pData->PNRStrength;
        pRegCmd->noiseRatioScale0.bitfields.R_SQUARE_SCALE      = pData->PNRNoiseScale[0];
        pRegCmd->noiseRatioScale1.bitfields.R_SQUARE_SCALE      = pData->PNRNoiseScale[1];
        pRegCmd->noiseRatioScale2.bitfields.R_SQUARE_SCALE      = pData->PNRNoiseScale[2];
        pRegCmd->noiseRatioScale3.bitfields.R_SQUARE_SCALE      = pData->PNRNoiseScale[3];

        pRegCmd->scaleShift.bitfields.R_SQUARE_SCALE            = pData->rSquareScale;
        pRegCmd->scaleShift.bitfields.R_SQUARE_SHIFT            = pData->rSquareShift;
        pRegCmd->squareInit.bitfields.VALUE                     = pData->rSquareInit;

        pModuleCfg->moduleConfig.bitfields.PNR_EN               = pData->enablePNR;
        pModuleCfg->moduleConfig.bitfields.GIC_EN               = pData->enableGIC;

        Utils::Memcpy(pOutputData->pDMIData,
            &pData->noiseStdLUTLevel0[pData->LUTBankSel][0],
            DMIRAM_GIC_NOISESTD_LENGTH_V30 * sizeof(UINT32));
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid data's %p %p", pOutputData, pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30Titan17x::~BPSGIC30Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSGIC30Titan17x::~BPSGIC30Titan17x()
{
    CAMX_FREE(m_pModuleConfig);
    m_pModuleConfig = NULL;

    CAMX_FREE(m_pRegCmd);
    m_pRegCmd = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSGIC30Titan17x::DumpRegConfig()
{
    BPSGIC30RegConfig* pRegCmd = static_cast<BPSGIC30RegConfig*>(m_pRegCmd);

    if (NULL != pRegCmd)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "filterConfig             = %x\n", pRegCmd->filterConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lineNoiseOffset          = %x\n", pRegCmd->lineNoiseOffset);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "anchorBaseSetting0       = %x\n", pRegCmd->anchorBaseSetting0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "anchorBaseSetting1       = %x\n", pRegCmd->anchorBaseSetting1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "anchorBaseSetting2       = %x\n", pRegCmd->anchorBaseSetting2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "anchorBaseSetting3       = %x\n", pRegCmd->anchorBaseSetting3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "anchorBaseSetting4       = %x\n", pRegCmd->anchorBaseSetting4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "anchorBaseSetting5       = %x\n", pRegCmd->anchorBaseSetting5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "slopeShiftSetting0       = %x\n", pRegCmd->slopeShiftSetting0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "slopeShiftSetting1       = %x\n", pRegCmd->slopeShiftSetting1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "slopeShiftSetting2       = %x\n", pRegCmd->slopeShiftSetting2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "slopeShiftSetting3       = %x\n", pRegCmd->slopeShiftSetting3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "slopeShiftSetting4       = %x\n", pRegCmd->slopeShiftSetting4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "slopeShiftSetting5       = %x\n", pRegCmd->slopeShiftSetting5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "horizontalVerticalOffset = %x\n", pRegCmd->horizontalVerticalOffset);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "squareInit               = %x\n", pRegCmd->squareInit);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "scaleShift               = %x\n", pRegCmd->scaleShift);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "noiseRatioScale0         = %x\n", pRegCmd->noiseRatioScale0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "noiseRatioScale1         = %x\n", pRegCmd->noiseRatioScale1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "noiseRatioScale2         = %x\n", pRegCmd->noiseRatioScale2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "noiseRatioScale3         = %x\n", pRegCmd->noiseRatioScale3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "noiseRatioFilterStrength = %x\n", pRegCmd->noiseRatioFilterStrength);
    }
}

CAMX_NAMESPACE_END
