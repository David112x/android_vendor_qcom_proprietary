////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxiferoundclamp11titan17x.h
/// @brief IFE ROUNDCLAMP11 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEROUNDCLAMP11TITAN17X_H
#define CAMXIFEROUNDCLAMP11TITAN17X_H

#include "titan170_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispiqmodule.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED
struct IFERoundClamp11FullLumaReg
{
    IFE_IFE_0_VFE_FULL_OUT_Y_CH0_CLAMP_CFG       clamp;    ///< Full output path Luma Clamp config
    IFE_IFE_0_VFE_FULL_OUT_Y_CH0_ROUNDING_CFG    round;    ///< Full output path Luma Roound config
} CAMX_PACKED;

struct IFERoundClamp11FullChromaReg
{
    IFE_IFE_0_VFE_FULL_OUT_C_CH0_CLAMP_CFG       clamp;    ///< Full output path Chroma Clamp config
    IFE_IFE_0_VFE_FULL_OUT_C_CH0_ROUNDING_CFG    round;    ///< Full output path Chroma Roound config
} CAMX_PACKED;

struct IFERoundClamp11FDLumaReg
{
    IFE_IFE_0_VFE_FD_OUT_Y_CH0_CLAMP_CFG         clamp;    ///< FD output path Luma Clamp config
    IFE_IFE_0_VFE_FD_OUT_Y_CH0_ROUNDING_CFG      round;    ///< FD output path Luma Roound config
} CAMX_PACKED;

struct IFERoundClamp11FDChromaReg
{
    IFE_IFE_0_VFE_FD_OUT_C_CH0_CLAMP_CFG         clamp;    ///< FD output path Chroma Clamp config
    IFE_IFE_0_VFE_FD_OUT_C_CH0_ROUNDING_CFG      round;    ///< FD output path Chroma Roound config
} CAMX_PACKED;

struct IFERoundClamp11DS4LumaReg
{
    IFE_IFE_0_VFE_DS4_Y_CH0_CLAMP_CFG            clamp;    ///< DS4 output path Luma Clamp config
    IFE_IFE_0_VFE_DS4_Y_CH0_ROUNDING_CFG         round;    ///< DS4 output path Luma Roound config
} CAMX_PACKED;

struct IFERoundClamp11DS4ChromaReg
{
    IFE_IFE_0_VFE_DS4_C_CH0_CLAMP_CFG            clamp;    ///< DS4 output path Chroma Clamp config
    IFE_IFE_0_VFE_DS4_C_CH0_ROUNDING_CFG         round;    ///< DS4 output path Chroma Roound config
} CAMX_PACKED;

struct IFERoundClamp11DS16LumaReg
{
    IFE_IFE_0_VFE_DS16_Y_CH0_CLAMP_CFG           clamp;    ///< DS16 output path Luma Clamp config
    IFE_IFE_0_VFE_DS16_Y_CH0_ROUNDING_CFG        round;    ///< DS16 output path Luma Roound config
} CAMX_PACKED;

struct IFERoundClamp11DS16ChromaReg
{
    IFE_IFE_0_VFE_DS16_C_CH0_CLAMP_CFG           clamp;    ///< DS16 output path Chroma Clamp config
    IFE_IFE_0_VFE_DS16_C_CH0_ROUNDING_CFG        round;    ///< DS16 output path Chroma Roound config
} CAMX_PACKED;

struct IFERoundClamp11DisplayLumaReg
{
    IFE_IFE_0_VFE_DISP_OUT_Y_CH0_CLAMP_CFG       clamp;    ///< Display output path Luma Clamp config
    IFE_IFE_0_VFE_DISP_OUT_Y_CH0_ROUNDING_CFG    round;    ///< Display output path Luma Roound config
} CAMX_PACKED;

struct IFERoundClamp11DisplayChromaReg
{
    IFE_IFE_0_VFE_DISP_OUT_C_CH0_CLAMP_CFG       clamp;    ///< Display output path Chroma Clamp config
    IFE_IFE_0_VFE_DISP_OUT_C_CH0_ROUNDING_CFG    round;    ///< Display output path Chroma Roound config
} CAMX_PACKED;

struct IFERoundClamp11DisplayDS4LumaReg
{
    IFE_IFE_0_VFE_DISP_DS4_Y_CH0_CLAMP_CFG       clamp;    ///< DS4 Display output path Luma Clamp config
    IFE_IFE_0_VFE_DISP_DS4_Y_CH0_ROUNDING_CFG    round;    ///< DS4 Display output path Luma Roound config
} CAMX_PACKED;

struct IFERoundClamp11DisplayDS4ChromaReg
{
    IFE_IFE_0_VFE_DISP_DS4_C_CH0_CLAMP_CFG       clamp;    ///< DS4 Display output path Chroma Clamp config
    IFE_IFE_0_VFE_DISP_DS4_C_CH0_ROUNDING_CFG    round;    ///< DS4 Display output path Chroma Roound config
} CAMX_PACKED;

