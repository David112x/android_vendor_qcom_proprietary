// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  dsx10setting.h
/// @brief DSX10 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef DSX10SETTING_H
#define DSX10SETTING_H

#include "dsx_1_0_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"
#include "DSX_Registers.h"
#include "DSX_Chromatix.h"
#include "DS4to1_Chromatix.h"

// NOWHINE NC004c: Share code with system team
struct DSX10UnpackedField
{
    DSX_REG dsxData;     ///< DSX unpacked data
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements DSX10 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class DSX10Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput        Pointer to the input data
    /// @param  pData         Pointer to the input data to the DSX10 interpolation module
    /// @param  pReserveType  Pointer to the Chromatix ReserveType field
    /// @param  pDS4to1Data   Pointer to the Chromatix DS4to1 field
    /// @param  pOutput       Pointer to the unpacked data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const DSX10InputData*                                 pInput,
        dsx_1_0_0::dsx10_rgn_dataType*                        pData,
        const dsx_1_0_0::chromatix_dsx10_reserveType*         pReserveData,
        const ds4to1_1_1_0::mod_ds4to1v11_pass_reserve_dataType::pass_dataStruct* pDS4to1Data,
        VOID*                                                 pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInitializationData
    ///
    /// @brief  Get DSX initialization data
    ///
    /// @param  pData    Pointer to DSXNcLibOutputData
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL GetInitializationData(
        struct DSXNcLibOutputData* pData);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapChromatix2DSXChromatix
    ///
    /// @brief  Map Chromatix Input to DSX defined chromatix data
    ///
    /// @param  pReserveData   Pointer to the input chromatix reserve data
    /// @param  pUnpackedData  Pointer to the input unpacked data
    /// @param  pDSXChromatix  Pointer to the output DSX chromatix data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID MapChromatix2DSXChromatix(
        const dsx_1_0_0::chromatix_dsx10_reserveType* pReserveData,
        dsx_1_0_0::dsx10_rgn_dataType*          pUnpackedData,
        DSX_Chromatix*                          pDSXChromatix);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapChromatix2DS4to1Chromatix
    ///
    /// @brief  Map Chromatix Input to DS4to1 defined chromatix data
    ///
    /// @param  pDS4to1Data    Pointer to the input chromatix DS4to1 data
    /// @param  pDSXChromatix  Pointer to the output DS4to1 chromatix data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID MapChromatix2DS4to1Chromatix(
        const ds4to1_1_1_0::mod_ds4to1v11_pass_reserve_dataType::pass_dataStruct* pDS4to1Data,
        DS4to1_Chromatix*          pDSXChromatix);
};
#endif // DSX10SETTING_H
