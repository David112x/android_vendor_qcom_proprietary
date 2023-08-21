////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecamiftitan17x.h
/// @brief IFE CAMIF register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFECAMIFTITAN17X_H
#define CAMXIFECAMIFTITAN17X_H

#include "titan170_ife.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED
/// @brief IFE CAMIF Module Register Set 1
struct IFECAMIFRegCmd1
{
    IFE_IFE_0_VFE_CAMIF_CMD statusRegister; ///< CAMIF Status Register 0x478
    IFE_IFE_0_VFE_CAMIF_CFG configRegister; ///< CAMIF Config Register 0x47c
} CAMX_PACKED;

/// @brief IFE CAMIF Module Register Set 2
struct IFECAMIFRegCmd2
{
    IFE_IFE_0_VFE_CAMIF_LINE_SKIP_PATTERN  lineSkipPatternRegister;  ///< CAMIF Line Skip Pattern Register 0x488
    IFE_IFE_0_VFE_CAMIF_PIXEL_SKIP_PATTERN pixelSkipPatternRegister; ///< pixel skip Pattern Register 0x48c
    IFE_IFE_0_VFE_CAMIF_SKIP_PERIOD        skipPeriodRegister;       ///< CAMIF Skip Period Register 0x490
} CAMX_PACKED;

/// @brief IFE CAMIF Module Register Set 3
struct IFECAMIFRegCmd3
{
    IFE_IFE_0_VFE_CAMIF_IRQ_SUBSAMPLE_PATTERN irqSubsamplePattern; ///< IRQ Subsample Pattern Register 0x49c

    // Removed IFE_IFE_0_VFE_CAMIF_EPOCH_IRQ due to the design decision to handle in the kernel side
} CAMX_PACKED;

/// @brief IFE CAMIF Module Register Set 4
struct IFECAMIFRegCmd4
{
    IFE_IFE_0_VFE_CAMIF_RAW_CROP_WIDTH_CFG    rawCropWidthConfig;  ///< Raw Image Width Crop Register 0xce4
    IFE_IFE_0_VFE_CAMIF_RAW_CROP_HEIGHT_CFG   rawCropHeightConfig; ///< Raw Image Height Crop Register 0xce8
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IFECAMIFRegLengthDWord1 = sizeof(IFECAMIFRegCmd1) / sizeof(UINT32);
static const UINT32 IFECAMIFRegLengthDWord2 = sizeof(IFECAMIFRegCmd2) / sizeof(UINT32);
static const UINT32 IFECAMIFRegLengthDWord3 = sizeof(IFECAMIFRegCmd3) / sizeof(UINT32);
static const UINT32 IFECAMIFRegLengthDWord4 = sizeof(IFECAMIFRegCmd4) / sizeof(UINT32);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE CAMIF register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFECAMIFTitan17x final : public ISPHWSetting
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
    /// ~IFECAMIFTitan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFECAMIFTitan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFECAMIFTitan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFECAMIFTitan17x();

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

    IFECAMIFRegCmd1    m_regCmd1;        ///< Register List 1 of this module
    IFECAMIFRegCmd2    m_regCmd2;        ///< Register List 2 of this module
    IFECAMIFRegCmd3    m_regCmd3;        ///< Register List 3 of this module
    IFECAMIFRegCmd4    m_regCmd4;        ///< Register List 4 of this module

    IFECAMIFTitan17x(const IFECAMIFTitan17x&)            = delete; ///< Disallow the copy constructor
    IFECAMIFTitan17x& operator=(const IFECAMIFTitan17x&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFECAMIFTITAN17X_H
