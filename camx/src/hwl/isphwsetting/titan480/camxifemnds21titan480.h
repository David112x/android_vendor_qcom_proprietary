////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifemnds21titan480.h
/// @brief IFE MNDS21 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEMNDS21TITAN480_H
#define CAMXIFEMNDS21TITAN480_H

#include "titan480_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispiqmodule.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

struct IFEMNDS21FDLumaReg
{
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_FD_OUT_MODULE_CFG                         module;           ///< Module CFG
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_FD_OUT_DOWNSCALE_MN_Y_CFG                 config;           ///< Y CFG
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_FD_OUT_DOWNSCALE_MN_Y_IMAGE_SIZE_CFG      imageSize;        ///< Y image size CFG
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_FD_OUT_DOWNSCALE_MN_Y_H_CFG               horizontalPhase;  ///< Y H Phase
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_FD_OUT_DOWNSCALE_MN_Y_H_PHASE_CFG         horizontalStripe; ///< Y H Stripe
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_FD_OUT_DOWNSCALE_MN_Y_V_CFG               verticalPhase;    ///< Y V Phase
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_FD_OUT_DOWNSCALE_MN_Y_V_PHASE_CFG         verticalStripe;   ///< Y V Stripe
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_FD_OUT_DOWNSCALE_MN_Y_CROP_LINE_CFG       cropLine;         ///< Y Crop Line
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_FD_OUT_DOWNSCALE_MN_Y_CROP_PIXEL_CFG      cropPixel;        ///< Y Crop Pixel
} CAMX_PACKED;

struct IFEMNDS21FDChromaReg
{
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_FD_OUT_MODULE_CFG                         module;           ///< Module CFG
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_FD_OUT_DOWNSCALE_MN_C_CFG                 config;           ///< C CFG
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_FD_OUT_DOWNSCALE_MN_C_IMAGE_SIZE_CFG      imageSize;        ///< C image size CFG
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_FD_OUT_DOWNSCALE_MN_C_H_CFG               horizontalPhase;  ///< C H Phase
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_FD_OUT_DOWNSCALE_MN_C_H_PHASE_CFG         horizontalStripe; ///< C H Stripe
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_FD_OUT_DOWNSCALE_MN_C_V_CFG               verticalPhase;    ///< C V Phase
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_FD_OUT_DOWNSCALE_MN_C_V_PHASE_CFG         verticalStripe;   ///< C V Stripe
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_FD_OUT_DOWNSCALE_MN_C_CROP_LINE_CFG       cropLine;         ///< C Crop Line
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_FD_OUT_DOWNSCALE_MN_C_CROP_PIXEL_CFG      cropPixel;        ///< C Crop Pixel
} CAMX_PACKED;

struct IFEMNDS21DispLumaReg
{
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_DISP_OUT_MODULE_CFG                         module;             ///< Module CFG
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_DISP_OUT_DOWNSCALE_MN_Y_CFG                 config;             ///< Y CFG
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_DISP_OUT_DOWNSCALE_MN_Y_IMAGE_SIZE_CFG      imageSize;          ///< Y image size CFG
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_DISP_OUT_DOWNSCALE_MN_Y_H_CFG               horizontalPhase;    ///< Y H Phase
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_DISP_OUT_DOWNSCALE_MN_Y_H_PHASE_CFG         horizontalStripe;   ///< Y H Stripe
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_DISP_OUT_DOWNSCALE_MN_Y_V_CFG               verticalPhase;      ///< Y V Phase
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_DISP_OUT_DOWNSCALE_MN_Y_V_PHASE_CFG         verticalStripe;     ///< Y V Stripe
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_DISP_OUT_DOWNSCALE_MN_Y_CROP_LINE_CFG       cropLine;           ///< Y Crop Line
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_DISP_OUT_DOWNSCALE_MN_Y_CROP_PIXEL_CFG      cropPixel;          ///< Y Crop Pixel
} CAMX_PACKED;

struct IFEMNDS21DispChromaReg
{
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_DISP_OUT_MODULE_CFG                         module;             ///< Module CFG
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_DISP_OUT_DOWNSCALE_MN_C_CFG                 config;             ///< C CFG
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_DISP_OUT_DOWNSCALE_MN_C_IMAGE_SIZE_CFG      imageSize;          ///< C image size CFG
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_DISP_OUT_DOWNSCALE_MN_C_H_CFG               horizontalPhase;    ///< C H Phase
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_DISP_OUT_DOWNSCALE_MN_C_H_PHASE_CFG         horizontalStripe;   ///< C H Stripe
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_DISP_OUT_DOWNSCALE_MN_C_V_CFG               verticalPhase;      ///< C V Phase
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_DISP_OUT_DOWNSCALE_MN_C_V_PHASE_CFG         verticalStripe;     ///< C V Stripe
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_DISP_OUT_DOWNSCALE_MN_C_CROP_LINE_CFG       cropLine;           ///< C Crop Line
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_DISP_OUT_DOWNSCALE_MN_C_CROP_PIXEL_CFG      cropPixel;          ///< C Crop Pixel
} CAMX_PACKED;

