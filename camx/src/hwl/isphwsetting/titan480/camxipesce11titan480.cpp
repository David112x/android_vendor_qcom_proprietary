////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipesce11titan480.cpp
/// @brief CAMXIPESCE11TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "sce11setting.h"
#include "camxutils.h"
#include "camxipesce11titan480.h"
#include "camxispiqmodule.h"
#include "titan480_ipe.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief: This structure contains skin enhancement vertex 1 set of registers
struct IPESCEVertex1
{
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_VERTEX1_COORD_CFG_0 cfg0;  ///< Cb, Cr Coordinate of vertex1 triange
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_VERTEX1_COORD_CFG_1 cfg1;  ///< Cb, Cr Coordinate of vertex1 triange
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_VERTEX1_COORD_CFG_2 cfg2;  ///< Cb, Cr Coordinate of vertex1 triange
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_VERTEX1_COORD_CFG_3 cfg3;  ///< Cb, Cr Coordinate of vertex1 triange
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_VERTEX1_COORD_CFG_4 cfg4;  ///< Cb, Cr Coordinate of vertex1 triange
} CAMX_PACKED;

/// @brief: This structure contains skin enhancement vertex 2 set of registers
struct IPESCEVertex2
{
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_VERTEX2_COORD_CFG_0 cfg0;  ///< Cb, Cr Coordinate of vertex2 triange
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_VERTEX2_COORD_CFG_1 cfg1;  ///< Cb, Cr Coordinate of vertex2 triange
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_VERTEX2_COORD_CFG_2 cfg2;  ///< Cb, Cr Coordinate of vertex2 triange
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_VERTEX2_COORD_CFG_3 cfg3;  ///< Cb, Cr Coordinate of vertex2 triange
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_VERTEX2_COORD_CFG_4 cfg4;  ///< Cb, Cr Coordinate of vertex2 triange
} CAMX_PACKED;

/// @brief: This structure contains skin enhancement vertex 3 set of registers
struct IPESCEVertex3
{
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_VERTEX3_COORD_CFG_0 cfg0;  ///< Cb, Cr Coordinate of vertex3 triange
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_VERTEX3_COORD_CFG_1 cfg1;  ///< Cb, Cr Coordinate of vertex3 triange
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_VERTEX3_COORD_CFG_2 cfg2;  ///< Cb, Cr Coordinate of vertex3 triange
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_VERTEX3_COORD_CFG_3 cfg3;  ///< Cb, Cr Coordinate of vertex3 triange
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_VERTEX3_COORD_CFG_4 cfg4;  ///< Cb, Cr Coordinate of vertex3 triange
} CAMX_PACKED;

/// @brief: This structure contains skin enhancement Cr coefficient set of registers
struct IPESCECrCoefficient
{
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CR_COEFF_CFG_0 cfg0;   ///< Cr component of the transform matrix
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CR_COEFF_CFG_1 cfg1;   ///< Cr component of the transform matrix
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CR_COEFF_CFG_2 cfg2;   ///< Cr component of the transform matrix
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CR_COEFF_CFG_3 cfg3;   ///< Cr component of the transform matrix
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CR_COEFF_CFG_4 cfg4;   ///< Cr component of the transform matrix
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CR_COEFF_CFG_5 cfg5;   ///< Cr component of the transform matrix
} CAMX_PACKED;

/// @brief: This structure contains skin enhancement Cb coefficient set of registers
struct IPESCECbCoefficient
{
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CB_COEFF_CFG_0 cfg0;   ///< Cb component of the transform matrix
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CB_COEFF_CFG_1 cfg1;   ///< Cb component of the transform matrix
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CB_COEFF_CFG_2 cfg2;   ///< Cb component of the transform matrix
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CB_COEFF_CFG_3 cfg3;   ///< Cb component of the transform matrix
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CB_COEFF_CFG_4 cfg4;   ///< Cb component of the transform matrix
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CB_COEFF_CFG_5 cfg5;   ///< Cb component of the transform matrix
} CAMX_PACKED;

