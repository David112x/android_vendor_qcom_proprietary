////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifeprecrop10titan17x.cpp
/// @brief CAMXIFEPRECROP10TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifeprecrop10titan17x.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPreCrop10Titan17x::IFEPreCrop10Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEPreCrop10Titan17x::IFEPreCrop10Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEPreCrop10DS4LumaReg) / RegisterWidthInBytes) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEPreCrop10DS4ChromaReg) / RegisterWidthInBytes));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPreCrop10Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPreCrop10Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result            = CamxResultSuccess;
    ISPInputData* pInputData        = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer        = NULL;
    UINT32        register1         = 0;
    UINT32        numberOfValues1   = 0;
    UINT32*       pValues1          = NULL;
    UINT32        register2         = 0;
    UINT32        numberOfValues2   = 0;;
    UINT32*       pValues2          = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if (NULL != pInputData && NULL != pInputData->pCmdBuffer)
    {
        pCmdBuffer = pInputData->pCmdBuffer;

        switch (m_modulePath)
        {
            case IFEPipelinePath::VideoDS4Path:
                register1   = regIFE_IFE_0_VFE_DS4_Y_PRE_CROP_LINE_CFG;
                register2   = regIFE_IFE_0_VFE_DS4_C_PRE_CROP_LINE_CFG;

                numberOfValues1 = sizeof(IFEPreCrop10DS4LumaReg) / RegisterWidthInBytes;
                pValues1        = reinterpret_cast<UINT32*>(&m_regCmd.DS4Luma);
                numberOfValues2 = sizeof(IFEPreCrop10DS4ChromaReg) / RegisterWidthInBytes;
                pValues2        = reinterpret_cast<UINT32*>(&m_regCmd.DS4Chroma);
                break;

            case IFEPipelinePath::VideoDS16Path:
                register1   = regIFE_IFE_0_VFE_DS16_Y_PRE_CROP_LINE_CFG;
                register2   = regIFE_IFE_0_VFE_DS16_C_PRE_CROP_LINE_CFG;

                // Note, the register types for both DS4/DS16 are identical.
                numberOfValues1 = sizeof(IFEPreCrop10DS4LumaReg) / RegisterWidthInBytes;
                pValues1        = reinterpret_cast<UINT32*>(&m_regCmd.DS16Luma);
                numberOfValues2 = sizeof(IFEPreCrop10DS4ChromaReg) / RegisterWidthInBytes;
                pValues2        = reinterpret_cast<UINT32*>(&m_regCmd.DS16Chroma);
                break;

            case IFEPipelinePath::DisplayDS4Path:
                register1   = regIFE_IFE_0_VFE_DISP_DS4_Y_PRE_CROP_LINE_CFG;
                register2   = regIFE_IFE_0_VFE_DISP_DS4_C_PRE_CROP_LINE_CFG;

                numberOfValues1 = sizeof(IFEPreCrop10DisplayDS4LumaReg) / RegisterWidthInBytes;
                pValues1        = reinterpret_cast<UINT32*>(&m_regCmd.displayDS4Luma);
                numberOfValues2 = sizeof(IFEPreCrop10DisplayDS4ChromaReg) / RegisterWidthInBytes;
                pValues2        = reinterpret_cast<UINT32*>(&m_regCmd.displayDS4Chroma);
                break;

            case IFEPipelinePath::DisplayDS16Path:
                register1   = regIFE_IFE_0_VFE_DISP_DS16_Y_PRE_CROP_LINE_CFG;
                register2   = regIFE_IFE_0_VFE_DISP_DS16_C_PRE_CROP_LINE_CFG;

                // Note, the register types for both DS4/DS16 are identical.
                numberOfValues1 = sizeof(IFEPreCrop10DisplayDS4LumaReg) / RegisterWidthInBytes;
                pValues1        = reinterpret_cast<UINT32*>(&m_regCmd.displayDS16Luma);
                numberOfValues2 = sizeof(IFEPreCrop10DisplayDS4ChromaReg) / RegisterWidthInBytes;
                pValues2        = reinterpret_cast<UINT32*>(&m_regCmd.displayDS16Chroma);
                break;

            default:
                CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid module path for pre-crop module");
                result = CamxResultEInvalidState;
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer, register1, numberOfValues1, pValues1);
            if (CamxResultSuccess == result)
            {
                result = PacketBuilder::WriteRegRange(pCmdBuffer, register2, numberOfValues2, pValues2);
            }
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure PreCrop Register");
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPreCrop10Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPreCrop10Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result      = CamxResultSuccess;

    ISPInputData*           pInputData  = static_cast<ISPInputData*>(pInput);
    PreCrop10OutputData*    pOutputData = static_cast<PreCrop10OutputData*>(pOutput);
    m_modulePath                        = pOutputData->modulePath;

    if (IFEPipelinePath::VideoDS4Path == m_modulePath)
    {
        DS4PreCropInfo* pPreCropInfo = &pInputData->pCalculatedData->preCropInfo;

        m_regCmd.DS4Luma.lineConfig.bitfields.FIRST_LINE   = pPreCropInfo->YCrop.firstLine;
        m_regCmd.DS4Luma.lineConfig.bitfields.LAST_LINE    = pPreCropInfo->YCrop.lastLine;
        m_regCmd.DS4Chroma.lineConfig.bitfields.FIRST_LINE = pPreCropInfo->CrCrop.firstLine;
        m_regCmd.DS4Chroma.lineConfig.bitfields.LAST_LINE  = pPreCropInfo->CrCrop.lastLine;

        // Copy the values computed in the main scalar
        m_regCmd.DS4Luma.pixelConfig.bitfields.FIRST_PIXEL = pPreCropInfo->YCrop.firstPixel;
        m_regCmd.DS4Luma.pixelConfig.bitfields.LAST_PIXEL  = pPreCropInfo->YCrop.lastPixel;

        m_regCmd.DS4Chroma.pixelConfig.bitfields.FIRST_PIXEL = pPreCropInfo->CrCrop.firstPixel;
        m_regCmd.DS4Chroma.pixelConfig.bitfields.LAST_PIXEL  = pPreCropInfo->CrCrop.lastPixel;
    }
    else if (IFEPipelinePath::VideoDS16Path == m_modulePath)
    {
        DS4PreCropInfo* pPreCropInfoDS16 = &pInputData->pCalculatedData->preCropInfoDS16;

        m_regCmd.DS16Luma.lineConfig.bitfields.FIRST_LINE   = pPreCropInfoDS16->YCrop.firstLine;
        m_regCmd.DS16Luma.lineConfig.bitfields.LAST_LINE    = pPreCropInfoDS16->YCrop.lastLine;
        m_regCmd.DS16Chroma.lineConfig.bitfields.FIRST_LINE = pPreCropInfoDS16->CrCrop.firstLine;
        m_regCmd.DS16Chroma.lineConfig.bitfields.LAST_LINE  = pPreCropInfoDS16->CrCrop.lastLine;

        // Copy the values computed in the main scalar
        m_regCmd.DS16Luma.pixelConfig.bitfields.FIRST_PIXEL = pPreCropInfoDS16->YCrop.firstPixel;
        m_regCmd.DS16Luma.pixelConfig.bitfields.LAST_PIXEL  = pPreCropInfoDS16->YCrop.lastPixel;

        m_regCmd.DS16Chroma.pixelConfig.bitfields.FIRST_PIXEL = pPreCropInfoDS16->CrCrop.firstPixel;
        m_regCmd.DS16Chroma.pixelConfig.bitfields.LAST_PIXEL  = pPreCropInfoDS16->CrCrop.lastPixel;
    }
    else if (IFEPipelinePath::DisplayDS4Path == m_modulePath)
    {
        DS4PreCropInfo* pPreCropInfo = &pInputData->pCalculatedData->dispPreCropInfo;

        m_regCmd.displayDS4Luma.lineConfig.bitfields.FIRST_LINE   = pPreCropInfo->YCrop.firstLine;
        m_regCmd.displayDS4Luma.lineConfig.bitfields.LAST_LINE    = pPreCropInfo->YCrop.lastLine;
        m_regCmd.displayDS4Chroma.lineConfig.bitfields.FIRST_LINE = pPreCropInfo->CrCrop.firstLine;
        m_regCmd.displayDS4Chroma.lineConfig.bitfields.LAST_LINE  = pPreCropInfo->CrCrop.lastLine;

        // Copy the values computed in the main scalar
        m_regCmd.displayDS4Luma.pixelConfig.bitfields.FIRST_PIXEL = pPreCropInfo->YCrop.firstPixel;
        m_regCmd.displayDS4Luma.pixelConfig.bitfields.LAST_PIXEL  = pPreCropInfo->YCrop.lastPixel;

        m_regCmd.displayDS4Chroma.pixelConfig.bitfields.FIRST_PIXEL = pPreCropInfo->CrCrop.firstPixel;
        m_regCmd.displayDS4Chroma.pixelConfig.bitfields.LAST_PIXEL  = pPreCropInfo->CrCrop.lastPixel;
    }
    else if (IFEPipelinePath::DisplayDS16Path == m_modulePath)
    {
        DS4PreCropInfo* pPreCropInfoDS16 =  &pInputData->pCalculatedData->dispPreCropInfoDS16;

        m_regCmd.displayDS16Luma.lineConfig.bitfields.FIRST_LINE   = pPreCropInfoDS16->YCrop.firstLine;
        m_regCmd.displayDS16Luma.lineConfig.bitfields.LAST_LINE    = pPreCropInfoDS16->YCrop.lastLine;
        m_regCmd.displayDS16Chroma.lineConfig.bitfields.FIRST_LINE = pPreCropInfoDS16->CrCrop.firstLine;
        m_regCmd.displayDS16Chroma.lineConfig.bitfields.LAST_LINE  = pPreCropInfoDS16->CrCrop.lastLine;

        // Copy the values computed in the main scalar
        m_regCmd.displayDS16Luma.pixelConfig.bitfields.FIRST_PIXEL = pPreCropInfoDS16->YCrop.firstPixel;
        m_regCmd.displayDS16Luma.pixelConfig.bitfields.LAST_PIXEL  = pPreCropInfoDS16->YCrop.lastPixel;

        m_regCmd.displayDS16Chroma.pixelConfig.bitfields.FIRST_PIXEL = pPreCropInfoDS16->CrCrop.firstPixel;
        m_regCmd.displayDS16Chroma.pixelConfig.bitfields.LAST_PIXEL  = pPreCropInfoDS16->CrCrop.lastPixel;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid IFE Path");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPreCrop10Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPreCrop10Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult    result      = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pInput);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPreCrop10Titan17x::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEPreCrop10Titan17x::CopyRegCmd(
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
// IFEPreCrop10Titan17x::~IFEPreCrop10Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEPreCrop10Titan17x::~IFEPreCrop10Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPreCrop10Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEPreCrop10Titan17x::DumpRegConfig()
{
    if (IFEPipelinePath::VideoDS4Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS4 Luma pre-Crop Config[%d, %d, %d, %d]",
                          m_regCmd.DS4Luma.pixelConfig.bitfields.FIRST_PIXEL,
                          m_regCmd.DS4Luma.lineConfig.bitfields.FIRST_LINE,
                          m_regCmd.DS4Luma.pixelConfig.bitfields.LAST_PIXEL,
                          m_regCmd.DS4Luma.lineConfig.bitfields.LAST_LINE);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS4 Chroma pre-Crop Config[%d, %d, %d, %d]",
                         m_regCmd.DS4Chroma.pixelConfig.bitfields.FIRST_PIXEL,
                         m_regCmd.DS4Chroma.lineConfig.bitfields.FIRST_LINE,
                         m_regCmd.DS4Chroma.pixelConfig.bitfields.LAST_PIXEL,
                         m_regCmd.DS4Chroma.lineConfig.bitfields.LAST_LINE);
    }
    else if (IFEPipelinePath::VideoDS16Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Luma pre-Crop Config[%d, %d, %d, %d]",
                         m_regCmd.DS16Luma.pixelConfig.bitfields.FIRST_PIXEL,
                         m_regCmd.DS16Luma.lineConfig.bitfields.FIRST_LINE,
                         m_regCmd.DS16Luma.pixelConfig.bitfields.LAST_PIXEL,
                         m_regCmd.DS16Luma.lineConfig.bitfields.LAST_LINE);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Chroma pre-Crop Config[%d, %d, %d, %d]",
                         m_regCmd.DS16Chroma.pixelConfig.bitfields.FIRST_PIXEL,
                         m_regCmd.DS16Chroma.lineConfig.bitfields.FIRST_LINE,
                         m_regCmd.DS16Chroma.pixelConfig.bitfields.LAST_PIXEL,
                         m_regCmd.DS16Chroma.lineConfig.bitfields.LAST_LINE);
    }
    else if (IFEPipelinePath::DisplayDS4Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "DS4 Display Luma pre-Crop Config[%d, %d, %d, %d]",
            m_regCmd.displayDS4Luma.pixelConfig.bitfields.FIRST_PIXEL,
            m_regCmd.displayDS4Luma.lineConfig.bitfields.FIRST_LINE,
            m_regCmd.displayDS4Luma.pixelConfig.bitfields.LAST_PIXEL,
            m_regCmd.displayDS4Luma.lineConfig.bitfields.LAST_LINE);

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "DS4 Display Chroma pre-Crop Config[%d, %d, %d, %d]",
            m_regCmd.displayDS4Chroma.pixelConfig.bitfields.FIRST_PIXEL,
            m_regCmd.displayDS4Chroma.lineConfig.bitfields.FIRST_LINE,
            m_regCmd.displayDS4Chroma.pixelConfig.bitfields.LAST_PIXEL,
            m_regCmd.displayDS4Chroma.lineConfig.bitfields.LAST_LINE);
    }
    else
    {
        // IFEPipelinePath::DisplayDS16Path == m_modulePath
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "DS16 Display Luma pre-Crop Config[%d, %d, %d, %d]",
            m_regCmd.displayDS16Luma.pixelConfig.bitfields.FIRST_PIXEL,
            m_regCmd.displayDS16Luma.lineConfig.bitfields.FIRST_LINE,
            m_regCmd.displayDS16Luma.pixelConfig.bitfields.LAST_PIXEL,
            m_regCmd.displayDS16Luma.lineConfig.bitfields.LAST_LINE);

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "DS16 Display Chroma pre-Crop Config[%d, %d, %d, %d]",
            m_regCmd.displayDS16Chroma.pixelConfig.bitfields.FIRST_PIXEL,
            m_regCmd.displayDS16Chroma.lineConfig.bitfields.FIRST_LINE,
            m_regCmd.displayDS16Chroma.pixelConfig.bitfields.LAST_PIXEL,
            m_regCmd.displayDS16Chroma.lineConfig.bitfields.LAST_LINE);
    }
}

CAMX_NAMESPACE_END
