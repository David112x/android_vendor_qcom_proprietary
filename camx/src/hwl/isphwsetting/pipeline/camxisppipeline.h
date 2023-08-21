////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxisppipeline.h
/// @brief ISP pipeline Base Class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXISPPIPELINE_H
#define CAMXISPPIPELINE_H

#include "camxcslispdefs.h"
#include "camxdefs.h"
#include "camxhwcontext.h"
#include "camxispiqmodule.h"
#include "camxispstatsmodule.h"
#include "camxcslifedefs.h"
#include "camxcslicpdefs.h"


CAMX_NAMESPACE_BEGIN

/// @todo (CAMX-1117) Update based on BPS IQ module size
const UINT MaxBPSIQModules = 30;                                  ///< Max Number of IQ Modules

static const UINT ICA_MODE_DISABLED  = 0;    ///< IPE ICA mode disabled
static const UINT ICA_MODE_ENABLED   = 1;    ///< IPE ICA mode enabled
static const UINT ICA_MODE_MAX       = 2;    ///< IPE ICA mode max

static const UINT UBWC_MODE_DISABLED = 0;    ///< IPE UBWC mode disabled
static const UINT UBWC_MODE_ENABLED  = 1;    ///< IPE UBWC mode enabled
static const UINT UBWC_MODE_MAX      = 2;    ///< IPE UBWC mode max

/// @brief BPS IQ Module Information
struct BPSIQModuleInfo
{
    ISPIQModuleType moduleType;                                   ///< IQ Module Type
    BOOL            isEnabled;                                    ///< Module Available or not
    CamxResult      (*IQCreate)(BPSModuleCreateData* pInputData); ///< Static create fuction of this IQ Module
    UINT32          moduleVersion;                                ///< Module version of the IQ module
};

/// @brief BPS Capability Information
struct BPSIQModuleList
{
    UINT             numBPSIQModules;                             ///< Number of BPS IQ Modules
    BPSIQModuleInfo* pBPSIQModule[MaxBPSIQModules];               ///< List of BPS IQ Module
};

/// @brief IFE IQ Module Information
struct IFEIQModuleInfo
{
    ISPIQModuleType   moduleType;                                      ///< IQ Module Type
    IFEPipelinePath   IFEPath;                                         ///< IFE pipeline path
    UINT8             installed;                                       ///< Module Available or not on particular chipset
    CamxResult        (*IQCreate)(IFEModuleCreateData* pInputData);    ///< Static create fuction of this IQ Module
};

/// @brief IFE Stats Module Information
struct IFEStatsModuleInfo
{
    ISPStatsModuleType moduleType;                                              ///< Stats Module Name
    UINT8              installed;                                               ///< Module Available or not on chipset
    CamxResult         (*StatsCreate)(IFEStatsModuleCreateData* pInputData);    ///< Static create fuction of this Stats Module
};

/// @brief IFE Capability Information
struct IFECapabilityInfo
{
    UINT                numIFEIQModule;              ///< Number of IQ Modules
    IFEIQModuleInfo*    pIFEIQModuleList;            ///< List of IQ Module
    UINT                numIFEStatsModule;           ///< Number of Stats Modules
    IFEStatsModuleInfo* pIFEStatsModuleList;         ///< List of Stats Module
    UINT                numRDILinkPerIFE;            ///< Number of RDI Link per IFE
    UINT                numIFE;                      ///< Number of IFE
    UINT32              UBWCSupportedVersionMask;    ///< UBWC supported version
    UBWCLossyMode       UBWCLossySupport;            ///< UBWC Lossy supported
    UINT32              lossy10bitWidth;             ///< Lossy 10 bit width
    UINT32              lossy10bitHeight;            ///< Lossy 10 bit height
    UINT32              lossy8bitWidth;              ///< Lossy 8 bit width
    UINT32              lossy8bitHeight;             ///< Lossy 8 bit height
    UINT32              ICAVersion;                  ///< ICA version;
    BOOL                LDCSupport;                  ///< LDC supported version
};

/// @brief BPS Capability Information
struct BPSCapabilityInfo
{
    UINT32              UBWCSupportedVersionMask;    ///< UBWC supported version
    UBWCLossyMode       UBWCLossySupport;            ///< UBWC lossy support
    UINT32              ICAVersion;                  ///< ICA version;
    BOOL                LDCSupport;                  ///< LDC supported version
    SWTMCVersion        tmcversion;                  ///< TMC version
};

