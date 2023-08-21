////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifeprecrop10titan17x.h
/// @brief IFE PRECROP register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEPRECROP10TITAN17X_H
#define CAMXIFEPRECROP10TITAN17X_H

#include "titan170_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispiqmodule.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED
struct IFEPreCrop10DS4LumaReg
{
    IFE_IFE_0_VFE_DS4_Y_PRE_CROP_LINE_CFG     lineConfig;     ///< DS4precrop output path Luma Crop line config
    IFE_IFE_0_VFE_DS4_Y_PRE_CROP_PIXEL_CFG    pixelConfig;    ///< DS4precrop output path Luma Crop pixel config
} CAMX_PACKED;

struct IFEPreCrop10DS4ChromaReg
{
    IFE_IFE_0_VFE_DS4_C_PRE_CROP_LINE_CFG     lineConfig;     ///< DS4precrop output path chroma Crop line config
    IFE_IFE_0_VFE_DS4_C_PRE_CROP_PIXEL_CFG    pixelConfig;    ///< DS4precrop output path chroma Crop pixel config
} CAMX_PACKED;

struct IFEPreCrop10DisplayDS4LumaReg
{
    IFE_IFE_0_VFE_DISP_DS4_Y_PRE_CROP_LINE_CFG     lineConfig;     ///< DS4precrop Display output path Luma Crop line config
    IFE_IFE_0_VFE_DISP_DS4_Y_PRE_CROP_PIXEL_CFG    pixelConfig;    ///< DS4precrop Display output path Luma Crop pixel config
} CAMX_PACKED;

struct IFEPreCrop10DisplayDS4ChromaReg
{
    IFE_IFE_0_VFE_DISP_DS4_C_PRE_CROP_LINE_CFG     lineConfig;     ///< DS4precrop Display output path chroma Crop line config
    IFE_IFE_0_VFE_DISP_DS4_C_PRE_CROP_PIXEL_CFG    pixelConfig;    ///< DS4precrop Display output path chroma Crop pixel config
} CAMX_PACKED;

CAMX_END_PACKED

struct IFEPreCrop10RegCmd
{
    IFEPreCrop10DS4LumaReg           DS4Luma;           ///< DS4 path precrop Luma config
    IFEPreCrop10DS4ChromaReg         DS4Chroma;         ///< DS4 path precrop chroma config
    IFEPreCrop10DS4LumaReg           DS16Luma;          ///< DS16 path precrop Luma config
    IFEPreCrop10DS4ChromaReg         DS16Chroma;        ///< DS16 path precrop chroma config
    IFEPreCrop10DisplayDS4LumaReg    displayDS4Luma;    ///< DS4 Display path precrop Luma config
    IFEPreCrop10DisplayDS4ChromaReg  displayDS4Chroma;  ///< DS4 Display path precrop chroma config
    IFEPreCrop10DisplayDS4LumaReg    displayDS16Luma;   ///< DS16 Display path precrop Luma config
    IFEPreCrop10DisplayDS4ChromaReg  displayDS16Chroma; ///< DS16 Display path precrop chroma config
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE PRECROP register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEPreCrop10Titan17x final : public ISPHWSetting
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
    /// ~IFEPreCrop10Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEPreCrop10Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEPreCrop10Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEPreCrop10Titan17x();

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
    IFEPreCrop10RegCmd  m_regCmd;       ///< Register List of this Module
    IFEPipelinePath     m_modulePath;   ///< IFE pipeline path for module

    IFEPreCrop10Titan17x(const IFEPreCrop10Titan17x&)            = delete; ///< Disallow the copy constructor
    IFEPreCrop10Titan17x& operator=(const IFEPreCrop10Titan17x&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEPRECROP10TITAN17X_H
