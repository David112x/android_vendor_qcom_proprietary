////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxtitan170ife.h
/// @brief Titan 170 IFE pipeline Class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXTITAN170IFE_H
#define CAMXTITAN170IFE_H

CAMX_NAMESPACE_BEGIN

/// @brief Structure to hold IFE pipelie enable config registers
struct IFE170ModuleEnableConfig
{
    IFE_IFE_0_VFE_MODULE_LENS_EN                 lensProcessingModuleConfig;            ///< Lens processing Module enable
    IFE_IFE_0_VFE_MODULE_COLOR_EN                colorProcessingModuleConfig;           ///< Color processing Module enable
    IFE_IFE_0_VFE_MODULE_ZOOM_EN                 frameProcessingModuleConfig;           ///< Frame processing Module enable
    IFE_IFE_0_VFE_FD_OUT_Y_CROP_RND_CLAMP_CFG    FDLumaCropRoundClampConfig;            ///< FD Luma path Module enable
    IFE_IFE_0_VFE_FD_OUT_C_CROP_RND_CLAMP_CFG    FDChromaCropRoundClampConfig;          ///< FD Chroma path Module enable
    IFE_IFE_0_VFE_FULL_OUT_Y_CROP_RND_CLAMP_CFG  videoLumaCropRoundClampConfig;         ///< Full Luma path Module enable
    IFE_IFE_0_VFE_FULL_OUT_C_CROP_RND_CLAMP_CFG  videoChromaCropRoundClampConfig;       ///< Full Chroma path Module enable
    IFE_IFE_0_VFE_DS4_Y_CROP_RND_CLAMP_CFG       videoDS4LumaCropRoundClampConfig;      ///< DS4 Luma path Module enable
    IFE_IFE_0_VFE_DS4_C_CROP_RND_CLAMP_CFG       videoDS4ChromaCropRoundClampConfig;    ///< DS4 Chroma path Module enable
    IFE_IFE_0_VFE_DS16_Y_CROP_RND_CLAMP_CFG      videoDS16LumaCropRoundClampConfig;     ///< DS16 Luma path Module enable
    IFE_IFE_0_VFE_DS16_C_CROP_RND_CLAMP_CFG      videoDS16ChromaCropRoundClampConfig;   ///< DS16 Chroma path Module enable
    IFE_IFE_0_VFE_MODULE_DISP_EN                 frameProcessingDisplayModuleConfig;    ///< Frame processing Disp module enable
    IFE_IFE_0_VFE_DISP_OUT_Y_CROP_RND_CLAMP_CFG  displayFullLumaCropRoundClampConfig;   ///< Full Disp Luma path Module enable
    IFE_IFE_0_VFE_DISP_OUT_C_CROP_RND_CLAMP_CFG  displayFullChromaCropRoundClampConfig; ///< Full Disp Chroma path Module enable
    IFE_IFE_0_VFE_DISP_DS4_Y_CROP_RND_CLAMP_CFG  displayDS4LumaCropRoundClampConfig;    ///< DS4 Disp Luma path Module enable
    IFE_IFE_0_VFE_DISP_DS4_C_CROP_RND_CLAMP_CFG  displayDS4ChromaCropRoundClampConfig;  ///< DS4 Disp Chroma path Module enable
    IFE_IFE_0_VFE_DISP_DS16_Y_CROP_RND_CLAMP_CFG displayDS16LumaCropRoundClampConfig;   ///< DS16 Disp Luma path Module enable
    IFE_IFE_0_VFE_DISP_DS16_C_CROP_RND_CLAMP_CFG displayDS16ChromaCropRoundClampConfig; ///< DS16 Disp Chroma path Module enable
    IFE_IFE_0_VFE_MODULE_STATS_EN                statsEnable;                           ///< Stats Module Enable
    IFE_IFE_0_VFE_STATS_CFG                      statsConfig;                           ///< Stats config
    IFE_IFE_0_VFE_DSP_TO_SEL                     DSPConfig;                             ///< Dsp Config
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Base Class for all IFE Titan 170 Pipeline setup
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Titan170IFE final : public ISPPipeline
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Titan170IFE
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Titan170IFE() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FetchPipelineCapability
    ///
    /// @brief  Update the Titan 170 IFE pipeline capability
    ///
    /// @param  pPipelineCapability Pointer to the IFE pipeline capability
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FetchPipelineCapability(
        VOID*  pPipelineCapability);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProgramIQModuleEnableConfig
    ///
    /// @brief  Reprogram the Module enable settings for the IQ Modules
    ///
    /// @param  pInputData          Pointer to the input data
    /// @param  pISPData            Pointer to the ISP internal data
    /// @param  pIFEOutputPathInfo  Pointer to the IFE output path data
    ///
    /// @return CamxResult True on successful execution else False on failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProgramIQModuleEnableConfig(
        ISPInputData*       pInputData,
        ISPInternalData*    pISPData,
        IFEOutputPath*      pIFEOutputPathInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetISPIQModulesOfType
    ///
    /// @brief  Get ISP IQ module
    ///
    /// @param  moduleType  Module type
    /// @param  pModuleInfo Pointer to the module info
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetISPIQModulesOfType(
        ISPIQModuleType moduleType,
        VOID*           pModuleInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillConfigTuningMetadata
    ///
    /// @brief  Helper to populate tuning data easily obtain at IFE node level
    ///
    /// @param  pInputData          Pointer to the data input and output stuctures are inside
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FillConfigTuningMetadata(
        ISPInputData*       pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpTuningMetadata
    ///
    /// @brief  Helper to publish tuning metadata
    ///
    /// @param  pInputData          Pointer to the input data
    /// @param  pDebugDataWriter    Pointer to tuning-data writer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpTuningMetadata(
        ISPInputData*       pInputData,
        DebugDataWriter*    pDebugDataWriter);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillFlushConfig
    ///
    /// @brief  Helper function to Setup the Flush Config
    ///
    /// @param  pCmdBuffer Pointer to the command buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FillFlushConfig(
        CmdBuffer*  pCmdBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillCGCConfig
    ///
    /// @brief  Helper function to fill the CGC Config
    ///
    /// @param  pCmdBuffer Pointer to the command buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FillCGCConfig(
        CmdBuffer*  pCmdBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIFEDefaultConfig
    ///
    /// @brief  Helper to populate default IFE configuration for IFE modules
    ///
    /// @param  pDefaultData    Pointer to the data to be use as default for IFE modules that differ from version to version
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetIFEDefaultConfig(
        IFEDefaultModuleConfig* pDefaultData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIFEBandWidthConfigurationVersion
    ///
    /// @brief  Helper to Get BandWidth configuration
    ///
    /// @return IFEBWconfiguration
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetIFEBandWidthConfigurationVersion()
    {
        return IFEGenericBlobTypeResourceBWConfig;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetISPAcquireInputInfoVersion
    ///
    /// @brief  Helper Function to Get ISPAcquireInputVersion
    ///
    /// @return UINT32
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 GetISPAcquireInputInfoVersion()
    {
        return ISPAcquireInputVersion2;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetISPAcquireCommonInfoVersion
    ///
    /// @brief  Helper Function to Get ISPAcquireInputVersion
    ///
    /// @return UINT32
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 GetISPAcquireCommonInfoVersion()
    {
        return ISPAcquireCommonVersion2;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAcquireHWStructVersion
    ///
    /// @brief  Helper Function to Get ISPAcquireInputVersion
    ///
    /// @return UINT32
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 GetAcquireHWStructVersion()
    {
        return CSLAcquiredDeviceVersion2;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateDMIBankSelectValue
    ///
    /// @brief  Helper method to set all double banked DMI to a given bank value
    ///
    /// @param  pBankUpdate     Pointer to the DMI Bank Update
    /// @param  isBankValueZero True to select bank 0, otherwise, select bank 1
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID UpdateDMIBankSelectValue(
        IFEDMIBankUpdate* pBankUpdate,
        BOOL              isBankValueZero);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~Titan170IFE
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~Titan170IFE() = default;

private:
    Titan170IFE(const Titan170IFE&)            = delete; ///< Disallow the copy constructor
    Titan170IFE& operator=(const Titan170IFE&) = delete; ///< Disallow assignment operator

    IFE170ModuleEnableConfig   m_moduleConfig;           ///< module config smember
};

CAMX_NAMESPACE_END

#endif // CAMXTITAN170IFE_H