/// @brief IPE IQ Module Information
struct IPEIQModuleInfo
{
    ISPIQModuleType   moduleType;                                   ///< IQ Module Type
    IPEPath           path;                                         ///< IPE path indicating if it is input or reference
    BOOL              installed;                                    ///< Module Available or not
    CamxResult        (*IQCreate)(IPEModuleCreateData* pInputData); ///< Static create fuction of this IQ Module
};

/// @brief IPE Capability Information
struct IPECapabilityInfo
{
    IPEIQModuleInfo* pIPEIQModuleList;                            ///< List of IQ Module
    UINT             numIPEIQModules;                             ///< Number of IQ Modules
    UINT             numIPE;                                      ///< Number of IPE
    UINT             maxInputWidth[ICA_MODE_MAX];                 ///< Max IPE input  width
    UINT             maxInputHeight[ICA_MODE_MAX];                ///< Max IPE input  height
    UINT             maxOutputWidth;                              ///< Max IPE output width
    UINT             maxOutputHeight;                             ///< Max IPE output height
    UINT             minInputWidth;                               ///< Min IPE input  width
    UINT             minInputHeight;                              ///< Min IPE input  height
    UINT             minOutputWidth;                              ///< Min IPE output width
    UINT             minOutputHeight;                             ///< Min IPE output width
    UINT             minOutputWidthUBWC;                          ///< Min IPE output width for UBWC
    UINT             minOutputHeightUBWC;                         ///< Min IPE output height for UBWC
    UINT32           UBWCSupportedVersionMask;                    ///< UBWC supported version
    UBWCLossyMode    UBWCLossySupport;                            ///< UBWC Lossy supported
    UINT32           lossy10bitWidth;                             ///< Lossy 10 bit width
    UINT32           lossy10bitHeight;                            ///< Lossy 10 bit height
    UINT32           lossy8bitWidth;                              ///< Lossy 8 bit width
    UINT32           lossy8bitHeight;                             ///< Lossy 8 bit height
    UINT32           ICAVersion;                                  ///< ICA version;
    UINT32           ICAGridGeometryRows;                         ///< Grid geometry rows
    UINT32           ICAGridGeometryColumns;                      ///< Grid geometry columns
    BOOL             disableLENRDS4Buffer;                        ///< LENR supported version
    BOOL             LENRSupport;                                 ///< LENR supported version
    BOOL             referenceLookaheadTransformSupported;        ///< Ref Lookahead Transform support
    BOOL             swStriping;                                  ///< Striping done in UMD
    FLOAT            maxUpscale[UBWC_MODE_MAX];                   ///< Max upscale limit
    FLOAT            maxDownscale[UBWC_MODE_MAX];                 ///< Max downscale limit
    FLOAT            realtimeClockEfficiency;                     ///< realtime clock Efficiency
    FLOAT            nonrealtimeClockEfficiency;                  ///< non real time clock Efficiency
    BOOL             LDCSupport;                                  ///< LDC supported version
    BOOL             isPostfilterWithBlend;                       ///< Flag to check if MFNR Postfilter has Blend stage
    BOOL             hasMFlimit;                                  ///< Flag to check MF processing resolution limit
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Base Class for all the ISP Pipeline setup
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ISPPipeline
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetModuleList
    ///
    /// @brief  Function to be implemented to get the pipleine capability
    ///
    /// @param  pIQmoduleInfo structure into which capability info is filled
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult GetModuleList(
                BPSIQModuleList* pIQmoduleInfo)
    {
        CAMX_UNREFERENCED_PARAM(pIQmoduleInfo);

        return CamxResultSuccess;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetModuleListForMode
    ///
    /// @brief  Function to be implemented to get the pipleine capability in mode requested
    ///
    /// @param  pIQmoduleInfo structure into which capability info is filled
    /// @param  mode          returns the capability in this mode
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult GetModuleListForMode(
                BPSIQModuleList* pIQmoduleInfo,
                UINT32 mode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FetchPipelineCapability
    ///
    /// @brief  Update the ISP pipeline capability
    ///
    /// @param  pPipelineCapability Pointer to the ISP pipeline capability
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID FetchPipelineCapability(
        VOID*  pPipelineCapability)
    {
        CAMX_UNREFERENCED_PARAM(pPipelineCapability);
    }

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
    virtual CamxResult ProgramIQModuleEnableConfig(
        ISPInputData*       pInputData,
        ISPInternalData*    pISPData,
        IFEOutputPath*      pIFEOutputPathInfo)
    {
        CAMX_UNREFERENCED_PARAM(pInputData);
        CAMX_UNREFERENCED_PARAM(pISPData);
        CAMX_UNREFERENCED_PARAM(pIFEOutputPathInfo);
        return CamxResultSuccess;
    }

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
    virtual VOID GetISPIQModulesOfType(
        ISPIQModuleType moduleType,
        VOID*           pModuleInfo)
    {
        CAMX_UNREFERENCED_PARAM(moduleType);
        CAMX_UNREFERENCED_PARAM(pModuleInfo);
    }

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
    virtual VOID DumpTuningMetadata(
        ISPInputData*       pInputData,
        DebugDataWriter*    pDebugDataWriter)
    {
        CAMX_UNREFERENCED_PARAM(pInputData);
        CAMX_UNREFERENCED_PARAM(pDebugDataWriter);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillFlushConfig
    ///
    /// @brief  Helper function to fill the Flush Config
    ///
    /// @param  pCmdBuffer Pointer to the command buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID FillFlushConfig(
        CmdBuffer*  pCmdBuffer)
    {
        CAMX_UNREFERENCED_PARAM(pCmdBuffer);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetICPClockAndBandWidthConfigurationVersion
    ///
    /// @brief  Helper to Get BandWidth configuration
    ///
    /// @return ICPBWconfiguration
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual UINT32 GetICPClockAndBandWidthConfigurationVersion()
    {
        return CSLICPGenericBlobCmdBufferClkV2;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupCoreConfig
    ///
    /// @brief  Helper function to setup Core Config
    ///
    /// @param  pCmdBuffer      Pointer to the command buffer
    /// @param  pStatsTapOut    Pointer to stats tap-out config
    /// @param  pCfg            Pointer to the Core Config Structure
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult SetupCoreConfig(
        CmdBuffer*      pCmdBuffer,
        IFEStatsTapOut* pStatsTapOut,
        VOID*           pCfg)
    {
        CAMX_UNREFERENCED_PARAM(pCmdBuffer);
        CAMX_UNREFERENCED_PARAM(pStatsTapOut);
        CAMX_UNREFERENCED_PARAM(pCfg);

        return CamxResultSuccess;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillCGCConfig
    ///
    /// @brief  Helper function to fill the CGC Config
    ///
    /// @param  pCmdBuffer Pointer to the command buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID FillCGCConfig(
        CmdBuffer*  pCmdBuffer)
    {
        CAMX_UNREFERENCED_PARAM(pCmdBuffer);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateWMConfig
    ///
    /// @brief  Helper function to Update WM Config
    ///
    /// @param  pCmdBuffer      Pointer to the command buffer
    /// @param  pCalculatedData Pointer to the ISPInetnalData Structure
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult UpdateWMConfig(
        CmdBuffer*       pCmdBuffer,
        ISPInternalData* pCalculatedData)
    {
        CAMX_UNREFERENCED_PARAM(pCmdBuffer);
        CAMX_UNREFERENCED_PARAM(pCalculatedData);

        return CamxResultSuccess;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetISPAcquireInputInfoVersion
    ///
    /// @brief  Helper Function to Get ISPAcquireInputVersion
    ///
    /// @return UINT32
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CAMX_INLINE UINT32 GetISPAcquireInputInfoVersion()
    {
        return ISPAcquireInputVersion1;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetISPAcquireCommonInfoVersion
    ///
    /// @brief  Helper Function to Get ISPAcquireInputVersion
    ///
    /// @return UINT32
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CAMX_INLINE UINT32 GetISPAcquireCommonInfoVersion()
    {
        return ISPAcquireCommonVersion1;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAcquireHWStructVersion
    ///
    /// @brief  Helper Function to Get ISPAcquireInputVersion
    ///
    /// @return UINT32
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CAMX_INLINE UINT32 GetAcquireHWStructVersion()
    {
        return CSLAcquiredDeviceVersion1;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIFEDefaultConfig
    ///
    /// @brief  Helper to populate default IFE configuration for IFE modules
    ///
    /// @param  pDefaultData    Pointer to the data to be use as default for IFE modules that differ from version to version
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID GetIFEDefaultConfig(
        IFEDefaultModuleConfig* pDefaultData)
    {
        CAMX_UNREFERENCED_PARAM(pDefaultData);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCapability
    ///
    /// @brief  virtual method to get the isp pipeline capability
    ///
    /// @param  pCapabilityInfo Pointer to the structure into which capability info is filled
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult GetCapability(
        VOID* pCapabilityInfo)
    {
        CAMX_UNREFERENCED_PARAM(pCapabilityInfo);

        return CamxResultSuccess;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPDHWCapability
    ///
    /// @brief  virtual method to get the isp pipeline PDHW capability
    ///
    /// @param  pCapabilityInfo Pointer to the structure into which capability info is filled
    /// @param  profileID       IFE node profile
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID GetPDHWCapability(
        PDHwAvailablity* pCapabilityInfo,
        IFEProfileId     profileID)
    {
        CAMX_UNREFERENCED_PARAM(profileID);

        if (NULL != pCapabilityInfo)
        {
            pCapabilityInfo->isDualPDHwAvailable    = FALSE;
            pCapabilityInfo->isLCRHwAvailable       = FALSE;
            pCapabilityInfo->isSparsePDHwAvailable  = FALSE;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupFlushRegDump
    ///
    /// @brief  virtual method to setup ISP Register Dump structures
    ///
    /// @param  pFlushBuffer Pointer to the flush dump Buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID SetupFlushRegDump(
        VOID* pFlushBuffer)
    {
        CAMX_UNREFERENCED_PARAM(pFlushBuffer);
        return;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseFlushRegDump
    ///
    /// @brief  virtual method to parse ISP Register Dump structures
    ///
    /// @param  pFlushBuffer Pointer to the flush dump Buffer
    /// @param  pBankUpdate  Pointer to the DMI Bank Update
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID ParseFlushRegDump(
        VOID*             pFlushBuffer,
        IFEDMIBankUpdate* pBankUpdate)
    {
        CAMX_UNREFERENCED_PARAM(pFlushBuffer);
        CAMX_UNREFERENCED_PARAM(pBankUpdate);
        return;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateDMIBankSelectValue
    ///
    /// @brief  virtual method to update DMI bank select to a given value
    ///
    /// @param  pBankUpdate     Pointer to the DMI Bank Update
    /// @param  isBankValueZero True if the DMI bank to be updated to bank zero
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID UpdateDMIBankSelectValue(
        IFEDMIBankUpdate* pBankUpdate,
        BOOL              isBankValueZero)
    {
        CAMX_UNREFERENCED_PARAM(pBankUpdate);
        CAMX_UNREFERENCED_PARAM(isBankValueZero);
        return;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupHangRegDump
    ///
    /// @brief  virtual method to setup ISP Register Dump structures
    ///
    /// @param  pHangBuffer     Pointer to the hang dump Buffer
    /// @param  mode            IFE mode
    /// @param  isPerFrameDump  indicator for per frame dump
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID SetupHangRegDump(
        VOID*         pHangBuffer,
        IFEModuleMode mode,
        BOOL          isPerFrameDump)
    {
        CAMX_UNREFERENCED_PARAM(pHangBuffer);
        CAMX_UNREFERENCED_PARAM(mode);
        CAMX_UNREFERENCED_PARAM(isPerFrameDump);

        return;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseHangRegDump
    ///
    /// @brief  Helper method to parse ISP Register Dump structures
    ///
    /// @param  pHangBuffer Pointer to the flush dump Buffer
    /// @param  leftHangFd  file descriptor fd for left Stripe
    /// @param  rightHangFd file descriptor fd for right Stripe
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID ParseHangRegDump(
        VOID* pHangBuffer,
        INT   leftHangFd,
        INT   rightHangFd)

    {
        CAMX_UNREFERENCED_PARAM(pHangBuffer);
        CAMX_UNREFERENCED_PARAM(leftHangFd);
        CAMX_UNREFERENCED_PARAM(rightHangFd);
        return;
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFlushDumpBufferSize
    ///
    /// @brief  Helper method to return size of the Flush Bump
    ///
    /// @return Size of the Dump buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetFlushDumpBufferSize()
    {
        return m_flushDumpBufferSizeBytes;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetFlushDumpBufferSize
    ///
    /// @brief  Helper method to set Flush Dump Buffer Size
    ///
    /// @param  sizeBytes size of the buffer in bytes
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetFlushDumpBufferSize(
        UINT32 sizeBytes)
    {
        m_flushDumpBufferSizeBytes = sizeBytes;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetHangDumpBufferSize
    ///
    /// @brief  Helper method to return size of the Hang Dump
    ///
    /// @return Size of the Dump buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetHangDumpBufferSize()
    {
        return m_hangDumpBufferSizeBytes;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetHangDumpBufferSize
    ///
    /// @brief  Sets the buffer size of the Hang Dump
    ///
    /// @param  sizeBytes size of the buffer in bytes
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetHangDumpBufferSize(
        UINT sizeBytes)
    {
        m_hangDumpBufferSizeBytes = sizeBytes;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRegDumpBufferSize
    ///
    /// @brief  Helper method to return size of the Per frame Reg Dump
    ///
    /// @return Size of the Dump bfer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetRegDumpBufferSize()
    {
        return m_regDumpBufferSizeBytes;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetRegDumpBufferSize
    ///
    /// @brief  Helper method to set size of the Per frame Reg Dump
    ///
    /// @param  sizeBytes size of the buffer in bytes
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetRegDumpBufferSize(
        UINT32 sizeBytes)
    {
        m_regDumpBufferSizeBytes = sizeBytes;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUBWCModeConfig
    ///
    /// @brief  Helper function to get UBWC mode Config
    ///
    /// @param  pImageFormat  Imageformat pointer
    /// @param  planeIndex    planeIndex of a format
    ///
    /// @return modeConfig value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual UINT32 GetUBWCModeConfig(
        const ImageFormat* pImageFormat,
        UINT32 planeIndex)
    {
        return ImageFormatUtils::GetUBWCModeConfig(pImageFormat, planeIndex);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUBWCPackerConfig
    ///
    /// @brief  Helper function to get UBWC Packer Config
    ///
    /// @param  pImageFormat   Imageformat pointer
    /// @param  pPackerConfig  pPackerConfig pointer
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult GetUBWCPackerConfig(
        const ImageFormat* pImageFormat,
        UINT32* pPackerConfig)
    {
        CamxResult result = CamxResultSuccess;
        *pPackerConfig = 0;
        switch (pImageFormat->format)
        {
            case Format::UBWCTP10:
                *pPackerConfig = 0xb;
                break;
            case Format::UBWCNV124R:
                *pPackerConfig = 0xe;
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid UBWC format: %d",
                    pImageFormat->format);
                result = CamxResultEFailed;
                break;
        }
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIFEBandWidthConfigurationVersion
    ///
    /// @brief  Helper to Get BandWidth configuration
    ///
    /// @return IFEBWconfiguration
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual UINT32 GetIFEBandWidthConfigurationVersion()
    {
        return IFEGenericBlobTypeResourceBWConfigV2;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetTitanChipVersion
    ///
    /// @brief  Set Titan Chip set version
    ///
    /// @param  titanChipVersion  titan chip set version
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetTitanChipVersion(
        CSLCameraTitanChipVersion titanChipVersion)
    {
        m_titanChipVersion = titanChipVersion;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTitanChipVersion
    ///
    /// @brief  Helper to Get Titan Chip version
    ///
    /// @return CSLCameraTitanChipVersion
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CSLCameraTitanChipVersion GetTitanChipVersion()
    {
        return m_titanChipVersion;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRegCmdSize
    ///
    /// @brief  Helper to Get Rgeister Command size needed for this pipeline in DWRDS
    ///
    /// @return size of the Rgeister comd size in DWORDS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetRegCmdSize()
    {
        return m_regCmdSizeInDwords;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetRegCmdSize
    ///
    /// @brief  Helper to Set Rgeister Command size needed for this pipeline in DWRDS
    ///
    /// @param  sizeInDwords  Size of the Reg Cmd in DWORDS
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetRegCmdSize(
        UINT32 sizeInDwords)
    {
        m_regCmdSizeInDwords = sizeInDwords;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetThrottlePattern
    ///
    /// @brief  Helper function to Set Throttle pattern
    ///
    /// @param  throughPut  ThroughtPut rate
    /// @param  pCmdBuffer  Pointer to the command buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID SetThrottlePattern(
        FLOAT      throughPut,
        CmdBuffer* pCmdBuffer)
    {
        CAMX_UNREFERENCED_PARAM(throughPut);
        CAMX_UNREFERENCED_PARAM(pCmdBuffer);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ISPPipeline
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ISPPipeline() = default;

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ISPPipeline
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ISPPipeline() = default;

private:
    ISPPipeline(const ISPPipeline&)            = delete; ///< Disallow the copy constructor
    ISPPipeline& operator=(const ISPPipeline&) = delete; ///< Disallow assignment operator

    CSLCameraTitanChipVersion m_titanChipVersion;         ///< Titan Chip Version
    UINT32                    m_hangDumpBufferSizeBytes;  ///< Hang Dump Buffer Size Bytes
    UINT32                    m_flushDumpBufferSizeBytes; ///< Flush Dump Buffer Size Bytes
    UINT32                    m_regDumpBufferSizeBytes;   ///< Reg Dump Buffer Size Bytes
    UINT32                    m_regCmdSizeInDwords;       ///< Register Command size in DWORDS
};

CAMX_NAMESPACE_END

#endif // CAMXISPPIPELINE_H
