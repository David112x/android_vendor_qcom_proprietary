// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  anr10interpolation.h
/// @brief ANR10 module tunning interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef ANR10INTERPOLATION_H
#define ANR10INTERPOLATION_H

#include "iqcommondefs.h"
#include "iqsettingutil.h"
#include "anr_1_0_0.h"

// constants for table sizes
static const UINT32 LUMA_FILTER_LUT_Y_SIZE           = 17;    ///< luma filter Y threshold modifier LUT
static const UINT32 LUMA_FILTER_LUT_UV_SIZE          = 17;    ///< luma filter UV threshold modifier LUT
static const UINT32 CHROMA_FILTER_LUT_Y_SIZE         = 17;    ///< chroma filter Y threshold modifier LUT
static const UINT32 CHROMA_FILTER_LUT_UV_SIZE        = 17;    ///< chroma filter UV threshold modifier LUT
static const UINT32 STRENGTH_MODIFIER_RADIUS_BLEND   = 17;    ///< strength modifier radius blend lut
static const UINT32 DETECT_ANGLE_START               = 5;
static const UINT32 DETECT_ANGLE_END                 = 5;
static const UINT32 DETECT_CHROMATICITY_START        = 5;
static const UINT32 DETECT_CHROMATICITY_END          = 5;
static const UINT32 DETECT_LUMA_START                = 5;
static const UINT32 DETECT_LUMA_END                  = 5;
static const UINT32 BOUNDARY_WEIGHT                  = 5;
static const UINT32 TRANSITION_RATIO                 = 5;
static const UINT32 LUMA_FILTER_THR_SCALE            = 5;
static const UINT32 LUMA_FILTER_OFFSET               = 5;
static const UINT32 CHROMA_FILTER_THR_SCALE          = 5;
static const UINT32 CHROMA_FILTER_OFFSET             = 5;
static const UINT32 LUMA_DCBLEND2_WEIGHT             = 5;
static const UINT32 CHROMA_DCBLEND2_WEIGHT           = 5;
static const UINT32 LUMA_FLAT_KERNEL_BLEND_WEIGHT    = 5;
static const UINT32 Y_THR_PER_Y                      = 17;
static const UINT32 Y_THR_PER_UV                     = 8;
static const UINT32 U_THR_PER_Y                      = 17;
static const UINT32 U_THR_PER_UV                     = 8;
static const UINT32 V_THR_PER_Y                      = 17;
static const UINT32 V_THR_PER_UV                     = 8;
static const UINT32 DCBLEN2_LUMA_STRENGTH_FUNCTION   = 5;
static const UINT32 DCBLEN2_CHROMA_STRENGTH_FUNCTION = 6;

/// @brief Trigger List for ANR 10 Module
// NOWHINE NC004c: Share code with system team
struct ANR10TriggerList
{
    anr_1_0_0::chromatix_anr10Type::control_methodStruct controlType;           ///< chromatix data
    FLOAT                                              triggerLensPosition;   ///< Lens Position trigger
    FLOAT                                              triggerLensZoom;       ///< Lens Zoom trigger
    FLOAT                                              triggerPostScaleRatio; ///< Post Scale Ratio trigger
    FLOAT                                              triggerPreScaleRatio;  ///< Pre Scale Ratio trigger
    FLOAT                                              triggerDRCgain;        ///< DRC Gain trigger
    FLOAT                                              triggerHDRAEC;         ///< HDRAEC trigger
    FLOAT                                              triggerAEC;            ///< AEC trigger
    FLOAT                                              triggerCCT;            ///< CCT trigger
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements ANR10 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class ANR10Interpolation
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
        ANR10InputData*   pTriggerData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunInterpolation
    ///
    /// @brief  Perform the interpolation.
    ///
    /// @param  pInput Input data to the ANR10 Module
    /// @param  pData  The output of the interpolation algorithem
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL RunInterpolation(
        const ANR10InputData*                             pInput,
        anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoInterpolation
    ///
    /// @brief  ANR10 Interpolation function
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
    /// LensPositionSearchNode
    ///
    /// @brief  Search Function for Lens Position Nodes
    ///
    /// @param  pParentNode     Pointer to the Parent Node
    /// @param  pTriggerData    Pointer to the Trigger Value List
    /// @param  pChildNode      Pointer to the Child Node
    ///
    /// @return Number of Child Node
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT LensPositionSearchNode(
        TuningNode* pParentNode,
        VOID*       pTriggerData,
        TuningNode* pChildNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LensZoomSearchNode
    ///
    /// @brief  Search Function for Lens Zoom Nodes
    ///
    /// @param  pParentNode     Pointer to the Parent Node
    /// @param  pTriggerData    Pointer to the Trigger Value List
    /// @param  pChildNode      Pointer to the Child Node
    ///
    /// @return Number of Child Node
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT LensZoomSearchNode(
        TuningNode* pParentNode,
        VOID*       pTriggerData,
        TuningNode* pChildNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostScaleRatioSearchNode
    ///
    /// @brief  Search Function for Post Scale Ratio Nodes
    ///
    /// @param  pParentNode     Pointer to the Parent Node
    /// @param  pTriggerData    Pointer to the Trigger Value List
    /// @param  pChildNode      Pointer to the Child Node
    ///
    /// @return Number of Child Node
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT PostScaleRatioSearchNode(
        TuningNode* pParentNode,
        VOID*       pTriggerData,
        TuningNode* pChildNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PreScaleRatioSearchNode
    ///
    /// @brief  Search Function for Pre Scale Ratio Nodes
    ///
    /// @param  pParentNode     Pointer to the Parent Node
    /// @param  pTriggerData    Pointer to the Trigger Value List
    /// @param  pChildNode      Pointer to the Child Node
    ///
    /// @return Number of Child Node
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT PreScaleRatioSearchNode(
        TuningNode* pParentNode,
        VOID*       pTriggerData,
        TuningNode* pChildNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DRCGainSearchNode
    ///
    /// @brief  Search Function for DRCGain Nodes
    ///
    /// @param  pParentNode     Pointer to the Parent Node
    /// @param  pTriggerData    Pointer to the Trigger Value List
    /// @param  pChildNode      Pointer to the Child Node
    ///
    /// @return Number of Child Node
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
    /// @return Number of Child Node
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
    /// @return Number of Child Node
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
    /// @return Number of Child Node
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT CCTSearchNode(
        TuningNode* pParentNode,
        VOID*       pTriggerData,
        TuningNode* pChildNode);

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InterpolationData
    ///
    /// @brief  Interpolation Function for ANR10 Module
    ///
    /// @param  pInput1   Pointer to input1
    /// @param  pInput2   Pointer to input2
    /// @param  ratio     Interpolation ratio to be used
    /// @param  pOutput   Interpolated output based on input1 and input2
    ///
    /// @return TRUE if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL InterpolationData(
        anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct* pInput1,
        anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct* pInput2,
        FLOAT                                            ratio,
        anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct* pOutput);
};
#endif // ANR10INTERPOLATION_H
