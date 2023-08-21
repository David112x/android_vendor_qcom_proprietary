////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipeasf30.h
/// @brief IPE ASF class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXIPEASF30_H
#define CAMXIPEASF30_H

#include "camxispiqmodule.h"
#include "ipe_data.h"
#include "iqcommondefs.h"

CAMX_NAMESPACE_BEGIN

/// @brief: Enumerator to map optimized Look Up Tables indices with DMI LUT_SEL in ASF module HPG/SWI
enum ASFDMILUTConfigLUTSelIndex
{
    NoLUTSelected                               = 0x0,    ///< No LUT
    Layer1ActivityNormalGainPosGainNegSoftThLUT = 0x1,    ///< L1 LUTs-Activity Clamp Th, Normal Scale, Gain +/-, Soft Th
    Layer1GainWeightLUT                         = 0x2,    ///< L1 LUT-Gain Weight
    Layer1SoftThresholdWeightLUT                = 0x3,    ///< L1 LUT-Soft Threshold Weight
    Layer2ActivityNormalGainPosGainNegSoftThLUT = 0x4,    ///< L2 LUTs-Activity Clamp Th, Normal Scale, Gain +/-, Soft Th
    Layer2GainWeightLUT                         = 0x5,    ///< L2 LUT-Gain Weight
    Layer2SoftThresholdWeightLUT                = 0x6,    ///< L2 LUT-Soft Threshold Weight
    ChromaGradientPosNegLUT                     = 0x7,    ///< Chroma Gradient +/- LUT
    ContrastPosNegLUT                           = 0x8,    ///< Contrast +/- LUT
    SkinActivityGainLUT                         = 0x9,    ///< Skit Weight, Activity Gain LUT
    MaxASFLUT                                   = 0xA,    ///< MAX LUT
};


/// @brief: This structure has information of number of entries for each LUT.
static const UINT IPEASFLUTNumEntries[MaxASFLUT] =
{
    0,      ///< NoLUTSelected
    256,    ///< Layer1ActivityNormalGainPosGainNegSoftThLUT
    256,    ///< Layer1GainWeightLUT
    256,    ///< Layer1SoftThresholdWeightLUT
    256,    ///< Layer2ActivityNormalGainPosGainNegSoftThLUT
    256,    ///< Layer2GainWeightLUT
    256,    ///< Layer2SoftThresholdWeightLUT
    256,    ///< ChromaGradientPosNegLUT
    256,    ///< ContrastPosNegLUT
    17,     ///< SkinActivityGainLUT
};

static const UINT MaxASF30LUTNumEntries =
    IPEASFLUTNumEntries[Layer1ActivityNormalGainPosGainNegSoftThLUT] +
    IPEASFLUTNumEntries[Layer1GainWeightLUT] +
    IPEASFLUTNumEntries[Layer1SoftThresholdWeightLUT] +
    IPEASFLUTNumEntries[Layer2ActivityNormalGainPosGainNegSoftThLUT] +
    IPEASFLUTNumEntries[Layer2GainWeightLUT] +
    IPEASFLUTNumEntries[Layer2SoftThresholdWeightLUT] +
    IPEASFLUTNumEntries[ChromaGradientPosNegLUT] +
    IPEASFLUTNumEntries[ContrastPosNegLUT] +
    IPEASFLUTNumEntries[SkinActivityGainLUT];

static const UINT32 IPEASFLUTBufferSize        = MaxASF30LUTNumEntries * sizeof(UINT32);
static const UINT32 IPEASFLUTBufferSizeInDWord = MaxASF30LUTNumEntries;
static const UINT32 NUM_OF_NZ_ENTRIES          = 8;

