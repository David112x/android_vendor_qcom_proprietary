////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecrop10titan17x.h
/// @brief IFE CAMIF register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFECROP10TITAN17X_H
#define CAMXIFECROP10TITAN17X_H

#include "titan170_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispiqmodule.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED
struct IFECrop10FullLumaReg
{
    IFE_IFE_0_VFE_FULL_OUT_Y_CROP_LINE_CFG     lineConfig;     ///< Full output path Luma Crop line config
    IFE_IFE_0_VFE_FULL_OUT_Y_CROP_PIXEL_CFG    pixelConfig;    ///< Full output path Luma Crop pixel config
} CAMX_PACKED;

struct IFECrop10FullChromaReg
{
    IFE_IFE_0_VFE_FULL_OUT_C_CROP_LINE_CFG     lineConfig;     ///< Full output path chroma Crop line config
    IFE_IFE_0_VFE_FULL_OUT_C_CROP_PIXEL_CFG    pixelConfig;    ///< Full output path chroma Crop pixel config
} CAMX_PACKED;

struct IFECrop10FDLumaReg
{
    IFE_IFE_0_VFE_FD_OUT_Y_CROP_LINE_CFG       lineConfig;     ///< FD output path Luma Crop line config
    IFE_IFE_0_VFE_FD_OUT_Y_CROP_PIXEL_CFG      pixelConfig;    ///< FD output path Luma Crop pixel config
} CAMX_PACKED;

struct IFECrop10FDChromaReg
{
    IFE_IFE_0_VFE_FD_OUT_C_CROP_LINE_CFG     lineConfig;     ///< FD output path chroma Crop line config
    IFE_IFE_0_VFE_FD_OUT_C_CROP_PIXEL_CFG    pixelConfig;    ///< FD output path chroma Crop pixel config
} CAMX_PACKED;

struct IFECrop10DS4LumaReg
{
    IFE_IFE_0_VFE_DS4_Y_CROP_LINE_CFG     lineConfig;     ///< DS4ostcrop output path Luma Crop line config
    IFE_IFE_0_VFE_DS4_Y_CROP_PIXEL_CFG    pixelConfig;    ///< DS4ostcrop output path Luma Crop pixel config
} CAMX_PACKED;

struct IFECrop10DS4ChromaReg
{
    IFE_IFE_0_VFE_DS4_C_CROP_LINE_CFG     lineConfig;     ///< DS4postcrop output path chroma Crop line config
    IFE_IFE_0_VFE_DS4_C_CROP_PIXEL_CFG    pixelConfig;    ///< DS4postcrop output path chroma Crop pixel config
} CAMX_PACKED;

struct IFECrop10DS16LumaReg
{
    IFE_IFE_0_VFE_DS16_Y_CROP_LINE_CFG     lineConfig;     ///< DS16ostcrop output path Luma Crop line config
    IFE_IFE_0_VFE_DS16_Y_CROP_PIXEL_CFG    pixelConfig;    ///< DS16ostcrop output path Luma Crop pixel config
} CAMX_PACKED;

struct IFECrop10DS16ChromaReg
{
    IFE_IFE_0_VFE_DS16_C_CROP_LINE_CFG     lineConfig;     ///< DS16postcrop output path chroma Crop line config
    IFE_IFE_0_VFE_DS16_C_CROP_PIXEL_CFG    pixelConfig;    ///< DS16postcrop output path chroma Crop pixel config
} CAMX_PACKED;

struct IFECropPixelRawReg
{
    IFE_IFE_0_VFE_PIXEL_RAW_CROP_PIXEL_CFG    pixelConfig;    ///< pixel raw output path Crop pixel config
    IFE_IFE_0_VFE_PIXEL_RAW_CROP_LINE_CFG     lineConfig;     ///< pixel raw output path  Crop line config
}  CAMX_PACKED;

struct IFECrop10DisplayFullLumaReg
{
    IFE_IFE_0_VFE_DISP_OUT_Y_CROP_LINE_CFG     lineConfig;     ///< Full Display output path Luma Crop line config
    IFE_IFE_0_VFE_DISP_OUT_Y_CROP_PIXEL_CFG    pixelConfig;    ///< Full Display output path Luma Crop pixel config
} CAMX_PACKED;

