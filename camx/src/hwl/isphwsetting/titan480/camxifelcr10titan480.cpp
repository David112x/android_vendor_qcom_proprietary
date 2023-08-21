////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifelcr10titan480.cpp
/// @brief CAMXIFELCR10TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifelcr10titan480.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELCR10Titan480::IFELCR10Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFELCR10Titan480::IFELCR10Titan480()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFE_IFE_0_CLC_LCRMODULE_CFG) / sizeof(UINT32)) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFELCR10Config) / sizeof(UINT32)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELCR10Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELCR10Titan480::CreateCmdList(
    VOID* pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result            = CamxResultSuccess;
    ISPInputData* pInputData        = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer        = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if (NULL != pInputData              &&
        NULL != pInputData->pCmdBuffer)
    {
        pCmdBuffer = pInputData->pCmdBuffer;
        result     = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_CLC_LCRMODULE_CFG,
                                                  sizeof(m_regCmd.moduleConfig) / sizeof(UINT32),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.moduleConfig));
        result     = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_CLC_LCRCROP_LINE_CFG,
                                                  sizeof(IFELCR10Config) / sizeof(UINT32),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.lcrConfig));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to write command buffer. result = %d", result);
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
// IFELCR10Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELCR10Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result      = CamxResultSuccess;

    ISPInputData*       pInputData   = static_cast<ISPInputData*>(pInput);
    IFELCR10OutputData* pOutputData = static_cast<IFELCR10OutputData*>(pOutput);

    if ((NULL != pInputData) && (NULL != pInputData->pPDHwConfig) && (NULL != pOutputData))
    {
        PDHwLCRConfig* pLCRConfig = &pInputData->pPDHwConfig->LCRConfig;

        UINT32 firstPixel         = pLCRConfig->crop.firstPixel;
        UINT32 lastPixel          = pLCRConfig->crop.lastPixel;

        if (TRUE == pInputData->pStripeConfig->overwriteStripes)
        {
            firstPixel = pInputData->pStripeConfig->pStripeOutput->lcrOutput.firstPixel;
            lastPixel  = pInputData->pStripeConfig->pStripeOutput->lcrOutput.lastPixel;
        }

        UINT64 combinedFlushMask  = 0;
        UINT32 cropWidth          = lastPixel - firstPixel + 1;
        UINT32 cropHeight         = pLCRConfig->crop.lastLine - pLCRConfig->crop.firstLine + 1;
        UINT64 validMask          = 0;
        UINT32 maxblockHeight     = MaxLCRBlockHeight; // Max Block height is 64 - 1
        UINT32 flushLinesPerBlock = 0;
        UINT32 numberOfBlocks     = 0;

        m_regCmd.moduleConfig.bitfields.EN                       = pLCRConfig->enable;
        m_regCmd.lcrConfig.cropLineConfig.bitfields.FIRST_LINE   = pLCRConfig->crop.firstLine;
        m_regCmd.lcrConfig.cropLineConfig.bitfields.LAST_LINE    = pLCRConfig->crop.lastLine;
        m_regCmd.lcrConfig.cropPixelConfig.bitfields.FIRST_PIXEL = firstPixel;
        m_regCmd.lcrConfig.cropPixelConfig.bitfields.LAST_PIXEL  = lastPixel;
        m_regCmd.lcrConfig.cfg0.bitfields.BLOCK_HEIGHT           = pLCRConfig->blockHeight;
        m_regCmd.lcrConfig.cfg0.bitfields.COMP_MASK              = pLCRConfig->componentMask;
        m_regCmd.lcrConfig.cfg1.bitfields.COMP_SHIFT_0           = pLCRConfig->componentShift[0];
        m_regCmd.lcrConfig.cfg1.bitfields.COMP_SHIFT_1           = pLCRConfig->componentShift[1];
        m_regCmd.lcrConfig.cfg1.bitfields.COMP_SHIFT_2           = pLCRConfig->componentShift[2];
        m_regCmd.lcrConfig.cfg1.bitfields.COMP_SHIFT_3           = pLCRConfig->componentShift[3];
        m_regCmd.lcrConfig.cfg2.bitfields.LINE_MASK_0            = pLCRConfig->lineMask[0];
        m_regCmd.lcrConfig.cfg3.bitfields.LINE_MASK_1            = pLCRConfig->lineMask[1];
        m_regCmd.lcrConfig.cfg4.bitfields.FLUSH_MASK_0           = pLCRConfig->flushMask[0];
        m_regCmd.lcrConfig.cfg5.bitfields.FLUSH_MASK_1           = pLCRConfig->flushMask[1];

        combinedFlushMask  = pLCRConfig->flushMask[1];
        combinedFlushMask  = (combinedFlushMask << 32) | pLCRConfig->flushMask[0];

        // Crop Height needs to be multiple of block Height. Its a hardware restriction
        if (0 != (cropHeight % (pLCRConfig->blockHeight + 1)))
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid Block Height %d and crop Height %d",
                           pLCRConfig->blockHeight, cropHeight);
            result = CamxResultEFailed;
        }

        // Flush mask detemines the number of output line per a give block height. The total
        // height of the buffer would be number of flush line per block x number of blocks
        if (CamxResultSuccess == result)
        {
            validMask = (pLCRConfig->blockHeight == maxblockHeight ? -1L : (1L <<  pLCRConfig->blockHeight) - 1L);
            flushLinesPerBlock = GetNumberOfSetBitsInMask(combinedFlushMask & validMask);
            numberOfBlocks     = cropHeight / (pLCRConfig->blockHeight + 1);

            pOutputData->width  = cropWidth;
            pOutputData->height = numberOfBlocks * flushLinesPerBlock;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELCR10Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELCR10Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELCR10Titan480::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFELCR10Titan480::CopyRegCmd(
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
// IFELCR10Titan480::~IFELCR10Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFELCR10Titan480::~IFELCR10Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELCR10Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFELCR10Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Lcr Module Cfg             [0x%x]", m_regCmd.moduleConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LCR Crop Line Config       [0x%x]", m_regCmd.lcrConfig.cropLineConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LCR Crop pixel Config      [0x%x]", m_regCmd.lcrConfig.cropPixelConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Cfg0                       [0x%x]", m_regCmd.lcrConfig.cfg0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Cfg1                       [0x%x]", m_regCmd.lcrConfig.cfg1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Cfg2                       [0x%x]", m_regCmd.lcrConfig.cfg2.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Cfg3                       [0x%x]", m_regCmd.lcrConfig.cfg3.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Cfg4                       [0x%x]", m_regCmd.lcrConfig.cfg4.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Cfg5                       [0x%x]", m_regCmd.lcrConfig.cfg5.u32All);
}

CAMX_NAMESPACE_END
