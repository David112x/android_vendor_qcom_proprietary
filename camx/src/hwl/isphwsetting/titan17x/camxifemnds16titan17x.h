////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifemnds16titan17x.h
/// @brief IFE MNDS16 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEMNDS16TITAN17X_H
#define CAMXIFEMNDS16TITAN17X_H

#include "titan170_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispiqmodule.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

struct IFEMNDS16FDLumaReg
{
    IFE_IFE_0_VFE_SCALE_FD_Y_CFG                    config;               ///< FD out Luma Channel config
    IFE_IFE_0_VFE_SCALE_FD_Y_H_IMAGE_SIZE_CFG       horizontalSize;       ///< FD out Luma horizontal image size config
    IFE_IFE_0_VFE_SCALE_FD_Y_H_PHASE_CFG            horizontalPhase;      ///< FD out Luma horizontal phase config
    IFE_IFE_0_VFE_SCALE_FD_Y_H_STRIPE_CFG_0         horizontalStripe0;    ///< FD out Luma horizontal stripe config
    IFE_IFE_0_VFE_SCALE_FD_Y_H_STRIPE_CFG_1         horizontalStripe1;    ///< FD out Luma horizontal stripe config
    IFE_IFE_0_VFE_SCALE_FD_Y_H_PAD_CFG              horizontalPadding;    ///< FD out Luma horizontal padding config
    IFE_IFE_0_VFE_SCALE_FD_Y_V_IMAGE_SIZE_CFG       verticalSize;         ///< FD out Luma vertical image size config
    IFE_IFE_0_VFE_SCALE_FD_Y_V_PHASE_CFG            verticalPhase;        ///< FD out Luma vertical phase config
    IFE_IFE_0_VFE_SCALE_FD_Y_V_STRIPE_CFG_0         verticalStripe0;      ///< FD out Luma vertical stripe config
    IFE_IFE_0_VFE_SCALE_FD_Y_V_STRIPE_CFG_1         verticalStripe1;      ///< FD out Luma vertical stripe config
    IFE_IFE_0_VFE_SCALE_FD_Y_V_PAD_CFG              verticalPadding;      ///< FD out Luma vertical padding config
} CAMX_PACKED;

struct IFEMNDS16FDChromaReg
{
    IFE_IFE_0_VFE_SCALE_FD_CBCR_CFG                 config;               ///< FD out Chroma Channel config
    IFE_IFE_0_VFE_SCALE_FD_CBCR_H_IMAGE_SIZE_CFG    horizontalSize;       ///< FD out Chroma horizontal image size config
    IFE_IFE_0_VFE_SCALE_FD_CBCR_H_PHASE_CFG         horizontalPhase;      ///< FD out Chroma horizontal phase config
    IFE_IFE_0_VFE_SCALE_FD_CBCR_H_STRIPE_CFG_0      horizontalStripe0;    ///< FD out Chroma horizontal stripe config
    IFE_IFE_0_VFE_SCALE_FD_CBCR_H_STRIPE_CFG_1      horizontalStripe1;    ///< FD out Chroma horizontal stripe config
    IFE_IFE_0_VFE_SCALE_FD_CBCR_H_PAD_CFG           horizontalPadding;    ///< FD out Chroma horizontal padding config
    IFE_IFE_0_VFE_SCALE_FD_CBCR_V_IMAGE_SIZE_CFG    verticalSize;         ///< FD out Chroma vertical image size config
    IFE_IFE_0_VFE_SCALE_FD_CBCR_V_PHASE_CFG         verticalPhase;        ///< FD out Chroma vertical phase config
    IFE_IFE_0_VFE_SCALE_FD_CBCR_V_STRIPE_CFG_0      verticalStripe0;      ///< FD out Chroma vertical stripe config
    IFE_IFE_0_VFE_SCALE_FD_CBCR_V_STRIPE_CFG_1      verticalStripe1;      ///< FD out Chroma vertical stripe config
    IFE_IFE_0_VFE_SCALE_FD_CBCR_V_PAD_CFG           verticalPadding;      ///< FD out Chroma vertical padding config
} CAMX_PACKED;

