// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  sce11interpolation.h
/// @brief SCE11 module tunning interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SCE11INTERPOLATION_H
#define SCE11INTERPOLATION_H

#include "iqcommondefs.h"
#include "iqsettingutil.h"
#include "sce_1_1_0.h"

// NOWHINE NC004c: Share code with system team
struct SCE11TriggerList
{
    sce_1_1_0::chromatix_sce11Type::control_methodStruct controlType; ///< chromatix data
    FLOAT                                                triggerAEC;  ///< AEC trigger
    FLOAT                                                triggerCCT;  ///< AEC trigger
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements SCE11 module tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class SCE11Interpolation
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunInterpolation
    ///
    /// @brief  Perform the interpolation.
    ///
    /// @param  pInput Pointer to the Input data to the Module
    /// @param  pData  Pointer to the Output of the interpolation algorithm
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL RunInterpolation(
        const SCE11InputData*           pInput,
        sce_1_1_0::sce11_rgn_dataType*  pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoInterpolation
    ///
    /// @brief  SCE11 Interpolation function
    ///
    /// @param  pData1   Pointer to the input data set 1
    /// @param  pData2   Pointer to the input data set 2
    /// @param  ratio    Interpolation Ratio
    /// @param  pOutput  Pointer to the result set
    ///
    /// @return TRUE if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL DoInterpolation(
        VOID* pData1,
        VOID* pData2,
        FLOAT ratio,
        VOID* pOutput);

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

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CCTSearchNode
    ///
    /// @brief  Search Function for CCT Nodes
    ///
    /// @param  pParentNode     Pointer to the Parent Node
    /// @param  pTriggerData    Pointer to the Trigger Value List
    /// @param  pChildNode      Pointer to the Child Node
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT CCTSearchNode(
        TuningNode* pParentNode,
        VOID*       pTriggerData,
        TuningNode* pChildNode);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InterpolationData
    ///
    /// @brief  Interpolation Function for SCE11 Module
    ///
    /// @param  pInput1 Poiner to the Input Data Set 1
    /// @param  pInput2 Pointer to the Input Data Set 2
    /// @param  ratio   Ratio Value
    /// @param  pOutput Pointer to the Output Data
    ///
    /// @return TRUE if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL InterpolationData(sce_1_1_0::sce11_rgn_dataType* pInput1,
                                  sce_1_1_0::sce11_rgn_dataType* pInput2,
                                  FLOAT                          ratio,
                                  sce_1_1_0::sce11_rgn_dataType* pOutput);
};
#endif // SCE11INTERPOLATION_H
