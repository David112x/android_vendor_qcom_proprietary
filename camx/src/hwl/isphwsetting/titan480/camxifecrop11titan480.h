////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecrop11titan480.h
/// @brief IFE CROP register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFECROP11TITAN480_H
#define CAMXIFECROP11TITAN480_H

#include "titan480_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispiqmodule.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED
struct IFECrop11FullLumaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_VID_OUT_CROP_LINE_CFG  lineConfig;  ///< Full Luma Crop line cfg
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_VID_OUT_CROP_PIXEL_CFG pixelConfig; ///< Full Luma Crop pixel cfg
} CAMX_PACKED;

struct IFECrop11FullChromaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_VID_OUT_CROP_LINE_CFG  lineConfig;  ///< Full chroma Crop line cfg
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_VID_OUT_CROP_PIXEL_CFG pixelConfig; ///< Full chroma Crop pixel cfg
} CAMX_PACKED;

struct IFECrop11FDLumaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_FD_OUT_CROP_LINE_CFG  lineConfig;  ///< FD Luma Crop line config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_FD_OUT_CROP_PIXEL_CFG pixelConfig; ///< FD Luma Crop pixel config
} CAMX_PACKED;

struct IFECrop11FDChromaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_FD_OUT_CROP_LINE_CFG    lineConfig;  ///< FD chroma Crop line config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_FD_OUT_CROP_PIXEL_CFG   pixelConfig; ///< FD chroma Crop pixel config
} CAMX_PACKED;

struct IFECrop11DS4LumaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_Y_VID_OUT_CROP_LINE_CFG     lineConfig;  ///< DS4 Luma Crop line config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_Y_VID_OUT_CROP_PIXEL_CFG    pixelConfig; ///< DS4 Luma Crop pixel config
} CAMX_PACKED;

struct IFECrop11DS4ChromaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_C_VID_OUT_CROP_LINE_CFG     lineConfig;  ///< DS4 chroma Crop line config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_C_VID_OUT_CROP_PIXEL_CFG    pixelConfig; ///< DS4 chroma Crop pixel config
} CAMX_PACKED;

struct IFECrop11DS16LumaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_VID_DS16_OUT_CROP_LINE_CFG  lineConfig;  ///< DS16 Y Crop line cfg
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_VID_DS16_OUT_CROP_PIXEL_CFG pixelConfig; ///< DS16 Y Crop pixel cfg
} CAMX_PACKED;

struct IFECrop11DS16ChromaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_VID_DS16_OUT_CROP_LINE_CFG  lineConfig;  ///< DS16 C Crop line cfg
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_VID_DS16_OUT_CROP_PIXEL_CFG pixelConfig; ///< DS16 C Crop pixel cfg
} CAMX_PACKED;

struct IFECropPixelRawReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_PIXEL_RAW_OUT_CROP_LINE_CFG  lineConfig; ///< pixel raw output path Crop pixel config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_PIXEL_RAW_OUT_CROP_PIXEL_CFG pixelConfig;  ///< pixel raw output path  Crop line config
}  CAMX_PACKED;

struct IFECrop11DisplayFullLumaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_DISP_OUT_CROP_LINE_CFG  lineConfig;  ///< Full Disp Y Crop line cfg
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_DISP_OUT_CROP_PIXEL_CFG pixelConfig; ///< Full Disp Y Crop pixel cfg
} CAMX_PACKED;

struct IFECrop11DisplayFullChromaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_DISP_OUT_CROP_LINE_CFG  lineConfig;  ///< Full Disp C Crop line cfg
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_DISP_OUT_CROP_PIXEL_CFG pixelConfig; ///< Full Disp C Crop pixel cfg
} CAMX_PACKED;

struct IFECrop11DisplayDS4LumaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS4_OUT_CROP_LINE_CFG  lineConfig;  ///< DS4 Y Crop line cfg
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS4_OUT_CROP_PIXEL_CFG pixelConfig; ///< DS4 Y Crop pixel cfg
} CAMX_PACKED;

struct IFECrop11DisplayDS4ChromaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS4_OUT_CROP_LINE_CFG  lineConfig;  ///< DS4 C Crop line cfg
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS4_OUT_CROP_PIXEL_CFG pixelConfig; ///< DS4 C Crop pixel cfg
} CAMX_PACKED;

