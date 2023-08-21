////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifepedestal13titan480.cpp
/// @brief CAMXIFEPEDESTAL13TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifepedestal13titan480.h"
#include "pedestal13setting.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPedestal13Titan480::IFEPedestal13Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEPedestal13Titan480::IFEPedestal13Titan480()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEPedestal13Titan480RegLengthDword1) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEPedestal13Titan480RegLengthDword2) +
                     PacketBuilder::RequiredWriteDMISizeInDwords() * IFEPedestal13Titan480NumDMITables);
    Set32bitDMILength(IFEPedestal13Titan480DMILengthDword);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPedestal13Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPedestal13Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult      result          = CamxResultSuccess;
    ISPInputData*   pInputData      = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*      pCmdBuffer      = pInputData->pCmdBuffer;
    UINT32          offset          = (*pDMIBufferOffset +
                                      (pInputData->pStripeConfig->stripeId * IFEPedestal13Titan480DMILengthDword)) *
                                      sizeof(UINT32);
    CmdBuffer*      pDMIBuffer      = pInputData->p32bitDMIBuffer;
    UINT32          lengthInByte    = IFEPedestal13Titan480DMISetSizeDword * sizeof(UINT32);

    if ((NULL != pCmdBuffer) && (NULL != pDMIBuffer))
    {
        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_PEDESTAL_MODULE_LUT_BANK_CFG,
                                              IFEPedestal13Titan480RegLengthDword1,
                                              reinterpret_cast<UINT32*>(&m_regCmd1));

        if (CamxResultSuccess == result)
        {

            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_PEDESTAL_MODULE_1_CFG,
                                                  IFEPedestal13Titan480RegLengthDword2,
                                                  reinterpret_cast<UINT32*>(&m_regCmd2));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteDMI(pCmdBuffer,
                                             regIFE_IFE_0_PP_CLC_PEDESTAL_DMI_CFG,
                                             IFE_IFE_0_PP_CLC_PEDESTAL_DMI_LUT_CFG_LUT_SEL_RED_LUT,
                                             pDMIBuffer,
                                             offset,
                                             lengthInByte);
        }

        if (CamxResultSuccess == result)
        {
            offset += lengthInByte;
            result = PacketBuilder::WriteDMI(pCmdBuffer,
                                             regIFE_IFE_0_PP_CLC_PEDESTAL_DMI_CFG,
                                             IFE_IFE_0_PP_CLC_PEDESTAL_DMI_LUT_CFG_LUT_SEL_BLUE_LUT,
                                             pDMIBuffer,
                                             offset,
                                             lengthInByte);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPedestal13Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPedestal13Titan480::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEPedestal13Titan480RegCmd1) <=
                           sizeof(pIFETuningMetadata->metadata480.IFEPedestalData.pedestalConfig1));
        Utils::Memcpy(&pIFETuningMetadata->metadata480.IFEPedestalData.pedestalConfig1,
                      &m_regCmd1,
                      sizeof(IFEPedestal13Titan480RegCmd1));

        CAMX_STATIC_ASSERT(sizeof(IFEPedestal13Titan480RegCmd2) <=
                           sizeof(pIFETuningMetadata->metadata480.IFEPedestalData.pedestalConfig2));
        Utils::Memcpy(&pIFETuningMetadata->metadata480.IFEPedestalData.pedestalConfig2,
                      &m_regCmd2,
                      sizeof(IFEPedestal13Titan480RegCmd2));

        if (NULL != m_pGRRLUTDMIBuffer)
        {
            Utils::Memcpy(&pIFETuningMetadata->metadata480.IFEDMIPacked.pedestalLUT.GRRLUT[0],
                          m_pGRRLUTDMIBuffer,
                          IFEPedestal13Titan480LUTTableSize);
        }

        if (NULL != m_pGBBLUTDMIBuffer)
        {
            Utils::Memcpy(&pIFETuningMetadata->metadata480.IFEDMIPacked.pedestalLUT.GBBLUT[0],
                          m_pGBBLUTDMIBuffer,
                          IFEPedestal13Titan480LUTTableSize);
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
// IFEPedestal13Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPedestal13Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult               result      = CamxResultSuccess;
    Pedestal13UnpackedField* pData       = static_cast<Pedestal13UnpackedField*>(pInput);
    Pedestal13OutputData*    pOutputData = static_cast<Pedestal13OutputData*>(pOutput);
    UINT32                   DMILength   = 0;
    UINT32 DMICnt = 0;

    if ((NULL != pData) && (NULL != pOutputData))
    {
        m_regCmd1.config.bitfields.EN                    = pData->enable;
        m_regCmd1.config.bitfields.SCALE_BYPASS          = pData->scaleBypass;
        m_regCmd1.config.bitfields.NUM_SUBBLOCKS         = pData->intpFactorL;
        m_regCmd1.config.bitfields.INIT_BLOCK_X          = pData->lxStartL;
        m_regCmd1.config.bitfields.INIT_BLOCK_Y          = pData->lyStartL;
        m_regCmd1.config.bitfields.INIT_SUBBLOCK_X       = pData->bxStartL;
        m_regCmd1.config.bitfields.INIT_SUBBLOCK_Y       = pData->byStartL;
        m_regCmd1.lutBankConfig0.bitfields.BANK_SEL      = pData->bankSel;
        m_regCmd2.config1.bitfields.BLOCK_HEIGHT         = pData->meshGridbHeightL;
        m_regCmd2.config1.bitfields.BLOCK_WIDTH          = pData->meshGridbWidthL;
        m_regCmd2.config2.bitfields.SUBBLOCK_WIDTH       = pData->bWidthL;
        m_regCmd2.config2.bitfields.INV_SUBBLOCK_WIDTH   = pData->xDeltaL;
        m_regCmd2.config3.bitfields.SUBBLOCK_HEIGHT      = pData->bHeightL;
        m_regCmd2.config3.bitfields.INV_SUBBLOCK_HEIGHT  = pData->yDeltaL;
        m_regCmd2.config4.bitfields.INIT_PIXEL_X         = pData->bxD1L;
        m_regCmd2.config4.bitfields.INIT_PIXEL_Y         = pData->byE1L;
        m_regCmd2.config5.bitfields.INIT_YDELTA          = (pData->yDeltaL * pData->byE1L);

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
// IFEPedestal13Titan480::CreateSubCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPedestal13Titan480::CreateSubCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if (NULL != pInputData &&
        NULL != pInputData->pCmdBuffer)
    {
        CmdBuffer* pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
            regIFE_IFE_0_PP_CLC_PEDESTAL_MODULE_CFG,
            sizeof(m_regCmd1.config) / RegisterWidthInBytes,
            reinterpret_cast<UINT32*>(&m_regCmd1.config));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPedestal13Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPedestal13Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    BOOL* pModuleEnable = static_cast<BOOL*>(pInput);

    if (NULL != pInput)
    {
        m_regCmd1.config.bitfields.EN = *pModuleEnable;
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPedestal13Titan480::~IFEPedestal13Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEPedestal13Titan480::~IFEPedestal13Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPedestal13Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEPedestal13Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pedestal13 Config SCALE_BYPASS         0x%x",
        m_regCmd1.config.bitfields.SCALE_BYPASS);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pedestal13 Config NUM_SUBBLOCKS        0x%x",
        m_regCmd1.config.bitfields.NUM_SUBBLOCKS);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pedestal13 Config INIT_BLOCK_X         0x%x",
        m_regCmd1.config.bitfields.INIT_BLOCK_X);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pedestal13 Config INIT_BLOCK_Y         0x%x",
        m_regCmd1.config.bitfields.INIT_BLOCK_Y);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pedestal13 Config INIT_SUBBLOCK_X      0x%x",
        m_regCmd1.config.bitfields.INIT_SUBBLOCK_X);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pedestal13 Config INIT_SUBBLOCK_Y      0x%x",
        m_regCmd1.config.bitfields.INIT_SUBBLOCK_Y);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pedestal13 LUT BANK_SEL                0x%x",
        m_regCmd1.lutBankConfig0.bitfields.BANK_SEL);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pedestal13 Config1 BLOCK_HEIGHT        0x%x",
        m_regCmd2.config1.bitfields.BLOCK_HEIGHT);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pedestal13 Config1 BLOCK_WIDTH         0x%x",
        m_regCmd2.config1.bitfields.BLOCK_WIDTH);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pedestal13 Config2 SUBBLOCK_WIDTH      0x%x",
        m_regCmd2.config2.bitfields.SUBBLOCK_WIDTH);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pedestal13 Config2 INV_SUBBLOCK_WIDTH  0x%x",
        m_regCmd2.config2.bitfields.INV_SUBBLOCK_WIDTH);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pedestal13 Config3 SUBBLOCK_HEIGHT     0x%x",
        m_regCmd2.config3.bitfields.SUBBLOCK_HEIGHT);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pedestal13 Config3 INV_SUBBLOCK_HEIGHT 0x%x",
        m_regCmd2.config3.bitfields.INV_SUBBLOCK_HEIGHT);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pedestal13 Config4 INIT_PIXEL_X        0x%x",
        m_regCmd2.config4.bitfields.INIT_PIXEL_X);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pedestal13 Config4 INIT_PIXEL_Y        0x%x",
        m_regCmd2.config4.bitfields.INIT_PIXEL_Y);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pedestal13 Config5 INIT_YDELTA         0x%x",
        m_regCmd2.config5.bitfields.INIT_YDELTA);
}

CAMX_NAMESPACE_END
