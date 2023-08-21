// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ifelinearization33interpolation.h
/// @brief Linearization33 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef IFELINEARIZATION33INTERPOLATION_H
#define IFELINEARIZATION33INTERPOLATION_H

#include "iqcommondefs.h"
#include "iqsettingutil.h"
#include "linearization_3_3_0.h"

/// @brief Linearization33 Module Trigger List
// NOWHINE NC004c: Share code with system team
struct Linearization33TriggerList
{
    linearization_3_3_0::chromatix_linearization33Type::control_methodStruct      controlType;        ///< chromatix data
    FLOAT                                                                         triggerDRCgain;     ///< DRC Gain
    FLOAT                                                                         triggerHDRAEC;      ///< HDRAEC trigger
    FLOAT                                                                         triggerLED;         ///< LED trigger
    UINT16                                                                        numberOfLED;        ///< Number of LED
    FLOAT                                                                         LEDFirstEntryRatio; ///< Dual LED Ratio
    FLOAT                                                                         triggerAEC;         ///< AEC trigger
    FLOAT                                                                         triggerCCT;         ///< CCT trigger
    linearization_3_3_0::chromatix_linearization33Type::private_informationStruct privateInfo;        ///< Private Information
};

static const FLOAT  Linearization33MaximumLUTVal = (1 << 14) - 1;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements Linearization33 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class IFELinearization33Interpolation
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
        ISPIQTriggerData*         pInput,
        Linearization33InputData* pTriggerData);

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
    /// @brief  Linearization33 Segment Interpolation function
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
    /// @param  pInput Input data to the Linearization33 Module
    /// @param  pData  The output of the interpolation algorithem
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL RunInterpolation(
        const Linearization33InputData*                    pInput,
        linearization_3_3_0::linearization33_rgn_dataType* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoInterpolation
    ///
    /// @brief  Linearization33 Interpolation function
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
    /// @brief  Interpolation Function for Linearization33 Module
    ///
    /// @param  pInput1  Pointer for the Region1 input data
    /// @param  pInput2  Pointer for the Region2 input data
    /// @param  ratio    Raio for the interpolation
    /// @param  pOutput  Pointer for the Final output
    ///
    /// @return BOOL     return TRUE on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL InterpolationData(
        linearization_3_3_0::linearization33_rgn_dataType* pInput1,
        linearization_3_3_0::linearization33_rgn_dataType* pInput2,
        FLOAT                                              ratio,
        linearization_3_3_0::linearization33_rgn_dataType* pOutput);
};
#endif // IFELINEARIZATION33INTERPOLATION_H
