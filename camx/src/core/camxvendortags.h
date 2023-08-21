////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxvendortags.h
/// @brief Interface and type defintion to define and query supported vendor tags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXVENDORTAGS_H
#define CAMXVENDORTAGS_H

#include "camxcommontypes.h"
#include "camxdefs.h"
#include "camxhashmap.h"
#include "camxpropertyblob.h"
#include "chivendortag.h"
#include "chiasdinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Type Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Define vendor tag masks
static const UINT32 NumOfVendorTagOffsetBit   = 16;
static const UINT32 VendorTagOffsetMask       = (0x1 << NumOfVendorTagOffsetBit) - 1;
static const UINT32 VendorTagBaseMask         = ~VendorTagOffsetMask;

/// @breif Enums for internal vendor tag section, these section will not exposed outside of driver
enum VendorTagSection
{
    VendorTagSectionPrivate                   = MetadataSectionVendorSection, ///< VenderTag section
    VendorTagSectionPrivateEnd                                                ///< VenderTag section
};

/// @brief Define the private vendor tag
typedef UINT32 VendorTag;

static const VendorTag VendorTagPrivate = static_cast<VendorTag>(VendorTagSectionPrivate << NumOfVendorTagOffsetBit);

/// @brief Define enum to tag visibility
enum TagSectionVisibility : UINT32
{
    TagSectionVisibleToOEM               = 0x1,              ///< VenderTag section visible to OEM
    TagSectionVisibleToFramework         = 0x2,              ///< VenderTag section visible to Android framework
    TagSectionVisibleToAll               = 0xFFFFFFFF,       ///< VenderTag section visible to All
};

/// @brief Encapsulates essential data describing a vendor tag
struct VendorTagData
{
    const CHAR*     pVendorTagName; ///< The string representation of the vendor tag name
    VendorTagType   vendorTagType;  ///< The type of the vendor tag
    SIZE_T          numUnits;      ///< The number of units of vendorTagType needed to program this tag
};

/// @brief Encapsulates essential data describing a vendor tag section
struct VendorTagSectionData
{
    const CHAR*          pVendorTagSectionName;              ///< The string representing the vendor tag section name
    VendorTag            firstVendorTag;                     ///< The first vendor tag in the vendor tag section
    UINT32               numTags;                            ///< The number of vendor tags in the section
    VendorTagData*       pVendorTagaData;                    ///< An array of vendor tag data arrays
    TagSectionVisibility visbility;                          ///< Visibility of this tag section
};

/// @brief Camera  vendor tags capabilities. These capabilities will be used to populate suported vendor tags by node.
struct VendorTagInfo
{
    VendorTagSectionData* pVendorTagDataArray;               ///< An array of vendor tag section
    UINT32                numSections;                       ///< The number of vendor tag section in pVendorTagDataArray
};

/// @brief Mapped Control Vendor Tag. These vendor tag IDs are populated at initialization to help in lookup
struct MappedControlVendorTag
{
    UINT32 AECControlVendorTag;                    ///< Tag ID for AEC Control Vendor Tag
    UINT32 AWBControlVendorTag;                    ///< Tag ID for AWB Control Vendor Tag
    UINT32 AFControlVendorTag;                     ///< Tag ID for AF Control Vendor Tag
    UINT32 AFDControlVendorTag;                    ///< Tag ID for AFD Control Vendor Tag
    UINT32 IHistControlVendorTag;                  ///< Tag ID for iHist Control Vendor Tag
    UINT32 AECFrameVendorTag;                      ///< Tag ID for AEC Frame Vendor Tag
    UINT32 AWBFrameVendorTag;                      ///< Tag ID for AWB Frame Vendor Tag
    UINT32 SensorGainVendorTag;                    ///< Tag ID for Sensor Vendor Tag
    UINT32 IFEResidualCropVendorTag;               ///< Tag ID for IFE Residual Crop Vendor Tag
    UINT32 IFEAppliedCropVendorTag;                ///< Tag ID for IFE Applied Crop Vendor Tag
    UINT32 ModifiedCropWindow;                     ///< Tag ID for Sensor+IFE Applied Crop Vendor Tag
    UINT32 GammaOuputVendorTag;                    ///< Tag ID for IFE/BPS Gammaoutput Vendor Tag
    UINT32 MFNRTotalNumFrames;                     ///< Tag ID for MFNR Total Number of Frames Tag
    UINT32 MFSRTotalNumFrames;                     ///< Tag ID for MFSR Total Number of Frames Tag
    UINT32 IntermediateDimensionTag;               ///< Tag ID for Intermediate dimension used in MFSR
};

