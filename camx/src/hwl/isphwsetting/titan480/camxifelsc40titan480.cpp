////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifelsc40titan480.cpp
/// @brief CAMXIFELSC40TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifelsc40titan480.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC40Titan480::IFELSC40Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFELSC40Titan480::IFELSC40Titan480()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFELSC40RegLengthDword1) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFELSC40RegLengthDword2) +
                     (PacketBuilder::RequiredWriteDMISizeInDwords() * IFELSC40NumDMITables));
    Set32bitDMILength(IFELSC40DMILengthDword);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC40Titan480::WriteLUTtoDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELSC40Titan480::WriteLUTtoDMI(
    VOID*  pInput,
    UINT32 offset)
{
    CamxResult      result      = CamxResultSuccess;
    ISPInputData*   pInputData  = static_cast<ISPInputData*>(pInput);

    // CDM pack the DMI buffer and patch the Red and Blue LUT DMI buffer into CDM pack
    result = PacketBuilder::WriteDMI(pInputData->pCmdBuffer,
                                     regIFE_IFE_0_PP_CLC_LSC_DMI_CFG,
                                     IFE_IFE_0_PP_CLC_LSC_DMI_LUT_CFG_LUT_SEL_RED_LUT,
                                     pInputData->p32bitDMIBuffer,
                                     offset,
                                     IFELSC40LUTTableSize);
    if (CamxResultSuccess == result)
    {
        offset += IFELSC40LUTTableSize;
        result = PacketBuilder::WriteDMI(pInputData->pCmdBuffer,
                                         regIFE_IFE_0_PP_CLC_LSC_DMI_CFG,
                                         IFE_IFE_0_PP_CLC_LSC_DMI_LUT_CFG_LUT_SEL_BLUE_LUT,
                                         pInputData->p32bitDMIBuffer,
                                         offset,
                                         IFELSC40LUTTableSize);
    }

    if (CamxResultSuccess == result)
    {
        offset += IFELSC40LUTTableSize;
        result = PacketBuilder::WriteDMI(pInputData->pCmdBuffer,
                                         regIFE_IFE_0_PP_CLC_LSC_DMI_CFG,
                                         IFE_IFE_0_PP_CLC_LSC_DMI_LUT_CFG_LUT_SEL_GRID_LUT,
                                         pInputData->p32bitDMIBuffer,
                                         offset,
                                         IFELSC40LUTTableSize);
    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Write DMI LUT failed");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC40Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELSC40Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result     = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer = NULL;
    UINT32        offset     = (*pDMIBufferOffset +
                               (pInputData->pStripeConfig->stripeId * IFELSC40DMILengthDword)) * sizeof(UINT32);

    if ((NULL != pInputData)                  &&
        (NULL != pInputData->pCmdBuffer)      &&
        (NULL != pInputData->p32bitDMIBuffer))
    {
        pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_LSC_DMI_LUT_BANK_CFG,
                                              IFELSC40RegLengthDword1,
                                              reinterpret_cast<UINT32*>(&m_regCmd1));
        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_LSC_LSC_0_CFG,
                                                  IFELSC40RegLengthDword2,
                                                  reinterpret_cast<UINT32*>(&m_regCmd2));
        }

        if (CamxResultSuccess == result)
        {
            result = WriteLUTtoDMI(pInputData, offset);
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to write command buffer");
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
// IFELSC40Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELSC40Titan480::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFELSC40RegCmd1) == sizeof(pIFETuningMetadata->metadata480.IFELSCData.LSCConfig1));
        Utils::Memcpy(&pIFETuningMetadata->metadata480.IFELSCData.LSCConfig1, &m_regCmd1, sizeof(IFELSC40RegCmd1));

        CAMX_STATIC_ASSERT(sizeof(IFELSC40RegCmd2) == sizeof(pIFETuningMetadata->metadata480.IFELSCData.LSCConfig2));
        Utils::Memcpy(&pIFETuningMetadata->metadata480.IFELSCData.LSCConfig2, &m_regCmd2, sizeof(IFELSC40RegCmd2));

        if (NULL != m_pGRRLUTDMIBuffer)
        {
            Utils::Memcpy(&pIFETuningMetadata->metadata480.IFEDMIPacked.LSCMesh.GRRLUT[0],
                          m_pGRRLUTDMIBuffer,
                          IFELSC40LUTTableSize);
        }

        if (NULL != m_pGBBLUTDMIBuffer)
        {
            Utils::Memcpy(&pIFETuningMetadata->metadata480.IFEDMIPacked.LSCMesh.GBBLUT[0],
                          m_pGBBLUTDMIBuffer,
                          IFELSC40LUTTableSize);
        }

        if (NULL != m_pGridLUTDMIBuffer)
        {
            Utils::Memcpy(&pIFETuningMetadata->metadata480.IFEDMIPacked.LSCMesh.gridLUT[0],
                          m_pGridLUTDMIBuffer,
                          IFELSC40LUTTableSize);
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
// IFELSC40Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELSC40Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result              = CamxResultSuccess;
    LSC40UnpackedField* pData               = static_cast<LSC40UnpackedField*>(pInput);
    LSC40OutputData*    pOutputData         = static_cast<LSC40OutputData*>(pOutput);
    UINT32              totalHorizontalMesh = 0;
    UINT32              totalVerticalMesh   = 0;
    UINT32              dmiCount            = 0;
    UINT32              horizontalMeshNum;
    UINT32              verticalMeshNum;

    if ((NULL != pOutputData) && (NULL != pData))
    {
        m_regCmd1.moduleConfig.bitfields.EN                  = pData->enable;
        m_regCmd1.moduleConfig.bitfields.CROP_EN             = pData->crop_enable;
        m_regCmd1.moduleConfig.bitfields.ALSC_EN             = pData->ALSC_enable;
        m_regCmd1.moduleConfig.bitfields.INIT_BLOCK_X        = pData->Lx_start;
        m_regCmd1.moduleConfig.bitfields.INIT_BLOCK_Y        = pData->Ly_start;
        m_regCmd1.moduleConfig.bitfields.INIT_SUBBLOCK_X     = pData->Bx_start;
        m_regCmd1.moduleConfig.bitfields.INIT_SUBBLOCK_Y     = pData->By_start;
        m_regCmd1.moduleConfig.bitfields.NUM_SUBBLOCKS       = pData->intp_factor;
        m_regCmd1.moduleLUTConfig.bitfields.BANK_SEL         = pData->bank_sel;
        m_regCmd1.dmiLUTConfig.bitfields.BANK_SEL            = pData->bank_sel; // Have the same bank selection to module

        m_regCmd2.config0.bitfields.NUM_BLOCKS_X             = pData->num_meshgain_h;
        m_regCmd2.config0.bitfields.NUM_BLOCKS_Y             = pData->num_meshgain_v;
        m_regCmd2.config1.bitfields.BLOCK_HEIGHT             = pData->MeshGridBheight;
        m_regCmd2.config1.bitfields.BLOCK_WIDTH              = pData->MeshGridBwidth;
        m_regCmd2.config2.bitfields.INV_SUBBLOCK_WIDTH       = pData->x_delta;
        m_regCmd2.config2.bitfields.SUBBLOCK_WIDTH           = pData->Bwidth;
        m_regCmd2.config3.bitfields.INV_SUBBLOCK_HEIGHT      = pData->y_delta;
        m_regCmd2.config3.bitfields.SUBBLOCK_HEIGHT          = pData->Bheight;
        m_regCmd2.config4.bitfields.INIT_PIXEL_X             = pData->Bx_d1;
        m_regCmd2.config4.bitfields.INIT_PIXEL_Y             = pData->By_e1;
        m_regCmd2.config5.bitfields.PIXEL_OFFSET             = pData->pixel_offset;
        m_regCmd2.config6.bitfields.INIT_YDELTA              = pData->By_init_e1;
        m_regCmd2.config7.bitfields.LUMA_WEIGHT_BASE_MIN     = pData->luma_weight_base_min;
        m_regCmd2.config7.bitfields.LUMA_WEIGHT_BASE_SCALE   = pData->luma_weight_base_scale;
        m_regCmd2.config8.bitfields.LUMA_WEIGHT_MIN          = pData->luma_weight_min;
        m_regCmd2.config9.bitfields.FIRST_PIXEL              = pData->first_pixel;
        m_regCmd2.config9.bitfields.LAST_PIXEL               = pData->last_pixel;
        m_regCmd2.config10.bitfields.FIRST_LINE              = pData->first_line;
        m_regCmd2.config10.bitfields.LAST_LINE               = pData->last_line;

        // (NUM_MESHGAIN_H+2) * (NUM_MESHGAIN_V+2) LUT entries for a frame.
        totalHorizontalMesh = pData->num_meshgain_h + 2;
        totalVerticalMesh   = pData->num_meshgain_v + 2;

        for (verticalMeshNum = 0; verticalMeshNum < totalVerticalMesh; verticalMeshNum++)
        {
            for (horizontalMeshNum = 0; horizontalMeshNum < totalHorizontalMesh; horizontalMeshNum++)
            {
                pOutputData->pGRRLUTDMIBuffer[dmiCount] =
                    ((pData->mesh_table[pData->bank_sel][0][verticalMeshNum][horizontalMeshNum] & Utils::AllOnes32(13)) |
                    ((pData->mesh_table[pData->bank_sel][1][verticalMeshNum][horizontalMeshNum] & Utils::AllOnes32(13)) << 13));

                pOutputData->pGBBLUTDMIBuffer[dmiCount] =
                    ((pData->mesh_table[pData->bank_sel][3][verticalMeshNum][horizontalMeshNum] & Utils::AllOnes32(13)) |
                    ((pData->mesh_table[pData->bank_sel][2][verticalMeshNum][horizontalMeshNum] & Utils::AllOnes32(13)) << 13));

                pOutputData->pGridLUTDMIBuffer[dmiCount] =
                    ((pData->grids_gain[pData->bank_sel][verticalMeshNum][horizontalMeshNum] & Utils::AllOnes32(12)) |
                    (((pData->grids_mean[pData->bank_sel][verticalMeshNum][horizontalMeshNum]) & Utils::AllOnes32(14)) << 12));

                dmiCount++;
            }
        }

        // Store DMI Buffer address
        m_pGRRLUTDMIBuffer  = pOutputData->pGRRLUTDMIBuffer;
        m_pGBBLUTDMIBuffer  = pOutputData->pGBBLUTDMIBuffer;
        m_pGridLUTDMIBuffer = pOutputData->pGridLUTDMIBuffer;

    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Data is NULL %p %p", pOutputData, pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC40Titan480::CreateSubCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELSC40Titan480::CreateSubCmdList(
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
            regIFE_IFE_0_PP_CLC_LSC_MODULE_CFG,
            sizeof(m_regCmd1.moduleConfig) / RegisterWidthInBytes,
            reinterpret_cast<UINT32*>(&m_regCmd1.moduleConfig));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC40Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELSC40Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    BOOL* pModuleEnable = static_cast<BOOL*>(pInput);

    if (NULL != pInput)
    {
        m_regCmd1.moduleConfig.bitfields.EN = *pModuleEnable;
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC40Titan480::~IFELSC40Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFELSC40Titan480::~IFELSC40Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC40Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFELSC40Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LSC40 Config 0  = %x\n", m_regCmd2.config0);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LSC40 Config 1  = %x\n", m_regCmd2.config1);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LSC40 Config 2  = %x\n", m_regCmd2.config2);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LSC40 Config 3  = %x\n", m_regCmd2.config3);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LSC40 Config 4  = %x\n", m_regCmd2.config4);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LSC40 Config 5  = %x\n", m_regCmd2.config5);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LSC40 Config 6  = %x\n", m_regCmd2.config6);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LSC40 Config 7  = %x\n", m_regCmd2.config7);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LSC40 Config 8  = %x\n", m_regCmd2.config8);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LSC40 Config 9  = %x\n", m_regCmd2.config9);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LSC40 Config 10 = %x\n", m_regCmd2.config10);
}

CAMX_NAMESPACE_END
