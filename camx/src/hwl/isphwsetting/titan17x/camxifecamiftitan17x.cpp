////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecamiftitan17x.cpp
/// @brief CAMXIFECAMIFTITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifecamiftitan17x.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFTitan17x::IFECAMIFTitan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECAMIFTitan17x::IFECAMIFTitan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFECAMIFRegLengthDWord1) +
             PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFECAMIFRegLengthDWord2) +
             PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFECAMIFRegLengthDWord3) +
             PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFECAMIFRegLengthDWord4));

    Utils::Memset(&m_regCmd1, 0, sizeof(m_regCmd1));
    Utils::Memset(&m_regCmd2, 0, sizeof(m_regCmd2));
    Utils::Memset(&m_regCmd3, 0, sizeof(m_regCmd3));
    Utils::Memset(&m_regCmd4, 0, sizeof(m_regCmd4));

    // Hardcode initial value for all the registers
    /// @todo (CAMX-677) This hardcode value is to support initial presil test. Will replace with memset to 0
    m_regCmd1.statusRegister.u32All           = 0x0;
    m_regCmd1.configRegister.u32All           = 0x0;
    m_regCmd2.lineSkipPatternRegister.u32All  = 0x0;
    m_regCmd2.pixelSkipPatternRegister.u32All = 0x0;
    m_regCmd2.skipPeriodRegister.u32All       = 0x0;
    m_regCmd3.irqSubsamplePattern.u32All      = 0x0;
    m_regCmd4.rawCropWidthConfig.u32All       = 0x0;
    m_regCmd4.rawCropHeightConfig.u32All      = 0x0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFTitan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIFTitan17x::CreateCmdList(
    VOID* pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if (NULL != pInputData && NULL != pInputData->pCmdBuffer)
    {
        pCmdBuffer = pInputData->pCmdBuffer;
        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_VFE_CAMIF_CMD,
                                              IFECAMIFRegLengthDWord1,
                                              reinterpret_cast<UINT32*>(&m_regCmd1));
        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_CAMIF_LINE_SKIP_PATTERN,
                                                  IFECAMIFRegLengthDWord2,
                                                  reinterpret_cast<UINT32*>(&m_regCmd2));
        }
        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_CAMIF_IRQ_SUBSAMPLE_PATTERN,
                                                  IFECAMIFRegLengthDWord3,
                                                  reinterpret_cast<UINT32*>(&m_regCmd3));
        }
        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_CAMIF_RAW_CROP_WIDTH_CFG,
                                                  IFECAMIFRegLengthDWord4,
                                                  reinterpret_cast<UINT32*>(&m_regCmd4));
        }
        if (CamxResultSuccess != result)
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Failed to write command buffer");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFTitan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIFTitan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult                    result        = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pInput);
    CAMX_UNREFERENCED_PARAM(pOutput);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFTitan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIFTitan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pInput);

    /// @todo (CAMX-677) This hardcode value is to support initial presil test. Will replace with memset to 0
    m_regCmd1.statusRegister.u32All           = 0x0;
    m_regCmd1.configRegister.u32All           = 0x0;
    m_regCmd2.lineSkipPatternRegister.u32All  = 0x0;
    m_regCmd2.pixelSkipPatternRegister.u32All = 0x0;
    m_regCmd2.skipPeriodRegister.u32All       = 0x0;
    m_regCmd3.irqSubsamplePattern.u32All      = 0x0;
    m_regCmd4.rawCropWidthConfig.u32All       = 0x0;
    m_regCmd4.rawCropHeightConfig.u32All      = 0x0;

     /// @todo (CAMX-677) Set up the regiser value based on the input data
    m_regCmd1.statusRegister.u32All                       = 0x0;
    m_regCmd1.statusRegister.bitfields.CLEAR_CAMIF_STATUS = 0x1;
    m_regCmd1.configRegister.u32All                       = 0x0;
    m_regCmd1.configRegister.bitfields.VFE_OUTPUT_EN      = 0x1;
    m_regCmd1.configRegister.bitfields.BUS_OUTPUT_EN      = 0x1;
    m_regCmd1.configRegister.bitfields.RAW_CROP_EN        = 0x1;
    m_regCmd1.configRegister.bitfields.BUS_SUBSAMPLE_EN   =
        pInputData->pStripeConfig->CAMIFSubsampleInfo.enableCAMIFSubsample;

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "PDAF pixelSkip 0x%X line SKip 0x%X",
        pInputData->pStripeConfig->CAMIFSubsampleInfo.CAMIFSubSamplePattern.pixelSkipPattern,
        pInputData->pStripeConfig->CAMIFSubsampleInfo.CAMIFSubSamplePattern.lineSkipPattern);

    m_regCmd2.lineSkipPatternRegister.u32All =
        pInputData->pStripeConfig->CAMIFSubsampleInfo.CAMIFSubSamplePattern.lineSkipPattern;
    m_regCmd2.pixelSkipPatternRegister.u32All =
        pInputData->pStripeConfig->CAMIFSubsampleInfo.CAMIFSubSamplePattern.pixelSkipPattern;

    CAMX_ASSERT(0 != pInputData->pipelineIFEData.numBatchedFrames);

    m_regCmd2.skipPeriodRegister.u32All                      = 0x0;
    m_regCmd3.irqSubsamplePattern.u32All                     = 1;
    m_regCmd2.skipPeriodRegister.bitfields.PIXEL_SKIP_PERIOD = 0xF;
    m_regCmd2.skipPeriodRegister.bitfields.LINE_SKIP_PERIOD  = 0xF;

    if (1 < pInputData->pipelineIFEData.numBatchedFrames)
    {
        m_regCmd2.skipPeriodRegister.bitfields.IRQ_SUBSAMPLE_PERIOD = pInputData->pipelineIFEData.numBatchedFrames - 1;
    }

    if (TRUE == pInputData->HVXData.DSEnabled)
    {
        m_regCmd4.rawCropWidthConfig.bitfields.LAST_PIXEL  = pInputData->HVXData.origCAMIFCrop.lastPixel;
        m_regCmd4.rawCropWidthConfig.bitfields.FIRST_PIXEL = pInputData->HVXData.origCAMIFCrop.firstPixel;
        m_regCmd4.rawCropHeightConfig.bitfields.LAST_LINE  = pInputData->HVXData.origCAMIFCrop.lastLine;
        m_regCmd4.rawCropHeightConfig.bitfields.FIRST_LINE = pInputData->HVXData.origCAMIFCrop.firstLine;
    }
    else
    {
        m_regCmd4.rawCropWidthConfig.bitfields.LAST_PIXEL  =
            pInputData->pStripeConfig->CAMIFSubsampleInfo.PDAFCAMIFCrop.lastPixel;
        m_regCmd4.rawCropWidthConfig.bitfields.FIRST_PIXEL =
            pInputData->pStripeConfig->CAMIFSubsampleInfo.PDAFCAMIFCrop.firstPixel;
        m_regCmd4.rawCropHeightConfig.bitfields.LAST_LINE  =
            pInputData->pStripeConfig->CAMIFSubsampleInfo.PDAFCAMIFCrop.lastLine;
        m_regCmd4.rawCropHeightConfig.bitfields.FIRST_LINE =
            pInputData->pStripeConfig->CAMIFSubsampleInfo.PDAFCAMIFCrop.firstLine;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFTitan17x::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFECAMIFTitan17x::CopyRegCmd(
    VOID* pData)
{
    UINT32 dataCopied = 0;

    if (NULL != pData)
    {
        Utils::Memcpy(pData, &m_regCmd1, sizeof(m_regCmd1));
        dataCopied = sizeof(m_regCmd1);
    }

    return dataCopied;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFTitan17x::~IFECAMIFTitan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECAMIFTitan17x::~IFECAMIFTitan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFTitan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECAMIFTitan17x::DumpRegConfig()
{
}

CAMX_NAMESPACE_END
