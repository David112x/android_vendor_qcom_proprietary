////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifebhiststats14titan480.cpp
/// @brief CAMX IFEBHistStats14Titan480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxispstatsmodule.h"
#include "camxiqinterface.h"
#include "camxifebhiststats14.h"
#include "camxifebhiststats14titan480.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBHistStats14Titan480::IFEBHistStats14Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEBHistStats14Titan480::IFEBHistStats14Titan480()
{
    SetCommandLength(
        PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEBHist14Titan480Config) / RegisterWidthInBytes) +
        PacketBuilder::RequiredWriteDMISizeInDwords() * IFEBHist14Titan480DMITables);

    Set32bitDMILength(IFEBHist14Titan480DMILengthDword);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBHistStats14Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBHistStats14Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult      result              = CamxResultSuccess;
    ISPInputData*   pInputData          = static_cast<ISPInputData*>(pSettingData);
    UINT32          offset              = 0;
    UINT32          lengthInByte        = IFEBHist14Titan480LutTableSize;
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
                                              regIFE_IFE_0_PP_CLC_STATS_BHIST_MODULE_CFG,
                                              (sizeof(IFE_IFE_0_PP_CLC_STATS_BHIST_MODULE_CFG) / RegisterWidthInBytes),
                                              reinterpret_cast<UINT32*>(&m_regCmd.moduleConfig));
        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_STATS_BHIST_RGN_OFFSET_CFG,
                                                  (sizeof(IFEBHist14Titan480RegionConfig) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.regionConfig));
        }

        if (TRUE == pInputData->isInitPacket)
        {
            Utils::Memset(CamX::Utils::VoidPtrInc(pDMI32BufferAddr, offset),
                          0x00,
                          IFEBHist14Titan480LutTableSize * IFEBHist14Titan480DMITables);

            if (CamxResultSuccess == result)
            {
                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                      regIFE_IFE_0_PP_CLC_STATS_BHIST_DMI_LUT_BANK_CFG,
                                                      (sizeof(IFEBHist14Titan480DMILUTConfig) / RegisterWidthInBytes),
                                                      reinterpret_cast<UINT32*>(&m_regCmd.DMILUTConfig));
            }

            if (CamxResultSuccess == result)
            {
                result = PacketBuilder::WriteDMI(pCmdBuffer,
                                                 regIFE_IFE_0_PP_CLC_STATS_BHIST_DMI_CFG,
                                                 IFEBHist14Titan480LUT,
                                                 pDMIBuffer,
                                                 offset,
                                                 lengthInByte);
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEInvalidPointer;
    }


    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to write command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBHistStats14Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBHistStats14Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CAMX_UNREFERENCED_PARAM(pOutput);
    CamxResult          result      = CamxResultSuccess;
    BHist14ConfigData*  pConfigData = static_cast<BHist14ConfigData*>(pInput);

    // Configure registers
    m_regCmd.moduleConfig.bitfields.EN                              = TRUE;
    m_regCmd.moduleConfig.bitfields.BHIST_BIN_UNIFORMITY            = pConfigData->uniform;
    m_regCmd.moduleConfig.bitfields.BHIST_CHAN_SEL                  = pConfigData->channel;
    m_regCmd.regionConfig.regionOffset.bitfields.RGN_H_OFFSET       = pConfigData->regionConfig.horizontalOffset;
    m_regCmd.regionConfig.regionOffset.bitfields.RGN_V_OFFSET       = pConfigData->regionConfig.verticalOffset;
    m_regCmd.regionConfig.regionNumber.bitfields.RGN_H_NUM          = pConfigData->regionConfig.horizontalRegionNum;
    m_regCmd.regionConfig.regionNumber.bitfields.RGN_V_NUM          = pConfigData->regionConfig.verticalRegionNum;
    m_regCmd.DMILUTConfig.DMILUTBankconfig.bitfields.BANK_SEL       = IFEBHist14Titan480DMILUTBankSelect;
    m_regCmd.DMILUTConfig.moduleLUTBankConfig.bitfields.BANK_SEL    = IFEBHist14Titan480ModuleLUTBankSelect;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBHistStats14Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBHistStats14Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBHistStats14Titan480::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEBHistStats14Titan480::CopyRegCmd(
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
// IFEBHistStats14Titan480::~IFEBHistStats14Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEBHistStats14Titan480::~IFEBHistStats14Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBHistStats14Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBHistStats14Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Module Config  [0x%x]", m_regCmd.moduleConfig);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region Offset Config  [0x%x]", m_regCmd.regionConfig.regionOffset);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region Number Config  [0x%x]", m_regCmd.regionConfig.regionNumber);
}

CAMX_NAMESPACE_END
