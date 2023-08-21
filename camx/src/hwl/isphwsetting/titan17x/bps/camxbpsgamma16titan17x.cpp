////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpsgamma16titan17x.cpp
/// @brief CAMXBPSGAMMA16TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxbpsgamma16titan17x.h"
#include "gamma16setting.h"
#include "camxiqinterface.h"
#include "titan170_bps.h"

CAMX_NAMESPACE_BEGIN

static const UINT32 Channel0LUT = BPS_BPS_0_CLC_GLUT_DMI_LUT_CFG_LUT_SEL_GLUT_CH0_LUT;
static const UINT32 Channel1LUT = BPS_BPS_0_CLC_GLUT_DMI_LUT_CFG_LUT_SEL_GLUT_CH1_LUT;
static const UINT32 Channel2LUT = BPS_BPS_0_CLC_GLUT_DMI_LUT_CFG_LUT_SEL_GLUT_CH2_LUT;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGAMMA16Titan17x::BPSGAMMA16Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSGAMMA16Titan17x::BPSGAMMA16Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGAMMA16Titan17x::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGAMMA16Titan17x::Create(
    ISPHWSetting** ppHWSetting)
{
    CamxResult result = CamxResultSuccess;

    BPSGAMMA16Titan17x* pHWSetting = CAMX_NEW BPSGAMMA16Titan17x;

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
// BPSGAMMA16Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGAMMA16Titan17x::CreateCmdList(
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
// BPSGAMMA16Titan17x::UpdateFirmwareData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGAMMA16Titan17x::UpdateFirmwareData(
    VOID* pSettingData,
    BOOL  moduleEnable)
{
    ISPInputData*  pInputData     = static_cast<ISPInputData*>(pSettingData);
    BpsIQSettings* pBPSIQSettings = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    CAMX_ASSERT(NULL != pBPSIQSettings);

    pBPSIQSettings->glutParameters.moduleCfg.EN = moduleEnable;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGAMMA16Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGAMMA16Titan17x::UpdateTuningMetadata(
    VOID* pSettingData)
{
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    BPSTuningMetadata* pBPSTuningMetadata = static_cast<BPSTuningMetadata*>(pInputData->pBPSTuningMetadata);
    BpsIQSettings*     pBPSIQSettings     = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);

    // Post tuning metadata if setting is enabled
    if (NULL != pBPSTuningMetadata)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupBPS, "Update tables in different change");
        // Nothing to do for now
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGAMMA16Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGAMMA16Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CAMX_UNREFERENCED_PARAM(pOutput);

    CAMX_LOG_ERROR(CamxLogGroupBPS, "%s not implemented.", __FUNCTION__);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGAMMA16Titan17x::~BPSGAMMA16Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSGAMMA16Titan17x::~BPSGAMMA16Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGAMMA16Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSGAMMA16Titan17x::DumpRegConfig()
{
    // No register Buffer to Dump
}

CAMX_NAMESPACE_END
