////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifedsx10titan480.h
/// @brief IFE DSX10 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEDSX10TITAN480_H
#define CAMXIFEDSX10TITAN480_H

#include "titan480_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispiqmodule.h"
#include "dsx10setting.h"

CAMX_NAMESPACE_BEGIN

static const UINT32 DSX_LUMA_KERNAL_WEIGHTS_HORIZ   = 96;
static const UINT32 DSX_LUMA_KERNAL_WEIGHTS_VERT    = 96;
static const UINT32 DSX_CHROMA_KERNAL_WEIGHTS_HORIZ = 48;
static const UINT32 DSX_CHROMA_KERNAL_WEIGHTS_VERT  = 48;

static const UINT32 IFEDSX10LumaHorDMISize   = DSX_LUMA_KERNAL_WEIGHTS_HORIZ * sizeof(UINT64);
static const UINT32 IFEDSX10LumaVerDMISize   = DSX_LUMA_KERNAL_WEIGHTS_VERT * sizeof(UINT64);
static const UINT32 IFEDSX10ChromaHorDMISize = DSX_CHROMA_KERNAL_WEIGHTS_HORIZ * sizeof(UINT64);
static const UINT32 IFEDSX10ChromaVerDMISize = DSX_CHROMA_KERNAL_WEIGHTS_VERT * sizeof(UINT64);


static const UINT32 IFEDSX10LumaHorizDword  =
                    (IFEDSX10LumaHorDMISize) / sizeof(UINT32);
static const UINT32 IFEDSX10LumaVertDWord =
                    (IFEDSX10LumaVerDMISize) / sizeof(UINT32);


static const UINT32 IFEDSX10ChromaHorizDword =
                    (IFEDSX10ChromaHorDMISize) / sizeof(UINT32);
static const UINT32 IFEDSX10ChromaVertDWord =
                    (IFEDSX10ChromaVerDMISize) / sizeof(UINT32);

static const UINT32 IFEDSX10DMITotalDWord = IFEDSX10LumaHorizDword +
                                            IFEDSX10LumaVertDWord +
                                            IFEDSX10ChromaHorizDword +
                                            IFEDSX10ChromaVertDWord;

static const UINT32 IFEDSXLumaHorizLUT   = IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_DMI_LUT_CFG_LUT_SEL_KERNEL_W_HORIZ_LUMA_LUT;
static const UINT32 IFEDSXLumaVertLUT    = IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_DMI_LUT_CFG_LUT_SEL_KERNEL_W_VERT_LUMA_LUT;
static const UINT32 IFEDSXChromaHorizLUT = IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_DMI_LUT_CFG_LUT_SEL_KERNEL_W_HORIZ_CHROMA_LUT;
static const UINT32 IFEDSXChromaVertLUT  = IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_DMI_LUT_CFG_LUT_SEL_KERNEL_W_VERT_CHROMA_LUT;

CAMX_BEGIN_PACKED

struct IFEDSXVideoLumaReg
{
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_DMI_LUT_BANK_CFG    lutBankCfg;       ///< Lut Bank Config
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_MODULE_LUT_BANK_CFG modulelutConfig;  ///< Module LUT;
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_MODULE_CFG          moduleConfig;     ///< Luma Module config
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_MODE                mode;             ///< Luma mode
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_START_LOCATION_X    startLocationX;   ///< Luma Start Location X
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_START_LOCATION_Y    startLocationY;   ///< Luma Start Location Y
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_SCALE_RATIO_X       scaleRatioX;      ///< Luma scaleRatio X
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_SCALE_RATIO_Y       scaleRatioY;      ///< Luma scaleRatio Y
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_OUT_SIZE            outputSize;       ///< Luma Output Size
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_IN_SIZE             inputSize;        ///< Luma Input Size
} CAMX_PACKED;

