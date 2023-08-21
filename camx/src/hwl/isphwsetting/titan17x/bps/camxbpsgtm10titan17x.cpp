////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpsgtm10titan17x.cpp
/// @brief CAMXBPSGTM10TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gtm10setting.h"
#include "camxbpsgtm10titan17x.h"
#include "camxiqinterface.h"
#include "titan170_bps.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGTM10Titan17x::BPSGTM10Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSGTM10Titan17x::BPSGTM10Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGTM10Titan17x::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGTM10Titan17x::Create(
    ISPHWSetting** ppHWSetting)
{
    CamxResult result = CamxResultSuccess;

    BPSGTM10Titan17x* pHWSetting = CAMX_NEW BPSGTM10Titan17x;

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
// BPSGTM10Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGTM10Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);
    CamxResult      result     = CamxResultSuccess;
    ISPInputData*   pInputData = static_cast<ISPInputData*>(pSettingData);

    // CDM pack the DMI buffer and patch the Red and Blue LUT DMI buffer into CDM pack
    if ((NULL != pInputData) && (NULL != pInputData->pDMICmdBuffer))
    {
        result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                         regBPS_BPS_0_CLC_GTM_DMI_CFG,
                                         GTM10LUT,
                                         pInputData->p64bitDMIBuffer, // m_pLUTDMICmdBuffer,
                                         0,
                                         BPSGTM10DMILengthInBytes);

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
// BPSGTM10Titan17x::UpdateFirmwareData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGTM10Titan17x::UpdateFirmwareData(
    VOID*   pSettingData,
    BOOL    moduleEnable)
{
    ISPInputData*  pInputData     = static_cast<ISPInputData*>(pSettingData);
    BpsIQSettings* pBPSIQSettings = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    CAMX_ASSERT(NULL != pBPSIQSettings);

    pBPSIQSettings->gtmParameters.moduleCfg.EN = moduleEnable;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGTM10Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGTM10Titan17x::UpdateTuningMetadata(
    VOID* pSettingData)
{
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    BPSTuningMetadata* pBPSTuningMetadata = static_cast<BPSTuningMetadata*>(pInputData->pBPSTuningMetadata);
    BpsIQSettings*     pBPSIQSettings     = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);

    // Post tuning metadata if setting is enabled
    if (NULL != pBPSTuningMetadata)
    {
        // Nothing to do for now
        CAMX_LOG_VERBOSE(CamxLogGroupBPS, "Update tables in different change");
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGTM10Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGTM10Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult            result     = CamxResultSuccess;
    UINT32                LUTEntry   = 0;
    GTM10UnpackedField*   pData      = static_cast<GTM10UnpackedField*>(pInput);
    GTM10OutputData*      pCmd       = static_cast<GTM10OutputData*>(pOutput);

    if ((NULL != pData) && (NULL != pCmd))
    {
        for (LUTEntry = 0; LUTEntry < GTM10LUTSize; LUTEntry++)
        {
            pCmd->regCmd.BPS.pDMIDataPtr[LUTEntry] =
                (static_cast<UINT64>(pData->YratioBase[pData->tableSel][LUTEntry]) & Utils::AllOnes64(18)) |
                ((static_cast<UINT64>(pData->YratioSlope[pData->tableSel][LUTEntry]) & Utils::AllOnes64(26)) << 18);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "data is pData %p pCmd %p ", pData, pCmd);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGTM10Titan17x::~BPSGTM10Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSGTM10Titan17x::~BPSGTM10Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGTM10Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSGTM10Titan17x::DumpRegConfig()
{
    // No register Buffer to Dump
}

CAMX_NAMESPACE_END
