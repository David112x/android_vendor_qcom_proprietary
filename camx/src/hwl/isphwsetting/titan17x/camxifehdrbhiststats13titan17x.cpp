////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifehdrbhiststats13titan17x.cpp
/// @brief IFEHDRBHistStats13Titan17x class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxispstatsmodule.h"
#include "camxiqinterface.h"
#include "camxifehdrbhiststats13.h"
#include "camxifehdrbhiststats13titan17x.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBHistStats13Titan17x::IFEHDRBHistStats13Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHDRBHistStats13Titan17x::IFEHDRBHistStats13Titan17x()
{
    SetCommandLength(
        PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEHDRBHistRegionConfig) / RegisterWidthInBytes) +
        (PacketBuilder::RequiredWriteDMISizeInDwords() * HDRBHistNoLut));
    memset(m_DMIConfig, 0, sizeof(m_DMIConfig));
    Set32bitDMILength(HDRBHistDMILength * HDRBHistNoLut);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBHistStats13Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDRBHistStats13Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    pCmdBuffer = pInputData->pCmdBuffer;

    result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                          regIFE_IFE_0_VFE_STATS_HDR_BHIST_RGN_OFFSET_CFG,
                                          (sizeof(IFEHDRBHistRegionConfig) / RegisterWidthInBytes),
                                          reinterpret_cast<UINT32*>(&m_regCmd));

    if (TRUE == pInputData->isInitPacket)
    {
        UINT32      lut = 0;
        CmdBuffer*  pDMI32Buffer     = pInputData->p32bitDMIBuffer;
        UINT32*     pDMI32BufferAddr = pInputData->p32bitDMIBufferAddr;
        UINT32      offsetDWord      = *pDMIBufferOffset + (pInputData->pStripeConfig->stripeId * Get32bitDMILength());

        UINT8       DMISelect[HDRBHistNoLut] = { HDRBHistRam0LUT, HDRBHistRam1LUT, HDRBHistRam2LUT, HDRBHistRamXLUT };

        for (lut = 0; lut < HDRBHistNoLut; lut++)
        {
            UINT32      lengthInBytes = HDRBHistDMILength * sizeof(UINT32);
            Utils::Memcpy(pDMI32BufferAddr + offsetDWord,
                          &m_DMIConfig,
                          sizeof(m_DMIConfig));

            result = PacketBuilder::WriteDMI(pCmdBuffer,
                                             regIFE_IFE_0_VFE_DMI_CFG,
                                             DMISelect[lut],
                                             pDMI32Buffer,
                                             offsetDWord * sizeof(UINT32),
                                             lengthInBytes);
            offsetDWord += HDRBHistDMILength;

            CAMX_LOG_VERBOSE(CamxLogGroupISP, "BHIST force DMI result %d", result);
        }
    }
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to write command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBHistStats13Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDRBHistStats13Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CAMX_UNREFERENCED_PARAM(pOutput);
    CamxResult              result      = CamxResultSuccess;
    HDRBHist13ConfigData*   pConfigData = static_cast<HDRBHist13ConfigData*>(pInput);

    m_regCmd.regionOffset.bitfields.RGN_H_OFFSET    = pConfigData->regionConfig.offsetHorizNum;
    m_regCmd.regionOffset.bitfields.RGN_V_OFFSET    = pConfigData->regionConfig.offsetVertNum;
    m_regCmd.regionNumber.bitfields.RGN_H_NUM       = pConfigData->regionConfig.regionHorizNum;
    m_regCmd.regionNumber.bitfields.RGN_V_NUM       = pConfigData->regionConfig.regionVertNum;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBHistStats13Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDRBHistStats13Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;
    HDRBHist13ConfigData* pConfigData = static_cast<HDRBHist13ConfigData*>(pInput);

    if (NULL != pConfigData->pISPInputData)
    {
        // Write to general stats configuration register
        pConfigData->pISPInputData->pCalculatedData->moduleEnable.statsModules.HDRBhistChannelSelect    =
            pConfigData->greenChannelSelect;
        pConfigData->pISPInputData->pCalculatedData->moduleEnable.statsModules.HDRBhistFieldSelect      =
            pConfigData->inputFieldSelect;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Missing data for configuration");
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBHistStats13Titan17x::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEHDRBHistStats13Titan17x::CopyRegCmd(
    VOID* pData)
{
    UINT32 dataCopied = 0;

    if (NULL != pData)
    {
        Utils::Memcpy(pData, &m_regCmd, sizeof(m_regCmd));
        dataCopied = sizeof(m_regCmd);
    }

    return dataCopied;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBHistStats13Titan17x::~IFEHDRBHistStats13Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHDRBHistStats13Titan17x::~IFEHDRBHistStats13Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBHistStats13Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEHDRBHistStats13Titan17x::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                     "Region Offset Config          [%dx%d]",
                     m_regCmd.regionOffset.bitfields.RGN_H_OFFSET,
                     m_regCmd.regionOffset.bitfields.RGN_V_OFFSET);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                     "Region Number Config          [%dx%d]",
                     m_regCmd.regionNumber.bitfields.RGN_H_NUM,
                     m_regCmd.regionNumber.bitfields.RGN_V_NUM);
}

CAMX_NAMESPACE_END