/// @brief: This structure contains skin enhancement Cr offset set of registers
struct IPESCECrOffset
{
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CR_OFFSET_CFG_0 cfg0;  ///< Offset of the transform matrix for Cr coordinate
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CR_OFFSET_CFG_1 cfg1;  ///< Offset of the transform matrix for Cr coordinate
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CR_OFFSET_CFG_2 cfg2;  ///< Offset of the transform matrix for Cr coordinate
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CR_OFFSET_CFG_3 cfg3;  ///< Offset of the transform matrix for Cr coordinate
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CR_OFFSET_CFG_4 cfg4;  ///< Offset of the transform matrix for Cr coordinate
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CR_OFFSET_CFG_5 cfg5;  ///< Offset of the transform matrix for Cr coordinate
} CAMX_PACKED;

/// @brief: This structure contains skin enhancement Cb offset set of registers
struct IPESCECbOffset
{
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CB_OFFSET_CFG_0 cfg0;  ///< Offset of the transform matrix for Cb coordinate
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CB_OFFSET_CFG_1 cfg1;  ///< Offset of the transform matrix for Cb coordinate
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CB_OFFSET_CFG_2 cfg2;  ///< Offset of the transform matrix for Cb coordinate
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CB_OFFSET_CFG_3 cfg3;  ///< Offset of the transform matrix for Cb coordinate
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CB_OFFSET_CFG_4 cfg4;  ///< Offset of the transform matrix for Cb coordinate
    IPE_IPE_0_PPS_CLC_SKIN_ENHAN_CB_OFFSET_CFG_5 cfg5;  ///< Offset of the transform matrix for Cb coordinate
} CAMX_PACKED;