struct IFECrop10DisplayFullChromaReg
{
    IFE_IFE_0_VFE_DISP_OUT_C_CROP_LINE_CFG     lineConfig;     ///< Full Display output path chroma Crop line config
    IFE_IFE_0_VFE_DISP_OUT_C_CROP_PIXEL_CFG    pixelConfig;    ///< Full Display output path chroma Crop pixel config
} CAMX_PACKED;

struct IFECrop10DisplayDS4LumaReg
{
    IFE_IFE_0_VFE_DISP_DS4_Y_CROP_LINE_CFG     lineConfig;     ///< Display DS4ostcrop output path Luma Crop line config
    IFE_IFE_0_VFE_DISP_DS4_Y_CROP_PIXEL_CFG    pixelConfig;    ///< Display DS4ostcrop output path Luma Crop pixel config
} CAMX_PACKED;

struct IFECrop10DisplayDS4ChromaReg
{
    IFE_IFE_0_VFE_DISP_DS4_C_CROP_LINE_CFG     lineConfig;     ///< Display DS4postcrop output path chroma Crop line config
    IFE_IFE_0_VFE_DISP_DS4_C_CROP_PIXEL_CFG    pixelConfig;    ///< Display DS4postcrop output path chroma Crop pixel config
} CAMX_PACKED;

struct IFECrop10DisplayDS16LumaReg
{
    IFE_IFE_0_VFE_DISP_DS16_Y_CROP_LINE_CFG     lineConfig;     ///< Display DS16ostcrop output path Luma Crop line config
    IFE_IFE_0_VFE_DISP_DS16_Y_CROP_PIXEL_CFG    pixelConfig;    ///< Display DS16ostcrop output path Luma Crop pixel config
} CAMX_PACKED;

struct IFECrop10DisplayDS16ChromaReg
{
    IFE_IFE_0_VFE_DISP_DS16_C_CROP_LINE_CFG     lineConfig;     ///< Display DS16postcrop output path chroma Crop line config
    IFE_IFE_0_VFE_DISP_DS16_C_CROP_PIXEL_CFG    pixelConfig;    ///< Display DS16postcrop output path chroma Crop pixel config
} CAMX_PACKED;

CAMX_END_PACKED

struct IFECrop10RegCmd
{
    IFECrop10FullLumaReg            fullLuma;           ///< Full video output path Luma config
    IFECrop10FullChromaReg          fullChroma;         ///< Full video output path chroma config
    IFECrop10FDLumaReg              FDLuma;             ///< FD output path Luma config
    IFECrop10FDChromaReg            FDChroma;           ///< FD output path chroma config
    IFECrop10DS4LumaReg             DS4Luma;            ///< DS4 output path Luma config
    IFECrop10DS4ChromaReg           DS4Chroma;          ///< DS4 output path chroma config
    IFECrop10DS16LumaReg            DS16Luma;           ///< DS16 output path Luma config
    IFECrop10DS16ChromaReg          DS16Chroma;         ///< DS16 output path chroma config
    IFECropPixelRawReg              pixelRaw;           ///< pixel raw output path config
    IFECrop10DisplayFullLumaReg     displayFullLuma;    ///< Full display output path Luma config
    IFECrop10DisplayFullChromaReg   displayFullChroma;  ///< Full display output path chroma config
    IFECrop10DisplayDS4LumaReg      displayDS4Luma;     ///< DS4 display output path Luma config
    IFECrop10DisplayDS4ChromaReg    displayDS4Chroma;   ///< DS4 display output path chroma config
    IFECrop10DisplayDS16LumaReg     displayDS16Luma;    ///< DS16 display output path Luma config
    IFECrop10DisplayDS16ChromaReg   displayDS16Chroma;  ///< DS16 display output path chroma config
};

