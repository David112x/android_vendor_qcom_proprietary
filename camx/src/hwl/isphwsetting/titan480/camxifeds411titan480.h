////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifeds411titan480.h
/// @brief IFE DS411 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEDS411TITAN480_H
#define CAMXIFEDS411TITAN480_H

#include "titan480_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispiqmodule.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

struct IFEDS4DisplayLumaReg
{
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_DISP_DS4_OUT_MODULE_CFG       lumaModuleConfig;   ///< Luma config
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_DISP_DS4_OUT_DS_COEFF         coefficients;       ///< Luma coefficients
} CAMX_PACKED;

struct IFEDS4DisplayLumaCropReg
{
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_DISP_DS4_OUT_CROP_LINE_CFG    lineConfig;         ///< Luma Crop Line config
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_DISP_DS4_OUT_CROP_PIXEL_CFG   pixelConfig;        ///< Luma Crop Pixel config
} CAMX_PACKED;

struct IFEDS4DisplayChromaReg
{
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_DISP_DS4_OUT_MODULE_CFG       chromaModuleConfig; ///< Chroma config
} CAMX_PACKED;

struct IFEDS4DisplayChromaCropReg
{
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_DISP_DS4_OUT_CROP_LINE_CFG    lineConfig;         ///< Chroma Crop Line config
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_DISP_DS4_OUT_CROP_PIXEL_CFG   pixelConfig;        ///< Chroma Crop Pixel config
} CAMX_PACKED;

struct IFEDS16DisplayLumaReg
{
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_DISP_DS16_OUT_MODULE_CFG      lumaModuleConfig;   ///< Luma DS16 config
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_DISP_DS16_OUT_DS_COEFF        coefficients;       ///< Luma DS16 coefficients
} CAMX_PACKED;

struct IFEDS16DisplayLumaCropReg
{
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_DISP_DS16_OUT_CROP_LINE_CFG   lineConfig;         ///< Luma DS16 Crop Linc config
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_DISP_DS16_OUT_CROP_PIXEL_CFG  pixelConfig;        ///< Luma DS16 Crop Pixel config
} CAMX_PACKED;

struct IFEDS16DisplayChromaReg
{
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_DISP_DS16_OUT_MODULE_CFG      chromaModuleConfig; ///< Chroma DS16 config
} CAMX_PACKED;

struct IFEDS16DisplayChromaCropReg
{
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_DISP_DS16_OUT_CROP_LINE_CFG   lineConfig;         ///< Chroma DS16 Crop Line config
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_DISP_DS16_OUT_CROP_PIXEL_CFG  pixelConfig;        ///< Chroma DS16 Crop Pixel config
} CAMX_PACKED;

struct IFEDS16VideoLumaReg
{
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_VID_DS16_OUT_MODULE_CFG       lumaModuleConfig;   ///< Luma VID DS16 config
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_VID_DS16_OUT_DS_COEFF         coefficients;       ///< Luma VID DS16 coefficients
} CAMX_PACKED;

struct IFEDS16VideoLumaCropReg
{
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_VID_DS16_OUT_CROP_LINE_CFG    lineConfig;         ///< Luma VID DS16 Crop line config
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_VID_DS16_OUT_CROP_PIXEL_CFG   pixelConfig;        ///< Luma VID DS16 Crop Pixel config
} CAMX_PACKED;

struct IFEDS16VideoChromaReg
{
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_VID_DS16_OUT_MODULE_CFG       chromaModuleConfig; ///< Chroma VID DS16 config
} CAMX_PACKED;

struct IFEDS16VideoChromaCropReg
{
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_VID_DS16_OUT_CROP_LINE_CFG    lineConfig;     ///< Chroma VID DS16 Crop line config
    IFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_VID_DS16_OUT_CROP_PIXEL_CFG   pixelConfig;    ///< Chroma VID DS16 Crop pixel config
} CAMX_PACKED;

CAMX_END_PACKED

struct IFEDS4DisplayReg
{
    IFEDS4DisplayLumaReg        luma;       ///< DS4 Luma Register
    IFEDS4DisplayLumaCropReg    lumaCrop;   ///< DS4 Luma Crop Register
    IFEDS4DisplayChromaReg      chroma;     ///< DS4 Chroma Register
    IFEDS4DisplayChromaCropReg  chromaCrop; ///< DS4 Chroma Crop Register
};

struct IFEDS16DisplayReg
{
    IFEDS16DisplayLumaReg       luma;       ///< DS16 Luma Register
    IFEDS16DisplayLumaCropReg   lumaCrop;   ///< DS16 Luma Crop Register
    IFEDS16DisplayChromaReg     chroma;     ///< DS16 Chroma Register
    IFEDS16DisplayChromaCropReg chromaCrop; ///< DS16 Chroma Crop Register
};

struct IFEVideoReg
{
    IFEDS16VideoLumaReg         luma;       ///< VID DS16 Luma Register
    IFEDS16VideoLumaCropReg     lumaCrop;   ///< VID DS16 Luma Crop Register
    IFEDS16VideoChromaReg       chroma;     ///< VID DS16 Chroma Register
    IFEDS16VideoChromaCropReg   chromaCrop; ///< VID DS16 Chroma Crop Register
};

struct IFEDS411RegCmd
{
    IFEDS4DisplayReg    displayDS4;        ///< DS4 Register
    IFEDS16DisplayReg   displayDS16;       ///< DS16 Register
    IFEVideoReg         videoDS16;         ///< DS4 Video Register
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE DS411 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEDS411Titan480 final : public ISPHWSetting
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
    /// @param  pInput       Pointer to the Input data to the module for calculation
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult SetupRegisterSetting(
        VOID*  pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IFEDS411Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEDS411Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEDS411Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEDS411Titan480();

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
    /// ConfigureDS4Registers
    ///
    /// @brief  Configure DS4 module registers
    ///
    /// @param  pData  Pointer to the Input data buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDS4Registers(
        DS411InputData* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDS16Registers
    ///
    /// @brief  Configure DS16 module registers
    ///
    /// @param  pData  Pointer to the Input data buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDS16Registers(
        DS411InputData* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureVideoDS16Registers
    ///
    /// @brief  Configure Video DS16 module registers
    ///
    /// @param  pData  Pointer to the Input data buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureVideoDS16Registers(
        DS411InputData* pData);


    IFEDS411RegCmd    m_regCmd;     ///< Register List of this Module
    IFEPipelinePath   m_modulePath; ///< IFE pipeline path
    DSState*          m_pState;     ///< Pointer to current DS State

    IFEDS411Titan480(const IFEDS411Titan480&)            = delete; ///< Disallow the copy constructor
    IFEDS411Titan480& operator=(const IFEDS411Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEDS411TITAN480_H
