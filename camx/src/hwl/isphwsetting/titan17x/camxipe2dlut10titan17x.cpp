////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipe2dlut10titan17x.cpp
/// @brief IPE2DLUT10Titan17x class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "titan170_ipe.h"
#include "ipe2dlut10setting.h"
#include "camxutils.h"
#include "camxipe2dlut10titan17x.h"
#include "camxipe2dlut10.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief: Registers programmed by software for 2D LUT module
struct IPER2DLUTRegCmd
{
    IPE_IPE_0_PPS_CLC_2D_LUT_H_SHIFT_CFG                hueShift;       ///< Hue Delta Range
    IPE_IPE_0_PPS_CLC_2D_LUT_S_SHIFT_CFG                satShift;       ///< Saturation Delta Range
    IPE_IPE_0_PPS_CLC_2D_LUT_L_BOUNDARY_START_A_CFG     startA;         ///< Boundary control based on L start threshold1
    IPE_IPE_0_PPS_CLC_2D_LUT_L_BOUNDARY_START_B_CFG     startB;         ///< Boundary control based on L start threshold2
    IPE_IPE_0_PPS_CLC_2D_LUT_L_BOUNDARY_END_A_CFG       endA;           ///< Boundary control based on L end threshold1
    IPE_IPE_0_PPS_CLC_2D_LUT_L_BOUNDARY_END_B_CFG       endB;           ///< Boundary control based on L end threshold2
    IPE_IPE_0_PPS_CLC_2D_LUT_L_BOUNDARY_START_INV_CFG   startInv;       ///< Start points inverse for L end to calculate ratio
    IPE_IPE_0_PPS_CLC_2D_LUT_L_BOUNDARY_END_INV_CFG     endInv;         ///< End points inverse for L end to calculate ratio
    IPE_IPE_0_PPS_CLC_2D_LUT_Y_BLEND_FACTOR_INTEGER_CFG blendFactor;    ///< Y offset weight for 0~1.0
    IPE_IPE_0_PPS_CLC_2D_LUT_K_B_INTEGER_CFG            kbIntegeter;    ///< Luma Delta Coefficent for Blue
    IPE_IPE_0_PPS_CLC_2D_LUT_K_R_INTEGER_CFG            krIntegeter;    ///< Luma Delta Coefficent for Red
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IPE2DLUTRegLength = sizeof(IPER2DLUTRegCmd) / RegisterWidthInBytes;
CAMX_STATIC_ASSERT((11 * 4) == sizeof(IPER2DLUTRegCmd));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPE2DLUT10Titan17x::IPE2DLUT10Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPE2DLUT10Titan17x::IPE2DLUT10Titan17x()
{
    m_pRegCmd = CAMX_CALLOC(sizeof(IPER2DLUTRegCmd));

    if (NULL == m_pRegCmd)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to allocate memory for Cmd Buffer");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPE2DLUT10Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPE2DLUT10Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pTDLHWSettingParams)
{
    CamxResult result = CamxResultSuccess;

    ISPInputData*       pInputData             = static_cast<ISPInputData*>(pSettingData);
    TDLHWSettingParams* pModuleHWSettingParams = reinterpret_cast<TDLHWSettingParams*>(pTDLHWSettingParams);

    m_pLUTDMICmdBuffer = pModuleHWSettingParams->pLUTDMICmdBuffer;

    result = WriteLUTtoDMI(pInputData);
    if (CamxResultSuccess == result)
    {
        CmdBuffer* pCmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferPostLTM];

        if (NULL != pCmdBuffer)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIPE_IPE_0_PPS_CLC_2D_LUT_H_SHIFT_CFG,
                                                  IPE2DLUTRegLength,
                                                  reinterpret_cast<UINT32*>(m_pRegCmd));
        }
        else
        {
            result = CamxResultEInvalidPointer;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Null command buffer");
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write command buffer");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "LUT write failed");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPE2DLUT10Titan17x::WriteLUTtoDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPE2DLUT10Titan17x::WriteLUTtoDMI(
    ISPInputData* pInputData)
{
    CamxResult  result        = CamxResultSuccess;
    UINT32      LUTOffset     = 0;
    CmdBuffer*  pDMICmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader];