/// @brief Camera  vendor tags capabilities. These capabilities will be used to populate suported vendor tags by external components.
struct ComponentVendorTagsInfo
{
    VendorTagInfo*        pVendorTagInfoArray;               ///< An array of vendor tag info
    UINT32                numVendorTagInfo;                  ///< The number of vendortag info section in pVendorTagInfoArray
};

/// @brief org.quic.camera2.oemfdconfig section for FD config
static VendorTagData g_VendorTagSectionOEMFDConfig[] =
{
    { VendorTagNameOEMFDConfig,  VendorTagType::Byte, sizeof(FDConfig) },
};

/// @brief org.quic.camera2.oemfdresults section for FD results
static VendorTagData g_VendorTagSectionOEMFDResults[] =
{
    { VendorTagNameOEMFDResults,  VendorTagType::Byte, sizeof(FaceROIInformation) },
};

/// @brief org.quic.camera2.objectTrackingConfig section for Tracker configure
static VendorTagData g_VendorTagSectionTrackerConfig[] =
{
    { "Enable",         VendorTagType::Byte, 1 },
    { "CmdTrigger",     VendorTagType::Int32, 1 },
    { "RegisterROI",    VendorTagType::Int32, 4 },
};

/// @brief org.quic.camera2.objectTrackingResults section for Tracker results
static VendorTagData g_VendorTagSectionTrackerResults[] =
{
    { "TrackerStatus",  VendorTagType::Int32, 1  },
    { "ResultROI",      VendorTagType::Int32, 4  },
    { "TrackerScore",   VendorTagType::Int32, 1  },
};

/// @brief org.quic.camera.eis_realtime section
static VendorTagData g_VendorTagSectionEISRealTimeConfig[] =
{
    { "Enabled",              VendorTagType::Byte, 1 },
    { "RequestedMargin",      VendorTagType::Byte, sizeof(MarginRequest) },
    { "StabilizationMargins", VendorTagType::Byte, sizeof(StabilizationMargin) },
    { "AdditionalCropOffset", VendorTagType::Byte, sizeof(ImageDimensions) },
    { "StabilizedOutputDims", VendorTagType::Byte, sizeof(CHIDimension) },
    { "MinimalTotalMargins",  VendorTagType::Byte, sizeof(MarginRequest) }
};

