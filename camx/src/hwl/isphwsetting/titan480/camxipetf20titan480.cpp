////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipetf20titan480.cpp
/// @brief CAMXIPETF20TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "camxipetf20titan480.h"
#include "tf20setting.h"
#include "camxutils.h"
#include "camxispiqmodule.h"
#include "tf20regcmd480.h"
#include "Process_TF480.h"
#include "NcLibWarp.h"


CAMX_NAMESPACE_BEGIN

static const UINT32 IPETFReg480Length = sizeof(IPETFRegCmd480) / sizeof(UINT32); // RegisterWidthInBytes;
CAMX_STATIC_ASSERT((99) == IPETFReg480Length);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF20Titan480::IPETF20Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPETF20Titan480::IPETF20Titan480()
{
    m_pRegCmd = CAMX_CALLOC(PASS_NAME_MAX * sizeof(IPETFRegCmd480));

    if (NULL != m_pRegCmd)
    {
        // Hardcode initial value for all the registers
        InitializeDefaultRegisterValues();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to allocate memory for Cmd Buffer");
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF20Titan480::InitializeDefaultRegisterValues
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void IPETF20Titan480::InitializeDefaultRegisterValues()
{
    RunCalculationFullPass();
    RunCalculationDS4Pass();
    RunCalculationDS16Pass();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF20Titan480::RunCalculationFullPass
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPETF20Titan480::RunCalculationFullPass()
{
    CamxResult result = CamxResultSuccess;
    IPETFRegCmd480* pRegCmd = static_cast<IPETFRegCmd480*>(m_pRegCmd);

    pRegCmd[PASS_NAME_FULL].config0.u32All                = 0x4004401e;
    pRegCmd[PASS_NAME_FULL].config1.u32All                = 0x00004040;
    pRegCmd[PASS_NAME_FULL].erodeConfig.u32All            = 0x000086cb;
    pRegCmd[PASS_NAME_FULL].dilateConfig.u32All           = 0x000086cd;
    pRegCmd[PASS_NAME_FULL].cropInHorizStart.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cropInHorizEnd.u32All         = 0x00046d1b;
    pRegCmd[PASS_NAME_FULL].lnrStartIDXH.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_FULL].usCrop.u32All                 = 0x00000080;
    pRegCmd[PASS_NAME_FULL].indCropConfig.u32All          = 0x00000000;
    pRegCmd[PASS_NAME_FULL].prngSeed.u32All               = 0x00000001;
    pRegCmd[PASS_NAME_FULL].refCfg0.u32All                = 0x000000f0;
    pRegCmd[PASS_NAME_FULL].refCfg1.u32All                = 0x00000000;
    pRegCmd[PASS_NAME_FULL].refCfg2.u32All                = 0x00080401;
    pRegCmd[PASS_NAME_FULL].cropInvertStart.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cropInvertEnd.u32All          = 0x021bc86f;
    pRegCmd[PASS_NAME_FULL].lnrStartIDXV.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_FULL].lnrScale.u32All               = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cropOutVert.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].refYCfg.u32All                = 0x01010001;
    pRegCmd[PASS_NAME_FULL].tdNtNpYContrib0.u32All        = 0x06018048;
    pRegCmd[PASS_NAME_FULL].tdNtNpYContrib1.u32All        = 0x06018048;
    pRegCmd[PASS_NAME_FULL].tdNtNpYContrib2.u32All        = 0x06018048;
    pRegCmd[PASS_NAME_FULL].tdNtNpYContrib3.u32All        = 0x06018048;
    pRegCmd[PASS_NAME_FULL].tdNtNpYContrib4.u32All        = 0x06018048;
    pRegCmd[PASS_NAME_FULL].tdNtNpYContrib5.u32All        = 0x06018048;
    pRegCmd[PASS_NAME_FULL].tdNtNpYContrib6.u32All        = 0x06018048;
    pRegCmd[PASS_NAME_FULL].tdNtNpYContrib7.u32All        = 0x06018048;
    pRegCmd[PASS_NAME_FULL].tdNtNpYContrib8.u32All        = 0x06018048;
    pRegCmd[PASS_NAME_FULL].tdNtNpYContrib9.u32All        = 0x06018048;
    pRegCmd[PASS_NAME_FULL].tdNtNpYContrib10.u32All       = 0x06018048;
    pRegCmd[PASS_NAME_FULL].tdNtNpYContrib11.u32All       = 0x06018048;
    pRegCmd[PASS_NAME_FULL].tdNtNpYContrib12.u32All       = 0x06018048;
    pRegCmd[PASS_NAME_FULL].tdNtNpYContrib13.u32All       = 0x06018048;
    pRegCmd[PASS_NAME_FULL].tdNtNpYContrib14.u32All       = 0x06018048;
    pRegCmd[PASS_NAME_FULL].tdNtNpYContrib15.u32All       = 0x06018048;
    pRegCmd[PASS_NAME_FULL].tdNtNpYContrib16.u32All       = 0x06018048;
    pRegCmd[PASS_NAME_FULL].tdNtNpCContribY0.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_FULL].tdNtNpCContribY1.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_FULL].tdNtNpCContribY2.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_FULL].tdNtNpCContribY3.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_FULL].tdNtNpCContribCB0.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_FULL].tdNtNpCContribCB1.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_FULL].tdNtNpCContribCB2.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_FULL].tdNtNpCContribCB3.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_FULL].tdNtNpCContribCR0.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_FULL].tdNtNpCContribCR1.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_FULL].tdNtNpCContribCR2.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_FULL].tdNtNpCContribCR3.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_FULL].tdNtNpUVLimit.u32All          = 0x07ffffff;
    pRegCmd[PASS_NAME_FULL].tdNtNpTopLimit.u32All         = 0x06018048;
    pRegCmd[PASS_NAME_FULL].tdNtNpBottomLimit.u32All      = 0x06018048;
    pRegCmd[PASS_NAME_FULL].tdNtLnrLutY0.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_FULL].tdNtLnrLutY1.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_FULL].tdNtLnrLutY2.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_FULL].tdNtLnrLutY3.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_FULL].tdNtLnrLutC0.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_FULL].tdNtLnrLutC1.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_FULL].tdNtLnrLutC2.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_FULL].tdNtLnrLutC3.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_FULL].fsDecisionParamsY0.u32All     = 0x00880008;
    pRegCmd[PASS_NAME_FULL].fsDecisionParamsY1.u32All     = 0x01680008;
    pRegCmd[PASS_NAME_FULL].fsDecisionParamsY2.u32All     = 0x02680008;
    pRegCmd[PASS_NAME_FULL].fsDecisionParamsY3.u32All     = 0x03480008;
    pRegCmd[PASS_NAME_FULL].fsDecisionParamsY4.u32All     = 0x04480008;
    pRegCmd[PASS_NAME_FULL].fsDecisionParamsY5.u32All     = 0x0529800a;
    pRegCmd[PASS_NAME_FULL].fsDecisionParamsY6.u32All     = 0x062b800c;
    pRegCmd[PASS_NAME_FULL].fsDecisionParamsY7.u32All     = 0x070d800e;
    pRegCmd[PASS_NAME_FULL].fsDecisionParamsY8.u32All     = 0x080f8010;
    pRegCmd[PASS_NAME_FULL].fsDecisionParamsOOFY.u32All   = 0x02020001;
    pRegCmd[PASS_NAME_FULL].fsDecisionParamsC0.u32All     = 0x08000000;
    pRegCmd[PASS_NAME_FULL].fsDecisionParamsC1.u32All     = 0x08018002;
    pRegCmd[PASS_NAME_FULL].fsDecisionParamsC2.u32All     = 0x08038004;
    pRegCmd[PASS_NAME_FULL].fsDecisionParamsC3.u32All     = 0x08058006;
    pRegCmd[PASS_NAME_FULL].fsDecisionParamsC4.u32All     = 0x08078008;
    pRegCmd[PASS_NAME_FULL].fsDecisionParamsC5.u32All     = 0x0809800a;
    pRegCmd[PASS_NAME_FULL].fsDecisionParamsC6.u32All     = 0x080b800c;
    pRegCmd[PASS_NAME_FULL].fsDecisionParamsC7.u32All     = 0x080d800e;
    pRegCmd[PASS_NAME_FULL].fsDecisionParamsC8.u32All     = 0x080f8010;
    pRegCmd[PASS_NAME_FULL].fsDecisionParamsOOFC.u32All   = 0x02020001;
    pRegCmd[PASS_NAME_FULL].a3T1T2Scale.u32All            = 0x001c71ef;
    pRegCmd[PASS_NAME_FULL].a3T1OFFS.u32All               = 0x00000000;
    pRegCmd[PASS_NAME_FULL].a3T2OFFS.u32All               = 0x00000000;
    pRegCmd[PASS_NAME_FULL].a2MinMax.u32All               = 0xfab4fa7d;
    pRegCmd[PASS_NAME_FULL].a2Slope.u32All                = 0x00002440;
    pRegCmd[PASS_NAME_FULL].fsToA1A4Map0.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_FULL].fsToA1A4Map1.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_FULL].fsToA1A4Map2.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_FULL].fsToA1A4Map3.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_FULL].fsToA1A4Map4.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_FULL].fsToA1A4Map5.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_FULL].fsToA1A4Map6.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_FULL].fsToA1A4Map7.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_FULL].fsToA1A4Map8.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_FULL].constantBlendingFactor.u32All = 0x00008080;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF20Titan480::RunCalculationDS4Pass
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPETF20Titan480::RunCalculationDS4Pass()
{
    CamxResult result = CamxResultSuccess;
    IPETFRegCmd480* pRegCmd = static_cast<IPETFRegCmd480*>(m_pRegCmd);

    pRegCmd[PASS_NAME_DC_4].config0.u32All                = 0x401a4624;
    pRegCmd[PASS_NAME_DC_4].config1.u32All                = 0x00008140;
    pRegCmd[PASS_NAME_DC_4].erodeConfig.u32All            = 0x000821cb;
    pRegCmd[PASS_NAME_DC_4].dilateConfig.u32All           = 0x000021cd;
    pRegCmd[PASS_NAME_DC_4].cropInHorizStart.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cropInHorizEnd.u32All         = 0x0003e4f9;
    pRegCmd[PASS_NAME_DC_4].lnrStartIDXH.u32All           = 0x001ab280;
    pRegCmd[PASS_NAME_DC_4].usCrop.u32All                 = 0x00002042;
    pRegCmd[PASS_NAME_DC_4].indCropConfig.u32All          = 0x0003a401;
    pRegCmd[PASS_NAME_DC_4].prngSeed.u32All               = 0x00000001;
    pRegCmd[PASS_NAME_DC_4].refCfg0.u32All                = 0x000000f1;
    pRegCmd[PASS_NAME_DC_4].refCfg1.u32All                = 0x00001474;
    pRegCmd[PASS_NAME_DC_4].refCfg2.u32All                = 0x000ee804;
    pRegCmd[PASS_NAME_DC_4].cropInvertStart.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cropInvertEnd.u32All          = 0x0086c21b;
    pRegCmd[PASS_NAME_DC_4].lnrStartIDXV.u32All           = 0x001d0468;
    pRegCmd[PASS_NAME_DC_4].lnrScale.u32All               = 0x02d402d4;
    pRegCmd[PASS_NAME_DC_4].cropOutVert.u32All            = 0x00043600;
    pRegCmd[PASS_NAME_DC_4].refYCfg.u32All                = 0x02120006;
    pRegCmd[PASS_NAME_DC_4].tdNtNpYContrib0.u32All        = 0x01c07015;
    pRegCmd[PASS_NAME_DC_4].tdNtNpYContrib1.u32All        = 0x01c07015;
    pRegCmd[PASS_NAME_DC_4].tdNtNpYContrib2.u32All        = 0x01c07015;
    pRegCmd[PASS_NAME_DC_4].tdNtNpYContrib3.u32All        = 0x01c07015;
    pRegCmd[PASS_NAME_DC_4].tdNtNpYContrib4.u32All        = 0x01c07015;
    pRegCmd[PASS_NAME_DC_4].tdNtNpYContrib5.u32All        = 0x01c07015;
    pRegCmd[PASS_NAME_DC_4].tdNtNpYContrib6.u32All        = 0x01c07015;
    pRegCmd[PASS_NAME_DC_4].tdNtNpYContrib7.u32All        = 0x01c07015;
    pRegCmd[PASS_NAME_DC_4].tdNtNpYContrib8.u32All        = 0x01c07015;
    pRegCmd[PASS_NAME_DC_4].tdNtNpYContrib9.u32All        = 0x01c07015;
    pRegCmd[PASS_NAME_DC_4].tdNtNpYContrib10.u32All       = 0x01c07015;
    pRegCmd[PASS_NAME_DC_4].tdNtNpYContrib11.u32All       = 0x01c07015;
    pRegCmd[PASS_NAME_DC_4].tdNtNpYContrib12.u32All       = 0x01c07015;
    pRegCmd[PASS_NAME_DC_4].tdNtNpYContrib13.u32All       = 0x01c07015;
    pRegCmd[PASS_NAME_DC_4].tdNtNpYContrib14.u32All       = 0x01c07015;
    pRegCmd[PASS_NAME_DC_4].tdNtNpYContrib15.u32All       = 0x01c07015;
    pRegCmd[PASS_NAME_DC_4].tdNtNpYContrib16.u32All       = 0x01c07015;
    pRegCmd[PASS_NAME_DC_4].tdNtNpCContribY0.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].tdNtNpCContribY1.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].tdNtNpCContribY2.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].tdNtNpCContribY3.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].tdNtNpCContribCB0.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].tdNtNpCContribCB1.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].tdNtNpCContribCB2.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].tdNtNpCContribCB3.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].tdNtNpCContribCR0.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].tdNtNpCContribCR1.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].tdNtNpCContribCR2.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].tdNtNpCContribCR3.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].tdNtNpUVLimit.u32All          = 0x07ffffff;
    pRegCmd[PASS_NAME_DC_4].tdNtNpTopLimit.u32All         = 0x01c07005;
    pRegCmd[PASS_NAME_DC_4].tdNtNpBottomLimit.u32All      = 0x01c07005;
    pRegCmd[PASS_NAME_DC_4].tdNtLnrLutY0.u32All           = 0x86838180;
    pRegCmd[PASS_NAME_DC_4].tdNtLnrLutY1.u32All           = 0x98948f89;
    pRegCmd[PASS_NAME_DC_4].tdNtLnrLutY2.u32All           = 0xafa8a19d;
    pRegCmd[PASS_NAME_DC_4].tdNtLnrLutY3.u32All           = 0xe1d2c4b8;
    pRegCmd[PASS_NAME_DC_4].tdNtLnrLutC0.u32All           = 0x86838180;
    pRegCmd[PASS_NAME_DC_4].tdNtLnrLutC1.u32All           = 0x97938e89;
    pRegCmd[PASS_NAME_DC_4].tdNtLnrLutC2.u32All           = 0xafa8a19c;
    pRegCmd[PASS_NAME_DC_4].tdNtLnrLutC3.u32All           = 0xe1d3c4b9;
    pRegCmd[PASS_NAME_DC_4].fsDecisionParamsY0.u32All     = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].fsDecisionParamsY1.u32All     = 0x00020002;
    pRegCmd[PASS_NAME_DC_4].fsDecisionParamsY2.u32All     = 0x00040004;
    pRegCmd[PASS_NAME_DC_4].fsDecisionParamsY3.u32All     = 0x00060006;
    pRegCmd[PASS_NAME_DC_4].fsDecisionParamsY4.u32All     = 0x00080008;
    pRegCmd[PASS_NAME_DC_4].fsDecisionParamsY5.u32All     = 0x0009800A;
    pRegCmd[PASS_NAME_DC_4].fsDecisionParamsY6.u32All     = 0x000B800C;
    pRegCmd[PASS_NAME_DC_4].fsDecisionParamsY7.u32All     = 0x000D800E;
    pRegCmd[PASS_NAME_DC_4].fsDecisionParamsY8.u32All     = 0x000F8010;
    pRegCmd[PASS_NAME_DC_4].fsDecisionParamsOOFY.u32All   = 0x02020001;
    pRegCmd[PASS_NAME_DC_4].fsDecisionParamsC0.u32All     = 0x08000000;
    pRegCmd[PASS_NAME_DC_4].fsDecisionParamsC1.u32All     = 0x08018002;
    pRegCmd[PASS_NAME_DC_4].fsDecisionParamsC2.u32All     = 0x08038004;
    pRegCmd[PASS_NAME_DC_4].fsDecisionParamsC3.u32All     = 0x08058006;
    pRegCmd[PASS_NAME_DC_4].fsDecisionParamsC4.u32All     = 0x08078008;
    pRegCmd[PASS_NAME_DC_4].fsDecisionParamsC5.u32All     = 0x0809800a;
    pRegCmd[PASS_NAME_DC_4].fsDecisionParamsC6.u32All     = 0x080b800c;
    pRegCmd[PASS_NAME_DC_4].fsDecisionParamsC7.u32All     = 0x080d800e;
    pRegCmd[PASS_NAME_DC_4].fsDecisionParamsC8.u32All     = 0x080f8010;
    pRegCmd[PASS_NAME_DC_4].fsDecisionParamsOOFC.u32All   = 0x02020001;
    pRegCmd[PASS_NAME_DC_4].a3T1T2Scale.u32All            = 0x001c71ef;
    pRegCmd[PASS_NAME_DC_4].a3T1OFFS.u32All               = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].a3T2OFFS.u32All               = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].a2MinMax.u32All               = 0xfab4fa19;
    pRegCmd[PASS_NAME_DC_4].a2Slope.u32All                = 0x00002472;
    pRegCmd[PASS_NAME_DC_4].fsToA1A4Map0.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_DC_4].fsToA1A4Map1.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_DC_4].fsToA1A4Map2.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_DC_4].fsToA1A4Map3.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_DC_4].fsToA1A4Map4.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_DC_4].fsToA1A4Map5.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_DC_4].fsToA1A4Map6.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_DC_4].fsToA1A4Map7.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_DC_4].fsToA1A4Map8.u32All           = 0x80808080;
    pRegCmd[PASS_NAME_DC_4].constantBlendingFactor.u32All = 0x00008080;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF20Titan480::RunCalculationDS16Pass
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPETF20Titan480::RunCalculationDS16Pass()
{
    CamxResult result = CamxResultSuccess;
    IPETFRegCmd480* pRegCmd = static_cast<IPETFRegCmd480*>(m_pRegCmd);

    pRegCmd[PASS_NAME_DC_16].config0.u32All              = 0x401a4230;
    pRegCmd[PASS_NAME_DC_16].config1.u32All              = 0x00008140;
    pRegCmd[PASS_NAME_DC_16].erodeConfig.u32All          = 0x00000003;
    pRegCmd[PASS_NAME_DC_16].dilateConfig.u32All         = 0x00000005;
    pRegCmd[PASS_NAME_DC_16].cropInHorizStart.u32All     = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cropInHorizEnd.u32All       = 0x0003bcef;
    pRegCmd[PASS_NAME_DC_16].lnrStartIDXH.u32All         = 0x001ab280;
    pRegCmd[PASS_NAME_DC_16].usCrop.u32All               = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].indCropConfig.u32All        = 0x0003bc01;
    pRegCmd[PASS_NAME_DC_16].prngSeed.u32All             = 0x00000001;
    pRegCmd[PASS_NAME_DC_16].refCfg0.u32All              = 0x000010f1;
    pRegCmd[PASS_NAME_DC_16].refCfg1.u32All              = 0x000004D7;
    pRegCmd[PASS_NAME_DC_16].refCfg2.u32All              = 0x000ee801;
    pRegCmd[PASS_NAME_DC_16].cropInvertStart.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cropInvertEnd.u32All        = 0x0021c087;
    pRegCmd[PASS_NAME_DC_16].lnrStartIDXV.u32All         = 0x001d0468;
    pRegCmd[PASS_NAME_DC_16].lnrScale.u32All             = 0x0b500b50;
    pRegCmd[PASS_NAME_DC_16].cropOutVert.u32All          = 0x00010e00;
    pRegCmd[PASS_NAME_DC_16].refYCfg.u32All              = 0x02108002;
    pRegCmd[PASS_NAME_DC_16].tdNtNpYContrib0.u32All      = 0x00c03008;
    pRegCmd[PASS_NAME_DC_16].tdNtNpYContrib1.u32All      = 0x00c03008;
    pRegCmd[PASS_NAME_DC_16].tdNtNpYContrib2.u32All      = 0x00c03008;
    pRegCmd[PASS_NAME_DC_16].tdNtNpYContrib3.u32All      = 0x00c03008;
    pRegCmd[PASS_NAME_DC_16].tdNtNpYContrib4.u32All      = 0x00c03008;
    pRegCmd[PASS_NAME_DC_16].tdNtNpYContrib5.u32All      = 0x00c03008;
    pRegCmd[PASS_NAME_DC_16].tdNtNpYContrib6.u32All      = 0x00c03008;
    pRegCmd[PASS_NAME_DC_16].tdNtNpYContrib7.u32All      = 0x00c03008;
    pRegCmd[PASS_NAME_DC_16].tdNtNpYContrib8.u32All      = 0x00c03008;
    pRegCmd[PASS_NAME_DC_16].tdNtNpYContrib9.u32All      = 0x00c03008;
    pRegCmd[PASS_NAME_DC_16].tdNtNpYContrib10.u32All     = 0x00c03008;
    pRegCmd[PASS_NAME_DC_16].tdNtNpYContrib11.u32All     = 0x00c03008;
    pRegCmd[PASS_NAME_DC_16].tdNtNpYContrib12.u32All     = 0x00c03008;
    pRegCmd[PASS_NAME_DC_16].tdNtNpYContrib13.u32All     = 0x00c03008;
    pRegCmd[PASS_NAME_DC_16].tdNtNpYContrib14.u32All     = 0x00c03008;
    pRegCmd[PASS_NAME_DC_16].tdNtNpYContrib15.u32All     = 0x00c03008;
    pRegCmd[PASS_NAME_DC_16].tdNtNpYContrib16.u32All     = 0x00c03008;
    pRegCmd[PASS_NAME_DC_16].tdNtNpCContribY0.u32All     = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].tdNtNpCContribY1.u32All     = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].tdNtNpCContribY2.u32All     = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].tdNtNpCContribY3.u32All     = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].tdNtNpCContribCB0.u32All    = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].tdNtNpCContribCB1.u32All    = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].tdNtNpCContribCB2.u32All    = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].tdNtNpCContribCB3.u32All    = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].tdNtNpCContribCR0.u32All    = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].tdNtNpCContribCR1.u32All    = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].tdNtNpCContribCR2.u32All    = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].tdNtNpCContribCR3.u32All    = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].tdNtNpUVLimit.u32All        = 0x07ffffff;
    pRegCmd[PASS_NAME_DC_16].tdNtNpTopLimit.u32All       = 0x00c03004;
    pRegCmd[PASS_NAME_DC_16].tdNtNpBottomLimit.u32All    = 0x00c03004;
    pRegCmd[PASS_NAME_DC_16].tdNtLnrLutY0.u32All         = 0x86838180;
    pRegCmd[PASS_NAME_DC_16].tdNtLnrLutY1.u32All         = 0x98948f89;
    pRegCmd[PASS_NAME_DC_16].tdNtLnrLutY2.u32All         = 0xafa8a19d;
    pRegCmd[PASS_NAME_DC_16].tdNtLnrLutY3.u32All         = 0xe1d2c4b8;
    pRegCmd[PASS_NAME_DC_16].tdNtLnrLutC0.u32All         = 0x86838180;
    pRegCmd[PASS_NAME_DC_16].tdNtLnrLutC1.u32All         = 0x97938e89;
    pRegCmd[PASS_NAME_DC_16].tdNtLnrLutC2.u32All         = 0xafa8a19c;
    pRegCmd[PASS_NAME_DC_16].tdNtLnrLutC3.u32All         = 0xe1d3c4b9;
    pRegCmd[PASS_NAME_DC_16].fsDecisionParamsY0.u32All   = 0x00880008;
    pRegCmd[PASS_NAME_DC_16].fsDecisionParamsY1.u32All   = 0x01680008;
    pRegCmd[PASS_NAME_DC_16].fsDecisionParamsY2.u32All   = 0x02680008;
    pRegCmd[PASS_NAME_DC_16].fsDecisionParamsY3.u32All   = 0x03480008;
    pRegCmd[PASS_NAME_DC_16].fsDecisionParamsY4.u32All   = 0x04480008;
    pRegCmd[PASS_NAME_DC_16].fsDecisionParamsY5.u32All   = 0x0529800a;
    pRegCmd[PASS_NAME_DC_16].fsDecisionParamsY6.u32All   = 0x062b800c;
    pRegCmd[PASS_NAME_DC_16].fsDecisionParamsY7.u32All   = 0x070d800e;
    pRegCmd[PASS_NAME_DC_16].fsDecisionParamsY8.u32All   = 0x080f8010;
    pRegCmd[PASS_NAME_DC_16].fsDecisionParamsOOFY.u32All = 0x02020001;
    pRegCmd[PASS_NAME_DC_16].fsDecisionParamsC0.u32All   = 0x08000000;
    pRegCmd[PASS_NAME_DC_16].fsDecisionParamsC1.u32All   = 0x08018002;
    pRegCmd[PASS_NAME_DC_16].fsDecisionParamsC2.u32All   = 0x08038004;
    pRegCmd[PASS_NAME_DC_16].fsDecisionParamsC3.u32All   = 0x08058006;
    pRegCmd[PASS_NAME_DC_16].fsDecisionParamsC4.u32All   = 0x08078008;
    pRegCmd[PASS_NAME_DC_16].fsDecisionParamsC5.u32All   = 0x0809800a;
    pRegCmd[PASS_NAME_DC_16].fsDecisionParamsC6.u32All   = 0x080b800c;
    pRegCmd[PASS_NAME_DC_16].fsDecisionParamsC7.u32All   = 0x080d800e;
    pRegCmd[PASS_NAME_DC_16].fsDecisionParamsC8.u32All   = 0x080f8010;
    pRegCmd[PASS_NAME_DC_16].fsDecisionParamsOOFC.u32All = 0x02020001;
    pRegCmd[PASS_NAME_DC_16].a3T1T2Scale.u32All          = 0x001c71ef;
    pRegCmd[PASS_NAME_DC_16].a3T1OFFS.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].a3T2OFFS.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].a2MinMax.u32All             = 0xfab4fa19;
    pRegCmd[PASS_NAME_DC_16].a2Slope.u32All              = 0x00002472;
    pRegCmd[PASS_NAME_DC_16].fsToA1A4Map0.u32All         = 0x80808080;
    pRegCmd[PASS_NAME_DC_16].fsToA1A4Map1.u32All         = 0x80808080;
    pRegCmd[PASS_NAME_DC_16].fsToA1A4Map2.u32All         = 0x80808080;
    pRegCmd[PASS_NAME_DC_16].fsToA1A4Map3.u32All         = 0x80808080;
    pRegCmd[PASS_NAME_DC_16].fsToA1A4Map4.u32All         = 0x80808080;
    pRegCmd[PASS_NAME_DC_16].fsToA1A4Map5.u32All         = 0x80808080;
    pRegCmd[PASS_NAME_DC_16].fsToA1A4Map6.u32All         = 0x80808080;
    pRegCmd[PASS_NAME_DC_16].fsToA1A4Map7.u32All         = 0x80808080;
    pRegCmd[PASS_NAME_DC_16].fsToA1A4Map8.u32All         = 0x80808080;

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF20Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPETF20Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result     = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer = NULL;
    IPETFRegCmd480*        pRegCmd = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if ((NULL != pInputData)                  &&
        (NULL != pInputData->pipelineIPEData.ppIPECmdBuffer) &&
        (NULL != m_pRegCmd))
    {
        pCmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferNPS];

        for (UINT passNumber = PASS_NAME_FULL; passNumber <= PASS_NAME_DC_64; passNumber++)
        {
            pRegCmd = static_cast<IPETFRegCmd480*>(m_pRegCmd);
            m_offsetPass[passNumber] = (pCmdBuffer->GetResourceUsedDwords() * RegisterWidthInBytes);

            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIPE_IPE_0_NPS_CLC_TF_TF_CONFIG_0,
                                                  IPETFReg480Length,
                                                  reinterpret_cast<UINT32*>(&pRegCmd[passNumber]));

            if (CamxResultSuccess != result)
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Failed to write command buffer");
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid input data");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF20Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPETF20Titan480::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutput)
{
    CamxResult result = CamxResultSuccess;
    INT32 intRet = NC_LIB_SUCCESS;
    TF20InputData*  pTFInput = static_cast<TF20InputData*>(pInput);
    tf_2_0_0::chromatix_tf20Type::enable_sectionStruct* pModuleEnable  = &(pTFInput->pChromatix->enable_section);

    CAMX_UNREFERENCED_PARAM(pOutput);

    if ((NULL != pInput))
    {
        IPETFRegCmd480* pRegs = static_cast<IPETFRegCmd480*>(m_pRegCmd);

        RefinementParameters* pFwRefineParams = static_cast<RefinementParameters*>(pTFInput->pRefinementParameters);
        TfParameters*         pFwTFParams     = static_cast<TfParameters*>(pTFInput->pTFParameters);
        TF_Chromatix*         pTFChromatix    = static_cast<TF_Chromatix*>(pTFInput->pNCChromatix);
        NCLIB_CONTEXT_TF      nclibContext    = {0};
        LmcParameters*        pLMCParameters  = static_cast<LmcParameters*>(pTFInput->pLMCParameters);

        RefinementPassParameters* pFwStructRefinement = &(pFwRefineParams->dc[0]);
        TfPassParameters*         pFwStructTf         = &(pFwTFParams->parameters[0]);

        CAMX_ASSERT(pTFInput->maxUsedPasses <= PASS_NAME_MAX);
        CAMX_ASSERT(NULL != pFwStructRefinement);
        CAMX_ASSERT(NULL != pFwStructTf);

        IQSettingUtils::Memset(pFwStructTf, 0, sizeof(TfPassParameters) * PASS_NAME_MAX);
        IQSettingUtils::Memset(pFwStructRefinement, 0, sizeof(RefinementPassParameters) * (PASS_NAME_MAX - 1));
        IQSettingUtils::Memset(pRegs, 0x0, sizeof(IPETFRegCmd480) * PASS_NAME_MAX);

        // pFwTFParams is the same to m_pIQSettings->tfParameters
        for (UINT32 pass = 0; pass < pTFInput->maxUsedPasses; pass++)
        {
            pFwTFParams->parameters[pass].moduleCfg.EN = pTFChromatix[pass].en;
        }

        nclibContext.fullPassIcaOutputFrameWidth  = pTFInput->fullPassIcaOutputFrameWidth;
        nclibContext.fullPassIcaOutputFrameHeight = pTFInput->fullPassIcaOutputFrameHeight;
        nclibContext.frameNumber                  = pTFInput->mfFrameNum;
        nclibContext.numberOfFrames               = pTFInput->numOfFrames;
        nclibContext.tfHasReferenceInput          = pTFInput->hasTFRefInput;
        nclibContext.upscalingFactorMFSR_ICA      = pTFInput->upscalingFactorMFSR;
        nclibContext.preferPaddingByReflection    = 0;
        nclibContext.isDigitalZoomEnabled         = pTFInput->isDigitalZoomEnabled;

        if (nclibContext.isDigitalZoomEnabled)
        {
            UINT32 frameWidth  = ((static_cast<UINT32>(static_cast<FLOAT>(nclibContext.fullPassIcaOutputFrameWidth) *
                nclibContext.upscalingFactorMFSR_ICA) + 1) >> 1) << 1;  // make it even
            UINT32 frameHeight = ((static_cast<UINT32>(static_cast<FLOAT>(nclibContext.fullPassIcaOutputFrameHeight) *
                nclibContext.upscalingFactorMFSR_ICA) + 1) >> 1) << 1;  // make it even

            nclibContext.dzStartX                  = pTFInput->digitalZoomStartX;
            nclibContext.dzStartY                  = pTFInput->digitalZoomStartY;
            nclibContext.dzSizeX                   = frameWidth;
            nclibContext.dzSizeY                   = frameHeight;
            nclibContext.preferPaddingByReflection = 1;
        }

        if (pTFInput->useCase == CONFIG_VIDEO)
        {
            nclibContext.tfConfigOption = IPE_CONFIG_VIDEO;
        }
        else if (pTFInput->useCase == CONFIG_STILL)
        {
            // MF_CONFIG_NONE / MF_CONFIG_POSTPROCESS
            nclibContext.tfConfigOption = IPE_CONFIG_STILL;
            if (pTFInput->configMF == MF_CONFIG_PREFILT)
            {
                nclibContext.tfHasReferenceInput = 0;
                nclibContext.tfConfigOption      = IPE_CONFIG_MF_PRE;
                nclibContext.numberOfFrames      = pTFInput->numOfFrames;
            }
            else if (pTFInput->configMF == MF_CONFIG_TEMPORAL)
            {
                nclibContext.tfHasReferenceInput = 1;
                nclibContext.numberOfFrames      = pTFInput->numOfFrames;
                nclibContext.tfConfigOption      = IPE_CONFIG_MF_TEMPORAL;
            }
        }
        else
        {
            /// @todo (CAMX-1812) Need to add logging for Common library
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input use case %d not supported", pTFInput->useCase);
            result = CamxResultEFailed;
        }
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "  ref input %d", nclibContext.tfHasReferenceInput);
        nclibContext.testMode = HwEnvironment::GetInstance()->GetStaticSettings()->tfTestMode;
        if (CamxResultSuccess == result)
        {
            if (NULL != pTFInput->pWarpGeometriesOutput)
            {
                for (UINT32 passType = 0; passType < pTFInput->maxUsedPasses; passType++)
                {
                    // Override the lnr opt center x / y coordinates with Warp Geometries Output
                    NcLibWarpGeomOut* pWarpDataOutput = static_cast<NcLibWarpGeomOut*>(pTFInput->pWarpGeometriesOutput);
                    pTFChromatix[passType].lnr_opt_center_x = pWarpDataOutput->tf_lnr_opt_center_x;
                    pTFChromatix[passType].lnr_opt_center_y = pWarpDataOutput->tf_lnr_opt_center_y;
                }
            }
            /* for (UINT32 passType = 0; passType < pInput->maxUsedPasses; passType++)
            {
                INT32 validateRes = Validate_TF_Chromatix(&pTFChromatix[passType]);
                if (0 != validateRes)
                {
                    /// @todo (CAMX-1812) Need to add logging for Common library
                    CAMX_LOG_WARN(CamxLogGroupIQMod, "Chromatix validation, number of invalid paramters: %d", validateRes);
                    result = FALSE;
                    break;
                }
            } */

            intRet = TF_ProcessNcLibFull480(pTFChromatix,
                                         &nclibContext,
                                         pTFInput->maxUsedPasses,
                                         pTFInput->perspectiveConfidence,
                                         pRegs,
                                         pFwRefineParams,
                                         pFwTFParams,
                                         pLMCParameters);
            if (NC_LIB_SUCCESS != intRet)
            {
                result = CamxResultEFailed;
                /// @todo (CAMX-1812) Need to add logging for Common library
                CAMX_LOG_WARN(CamxLogGroupIQMod, "TF_ProcessNcLibFull failed");
            }

            /* for (UINT32 passType = 0; passType < pInput->maxUsedPasses; passType++)
            {
                if (TRUE == pRegPass[passType].en)
                {
                    validateRes = Validate_TF_REG(&pRegPass[passType]);
                    if (0 != validateRes)
                    {
                        /// @todo (CAMX-1812) Need to add logging for Common library
                        CAMX_LOG_WARN(CamxLogGroupIQMod, "passType=%d regs validation failed %d", passType, validateRes);
                    }

                    intRet = ValidateTfCalc(&pFwStructTf[passType], pRefinementPassParams, curPassScalingRatioLog4);
                    if (NC_LIB_SUCCESS != intRet)
                    {
                        /// @todo (CAMX-1812) Need to add logging for Common library
                        CAMX_LOG_WARN(CamxLogGroupIQMod, "passType=%d ValidateTfCalc failed", passType);
                    }
                }
            } */
        }
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "pInput=%p", pTFInput);
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF20Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPETF20Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pInput);

    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Function not implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF20Titan480::GetRegSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPETF20Titan480::GetRegSize()
{
    return sizeof(IPETFRegCmd480);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF20Titan480::~IPETF20Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPETF20Titan480::~IPETF20Titan480()
{
    if (NULL != m_pRegCmd)
    {
        CAMX_FREE(m_pRegCmd);
        m_pRegCmd = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPETF20Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPETF20Titan480::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    IPETuningMetadata* pIPETuningMetadata = static_cast<IPETuningMetadata*>(pInputData->pIPETuningMetadata);
    IpeIQSettings*     pIPEIQSettings     = static_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);
    IPETFRegCmd480*    pRegCmd            = static_cast<IPETFRegCmd480*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if ((NULL != pInputData->pIPETuningMetadata) && (NULL != pRegCmd))
    {
        CAMX_STATIC_ASSERT((sizeof(IPETFRegCmd480) * PASS_NAME_MAX) ==
            sizeof(pIPETuningMetadata->IPETuningMetadata480.IPETFData.TFData));


        for (UINT passNumber = PASS_NAME_FULL; passNumber < PASS_NAME_MAX; passNumber++)
        {
            Utils::Memcpy(&pIPETuningMetadata->IPETuningMetadata480.IPETFData.TFData[passNumber],
                          &pRegCmd[passNumber],
                          sizeof(IPETFRegCmd480));
        }

        if (TRUE == pIPEIQSettings->tfParameters.parameters[0].moduleCfg.EN)
        {
            DebugDataTagID dataTagID;
            if (TRUE == pInputData->pipelineIPEData.realtimeFlag)
            {
                dataTagID = DebugDataTagID::TuningIPETF20Register;
            }
            else
            {
                dataTagID = DebugDataTagID::TuningIPETF20RegisterOffline;
            }

            result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                dataTagID,
                DebugDataTagType::TuningTF20Config,
                CAMX_ARRAY_SIZE(pIPETuningMetadata->IPETuningMetadata480.IPETFData.TFData),
                &pIPETuningMetadata->IPETuningMetadata480.IPETFData.TFData,
                sizeof(pIPETuningMetadata->IPETuningMetadata480.IPETFData.TFData));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPETF20Titan480::SetupInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPETF20Titan480::SetupInternalData(
    VOID* pData)
{
    CamxResult result = CamxResultSuccess;

    ISPInternalData*  pCalculatedData = NULL;
    IPETFRegCmd480* pRegCmd           = NULL;
    BOOL              bypassMode      = FALSE;
    TF20internalData* pInternalData = reinterpret_cast<TF20internalData*>(pData);
    IpeIQSettings*  pIPEIQSettings    =
        reinterpret_cast<IpeIQSettings*>(pInternalData->pISPInputData->pipelineIPEData.pIPEIQSettings);
    ISPInputData* pInputData          = pInternalData->pISPInputData;
    TF20InputData* pDependenceData      = pInternalData->pDependenceData;

    Utils::Memcpy(pInternalData->pOffsetPass, m_offsetPass, (sizeof(UINT)*PASS_NAME_MAX));

    if ((NoiseReductionModeOff     == pInputData->pHALTagsData->noiseReductionMode) ||
        (NoiseReductionModeMinimal == pInputData->pHALTagsData->noiseReductionMode) ||
        ((FALSE == pInputData->pipelineIPEData.isLowResolution) &&
        (NoiseReductionModeZeroShutterLag == pInputData->pHALTagsData->noiseReductionMode)))
    {
        bypassMode                = TRUE;
    }

    if ((NULL != pData) && (NULL != m_pRegCmd) && (NULL != pIPEIQSettings))
    {
        pRegCmd = static_cast<IPETFRegCmd480*>(m_pRegCmd);

        for (UINT32 passType = PASS_NAME_FULL; passType <= PASS_NAME_DC_64; passType++)
        {
            FillFirmwareConfig0(pIPEIQSettings, passType);
            FillFirmwareConfig1(pIPEIQSettings, passType, bypassMode);

            pIPEIQSettings->tfParameters.parameters[passType].morphErode     =
                static_cast<TfIndMorphology>(pRegCmd[passType].erodeConfig.bitfields.MORPH_ERODE_SIZE);
            pIPEIQSettings->tfParameters.parameters[passType].morphDilate    =
                static_cast<TfIndMorphology>(pRegCmd[passType].dilateConfig.bitfields.MORPH_DILATE_SIZE);

            pIPEIQSettings->tfParameters.parameters[passType].lnrStartIdxH   =
                (0x1fffff & pRegCmd[passType].lnrStartIDXH.bitfields.LNRSTARTIDXH) |
                (0x100000 & pRegCmd[passType].lnrStartIDXH.bitfields.LNRSTARTIDXH ? 0xffe00000 : 0);
            pIPEIQSettings->tfParameters.parameters[passType].lnrStartIdxV   =
                (0x1fffff & pRegCmd[passType].lnrStartIDXV.bitfields.LNRSTARTIDXV) |
                (0x100000 & pRegCmd[passType].lnrStartIDXV.bitfields.LNRSTARTIDXV ? 0xffe00000 : 0);
            pIPEIQSettings->tfParameters.parameters[passType].lnrScaleH      =
                static_cast<INT32>(pRegCmd[passType].lnrScale.bitfields.LNRSCALEH);
            pIPEIQSettings->tfParameters.parameters[passType].lnrScaleV      =
                static_cast<INT32>(pRegCmd[passType].lnrScale.bitfields.LNRSCALEV);

            if (bypassMode == FALSE)
            {
                pIPEIQSettings->tfParameters.parameters[passType].a2MaxC =
                    pRegCmd[passType].a2MinMax.bitfields.A2MAXC;
                pIPEIQSettings->tfParameters.parameters[passType].a2MinC =
                    pRegCmd[passType].a2MinMax.bitfields.A2MINC;
                pIPEIQSettings->tfParameters.parameters[passType].a2MaxY =
                    pRegCmd[passType].a2MinMax.bitfields.A2MAXY;
                pIPEIQSettings->tfParameters.parameters[passType].a2MinY =
                    pRegCmd[passType].a2MinMax.bitfields.A2MINY;
                pIPEIQSettings->tfParameters.parameters[passType].a2SlopeC =
                    pRegCmd[passType].a2Slope.bitfields.A2SLOPEC;
                pIPEIQSettings->tfParameters.parameters[passType].a2SlopeY =
                    pRegCmd[passType].a2Slope.bitfields.A2SLOPEY;
            }
            else
            {
                pIPEIQSettings->tfParameters.parameters[passType].a2MaxC   = 0;
                pIPEIQSettings->tfParameters.parameters[passType].a2MinC   = 0;
                pIPEIQSettings->tfParameters.parameters[passType].a2MaxY   = 0;
                pIPEIQSettings->tfParameters.parameters[passType].a2MinY   = 0;
                pIPEIQSettings->tfParameters.parameters[passType].a2SlopeC = 0;
                pIPEIQSettings->tfParameters.parameters[passType].a2SlopeY = 0;
            }

            FillFirmwareFStoA1A4Map(pIPEIQSettings, passType);
            pIPEIQSettings->tfParameters.parameters[passType].transformConfidenceVal = 256;
            pIPEIQSettings->tfParameters.parameters[passType].enableTransformConfidence = 0;

            CAMX_LOG_INFO(CamxLogGroupIQMod, " pass %d transformConfidenceVal %d,  enableTransformConfidence %d",
                          passType,
                          pIPEIQSettings->tfParameters.parameters[passType].transformConfidenceVal,
                          pIPEIQSettings->tfParameters.parameters[passType].enableTransformConfidence);

            CAMX_LOG_INFO(CamxLogGroupIQMod, "pass %d invalidMctfpImage  %d, disable output %d, disableindica %d"
                          "useImgAsMctfpInput %d, useAnroAsImgInput %d, bneding mode %d",
                          passType, pIPEIQSettings->tfParameters.parameters[passType].invalidMctfpImage,
                          pIPEIQSettings->tfParameters.parameters[passType].disableOutputIndications,
                          pIPEIQSettings->tfParameters.parameters[passType].disableUseIndications,
                          pIPEIQSettings->tfParameters.parameters[passType].useImgAsMctfpInput,
                          pIPEIQSettings->tfParameters.parameters[passType].useAnroAsImgInput,
                          pIPEIQSettings->tfParameters.parameters[passType].blendingMode);
        }

        // Each TF pass enable is set in CalculateHWSetting from Chromatix region data.
        SetTFEnable(pInputData, pDependenceData);
        SetRefinementEnable(pInputData, pInternalData->bDisableTFRefinement);
        SetLMCEnable(pInputData, pDependenceData);
        ValidateAndCorrectTFParams(pInputData, pDependenceData, pInternalData->bValidateTFParams);

        // Post noise reduction mode metadat
        if (NULL != pInputData->pCalculatedData)
        {
            pInputData->pCalculatedData->metadata.noiseReductionMode = pInputData->pHALTagsData->noiseReductionMode;
        }


    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "NULL Input Buffer");
    }

    return result;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF20Titan480::FillFirmwareConfig0
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPETF20Titan480::FillFirmwareConfig0(
    IpeIQSettings*  pIPEIQSettings,
    UINT32          passType)
{

    IPETFRegCmd480* pRegCmd = static_cast<IPETFRegCmd480*>(m_pRegCmd);

    pIPEIQSettings->tfParameters.parameters[passType].invalidMctfpImage               =
        pRegCmd[passType].config0.bitfields.INVALIDMCTFPIMAGE;
    pIPEIQSettings->tfParameters.parameters[passType].disableOutputIndications        =
        pRegCmd[passType].config0.bitfields.DISABLEOUTPUTINDICATIONS;
    pIPEIQSettings->tfParameters.parameters[passType].disableUseIndications           =
        (pRegCmd[passType].config0.bitfields.USEINDICATIONS == 0) ? 1 : 0;
    pIPEIQSettings->tfParameters.parameters[passType].smearInputsForDecisions         =
        pRegCmd[passType].config0.bitfields.SMEARINPUTSFORDECISIONS;
    pIPEIQSettings->tfParameters.parameters[passType].useAnrForDecisions_Y            =
        pRegCmd[passType].config0.bitfields.USEANRFORDECISIONS_Y;
    pIPEIQSettings->tfParameters.parameters[passType].useAnrForDecisions_C            =
        pRegCmd[passType].config0.bitfields.USEANRFORDECISIONS_C;
    pIPEIQSettings->tfParameters.parameters[passType].enableLNR                       =
        pRegCmd[passType].config0.bitfields.ENABLELNR;
    pIPEIQSettings->tfParameters.parameters[passType].enableNoiseEstByLuma            =
        pRegCmd[passType].config0.bitfields.ENABLENOISEESTBYLUMA;
    pIPEIQSettings->tfParameters.parameters[passType].enableNoiseEstByChroma          =
        pRegCmd[passType].config0.bitfields.ENABLENOISEESTBYCHROMA;
    pIPEIQSettings->tfParameters.parameters[passType].paddingByReflection             =
        pRegCmd[passType].config0.bitfields.PADDINGBYREFLECTION;
    pIPEIQSettings->tfParameters.parameters[passType].sadYCalcMode                    =
        static_cast<TfSadYCalcMode>(pRegCmd[passType].config0.bitfields.SADYCALCMODE);
    pIPEIQSettings->tfParameters.parameters[passType].isSameBlendingForAllFrequencies =
        pRegCmd[passType].config0.bitfields.ISSAMEBLENDINGFORALLFREQUENCIES;
    pIPEIQSettings->tfParameters.parameters[passType].isSadC5x3                       =
        pRegCmd[passType].config0.bitfields.ISSADC5X3;
    pIPEIQSettings->tfParameters.parameters[passType].lnrLutShiftY                    =
        pRegCmd[passType].config0.bitfields.LNRLUTSHIFTY;
    pIPEIQSettings->tfParameters.parameters[passType].lnrLutShiftC                    =
        pRegCmd[passType].config0.bitfields.LNRLUTSHIFTC;
    pIPEIQSettings->tfParameters.parameters[passType].isDCI                           =
        pRegCmd[passType].config0.bitfields.ISDCI;
    pIPEIQSettings->tfParameters.parameters[passType].indicationsDominateFsDecision   =
        pRegCmd[passType].config0.bitfields.INDICATIONSDOMINATEFSDECISION;
    pIPEIQSettings->tfParameters.parameters[passType].applyFsRankFilter               =
        pRegCmd[passType].config0.bitfields.APPLYFSRANKFILTER;
    pIPEIQSettings->tfParameters.parameters[passType].applyFsLpf                      =
        pRegCmd[passType].config0.bitfields.APPLYFSLPF;
    pIPEIQSettings->tfParameters.parameters[passType].takeOofIndFrom                  =
        pRegCmd[passType].config0.bitfields.TAKEOOFINDFROM;
    pIPEIQSettings->tfParameters.parameters[passType].isSameBlendingForAllFrequencies =
        pRegCmd[passType].config0.bitfields.ISSAMEBLENDINGFORALLFREQUENCIES;
    pIPEIQSettings->tfParameters.parameters[passType].fsDecisionFreeParamShiftC       =
        pRegCmd[passType].config0.bitfields.FSDECISIONFREEPARAMSHIFTC;
    pIPEIQSettings->tfParameters.parameters[passType].fsDecisionFreeParamShiftY       =
        pRegCmd[passType].config0.bitfields.FSDECISIONFREEPARAMSHIFTY;
    pIPEIQSettings->tfParameters.parameters[passType].outOfFramePixelsConfidence      =
        pRegCmd[passType].config0.bitfields.OUTOFFRAMEPIXELSCONFIDENCE;
    pIPEIQSettings->tfParameters.parameters[passType].disableChromaGhostDetection     =
        pRegCmd[passType].config0.bitfields.DISABLECHROMAGHOSTDETECTION;
    pIPEIQSettings->tfParameters.parameters[passType].disableLumaGhostDetection       =
        pRegCmd[passType].config0.bitfields.DISABLELUMAGHOSTDETECTION;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF20Titan480::FillFirmwareConfig1
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPETF20Titan480::FillFirmwareConfig1(
    IpeIQSettings*  pIPEIQSettings,
    UINT32          passType,
    BOOL            bypassMode)
{
    IPETFRegCmd480* pRegCmd = static_cast<IPETFRegCmd480*>(m_pRegCmd);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "X bypassmode %d", bypassMode);
    pIPEIQSettings->tfParameters.parameters[passType].useImgAsMctfpInput              =
        pRegCmd[passType].config1.bitfields.USEIMGASMCTFPINPUT;
    pIPEIQSettings->tfParameters.parameters[passType].useAnroAsImgInput               =
        pRegCmd[passType].config1.bitfields.USEANROASIMGINPUT;
    pIPEIQSettings->tfParameters.parameters[passType].ditherCr                        =
        pRegCmd[passType].config1.bitfields.DITHERCR;
    pIPEIQSettings->tfParameters.parameters[passType].ditherCb                        =
        pRegCmd[passType].config1.bitfields.DITHERCB;
    pIPEIQSettings->tfParameters.parameters[passType].ditherY                         =
        pRegCmd[passType].config1.bitfields.DITHERY;
    pIPEIQSettings->tfParameters.parameters[passType].blendingMode                    =
        static_cast<TfBlendMode>(pRegCmd[passType].config1.bitfields.BLENDINGMODE);

    if (FALSE == bypassMode)
    {
        pIPEIQSettings->tfParameters.parameters[passType].invertTemporalBlendingWeights =
            pRegCmd[passType].config1.bitfields.INVERTTEMPORALBLENDINGWEIGHTS;
    }
    else
    {
        pIPEIQSettings->tfParameters.parameters[passType].invertTemporalBlendingWeights   = 0;
    }

    pIPEIQSettings->tfParameters.parameters[passType].enableIndicationsDecreaseFactor =
        (pRegCmd[passType].config1.bitfields.INDICATIONSDECREASEFACTOR == 16) ? 0 : 1;
    pIPEIQSettings->tfParameters.parameters[passType].indicationsDecreaseFactor       =
        pRegCmd[passType].config1.bitfields.INDICATIONSDECREASEFACTOR;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF20Titan480::FillFirmwareFStoA1A4Map
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPETF20Titan480::FillFirmwareFStoA1A4Map(
    IpeIQSettings*  pIPEIQSettings,
    UINT32          passType)
{
    IPETFRegCmd480* pRegCmd = static_cast<IPETFRegCmd480*>(m_pRegCmd);

    pIPEIQSettings->tfParameters.parameters[passType].fsToA4MapC[0] =
        pRegCmd[passType].fsToA1A4Map0.bitfields.FSTOA4MAPC_0;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA4MapY[0] =
        pRegCmd[passType].fsToA1A4Map0.bitfields.FSTOA4MAPY_0;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA1MapC[0] =
        pRegCmd[passType].fsToA1A4Map0.bitfields.FSTOA1MAPC_0;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA1MapY[0] =
        pRegCmd[passType].fsToA1A4Map0.bitfields.FSTOA1MAPY_0;

    pIPEIQSettings->tfParameters.parameters[passType].fsToA4MapC[1] =
        pRegCmd[passType].fsToA1A4Map1.bitfields.FSTOA4MAPC_1;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA4MapY[1] =
        pRegCmd[passType].fsToA1A4Map1.bitfields.FSTOA4MAPY_1;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA1MapC[1] =
        pRegCmd[passType].fsToA1A4Map1.bitfields.FSTOA1MAPC_1;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA1MapY[1] =
        pRegCmd[passType].fsToA1A4Map1.bitfields.FSTOA1MAPY_1;

    pIPEIQSettings->tfParameters.parameters[passType].fsToA4MapC[2] =
        pRegCmd[passType].fsToA1A4Map2.bitfields.FSTOA4MAPC_2;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA4MapY[2] =
        pRegCmd[passType].fsToA1A4Map2.bitfields.FSTOA4MAPY_2;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA1MapC[2] =
        pRegCmd[passType].fsToA1A4Map2.bitfields.FSTOA1MAPC_2;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA1MapY[2] =
        pRegCmd[passType].fsToA1A4Map2.bitfields.FSTOA1MAPY_2;

    pIPEIQSettings->tfParameters.parameters[passType].fsToA4MapC[3] =
        pRegCmd[passType].fsToA1A4Map3.bitfields.FSTOA4MAPC_3;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA4MapY[3] =
        pRegCmd[passType].fsToA1A4Map3.bitfields.FSTOA4MAPY_3;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA1MapC[3] =
        pRegCmd[passType].fsToA1A4Map3.bitfields.FSTOA1MAPC_3;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA1MapY[3] =
        pRegCmd[passType].fsToA1A4Map3.bitfields.FSTOA1MAPY_3;

    pIPEIQSettings->tfParameters.parameters[passType].fsToA4MapC[4] =
        pRegCmd[passType].fsToA1A4Map4.bitfields.FSTOA4MAPC_4;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA4MapY[4] =
        pRegCmd[passType].fsToA1A4Map4.bitfields.FSTOA4MAPY_4;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA1MapC[4] =
        pRegCmd[passType].fsToA1A4Map4.bitfields.FSTOA1MAPC_4;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA1MapY[4] =
        pRegCmd[passType].fsToA1A4Map4.bitfields.FSTOA1MAPY_4;

    pIPEIQSettings->tfParameters.parameters[passType].fsToA4MapC[5] =
        pRegCmd[passType].fsToA1A4Map5.bitfields.FSTOA4MAPC_5;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA4MapY[5] =
        pRegCmd[passType].fsToA1A4Map5.bitfields.FSTOA4MAPY_5;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA1MapC[5] =
        pRegCmd[passType].fsToA1A4Map5.bitfields.FSTOA1MAPC_5;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA1MapY[5] =
        pRegCmd[passType].fsToA1A4Map5.bitfields.FSTOA1MAPY_5;

    pIPEIQSettings->tfParameters.parameters[passType].fsToA4MapC[6] =
        pRegCmd[passType].fsToA1A4Map6.bitfields.FSTOA4MAPC_6;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA4MapY[6] =
        pRegCmd[passType].fsToA1A4Map6.bitfields.FSTOA4MAPY_6;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA1MapC[6] =
        pRegCmd[passType].fsToA1A4Map6.bitfields.FSTOA1MAPC_6;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA1MapY[6] =
        pRegCmd[passType].fsToA1A4Map6.bitfields.FSTOA1MAPY_6;

    pIPEIQSettings->tfParameters.parameters[passType].fsToA4MapC[7] =
        pRegCmd[passType].fsToA1A4Map7.bitfields.FSTOA4MAPC_7;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA4MapY[7] =
        pRegCmd[passType].fsToA1A4Map7.bitfields.FSTOA4MAPY_7;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA1MapC[7] =
        pRegCmd[passType].fsToA1A4Map7.bitfields.FSTOA1MAPC_7;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA1MapY[7] =
        pRegCmd[passType].fsToA1A4Map7.bitfields.FSTOA1MAPY_7;

    pIPEIQSettings->tfParameters.parameters[passType].fsToA4MapC[8] =
        pRegCmd[passType].fsToA1A4Map8.bitfields.FSTOA4MAPC_8;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA4MapY[8] =
        pRegCmd[passType].fsToA1A4Map8.bitfields.FSTOA4MAPY_8;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA1MapC[8] =
        pRegCmd[passType].fsToA1A4Map8.bitfields.FSTOA1MAPC_8;
    pIPEIQSettings->tfParameters.parameters[passType].fsToA1MapY[8] =
        pRegCmd[passType].fsToA1A4Map8.bitfields.FSTOA1MAPY_8;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF20Titan480::ValidateAndCorrectTFParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPETF20Titan480::ValidateAndCorrectTFParams(
    const ISPInputData* pInputData,
    TF20InputData* pDependenceData,
    BOOL bValidateTFParams)
{
    CamxResult result = CamxResultSuccess;


    if (TRUE == bValidateTFParams)
    {
        switch (pDependenceData->useCase)
        {
            case CONFIG_VIDEO:
                ValidateAndCorrectMCTFParameters(pInputData, pDependenceData);
                break;
            case CONFIG_STILL:
                ValidateAndCorrectStillModeParameters(pInputData, pDependenceData);
                break;
            default:
                break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF20Titan480::ValidateAndCorrectStillModeParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPETF20Titan480::ValidateAndCorrectStillModeParameters(
    const ISPInputData* pInputData,
    TF20InputData* pDependenceData)
{
    CamxResult      result         = CamxResultSuccess;
    IpeIQSettings*  pIPEIQSettings = reinterpret_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);

    // Regular Still Mode/Prefilter and Postfilter
    if ((MF_CONFIG_NONE == pDependenceData->configMF) ||
        (MF_CONFIG_POSTPROCESS == pDependenceData->configMF))
    {
        if (0 != pDependenceData->hasTFRefInput)
        {
            CAMX_LOG_WARN(CamxLogGroupIQMod, "Reference input present for still mode , invalid config");
        }

        if (TRUE == pIPEIQSettings->tfParameters.parameters[PASS_NAME_FULL].moduleCfg.EN)
        {
            ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[PASS_NAME_FULL].disableUseIndications,
                                 PASS_NAME_FULL, 1, "Still-disableUseIndications");

            ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[PASS_NAME_FULL].disableOutputIndications,
                                 PASS_NAME_FULL, 1, "Still-disableOutputIndications");

            ValidateAndSetParams(reinterpret_cast<UINT32*>(&pIPEIQSettings->tfParameters.parameters[0].blendingMode),
                                 PASS_NAME_FULL, TF_BLEND_MODE_REGULAR, "Still-blendingMode");

            ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[PASS_NAME_FULL].useImgAsMctfpInput,
                                 PASS_NAME_FULL, 1, "Still-useImgAsMctfpInput");

            ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[PASS_NAME_FULL].useAnroAsImgInput,
                                 PASS_NAME_FULL, 1, "Still-useAnroAsImgInput");

            ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[PASS_NAME_FULL].invalidMctfpImage,
                                 PASS_NAME_FULL, 0, "Still-invalidMctfpImage");
        }

        ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[PASS_NAME_DC_4].moduleCfg.EN,
                             PASS_NAME_DC_4, 0, "Still-DC4 pass");

        ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[PASS_NAME_DC_16].moduleCfg.EN,
                             PASS_NAME_DC_16, 0, "Still-DC16 pass");

        ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[PASS_NAME_DC_64].moduleCfg.EN,
                             PASS_NAME_DC_64, 0, "Still-DC64 pass");
    }
    else if (MF_CONFIG_PREFILT == pDependenceData->configMF)
    {
        if (0 != pDependenceData->hasTFRefInput)
        {
            CAMX_LOG_WARN(CamxLogGroupIQMod, "Reference input present for prefilter mode , invalid config");
        }

        for (UINT32 pass = 0; pass < pDependenceData->maxUsedPasses; pass++)
        {
            // Check if ANR pass is enabled for each pass, enable TF for same pass
            if (TRUE == pIPEIQSettings->anrParameters.parameters[pass].moduleCfg.EN)
            {
                ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].moduleCfg.EN,
                                 pass, 1, "Prefilter- TF enable");

                ValidateAndSetParams(reinterpret_cast<UINT32*>(&pIPEIQSettings->tfParameters.parameters[pass].blendingMode),
                                 pass, TF_BLEND_MODE_CONSTANT_BLENDING, "Prefilter-blendingMode");

                ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].useImgAsMctfpInput,
                                 pass, 1, "Prefilter-useImgAsMctfpInput");

                ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].useAnroAsImgInput,
                                 pass, 1, "Prefilter-useAnroAsImgInput");

                ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].invalidMctfpImage,
                                 pass, 0, "Prefilter-invalidMctfpImage");
            }
        }
    }
    // Still mode blend
    else
    {
        for (UINT32 pass = 0; pass < pDependenceData->maxUsedPasses; pass++)
        {
            if (TRUE == pIPEIQSettings->tfParameters.parameters[pass].moduleCfg.EN)
            {
                if (PASS_NAME_FULL == pass)
                {
                    ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].disableUseIndications,
                                         pass, 0, "Blend-disableUseIndications");

                    ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].disableOutputIndications,
                                         pass, 1, "Blend-disableOutputIndications");

                    ValidateAndSetParams(reinterpret_cast<UINT32*>(&pIPEIQSettings->tfParameters.parameters[pass].blendingMode),
                                         pass, 0, "Blend-blendingMode");
                }
                else if (PASS_NAME_DC_4 == pass)
                {
                    ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].disableUseIndications,
                                         pass, 0, "Blend-disableUseIndications");

                    ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].disableOutputIndications,
                                         pass, 0, "Blend-disableOutputIndications");

                    ValidateAndSetParams(reinterpret_cast<UINT32*>(&pIPEIQSettings->tfParameters.parameters[pass].blendingMode),
                                         pass, 1, "Blend-blendingMode");
                }
                else if (PASS_NAME_DC_16 == pass)
                {
                    ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].disableUseIndications,
                                         pass, 0, "Blend-disableUseIndications");

                    ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].disableOutputIndications,
                                         pass, 0, "Blend-disableOutputIndications");

                    ValidateAndSetParams(reinterpret_cast<UINT32*>(&pIPEIQSettings->tfParameters.parameters[pass].blendingMode),
                                         pass, 1, "Blend-blendingMode");
                }
                else
                {
                    ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].disableUseIndications,
                                         pass, 1, "Blend-disableUseIndications");

                    ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].disableOutputIndications,
                                         pass, 0, "Blend-disableOutputIndications");

                    ValidateAndSetParams(reinterpret_cast<UINT32*>(&pIPEIQSettings->tfParameters.parameters[pass].blendingMode),
                                         pass, 1, "Blend-blendingMode");
                }

                ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].useImgAsMctfpInput,
                                     pass, 0, "Blend-useImgAsMctfpInput");

                ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].useAnroAsImgInput,
                                     pass, 0, "Blend-useAnroAsImgInput");

                ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].invalidMctfpImage,
                                     pass, 0, "Blend-invalidMctfpImage");
            }
        }
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF20Titan480::ValidateAndCorrectMCTFParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPETF20Titan480::ValidateAndCorrectMCTFParameters(
    const ISPInputData* pInputData,
    TF20InputData* pDependenceData)
{

    CamxResult      result         = CamxResultSuccess;
    IpeIQSettings*  pIPEIQSettings = reinterpret_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);

    if ((PASS_NAME_MAX - 1) != pDependenceData->maxUsedPasses)
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "Max passes for MCTF is incorrect");
    }

    UINT32 validInvalidMCTFpImageValue = (TRUE == pDependenceData->hasTFRefInput) ? 0 : 1;

    for (UINT32 pass = 0; pass < pDependenceData->maxUsedPasses; pass++)
    {
        if (TRUE == pIPEIQSettings->tfParameters.parameters[pass].moduleCfg.EN)
        {
            if (PASS_NAME_FULL == pass)
            {
                if (0 == validInvalidMCTFpImageValue)
                {
                    ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].disableUseIndications,
                        pass, 0, "MCTF-disableUseIndications");
                }
                else
                {
                    ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].disableUseIndications,
                        pass, 1, "MCTF-disableUseIndications");
                }

                ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].disableOutputIndications,
                                     pass, 1, "MCTF-disableOutputIndications");

                ValidateAndSetParams(reinterpret_cast<UINT32*>(&pIPEIQSettings->tfParameters.parameters[pass].blendingMode),
                                     pass, 0, "MCTF-blendingMode");
            }
            else if (PASS_NAME_DC_4 == pass)
            {
                if (0 == validInvalidMCTFpImageValue)
                {
                    ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].disableUseIndications,
                        pass, 0, "MCTF-disableUseIndications");
                }
                else
                {
                    ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].disableUseIndications,
                        pass, 1, "MCTF-disableUseIndications");
                }

                ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].disableOutputIndications,
                                     pass, 0, "MCTF-disableOutputIndications");

                ValidateAndSetParams(reinterpret_cast<UINT32*>(&pIPEIQSettings->tfParameters.parameters[pass].blendingMode),
                                     pass, 1, "MCTF-blendingMode");
            }
            else if (PASS_NAME_DC_16 == pass)
            {
                ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].disableUseIndications,
                                     pass, 1, "MCTF-disableUseIndications");

                ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].disableOutputIndications,
                                     pass, 0, "MCTF-disableOutputIndications");

                ValidateAndSetParams(reinterpret_cast<UINT32*>(&pIPEIQSettings->tfParameters.parameters[pass].blendingMode),
                                     pass, 1, "MCTF-blendingMode");
            }
            else
            {
                ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[PASS_NAME_DC_64].moduleCfg.EN,
                                     PASS_NAME_DC_64, 0, "MCTF-DC64 pass");
            }

            ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].useImgAsMctfpInput,
                                 pass, 0, "MCTF-useImgAsMctfpInput");

            ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].useAnroAsImgInput,
                                 pass, 0, "MCTF-useAnroAsImgInput");

            ValidateAndSetParams(&pIPEIQSettings->tfParameters.parameters[pass].invalidMctfpImage,
                                 pass, validInvalidMCTFpImageValue, "MCTF-invalidMctfpImage");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF20Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPETF20Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DumpRegConfig ");
    CHAR  dumpFilename[256];
    FILE* pFile = NULL;
    CamX::OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename), "%s/IPE_regdump.txt", ConfigFileDirectory);
    pFile = CamX::OsUtils::FOpen(dumpFilename, "a+");

    IPETFRegCmd480* pRegCmd = static_cast<IPETFRegCmd480*>(m_pRegCmd);

    if (NULL != pFile)
    {
        CamX::OsUtils::FPrintF(pFile, "******** IPE TF20 [HEX] ********\n");
        for (UINT passType = 0; passType < PASS_NAME_MAX; passType++)
        {
            CamX::OsUtils::FPrintF(pFile, "== passType = %d ================================================\n", passType);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].config0.u32All                            = %x\n",
                passType, pRegCmd[passType].config0.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].config1.u32All                            = %x\n",
                passType, pRegCmd[passType].config1.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].erodeConfig.u32All                        = %x\n",
                passType, pRegCmd[passType].erodeConfig.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].dilateConfig.u32All                       = %x\n",
                passType, pRegCmd[passType].dilateConfig.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].cropInHorizStart.u32All                   = %x\n",
                passType, pRegCmd[passType].cropInHorizStart.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].cropInHorizStart.u32All                   = %x\n",
                passType, pRegCmd[passType].cropInHorizStart.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].cropInHorizEnd.u32All                     = %x\n",
                passType, pRegCmd[passType].cropInHorizEnd.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].lnrStartIDXH.u32All                       = %x\n",
                passType, pRegCmd[passType].lnrStartIDXH.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].usCrop.u32All                             = %x\n",
                passType, pRegCmd[passType].usCrop.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].indCropConfig.u32All                      = %x\n",
                passType, pRegCmd[passType].indCropConfig.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].prngSeed.u32All                           = %x\n",
                passType, pRegCmd[passType].prngSeed.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].refCfg0.u32All                            = %x\n",
                passType, pRegCmd[passType].refCfg0.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].refCfg1.u32All                            = %x\n",
                passType, pRegCmd[passType].refCfg1.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].refCfg2.u32All                            = %x\n",
                passType, pRegCmd[passType].refCfg2.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].cropInvertStart.u32All                    = %x\n",
                passType, pRegCmd[passType].cropInvertStart.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].cropInvertEnd.u32All                      = %x\n",
                passType, pRegCmd[passType].cropInvertEnd.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].lnrStartIDXV.u32All                       = %x\n",
                passType, pRegCmd[passType].lnrStartIDXV.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].lnrScale.u32All                           = %x\n",
                passType, pRegCmd[passType].lnrScale.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].cropOutVert.u32All                        = %x\n",
                passType, pRegCmd[passType].cropOutVert.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].refYCfg.u32All                            = %x\n",
                passType, pRegCmd[passType].refYCfg.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpYContrib0.u32All                    = %x\n",
                passType, pRegCmd[passType].tdNtNpYContrib0.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpYContrib1.u32All                    = %x\n",
                passType, pRegCmd[passType].tdNtNpYContrib1.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpYContrib2.u32All                    = %x\n",
                passType, pRegCmd[passType].tdNtNpYContrib2.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpYContrib3.u32All                    = %x\n",
                passType, pRegCmd[passType].tdNtNpYContrib3.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpYContrib4.u32All                    = %x\n",
                passType, pRegCmd[passType].tdNtNpYContrib4.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpYContrib5.u32All                    = %x\n",
                passType, pRegCmd[passType].tdNtNpYContrib5.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpYContrib6.u32All                    = %x\n",
                passType, pRegCmd[passType].tdNtNpYContrib6.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpYContrib7.u32All                    = %x\n",
                passType, pRegCmd[passType].tdNtNpYContrib7.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpYContrib8.u32All                    = %x\n",
                passType, pRegCmd[passType].tdNtNpYContrib8.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpYContrib9.u32All                    = %x\n",
                passType, pRegCmd[passType].tdNtNpYContrib9.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpYContrib10.u32All                   = %x\n",
                passType, pRegCmd[passType].tdNtNpYContrib10.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpYContrib11.u32All                   = %x\n",
                passType, pRegCmd[passType].tdNtNpYContrib12.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpYContrib12.u32All                   = %x\n",
                passType, pRegCmd[passType].tdNtNpYContrib12.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpYContrib13.u32All                   = %x\n",
                passType, pRegCmd[passType].tdNtNpYContrib13.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpYContrib14.u32All                   = %x\n",
                passType, pRegCmd[passType].tdNtNpYContrib14.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpYContrib15.u32All                   = %x\n",
                passType, pRegCmd[passType].tdNtNpYContrib15.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpYContrib16.u32All                   = %x\n",
                passType, pRegCmd[passType].tdNtNpYContrib16.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpCContribY0.u32All                   = %x\n",
                passType, pRegCmd[passType].tdNtNpCContribY0.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpCContribY1.u32All                   = %x\n",
                passType, pRegCmd[passType].tdNtNpCContribY1.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpCContribY2.u32All                   = %x\n",
                passType, pRegCmd[passType].tdNtNpCContribY2.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpCContribY3.u32All                   = %x\n",
                passType, pRegCmd[passType].tdNtNpCContribY3.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpCContribCB0.u32All                  = %x\n",
                passType, pRegCmd[passType].tdNtNpCContribCB0.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpCContribCB1.u32All                  = %x\n",
                passType, pRegCmd[passType].tdNtNpCContribCB1.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpCContribCB2.u32All                  = %x\n",
                passType, pRegCmd[passType].tdNtNpCContribCB2.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpCContribCB3.u32All                  = %x\n",
                passType, pRegCmd[passType].tdNtNpCContribCB3.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpCContribCR0.u32All                  = %x\n",
                passType, pRegCmd[passType].tdNtNpCContribCR0.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpCContribCR1.u32All                  = %x\n",
                passType, pRegCmd[passType].tdNtNpCContribCR1.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpCContribCR2.u32All                  = %x\n",
                passType, pRegCmd[passType].tdNtNpCContribCR2.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpCContribCR3.u32All                  = %x\n",
                passType, pRegCmd[passType].tdNtNpCContribCR3.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpUVLimit.u32All                      = %x\n",
                passType, pRegCmd[passType].tdNtNpUVLimit.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpTopLimit.u32All                     = %x\n",
                passType, pRegCmd[passType].tdNtNpTopLimit.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtNpBottomLimit.u32All                  = %x\n",
                passType, pRegCmd[passType].tdNtNpBottomLimit.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtLnrLutY0.u32All                       = %x\n",
                passType, pRegCmd[passType].tdNtLnrLutY0.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtLnrLutY1.u32All                       = %x\n",
                passType, pRegCmd[passType].tdNtLnrLutY1.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtLnrLutY2.u32All                       = %x\n",
                passType, pRegCmd[passType].tdNtLnrLutY2.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtLnrLutY3.u32All                       = %x\n",
                passType, pRegCmd[passType].tdNtLnrLutY3.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtLnrLutC0.u32All                       = %x\n",
                passType, pRegCmd[passType].tdNtLnrLutC0.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtLnrLutC1.u32All                       = %x\n",
                passType, pRegCmd[passType].tdNtLnrLutC1.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtLnrLutC2.u32All                       = %x\n",
                passType, pRegCmd[passType].tdNtLnrLutC2.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].tdNtLnrLutC3.u32All                       = %x\n",
                passType, pRegCmd[passType].tdNtLnrLutC3.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsDecisionParamsY0.u32All                 = %x\n",
                passType, pRegCmd[passType].fsDecisionParamsY0.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsDecisionParamsY1.u32All                 = %x\n",
                passType, pRegCmd[passType].fsDecisionParamsY1.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsDecisionParamsY2.u32All                 = %x\n",
                passType, pRegCmd[passType].fsDecisionParamsY2.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsDecisionParamsY3.u32All                 = %x\n",
                passType, pRegCmd[passType].fsDecisionParamsY3.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsDecisionParamsY4.u32All                 = %x\n",
                passType, pRegCmd[passType].fsDecisionParamsY4.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsDecisionParamsY5.u32All                 = %x\n",
                passType, pRegCmd[passType].fsDecisionParamsY5.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsDecisionParamsY6.u32All                 = %x\n",
                passType, pRegCmd[passType].fsDecisionParamsY6.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsDecisionParamsY7.u32All                 = %x\n",
                passType, pRegCmd[passType].fsDecisionParamsY7.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsDecisionParamsY8.u32All                 = %x\n",
                passType, pRegCmd[passType].fsDecisionParamsY8.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsDecisionParamsOOFY.u32All               = %x\n",
                passType, pRegCmd[passType].fsDecisionParamsOOFY.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsDecisionParamsC0.u32All                 = %x\n",
                passType, pRegCmd[passType].fsDecisionParamsC0.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsDecisionParamsC1.u32All                 = %x\n",
                passType, pRegCmd[passType].fsDecisionParamsC1.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsDecisionParamsC2.u32All                 = %x\n",
                passType, pRegCmd[passType].fsDecisionParamsC2.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsDecisionParamsC3.u32All                 = %x\n",
                passType, pRegCmd[passType].fsDecisionParamsC3.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsDecisionParamsC4.u32All                 = %x\n",
                passType, pRegCmd[passType].fsDecisionParamsC4.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsDecisionParamsC5.u32All                 = %x\n",
                passType, pRegCmd[passType].fsDecisionParamsC5.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsDecisionParamsC6.u32All                 = %x\n",
                passType, pRegCmd[passType].fsDecisionParamsC6.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsDecisionParamsC7.u32All                 = %x\n",
                passType, pRegCmd[passType].fsDecisionParamsC7.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsDecisionParamsC8.u32All                 = %x\n",
                passType, pRegCmd[passType].fsDecisionParamsC8.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsDecisionParamsOOFC.u32All               = %x\n",
                passType, pRegCmd[passType].fsDecisionParamsOOFC.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].a3T1T2Scale.u32All                        = %x\n",
                passType, pRegCmd[passType].a3T1T2Scale.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].a3T1OFFS.u32All                           = %x\n",
                passType, pRegCmd[passType].a3T1OFFS.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].a3T2OFFS.u32All                           = %x\n",
                passType, pRegCmd[passType].a3T2OFFS.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].a2MinMax.u32All                           = %x\n",
                passType, pRegCmd[passType].a2MinMax.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].a2Slope.u32All                            = %x\n",
                passType, pRegCmd[passType].a2Slope.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsToA1A4Map0.u32All                       = %x\n",
                passType, pRegCmd[passType].fsToA1A4Map0.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsToA1A4Map1.u32All                       = %x\n",
                passType, pRegCmd[passType].fsToA1A4Map1.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsToA1A4Map2.u32All                       = %x\n",
                passType, pRegCmd[passType].fsToA1A4Map2.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsToA1A4Map3.u32All                       = %x\n",
                passType, pRegCmd[passType].fsToA1A4Map3.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsToA1A4Map4.u32All                       = %x\n",
                passType, pRegCmd[passType].fsToA1A4Map4.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsToA1A4Map5.u32All                       = %x\n",
                passType, pRegCmd[passType].fsToA1A4Map5.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsToA1A4Map6.u32All                       = %x\n",
                passType, pRegCmd[passType].fsToA1A4Map6.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsToA1A4Map7.u32All                       = %x\n",
                passType, pRegCmd[passType].fsToA1A4Map7.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].fsToA1A4Map8.u32All                       = %x\n",
                passType, pRegCmd[passType].fsToA1A4Map8.u32All);
            CamX::OsUtils::FPrintF(pFile, "pRegCmd[%d].constantBlendingFactor.u32All             = %x\n",
                passType, pRegCmd[passType].constantBlendingFactor.u32All);
            CamX::OsUtils::FPrintF(pFile, "\n");
        }
        CamX::OsUtils::FPrintF(pFile, "\n\n");
        CamX::OsUtils::FClose(pFile);
    }

    for (UINT passType = 0; passType < PASS_NAME_MAX; passType++)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].config0.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].config0.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].config1.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].config1.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].erodeConfig.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].erodeConfig.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].dilateConfig.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].dilateConfig.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].cropInHorizStart.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].cropInHorizStart.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].cropInHorizEnd.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].cropInHorizEnd.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].lnrStartIDXH.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].lnrStartIDXH.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].usCrop.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].usCrop.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].indCropConfig.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].indCropConfig.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].prngSeed.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].prngSeed.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].refCfg0.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].refCfg0.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].refCfg1.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].refCfg1.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].refCfg2.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].refCfg2.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].cropInvertStart.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].cropInvertStart.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].cropInvertEnd.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].cropInvertEnd.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].lnrStartIDXV.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].lnrStartIDXV.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].lnrScale.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].lnrScale.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].cropOutVert.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].cropOutVert.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].refYCfg.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].refYCfg.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpYContrib0.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpYContrib0.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpYContrib1.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpYContrib1.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpYContrib2.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpYContrib2.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpYContrib3.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpYContrib3.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpYContrib4.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpYContrib4.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpYContrib5.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpYContrib5.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpYContrib6.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpYContrib6.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpYContrib7.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpYContrib7.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpYContrib8.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpYContrib8.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpYContrib9.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpYContrib9.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpYContrib10.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpYContrib10.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpYContrib11.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpYContrib11.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpYContrib12.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpYContrib12.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpYContrib13.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpYContrib13.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpYContrib14.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpYContrib14.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpYContrib15.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpYContrib15.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpYContrib16.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpYContrib16.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpCContribY0.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpCContribY0.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpCContribY1.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpCContribY1.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpCContribY2.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpCContribY2.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpCContribY3.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpCContribY3.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpCContribCB0.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpCContribCB0.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpCContribCB1.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpCContribCB1.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpCContribCB2.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpCContribCB2.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpCContribCB3.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpCContribCB3.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpCContribCR0.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpCContribCR0.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpCContribCR1.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpCContribCR1.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpCContribCR2.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpCContribCR2.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpCContribCR3.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpCContribCR3.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpUVLimit.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpUVLimit.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpTopLimit.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpTopLimit.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtNpBottomLimit.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtNpBottomLimit.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtLnrLutY0.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtLnrLutY0.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtLnrLutY1.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtLnrLutY1.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtLnrLutY2.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtLnrLutY2.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtLnrLutY3.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtLnrLutY3.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtLnrLutC0.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtLnrLutC0.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtLnrLutC1.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtLnrLutC1.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtLnrLutC2.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtLnrLutC2.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].tdNtLnrLutC3.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].tdNtLnrLutC3.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsDecisionParamsY0.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsDecisionParamsY0.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsDecisionParamsY1.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsDecisionParamsY1.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsDecisionParamsY2.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsDecisionParamsY2.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsDecisionParamsY3.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsDecisionParamsY3.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsDecisionParamsY4.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsDecisionParamsY4.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsDecisionParamsY5.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsDecisionParamsY5.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsDecisionParamsY6.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsDecisionParamsY6.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsDecisionParamsY7.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsDecisionParamsY7.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsDecisionParamsY8.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsDecisionParamsY8.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsDecisionParamsOOFY.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsDecisionParamsOOFY.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsDecisionParamsC0.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsDecisionParamsC0.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsDecisionParamsC1.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsDecisionParamsC1.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsDecisionParamsC2.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsDecisionParamsC2.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsDecisionParamsC3.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsDecisionParamsC3.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsDecisionParamsC4.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsDecisionParamsC4.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsDecisionParamsC5.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsDecisionParamsC5.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsDecisionParamsC6.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsDecisionParamsC6.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsDecisionParamsC7.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsDecisionParamsC7.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsDecisionParamsC8.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsDecisionParamsC8.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsDecisionParamsOOFC.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsDecisionParamsOOFC.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].a3T1T2Scale.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].a3T1T2Scale.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].a3T1OFFS.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].a3T1OFFS.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].a3T2OFFS.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].a3T2OFFS.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].a2MinMax.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].a2MinMax.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].a2Slope.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].a2Slope.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsToA1A4Map0.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsToA1A4Map0.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsToA1A4Map1.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsToA1A4Map1.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsToA1A4Map2.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsToA1A4Map2.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsToA1A4Map3.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsToA1A4Map3.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsToA1A4Map4.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsToA1A4Map4.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsToA1A4Map5.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsToA1A4Map5.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsToA1A4Map6.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsToA1A4Map6.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsToA1A4Map7.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsToA1A4Map7.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].fsToA1A4Map8.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].fsToA1A4Map8.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                      "pRegCmd[%d].constantBlendingFactor.u32All = 0x%x",
                      passType,
                      pRegCmd[passType].constantBlendingFactor.u32All);
    }

    return;
}





CAMX_NAMESPACE_END
