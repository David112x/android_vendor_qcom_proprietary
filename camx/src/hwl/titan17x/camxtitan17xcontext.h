////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxtitan17xcontext.h
/// @brief Titan17xContext class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXTITAN17XCONTEXT_H
#define CAMXTITAN17XCONTEXT_H

#include "camxhwcontext.h"
#include "camxhwenvironment.h"
#include "camxtitan17xdefs.h"
#include "camxtitan17xsettingsmanager.h"
#include "camxvendortags.h"
#include "titan170_cpas_top.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the context to create HW dependent object hierarchies.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Titan17xContext final : public HwContext
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create an instance of the device HwContext
    ///
    /// @param  pCreateData Hardware context create data.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        HwContextCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStaticMetadataKeysInfo
    ///
    /// @brief  Retrieve static available metadata keys array info by metadata tag from the HwContext.
    ///
    /// @param  pKeysInfo  Static available metadata keys info pointer to be filled in.
    /// @param  tag        Static metadata type to be retrieved.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetStaticMetadataKeysInfo(
        StaticMetadataKeysInfo* pKeysInfo,
        CameraMetadataTag       tag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryVendorTagsInfo
    ///
    /// @brief  Retrieve supported vendor tags
    ///
    /// @param  pVendorTagInfo   Vendor tag info to be populated.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult QueryVendorTagsInfo(
        VendorTagInfo* pVendorTagInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryExternalComponentVendorTagsInfo
    ///
    /// @brief  Retrieve supported External component vendor tags
    ///
    /// @param  pComponentVendorTagsInfo  Vendor tag info to be populated.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult QueryExternalComponentVendorTagsInfo(
        ComponentVendorTagsInfo* pComponentVendorTagsInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStaticCaps
    ///
    /// @brief  Retrieve static capabilities for the platform from the HwContext. This does not include capabilities of the
    ///         camera sensor.
    ///
    /// @param  pCaps   Static capabilities to be populated.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetStaticCaps(
        PlatformStaticCaps* pCaps);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetHWBugWorkarounds
    ///
    /// @brief  Retrieve static list of the known HW bug workarounds and their properties for the platform from the HwContext.
    ///
    /// @param  pWorkarounds   Static workarounds to be populated.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetHWBugWorkarounds(
        HWBugWorkarounds* pWorkarounds);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumberOfIPE
    ///
    /// @brief  Query CSL and return an integer with number of IPEs.
    ///
    /// @return An integer with number of IPEs filled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    virtual UINT32 GetNumberOfIPE();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeSOCDependentParams
    ///
    /// @brief  Initialize the SOC dependent params
    ///
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult InitializeSOCDependentParams();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~Titan17xContext
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~Titan17xContext();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HwInitialize
    ///
    /// @brief  Initialize the hwl object
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult HwInitialize();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsNodeTypeSinkNoBuffer
    ///
    /// @brief  Pure virtual method to query if it is a Sink node with no output target
    ///
    /// @param  nodeType Node Type
    ///
    /// @return TRUE if it is a Sink node with no output target, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsNodeTypeSinkNoBuffer(
        UINT nodeType) const
    {
        return ((SinkNoBuffer == nodeType) ? TRUE : FALSE);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStatsParser
    ///
    /// @brief  Returns the stats parser object.
    ///
    /// @return The stats parser object.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual StatsParser* GetStatsParser();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTitan17xSettingsManager
    ///
    /// @brief  Returns a pointer to the hardware-specific settings manager.
    ///
    /// @return A pointer to the hardware-specific settings manager.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const static Titan17xSettingsManager* GetTitan17xSettingsManager()
    {
        return reinterpret_cast<const Titan17xSettingsManager*>(HwEnvironment::GetInstance()->GetSettingsManager());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTitanVersion
    ///
    /// @brief  Query CSL and return an integer with the titan version.
    ///
    /// @return An integer with titan version filled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 GetTitanVersion()
    {
        CSLCameraPlatform   CSLPlatform          = {};
        CamxResult          result               = CamxResultSuccess;
        UINT32              titanVersion         = 0;
        UINT32              titanVersionOverride = 0;

        result = CSLQueryCameraPlatform(&CSLPlatform);

        if (CamxResultSuccess == result)
        {
            titanVersion =
                (CSLPlatform.platformVersion.majorVersion << CPAS_TOP_CPAS_0_TITAN_VERSION_GENERATION_SHIFT) |
                (CSLPlatform.platformVersion.minorVersion << CPAS_TOP_CPAS_0_TITAN_VERSION_TIER_SHIFT) |
                (CSLPlatform.platformVersion.revVersion   << CPAS_TOP_CPAS_0_TITAN_VERSION_STEP_SHIFT);

            titanVersionOverride = GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->camVersionOverride;

            if (titanVersionOverride != 0)
            {
                titanVersion = titanVersionOverride;
                CAMX_LOG_INFO(CamxLogGroupCore, "Titan version override = 0x%x", titanVersion);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Titan17xContext returned invalid titan version");
        }

        CAMX_ASSERT(0 != titanVersion);
        return titanVersion;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetHwVersion
    ///
    /// @brief  Query CSL and return an integer with the hardware version.
    ///
    /// @return An integer with hardware version filled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 GetHwVersion()
    {
        CSLCameraPlatform   CSLPlatform = {};
        CamxResult          result      = CamxResultSuccess;
        UINT32              hwVersion   = 0;

        result = CSLQueryCameraPlatform(&CSLPlatform);

        if (CamxResultSuccess == result)
        {
            hwVersion =
                (CSLPlatform.CPASVersion.majorVersion << CPAS_TOP_CPAS_0_HW_VERSION_MAJOR_SHIFT) |
                (CSLPlatform.CPASVersion.minorVersion << CPAS_TOP_CPAS_0_HW_VERSION_MINOR_SHIFT) |
                (CSLPlatform.CPASVersion.revVersion   << CPAS_TOP_CPAS_0_HW_VERSION_STEP_SHIFT);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Titan17xContext returned invalid hw version");
        }

        CAMX_ASSERT(0 != hwVersion);
        return hwVersion;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTitanChipVersion
    ///
    /// @brief  Query CSL and return Titan Chip Version
    ///
    /// @return An integer with hardware version filled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE static CSLCameraTitanChipVersion GetTitanChipVersion()
    {
        CSLCameraPlatform      CSLPlatform = {};
        CamxResult             result = CamxResultSuccess;
        UINT32                 platformVersion = 0;
        CSLCameraTitanVersion  titanVersion;
        CSLCameraTitanChipVersion titanChipVersion = CSLTitanNone;

        result = CSLQueryCameraPlatform(&CSLPlatform);

        if (CamxResultSuccess == result)
        {
            platformVersion =
                (CSLPlatform.platformVersion.majorVersion << CPAS_TOP_CPAS_0_TITAN_VERSION_GENERATION_SHIFT) |
                (CSLPlatform.platformVersion.minorVersion << CPAS_TOP_CPAS_0_TITAN_VERSION_TIER_SHIFT) |
                (CSLPlatform.platformVersion.revVersion << CPAS_TOP_CPAS_0_TITAN_VERSION_STEP_SHIFT);

            titanVersion = static_cast<CSLCameraTitanVersion >(platformVersion);
            switch (titanVersion)
            {
                case CSLCameraTitanVersion::CSLTitan170:
                    if (CSLPlatform.CPASVersion.majorVersion == 1)
                    {
                        if ((CSLPlatform.CPASVersion.minorVersion == 0) &&
                            (CSLPlatform.CPASVersion.revVersion == 0))
                        {
                            titanChipVersion = CSLTitan170V100;
                        }
                        else if ((CSLPlatform.CPASVersion.minorVersion == 1) &&
                                 (CSLPlatform.CPASVersion.revVersion == 0))
                        {
                            titanChipVersion = CSLTitan170V110;
                        }
                        else if ((CSLPlatform.CPASVersion.minorVersion == 2) &&
                                 (CSLPlatform.CPASVersion.revVersion == 0))
                        {
                            titanChipVersion = CSLTitan170V120;
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupISP, "Unsupported chip version %d,%d,%d",
                                CSLPlatform.CPASVersion.majorVersion,
                                CSLPlatform.CPASVersion.minorVersion,
                                CSLPlatform.CPASVersion.revVersion);
                        }
                    }
                    else if (CSLPlatform.CPASVersion.majorVersion == 2)
                    {
                        if ((CSLPlatform.CPASVersion.minorVersion == 0) &&
                            (CSLPlatform.CPASVersion.revVersion == 0))
                        {
                            titanChipVersion = CSLTitan170V200;
                        }
                    }
                    break;

                case CSLCameraTitanVersion::CSLTitan175:
                    if (CSLPlatform.CPASVersion.majorVersion == 1)
                    {
                        if ((CSLPlatform.CPASVersion.minorVersion == 0) &&
                            (CSLPlatform.CPASVersion.revVersion == 0))
                        {
                            titanChipVersion = CSLTitan175V100;
                        }
                        else if ((CSLPlatform.CPASVersion.minorVersion == 0) &&
                            (CSLPlatform.CPASVersion.revVersion == 1))
                        {
                            titanChipVersion = CSLTitan175V101;
                        }
                        else if ((CSLPlatform.CPASVersion.minorVersion == 2) &&
                            (CSLPlatform.CPASVersion.revVersion == 0))
                        {
                            titanChipVersion = CSLTitan175V120;
                        }
                        else if ((CSLPlatform.CPASVersion.minorVersion == 3) &&
                            (CSLPlatform.CPASVersion.revVersion == 0))
                        {
                            titanChipVersion = CSLTitan175V130;
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupISP, "Unsupported chip version %d,%d,%d",
                                CSLPlatform.CPASVersion.majorVersion,
                                CSLPlatform.CPASVersion.minorVersion,
                                CSLPlatform.CPASVersion.revVersion);
                        }
                    }
                    break;

                case CSLCameraTitanVersion::CSLTitan150:
                    if (CSLPlatform.CPASVersion.majorVersion == 1)
                    {
                        if ((CSLPlatform.CPASVersion.minorVersion == 0) &&
                            (CSLPlatform.CPASVersion.revVersion == 0))
                        {
                            titanChipVersion = CSLTitan150V100;
                        }
                        else if ((CSLPlatform.CPASVersion.minorVersion == 1) &&
                            (CSLPlatform.CPASVersion.revVersion == 0))
                        {
                            titanChipVersion = CSLTitan150V110;
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupISP, "Unsupported chip version %d,%d,%d",
                                CSLPlatform.CPASVersion.majorVersion,
                                CSLPlatform.CPASVersion.minorVersion,
                                CSLPlatform.CPASVersion.revVersion);
                        }
                    }
                    break;

                case CSLCameraTitanVersion::CSLTitan480:
                    if (CSLPlatform.CPASVersion.majorVersion == 1)
                    {
                        if ((CSLPlatform.CPASVersion.minorVersion == 0) &&
                            (CSLPlatform.CPASVersion.revVersion == 0))
                        {
                            titanChipVersion = CSLTitan480V100;
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupISP, "Unsupported chip version %d,%d,%d",
                                CSLPlatform.CPASVersion.majorVersion,
                                CSLPlatform.CPASVersion.minorVersion,
                                CSLPlatform.CPASVersion.revVersion);
                        }
                    }
                    break;

                default:
                    result = CamxResultEUnsupported;
                    CAMX_LOG_ERROR(CamxLogGroupISP, "Unsupported chip version %d,%d,%d",
                        CSLPlatform.CPASVersion.majorVersion,
                        CSLPlatform.CPASVersion.minorVersion,
                        CSLPlatform.CPASVersion.revVersion);
                    break;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Titan17xContext returned invalid hw version");
        }

        return titanChipVersion;
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUBWCLossyThreshold0
    ///
    /// @brief  Calculate the UBWC partial tile information
    ///
    /// @param  version    UBWC version
    /// @param  path       UBWC path
    /// @param  pFormat    image format pointer
    ///
    /// @return UBWC Lossy threshold value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetUBWCLossyThreshold0(
        UINT32 version,
        UINT32 path,
        const ImageFormat* pFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUBWCLossyThreshold1
    ///
    /// @brief  Calculate the UBWC partial tile information
    ///
    /// @param  version    UBWC version
    /// @param  path       UBWC path
    /// @param  pFormat    image format pointer
    ///
    /// @return UBWC Lossy threshold value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetUBWCLossyThreshold1(
        UINT32 version,
        UINT32 path,
        const ImageFormat* pFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUBWCBandwidthLimit
    ///
    /// @brief  Get ubwc bandwidth limit
    ///
    /// @param  version       UBWC version
    /// @param  path          UBWC path
    /// @param  planeIndex    plane Index
    /// @param  pFormat       image format pointer
    ///
    /// @return UBWC Bandwidth Limit
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetUBWCBandwidthLimit(
        UINT32 version,
        UINT32 path,
        UINT32 planeIndex,
        const ImageFormat* pFormat);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Titan17xContext
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Titan17xContext();

    // Do not implement the copy constructor or assignment operator
    Titan17xContext(const Titan17xContext&) = delete;
    Titan17xContext& operator=(const Titan17xContext&) = delete;
};

CAMX_NAMESPACE_END

#endif // CAMXTITAN17XCONTEXT_H
