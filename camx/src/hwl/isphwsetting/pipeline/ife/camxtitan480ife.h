////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxtitan480ife.h
/// @brief Titan 480 IFE pipeline Class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXTITAN480IFE_H
#define CAMXTITAN480IFE_H

#include "titan480_ife.h"

CAMX_NAMESPACE_BEGIN


// BF v2.5 (Titan480 specific)
static const UINT32 DefaultBF25DMISelShift                   = 73;
static const UINT32 DefaultBF25DMIEndOfBufferShift           = 72;
static const UINT32 DefaultBF25DMIMergeShift                 = 71;

// Due to 96-bit of BF v2.5 DMI entry manipulation requires two unsigned 64-bit operations.
static const UINT32 DefaultBF25DMIUpper64bitSelShift         = DefaultBF25DMISelShift - 64;
static const UINT32 DefaultBF25DMIUpper64bitEndOfBufferShift = DefaultBF25DMIEndOfBufferShift - 64;
static const UINT32 DefaultBF25DMIUpper64bitMergeShift       = DefaultBF25DMIMergeShift - 64;
static const UINT32 DefaultBF25DMIOutputIdShift              = 63;

// OutputId bit mask is 8-bit size, however, it is divided into two parts:
/// (1) upper 64-bit bitmask (7-bit) and (2) lower 64-bit bitmask (1-bit).
static const UINT32 DefaultBF25DMIUpper64bitOutputIdBits     = 0x7f; // 7-bit mask
static const UINT32 DefaultBF25DMILower64bitOutputIdBits     = 0x1;  // 1-bit mask
static const UINT32 DefaultBF25DMIIndexShift                 = 55; // Different from BF v2.3 and v2.4 (not bit position 54)
static const UINT32 DefaultBF25DMIIndexBits                  = 0xFF; // 8-bit mask [62:55]

static const UINT32 MaxIFEFlushDumpRegions   = 32;  // Max IFE Flush Dump regions


static const UINT32 IFEThrottleCfgSize = sizeof(IFE_IFE_0_TOP_CORE_CFG_2) / sizeof(UINT32);  ///< IFE Throttle Cfg Size

