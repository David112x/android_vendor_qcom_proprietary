// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  gra10interpolation.h
/// @brief GRA0 module tunning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GRA10INTERPOLATION_H
#define GRA10INTERPOLATION_H

#include "gra_1_0_0.h"
#include "gra10setting.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

// NOWHINE NC004c: Share code with system team
struct GRA10TriggerList
{
    gra_1_0_0::chromatix_gra10Type::control_methodStruct controlType;          ///< chromatix data
    FLOAT                                                preScaleRatioTrigger; ///< PreScaleRatioTrigger Mode
    FLOAT                                                triggerAEC;           ///< AEC trigger
    FLOAT                                                triggerCCT;           ///< CCT trigger
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements GRA10 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class GRA10Interpolation
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
        const ISPIQTriggerData*   pInput,
        GRA10IQInput*       pTriggerData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunInterpolation
    ///
    /// @brief  Perform the interpolation.
    ///
    /// @param   pInput Pointer to iput data to the GRA10 Module
    /// @param   pData  Pointer to output of the interpolation algorithem
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL RunInterpolation(
        const GRA10IQInput*            pInput,
        gra_1_0_0::gra10_rgn_dataType* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoInterpolation
    ///
    /// @brief    GRA10    Interpolation function
    ///
    /// @param    pData1     Pointer to the input data set 1
    /// @param    pData2     Pointer to the input data set 2
    /// @param    ratio      Interpolation Ratio
    /// @param    pOutput    Pointer to the result set
    ///
    /// @return   TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL DoInterpolation(
        VOID* pData1,
        VOID* pData2,
        FLOAT ratio,
        VOID* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PreScaleRatioSearchNode
    ///
    /// @brief    Search Function for AEC Nodes
    ///
    /// @param    pParentNode           Pointer to the Parent Node
    /// @param    pTriggerData          Pointer to the Trigger Value List
    /// @param    pChildNode            Pointer to the Chile Node
    ///
    /// @return   Number of Child Nodes
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT PreScaleRatioSearchNode(
        TuningNode* pParentNode,
        VOID*       pTriggerData,
        TuningNode* pChildNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CCTSearchNode
    ///
    /// @brief    Search Function for AEC Nodes
    ///
    /// @param    pParentNode         Pointer to the Parent Node
    /// @param    pTriggerData        Pointer to the Trigger Value List
    /// @param    pChildNode          Pointer to the Chile Node
    ///
    /// @return   Number of Child Nodes
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT CCTSearchNode(
        TuningNode* pParentNode,
        VOID*       pTriggerData,
        TuningNode* pChildNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AECSearchNode
    ///
    /// @brief    Search Function for AEC Nodes
    ///
    /// @param    pParentNode       Pointer to the Parent Node
    /// @param    pTriggerData      Pointer to the Trigger Value List
    /// @param    pChildNode        Pointer to the Chile Node
    ///
    /// @return   Number of Child Nodes
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT AECSearchNode(
        TuningNode* pParentNode,
        VOID*       pTriggerData,
        TuningNode* pChildNode);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InterpolationData
    ///
    /// @brief    Interpolation Function for GRA10 Module
    ///
    /// @param    pInput1 Pointer to input1
    /// @param    pInput2 Pointer to input2
    /// @param    ratio   Interpolation ratio to be used
    /// @param    pOutput Pointer to interpolated output based on input1 and input2
    ///
    /// @return VOID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID InterpolationData(
        gra_1_0_0::gra10_rgn_dataType* pInput1,
        gra_1_0_0::gra10_rgn_dataType* pInput2,
        FLOAT                          ratio,
        gra_1_0_0::gra10_rgn_dataType* pOutput);

};
#endif // GRA10INTERPOLATION_H
