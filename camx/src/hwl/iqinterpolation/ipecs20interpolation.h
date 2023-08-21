// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ipecs20interpolation.h
/// @brief CS20 module tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef IPECS20INTERPOLATION_H
#define IPECS20INTERPOLATION_H

#include "cs_2_0_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

/// @brief CS20 Module Trigger List
// NOWHINE NC004c: Share code with system team
struct CS20TriggerList
{
    cs_2_0_0::chromatix_cs20Type::control_methodStruct controlType;    ///< chromatix data
    FLOAT                                              triggerDRCgain; ///< DRC Gain
    FLOAT                                              triggerHDRAEC;  ///< HDRAEC trigger
    FLOAT                                              triggerAEC;     ///< AEC trigger
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements CS20 module tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class IPECS20Interpolation
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
        CS20InputData*    pTriggerData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunInterpolation
    ///
    /// @brief  Perform the interpolation.
    ///
    /// @param  pInput Input data to the CS20 Module
    /// @param  pData  The output of the interpolation algorithem
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL RunInterpolation(
        const CS20InputData*         pInput,
        cs_2_0_0::cs20_rgn_dataType* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoInterpolation
    ///
    /// @brief  CS20 Interpolation function
    ///
    /// @param  pData1   Pointer to the input data set 1
    /// @param  pData2   Pointer to the input data set 2
    /// @param  ratio    Interpolation Ratio
    /// @param  pOutput  Pointer to the result set
    ///
    /// @return Return TRUE on success
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
    /// @brief  Interpolation Function for CS20 Module
    ///
    /// @param  pInput1 Pointer of the first set of input data
    /// @param  pInput2 Pointer of the second set of input data
    /// @param  ratio   Interpolation Ratio
    /// @param  pOutput Pointer of the output data buffer
    ///
    /// @return return TRUE if succeed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL InterpolationData(
        cs_2_0_0::cs20_rgn_dataType* pInput1,
        cs_2_0_0::cs20_rgn_dataType* pInput2,
        FLOAT                        ratio,
        cs_2_0_0::cs20_rgn_dataType* pOutput);
};
#endif // IPECS20INTERPOLATION_H