struct IFERoundClamp11displayDS16LumaReg
{
    IFE_IFE_0_VFE_DISP_DS16_Y_CH0_CLAMP_CFG      clamp;    ///< DS16 Display output path Luma Clamp config
    IFE_IFE_0_VFE_DISP_DS16_Y_CH0_ROUNDING_CFG   round;    ///< DS16 Display output path Luma Roound config
} CAMX_PACKED;

struct IFERoundClamp11displayDS16ChromaReg
{
    IFE_IFE_0_VFE_DISP_DS16_C_CH0_CLAMP_CFG      clamp;    ///< DS16 Display output path Chroma Clamp config
    IFE_IFE_0_VFE_DISP_DS16_C_CH0_ROUNDING_CFG   round;    ///< DS16 Display output path Chroma Roound config
} CAMX_PACKED;

CAMX_END_PACKED

struct IFERoundClamp11RegCmd
{
    IFERoundClamp11FullLumaReg                   fullLuma;          ///< FD path Luma config
    IFERoundClamp11FullChromaReg                 fullChroma;        ///< FD path Chroma config
    IFERoundClamp11FDLumaReg                     FDLuma;            ///< Full path Luma config
    IFERoundClamp11FDChromaReg                   FDChroma;          ///< Full path Luma config
    IFERoundClamp11DS4LumaReg                    DS4Luma;           ///< DS4 path Luma config
    IFERoundClamp11DS4ChromaReg                  DS4Chroma;         ///< DS4 path Luma config
    IFERoundClamp11DS16LumaReg                   DS16Luma;          ///< DS16 path Luma config
    IFERoundClamp11DS16ChromaReg                 DS16Chroma;        ///< DS16 path Luma config
    IFERoundClamp11DisplayLumaReg                displayLuma;       ///< Disp path Luma config
    IFERoundClamp11DisplayChromaReg              displayChroma;     ///< Disp path Chroma config
    IFERoundClamp11DisplayDS4LumaReg             displayDS4Luma;    ///< DS4 Disp path Luma config
    IFERoundClamp11DisplayDS4ChromaReg           displayDS4Chroma;  ///< DS4 Disp path Luma config
    IFERoundClamp11displayDS16LumaReg            displayDS16Luma;   ///< DS16 Disp path Luma config
    IFERoundClamp11displayDS16ChromaReg          displayDS16Chroma; ///< DS16 Disp path Luma config
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE ROUNDCLAMP11 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFERoundClamp11Titan17x final : public ISPHWSetting
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
    /// ~IFERoundClamp11Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFERoundClamp11Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFERoundClamp11Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFERoundClamp11Titan17x();

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

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureFDPathRegisters
    ///
    /// @brief  Configure RoundClamp11 FD output path registers
    ///
    /// @param  minValue   Minimum threshold to clamp
    /// @param  maxValue   Maximum threshold to clamp
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureFDPathRegisters(
        UINT32 minValue,
        UINT32 maxValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureFullPathRegisters
    ///
    /// @brief  Configure RoundClamp11 Full output path registers
    ///
    /// @param  minValue   Minimum threshold to clamp
    /// @param  maxValue   Maximum threshold to clamp
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureFullPathRegisters(
        UINT32 minValue,
        UINT32 maxValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDS4PathRegisters
    ///
    /// @brief  Configure RoundClamp11 DS4 output path registers
    ///
    /// @param  minValue   Minimum threshold to clamp
    /// @param  maxValue   Maximum threshold to clamp
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDS4PathRegisters(
        UINT32 minValue,
        UINT32 maxValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDS16PathRegisters
    ///
    /// @brief  Configure RoundClamp11 DS16 output path registers
    ///
    /// @param  minValue   Minimum threshold to clamp
    /// @param  maxValue   Maximum threshold to clamp
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDS16PathRegisters(
        UINT32 minValue,
        UINT32 maxValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDisplayPathRegisters
    ///
    /// @brief  Configure RoundClamp11 Display output path registers
    ///
    /// @param  minValue   Minimum threshold to clamp
    /// @param  maxValue   Maximum threshold to clamp
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDisplayPathRegisters(
        UINT32 minValue,
        UINT32 maxValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDS4DisplayPathRegisters
    ///
    /// @brief  Configure RoundClamp11 DS4 Display output path registers
    ///
    /// @param  minValue   Minimum threshold to clamp
    /// @param  maxValue   Maximum threshold to clamp
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDS4DisplayPathRegisters(
        UINT32 minValue,
        UINT32 maxValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDS16DisplayPathRegisters
    ///
    /// @brief  Configure RoundClamp11 DS16 Display output path registers
    ///
    /// @param  minValue   Minimum threshold to clamp
    /// @param  maxValue   Maximum threshold to clamp
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDS16DisplayPathRegisters(
        UINT32 minValue,
        UINT32 maxValue);

    IFERoundClamp11RegCmd     m_regCmd;       ///< Register List of this Module
    IFEPipelinePath           m_modulePath;   ///< IFE pipeline path for module
    INT32                     m_bitWidth;     ///< ISP output bit width based on ISP output format

    IFERoundClamp11Titan17x(const IFERoundClamp11Titan17x&)            = delete; ///< Disallow the copy constructor
    IFERoundClamp11Titan17x& operator=(const IFERoundClamp11Titan17x&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEROUNDCLAMP11TITAN17X_H
