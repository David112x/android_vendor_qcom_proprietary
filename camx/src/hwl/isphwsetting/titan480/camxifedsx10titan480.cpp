////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifedsx10titan480.cpp
/// @brief CAMXIFEDSX10TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifedsx10titan480.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSX10Titan480::IFEDSX10Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDSX10Titan480::IFEDSX10Titan480()
{
    SetCommandLength(
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEDSXVideoLumaReg)/ RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEDSXVideoLumaPaddingTopBottomReg)/
                                                                                            RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEDSXVideoLumaPaddingLeftRightReg)/
                                                                                            RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEDSXVideoChromaReg) / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEDSXVideoChromaPaddingTopBottomReg)/
                                                                                                RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEDSXVideoChromaPaddingLeftRightReg)/
                                                                                                RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteDMISizeInDwords() * 4);

    Set32bitDMILength(IFEDSX10DMITotalDWord);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSX10Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSX10Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;
    UINT32        offset        = 0;
    UINT32        lengthInBytes = 0;

    if ((NULL != pInputData) && (NULL != pInputData->pCmdBuffer))
    {
        pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                regIFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_DMI_LUT_BANK_CFG,
                                              (sizeof(IFEDSXVideoLumaReg) / RegisterWidthInBytes),
                                              reinterpret_cast<UINT32*>(&m_regCmd.luma));
        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_0,
                                                  (sizeof(IFEDSXVideoLumaPaddingTopBottomReg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.lumaTopBottom));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_0,
                                                  (sizeof(IFEDSXVideoLumaPaddingLeftRightReg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.lumaLeftRight));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_DSX_C_VID_OUT_DMI_LUT_BANK_CFG,
                                                  (sizeof(IFEDSXVideoChromaReg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.chroma));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_DSX_C_VID_OUT_PADDING_WEIGHTS_TOP_BOT_CHROMA_0,
                                                  (sizeof(IFEDSXVideoChromaPaddingTopBottomReg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.chromaTopBottom));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_DSX_C_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_CHROMA_0,
                                                  (sizeof(IFEDSXVideoChromaPaddingLeftRightReg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.chromaLeftRight));
        }


        if (CamxResultSuccess == result)
        {
            offset = (*pDMIBufferOffset + (pInputData->pStripeConfig->stripeId * (IFEDSX10DMITotalDWord))) * sizeof(UINT32);
            lengthInBytes = IFEDSX10LumaHorDMISize;

            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Offset %d lengthInBytes %d ", offset, lengthInBytes);

            result = PacketBuilder::WriteDMI(pInputData->pCmdBuffer,
                                             regIFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_DMI_CFG,
                                             IFEDSXLumaHorizLUT,
                                             pInputData->p32bitDMIBuffer,
                                             offset,
                                             lengthInBytes);
        }

        if (CamxResultSuccess == result)
        {
            offset += lengthInBytes;
            lengthInBytes = IFEDSX10LumaVerDMISize;

            result = PacketBuilder::WriteDMI(pInputData->pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_DMI_CFG,
                                             IFEDSXLumaVertLUT,
                                             pInputData->p32bitDMIBuffer,
                                             offset,
                                             lengthInBytes);
        }
        if (CamxResultSuccess == result)
        {
            offset += lengthInBytes;
            lengthInBytes = IFEDSX10ChromaHorDMISize;

            result = PacketBuilder::WriteDMI(pInputData->pCmdBuffer,
                                            regIFE_IFE_0_PP_CLC_DSX_C_VID_OUT_DMI_CFG,
                                            IFEDSXChromaHorizLUT,
                                            pInputData->p32bitDMIBuffer,
                                            offset,
                                            lengthInBytes);
        }

        if (CamxResultSuccess == result)
        {
            offset += lengthInBytes;
            lengthInBytes = IFEDSX10ChromaVerDMISize;

            result = PacketBuilder::WriteDMI(pInputData->pCmdBuffer,
                                        regIFE_IFE_0_PP_CLC_DSX_C_VID_OUT_DMI_CFG,
                                        IFEDSXChromaVertLUT,
                                        pInputData->p32bitDMIBuffer,
                                        offset,
                                        lengthInBytes);
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure Register/DMI");
        }


    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSX10Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSX10Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result  = CamxResultSuccess;
    DSX10UnpackedField* pData   = static_cast<DSX10UnpackedField*>(pInput);
    DSX10OutputData*    pDSxOutputData = static_cast<DSX10OutputData*>(pOutput);

    if ((NULL != pData) && (NULL != pDSxOutputData) && (NULL != pDSxOutputData->pState))
    {

        m_regCmd.luma.lutBankCfg.bitfields.BANK_SEL      = pDSxOutputData->pState->bankSelect;
        m_regCmd.luma.modulelutConfig.bitfields.BANK_SEL = pDSxOutputData->pState->bankSelect;

        m_regCmd.chroma.lutBankCfg.bitfields.BANK_SEL      = pDSxOutputData->pState->bankSelect;
        m_regCmd.chroma.modulelutConfig.bitfields.BANK_SEL = pDSxOutputData->pState->bankSelect;

        m_regCmd.luma.moduleConfig.bitfields.EN                 = pData->dsxData.en_Y_MODULE_CFG;
        m_regCmd.luma.mode.bitfields.MODE                       = pData->dsxData.luma_mode;
        m_regCmd.luma.mode.bitfields.PADDING_WEIGHTS_EN         = pData->dsxData.luma_padding_weights_en;
        m_regCmd.luma.startLocationX.bitfields.START_LOCATION_X = pData->dsxData.luma_start_location_x;
        m_regCmd.luma.startLocationY.bitfields.START_LOCATION_Y = pData->dsxData.luma_start_location_y;
        m_regCmd.luma.scaleRatioX.bitfields.SCALE_RATIO_X       = pData->dsxData.luma_scale_ratio_x;
        m_regCmd.luma.scaleRatioY.bitfields.SCALE_RATIO_Y       = pData->dsxData.luma_scale_ratio_y;
        m_regCmd.luma.outputSize.bitfields.OUT_WIDTH            = pData->dsxData.luma_out_width;
        m_regCmd.luma.outputSize.bitfields.OUT_HEIGHT           = pData->dsxData.luma_out_height;
        m_regCmd.luma.inputSize.bitfields.IN_WIDTH              = pData->dsxData.luma_input_width;
        m_regCmd.luma.inputSize.bitfields.IN_HEIGHT             = pData->dsxData.luma_input_height;

        for (UINT32 i = 0; i < DSX_LUMA_KERNAL_WEIGHTS_HORIZ; i++)
        {
            pDSxOutputData->pLumaKernelWeightsHoriz[i] = pData->dsxData.luma_kernel_weights_horiz[i];
            pDSxOutputData->pLumaKernelWeightsVert[i]  = pData->dsxData.luma_kernel_weights_vert[i];
        }

        IFEDSXVideoLumaPaddingTopBottomReg* pLumaTopBot = &m_regCmd.lumaTopBottom;

        pLumaTopBot->paddingWeightTopBotLuma0.bitfields.PADDING_WEIGHTS_BOT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_bot[0];
        pLumaTopBot->paddingWeightTopBotLuma0.bitfields.PADDING_WEIGHTS_TOP_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_top[0];
        pLumaTopBot->paddingWeightTopBotLuma1.bitfields.PADDING_WEIGHTS_BOT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_bot[1];
        pLumaTopBot->paddingWeightTopBotLuma1.bitfields.PADDING_WEIGHTS_TOP_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_top[1];
        pLumaTopBot->paddingWeightTopBotLuma2.bitfields.PADDING_WEIGHTS_BOT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_bot[2];
        pLumaTopBot->paddingWeightTopBotLuma2.bitfields.PADDING_WEIGHTS_TOP_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_top[2];
        pLumaTopBot->paddingWeightTopBotLuma3.bitfields.PADDING_WEIGHTS_BOT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_bot[3];
        pLumaTopBot->paddingWeightTopBotLuma3.bitfields.PADDING_WEIGHTS_TOP_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_top[3];
        pLumaTopBot->paddingWeightTopBotLuma4.bitfields.PADDING_WEIGHTS_BOT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_bot[4];
        pLumaTopBot->paddingWeightTopBotLuma4.bitfields.PADDING_WEIGHTS_TOP_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_top[4];
        pLumaTopBot->paddingWeightTopBotLuma5.bitfields.PADDING_WEIGHTS_BOT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_bot[5];
        pLumaTopBot->paddingWeightTopBotLuma5.bitfields.PADDING_WEIGHTS_TOP_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_top[5];
        pLumaTopBot->paddingWeightTopBotLuma6.bitfields.PADDING_WEIGHTS_BOT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_bot[6];
        pLumaTopBot->paddingWeightTopBotLuma6.bitfields.PADDING_WEIGHTS_TOP_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_top[6];
        pLumaTopBot->paddingWeightTopBotLuma7.bitfields.PADDING_WEIGHTS_BOT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_bot[7];
        pLumaTopBot->paddingWeightTopBotLuma7.bitfields.PADDING_WEIGHTS_TOP_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_top[7];
        pLumaTopBot->paddingWeightTopBotLuma8.bitfields.PADDING_WEIGHTS_BOT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_bot[8];
        pLumaTopBot->paddingWeightTopBotLuma8.bitfields.PADDING_WEIGHTS_TOP_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_top[8];
        pLumaTopBot->paddingWeightTopBotLuma9.bitfields.PADDING_WEIGHTS_BOT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_bot[9];
        pLumaTopBot->paddingWeightTopBotLuma9.bitfields.PADDING_WEIGHTS_TOP_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_top[9];
        pLumaTopBot->paddingWeightTopBotLuma10.bitfields.PADDING_WEIGHTS_BOT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_bot[10];
        pLumaTopBot->paddingWeightTopBotLuma10.bitfields.PADDING_WEIGHTS_TOP_LUMA =
                                                                    pData->dsxData.luma_padding_weights_top[10];
        pLumaTopBot->paddingWeightTopBotLuma11.bitfields.PADDING_WEIGHTS_BOT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_bot[11];
        pLumaTopBot->paddingWeightTopBotLuma11.bitfields.PADDING_WEIGHTS_TOP_LUMA =
                                                                    pData->dsxData.luma_padding_weights_top[11];
        pLumaTopBot->paddingWeightTopBotLuma12.bitfields.PADDING_WEIGHTS_BOT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_bot[12];
        pLumaTopBot->paddingWeightTopBotLuma12.bitfields.PADDING_WEIGHTS_TOP_LUMA =
                                                                    pData->dsxData.luma_padding_weights_top[12];
        pLumaTopBot->paddingWeightTopBotLuma13.bitfields.PADDING_WEIGHTS_BOT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_bot[13];
        pLumaTopBot->paddingWeightTopBotLuma13.bitfields.PADDING_WEIGHTS_TOP_LUMA =
                                                                    pData->dsxData.luma_padding_weights_top[13];
        pLumaTopBot->paddingWeightTopBotLuma14.bitfields.PADDING_WEIGHTS_BOT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_bot[14];
        pLumaTopBot->paddingWeightTopBotLuma14.bitfields.PADDING_WEIGHTS_TOP_LUMA =
                                                                    pData->dsxData.luma_padding_weights_top[14];
        pLumaTopBot->paddingWeightTopBotLuma15.bitfields.PADDING_WEIGHTS_BOT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_bot[15];
        pLumaTopBot->paddingWeightTopBotLuma15.bitfields.PADDING_WEIGHTS_TOP_LUMA =
                                                                    pData->dsxData.luma_padding_weights_top[15];
        pLumaTopBot->paddingWeightTopBotLuma16.bitfields.PADDING_WEIGHTS_BOT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_bot[16];
        pLumaTopBot->paddingWeightTopBotLuma16.bitfields.PADDING_WEIGHTS_TOP_LUMA =
                                                                    pData->dsxData.luma_padding_weights_top[16];
        pLumaTopBot->paddingWeightTopBotLuma17.bitfields.PADDING_WEIGHTS_BOT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_bot[17];
        pLumaTopBot->paddingWeightTopBotLuma17.bitfields.PADDING_WEIGHTS_TOP_LUMA =
                                                                    pData->dsxData.luma_padding_weights_top[17];
        pLumaTopBot->paddingWeightTopBotLuma18.bitfields.PADDING_WEIGHTS_BOT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_bot[18];
        pLumaTopBot->paddingWeightTopBotLuma18.bitfields.PADDING_WEIGHTS_TOP_LUMA =
                                                                    pData->dsxData.luma_padding_weights_top[18];
        pLumaTopBot->paddingWeightTopBotLuma19.bitfields.PADDING_WEIGHTS_BOT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_bot[19];
        pLumaTopBot->paddingWeightTopBotLuma19.bitfields.PADDING_WEIGHTS_TOP_LUMA =
                                                                    pData->dsxData.luma_padding_weights_top[19];
        pLumaTopBot->paddingWeightTopBotLuma20.bitfields.PADDING_WEIGHTS_BOT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_bot[20];
        pLumaTopBot->paddingWeightTopBotLuma20.bitfields.PADDING_WEIGHTS_TOP_LUMA =
                                                                    pData->dsxData.luma_padding_weights_top[20];
        pLumaTopBot->paddingWeightTopBotLuma21.bitfields.PADDING_WEIGHTS_BOT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_bot[21];
        pLumaTopBot->paddingWeightTopBotLuma21.bitfields.PADDING_WEIGHTS_TOP_LUMA =
                                                                    pData->dsxData.luma_padding_weights_top[21];
        pLumaTopBot->paddingWeightTopBotLuma22.bitfields.PADDING_WEIGHTS_BOT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_bot[22];
        pLumaTopBot->paddingWeightTopBotLuma22.bitfields.PADDING_WEIGHTS_TOP_LUMA =
                                                                    pData->dsxData.luma_padding_weights_top[22];
        pLumaTopBot->paddingWeightTopBotLuma23.bitfields.PADDING_WEIGHTS_BOT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_bot[23];
        pLumaTopBot->paddingWeightTopBotLuma23.bitfields.PADDING_WEIGHTS_TOP_LUMA =
                                                                    pData->dsxData.luma_padding_weights_top[23];


        IFEDSXVideoLumaPaddingLeftRightReg* pLumaLeftRgt = &m_regCmd.lumaLeftRight;

        pLumaLeftRgt->paddingWeightLeftRgtLuma0.bitfields.PADDING_WEIGHTS_LEFT_LUMA   =
                                                                    pData->dsxData.luma_padding_weights_left[0];
        pLumaLeftRgt->paddingWeightLeftRgtLuma0.bitfields.PADDING_WEIGHTS_RIGHT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_right[0];
        pLumaLeftRgt->paddingWeightLeftRgtLuma1.bitfields.PADDING_WEIGHTS_LEFT_LUMA   =
                                                                    pData->dsxData.luma_padding_weights_left[1];
        pLumaLeftRgt->paddingWeightLeftRgtLuma1.bitfields.PADDING_WEIGHTS_RIGHT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_right[1];
        pLumaLeftRgt->paddingWeightLeftRgtLuma2.bitfields.PADDING_WEIGHTS_LEFT_LUMA   =
                                                                    pData->dsxData.luma_padding_weights_left[2];
        pLumaLeftRgt->paddingWeightLeftRgtLuma2.bitfields.PADDING_WEIGHTS_RIGHT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_right[2];
        pLumaLeftRgt->paddingWeightLeftRgtLuma3.bitfields.PADDING_WEIGHTS_LEFT_LUMA   =
                                                                    pData->dsxData.luma_padding_weights_left[3];
        pLumaLeftRgt->paddingWeightLeftRgtLuma3.bitfields.PADDING_WEIGHTS_RIGHT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_right[3];
        pLumaLeftRgt->paddingWeightLeftRgtLuma4.bitfields.PADDING_WEIGHTS_LEFT_LUMA   =
                                                                    pData->dsxData.luma_padding_weights_left[4];
        pLumaLeftRgt->paddingWeightLeftRgtLuma4.bitfields.PADDING_WEIGHTS_RIGHT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_right[4];
        pLumaLeftRgt->paddingWeightLeftRgtLuma5.bitfields.PADDING_WEIGHTS_LEFT_LUMA   =
                                                                    pData->dsxData.luma_padding_weights_left[5];
        pLumaLeftRgt->paddingWeightLeftRgtLuma5.bitfields.PADDING_WEIGHTS_RIGHT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_right[5];
        pLumaLeftRgt->paddingWeightLeftRgtLuma6.bitfields.PADDING_WEIGHTS_LEFT_LUMA   =
                                                                    pData->dsxData.luma_padding_weights_left[6];
        pLumaLeftRgt->paddingWeightLeftRgtLuma6.bitfields.PADDING_WEIGHTS_RIGHT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_right[6];
        pLumaLeftRgt->paddingWeightLeftRgtLuma7.bitfields.PADDING_WEIGHTS_LEFT_LUMA   =
                                                                    pData->dsxData.luma_padding_weights_left[7];
        pLumaLeftRgt->paddingWeightLeftRgtLuma7.bitfields.PADDING_WEIGHTS_RIGHT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_right[7];
        pLumaLeftRgt->paddingWeightLeftRgtLuma8.bitfields.PADDING_WEIGHTS_LEFT_LUMA   =
                                                                    pData->dsxData.luma_padding_weights_left[8];
        pLumaLeftRgt->paddingWeightLeftRgtLuma8.bitfields.PADDING_WEIGHTS_RIGHT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_right[8];
        pLumaLeftRgt->paddingWeightLeftRgtLuma9.bitfields.PADDING_WEIGHTS_LEFT_LUMA   =
                                                                    pData->dsxData.luma_padding_weights_left[9];
        pLumaLeftRgt->paddingWeightLeftRgtLuma9.bitfields.PADDING_WEIGHTS_RIGHT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_right[9];
        pLumaLeftRgt->paddingWeightLeftRgtLuma10.bitfields.PADDING_WEIGHTS_LEFT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_left[10];
        pLumaLeftRgt->paddingWeightLeftRgtLuma10.bitfields.PADDING_WEIGHTS_RIGHT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_right[10];
        pLumaLeftRgt->paddingWeightLeftRgtLuma11.bitfields.PADDING_WEIGHTS_LEFT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_left[11];
        pLumaLeftRgt->paddingWeightLeftRgtLuma11.bitfields.PADDING_WEIGHTS_RIGHT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_right[11];
        pLumaLeftRgt->paddingWeightLeftRgtLuma12.bitfields.PADDING_WEIGHTS_LEFT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_left[12];
        pLumaLeftRgt->paddingWeightLeftRgtLuma12.bitfields.PADDING_WEIGHTS_RIGHT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_right[12];
        pLumaLeftRgt->paddingWeightLeftRgtLuma13.bitfields.PADDING_WEIGHTS_LEFT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_left[13];
        pLumaLeftRgt->paddingWeightLeftRgtLuma13.bitfields.PADDING_WEIGHTS_RIGHT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_right[13];
        pLumaLeftRgt->paddingWeightLeftRgtLuma14.bitfields.PADDING_WEIGHTS_LEFT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_left[14];
        pLumaLeftRgt->paddingWeightLeftRgtLuma14.bitfields.PADDING_WEIGHTS_RIGHT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_right[14];
        pLumaLeftRgt->paddingWeightLeftRgtLuma15.bitfields.PADDING_WEIGHTS_LEFT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_left[15];
        pLumaLeftRgt->paddingWeightLeftRgtLuma15.bitfields.PADDING_WEIGHTS_RIGHT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_right[15];
        pLumaLeftRgt->paddingWeightLeftRgtLuma16.bitfields.PADDING_WEIGHTS_LEFT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_left[16];
        pLumaLeftRgt->paddingWeightLeftRgtLuma16.bitfields.PADDING_WEIGHTS_RIGHT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_right[16];
        pLumaLeftRgt->paddingWeightLeftRgtLuma17.bitfields.PADDING_WEIGHTS_LEFT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_left[17];
        pLumaLeftRgt->paddingWeightLeftRgtLuma17.bitfields.PADDING_WEIGHTS_RIGHT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_right[17];
        pLumaLeftRgt->paddingWeightLeftRgtLuma18.bitfields.PADDING_WEIGHTS_LEFT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_left[18];
        pLumaLeftRgt->paddingWeightLeftRgtLuma18.bitfields.PADDING_WEIGHTS_RIGHT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_right[18];
        pLumaLeftRgt->paddingWeightLeftRgtLuma19.bitfields.PADDING_WEIGHTS_LEFT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_left[19];
        pLumaLeftRgt->paddingWeightLeftRgtLuma19.bitfields.PADDING_WEIGHTS_RIGHT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_right[19];
        pLumaLeftRgt->paddingWeightLeftRgtLuma20.bitfields.PADDING_WEIGHTS_LEFT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_left[20];
        pLumaLeftRgt->paddingWeightLeftRgtLuma20.bitfields.PADDING_WEIGHTS_RIGHT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_right[20];
        pLumaLeftRgt->paddingWeightLeftRgtLuma21.bitfields.PADDING_WEIGHTS_LEFT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_left[21];
        pLumaLeftRgt->paddingWeightLeftRgtLuma21.bitfields.PADDING_WEIGHTS_RIGHT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_right[21];
        pLumaLeftRgt->paddingWeightLeftRgtLuma22.bitfields.PADDING_WEIGHTS_LEFT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_left[22];
        pLumaLeftRgt->paddingWeightLeftRgtLuma22.bitfields.PADDING_WEIGHTS_RIGHT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_right[22];
        pLumaLeftRgt->paddingWeightLeftRgtLuma23.bitfields.PADDING_WEIGHTS_LEFT_LUMA  =
                                                                    pData->dsxData.luma_padding_weights_left[23];
        pLumaLeftRgt->paddingWeightLeftRgtLuma23.bitfields.PADDING_WEIGHTS_RIGHT_LUMA =
                                                                    pData->dsxData.luma_padding_weights_right[23];

        m_regCmd.chroma.moduleConfig.bitfields.EN                 = pData->dsxData.en_C_MODULE_CFG;

        m_regCmd.chroma.mode.bitfields.MODE                       = pData->dsxData.chroma_mode;
        m_regCmd.chroma.mode.bitfields.PADDING_WEIGHTS_EN         = pData->dsxData.chroma_padding_weights_en;
        m_regCmd.chroma.startLocationX.bitfields.START_LOCATION_X = pData->dsxData.chroma_start_location_x;
        m_regCmd.chroma.startLocationY.bitfields.START_LOCATION_Y = pData->dsxData.chroma_start_location_y;
        m_regCmd.chroma.scaleRatioX.bitfields.SCALE_RATIO_X       = pData->dsxData.chroma_scale_ratio_x;
        m_regCmd.chroma.scaleRatioY.bitfields.SCALE_RATIO_Y       = pData->dsxData.chroma_scale_ratio_y;
        m_regCmd.chroma.outputSize.bitfields.OUT_WIDTH            = pData->dsxData.chroma_out_width;
        m_regCmd.chroma.outputSize.bitfields.OUT_HEIGHT           = pData->dsxData.chroma_out_height;
        m_regCmd.chroma.inputSize.bitfields.IN_WIDTH              = pData->dsxData.chroma_input_width;
        m_regCmd.chroma.inputSize.bitfields.IN_HEIGHT             = pData->dsxData.chroma_input_height;

        for (UINT32 i = 0; i < DSX_CHROMA_KERNAL_WEIGHTS_HORIZ; i++)
        {
            pDSxOutputData->pChromaKernelWeightsHoriz[i] = pData->dsxData.chroma_kernel_weights_horiz[i];
            pDSxOutputData->pChromaKernelWeightsVert[i]  = pData->dsxData.chroma_kernel_weights_vert[i];
        }

        IFEDSXVideoChromaPaddingTopBottomReg* pChromaTopBot = &m_regCmd.chromaTopBottom;

        pChromaTopBot->paddingWeightTopBotChroma0.bitfields.PADDING_WEIGHTS_BOT_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_bot[0];
        pChromaTopBot->paddingWeightTopBotChroma0.bitfields.PADDING_WEIGHTS_TOP_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_top[0];
        pChromaTopBot->paddingWeightTopBotChroma1.bitfields.PADDING_WEIGHTS_BOT_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_bot[1];
        pChromaTopBot->paddingWeightTopBotChroma1.bitfields.PADDING_WEIGHTS_TOP_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_top[1];
        pChromaTopBot->paddingWeightTopBotChroma2.bitfields.PADDING_WEIGHTS_BOT_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_bot[2];
        pChromaTopBot->paddingWeightTopBotChroma2.bitfields.PADDING_WEIGHTS_TOP_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_top[2];
        pChromaTopBot->paddingWeightTopBotChroma3.bitfields.PADDING_WEIGHTS_BOT_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_bot[3];
        pChromaTopBot->paddingWeightTopBotChroma3.bitfields.PADDING_WEIGHTS_TOP_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_top[3];
        pChromaTopBot->paddingWeightTopBotChroma4.bitfields.PADDING_WEIGHTS_BOT_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_bot[4];
        pChromaTopBot->paddingWeightTopBotChroma4.bitfields.PADDING_WEIGHTS_TOP_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_top[4];
        pChromaTopBot->paddingWeightTopBotChroma5.bitfields.PADDING_WEIGHTS_BOT_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_bot[5];
        pChromaTopBot->paddingWeightTopBotChroma5.bitfields.PADDING_WEIGHTS_TOP_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_top[5];
        pChromaTopBot->paddingWeightTopBotChroma6.bitfields.PADDING_WEIGHTS_BOT_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_bot[6];
        pChromaTopBot->paddingWeightTopBotChroma6.bitfields.PADDING_WEIGHTS_TOP_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_top[6];
        pChromaTopBot->paddingWeightTopBotChroma7.bitfields.PADDING_WEIGHTS_BOT_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_bot[7];
        pChromaTopBot->paddingWeightTopBotChroma7.bitfields.PADDING_WEIGHTS_TOP_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_top[7];

        IFEDSXVideoChromaPaddingLeftRightReg* pChromaLeftRgt = &m_regCmd.chromaLeftRight;

        pChromaLeftRgt->paddingWeightLeftRgtChroma0.bitfields.PADDING_WEIGHTS_LEFT_CHROMA  =
                                                                        pData->dsxData.chroma_padding_weights_left[0];
        pChromaLeftRgt->paddingWeightLeftRgtChroma0.bitfields.PADDING_WEIGHTS_RIGHT_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_right[0];
        pChromaLeftRgt->paddingWeightLeftRgtChroma1.bitfields.PADDING_WEIGHTS_LEFT_CHROMA  =
                                                                        pData->dsxData.chroma_padding_weights_left[1];
        pChromaLeftRgt->paddingWeightLeftRgtChroma1.bitfields.PADDING_WEIGHTS_RIGHT_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_right[1];
        pChromaLeftRgt->paddingWeightLeftRgtChroma2.bitfields.PADDING_WEIGHTS_LEFT_CHROMA  =
                                                                        pData->dsxData.chroma_padding_weights_left[2];
        pChromaLeftRgt->paddingWeightLeftRgtChroma2.bitfields.PADDING_WEIGHTS_RIGHT_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_right[2];
        pChromaLeftRgt->paddingWeightLeftRgtChroma3.bitfields.PADDING_WEIGHTS_LEFT_CHROMA  =
                                                                        pData->dsxData.chroma_padding_weights_left[3];
        pChromaLeftRgt->paddingWeightLeftRgtChroma3.bitfields.PADDING_WEIGHTS_RIGHT_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_right[3];
        pChromaLeftRgt->paddingWeightLeftRgtChroma4.bitfields.PADDING_WEIGHTS_LEFT_CHROMA  =
                                                                        pData->dsxData.chroma_padding_weights_left[4];
        pChromaLeftRgt->paddingWeightLeftRgtChroma4.bitfields.PADDING_WEIGHTS_RIGHT_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_right[4];
        pChromaLeftRgt->paddingWeightLeftRgtChroma5.bitfields.PADDING_WEIGHTS_LEFT_CHROMA  =
                                                                        pData->dsxData.chroma_padding_weights_left[5];
        pChromaLeftRgt->paddingWeightLeftRgtChroma5.bitfields.PADDING_WEIGHTS_RIGHT_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_right[5];
        pChromaLeftRgt->paddingWeightLeftRgtChroma6.bitfields.PADDING_WEIGHTS_LEFT_CHROMA  =
                                                                        pData->dsxData.chroma_padding_weights_left[6];
        pChromaLeftRgt->paddingWeightLeftRgtChroma6.bitfields.PADDING_WEIGHTS_RIGHT_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_right[6];
        pChromaLeftRgt->paddingWeightLeftRgtChroma7.bitfields.PADDING_WEIGHTS_LEFT_CHROMA  =
                                                                        pData->dsxData.chroma_padding_weights_left[7];
        pChromaLeftRgt->paddingWeightLeftRgtChroma7.bitfields.PADDING_WEIGHTS_RIGHT_CHROMA =
                                                                        pData->dsxData.chroma_padding_weights_right[7];
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input Data pData %p pDSxOutputData %p", pData, pDSxOutputData);
        result = CamxResultEInvalidArg;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSX10Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSX10Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Not Implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSX10Titan480::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEDSX10Titan480::CopyRegCmd(
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
// IFEDSX10Titan480::~IFEDSX10Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDSX10Titan480::~IFEDSX10Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSX10Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDSX10Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX luma Config         [0x%x]",
          m_regCmd.luma.moduleConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX luma mode           [0x%x]",
            m_regCmd.luma.mode.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX.luma start X        [0x%x]",
            m_regCmd.luma.startLocationX.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX luma start Y        [0x%x]",
            m_regCmd.luma.startLocationY.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX luma scale Ratio X  [0x%x]",
            m_regCmd.luma.scaleRatioX.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX luma scale ratio Y  [0x%x]",
            m_regCmd.luma.scaleRatioX.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX luma output Size    [0x%x]",
            m_regCmd.luma.outputSize.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX luma Input  Size    [0x%x]",
            m_regCmd.luma.inputSize.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX luma LutBnkCfg    [0x%x]",
            m_regCmd.luma.lutBankCfg.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX luma modulelutConfig    [0x%x]",
        m_regCmd.luma.modulelutConfig.u32All);

    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 0   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 1   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 2   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma2.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 3   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma3.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 4   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma4.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 5   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma5.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 6   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma6.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 7   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma7.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 8   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma8.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 9   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma9.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 10   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma10.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 11   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma11.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 12   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma12.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 13   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma13.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 14  [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma14.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 15   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma15.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 16   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma16.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 17   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma17.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 18   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma18.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 19   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma19.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 20   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma20.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 21   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma21.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 22   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma22.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 23   [0x%x]",
        m_regCmd.lumaTopBottom.paddingWeightTopBotLuma23.u32All);


    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 0   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 1   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 2   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma2.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 3   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma3.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 4   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma4.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 5   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma5.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 6   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma6.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 7   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma7.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 8   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma8.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 9   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma9.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 10   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma10.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 11   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma11.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 12   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma12.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 13   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma13.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 14  [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma14.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 15   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma15.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 16   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma16.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 17   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma17.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 18   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma18.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 19   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma19.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 20   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma20.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 21   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma21.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 22   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma22.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 23   [0x%x]",
        m_regCmd.lumaLeftRight.paddingWeightLeftRgtLuma23.u32All);

    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX chroma Config        [0x%x]",
          m_regCmd.chroma.moduleConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX chroma mode          [0x%x]",
            m_regCmd.chroma.mode.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX.chroma start X       [0x%x]",
            m_regCmd.chroma.startLocationX.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX chroma start Y       [0x%x]",
            m_regCmd.chroma.startLocationY.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX chroma scale Ratio X [0x%x]",
            m_regCmd.chroma.scaleRatioX.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX chroma scale ratio Y [0x%x]",
            m_regCmd.chroma.scaleRatioX.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX chroma output Size   [0x%x]",
            m_regCmd.chroma.outputSize.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX chroma Input  Size   [0x%x]",
            m_regCmd.chroma.inputSize.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX chromaLUT LutBnkCfg    [0x%x]",
        m_regCmd.chroma.lutBankCfg.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX chromaLUT modulelutConfig    [0x%x]",
        m_regCmd.chroma.modulelutConfig.u32All);


    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 0   [0x%x]",
        m_regCmd.chromaTopBottom.paddingWeightTopBotChroma0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 1   [0x%x]",
        m_regCmd.chromaTopBottom.paddingWeightTopBotChroma1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 2   [0x%x]",
        m_regCmd.chromaTopBottom.paddingWeightTopBotChroma2.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 3   [0x%x]",
        m_regCmd.chromaTopBottom.paddingWeightTopBotChroma3.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 4   [0x%x]",
        m_regCmd.chromaTopBottom.paddingWeightTopBotChroma4.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 5   [0x%x]",
        m_regCmd.chromaTopBottom.paddingWeightTopBotChroma5.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 6   [0x%x]",
        m_regCmd.chromaTopBottom.paddingWeightTopBotChroma6.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 7   [0x%x]",
        m_regCmd.chromaTopBottom.paddingWeightTopBotChroma7.u32All);


    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 0   [0x%x]",
        m_regCmd.chromaLeftRight.paddingWeightLeftRgtChroma0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 1   [0x%x]",
        m_regCmd.chromaLeftRight.paddingWeightLeftRgtChroma1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 2   [0x%x]",
        m_regCmd.chromaLeftRight.paddingWeightLeftRgtChroma2.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 3   [0x%x]",
        m_regCmd.chromaLeftRight.paddingWeightLeftRgtChroma3.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 4   [0x%x]",
        m_regCmd.chromaLeftRight.paddingWeightLeftRgtChroma4.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 5   [0x%x]",
        m_regCmd.chromaLeftRight.paddingWeightLeftRgtChroma5.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 6   [0x%x]",
        m_regCmd.chromaLeftRight.paddingWeightLeftRgtChroma6.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Top Bottom Pad Config 7   [0x%x]",
        m_regCmd.chromaLeftRight.paddingWeightLeftRgtChroma7.u32All);



}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSX10Titan480::DumpDMIData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDSX10Titan480::DumpDMIData(
    VOID* pData)
{
    DSX10OutputData* pOutputData = static_cast<DSX10OutputData*>(pData);

    if (NULL != pOutputData)
    {
        for (UINT32 i = 0; i < DSX_LUMA_KERNAL_WEIGHTS_HORIZ; )
        {
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pLumaKernelWeightsHoriz %llu %llu %llu %llu %llu %llu %llu %llu",
                pOutputData->pLumaKernelWeightsHoriz[i],
                pOutputData->pLumaKernelWeightsHoriz[i + 1],
                pOutputData->pLumaKernelWeightsHoriz[i + 2],
                pOutputData->pLumaKernelWeightsHoriz[i + 3],
                pOutputData->pLumaKernelWeightsHoriz[i + 4],
                pOutputData->pLumaKernelWeightsHoriz[i + 5],
                pOutputData->pLumaKernelWeightsHoriz[i + 6],
                pOutputData->pLumaKernelWeightsHoriz[i + 7]);
            i = i + 8;
        }

        for (UINT32 i = 0; i < DSX_LUMA_KERNAL_WEIGHTS_HORIZ; )
        {
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pLumaKernelWeightsVert %llu %llu %llu %llu %llu %llu %llu %llu",
                pOutputData->pLumaKernelWeightsVert[i],
                pOutputData->pLumaKernelWeightsVert[i + 1],
                pOutputData->pLumaKernelWeightsVert[i + 2],
                pOutputData->pLumaKernelWeightsVert[i + 3],
                pOutputData->pLumaKernelWeightsVert[i + 4],
                pOutputData->pLumaKernelWeightsVert[i + 5],
                pOutputData->pLumaKernelWeightsVert[i + 6],
                pOutputData->pLumaKernelWeightsVert[i + 7]);
            i = i + 8;
        }

        for (UINT32 i = 0; i < DSX_CHROMA_KERNAL_WEIGHTS_HORIZ; )
        {
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pChromaKernelWeightsHoriz %llu %llu %llu %llu %llu %llu %llu %llu",
                pOutputData->pChromaKernelWeightsHoriz[i],
                pOutputData->pChromaKernelWeightsHoriz[i + 1],
                pOutputData->pChromaKernelWeightsHoriz[i + 2],
                pOutputData->pChromaKernelWeightsHoriz[i + 3],
                pOutputData->pChromaKernelWeightsHoriz[i + 4],
                pOutputData->pChromaKernelWeightsHoriz[i + 5],
                pOutputData->pChromaKernelWeightsHoriz[i + 6],
                pOutputData->pChromaKernelWeightsHoriz[i + 7]);
            i = i + 8;
        }

        for (UINT32 i = 0; i < DSX_CHROMA_KERNAL_WEIGHTS_HORIZ; )
        {
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pChromaKernelWeightsVert %llu %llu %llu %llu %llu %llu %llu %llu",
                pOutputData->pChromaKernelWeightsVert[i],
                pOutputData->pChromaKernelWeightsVert[i + 1],
                pOutputData->pChromaKernelWeightsVert[i + 2],
                pOutputData->pChromaKernelWeightsVert[i + 3],
                pOutputData->pChromaKernelWeightsVert[i + 4],
                pOutputData->pChromaKernelWeightsVert[i + 5],
                pOutputData->pChromaKernelWeightsVert[i + 6],
                pOutputData->pChromaKernelWeightsVert[i + 7]);
            i = i + 8;
        }
    }

}



CAMX_NAMESPACE_END