    if ((NULL != pDMICmdBuffer) && (NULL != m_pLUTDMICmdBuffer))
    {
        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_2D_LUT_DMI_CFG,
                                         (LUT2DIndexHue0 + 1),
                                         m_pLUTDMICmdBuffer,
                                         LUTOffset,
                                         IPE2DLUTLUTSize[LUT2DIndexHue0] * sizeof(UINT32));
    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "PacketBuilder::WriteDMI fail for LUT2DIndexHue0!");
    }

    LUTOffset += (IPE2DLUTLUTSize[LUT2DIndexHue0] * sizeof(UINT32));

    result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                     regIPE_IPE_0_PPS_CLC_2D_LUT_DMI_CFG,
                                     (LUT2DIndexHue1 + 1),
                                     m_pLUTDMICmdBuffer,
                                     LUTOffset,
                                     IPE2DLUTLUTSize[LUT2DIndexHue1] * sizeof(UINT32));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "PacketBuilder::WriteDMI fail for LUT2DIndexHue1!");
    }

    LUTOffset += (IPE2DLUTLUTSize[LUT2DIndexHue1] * sizeof(UINT32));

    result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                     regIPE_IPE_0_PPS_CLC_2D_LUT_DMI_CFG,
                                     (LUT2DIndexHue2 + 1),
                                     m_pLUTDMICmdBuffer,
                                     LUTOffset,
                                     IPE2DLUTLUTSize[LUT2DIndexHue2] * sizeof(UINT32));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "PacketBuilder::WriteDMI fail for LUT2DIndexHue2!");
    }

    LUTOffset += (IPE2DLUTLUTSize[LUT2DIndexHue2] * sizeof(UINT32));

    result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                     regIPE_IPE_0_PPS_CLC_2D_LUT_DMI_CFG,
                                     (LUT2DIndexHue3 + 1),
                                     m_pLUTDMICmdBuffer,
                                     LUTOffset,
                                     IPE2DLUTLUTSize[LUT2DIndexHue3] * sizeof(UINT32));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "PacketBuilder::WriteDMI fail for LUT2DIndexHue3!");
    }

    LUTOffset += (IPE2DLUTLUTSize[LUT2DIndexHue3] * sizeof(UINT32));

    result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                     regIPE_IPE_0_PPS_CLC_2D_LUT_DMI_CFG,
                                     (LUT2DIndexSaturation0 + 1),
                                     m_pLUTDMICmdBuffer,
                                     LUTOffset,
                                     IPE2DLUTLUTSize[LUT2DIndexSaturation0] * sizeof(UINT32));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "PacketBuilder::WriteDMI fail for LUT2DIndexSaturation0!");
    }

    LUTOffset += (IPE2DLUTLUTSize[LUT2DIndexSaturation0] * sizeof(UINT32));

    result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                     regIPE_IPE_0_PPS_CLC_2D_LUT_DMI_CFG,
                                     (LUT2DIndexSaturation1 + 1),
                                     m_pLUTDMICmdBuffer,
                                     LUTOffset,
                                     IPE2DLUTLUTSize[LUT2DIndexSaturation1] * sizeof(UINT32));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "PacketBuilder::WriteDMI fail for LUT2DIndexSaturation1!");
    }

    LUTOffset += (IPE2DLUTLUTSize[LUT2DIndexSaturation1] * sizeof(UINT32));

    result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                     regIPE_IPE_0_PPS_CLC_2D_LUT_DMI_CFG,
                                     (LUT2DIndexSaturation2 + 1),
                                     m_pLUTDMICmdBuffer,
                                     LUTOffset,
                                     IPE2DLUTLUTSize[LUT2DIndexSaturation2] * sizeof(UINT32));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "PacketBuilder::WriteDMI fail for LUT2DIndexSaturation2!");
    }

    LUTOffset += (IPE2DLUTLUTSize[LUT2DIndexSaturation2] * sizeof(UINT32));

    result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                     regIPE_IPE_0_PPS_CLC_2D_LUT_DMI_CFG,
                                     (LUT2DIndexSaturation3 + 1),
                                     m_pLUTDMICmdBuffer,
                                     LUTOffset,
                                     IPE2DLUTLUTSize[LUT2DIndexSaturation3] * sizeof(UINT32));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "PacketBuilder::WriteDMI fail for LUT2DIndexSaturation3!");
    }

    LUTOffset += (IPE2DLUTLUTSize[LUT2DIndexSaturation3] * sizeof(UINT32));

    result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                     regIPE_IPE_0_PPS_CLC_2D_LUT_DMI_CFG,
                                     (LUT2DIndexInverseHue + 1),
                                     m_pLUTDMICmdBuffer,
                                     LUTOffset,
                                     IPE2DLUTLUTSize[LUT2DIndexInverseHue] * sizeof(UINT32));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "PacketBuilder::WriteDMI fail for LUT2DIndexInverseHue!");
    }

    LUTOffset += (IPE2DLUTLUTSize[LUT2DIndexInverseHue] * sizeof(UINT32));

    result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                     regIPE_IPE_0_PPS_CLC_2D_LUT_DMI_CFG,
                                     (LUT2DIndexInverseSaturation + 1),
                                     m_pLUTDMICmdBuffer,
                                     LUTOffset,
                                     IPE2DLUTLUTSize[LUT2DIndexInverseSaturation] * sizeof(UINT32));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "PacketBuilder::WriteDMI fail for LUT2DIndexInverseSaturation!");
    }

    LUTOffset += (IPE2DLUTLUTSize[LUT2DIndexInverseSaturation] * sizeof(UINT32));

    result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                     regIPE_IPE_0_PPS_CLC_2D_LUT_DMI_CFG,
                                     (LUT2DIndex1DHue + 1),
                                     m_pLUTDMICmdBuffer,
                                     LUTOffset,
                                     IPE2DLUTLUTSize[LUT2DIndex1DHue] * sizeof(UINT32));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "PacketBuilder::WriteDMI fail for LUT2DIndex1DHue!");
    }

    LUTOffset += (IPE2DLUTLUTSize[LUT2DIndex1DHue] * sizeof(UINT32));

    result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                     regIPE_IPE_0_PPS_CLC_2D_LUT_DMI_CFG,
                                     (LUT2DIndex1DSaturation + 1),
                                     m_pLUTDMICmdBuffer,
                                     LUTOffset,
                                     IPE2DLUTLUTSize[LUT2DIndex1DSaturation] * sizeof(UINT32));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "PacketBuilder::WriteDMI fail for LUT2DIndex1DSaturation!");
    }

    LUTOffset += (IPE2DLUTLUTSize[LUT2DIndex1DSaturation] * sizeof(UINT32));

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPE2DLUT10Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPE2DLUT10Titan17x::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutputVal)
{
    CAMX_UNREFERENCED_PARAM(pOutputVal);
    CamxResult result = CamxResultSuccess;

    if ((NULL != pInput) && (NULL != m_pRegCmd))
    {
        IPER2DLUTRegCmd*    pRegCmd = static_cast<IPER2DLUTRegCmd*>(m_pRegCmd);
        TDL10UnpackedField* pData   = static_cast<TDL10UnpackedField*>(pInput);

        pRegCmd->blendFactor.bitfields.Y_BLEND_FACTOR_INTEGER = pData->y_blend_factor_integer;
        pRegCmd->endA.bitfields.L_BOUNDARY_END_A              = pData->l_boundary_end_A;
        pRegCmd->endB.bitfields.L_BOUNDARY_END_B              = pData->l_boundary_end_B;
        pRegCmd->endInv.bitfields.L_BOUNDARY_END_INV          = pData->l_boundary_end_inv;
        pRegCmd->hueShift.bitfields.H_SHIFT                   = pData->h_shift;
        pRegCmd->kbIntegeter.bitfields.K_B_INTEGER            = pData->k_b_integer;
        pRegCmd->krIntegeter.bitfields.K_R_INTEGER            = pData->k_r_integer;
        pRegCmd->satShift.bitfields.S_SHIFT                   = pData->s_shift;
        pRegCmd->startA.bitfields.L_BOUNDARY_START_A          = pData->l_boundary_start_A;
        pRegCmd->startB.bitfields.L_BOUNDARY_START_B          = pData->l_boundary_start_B;
        pRegCmd->startInv.bitfields.L_BOUNDARY_START_INV      = pData->l_boundary_start_inv;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid input");
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPE2DLUT10Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPE2DLUT10Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pInput);

    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPE2DLUT10Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPE2DLUT10Titan17x::UpdateTuningMetadata(
    VOID*  pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    IPETuningMetadata* pIPETuningMetadata = static_cast<IPETuningMetadata*>(pInputData->pIPETuningMetadata);
    IpeIQSettings*     pIPEIQSettings     = static_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);;
    IPER2DLUTRegCmd*   pRegCmd            = static_cast<IPER2DLUTRegCmd*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if ((NULL != pInputData->pIPETuningMetadata) && (NULL != pRegCmd))
    {
        CAMX_STATIC_ASSERT(sizeof(IPER2DLUTRegCmd) ==
            sizeof(pIPETuningMetadata->IPETuningMetadata17x.IPE2DLUTData.LUT2DConfig));

        Utils::Memcpy(&pIPETuningMetadata->IPETuningMetadata17x.IPE2DLUTData.LUT2DConfig,
                        pRegCmd,
                        sizeof(IPER2DLUTRegCmd));

        if (TRUE == pIPEIQSettings->lut2dParameters.moduleCfg.EN)
        {
            DebugDataTagID dataTagID;

            if (TRUE == pInputData->pipelineIPEData.realtimeFlag)
            {
                dataTagID = DebugDataTagID::TuningIPE2DLUT10Register;
            }
            else
            {
                dataTagID = DebugDataTagID::TuningIPE2DLUT10RegisterOffline;
            }

            result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                dataTagID,
                DebugDataTagType::UInt32,
                CAMX_ARRAY_SIZE(pIPETuningMetadata->IPETuningMetadata17x.IPE2DLUTData.LUT2DConfig),
                &pIPETuningMetadata->IPETuningMetadata17x.IPE2DLUTData.LUT2DConfig,
                sizeof(pIPETuningMetadata->IPETuningMetadata17x.IPE2DLUTData.LUT2DConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPE2DLUT10Titan17x::GetRegSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPE2DLUT10Titan17x::GetRegSize()
{
    return sizeof(IPER2DLUTRegCmd);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPE2DLUT10Titan17x::~IPE2DLUT10Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPE2DLUT10Titan17x::~IPE2DLUT10Titan17x()
{
    if (NULL != m_pRegCmd)
    {
        CAMX_FREE(m_pRegCmd);
        m_pRegCmd = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPE2DLUT10Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPE2DLUT10Titan17x::DumpRegConfig()
{
    if (NULL != m_pRegCmd)
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
                regIPE_IPE_0_PPS_CLC_2D_LUT_H_SHIFT_CFG,
                IPE2DLUTRegLength,
                reinterpret_cast<UINT32*>(m_pRegCmd)
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
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Error m_pRegCmd is NULL");
    }
}

CAMX_NAMESPACE_END
