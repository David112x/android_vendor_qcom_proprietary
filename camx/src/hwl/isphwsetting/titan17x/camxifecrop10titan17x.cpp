////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecrop10titan17x.cpp
/// @brief CAMXIFECROP10TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifecrop10titan17x.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::IFECrop10Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECrop10Titan17x::IFECrop10Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFECrop10FDLumaReg)     / RegisterWidthInBytes) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFECrop10FDChromaReg)   / RegisterWidthInBytes) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFECrop10FullLumaReg)   / RegisterWidthInBytes) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFECrop10FullChromaReg) / RegisterWidthInBytes) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFECropPixelRawReg)     / RegisterWidthInBytes));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECrop10Titan17x::CreateCmdList(
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
        switch (m_output)
        {
            case FDOutput:
                register1       = regIFE_IFE_0_VFE_FD_OUT_Y_CROP_LINE_CFG;
                numberOfValues1 = sizeof(IFECrop10FDLumaReg) / RegisterWidthInBytes;
                pValues1        = reinterpret_cast<UINT32*>(&m_regCmd.FDLuma);

                register2       = regIFE_IFE_0_VFE_FD_OUT_C_CROP_LINE_CFG;
                numberOfValues2 = sizeof(IFECrop10FDChromaReg) / RegisterWidthInBytes;
                pValues2        = reinterpret_cast<UINT32*>(&m_regCmd.FDChroma);
                break;

            case FullOutput:
                register1       = regIFE_IFE_0_VFE_FULL_OUT_Y_CROP_LINE_CFG;
                numberOfValues1 = (sizeof(IFECrop10FullLumaReg) / RegisterWidthInBytes);
                pValues1        = reinterpret_cast<UINT32*>(&m_regCmd.fullLuma);

                register2       = regIFE_IFE_0_VFE_FULL_OUT_C_CROP_LINE_CFG;
                numberOfValues2 = sizeof(IFECrop10FullChromaReg) / RegisterWidthInBytes;
                pValues2        = reinterpret_cast<UINT32*>(&m_regCmd.fullChroma);
                break;

            case DS4Output:
                register1       = regIFE_IFE_0_VFE_DS4_Y_CROP_LINE_CFG;
                numberOfValues1 = sizeof(IFECrop10DS4LumaReg) / RegisterWidthInBytes;
                pValues1        = reinterpret_cast<UINT32*>(&m_regCmd.DS4Luma);

                register2       = regIFE_IFE_0_VFE_DS4_C_CROP_LINE_CFG;
                numberOfValues2 = sizeof(IFECrop10DS4ChromaReg) / RegisterWidthInBytes;
                pValues2        = reinterpret_cast<UINT32*>(&m_regCmd.DS4Chroma);;
                break;

            case DS16Output:
                register1       = regIFE_IFE_0_VFE_DS16_Y_CROP_LINE_CFG;
                numberOfValues1 = sizeof(IFECrop10DS16LumaReg) / RegisterWidthInBytes;
                pValues1        = reinterpret_cast<UINT32*>(&m_regCmd.DS16Luma);

                register2       = regIFE_IFE_0_VFE_DS16_C_CROP_LINE_CFG;
                numberOfValues2 = sizeof(IFECrop10DS16ChromaReg) / RegisterWidthInBytes;
                pValues2        = reinterpret_cast<UINT32*>(&m_regCmd.DS16Chroma);
                break;

            case PixelRawOutput:
                register1       = regIFE_IFE_0_VFE_CROP_PIXEL_RAW_DUMP_WIDTH_CFG;
                numberOfValues1 = sizeof(IFECropPixelRawReg) / RegisterWidthInBytes;
                pValues1        = reinterpret_cast<UINT32*>(&m_regCmd.pixelRaw);
                numberOfValues2 = 0;
                break;

            case DisplayFullOutput:
                register1       = regIFE_IFE_0_VFE_DISP_OUT_Y_CROP_LINE_CFG;
                numberOfValues1 = (sizeof(IFECrop10DisplayFullLumaReg) / RegisterWidthInBytes);
                pValues1        = reinterpret_cast<UINT32*>(&m_regCmd.displayFullLuma);

                register2       = regIFE_IFE_0_VFE_DISP_OUT_C_CROP_LINE_CFG;
                numberOfValues2 = sizeof(IFECrop10DisplayFullChromaReg) / RegisterWidthInBytes;
                pValues2        = reinterpret_cast<UINT32*>(&m_regCmd.displayFullChroma);
                break;

            case DisplayDS4Output:
                register1       = regIFE_IFE_0_VFE_DISP_DS4_Y_CROP_LINE_CFG;
                numberOfValues1 = sizeof(IFECrop10DisplayDS4LumaReg) / RegisterWidthInBytes;
                pValues1        = reinterpret_cast<UINT32*>(&m_regCmd.displayDS4Luma);

                register2       = regIFE_IFE_0_VFE_DISP_DS4_C_CROP_LINE_CFG;
                numberOfValues2 = sizeof(IFECrop10DisplayDS4ChromaReg) / RegisterWidthInBytes;
                pValues2        = reinterpret_cast<UINT32*>(&m_regCmd.displayDS4Chroma);;
                break;

            case DisplayDS16Output:
                register1       = regIFE_IFE_0_VFE_DISP_DS16_Y_CROP_LINE_CFG;
                numberOfValues1 = sizeof(IFECrop10DisplayDS16LumaReg) / RegisterWidthInBytes;
                pValues1        = reinterpret_cast<UINT32*>(&m_regCmd.displayDS16Luma);

                register2       = regIFE_IFE_0_VFE_DISP_DS16_C_CROP_LINE_CFG;
                numberOfValues2 = sizeof(IFECrop10DisplayDS16ChromaReg) / RegisterWidthInBytes;
                pValues2        = reinterpret_cast<UINT32*>(&m_regCmd.displayDS16Chroma);
                break;

            default:
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "We would never runinto this case");
                return CamxResultEInvalidState;
                break;
        }

        result = PacketBuilder::WriteRegRange(pCmdBuffer, register1, numberOfValues1, pValues1);

        if ((CamxResultSuccess == result) && (numberOfValues2 > 0))
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer, register2, numberOfValues2, pValues2);
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure Crop Register for path %d", m_output);
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
// IFECrop10Titan17x::GetChromaSubsampleFactor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECrop10Titan17x::GetChromaSubsampleFactor(
    FLOAT* pHorizontalSubSampleFactor,
    FLOAT* pVerticalSubsampleFactor,
    Format pixelFormat)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pHorizontalSubSampleFactor) &&
        (NULL != pVerticalSubsampleFactor))
    {
        switch (pixelFormat)
        {
            /// @todo (CAMX-526) Check for all the formats and subsample factors
            case Format::YUV420NV12TP10:
            case Format::YUV420NV21TP10:
            case Format::YUV422NV16TP10:
            case Format::YUV420NV12:
            case Format::YUV420NV21:
            case Format::YUV422NV16:
            case Format::UBWCTP10:
            case Format::UBWCNV124R:
            case Format::PD10:
            case Format::P010:
                *pHorizontalSubSampleFactor = 1.0f;
                *pVerticalSubsampleFactor   = 2.0f;
                break;

            case Format::Y8:
            case Format::Y16:
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupISP, "Incompatible Format: %d, path %d",  pixelFormat, m_modulePath);
                *pHorizontalSubSampleFactor = 1.0f;
                *pVerticalSubsampleFactor   = 1.0f;
                result = CamxResultEInvalidArg;
                break;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input Null pointer");
        result = CamxResultEFailed;
    }

    return  result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::ModifyCropWindow
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECrop10Titan17x::ModifyCropWindow(
    CropType cropType)
{
    CamxResult result            = CamxResultSuccess;
    FLOAT      outputAspectRatio;
    FLOAT      cropAspectRatio;
    UINT32     temporaryValue;

    CAMX_UNREFERENCED_PARAM(cropType);

    CAMX_ASSERT_MESSAGE(0 != m_pState->streamDimension.height, "Invalid output height");
    CAMX_ASSERT_MESSAGE(0 != m_pState->modifiedCropWindow.height, "Invalid Crop window height");

    outputAspectRatio = static_cast<FLOAT>(m_pState->streamDimension.width)    / m_pState->streamDimension.height;
    cropAspectRatio   = static_cast<FLOAT>(m_pState->modifiedCropWindow.width) / m_pState->modifiedCropWindow.height;

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "output m_output %d, MNDS Output %d * %d, Crop Window [%d, %d, %d, %d]",
                     m_output,
                     m_pState->scalerOutput.dimension.width,
                     m_pState->scalerOutput.dimension.height,
                     m_pState->modifiedCropWindow.left,
                     m_pState->modifiedCropWindow.top,
                     m_pState->modifiedCropWindow.width,
                     m_pState->modifiedCropWindow.height);

    // Modify crop window based on the aspect ratio, Crop window should match the output
    if (Utils::AbsoluteFLOAT(outputAspectRatio - cropAspectRatio) > AspectRatioTolerance)
    {
        if (outputAspectRatio > cropAspectRatio)
        {
            temporaryValue                      = m_pState->modifiedCropWindow.height;
            m_pState->modifiedCropWindow.height =
                Utils::FloorUINT32(2, (static_cast<UINT32>(m_pState->modifiedCropWindow.width / outputAspectRatio)));
            m_pState->modifiedCropWindow.top    =
                m_pState->modifiedCropWindow.top + ((temporaryValue - m_pState->modifiedCropWindow.height) >> 1);
        }
        else
        {
            temporaryValue                        = m_pState->modifiedCropWindow.width;
            m_pState->modifiedCropWindow.width    =
                Utils::Utils::FloorUINT32(2, (static_cast<UINT32>(m_pState->modifiedCropWindow.height * outputAspectRatio)));
            if (CropTypeFromRight == cropType)
            {
                m_pState->modifiedCropWindow.left = m_pState->modifiedCropWindow.left;
            }
            else if (CropTypeFromLeft == cropType)
            {
                m_pState->modifiedCropWindow.left =
                    m_pState->modifiedCropWindow.left + (temporaryValue - m_pState->modifiedCropWindow.width);
            }
            else
            {
                m_pState->modifiedCropWindow.left =
                    m_pState->modifiedCropWindow.left + ((temporaryValue - m_pState->modifiedCropWindow.width) >> 1);
            }
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Modified Crop Window [%d, %d, %d, %d]",
                     m_pState->modifiedCropWindow.left,
                     m_pState->modifiedCropWindow.top,
                     m_pState->modifiedCropWindow.width,
                     m_pState->modifiedCropWindow.height);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::ConfigureFDLumaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECrop10Titan17x::ConfigureFDLumaRegisters(
    CropInfo* pCrop)
{
    m_regCmd.FDLuma.pixelConfig.bitfields.FIRST_PIXEL = pCrop->firstPixel;
    m_regCmd.FDLuma.pixelConfig.bitfields.LAST_PIXEL  = pCrop->lastPixel;
    m_regCmd.FDLuma.lineConfig.bitfields.FIRST_LINE   = pCrop->firstLine;
    m_regCmd.FDLuma.lineConfig.bitfields.LAST_LINE    = pCrop->lastLine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::ConfigureFDChromaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECrop10Titan17x::ConfigureFDChromaRegisters(
    CropInfo* pCrop)
{
    m_regCmd.FDChroma.pixelConfig.bitfields.FIRST_PIXEL = pCrop->firstPixel;
    m_regCmd.FDChroma.pixelConfig.bitfields.LAST_PIXEL  = pCrop->lastPixel;
    m_regCmd.FDChroma.lineConfig.bitfields.FIRST_LINE   = pCrop->firstLine;
    m_regCmd.FDChroma.lineConfig.bitfields.LAST_LINE    = pCrop->lastLine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::ConfigureFullLumaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECrop10Titan17x::ConfigureFullLumaRegisters(
    CropInfo* pCrop)
{
    m_regCmd.fullLuma.pixelConfig.bitfields.FIRST_PIXEL = pCrop->firstPixel;
    m_regCmd.fullLuma.pixelConfig.bitfields.LAST_PIXEL  = pCrop->lastPixel;
    m_regCmd.fullLuma.lineConfig.bitfields.FIRST_LINE   = pCrop->firstLine;
    m_regCmd.fullLuma.lineConfig.bitfields.LAST_LINE    = pCrop->lastLine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::ConfigureFullChromaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECrop10Titan17x::ConfigureFullChromaRegisters(
    CropInfo* pCrop)
{
    m_regCmd.fullChroma.pixelConfig.bitfields.FIRST_PIXEL = pCrop->firstPixel;
    m_regCmd.fullChroma.pixelConfig.bitfields.LAST_PIXEL  = pCrop->lastPixel;
    m_regCmd.fullChroma.lineConfig.bitfields.FIRST_LINE   = pCrop->firstLine;
    m_regCmd.fullChroma.lineConfig.bitfields.LAST_LINE    = pCrop->lastLine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::ConfigureDisplayFullLumaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECrop10Titan17x::ConfigureDisplayFullLumaRegisters(
    CropInfo* pCrop)
{
    m_regCmd.displayFullLuma.pixelConfig.bitfields.FIRST_PIXEL = pCrop->firstPixel;
    m_regCmd.displayFullLuma.pixelConfig.bitfields.LAST_PIXEL  = pCrop->lastPixel;
    m_regCmd.displayFullLuma.lineConfig.bitfields.FIRST_LINE   = pCrop->firstLine;
    m_regCmd.displayFullLuma.lineConfig.bitfields.LAST_LINE    = pCrop->lastLine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::ConfigureDisplayFullChromaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECrop10Titan17x::ConfigureDisplayFullChromaRegisters(
    CropInfo* pCrop)
{
    m_regCmd.displayFullChroma.pixelConfig.bitfields.FIRST_PIXEL = pCrop->firstPixel;
    m_regCmd.displayFullChroma.pixelConfig.bitfields.LAST_PIXEL  = pCrop->lastPixel;
    m_regCmd.displayFullChroma.lineConfig.bitfields.FIRST_LINE   = pCrop->firstLine;
    m_regCmd.displayFullChroma.lineConfig.bitfields.LAST_LINE    = pCrop->lastLine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::ConfigureDS4LumaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECrop10Titan17x::ConfigureDS4LumaRegisters(
    CropInfo* pCrop)
{
    m_regCmd.DS4Luma.pixelConfig.bitfields.FIRST_PIXEL = pCrop->firstPixel;
    m_regCmd.DS4Luma.pixelConfig.bitfields.LAST_PIXEL  = pCrop->lastPixel;
    m_regCmd.DS4Luma.lineConfig.bitfields.FIRST_LINE   = pCrop->firstLine;
    m_regCmd.DS4Luma.lineConfig.bitfields.LAST_LINE    = pCrop->lastLine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::ConfigureDS4ChromaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECrop10Titan17x::ConfigureDS4ChromaRegisters(
    CropInfo* pCrop)
{
    m_regCmd.DS4Chroma.pixelConfig.bitfields.FIRST_PIXEL = pCrop->firstPixel;
    m_regCmd.DS4Chroma.pixelConfig.bitfields.LAST_PIXEL  = pCrop->lastPixel;
    m_regCmd.DS4Chroma.lineConfig.bitfields.FIRST_LINE   = pCrop->firstLine;
    m_regCmd.DS4Chroma.lineConfig.bitfields.LAST_LINE    = pCrop->lastLine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::ConfigureDS16LumaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECrop10Titan17x::ConfigureDS16LumaRegisters(
    CropInfo* pCrop)
{
    m_regCmd.DS16Luma.pixelConfig.bitfields.FIRST_PIXEL = pCrop->firstPixel;
    m_regCmd.DS16Luma.pixelConfig.bitfields.LAST_PIXEL  = pCrop->lastPixel;
    m_regCmd.DS16Luma.lineConfig.bitfields.FIRST_LINE   = pCrop->firstLine;
    m_regCmd.DS16Luma.lineConfig.bitfields.LAST_LINE    = pCrop->lastLine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::ConfigureDS16ChromaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECrop10Titan17x::ConfigureDS16ChromaRegisters(
    CropInfo* pCrop)
{
    m_regCmd.DS16Chroma.pixelConfig.bitfields.FIRST_PIXEL = pCrop->firstPixel;
    m_regCmd.DS16Chroma.pixelConfig.bitfields.LAST_PIXEL  = pCrop->lastPixel;
    m_regCmd.DS16Chroma.lineConfig.bitfields.FIRST_LINE   = pCrop->firstLine;
    m_regCmd.DS16Chroma.lineConfig.bitfields.LAST_LINE    = pCrop->lastLine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::ConfigurePixelRawRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECrop10Titan17x::ConfigurePixelRawRegisters(
    CropInfo* pCrop)
{
    m_regCmd.pixelRaw.pixelConfig.bitfields.FIRST_PIXEL = pCrop->firstPixel;
    m_regCmd.pixelRaw.pixelConfig.bitfields.LAST_PIXEL  = pCrop->lastPixel;
    m_regCmd.pixelRaw.lineConfig.bitfields.FIRST_LINE   = pCrop->firstLine;
    m_regCmd.pixelRaw.lineConfig.bitfields.LAST_LINE    = pCrop->lastLine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::ConfigureDisplayDS4LumaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECrop10Titan17x::ConfigureDisplayDS4LumaRegisters(
    CropInfo* pCrop)
{
    m_regCmd.displayDS4Luma.pixelConfig.bitfields.FIRST_PIXEL = pCrop->firstPixel;
    m_regCmd.displayDS4Luma.pixelConfig.bitfields.LAST_PIXEL  = pCrop->lastPixel;
    m_regCmd.displayDS4Luma.lineConfig.bitfields.FIRST_LINE   = pCrop->firstLine;
    m_regCmd.displayDS4Luma.lineConfig.bitfields.LAST_LINE    = pCrop->lastLine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::ConfigureDisplayDS4ChromaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECrop10Titan17x::ConfigureDisplayDS4ChromaRegisters(
    CropInfo* pCrop)
{
    m_regCmd.displayDS4Chroma.pixelConfig.bitfields.FIRST_PIXEL = pCrop->firstPixel;
    m_regCmd.displayDS4Chroma.pixelConfig.bitfields.LAST_PIXEL  = pCrop->lastPixel;
    m_regCmd.displayDS4Chroma.lineConfig.bitfields.FIRST_LINE   = pCrop->firstLine;
    m_regCmd.displayDS4Chroma.lineConfig.bitfields.LAST_LINE    = pCrop->lastLine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::ConfigureDisplayDS16LumaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECrop10Titan17x::ConfigureDisplayDS16LumaRegisters(
    CropInfo* pCrop)
{
    m_regCmd.displayDS16Luma.pixelConfig.bitfields.FIRST_PIXEL = pCrop->firstPixel;
    m_regCmd.displayDS16Luma.pixelConfig.bitfields.LAST_PIXEL  = pCrop->lastPixel;
    m_regCmd.displayDS16Luma.lineConfig.bitfields.FIRST_LINE   = pCrop->firstLine;
    m_regCmd.displayDS16Luma.lineConfig.bitfields.LAST_LINE    = pCrop->lastLine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::ConfigureDisplayDS16ChromaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECrop10Titan17x::ConfigureDisplayDS16ChromaRegisters(
    CropInfo* pCrop)
{
    m_regCmd.displayDS16Chroma.pixelConfig.bitfields.FIRST_PIXEL = pCrop->firstPixel;
    m_regCmd.displayDS16Chroma.pixelConfig.bitfields.LAST_PIXEL  = pCrop->lastPixel;
    m_regCmd.displayDS16Chroma.lineConfig.bitfields.FIRST_LINE   = pCrop->firstLine;
    m_regCmd.displayDS16Chroma.lineConfig.bitfields.LAST_LINE    = pCrop->lastLine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::UpdateAppliedCrop
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECrop10Titan17x::UpdateAppliedCrop(
    CropInfo*   pCrop)
{
    CamxResult result = CamxResultSuccess;
    if ((NULL != pCrop) && (NULL != m_pState))
    {
        if ((TRUE                    == m_pState->scalerOutput.scalingLimitHit) ||
            (IFECropScalingThreshold >  m_pState->scalerOutput.scalingFactor)   ||
            (FDOutput                == m_output))
        {
            // Applied crop needs to be posted w.r.t Sensor Dimension.
            // pCrop values are w.r.t to Scaler output.
            m_pState->appliedCropInfo.left   = pCrop->firstPixel * m_pState->scalerOutput.scalingFactor;;
            m_pState->appliedCropInfo.top    = pCrop->firstLine * m_pState->scalerOutput.scalingFactor;;
            m_pState->appliedCropInfo.width  = ((pCrop->lastPixel - pCrop->firstPixel) + 1) *
                m_pState->scalerOutput.scalingFactor;
            m_pState->appliedCropInfo.height = ((pCrop->lastLine - pCrop->firstLine) + 1) *
                m_pState->scalerOutput.scalingFactor;
        }
        else
        {
            m_pState->appliedCropInfo.left   = m_pState->modifiedCropWindow.left;
            m_pState->appliedCropInfo.top    = m_pState->modifiedCropWindow.top;
            m_pState->appliedCropInfo.width  = m_pState->modifiedCropWindow.width;
            m_pState->appliedCropInfo.height = m_pState->modifiedCropWindow.height;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::CalculateCropCoordinates
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECrop10Titan17x::CalculateCropCoordinates(
    CropInfo*   pCrop,
    CropType    cropType)
{
    UINT32 cropLeft;
    UINT32 cropTop;
    UINT32 cropWidth;
    UINT32 cropHeight;
    UINT32 offCenterCoordinateX;
    UINT32 offCenterCoordinateY;
    UINT32 centerCoordinateX;
    UINT32 centerCoordinateY;
    UINT32 temporaryValue;

    m_pState->cropInfo.left   = 0;
    m_pState->cropInfo.top    = 0;
    m_pState->cropInfo.width  = m_pState->streamDimension.width;
    m_pState->cropInfo.height = m_pState->streamDimension.height;

    m_pState->appliedCropInfo.left   = 0;
    m_pState->appliedCropInfo.top    = 0;
    m_pState->appliedCropInfo.width  = m_pState->streamDimension.width;
    m_pState->appliedCropInfo.height = m_pState->streamDimension.height;

    CAMX_ASSERT_MESSAGE(0 != m_pState->scalerOutput.scalingFactor, "Invalid Scale factor");

    // Map crop window on scaler output
    cropLeft   = Utils::EvenCeilingUINT32(static_cast<UINT32>(m_pState->modifiedCropWindow.left   /
                        m_pState->scalerOutput.scalingFactor));
    cropTop    = Utils::EvenCeilingUINT32(static_cast<UINT32>(m_pState->modifiedCropWindow.top    /
                        m_pState->scalerOutput.scalingFactor));
    cropWidth  = Utils::EvenCeilingUINT32(static_cast<UINT32>(m_pState->modifiedCropWindow.width  /
                        m_pState->scalerOutput.scalingFactor));
    cropHeight = Utils::EvenCeilingUINT32(static_cast<UINT32>(m_pState->modifiedCropWindow.height /
                        m_pState->scalerOutput.scalingFactor));

    // Check and adjust crop window, if it exceeds scaler output dimensions due to roundings
    if ((cropLeft + cropWidth) > m_pState->scalerOutput.dimension.width)
    {
        if (cropWidth < m_pState->scalerOutput.dimension.width)
        {
            cropLeft = (m_pState->scalerOutput.dimension.width - cropWidth) / 2;
        }
        else
        {
            cropWidth = m_pState->scalerOutput.dimension.width - (cropLeft * 2);
        }
    }

    if ((cropTop + cropHeight) > m_pState->scalerOutput.dimension.height)
    {
        if (cropHeight < m_pState->scalerOutput.dimension.height)
        {
            cropTop = (m_pState->scalerOutput.dimension.height - cropHeight) / 2;
        }
        else
        {
            cropHeight = m_pState->scalerOutput.dimension.height - (cropTop * 2);
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Path %d, Crop Map [%d , %d, %d, %d], Scale factor %f\n",
                     m_output,
                     cropLeft,
                     cropTop,
                     cropWidth,
                     cropHeight,
                     m_pState->scalerOutput.scalingFactor);

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Path %d, Crop Window [%d , %d, %d, %d], Scale factor %f\n",
                     m_output,
                     m_pState->modifiedCropWindow.left,
                     m_pState->modifiedCropWindow.top,
                     m_pState->modifiedCropWindow.width,
                     m_pState->modifiedCropWindow.height,
                     m_pState->scalerOutput.scalingFactor);

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Path %d, Scaler output[%d * %d], IFE output [%d * %d], Scale factor %f\n",
                     m_output,
                     m_pState->scalerOutput.dimension.width,
                     m_pState->scalerOutput.dimension.height,
                     m_pState->streamDimension.width,
                     m_pState->streamDimension.height,
                     m_pState->scalerOutput.scalingFactor);

    // Calculate the pixel co-ordinates for crop
    if ((cropWidth >= m_pState->streamDimension.width) &&
        (cropWidth <= m_pState->scalerOutput.dimension.width))
    {
        // Analog zoom case
        if (CropTypeFromRight == cropType)
        {
            pCrop->firstPixel = Utils::EvenFloorUINT32(cropLeft);
            pCrop->lastPixel  = pCrop->firstPixel + m_pState->streamDimension.width - 1;
        }
        else if (CropTypeFromLeft == cropType)
        {
            pCrop->firstPixel = Utils::EvenFloorUINT32(cropLeft + (cropWidth - m_pState->streamDimension.width));
            pCrop->lastPixel  = pCrop->firstPixel + m_pState->streamDimension.width - 1;
        }
        else
        {
            pCrop->firstPixel = Utils::FloorUINT32(2, (cropLeft + ((cropWidth - m_pState->streamDimension.width) / 2)));
            pCrop->lastPixel  = pCrop->firstPixel + m_pState->streamDimension.width - 1;
        }
    }
    else
    {
        // Digital zoom case
        /// @todo (CAMX-2121) Handle digital zoom properly (for offcenter cases / dual IFE cases)
        if (CropTypeFromRight == cropType)
        {
            pCrop->firstPixel = Utils::EvenFloorUINT32(cropLeft);
            pCrop->lastPixel  = pCrop->firstPixel + m_pState->streamDimension.width - 1;

            // Beyond IFE scaling range IPE cropping needed for zoom
            m_pState->cropInfo.left  = 0;
            m_pState->cropInfo.width = cropWidth;
        }
        else if (CropTypeFromLeft == cropType)
        {
            UINT32 cropRight        = cropLeft + cropWidth;
            offCenterCoordinateX    =
                (cropRight > m_pState->streamDimension.width) ? (cropRight - m_pState->streamDimension.width) : 0;

            pCrop->firstPixel = Utils::FloorUINT32(2, offCenterCoordinateX);
            pCrop->lastPixel  = pCrop->firstPixel + m_pState->streamDimension.width - 1;

            // Beyond IFE scaling range IPE cropping needed for zoom
            m_pState->cropInfo.left  = cropLeft - pCrop->firstPixel;
            m_pState->cropInfo.width = cropWidth;
        }
        else
        {
            centerCoordinateX  =
                Utils::FloorUINT32(2, ((m_pState->scalerOutput.dimension.width - m_pState->streamDimension.width) / 2));

            // Minimum to make sure the 1st pixel of crop window falls within the output of FOV module
            offCenterCoordinateX = Utils::MinUINT32(centerCoordinateX, cropLeft);

            // make sure the last pixel of crop window falls within the output of FOV module
            if ((offCenterCoordinateX + m_pState->streamDimension.width) < (cropLeft + cropWidth))
            {
                // Distance from the last pixel of the input to the last pixel of the crop window
                temporaryValue       = m_pState->scalerOutput.dimension.width - (cropLeft + cropWidth);
                // Move the coordinate furthuer to have the crop window in the FOV output
                offCenterCoordinateX = offCenterCoordinateX + (centerCoordinateX - temporaryValue);
            }

            pCrop->firstPixel = Utils::FloorUINT32(2, offCenterCoordinateX);
            if ((pCrop->firstPixel > m_pState->scalerOutput.dimension.width) ||
                (pCrop->firstPixel + m_pState->streamDimension.width >
                m_pState->scalerOutput.dimension.width))
            {
                pCrop->firstPixel = Utils::FloorUINT32(2, centerCoordinateX);
            }

            pCrop->lastPixel  = pCrop->firstPixel + m_pState->streamDimension.width - 1;

            // Beyond IFE scaling range IPE cropping needed for zoom
            m_pState->cropInfo.left  = cropLeft - pCrop->firstPixel;
            m_pState->cropInfo.width = cropWidth;
        }
    }

    // Calculate the line co-ordinates for crop
    if ((cropHeight >= m_pState->streamDimension.height) &&
        (cropHeight <= m_pState->scalerOutput.dimension.height))
    {
        pCrop->firstLine = Utils::FloorUINT32(2, (cropTop + ((cropHeight - m_pState->streamDimension.height) / 2)));
        pCrop->lastLine  = pCrop->firstLine + m_pState->streamDimension.height - 1;
    }
    else
    {
        // Digital zoom case
        // First line if we consider center zoom
        centerCoordinateY  =
            Utils::FloorUINT32(2, ((m_pState->scalerOutput.dimension.height - m_pState->streamDimension.height) / 2));

        // Minimum to make sure the 1st line of crop window falls within the output of FOV module
        offCenterCoordinateY = Utils::MinUINT32(centerCoordinateY, cropTop);

        // make sure the last line of crop window falls within the output of FOV module
        if ((offCenterCoordinateY + m_pState->streamDimension.height) < (cropTop + cropHeight))
        {
            // Distance from the last line of the input to the last line of the crop window
            temporaryValue       = m_pState->scalerOutput.dimension.height - (cropTop + cropHeight);
            // Move the coordinate furthuer to have the crop window in the FOV output
            offCenterCoordinateY = offCenterCoordinateY + (centerCoordinateY - temporaryValue);
        }

        pCrop->firstLine = Utils::FloorUINT32(2, offCenterCoordinateY);

        if ((pCrop->firstLine > m_pState->scalerOutput.dimension.height) ||
            (pCrop->firstLine + m_pState->streamDimension.height >
            m_pState->scalerOutput.dimension.height))
        {
            pCrop->firstLine = Utils::FloorUINT32(2, centerCoordinateY);
        }

        pCrop->lastLine  = pCrop->firstLine + m_pState->streamDimension.height - 1;

        // Beyond IFE scaling range IPE cropping needed for zoom
        m_pState->cropInfo.top    = cropTop - pCrop->firstLine;
        m_pState->cropInfo.height = cropHeight;
    }

    // Adjusting IPE cropping needed for zoom to be centered
    UINT32 widthPixel = m_pState->cropInfo.width  + (m_pState->cropInfo.left * 2);
    UINT32 heightLine = m_pState->cropInfo.height + (m_pState->cropInfo.top  * 2);

    if (widthPixel > m_pState->streamDimension.width)
    {
        if (m_pState->streamDimension.width > static_cast<UINT32>(m_pState->cropInfo.width))
        {
            m_pState->cropInfo.left = (m_pState->streamDimension.width - m_pState->cropInfo.width) / 2;
        }
        else
        {
            m_pState->cropInfo.width = m_pState->streamDimension.width - (m_pState->cropInfo.left * 2);
        }
    }

    if (heightLine > m_pState->streamDimension.height)
    {
        if (m_pState->streamDimension.height > static_cast<UINT32>(m_pState->cropInfo.height))
        {
            m_pState->cropInfo.top = (m_pState->streamDimension.height - m_pState->cropInfo.height) / 2;
        }
        else
        {
            m_pState->cropInfo.height = m_pState->streamDimension.height - (m_pState->cropInfo.top * 2);
        }
    }

    if (cropType == CropTypeCentered)
    {
        if (((m_previousAppliedCrop[m_output].firstPixel   == pCrop->firstPixel) &&
            (m_previousAppliedCrop[m_output].lastPixel   ==  pCrop->lastPixel)) ||
            ((m_previousAppliedCrop[m_output].firstLine    ==  pCrop->firstLine) &&
            (m_previousAppliedCrop[m_output].lastLine  ==  pCrop->lastLine)))
        {
            CAMX_LOG_INFO(CamxLogGroupISP, "Same crop window - cropWindow[%d %d %d %d] path=%d",
               pCrop->firstPixel,
               pCrop->lastPixel,
               pCrop->firstLine,
               pCrop->lastLine,
               m_output);

            pCrop->firstPixel = m_previousAppliedCrop[m_output].firstPixel;
            pCrop->lastPixel  = m_previousAppliedCrop[m_output].lastPixel;
            pCrop->firstLine  = m_previousAppliedCrop[m_output].firstLine;
            pCrop->lastLine   = m_previousAppliedCrop[m_output].lastLine;
        }
        else
        {
            m_previousAppliedCrop[m_output].firstPixel = pCrop->firstPixel;
            m_previousAppliedCrop[m_output].lastPixel  = pCrop->lastPixel;
            m_previousAppliedCrop[m_output].firstLine  = pCrop->firstLine;
            m_previousAppliedCrop[m_output].lastLine   = pCrop->lastLine;
        }
    }

    UpdateAppliedCrop(pCrop);

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Path m_output=%d, Luma Crop window [%d, %d, %d, %d]",
                     m_output,
                     pCrop->firstPixel,
                     pCrop->lastPixel,
                     pCrop->firstLine,
                     pCrop->lastLine);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::CalculateCropInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECrop10Titan17x::CalculateCropInfo(
    CropInfo*   pLumaCrop,
    CropInfo*   pChromaCrop,
    CropType    cropType,
    Format      pixelFormat)
{
    FLOAT      horizontalSubSampleFactor = 1.0f;
    FLOAT      verticalSubsampleFactor   = 1.0f;
    CamxResult result                    = CamxResultSuccess;

    CAMX_ASSERT(NULL != pLumaCrop);
    CAMX_ASSERT(NULL != pChromaCrop);

    if (TRUE == m_pInputData->pStripeConfig->overwriteStripes)
    {
        result = UpdateAppliedCrop(pLumaCrop);
    }
    else
    {
        // Core calculation of crop baased on FOV
        CalculateCropCoordinates(pLumaCrop, cropType);
    }

    result = GetChromaSubsampleFactor(&horizontalSubSampleFactor, &verticalSubsampleFactor, pixelFormat);

    CAMX_ASSERT_MESSAGE(0 != horizontalSubSampleFactor, "Invalid horizontalSubSampleFactor");
    CAMX_ASSERT_MESSAGE(0 != horizontalSubSampleFactor, "Invalid verticalSubsampleFactor");

    if (CamxResultSuccess == result)
    {
        if (FALSE == m_pInputData->pStripeConfig->overwriteStripes)
        {
            pChromaCrop->firstPixel = static_cast<UINT32>(pLumaCrop->firstPixel / horizontalSubSampleFactor);
            pChromaCrop->lastPixel = pChromaCrop->firstPixel +
                (static_cast<UINT32>(m_pState->streamDimension.width / horizontalSubSampleFactor)) - 1;
        }
        pChromaCrop->firstLine = static_cast<UINT32>(pLumaCrop->firstLine / verticalSubsampleFactor);
        pChromaCrop->lastLine  = pChromaCrop->firstLine +
            (static_cast<UINT32>(m_pState->streamDimension.height / verticalSubsampleFactor)) - 1;
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECrop10Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result      = CamxResultSuccess;

    ISPInputData*       pInputData  = static_cast<ISPInputData*>(pInput);
    m_pInputData                    = pInputData;
    CropOutputData*     pOutputData = static_cast<CropOutputData*>(pOutput);
    m_output                        = pOutputData->ifeOutputPath;
    m_modulePath                    = pOutputData->modulePath;
    m_pState                        = pOutputData->pCropState;

    CropInfo            lumaCrop    = {0, 0, 0, 0};
    CropInfo            chromaCrop  = {0, 0, 0, 0};

    if (PixelRawOutput == m_output)
    {
        lumaCrop.firstPixel = pInputData->pStripeConfig->HALCrop[PixelRawOutput].left;
        lumaCrop.lastPixel  = pInputData->pStripeConfig->HALCrop[PixelRawOutput].left  +
                              pInputData->pStripeConfig->HALCrop[PixelRawOutput].width - 1;
        lumaCrop.firstLine  = 0;
        lumaCrop.lastLine   = pInputData->pStripeConfig->HALCrop[PixelRawOutput].height - 1;
    }

    if (TRUE == pInputData->pStripeConfig->overwriteStripes)
    {
        if (((CropTypeFromLeft == pInputData->pStripeConfig->cropType) ||
            (CropTypeFromRight == pInputData->pStripeConfig->cropType)))
        {
            const UINT modulePathIndex = static_cast<UINT>(m_modulePath);

            // Get the MNDS output height
            UINT32 currentModuleMNDSOutputHeight = pInputData->pStripeConfig->stateDS[modulePathIndex].MNDSOutput.height;

            if (FullOutput == m_output)
            {
                ISPInternalData* pFrameLevel = pInputData->pStripeConfig->pFrameLevelData->pFrameData;
                lumaCrop.firstPixel   = pInputData->pStripeConfig->pStripeOutput->outCropVideoFullLuma.firstOut;
                lumaCrop.lastPixel    = pInputData->pStripeConfig->pStripeOutput->outCropVideoFullLuma.lastOut;
                lumaCrop.firstLine    = pFrameLevel->fullOutCrop.firstLine;
                lumaCrop.lastLine     = pFrameLevel->fullOutCrop.lastLine;
                chromaCrop.firstPixel = pInputData->pStripeConfig->pStripeOutput->outCropVideoFullChroma.firstOut * 2;
                chromaCrop.lastPixel  = pInputData->pStripeConfig->pStripeOutput->outCropVideoFullChroma.lastOut * 2 + 1;
                chromaCrop.firstLine  = pFrameLevel->fullOutCrop.firstLine / 2;
                chromaCrop.lastLine   = (pFrameLevel->fullOutCrop.lastLine + 1) / 2;
            }
            else if (FDOutput == m_output)
            {
                ISPInternalData*    pFrameLevel = pInputData->pStripeConfig->pFrameLevelData->pFrameData;

                lumaCrop.firstPixel   = pInputData->pStripeConfig->pStripeOutput->outCropFDLuma.firstOut;
                lumaCrop.lastPixel    = pInputData->pStripeConfig->pStripeOutput->outCropFDLuma.lastOut;
                lumaCrop.firstLine    = pFrameLevel->fdOutCrop.firstLine;
                lumaCrop.lastLine     = pFrameLevel->fdOutCrop.lastLine;

                chromaCrop.firstPixel = pInputData->pStripeConfig->pStripeOutput->outCropFDChroma.firstOut * 2;
                chromaCrop.lastPixel  = pInputData->pStripeConfig->pStripeOutput->outCropFDChroma.lastOut * 2 + 1;
                chromaCrop.firstLine  = pFrameLevel->fullOutCrop.firstLine / 2;
                chromaCrop.lastLine   = (pFrameLevel->fullOutCrop.lastLine + 1) / 2;
            }
            else if (DS4Output == m_output)
            {
                lumaCrop.firstPixel   = pInputData->pStripeConfig->pStripeOutput->outCropVideoDS4Luma.firstOut;
                lumaCrop.lastPixel    = pInputData->pStripeConfig->pStripeOutput->outCropVideoDS4Luma.lastOut;
                chromaCrop.firstPixel = pInputData->pStripeConfig->pStripeOutput->outCropVideoDS4Chroma.firstOut * 2;
                chromaCrop.lastPixel  = pInputData->pStripeConfig->pStripeOutput->outCropVideoDS4Chroma.lastOut * 2 + 1;

                lumaCrop.firstLine    = 0;
                lumaCrop.lastLine     = Utils::AlignGeneric32(currentModuleMNDSOutputHeight, (DS4Factor * 2)) / DS4Factor - 1;
                chromaCrop.firstLine  = 0;
                chromaCrop.lastLine   = ((lumaCrop.lastLine + 1) / 2)  - 1;
            }
            else if (DS16Output == m_output)
            {
                lumaCrop.firstPixel   = pInputData->pStripeConfig->pStripeOutput->outCropVideoDS16Luma.firstOut;
                lumaCrop.lastPixel    = pInputData->pStripeConfig->pStripeOutput->outCropVideoDS16Luma.lastOut;
                chromaCrop.firstPixel = pInputData->pStripeConfig->pStripeOutput->outCropVideoDS16Chroma.firstOut * 2;
                chromaCrop.lastPixel  = pInputData->pStripeConfig->pStripeOutput->outCropVideoDS16Chroma.lastOut * 2 + 1;

                lumaCrop.firstLine    = 0;
                lumaCrop.lastLine     = Utils::AlignGeneric32(currentModuleMNDSOutputHeight, (DS4Factor * 2)) / DS4Factor - 1;
                chromaCrop.firstLine  = 0;
                chromaCrop.lastLine   = ((lumaCrop.lastLine + 1) / 2)  - 1;
            }
            else if (PixelRawOutput == m_output)
            {
                // already configured
            }
            else if (DisplayFullOutput == m_output)
            {
                ISPInternalData*    pFrameLevel = pInputData->pStripeConfig->pFrameLevelData->pFrameData;
                lumaCrop.firstPixel   = pInputData->pStripeConfig->pStripeOutput->outCropDispFullLuma.firstOut;
                lumaCrop.lastPixel    = pInputData->pStripeConfig->pStripeOutput->outCropDispFullLuma.lastOut;
                lumaCrop.firstLine    = pFrameLevel->dispOutCrop.firstLine;
                lumaCrop.lastLine     = pFrameLevel->dispOutCrop.lastLine;
                chromaCrop.firstPixel = pInputData->pStripeConfig->pStripeOutput->outCropDispFullChroma.firstOut * 2;
                chromaCrop.lastPixel  = pInputData->pStripeConfig->pStripeOutput->outCropDispFullChroma.lastOut * 2 + 1;
                chromaCrop.firstLine  = pFrameLevel->dispOutCrop.firstLine / 2;
                chromaCrop.lastLine   = (pFrameLevel->dispOutCrop.lastLine + 1) / 2;
            }
            else if (DisplayDS4Output == m_output)
            {
                lumaCrop.firstPixel   = pInputData->pStripeConfig->pStripeOutput->outCropDispDS4Luma.firstOut;
                lumaCrop.lastPixel    = pInputData->pStripeConfig->pStripeOutput->outCropDispDS4Luma.lastOut;
                chromaCrop.firstPixel = pInputData->pStripeConfig->pStripeOutput->outCropDispDS4Chroma.firstOut * 2;
                chromaCrop.lastPixel  = pInputData->pStripeConfig->pStripeOutput->outCropDispDS4Chroma.lastOut * 2 + 1;

                lumaCrop.firstLine    = 0;
                lumaCrop.lastLine     = Utils::AlignGeneric32(currentModuleMNDSOutputHeight, (DS4Factor * 2)) / DS4Factor - 1;
                chromaCrop.firstLine  = 0;
                chromaCrop.lastLine   = ((lumaCrop.lastLine + 1) / 2)  - 1;

            }
            else if (DisplayDS16Output == m_output)
            {
                lumaCrop.firstPixel   = pInputData->pStripeConfig->pStripeOutput->outCropDispDS16Luma.firstOut;
                lumaCrop.lastPixel    = pInputData->pStripeConfig->pStripeOutput->outCropDispDS16Luma.lastOut;
                chromaCrop.firstPixel = pInputData->pStripeConfig->pStripeOutput->outCropDispDS16Chroma.firstOut * 2;
                chromaCrop.lastPixel  = pInputData->pStripeConfig->pStripeOutput->outCropDispDS16Chroma.lastOut * 2 + 1;

                lumaCrop.firstLine    = 0;
                lumaCrop.lastLine     = Utils::AlignGeneric32(currentModuleMNDSOutputHeight, (DS4Factor * 2)) / DS4Factor - 1;
                chromaCrop.firstLine  = 0;
                chromaCrop.lastLine   = ((lumaCrop.lastLine + 1) / 2)  - 1;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid IFE output path!");
            }
        }
    }

    if (PixelRawOutput != m_output)
    {
        result = ModifyCropWindow(pInputData->pStripeConfig->cropType);
    }

    if ((CamxResultSuccess == result) &&
        (PixelRawOutput != m_output))
    {
        result = CalculateCropInfo(&lumaCrop,
                                   &chromaCrop,
                                   pInputData->pStripeConfig->cropType,
                                   pInputData->HALData.format[m_output]);
    }

    if (FullOutput == m_output)
    {
        pInputData->pCalculatedData->fullOutCrop = lumaCrop;
    }

    // FD ouput path configuration
    if ((CamxResultSuccess == result) &&
        (FDOutput == m_output))
    {
        pInputData->pCalculatedData->fdOutCrop = lumaCrop;
        ConfigureFDLumaRegisters(&lumaCrop);
        ConfigureFDChromaRegisters(&chromaCrop);
    }

    // Full output path configuration
    if ((CamxResultSuccess == result) &&
        FullOutput == m_output)
    {
        ConfigureFullLumaRegisters(&lumaCrop);
        ConfigureFullChromaRegisters(&chromaCrop);
    }

    if ((CamxResultSuccess == result) &&
        (DS4Output == m_output))
    {
        ConfigureDS4LumaRegisters(&lumaCrop);
        ConfigureDS4ChromaRegisters(&chromaCrop);
    }

    if ((CamxResultSuccess == result) &&
        (DS16Output == m_output))
    {
        ConfigureDS16LumaRegisters(&lumaCrop);
        ConfigureDS16ChromaRegisters(&chromaCrop);
    }

    if ((CamxResultSuccess == result) &&
        (PixelRawOutput == m_output))
    {
        ConfigurePixelRawRegisters(&lumaCrop);
    }

    if ((CamxResultSuccess == result) &&
        DisplayFullOutput == m_output)
    {
        pInputData->pCalculatedData->dispOutCrop = lumaCrop;
        ConfigureDisplayFullLumaRegisters(&lumaCrop);
        ConfigureDisplayFullChromaRegisters(&chromaCrop);
    }

    if ((CamxResultSuccess == result) &&
        (DisplayDS4Output == m_output))
    {
        ConfigureDisplayDS4LumaRegisters(&lumaCrop);
        ConfigureDisplayDS4ChromaRegisters(&chromaCrop);
    }
    if ((CamxResultSuccess == result) &&
        (DisplayDS16Output == m_output))
    {
        ConfigureDisplayDS16LumaRegisters(&lumaCrop);
        ConfigureDisplayDS16ChromaRegisters(&chromaCrop);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECrop10Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult    result      = CamxResultSuccess;
    ISPInputData* pInputData  = static_cast<ISPInputData*>(pInput);

    switch (m_output)
    {
        case FDOutput:
            pInputData->pCalculatedData->metadata.cropInfo.FDPath               = m_pState->cropInfo;
            pInputData->pCalculatedData->metadata.appliedCropInfo.FDPath        = m_pState->appliedCropInfo;
            break;

        case FullOutput:
            pInputData->pCalculatedData->metadata.cropInfo.fullPath               = m_pState->cropInfo;
            pInputData->pCalculatedData->metadata.appliedCropInfo.fullPath        = m_pState->appliedCropInfo;


            if (CropTypeCentered == pInputData->pStripeConfig->cropType)
            {
                // In single IFE, use post-crops coordinates for DS4 precrop
                pInputData->pCalculatedData->preCropInfo.YCrop.firstLine    =
                    m_regCmd.fullLuma.lineConfig.bitfields.FIRST_LINE;
                pInputData->pCalculatedData->preCropInfo.YCrop.lastLine     =
                    m_regCmd.fullLuma.lineConfig.bitfields.LAST_LINE;
                pInputData->pCalculatedData->preCropInfo.YCrop.firstPixel   =
                    m_regCmd.fullLuma.pixelConfig.bitfields.FIRST_PIXEL;
                pInputData->pCalculatedData->preCropInfo.YCrop.lastPixel    =
                    m_regCmd.fullLuma.pixelConfig.bitfields.LAST_PIXEL;
                pInputData->pCalculatedData->preCropInfo.CrCrop.firstLine   =
                    m_regCmd.fullChroma.lineConfig.bitfields.FIRST_LINE;
                pInputData->pCalculatedData->preCropInfo.CrCrop.lastLine    =
                    m_regCmd.fullChroma.lineConfig.bitfields.LAST_LINE;
                pInputData->pCalculatedData->preCropInfo.CrCrop.firstPixel  =
                    m_regCmd.fullChroma.pixelConfig.bitfields.FIRST_PIXEL;
                pInputData->pCalculatedData->preCropInfo.CrCrop.lastPixel   =
                    m_regCmd.fullChroma.pixelConfig.bitfields.LAST_PIXEL;
            }
            else
            {
                // When not overwriting, we need to calculate pre-crop properly to respect DS4,
                // but for now, we know we will overwrite so only do rudimentary values.
                pInputData->pCalculatedData->preCropInfo.YCrop.firstLine    =
                    m_regCmd.fullLuma.lineConfig.bitfields.FIRST_LINE;
                pInputData->pCalculatedData->preCropInfo.YCrop.lastLine     =
                    m_regCmd.fullLuma.lineConfig.bitfields.LAST_LINE;
                pInputData->pCalculatedData->preCropInfo.YCrop.firstPixel   = 0;
                pInputData->pCalculatedData->preCropInfo.YCrop.lastPixel    = m_pState->scalerOutput.dimension.width - 1;
                pInputData->pCalculatedData->preCropInfo.CrCrop.firstLine   =
                    m_regCmd.fullChroma.lineConfig.bitfields.FIRST_LINE;
                pInputData->pCalculatedData->preCropInfo.CrCrop.lastLine    =
                    m_regCmd.fullChroma.lineConfig.bitfields.LAST_LINE;
                pInputData->pCalculatedData->preCropInfo.CrCrop.firstPixel  = 0;
                pInputData->pCalculatedData->preCropInfo.CrCrop.lastPixel   = m_pState->scalerOutput.dimension.width - 1;

                // In dual IFE, overlap is cropped in post-crop so we don't use it for pre-crop yet.
                if (TRUE == pInputData->pStripeConfig->overwriteStripes)
                {
                    pInputData->pCalculatedData->preCropInfo.YCrop.firstPixel =
                        pInputData->pStripeConfig->pStripeOutput->preDS4CropVideoDS4Luma.firstOut;
                    pInputData->pCalculatedData->preCropInfo.YCrop.lastPixel  =
                        pInputData->pStripeConfig->pStripeOutput->preDS4CropVideoDS4Luma.lastOut;

                    pInputData->pCalculatedData->preCropInfo.CrCrop.firstPixel =
                        pInputData->pStripeConfig->pStripeOutput->preDS4CropVideoDS4Chroma.firstOut * 2;
                    pInputData->pCalculatedData->preCropInfo.CrCrop.lastPixel  =
                        pInputData->pStripeConfig->pStripeOutput->preDS4CropVideoDS4Chroma.lastOut * 2 + 1;
                }
            }
            break;

        case DS4Output:
            pInputData->pCalculatedData->metadata.cropInfo.DS4Path               = m_pState->cropInfo;
            pInputData->pCalculatedData->metadata.appliedCropInfo.DS4Path        = m_pState->appliedCropInfo;

            // When not overwriting, we need to calculate pre-crop properly to respect DS4,
            // but for now, we know we will overwrite so only do rudimentary values.
            pInputData->pCalculatedData->preCropInfoDS16.YCrop.firstLine    =
                m_regCmd.DS4Luma.lineConfig.bitfields.FIRST_LINE;
            pInputData->pCalculatedData->preCropInfoDS16.YCrop.lastLine     =
                m_regCmd.DS4Luma.lineConfig.bitfields.LAST_LINE;
            pInputData->pCalculatedData->preCropInfoDS16.YCrop.firstPixel   = 0;
            pInputData->pCalculatedData->preCropInfoDS16.YCrop.lastPixel    = m_pState->scalerOutput.dimension.width - 1;
            pInputData->pCalculatedData->preCropInfoDS16.CrCrop.firstLine   =
                m_regCmd.DS4Chroma.lineConfig.bitfields.FIRST_LINE;
            pInputData->pCalculatedData->preCropInfoDS16.CrCrop.lastLine    =
                m_regCmd.DS4Chroma.lineConfig.bitfields.LAST_LINE;
            pInputData->pCalculatedData->preCropInfoDS16.CrCrop.firstPixel  = 0;
            pInputData->pCalculatedData->preCropInfoDS16.CrCrop.lastPixel   = m_pState->scalerOutput.dimension.width - 1;

            if (TRUE == pInputData->pStripeConfig->overwriteStripes)
            {
                pInputData->pCalculatedData->preCropInfoDS16.YCrop.firstPixel =
                    pInputData->pStripeConfig->pStripeOutput->preDS4CropVideoDS16Luma.firstOut;
                pInputData->pCalculatedData->preCropInfoDS16.YCrop.lastPixel  =
                    pInputData->pStripeConfig->pStripeOutput->preDS4CropVideoDS16Luma.lastOut;

                pInputData->pCalculatedData->preCropInfoDS16.CrCrop.firstPixel =
                    pInputData->pStripeConfig->pStripeOutput->preDS4CropVideoDS16Chroma.firstOut * 2;
                pInputData->pCalculatedData->preCropInfoDS16.CrCrop.lastPixel  =
                    pInputData->pStripeConfig->pStripeOutput->preDS4CropVideoDS16Chroma.lastOut * 2 + 1;
            }
            break;

        case DS16Output:
            pInputData->pCalculatedData->metadata.cropInfo.DS16Path               = m_pState->cropInfo;
            pInputData->pCalculatedData->metadata.appliedCropInfo.DS16Path        = m_pState->appliedCropInfo;
            break;

        case DisplayFullOutput:
            pInputData->pCalculatedData->metadata.cropInfo.displayFullPath               = m_pState->cropInfo;
            pInputData->pCalculatedData->metadata.appliedCropInfo.displayFullPath        = m_pState->appliedCropInfo;

            if (CropTypeCentered == pInputData->pStripeConfig->cropType)
            {
                // In single IFE, use post-crops coordinates for DS4 precrop
                pInputData->pCalculatedData->dispPreCropInfo.YCrop.firstLine    =
                    m_regCmd.displayFullLuma.lineConfig.bitfields.FIRST_LINE;
                pInputData->pCalculatedData->dispPreCropInfo.YCrop.lastLine     =
                    m_regCmd.displayFullLuma.lineConfig.bitfields.LAST_LINE;
                pInputData->pCalculatedData->dispPreCropInfo.YCrop.firstPixel   =
                    m_regCmd.displayFullLuma.pixelConfig.bitfields.FIRST_PIXEL;
                pInputData->pCalculatedData->dispPreCropInfo.YCrop.lastPixel    =
                    m_regCmd.displayFullLuma.pixelConfig.bitfields.LAST_PIXEL;
                pInputData->pCalculatedData->dispPreCropInfo.CrCrop.firstLine   =
                    m_regCmd.displayFullChroma.lineConfig.bitfields.FIRST_LINE;
                pInputData->pCalculatedData->dispPreCropInfo.CrCrop.lastLine    =
                    m_regCmd.displayFullChroma.lineConfig.bitfields.LAST_LINE;
                pInputData->pCalculatedData->dispPreCropInfo.CrCrop.firstPixel  =
                    m_regCmd.displayFullChroma.pixelConfig.bitfields.FIRST_PIXEL;
                pInputData->pCalculatedData->dispPreCropInfo.CrCrop.lastPixel   =
                    m_regCmd.displayFullChroma.pixelConfig.bitfields.LAST_PIXEL;
            }
            else
            {
                // When not overwriting, we need to calculate pre-crop properly to respect DS4,
                // but for now, we know we will overwrite so only do rudimentary values.
                pInputData->pCalculatedData->dispPreCropInfo.YCrop.firstLine    =
                    m_regCmd.displayFullLuma.lineConfig.bitfields.FIRST_LINE;
                pInputData->pCalculatedData->dispPreCropInfo.YCrop.lastLine     =
                    m_regCmd.displayFullLuma.lineConfig.bitfields.LAST_LINE;
                pInputData->pCalculatedData->dispPreCropInfo.YCrop.firstPixel   = 0;
                pInputData->pCalculatedData->dispPreCropInfo.YCrop.lastPixel    = m_pState->scalerOutput.dimension.width - 1;
                pInputData->pCalculatedData->dispPreCropInfo.CrCrop.firstLine   =
                    m_regCmd.displayFullChroma.lineConfig.bitfields.FIRST_LINE;
                pInputData->pCalculatedData->dispPreCropInfo.CrCrop.lastLine    =
                    m_regCmd.displayFullChroma.lineConfig.bitfields.LAST_LINE;
                pInputData->pCalculatedData->dispPreCropInfo.CrCrop.firstPixel  = 0;
                pInputData->pCalculatedData->dispPreCropInfo.CrCrop.lastPixel   = m_pState->scalerOutput.dimension.width - 1;

                // In dual IFE, overlap is cropped in post-crop so we don't use it for pre-crop yet.
                if (TRUE == pInputData->pStripeConfig->overwriteStripes)
                {
                    pInputData->pCalculatedData->dispPreCropInfo.YCrop.firstPixel =
                        pInputData->pStripeConfig->pStripeOutput->preDS4CropDispDS4Luma.firstOut;
                    pInputData->pCalculatedData->dispPreCropInfo.YCrop.lastPixel  =
                        pInputData->pStripeConfig->pStripeOutput->preDS4CropDispDS4Luma.lastOut;

                    pInputData->pCalculatedData->dispPreCropInfo.CrCrop.firstPixel =
                        pInputData->pStripeConfig->pStripeOutput->preDS4CropDispDS4Chroma.firstOut * 2;
                    pInputData->pCalculatedData->dispPreCropInfo.CrCrop.lastPixel  =
                        pInputData->pStripeConfig->pStripeOutput->preDS4CropDispDS4Chroma.lastOut * 2 + 1;
                }
            }
            break;

        case DisplayDS4Output:
            pInputData->pCalculatedData->metadata.cropInfo.displayDS4Path               = m_pState->cropInfo;
            pInputData->pCalculatedData->metadata.appliedCropInfo.displayDS4Path        = m_pState->appliedCropInfo;

            // When not overwriting, we need to calculate pre-crop properly to respect DS4,
            // but for now, we know we will overwrite so only do rudimentary values.
            pInputData->pCalculatedData->dispPreCropInfoDS16.YCrop.firstLine    =
                m_regCmd.displayDS4Luma.lineConfig.bitfields.FIRST_LINE;
            pInputData->pCalculatedData->dispPreCropInfoDS16.YCrop.lastLine     =
                m_regCmd.displayDS4Luma.lineConfig.bitfields.LAST_LINE;
            pInputData->pCalculatedData->dispPreCropInfoDS16.YCrop.firstPixel   = 0;
            pInputData->pCalculatedData->dispPreCropInfoDS16.YCrop.lastPixel    = m_pState->scalerOutput.dimension.width - 1;
            pInputData->pCalculatedData->dispPreCropInfoDS16.CrCrop.firstLine   =
                m_regCmd.displayDS4Chroma.lineConfig.bitfields.FIRST_LINE;
            pInputData->pCalculatedData->dispPreCropInfoDS16.CrCrop.lastLine    =
                m_regCmd.displayDS4Chroma.lineConfig.bitfields.LAST_LINE;
            pInputData->pCalculatedData->dispPreCropInfoDS16.CrCrop.firstPixel  = 0;
            pInputData->pCalculatedData->dispPreCropInfoDS16.CrCrop.lastPixel   = m_pState->scalerOutput.dimension.width - 1;

            if (TRUE == pInputData->pStripeConfig->overwriteStripes)
            {
                pInputData->pCalculatedData->dispPreCropInfoDS16.YCrop.firstPixel =
                    pInputData->pStripeConfig->pStripeOutput->preDS4CropDispDS16Luma.firstOut;
                pInputData->pCalculatedData->dispPreCropInfoDS16.YCrop.lastPixel  =
                    pInputData->pStripeConfig->pStripeOutput->preDS4CropDispDS16Luma.lastOut;

                pInputData->pCalculatedData->dispPreCropInfoDS16.CrCrop.firstPixel =
                    pInputData->pStripeConfig->pStripeOutput->preDS4CropDispDS16Chroma.firstOut * 2;
                pInputData->pCalculatedData->dispPreCropInfoDS16.CrCrop.lastPixel  =
                    pInputData->pStripeConfig->pStripeOutput->preDS4CropDispDS16Chroma.lastOut * 2 + 1;
            }
            break;

        case DisplayDS16Output:
            pInputData->pCalculatedData->metadata.cropInfo.displayDS16Path               = m_pState->cropInfo;
            pInputData->pCalculatedData->metadata.appliedCropInfo.displayDS16Path        = m_pState->appliedCropInfo;
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid format %d", m_output);
            break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFECrop10Titan17x::CopyRegCmd(
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
// IFECrop10Titan17x::~IFECrop10Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECrop10Titan17x::~IFECrop10Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop10Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECrop10Titan17x::DumpRegConfig()
{
    if (FDOutput == m_output)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "FD Luma Crop Config[%d, %d, %d, %d]",
                         m_regCmd.FDLuma.pixelConfig.bitfields.FIRST_PIXEL,
                         m_regCmd.FDLuma.lineConfig.bitfields.FIRST_LINE,
                         m_regCmd.FDLuma.pixelConfig.bitfields.LAST_PIXEL,
                         m_regCmd.FDLuma.lineConfig.bitfields.LAST_LINE);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "FD Chroma Crop Config[%d, %d, %d, %d]",
                         m_regCmd.FDChroma.pixelConfig.bitfields.FIRST_PIXEL,
                         m_regCmd.FDChroma.lineConfig.bitfields.FIRST_LINE,
                         m_regCmd.FDChroma.pixelConfig.bitfields.LAST_PIXEL,
                         m_regCmd.FDChroma.lineConfig.bitfields.LAST_LINE);
    }

    if (FullOutput == m_output)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Full Luma Crop Config[%d, %d, %d, %d]",
                         m_regCmd.fullLuma.pixelConfig.bitfields.FIRST_PIXEL,
                         m_regCmd.fullLuma.lineConfig.bitfields.FIRST_LINE,
                         m_regCmd.fullLuma.pixelConfig.bitfields.LAST_PIXEL,
                         m_regCmd.fullLuma.lineConfig.bitfields.LAST_LINE);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Full Chroma Crop Config[%d, %d, %d, %d]",
                         m_regCmd.fullChroma.pixelConfig.bitfields.FIRST_PIXEL,
                         m_regCmd.fullChroma.lineConfig.bitfields.FIRST_LINE,
                         m_regCmd.fullChroma.pixelConfig.bitfields.LAST_PIXEL,
                         m_regCmd.fullChroma.lineConfig.bitfields.LAST_LINE);
    }

    if (DS4Output == m_output)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS4 Luma Crop Config[%d, %d, %d, %d]",
                         m_regCmd.DS4Luma.pixelConfig.bitfields.FIRST_PIXEL,
                         m_regCmd.DS4Luma.lineConfig.bitfields.FIRST_LINE,
                         m_regCmd.DS4Luma.pixelConfig.bitfields.LAST_PIXEL,
                         m_regCmd.DS4Luma.lineConfig.bitfields.LAST_LINE);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS4 Chroma Crop Config[%d, %d, %d, %d]",
                         m_regCmd.DS4Chroma.pixelConfig.bitfields.FIRST_PIXEL,
                         m_regCmd.DS4Chroma.lineConfig.bitfields.FIRST_LINE,
                         m_regCmd.DS4Chroma.pixelConfig.bitfields.LAST_PIXEL,
                         m_regCmd.DS4Chroma.lineConfig.bitfields.LAST_LINE);

    }

    if (DS16Output == m_output)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Luma Crop Config[%d, %d, %d, %d]",
                         m_regCmd.DS16Luma.pixelConfig.bitfields.FIRST_PIXEL,
                         m_regCmd.DS16Luma.lineConfig.bitfields.FIRST_LINE,
                         m_regCmd.DS16Luma.pixelConfig.bitfields.LAST_PIXEL,
                         m_regCmd.DS16Luma.lineConfig.bitfields.LAST_LINE);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Chroma Crop Config[%d, %d, %d, %d]",
                         m_regCmd.DS16Chroma.pixelConfig.bitfields.FIRST_PIXEL,
                         m_regCmd.DS16Chroma.lineConfig.bitfields.FIRST_LINE,
                         m_regCmd.DS16Chroma.pixelConfig.bitfields.LAST_PIXEL,
                         m_regCmd.DS16Chroma.lineConfig.bitfields.LAST_LINE);
    }

    if (PixelRawOutput == m_output)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pixel raw Crop Config[%d, %d, %d, %d]",
                         m_regCmd.pixelRaw.pixelConfig.bitfields.FIRST_PIXEL,
                         m_regCmd.pixelRaw.lineConfig.bitfields.FIRST_LINE,
                         m_regCmd.pixelRaw.pixelConfig.bitfields.LAST_PIXEL,
                         m_regCmd.pixelRaw.lineConfig.bitfields.LAST_LINE);
    }

    if (DisplayFullOutput == m_output)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display Luma Crop Config   [%d, %d, %d, %d]",
            m_regCmd.displayFullLuma.pixelConfig.bitfields.FIRST_PIXEL,
            m_regCmd.displayFullLuma.lineConfig.bitfields.FIRST_LINE,
            m_regCmd.displayFullLuma.pixelConfig.bitfields.LAST_PIXEL,
            m_regCmd.displayFullLuma.lineConfig.bitfields.LAST_LINE);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display Chroma Crop Config [%d, %d, %d, %d]",
            m_regCmd.displayFullChroma.pixelConfig.bitfields.FIRST_PIXEL,
            m_regCmd.displayFullChroma.lineConfig.bitfields.FIRST_LINE,
            m_regCmd.displayFullChroma.pixelConfig.bitfields.LAST_PIXEL,
            m_regCmd.displayFullChroma.lineConfig.bitfields.LAST_LINE);
    }

    if (DisplayDS4Output == m_output)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display DS4 Luma Crop Config   [%d, %d, %d, %d]",
            m_regCmd.displayDS4Luma.pixelConfig.bitfields.FIRST_PIXEL,
            m_regCmd.displayDS4Luma.lineConfig.bitfields.FIRST_LINE,
            m_regCmd.displayDS4Luma.pixelConfig.bitfields.LAST_PIXEL,
            m_regCmd.displayDS4Luma.lineConfig.bitfields.LAST_LINE);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display DS4 Chroma Crop Config [%d, %d, %d, %d]",
            m_regCmd.displayDS4Chroma.pixelConfig.bitfields.FIRST_PIXEL,
            m_regCmd.displayDS4Chroma.lineConfig.bitfields.FIRST_LINE,
            m_regCmd.displayDS4Chroma.pixelConfig.bitfields.LAST_PIXEL,
            m_regCmd.displayDS4Chroma.lineConfig.bitfields.LAST_LINE);

    }

    if (DisplayDS16Output == m_output)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display DS16 Luma Crop Config   [%d, %d, %d, %d]",
            m_regCmd.displayDS16Luma.pixelConfig.bitfields.FIRST_PIXEL,
            m_regCmd.displayDS16Luma.lineConfig.bitfields.FIRST_LINE,
            m_regCmd.displayDS16Luma.pixelConfig.bitfields.LAST_PIXEL,
            m_regCmd.displayDS16Luma.lineConfig.bitfields.LAST_LINE);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display DS16 Chroma Crop Config [%d, %d, %d, %d]",
            m_regCmd.displayDS16Chroma.pixelConfig.bitfields.FIRST_PIXEL,
            m_regCmd.displayDS16Chroma.lineConfig.bitfields.FIRST_LINE,
            m_regCmd.displayDS16Chroma.pixelConfig.bitfields.LAST_PIXEL,
            m_regCmd.displayDS16Chroma.lineConfig.bitfields.LAST_LINE);
    }
}

CAMX_NAMESPACE_END
