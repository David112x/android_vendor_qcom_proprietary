////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifeabf34titan17x.cpp
/// @brief CAMXIFEABF34TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifeabf34titan17x.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF34Titan17x::IFEABF34Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEABF34Titan17x::IFEABF34Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEABF34RegLength1DWord) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEABF34RegLength2DWord) +
                     PacketBuilder::RequiredWriteDMISizeInDwords());
    Set32bitDMILength(IFEABF34LUTLengthDWord);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF34Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF34Titan17x::CreateCmdList(
    VOID* pSettingData,
    UINT32* pDMIBufferOffset)
{

    CamxResult      result          = CamxResultSuccess;
    ISPInputData*   pInputData      = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*      pCmdBuffer      = NULL;
    CmdBuffer*      pDMIBuffer      = NULL;
    UINT32          offset          = (*pDMIBufferOffset +
                                      (pInputData->pStripeConfig->stripeId * IFEABF34LUTLengthDWord)) *
                                      sizeof(UINT32);
    UINT32          lengthInByte    = IFEABF34LUTLengthDWord * sizeof(UINT32);
    UINT32          bankSelect      = (m_regCmd1.configReg.bitfields.LUT_BANK_SEL == 0) ? ABFBank0 : ABFBank1;

    if (NULL != pInputData                  &&
        NULL != pInputData->pCmdBuffer      &&
        NULL != pInputData->p32bitDMIBuffer)
    {
        pCmdBuffer = pInputData->pCmdBuffer;
        pDMIBuffer = pInputData->p32bitDMIBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_VFE_ABF_CFG,
                                              IFEABF34RegLength1DWord,
                                              reinterpret_cast<UINT32*>(&m_regCmd1));
        CAMX_ASSERT(CamxResultSuccess == result);

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_VFE_ABF_GR_CFG,
                                              IFEABF34RegLength2DWord,
                                              reinterpret_cast<UINT32*>(&m_regCmd2));
        CAMX_ASSERT(CamxResultSuccess == result);

        result = PacketBuilder::WriteDMI(pCmdBuffer,
                                         regIFE_IFE_0_VFE_DMI_CFG,
                                         static_cast<UINT8>(bankSelect),
                                         pDMIBuffer,
                                         offset,
                                         lengthInByte);
        CAMX_ASSERT(CamxResultSuccess == result);

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "abfBankSelect: %d, LUT_BANK_SEL: %d",
                        bankSelect, m_regCmd1.configReg.bitfields.LUT_BANK_SEL);
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Null Input Data");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF34Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF34Titan17x::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEABF34RegCmd1) <= sizeof(pIFETuningMetadata->metadata17x.IFEABFData.ABFConfig1));
        Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFEABFData.ABFConfig1, &m_regCmd1, sizeof(IFEABF34RegCmd1));

        CAMX_STATIC_ASSERT(sizeof(IFEABF34RegCmd2) <= sizeof(pIFETuningMetadata->metadata17x.IFEABFData.ABFConfig2));
        Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFEABFData.ABFConfig2, &m_regCmd2, sizeof(IFEABF34RegCmd2));

        if (NULL != m_pDMIData)
        {
            Utils::Memcpy(&(pIFETuningMetadata->metadata17x.IFEDMIPacked.ABFLUT.noiseLUT),
                          m_pDMIData,
                          DMIRAM_ABF34_NOISESTD_LENGTH * sizeof(UINT32));
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "IFE Tuning metadata is Null");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF34Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF34Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result        = CamxResultSuccess;
    ABF34UnpackedField* pData         = static_cast<ABF34UnpackedField*>(pInput);
    ABF34OutputData*    pOutputData   = static_cast<ABF34OutputData*>(pOutput);

    if ((NULL != pOutputData) && (NULL != pData))
    {
        m_regCmd1.configReg.bitfields.CROSS_PLANE_EN          = pData->cross_process_en;
        m_regCmd1.configReg.bitfields.DISTANCE_GRGB_0         = pData->distance_ker[0][0];
        m_regCmd1.configReg.bitfields.DISTANCE_GRGB_1         = pData->distance_ker[0][1];
        m_regCmd1.configReg.bitfields.DISTANCE_GRGB_2         = pData->distance_ker[0][2];
        m_regCmd1.configReg.bitfields.DISTANCE_RB_0           = pData->distance_ker[1][0];
        m_regCmd1.configReg.bitfields.DISTANCE_RB_1           = pData->distance_ker[1][1];
        m_regCmd1.configReg.bitfields.DISTANCE_RB_2           = pData->distance_ker[1][2];
        m_regCmd1.configReg.bitfields.FILTER_EN               = pData->filter_en;
        m_regCmd1.configReg.bitfields.LUT_BANK_SEL            = pData->lut_bank_sel;
        m_regCmd1.configReg.bitfields.PIXEL_MATCH_LEVEL_GRGB  = pData->blk_pix_matching_g;
        m_regCmd1.configReg.bitfields.PIXEL_MATCH_LEVEL_RB    = pData->blk_pix_matching_rb;
        m_regCmd1.configReg.bitfields.SINGLE_BPC_EN           = pData->single_bpc_en;
        m_regCmd2.configGRChannel.bitfields.CURVE_OFFSET      = pData->curve_offset[0];
        m_regCmd2.configGBChannel.bitfields.CURVE_OFFSET      = pData->curve_offset[1];
        m_regCmd2.configRChannel.bitfields.CURVE_OFFSET       = pData->curve_offset[2];
        m_regCmd2.configBChannel.bitfields.CURVE_OFFSET       = pData->curve_offset[3];
        m_regCmd2.configRNR0.bitfields.BX                     = pData->bx;
        m_regCmd2.configRNR0.bitfields.BY                     = pData->by;
        m_regCmd2.configRNR1.bitfields.INIT_RSQUARE           = pData->r_square_init;
        m_regCmd2.configRNR2.bitfields.ANCHOR_0               = pData->anchor_table[0];
        m_regCmd2.configRNR2.bitfields.ANCHOR_1               = pData->anchor_table[1];
        m_regCmd2.configRNR3.bitfields.ANCHOR_2               = pData->anchor_table[2];
        m_regCmd2.configRNR3.bitfields.ANCHOR_3               = pData->anchor_table[3];
        m_regCmd2.configRNR4.bitfields.COEFF_BASE_0           = pData->base_table[0][0];
        m_regCmd2.configRNR4.bitfields.COEFF_SHIFT_0          = pData->shift_table[0][0];
        m_regCmd2.configRNR4.bitfields.COEFF_SLOPE_0          = pData->slope_table[0][0];
        m_regCmd2.configRNR5.bitfields.COEFF_BASE_1           = pData->base_table[0][1];
        m_regCmd2.configRNR5.bitfields.COEFF_SHIFT_1          = pData->shift_table[0][1];
        m_regCmd2.configRNR5.bitfields.COEFF_SLOPE_1          = pData->slope_table[0][1];
        m_regCmd2.configRNR6.bitfields.COEFF_BASE_2           = pData->base_table[0][2];
        m_regCmd2.configRNR6.bitfields.COEFF_SHIFT_2          = pData->shift_table[0][2];
        m_regCmd2.configRNR6.bitfields.COEFF_SLOPE_2          = pData->slope_table[0][2];
        m_regCmd2.configRNR7.bitfields.COEFF_BASE_3           = pData->base_table[0][3];
        m_regCmd2.configRNR7.bitfields.COEFF_SHIFT_3          = pData->shift_table[0][3];
        m_regCmd2.configRNR7.bitfields.COEFF_SLOPE_3          = pData->slope_table[0][3];
        m_regCmd2.configRNR8.bitfields.THRESH_BASE_0          = pData->base_table[1][0];
        m_regCmd2.configRNR8.bitfields.THRESH_SHIFT_0         = pData->shift_table[1][0];
        m_regCmd2.configRNR8.bitfields.THRESH_SLOPE_0         = pData->slope_table[1][0];
        m_regCmd2.configRNR9.bitfields.THRESH_BASE_1          = pData->base_table[1][1];
        m_regCmd2.configRNR9.bitfields.THRESH_SHIFT_1         = pData->shift_table[1][1];
        m_regCmd2.configRNR9.bitfields.THRESH_SLOPE_1         = pData->slope_table[1][1];
        m_regCmd2.configRNR10.bitfields.THRESH_BASE_2         = pData->base_table[1][2];
        m_regCmd2.configRNR10.bitfields.THRESH_SHIFT_2        = pData->shift_table[1][2];
        m_regCmd2.configRNR10.bitfields.THRESH_SLOPE_2        = pData->slope_table[1][2];
        m_regCmd2.configRNR11.bitfields.THRESH_BASE_3         = pData->base_table[1][3];
        m_regCmd2.configRNR11.bitfields.THRESH_SHIFT_3        = pData->shift_table[1][3];
        m_regCmd2.configRNR11.bitfields.THRESH_SLOPE_3        = pData->slope_table[1][3];
        m_regCmd2.configRNR12.bitfields.RSQUARE_SHIFT         = pData->r_square_shft;
        m_regCmd2.configBPC0.bitfields.FMAX                   = pData->bpc_fmax;
        m_regCmd2.configBPC0.bitfields.FMIN                   = pData->bpc_fmin;
        m_regCmd2.configBPC0.bitfields.OFFSET                 = pData->bpc_offset;
        m_regCmd2.configBPC1.bitfields.BLS                    = pData->bpc_bls;
        m_regCmd2.configBPC1.bitfields.MAX_SHIFT              = pData->bpc_maxshft;
        m_regCmd2.configBPC1.bitfields.MIN_SHIFT              = pData->bpc_minshft;
        m_regCmd2.noisePreserveConfig0.bitfields.ANCHOR_GAP   = pData->noise_prsv_anchor_gap;
        m_regCmd2.noisePreserveConfig0.bitfields.ANCHOR_LO    = pData->noise_prsv_anchor_lo;
        m_regCmd2.noisePreserveConfig1.bitfields.LO_GRGB      = pData->noise_prsv_lo[0];
        m_regCmd2.noisePreserveConfig1.bitfields.SHIFT_GRGB   = pData->noise_prsv_shft[0];
        m_regCmd2.noisePreserveConfig1.bitfields.SLOPE_GRGB   = pData->noise_prsv_slope[0];
        m_regCmd2.noisePreserveConfig2.bitfields.LO_RB        = pData->noise_prsv_lo[0];
        m_regCmd2.noisePreserveConfig2.bitfields.SHIFT_RB     = pData->noise_prsv_shft[0];
        m_regCmd2.noisePreserveConfig2.bitfields.SLOPE_RB     = pData->noise_prsv_slope[0];

        for (UINT32 index = 0; index < DMIRAM_ABF34_NOISESTD_LENGTH; index++)
        {
            pOutputData->pDMIData[index] = pData->noise_std_lut_level0[pData->lut_bank_sel][index];
        }

        // Store DMI address to use it for meta dump
        m_pDMIData = pOutputData->pDMIData;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input params are NULL pData=0x%p pOutputData=0x%p", pData, pOutputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF34Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF34Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF34Titan17x::~IFEABF34Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEABF34Titan17x::~IFEABF34Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF34Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEABF34Titan17x::DumpRegConfig()
{
    /// @brief Local debug dump register structure
    struct DumpInfo
    {
        UINT32  startRegAddr;    ///< Start address of the register of range
        UINT32  numRegs;         ///< The number of registers to be programmed.
        UINT32* pRegRangeAddr;   ///< The pointer to the structure in memory or a single varaible.
    };

    DumpInfo dumpRegInfoArray[] =
    {
        {
            regIFE_IFE_0_VFE_ABF_CFG,
            IFEABF34RegLength1DWord,
            reinterpret_cast<UINT32*>(&m_regCmd1)
        },
        {
            regIFE_IFE_0_VFE_ABF_GR_CFG,
            IFEABF34RegLength2DWord,
            reinterpret_cast<UINT32*>(&m_regCmd2)
        }
    };

    for (UINT i = 0; i < CAMX_ARRAY_SIZE(dumpRegInfoArray); i++)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SECTION[%d]: %08x: %08x",
            i, dumpRegInfoArray[i].startRegAddr,
            dumpRegInfoArray[i].startRegAddr + (dumpRegInfoArray[i].numRegs - 1) * RegisterWidthInBytes);

        for (UINT j = 0; j < dumpRegInfoArray[i].numRegs; j++)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "%08x: %08x",
                dumpRegInfoArray[i].startRegAddr + j * 4, *(dumpRegInfoArray[i].pRegRangeAddr + j));
        }
    }
}

CAMX_NAMESPACE_END
