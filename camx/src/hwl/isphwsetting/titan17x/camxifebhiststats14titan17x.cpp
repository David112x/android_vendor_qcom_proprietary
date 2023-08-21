////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifebhiststats14titan17x.cpp
/// @brief IFEBHistStats14Titan17x class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxispstatsmodule.h"
#include "camxiqinterface.h"
#include "camxifebhiststats14.h"
#include "camxifebhiststats14titan17x.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBHistStats14Titan17x::IFEBHistStats14Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEBHistStats14Titan17x::IFEBHistStats14Titan17x()
{
    SetCommandLength(
        PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEBHistRegionConfig) / RegisterWidthInBytes) +
        PacketBuilder::RequiredWriteDMISizeInDwords());
    memset(m_DMIConfig, 0, sizeof(m_DMIConfig));
    Set32bitDMILength(BHistDMILength);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBHistStats14Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBHistStats14Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result     = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    CAMX_ASSERT_MESSAGE(NULL != pInputData->pCmdBuffer, "bHist invalid cmd buffer pointer");

    if (NULL != pInputData->pCmdBuffer)
    {
        CmdBuffer* pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_VFE_STATS_BHIST_RGN_OFFSET_CFG,
                                              (sizeof(IFEBHistRegionConfig) / RegisterWidthInBytes),
                                              reinterpret_cast<UINT32*>(&m_regCmd));

        if (TRUE == pInputData->isInitPacket)
        {
            CmdBuffer*  pDMI32Buffer     = pInputData->p32bitDMIBuffer;
            UINT32*     pDMI32BufferAddr = pInputData->p32bitDMIBufferAddr;
            UINT32      offsetDWord      = *pDMIBufferOffset +
                                           (pInputData->pStripeConfig->stripeId * Get32bitDMILength());
            UINT32      lengthInBytes    = BHistDMILength * sizeof(UINT32);
            UINT8       DMISelect        = BHistRamLUT;

            Utils::Memcpy(pDMI32BufferAddr + offsetDWord,
                          & m_DMIConfig,
                          sizeof(m_DMIConfig));

            result = PacketBuilder::WriteDMI(pCmdBuffer,
                                             regIFE_IFE_0_VFE_DMI_CFG,
                                             DMISelect,
                                             pDMI32Buffer,
                                             offsetDWord * sizeof(UINT32),
                                             lengthInBytes);

            CAMX_LOG_VERBOSE(CamxLogGroupISP, "BHIST force DMI result %d", result);
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to write command buffer");
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBHistStats14Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBHistStats14Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CAMX_UNREFERENCED_PARAM(pOutput);
    CamxResult          result      = CamxResultSuccess;
    BHist14ConfigData*  pConfigData = static_cast<BHist14ConfigData*>(pInput);

    // Configure registers
    m_regCmd.regionOffset.bitfields.RGN_H_OFFSET    = pConfigData->regionConfig.horizontalOffset;
    m_regCmd.regionOffset.bitfields.RGN_V_OFFSET    = pConfigData->regionConfig.verticalOffset;
    m_regCmd.regionNumber.bitfields.RGN_H_NUM       = pConfigData->regionConfig.horizontalRegionNum;
    m_regCmd.regionNumber.bitfields.RGN_V_NUM       = pConfigData->regionConfig.verticalRegionNum;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBHistStats14Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBHistStats14Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;
    BHist14ConfigData* pConfigData = static_cast<BHist14ConfigData*>(pInput);

    if (NULL != pConfigData->pISPInputData)
    {
        // Write to general stats configuration register
        pConfigData->pISPInputData->pCalculatedData->moduleEnable.statsModules.BhistChannelSelect   = pConfigData->channel;
        pConfigData->pISPInputData->pCalculatedData->moduleEnable.statsModules.BhistBinUniformity   = pConfigData->uniform;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Missing data for configuration");
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBHistStats14Titan17x::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEBHistStats14Titan17x::CopyRegCmd(
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
// IFEBHistStats14Titan17x::~IFEBHistStats14Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEBHistStats14Titan17x::~IFEBHistStats14Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBHistStats14Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBHistStats14Titan17x::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region Offset Config  [0x%x]", m_regCmd.regionOffset);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region Number Config  [0x%x]", m_regCmd.regionNumber);
}

CAMX_NAMESPACE_END