/// @brief org.quic.camera.eis_lookahead section
static VendorTagData g_VendorTagSectionEISLookAheadConfig[] =
{
    { "Enabled",              VendorTagType::Byte, 1 },
    { "FrameDelay",           VendorTagType::Byte, sizeof(UINT32) },
    { "RequestedMargin",      VendorTagType::Byte, sizeof(MarginRequest) },
    { "StabilizationMargins", VendorTagType::Byte, sizeof(StabilizationMargin) },
    { "AdditionalCropOffset", VendorTagType::Byte, sizeof(ImageDimensions) },
    { "StabilizedOutputDims", VendorTagType::Byte, sizeof(CHIDimension) },
    { "MinimalTotalMargins",  VendorTagType::Byte, sizeof(MarginRequest) }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief The VendorTagManager class defines the interface to add/query vendor tag information
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class VendorTagManager
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Public Methods
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInstance
    ///
    /// @brief  This function returns the singleton instance of the VendorTagManager.
    ///
    /// @return A pointer to the singleton instance of the VendorTagManager
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VendorTagManager* GetInstance();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryVendorTagSectionBase
    ///
    /// @brief  Query vendor tag section base
    ///
    /// @return CamxResultSuccess if successful otherwise CamxResultNoSuch
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult QueryVendorTagSectionBase(
        const CHAR* pSectionName,
        UINT32*     pSectionBase);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryVendorTagLocation
    ///
    /// @brief  Query vendor tag location assigned by vendor tag manager
    ///
    /// @return CamxResultSuccess if successful otherwise CamxResultNoSuch
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult QueryVendorTagLocation(
        const CHAR* pSectionName,
        const CHAR* pTagName,
        UINT32*     pTagLocation);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTagCount
    ///
    /// @brief  Get the number of vendor tags supported on this platform. Please refer to the get_tag_count documentation in
    ///         system/camera_vendor_tags.h.
    ///
    /// @return Number of vendor tags supported
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 GetTagCount(TagSectionVisibility visibility);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAllTagCount
    ///
    /// @brief  Get the number of vendor tags supported on this platform for all visibilities
    ///
    /// @return Number of vendor tags supported
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE UINT32 GetAllTagCount()
    {
        return GetInstance()->m_sizeAll;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAllTags
    ///
    /// @brief  Fill an array with all of the supported vendor tags on this platform. Please refer to the get_all_tags
    ///         documentation in system/camera_vendor_tags.h.
    ///
    /// @param  pVendorTags An array of supported tags
    /// @param  visibility visibility flag for vendor section
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GetAllTags(
        VendorTag* pVendorTags,
        TagSectionVisibility visibility);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSectionName
    ///
    /// @brief  Get the vendor section name for a vendor-specified entry tag. Please refer to the get_section_name documentation
    ///         in system/camera_vendor_tags.h.
    ///
    /// @param  vendorTag The tag id to retrieve the section name for
    ///
    /// @return The section name
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static const CHAR* GetSectionName(
        VendorTag vendorTag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTagName
    ///
    /// @brief  Get the tag name for a vendor-specified entry tag. Please refer to the get_tag_name documentation in
    ///         system/camera_vendor_tags.h.
    ///
    /// @param  vendorTag The tag id to retrieve the tag name for
    ///
    /// @return The tag name
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static const CHAR* GetTagName(
        VendorTag vendorTag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTagType
    ///
    /// @brief  Get tag type for a vendor-specified entry tag. Please refer to the get_tag_type documentation in
    ///         system/camera_vendor_tags.h.
    ///
    /// @param  vendorTag The tag id to retrieve the tag type for
    ///
    /// @return The tag type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VendorTagType GetTagType(
        VendorTag vendorTag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetVendorTagBlobSize
    ///
    /// @brief  Get the the blob size for all vendor tags
    ///
    /// @param  visibility visibility flag for vendor section
    ///
    /// @return The blob size of all vendor tags
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static SIZE_T GetVendorTagBlobSize(
        TagSectionVisibility visibility);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMappedPropertyID
    ///
    /// @brief  Get the mapped property ID for the Vendor tag if it exists
    ///
    /// @return The Mapped propertyID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static PropertyID GetMappedPropertyID(VendorTag vendorTag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateControlVendorTagId
    ///
    /// @brief  Populate vendor Tag id that are mapped to property ID
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID PopulateControlVendorTagId();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// VendorTagOffset
    ///
    /// @brief  Determines the offset, starting at MaxMetadataTagCount, of a vendor tag, linearly
    ///
    /// @return Offset
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE SIZE_T VendorTagOffset(
        UINT32 tag)
    {
        UINT32 tagVendorSection = ((tag - MetadataSectionVendorSectionStart) & VendorTagBaseMask) >> NumOfVendorTagOffsetBit;

        // section offset in tags via LUT + add tag offset within section
        return GetInstance()->m_pSectionOffsets[tagVendorSection] + (tag & VendorTagOffsetMask);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// VendorTagSize
    ///
    /// @brief  Determines the size of a tag
    ///
    /// @return Size
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE SIZE_T VendorTagSize(
        SIZE_T tagIndex)
    {
        return GetInstance()->m_pTagSizes[tagIndex];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTagVisibility
    ///
    /// @brief  Get the visibility of the tag
    ///
    /// @param  vendorTag vendor tag
    ///
    /// @return visibility visibility flag for vendor section
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static TagSectionVisibility GetTagVisibility(
            VendorTag vendorTag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetVendorTagBlobSize
    ///
    /// @brief  Get the the blob size for all vendor tags
    ///
    /// @param  vendorTag vendor tag
    ///
    /// @return TRUE is vendor tag FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL isVendorTag(
            VendorTag vendorTag);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Private Methods
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// VendorTagManager
    ///
    /// @brief  Default constructor for the VendorTagManager class
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VendorTagManager();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~VendorTagManager
    ///
    /// @brief  Default destructor for the VendorTagManager class
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~VendorTagManager();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~InitializeVendorTagInfo
    ///
    /// @brief  Collect vendor tag information from system
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitializeVendorTagInfo();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AppendVendorTagInfo
    ///
    /// @brief  Append vendor tags sections to existing vendor tag sections
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AppendVendorTagInfo(const VendorTagInfo* pVendorTagInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyVendorTagSectionData
    ///
    /// @brief  Deep copy src section to dst section
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CopyVendorTagSectionData(
        VendorTagSectionData* pDstSection,
        const VendorTagSectionData* pSrcSection);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FreeVendorTagSectionData
    ///
    /// @brief  Free section data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FreeVendorTagSectionData(VendorTagSectionData* pSection);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FreeVendorTagInfo
    ///
    /// @brief  Free vendor tag info
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FreeVendorTagInfo(VendorTagInfo* pVendorTagInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNextAvailableSectionBase
    ///
    /// @brief  GetNextAvailable section base
    ///
    /// @return next available section base
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 GetNextAvailableSectionBase()
    {
        return m_nextSectionBase++;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetVendorTagInfo
    ///
    /// @brief  This function returns vendor tag information.
    ///
    /// @return A pointer to the vendor tag information
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const VendorTagInfo* GetVendorTagInfo()
    {
        return &m_vendorTagInfo;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddTagToHashMap
    ///
    /// @brief  Adds the key string to the Vendor Tag hash map
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AddTagToHashMap(
        const CHAR* pKeyString,
        UINT32*     pTagLocation);

    // Do not implement the copy constructor or assignment operator
    VendorTagManager(const VendorTagManager& rVendorTagManager)             = delete;
    VendorTagManager& operator= (const VendorTagManager& rVendorTagManager) = delete;

    UINT32*          m_pSectionOffsets;  ///< LUT for the offset of each section in number of tags
    SIZE_T*          m_pTagSizes;        ///< Size of data backing each tag
    UINT32           m_sizeAll;          ///< Count of all vendor tags
    VendorTagInfo    m_vendorTagInfo;    ///< container for vendor tag inforamtion
    UINT32           m_nextSectionBase;  ///< available section base for next section
    BOOL             m_initialized;      ///< if the vendor tag information has been initialized
    Hashmap*         m_pLocationMap;     ///< Hashmap to store tag loation
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SectionTagToIndex
///
/// @brief Converts a section tag value into a contiguous index from 0 to MaxMetadataTagCount - 1
///
/// @return index if successful, else InvalidIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE UINT32 SectionTagToIndex(UINT32 tag) {
    UINT32 index = ((tag & ((~DriverInternalGroupMask) & 0xFFFF0000)) >> 16);
    if (index >= CAMX_ARRAY_SIZE(SectionLinearTagLUT))
    {
        return InvalidIndex;
    }
    return ((tag & 0xFFFF) + SectionLinearTagLUT[index]);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAMX_TAG_TO_INDEX
///
/// @brief Converts a tag value into a contiguous index from 0 to MaxMetadataTagCount - 1, with vendor tags following
///
/// @param tag the tag to convert
///
/// @note AND with DriverInternalMask removes identifiers that do not define the underlying tag to be used
///
/// @return index of tag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_TAG_TO_INDEX(tag)                                                                              \
    (0 == (tag & MetadataSectionVendorSectionStart))                                                      ? \
        SectionTagToIndex(tag)                                                                            : \
        MaxMetadataTagCount + static_cast<UINT32>(VendorTagManager::VendorTagOffset((tag & (~DriverInternalGroupMask))))

#define MAX_SECTION_TAG_COMBINED_LEN 128

CAMX_NAMESPACE_END

#endif // CAMXVENDORTAGS_H