static const UINT32 IFEThrottlePattern25 = 0x4444;     ///< Throttle count Pattern for througtput < 25 %
static const UINT32 IFEThrottlePattern50 = 0xAAAA;     ///< Throttle count pattern for throughput < 50 %
static const UINT32 IFEThrottlePattern75 = 0xEEEE;     ///< Throttle count pattern for throughput < 100 %

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Base Class for all IFE Titan 480 Pipeline setup
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Titan480IFE final : public ISPPipeline
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FetchPipelineCapability
    ///
    /// @brief  Update the Titan 480 IFE pipeline capability
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
    /// GetUBWCModeConfig
    ///
    /// @brief  Helper function to get UBWC mode Config
    ///
    /// @param  pImageFormat  Imageformat pointer
    /// @param  planeIndex    planeIndex of a format
    ///
    /// @return mode config value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual UINT32 GetUBWCModeConfig(
        const ImageFormat* pImageFormat,
        UINT32 planeIndex)
    {
        UINT modeConfig = 0;
        switch (pImageFormat->format)
        {
            case Format::UBWCTP10:
                modeConfig              = (0 == planeIndex) ? 0x2 : 0x3;
                break;
            case Format::UBWCNV124R:
                modeConfig              = (0 == planeIndex) ? 0x4 : 0x5;
                break;
            default:
                CAMX_ASSERT_ALWAYS_MESSAGE("Invalid UBWC format: %d", pImageFormat->format);
                break;
        }
        modeConfig   = (modeConfig << 4);   // format specific configuration
        modeConfig  |= (0x3);              // COMPRESS_EN<<1 | UBWC_EN
        return modeConfig;
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
                *pPackerConfig = 0x3;
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
    /// GetPDHWCapability
    ///
    /// @brief  virtual method to get the isp pipeline PDHW capability
    ///
    /// @param  pCapabilityInfo Pointer to the structure into which capability info is filled
    /// @param  profileID       IFE node profile
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetPDHWCapability(
        PDHwAvailablity* pCapabilityInfo,
        IFEProfileId     profileID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupFlushRegDump
    ///
    /// @brief  virtual method to setup ISP Register Dump structures
    ///
    /// @param  pFlushBuffer Pointer to the flush dump Buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetupFlushRegDump(
        VOID* pFlushBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseFlushRegDump
    ///
    /// @brief  Helper method to parse ISP Register Dump structures
    ///
    /// @param  pFlushBuffer Pointer to the flush dump Buffer
    /// @param  pBankUpdate  Pointer to the DMI Bank Update
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ParseFlushRegDump(
        VOID*             pFlushBuffer,
        IFEDMIBankUpdate* pBankUpdate);

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
    /// SetupHangRegDump
    ///
    /// @brief  Helper method to setup ISP Register Dump structures
    ///
    /// @param  pHangBuffer     Pointer to the hang dump Buffer
    /// @param  mode            IFE Mode
    /// @param  isPerFrameDump  indicator for Per Frame Dump
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID SetupHangRegDump(
        VOID*         pHangBuffer,
        IFEModuleMode mode,
        BOOL          isPerFrameDump);


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
        INT   rightHangFd);


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
    CamxResult SetupCoreConfig(
        CmdBuffer*      pCmdBuffer,
        IFEStatsTapOut* pStatsTapOut,
        VOID*           pCfg);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateWMConfig
    ///
    /// @brief  Helper function to Update WM Config
    ///
    /// @param  pCmdBuffer      Pointer to the command buffer
    /// @param  pCalculatedData Pointer to the ISPInternalData Structure
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateWMConfig(
        CmdBuffer*       pCmdBuffer,
        ISPInternalData* pCalculatedData);

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
    VOID SetThrottlePattern(
        FLOAT      throughPut,
        CmdBuffer* pCmdBuffer);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIFEBandWidthConfigurationVersion
    ///
    /// @brief  Helper to Get BandWidth configuration
    ///
    /// @return IFEBWconfiguration
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetIFEBandWidthConfigurationVersion()
    {
        return IFEGenericBlobTypeResourceBWConfigV2;
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
    /// Titan480IFE
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Titan480IFE();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~Titan480IFE
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~Titan480IFE();

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillIFERegReadInfo
    ///
    /// @brief  Helper function to fill Reg Read Info
    ///
    /// @param  pRegReadInfo  Pointer to the Register Read Info to be filled
    /// @param  offset        Reg Offset
    /// @param  numberOfReads number of DMI Data reads
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID FillIFERegReadInfo(
        IFERegReadInfo*  pRegReadInfo,
        UINT32           offset,
        UINT32           numberOfReads)
    {
        if (NULL != pRegReadInfo)
        {
            pRegReadInfo->readType                                   = IFERegDumpReadTypeReg;
            pRegReadInfo->regDescriptor.regReadCmd.offset            = offset;
            pRegReadInfo->regDescriptor.regReadCmd.numberOfRegisters = numberOfReads;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillIFEDMIReadInfo
    ///
    /// @brief  Helper function to fill Reg Read Info
    ///
    /// @param  pDMIReadInfo  Pointer to the DMI Read Info to be filled
    /// @param  offset        DMI Reg Offset
    /// @param  bankOffset    DMI Bank Offset
    /// @param  numberOfReads number of DMI Data reads
    /// @param  bankSelect    bank Select
    /// @param  lutSelect     LUT Select
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID FillIFEDMIReadInfo(
        IFERegReadInfo*  pDMIReadInfo,
        UINT32           offset,
        UINT32           bankOffset,
        UINT32           numberOfReads,
        UINT32           bankSelect,
        UINT32           lutSelect)
    {
        if (NULL != pDMIReadInfo)
        {
            pDMIReadInfo->readType = IFERegDumpReadTypeDMI;
            pDMIReadInfo->regDescriptor.dmiReadCmd.regWriteCmd[0].offset = offset;
            pDMIReadInfo->regDescriptor.dmiReadCmd.regWriteCmd[0].value  = 0x100000;
            pDMIReadInfo->regDescriptor.dmiReadCmd.numberOfRegWrites++;
            pDMIReadInfo->regDescriptor.dmiReadCmd.regWriteCmd[1].offset = offset + 0x4;
            pDMIReadInfo->regDescriptor.dmiReadCmd.regWriteCmd[1].value  = lutSelect;
            pDMIReadInfo->regDescriptor.dmiReadCmd.numberOfRegWrites++;
            pDMIReadInfo->regDescriptor.dmiReadCmd.regWriteCmd[2].offset = bankOffset;
            pDMIReadInfo->regDescriptor.dmiReadCmd.regWriteCmd[2].value  = bankSelect;
            pDMIReadInfo->regDescriptor.dmiReadCmd.numberOfRegWrites++;
            pDMIReadInfo->regDescriptor.dmiReadCmd.regRangeCmd.numberOfRegisters = numberOfReads;
            pDMIReadInfo->regDescriptor.dmiReadCmd.regRangeCmd.offset = offset + 0x8;
            pDMIReadInfo->regDescriptor.dmiReadCmd.postregWriteCmd[1].offset = offset + 0x4;
            pDMIReadInfo->regDescriptor.dmiReadCmd.postregWriteCmd[1].value = 0;
            pDMIReadInfo->regDescriptor.dmiReadCmd.numberOfPostWrites++;
        }
    }

    Titan480IFE(const Titan480IFE&)             = delete;   ///< Disallow the copy constructor
    Titan480IFE& operator=(const Titan480IFE&)  = delete;   ///< Disallow assignment operator

    UINT32 m_hangDumpOutputBufferSize           = 0;        ///< Hang Dump Buffer size in Bytes
    UINT32 m_perFrameDumpOutputBufferSize       = 0;        ///< Per Frame Dump Buffer size in Bytes
};

CAMX_NAMESPACE_END

#endif // CAMXTITAN480IFE_H
