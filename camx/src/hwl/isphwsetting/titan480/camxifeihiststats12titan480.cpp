////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifeihiststats12titan480.cpp
/// @brief CAMX IFEIHistStats12Titan480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxispstatsmodule.h"
#include "camxiqinterface.h"
#include "camxifeihiststats12.h"
#include "camxifeihiststats12titan480.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEIHistStats12Titan480::IFEIHistStats12Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEIHistStats12Titan480::IFEIHistStats12Titan480()
{
    SetCommandLength(
        PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEIHist12Titan480Config) / RegisterWidthInBytes) +
        PacketBuilder::RequiredWriteDMISizeInDwords() * IFEIHist12Titan480DMITables);

    Set32bitDMILength(IFEIHist12Titan480DMILengthDword);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEIHistStats12Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEIHistStats12Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult      result              = CamxResultSuccess;
    ISPInputData*   pInputData          = static_cast<ISPInputData*>(pSettingData);
    UINT32          offset              = 0;
    UINT32          lengthInByte        = IFEIHist12Titan480LutTableSize;
    CmdBuffer*      pCmdBuffer          = NULL;
    CmdBuffer*      pDMIBuffer          = NULL;
    UINT32*         pDMI32BufferAddr    = NULL;

    if ((NULL != pInputData)                    &&
        (NULL != pDMIBufferOffset)              &&
        (NULL != pInputData->pCmdBuffer)        &&
        (NULL != pInputData->p32bitDMIBuffer)   &&
        (NULL != pInputData->p32bitDMIBufferAddr))
    {
        offset              = (*pDMIBufferOffset) * sizeof(UINT32);
        pCmdBuffer          = pInputData->pCmdBuffer;
        pDMIBuffer          = pInputData->p32bitDMIBuffer;
        pDMI32BufferAddr    = pInputData->p32bitDMIBufferAddr;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_STATS_IHIST_MODULE_CFG,
                                              (sizeof(IFE_IFE_0_PP_CLC_STATS_IHIST_MODULE_CFG) / RegisterWidthInBytes),
                                              reinterpret_cast<UINT32*>(&m_regCmd.moduleConfig));
        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_STATS_IHIST_RGN_OFFSET_CFG,
                                                  (sizeof(IFEIHist12Titan480RegionConfig) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.regionConfig));
        }

        if (TRUE == pInputData->isInitPacket)
        {
            Utils::Memset(CamX::Utils::VoidPtrInc(pDMI32BufferAddr, offset),
                          0x00,
                          IFEIHist12Titan480LutTableSize * IFEIHist12Titan480DMITables);

            if (CamxResultSuccess == result)
            {
                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                      regIFE_IFE_0_PP_CLC_STATS_IHIST_DMI_LUT_BANK_CFG,
                                                      (sizeof(IFEIHist12Titan480DMILUTConfig) / RegisterWidthInBytes),
                                                      reinterpret_cast<UINT32*>(&m_regCmd.DMILUTConfig));
            }

            if (CamxResultSuccess == result)
            {
                result = PacketBuilder::WriteDMI(pCmdBuffer,
                                                 regIFE_IFE_0_PP_CLC_STATS_IHIST_DMI_CFG,
                                                 IFEIHist12Titan480YLUT,
                                                 pDMIBuffer,
                                                 offset,
                                                 lengthInByte);
            }
            if (CamxResultSuccess == result)
            {
                offset += lengthInByte;
                result = PacketBuilder::WriteDMI(pCmdBuffer,
                                                 regIFE_IFE_0_PP_CLC_STATS_IHIST_DMI_CFG,
                                                 IFEIHist12Titan480GLUT,
                                                 pDMIBuffer,
                                                 offset,
                                                 lengthInByte);
            }
            if (CamxResultSuccess == result)
            {
                offset += lengthInByte;
                result = PacketBuilder::WriteDMI(pCmdBuffer,
                                                 regIFE_IFE_0_PP_CLC_STATS_IHIST_DMI_CFG,
                                                 IFEIHist12Titan480BLUT,
                                                 pDMIBuffer,
                                                 offset,
                                                 lengthInByte);
            }
            if (CamxResultSuccess == result)
            {
                offset += lengthInByte;
                result = PacketBuilder::WriteDMI(pCmdBuffer,
                                                 regIFE_IFE_0_PP_CLC_STATS_IHIST_DMI_CFG,
                                                 IFEIHist12Titan480RLUT,
                                                 pDMIBuffer,
                                                 offset,
                                                 lengthInByte);
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
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEIHistStats12Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEIHistStats12Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CAMX_UNREFERENCED_PARAM(pOutput);
    CamxResult          result        = CamxResultSuccess;
    IHist12ConfigData*  pConfigData   = static_cast<IHist12ConfigData*>(pInput);

    m_regCmd.moduleConfig.bitfields.EN                              = TRUE;
    m_regCmd.moduleConfig.bitfields.IHIST_CHAN_SEL                  = pConfigData->YCCChannel;
    m_regCmd.moduleConfig.bitfields.IHIST_SHIFT_BITS                = pConfigData->shiftBits;
    m_regCmd.regionConfig.regionOffset.bitfields.RGN_H_OFFSET       = pConfigData->regionConfig.horizontalOffset;
    m_regCmd.regionConfig.regionOffset.bitfields.RGN_V_OFFSET       = pConfigData->regionConfig.verticalOffset;
    m_regCmd.regionConfig.regionNumber.bitfields.RGN_H_NUM          = pConfigData->regionConfig.horizontalRegionNum;
    m_regCmd.regionConfig.regionNumber.bitfields.RGN_V_NUM          = pConfigData->regionConfig.verticalRegionNum;
    m_regCmd.DMILUTConfig.DMILUTBankconfig.bitfields.BANK_SEL       = IFEIHist12Titan480DMILUTBankSelect;
    m_regCmd.DMILUTConfig.moduleLUTBankConfig.bitfields.BANK_SEL    = IFEIHist12Titan480ModuleLUTBankSelect;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEIHistStats12Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEIHistStats12Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEIHistStats12Titan480::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEIHistStats12Titan480::CopyRegCmd(
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
// IFEIHistStats12Titan480::~IFEIHistStats12Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEIHistStats12Titan480::~IFEIHistStats12Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEIHistStats12Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEIHistStats12Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "IHist moduleConfig: 0x%x", m_regCmd.moduleConfig);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "IHist region offset HxV     [%u x %u]",
                     m_regCmd.regionConfig.regionOffset.bitfields.RGN_H_OFFSET,
                     m_regCmd.regionConfig.regionOffset.bitfields.RGN_V_OFFSET);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "IHist region num HxV        [%u x %u]",
                     m_regCmd.regionConfig.regionNumber.bitfields.RGN_H_NUM,
                     m_regCmd.regionConfig.regionNumber.bitfields.RGN_V_NUM);
}

CAMX_NAMESPACE_END
