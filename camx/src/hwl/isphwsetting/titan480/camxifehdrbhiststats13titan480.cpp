////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifehdrbhiststats13titan480.cpp
/// @brief CAMX IFEHDRBHistStats13Titan480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxispstatsmodule.h"
#include "camxiqinterface.h"
#include "camxifehdrbhiststats13.h"
#include "camxifehdrbhiststats13titan480.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBHistStats13Titan480::IFEHDRBHistStats13Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHDRBHistStats13Titan480::IFEHDRBHistStats13Titan480()
{
    SetCommandLength(
        PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEHDRBHist13Titan480Config) / RegisterWidthInBytes) +
        PacketBuilder::RequiredWriteDMISizeInDwords() * IFEHDRBHist13Titan480DMITables);

    Set32bitDMILength(IFEHDRBHist13Titan480DMILengthDword);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBHistStats13Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDRBHistStats13Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult      result              = CamxResultSuccess;
    ISPInputData*   pInputData          = static_cast<ISPInputData*>(pSettingData);
    UINT32          offset              = 0;
    UINT32          lengthInByte        = IFEHDRBHist13Titan480LutTableSize;
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
                                              regIFE_IFE_0_PP_CLC_STATS_HDR_BHIST_MODULE_CFG,
                                              (sizeof(IFE_IFE_0_PP_CLC_STATS_HDR_BHIST_MODULE_CFG) / RegisterWidthInBytes),
                                              reinterpret_cast<UINT32*>(&m_regCmd.moduleConfig));
        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_STATS_HDR_BHIST_RGN_OFFSET_CFG,
                                                  (sizeof(IFEHDRBHist13Titan480RegionConfig) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.regionConfig));
        }

        if (TRUE == pInputData->isInitPacket)
        {
            Utils::Memset(CamX::Utils::VoidPtrInc(pDMI32BufferAddr, offset),
                          0x00,
                          IFEHDRBHist13Titan480LutTableSize * IFEHDRBHist13Titan480DMITables);

            if (CamxResultSuccess == result)
            {
                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                      regIFE_IFE_0_PP_CLC_STATS_HDR_BHIST_DMI_LUT_BANK_CFG,
                                                      (sizeof(IFEHDRBHist13Titan480DMILUTConfig) / RegisterWidthInBytes),
                                                      reinterpret_cast<UINT32*>(&m_regCmd.DMILUTConfig));
            }

            if (CamxResultSuccess == result)
            {
                result = PacketBuilder::WriteDMI(pCmdBuffer,
                                                 regIFE_IFE_0_PP_CLC_STATS_HDR_BHIST_DMI_CFG,
                                                 IFEHDRBHist13Titan480GLUT,
                                                 pDMIBuffer,
                                                 offset,
                                                 lengthInByte);
            }
            if (CamxResultSuccess == result)
            {
                offset += lengthInByte;
                result = PacketBuilder::WriteDMI(pCmdBuffer,
                                                 regIFE_IFE_0_PP_CLC_STATS_HDR_BHIST_DMI_CFG,
                                                 IFEHDRBHist13Titan480BLUT,
                                                 pDMIBuffer,
                                                 offset,
                                                 lengthInByte);
            }
            if (CamxResultSuccess == result)
            {
                offset += lengthInByte;
                result = PacketBuilder::WriteDMI(pCmdBuffer,
                                                 regIFE_IFE_0_PP_CLC_STATS_HDR_BHIST_DMI_CFG,
                                                 IFEHDRBHist13Titan480RLUT,
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
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBHistStats13Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDRBHistStats13Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CAMX_UNREFERENCED_PARAM(pOutput);
    CamxResult              result      = CamxResultSuccess;
    HDRBHist13ConfigData*   pConfigData = static_cast<HDRBHist13ConfigData*>(pInput);

    if (NULL != pConfigData->pISPInputData)
    {
        m_regCmd.moduleConfig.bitfields.EN                              = TRUE;
        m_regCmd.moduleConfig.bitfields.HDR_BHIST_CHAN_SEL              = pConfigData->greenChannelSelect;
        m_regCmd.moduleConfig.bitfields.HDR_BHIST_FIELD_SEL             = pConfigData->inputFieldSelect;
        m_regCmd.moduleConfig.bitfields.ZHDR_FIRST_RB_EXP               = pConfigData->ZZHDRFirstRBEXP;
        m_regCmd.regionConfig.regionOffset.bitfields.RGN_H_OFFSET       = pConfigData->regionConfig.offsetHorizNum;
        m_regCmd.regionConfig.regionOffset.bitfields.RGN_V_OFFSET       = pConfigData->regionConfig.offsetVertNum;
        m_regCmd.regionConfig.regionNumber.bitfields.RGN_H_NUM          = pConfigData->regionConfig.regionHorizNum;
        m_regCmd.regionConfig.regionNumber.bitfields.RGN_V_NUM          = pConfigData->regionConfig.regionVertNum;
        m_regCmd.DMILUTConfig.DMILUTBankconfig.bitfields.BANK_SEL       = IFEHDRBHist13Titan480DMILUTBankSelect;
        m_regCmd.DMILUTConfig.moduleLUTBankConfig.bitfields.BANK_SEL    = IFEHDRBHist13Titan480ModuleLUTBankSelect;

        if (0 == pConfigData->inputFieldSelect)
        {
            // Map input pattern to Titan 480, zero means non-HDR
            m_regCmd.moduleConfig.bitfields.HDR_BHIST_HDR_SEL = 0;
        }
        else
        {
            // Map input pattern to Titan 480
            // case ZZHDRPattern == 0: HDR_BHIST_HDR_SEL = 4;
            // case ZZHDRPattern == 1: HDR_BHIST_HDR_SEL = 5;
            // case ZZHDRPattern == 2: HDR_BHIST_HDR_SEL = 6;
            // case ZZHDRPattern == 3: HDR_BHIST_HDR_SEL = 7;
            m_regCmd.moduleConfig.bitfields.HDR_BHIST_HDR_SEL = pConfigData->ZZHDRPattern + 4;
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBHistStats13Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDRBHistStats13Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBHistStats13Titan480::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEHDRBHistStats13Titan480::CopyRegCmd(
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
// IFEHDRBHistStats13Titan480::~IFEHDRBHistStats13Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHDRBHistStats13Titan480::~IFEHDRBHistStats13Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBHistStats13Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEHDRBHistStats13Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Module Config  [0x%x]", m_regCmd.moduleConfig);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                     "Region Offset Config          [%dx%d]",
                     m_regCmd.regionConfig.regionOffset.bitfields.RGN_H_OFFSET,
                     m_regCmd.regionConfig.regionOffset.bitfields.RGN_V_OFFSET);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                     "Region Number Config          [%dx%d]",
                     m_regCmd.regionConfig.regionNumber.bitfields.RGN_H_NUM,
                     m_regCmd.regionConfig.regionNumber.bitfields.RGN_V_NUM);
}

CAMX_NAMESPACE_END
