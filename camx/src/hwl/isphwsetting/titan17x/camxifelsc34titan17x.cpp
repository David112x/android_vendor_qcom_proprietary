////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifelsc34titan17x.cpp
/// @brief CAMXIFELSC34TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifelsc34titan17x.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC34Titan17x::IFELSC34Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFELSC34Titan17x::IFELSC34Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFELSC34RegLengthDword));
    Set32bitDMILength(IFELSC34DMILengthDword);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC34Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELSC34Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result                = CamxResultSuccess;
    ISPInputData* pInputData            = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer            = NULL;
    CmdBuffer*    pDMIBuffer            = NULL;
    UINT32        offset                = (*pDMIBufferOffset +
                                          (pInputData->pStripeConfig->stripeId * IFELSC34DMILengthDword)) * sizeof(UINT32);
    UINT32        lengthInByte          = IFELSC34DMISetSizeDword * sizeof(UINT32);
    UINT8         lRolloffGRRBankSelect = RolloffLGRRBank0;
    UINT8         lRolloffGBBBankSelect = RolloffLGBBBank0;

    if (NULL != pInputData                  &&
        NULL != pInputData->pCmdBuffer      &&
        NULL != pInputData->p32bitDMIBuffer)
    {
        pCmdBuffer = pInputData->pCmdBuffer;
        pDMIBuffer = pInputData->p32bitDMIBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_VFE_ROLLOFF_CFG,
                                              IFELSC34RegLengthDword,
                                              reinterpret_cast<UINT32*>(&m_regCmd));
        CAMX_ASSERT(CamxResultSuccess == result);

        lRolloffGRRBankSelect =
                    (m_regCmd.configRegister.bitfields.PCA_LUT_BANK_SEL == 0) ? RolloffLGRRBank0 : RolloffLGRRBank1;
        lRolloffGBBBankSelect =
                    (m_regCmd.configRegister.bitfields.PCA_LUT_BANK_SEL == 0) ? RolloffLGBBBank0 : RolloffLGBBBank1;

        result = PacketBuilder::WriteDMI(pCmdBuffer,
                                         regIFE_IFE_0_VFE_DMI_CFG,
                                         lRolloffGRRBankSelect,
                                         pDMIBuffer,
                                         offset,
                                         lengthInByte);
        CAMX_ASSERT(CamxResultSuccess == result);

        offset += lengthInByte;
        result = PacketBuilder::WriteDMI(pCmdBuffer,
                                         regIFE_IFE_0_VFE_DMI_CFG,
                                         lRolloffGBBBankSelect,
                                         pDMIBuffer,
                                         offset,
                                         lengthInByte);
        CAMX_ASSERT(CamxResultSuccess == result);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC34Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELSC34Titan17x::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFELSC34RegCmd) == sizeof(pIFETuningMetadata->metadata17x.IFELSCData.LSCConfig1));
        Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFELSCData.LSCConfig1, &m_regCmd, sizeof(IFELSC34RegCmd));

        if (NULL != m_pGRRLUTDMIBuffer)
        {
            Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFEDMIPacked.LSCMesh.GRRLUT[0],
                          m_pGRRLUTDMIBuffer,
                          IFELSC34LUTTableSize);
        }

        if (NULL != m_pGBBLUTDMIBuffer)
        {
            Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFEDMIPacked.LSCMesh.GBBLUT[0],
                          m_pGBBLUTDMIBuffer,
                          IFELSC34LUTTableSize);
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
// IFELSC34Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELSC34Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result        = CamxResultSuccess;
    LSC34UnpackedField* pData         = static_cast<LSC34UnpackedField*>(pInput);
    LSC34OutputData*    pOutputData   = static_cast<LSC34OutputData*>(pOutput);
    UINT32              totalHorMesh  = 0;
    UINT32              toatlVerMesh  = 0;
    UINT32              dmiCount      = 0;
    UINT32              horMeshNum;
    UINT32              verMeshNum;

    if ((NULL != pOutputData) && (NULL != pData))
    {
        m_regCmd.configRegister.bitfields.NUM_MESHGAIN_H               = pData->num_meshgain_h;
        m_regCmd.configRegister.bitfields.NUM_MESHGAIN_V               = pData->num_meshgain_v;
        m_regCmd.configRegister.bitfields.PCA_LUT_BANK_SEL             = pData->bank_sel;
        m_regCmd.configRegister.bitfields.PIXEL_OFFSET                 = pData->pixel_offset;

        m_regCmd.gridConfigRegister0.bitfields.BLOCK_HEIGHT            = pData->meshGridBheight_l;
        m_regCmd.gridConfigRegister0.bitfields.BLOCK_WIDTH             = pData->meshGridBwidth_l;

        m_regCmd.gridConfigRegister1.bitfields.INTERP_FACTOR           = pData->intp_factor_l;
        m_regCmd.gridConfigRegister1.bitfields.SUB_GRID_HEIGHT         = pData->bheight_l;
        m_regCmd.gridConfigRegister1.bitfields.SUB_GRID_Y_DELTA        = pData->y_delta_l;

        m_regCmd.gridConfigRegister2.bitfields.SUB_GRID_WIDTH          = pData->bwidth_l;
        m_regCmd.gridConfigRegister2.bitfields.SUB_GRID_X_DELTA        = pData->x_delta_l;

        m_regCmd.rightGridConfigRegister0.bitfields.BLOCK_HEIGHT       = pData->meshGridBheight_r;
        m_regCmd.rightGridConfigRegister0.bitfields.BLOCK_WIDTH        = pData->meshGridBwidth_r;

        m_regCmd.rightGridConfigRegister1.bitfields.INTERP_FACTOR      = pData->intp_factor_r;
        m_regCmd.rightGridConfigRegister1.bitfields.SUB_GRID_HEIGHT    = pData->bheight_r;
        m_regCmd.rightGridConfigRegister1.bitfields.SUB_GRID_Y_DELTA   = pData->y_delta_r;

        m_regCmd.rightGridConfigRegister2.bitfields.SUB_GRID_WIDTH     = pData->bwidth_r;
        m_regCmd.rightGridConfigRegister2.bitfields.SUB_GRID_X_DELTA   = pData->x_delta_r;

        m_regCmd.rightStripeConfigRegister0.bitfields.BLOCK_X_INDEX    = pData->lx_start_r;
        m_regCmd.rightStripeConfigRegister0.bitfields.BLOCK_Y_INDEX    = pData->ly_start_r;
        m_regCmd.rightStripeConfigRegister0.bitfields.Y_DELTA_ACCUM    = pData->y_delta_r * pData->by_e1_r;

        m_regCmd.rightStripeConfigRegister1.bitfields.PIXEL_X_INDEX    = pData->bx_d1_r;
        m_regCmd.rightStripeConfigRegister1.bitfields.PIXEL_Y_INDEX    = pData->by_e1_r;
        m_regCmd.rightStripeConfigRegister1.bitfields.SUB_GRID_X_INDEX = pData->bx_start_r;
        m_regCmd.rightStripeConfigRegister1.bitfields.SUB_GRID_Y_INDEX = pData->by_start_r;

        m_regCmd.stripeConfigRegister0.bitfields.BLOCK_X_INDEX         = pData->lx_start_l;
        m_regCmd.stripeConfigRegister0.bitfields.BLOCK_Y_INDEX         = pData->ly_start_l;
        m_regCmd.stripeConfigRegister0.bitfields.Y_DELTA_ACCUM         = pData->y_delta_l * pData->by_e1_l;

        m_regCmd.stripeConfigRegister1.bitfields.PIXEL_X_INDEX         = pData->bx_d1_l;
        m_regCmd.stripeConfigRegister1.bitfields.PIXEL_Y_INDEX         = pData->by_e1_l;
        m_regCmd.stripeConfigRegister1.bitfields.SUB_GRID_X_INDEX      = pData->bx_start_l;
        m_regCmd.stripeConfigRegister1.bitfields.SUB_GRID_Y_INDEX      = pData->by_start_l;

        // (NUM_MESHGAIN_H+2) * (NUM_MESHGAIN_V+2) LUT entries for a frame.
        totalHorMesh = pData->num_meshgain_h + 2;
        toatlVerMesh = pData->num_meshgain_v + 2;

        for (verMeshNum = 0; verMeshNum < toatlVerMesh; verMeshNum++)
        {
            for (horMeshNum = 0; horMeshNum < totalHorMesh; horMeshNum++)
            {
                pOutputData->pGRRLUTDMIBuffer[dmiCount] =
                    ((pData->mesh_table_l[pData->bank_sel][0][verMeshNum][horMeshNum] & Utils::AllOnes32(13)) |
                    ((pData->mesh_table_l[pData->bank_sel][1][verMeshNum][horMeshNum]) << 13));

                pOutputData->pGBBLUTDMIBuffer[dmiCount] =
                    ((pData->mesh_table_l[pData->bank_sel][3][verMeshNum][horMeshNum] & Utils::AllOnes32(13)) |
                    ((pData->mesh_table_l[pData->bank_sel][2][verMeshNum][horMeshNum]) << 13));
                dmiCount++;
            }
        }

        // Store DMI buffer pointer
        m_pGRRLUTDMIBuffer = pOutputData->pGRRLUTDMIBuffer;
        m_pGBBLUTDMIBuffer = pOutputData->pGBBLUTDMIBuffer;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input params are NULL pData=0x%p pOutputData=0x%p", pData, pOutputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC34Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELSC34Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC34Titan17x::~IFELSC34Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFELSC34Titan17x::~IFELSC34Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC34Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFELSC34Titan17x::DumpRegConfig()
{
}

CAMX_NAMESPACE_END
