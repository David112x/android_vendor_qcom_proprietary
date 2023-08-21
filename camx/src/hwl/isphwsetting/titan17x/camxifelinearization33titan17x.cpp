////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifelinearization33titan17x.cpp
/// @brief CAMXIFELINEARIZATION37TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifelinearization33titan17x.h"
#include "ifelinearization33setting.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization33Titan17x::IFELinearization33Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFELinearization33Titan17x::IFELinearization33Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFELinearization33RegLengthDWord) +
                     PacketBuilder::RequiredWriteDMISizeInDwords());
    Set64bitDMILength(IFELinearization33DMILengthDWord);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization33Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELinearization33Titan17x::CreateCmdList(
    VOID* pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;
    CmdBuffer*    pDMIBuffer    = NULL;
    UINT32        offset        = *pDMIBufferOffset * sizeof(UINT32);
    UINT32        lengthInByte  = IFELinearization33DMILengthDWord * sizeof(UINT32);
    UINT8         bankSelect    = 0;

    if (NULL != pInputData                  &&
        NULL != pInputData->pCmdBuffer      &&
        NULL != pInputData->p64bitDMIBuffer &&
        NULL != pInputData->p64bitDMIBufferAddr)
    {
        pCmdBuffer = pInputData->pCmdBuffer;
        pDMIBuffer = pInputData->p64bitDMIBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_VFE_BLACK_CFG,
                                              IFELinearization33RegLengthDWord,
                                              reinterpret_cast<UINT32*>(&m_regCmd));
        CAMX_ASSERT(CamxResultSuccess == result);

        bankSelect = (m_regCmd.configRegister.bitfields.LUT_BANK_SEL == 0) ? BlackLUTBank0 : BlackLUTBank1;

        result = PacketBuilder::WriteDMI(pCmdBuffer,
                                         regIFE_IFE_0_VFE_DMI_CFG,
                                         bankSelect,
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
// IFELinearization33Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELinearization33Titan17x::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFELinearization33RegCmd) <=
            sizeof(pIFETuningMetadata->metadata17x.IFELinearizationData.linearizationConfig1));
        Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFELinearizationData.linearizationConfig1,
                      &m_regCmd,
                      sizeof(IFELinearization33RegCmd));

        if (NULL != m_pLUTDMIBuffer)
        {
            Utils::Memcpy(pIFETuningMetadata->metadata17x.IFEDMIPacked.linearizationLUT.linearizationLUT,
                          m_pLUTDMIBuffer,
                          IFELinearizationLutTableSize * sizeof(UINT64));
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
// IFELinearization33Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELinearization33Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult                    result        = CamxResultSuccess;
    Linearization33UnpackedField* pData         = static_cast<Linearization33UnpackedField*>(pInput);
    Linearization33OutputData*    pOutputData   = static_cast<Linearization33OutputData*>(pOutput);
    UINT32                        DMILength     = 0;

    if ((NULL != pOutputData) && (NULL != pData) && (NULL != pOutputData->pDMIDataPtr))
    {
        m_regCmd.configRegister.bitfields.LUT_BANK_SEL     = pData->lut_bank_sel;

        /// Left plane
        m_regCmd.interpolationB0Register.bitfields.LUT_P0  = pData->b_lut_p_l[0];
        m_regCmd.interpolationB0Register.bitfields.LUT_P1  = pData->b_lut_p_l[1];
        m_regCmd.interpolationB1Register.bitfields.LUT_P2  = pData->b_lut_p_l[2];
        m_regCmd.interpolationB1Register.bitfields.LUT_P3  = pData->b_lut_p_l[3];
        m_regCmd.interpolationB2Register.bitfields.LUT_P4  = pData->b_lut_p_l[4];
        m_regCmd.interpolationB2Register.bitfields.LUT_P5  = pData->b_lut_p_l[5];
        m_regCmd.interpolationB3Register.bitfields.LUT_P6  = pData->b_lut_p_l[6];
        m_regCmd.interpolationB3Register.bitfields.LUT_P7  = pData->b_lut_p_l[7];

        m_regCmd.interpolationR0Register.bitfields.LUT_P0  = pData->r_lut_p_l[0];
        m_regCmd.interpolationR0Register.bitfields.LUT_P1  = pData->r_lut_p_l[1];
        m_regCmd.interpolationR1Register.bitfields.LUT_P2  = pData->r_lut_p_l[2];
        m_regCmd.interpolationR1Register.bitfields.LUT_P3  = pData->r_lut_p_l[3];
        m_regCmd.interpolationR2Register.bitfields.LUT_P4  = pData->r_lut_p_l[4];
        m_regCmd.interpolationR2Register.bitfields.LUT_P5  = pData->r_lut_p_l[5];
        m_regCmd.interpolationR3Register.bitfields.LUT_P6  = pData->r_lut_p_l[6];
        m_regCmd.interpolationR3Register.bitfields.LUT_P7  = pData->r_lut_p_l[7];

        m_regCmd.interpolationGB0Register.bitfields.LUT_P0 = pData->gb_lut_p_l[0];
        m_regCmd.interpolationGB0Register.bitfields.LUT_P1 = pData->gb_lut_p_l[1];
        m_regCmd.interpolationGB1Register.bitfields.LUT_P2 = pData->gb_lut_p_l[2];
        m_regCmd.interpolationGB1Register.bitfields.LUT_P3 = pData->gb_lut_p_l[3];
        m_regCmd.interpolationGB2Register.bitfields.LUT_P4 = pData->gb_lut_p_l[4];
        m_regCmd.interpolationGB2Register.bitfields.LUT_P5 = pData->gb_lut_p_l[5];
        m_regCmd.interpolationGB3Register.bitfields.LUT_P6 = pData->gb_lut_p_l[6];
        m_regCmd.interpolationGB3Register.bitfields.LUT_P7 = pData->gb_lut_p_l[7];

        m_regCmd.interpolationGR0Register.bitfields.LUT_P0 = pData->gr_lut_p_l[0];
        m_regCmd.interpolationGR0Register.bitfields.LUT_P1 = pData->gr_lut_p_l[1];
        m_regCmd.interpolationGR1Register.bitfields.LUT_P2 = pData->gr_lut_p_l[2];
        m_regCmd.interpolationGR1Register.bitfields.LUT_P3 = pData->gr_lut_p_l[3];
        m_regCmd.interpolationGR2Register.bitfields.LUT_P4 = pData->gr_lut_p_l[4];
        m_regCmd.interpolationGR2Register.bitfields.LUT_P5 = pData->gr_lut_p_l[5];
        m_regCmd.interpolationGR3Register.bitfields.LUT_P6 = pData->gr_lut_p_l[6];
        m_regCmd.interpolationGR3Register.bitfields.LUT_P7 = pData->gr_lut_p_l[7];

        ///< packing the DMI table
        for (UINT32 count = 0; count < MaxSlope ; count++, DMILength += 4)
        {
            pOutputData->pDMIDataPtr[DMILength]    =
                ((static_cast<UINT64>(pData->r_lut_base_l[pData->lut_bank_sel][count])) & Utils::AllOnes64(14)) |
                ((static_cast<UINT64>(pData->r_lut_delta_l[pData->lut_bank_sel][count]) & Utils::AllOnes64(26)) << 14);

            pOutputData->pDMIDataPtr[DMILength + 1] =
                ((static_cast<UINT64>(pData->gr_lut_base_l[pData->lut_bank_sel][count])) & Utils::AllOnes64(14)) |
                ((static_cast<UINT64>( pData->gr_lut_delta_l[pData->lut_bank_sel][count]) & Utils::AllOnes64(26)) << 14);

            pOutputData->pDMIDataPtr[DMILength + 2] =
                ((static_cast<UINT64>(pData->gb_lut_base_l[pData->lut_bank_sel][count])) & Utils::AllOnes64(14)) |
                ((static_cast<UINT64>(pData->gb_lut_delta_l[pData->lut_bank_sel][count]) & Utils::AllOnes64(26)) << 14);

            pOutputData->pDMIDataPtr[DMILength + 3] =
                ((static_cast<UINT64>(pData->b_lut_base_l[pData->lut_bank_sel][count])) & Utils::AllOnes64(14)) |
                ((static_cast<UINT64>(pData->b_lut_delta_l[pData->lut_bank_sel][count]) & Utils::AllOnes64(26)) << 14);

            if (TRUE != pOutputData->registerBETEn)
            {
                const HwEnvironment* pHwEnvironment = HwEnvironment::GetInstance();
                if ((pHwEnvironment->
                    IsHWBugWorkaroundEnabled(Titan17xWorkarounds::Titan17xWorkaroundsCDMDMI64EndiannessBug)) &&
                    ((FALSE == pHwEnvironment->GetStaticSettings()->ifeSWCDMEnable)))
                {
                    pOutputData->pDMIDataPtr[DMILength] =
                        ((pOutputData->pDMIDataPtr[DMILength] & CamX::Utils::AllOnes64(32)) << 32)     |
                        (((pOutputData->pDMIDataPtr[DMILength] >> 32) & CamX::Utils::AllOnes64(32)));
                    pOutputData->pDMIDataPtr[DMILength + 1] =
                        ((pOutputData->pDMIDataPtr[DMILength + 1] & CamX::Utils::AllOnes64(32)) << 32) |
                        (((pOutputData->pDMIDataPtr[DMILength + 1] >> 32) & CamX::Utils::AllOnes64(32)));
                    pOutputData->pDMIDataPtr[DMILength + 2] =
                        ((pOutputData->pDMIDataPtr[DMILength + 2] & CamX::Utils::AllOnes64(32)) << 32) |
                        (((pOutputData->pDMIDataPtr[DMILength + 2] >> 32) & CamX::Utils::AllOnes64(32)));
                    pOutputData->pDMIDataPtr[DMILength + 3] =
                        ((pOutputData->pDMIDataPtr[DMILength + 3] & CamX::Utils::AllOnes64(32)) << 32) |
                        (((pOutputData->pDMIDataPtr[DMILength + 3] >> 32) & CamX::Utils::AllOnes64(32)));
                }
            }

            // Store DMI pointer
            m_pLUTDMIBuffer = pOutputData->pDMIDataPtr;
        }

        /// Right plane
        m_regCmd.rightPlaneInterpolationB0Register.bitfields.LUT_P0  = pData->b_lut_p_r[0];
        m_regCmd.rightPlaneInterpolationB0Register.bitfields.LUT_P1  = pData->b_lut_p_r[1];
        m_regCmd.rightPlaneInterpolationB1Register.bitfields.LUT_P2  = pData->b_lut_p_r[2];
        m_regCmd.rightPlaneInterpolationB1Register.bitfields.LUT_P3  = pData->b_lut_p_r[3];
        m_regCmd.rightPlaneInterpolationB2Register.bitfields.LUT_P4  = pData->b_lut_p_r[4];
        m_regCmd.rightPlaneInterpolationB2Register.bitfields.LUT_P5  = pData->b_lut_p_r[5];
        m_regCmd.rightPlaneInterpolationB3Register.bitfields.LUT_P6  = pData->b_lut_p_r[6];
        m_regCmd.rightPlaneInterpolationB3Register.bitfields.LUT_P7  = pData->b_lut_p_r[7];

        m_regCmd.rightPlaneInterpolationR0Register.bitfields.LUT_P0  = pData->r_lut_p_r[0];
        m_regCmd.rightPlaneInterpolationR0Register.bitfields.LUT_P1  = pData->r_lut_p_r[1];
        m_regCmd.rightPlaneInterpolationR1Register.bitfields.LUT_P2  = pData->r_lut_p_r[2];
        m_regCmd.rightPlaneInterpolationR1Register.bitfields.LUT_P3  = pData->r_lut_p_r[3];
        m_regCmd.rightPlaneInterpolationR2Register.bitfields.LUT_P4  = pData->r_lut_p_r[4];
        m_regCmd.rightPlaneInterpolationR2Register.bitfields.LUT_P5  = pData->r_lut_p_r[5];
        m_regCmd.rightPlaneInterpolationR3Register.bitfields.LUT_P6  = pData->r_lut_p_r[6];
        m_regCmd.rightPlaneInterpolationR3Register.bitfields.LUT_P7  = pData->r_lut_p_r[7];

        m_regCmd.rightPlaneInterpolationGB0Register.bitfields.LUT_P0 = pData->gb_lut_p_r[0];
        m_regCmd.rightPlaneInterpolationGB0Register.bitfields.LUT_P1 = pData->gb_lut_p_r[1];
        m_regCmd.rightPlaneInterpolationGB1Register.bitfields.LUT_P2 = pData->gb_lut_p_r[2];
        m_regCmd.rightPlaneInterpolationGB1Register.bitfields.LUT_P3 = pData->gb_lut_p_r[3];
        m_regCmd.rightPlaneInterpolationGB2Register.bitfields.LUT_P4 = pData->gb_lut_p_r[4];
        m_regCmd.rightPlaneInterpolationGB2Register.bitfields.LUT_P5 = pData->gb_lut_p_r[5];
        m_regCmd.rightPlaneInterpolationGB3Register.bitfields.LUT_P6 = pData->gb_lut_p_r[6];
        m_regCmd.rightPlaneInterpolationGB3Register.bitfields.LUT_P7 = pData->gb_lut_p_r[7];

        m_regCmd.rightPlaneInterpolationGR0Register.bitfields.LUT_P0 = pData->gr_lut_p_r[0];
        m_regCmd.rightPlaneInterpolationGR0Register.bitfields.LUT_P1 = pData->gr_lut_p_r[1];
        m_regCmd.rightPlaneInterpolationGR1Register.bitfields.LUT_P2 = pData->gr_lut_p_r[2];
        m_regCmd.rightPlaneInterpolationGR1Register.bitfields.LUT_P3 = pData->gr_lut_p_r[3];
        m_regCmd.rightPlaneInterpolationGR2Register.bitfields.LUT_P4 = pData->gr_lut_p_r[4];
        m_regCmd.rightPlaneInterpolationGR2Register.bitfields.LUT_P5 = pData->gr_lut_p_r[5];
        m_regCmd.rightPlaneInterpolationGR3Register.bitfields.LUT_P6 = pData->gr_lut_p_r[6];
        m_regCmd.rightPlaneInterpolationGR3Register.bitfields.LUT_P7 = pData->gr_lut_p_r[7];
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input params are NULL pData=0x%p pOutputData=0x%p", pData, pOutputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization33Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELinearization33Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization33Titan17x::~IFELinearization33Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFELinearization33Titan17x::~IFELinearization33Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization33Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFELinearization33Titan17x::DumpRegConfig()
{
}

CAMX_NAMESPACE_END