struct IFEMNDS21VideoLumaReg
{
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_VID_OUT_MODULE_CFG                         module;              ///< Module CFG
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_VID_OUT_DOWNSCALE_MN_Y_CFG                 config;              ///< Y CFG
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_VID_OUT_DOWNSCALE_MN_Y_IMAGE_SIZE_CFG      imageSize;           ///< Y image size CFG
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_VID_OUT_DOWNSCALE_MN_Y_H_CFG               horizontalPhase;     ///< Y H Phase
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_VID_OUT_DOWNSCALE_MN_Y_H_PHASE_CFG         horizontalStripe;    ///< Y H Stripe
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_VID_OUT_DOWNSCALE_MN_Y_V_CFG               verticalPhase;       ///< Y V Phase
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_VID_OUT_DOWNSCALE_MN_Y_V_PHASE_CFG         verticalStripe;      ///< Y V Stripe
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_VID_OUT_DOWNSCALE_MN_Y_CROP_LINE_CFG       cropLine;            ///< Y Crop Line
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_VID_OUT_DOWNSCALE_MN_Y_CROP_PIXEL_CFG      cropPixel;           ///< Y Crop Pixel
} CAMX_PACKED;

struct IFEMNDS21VideoChromaReg
{
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_VID_OUT_MODULE_CFG                         module;              ///< Module CFG
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_VID_OUT_DOWNSCALE_MN_C_CFG                 config;              ///< C CFG
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_VID_OUT_DOWNSCALE_MN_C_IMAGE_SIZE_CFG      imageSize;           ///< C image size CFG
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_VID_OUT_DOWNSCALE_MN_C_H_CFG               horizontalPhase;     ///< C H Phase
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_VID_OUT_DOWNSCALE_MN_C_H_PHASE_CFG         horizontalStripe;    ///< C H Stripe
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_VID_OUT_DOWNSCALE_MN_C_V_CFG               verticalPhase;       ///< C V Phase
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_VID_OUT_DOWNSCALE_MN_C_V_PHASE_CFG         verticalStripe;      ///< C V Stripe
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_VID_OUT_DOWNSCALE_MN_C_CROP_LINE_CFG       cropLine;            ///< C Crop Line
    IFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_VID_OUT_DOWNSCALE_MN_C_CROP_PIXEL_CFG      cropPixel;           ///< C Crop Pixel
} CAMX_PACKED;

CAMX_END_PACKED

struct IFEMNDS21RegCmd
{
    IFEMNDS21FDLumaReg         FDLuma;         ///< FD out Luma register config
    IFEMNDS21FDChromaReg       FDChroma;       ///< FD out Chroma register config
    IFEMNDS21DispLumaReg       dispLuma;       ///< Display out Luma register config
    IFEMNDS21DispChromaReg     dispChroma;     ///< Display out Chroma register config
    IFEMNDS21VideoLumaReg      videoLuma;      ///< Video(Full) out Luma register config
    IFEMNDS21VideoChromaReg    videoChroma;    ///< Video(Full) out Chroma register config
};

