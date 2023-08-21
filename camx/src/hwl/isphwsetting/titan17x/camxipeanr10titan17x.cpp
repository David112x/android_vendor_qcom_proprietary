////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipeanr10titan17x.cpp
/// @brief CAMXIPEANR10TITAN17x class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "anr10setting.h"
#include "camxutils.h"
#include "camxipeanr10titan17x.h"
#include "camxipeanr10.h"
#include "camxispiqmodule.h"
#include "titan170_ipe.h"
#include "camxcdmdefs.h"
#include "Process_AnrPass17x.h"
#include "anr10regcmd17x.h"

CAMX_NAMESPACE_BEGIN
static const UINT32 IPEANRDCBlend1RegCmdLength          = sizeof(IPEANRDCBlend1RegCmd)          / RegisterWidthInBytes;
static const UINT32 IPEANRRNFRegCmdLength               = sizeof(IPEANRRNFRegCmd)               / RegisterWidthInBytes;
static const UINT32 IPEANRDCUSRegCmdLength              = sizeof(IPEANRDCUSRegCmd)              / RegisterWidthInBytes;
static const UINT32 IPEANRCFilter2RegCmdLength          = sizeof(IPEANRCFilter2RegCmd)          / RegisterWidthInBytes;
static const UINT32 IPEANRDCBlend2RegCmdLength          = sizeof(IPEANRDCBlend2RegCmd)          / RegisterWidthInBytes;
static const UINT32 IPEANRCYLPFPreLensGainRegCmdLength  = sizeof(IPEANRCYLPFPreLensGainRegCmd)  / RegisterWidthInBytes;
static const UINT32 IPEANRCYLPFLensGainRegCmdLength     = sizeof(IPEANRCYLPFLensGainRegCmd)     / RegisterWidthInBytes;
static const UINT32 IPEANRCYLPFPostLensGainRegCmdLength = sizeof(IPEANRCYLPFPostLensGainRegCmd) / RegisterWidthInBytes;
static const UINT32 IPEANRCNRRegCmdLength               = sizeof(IPEANRCNRRegCmd)               / RegisterWidthInBytes;
static const UINT32 IPEANRNumRegWriteCmds               = 9;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10Titan17x::IPEANR10Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEANR10Titan17x::IPEANR10Titan17x()
{
    m_pRegCmd = CAMX_CALLOC(PASS_NAME_MAX * sizeof(IPEANRRegCmd17x));
    if (NULL == m_pRegCmd)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to allocate memory for Cmd Buffer");
    }

    UINT32 cmdBufferSize = (sizeof(IPEANRDCBlend1RegCmd)          +
                            sizeof(IPEANRRNFRegCmd)               +
                            sizeof(IPEANRDCUSRegCmd)              +
                            sizeof(IPEANRCFilter2RegCmd)          +
                            sizeof(IPEANRDCBlend2RegCmd)          +
                            sizeof(IPEANRCYLPFPreLensGainRegCmd)  +
                            sizeof(IPEANRCYLPFLensGainRegCmd)     +
                            sizeof(IPEANRCYLPFPostLensGainRegCmd) +
                            sizeof(IPEANRCNRRegCmd)               +
                            (IPEANRNumRegWriteCmds * (cdm_get_cmd_header_size(CDMCmdRegContinuous) * 4)));
    SetCommandLength(cmdBufferSize * PASS_NAME_MAX);

    m_singlePassCmdLength = cmdBufferSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10Titan17x::~IPEANR10Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEANR10Titan17x::~IPEANR10Titan17x()
{
    if (NULL != m_pRegCmd)
    {
        CAMX_FREE(m_pRegCmd);
        m_pRegCmd = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEANR10Titan17x::GetModuleData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEANR10Titan17x::GetModuleData(
    VOID* pModuleData)
{
    IPEIQModuleData* pData = reinterpret_cast<IPEIQModuleData*>(pModuleData);

    // data is expected to be filled after execute
    pData->singlePassCmdLength = m_singlePassCmdLength;
    Utils::Memcpy(pData->offsetPass, m_offsetPass, sizeof(pData->offsetPass));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEANR10Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    /// @todo (CAMX-729) Implement IQ block and hook up with Common library
    /// @todo (CAMX-735) Link IQ module with Chromatix adapter
    CamxResult       result     = CamxResultSuccess;
    ISPInputData*    pInputData = static_cast<ISPInputData*>(pSettingData);
    IPEANRRegCmd17x* pRegCmd    = static_cast<IPEANRRegCmd17x*>(m_pRegCmd);
    CmdBuffer*       pCmdBuffer = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if ((NULL != pInputData) &&
        (NULL != pInputData->pipelineIPEData.ppIPECmdBuffer))
    {
        pCmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferNPS];
        if (NULL != pCmdBuffer)
        {
            for (UINT passNumber = PASS_NAME_FULL; passNumber < PASS_NAME_MAX; passNumber++)
            {
                m_offsetPass[passNumber] = (pCmdBuffer->GetResourceUsedDwords() * RegisterWidthInBytes);
                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                    regIPE_IPE_0_NPS_CLC_ANR_DCBLEND1_BYPASS,
                    IPEANRDCBlend1RegCmdLength,
                    reinterpret_cast<UINT32*>(&pRegCmd[passNumber].dcBlend1Cmd));
                CAMX_ASSERT(CamxResultSuccess == result);

                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                    regIPE_IPE_0_NPS_CLC_ANR_RNF_BYPASS,
                    IPEANRRNFRegCmdLength,
                    reinterpret_cast<UINT32*>(&pRegCmd[passNumber].rnfCmd));
                CAMX_ASSERT(CamxResultSuccess == result);

                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                    regIPE_IPE_0_NPS_CLC_ANR_DCUS_BYPASS,
                    IPEANRDCUSRegCmdLength,
                    reinterpret_cast<UINT32*>(&pRegCmd[passNumber].dcusCmd));
                CAMX_ASSERT(CamxResultSuccess == result);

                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                    regIPE_IPE_0_NPS_CLC_ANR_CYLPF_FILTER2_BYPASS,
                    IPEANRCFilter2RegCmdLength,
                    reinterpret_cast<UINT32*>(&pRegCmd[passNumber].filter2Cmd));
                CAMX_ASSERT(CamxResultSuccess == result);

                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                    regIPE_IPE_0_NPS_CLC_ANR_DCBLEND2_BYPASS_LUMA,
                    IPEANRDCBlend2RegCmdLength,
                    reinterpret_cast<UINT32*>(&pRegCmd[passNumber].dcBlend2Cmd));
                CAMX_ASSERT(CamxResultSuccess == result);

                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                    regIPE_IPE_0_NPS_CLC_ANR_CYLPF_BYPASS,
                    IPEANRCYLPFPreLensGainRegCmdLength,
                    reinterpret_cast<UINT32*>(&pRegCmd[passNumber].cylpfPreLensGainCmd));
                CAMX_ASSERT(CamxResultSuccess == result);

                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                    regIPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_YFILTER_LUT_THR_Y_0,
                    IPEANRCYLPFLensGainRegCmdLength,
                    reinterpret_cast<UINT32*>(&pRegCmd[passNumber].cylpfLensGainCmd));
                CAMX_ASSERT(CamxResultSuccess == result);

                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                    regIPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_UVLIMIT,
                    IPEANRCYLPFPostLensGainRegCmdLength,
                    reinterpret_cast<UINT32*>(&pRegCmd[passNumber].cylpfPostLensGainCmd));
                CAMX_ASSERT(CamxResultSuccess == result);

                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                    regIPE_IPE_0_NPS_CLC_ANR_CNR_BYPASS,
                    IPEANRCNRRegCmdLength,
                    reinterpret_cast<UINT32*>(&pRegCmd[passNumber].cnrCmd));
                CAMX_ASSERT(CamxResultSuccess == result);
            }
        }
        else
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Can't Find Cmd Buffer");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10Titan17x::ValidateAndCorrectPreviewsAndvideoParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEANR10Titan17x::ValidateAndCorrectPreviewsAndvideoParameters(
    AnrParameters* pAnrParameters,
    UINT32         numOfPasses)
{
    AnrParameters*    pFWStruct = pAnrParameters;
    IPEANRRegCmd17x*  pRegPass  = static_cast<IPEANRRegCmd17x*>(m_pRegCmd);

    for (UINT32 pass = 0; pass < numOfPasses; pass++)
    {
        if (TRUE == pFWStruct->parameters[pass].moduleCfg.EN)
        {
            if (PASS_NAME_FULL == pass)
            {
                ValidateAndSetParams(&pFWStruct->parameters[pass].filter2Parameters.filter2Bypass.
                    CYLPF_FILTER2_BYPASS, pass, 1, "Preview/Video FIL2 Config");
                ValidateAndSetParams(&pFWStruct->parameters[pass].rnfParameters.rnfBypass.
                    RNF_BYPASS__LUMA, pass, 1, "Preview/Video RNF LUMA Config");
                ValidateAndSetParams(&pFWStruct->parameters[pass].rnfParameters.rnfBypass.
                    RNF_BYPASS__CHROMA, pass, 1, "Preview/Video RNF CHROMA Config");
            }
            else if (PASS_NAME_DC_4 == pass)
            {
                ValidateAndSetParams(&pFWStruct->parameters[pass].filter2Parameters.filter2Bypass.
                    CYLPF_FILTER2_BYPASS, pass, 0, "Preview/Video FIL2 Config");
                ValidateAndSetParams(&pFWStruct->parameters[pass].rnfParameters.rnfBypass.
                    RNF_BYPASS__LUMA, pass, 0, "Preview/Video RNF LUMA Config");
                ValidateAndSetParams(&pFWStruct->parameters[pass].rnfParameters.rnfBypass.
                    RNF_BYPASS__CHROMA, pass, 0, "Preview/Video RNF CHROMA Config");
            }
            else if (PASS_NAME_DC_16 == pass)
            {
                ValidateAndSetParams(&pFWStruct->parameters[pass].filter2Parameters.filter2Bypass.
                     CYLPF_FILTER2_BYPASS, pass, 0, "Preview/Video FIL2 Config");
                ValidateAndSetParams(&pFWStruct->parameters[pass].rnfParameters.rnfBypass.
                    RNF_BYPASS__LUMA, pass, 0, "Preview/Video RNF LUMA Config");
                ValidateAndSetParams(&pFWStruct->parameters[pass].rnfParameters.rnfBypass.
                    RNF_BYPASS__CHROMA, pass, 0, "Preview/Video RNF CHROMA Config");

                // Below parameters should be bypassed in Video/Preview mode
                CAMX_LOG_INFO(CamxLogGroupPProc, "DCBLEND1_BYPASS = %d, DCUS_BYPASS = %d for Pass %d",
                    pRegPass[PASS_TYPE_DC16].dcBlend1Cmd.bypass.bitfields.DCBLEND1_BYPASS,
                    pRegPass[PASS_TYPE_DC16].dcusCmd.bypass.bitfields.DCUS_BYPASS,
                    PASS_TYPE_DC64);

                pRegPass[PASS_TYPE_DC16].dcBlend1Cmd.bypass.bitfields.DCBLEND1_BYPASS = 1;
                pRegPass[PASS_TYPE_DC16].dcusCmd.bypass.bitfields.DCUS_BYPASS = 1;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10Titan17x::ValidateAndCorrectStillModeParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEANR10Titan17x::ValidateAndCorrectStillModeParameters(
    AnrParameters* pAnrParameters,
    UINT32         numOfPasses)
{
    AnrParameters*    pFWStruct = pAnrParameters;
    IPEANRRegCmd17x*  pRegPass  = static_cast<IPEANRRegCmd17x*>(m_pRegCmd);

    for (UINT32 pass = 0; pass < numOfPasses; pass++)
    {
        if (TRUE == pFWStruct->parameters[pass].moduleCfg.EN)
        {
            if (PASS_NAME_FULL == pass)
            {
                ValidateAndSetParams(&pFWStruct->parameters[pass].filter2Parameters.filter2Bypass.
                    CYLPF_FILTER2_BYPASS, pass, 0, "Snapshot/Still processing FIL2 Config");
                ValidateAndSetParams(&pFWStruct->parameters[pass].rnfParameters.rnfBypass.
                    RNF_BYPASS__LUMA, pass, 0, "Snapshot/Still processing RNF LUMA Config");
                ValidateAndSetParams(&pFWStruct->parameters[pass].rnfParameters.rnfBypass.
                    RNF_BYPASS__CHROMA, pass, 0, "Snapshot/Still processing RNF CHROMA Config");
            }
            else if (PASS_NAME_DC_4 == pass)
            {
                ValidateAndSetParams(&pFWStruct->parameters[pass].filter2Parameters.
                    filter2Bypass.CYLPF_FILTER2_BYPASS, pass, 0, "Snapshot/Still processing FIL2 Config");
                ValidateAndSetParams(&pFWStruct->parameters[pass].rnfParameters.rnfBypass.
                    RNF_BYPASS__LUMA, pass, 0, "Snapshot/Still processing RNF LUMA Config");
                ValidateAndSetParams(&pFWStruct->parameters[pass].rnfParameters.rnfBypass.
                    RNF_BYPASS__CHROMA, pass, 0, "Snapshot/Still processing RNF CHROMA Config");
            }
            else if (PASS_NAME_DC_16 == pass)
            {
                ValidateAndSetParams(&pFWStruct->parameters[pass].filter2Parameters.
                    filter2Bypass.CYLPF_FILTER2_BYPASS, pass, 0, "Snapshot/Still processing FIL2 Config");
                ValidateAndSetParams(&pFWStruct->parameters[pass].rnfParameters.rnfBypass.
                    RNF_BYPASS__LUMA, pass, 0, "Snapshot/Still processing RNF LUMA Config");
                ValidateAndSetParams(&pFWStruct->parameters[pass].rnfParameters.rnfBypass.
                    RNF_BYPASS__CHROMA, pass, 0, "Snapshot/Still processing RNF CHROMA Config");
            }
            else if (PASS_NAME_DC_64 == pass)
            {
                ValidateAndSetParams(&pFWStruct->parameters[pass].filter2Parameters.
                    filter2Bypass.CYLPF_FILTER2_BYPASS, pass, 0, "Snapshot/Still processing FIL2 Config");
                ValidateAndSetParams(&pFWStruct->parameters[pass].rnfParameters.rnfBypass.
                    RNF_BYPASS__LUMA, pass, 0, "Snapshot/Still processing RNF LUMA Config");
                ValidateAndSetParams(&pFWStruct->parameters[pass].rnfParameters.rnfBypass.
                    RNF_BYPASS__CHROMA, pass, 0, "Snapshot/Still processing CHROMA Config");

                // Below parameters should be bypassed in Still mode
                CAMX_LOG_INFO(CamxLogGroupPProc, "DCBLEND1_BYPASS = %d, DCUS_BYPASS = %d for Pass %d",
                    pRegPass[PASS_TYPE_DC64].dcBlend1Cmd.bypass.bitfields.DCBLEND1_BYPASS,
                    pRegPass[PASS_TYPE_DC64].dcusCmd.bypass.bitfields.DCUS_BYPASS,
                    PASS_TYPE_DC64);

                pRegPass[PASS_TYPE_DC64].dcBlend1Cmd.bypass.bitfields.DCBLEND1_BYPASS = 1;
                pRegPass[PASS_TYPE_DC64].dcusCmd.bypass.bitfields.DCUS_BYPASS = 1;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10Titan17x::ValidateAndCorrectAnrSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEANR10Titan17x::ValidateAndCorrectAnrSettings(
    AnrParameters*  pAnrParameters,
    BOOL            isRealTime,
    UINT32          numOfPasses)
{
    AnrParameters*    pFWStruct = pAnrParameters;
    IPEANRRegCmd17x*  pRegPass  = static_cast<IPEANRRegCmd17x*>(m_pRegCmd);

    if (TRUE == isRealTime)
    {
        ValidateAndCorrectPreviewsAndvideoParameters(pFWStruct, numOfPasses);
    }
    else
    {
        ValidateAndCorrectStillModeParameters(pFWStruct, numOfPasses);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEANR10Titan17x::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutput)
{
    CamxResult result = CamxResultSuccess;
    INT32      intRet = 0;

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pInput)
    {
        ANR10InputData*   pInputData                  = static_cast<ANR10InputData*>(pInput);
        ANR_Chromatix*    pAnrChromatix               = static_cast<ANR_Chromatix*>(pInputData->pNCChromatix);
        NCLIB_CONTEXT_ANR nclibContext;
        IPEANRRegCmd17x*  pRegPass                    = static_cast<IPEANRRegCmd17x*>(m_pRegCmd);
        AnrParameters*    pFWStruct                   = static_cast<AnrParameters*>(pInputData->pANRParameters);
        UINT32            numOfPasses                 = pInputData->numPasses;
        UINT32            imageWidthPixelsWithMargin  = pInputData->pImageDimensions->widthPixels;
        UINT32            imageHeightPixelsWithMargin = pInputData->pImageDimensions->heightLines;
        UINT32            imageWidthPixels            = (NULL != pInputData->pMarginDimensions) ?
            imageWidthPixelsWithMargin - pInputData->pMarginDimensions->widthPixels : imageWidthPixelsWithMargin;
        UINT32            imageHeightPixels           = (NULL != pInputData->pMarginDimensions) ?
            imageHeightPixelsWithMargin - pInputData->pMarginDimensions->heightLines : imageHeightPixelsWithMargin;
        UINT32            mE                          = pInputData->bitWidth - 8;

        IQSettingUtils::Memset(pRegPass, 0x0, sizeof(IPEANRRegCmd17x) * numOfPasses);
        IQSettingUtils::Memset(pFWStruct, 0x0, sizeof(AnrParameters));
        IQSettingUtils::Memset(&nclibContext, 0x0, sizeof(NCLIB_CONTEXT_ANR));
        nclibContext.frameNumber = static_cast<uint32_t>(pInputData->frameNum);
        ANR10Setting::UpdateGeometryParameters(pInputData, &nclibContext);

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "numOfPasses %d, w %d, h %d, marginw %d, marginH %d",
                         numOfPasses, imageWidthPixels, imageHeightPixels,
                         imageWidthPixelsWithMargin, imageHeightPixelsWithMargin);

        intRet = ANR_FlowProcessNcLib17x(pAnrChromatix, &nclibContext, pRegPass, pFWStruct, numOfPasses,
            imageWidthPixels, imageHeightPixels, imageWidthPixelsWithMargin,
            imageHeightPixelsWithMargin, pInputData->bitWidth);
        if (NC_LIB_SUCCESS != intRet)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "ANR_FlowProcessNcLib failed %d", intRet);
            result = CamxResultEFailed;
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "ANR10Setting::CalculateHWSetting success");

            if (TRUE == pInputData->validateANRSettings)
            {
                // intRet = ANR_ValidateNcLibRegs17x(pRegPass, numOfPasses, mE);
                // if (NC_LIB_SUCCESS != intRet)
                // {
                //      CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Validation of registers returned failure %d", intRet);
                // }
                // else
                // {
                //     CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Validation of registers returned success %d", intRet);
                // }
                ValidateAndCorrectAnrSettings(pFWStruct, pInputData->realtimeFlag, numOfPasses);
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid argument");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEANR10Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    ISPInputData* pInputData = static_cast<ISPInputData*>(pInput);
    CamxResult    result     = CamxResultSuccess;

    RunCalculationFullPass(pInputData);
    RunCalculationDS4Pass(pInputData);
    RunCalculationDS16Pass(pInputData);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEANR10Titan17x::UpdateTuningMetadata(
    VOID*  pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    IPETuningMetadata* pIPETuningMetadata = static_cast<IPETuningMetadata*>(pInputData->pIPETuningMetadata);
    IpeIQSettings*     pIPEIQSettings     = static_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);;
    IPEANRRegCmd17x*   pRegCmd            = static_cast<IPEANRRegCmd17x*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if ((NULL != pInputData->pIPETuningMetadata) && (NULL != pRegCmd))
    {
        CAMX_STATIC_ASSERT((sizeof(IPEANRRegCmd17x) * PASS_NAME_MAX) ==
            sizeof(pIPETuningMetadata->IPETuningMetadata17x.IPEANRData.ANRData));

        for (UINT passNumber = PASS_NAME_FULL; passNumber < PASS_NAME_MAX; ++passNumber)
        {
            Utils::Memcpy(&pIPETuningMetadata->IPETuningMetadata17x.IPEANRData.ANRData[passNumber],
                          &pRegCmd[passNumber],
                          sizeof(IPEANRRegCmd17x));
        }

        if (TRUE == pIPEIQSettings->anrParameters.parameters[PASS_NAME_FULL].moduleCfg.EN)
        {
            DebugDataTagID dataTagID;

            if (TRUE == pInputData->pipelineIPEData.realtimeFlag)
            {
                dataTagID = DebugDataTagID::TuningIPEANR10Register;
            }
            else
            {
                dataTagID = DebugDataTagID::TuningIPEANR10RegisterOffline;
            }

            result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                dataTagID,
                DebugDataTagType::TuningANR10Config,
                CAMX_ARRAY_SIZE(pIPETuningMetadata->IPETuningMetadata17x.IPEANRData.ANRData),
                &pIPETuningMetadata->IPETuningMetadata17x.IPEANRData.ANRData,
                sizeof(pIPETuningMetadata->IPETuningMetadata17x.IPEANRData.ANRData));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEANR10Titan17x::DumpRegConfig()
{
    IPEANRRegCmd17x* pRegCmd            = static_cast<IPEANRRegCmd17x*>(m_pRegCmd);
    CHAR             dumpFilename[256];
    FILE*            pFile              = NULL;
    CamX::OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename), "%s/IPE_regdump.txt", ConfigFileDirectory);
    pFile = CamX::OsUtils::FOpen(dumpFilename, "a+");

    if (NULL != pFile)
    {
        CamX::OsUtils::FPrintF(pFile, "******** IPE ANR10 [HEX] ********\n");

        for (UINT passNumber = PASS_NAME_FULL; passNumber < PASS_NAME_MAX; passNumber++)
        {
            CamX::OsUtils::FPrintF(pFile, "== PASSNUMBER = %d =================================\n", passNumber);
            CamX::OsUtils::FPrintF(pFile, "dcBlend1Cmd.bypass                            = %x\n",
                pRegCmd[passNumber].dcBlend1Cmd.bypass);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.bypass                                 = %x\n",
                pRegCmd[passNumber].rnfCmd.bypass);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.subBypass                              = %x\n",
                pRegCmd[passNumber].rnfCmd.subBypass);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.debug                                  = %x\n",
                pRegCmd[passNumber].rnfCmd.debug);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.indRankConfigYCFG                      = %x\n",
                pRegCmd[passNumber].rnfCmd.indRankConfigYCFG);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.indRankYHV                             = %x\n",
                pRegCmd[passNumber].rnfCmd.indRankYHV);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.indRankYDG                             = %x\n",
                pRegCmd[passNumber].rnfCmd.indRankYDG);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.indRankCCFG                            = %x\n",
                pRegCmd[passNumber].rnfCmd.indRankCCFG);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.indRankCHV                             = %x\n",
                pRegCmd[passNumber].rnfCmd.indRankCHV);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.indRankCDG                             = %x\n",
                pRegCmd[passNumber].rnfCmd.indRankCDG);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.peakConfigY                            = %x\n",
                pRegCmd[passNumber].rnfCmd.peakConfigY);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.peakConfigc                            = %x\n",
                pRegCmd[passNumber].rnfCmd.peakConfigc);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.peakImgCondS0Y                         = %x\n",
                pRegCmd[passNumber].rnfCmd.peakImgCondS0Y);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.peakImgCondS1Y                         = %x\n",
                pRegCmd[passNumber].rnfCmd.peakImgCondS1Y);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.peakImgCondS0C                         = %x\n",
                pRegCmd[passNumber].rnfCmd.peakImgCondS0C);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.peakImgCondS1C                         = %x\n",
                pRegCmd[passNumber].rnfCmd.peakImgCondS1C);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.peakDCCondS0Y                          = %x\n",
                pRegCmd[passNumber].rnfCmd.peakDCCondS0Y);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.peakDCCondS1Y                          = %x\n",
                pRegCmd[passNumber].rnfCmd.peakDCCondS1Y);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.peakDCCondS0C                          = %x\n",
                pRegCmd[passNumber].rnfCmd.peakDCCondS0C);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.peakDCCondS1C                          = %x\n",
                pRegCmd[passNumber].rnfCmd.peakDCCondS1C);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.peakLumaIsoThr                         = %x\n",
                pRegCmd[passNumber].rnfCmd.peakLumaIsoThr);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.peakChromaIsoThr                       = %x\n",
                pRegCmd[passNumber].rnfCmd.peakChromaIsoThr);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.firConfigY                             = %x\n",
                pRegCmd[passNumber].rnfCmd.firConfigY);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.firConfigC                             = %x\n",
                pRegCmd[passNumber].rnfCmd.firConfigC);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.firDirThrY                             = %x\n",
                pRegCmd[passNumber].rnfCmd.firDirThrY);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.firDirThrC                             = %x\n",
                pRegCmd[passNumber].rnfCmd.firDirThrC);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.firTransY0                             = %x\n",
                pRegCmd[passNumber].rnfCmd.firTransY0);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.firTransY1                             = %x\n",
                pRegCmd[passNumber].rnfCmd.firTransY1);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.firTransY2                             = %x\n",
                pRegCmd[passNumber].rnfCmd.firTransY2);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.firTransC0                             = %x\n",
                pRegCmd[passNumber].rnfCmd.firTransC0);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.firTransC1                             = %x\n",
                pRegCmd[passNumber].rnfCmd.firTransC1);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.firTransC2                             = %x\n",
                pRegCmd[passNumber].rnfCmd.firTransC2);
            CamX::OsUtils::FPrintF(pFile, "rnfCmd.firTransC3                             = %x\n",
                pRegCmd[passNumber].rnfCmd.firTransC3);
            CamX::OsUtils::FPrintF(pFile, "dcusCmd.bypass                                = %x\n",
                pRegCmd[passNumber].dcusCmd.bypass);
            CamX::OsUtils::FPrintF(pFile, "filter2Cmd.byPass                             = %x\n",
                pRegCmd[passNumber].filter2Cmd.byPass);
            CamX::OsUtils::FPrintF(pFile, "filter2Cmd.debug                              = %x\n",
                pRegCmd[passNumber].filter2Cmd.debug);
            CamX::OsUtils::FPrintF(pFile, "filter2Cmd.subBypass                          = %x\n",
                pRegCmd[passNumber].filter2Cmd.subBypass);
            CamX::OsUtils::FPrintF(pFile, "filter2Cmd.chromaFiltering                    = %x\n",
                pRegCmd[passNumber].filter2Cmd.chromaFiltering);
            CamX::OsUtils::FPrintF(pFile, "filter2Cmd.chromaIndThr9                      = %x\n",
                pRegCmd[passNumber].filter2Cmd.chromaIndThr9);
            CamX::OsUtils::FPrintF(pFile, "filter2Cmd.chromaIndThr11                     = %x\n",
                pRegCmd[passNumber].filter2Cmd.chromaIndThr11);
            CamX::OsUtils::FPrintF(pFile, "filter2Cmd.greyIndThr9                        = %x\n",
                pRegCmd[passNumber].filter2Cmd.greyIndThr9);
            CamX::OsUtils::FPrintF(pFile, "filter2Cmd.greyIndThr11                       = %x\n",
                pRegCmd[passNumber].filter2Cmd.greyIndThr11);
            CamX::OsUtils::FPrintF(pFile, "filter2Cmd.chroma                             = %x\n",
                pRegCmd[passNumber].filter2Cmd.chroma);
            CamX::OsUtils::FPrintF(pFile, "filter2Cmd.medianThr                          = %x\n",
                pRegCmd[passNumber].filter2Cmd.medianThr);
            CamX::OsUtils::FPrintF(pFile, "filter2Cmd.medianThrHV                        = %x\n",
                pRegCmd[passNumber].filter2Cmd.medianThrHV);
            CamX::OsUtils::FPrintF(pFile, "filter2Cmd.medianThrDG12                      = %x\n",
                pRegCmd[passNumber].filter2Cmd.medianThrDG12);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.lumaBypass                        = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.lumaBypass);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.chromaBypass                      = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.chromaBypass);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.yDebug                            = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.yDebug);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.cDebug                            = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.cDebug);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.lumaConfig                        = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.lumaConfig);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.chromaConfig                      = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.chromaConfig);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.yBlend1X1                         = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.yBlend1X1);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.yBlend3X3                         = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.yBlend3X3);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.yBlend5X5                         = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.yBlend5X5);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.yBlend7X7                         = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.yBlend7X7);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.yBlend9X9                         = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.yBlend9X9);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.yBlend1X1Alt                      = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.yBlend1X1Alt);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.yBlend3X3Alt                      = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.yBlend3X3Alt);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.yBlend5X5Alt                      = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.yBlend5X5Alt);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.yBlend7X7Alt                      = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.yBlend7X7Alt);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.yBlend9X9Alt                      = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.yBlend9X9Alt);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.cBlend1X1                         = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.cBlend1X1);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.cBlend3X3                         = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.cBlend3X3);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.cBlend5X5                         = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.cBlend5X5);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.cBlend7X7                         = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.cBlend7X7);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.cBlend9X9                         = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.cBlend9X9);
            CamX::OsUtils::FPrintF(pFile, "dcBlend2Cmd.cblend11X11                       = %x\n",
                pRegCmd[passNumber].dcBlend2Cmd.cblend11X11);
            CamX::OsUtils::FPrintF(pFile, "cylpfPreLensGainCmd.bypass                    = %x\n",
                pRegCmd[passNumber].cylpfPreLensGainCmd.bypass);
            CamX::OsUtils::FPrintF(pFile, "cylpfPreLensGainCmd.subBypass                 = %x\n",
                pRegCmd[passNumber].cylpfPreLensGainCmd.subBypass);
            CamX::OsUtils::FPrintF(pFile, "cylpfPreLensGainCmd.debug                     = %x\n",
                pRegCmd[passNumber].cylpfPreLensGainCmd.debug);
            CamX::OsUtils::FPrintF(pFile, "cylpfPreLensGainCmd.config                    = %x\n",
                pRegCmd[passNumber].cylpfPreLensGainCmd.config);
            CamX::OsUtils::FPrintF(pFile, "cylpfPreLensGainCmd.tcfConfig                 = %x\n",
                pRegCmd[passNumber].cylpfPreLensGainCmd.tcfConfig);
            CamX::OsUtils::FPrintF(pFile, "cylpfPreLensGainCmd.tcfRadius                 = %x\n",
                pRegCmd[passNumber].cylpfPreLensGainCmd.tcfRadius);
            CamX::OsUtils::FPrintF(pFile, "cylpfPreLensGainCmd.tcfLowThrLUT0             = %x\n",
                pRegCmd[passNumber].cylpfPreLensGainCmd.tcfLowThrLUT0);
            CamX::OsUtils::FPrintF(pFile, "cylpfPreLensGainCmd.tcfLowThrLUT1             = %x\n",
                pRegCmd[passNumber].cylpfPreLensGainCmd.tcfLowThrLUT1);
            CamX::OsUtils::FPrintF(pFile, "cylpfPreLensGainCmd.tcfLowThrLUT2             = %x\n",
                pRegCmd[passNumber].cylpfPreLensGainCmd.tcfLowThrLUT2);
            CamX::OsUtils::FPrintF(pFile, "cylpfPreLensGainCmd.tcfHighThrLUT0            = %x\n",
                pRegCmd[passNumber].cylpfPreLensGainCmd.tcfHighThrLUT0);
            CamX::OsUtils::FPrintF(pFile, "cylpfPreLensGainCmd.tcfHighThrLUT1            = %x\n",
                pRegCmd[passNumber].cylpfPreLensGainCmd.tcfHighThrLUT1);
            CamX::OsUtils::FPrintF(pFile, "cylpfPreLensGainCmd.tcfHighThrLUT2            = %x\n",
                pRegCmd[passNumber].cylpfPreLensGainCmd.tcfHighThrLUT2);
            CamX::OsUtils::FPrintF(pFile, "cylpfPreLensGainCmd.tcfLowThrLimit            = %x\n",
                pRegCmd[passNumber].cylpfPreLensGainCmd.tcfLowThrLimit);
            CamX::OsUtils::FPrintF(pFile, "cylpfPreLensGainCmd.tcfHighThrLimit           = %x\n",
                pRegCmd[passNumber].cylpfPreLensGainCmd.tcfHighThrLimit);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgYFilterLUTThrY0            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgYFilterLUTThrY0);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgYFilterLUTThrY1            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgYFilterLUTThrY1);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgYFilterLUTThrY2            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgYFilterLUTThrY2);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgYFilterLUTThrY3            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgYFilterLUTThrY3);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgYFilterLUTThrY4            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgYFilterLUTThrY4);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgYFilterLUTThrY5            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgYFilterLUTThrY5);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgYFilterLUTThrY6            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgYFilterLUTThrY6);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgYFilterLUTThrY7            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgYFilterLUTThrY7);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgYFilterLUTThrY8            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgYFilterLUTThrY8);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgYFilterLUTThrC0            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgYFilterLUTThrC0);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgYFilterLUTThrC1            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgYFilterLUTThrC1);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgYFilterLUTThrC2            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgYFilterLUTThrC2);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgYFilterLUTThrC3            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgYFilterLUTThrC3);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgYFilterLUTThrC4            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgYFilterLUTThrC4);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgYFilterLUTThrC5            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgYFilterLUTThrC5);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgYFilterLUTThrC6            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgYFilterLUTThrC6);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgYFilterLUTThrC7            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgYFilterLUTThrC7);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgYFilterLUTThrC8            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgYFilterLUTThrC8);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgCFilterLUTThrY0            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgCFilterLUTThrY0);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgCFilterLUTThrY1            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgCFilterLUTThrY1);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgCFilterLUTThrY2            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgCFilterLUTThrY2);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgCFilterLUTThrY3            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgCFilterLUTThrY3);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgCFilterLUTThrY4            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgCFilterLUTThrY4);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgCFilterLUTThrY5            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgCFilterLUTThrY5);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgCFilterLUTThrY6            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgCFilterLUTThrY6);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgCFilterLUTThrY7            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgCFilterLUTThrY7);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgCFilterLUTThrY8            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgCFilterLUTThrY8);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgCFilterLUTThrC0            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgCFilterLUTThrC0);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgCFilterLUTThrC1            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgCFilterLUTThrC1);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgCFilterLUTThrC2            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgCFilterLUTThrC2);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgCFilterLUTThrC3            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgCFilterLUTThrC3);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgCFilterLUTThrC4            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgCFilterLUTThrC4);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgCFilterLUTThrC5            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgCFilterLUTThrC5);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgCFilterLUTThrC6            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgCFilterLUTThrC6);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgCFilterLUTThrC7            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgCFilterLUTThrC7);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgCFilterLUTThrC8            = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgCFilterLUTThrC8);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgLUTBlend0                  = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgLUTBlend0);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgLUTBlend1                  = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgLUTBlend1);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgLUTBlend2                  = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgLUTBlend2);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgLUTBlend3                  = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgLUTBlend3);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgLUTBlend4                  = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgLUTBlend4);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgLUTBlend5                  = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgLUTBlend5);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgLUTBlend6                  = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgLUTBlend6);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgLUTBlend7                  = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgLUTBlend7);
            CamX::OsUtils::FPrintF(pFile, "cylpfLensGainCmd.lgLUTBlend8                  = %x\n",
                pRegCmd[passNumber].cylpfLensGainCmd.lgLUTBlend8);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterUVLmit            = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterUVLmit);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterBaseBlendFarY     = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterBaseBlendFarY);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterBaseBlendFarC     = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterBaseBlendFarC);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterYYTB0             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterYYTB0);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterYYTB1             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterYYTB1);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterYYTB2             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterYYTB2);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterYYTB3             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterYYTB3);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterYYTB4             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterYYTB4);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterYYTB5             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterYYTB5);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterYCTB0             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterYCTB0);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterYCTB1             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterYCTB1);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterYCTB2             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterYCTB2);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterYCTB3             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterYCTB3);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterYTBLimit          = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterYTBLimit);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterUYTB0             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterUYTB0);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterUYTB1             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterUYTB1);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterUYTB2             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterUYTB2);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterUYTB3             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterUYTB3);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterUYTB4             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterUYTB4);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterUCTB0             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterUCTB0);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterUCTB1             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterUCTB1);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterUCTB2             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterUCTB2);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterUTBLimit          = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterUTBLimit);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterVYTB0             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterVYTB0);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterVYTB1             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterVYTB1);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterVYTB2             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterVYTB2);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterVYTB3             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterVYTB3);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterVYTB4             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterVYTB4);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterVCTB0             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterVCTB0);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterVCTB1             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterVCTB1);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterVCTB2             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterVCTB2);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterVTBLimit          = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterVTBLimit);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterUVLimit           = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterUVLimit);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterBaseBlendFarY     = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterBaseBlendFarY);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterBaseBlendFarC     = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterBaseBlendFarC);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterYYTB0             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterYYTB0);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterYYTB1             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterYYTB1);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterYYTB2             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterYYTB2);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterYYTB3             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterYYTB3);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterYYTB4             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterYYTB4);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterYYTB5             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterYYTB5);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterYCTB0             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterYCTB0);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterYCTB1             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterYCTB1);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterYCTB2             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterYCTB2);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterYCTB3             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterYCTB3);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterYTBLimit          = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterYTBLimit);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterUYTB0             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterUYTB0);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterUYTB1             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterUYTB1);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterUYTB2             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterUYTB2);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterUYTB3             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterUYTB3);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterUYTB4             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterUYTB4);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterUCTB0             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterUCTB0);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterUCTB1             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterUCTB1);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterUCTB2             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterUCTB2);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterUTBLimit          = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterUTBLimit);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterVYTB0             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterVYTB0);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterVYTB1             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterVYTB1);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterVYTB2             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterVYTB2);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterVYTB3             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterVYTB3);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterVYTB4             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterVYTB4);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterVCTB0             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterVCTB0);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterVCTB1             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterVCTB1);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterVCTB2             = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterVCTB2);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterVTBLimit          = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterVTBLimit);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterConfig            = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterConfig);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterKernel            = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterKernel);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterAltKernel         = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterAltKernel);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterDCIndFlags        = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterDCIndFlags);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterDerFlags          = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterDerFlags);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterDer2Flags         = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterDer2Flags);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterDirConfig         = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterDirConfig);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterExternalDirConfig = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterExternalDirConfig);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterYThrehClose3Mod   = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterYThrehClose3Mod);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterYThreshDer2Close3Mod   = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterYThreshDer2Close3Mod);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterYThrFarMod        = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterYThrFarMod);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterUThrFarMod        = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterUThrFarMod);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterVThrFarMod        = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterVThrFarMod);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterYThrCloseMod      = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterYThrCloseMod);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterYThrFarExMod      = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterYThrFarExMod);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterUThrFarExMod      = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterUThrFarExMod);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yFilterVThrFarExMod      = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yFilterVThrFarExMod);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yDCIndYGain30            = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yDCIndYGain30);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yDCIndYGain50            = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yDCIndYGain50);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yDCIndYGain70            = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yDCIndYGain70);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yDCIndYGain90            = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yDCIndYGain90);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yDCIndYGain5X0           = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yDCIndYGain5X0);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yDCIndYGain9X0           = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yDCIndYGain9X0);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yDCIndUVGain30           = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yDCIndUVGain30);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yDCIndUVGain50           = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yDCIndUVGain50);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.yDCIndUVGain5X0          = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.yDCIndUVGain5X0);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterConfig            = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterConfig);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterDetailCond        = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterDetailCond);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cThreshold               = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cThreshold);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterGreyYDer          = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterGreyYDer);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterGreyChromaticity  = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterGreyChromaticity);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterGreyLimits        = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterGreyLimits);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterGreyBlendScale    = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterGreyBlendScale);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterGreyBlendOffset   = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterGreyBlendOffset);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterGreySizeThr35     = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterGreySizeThr35);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterGreySizeThr79     = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterGreySizeThr79);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterGreyThrClose      = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterGreyThrClose);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterGreyThrFar        = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterGreyThrFar);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterDCIndFlags        = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterDCIndFlags);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterDerFlags          = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterDerFlags);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterDirConfig         = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterDirConfig);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterDirConfigEx       = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterDirConfigEx);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterYThrFarMod        = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterYThrFarMod);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterUThrFarMod        = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterUThrFarMod);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterVThrFarMod        = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterVThrFarMod);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterUThrDistMod       = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterUThrDistMod);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterVThrDisMod        = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterVThrDisMod);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterYThrCloseMod      = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterYThrCloseMod);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterYThrFarExMod      = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterYThrFarExMod);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterUThrFarExMod      = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterUThrFarExMod);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cFilterVThrFarExMod      = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cFilterVThrFarExMod);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cDCIndYGain30            = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cDCIndYGain30);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cDCIndYGain50            = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cDCIndYGain50);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cDCIndYGain70            = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cDCIndYGain70);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cDCIndYGain90            = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cDCIndYGain90);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cDCIndYGain5X0           = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cDCIndYGain5X0);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cDCIndYGain9X0           = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cDCIndYGain9X0);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cDCIndUVGain30           = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cDCIndUVGain30);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cDCIndUVGain50           = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cDCIndUVGain50);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cDCIndUVGain70           = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cDCIndUVGain70);
            CamX::OsUtils::FPrintF(pFile, "cylpfPostLensGainCmd.cDCIndUVGain5X0          = %x\n",
                pRegCmd[passNumber].cylpfPostLensGainCmd.cDCIndUVGain5X0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.bypass                                 = %x\n",
                pRegCmd[passNumber].cnrCmd.bypass);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.config                                 = %x\n",
                pRegCmd[passNumber].cnrCmd.config);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.debug                                  = %x\n",
                pRegCmd[passNumber].cnrCmd.debug);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.filterMode                             = %x\n",
                pRegCmd[passNumber].cnrCmd.filterMode);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.specialParams                          = %x\n",
                pRegCmd[passNumber].cnrCmd.specialParams);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.cMode                                  = %x\n",
                pRegCmd[passNumber].cnrCmd.cMode);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.angleMin0                              = %x\n",
                pRegCmd[passNumber].cnrCmd.angleMin0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.angleMin1                              = %x\n",
                pRegCmd[passNumber].cnrCmd.angleMin1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.angleMin2                              = %x\n",
                pRegCmd[passNumber].cnrCmd.angleMin2);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.angleMax0                              = %x\n",
                pRegCmd[passNumber].cnrCmd.angleMax0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.angleMax1                              = %x\n",
                pRegCmd[passNumber].cnrCmd.angleMax1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.angleMax2                              = %x\n",
                pRegCmd[passNumber].cnrCmd.angleMax2);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.angleInternal                          = %x\n",
                pRegCmd[passNumber].cnrCmd.angleInternal);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.colorSatMin0                           = %x\n",
                pRegCmd[passNumber].cnrCmd.colorSatMin0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.colorSatMin1                           = %x\n",
                pRegCmd[passNumber].cnrCmd.colorSatMin1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.colorSatMin2                           = %x\n",
                pRegCmd[passNumber].cnrCmd.colorSatMin2);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.colorSatMax0                           = %x\n",
                pRegCmd[passNumber].cnrCmd.colorSatMax0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.colorSatMax1                           = %x\n",
                pRegCmd[passNumber].cnrCmd.colorSatMax1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.colorSatMax2                           = %x\n",
                pRegCmd[passNumber].cnrCmd.colorSatMax2);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.yCompMin0                              = %x\n",
                pRegCmd[passNumber].cnrCmd.yCompMin0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.yCompMin1                              = %x\n",
                pRegCmd[passNumber].cnrCmd.yCompMin1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.yCompMax0                              = %x\n",
                pRegCmd[passNumber].cnrCmd.yCompMax0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.yCompMax1                              = %x\n",
                pRegCmd[passNumber].cnrCmd.yCompMax1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.boundaryProbability                    = %x\n",
                pRegCmd[passNumber].cnrCmd.boundaryProbability);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.q0                                     = %x\n",
                pRegCmd[passNumber].cnrCmd.q0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.q1                                     = %x\n",
                pRegCmd[passNumber].cnrCmd.q1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.gainFactorYY0                          = %x\n",
                pRegCmd[passNumber].cnrCmd.gainFactorYY0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.gainFactorYY1                          = %x\n",
                pRegCmd[passNumber].cnrCmd.gainFactorYY1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.gainFactorYY2                          = %x\n",
                pRegCmd[passNumber].cnrCmd.gainFactorYY2);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.gainFactorUVY0                         = %x\n",
                pRegCmd[passNumber].cnrCmd.gainFactorUVY0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.gainFactorUVY1                         = %x\n",
                pRegCmd[passNumber].cnrCmd.gainFactorUVY1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.gainFactorUVY2                         = %x\n",
                pRegCmd[passNumber].cnrCmd.gainFactorUVY2);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.offsetYY0                              = %x\n",
                pRegCmd[passNumber].cnrCmd.offsetYY0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.offsetYY1                              = %x\n",
                pRegCmd[passNumber].cnrCmd.offsetYY1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.offsetYY2                              = %x\n",
                pRegCmd[passNumber].cnrCmd.offsetYY2);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.offsetUY0                              = %x\n",
                pRegCmd[passNumber].cnrCmd.offsetUY0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.offsetUY1                              = %x\n",
                pRegCmd[passNumber].cnrCmd.offsetUY1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.offsetVY0                              = %x\n",
                pRegCmd[passNumber].cnrCmd.offsetVY0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.offsetVY1                              = %x\n",
                pRegCmd[passNumber].cnrCmd.offsetVY1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.gainFactorYC0                          = %x\n",
                pRegCmd[passNumber].cnrCmd.gainFactorYC0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.gainFactorYC1                          = %x\n",
                pRegCmd[passNumber].cnrCmd.gainFactorYC1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.gainFactorYC2                          = %x\n",
                pRegCmd[passNumber].cnrCmd.gainFactorYC2);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.gainFactorUVC0                         = %x\n",
                pRegCmd[passNumber].cnrCmd.gainFactorUVC0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.gainFactorUVC1                         = %x\n",
                pRegCmd[passNumber].cnrCmd.gainFactorUVC1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.gainFactorUVC2                         = %x\n",
                pRegCmd[passNumber].cnrCmd.gainFactorUVC2);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.offsetYC0                              = %x\n",
                pRegCmd[passNumber].cnrCmd.offsetYC0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.offsetYC1                              = %x\n",
                pRegCmd[passNumber].cnrCmd.offsetYC1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.offsetYC2                              = %x\n",
                pRegCmd[passNumber].cnrCmd.offsetYC2);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.offsetUC0                              = %x\n",
                pRegCmd[passNumber].cnrCmd.offsetUC0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.offsetUC1                              = %x\n",
                pRegCmd[passNumber].cnrCmd.offsetUC1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.offsetVC0                              = %x\n",
                pRegCmd[passNumber].cnrCmd.offsetVC0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.offsetVC1                              = %x\n",
                pRegCmd[passNumber].cnrCmd.offsetVC1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.blendFactorYDC0                        = %x\n",
                pRegCmd[passNumber].cnrCmd.blendFactorYDC0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.blendFactorYDC1                        = %x\n",
                pRegCmd[passNumber].cnrCmd.blendFactorYDC1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.blendFactorYALT0                       = %x\n",
                pRegCmd[passNumber].cnrCmd.blendFactorYALT0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.blendFactorYALT1                       = %x\n",
                pRegCmd[passNumber].cnrCmd.blendFactorYALT1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.blendFactorC0                          = %x\n",
                pRegCmd[passNumber].cnrCmd.blendFactorC0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.blendFactorC1                          = %x\n",
                pRegCmd[passNumber].cnrCmd.blendFactorC1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.fdOffset                               = %x\n",
                pRegCmd[passNumber].cnrCmd.fdOffset);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.xCenter0                               = %x\n",
                pRegCmd[passNumber].cnrCmd.xCenter0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.xCenter1                               = %x\n",
                pRegCmd[passNumber].cnrCmd.xCenter1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.xCenter2                               = %x\n",
                pRegCmd[passNumber].cnrCmd.xCenter2);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.yCenter0                               = %x\n",
                pRegCmd[passNumber].cnrCmd.yCenter0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.yCenter1                               = %x\n",
                pRegCmd[passNumber].cnrCmd.yCenter1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.yCenter2                               = %x\n",
                pRegCmd[passNumber].cnrCmd.yCenter2);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.rBoundary0                             = %x\n",
                pRegCmd[passNumber].cnrCmd.rBoundary0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.rBoundary1                             = %x\n",
                pRegCmd[passNumber].cnrCmd.rBoundary1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.rBoundary2                             = %x\n",
                pRegCmd[passNumber].cnrCmd.rBoundary2);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.rBoundaryShift0                        = %x\n",
                pRegCmd[passNumber].cnrCmd.rBoundaryShift0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.rSlope0                                = %x\n",
                pRegCmd[passNumber].cnrCmd.rSlope0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.rSlope1                                = %x\n",
                pRegCmd[passNumber].cnrCmd.rSlope1);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.rSlope2                                = %x\n",
                pRegCmd[passNumber].cnrCmd.rSlope2);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.rSlopeShift0                           = %x\n",
                pRegCmd[passNumber].cnrCmd.rSlopeShift0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.combine0                               = %x\n",
                pRegCmd[passNumber].cnrCmd.combine0);
            CamX::OsUtils::FPrintF(pFile, "cnrCmd.fdConfig                               = %x\n",
                pRegCmd[passNumber].cnrCmd.fdConfig);
            CamX::OsUtils::FPrintF(pFile, "\n");
        }


        CamX::OsUtils::FPrintF(pFile, "\n\n");
        CamX::OsUtils::FClose(pFile);
    }
    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "DumpRegConfig ");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10Titan17x::RunCalculationFullPass
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEANR10Titan17x::RunCalculationFullPass(
    ISPInputData* pInputData)
{
    CAMX_UNREFERENCED_PARAM(pInputData);
    IPEANRRegCmd17x* pRegCmd = static_cast<IPEANRRegCmd17x*>(m_pRegCmd);
    CamxResult       result  = CamxResultSuccess;

    /// @todo (CAMX-729) Consume 3A updates and call common IQ library
    pRegCmd[PASS_NAME_FULL].dcBlend1Cmd.bypass.u32All = 0x00000000;

    pRegCmd[PASS_NAME_FULL].rnfCmd.bypass.u32All               = 0x00000003;
    pRegCmd[PASS_NAME_FULL].rnfCmd.subBypass.u32All            = 0x00000024;
    pRegCmd[PASS_NAME_FULL].rnfCmd.debug.u32All                = 0x00000000;
    pRegCmd[PASS_NAME_FULL].rnfCmd.indRankConfigYCFG.u32All    = 0x00005128;
    pRegCmd[PASS_NAME_FULL].rnfCmd.indRankYHV.u32All           = 0x0002f002;
    pRegCmd[PASS_NAME_FULL].rnfCmd.indRankYDG.u32All           = 0x0002f002;
    pRegCmd[PASS_NAME_FULL].rnfCmd.indRankCCFG.u32All          = 0x00002894;
    pRegCmd[PASS_NAME_FULL].rnfCmd.indRankCHV.u32All           = 0x00000382;
    pRegCmd[PASS_NAME_FULL].rnfCmd.indRankCDG.u32All           = 0x00000382;
    pRegCmd[PASS_NAME_FULL].rnfCmd.peakConfigY.u32All          = 0x00000000;
    pRegCmd[PASS_NAME_FULL].rnfCmd.peakConfigc.u32All          = 0x00000000;
    pRegCmd[PASS_NAME_FULL].rnfCmd.peakImgCondS0Y.u32All       = 0x00ff0002;
    pRegCmd[PASS_NAME_FULL].rnfCmd.peakImgCondS1Y.u32All       = 0x000000ff;
    pRegCmd[PASS_NAME_FULL].rnfCmd.peakImgCondS0C.u32All       = 0x00ff000a;
    pRegCmd[PASS_NAME_FULL].rnfCmd.peakImgCondS1C.u32All       = 0x000000ff;
    pRegCmd[PASS_NAME_FULL].rnfCmd.peakDCCondS0Y.u32All        = 0x00d43d42;
    pRegCmd[PASS_NAME_FULL].rnfCmd.peakDCCondS1Y.u32All        = 0x00d44d44;
    pRegCmd[PASS_NAME_FULL].rnfCmd.peakDCCondS0C.u32All        = 0x000021c2;
    pRegCmd[PASS_NAME_FULL].rnfCmd.peakDCCondS1C.u32All        = 0x00002244;
    pRegCmd[PASS_NAME_FULL].rnfCmd.peakLumaIsoThr.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_FULL].rnfCmd.peakChromaIsoThr.u32All     = 0x00000000;
    pRegCmd[PASS_NAME_FULL].rnfCmd.firConfigY.u32All           = 0x00000020;
    pRegCmd[PASS_NAME_FULL].rnfCmd.firConfigC.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_FULL].rnfCmd.firDirThrY.u32All           = 0x00a495d4;
    pRegCmd[PASS_NAME_FULL].rnfCmd.firDirThrC.u32All           = 0x0004ddd2;
    pRegCmd[PASS_NAME_FULL].rnfCmd.firTransY0.u32All           = 0x4920a09a;
    pRegCmd[PASS_NAME_FULL].rnfCmd.firTransY1.u32All           = 0x00024d05;
    pRegCmd[PASS_NAME_FULL].rnfCmd.firTransY2.u32All           = 0x6a625105;
    pRegCmd[PASS_NAME_FULL].rnfCmd.firTransC0.u32All           = 0x1d0c408a;
    pRegCmd[PASS_NAME_FULL].rnfCmd.firTransC1.u32All           = 0x0000ec62;
    pRegCmd[PASS_NAME_FULL].rnfCmd.firTransC2.u32All           = 0x2a20f062;
    pRegCmd[PASS_NAME_FULL].rnfCmd.firTransC3.u32All           = 0x0003401a;

    pRegCmd[PASS_NAME_FULL].dcusCmd.bypass.u32All = 0x00000000;

    pRegCmd[PASS_NAME_FULL].filter2Cmd.byPass.u32All           = 0x00000001;
    pRegCmd[PASS_NAME_FULL].filter2Cmd.debug.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].filter2Cmd.subBypass.u32All        = 0x00000018;
    pRegCmd[PASS_NAME_FULL].filter2Cmd.chromaFiltering.u32All  = 0x00000006;
    pRegCmd[PASS_NAME_FULL].filter2Cmd.chromaIndThr9.u32All    = 0x00000006;
    pRegCmd[PASS_NAME_FULL].filter2Cmd.chromaIndThr11.u32All   = 0x00000006;
    pRegCmd[PASS_NAME_FULL].filter2Cmd.greyIndThr9.u32All      = 0x00000015;
    pRegCmd[PASS_NAME_FULL].filter2Cmd.greyIndThr11.u32All     = 0x0000002a;
    pRegCmd[PASS_NAME_FULL].filter2Cmd.chroma.u32All           = 0x00000006;
    pRegCmd[PASS_NAME_FULL].filter2Cmd.medianThr.u32All        = 0x000726dd;
    pRegCmd[PASS_NAME_FULL].filter2Cmd.medianThrHV.u32All      = 0x00146449;
    pRegCmd[PASS_NAME_FULL].filter2Cmd.medianThrDG12.u32All    = 0x00148449;

    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.lumaBypass.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.chromaBypass.u32All    = 0x00000000;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.yDebug.u32All          = 0x00000000;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.cDebug.u32All          = 0x00000000;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.lumaConfig.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.chromaConfig.u32All    = 0x0000004f;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.yBlend1X1.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.yBlend3X3.u32All       = 0x09090909;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.yBlend5X5.u32All       = 0x1b1b1b1b;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.yBlend7X7.u32All       = 0x36363636;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.yBlend9X9.u32All       = 0x5a5a5a5a;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.yBlend1X1Alt.u32All    = 0x00000000;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.yBlend3X3Alt.u32All    = 0x19191919;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.yBlend5X5Alt.u32All    = 0x40404040;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.yBlend7X7Alt.u32All    = 0x66666666;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.yBlend9X9Alt.u32All    = 0x80808080;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.cBlend1X1.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.cBlend3X3.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.cBlend5X5.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.cBlend7X7.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.cBlend9X9.u32All       = 0x00005555;
    pRegCmd[PASS_NAME_FULL].dcBlend2Cmd.cblend11X11.u32All     = 0x00008080;

    pRegCmd[PASS_NAME_FULL].cylpfPreLensGainCmd.bypass.u32All          = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPreLensGainCmd.subBypass.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPreLensGainCmd.debug.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPreLensGainCmd.config.u32All          = 0x00004004;
    pRegCmd[PASS_NAME_FULL].cylpfPreLensGainCmd.tcfConfig.u32All       = 0x0000000c;
    pRegCmd[PASS_NAME_FULL].cylpfPreLensGainCmd.tcfRadius.u32All       = 0x00001168;
    pRegCmd[PASS_NAME_FULL].cylpfPreLensGainCmd.tcfLowThrLUT0.u32All   = 0x00a01402;
    pRegCmd[PASS_NAME_FULL].cylpfPreLensGainCmd.tcfLowThrLUT1.u32All   = 0x0190500f;
    pRegCmd[PASS_NAME_FULL].cylpfPreLensGainCmd.tcfLowThrLUT2.u32All   = 0x02808c1e;
    pRegCmd[PASS_NAME_FULL].cylpfPreLensGainCmd.tcfHighThrLUT0.u32All  = 0x06e1a466;
    pRegCmd[PASS_NAME_FULL].cylpfPreLensGainCmd.tcfHighThrLUT1.u32All  = 0x07d1e073;
    pRegCmd[PASS_NAME_FULL].cylpfPreLensGainCmd.tcfHighThrLUT2.u32All  = 0x08c21c82;
    pRegCmd[PASS_NAME_FULL].cylpfPreLensGainCmd.tcfLowThrLimit.u32All  = 0x000003ff;
    pRegCmd[PASS_NAME_FULL].cylpfPreLensGainCmd.tcfHighThrLimit.u32All = 0x000003ff;

    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgYFilterLUTThrY0.u32All  = 0x00850083;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgYFilterLUTThrY1.u32All  = 0x008b0087;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgYFilterLUTThrY2.u32All  = 0x0091008e;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgYFilterLUTThrY3.u32All  = 0x00980094;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgYFilterLUTThrY4.u32All  = 0x00a0009c;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgYFilterLUTThrY5.u32All  = 0x00a900a4;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgYFilterLUTThrY6.u32All  = 0x00b400ae;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgYFilterLUTThrY7.u32All  = 0x00be00b9;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgYFilterLUTThrY8.u32All  = 0x000000c1;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgYFilterLUTThrC0.u32All  = 0x00890085;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgYFilterLUTThrC1.u32All  = 0x0095008f;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgYFilterLUTThrC2.u32All  = 0x00a4009d;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgYFilterLUTThrC3.u32All  = 0x00b300ab;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgYFilterLUTThrC4.u32All  = 0x00c300bb;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgYFilterLUTThrC5.u32All  = 0x00d500cc;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgYFilterLUTThrC6.u32All  = 0x00ea00df;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgYFilterLUTThrC7.u32All  = 0x00fb00f4;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgYFilterLUTThrC8.u32All  = 0x00000101;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgCFilterLUTThrY0.u32All  = 0x00890085;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgCFilterLUTThrY1.u32All  = 0x0095008f;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgCFilterLUTThrY2.u32All  = 0x00a4009d;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgCFilterLUTThrY3.u32All  = 0x00b300ab;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgCFilterLUTThrY4.u32All  = 0x00c300bb;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgCFilterLUTThrY5.u32All  = 0x00d500cc;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgCFilterLUTThrY6.u32All  = 0x00ea00df;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgCFilterLUTThrY7.u32All  = 0x00fb00f4;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgCFilterLUTThrY8.u32All  = 0x00000101;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgCFilterLUTThrC0.u32All  = 0x00890085;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgCFilterLUTThrC1.u32All  = 0x0095008f;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgCFilterLUTThrC2.u32All  = 0x00a4009d;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgCFilterLUTThrC3.u32All  = 0x00b300ab;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgCFilterLUTThrC4.u32All  = 0x00c300bb;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgCFilterLUTThrC5.u32All  = 0x00d500cc;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgCFilterLUTThrC6.u32All  = 0x00ea00df;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgCFilterLUTThrC7.u32All  = 0x00fb00f4;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgCFilterLUTThrC8.u32All  = 0x00000101;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgLUTBlend0.u32All        = 0x7e7e7f80;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgLUTBlend1.u32All        = 0x7a7b7c7d;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgLUTBlend2.u32All        = 0x75767879;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgLUTBlend3.u32All        = 0x71737475;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgLUTBlend4.u32All        = 0x6e6f7071;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgLUTBlend5.u32All        = 0x6b6c6d6d;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgLUTBlend6.u32All        = 0x68696a6a;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgLUTBlend7.u32All        = 0x67676868;
    pRegCmd[PASS_NAME_FULL].cylpfLensGainCmd.lgLUTBlend8.u32All        = 0x00000067;

    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterUVLmit.u32All                  = 0x0000007f;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterBaseBlendFarY.u32All           = 0x00000004;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterBaseBlendFarC.u32All           = 0x00000004;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterYYTB0.u32All                   = 0x02c0941e;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterYYTB1.u32All                   = 0x0330cc32;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterYYTB2.u32All                   = 0x0340e038;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterYYTB3.u32All                   = 0x0330cc33;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterYYTB4.u32All                   = 0x0330cc33;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterYYTB5.u32All                   = 0x00009c31;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterYCTB0.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterYCTB1.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterYCTB2.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterYCTB3.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterYTBLimit.u32All                = 0x000003ff;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterUYTB0.u32All                   = 0x08080706;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterUYTB1.u32All                   = 0x08080808;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterUYTB2.u32All                   = 0x08080808;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterUYTB3.u32All                   = 0x07070808;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterUYTB4.u32All                   = 0x00000006;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterUCTB0.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterUCTB1.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterUCTB2.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterUTBLimit.u32All                = 0x000000ff;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterVYTB0.u32All                   = 0x08080706;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterVYTB1.u32All                   = 0x08080808;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterVYTB2.u32All                   = 0x08080808;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterVYTB3.u32All                   = 0x07080808;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterVYTB4.u32All                   = 0x00000007;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterVCTB0.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterVCTB1.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterVCTB2.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterVTBLimit.u32All                = 0x000000ff;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterUVLimit.u32All                 = 0x0000007f;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterBaseBlendFarY.u32All           = 0x00000004;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterBaseBlendFarC.u32All           = 0x00000004;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterYYTB0.u32All                   = 0x01f0781d;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterYYTB1.u32All                   = 0x01605c1f;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterYYTB2.u32All                   = 0x0200b02a;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterYYTB3.u32All                   = 0x01f0741c;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterYYTB4.u32All                   = 0x02308c22;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterYYTB5.u32All                   = 0x00008023;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterYCTB0.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterYCTB1.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterYCTB2.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterYCTB3.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterYTBLimit.u32All                = 0x000003ff;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterUYTB0.u32All                   = 0x05050506;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterUYTB1.u32All                   = 0x05050303;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterUYTB2.u32All                   = 0x05050505;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterUYTB3.u32All                   = 0x05050505;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterUYTB4.u32All                   = 0x00000004;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterUCTB0.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterUCTB1.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterUCTB2.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterUTBLimit.u32All                = 0x000000ff;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterVYTB0.u32All                   = 0x05050506;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterVYTB1.u32All                   = 0x05050303;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterVYTB2.u32All                   = 0x05050505;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterVYTB3.u32All                   = 0x05050505;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterVYTB4.u32All                   = 0x00000004;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterVCTB0.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterVCTB1.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterVCTB2.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterVTBLimit.u32All                = 0x000000ff;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterConfig.u32All                  = 0x00014014;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterKernel.u32All                  = 0x000ad4da;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterAltKernel.u32All               = 0x780000db;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterDCIndFlags.u32All              = 0x00934f3e;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterDerFlags.u32All                = 0x00934f3e;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterDer2Flags.u32All               = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterDirConfig.u32All               = 0xffff0000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterExternalDirConfig.u32All       = 0x0000ff00;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterYThrehClose3Mod.u32All         = 0x00000010;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterYThreshDer2Close3Mod.u32All    = 0x00000010;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterYThrFarMod.u32All              = 0x0000000c;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterUThrFarMod.u32All              = 0x00000010;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterVThrFarMod.u32All              = 0x00000010;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterYThrCloseMod.u32All            = 0x00000010;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterYThrFarExMod.u32All            = 0x00000010;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterUThrFarExMod.u32All            = 0x00000010;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yFilterVThrFarExMod.u32All            = 0x00000010;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yDCIndYGain30.u32All                  = 0x2aaaaaaa;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yDCIndYGain50.u32All                  = 0x2aaaaaaa;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yDCIndYGain70.u32All                  = 0x2aaaaaaa;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yDCIndYGain90.u32All                  = 0x2aaaaaaa;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yDCIndYGain5X0.u32All                 = 0x2aaaaaaa;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yDCIndYGain9X0.u32All                 = 0x2aaaaaaa;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yDCIndUVGain30.u32All                 = 0x2aaaaaaa;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yDCIndUVGain50.u32All                 = 0x2aaaaaaa;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.yDCIndUVGain5X0.u32All                = 0x2aaaaaaa;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterConfig.u32All                  = 0x00048383;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterDetailCond.u32All              = 0x00001e3c;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cThreshold.u32All                     = 0x00000033;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterGreyYDer.u32All                = 0x000f003c;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterGreyChromaticity.u32All        = 0x00521f60;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterGreyLimits.u32All              = 0x00000040;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterGreyBlendScale.u32All          = 0x00000010;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterGreyBlendOffset.u32All         = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterGreySizeThr35.u32All           = 0x001b1e1e;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterGreySizeThr79.u32All           = 0x00000048;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterGreyThrClose.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterGreyThrFar.u32All              = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterDCIndFlags.u32All              = 0x00004f3e;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterDerFlags.u32All                = 0x00004f3e;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterDirConfig.u32All               = 0xffff0000;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterDirConfigEx.u32All             = 0x0000ff00;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterYThrFarMod.u32All              = 0x000000c9;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterUThrFarMod.u32All              = 0x00000010;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterVThrFarMod.u32All              = 0x00000010;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterUThrDistMod.u32All             = 0x00000010;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterVThrDisMod.u32All              = 0x00000010;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterYThrCloseMod.u32All            = 0x00000010;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterYThrFarExMod.u32All            = 0x00000008;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterUThrFarExMod.u32All            = 0x00000010;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cFilterVThrFarExMod.u32All            = 0x00000010;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cDCIndYGain30.u32All                  = 0x002ed12a;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cDCIndYGain50.u32All                  = 0x002ed12a;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cDCIndYGain70.u32All                  = 0x00084aaa;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cDCIndYGain90.u32All                  = 0x00084aaa;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cDCIndYGain5X0.u32All                 = 0x002ed12a;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cDCIndYGain9X0.u32All                 = 0x00084aaa;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cDCIndUVGain30.u32All                 = 0x002ed12a;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cDCIndUVGain50.u32All                 = 0x002ed12a;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cDCIndUVGain70.u32All                 = 0x00084aaa;
    pRegCmd[PASS_NAME_FULL].cylpfPostLensGainCmd.cDCIndUVGain5X0.u32All                = 0x002ed12a;

    pRegCmd[PASS_NAME_FULL].cnrCmd.bypass.u32All               = 0x00000001;
    pRegCmd[PASS_NAME_FULL].cnrCmd.config.u32All               = 0x00000027;
    pRegCmd[PASS_NAME_FULL].cnrCmd.debug.u32All                = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.filterMode.u32All           = 0x0000000f;
    pRegCmd[PASS_NAME_FULL].cnrCmd.specialParams.u32All        = 0x00700001;
    pRegCmd[PASS_NAME_FULL].cnrCmd.cMode.u32All                = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.angleMin0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.angleMin1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.angleMin2.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.angleMax0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.angleMax1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.angleMax2.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.angleInternal.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.colorSatMin0.u32All         = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.colorSatMin1.u32All         = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.colorSatMin2.u32All         = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.colorSatMax0.u32All         = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.colorSatMax1.u32All         = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.colorSatMax2.u32All         = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.yCompMin0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.yCompMin1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.yCompMax0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.yCompMax1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.boundaryProbability.u32All  = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.q0.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.q1.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.gainFactorYY0.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.gainFactorYY1.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.gainFactorYY2.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.gainFactorUVY0.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.gainFactorUVY1.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.gainFactorUVY2.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.offsetYY0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.offsetYY1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.offsetYY2.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.offsetUY0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.offsetUY1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.offsetVY0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.offsetVY1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.gainFactorYC0.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.gainFactorYC1.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.gainFactorYC2.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.gainFactorUVC0.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.gainFactorUVC1.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.gainFactorUVC2.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.offsetYC0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.offsetYC1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.offsetYC2.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.offsetUC0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.offsetUC1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.offsetVC0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.offsetVC1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.blendFactorYDC0.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.blendFactorYDC1.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.blendFactorYALT0.u32All     = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.blendFactorYALT1.u32All     = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.blendFactorC0.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.blendFactorC1.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.fdOffset.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.xCenter0.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.xCenter1.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.xCenter2.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.yCenter0.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.yCenter1.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.yCenter2.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.rBoundary0.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.rBoundary1.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.rBoundary2.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.rBoundaryShift0.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.rSlope0.u32All              = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.rSlope1.u32All              = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.rSlope2.u32All              = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.rSlopeShift0.u32All         = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.combine0.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_FULL].cnrCmd.fdConfig.u32All             = 0x0000000a;

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10Titan17x::RunCalculationDS4Pass
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEANR10Titan17x::RunCalculationDS4Pass(
    ISPInputData* pInputData)
{
    CAMX_UNREFERENCED_PARAM(pInputData);
    IPEANRRegCmd17x* pRegCmd = static_cast<IPEANRRegCmd17x*>(m_pRegCmd);
    CamxResult       result  = CamxResultSuccess;

    /// @todo (CAMX-729) Consume 3A updates and call common IQ library

    pRegCmd[PASS_NAME_DC_4].dcBlend1Cmd.bypass.u32All = 0x00000000;

    pRegCmd[PASS_NAME_DC_4].rnfCmd.bypass.u32All               = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.subBypass.u32All            = 0x00000009;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.debug.u32All                = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.indRankConfigYCFG.u32All    = 0x0000412b;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.indRankYHV.u32All           = 0x0002f866;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.indRankYDG.u32All           = 0x0002fa66;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.indRankCCFG.u32All          = 0x00001093;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.indRankCHV.u32All           = 0x000003c3;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.indRankCDG.u32All           = 0x000003c3;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.peakConfigY.u32All          = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.peakConfigc.u32All          = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.peakImgCondS0Y.u32All       = 0x00ff00ff;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.peakImgCondS1Y.u32All       = 0x0020ff00;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.peakImgCondS0C.u32All       = 0x00ff00ff;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.peakImgCondS1C.u32All       = 0x0020ff00;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.peakDCCondS0Y.u32All        = 0x00bc2bc2;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.peakDCCondS1Y.u32All        = 0x00c44c43;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.peakDCCondS0C.u32All        = 0x00002142;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.peakDCCondS1C.u32All        = 0x00002243;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.peakLumaIsoThr.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.peakChromaIsoThr.u32All     = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.firConfigY.u32All           = 0x00000020;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.firConfigC.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.firDirThrY.u32All           = 0x00a495d4;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.firDirThrC.u32All           = 0x0004c8c6;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.firTransY0.u32All           = 0x4920a09a;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.firTransY1.u32All           = 0x00024d05;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.firTransY2.u32All           = 0x6a625105;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.firTransC0.u32All           = 0x1d0c408a;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.firTransC1.u32All           = 0x0000ec62;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.firTransC2.u32All           = 0x2a20f062;
    pRegCmd[PASS_NAME_DC_4].rnfCmd.firTransC3.u32All           = 0x0003401a;

    pRegCmd[PASS_NAME_DC_4].dcusCmd.bypass.u32All = 0x00000000;

    pRegCmd[PASS_NAME_DC_4].filter2Cmd.byPass.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].filter2Cmd.debug.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].filter2Cmd.subBypass.u32All        = 0x0000001c;
    pRegCmd[PASS_NAME_DC_4].filter2Cmd.chromaFiltering.u32All  = 0x00000006;
    pRegCmd[PASS_NAME_DC_4].filter2Cmd.chromaIndThr9.u32All    = 0x00000006;
    pRegCmd[PASS_NAME_DC_4].filter2Cmd.chromaIndThr11.u32All   = 0x00000006;
    pRegCmd[PASS_NAME_DC_4].filter2Cmd.greyIndThr9.u32All      = 0x00000014;
    pRegCmd[PASS_NAME_DC_4].filter2Cmd.greyIndThr11.u32All     = 0x00000014;
    pRegCmd[PASS_NAME_DC_4].filter2Cmd.chroma.u32All           = 0x00000006;
    pRegCmd[PASS_NAME_DC_4].filter2Cmd.medianThr.u32All        = 0x0002924c;
    pRegCmd[PASS_NAME_DC_4].filter2Cmd.medianThrHV.u32All      = 0x000a3225;
    pRegCmd[PASS_NAME_DC_4].filter2Cmd.medianThrDG12.u32All    = 0x000a4225;

    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.lumaBypass.u32All      = 0x00000000;
    // Temporarily disable
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.chromaBypass.u32All    = 0x00000001;
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.yDebug.u32All          = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.cDebug.u32All          = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.lumaConfig.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.chromaConfig.u32All    = 0x00000048;
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.yBlend1X1.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.yBlend3X3.u32All       = 0x0d0d0d0d;
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.yBlend5X5.u32All       = 0x26262626;
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.yBlend7X7.u32All       = 0x4d4d4d4d;
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.yBlend9X9.u32All       = 0x80808080;
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.yBlend1X1Alt.u32All    = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.yBlend3X3Alt.u32All    = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.yBlend5X5Alt.u32All    = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.yBlend7X7Alt.u32All    = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.yBlend9X9Alt.u32All    = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.cBlend1X1.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.cBlend3X3.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.cBlend5X5.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.cBlend7X7.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.cBlend9X9.u32All       = 0x00005555;
    pRegCmd[PASS_NAME_DC_4].dcBlend2Cmd.cblend11X11.u32All     = 0x00008080;

    pRegCmd[PASS_NAME_DC_4].cylpfPreLensGainCmd.bypass.u32All          = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPreLensGainCmd.subBypass.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPreLensGainCmd.debug.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPreLensGainCmd.config.u32All          = 0x00004004;
    pRegCmd[PASS_NAME_DC_4].cylpfPreLensGainCmd.tcfConfig.u32All       = 0x0000000c;
    pRegCmd[PASS_NAME_DC_4].cylpfPreLensGainCmd.tcfRadius.u32All       = 0x00001168;
    pRegCmd[PASS_NAME_DC_4].cylpfPreLensGainCmd.tcfLowThrLUT0.u32All   = 0x00a01402;
    pRegCmd[PASS_NAME_DC_4].cylpfPreLensGainCmd.tcfLowThrLUT1.u32All   = 0x0190500f;
    pRegCmd[PASS_NAME_DC_4].cylpfPreLensGainCmd.tcfLowThrLUT2.u32All   = 0x02808c1e;
    pRegCmd[PASS_NAME_DC_4].cylpfPreLensGainCmd.tcfHighThrLUT0.u32All  = 0x06e1a466;
    pRegCmd[PASS_NAME_DC_4].cylpfPreLensGainCmd.tcfHighThrLUT1.u32All  = 0x07d1e073;
    pRegCmd[PASS_NAME_DC_4].cylpfPreLensGainCmd.tcfHighThrLUT2.u32All  = 0x08c21c82;
    pRegCmd[PASS_NAME_DC_4].cylpfPreLensGainCmd.tcfLowThrLimit.u32All  = 0x000003ff;
    pRegCmd[PASS_NAME_DC_4].cylpfPreLensGainCmd.tcfHighThrLimit.u32All = 0x000003ff;

    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgYFilterLUTThrY0.u32All  = 0x00890085;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgYFilterLUTThrY1.u32All  = 0x0094008e;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgYFilterLUTThrY2.u32All  = 0x009e0099;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgYFilterLUTThrY3.u32All  = 0x00a900a4;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgYFilterLUTThrY4.u32All  = 0x00b600af;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgYFilterLUTThrY5.u32All  = 0x00c500bd;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgYFilterLUTThrY6.u32All  = 0x00d900ce;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgYFilterLUTThrY7.u32All  = 0x00e900e2;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgYFilterLUTThrY8.u32All  = 0x000000f0;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgYFilterLUTThrC0.u32All  = 0x00890085;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgYFilterLUTThrC1.u32All  = 0x0095008f;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgYFilterLUTThrC2.u32All  = 0x00a4009d;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgYFilterLUTThrC3.u32All  = 0x00b300ab;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgYFilterLUTThrC4.u32All  = 0x00c300bb;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgYFilterLUTThrC5.u32All  = 0x00d500cc;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgYFilterLUTThrC6.u32All  = 0x00ea00df;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgYFilterLUTThrC7.u32All  = 0x00fb00f4;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgYFilterLUTThrC8.u32All  = 0x00000101;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgCFilterLUTThrY0.u32All  = 0x00890085;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgCFilterLUTThrY1.u32All  = 0x0095008f;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgCFilterLUTThrY2.u32All  = 0x00a4009d;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgCFilterLUTThrY3.u32All  = 0x00b300ab;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgCFilterLUTThrY4.u32All  = 0x00c300bb;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgCFilterLUTThrY5.u32All  = 0x00d500cc;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgCFilterLUTThrY6.u32All  = 0x00ea00df;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgCFilterLUTThrY7.u32All  = 0x00fb00f4;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgCFilterLUTThrY8.u32All  = 0x00000101;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgCFilterLUTThrC0.u32All  = 0x00890085;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgCFilterLUTThrC1.u32All  = 0x0095008f;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgCFilterLUTThrC2.u32All  = 0x00a4009d;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgCFilterLUTThrC3.u32All  = 0x00b300ab;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgCFilterLUTThrC4.u32All  = 0x00c300bb;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgCFilterLUTThrC5.u32All  = 0x00d500cc;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgCFilterLUTThrC6.u32All  = 0x00ea00df;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgCFilterLUTThrC7.u32All  = 0x00fb00f4;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgCFilterLUTThrC8.u32All  = 0x00000101;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgLUTBlend0.u32All        = 0x7e7e7f80;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgLUTBlend1.u32All        = 0x7a7b7c7d;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgLUTBlend2.u32All        = 0x75767879;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgLUTBlend3.u32All        = 0x71737475;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgLUTBlend4.u32All        = 0x6e6f7071;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgLUTBlend5.u32All        = 0x6b6c6d6d;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgLUTBlend6.u32All        = 0x68696a6a;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgLUTBlend7.u32All        = 0x67676868;
    pRegCmd[PASS_NAME_DC_4].cylpfLensGainCmd.lgLUTBlend8.u32All        = 0x00000067;

    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterUVLmit.u32All                  = 0x0000007f;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterBaseBlendFarY.u32All           = 0x00000004;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterBaseBlendFarC.u32All           = 0x00000004;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterYYTB0.u32All                   = 0x00e03006;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterYYTB1.u32All                   = 0x00e0380e;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterYYTB2.u32All                   = 0x00e04010;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterYYTB3.u32All                   = 0x00e0380e;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterYYTB4.u32All                   = 0x0100400e;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterYYTB5.u32All                   = 0x0000280e;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterYCTB0.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterYCTB1.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterYCTB2.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterYCTB3.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterYTBLimit.u32All                = 0x000003ff;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterUYTB0.u32All                   = 0x05050403;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterUYTB1.u32All                   = 0x05050505;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterUYTB2.u32All                   = 0x05050505;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterUYTB3.u32All                   = 0x04050505;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterUYTB4.u32All                   = 0x00000003;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterUCTB0.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterUCTB1.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterUCTB2.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterUTBLimit.u32All                = 0x000000ff;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterVYTB0.u32All                   = 0x05050403;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterVYTB1.u32All                   = 0x05050505;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterVYTB2.u32All                   = 0x05050505;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterVYTB3.u32All                   = 0x04050505;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterVYTB4.u32All                   = 0x00000003;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterVCTB0.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterVCTB1.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterVCTB2.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterVTBLimit.u32All                = 0x000000ff;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterUVLimit.u32All                 = 0x0000007f;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterBaseBlendFarY.u32All           = 0x00000004;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterBaseBlendFarC.u32All           = 0x00000004;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterYYTB0.u32All                   = 0x00c02407;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterYYTB1.u32All                   = 0x00c0240a;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterYYTB2.u32All                   = 0x00a0300e;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterYYTB3.u32All                   = 0x00a02409;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterYYTB4.u32All                   = 0x0100380e;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterYYTB5.u32All                   = 0x00002c0d;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterYCTB0.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterYCTB1.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterYCTB2.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterYCTB3.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterYTBLimit.u32All                = 0x000003ff;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterUYTB0.u32All                   = 0x04040302;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterUYTB1.u32All                   = 0x04040404;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterUYTB2.u32All                   = 0x04040404;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterUYTB3.u32All                   = 0x03040404;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterUYTB4.u32All                   = 0x00000002;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterUCTB0.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterUCTB1.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterUCTB2.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterUTBLimit.u32All                = 0x000000ff;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterVYTB0.u32All                   = 0x04040403;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterVYTB1.u32All                   = 0x04040404;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterVYTB2.u32All                   = 0x04040404;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterVYTB3.u32All                   = 0x04040404;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterVYTB4.u32All                   = 0x00000002;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterVCTB0.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterVCTB1.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterVCTB2.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterVTBLimit.u32All                = 0x000000ff;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterConfig.u32All                  = 0x0000800c;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterKernel.u32All                  = 0x02355490;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterAltKernel.u32All               = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterDCIndFlags.u32All              = 0x00934f3e;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterDerFlags.u32All                = 0x00934f3e;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterDer2Flags.u32All               = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterDirConfig.u32All               = 0xffff0000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterExternalDirConfig.u32All       = 0x0000ff00;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterYThrehClose3Mod.u32All         = 0x00000010;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterYThreshDer2Close3Mod.u32All    = 0x00000010;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterYThrFarMod.u32All              = 0x0001ffd0;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterUThrFarMod.u32All              = 0x00000010;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterVThrFarMod.u32All              = 0x00000010;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterYThrCloseMod.u32All            = 0x00000010;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterYThrFarExMod.u32All            = 0x00000010;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterUThrFarExMod.u32All            = 0x00000010;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yFilterVThrFarExMod.u32All            = 0x00000010;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yDCIndYGain30.u32All                  = 0x0c34316a;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yDCIndYGain50.u32All                  = 0x0c34316a;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yDCIndYGain70.u32All                  = 0x030c5aaa;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yDCIndYGain90.u32All                  = 0x030c5aaa;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yDCIndYGain5X0.u32All                 = 0x0c34316a;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yDCIndYGain9X0.u32All                 = 0x030c5aaa;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yDCIndUVGain30.u32All                 = 0x0c34316a;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yDCIndUVGain50.u32All                 = 0x0c34316a;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.yDCIndUVGain5X0.u32All                = 0x0c34316a;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterConfig.u32All                  = 0x00028383;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterDetailCond.u32All              = 0x00005a0a;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cThreshold.u32All                     = 0x00000029;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterGreyYDer.u32All                = 0x002d000a;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterGreyChromaticity.u32All        = 0x00201f60;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterGreyLimits.u32All              = 0x00000040;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterGreyBlendScale.u32All          = 0x00000010;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterGreyBlendOffset.u32All         = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterGreySizeThr35.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterGreySizeThr79.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterGreyThrClose.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterGreyThrFar.u32All              = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterDCIndFlags.u32All              = 0x00004f3e;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterDerFlags.u32All                = 0x00004f3e;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterDirConfig.u32All               = 0xffff0000;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterDirConfigEx.u32All             = 0x0000ff00;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterYThrFarMod.u32All              = 0x0000004c;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterUThrFarMod.u32All              = 0x00000010;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterVThrFarMod.u32All              = 0x00000010;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterUThrDistMod.u32All             = 0x00000010;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterVThrDisMod.u32All              = 0x00000010;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterYThrCloseMod.u32All            = 0x00000010;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterYThrFarExMod.u32All            = 0x00000008;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterUThrFarExMod.u32All            = 0x00000010;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cFilterVThrFarExMod.u32All            = 0x00000010;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cDCIndYGain30.u32All                  = 0x002ed12a;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cDCIndYGain50.u32All                  = 0x002ed12a;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cDCIndYGain70.u32All                  = 0x00084aaa;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cDCIndYGain90.u32All                  = 0x00084aaa;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cDCIndYGain5X0.u32All                 = 0x002ed12a;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cDCIndYGain9X0.u32All                 = 0x00084aaa;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cDCIndUVGain30.u32All                 = 0x002ed12a;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cDCIndUVGain50.u32All                 = 0x002ed12a;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cDCIndUVGain70.u32All                 = 0x00084aaa;
    pRegCmd[PASS_NAME_DC_4].cylpfPostLensGainCmd.cDCIndUVGain5X0.u32All                = 0x002ed12a;

    pRegCmd[PASS_NAME_DC_4].cnrCmd.bypass.u32All               = 0x00000001;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.config.u32All               = 0x00000023;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.debug.u32All                = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.filterMode.u32All           = 0x0000000f;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.specialParams.u32All        = 0x00700001;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.cMode.u32All                = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.angleMin0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.angleMin1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.angleMin2.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.angleMax0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.angleMax1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.angleMax2.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.angleInternal.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.colorSatMin0.u32All         = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.colorSatMin1.u32All         = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.colorSatMin2.u32All         = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.colorSatMax0.u32All         = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.colorSatMax1.u32All         = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.colorSatMax2.u32All         = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.yCompMin0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.yCompMin1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.yCompMax0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.yCompMax1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.boundaryProbability.u32All  = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.q0.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.q1.u32All                   = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.gainFactorYY0.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.gainFactorYY1.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.gainFactorYY2.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.gainFactorUVY0.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.gainFactorUVY1.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.gainFactorUVY2.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.offsetYY0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.offsetYY1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.offsetYY2.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.offsetUY0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.offsetUY1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.offsetVY0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.offsetVY1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.gainFactorYC0.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.gainFactorYC1.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.gainFactorYC2.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.gainFactorUVC0.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.gainFactorUVC1.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.gainFactorUVC2.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.offsetYC0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.offsetYC1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.offsetYC2.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.offsetUC0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.offsetUC1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.offsetVC0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.offsetVC1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.blendFactorYDC0.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.blendFactorYDC1.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.blendFactorYALT0.u32All     = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.blendFactorYALT1.u32All     = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.blendFactorC0.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.blendFactorC1.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.fdOffset.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.xCenter0.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.xCenter1.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.xCenter2.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.yCenter0.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.yCenter1.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.yCenter2.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.rBoundary0.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.rBoundary1.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.rBoundary2.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.rBoundaryShift0.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.rSlope0.u32All              = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.rSlope1.u32All              = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.rSlope2.u32All              = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.rSlopeShift0.u32All         = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.combine0.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_DC_4].cnrCmd.fdConfig.u32All             = 0x0000000a;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10Titan17x::RunCalculationDS16Pass
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEANR10Titan17x::RunCalculationDS16Pass(
    ISPInputData* pInputData)
{
    CAMX_UNREFERENCED_PARAM(pInputData);
    IPEANRRegCmd17x* pRegCmd = static_cast<IPEANRRegCmd17x*>(m_pRegCmd);
    CamxResult       result  = CamxResultSuccess;

    /// @todo (CAMX-729) Consume 3A updates and call common IQ library

    pRegCmd[PASS_NAME_DC_16].dcBlend1Cmd.bypass.u32All = 0x00000001;

    pRegCmd[PASS_NAME_DC_16].rnfCmd.bypass.u32All              = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.subBypass.u32All           = 0x0000001b;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.debug.u32All               = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.indRankConfigYCFG.u32All   = 0x00002126;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.indRankYHV.u32All          = 0x0002f854;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.indRankYDG.u32All          = 0x0002fa54;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.indRankCCFG.u32All         = 0x00001093;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.indRankCHV.u32All          = 0x000003c3;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.indRankCDG.u32All          = 0x000003c3;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.peakConfigY.u32All         = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.peakConfigc.u32All         = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.peakImgCondS0Y.u32All      = 0x00ff00ff;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.peakImgCondS1Y.u32All      = 0x0020ff00;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.peakImgCondS0C.u32All      = 0x00ff00ff;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.peakImgCondS1C.u32All      = 0x0020ff00;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.peakDCCondS0Y.u32All       = 0x00bc2bc2;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.peakDCCondS1Y.u32All       = 0x00c44c43;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.peakDCCondS0C.u32All       = 0x00002142;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.peakDCCondS1C.u32All       = 0x00002243;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.peakLumaIsoThr.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.peakChromaIsoThr.u32All    = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.firConfigY.u32All          = 0x00000020;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.firConfigC.u32All          = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.firDirThrY.u32All          = 0x00a644db;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.firDirThrC.u32All          = 0x0004c8db;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.firTransY0.u32All          = 0x4920a09a;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.firTransY1.u32All          = 0x00024d05;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.firTransY2.u32All          = 0x6a625105;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.firTransC0.u32All          = 0x4920a09a;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.firTransC1.u32All          = 0x00024d05;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.firTransC2.u32All          = 0x6a625105;
    pRegCmd[PASS_NAME_DC_16].rnfCmd.firTransC3.u32All          = 0x0003401a;

    pRegCmd[PASS_NAME_DC_16].dcusCmd.bypass.u32All = 0x00000001;

    pRegCmd[PASS_NAME_DC_16].filter2Cmd.byPass.u32All          = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].filter2Cmd.debug.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].filter2Cmd.subBypass.u32All       = 0x0000001c;
    pRegCmd[PASS_NAME_DC_16].filter2Cmd.chromaFiltering.u32All = 0x00000006;
    pRegCmd[PASS_NAME_DC_16].filter2Cmd.chromaIndThr9.u32All   = 0x00000006;
    pRegCmd[PASS_NAME_DC_16].filter2Cmd.chromaIndThr11.u32All  = 0x00000006;
    pRegCmd[PASS_NAME_DC_16].filter2Cmd.greyIndThr9.u32All     = 0x00000014;
    pRegCmd[PASS_NAME_DC_16].filter2Cmd.greyIndThr11.u32All    = 0x00000028;
    pRegCmd[PASS_NAME_DC_16].filter2Cmd.chroma.u32All          = 0x00000006;
    pRegCmd[PASS_NAME_DC_16].filter2Cmd.medianThr.u32All       = 0x00024200;
    pRegCmd[PASS_NAME_DC_16].filter2Cmd.medianThrHV.u32All     = 0x00021125;
    pRegCmd[PASS_NAME_DC_16].filter2Cmd.medianThrDG12.u32All   = 0x00021125;

    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.lumaBypass.u32All     = 0x00000001;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.chromaBypass.u32All   = 0x00000001;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.yDebug.u32All         = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.cDebug.u32All         = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.lumaConfig.u32All     = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.chromaConfig.u32All   = 0x00000048;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.yBlend1X1.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.yBlend3X3.u32All      = 0x0d0d0d0d;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.yBlend5X5.u32All      = 0x26262626;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.yBlend7X7.u32All      = 0x4d4d4d4d;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.yBlend9X9.u32All      = 0x80808080;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.yBlend1X1Alt.u32All   = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.yBlend3X3Alt.u32All   = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.yBlend5X5Alt.u32All   = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.yBlend7X7Alt.u32All   = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.yBlend9X9Alt.u32All   = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.cBlend1X1.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.cBlend3X3.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.cBlend5X5.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.cBlend7X7.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.cBlend9X9.u32All      = 0x00005555;
    pRegCmd[PASS_NAME_DC_16].dcBlend2Cmd.cblend11X11.u32All    = 0x00008080;

    pRegCmd[PASS_NAME_DC_16].cylpfPreLensGainCmd.bypass.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPreLensGainCmd.subBypass.u32All          = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPreLensGainCmd.debug.u32All              = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPreLensGainCmd.config.u32All             = 0x00004004;
    pRegCmd[PASS_NAME_DC_16].cylpfPreLensGainCmd.tcfConfig.u32All          = 0x0000000c;
    pRegCmd[PASS_NAME_DC_16].cylpfPreLensGainCmd.tcfRadius.u32All          = 0x00001168;
    pRegCmd[PASS_NAME_DC_16].cylpfPreLensGainCmd.tcfLowThrLUT0.u32All      = 0x00a01402;
    pRegCmd[PASS_NAME_DC_16].cylpfPreLensGainCmd.tcfLowThrLUT1.u32All      = 0x0190500f;
    pRegCmd[PASS_NAME_DC_16].cylpfPreLensGainCmd.tcfLowThrLUT2.u32All      = 0x02808c1e;
    pRegCmd[PASS_NAME_DC_16].cylpfPreLensGainCmd.tcfHighThrLUT0.u32All     = 0x06e1a466;
    pRegCmd[PASS_NAME_DC_16].cylpfPreLensGainCmd.tcfHighThrLUT1.u32All     = 0x07d1e073;
    pRegCmd[PASS_NAME_DC_16].cylpfPreLensGainCmd.tcfHighThrLUT2.u32All     = 0x08c21c82;
    pRegCmd[PASS_NAME_DC_16].cylpfPreLensGainCmd.tcfLowThrLimit.u32All     = 0x000003ff;
    pRegCmd[PASS_NAME_DC_16].cylpfPreLensGainCmd.tcfHighThrLimit.u32All    = 0x000003ff;

    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgYFilterLUTThrY0.u32All = 0x00890085;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgYFilterLUTThrY1.u32All = 0x0094008e;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgYFilterLUTThrY2.u32All = 0x009e0099;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgYFilterLUTThrY3.u32All = 0x00a900a4;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgYFilterLUTThrY4.u32All = 0x00b600af;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgYFilterLUTThrY5.u32All = 0x00c500bd;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgYFilterLUTThrY6.u32All = 0x00d900ce;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgYFilterLUTThrY7.u32All = 0x00e900e2;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgYFilterLUTThrY8.u32All = 0x000000f0;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgYFilterLUTThrC0.u32All = 0x00890085;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgYFilterLUTThrC1.u32All = 0x0095008f;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgYFilterLUTThrC2.u32All = 0x00a4009d;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgYFilterLUTThrC3.u32All = 0x00b300ab;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgYFilterLUTThrC4.u32All = 0x00c300bb;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgYFilterLUTThrC5.u32All = 0x00d500cc;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgYFilterLUTThrC6.u32All = 0x00ea00df;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgYFilterLUTThrC7.u32All = 0x00fb00f4;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgYFilterLUTThrC8.u32All = 0x00000101;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgCFilterLUTThrY0.u32All = 0x00890085;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgCFilterLUTThrY1.u32All = 0x0095008f;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgCFilterLUTThrY2.u32All = 0x00a4009d;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgCFilterLUTThrY3.u32All = 0x00b300ab;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgCFilterLUTThrY4.u32All = 0x00c300bb;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgCFilterLUTThrY5.u32All = 0x00d500cc;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgCFilterLUTThrY6.u32All = 0x00ea00df;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgCFilterLUTThrY7.u32All = 0x00fb00f4;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgCFilterLUTThrY8.u32All = 0x00000101;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgCFilterLUTThrC0.u32All = 0x00890085;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgCFilterLUTThrC1.u32All = 0x0095008f;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgCFilterLUTThrC2.u32All = 0x00a4009d;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgCFilterLUTThrC3.u32All = 0x00b300ab;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgCFilterLUTThrC4.u32All = 0x00c300bb;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgCFilterLUTThrC5.u32All = 0x00d500cc;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgCFilterLUTThrC6.u32All = 0x00ea00df;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgCFilterLUTThrC7.u32All = 0x00fb00f4;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgCFilterLUTThrC8.u32All = 0x00000101;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgLUTBlend0.u32All       = 0x7e7e7f80;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgLUTBlend1.u32All       = 0x7a7b7c7d;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgLUTBlend2.u32All       = 0x75767879;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgLUTBlend3.u32All       = 0x71737475;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgLUTBlend4.u32All       = 0x6e6f7071;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgLUTBlend5.u32All       = 0x6b6c6d6d;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgLUTBlend6.u32All       = 0x68696a6a;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgLUTBlend7.u32All       = 0x67676868;
    pRegCmd[PASS_NAME_DC_16].cylpfLensGainCmd.lgLUTBlend8.u32All       = 0x00000067;

    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterUVLmit.u32All                 = 0x0000007f;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterBaseBlendFarY.u32All          = 0x00000004;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterBaseBlendFarC.u32All          = 0x00000004;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterYYTB0.u32All                  = 0x00200802;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterYYTB1.u32All                  = 0x00200802;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterYYTB2.u32All                  = 0x00200802;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterYYTB3.u32All                  = 0x00200802;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterYYTB4.u32All                  = 0x00200802;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterYYTB5.u32All                  = 0x00000802;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterYCTB0.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterYCTB1.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterYCTB2.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterYCTB3.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterYTBLimit.u32All               = 0x000003ff;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterUYTB0.u32All                  = 0x01010101;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterUYTB1.u32All                  = 0x01010101;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterUYTB2.u32All                  = 0x01010101;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterUYTB3.u32All                  = 0x01010101;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterUYTB4.u32All                  = 0x00000001;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterUCTB0.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterUCTB1.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterUCTB2.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterUTBLimit.u32All               = 0x000000ff;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterVYTB0.u32All                  = 0x01010101;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterVYTB1.u32All                  = 0x01010101;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterVYTB2.u32All                  = 0x01010101;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterVYTB3.u32All                  = 0x01010101;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterVYTB4.u32All                  = 0x00000001;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterVCTB0.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterVCTB1.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterVCTB2.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterVTBLimit.u32All               = 0x000000ff;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterUVLimit.u32All                = 0x0000007f;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterBaseBlendFarY.u32All          = 0x00000004;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterBaseBlendFarC.u32All          = 0x00000004;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterYYTB0.u32All                  = 0x00100401;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterYYTB1.u32All                  = 0x00100401;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterYYTB2.u32All                  = 0x00100401;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterYYTB3.u32All                  = 0x00100401;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterYYTB4.u32All                  = 0x00100401;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterYYTB5.u32All                  = 0x00000401;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterYCTB0.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterYCTB1.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterYCTB2.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterYCTB3.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterYTBLimit.u32All               = 0x000003ff;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterUYTB0.u32All                  = 0x01010101;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterUYTB1.u32All                  = 0x01010101;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterUYTB2.u32All                  = 0x01010101;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterUYTB3.u32All                  = 0x01010101;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterUYTB4.u32All                  = 0x00000001;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterUCTB0.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterUCTB1.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterUCTB2.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterUTBLimit.u32All               = 0x000000ff;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterVYTB0.u32All                  = 0x01010101;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterVYTB1.u32All                  = 0x01010101;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterVYTB2.u32All                  = 0x01010101;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterVYTB3.u32All                  = 0x01010101;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterVYTB4.u32All                  = 0x00000001;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterVCTB0.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterVCTB1.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterVCTB2.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterVTBLimit.u32All               = 0x000000ff;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterConfig.u32All                 = 0x0000800c;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterKernel.u32All                 = 0x02355490;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterAltKernel.u32All              = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterDCIndFlags.u32All             = 0x00934f3e;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterDerFlags.u32All               = 0x00934f3e;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterDer2Flags.u32All              = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterDirConfig.u32All              = 0xffff0000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterExternalDirConfig.u32All      = 0x0000ff00;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterYThrehClose3Mod.u32All        = 0x00000010;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterYThreshDer2Close3Mod.u32All   = 0x00000010;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterYThrFarMod.u32All             = 0x00000010;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterUThrFarMod.u32All             = 0x00000010;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterVThrFarMod.u32All             = 0x00000010;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterYThrCloseMod.u32All           = 0x00000010;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterYThrFarExMod.u32All           = 0x00000010;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterUThrFarExMod.u32All           = 0x00000010;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yFilterVThrFarExMod.u32All           = 0x00000010;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yDCIndYGain30.u32All                 = 0x2aaaaaaa;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yDCIndYGain50.u32All                 = 0x2aaaaaaa;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yDCIndYGain70.u32All                 = 0x2aaaaaaa;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yDCIndYGain90.u32All                 = 0x2aaaaaaa;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yDCIndYGain5X0.u32All                = 0x2aaaaaaa;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yDCIndYGain9X0.u32All                = 0x2aaaaaaa;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yDCIndUVGain30.u32All                = 0x2aaaaaaa;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yDCIndUVGain50.u32All                = 0x2aaaaaaa;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.yDCIndUVGain5X0.u32All               = 0x2aaaaaaa;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterConfig.u32All                 = 0x00028383;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterDetailCond.u32All             = 0x0000ff00;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cThreshold.u32All                    = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterGreyYDer.u32All               = 0x007f0100;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterGreyChromaticity.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterGreyLimits.u32All             = 0x00000040;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterGreyBlendScale.u32All         = 0x00000010;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterGreyBlendOffset.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterGreySizeThr35.u32All          = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterGreySizeThr79.u32All          = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterGreyThrClose.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterGreyThrFar.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterDCIndFlags.u32All             = 0x00004f3e;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterDerFlags.u32All               = 0x00004f3e;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterDirConfig.u32All              = 0xffff0000;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterDirConfigEx.u32All            = 0x0000ff00;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterYThrFarMod.u32All             = 0x00000010;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterUThrFarMod.u32All             = 0x00000010;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterVThrFarMod.u32All             = 0x00000010;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterUThrDistMod.u32All            = 0x00000010;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterVThrDisMod.u32All             = 0x00000010;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterYThrCloseMod.u32All           = 0x00000010;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterYThrFarExMod.u32All           = 0x00000010;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterUThrFarExMod.u32All           = 0x00000010;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cFilterVThrFarExMod.u32All           = 0x00000010;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cDCIndYGain30.u32All                 = 0x00aaaaaa;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cDCIndYGain50.u32All                 = 0x00aaaaaa;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cDCIndYGain70.u32All                 = 0x00aaaaaa;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cDCIndYGain90.u32All                 = 0x00aaaaaa;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cDCIndYGain5X0.u32All                = 0x00aaaaaa;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cDCIndYGain9X0.u32All                = 0x00aaaaaa;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cDCIndUVGain30.u32All                = 0x00aaaaaa;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cDCIndUVGain50.u32All                = 0x00aaaaaa;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cDCIndUVGain70.u32All                = 0x00aaaaaa;
    pRegCmd[PASS_NAME_DC_16].cylpfPostLensGainCmd.cDCIndUVGain5X0.u32All               = 0x00aaaaaa;

    pRegCmd[PASS_NAME_DC_16].cnrCmd.bypass.u32All              = 0x00000001;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.config.u32All              = 0x00000032;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.debug.u32All               = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.filterMode.u32All          = 0x0000000f;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.specialParams.u32All       = 0x00700000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.cMode.u32All               = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.angleMin0.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.angleMin1.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.angleMin2.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.angleMax0.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.angleMax1.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.angleMax2.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.angleInternal.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.colorSatMin0.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.colorSatMin1.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.colorSatMin2.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.colorSatMax0.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.colorSatMax1.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.colorSatMax2.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.yCompMin0.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.yCompMin1.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.yCompMax0.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.yCompMax1.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.boundaryProbability.u32All = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.q0.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.q1.u32All                  = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.gainFactorYY0.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.gainFactorYY1.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.gainFactorYY2.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.gainFactorUVY0.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.gainFactorUVY1.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.gainFactorUVY2.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.offsetYY0.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.offsetYY1.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.offsetYY2.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.offsetUY0.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.offsetUY1.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.offsetVY0.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.offsetVY1.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.gainFactorYC0.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.gainFactorYC1.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.gainFactorYC2.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.gainFactorUVC0.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.gainFactorUVC1.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.gainFactorUVC2.u32All      = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.offsetYC0.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.offsetYC1.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.offsetYC2.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.offsetUC0.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.offsetUC1.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.offsetVC0.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.offsetVC1.u32All           = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.blendFactorYDC0.u32All     = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.blendFactorYDC1.u32All     = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.blendFactorYALT0.u32All    = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.blendFactorYALT1.u32All    = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.blendFactorC0.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.blendFactorC1.u32All       = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.fdOffset.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.xCenter0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.xCenter1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.xCenter2.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.yCenter0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.yCenter1.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.yCenter2.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.rBoundary0.u32All          = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.rBoundary1.u32All          = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.rBoundary2.u32All          = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.rBoundaryShift0.u32All     = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.rSlope0.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.rSlope1.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.rSlope2.u32All             = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.rSlopeShift0.u32All        = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.combine0.u32All            = 0x00000000;
    pRegCmd[PASS_NAME_DC_16].cnrCmd.fdConfig.u32All            = 0x0000000a;

    return result;
}

CAMX_NAMESPACE_END