struct IFECrop11DisplayDS16LumaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS16_OUT_CROP_LINE_CFG  lineConfig;  ///< DS16 Y Crop line
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS16_OUT_CROP_PIXEL_CFG pixelConfig; ///< DS16 Y Crop pixel
} CAMX_PACKED;

struct IFECrop11DisplayDS16ChromaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS16_OUT_CROP_LINE_CFG  lineConfig;  ///< DS16 C Crop line
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS16_OUT_CROP_PIXEL_CFG pixelConfig; ///< DS16 C Crop pixel
} CAMX_PACKED;

CAMX_END_PACKED

struct IFECrop11RegCmd
{
    IFECrop11FullLumaReg            fullLuma;           ///< Full video output path Luma config
    IFECrop11FullChromaReg          fullChroma;         ///< Full video output path chroma config
    IFECrop11FDLumaReg              FDLuma;             ///< FD output path Luma config
    IFECrop11FDChromaReg            FDChroma;           ///< FD output path chroma config
    IFECrop11DS4LumaReg             DS4Luma;            ///< DS4 output path Luma config
    IFECrop11DS4ChromaReg           DS4Chroma;          ///< DS4 output path chroma config
    IFECrop11DS16LumaReg            DS16Luma;           ///< DS16 output path Luma config
    IFECrop11DS16ChromaReg          DS16Chroma;         ///< DS16 output path chroma config
    IFECropPixelRawReg              pixelRaw;           ///< pixel raw output path config
    IFECrop11DisplayFullLumaReg     displayFullLuma;    ///< Full display output path Luma config
    IFECrop11DisplayFullChromaReg   displayFullChroma;  ///< Full display output path chroma config
    IFECrop11DisplayDS4LumaReg      displayDS4Luma;     ///< DS4 display output path Luma config
    IFECrop11DisplayDS4ChromaReg    displayDS4Chroma;   ///< DS4 display output path chroma config
    IFECrop11DisplayDS16LumaReg     displayDS16Luma;    ///< DS16 display output path Luma config
    IFECrop11DisplayDS16ChromaReg   displayDS16Chroma;  ///< DS16 display output path chroma config
};

static const FLOAT IFECropScalingThreshold = 1.05f;    ///< Threshold scale factor when IFE cannot crop frame further

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE CAMIF register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFECrop11Titan480 final : public ISPHWSetting
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
    /// ~IFECrop11Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFECrop11Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFECrop11Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFECrop11Titan480();

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

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize the class with module path
    ///
    /// @param  modulePath    m_modulePath
    /// @param  output        output
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID Initialize(
        IFEPipelinePath     modulePath,
        UINT32              output)
    {
        m_modulePath = modulePath;
        m_output     = output;
    }

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
    /// @brief  Configure Crop11 registers
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
    /// @brief  Configure Crop11 FD Luma registers
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
    /// @brief  Configure Crop11 FD Chroma registers
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
    /// @brief  Configure Crop11 Full Luma registers
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
    /// @brief  Configure Crop11 Full Chroma registers
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
    /// @brief  Configure Crop11 Full Diplay Luma registers
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
    /// @brief  Configure Crop11 Full Display Chroma registers
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
    /// @brief  Configure Crop11 DS4 Luma registers
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
    /// @brief  Configure Crop11 DS4 Chroma registers
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
    /// @brief  Configure Crop11 DS16 Luma registers
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
    /// @brief  Configure Crop11 DS16 Chroma registers
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
    /// @brief  Configure Crop11 pixel raw registers
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
    /// @brief  Configure Crop11 DS4 Display Luma registers
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
    /// @brief  Configure Crop11 DS4 Display Chroma registers
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
    /// @brief  Configure Crop11 DS16 Display Luma registers
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
    /// @brief  Configure Crop11 DS16 Disp Chroma registers
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

    IFECrop11RegCmd     m_regCmd;       ///< Register List of this Module
    ISPInputData*       m_pInputData;   ///< Pointer to current input data
    CropState*          m_pState;                     ///< State
    IFEPipelinePath     m_modulePath;                 ///< IFE pipeline path for module
    UINT32              m_output;                     ///< IPE piepline output
    CropInfo            m_previousAppliedCrop[MaxRoundClampPath]; ///< Last Applied crop info

    IFECrop11Titan480(const IFECrop11Titan480&)            = delete; ///< Disallow the copy constructor
    IFECrop11Titan480& operator=(const IFECrop11Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFECROP11TITAN480_H