static const UINT32 MNDS21ScaleRatioLimit    = 105;    ///< Scaling ratio limit beyond which artificats are seen
static const UINT32 MNDS21InterpolationShift = 14;     ///< Interpolation shift value
static const UINT32 MNDS21MaxScaleFactor     = 64;     ///< Max downscaling factor considering chroma needs to be

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE MNDS21 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEMNDS21Titan480 final : public ISPHWSetting
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
    /// @brief  Calculate register settings based on CAMX Input
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
    /// @param  pInput  Pointer to the Input data to the module for calculation
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult SetupRegisterSetting(
        VOID*  pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IFEMNDS21Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEMNDS21Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEMNDS21Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEMNDS21Titan480();

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
    /// ConfigureFDLumaRegisters
    ///
    /// @brief  Configure scaler FD Luma output path registers
    ///
    /// @param  horizontalInterpolationResolution horizontal interpolation resolution
    /// @param  verticalInterpolationResolution   vertical interpolation resolution
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureFDLumaRegisters(
        UINT32 horizontalInterpolationResolution,
        UINT32 verticalInterpolationResolution);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureFullLumaRegisters
    ///
    /// @brief  Configure scaler Full Luma output path registers
    ///
    /// @param  horizontalInterpolationResolution horizontal interpolation resolution
    /// @param  verticalInterpolationResolution   vertical interpolation resolution
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureFullLumaRegisters(
        UINT32 horizontalInterpolationResolution,
        UINT32 verticalInterpolationResolution);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDispLumaRegisters
    ///
    /// @brief  Configure scaler Display Luma output path registers
    ///
    /// @param  horizontalInterpolationResolution horizontal interpolation resolution
    /// @param  verticalInterpolationResolution   vertical interpolation resolution
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDispLumaRegisters(
        UINT32 horizontalInterpolationResolution,
        UINT32 verticalInterpolationResolution);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureFDChromaRegisters
    ///
    /// @brief  Configure scaler FD Chroma output path registers
    ///
    /// @param  horizontalInterpolationResolution horizontal interpolation resolution
    /// @param  verticalInterpolationResolution   vertical interpolation resolution
    /// @param  horizontalSubSampleFactor         horizontal subsampling factor
    /// @param  verticalSubsampleFactor           vertical subsampling factor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureFDChromaRegisters(
       UINT32 horizontalInterpolationResolution,
       UINT32 verticalInterpolationResolution,
       FLOAT  horizontalSubSampleFactor,
       FLOAT  verticalSubsampleFactor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureFullChromaRegisters
    ///
    /// @brief  Configure scaler Full Chroma output path registers
    ///
    /// @param  horizontalInterpolationResolution horizontal interpolation resolution
    /// @param  verticalInterpolationResolution   vertical interpolation resolution
    /// @param  horizontalSubSampleFactor         horizontal subsampling factor
    /// @param  verticalSubsampleFactor           vertical subsampling factor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureFullChromaRegisters(
        UINT32 horizontalInterpolationResolution,
        UINT32 verticalInterpolationResolution,
        FLOAT  horizontalSubSampleFactor,
        FLOAT  verticalSubsampleFactor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDispChromaRegisters
    ///
    /// @brief  Configure scaler Display Chroma output path registers
    ///
    /// @param  horizontalInterpolationResolution horizontal interpolation resolution
    /// @param  verticalInterpolationResolution   vertical interpolation resolution
    /// @param  horizontalSubSampleFactor         horizontal subsampling factor
    /// @param  verticalSubsampleFactor           vertical subsampling factor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDispChromaRegisters(
        UINT32 horizontalInterpolationResolution,
        UINT32 verticalInterpolationResolution,
        FLOAT  horizontalSubSampleFactor,
        FLOAT  verticalSubsampleFactor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateInterpolationResolution
    ///
    /// @brief  Calculate the interpolation resolution for scaling
    ///
    /// @param  scaleFactorHorizontal              horizontal scale factor
    /// @param  scaleFactorVertical                vertical scale factor
    /// @param  pHorizontalInterpolationResolution Pointer to horizontal interpolation resolution
    /// @param  pVerticalInterpolationResolution   Pointer to vertical interpolation resolution
    ///
    /// @return CamxResult Invalid input Error code for invalid scaling or format, else success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CalculateInterpolationResolution(
        UINT32  scaleFactorHorizontal,
        UINT32  scaleFactorVertical,
        UINT32* pHorizontalInterpolationResolution,
        UINT32* pVerticalInterpolationResolution);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetChromaSubsampleFactor
    ///
    /// @brief  Get the subsample factor based on the pixel format
    ///
    /// @param  pHorizontalSubSampleFactor Pointer to horizontal chroma subsample factor
    /// @param  pVerticalSubsampleFactor   Pointer to Vertical chroma subsample factor
    ///
    /// @return CamxResult success if the format is handled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetChromaSubsampleFactor(
        FLOAT* pHorizontalSubSampleFactor,
        FLOAT* pVerticalSubsampleFactor);

    IFEMNDS21RegCmd    m_regCmd;        ///< Register List of this Module
    MNDSState*         m_pState;        ///< State
    Format             m_pixelFormat;   ///< Output pixel format
    UINT32             m_output;        ///< IPE piepline output
    ISPInputData*      m_pInputData;    ///< Pointer to current input data

    IFEMNDS21Titan480(const IFEMNDS21Titan480&)            = delete; ///< Disallow the copy constructor
    IFEMNDS21Titan480& operator=(const IFEMNDS21Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEMNDS21TITAN480_H