struct IFEMNDS16VideoLumaReg
{
    IFE_IFE_0_VFE_SCALE_VID_Y_CFG                   config;               ///< Video(Full) out Luma Channel config
    IFE_IFE_0_VFE_SCALE_VID_Y_H_IMAGE_SIZE_CFG      horizontalSize;       ///< Video(Full) out Luma horizontal image size config
    IFE_IFE_0_VFE_SCALE_VID_Y_H_PHASE_CFG           horizontalPhase;      ///< Video(Full) out Luma horizontal phase config
    IFE_IFE_0_VFE_SCALE_VID_Y_H_STRIPE_CFG_0        horizontalStripe0;    ///< Video(Full) out Luma horizontal stripe config
    IFE_IFE_0_VFE_SCALE_VID_Y_H_STRIPE_CFG_1        horizontalStripe1;    ///< Video(Full) out Luma horizontal stripe config
    IFE_IFE_0_VFE_SCALE_VID_Y_H_PAD_CFG             horizontalPadding;    ///< Video(Full) out Luma horizontal padding config
    IFE_IFE_0_VFE_SCALE_VID_Y_V_IMAGE_SIZE_CFG      verticalSize;         ///< Video(Full) out Luma vertical image size config
    IFE_IFE_0_VFE_SCALE_VID_Y_V_PHASE_CFG           verticalPhase;        ///< Video(Full) out Luma vertical phase config
    IFE_IFE_0_VFE_SCALE_VID_Y_V_STRIPE_CFG_0        verticalStripe0;      ///< Video(Full) out Luma vertical stripe config
    IFE_IFE_0_VFE_SCALE_VID_Y_V_STRIPE_CFG_1        verticalStripe1;      ///< Video(Full) out Luma vertical stripe config
    IFE_IFE_0_VFE_SCALE_VID_Y_V_PAD_CFG             verticalPadding;      ///< Video(Full) out Luma vertical padding config
} CAMX_PACKED;

struct IFEMNDS16VideoChromaReg
{
    IFE_IFE_0_VFE_SCALE_VID_CBCR_CFG                config;              ///< Video(Full) out Chroma Channel config
    IFE_IFE_0_VFE_SCALE_VID_CBCR_H_IMAGE_SIZE_CFG   horizontalSize;      ///< Video(Full) out Chroma horizontal image size
                                                                         /// config
    IFE_IFE_0_VFE_SCALE_VID_CBCR_H_PHASE_CFG        horizontalPhase;     ///< Video(Full) out Chroma horizontal phase config
    IFE_IFE_0_VFE_SCALE_VID_CBCR_H_STRIPE_CFG_0     horizontalStripe0;   ///< Video(Full) out Chroma horizontal stripe config
    IFE_IFE_0_VFE_SCALE_VID_CBCR_H_STRIPE_CFG_1     horizontalStripe1;   ///< Video(Full) out Chroma horizontal stripe config
    IFE_IFE_0_VFE_SCALE_VID_CBCR_H_PAD_CFG          horizontalPadding;   ///< Video(Full) out Chroma horizontal padding config
    IFE_IFE_0_VFE_SCALE_VID_CBCR_V_IMAGE_SIZE_CFG   verticalSize;        ///< Video(Full) out Chroma vertical image size config
    IFE_IFE_0_VFE_SCALE_VID_CBCR_V_PHASE_CFG        verticalPhase;       ///< Video(Full) out Chroma vertical phase config
    IFE_IFE_0_VFE_SCALE_VID_CBCR_V_STRIPE_CFG_0     verticalStripe0;     ///< Video(Full) out Chroma vertical stripe config
    IFE_IFE_0_VFE_SCALE_VID_CBCR_V_STRIPE_CFG_1     verticalStripe1;     ///< Video(Full) out Chroma vertical stripe config
    IFE_IFE_0_VFE_SCALE_VID_CBCR_V_PAD_CFG          verticalPadding;     ///< Video(Full) out Chroma vertical padding config
} CAMX_PACKED;


struct IFEMNDS16DisplayLumaReg
{
    IFE_IFE_0_VFE_SCALE_DISP_Y_CFG                   config;            ///< Display(Full) out Luma Channel config
    IFE_IFE_0_VFE_SCALE_DISP_Y_H_IMAGE_SIZE_CFG      horizontalSize;    ///< Display(Full) out Luma horizontal image size config
    IFE_IFE_0_VFE_SCALE_DISP_Y_H_PHASE_CFG           horizontalPhase;   ///< Display(Full) out Luma horizontal phase config
    IFE_IFE_0_VFE_SCALE_DISP_Y_H_STRIPE_CFG_0        horizontalStripe0; ///< Display(Full) out Luma horizontal stripe config
    IFE_IFE_0_VFE_SCALE_DISP_Y_H_STRIPE_CFG_1        horizontalStripe1; ///< Display(Full) out Luma horizontal stripe config
    IFE_IFE_0_VFE_SCALE_DISP_Y_H_PAD_CFG             horizontalPadding; ///< Display(Full) out Luma horizontal padding config
    IFE_IFE_0_VFE_SCALE_DISP_Y_V_IMAGE_SIZE_CFG      verticalSize;      ///< Display(Full) out Luma vertical image size config
    IFE_IFE_0_VFE_SCALE_DISP_Y_V_PHASE_CFG           verticalPhase;     ///< Display(Full) out Luma vertical phase config
    IFE_IFE_0_VFE_SCALE_DISP_Y_V_STRIPE_CFG_0        verticalStripe0;   ///< Display(Full) out Luma vertical stripe config
    IFE_IFE_0_VFE_SCALE_DISP_Y_V_STRIPE_CFG_1        verticalStripe1;   ///< Display(Full) out Luma vertical stripe config
    IFE_IFE_0_VFE_SCALE_DISP_Y_V_PAD_CFG             verticalPadding;   ///< Display(Full) out Luma vertical padding config
} CAMX_PACKED;

