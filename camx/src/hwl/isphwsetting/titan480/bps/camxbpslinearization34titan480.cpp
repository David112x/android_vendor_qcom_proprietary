////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpslinearization34titan480.cpp
/// @brief CAMXBPSLINEARIZATION34TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxbpslinearization34titan480.h"
#include "linearization34setting.h"
#include "camxiqinterface.h"
#include "titan480_bps.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

static const UINT MaxSlope = 9;

/// @brief Linearization register Configuration
struct BPSLinearization34RegConfig
{
    BPS_BPS_0_CLC_LINEARIZATION_KNEEPOINT_R_0_CFG   kneepointR0Config;  ///< Linearization kneepoint R 0 Config
    BPS_BPS_0_CLC_LINEARIZATION_KNEEPOINT_R_1_CFG   kneepointR1Config;  ///< Linearization kneepoint R 1 Config
    BPS_BPS_0_CLC_LINEARIZATION_KNEEPOINT_R_2_CFG   kneepointR2Config;  ///< Linearization kneepoint R 2 Config
    BPS_BPS_0_CLC_LINEARIZATION_KNEEPOINT_R_3_CFG   kneepointR3Config;  ///< Linearization kneepoint R 3 Config
    BPS_BPS_0_CLC_LINEARIZATION_KNEEPOINT_GR_0_CFG  kneepointGR0Config; ///< Linearization kneepoint GR 0 Config
    BPS_BPS_0_CLC_LINEARIZATION_KNEEPOINT_GR_1_CFG  kneepointGR1Config; ///< Linearization kneepoint GR 1 Config
    BPS_BPS_0_CLC_LINEARIZATION_KNEEPOINT_GR_2_CFG  kneepointGR2Config; ///< Linearization kneepoint GR 2 Config
    BPS_BPS_0_CLC_LINEARIZATION_KNEEPOINT_GR_3_CFG  kneepointGR3Config; ///< Linearization kneepoint GR 3 Config
    BPS_BPS_0_CLC_LINEARIZATION_KNEEPOINT_B_0_CFG   kneepointB0Config;  ///< Linearization kneepoint B 0 Config
    BPS_BPS_0_CLC_LINEARIZATION_KNEEPOINT_B_1_CFG   kneepointB1Config;  ///< Linearization kneepoint B 1 Config
    BPS_BPS_0_CLC_LINEARIZATION_KNEEPOINT_B_2_CFG   kneepointB2Config;  ///< Linearization kneepoint B 2 Config
    BPS_BPS_0_CLC_LINEARIZATION_KNEEPOINT_B_3_CFG   kneepointB3Config;  ///< Linearization kneepoint B 3 Config
    BPS_BPS_0_CLC_LINEARIZATION_KNEEPOINT_GB_0_CFG  kneepointGB0Config; ///< Linearization kneepoint GB 0 Config
    BPS_BPS_0_CLC_LINEARIZATION_KNEEPOINT_GB_1_CFG  kneepointGB1Config; ///< Linearization kneepoint GB 1 Config
    BPS_BPS_0_CLC_LINEARIZATION_KNEEPOINT_GB_2_CFG  kneepointGB2Config; ///< Linearization kneepoint GB 2 Config
    BPS_BPS_0_CLC_LINEARIZATION_KNEEPOINT_GB_3_CFG  kneepointGB3Config; ///< Linearization kneepoint GB 3 Config
} CAMX_PACKED;