/// @brief: This structure contains skin color enhancement registers programmed by software
/// There are 6 transform matrices for each of the triangles, starting from triangle 0. Triangle 5 is not
/// really a triangle but the plane outside of all triangles. CR_COEFF_n, n=[0,5]
struct IPESCERegCmd
{
    IPESCEVertex1       vertex1;        ///< Cb, Cr Coordinate of vertex1 triange
    IPESCEVertex2       vertex2;        ///< Cb, Cr Coordinate of vertex2 triange
    IPESCEVertex3       vertex3;        ///< Cb, Cr Coordinate of vertex3 triange
    IPESCECrCoefficient crCoefficient;  ///< Cr component of the transform matrix
    IPESCECbCoefficient cbCoefficient;  ///< Cb component of the transform matrix
    IPESCECrOffset      crOffset;       ///< Offset of the transform matrix for Cr coordinate
    IPESCECbOffset      cbOffset;       ///< Offset of the transform matrix for Cb coordinate
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IPESCERegLength = sizeof(IPESCERegCmd) / RegisterWidthInBytes;
CAMX_STATIC_ASSERT((39 * 4)  == sizeof(IPESCERegCmd));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPESCE11Titan480::IPESCE11Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPESCE11Titan480::IPESCE11Titan480()
{
    m_pRegCmd = CAMX_CALLOC(sizeof(IPESCERegCmd));

    if (NULL != m_pRegCmd)
    {
        // Hardcode initial value for all the registers
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to allocate memory for Cmd Buffer");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPESCE11Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPESCE11Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result     = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if ((NULL != pInputData)                  &&
        (NULL != pInputData->pipelineIPEData.ppIPECmdBuffer) &&
        (NULL != m_pRegCmd))
    {
        pCmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferPostLTM];

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIPE_IPE_0_PPS_CLC_SKIN_ENHAN_VERTEX1_COORD_CFG_0,
                                              IPESCERegLength,
                                              reinterpret_cast<UINT32*>(m_pRegCmd));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write command buffer");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPESCE11Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPESCE11Titan480::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutput)
{
    CamxResult           result   = CamxResultSuccess;
    SCE11UnpackedField*  pDataSCE = static_cast<SCE11UnpackedField*>(pInput);
    IPESCERegCmd*        pRegCmd  = NULL;

    CAMX_UNREFERENCED_PARAM(pOutput);

    if ((NULL != m_pRegCmd) && (NULL != pDataSCE))
    {
        pRegCmd = static_cast<IPESCERegCmd*>(m_pRegCmd);

        pRegCmd->vertex1.cfg0.bitfields.CB_COORD_1 = pDataSCE->t0_Cb1;
        pRegCmd->vertex1.cfg0.bitfields.CR_COORD_1 = pDataSCE->t0_Cr1;
        pRegCmd->vertex1.cfg1.bitfields.CB_COORD_1 = pDataSCE->t1_Cb1;
        pRegCmd->vertex1.cfg1.bitfields.CR_COORD_1 = pDataSCE->t1_Cr1;
        pRegCmd->vertex1.cfg2.bitfields.CB_COORD_1 = pDataSCE->t2_Cb1;
        pRegCmd->vertex1.cfg2.bitfields.CR_COORD_1 = pDataSCE->t2_Cr1;
        pRegCmd->vertex1.cfg3.bitfields.CB_COORD_1 = pDataSCE->t3_Cb1;
        pRegCmd->vertex1.cfg3.bitfields.CR_COORD_1 = pDataSCE->t3_Cr1;
        pRegCmd->vertex1.cfg4.bitfields.CB_COORD_1 = pDataSCE->t4_Cb1;
        pRegCmd->vertex1.cfg4.bitfields.CR_COORD_1 = pDataSCE->t4_Cr1;

        pRegCmd->vertex2.cfg0.bitfields.CB_COORD_2 = pDataSCE->t0_Cb2;
        pRegCmd->vertex2.cfg0.bitfields.CR_COORD_2 = pDataSCE->t0_Cr2;
        pRegCmd->vertex2.cfg1.bitfields.CB_COORD_2 = pDataSCE->t1_Cb2;
        pRegCmd->vertex2.cfg1.bitfields.CR_COORD_2 = pDataSCE->t1_Cr2;
        pRegCmd->vertex2.cfg2.bitfields.CB_COORD_2 = pDataSCE->t2_Cb2;
        pRegCmd->vertex2.cfg2.bitfields.CR_COORD_2 = pDataSCE->t2_Cr2;
        pRegCmd->vertex2.cfg3.bitfields.CB_COORD_2 = pDataSCE->t3_Cb2;
        pRegCmd->vertex2.cfg3.bitfields.CR_COORD_2 = pDataSCE->t3_Cr2;
        pRegCmd->vertex2.cfg4.bitfields.CB_COORD_2 = pDataSCE->t4_Cb2;
        pRegCmd->vertex2.cfg4.bitfields.CR_COORD_2 = pDataSCE->t4_Cr2;

        pRegCmd->vertex3.cfg0.bitfields.CB_COORD_3 = pDataSCE->t0_Cb3;
        pRegCmd->vertex3.cfg0.bitfields.CR_COORD_3 = pDataSCE->t0_Cr3;
        pRegCmd->vertex3.cfg1.bitfields.CB_COORD_3 = pDataSCE->t1_Cb3;
        pRegCmd->vertex3.cfg1.bitfields.CR_COORD_3 = pDataSCE->t1_Cr3;
        pRegCmd->vertex3.cfg2.bitfields.CB_COORD_3 = pDataSCE->t2_Cb3;
        pRegCmd->vertex3.cfg2.bitfields.CR_COORD_3 = pDataSCE->t2_Cr3;
        pRegCmd->vertex3.cfg3.bitfields.CB_COORD_3 = pDataSCE->t3_Cb3;
        pRegCmd->vertex3.cfg3.bitfields.CR_COORD_3 = pDataSCE->t3_Cr3;
        pRegCmd->vertex3.cfg4.bitfields.CB_COORD_3 = pDataSCE->t4_Cb3;
        pRegCmd->vertex3.cfg4.bitfields.CR_COORD_3 = pDataSCE->t4_Cr3;

        pRegCmd->crCoefficient.cfg0.bitfields.COEFF_A = pDataSCE->t0_A;
        pRegCmd->crCoefficient.cfg0.bitfields.COEFF_B = pDataSCE->t0_B;
        pRegCmd->crCoefficient.cfg1.bitfields.COEFF_A = pDataSCE->t1_A;
        pRegCmd->crCoefficient.cfg1.bitfields.COEFF_B = pDataSCE->t1_B;
        pRegCmd->crCoefficient.cfg2.bitfields.COEFF_A = pDataSCE->t2_A;
        pRegCmd->crCoefficient.cfg2.bitfields.COEFF_B = pDataSCE->t2_B;
        pRegCmd->crCoefficient.cfg3.bitfields.COEFF_A = pDataSCE->t3_A;
        pRegCmd->crCoefficient.cfg3.bitfields.COEFF_B = pDataSCE->t3_B;
        pRegCmd->crCoefficient.cfg4.bitfields.COEFF_A = pDataSCE->t4_A;
        pRegCmd->crCoefficient.cfg4.bitfields.COEFF_B = pDataSCE->t4_B;
        pRegCmd->crCoefficient.cfg5.bitfields.COEFF_A = pDataSCE->t5_A;
        pRegCmd->crCoefficient.cfg5.bitfields.COEFF_B = pDataSCE->t5_B;

        pRegCmd->cbCoefficient.cfg0.bitfields.COEFF_D = pDataSCE->t0_D;
        pRegCmd->cbCoefficient.cfg0.bitfields.COEFF_E = pDataSCE->t0_E;
        pRegCmd->cbCoefficient.cfg1.bitfields.COEFF_D = pDataSCE->t1_D;
        pRegCmd->cbCoefficient.cfg1.bitfields.COEFF_E = pDataSCE->t1_E;
        pRegCmd->cbCoefficient.cfg2.bitfields.COEFF_D = pDataSCE->t2_D;
        pRegCmd->cbCoefficient.cfg2.bitfields.COEFF_E = pDataSCE->t2_E;
        pRegCmd->cbCoefficient.cfg3.bitfields.COEFF_D = pDataSCE->t3_D;
        pRegCmd->cbCoefficient.cfg3.bitfields.COEFF_E = pDataSCE->t3_E;
        pRegCmd->cbCoefficient.cfg4.bitfields.COEFF_D = pDataSCE->t4_D;
        pRegCmd->cbCoefficient.cfg4.bitfields.COEFF_E = pDataSCE->t4_E;
        pRegCmd->cbCoefficient.cfg5.bitfields.COEFF_D = pDataSCE->t5_D;
        pRegCmd->cbCoefficient.cfg5.bitfields.COEFF_E = pDataSCE->t5_E;

        pRegCmd->crOffset.cfg0.bitfields.MATRIX_SHIFT = pDataSCE->t0_Q1;
        pRegCmd->crOffset.cfg0.bitfields.OFFSET_C     = pDataSCE->t0_C;
        pRegCmd->crOffset.cfg1.bitfields.MATRIX_SHIFT = pDataSCE->t1_Q1;
        pRegCmd->crOffset.cfg1.bitfields.OFFSET_C     = pDataSCE->t1_C;
        pRegCmd->crOffset.cfg2.bitfields.MATRIX_SHIFT = pDataSCE->t2_Q1;
        pRegCmd->crOffset.cfg2.bitfields.OFFSET_C     = pDataSCE->t2_C;
        pRegCmd->crOffset.cfg3.bitfields.MATRIX_SHIFT = pDataSCE->t3_Q1;
        pRegCmd->crOffset.cfg3.bitfields.OFFSET_C     = pDataSCE->t3_C;
        pRegCmd->crOffset.cfg4.bitfields.MATRIX_SHIFT = pDataSCE->t4_Q1;
        pRegCmd->crOffset.cfg4.bitfields.OFFSET_C     = pDataSCE->t4_C;
        pRegCmd->crOffset.cfg5.bitfields.MATRIX_SHIFT = pDataSCE->t5_Q1;
        pRegCmd->crOffset.cfg5.bitfields.OFFSET_C     = pDataSCE->t5_C;

        pRegCmd->cbOffset.cfg0.bitfields.OFFSET_SHIFT = pDataSCE->t0_Q2;
        pRegCmd->cbOffset.cfg0.bitfields.OFFSET_F     = pDataSCE->t0_F;
        pRegCmd->cbOffset.cfg1.bitfields.OFFSET_SHIFT = pDataSCE->t1_Q2;
        pRegCmd->cbOffset.cfg1.bitfields.OFFSET_F     = pDataSCE->t1_F;
        pRegCmd->cbOffset.cfg2.bitfields.OFFSET_SHIFT = pDataSCE->t2_Q2;
        pRegCmd->cbOffset.cfg2.bitfields.OFFSET_F     = pDataSCE->t2_F;
        pRegCmd->cbOffset.cfg3.bitfields.OFFSET_SHIFT = pDataSCE->t3_Q2;
        pRegCmd->cbOffset.cfg3.bitfields.OFFSET_F     = pDataSCE->t3_F;
        pRegCmd->cbOffset.cfg4.bitfields.OFFSET_SHIFT = pDataSCE->t4_Q2;
        pRegCmd->cbOffset.cfg4.bitfields.OFFSET_F     = pDataSCE->t4_C;
        pRegCmd->cbOffset.cfg5.bitfields.OFFSET_SHIFT = pDataSCE->t5_Q2;
        pRegCmd->cbOffset.cfg5.bitfields.OFFSET_F     = pDataSCE->t5_F;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Abort! NULL data pointer  pDataSCE %p m_pRegCmd ", pDataSCE, m_pRegCmd);
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPESCE11Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPESCE11Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pInput);

    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPESCE11Titan480::GetRegSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPESCE11Titan480::GetRegSize()
{
    return sizeof(IPESCERegCmd);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPESCE11Titan480::~IPESCE11Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPESCE11Titan480::~IPESCE11Titan480()
{
    if (NULL != m_pRegCmd)
    {
        CAMX_FREE(m_pRegCmd);
        m_pRegCmd = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPESCE11Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPESCE11Titan480::DumpRegConfig()
{
    IPESCERegCmd*        pRegCmd = NULL;

    if (NULL != m_pRegCmd)
    {
        pRegCmd = static_cast<IPESCERegCmd*>(m_pRegCmd);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.vertex1.cfg0        = %x\n", pRegCmd->vertex1.cfg0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.vertex1.cfg1        = %x\n", pRegCmd->vertex1.cfg1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.vertex1.cfg2        = %x\n", pRegCmd->vertex1.cfg2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.vertex1.cfg3        = %x\n", pRegCmd->vertex1.cfg3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.vertex1.cfg4        = %x\n", pRegCmd->vertex1.cfg4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.vertex2.cfg0        = %x\n", pRegCmd->vertex2.cfg0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.vertex2.cfg1        = %x\n", pRegCmd->vertex2.cfg1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.vertex2.cfg2        = %x\n", pRegCmd->vertex2.cfg2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.vertex2.cfg3        = %x\n", pRegCmd->vertex2.cfg3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.vertex2.cfg4        = %x\n", pRegCmd->vertex2.cfg4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.vertex3.cfg0        = %x\n", pRegCmd->vertex3.cfg0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.vertex3.cfg1        = %x\n", pRegCmd->vertex3.cfg1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.vertex3.cfg2        = %x\n", pRegCmd->vertex3.cfg2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.vertex3.cfg3        = %x\n", pRegCmd->vertex3.cfg3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.vertex3.cfg4        = %x\n", pRegCmd->vertex3.cfg4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.crCoefficient.cfg0  = %x\n", pRegCmd->crCoefficient.cfg0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.crCoefficient.cfg1  = %x\n", pRegCmd->crCoefficient.cfg1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.crCoefficient.cfg2  = %x\n", pRegCmd->crCoefficient.cfg2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.crCoefficient.cfg3  = %x\n", pRegCmd->crCoefficient.cfg3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.crCoefficient.cfg4  = %x\n", pRegCmd->crCoefficient.cfg4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.crCoefficient.cfg5  = %x\n", pRegCmd->crCoefficient.cfg5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.cbCoefficient.cfg0  = %x\n", pRegCmd->cbCoefficient.cfg0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.cbCoefficient.cfg1  = %x\n", pRegCmd->cbCoefficient.cfg1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.cbCoefficient.cfg2  = %x\n", pRegCmd->cbCoefficient.cfg2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.cbCoefficient.cfg3  = %x\n", pRegCmd->cbCoefficient.cfg3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.cbCoefficient.cfg4  = %x\n", pRegCmd->cbCoefficient.cfg4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.cbCoefficient.cfg5  = %x\n", pRegCmd->cbCoefficient.cfg5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.crOffset.cfg0       = %x\n", pRegCmd->crOffset.cfg0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.crOffset.cfg1       = %x\n", pRegCmd->crOffset.cfg1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.crOffset.cfg2       = %x\n", pRegCmd->crOffset.cfg2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.crOffset.cfg3       = %x\n", pRegCmd->crOffset.cfg3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.crOffset.cfg4       = %x\n", pRegCmd->crOffset.cfg4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.crOffset.cfg5       = %x\n", pRegCmd->crOffset.cfg5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.cbOffset.cfg0       = %x\n", pRegCmd->cbOffset.cfg0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.cbOffset.cfg1       = %x\n", pRegCmd->cbOffset.cfg1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.cbOffset.cfg2       = %x\n", pRegCmd->cbOffset.cfg2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.cbOffset.cfg3       = %x\n", pRegCmd->cbOffset.cfg3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.cbOffset.cfg4       = %x\n", pRegCmd->cbOffset.cfg4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SCE11.cbOffset.cfg5       = %x\n", pRegCmd->cbOffset.cfg5);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Error m_pRegCmd is NULL");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPESCE11Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPESCE11Titan480::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    IPETuningMetadata* pIPETuningMetadata = static_cast<IPETuningMetadata*>(pInputData->pIPETuningMetadata);
    IpeIQSettings*     pIPEIQSettings     = static_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);
    IPESCERegCmd*      pRegCmd            = static_cast<IPESCERegCmd*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if ((NULL != pInputData->pIPETuningMetadata) && (NULL != pRegCmd))
    {
        CAMX_STATIC_ASSERT(sizeof(IPESCERegCmd) ==
            sizeof(pIPETuningMetadata->IPETuningMetadata480.IPESCEData.SCEConfig));

        Utils::Memcpy(&pIPETuningMetadata->IPETuningMetadata480.IPESCEData.SCEConfig,
                      pRegCmd,
                      sizeof(IPESCERegCmd));

        if (TRUE == pIPEIQSettings->skinEnhancementParameters.moduleCfg.EN)
        {
            DebugDataTagID dataTagID;
            if (TRUE == pInputData->pipelineIPEData.realtimeFlag)
            {
                dataTagID = DebugDataTagID::TuningIPESCE11Register;
            }
            else
            {
                dataTagID = DebugDataTagID::TuningIPESCE11RegisterOffline;
            }

            result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                dataTagID,
                DebugDataTagType::UInt32,
                CAMX_ARRAY_SIZE(pIPETuningMetadata->IPETuningMetadata480.IPESCEData.SCEConfig),
                &pIPETuningMetadata->IPETuningMetadata480.IPESCEData.SCEConfig,
                sizeof(pIPETuningMetadata->IPETuningMetadata480.IPESCEData.SCEConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
            }
        }
    }
    return result;
}

CAMX_NAMESPACE_END
