////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpsgamma16titan480.cpp
/// @brief CAMXBPSGAMMA16TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxbpsgamma16titan480.h"
#include "gamma16setting.h"
#include "camxiqinterface.h"
#include "titan480_bps.h"

CAMX_NAMESPACE_BEGIN

static const UINT32 Channel0LUT = BPS_BPS_0_CLC_GLUT_DMI_LUT_CFG_LUT_SEL_GLUT_CH0_LUT;
static const UINT32 Channel1LUT = BPS_BPS_0_CLC_GLUT_DMI_LUT_CFG_LUT_SEL_GLUT_CH1_LUT;
static const UINT32 Channel2LUT = BPS_BPS_0_CLC_GLUT_DMI_LUT_CFG_LUT_SEL_GLUT_CH2_LUT;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGAMMA16Titan480::BPSGAMMA16Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSGAMMA16Titan480::BPSGAMMA16Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGAMMA16Titan480::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGAMMA16Titan480::Create(
    ISPHWSetting** ppHWSetting)
{
    CamxResult result = CamxResultSuccess;

    BPSGAMMA16Titan480* pHWSetting = CAMX_NEW BPSGAMMA16Titan480;

    if (NULL != pHWSetting)
    {
        (*ppHWSetting) = pHWSetting;
    }
    else
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "%s failed, no memory", __FUNCTION__);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGAMMA16Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGAMMA16Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);
    CamxResult      result     = CamxResultSuccess;
    ISPInputData*   pInputData = static_cast<ISPInputData*>(pSettingData);

    // CDM pack the DMI buffer and patch the Red and Blue LUT DMI buffer into CDM pack
    if ((NULL != pInputData) && (NULL != pInputData->pDMICmdBuffer))
    {
        UINT32 LUTOffset = 0;
        result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                         regBPS_BPS_0_CLC_GLUT_DMI_CFG,
                                         Channel0LUT,
                                         pInputData->p64bitDMIBuffer,
                                         LUTOffset,
                                         GammaSizeLUTInBytes);

        if (CamxResultSuccess == result)
        {
            LUTOffset += GammaSizeLUTInBytes;
            result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                             regBPS_BPS_0_CLC_GLUT_DMI_CFG,
                                             Channel1LUT,
                                             pInputData->p64bitDMIBuffer,
                                             LUTOffset,
                                             GammaSizeLUTInBytes);
        }

        if (CamxResultSuccess == result)
        {
            LUTOffset += GammaSizeLUTInBytes;
            result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                             regBPS_BPS_0_CLC_GLUT_DMI_CFG,
                                             Channel2LUT,
                                             pInputData->p64bitDMIBuffer,
                                             LUTOffset,
                                             GammaSizeLUTInBytes);
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to write DMI data");
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGAMMA16Titan480::UpdateFirmwareData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGAMMA16Titan480::UpdateFirmwareData(
    VOID*   pSettingData,
    BOOL    moduleEnable)
{
    ISPInputData*  pInputData     = static_cast<ISPInputData*>(pSettingData);
    BpsIQSettings* pBPSIQSettings = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    CAMX_ASSERT(NULL != pBPSIQSettings);

    pBPSIQSettings->glutParameters.moduleCfg.EN = moduleEnable;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGAMMA16Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGAMMA16Titan480::UpdateTuningMetadata(
    VOID* pSettingData)
{
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    BPSTuningMetadata* pBPSTuningMetadata = static_cast<BPSTuningMetadata*>(pInputData->pBPSTuningMetadata);
    BpsIQSettings*     pBPSIQSettings     = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);

    // Post tuning metadata if setting is enabled
    if (NULL != pBPSTuningMetadata)
    {
        // Nothing to do for now
        CAMX_LOG_VERBOSE(CamxLogGroupBPS, "Update tables in different change")
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGAMMA16Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGAMMA16Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CAMX_UNREFERENCED_PARAM(pOutput);

    CAMX_LOG_ERROR(CamxLogGroupBPS, "%s not implemented.", __FUNCTION__);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGAMMA16Titan480::~BPSGAMMA16Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSGAMMA16Titan480::~BPSGAMMA16Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGAMMA16Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSGAMMA16Titan480::DumpRegConfig()
{
    // No register Buffer to Dump
}

CAMX_NAMESPACE_END
