////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifepdaf20titan480.cpp
/// @brief CAMXIFEPDAF20TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifepdaf20titan480.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDAF20Titan480::IFEPDAF20Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEPDAF20Titan480::IFEPDAF20Titan480()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(m_regCmd.DMILUTBankConfig))                      +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(m_regCmd.moduleLUTBankConfig))                   +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(m_regCmd.moduleConfig))                          +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(m_regCmd.sparsePDHWConfig.pixelExtractor))       +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(m_regCmd.sparsePDHWConfig.horizontalCrop))       +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(m_regCmd.sparsePDHWConfig.verticalCrop))         +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(m_regCmd.sparsePDHWConfig.lineExtractorConfig))  +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(m_regCmd.sparsePDHWConfig.pixelSeparatorConfig)) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(m_regCmd.sparsePDHWConfig.resamplerConfig))      +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(m_regCmd.sparsePDHWConfig.firConfig))            +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(m_regCmd.dualPDConfig.cfg1))                     +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(m_regCmd.dualPDConfig.cfg2))                     +
                     PacketBuilder::RequiredWriteDMISizeInDwords() * 5);
    Set32bitDMILength(IFEPDAF20DMILutSizeDword);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDAF20Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDAF20Titan480::CreateCmdList(
    VOID* pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result            = CamxResultSuccess;
    ISPInputData* pInputData        = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer        = NULL;
    CmdBuffer*    pDMIBuffer        = NULL;
    UINT32        offset            = *pDMIBufferOffset * sizeof(UINT32);


    if (NULL != pInputData                  &&
        NULL != pInputData->pCmdBuffer      &&
        NULL != pInputData->p32bitDMIBuffer)
    {
        pCmdBuffer = pInputData->pCmdBuffer;
        pDMIBuffer = pInputData->p32bitDMIBuffer;
        result     = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_CLC_PDLIB_DMI_LUT_BANK_CFG,
                                                  sizeof(m_regCmd.DMILUTBankConfig) / sizeof(UINT32),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.DMILUTBankConfig));
        if (CamxResultSuccess == result)
        {
            result     = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                      regIFE_IFE_0_CLC_PDLIB_MODULE_LUT_BANK_CFG,
                                                      sizeof(m_regCmd.moduleLUTBankConfig) / sizeof(UINT32),
                                                      reinterpret_cast<UINT32*>(&m_regCmd.moduleLUTBankConfig));
        }

        if (CamxResultSuccess == result)
        {
            result     = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                      regIFE_IFE_0_CLC_PDLIB_MODULE_CFG,
                                                      sizeof(m_regCmd.moduleConfig) / sizeof(UINT32),
                                                      reinterpret_cast<UINT32*>(&m_regCmd.moduleConfig));
        }

        if ((CamxResultSuccess == result)      &&
            (TRUE == pInputData->isInitPacket) &&
            (0 != m_regCmd.sparsePDHWConfig.pixelExtractor.bitfields.PE_ENABLE))
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_CLC_CSID_PPP_SPARSE_PD_PIX_EXTRACT_CFG0,
                                                  sizeof(m_regCmd.sparsePDHWConfig.pixelExtractor) / sizeof(UINT32),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.sparsePDHWConfig.pixelExtractor));
            if (CamxResultSuccess == result)
            {
                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                      regIFE_IFE_0_CLC_CSID_PPP_HCROP,
                                                      sizeof(m_regCmd.sparsePDHWConfig.horizontalCrop) / sizeof(UINT32),
                                                      reinterpret_cast<UINT32*>(&m_regCmd.sparsePDHWConfig.horizontalCrop));
            }

            if (CamxResultSuccess == result)
            {
                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                      regIFE_IFE_0_CLC_CSID_PPP_VCROP,
                                                      sizeof(m_regCmd.sparsePDHWConfig.verticalCrop) / sizeof(UINT32),
                                                      reinterpret_cast<UINT32*>(&m_regCmd.sparsePDHWConfig.verticalCrop));
            }
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG1,
                                                  sizeof(IFEPDAF20LineExtractorConfig) / sizeof(UINT32),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.sparsePDHWConfig.lineExtractorConfig));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_CLC_PDLIB_SPARSE_PD_PS_CFG0,
                                                  sizeof(m_regCmd.sparsePDHWConfig.pixelSeparatorConfig) / sizeof(UINT32),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.sparsePDHWConfig.pixelSeparatorConfig));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_CLC_PDLIB_SPARSE_PD_RESAMPLER,
                                                  sizeof(m_regCmd.sparsePDHWConfig.resamplerConfig) / sizeof(UINT32),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.sparsePDHWConfig.resamplerConfig));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_CLC_PDLIB_SPARSE_PD_FIR_CFG0,
                                                  sizeof(IFEPDAF20SparsePDFIRConfig) / sizeof(UINT32),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.sparsePDHWConfig.firConfig));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_CLC_PDLIB_CROP_WIDTH_CFG,
                                                  sizeof(IFEPDAF20DualPDConfigCmd1) / sizeof(UINT32),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.dualPDConfig.cfg1));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_CLC_PDLIB_GM_CFG_2,
                                                  sizeof(IFEPDAF20DualPDConfigCmd2) / sizeof(UINT32),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.dualPDConfig.cfg2));
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to write command buffer. result = %d", result);
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteDMI(pCmdBuffer,
                                             regIFE_IFE_0_CLC_PDLIB_DMI_CFG,
                                             IFEPDAF20GainMapLUTSelect,
                                             pDMIBuffer,
                                             offset + IFEPDAF20GainMapDMIOffset,
                                             IFEPDAF20GainMapDMILengthWord);
        }

        if (CamxResultSuccess == result)
        {
            if (TRUE == pInputData->isInitPacket)
            {
                result     = PacketBuilder::WriteDMI(pCmdBuffer,
                                                    regIFE_IFE_0_CLC_PDLIB_DMI_CFG,
                                                    IFEPDAF20PixelSeparatorLUTSelect,
                                                    pDMIBuffer,
                                                    offset + IFEPDAF20PixelSeparatorMapDMIOffset,
                                                    IFEPDAF20PixelSepartorMapDMILengthWord);
            }
        }

        if (CamxResultSuccess == result)
        {
            result     = PacketBuilder::WriteDMI(pCmdBuffer,
                                                 regIFE_IFE_0_CLC_PDLIB_DMI_CFG,
                                                 IFEPDAF20ResamplerPixelLUTSelect,
                                                 pDMIBuffer,
                                                 offset + IFEPDAF20ResamplerPixelMapDMIOffset,
                                                 IFEPDAF20ResamplerPixelMapDMILengthWord);
        }

        if (CamxResultSuccess == result)
        {
            result     = PacketBuilder::WriteDMI(pCmdBuffer,
                                                 regIFE_IFE_0_CLC_PDLIB_DMI_CFG,
                                                 IFEPDAF20ResamplerCoeffLUTSelect,
                                                 pDMIBuffer,
                                                 offset + IFEPDAF20ResamplerCoefficientDMIOffset,
                                                 IFEPDAF20ResamplerCoefficientDMILengthWord);
        }

        if ((CamxResultSuccess == result) && (TRUE == pInputData->isInitPacket))
        {
            // CSID DMI is only single buffered. So, there is no need to specify the bank
            result  = PacketBuilder::WriteDMI(pCmdBuffer,
                                              regIFE_IFE_0_CLC_CSID_DMI_CFG,
                                              IFEPDAF20CSIDPixelExtractorDMISelect,
                                              pDMIBuffer,
                                              offset + IFEPDAF20CSIDPixelExtracionMapDMIOffset,
                                              IFEPDAF20CSIDPixelExtracionMapDMILengthWord);
        }

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
// IFEPDAF20Titan480::ConfigureSparsePD
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDAF20Titan480::ConfigureSparsePD(
    PDHwConfig* pHWConfig,
    VOID* pOutput)
{
    CamxResult                     result                 = CamxResultSuccess;
    PDHwCSIDPixelExtractorConfig*  pPixelExtractorConfig  = NULL;
    PDHwFIRConfig*                 pFIRConfig             = NULL;
    PDHwResamplerConfig*           pResamplerConfig       = NULL;
    PDHwPixelSeparatorConfig*      pPixelSeparatorConfig  = NULL;
    PDHwLineExtractorConfig*       pHWLineExtractorConfig = NULL;
    PDAF20OutputData*              pOutputData            = static_cast<PDAF20OutputData*>(pOutput);
    UINT8*                         pDMIPtr                = reinterpret_cast<UINT8*>(pOutputData->pDMIDataPtr);

    // Configure CSID Pixel Extractor
    pPixelExtractorConfig = &pHWConfig->sparseConfig.PEConfig;
    m_regCmd.sparsePDHWConfig.pixelExtractor.bitfields.PE_ENABLE       = pPixelExtractorConfig->enable;
    m_regCmd.sparsePDHWConfig.pixelExtractor.bitfields.PE_BLOCK_WIDTH  = pPixelExtractorConfig->blockWidth - 1;
    m_regCmd.sparsePDHWConfig.pixelExtractor.bitfields.PE_BLOCK_HEIGHT = pPixelExtractorConfig->blockHeight - 1;
    m_regCmd.sparsePDHWConfig.horizontalCrop.bitfields.START_PIXEL     = pPixelExtractorConfig->cropConfig.firstPixelCrop;
    m_regCmd.sparsePDHWConfig.horizontalCrop.bitfields.END_PIXEL       = pPixelExtractorConfig->cropConfig.lastPixelCrop;
    m_regCmd.sparsePDHWConfig.verticalCrop.bitfields.START_LINE        = pPixelExtractorConfig->cropConfig.firstLineCrop;
    m_regCmd.sparsePDHWConfig.verticalCrop.bitfields.END_LINE          = pPixelExtractorConfig->cropConfig.lastLineCrop;

    UINT64* pPixelExtractionDMIPtr = reinterpret_cast<UINT64*>(pDMIPtr + IFEPDAF20CSIDPixelExtracionMapDMIOffset);
    UINT32 index                   = 0;
    UINT32 skipIndex               = 0;
    BOOL   skipValue               = TRUE;

    // Pack 4096 values into LUT of length 64, each of 64 bit wide
    // CSID DMI has an issue where the alternate 4 indexs are skipped if we write LUT data through CDM
    // To avoid this, pack the 64 LUT data by skipping alternate 4 indices, like ....,4,5,6,7,....,12,13,14,15 etc
    for (UINT entryIndex = 0; entryIndex < (IFEPDAF20CSIDPixelExtractorRows * IFEPDAF20CSIDPixelExtractorColomns) * 2;
                              entryIndex += IFEPDAF20CSIDPixelExtractorRows)
    {
        if (IFEPDAF20CSIDSkipDMIValueOffset == skipIndex)
        {
            skipValue = (skipValue == FALSE) ? TRUE : FALSE;
            skipIndex = 0;
        }
        if (FALSE == skipValue)
        {
            for (UINT packIndex = 0; packIndex < IFEPDAF20CSIDPixelExtractorRows; packIndex++)
            {
                *pPixelExtractionDMIPtr |= ((pPixelExtractorConfig->pdPixelMap[index] << packIndex));
                index++;
            }
        }
        pPixelExtractionDMIPtr++;
        skipIndex++;
    }

    // Configure Resampler Config
    pResamplerConfig                                               = &pHWConfig->sparseConfig.RSConfig;
    m_regCmd.moduleConfig.bitfields.SPARSE_PD_RESAMPLER_EN         = pResamplerConfig->enable;
    m_regCmd.sparsePDHWConfig.resamplerConfig.bitfields.IN_HEIGHT  = pResamplerConfig->inputHeight;
    m_regCmd.sparsePDHWConfig.resamplerConfig.bitfields.IN_WIDTH   = pResamplerConfig->inputWidth;
    m_regCmd.sparsePDHWConfig.resamplerConfig.bitfields.OUT_WIDTH  = pResamplerConfig->outputWidth;
    m_regCmd.sparsePDHWConfig.resamplerConfig.bitfields.OUT_HEIGHT = pResamplerConfig->outputHeight;

    IFEPDAF20SparsePDResamplerPixelLUT* pPixelLUT =
        reinterpret_cast<IFEPDAF20SparsePDResamplerPixelLUT*>(pDMIPtr + IFEPDAF20ResamplerPixelMapDMIOffset);
    for (UINT index = 0; index < IFEPDAF20ResamplerPixelLutEntries; index++)
    {
        (*pPixelLUT).pixel0X = pResamplerConfig->instruction[index].pixelLUT[0].x;
        (*pPixelLUT).pixel0Y = pResamplerConfig->instruction[index].pixelLUT[0].y;
        (*pPixelLUT).pixel1X = pResamplerConfig->instruction[index].pixelLUT[1].x;
        (*pPixelLUT).pixel1Y = pResamplerConfig->instruction[index].pixelLUT[1].y;
        (*pPixelLUT).pixel2X = pResamplerConfig->instruction[index].pixelLUT[2].x;
        (*pPixelLUT).pixel2Y = pResamplerConfig->instruction[index].pixelLUT[2].y;
        (*pPixelLUT).pixel3X = pResamplerConfig->instruction[index].pixelLUT[3].x;
        (*pPixelLUT).pixel3Y = pResamplerConfig->instruction[index].pixelLUT[3].y;
        pPixelLUT++;
    }

    IFEPDAF20SparsePDResamplerCoefficientLUT* pPixelCoeffLUT =
        reinterpret_cast<IFEPDAF20SparsePDResamplerCoefficientLUT*>(pDMIPtr + IFEPDAF20ResamplerCoefficientDMIOffset);

    for (UINT index = 0; index < IFEPDAF20ResamplerCoefficientLutEntries; index++)
    {
        (*pPixelCoeffLUT).OP0K0 = FLOAT_TO_Q(3, pResamplerConfig->instruction[index].coeffLUT[0].k0);
        (*pPixelCoeffLUT).OP0K1 = FLOAT_TO_Q(3, pResamplerConfig->instruction[index].coeffLUT[0].k1);
        (*pPixelCoeffLUT).OP0K2 = FLOAT_TO_Q(3, pResamplerConfig->instruction[index].coeffLUT[0].k2);
        (*pPixelCoeffLUT).OP0K3 = FLOAT_TO_Q(3, pResamplerConfig->instruction[index].coeffLUT[0].k3);
        (*pPixelCoeffLUT).OP1K0 = FLOAT_TO_Q(3, pResamplerConfig->instruction[index].coeffLUT[1].k0);
        (*pPixelCoeffLUT).OP1K1 = FLOAT_TO_Q(3, pResamplerConfig->instruction[index].coeffLUT[1].k1);
        (*pPixelCoeffLUT).OP1K2 = FLOAT_TO_Q(3, pResamplerConfig->instruction[index].coeffLUT[1].k2);
        (*pPixelCoeffLUT).OP1K3 = FLOAT_TO_Q(3, pResamplerConfig->instruction[index].coeffLUT[1].k3);
        (*pPixelCoeffLUT).OP2K0 = FLOAT_TO_Q(3, pResamplerConfig->instruction[index].coeffLUT[2].k0);
        (*pPixelCoeffLUT).OP2K1 = FLOAT_TO_Q(3, pResamplerConfig->instruction[index].coeffLUT[2].k1);
        (*pPixelCoeffLUT).OP2K2 = FLOAT_TO_Q(3, pResamplerConfig->instruction[index].coeffLUT[2].k2);
        (*pPixelCoeffLUT).OP2K3 = FLOAT_TO_Q(3, pResamplerConfig->instruction[index].coeffLUT[2].k3);
        (*pPixelCoeffLUT).OP3K0 = FLOAT_TO_Q(3, pResamplerConfig->instruction[index].coeffLUT[3].k0);
        (*pPixelCoeffLUT).OP3K1 = FLOAT_TO_Q(3, pResamplerConfig->instruction[index].coeffLUT[3].k1);
        (*pPixelCoeffLUT).OP3K2 = FLOAT_TO_Q(3, pResamplerConfig->instruction[index].coeffLUT[3].k2);
        (*pPixelCoeffLUT).OP3K3 = FLOAT_TO_Q(3, pResamplerConfig->instruction[index].coeffLUT[3].k3);
        pPixelCoeffLUT++;
    }

    // Configure Pixel Separator Config
    pPixelSeparatorConfig                                                            = &pHWConfig->sparseConfig.PSConfig;
    m_regCmd.sparsePDHWConfig.pixelSeparatorConfig.bitfields.MAX_PD_PIXELS_PER_BLOCK = pPixelSeparatorConfig->pixelsPerBlock;
    m_regCmd.sparsePDHWConfig.pixelSeparatorConfig.bitfields.ROWS_PER_BLOCK          = pPixelSeparatorConfig->rowPerBlock;
    m_regCmd.sparsePDHWConfig.pixelSeparatorConfig.bitfields.OUT_HEIGHT              = pPixelSeparatorConfig->outputHeight;
    m_regCmd.sparsePDHWConfig.pixelSeparatorConfig.bitfields.OUT_WIDTH               = pPixelSeparatorConfig->outputWidth;

    IFEPDAF20PixelSeparatorMapLUT* pPixelSeparatorLUT =
        reinterpret_cast<IFEPDAF20PixelSeparatorMapLUT*>(pDMIPtr + IFEPDAF20PixelSeparatorMapDMIOffset);

    for (UINT index = 0; index < IFEPDAF20PixelSeparatorMapTableEntries; index++)
    {
        (*pPixelSeparatorLUT).rightPixelIndex = pPixelSeparatorConfig->pdRMap[index];
        (*pPixelSeparatorLUT).leftPixelIndex  = pPixelSeparatorConfig->pdLMap[index];
        pPixelSeparatorLUT++;
    }

    // Configure Line Extractor Config
    pHWLineExtractorConfig                                                       = &pHWConfig->sparseConfig.LEConfig;
    m_regCmd.moduleConfig.bitfields.SPARSE_PD_LE_EN                              = 1;
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg1.bitfields.GLOBAL_OFFSET   = pHWLineExtractorConfig->globalOffsetLines;
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg2.bitfields.LINES_PER_BLOCK = pHWLineExtractorConfig->blockPDRowCount;
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg2.bitfields.T2H             = pHWLineExtractorConfig->blockHeight;
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg3.bitfields.LINE_LENGTH     = pHWLineExtractorConfig->pdPixelWidth;
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg3.bitfields.NO_OF_BLOCKS    = pHWLineExtractorConfig->verticalBlockCount;
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg4.bitfields.X_OFFSET0       = pHWLineExtractorConfig->horizontalOffset[0];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg4.bitfields.Y_OFFSET0       = pHWLineExtractorConfig->verticalOffset[0];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg5.bitfields.X_OFFSET1       = pHWLineExtractorConfig->horizontalOffset[1];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg5.bitfields.Y_OFFSET1       = pHWLineExtractorConfig->verticalOffset[1];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg6.bitfields.X_OFFSET2       = pHWLineExtractorConfig->horizontalOffset[2];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg6.bitfields.Y_OFFSET2       = pHWLineExtractorConfig->verticalOffset[2];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg7.bitfields.X_OFFSET3       = pHWLineExtractorConfig->horizontalOffset[3];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg7.bitfields.Y_OFFSET3       = pHWLineExtractorConfig->verticalOffset[3];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg8.bitfields.X_OFFSET4       = pHWLineExtractorConfig->horizontalOffset[4];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg8.bitfields.Y_OFFSET4       = pHWLineExtractorConfig->verticalOffset[4];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg9.bitfields.X_OFFSET5       = pHWLineExtractorConfig->horizontalOffset[5];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg9.bitfields.Y_OFFSET5       = pHWLineExtractorConfig->verticalOffset[5];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg10.bitfields.X_OFFSET6      = pHWLineExtractorConfig->horizontalOffset[6];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg10.bitfields.Y_OFFSET6      = pHWLineExtractorConfig->verticalOffset[6];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg11.bitfields.X_OFFSET7      = pHWLineExtractorConfig->horizontalOffset[7];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg11.bitfields.Y_OFFSET7      = pHWLineExtractorConfig->verticalOffset[7];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg12.bitfields.X_OFFSET8      = pHWLineExtractorConfig->horizontalOffset[8];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg12.bitfields.Y_OFFSET8      = pHWLineExtractorConfig->verticalOffset[8];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg13.bitfields.X_OFFSET9      = pHWLineExtractorConfig->horizontalOffset[9];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg13.bitfields.Y_OFFSET9      = pHWLineExtractorConfig->verticalOffset[9];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg14.bitfields.X_OFFSET10     = pHWLineExtractorConfig->horizontalOffset[10];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg14.bitfields.Y_OFFSET10     = pHWLineExtractorConfig->verticalOffset[10];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg15.bitfields.X_OFFSET11     = pHWLineExtractorConfig->horizontalOffset[11];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg15.bitfields.Y_OFFSET11     = pHWLineExtractorConfig->verticalOffset[11];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg16.bitfields.X_OFFSET12     = pHWLineExtractorConfig->horizontalOffset[12];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg16.bitfields.Y_OFFSET12     = pHWLineExtractorConfig->verticalOffset[12];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg17.bitfields.X_OFFSET13     = pHWLineExtractorConfig->horizontalOffset[13];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg17.bitfields.Y_OFFSET13     = pHWLineExtractorConfig->verticalOffset[13];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg18.bitfields.X_OFFSET14     = pHWLineExtractorConfig->horizontalOffset[14];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg18.bitfields.Y_OFFSET14     = pHWLineExtractorConfig->verticalOffset[14];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg19.bitfields.X_OFFSET15     = pHWLineExtractorConfig->horizontalOffset[15];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg19.bitfields.Y_OFFSET15     = pHWLineExtractorConfig->verticalOffset[15];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg20.bitfields.HALF_LINE0     = pHWLineExtractorConfig->halfLine[0];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg20.bitfields.HALF_LINE1     = pHWLineExtractorConfig->halfLine[1];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg20.bitfields.HALF_LINE2     = pHWLineExtractorConfig->halfLine[2];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg20.bitfields.HALF_LINE3     = pHWLineExtractorConfig->halfLine[3];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg20.bitfields.HALF_LINE4     = pHWLineExtractorConfig->halfLine[4];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg20.bitfields.HALF_LINE5     = pHWLineExtractorConfig->halfLine[5];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg20.bitfields.HALF_LINE6     = pHWLineExtractorConfig->halfLine[6];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg20.bitfields.HALF_LINE7     = pHWLineExtractorConfig->halfLine[7];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg20.bitfields.HALF_LINE8     = pHWLineExtractorConfig->halfLine[8];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg20.bitfields.HALF_LINE9     = pHWLineExtractorConfig->halfLine[9];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg20.bitfields.HALF_LINE10    = pHWLineExtractorConfig->halfLine[10];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg20.bitfields.HALF_LINE11    = pHWLineExtractorConfig->halfLine[11];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg20.bitfields.HALF_LINE12    = pHWLineExtractorConfig->halfLine[12];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg20.bitfields.HALF_LINE13    = pHWLineExtractorConfig->halfLine[13];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg20.bitfields.HALF_LINE14    = pHWLineExtractorConfig->halfLine[14];
    m_regCmd.sparsePDHWConfig.lineExtractorConfig.cfg20.bitfields.HALF_LINE15    = pHWLineExtractorConfig->halfLine[15];

    // FIR Config
    pFIRConfig                                                  = &pHWConfig->sparseConfig.FIRConfig;
    m_regCmd.moduleConfig.bitfields.FIR_EN                      = pFIRConfig->enable;
    m_regCmd.sparsePDHWConfig.firConfig.cfg0.bitfields.COEFF_A0 = FLOAT_TO_Q(11, pFIRConfig->a0);
    m_regCmd.sparsePDHWConfig.firConfig.cfg0.bitfields.COEFF_A1 = FLOAT_TO_Q(11, pFIRConfig->a1);
    m_regCmd.sparsePDHWConfig.firConfig.cfg1.bitfields.COEFF_A2 = FLOAT_TO_Q(11, pFIRConfig->a2);
    m_regCmd.sparsePDHWConfig.firConfig.cfg1.bitfields.COEFF_A3 = FLOAT_TO_Q(11, pFIRConfig->a3);
    m_regCmd.sparsePDHWConfig.firConfig.cfg2.bitfields.COEFF_A4 = FLOAT_TO_Q(11, pFIRConfig->a4);
    m_regCmd.sparsePDHWConfig.firConfig.cfg2.bitfields.SHIFT    = pFIRConfig->shift;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDAF20Titan480::ConfigureDualPD
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDAF20Titan480::ConfigureDualPD(
    PDHwConfig* pHWConfig,
    VOID* pOutput)
{
    CamxResult          result      = CamxResultSuccess;
    PDAF20OutputData*   pOutputData = static_cast<PDAF20OutputData*>(pOutput);
    UINT32*             pDMIPtr     = reinterpret_cast<UINT32*>(pOutputData->pDMIDataPtr);

    m_regCmd.dualPDConfig.cfg1.pdCropWidthCfg.bitfields.LAST_PIXEL      = pHWConfig->cropConfig.lastPixelCrop;
    m_regCmd.dualPDConfig.cfg1.pdCropWidthCfg.bitfields.FIRST_PIXEL     = pHWConfig->cropConfig.firstPixelCrop;
    m_regCmd.dualPDConfig.cfg1.pdCropHeightCfg.bitfields.LAST_LINE      = pHWConfig->cropConfig.lastLineCrop;
    m_regCmd.dualPDConfig.cfg1.pdCropHeightCfg.bitfields.FIRST_LINE     = pHWConfig->cropConfig.firstLineCrop;
    m_regCmd.dualPDConfig.cfg1.pdBLSCfg.bitfields.LEFT_PIXEL_OFFSET     = pHWConfig->BLSConfig.leftPixelBLSCorrection;
    m_regCmd.dualPDConfig.cfg1.pdBLSCfg.bitfields.RIGHT_PIXEL_OFFSET    = pHWConfig->BLSConfig.rightPixelBLSCorrection;
    m_regCmd.dualPDConfig.cfg1.pdRegionOffsetCfg.bitfields.RGN_H_OFFSET = pHWConfig->SADConfig.horizontalOffset;
    m_regCmd.dualPDConfig.cfg1.pdRegionOffsetCfg.bitfields.RGN_V_OFFSET = pHWConfig->SADConfig.verticalOffset;
    m_regCmd.dualPDConfig.cfg1.pdRegionNumberCfg.bitfields.RGN_H_NUM    = pHWConfig->SADConfig.horizontalNumber;
    m_regCmd.dualPDConfig.cfg1.pdRegionNumberCfg.bitfields.RGN_V_NUM    = pHWConfig->SADConfig.verticalNumber;
    m_regCmd.dualPDConfig.cfg1.pdRegionSizeCfg.bitfields.RGN_HEIGHT     = pHWConfig->SADConfig.regionHeight;
    m_regCmd.dualPDConfig.cfg1.pdRegionSizeCfg.bitfields.RGN_WIDTH      = pHWConfig->SADConfig.regionWidth;
    m_regCmd.dualPDConfig.cfg1.pdHDRCfg.bitfields.FIRST_PIXEL_LG        = pHWConfig->HDRConfig.hdrFirstPixel;
    m_regCmd.dualPDConfig.cfg1.pdHDRCfg.bitfields.MODE_SEL              = pHWConfig->HDRConfig.hdrModeSel;
    m_regCmd.dualPDConfig.cfg1.pdHDRCfg.bitfields.LONG_TH               = FLOAT_TO_Q(14, pHWConfig->HDRConfig.hdrThreshhold);
    m_regCmd.dualPDConfig.cfg1.pdHDRCfg.bitfields.EXP_RATIO             =
        FLOAT_TO_Q(14, pHWConfig->HDRConfig.hdrExposureRatio) - 1;
    m_regCmd.dualPDConfig.cfg1.pdBinSkipCfg.bitfields.H_SHIFT           = pHWConfig->binConfig.horizontalBinningSkip;
    m_regCmd.dualPDConfig.cfg1.pdBinSkipCfg.bitfields.H_BIN_PIX_NUM     = pHWConfig->binConfig.horizontalBinningPixelCount;
    m_regCmd.dualPDConfig.cfg1.pdBinSkipCfg.bitfields.V_BIN_LN_NUM      = pHWConfig->binConfig.verticalBinningLineCount;
    m_regCmd.dualPDConfig.cfg1.pdBinSkipCfg.bitfields.V_DEC_LN_NUM      = pHWConfig->binConfig.verticalDecimateCount;
    m_regCmd.dualPDConfig.cfg1.pdBinSkipCfg.bitfields.V_SHIFT           = pHWConfig->binConfig.verticalBinningSkip;
    m_regCmd.dualPDConfig.cfg1.pdGMCfg0.bitfields.V_PHASE_INIT          = pHWConfig->gainMapConfig.verticalPhaseInit;
    m_regCmd.dualPDConfig.cfg1.pdGMCfg1.bitfields.H_PHASE_INIT          = pHWConfig->gainMapConfig.horizontalPhaseInit;
    m_regCmd.dualPDConfig.cfg2.pdGMCfg2.bitfields.V_NUM                 = pHWConfig->gainMapConfig.numberVerticalGrids;
    m_regCmd.dualPDConfig.cfg2.pdGMCfg2.bitfields.V_PHASE_STEP          = pHWConfig->gainMapConfig.verticalPhaseStep;
    m_regCmd.dualPDConfig.cfg2.pdGMCfg3.bitfields.H_NUM                 = pHWConfig->gainMapConfig.numberHorizontalGrids;
    m_regCmd.dualPDConfig.cfg2.pdGMCfg3.bitfields.H_PHASE_STEP          = pHWConfig->gainMapConfig.horizontalPhaseStep;
    m_regCmd.dualPDConfig.cfg2.pdIIRCfg0.bitfields.B0                   = FLOAT_TO_Q(13, pHWConfig->IIRConfig.b0);
    m_regCmd.dualPDConfig.cfg2.pdIIRCfg0.bitfields.B1                   = FLOAT_TO_Q(13, pHWConfig->IIRConfig.b1);
    m_regCmd.dualPDConfig.cfg2.pdIIRCfg1.bitfields.B2                   = FLOAT_TO_Q(13, pHWConfig->IIRConfig.b2);
    m_regCmd.dualPDConfig.cfg2.pdIIRCfg2.bitfields.A0                   = FLOAT_TO_Q(13, pHWConfig->IIRConfig.a0);
    m_regCmd.dualPDConfig.cfg2.pdIIRCfg2.bitfields.A1                   = FLOAT_TO_Q(13, pHWConfig->IIRConfig.a1);
    m_regCmd.dualPDConfig.cfg2.pdIIRCfg3.bitfields.INIT_0               = FLOAT_TO_Q(13, pHWConfig->IIRConfig.init0);
    m_regCmd.dualPDConfig.cfg2.pdIIRCfg3.bitfields.INIT_1               = FLOAT_TO_Q(13, pHWConfig->IIRConfig.init1);
    m_regCmd.dualPDConfig.cfg2.pdSADPhaseCfg0.bitfields.PHASE_MASK      = pHWConfig->SADConfig.config0Phase;
    m_regCmd.dualPDConfig.cfg2.pdSADPhaseCfg1.bitfields.PHASE_MASK      = pHWConfig->SADConfig.config1Phase;

    for (UINT32 i = 0; i < IFEPDAF20GainMapLutTableSize; i++)
    {
        pDMIPtr[i] = pHWConfig->gainMapConfig.leftLUT[i] << 16 |
            pHWConfig->gainMapConfig.rightLUT[i];
    }

    // Note: width is calculated using a factor of two to consider left and right pixels
    pOutputData->width  = (pHWConfig->cropConfig.lastPixelCrop - pHWConfig->cropConfig.firstPixelCrop + 1) * 2;
    pOutputData->height = pHWConfig->cropConfig.lastLineCrop - pHWConfig->cropConfig.firstLineCrop + 1;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDAF20Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDAF20Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result      = CamxResultSuccess;

    ISPInputData*       pInputData  = static_cast<ISPInputData*>(pInput);
    PDAF20OutputData*   pOutputData = static_cast<PDAF20OutputData*>(pOutput);
    PDHwConfig*         pPDHwConfig = pInputData->pPDHwConfig;

    if ((NULL != pInput)                   &&
        (NULL != pOutputData)              &&
        (NULL != pOutputData->pDMIDataPtr) &&
        (NULL != pPDHwConfig))
    {
        // m_pdModuleEnable.pdModuleEnable.bitfields.DUAL_PD_EN = pPDHwConfig->enablePDHw;
        m_regCmd.moduleConfig.bitfields.EN                  = pPDHwConfig->enablePDHw; // Need to check

        if (pPDHwConfig->modeSelect == PDHwModeSelect::PDHwModeSelectDualPD)
        {
            m_regCmd.moduleConfig.bitfields.PDAF_MODE_SEL = 0;
        }
        else if (pPDHwConfig->modeSelect == PDHwModeSelect::PDHwModeSelectSparsePD)
        {
            m_regCmd.moduleConfig.bitfields.PDAF_MODE_SEL = 1;
        }
        else
        {
            m_regCmd.moduleConfig.bitfields.EN = 0;
        }

        if (TRUE == pInputData->bankUpdate.isValid)
        {
            m_bankSelect = pInputData->bankUpdate.PDAFBank;
        }

        m_regCmd.DMILUTBankConfig.bitfields.BANK_SEL      = m_bankSelect;
        m_regCmd.moduleLUTBankConfig.bitfields.BANK_SEL   = m_bankSelect;

        // Toggle the bank value for the next request
        m_bankSelect ^= 1;

        m_regCmd.moduleConfig.bitfields.CROP_EN           = pPDHwConfig->cropConfig.enablePDCrop;
        m_regCmd.moduleConfig.bitfields.HDR_EN            = pPDHwConfig->HDRConfig.hdrEnable;
        m_regCmd.moduleConfig.bitfields.BIN_SKIP_EN       = pPDHwConfig->binConfig.enableSkipBinning;
        m_regCmd.moduleConfig.bitfields.GAIN_MAP_EN       = pPDHwConfig->gainMapConfig.gainMapEnable;
        m_regCmd.moduleConfig.bitfields.IIR_EN            = pPDHwConfig->IIRConfig.IIREnable;
        m_regCmd.moduleConfig.bitfields.SAD_EN            = pPDHwConfig->SADConfig.sadEnable;
        m_regCmd.moduleConfig.bitfields.PD_FIRST_PIXEL_LT = pPDHwConfig->firstPixelSelect;
        m_regCmd.moduleConfig.bitfields.SAD_SHIFT         = pPDHwConfig->SADConfig.sadShift;

        result = ConfigureDualPD(pPDHwConfig, pOutput);

        if (pPDHwConfig->modeSelect == PDHwModeSelect::PDHwModeSelectSparsePD)
        {
            result = ConfigureSparsePD(pPDHwConfig, pOutput);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDAF20Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDAF20Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDAF20Titan480::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEPDAF20Titan480::CopyRegCmd(
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
// IFEPDAF20Titan480::~IFEPDAF20Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEPDAF20Titan480::~IFEPDAF20Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDAF20Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEPDAF20Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDAF20 Config     [0x%x]", m_regCmd.moduleConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdCropWidthCfg    [0x%x]", m_regCmd.dualPDConfig.cfg1.pdCropWidthCfg.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdCropHeightCfg   [0x%x]", m_regCmd.dualPDConfig.cfg1.pdCropHeightCfg.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdBLSCfg          [0x%x]", m_regCmd.dualPDConfig.cfg1.pdBLSCfg.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdRegionOffsetCfg [0x%x]", m_regCmd.dualPDConfig.cfg1.pdRegionOffsetCfg.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdRegionNumberCfg [0x%x]", m_regCmd.dualPDConfig.cfg1.pdRegionNumberCfg.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdRegionSizeCfg   [0x%x]", m_regCmd.dualPDConfig.cfg1.pdRegionSizeCfg.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdHDRCfg          [0x%x]", m_regCmd.dualPDConfig.cfg1.pdHDRCfg.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdBinSkipCfg      [0x%x]", m_regCmd.dualPDConfig.cfg1.pdBinSkipCfg.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdGMCfg0          [0x%x]", m_regCmd.dualPDConfig.cfg1.pdGMCfg0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdGMCfg1          [0x%x]", m_regCmd.dualPDConfig.cfg1.pdGMCfg1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdGMCfg2          [0x%x]", m_regCmd.dualPDConfig.cfg2.pdGMCfg2.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdGMCfg3          [0x%x]", m_regCmd.dualPDConfig.cfg2.pdGMCfg3.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdIIRCfg0         [0x%x]", m_regCmd.dualPDConfig.cfg2.pdIIRCfg0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdIIRCfg1         [0x%x]", m_regCmd.dualPDConfig.cfg2.pdIIRCfg1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdIIRCfg2         [0x%x]", m_regCmd.dualPDConfig.cfg2.pdIIRCfg2.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdIIRCfg3         [0x%x]", m_regCmd.dualPDConfig.cfg2.pdIIRCfg3.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdSADPhaseCfg0    [0x%x]", m_regCmd.dualPDConfig.cfg2.pdSADPhaseCfg0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pdSADPhaseCfg1    [0x%x]", m_regCmd.dualPDConfig.cfg2.pdSADPhaseCfg1.u32All);
}

CAMX_NAMESPACE_END
