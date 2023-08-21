////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecamifpptitan480.h
/// @brief IFE CAMIF register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFECAMIFPPTITAN480_H
#define CAMXIFECAMIFPPTITAN480_H

// NOWHINE FILE GR027:  Hardware register naming convension
// NOWHINE FILE CP019:  Explicit constructor

#include "titan480_ife.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED
/// @brief IFE CAMIF Module Register Set 1
struct IFECAMIFPPRegCmd1
{
    IFE_IFE_0_PP_CLC_CAMIF_MODULE_CFG               configRegister;                     ///< Camif module configuration
} CAMX_PACKED;

/// @brief IFE CAMIF Module Register Set 2
struct IFECAMIFPPRegCmd2
{
    IFE_IFE_0_PP_CLC_CAMIF_LINE_SKIP_PATTERN_CFG    lineSkipPatternRegister;            ///< Camif line skip pattern
    IFE_IFE_0_PP_CLC_CAMIF_PIXEL_SKIP_PATTERN_CFG   pixelSkipPatternRegister;           ///< Camif pixel skip pattern
} CAMX_PACKED;

/// @brief IFE CAMIF Module Register Set 3
struct IFECAMIFPPRegCmd3
{
    IFE_IFE_0_PP_CLC_CAMIF_PERIOD_CFG                   periodCongureRegister;          ///< Camif skip period
    IFE_IFE_0_PP_CLC_CAMIF_IRQ_SUBSAMPLE_PATTERN_CFG    irqSubsamplePatternRegister;    ///< Camif irq subsample pattern
} CAMX_PACKED;

/// @brief IFE CAMIF Module Register Set 4
struct IFECAMIFPPRegCmd4
{
    IFE_IFE_0_PP_CLC_CAMIF_PDAF_RAW_CROP_WIDTH_CFG      rawCropWidthRegister;           ///< Camif PDAF raw crop width
    IFE_IFE_0_PP_CLC_CAMIF_PDAF_RAW_CROP_HEIGHT_CFG     rawCropHeightRegister;          ///< Camif PDAF raw crop height
} CAMX_PACKED;

struct IFEPixelRawConfigCmd
{
    IFE_IFE_0_TOP_CORE_CFG_1 rawPixelPathConfig;    ///< Core config1 to pick the Raw path
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IFECAMIFPPRegLengthDWord1 = sizeof(IFECAMIFPPRegCmd1) / sizeof(UINT32);
static const UINT32 IFECAMIFPPRegLengthDWord2 = sizeof(IFECAMIFPPRegCmd2) / sizeof(UINT32);
static const UINT32 IFECAMIFPPRegLengthDWord3 = sizeof(IFECAMIFPPRegCmd3) / sizeof(UINT32);
static const UINT32 IFECAMIFPPRegLengthDWord4 = sizeof(IFECAMIFPPRegCmd4) / sizeof(UINT32);

static const UINT32 IFEPixelRawRegLengthDWord = sizeof(IFEPixelRawConfigCmd) / sizeof(UINT32);

const UINT32 IFECamifPPRegCmd1Offset = regIFE_IFE_0_PP_CLC_CAMIF_MODULE_CFG;
const UINT32 IFECamifPPRegCmd2Offset = regIFE_IFE_0_PP_CLC_CAMIF_LINE_SKIP_PATTERN_CFG;
const UINT32 IFECamifPPRegCmd3Offset = regIFE_IFE_0_PP_CLC_CAMIF_PERIOD_CFG;
const UINT32 IFECamifPPRegCmd4Offset = regIFE_IFE_0_PP_CLC_CAMIF_PDAF_RAW_CROP_WIDTH_CFG;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE CAMIF register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFECAMIFPPTitan480 final : public ISPHWSetting
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
    /// CreateSubCmdList
    ///
    /// @brief  Generate the Sub Command List
    ///
    /// @param  pInputData       Pointer to the Inputdata
    /// @param  pDMIBufferOffset Pointer for DMI Buffer
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult CreateSubCmdList(
        VOID*   pInputData,
        UINT32* pDMIBufferOffset);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PackIQRegisterSetting
    ///
    /// @brief  Calculate register settings based on common library result
    ///
    /// @param  pInput       Pointer to the Input data to the module for calculation
    /// @param  pOutput      Pointer to the Output data
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
    /// ~IFECAMIFPPTitan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFECAMIFPPTitan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFECAMIFPPTitan480
    ///
    /// @brief  Constructor
    ///
    /// @param  instance       PP camif instance id. (zero based)
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFECAMIFPPTitan480(
        UINT32 instance);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
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

    UINT32                  m_instance;         ///< Instance id (zero based)
    IFECAMIFPPRegCmd1       m_regCmd1;          ///< Register List 1 of this module
    IFECAMIFPPRegCmd2       m_regCmd2;          ///< Register List 2 of this module
    IFECAMIFPPRegCmd3       m_regCmd3;          ///< Register List 3 of this module
    IFECAMIFPPRegCmd4       m_regCmd4;          ///< Register List 4 of this module
    IFEPixelRawConfigCmd    m_pixelRawRegCmd;   ///< Pixel raw path config

    IFECAMIFPPTitan480(const IFECAMIFPPTitan480&)            = delete; ///< Disallow the copy constructor
    IFECAMIFPPTitan480& operator=(const IFECAMIFPPTitan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFECAMIFTITAN17X_H
