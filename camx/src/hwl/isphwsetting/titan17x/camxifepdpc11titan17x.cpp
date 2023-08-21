////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifepdpc11titan17x.cpp
/// @brief CAMXIFEPDPC11TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifepdpc11titan17x.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC11Titan17x::IFEPDPC11Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEPDPC11Titan17x::IFEPDPC11Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEPDPC11RegLengthDword));
    Set32bitDMILength(IFEPDPC11DMILengthDword);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC11Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDPC11Titan17x::CreateCmdList(
    VOID* pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result       = CamxResultSuccess;
    ISPInputData* pInputData   = static_cast<ISPInputData*>(pSettingData);
    UINT32        offset       = (*pDMIBufferOffset +
                                 (pInputData->pStripeConfig->stripeId * IFEPDPC11DMILengthDword)) * sizeof(UINT32);
    UINT32        lengthInByte = IFEPDPC11DMILengthDword * sizeof(UINT32);

    if (NULL != pInputData                  &&
        NULL != pInputData->pCmdBuffer      &&
        NULL != pInputData->p32bitDMIBuffer)
    {
        CmdBuffer*    pCmdBuffer   = pInputData->pCmdBuffer;
        CmdBuffer*    pDMIBuffer   = pInputData->p32bitDMIBuffer;
        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_VFE_PDAF_CFG,
                                              IFEPDPC11RegLengthDword,
                                              reinterpret_cast<UINT32*>(&m_regCmd));
        CAMX_ASSERT(CamxResultSuccess == result);

        if (FALSE == m_isLUTLoaded)
        {
            result = PacketBuilder::WriteDMI(pCmdBuffer,
                    regIFE_IFE_0_VFE_DMI_CFG,
                    PDAFLUT,
                    pDMIBuffer,
                    offset,
                    lengthInByte);

            if (CamxResultSuccess == result)
            {
                m_isLUTLoaded = TRUE;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to program DMI buffer");
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC11Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDPC11Titan17x::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEPDPC11RegCmd) <= sizeof(pIFETuningMetadata->metadata17x.IFEPDPCData.PDPCConfig));
        Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFEPDPCData.PDPCConfig, &m_regCmd, sizeof(IFEPDPC11RegCmd));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "IFE Tuning metadata is Null");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC11Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDPC11Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult              result        = CamxResultSuccess;
    PDPC11UnpackedField*    pData         = static_cast<PDPC11UnpackedField*>(pInput);
    PDPC11OutputData*       pOutputData   = static_cast<PDPC11OutputData*>(pOutput);

    if ((NULL != pOutputData) && (NULL != pData) && (NULL != pOutputData->pDMIDataPtr))
    {
        m_regCmd.configRegister.bitfields.BLACK_LEVEL             = pData->pdaf_blacklevel;
        m_regCmd.configRegister.bitfields.PDAF_DSBPC_EN           = pData->pdaf_dsbpc_en;
        m_regCmd.configRegister.bitfields.PDAF_PDPC_EN            = pData->pdaf_pdpc_en;

        m_regCmd.gainBGWBRegister.bitfields.BG_WB_GAIN            = pData->bg_wb_gain;
        m_regCmd.gainGBWBRegister.bitfields.GB_WB_GAIN            = pData->gb_wb_gain;
        m_regCmd.gainGRWBRegister.bitfields.GR_WB_GAIN            = pData->gr_wb_gain;
        m_regCmd.gainRGWBRegister.bitfields.RG_WB_GAIN            = pData->rg_wb_gain;

        m_regCmd.offsetBPRegister.bitfields.OFFSET_G_PIXEL        = pData->bp_offset_g_pixel;
        m_regCmd.offsetBPRegister.bitfields.OFFSET_RB_PIXEL       = pData->bp_offset_rb_pixel;

        m_regCmd.offsetLOCConfigRegister.bitfields.X_OFFSET       = pData->pdaf_global_offset_x;
        m_regCmd.offsetLOCConfigRegister.bitfields.Y_OFFSET       = pData->pdaf_global_offset_y;

        m_regCmd.offsetT2BPRegister.bitfields.T2_OFFSET_G_PIXEL   = pData->t2_bp_offset_g_pixel;
        m_regCmd.offsetT2BPRegister.bitfields.T2_OFFSET_RB_PIXEL  = pData->t2_bp_offset_rb_pixel;

        m_regCmd.ratioHDRRegister.bitfields.EXP_RATIO             = pData->pdaf_hdr_exp_ratio;
        m_regCmd.ratioHDRRegister.bitfields.EXP_RATIO_RECIP       = pData->pdaf_hdr_exp_ratio_recip;

        m_regCmd.setBPTHregister.bitfields.FMAX                   = pData->fmax_pixel_Q6;
        m_regCmd.setBPTHregister.bitfields.FMIN                   = pData->fmin_pixel_Q6;

        m_regCmd.setLOCENDConfigRegister.bitfields.X_END          = pData->pdaf_x_end;
        m_regCmd.setLOCENDConfigRegister.bitfields.Y_END          = pData->pdaf_y_end;

        if (NULL != pOutputData->pDMIDataPtr)
        {
            Utils::Memcpy(pOutputData->pDMIDataPtr, pData->PDAF_PD_Mask, sizeof(UINT32)* DMIRAM_PDAF_LUT_LENGTH);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input params are NULL pData=0x%p pOutputData=0x%p", pData, pOutputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC11Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDPC11Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC11Titan17x::~IFEPDPC11Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEPDPC11Titan17x::~IFEPDPC11Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC11Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEPDPC11Titan17x::DumpRegConfig()
{
}

CAMX_NAMESPACE_END
