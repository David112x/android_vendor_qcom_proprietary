////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipecst12titan480.h
/// @brief IPE CST3 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIPECST12TITAN480_H
#define CAMXIPECST12TITAN480_H

#include "camxisphwsetting.h"
#include "camxcmdbuffer.h"
#include "camxispiqmodule.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IPE CST12 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IPECST12Titan480 final : public ISPHWSetting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPECST12Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IPECST12Titan480();

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
    /// UpdateTuningMetadata
    ///
    /// @brief  Update Tuning Metadata
    ///
    /// @param  pInputData      Pointer to the InputData
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult UpdateTuningMetadata(
        VOID*  pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PackIQRegisterSetting
    ///
    /// @brief  Calculate register settings based on common library result
    ///
    /// @param  pInput       Pointer to the Input data to the IQ module for calculation
    /// @param  pOutput      Pointer to the Output data to the IQ module for DMI calculation
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PackIQRegisterSetting(
        VOID*  pInput,
        VOID*  pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupRegisterSetting
    ///
    /// @brief  Setup register value based on CamX Input
    ///
    /// @param  pInput       Pointer to the Input data to the Demosaic37 module for calculation
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult SetupRegisterSetting(
        VOID*  pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID DumpRegConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRegSize
    ///
    /// @brief  Returns register size
    ///
    ///
    /// @return Number of bytes of register structure.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual UINT GetRegSize();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IPECST12Titan480
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IPECST12Titan480();

private:
    IPECST12Titan480(const IPECST12Titan480&)            = delete; ///< Disallow the copy constructor
    IPECST12Titan480& operator=(const IPECST12Titan480&) = delete; ///< Disallow assignment operator

    VOID* m_pRegCmd;                                                 ///< Register List of this Module
};

CAMX_NAMESPACE_END

#endif // CAMXIPECST12TITAN480_H
