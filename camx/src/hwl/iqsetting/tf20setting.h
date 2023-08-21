// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  tf20setting.h
/// @brief TF1_0 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef TF20SETTING_H
#define TF20SETTING_H

#include "ipe_data.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"
#include "tf_2_0_0.h"
#include "TF_Chromatix.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements TF20 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class TF20Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput        Pointer to the input data
    /// @param  pData         Pointer to the interpolation result
    /// @param  pReserveData  Pointer to the Chromatix ReserveType field
    /// @param  pModuleEnable Pointer to the variable(s) to enable this module
    /// @param  pOutput       Pointer to the unpacked data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const TF20InputData*                                pInput,
        tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct*    pData,
        tf_2_0_0::chromatix_tf20_reserveType*               pReserveData,
        tf_2_0_0::chromatix_tf20Type::enable_sectionStruct* pModuleEnable,
        VOID*                                               pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInitializationData
    ///
    /// @brief  Get TF initialization data
    ///
    /// @param  pData         Pointer to TFNCLibOutputData
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL GetInitializationData(
        struct TFNcLibOutputData* pData);

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTFPerPassRegionData
    ///
    /// @brief  Get Per Pass TF Region Data
    ///
    /// @param  pUnpackedData  Pointer to the unpacked data
    /// @param  pInput         Pointer to the input data
    /// @param  passType       pass types used
    ///
    /// @return pass specific unpacked data if success or NULL otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static tf_2_0_0::tf20_rgn_dataType* GetTFPerPassRegionData(
        tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct* pUnpackedData,
        const TF20InputData*                             pInput,
        UINT32                                           passType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTFPerPassReservedData
    ///
    /// @brief  Get Per Pass TF Reserved Data
    ///
    /// @param  pReserveData   Pointer to the input reserved data
    /// @param  pInput         Pointer to the input data
    /// @param  passType       pass types used
    ///
    /// @return pass specific reserved data if success or NULL otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static tf_2_0_0::mod_tf20_pass_reserve_dataType::pass_dataStruct* GetTFPerPassReservedData(
        tf_2_0_0::mod_tf20_pass_reserve_dataType*        pReserveData,
        const TF20InputData*                             pInput,
        UINT32                                           passType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapChromatix2TFChromatix
    ///
    /// @brief  Map Chromatix Input to TF defined chromatix data
    ///
    /// @param  pReserveData   Pointer to the input chromatix reserve data
    /// @param  pUnpackedData  Pointer to the input unpacked data
    /// @param  pInput         Pointer to the input data
    /// @param  passNumMax     Maximum number of pass types used
    /// @param  pTFChromatix   Pointer to the output TF chromatix data
    ///
    /// @return true if success , false on error
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL MapChromatix2TFChromatix(
        tf_2_0_0::mod_tf20_pass_reserve_dataType*        pReserveData,
        tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct* pUnpackedData,
        const TF20InputData*                             pInput,
        UINT32                                           passNumMax,
        TF_Chromatix*                                    pTFChromatix);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyUINT32toINT32ArrayWithSize
    ///
    /// @brief  This template function copy UINT32 array to INT32 array with arraySize
    ///
    /// @param  pDestArrayName  Pointer to destination array
    /// @param  pSrcArrayName   Pointer to source array
    /// @param  arraySize       Size of array to be copied
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CopyUINT32toINT32ArrayWithSize(
        INT32*        pDestArrayName,
        const UINT32* pSrcArrayName,
        INT           arraySize);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyINT32toINT32ArrayWithSize
    ///
    /// @brief  This template function copy INT32 array to INT32 array with arraySize
    ///
    /// @param  pDestArrayName  Pointer to destination array
    /// @param  pSrcArrayName   Pointer to source array
    /// @param  arraySize       Size of array to be copied
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CopyINT32toINT32ArrayWithSize(
        INT32*       pDestArrayName,
        const INT32* pSrcArrayName,
        INT          arraySize);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyUINT32toUINT8ArrayWithSize
    ///
    /// @brief  This template function copy UINT32 array to UINT8 array with arraySize
    ///
    /// @param  pDestArrayName  Pointer to destination array
    /// @param  pSrcArrayName   Pointer to source array
    /// @param  arraySize       Size of array to be copied
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CopyUINT32toUINT8ArrayWithSize(
        UINT8*        pDestArrayName,
        const UINT32* pSrcArrayName,
        INT           arraySize);
};

#endif // TF20SETTING_H