struct IFEMNDS16DisplayChromaReg
{
    IFE_IFE_0_VFE_SCALE_DISP_CBCR_CFG                config;            ///< Display(Full) out Chroma Channel config
    IFE_IFE_0_VFE_SCALE_DISP_CBCR_H_IMAGE_SIZE_CFG   horizontalSize;    ///< Display(Full) out Chroma horizontal image size
                                                                        /// config
    IFE_IFE_0_VFE_SCALE_DISP_CBCR_H_PHASE_CFG        horizontalPhase;   ///< Display(Full) out Chroma horizontal phase config
    IFE_IFE_0_VFE_SCALE_DISP_CBCR_H_STRIPE_CFG_0     horizontalStripe0; ///< Display(Full) out Chroma horizontal stripe config
    IFE_IFE_0_VFE_SCALE_DISP_CBCR_H_STRIPE_CFG_1     horizontalStripe1; ///< Display(Full) out Chroma horizontal stripe config
    IFE_IFE_0_VFE_SCALE_DISP_CBCR_H_PAD_CFG          horizontalPadding; ///< Display(Full) out Chroma horizontal padding config
    IFE_IFE_0_VFE_SCALE_DISP_CBCR_V_IMAGE_SIZE_CFG   verticalSize;      ///< Display(Full) out Chroma vertical image size config
    IFE_IFE_0_VFE_SCALE_DISP_CBCR_V_PHASE_CFG        verticalPhase;     ///< Display(Full) out Chroma vertical phase config
    IFE_IFE_0_VFE_SCALE_DISP_CBCR_V_STRIPE_CFG_0     verticalStripe0;   ///< Display(Full) out Chroma vertical stripe config
    IFE_IFE_0_VFE_SCALE_DISP_CBCR_V_STRIPE_CFG_1     verticalStripe1;   ///< Display(Full) out Chroma vertical stripe config
    IFE_IFE_0_VFE_SCALE_DISP_CBCR_V_PAD_CFG          verticalPadding;   ///< Display(Full) out Chroma vertical padding config
} CAMX_PACKED;

CAMX_END_PACKED

struct IFEMNDS16RegCmd
{
    IFEMNDS16FDLumaReg          FDLuma;         ///< FD out Luma register config
    IFEMNDS16FDChromaReg        FDChroma;       ///< FD out Chroma register config
    IFEMNDS16VideoLumaReg       videoLuma;      ///< Video(Full) out Luma register config
    IFEMNDS16VideoChromaReg     videoChroma;    ///< Video(Full) out Chroma register config
    IFEMNDS16DisplayLumaReg     displayLuma;    ///< Display(Full) out Luma register config
    IFEMNDS16DisplayChromaReg   displayChroma;  ///< Display(Full) out Chroma register config
};

static const UINT32 MNDS16InterpolationShift = 14;     ///< Interpolation shift value
static const UINT32 MNDS16ScalingLimit       = 128;    ///< Downscaling limit of MNDS 16 version
static const UINT32 MNDS16MaxScaleFactor     = 64;     ///< Max downscaling factor considering chroma needs to be
                                                       /// downscaled twice as much as luma channel

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE MNDS16 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEMNDS16Titan17x final : public ISPHWSetting
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
    /// @brief  Calculate register settings based on common library result
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
    /// ~IFEMNDS16Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEMNDS16Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEMNDS16Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEMNDS16Titan17x();

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
    /// ConfigureDisplayLumaRegisters
    ///
    /// @brief  Configure scaler Display Luma output path registers
    ///
    /// @param  horizontalInterpolationResolution horizontal interpolation resolution
    /// @param  verticalInterpolationResolution   vertical interpolation resolution
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDisplayLumaRegisters(
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
    /// ConfigureDisplayChromaRegisters
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
    VOID ConfigureDisplayChromaRegisters(
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

    IFEMNDS16RegCmd    m_regCmd;        ///< Register List of this Module
    MNDSState*         m_pState;        ///< State
    Format             m_pixelFormat;   ///< Output pixel format
    UINT32             m_output;        ///< IPE piepline output
    ISPInputData*      m_pInputData;    ///< Pointer to current input data

    IFEMNDS16Titan17x(const IFEMNDS16Titan17x&)            = delete; ///< Disallow the copy constructor
    IFEMNDS16Titan17x& operator=(const IFEMNDS16Titan17x&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEMNDS16TITAN17X_H
