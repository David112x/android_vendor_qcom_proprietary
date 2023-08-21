////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecamifrdititan480.h
/// @brief IFE RDI CAMIF register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFECAMIFRDITITAN480_H
#define CAMXIFECAMIFRDITITAN480_H

#include "titan480_ife.h"
#include "titan480_ife_lite_wrapper.h"
#include "camxisphwsetting.h"

// NOWHINE FILE GR027:  Hardware register naming convension
// NOWHINE FILE CP019:  Explicit constructor

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

struct IFECAMIFRdiRegCmd1
{
    IFE_IFE_0_RDI0_CLC_CAMIF_MODULE_CFG                configRegister;      ///< CAMIF LITE Config Register 0xfc4
} CAMX_PACKED;

struct IFECAMIFRdiRegCmd2
{
    IFE_IFE_0_RDI0_CLC_CAMIF_PERIOD_CFG                skipPeriodRegister;  ///< CAMIF Skip Period Register 0xfc8
    IFE_IFE_0_RDI0_CLC_CAMIF_IRQ_SUBSAMPLE_PATTERN_CFG irqSubsamplePattern; ///< IRQ Subsample Pattern Register 0xfcc
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IFECAMIFRdiRegCmd1LengthDWord = sizeof(IFECAMIFRdiRegCmd1) / sizeof(UINT32);
static const UINT32 IFECAMIFRdiRegCmd2LengthDWord = sizeof(IFECAMIFRdiRegCmd2) / sizeof(UINT32);

// Normal IFE RDI Register offset
const UINT32 IFECamifRdiRegCmd1Offset       = regIFE_IFE_0_RDI0_CLC_CAMIF_MODULE_CFG;
const UINT32 IFECamifRdiRegCmd2Offset       = regIFE_IFE_0_RDI0_CLC_CAMIF_PERIOD_CFG;

// Different RDI Instance offset, Normal IFE and Lite is same
const UINT32 IFECamifRdiRegInstanceOffset   = (regIFE_IFE_0_RDI1_CLC_CAMIF_MODULE_CFG - regIFE_IFE_0_RDI0_CLC_CAMIF_MODULE_CFG);

// IFE_Lite RDI register offset
const UINT32 IFELiteCamifRdiRegCmd1Offset   = regIFE_LITE_WRAPPER_IFELITE_WRAPPER_IFE_LITE0_CLC_CAMIF0_MODULE_CFG;
const UINT32 IFELiteCamifRdiRegCmd2Offset   = regIFE_LITE_WRAPPER_IFELITE_WRAPPER_IFE_LITE0_CLC_CAMIF0_PERIOD_CFG;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE CAMIF register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFECAMIFRdiTitan480 final : public ISPHWSetting
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
    /// ~IFECAMIFRdiTitan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFECAMIFRdiTitan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFECAMIFRdiTitan480
    ///
    /// @brief  Constructor
    ///
    /// @param  instance            rdi camif instance id. (zero based)
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFECAMIFRdiTitan480(
        UINT32 instance);

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

    UINT32                 m_instance;       ///< Instance id (zero based)
    IFECAMIFRdiRegCmd1     m_regCmd1;        ///< Register List for camif module cfg
    IFECAMIFRdiRegCmd2     m_regCmd2;        ///< Register List for camif IRQ and period cfg

    IFECAMIFRdiTitan480(const IFECAMIFRdiTitan480&)            = delete; ///< Disallow the copy constructor
    IFECAMIFRdiTitan480& operator=(const IFECAMIFRdiTitan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFECAMIFRDITITAN480_H