struct IFEDSXVideoLumaPaddingTopBottomReg
{
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_0   paddingWeightTopBotLuma0;  ///< Luma Top bottom 0
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_1   paddingWeightTopBotLuma1;  ///< Luma Top bottom 1
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_2   paddingWeightTopBotLuma2;  ///< Luma Top bottom 2
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_3   paddingWeightTopBotLuma3;  ///< Luma Top bottom 3
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_4   paddingWeightTopBotLuma4;  ///< Luma Top bottom 4
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_5   paddingWeightTopBotLuma5;  ///< Luma Top bottom 5
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_6   paddingWeightTopBotLuma6;  ///< Luma Top bottom 6
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_7   paddingWeightTopBotLuma7;  ///< Luma Top bottom 7
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_8   paddingWeightTopBotLuma8;  ///< Luma Top bottom 8
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_9   paddingWeightTopBotLuma9;  ///< Luma Top bottom 9
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_10  paddingWeightTopBotLuma10; ///< Luma Top bottom 10
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_11  paddingWeightTopBotLuma11; ///< Luma Top bottom 11
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_12  paddingWeightTopBotLuma12; ///< Luma Top bottom 12
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_13  paddingWeightTopBotLuma13; ///< Luma Top bottom 13
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_14  paddingWeightTopBotLuma14; ///< Luma Top bottom 14
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_15  paddingWeightTopBotLuma15; ///< Luma Top bottom 15
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_16  paddingWeightTopBotLuma16; ///< Luma Top bottom 16
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_17  paddingWeightTopBotLuma17; ///< Luma Top bottom 17
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_18  paddingWeightTopBotLuma18; ///< Luma Top bottom 18
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_19  paddingWeightTopBotLuma19; ///< Luma Top bottom 19
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_20  paddingWeightTopBotLuma20; ///< Luma Top bottom 20
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_21  paddingWeightTopBotLuma21; ///< Luma Top bottom 21
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_22  paddingWeightTopBotLuma22; ///< Luma Top bottom 22
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_TOP_BOT_LUMA_23  paddingWeightTopBotLuma23; ///< Luma Top bottom 23
} CAMX_PACKED;


struct IFEDSXVideoLumaPaddingLeftRightReg
{
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_0   paddingWeightLeftRgtLuma0;  ///< Luma Left Right 0
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_1   paddingWeightLeftRgtLuma1;  ///< Luma Left Right 1
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_2   paddingWeightLeftRgtLuma2;  ///< Luma Left Right 2
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_3   paddingWeightLeftRgtLuma3;  ///< Luma Left Right 3
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_4   paddingWeightLeftRgtLuma4;  ///< Luma Left Right 4
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_5   paddingWeightLeftRgtLuma5;  ///< Luma Left Right 5
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_6   paddingWeightLeftRgtLuma6;  ///< Luma Left Right 6
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_7   paddingWeightLeftRgtLuma7;  ///< Luma Left Right 7
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_8   paddingWeightLeftRgtLuma8;  ///< Luma Left Right 8
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_9   paddingWeightLeftRgtLuma9;  ///< Luma Left Right 9
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_10  paddingWeightLeftRgtLuma10; ///< Luma Left Right 10
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_11  paddingWeightLeftRgtLuma11; ///< Luma Left Right 11
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_12  paddingWeightLeftRgtLuma12; ///< Luma Left Right 12
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_13  paddingWeightLeftRgtLuma13; ///< Luma Left Right 13
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_14  paddingWeightLeftRgtLuma14; ///< Luma Left Right 14
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_15  paddingWeightLeftRgtLuma15; ///< Luma Left Right 15
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_16  paddingWeightLeftRgtLuma16; ///< Luma Left Right 16
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_17  paddingWeightLeftRgtLuma17; ///< Luma Left Right 17
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_18  paddingWeightLeftRgtLuma18; ///< Luma Left Right 18
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_19  paddingWeightLeftRgtLuma19; ///< Luma Left Right 19
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_20  paddingWeightLeftRgtLuma20; ///< Luma Left Right 20
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_21  paddingWeightLeftRgtLuma21; ///< Luma Left Right 21
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_22  paddingWeightLeftRgtLuma22; ///< Luma Left Right 22
    IFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_23  paddingWeightLeftRgtLuma23; ///< Luma Left Right 23
} CAMX_PACKED;


