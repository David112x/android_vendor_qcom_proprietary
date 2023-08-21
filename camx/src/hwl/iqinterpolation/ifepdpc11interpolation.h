// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ifepdpc11interpolation.h
/// @brief PDPC11 module tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef IFEPDPC11INTERPOLATION_H
#define IFEPDPC11INTERPOLATION_H

#include "iqcommondefs.h"
#include "iqsettingutil.h"
#include "pdpc_1_1_0.h"

/// @brief Trigger List for PDPC11 Module
// NOWHINE NC004c: Share code with system team
struct PDPC11TriggerList
{
    pdpc_1_1_0::chromatix_pdpc11Type::control_methodStruct  controlType;    ///< chromatix data
    FLOAT                                                   triggerDRCgain; ///< DRC Gain
    FLOAT                                                   triggerHDRAEC;  ///< HDRAEC trigger
    FLOAT                                                   triggerAEC;     ///< AEC trigger
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements PDPC11 module tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class IFEPDPC11Interpolation
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
        PDPC11InputData*  pTriggerData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunInterpolation
    ///
    /// @brief  Perform the interpolation.
    ///
    /// @param  pInput Input data to the PDPC11 Module
    /// @param  pData  The output of the interpolation algorithem
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL RunInterpolation(
        const PDPC11InputData*               pInput,
        pdpc_1_1_0::pdpc11_rgn_dataType*     pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoInterpolation
    ///
    /// @brief  PDPC11 Interpolation function
    ///
    /// @param  pData1   Pointer to the input data set 1
    /// @param  pData2   Pointer to the input data set 2
    /// @param  ratio    Interpolation Ratio
    /// @param  pOutput  Pointer to the result set
    ///
    /// @return BOOl     return TRUE on success
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
    /// @return TRUE for success
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
    /// @brief  Interpolation Function for PDPC11 Module
    ///
    /// @param  pInput1  Input Region 1
    /// @param  pInput2  Input Region 2
    /// @param  ratio    Ratio for interpolation
    /// @param  pOutput  Output after interpolation
    ///
    /// @return BOOL     return TRUE on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL InterpolationData(
        pdpc_1_1_0::pdpc11_rgn_dataType* pInput1,
        pdpc_1_1_0::pdpc11_rgn_dataType* pInput2,
        FLOAT                            ratio,
        pdpc_1_1_0::pdpc11_rgn_dataType* pOutput);

};

#endif // IFEPDPC11INTERPOLATION_H
