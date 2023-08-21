////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifeihiststats12titan17x.cpp
/// @brief IFEIHistStats12Titan17x class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxispstatsmodule.h"
#include "camxiqinterface.h"
#include "camxifeihiststats12.h"
#include "camxifeihiststats12titan17x.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEIHistStats12Titan17x::IFEIHistStats12Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEIHistStats12Titan17x::IFEIHistStats12Titan17x()
{
    SetCommandLength(
        PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEIHist12RegionConfig) / RegisterWidthInBytes) +
        PacketBuilder::RequiredWriteDMISizeInDwords() * IHistNoOfLut);
    memset(m_DMIConfig, 0, sizeof(m_DMIConfig));
    Set32bitDMILength(IHistDMISize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEIHistStats12Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEIHistStats12Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    CAMX_ASSERT_MESSAGE(NULL != pInputData->pCmdBuffer, "iHist invalid cmd buffer pointer");

    if (NULL != pInputData->pCmdBuffer)
    {
        pCmdBuffer = pInputData->pCmdBuffer;

        result     = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_STATS_IHIST_RGN_OFFSET_CFG,
                                                  (sizeof(IFEIHist12RegionConfig) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd));

        if (TRUE == pInputData->isInitPacket)
        {
            UINT32      lut                     = 0;
            CmdBuffer*  pDMI32Buffer            = pInputData->p32bitDMIBuffer;
            UINT32*     pDMI32BufferAddr        = pInputData->p32bitDMIBufferAddr;
            UINT32      offsetDWord             = *pDMIBufferOffset +
                                                  (pInputData->pStripeConfig->stripeId * Get32bitDMILength());
            UINT8       DMISelect[IHistNoOfLut] = { IHistRam0LUT, IHistRam1LUT, IHistRam2LUT, IHistRam4LUT };

            for (lut = 0; lut < IHistNoOfLut; lut++)
            {
                UINT32 lengthInBytes = IHistDMILength * sizeof(UINT32);

                Utils::Memcpy(pDMI32BufferAddr + offsetDWord,
                              & m_DMIConfig,
                              sizeof(m_DMIConfig));

                result = PacketBuilder::WriteDMI(pCmdBuffer,
                                                 regIFE_IFE_0_VFE_DMI_CFG,
                                                 DMISelect[lut],
                                                 pDMI32Buffer,
                                                 offsetDWord * sizeof(UINT32),
                                                 lengthInBytes);
                offsetDWord += IHistDMILength;

                CAMX_LOG_VERBOSE(CamxLogGroupISP, "IHist force DMI result %d", result);
            }
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
// IFEIHistStats12Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEIHistStats12Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CAMX_UNREFERENCED_PARAM(pOutput);
    CamxResult          result        = CamxResultSuccess;
    IHist12ConfigData*  pConfigData   = static_cast<IHist12ConfigData*>(pInput);

    m_regCmd.regionOffset.bitfields.RGN_H_OFFSET    = pConfigData->regionConfig.horizontalOffset;
    m_regCmd.regionOffset.bitfields.RGN_V_OFFSET    = pConfigData->regionConfig.verticalOffset;
    m_regCmd.regionNumber.bitfields.RGN_H_NUM       = pConfigData->regionConfig.horizontalRegionNum;
    m_regCmd.regionNumber.bitfields.RGN_V_NUM       = pConfigData->regionConfig.verticalRegionNum;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEIHistStats12Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEIHistStats12Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;
    IHist12ConfigData* pConfigData = static_cast<IHist12ConfigData*>(pInput);

    if ((NULL != pConfigData) && (NULL != pConfigData->pISPInputData))
    {
        // Write to general stats configuration register
        // Configure per-frame register configuration
        pConfigData->pISPInputData->pCalculatedData->moduleEnable.statsModules.IhistChannelSelect = pConfigData->YCCChannel;
        pConfigData->pISPInputData->pCalculatedData->moduleEnable.statsModules.IHistShiftBits     = pConfigData->shiftBits;
        pConfigData->pISPInputData->pCalculatedData->moduleEnable.statsModules.IHistSiteSelect    =
            pConfigData->pISPInputData->statsTapOut.IHistStatsSrcSelection;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Missing data for configuration");
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEIHistStats12Titan17x::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEIHistStats12Titan17x::CopyRegCmd(
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
// IFEIHistStats12Titan17x::~IFEIHistStats12Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEIHistStats12Titan17x::~IFEIHistStats12Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEIHistStats12Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEIHistStats12Titan17x::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "IHist region offset HxV     [%u x %u]",
                     m_regCmd.regionOffset.bitfields.RGN_H_OFFSET,
                     m_regCmd.regionOffset.bitfields.RGN_V_OFFSET);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "IHist region num HxV        [%u x %u]",
                     m_regCmd.regionNumber.bitfields.RGN_H_NUM,
                     m_regCmd.regionNumber.bitfields.RGN_V_NUM);
}

CAMX_NAMESPACE_END
