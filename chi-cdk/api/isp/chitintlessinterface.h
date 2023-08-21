////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chitintlessinterface.h
///
/// @brief Qualcomm Technologies, Inc. Tintless Algorithm Interface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHITINTLESSINTERFACE_H
#define CHITINTLESSINTERFACE_H

#include "camxcdktypes.h"

#include "chiispstatsdefs.h"

#ifdef __cplusplus

extern "C"
{
#endif // __cplusplus

/// @brief Data structures that are exposed to OEM must be packed to have the expected layout.
#pragma pack(push, CDK_PACK_SIZE)

/// @brief Number of Tintless threshold entries
#define TINTLESS_THRESHOLD_ENTRIES 16
#define MAX_STATS_NUM              3072 ///< Maximum nuber of stats entries 64 x 48, but tintless algo support only 32 x 24
#define ROLLOFF_SIZE               221  ///< 17 x 13 entries per mesh table

// This is used by the new ALSC algo for output.
#define ALSC_MESH_PT_H             17
#define ALSC_MESH_PT_V             13
#define ALSC_BUFFER_LIST_SIZE      16

static const CHAR* pTintlessLibName  = "libcamxtintlessalgo";
static const CHAR* pTintlessFuncName = "CreateTintless";
static const CHAR* pAlscFuncName     = "CreateALSC";

/// @brief Represents stats configuration parameters passed to ITintlessAlgorithm::Configure()
struct StatsConfigParams
{
    UINT32            camifWidth;                     ///< Input image width
    UINT32            camifHeight;                    ///< Input image Height
    UINT32            statsRegionWidth;               ///< Region width of stats
    UINT32            statsRegionHeight;              ///< Region height of stats
    UINT32            statsNumberOfHorizontalRegions; ///< Total number of horizontal regions
    UINT32            statsNumberOfVerticalRegions;   ///< Total number of vertical regions
    UINT32            postBayer;                      ///< HRDBE/TintlessBG postBayer = 0/ AWBBG postBayer = 1
    UINT32            saturationLimit[ChannelIndexCount];                ///< Saturation limit for stats
    UINT32            statsBitDepth;                  ///< Bit depth of stats
};

/// @brief Represents LSC configuration parameters
struct LSCConfigParams
{
    UINT32 LSCTableWidth;              ///< Mesh rolloff table width
    UINT32 LSCTableHeight;             ///< Mesh rolloff table height
    UINT32 LSCSubgridWidth;            ///< Mesh rolloff subgrid width
    UINT32 LSCSubgridHeight;           ///< Mesh rolloff subgrid height
    UINT32 numberOfLSCSubgrids;        ///< Number of mesh rolloff sub grids
    UINT32 LSCSubgridHorizontalOffset; ///< Sub grid horizontal offset
    UINT32 LSCSubgridVerticalOffset;   ///< Sub grid vertical offset
};

/// @brief Represents Tintless configuration parameters
struct TintlessConfigParams
{
    UINT8 tintlessCorrectionStrength;                    ///< Tintless Algorithm Correction strength
    UINT8 tintlessThreshold[TINTLESS_THRESHOLD_ENTRIES]; ///< Tintless Threshold
    UINT8 tintAcuuracy;                                  ///< Tint correction accuracy
    UINT8 updateDelay;                                   ///< Tintless update Delay
    FLOAT tracePercentage;                               ///< Tintless trace percentage
    FLOAT centerWeight;                                  ///< Tintless correction center weight
    FLOAT cornerWeight;                                  ///< Tintless correction corner weight
    UINT8 applyTemporalFiltering;                        ///< 1 - apply Temporal Filtering 0 - dont apply
};

/// @brief Tintless Mesh rolloff Table
struct TintlessRolloffTable
{
    UINT16 TintlessRoloffTableSize; ///< Tintless Rolloff table size
    FLOAT* RGain;                   ///< R Channel gains for Tintless table
    FLOAT* GRGain;                  ///< Gr Channel gains for Tintless table
    FLOAT* GBGain;                  ///< Gb Channel gains for Tintless table
    FLOAT* BGain;                   ///< B Channel gains for Tintless table
};

/// @brief Tintless Config
struct TintlessConfig
{
    LSCConfigParams      rolloffConfig;
    StatsConfigParams    statsConfig;
    TintlessConfigParams tintlessParamConfig;
};

struct ALSCHelperParam {        //parameters to be tuned
    BOOL    ALSC_enable;
    UINT32  threshold_highlight;
    UINT32  threshold_lowlight;
    UINT32  c_r;
    UINT32  c_g;
    UINT32  c_b;
    UINT32  c_max;
    UINT32  adaptive_gain_low;
    UINT32  adaptive_gain_high;
    UINT32  lowlight_gain_strength;
    UINT32  highlight_gain_strength;
    UINT32* ALSCbufferList[ALSC_BUFFER_LIST_SIZE];
};

struct ALSCHelperOutput {       //output stats
    float nGrid_gain[ALSC_MESH_PT_V][ALSC_MESH_PT_H];   //keep floating point
    float nGrid_mean[ALSC_MESH_PT_V][ALSC_MESH_PT_H];
};

struct AWBStatsSizeInput {   //AWB stats size info
    UINT16 nBG_Grid_H;
    UINT16 nBG_Grid_V;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Interface for Tintless algorithm.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TintlessAlgorithmProcess
    ///
    /// @brief  Process the Input Table and generate the Output table
    ///
    /// @param  pChiTintlessAlgo  Pointer to the CHI tintless interface
    /// @param  pTintlessConfig   Pointer to Tintless Algo Config.
    /// @param  pTintlessStats    Pointer to the Tintless Stats data
    /// @param  PInputTable       Pointer to the Input Rolloff Table
    /// @param  POutputTable      Pointer to the Output Rolloff Table
    ///
    /// @return CDKResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult(*TintlessAlgorithmProcess)(
        VOID*                              pChiTintlessAlgo,
        const TintlessConfig*              pTintlessConfig,
        const ParsedTintlessBGStatsOutput* pTintlessStats,
        TintlessRolloffTable*              pInputTable,
        TintlessRolloffTable*              pOutputTable);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TintlessDestroy
    ///
    /// @brief  This method destroys the derived instance of the interface
    ///
    /// @param  pChiTintlessAlgo  Pointer to the CHI tintless interface
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID(*TintlessDestroy)(VOID* pChiTintlessAlgo);

    VOID* pTintlessAlgoResource; ///< Tintless Algorithm Resource
} CHITintlessAlgorithm;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CreateTintless
///
/// @brief  This function pointer to method of creating a handle of CHITintlessAlgorithm.
///
/// @param  ppCHITintlessAlgorithm  Pointer to pointer of the created CHITintlessAlgorithm instance
///
/// @return CDKResultSuccess upon success.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*CREATETINTLESS)(
    CHITintlessAlgorithm** ppCHITintlessAlgorithm);



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Interface for ALSC algorithm.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ALSCAlgorithmProcess
    ///
    /// @brief  Process the Input Table and generate the Output table
    ///
    /// @param  pChiALSCAlgo    Pointer to the CHI ALSC interface
    /// @param  pStatsAWB       Pointer to Parsed AWB stats
    /// @param  pAlscParam      Pointer to ALSC configuration
    /// @param  awbSize         AWB stats size input
    /// @param  pAlscOutput     Pointer to the ALSC Output
    ///
    /// @return CDKResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult(*ALSCAlgorithmProcess)(
        VOID*                     pChiALSCAlgo,
        ParsedAWBBGStatsOutput*   pStatsAWB,
        ALSCHelperParam*          pAlscParam,
        AWBStatsSizeInput         awbSize,
        ALSCHelperOutput*         pAlscOutput
        );

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ALSCDestroy
    ///
    /// @brief  This method destroys the derived instance of the interface
    ///
    /// @param  pChiALSCAlgo  Pointer to the CHI ALSC interface
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID(*ALSCDestroy)(VOID* pChiALSCAlgo);

    VOID* pALSCAlgoResource; ///< ALSC Algorithm Resource
} CHIALSCAlgorithm;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CreateALSC
///
/// @brief  This function pointer to method of creating a handle of CHIALSCAlgorithm.
///
/// @param  ppCHIALSCAlgorithm  Pointer to pointer of the created CHIALSCAlgorithm instance
///
/// @return CDKResultSuccess upon success.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*CREATEALSC)(
    CHIALSCAlgorithm** ppCHIALSCAlgorithm);


CamxResult ALSCHelper(
    ParsedAWBBGStatsOutput* pStatsAWB,
    const ALSCHelperParam*  pAlscParam,
    const AWBStatsSizeInput awbSize,
    ALSCHelperOutput*       pAlscOutput);


#pragma pack(pop)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CHITINTLESSINTERFACE_H
