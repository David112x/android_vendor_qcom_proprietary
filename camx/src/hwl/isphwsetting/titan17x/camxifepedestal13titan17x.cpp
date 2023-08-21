////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifepedestal13titan17x.cpp
/// @brief CAMXIFEPEDESTAL13TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifepedestal13titan17x.h"
#include "pedestal13setting.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPedestal13Titan17x::IFEPedestal13Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEPedestal13Titan17x::IFEPedestal13Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEPedestal13RegLengthDword) +
                     PacketBuilder::RequiredWriteDMISizeInDwords() * IFEPedestal13NumDMITables);
    Set32bitDMILength(IFEPedestal13DMILengthDword);

    m_leftGRRBankSelect = PedestalLGRRBank0; ///< By Default, Select BANK 0
    m_leftGBBBankSelect = PedestalLGBBBank0; ///< By Default, Select BANK 0
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPedestal13Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPedestal13Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{

    CamxResult result        = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer* pCmdBuffer    = pInputData->pCmdBuffer;
    UINT32     offset        = (*pDMIBufferOffset +
                               (pInputData->pStripeConfig->stripeId * IFEPedestal13DMILengthDword)) * sizeof(UINT32);
    CmdBuffer* pDMIBuffer    = pInputData->p32bitDMIBuffer;
    UINT32     lengthInByte  = IFEPedestal13DMISetSizeDword * sizeof(UINT32);

    if (TRUE == pInputData->bankUpdate.isValid)
    {
        m_leftGRRBankSelect = (pInputData->bankUpdate.pedestalBank == 0) ? PedestalLGRRBank0 : PedestalLGRRBank1;
        m_leftGBBBankSelect = (pInputData->bankUpdate.pedestalBank == 0) ? PedestalLGBBBank0 : PedestalLGBBBank1;
    }

    CAMX_ASSERT(NULL != pCmdBuffer);
    CAMX_ASSERT(NULL != pDMIBuffer);

    result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                          regIFE_IFE_0_VFE_PEDESTAL_CFG,
                                          IFEPedestal13RegLengthDword,
                                          reinterpret_cast<UINT32*>(&m_regCmd));

    CAMX_ASSERT(CamxResultSuccess == result);

    result = PacketBuilder::WriteDMI(pCmdBuffer,
                                     regIFE_IFE_0_VFE_DMI_CFG,
                                     m_leftGRRBankSelect,
                                     pDMIBuffer,
                                     offset,
                                     lengthInByte);

    CAMX_ASSERT(CamxResultSuccess == result);

    offset   += lengthInByte;
    result = PacketBuilder::WriteDMI(pCmdBuffer,
                                     regIFE_IFE_0_VFE_DMI_CFG,
                                     m_leftGBBBankSelect ,
                                     pDMIBuffer,
                                     offset,
                                     lengthInByte);
    CAMX_ASSERT(CamxResultSuccess == result);

    m_leftGRRBankSelect = (m_leftGRRBankSelect == PedestalLGRRBank0) ?  PedestalLGRRBank1 : PedestalLGRRBank0;
    m_leftGBBBankSelect = (m_leftGBBBankSelect == PedestalLGBBBank0) ?  PedestalLGBBBank1 : PedestalLGBBBank0;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPedestal13Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPedestal13Titan17x::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEPedestal13RegCmd) <=
            sizeof(pIFETuningMetadata->metadata17x.IFEPedestalData.pedestalConfig));
        Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFEPedestalData.pedestalConfig,
                      &m_regCmd,
                      sizeof(IFEPedestal13RegCmd));

        if (NULL != m_pGRRLUTDMIBuffer)
        {
            Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFEDMIPacked.pedestalLUT.GRRLUT[0],
                          m_pGRRLUTDMIBuffer,
                          IFEPedestal13LUTTableSize);
        }

        if (NULL != m_pGBBLUTDMIBuffer)
        {
            Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFEDMIPacked.pedestalLUT.GBBLUT[0],
                          m_pGBBLUTDMIBuffer,
                          IFEPedestal13LUTTableSize);
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
// IFEPedestal13Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPedestal13Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult                    result        = CamxResultSuccess;
    Pedestal13UnpackedField*      pData         = static_cast<Pedestal13UnpackedField*>(pInput);
    Pedestal13OutputData*         pOutputData   = static_cast<Pedestal13OutputData*>(pOutput);
    UINT32                        DMILength     = 0;
    UINT32 DMICnt = 0;

    if ((NULL != pData) && (NULL != pOutputData))
    {
        m_regCmd.config.bitfields.HDR_EN                       = pData->HDRen;
        m_regCmd.config.bitfields.LUT_BANK_SEL                 = pData->bankSel;
        m_regCmd.config.bitfields.SCALE_BYPASS                 = pData->scaleBypass;
        m_regCmd.config0.bitfields.BLOCK_HEIGHT                = pData->meshGridbHeightL;
        m_regCmd.config0.bitfields.BLOCK_WIDTH                 = pData->meshGridbWidthL;
        m_regCmd.config1.bitfields.INTERP_FACTOR               = pData->intpFactorL;
        m_regCmd.config1.bitfields.SUB_GRID_HEIGHT             = pData->bHeightL;
        m_regCmd.config1.bitfields.SUB_GRID_Y_DELTA            = pData->yDeltaL;
        m_regCmd.config2.bitfields.SUB_GRID_WIDTH              = pData->bWidthL;
        m_regCmd.config2.bitfields.SUB_GRID_X_DELTA            = pData->xDeltaL;
        m_regCmd.rightGridConfig0.bitfields.BLOCK_HEIGHT       = pData->meshGridbHeightR;
        m_regCmd.rightGridConfig0.bitfields.BLOCK_WIDTH        = pData->meshGridbWidthL;
        m_regCmd.rightGridConfig1.bitfields.INTERP_FACTOR      = pData->intpFactorL;
        m_regCmd.rightGridConfig1.bitfields.SUB_GRID_HEIGHT    = pData->bHeightL;
        m_regCmd.rightGridConfig1.bitfields.SUB_GRID_Y_DELTA   = pData->yDeltaL;
        m_regCmd.rightGridConfig2.bitfields.SUB_GRID_WIDTH     = pData->bWidthL;
        m_regCmd.rightGridConfig2.bitfields.SUB_GRID_X_DELTA   = pData->xDeltaL;
        m_regCmd.rightStripeConfig0.bitfields.BLOCK_X_INDEX    = pData->lxStartL;
        m_regCmd.rightStripeConfig0.bitfields.BLOCK_Y_INDEX    = pData->lyStartL;
        m_regCmd.rightStripeConfig1.bitfields.PIXEL_X_INDEX    = pData->bxD1L;
        m_regCmd.rightStripeConfig1.bitfields.PIXEL_Y_INDEX    = pData->byE1L;
        m_regCmd.rightStripeConfig1.bitfields.SUB_GRID_X_INDEX = pData->bxStartL;
        m_regCmd.rightStripeConfig1.bitfields.SUB_GRID_Y_INDEX = pData->byStartL;
        m_regCmd.stripeConfig0.bitfields.BLOCK_X_INDEX         = pData->lxStartL;
        m_regCmd.stripeConfig0.bitfields.BLOCK_Y_INDEX         = pData->lyStartL;
        m_regCmd.stripeConfig0.bitfields.Y_DELTA_ACCUM         = (pData->yDeltaL * pData->byE1L);
        m_regCmd.stripeConfig1.bitfields.PIXEL_X_INDEX         = pData->bxD1L;
        m_regCmd.stripeConfig1.bitfields.PIXEL_Y_INDEX         = pData->byE1L;
        m_regCmd.stripeConfig1.bitfields.SUB_GRID_X_INDEX      = pData->bxStartL;
        m_regCmd.stripeConfig1.bitfields.SUB_GRID_Y_INDEX      = pData->byStartL;

        for (UINT32 verMeshNum = 0; verMeshNum < PED_MESH_PT_V_V13; verMeshNum++)
        {
            for (UINT32 horMeshNum = 0; horMeshNum < PED_MESH_PT_H_V13; horMeshNum++)
            {
                // 0->R, 1->Gr, 2->Gb, 3->B
                pOutputData->pGRRLUTDMIBuffer[DMILength] =
                    ((pData->meshTblT1L[pData->bankSel][0][verMeshNum][horMeshNum] & Utils::AllOnes32(12)) |
                    ((pData->meshTblT1L[pData->bankSel][1][verMeshNum][horMeshNum] & Utils::AllOnes32(12)) << 12));
                pOutputData->pGBBLUTDMIBuffer[DMILength] =
                    ((pData->meshTblT1L[pData->bankSel][3][verMeshNum][horMeshNum] & Utils::AllOnes32(12)) |
                    ((pData->meshTblT1L[pData->bankSel][2][verMeshNum][horMeshNum] & Utils::AllOnes32(12)) << 12));
                DMILength++;
            }
        }

        // Store DMI buffer address
        m_pGRRLUTDMIBuffer = pOutputData->pGRRLUTDMIBuffer;
        m_pGBBLUTDMIBuffer = pOutputData->pGBBLUTDMIBuffer;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "data is pData %p, pOutputData %p", pData, pOutputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPedestal13Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPedestal13Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPedestal13Titan17x::~IFEPedestal13Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEPedestal13Titan17x::~IFEPedestal13Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPedestal13Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEPedestal13Titan17x::DumpRegConfig()
{
    // Print regster config
}

CAMX_NAMESPACE_END
