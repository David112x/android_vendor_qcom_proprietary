// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ifehdr20interpolation.h
/// @brief HDR20 module tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef IFEHDR20INTERPOLATION_H
#define IFEHDR20INTERPOLATION_H
#include "hdr_2_0_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

/// @brief HDR20 Trigger List
// NOWHINE NC004c: Share code with system team
struct HDR20TriggerList
{
    hdr_2_0_0::chromatix_hdr20Type::control_methodStruct controlType;   ///< chromatix data
    FLOAT                                                triggerHDRAEC; ///< HDRAEC trigger
    FLOAT                                                triggerAEC;    ///< AEC trigger
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements HDR20 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class IFEHDR20Interpolation
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckUpdateTrigger
    ///
    /// @brief  Check and update the Trigger Status
    ///
    /// @param  pInput       Pointer to the input data from Node
    /// @param  pTriggerData Pointer to the Trigger data set of this IQ module
    ///
    /// @return TRUE if trigger data has changed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CheckUpdateTrigger(
        ISPIQTriggerData* pInput,
        HDR20InputData*   pTriggerData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunInterpolation
    ///
    /// @brief  Perform the interpolation.
    ///
    /// @param  pInput Input data to the HDR20 Module
    /// @param  pData  The output of the interpolation algorithem
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL RunInterpolation(
        const HDR20InputData*          pInput,
        hdr_2_0_0::hdr20_rgn_dataType* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoInterpolation
    ///
    /// @brief  HDR20 Interpolation function
    ///
    /// @param  pData1   Pointer to the input data set 1
    /// @param  pData2   Pointer to the input data set 2
    /// @param  ratio    Interpolation Ratio
    /// @param  pOutput  Pointer to the result set
    ///
    /// @return BOOL     return TRUE on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL DoInterpolation(
        VOID* pData1,
        VOID* pData2,
        FLOAT ratio,
        VOID* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HDRAECSearchNode
    ///
    /// @brief  Search Function for HDRAEC Nodes
    ///
    /// @param  pParentNode     Pointer to the Parent Node
    /// @param  pTriggerData    Pointer to the Trigger Value List
    /// @param  pChildNode      Pointer to the Child Node
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT HDRAECSearchNode(
        TuningNode* pParentNode,
        VOID*       pTriggerData,
        TuningNode* pChildNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AECSearchNode
    ///
    /// @brief  Search Function for AEC Nodes
    ///
    /// @param  pParentNode     Pointer to the Parent Node
    /// @param  pTriggerData    Pointer to the Trigger Value List
    /// @param  pChildNode      Pointer to the Child Node
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT AECSearchNode(
        TuningNode* pParentNode,
        VOID*       pTriggerData,
        TuningNode* pChildNode);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InterpolationData
    ///
    /// @brief  Interpolation Function for HDR20 Module
    ///
    /// @param  pInput1  Pointer to input1
    /// @param  pInput2  Pointer to input2
    /// @param  ratio    Interpolation ratio to be used
    /// @param  pOutput  Interpolated output based on input1 and input2
    ///
    /// @return return TRUE if succeed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL InterpolationData(
        hdr_2_0_0::hdr20_rgn_dataType* pInput1,
        hdr_2_0_0::hdr20_rgn_dataType* pInput2,
        FLOAT                          ratio,
        hdr_2_0_0::hdr20_rgn_dataType* pOutput);
};
#endif // IFEHDR20INTERPOLATION_H