static const FLOAT IFECropScalingThreshold = 1.05f;    ///< Threshold scale factor when IFE cannot crop frame further

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE CAMIF register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFECrop10Titan17x final : public ISPHWSetting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateCmdList
    ///
    /// @brief  Generate the Command List
    ///
    /// @param  pInputData       Pointer to the Inputdata
    /// @param  pDMIBufferOffset Pointer for DMI Buffer
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult CreateCmdList(
        VOID*   pInputData,
        UINT32* pDMIBufferOffset);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PackIQRegisterSetting
    ///
    /// @brief  Packing register setting based on calculated data
    ///
    /// @param  pInput       Pointer to the Input data to the module for calculation
    /// @param  pOutput      Pointer to the Output data to the module for DMI buffer
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PackIQRegisterSetting(
        VOID* pInput,
        VOID* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupRegisterSetting
    ///
    /// @brief  Setup register value based on CamX Input
    ///
    /// @param  pInput       Pointer to the Input data to the module for calculation
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult SetupRegisterSetting(
        VOID*  pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IFECrop10Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFECrop10Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFECrop10Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFECrop10Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyRegCmd
    ///
    /// @brief  Copy register settings to the input buffer
    ///
    /// @param  pData  Pointer to the Input data buffer
    ///
    /// @return Number of bytes copied
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 CopyRegCmd(
        VOID* pData);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateCropInfo
    ///
    /// @brief  Calculate Crop cordinates on scaler window
    ///
    /// @param  pLumaCrop       Pointer to Luma crop
    /// @param  pChromaCrop     Pointer to Chroma crop
    /// @param  cropType        Crop type
    /// @param  format          Path Pixel format
    ///
    /// @return CamxResult Success if all configuration done, else error code
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CalculateCropInfo(
        CropInfo*   pLumaCrop,
        CropInfo*   pChromaCrop,
        CropType    cropType,
        Format      format);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateCropCoordinates
    ///
    /// @brief  Configure Crop10 registers
    ///
    /// @param  pCrop       Pointer to the crop coordinates
    /// @param  cropType    Crop type
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CalculateCropCoordinates(
        CropInfo*   pCrop,
        CropType    cropType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateAppliedCrop
    ///
    /// @brief  Update Applied crop
    ///
    /// @param  pCrop       Pointer to the crop coordinates
    ///
    /// @return CamxResult Success if all configuration done, else error code
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateAppliedCrop(
        CropInfo*   pCrop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureFDLumaRegisters
    ///
    /// @brief  Configure Crop10 FD Luma registers
    ///
    /// @param  pCrop Pointer to First/last pixel and line coordinates on scaler window to be cropped
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureFDLumaRegisters(
        CropInfo* pCrop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureFDChromaRegisters
    ///
    /// @brief  Configure Crop10 FD Chroma registers
    ///
    /// @param  pCrop Pointer to First/last pixel and line coordinates on scaler window to be cropped
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureFDChromaRegisters(
        CropInfo* pCrop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureFullLumaRegisters
    ///
    /// @brief  Configure Crop10 Full Luma registers
    ///
    /// @param  pCrop Pointer to First/last pixel and line coordinates on scaler window to be cropped
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureFullLumaRegisters(
        CropInfo* pCrop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureFullChromaRegisters
    ///
    /// @brief  Configure Crop10 Full Chroma registers
    ///
    /// @param  pCrop Pointer to First/last pixel and line coordinates on scaler window to be cropped
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureFullChromaRegisters(
        CropInfo* pCrop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDisplayFullLumaRegisters
    ///
    /// @brief  Configure Crop10 Full Diplay Luma registers
    ///
    /// @param  pCrop Pointer to First/last pixel and line coordinates on scaler window to be cropped
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDisplayFullLumaRegisters(
        CropInfo* pCrop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDisplayFullChromaRegisters
    ///
    /// @brief  Configure Crop10 Full Display Chroma registers
    ///
    /// @param  pCrop Pointer to First/last pixel and line coordinates on scaler window to be cropped
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDisplayFullChromaRegisters(
        CropInfo* pCrop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDS4LumaRegisters
    ///
    /// @brief  Configure Crop10 DS4 Luma registers
    ///
    /// @param  pCrop Pointer to First/last pixel and line coordinates on scaler window to be cropped
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDS4LumaRegisters(
        CropInfo* pCrop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDS4ChromaRegisters
    ///
    /// @brief  Configure Crop10 DS4 Chroma registers
    ///
    /// @param  pCrop Pointer to First/last pixel and line coordinates on scaler window to be cropped
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDS4ChromaRegisters(
        CropInfo* pCrop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDS16LumaRegisters
    ///
    /// @brief  Configure Crop10 DS16 Luma registers
    ///
    /// @param  pCrop     Pointer to First/last pixel and line coordinates on scaler window to be cropped
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDS16LumaRegisters(
        CropInfo* pCrop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDS16ChromaRegisters
    ///
    /// @brief  Configure Crop10 DS16 Chroma registers
    ///
    /// @param  pCrop     Pointer to First/last pixel and line coordinates on scaler window to be cropped
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDS16ChromaRegisters(
        CropInfo* pCrop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigurePixelRawRegisters
    ///
    /// @brief  Configure Crop10 pixel raw registers
    ///
    /// @param  pCrop     Pointer to First/last pixel and line coordinates on scaler window to be cropped
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigurePixelRawRegisters(
        CropInfo* pCrop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDisplayDS4LumaRegisters
    ///
    /// @brief  Configure Crop10 DS4 Display Luma registers
    ///
    /// @param  pCrop Pointer to First/last pixel and line coordinates on scaler window to be cropped
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDisplayDS4LumaRegisters(
        CropInfo* pCrop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDisplayDS4ChromaRegisters
    ///
    /// @brief  Configure Crop10 DS4 Display Chroma registers
    ///
    /// @param  pCrop Pointer to First/last pixel and line coordinates on scaler window to be cropped
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDisplayDS4ChromaRegisters(
        CropInfo* pCrop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDisplayDS16LumaRegisters
    ///
    /// @brief  Configure Crop10 DS16 Display Luma registers
    ///
    /// @param  pCrop     Pointer to First/last pixel and line coordinates on scaler window to be cropped
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDisplayDS16LumaRegisters(
        CropInfo* pCrop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDisplayDS16ChromaRegisters
    ///
    /// @brief  Configure Crop10 DS16 Disp Chroma registers
    ///
    /// @param  pCrop     Pointer to First/last pixel and line coordinates on scaler window to be cropped
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDisplayDS16ChromaRegisters(
        CropInfo* pCrop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetChromaSubsampleFactor
    ///
    /// @brief  Get the subsample factor based on the pixel format
    ///
    /// @param  pHorizontalSubSampleFactor Pointer to horizontal chroma subsample factor
    /// @param  pVerticalSubsampleFactor   Pointer to Vertical chroma subsample factor
    /// @param  format                     Path Pixel format
    ///
    /// @return CamxResult Success if the format is handled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetChromaSubsampleFactor(
        FLOAT* pHorizontalSubSampleFactor,
        FLOAT* pVerticalSubsampleFactor,
        Format format);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ModifyCropWindow
    ///
    /// @brief  Modify the client crop window based on the aspect ratio of the output path
    ///
    /// @param  cropType    Crop type
    ///
    /// @return CamxResult Success if the crop window is valid
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ModifyCropWindow(
        CropType cropType);

    IFECrop10RegCmd     m_regCmd;       ///< Register List of this Module
    ISPInputData*       m_pInputData;   ///< Pointer to current input data
    CropState*          m_pState;       ///< State
    IFEPipelinePath     m_modulePath;   ///< IFE pipeline path for module
    UINT32              m_output;       ///< IFE piepline output

    CropInfo            m_previousAppliedCrop[MaxRoundClampPath];    ///< Last Applied crop info
    IFECrop10Titan17x(const IFECrop10Titan17x&)            = delete; ///< Disallow the copy constructor
    IFECrop10Titan17x& operator=(const IFECrop10Titan17x&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFECROP10TITAN17X_H
