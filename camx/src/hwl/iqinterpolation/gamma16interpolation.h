// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  gamma16interpolation.h
/// @brief Gamma16 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GAMMA16INTERPOLATION_H
#define GAMMA16INTERPOLATION_H

#include "gamma_1_6_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

/// @brief Trigger information from 3A
// NOWHINE NC004c: Share code with system team
struct Gamma16TriggerList
{
    gamma_1_6_0::chromatix_gamma16Type::control_methodStruct      controlType;        ///< chromatix data
    FLOAT                                                         triggerDRCgain;     ///< DRC Gain
    FLOAT                                                         triggerHDRAEC;      ///< HDRAEC trigger
    FLOAT                                                         triggerLED;         ///< LED trigger
    UINT16                                                        numberOfLED;        ///< Number of LED
    FLOAT                                                         LEDFirstEntryRatio; ///< Dual LED Ratio
    FLOAT                                                         triggerAEC;         ///< AEC trigger
    FLOAT                                                         triggerCCT;         ///< CCT trigger
    gamma_1_6_0::chromatix_gamma16Type::private_informationStruct privateInfo;        ///< Private Information
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements Gamma16 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class Gamma16Interpolation
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
        Gamma16InputData* pTriggerData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunInterpolation
    ///
    /// @brief  Perform the interpolation.
    ///
    /// @param  pInput Pointer to input data to the Gamma16 Module
    /// @param  pData  Pointer to output of the interpolation algorithem
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL RunInterpolation(
        const Gamma16InputData*                    pInput,
        gamma_1_6_0::mod_gamma16_channel_dataType* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoInterpolation
    ///
    /// @brief  Gamma16 Interpolation function
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
    /// @brief  Interpolation Function for Gamma16 Module
    ///
    /// @param  pInput1     Pointer to input1
    /// @param  pInput2     Pointer to input2
    /// @param  ratio       Interpolation ratio to be used
    /// @param  pOutput     Pointer to interpolated output based on input1 and input2
    ///
    /// @return BOOL        return TRUE on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL InterpolationData(
        gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct* pInput1,
        gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct* pInput2,
        FLOAT                                                  ratio,
        gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGammaTables
    ///
    /// @brief  Get Gamma Tables in right order with different Gamma Channel Types
    ///
    /// @param  pInput     Pointer to input CCT data structure
    /// @param  pOutTable  Pointer to 3 input channel Gamma tables
    ///
    /// @return TRUE if succeed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL GetGammaTables(
        gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct* pInput,
        FLOAT*                                                 pOutTable[GammaLUTMax]);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyCCTData
    ///
    /// @brief  Copy Input CCT data structure to Output CCT data structure
    ///
    /// @param  pInput     Pointer to input CCT data structure
    /// @param  pOutput    Pointer to output CCT data structure
    ///
    /// @return TRUE if succeed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CopyCCTData(
        gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct* pInput,
        gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct* pOutput);
};

#endif // GAMMA16INTERPOLATION_H