/// @brief Linearization module Configuration
struct BPSLinearization34ModuleConfig
{
    BPS_BPS_0_CLC_LINEARIZATION_MODULE_CFG  moduleConfig;   ///< Module Configuration
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 BPSLinearization34RegLengthDWord = sizeof(BPSLinearization34RegConfig) / sizeof(UINT32);

static const UINT32 LinearizationLUT      = BPS_BPS_0_CLC_LINEARIZATION_DMI_LUT_CFG_LUT_SEL_LUT;
static const UINT32 LinearizationLUTBank0 = BPS_BPS_0_CLC_LINEARIZATION_DMI_LUT_BANK_CFG_BANK_SEL_LUT_BANK0;
static const UINT32 LinearizationLUTBank1 = BPS_BPS_0_CLC_LINEARIZATION_DMI_LUT_BANK_CFG_BANK_SEL_LUT_BANK1;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLinearization34Titan480::BPSLinearization34Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSLinearization34Titan480::BPSLinearization34Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLinearization34Titan480::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLinearization34Titan480::Create(
    ISPHWSetting** ppHWSetting)
{
    CamxResult result = CamxResultSuccess;

    BPSLinearization34Titan480* pHWSetting = CAMX_NEW BPSLinearization34Titan480;

    if (NULL != pHWSetting)
    {
        result = pHWSetting->Initialize();
        if (CamxResultSuccess == result)
        {
            (*ppHWSetting) = pHWSetting;
        }
        else
        {
            CAMX_DELETE pHWSetting;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to initialize in %s, no memory", __FUNCTION__);
        }
    }
    else
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "%s failed, no memory", __FUNCTION__);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSLinearization34Titan480::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLinearization34Titan480::Initialize()
{
    CamxResult result = CamxResultSuccess;

    m_pRegCmd = CAMX_CALLOC(sizeof(BPSLinearization34RegConfig));

    if (NULL != m_pRegCmd)
    {
        SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(BPSLinearization34RegLengthDWord));
    }
    else
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "No memory for register configuration!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLinearization34Titan480::WriteLUTtoDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLinearization34Titan480::WriteLUTtoDMI(
    VOID*   pInput)
{
    CamxResult      result      = CamxResultSuccess;
    ISPInputData*   pInputData  = static_cast<ISPInputData*>(pInput);

    // CDM pack the DMI buffer and patch the Red and Blue LUT DMI buffer into CDM pack
    if ((NULL != pInputData) && (NULL != pInputData->pDMICmdBuffer))
    {
        UINT32 lengthInByte = BPSLinearization34LUTLengthDword * sizeof(UINT32);

        result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                         regBPS_BPS_0_CLC_LINEARIZATION_DMI_CFG,
                                         LinearizationLUT,
                                         pInputData->p64bitDMIBuffer,
                                         0,
                                         lengthInByte);

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
// BPSLinearization34Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLinearization34Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    CamxResult    result     = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);

    result = WriteLUTtoDMI(pInputData);

    if ((CamxResultSuccess == result) && (NULL != pInputData->pCmdBuffer))
    {
        CmdBuffer* pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regBPS_BPS_0_CLC_LINEARIZATION_KNEEPOINT_R_0_CFG,
                                              BPSLinearization34RegLengthDWord,
                                              static_cast<UINT32*>(m_pRegCmd));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to write command buffer");
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
// BPSLinearization34Titan480::UpdateFirmwareData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLinearization34Titan480::UpdateFirmwareData(
    VOID*   pSettingData,
    BOOL    moduleEnable)
{
    ISPInputData*                pInputData     = static_cast<ISPInputData*>(pSettingData);
    BpsIQSettings*               pBPSIQSettings = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    BPSLinearization34RegConfig* pRegCmd        = static_cast<BPSLinearization34RegConfig*>(m_pRegCmd);
    CAMX_ASSERT(NULL != pBPSIQSettings);

    pBPSIQSettings->linearizationParameters.moduleCfg.EN = moduleEnable;

    pInputData->pCalculatedData->blackLevelLock          = pInputData->pHALTagsData->blackLevelLock;

    pInputData->pCalculatedMetadata->linearizationAppliedBlackLevel[0] = pRegCmd->kneepointR0Config.bitfields.P0;
    pInputData->pCalculatedMetadata->linearizationAppliedBlackLevel[1] = pRegCmd->kneepointGR0Config.bitfields.P0;
    pInputData->pCalculatedMetadata->linearizationAppliedBlackLevel[2] = pRegCmd->kneepointGB0Config.bitfields.P0;
    pInputData->pCalculatedMetadata->linearizationAppliedBlackLevel[3] = pRegCmd->kneepointB0Config.bitfields.P0;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLinearization34Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLinearization34Titan480::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult                   result             = CamxResultSuccess;
    ISPInputData*                pInputData         = static_cast<ISPInputData*>(pSettingData);
    BPSTuningMetadata*           pBPSTuningMetadata = static_cast<BPSTuningMetadata*>(pInputData->pBPSTuningMetadata);
    BpsIQSettings*               pBPSIQSettings     = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    BPSLinearization34RegConfig* pRegCmd            = static_cast<BPSLinearization34RegConfig*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if (NULL != pBPSTuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(BPSLinearization34RegConfig) <=
            sizeof(pBPSTuningMetadata->BPSTuningMetadata480.BPSLinearizationData.linearizationConfig));

        Utils::Memcpy(&pBPSTuningMetadata->BPSTuningMetadata480.BPSLinearizationData.linearizationConfig,
                        pRegCmd,
                        sizeof(BPSLinearization34RegConfig));
    }
    if ((TRUE == pBPSIQSettings->linearizationParameters.moduleCfg.EN) &&
        (NULL != pInputData->pipelineBPSData.pDebugDataWriter))
    {
        result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                        DebugDataTagID::TuningBPSLinearizationRegister,
                        DebugDataTagType::UInt32,
                        CAMX_ARRAY_SIZE(pBPSTuningMetadata->BPSTuningMetadata480.BPSLinearizationData.linearizationConfig),
                        &pBPSTuningMetadata->BPSTuningMetadata480.BPSLinearizationData.linearizationConfig,
                        sizeof(pBPSTuningMetadata->BPSTuningMetadata480.BPSLinearizationData.linearizationConfig));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLinearization34Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLinearization34Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult                    result   = CamxResultSuccess;
    Linearization34UnpackedField* pData    = static_cast<Linearization34UnpackedField*>(pInput);
    Linearization34OutputData*    pCmd     = static_cast<Linearization34OutputData*>(pOutput);
    BPSLinearization34RegConfig*  pRegCmd  = static_cast<BPSLinearization34RegConfig*>(m_pRegCmd);

    UINT32  DMILength = 0;
    UINT    count = 0;

    if ((NULL != pData) && (NULL != pCmd))
    {
        /// @todo (CAMX-1399) Verify all the values from unpacked are falls below bitfields
        pRegCmd->kneepointR0Config.bitfields.P0  = pData->rLUTkneePointL[0];
        pRegCmd->kneepointR0Config.bitfields.P1  = pData->rLUTkneePointL[1];
        pRegCmd->kneepointR1Config.bitfields.P2  = pData->rLUTkneePointL[2];
        pRegCmd->kneepointR1Config.bitfields.P3  = pData->rLUTkneePointL[3];
        pRegCmd->kneepointR2Config.bitfields.P4  = pData->rLUTkneePointL[4];
        pRegCmd->kneepointR2Config.bitfields.P5  = pData->rLUTkneePointL[5];
        pRegCmd->kneepointR3Config.bitfields.P6  = pData->rLUTkneePointL[6];
        pRegCmd->kneepointR3Config.bitfields.P7  = pData->rLUTkneePointL[7];
        pRegCmd->kneepointGR0Config.bitfields.P0 = pData->grLUTkneePointL[0];
        pRegCmd->kneepointGR0Config.bitfields.P1 = pData->grLUTkneePointL[1];
        pRegCmd->kneepointGR1Config.bitfields.P2 = pData->grLUTkneePointL[2];
        pRegCmd->kneepointGR1Config.bitfields.P3 = pData->grLUTkneePointL[3];
        pRegCmd->kneepointGR2Config.bitfields.P4 = pData->grLUTkneePointL[4];
        pRegCmd->kneepointGR2Config.bitfields.P5 = pData->grLUTkneePointL[5];
        pRegCmd->kneepointGR3Config.bitfields.P6 = pData->grLUTkneePointL[6];
        pRegCmd->kneepointGR3Config.bitfields.P7 = pData->grLUTkneePointL[7];
        pRegCmd->kneepointGB0Config.bitfields.P0 = pData->gbLUTkneePointL[0];
        pRegCmd->kneepointGB0Config.bitfields.P1 = pData->gbLUTkneePointL[1];
        pRegCmd->kneepointGB1Config.bitfields.P2 = pData->gbLUTkneePointL[2];
        pRegCmd->kneepointGB1Config.bitfields.P3 = pData->gbLUTkneePointL[3];
        pRegCmd->kneepointGB2Config.bitfields.P4 = pData->gbLUTkneePointL[4];
        pRegCmd->kneepointGB2Config.bitfields.P5 = pData->gbLUTkneePointL[5];
        pRegCmd->kneepointGB3Config.bitfields.P6 = pData->gbLUTkneePointL[6];
        pRegCmd->kneepointGB3Config.bitfields.P7 = pData->gbLUTkneePointL[7];
        pRegCmd->kneepointB0Config.bitfields.P0  = pData->bLUTkneePointL[0];
        pRegCmd->kneepointB0Config.bitfields.P1  = pData->bLUTkneePointL[1];
        pRegCmd->kneepointB1Config.bitfields.P2  = pData->bLUTkneePointL[2];
        pRegCmd->kneepointB1Config.bitfields.P3  = pData->bLUTkneePointL[3];
        pRegCmd->kneepointB2Config.bitfields.P4  = pData->bLUTkneePointL[4];
        pRegCmd->kneepointB2Config.bitfields.P5  = pData->bLUTkneePointL[5];
        pRegCmd->kneepointB3Config.bitfields.P6  = pData->bLUTkneePointL[6];
        pRegCmd->kneepointB3Config.bitfields.P7  = pData->bLUTkneePointL[7];

        ///< packing the DMI table
        for (count = 0; count < MaxSlope; count++, DMILength += 4)
        {
            pCmd->pDMIDataPtr[DMILength]        =
                ((static_cast<UINT32>(pData->rLUTbaseL[pData->LUTbankSelection][count])) & Utils::AllOnes32(14)) |
                ((static_cast<UINT32>(pData->rLUTdeltaL[pData->LUTbankSelection][count]) & Utils::AllOnes32(14)) << 14);
            pCmd->pDMIDataPtr[DMILength + 1]    =
                ((static_cast<UINT32>(pData->grLUTbaseL[pData->LUTbankSelection][count])) & Utils::AllOnes32(14)) |
                ((static_cast<UINT32>(pData->grLUTdeltaL[pData->LUTbankSelection][count]) & Utils::AllOnes32(14)) << 14);
            pCmd->pDMIDataPtr[DMILength + 2]    =
                ((static_cast<UINT32>(pData->gbLUTbaseL[pData->LUTbankSelection][count])) & Utils::AllOnes32(14)) |
                ((static_cast<UINT32>(pData->gbLUTdeltaL[pData->LUTbankSelection][count]) & Utils::AllOnes32(14)) << 14);
            pCmd->pDMIDataPtr[DMILength + 3]    =
                ((static_cast<UINT32>(pData->bLUTbaseL[pData->LUTbankSelection][count])) & Utils::AllOnes32(14)) |
                ((static_cast<UINT32>(pData->bLUTdeltaL[pData->LUTbankSelection][count]) & Utils::AllOnes32(14)) << 14);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Input params are NULL pData=0x%p pCmd=0x%p", pData, pCmd);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLinearization34Titan480::~BPSLinearization34Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSLinearization34Titan480::~BPSLinearization34Titan480()
{
    CAMX_FREE(m_pRegCmd);
    m_pRegCmd = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLinearization34Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSLinearization34Titan480::DumpRegConfig()
{
    BPSLinearization34RegConfig* pRegCmd    = static_cast<BPSLinearization34RegConfig*>(m_pRegCmd);

    if (NULL != pRegCmd)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "* Linearization34 CFG [HEX] \n");
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearizatioin34.kneepointR0Config  = %x\n", pRegCmd->kneepointR0Config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearizatioin34.kneepointR10Config = %x\n", pRegCmd->kneepointR1Config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearizatioin34.kneepointR2Config  = %x\n", pRegCmd->kneepointR2Config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearizatioin34.kneepointR3Config  = %x\n", pRegCmd->kneepointR3Config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearizatioin34.kneepointGR0Config = %x\n", pRegCmd->kneepointGR0Config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearizatioin34.kneepointGR1Config = %x\n", pRegCmd->kneepointGR1Config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearizatioin34.kneepointGR2Config = %x\n", pRegCmd->kneepointGR2Config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearizatioin34.kneepointGR3Config = %x\n", pRegCmd->kneepointGR3Config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearizatioin34.kneepointB0Config  = %x\n", pRegCmd->kneepointB0Config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearizatioin34.kneepointB1Config  = %x\n", pRegCmd->kneepointB1Config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearizatioin34.kneepointB2Config  = %x\n", pRegCmd->kneepointB2Config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearizatioin34.kneepointB3Config  = %x\n", pRegCmd->kneepointB3Config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearizatioin34.kneepointGB0Config = %x\n", pRegCmd->kneepointGB0Config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearizatioin34.kneepointGB1Config = %x\n", pRegCmd->kneepointGB1Config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearizatioin34.kneepointGB2Config = %x\n", pRegCmd->kneepointGB2Config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearizatioin34.kneepointGB3Config = %x\n", pRegCmd->kneepointGB3Config);
    }
}

CAMX_NAMESPACE_END
