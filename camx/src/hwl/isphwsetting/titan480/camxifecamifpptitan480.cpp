////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecamifpptitan480.cpp
/// @brief IFECAMIFPPTitan480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifecamifpptitan480.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFPPTitan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIFPPTitan480::CreateCmdList(
    VOID* pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if ((NULL != pInputData) && (NULL != pInputData->pCmdBuffer))
    {
        pCmdBuffer = pInputData->pCmdBuffer;
        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              IFECamifPPRegCmd1Offset,
                                              IFECAMIFPPRegLengthDWord1,
                                              reinterpret_cast<UINT32*>(&m_regCmd1));
        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  IFECamifPPRegCmd2Offset,
                                                  IFECAMIFPPRegLengthDWord2,
                                                  reinterpret_cast<UINT32*>(&m_regCmd2));
        }
        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  IFECamifPPRegCmd3Offset,
                                                  IFECAMIFPPRegLengthDWord3,
                                                  reinterpret_cast<UINT32*>(&m_regCmd3));
        }
        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  IFECamifPPRegCmd4Offset,
                                                  IFECAMIFPPRegLengthDWord4,
                                                  reinterpret_cast<UINT32*>(&m_regCmd4));
        }
        if ((CamxResultSuccess == result) && (TRUE == pInputData->isInitPacket))
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_TOP_CORE_CFG_1,
                                                  IFEPixelRawRegLengthDWord,
                                                  reinterpret_cast<UINT32*>(&m_pixelRawRegCmd));
        }
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to write command buffer");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFPPTitan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIFPPTitan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pBayerPattern)
{
    CamxResult result           = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pInput);
    UINT8*        pPixelPattern = static_cast<UINT8*>(pBayerPattern);

    // Setup camif configuration
    m_regCmd1.configRegister.bitfields.EN                   = 0x1; // enable camif module
    m_regCmd1.configRegister.bitfields.STRIPE_LOC           = 0x0; // stripe location. For IFE, "0" is full frame
    m_regCmd1.configRegister.bitfields.IFE_OUTPUT_EN        = 0x1; // enable ife output

    // 0x0:RGRGRG, 0x1:GRGRGR, 0x2:BGBGBG, 0x3:GBGBGB,
    // 0x4:YCBYCR, 0x5:YCRYCB, 0x6:CBYCRY, 0x7:CRYCBY
    m_regCmd1.configRegister.bitfields.PIXEL_PATTERN        = *pPixelPattern; // Bayer pattern

    // Setup camif subample
    m_regCmd1.configRegister.bitfields.IFE_SUBSAMPLE_EN     = 0x0;        // default no subsample;
    m_regCmd3.irqSubsamplePatternRegister.bitfields.PATTERN = 0xFFFFFFFF; // no skip irq
    m_regCmd3.periodCongureRegister.u32All                  = 0x0;        // period is 1

    CAMX_ASSERT(0 != pInputData->pipelineIFEData.numBatchedFrames);
    if (1 < pInputData->pipelineIFEData.numBatchedFrames)
    {
        m_regCmd3.periodCongureRegister.bitfields.IRQ_SUBSAMPLE_PERIOD  = pInputData->pipelineIFEData.numBatchedFrames - 1;
        m_regCmd3.irqSubsamplePatternRegister.bitfields.PATTERN         = 1;
    }

    // Setup PDAF skip pattern
    m_regCmd2.lineSkipPatternRegister.bitfields.PATTERN  = 0xFFFFFFFF;
    m_regCmd2.pixelSkipPatternRegister.bitfields.PATTERN = 0xFFFFFFFF;

    // PDAF Type 3 extraction will happen through CSID Pixel extractor, So disbaling CAMIF Extraction
    m_regCmd1.configRegister.bitfields.PDAF_SUBSAMPLE_EN = 0x0;
    m_regCmd2.lineSkipPatternRegister.bitfields.PATTERN  = 0x0;
    m_regCmd2.pixelSkipPatternRegister.bitfields.PATTERN = 0x0;

    // Setup PDAF skip period. (based on the current PDAF implementation, never skip)
    m_regCmd3.periodCongureRegister.bitfields.LINE_SKIP_PERIOD  = 0;
    m_regCmd3.periodCongureRegister.bitfields.PIXEL_SKIP_PERIOD = 0;

    // Setup PDAF crop
    m_regCmd1.configRegister.bitfields.PDAF_RAW_CROP_EN = 0; /// TO DO: Need to have correct value populated
    if (TRUE == pInputData->HVXData.DSEnabled)
    {
        m_regCmd4.rawCropWidthRegister.bitfields.LAST_PIXEL  = pInputData->HVXData.origCAMIFCrop.lastPixel;
        m_regCmd4.rawCropWidthRegister.bitfields.FIRST_PIXEL = pInputData->HVXData.origCAMIFCrop.firstPixel;
        m_regCmd4.rawCropHeightRegister.bitfields.LAST_LINE  = pInputData->HVXData.origCAMIFCrop.lastLine;
        m_regCmd4.rawCropHeightRegister.bitfields.FIRST_LINE = pInputData->HVXData.origCAMIFCrop.firstLine;
    }
    else
    {
        m_regCmd4.rawCropWidthRegister.bitfields.LAST_PIXEL  =
            pInputData->pStripeConfig->CAMIFSubsampleInfo.PDAFCAMIFCrop.lastPixel;
        m_regCmd4.rawCropWidthRegister.bitfields.FIRST_PIXEL =
            pInputData->pStripeConfig->CAMIFSubsampleInfo.PDAFCAMIFCrop.firstPixel;
        m_regCmd4.rawCropHeightRegister.bitfields.LAST_LINE  =
            pInputData->pStripeConfig->CAMIFSubsampleInfo.PDAFCAMIFCrop.lastLine;
        m_regCmd4.rawCropHeightRegister.bitfields.FIRST_LINE =
            pInputData->pStripeConfig->CAMIFSubsampleInfo.PDAFCAMIFCrop.firstLine;
    }

    // Setup the IFE pixel raw output tap point
    if (pInputData->IFEPixelRawPort == IFEOutputPortCAMIFRaw)
    {
        m_pixelRawRegCmd.rawPixelPathConfig.bitfields.PIXEL_RAW_SRC_SEL = 0x0;
    }
    else if (pInputData->IFEPixelRawPort == IFEOutputPortLSCRaw)
    {
        m_pixelRawRegCmd.rawPixelPathConfig.bitfields.PIXEL_RAW_SRC_SEL = 0x1;
    }
    else if (pInputData->IFEPixelRawPort == IFEOutputPortGTMRaw)
    {
        m_pixelRawRegCmd.rawPixelPathConfig.bitfields.PIXEL_RAW_SRC_SEL = 0x2;
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupISP, " Invalid port for pixel Raw %d", pInputData->IFEPixelRawPort);
    }

    m_pixelRawRegCmd.rawPixelPathConfig.bitfields.PIXEL_RAW_FMT = 0x1;
    m_pixelRawRegCmd.rawPixelPathConfig.bitfields.ALPHA_VALUE   = 0x0;
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "IFE pixel raw path %d, TopConfig1 %x",
                     pInputData->IFEPixelRawPort,
                     m_pixelRawRegCmd.rawPixelPathConfig.bitfields);


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFPPTitan480::CreateSubCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIFPPTitan480::CreateSubCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if (NULL != pInputData &&
        NULL != pInputData->pCmdBuffer)
    {
        CmdBuffer* pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
            regIFE_IFE_0_PP_CLC_CAMIF_MODULE_CFG,
            sizeof(m_regCmd1.configRegister) / RegisterWidthInBytes,
            reinterpret_cast<UINT32*>(&m_regCmd1.configRegister));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFPPTitan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIFPPTitan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult  result          = CamxResultSuccess;
    BOOL*       pModuleEnable   = static_cast<BOOL*>(pInput);

    if (NULL != pInput)
    {
        m_regCmd1.configRegister.bitfields.EN = *pModuleEnable;
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFPPTitan480::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFECAMIFPPTitan480::CopyRegCmd(
    VOID* pData)
{
    CAMX_UNREFERENCED_PARAM(pData);

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "No need to implement");

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFPPTitan480::~IFECAMIFPPTitan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECAMIFPPTitan480::~IFECAMIFPPTitan480()
{
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Destroyed");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFECAMIFPPTitan480::IFECAMIFPPTitan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECAMIFPPTitan480::IFECAMIFPPTitan480(
    UINT32 instance)
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFECAMIFPPRegLengthDWord1) +
             PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFECAMIFPPRegLengthDWord2) +
             PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFECAMIFPPRegLengthDWord3) +
             PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFECAMIFPPRegLengthDWord4) +
             PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEPixelRawRegLengthDWord));
    Utils::Memset(&m_regCmd1, 0, sizeof(m_regCmd1));
    Utils::Memset(&m_regCmd2, 0, sizeof(m_regCmd2));
    Utils::Memset(&m_regCmd3, 0, sizeof(m_regCmd3));
    Utils::Memset(&m_regCmd4, 0, sizeof(m_regCmd4));

    m_instance = instance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFPPTitan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECAMIFPPTitan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Camif Module config:         0x%x", m_regCmd1.configRegister.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Camif Line skip pattern:     0x%x", m_regCmd2.lineSkipPatternRegister.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Camif Pixel skip pattern:    0x%x", m_regCmd2.pixelSkipPatternRegister.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Camif irq subsample pattern: 0x%x", m_regCmd3.irqSubsamplePatternRegister.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Camif subsample period:      0x%x", m_regCmd3.periodCongureRegister.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Camif PDAF Raw crop height:  0x%x", m_regCmd4.rawCropHeightRegister.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Camif PDAF Raw crop width:   0x%x", m_regCmd4.rawCropWidthRegister.u32All);
}

CAMX_NAMESPACE_END