struct IFEDSXVideoChromaReg
{
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_DMI_LUT_BANK_CFG    lutBankCfg;      ///< Lut Bank Config
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_MODULE_LUT_BANK_CFG modulelutConfig; ///< Module LUT;
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_MODULE_CFG          moduleConfig;    ///< Chroma Module config
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_MODE                mode;            ///< Chroma mode
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_START_LOCATION_X    startLocationX;  ///< Chroma Start Location X
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_START_LOCATION_Y    startLocationY;  ///< Chroma Start Location Y
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_SCALE_RATIO_X       scaleRatioX;     ///< Chroma scaleRatio X
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_SCALE_RATIO_Y       scaleRatioY;     ///< Chroma scaleRatio Y
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_OUT_SIZE            outputSize;      ///< Chroma Output Size
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_IN_SIZE             inputSize;       ///< Chroma Input Size
} CAMX_PACKED;

struct IFEDSXVideoChromaPaddingTopBottomReg
{
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_PADDING_WEIGHTS_TOP_BOT_CHROMA_0   paddingWeightTopBotChroma0;   ///< chroma Top bottom 0
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_PADDING_WEIGHTS_TOP_BOT_CHROMA_1   paddingWeightTopBotChroma1;   ///< chroma Top bottom 1
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_PADDING_WEIGHTS_TOP_BOT_CHROMA_2   paddingWeightTopBotChroma2;   ///< chroma Top bottom 2
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_PADDING_WEIGHTS_TOP_BOT_CHROMA_3   paddingWeightTopBotChroma3;   ///< chroma Top bottom 3
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_PADDING_WEIGHTS_TOP_BOT_CHROMA_4   paddingWeightTopBotChroma4;   ///< chroma Top bottom 4
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_PADDING_WEIGHTS_TOP_BOT_CHROMA_5   paddingWeightTopBotChroma5;   ///< chroma Top bottom 5
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_PADDING_WEIGHTS_TOP_BOT_CHROMA_6   paddingWeightTopBotChroma6;   ///< chroma Top bottom 6
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_PADDING_WEIGHTS_TOP_BOT_CHROMA_7   paddingWeightTopBotChroma7;   ///< chroma Top bottom 7
} CAMX_PACKED;


struct IFEDSXVideoChromaPaddingLeftRightReg
{
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_CHROMA_0   paddingWeightLeftRgtChroma0;  ///< chroma Left Right 0
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_CHROMA_1   paddingWeightLeftRgtChroma1;  ///< chroma Left Right 1
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_CHROMA_2   paddingWeightLeftRgtChroma2;  ///< chroma Left Right 2
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_CHROMA_3   paddingWeightLeftRgtChroma3;  ///< chroma Left Right 3
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_CHROMA_4   paddingWeightLeftRgtChroma4;  ///< chroma Left Right 4
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_CHROMA_5   paddingWeightLeftRgtChroma5;  ///< chroma Left Right 5
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_CHROMA_6   paddingWeightLeftRgtChroma6;  ///< chroma Left Right 6
    IFE_IFE_0_PP_CLC_DSX_C_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_CHROMA_7   paddingWeightLeftRgtChroma7;  ///< chroma Left Right 7
} CAMX_PACKED;


struct IFEDSX10RegCmd
{
    IFEDSXVideoLumaReg                    luma;              ///< DSX Luma Config Register
    IFEDSXVideoLumaPaddingTopBottomReg    lumaTopBottom;     ///< DSX Luma Padding Register
    IFEDSXVideoLumaPaddingLeftRightReg    lumaLeftRight;     ///< DSX Luma Padding Register
    IFEDSXVideoChromaReg                  chroma;            ///< DSX Chroma Config Register
    IFEDSXVideoChromaPaddingTopBottomReg  chromaTopBottom;   ///< DSX Chroma Padding Register
    IFEDSXVideoChromaPaddingLeftRightReg  chromaLeftRight;   ///< DSX Chroma Padding Register
} CAMX_PACKED;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE DS411 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEDSX10Titan480 final : public ISPHWSetting
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
    /// ~IFEDSX10Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEDSX10Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEDSX10Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEDSX10Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of DSX module for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpDMIData
    ///
    /// @brief  Print DMI configuration of DSX module for debug
    ///
    /// @param  pData  Pointer to the Output Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpDMIData(
        VOID* pData);

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

    IFEDSX10RegCmd    m_regCmd;     ///< Register List of this Module
    UINT32            m_lutBank;    ///< Lut Bank Switch

    IFEDSX10Titan480(const IFEDSX10Titan480&)            = delete; ///< Disallow the copy constructor
    IFEDSX10Titan480& operator=(const IFEDSX10Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEDSX10TITAN480_H
