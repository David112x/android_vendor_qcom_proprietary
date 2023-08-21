// NOWHINE NC009 <- Shared file with system team so uses non-Camx file nameing
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  upscale20interpolation.h
/// @brief upscale20 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef UPSCALE20INTERPOLATION_H
#define UPSCALE20INTERPOLATION_H

#include "iqcommondefs.h"
#include "iqsettingutil.h"
#include "upscale_2_0_0.h"

/// @brief Trigger List for Upscale v20 Module
// NOWHINE NC004c: Share code with system team
struct Upscale20TriggerList
{
    upscale_2_0_0::chromatix_upscale20Type::control_methodStruct controlType;            ///< chromatix data
    FLOAT                                                        triggerTotalScaleRatio; ///< TotalScaleRatio trigger
    FLOAT                                                        triggerAEC;             ///< AEC trigger
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements Upscale20 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class Upscale20Interpolation
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunInterpolation
    ///
    /// @brief  Perform the interpolation.
    ///
    /// @param  pInput Input data to the Upscale20 Module
    /// @param  pData  The output of the interpolation algorithem
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL RunInterpolation(
        const Upscale20InputData*              pInput,
        upscale_2_0_0::upscale20_rgn_dataType* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoInterpolation
    ///
    /// @brief  Upscale20 Interpolation function
    ///
    /// @param  pData1   Pointer to the input data set 1
    /// @param  pData2   Pointer to the input data set 2
    /// @param  ratio    Interpolation Ratio
    /// @param  pOutput  Pointer to the result set
    ///
    /// @return BOOL     Return TRUE on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL DoInterpolation(
        VOID* pData1,
        VOID* pData2,
        FLOAT ratio,
        VOID* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TotalScaleRatioSearchNode
    ///
    /// @brief  Search Function for TotalScaleRatio Nodes
    ///
    /// @param  pParentNode     Pointer to the Parent Node
    /// @param  pTriggerData    Pointer to the Trigger Value List
    /// @param  pChildNode      Pointer to the Child Node
    ///
    /// @return Number of Child Node
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT TotalScaleRatioSearchNode(
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
    /// @return Number of Child Node
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT AECSearchNode(
        TuningNode* pParentNode,
        VOID*       pTriggerData,
        TuningNode* pChildNode);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InterpolationData
    ///
    /// @brief  Interpolation Function for Upscale20 Module
    ///
    /// @param  pInput1     Pointer to input1
    /// @param  pInput2     Pointer to input2
    /// @param  ratio       Interpolation ratio to be used
    /// @param  pOutput     Interpolated output based on input1 and input2
    ///
    /// @return VOID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID InterpolationData(
        upscale_2_0_0::upscale20_rgn_dataType* pInput1,
        upscale_2_0_0::upscale20_rgn_dataType* pInput2,
        FLOAT                                  ratio,
        upscale_2_0_0::upscale20_rgn_dataType* pOutput);
};

#endif // UPSCALE20INTERPOLATION_H
