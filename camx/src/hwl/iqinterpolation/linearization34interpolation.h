// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  linearization34interpolation.h
/// @brief Linearization34 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef LINEARIZATION34INTERPOLATION_H
#define LINEARIZATION34INTERPOLATION_H

#include "iqcommondefs.h"
#include "iqsettingutil.h"
#include "linearization_3_4_0.h"

static const FLOAT  Linearization34MaximumLUTVal            = (1 << 14) - 1;

/// @brief Linearization34 Module trigger list
// NOWHINE NC004c: Share code with system team
struct Linearization34TriggerList
{
    linearization_3_4_0::chromatix_linearization34Type::control_methodStruct      controlType;        ///< chromatix data
    FLOAT                                                                         triggerDRCgain;     ///< DRC Gain
    FLOAT                                                                         triggerHDRAEC;      ///< HDRAEC trigger
    FLOAT                                                                         triggerLED;         ///< LED trigger
    UINT16                                                                        numberOfLED;        ///< Number of LED
    FLOAT                                                                         LEDFirstEntryRatio; ///< Ratio of Dual LED
    FLOAT                                                                         triggerAEC;         ///< AEC trigger
    FLOAT                                                                         triggerCCT;         ///< CCT trigger
    linearization_3_4_0::chromatix_linearization34Type::private_informationStruct privateInfo;        ///< Private Information
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements Linearization34 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class Linearization34Interpolation
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckUpdateTrigger
    ///
    /// @brief  Check and update the trigger condition
    ///
    /// @param  pInput  Pointer to the Input data of the latest trigger data
    /// @param  pOutput Pointer to the dependence data of this IQ module
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CheckUpdateTrigger(
        ISPIQTriggerData*       pInput,
        Linearization34IQInput* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InterpSeg
    ///
    /// @brief  Interpolation Segment Function
    ///
    /// @param  pX       Pointer to the input data set 1
    /// @param  pY       Pointer to the input data set 2
    /// @param  x0       Input value
    /// @param  maxValue Input Max value
    ///
    /// @return Interpolated float value.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static FLOAT InterpSeg(
        FLOAT* pX,
        FLOAT* pY,
        FLOAT  x0,
        FLOAT  maxValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SegmentInterpolate
    ///
    /// @brief  Linearization34 Segment Interpolation function
    ///
    /// @param  pRLutP              Pointer to the input1 data set of rLutP
    /// @param  pRLutBase           Pointer to the input1 data set of RLutBase
    /// @param  pInput2RLutP        Pointer to the input2 data set of rLutP
    /// @param  pInput2RLutBase     Pointer to the input2 data set of RLutBase
    /// @param  pModOutputRLutP     Pointer to the output data set of rLutP
    /// @param  pModOutputRLutBase  Pointer to the output data set of rLutBase.
    /// @param  ratio               Interpolation Ratio
    /// @param  maxValue            Input max Value.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void SegmentInterpolate(
        FLOAT* pRLutP,
        FLOAT* pRLutBase,
        FLOAT* pInput2RLutP,
        FLOAT* pInput2RLutBase,
        FLOAT* pModOutputRLutP,
        FLOAT* pModOutputRLutBase,
        FLOAT  ratio,
        FLOAT  maxValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunInterpolation
    ///
    /// @brief  Perform the interpolation.
    ///
    /// @param  pInput Pointer to input data of the Linearization34 Module
    /// @param  pData  Pointer to output of the interpolation algorithm
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL RunInterpolation(
        const Linearization34IQInput*                      pInput,
        linearization_3_4_0::linearization34_rgn_dataType* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateDelta
    ///
    /// @brief  CalculateDelta Function for Linearization34 Module
    ///
    /// @param  pLutP           Pointer to the LUT variable
    /// @param  pLutBase        Pointer to LUT base
    /// @param  pLutDelta       Pointer to the LUT Delta
    /// @param  pedestalEnable  Pedestal is enabled or not
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void CalculateDelta(
        FLOAT* pLutP,
        FLOAT* pLutBase,
        FLOAT* pLutDelta,
        BOOL   pedestalEnable);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoInterpolation
    ///
    /// @brief  Linearization34 Interpolation function
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
    /// LEDSearchNode
    ///
    /// @brief  Search Function for LED Nodes
    ///
    /// @param  pParentNode     Pointer to the Parent Node
    /// @param  pTriggerData    Pointer to the Trigger Value List
    /// @param  pChildNode      Pointer to the Child Node
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT LEDSearchNode(
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
    /// @brief  Interpolation Function for Linearization34 Module
    ///
    /// @param  pInput1      Pointer to input1
    /// @param  pInput2      Pointer to input2
    /// @param  ratio        Interpolation ratio to be used
    /// @param  pOutput      Pointer to interpolated output based on input1 and input2
    ///
    /// @return BOOL         return TRUE on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL InterpolationData(
        linearization_3_4_0::linearization34_rgn_dataType* pInput1,
        linearization_3_4_0::linearization34_rgn_dataType* pInput2,
        FLOAT                                              ratio,
        linearization_3_4_0::linearization34_rgn_dataType* pOutput);
};

#endif // LINEARIZATION34INTERPOLATION_H
