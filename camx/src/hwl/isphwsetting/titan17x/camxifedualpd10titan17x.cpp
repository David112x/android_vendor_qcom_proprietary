////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifedualpd10titan17x.cpp
/// @brief CAMXIFEDUALPD10TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifedualpd10titan17x.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDualPD10Titan17x::IFEDualPD10Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDualPD10Titan17x::IFEDualPD10Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEDUALPD10RegLengthDWord) +
                     PacketBuilder::RequiredWriteDMISizeInDwords());
    Set32bitDMILength(IFEDualPD10LutTableSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDualPD10Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDualPD10Titan17x::CreateCmdList(
    VOID* pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result            = CamxResultSuccess;
    ISPInputData* pInputData        = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer        = NULL;
    CmdBuffer*    pDMIBuffer        = NULL;
    UINT32        offset            = *pDMIBufferOffset * sizeof(UINT32);
    UINT32        lengthInByte      = IFEDualPD10LutTableSize * sizeof(UINT32);
    UINT8         bankSelect        = 0;

    if (NULL != pInputData                  &&
        NULL != pInputData->pCmdBuffer      &&
        NULL != pInputData->p32bitDMIBuffer)
    {
        pCmdBuffer = pInputData->pCmdBuffer;
        pDMIBuffer = pInputData->p32bitDMIBuffer;
        result     = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_MODULE_DUAL_PD_EN,
                                                  1,
                                                  reinterpret_cast<UINT32*>(&m_pdModuleEnable));
        result     = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_DUAL_PD_CFG,
                                                  IFEDUALPD10RegLengthDWord,
                                                  reinterpret_cast<UINT32*>(&m_regCmd));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to write command buffer. result = %d", result);
        }

        bankSelect = (m_regCmd.pdCfg.bitfields.GAIN_MAP_LUT_BANK_SEL == 0) ? DualPDLUTBank0 : DualPDLUTBank1;
        result     = PacketBuilder::WriteDMI(pCmdBuffer,
                                         regIFE_IFE_0_VFE_DMI_CFG,
                                         static_cast<UINT8>(bankSelect),
                                         pDMIBuffer,
                                         offset,
                                         lengthInByte);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to write DMI buffer. result = %d", result);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDualPD10Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDualPD10Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result      = CamxResultSuccess;

    ISPInputData*       pInputData  = static_cast<ISPInputData*>(pInput);
    DualPD10OutputData* pOutputData = static_cast<DualPD10OutputData*>(pOutput);

    if ((NULL != pOutputData) && (NULL != pInputData) && (NULL != pOutputData->pDMIDataPtr))
    {
        PDHwConfig* pPDHwConfig    = pInputData->pPDHwConfig;
        UINT32*     pDMIDataBuffer = reinterpret_cast<UINT32*>(pOutputData->pDMIDataPtr);
        m_pdModuleEnable.pdModuleEnable.bitfields.DUAL_PD_EN = pPDHwConfig->enablePDHw;

        m_regCmd.pdCfg.bitfields.CROP_EN                    = pPDHwConfig->cropConfig.enablePDCrop;
        m_regCmd.pdCfg.bitfields.HDR_EN                     = pPDHwConfig->HDRConfig.hdrEnable;
        m_regCmd.pdCfg.bitfields.BIN_SKIP_EN                = pPDHwConfig->binConfig.enableSkipBinning;
        m_regCmd.pdCfg.bitfields.GAIN_MAP_EN                = pPDHwConfig->gainMapConfig.gainMapEnable;
        m_regCmd.pdCfg.bitfields.GAIN_MAP_LUT_BANK_SEL     ^= 1;
        m_regCmd.pdCfg.bitfields.IIR_EN                     = pPDHwConfig->IIRConfig.IIREnable;
        m_regCmd.pdCfg.bitfields.SAD_EN                     = pPDHwConfig->SADConfig.sadEnable;
        m_regCmd.pdCfg.bitfields.PD_FIRST_PIXEL_LT          = pPDHwConfig->firstPixelSelect;
        m_regCmd.pdCfg.bitfields.SAD_STATS_SEL              = pPDHwConfig->SADConfig.sadSelect;
        m_regCmd.pdCfg.bitfields.SAD_SHIFT                  = pPDHwConfig->SADConfig.sadShift;
        m_regCmd.pdCropWidthCfg.bitfields.LAST_PIXEL        = pPDHwConfig->cropConfig.lastPixelCrop;
        m_regCmd.pdCropWidthCfg.bitfields.FIRST_PIXEL       = pPDHwConfig->cropConfig.firstPixelCrop;
        m_regCmd.pdCropHeightCfg.bitfields.LAST_LINE        = pPDHwConfig->cropConfig.lastLineCrop;
        m_regCmd.pdCropHeightCfg.bitfields.FIRST_LINE       = pPDHwConfig->cropConfig.firstLineCrop;
        m_regCmd.pdBLSCfg.bitfields.LEFT_PIXEL_OFFSET       = pPDHwConfig->BLSConfig.leftPixelBLSCorrection;
        m_regCmd.pdBLSCfg.bitfields.RIGHT_PIXEL_OFFSET      = pPDHwConfig->BLSConfig.rightPixelBLSCorrection;
        m_regCmd.pdRegionOffsetCfg.bitfields.RGN_H_OFFSET   = pPDHwConfig->SADConfig.horizontalOffset;
        m_regCmd.pdRegionOffsetCfg.bitfields.RGN_V_OFFSET   = pPDHwConfig->SADConfig.verticalOffset;
        m_regCmd.pdRegionNumberCfg.bitfields.RGN_H_NUM      = pPDHwConfig->SADConfig.horizontalNumber;
        m_regCmd.pdRegionNumberCfg.bitfields.RGN_V_NUM      = pPDHwConfig->SADConfig.verticalNumber;
        m_regCmd.pdRegionSizeCfg.bitfields.RGN_HEIGHT       = pPDHwConfig->SADConfig.regionHeight;
        m_regCmd.pdRegionSizeCfg.bitfields.RGN_WIDTH        = pPDHwConfig->SADConfig.regionWidth;
        m_regCmd.pdHDRCfg.bitfields.FIRST_PIXEL_LG          = pPDHwConfig->HDRConfig.hdrFirstPixel;
        m_regCmd.pdHDRCfg.bitfields.MODE_SEL                = pPDHwConfig->HDRConfig.hdrModeSel;
        m_regCmd.pdHDRCfg.bitfields.LONG_TH                 = FLOAT_TO_Q(14, pPDHwConfig->HDRConfig.hdrThreshhold);
        m_regCmd.pdHDRCfg.bitfields.EXP_RATIO               = FLOAT_TO_Q(14, pPDHwConfig->HDRConfig.hdrExposureRatio) - 1;
        m_regCmd.pdBinSkipCfg.bitfields.H_SHIFT             = pPDHwConfig->binConfig.horizontalBinningSkip;
        m_regCmd.pdBinSkipCfg.bitfields.H_BIN_PIX_NUM       = pPDHwConfig->binConfig.horizontalBinningPixelCount;
        m_regCmd.pdBinSkipCfg.bitfields.V_BIN_LN_NUM        = pPDHwConfig->binConfig.verticalBinningLineCount;
        m_regCmd.pdBinSkipCfg.bitfields.V_DEC_LN_NUM        = pPDHwConfig->binConfig.verticalDecimateCount;
        m_regCmd.pdBinSkipCfg.bitfields.V_SHIFT             = pPDHwConfig->binConfig.verticalBinningSkip;
        m_regCmd.pdGMCfg0.bitfields.V_PHASE_INIT            = pPDHwConfig->gainMapConfig.verticalPhaseInit;
        m_regCmd.pdGMCfg1.bitfields.H_PHASE_INIT            = pPDHwConfig->gainMapConfig.horizontalPhaseInit;
        m_regCmd.pdGMCfg2.bitfields.V_NUM                   = pPDHwConfig->SADConfig.verticalNumber;
        m_regCmd.pdGMCfg2.bitfields.V_PHASE_STEP            = pPDHwConfig->gainMapConfig.verticalPhaseStep;
        m_regCmd.pdGMCfg3.bitfields.H_NUM                   = pPDHwConfig->SADConfig.horizontalNumber;
        m_regCmd.pdGMCfg3.bitfields.H_PHASE_STEP            = pPDHwConfig->gainMapConfig.horizontalPhaseStep;
        m_regCmd.pdIIRCfg0.bitfields.B0                     = FLOAT_TO_Q(13, pPDHwConfig->IIRConfig.b0);
        m_regCmd.pdIIRCfg0.bitfields.B1                     = FLOAT_TO_Q(13, pPDHwConfig->IIRConfig.b1);
        m_regCmd.pdIIRCfg1.bitfields.B2                     = FLOAT_TO_Q(13, pPDHwConfig->IIRConfig.b2);
        m_regCmd.pdIIRCfg2.bitfields.A0                     = FLOAT_TO_Q(13, pPDHwConfig->IIRConfig.a0);
        m_regCmd.pdIIRCfg2.bitfields.A1                     = FLOAT_TO_Q(13, pPDHwConfig->IIRConfig.a1);
        m_regCmd.pdIIRCfg3.bitfields.INIT_0                 = FLOAT_TO_Q(13, pPDHwConfig->IIRConfig.init0);
        m_regCmd.pdIIRCfg3.bitfields.INIT_1                 = FLOAT_TO_Q(13, pPDHwConfig->IIRConfig.init1);
        m_regCmd.pdSADPhaseCfg0.bitfields.PHASE_MASK        = pPDHwConfig->SADConfig.config0Phase;
        m_regCmd.pdSADPhaseCfg1.bitfields.PHASE_MASK        = pPDHwConfig->SADConfig.config1Phase;
        for (UINT32 i = 0; i < IFEDualPD10LutTableSize; i++)
        {
            pDMIDataBuffer[i] = pPDHwConfig->gainMapConfig.leftLUT[i] << 16 |
                pPDHwConfig->gainMapConfig.rightLUT[i];
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDualPD10Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDualPD10Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDualPD10Titan17x::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEDualPD10Titan17x::CopyRegCmd(
    VOID* pData)
{
    UINT32 dataCopied = 0;

    if (NULL != pData)
    {
        Utils::Memcpy(pData, &m_regCmd, sizeof(m_regCmd));
        dataCopied = sizeof(m_regCmd);
    }

    return dataCopied;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDualPD10Titan17x::~IFEDualPD10Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDualPD10Titan17x::~IFEDualPD10Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDualPD10Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDualPD10Titan17x::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdCfg             [0x%x]", m_regCmd.pdCfg.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdCropWidthCfg    [0x%x]", m_regCmd.pdCropWidthCfg.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdCropHeightCfg   [0x%x]", m_regCmd.pdCropHeightCfg.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdBLSCfg          [0x%x]", m_regCmd.pdBLSCfg.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdRegionOffsetCfg [0x%x]", m_regCmd.pdRegionOffsetCfg.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdRegionNumberCfg [0x%x]", m_regCmd.pdRegionNumberCfg.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdRegionSizeCfg   [0x%x]", m_regCmd.pdRegionSizeCfg.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdHDRCfg          [0x%x]", m_regCmd.pdHDRCfg.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdBinSkipCfg      [0x%x]", m_regCmd.pdBinSkipCfg.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdGMCfg0          [0x%x]", m_regCmd.pdGMCfg0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdGMCfg1          [0x%x]", m_regCmd.pdGMCfg1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdGMCfg2          [0x%x]", m_regCmd.pdGMCfg2.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdGMCfg3          [0x%x]", m_regCmd.pdGMCfg3.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdIIRCfg0         [0x%x]", m_regCmd.pdIIRCfg0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdIIRCfg1         [0x%x]", m_regCmd.pdIIRCfg1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdIIRCfg2         [0x%x]", m_regCmd.pdIIRCfg2.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdIIRCfg3         [0x%x]", m_regCmd.pdIIRCfg3.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdSADPhaseCfg0    [0x%x]", m_regCmd.pdSADPhaseCfg0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdSADPhaseCfg1    [0x%x]", m_regCmd.pdSADPhaseCfg1.u32All);
}

CAMX_NAMESPACE_END