struct ASFHwSettingParams
{
    CmdBuffer* pLUTDMICmdBuffer;   ///< Pointer to module LUT DMI cmd buffer
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  Class for IPE ASF Module
///
///         IPE::PPS::ASF  component operates  on YUV  Color Space  Format  (YUV 4:4:4)  10bit input  to perform  image
///         enhancement features  such as Edge Enhancement,  Edge Alignment, Sharpening of  Skin-tone Texture, Contrast
///         Improvement of the Darker details to near by  White objects/boundaries and Special Effects. Output would be
///         in YUV 4:4:4 format  to be used in Viewfinder, Video Capture and  Snapshot processing modes. These features
///         are achieved by operating on the Luminance/Luma (Y)  channel, with the help of various mechanisms supported
///         -  Cross-Media Filter:  3x3, the  Sharpening Filters  - Layer-1:  7x7, Layer-2:  13x13, parameters  such as
///         Thresholds and  Gain to control sharpeneing  and smoothing effects, Radial  Correction, Clamping, Blending,
///         local Y  & activity (busyness)  values for contrast  and texture, besides  sketch, emboss and  neon special
///         effects. Chroma component of the image are used to  sharpen objects with skin tone but around the face map,
///         gain reduction based on chroma gradient for color edges.
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IPEASF30 final : public ISPIQModule
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create ASF Object
    ///
    /// @param  pCreateData  Pointer to resource and intialization data for ASF Creation
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        IPEModuleCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillCmdBufferManagerParams
    ///
    /// @brief  Fills the command buffer manager params needed by IQ Module
    ///
    /// @param  pInputData Pointer to the IQ Module Input data structure
    /// @param  pParam     Pointer to the structure containing the command buffer manager param to be filled by IQ Module
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult FillCmdBufferManagerParams(
       const ISPInputData*     pInputData,
       IQModuleCmdBufferParam* pParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Execute
    ///
    /// @brief  Execute process capture request to configure module
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult Execute(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IPEASF30
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IPEASF30();

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPEASF30
    ///
    /// @brief  Constructor
    ///
    /// @param  pNodeIdentifier     String identifier for the Node that creating this IQ Module object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit IPEASF30(
        const CHAR* pNodeIdentifier);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AllocateCommonLibraryData
    ///
    /// @brief  Allocate memory required for common library
    ///
    /// @return CamxResult success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AllocateCommonLibraryData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeallocateCommonLibraryData
    ///
    /// @brief  Deallocate memory required for common library
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DeallocateCommonLibraryData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize ASF Object
    ///
    /// @param  pCreateData Pointer to the Module Creation
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult Initialize(
        IPEModuleCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateDependenceParams
    ///
    /// @brief  Validate the input info from client
    ///
    /// @param  pInputData Pointer to the IPE/PPS/ASF input data
    ///
    /// @return CamxResult Indicates if the input is valid or invalid
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ValidateDependenceParams(
        const ISPInputData* pInputData
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckDependenceChange
    ///
    /// @brief  Check to see if the Dependence Trigger Data Changed
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return BOOL Indicates if the settings have changed compared to last setting
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CheckDependenceChange(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunCalculation
    ///
    /// @brief  Calculate the Register Value
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return CamxResult Indicates if configuration calculation was success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RunCalculation(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FetchDMIBuffer
    ///
    /// @brief  Fetch ASF30 DMI LUT
    ///
    /// @return CamxResult Indicates if fetch DMI was success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FetchDMIBuffer();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateIPEInternalData
    ///
    /// @brief  Update Pipeline input data, such as metadata, IQSettings.
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return CamxResult success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateIPEInternalData(
        ISPInputData* pInputData
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAdditionalASF30Input
    ///
    /// @brief  Initialize ASF 30 additional Input
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetAdditionalASF30Input(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of ASF module for debug
    ///
    /// @param  pDMIDataPtr Pointer to the DMI data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig(
        UINT32* pDMIDataPtr
        ) const;

    IPEASF30(const IPEASF30&)            = delete;           ///< Disallow the copy constructor
    IPEASF30& operator=(const IPEASF30&) = delete;           ///< Disallow copy assignment operator

    // Member variables
    const CHAR*                     m_pNodeIdentifier;       ///< String identifier for the Node that created this object
    ASF30InputData                  m_dependenceData;        ///< Dependence Data for this Module
    AsfParameters                   m_ASFParameters;         ///< IQ parameters

    CmdBufferManager*               m_pLUTCmdBufferManager;  ///< DMI Command buffer manager for all LUTs
    CmdBuffer*                      m_pLUTDMICmdBuffer;      ///< LUT DMI Command buffer

    UINT32*                         m_pASFLUTs;              ///< Tuning ASF, holder of LUTs
    BOOL                            m_bypassMode;            ///< Bypass ASF

    asf_3_0_0::chromatix_asf30Type* m_pChromatix;            ///< Pointer to tuning mode data
};

CAMX_NAMESPACE_END

#endif // CAMXIPEASF30_H
