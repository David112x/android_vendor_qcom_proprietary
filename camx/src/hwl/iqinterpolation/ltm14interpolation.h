// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ltm14interpolation.h
/// @brief LTM14 module tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef LTM14INTERPOLATION_H
#define LTM14INTERPOLATION_H
#include "ltm_1_4_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

/// @brief LTM trigger list
// NOWHINE NC004c: Share code with system team
struct LTM14TriggerList
{
    ltm_1_4_0::chromatix_ltm14Type::control_methodStruct controlType;    ///< chromatix data
    FLOAT                                                triggerDRCgain; ///< DRC Gain
    FLOAT                                                triggerHDRAEC;  ///< HDRAEC trigger
    FLOAT                                                triggerAEC;     ///< AEC trigger
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements LTM14 module tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class LTM14Interpolation
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
        LTM14InputData*   pTriggerData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunInterpolation
    ///
    /// @brief  Perform the interpolation.
    ///
    /// @param  pInput Pointer to input data to the LTM14 Module
    /// @param  pData  Pointer to output of the interpolation algorithm
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL RunInterpolation(
        const LTM14InputData*          pInput,
        ltm_1_4_0::ltm14_rgn_dataType* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoInterpolation
    ///
    /// @brief  LTM14 Interpolation function
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
    /// DRCGainSearchNode
    ///
    /// @brief  Search Function for DRCGain Nodes
    ///
    /// @param  pParentNode     Pointer to the Parent Node
    /// @param  pTriggerData    Pointer to the Trigger Value List
    /// @param  pChildNode      Pointer to the Child Node
    ///
    /// @return Number of Search Node
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT DRCGainSearchNode(
        TuningNode* pParentNode,
        VOID*       pTriggerData,
        TuningNode* pChildNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HDRAECSearchNode
    ///
    /// @brief  Search Function for HDRAEC Nodes
    ///
    /// @param  pParentNode     Pointer to the Parent Node
    /// @param  pTriggerData    Pointer to the Trigger Value List
    /// @param  pChildNode      Pointer to the Child Node
    ///
    /// @return Number of Search Node
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
    /// @return Number of Search Node
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT AECSearchNode(
        TuningNode* pParentNode,
        VOID*       pTriggerData,
        TuningNode* pChildNode);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InterpolationData
    ///
    /// @brief  Interpolation Function for LTM14 Module
    ///
    /// @param  pInput1  Pointer for the Region1 input data
    /// @param  pInput2  Pointer for the Region2 input data
    /// @param  ratio    Ratio for the interpolation
    /// @param  pOutput  Pointer for the Final output
    ///
    /// @return BOOL     Return TRUE on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL InterpolationData(
        ltm_1_4_0::ltm14_rgn_dataType* pInput1,
        ltm_1_4_0::ltm14_rgn_dataType* pInput2,
        FLOAT                          ratio,
        ltm_1_4_0::ltm14_rgn_dataType* pOutput);
};

#endif // LTM14INTERPOLATION_H
