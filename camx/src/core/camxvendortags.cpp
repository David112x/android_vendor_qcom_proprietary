////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxvendortags.cpp
/// @brief VendorTagManager class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxhal3metadatautil.h"
#include "camxhal3module.h"
#include "camxvendortags.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Static Data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static MappedControlVendorTag g_controlVendorTag; ///< Control Vendor tag IDs

/// Vendor tags defined per section,  here we should only define CHI level or node-agnostic vendor tag section
/// @todo (CAMX-1388) move node specific vendor tag definition to respective node and update the numUnits info

static VendorTagData g_VendorTagSectionHALPrivate[] =
{
    { "reprocess_flags",        VendorTagType::Byte , 1 },
    { "reprocess_data_blob",    VendorTagType::Byte , 1 },
    { "exif_debug_data_blob",   VendorTagType::Byte , 1 },
    { "debug_image_dump",       VendorTagType::Byte , 1 },
    { "debug_image_dump_mask",  VendorTagType::Int32, 1 },
};

static VendorTagData g_VendorTagSensorBps[] =
{
    { "mode_index",      VendorTagType::Int32 , 1 },
    { "gain",            VendorTagType::Float , 1 }
};

static VendorTagData g_VendorTagSectionInernalPrivate[] =
{
    { "private_property", VendorTagType::Byte, PropertyBlobSize } // tagId + value + number of tags
};

static VendorTagData g_VendorTagStatsControl[] =
{
    { "is_flash_snapshot", VendorTagType::Int32, 1 },
    { "is_lls_needed",     VendorTagType::Int32, 1 }
};

static VendorTagData g_VendorTagSeamlessInSensorControl[] =
{
    { "seamless_insensor_state",   VendorTagType::Int32, 1 }
};

/// This is an array of all vendor tag section data
static VendorTagSectionData g_CamXVendorTagSections[] =
{
    {
        "org.codeaurora.qcamera3.internal_private", VendorTagPrivate,
        CAMX_ARRAY_SIZE(g_VendorTagSectionInernalPrivate), g_VendorTagSectionInernalPrivate,
        TagSectionVisibility::TagSectionVisibleToOEM
    },
    {
        "com.qti.sensorbps",  0,
        CAMX_ARRAY_SIZE(g_VendorTagSensorBps), g_VendorTagSensorBps,
        TagSectionVisibility::TagSectionVisibleToOEM
    },
    {
        "org.codeaurora.qcamera3.hal_private_data",  0,
        CAMX_ARRAY_SIZE(g_VendorTagSectionHALPrivate), g_VendorTagSectionHALPrivate,
        TagSectionVisibility::TagSectionVisibleToOEM
    },
    {
        "com.qti.stats_control",  0,
        CAMX_ARRAY_SIZE(g_VendorTagStatsControl), g_VendorTagStatsControl,
        TagSectionVisibility::TagSectionVisibleToOEM
    },
    {
        "com.qti.insensor_control",  0,
        CAMX_ARRAY_SIZE(g_VendorTagSeamlessInSensorControl), g_VendorTagSeamlessInSensorControl,
        TagSectionVisibility::TagSectionVisibleToOEM
    },
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VendorTagManager::GetTagCount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 VendorTagManager::GetTagCount(
    TagSectionVisibility visibility)
{
    const VendorTagInfo* pVendorTagInfo = VendorTagManager::GetInstance()->GetVendorTagInfo();
    UINT32               count          = 0;

    if (NULL != pVendorTagInfo)
    {
        for (UINT32 i = 0; i < pVendorTagInfo->numSections; i++)
        {
            if ((pVendorTagInfo->pVendorTagDataArray[i].visbility == TagSectionVisibleToAll) ||
                    visibility == TagSectionVisibleToAll ||
                    Utils::IsBitMaskSet(pVendorTagInfo->pVendorTagDataArray[i].visbility, visibility))
            {
                count += pVendorTagInfo->pVendorTagDataArray[i].numTags;
            }
        }
    }

    return count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VendorTagManager::GetAllTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID VendorTagManager::GetAllTags(
    VendorTag*           pVendorTags,
    TagSectionVisibility visibility)
{
    const VendorTagInfo* pVendorTagInfo = VendorTagManager::GetInstance()->GetVendorTagInfo();
    UINT32               size           = 0;

    if (NULL != pVendorTagInfo)
    {
        for (UINT32 i = 0; i < pVendorTagInfo->numSections; i++)
        {
            if ((pVendorTagInfo->pVendorTagDataArray[i].visbility == TagSectionVisibleToAll) ||
                    TagSectionVisibleToAll == visibility ||
                    Utils::IsBitMaskSet(pVendorTagInfo->pVendorTagDataArray[i].visbility, visibility))
            {
                for (UINT32 j = 0; j < pVendorTagInfo->pVendorTagDataArray[i].numTags; j++)
                {
                    pVendorTags[size + j] = pVendorTagInfo->pVendorTagDataArray[i].firstVendorTag + j;
                }

                size += pVendorTagInfo->pVendorTagDataArray[i].numTags;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VendorTagManager::GetSectionName
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const CHAR* VendorTagManager::GetSectionName(
    VendorTag vendorTag)
{
    const VendorTagInfo* pVendorTagInfo        = VendorTagManager::GetInstance()->GetVendorTagInfo();
    UINT32               vendorSection         = vendorTag >> NumOfVendorTagOffsetBit;
    const CHAR*          pVendorTagSectionName = NULL;

    if (NULL != pVendorTagInfo)
    {
        if (vendorSection >= VendorTagSectionPrivate &&
            vendorSection < (VendorTagSectionPrivate + pVendorTagInfo->numSections))
        {
            UINT32 vendorSectionIndex = vendorSection - VendorTagSectionPrivate;
            pVendorTagSectionName = pVendorTagInfo->pVendorTagDataArray[vendorSectionIndex].pVendorTagSectionName;
        }
    }

    return pVendorTagSectionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VendorTagManager::GetTagName
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const CHAR* VendorTagManager::GetTagName(
    VendorTag vendorTag)
{
    const VendorTagInfo* pVendorTagInfo = GetInstance()->GetVendorTagInfo();
    UINT32 vendorSection                = vendorTag >> NumOfVendorTagOffsetBit;
    UINT32 vendorTagIndex               = vendorTag & VendorTagOffsetMask;
    const CHAR* pVendorTagName          = NULL;

    if (NULL != pVendorTagInfo && vendorSection >= VendorTagSectionPrivate)
    {
        UINT32 vendorSectionIndex = vendorSection - VendorTagSectionPrivate;
        if (vendorSection < (VendorTagSectionPrivate + pVendorTagInfo->numSections) &&
            vendorTag < (pVendorTagInfo->pVendorTagDataArray[vendorSectionIndex].firstVendorTag
                         + pVendorTagInfo->pVendorTagDataArray[vendorSectionIndex].numTags))
        {
            pVendorTagName =
                pVendorTagInfo->pVendorTagDataArray[vendorSectionIndex].pVendorTagaData[vendorTagIndex].pVendorTagName;
        }
    }

    return pVendorTagName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VendorTagManager::isVendorTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL VendorTagManager::isVendorTag(
    VendorTag vendorTag)
{
    const VendorTagInfo* pVendorTagInfo = VendorTagManager::GetInstance()->GetVendorTagInfo();
    UINT32 vendorSection                = vendorTag >> NumOfVendorTagOffsetBit;

    if (vendorSection >= VendorTagSectionPrivate &&
        vendorSection < (VendorTagSectionPrivate + pVendorTagInfo->numSections))
    {
        UINT32 vendorSectionIndex = vendorSection - VendorTagSectionPrivate;
        if (vendorTag < (pVendorTagInfo->pVendorTagDataArray[vendorSectionIndex].firstVendorTag
                         + pVendorTagInfo->pVendorTagDataArray[vendorSectionIndex].numTags))
        {
            return TRUE;
        }
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VendorTagManager::GetTagVisibility
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TagSectionVisibility VendorTagManager::GetTagVisibility(
    VendorTag vendorTag)
{
    if (isVendorTag(vendorTag))
    {
        const VendorTagInfo* pVendorTagInfo = VendorTagManager::GetInstance()->GetVendorTagInfo();
        UINT32 vendorSection                = vendorTag >> NumOfVendorTagOffsetBit;
        UINT32 vendorSectionIndex           = vendorSection - VendorTagSectionPrivate;

        return pVendorTagInfo->pVendorTagDataArray[vendorSectionIndex].visbility;
    }
    return TagSectionVisibleToAll;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VendorTagManager::GetTagType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VendorTagType VendorTagManager::GetTagType(
    VendorTag vendorTag)
{
    const VendorTagInfo* pVendorTagInfo = VendorTagManager::GetInstance()->GetVendorTagInfo();
    UINT32 vendorSection                = vendorTag >> NumOfVendorTagOffsetBit;
    UINT32 vendorTagIndex               = vendorTag & VendorTagOffsetMask;
    VendorTagType vendorTagType         = VendorTagType::Error;

    if (NULL != pVendorTagInfo && vendorSection >= VendorTagSectionPrivate)
    {
        UINT32 vendorSectionIndex = vendorSection - VendorTagSectionPrivate;
        if (vendorSection < (VendorTagSectionPrivate + pVendorTagInfo->numSections) &&
            vendorTag < (pVendorTagInfo->pVendorTagDataArray[vendorSectionIndex].firstVendorTag
                      + pVendorTagInfo->pVendorTagDataArray[vendorSectionIndex].numTags))
        {
            vendorTagType
                = pVendorTagInfo->pVendorTagDataArray[vendorSectionIndex].pVendorTagaData[vendorTagIndex].vendorTagType;
        }
    }

    return vendorTagType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VendorTagManager::QueryVendorTagSectionBase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult VendorTagManager::QueryVendorTagSectionBase(
    const CHAR* pSectionName,
    UINT32*     pSectionBase)
{
    CamxResult           result         = CamxResultENoSuch;
    const VendorTagInfo* pVendorTagInfo = VendorTagManager::GetInstance()->GetVendorTagInfo();

    for (UINT32 i = 0; i < pVendorTagInfo->numSections; i++)
    {
        if (OsUtils::StrCmp(pVendorTagInfo->pVendorTagDataArray[i].pVendorTagSectionName, pSectionName) == 0)
        {
            *pSectionBase = (pVendorTagInfo->pVendorTagDataArray[i].firstVendorTag & VendorTagBaseMask);
            result        = CamxResultSuccess;
            break;
        }
    }

    if (CamxResultENoSuch == result)
    {
        CAMX_LOG_WARN(CamxLogGroupCore, "Vendor tag section %s does not exist!", pSectionName);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VendorTagManager::AddTagToHashMap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult VendorTagManager::AddTagToHashMap(
    const CHAR* pKeyString,
    UINT32*     pTagLocation)
{
    CamxResult result = CamxResultEInvalidPointer;

    if (NULL != pKeyString)
    {
        result = CamxResultEFailed;
        if (NULL != VendorTagManager::GetInstance()->m_pLocationMap)
        {
            const SIZE_T keyStringLen = OsUtils::StrLen(pKeyString);
            if (MAX_SECTION_TAG_COMBINED_LEN > keyStringLen)
            {
                VendorTagManager::GetInstance()->m_pLocationMap->Put(
                    // NOWHINE CP036: Put doesn't take a const
                    static_cast<VOID *>(const_cast<CHAR*>(pKeyString)), pTagLocation);
                CAMX_LOG_VERBOSE(CamxLogGroupCore, "put hash %s into Locationmap!", pKeyString);
                result = CamxResultSuccess;
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupCore, "Failed to add hash %s", pKeyString);
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VendorTagManager::QueryVendorTagLocation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult VendorTagManager::QueryVendorTagLocation(
    const CHAR* pSectionName,
    const CHAR* pTagName,
    UINT32*     pTagLocation)
{
    CamxResult           result         = CamxResultENoSuch;
    const VendorTagInfo* pVendorTagInfo = VendorTagManager::GetInstance()->GetVendorTagInfo();

    CHAR hash[MAX_SECTION_TAG_COMBINED_LEN] = {0};
    BOOL validKeyStr = FALSE;

    if (NULL != VendorTagManager::GetInstance()->m_pLocationMap)
    {
        const SIZE_T maxCharsAllowed = (MAX_SECTION_TAG_COMBINED_LEN) - 1;

        SIZE_T lenSectionName = OsUtils::StrLCpy(hash, pSectionName, MAX_SECTION_TAG_COMBINED_LEN);
        SIZE_T totalCopied    = lenSectionName;

        if (maxCharsAllowed > totalCopied)
        {
            totalCopied += OsUtils::StrLCpy(hash + lenSectionName, pTagName, MAX_SECTION_TAG_COMBINED_LEN - lenSectionName);

            // Only need additional checks if we've maxed out the buffer, but we should still allow the case
            // where we've used the complete maxCharsAllowed
            if (maxCharsAllowed <= totalCopied)
            {
                SIZE_T lenTag = OsUtils::StrLen(pTagName);
                if (maxCharsAllowed == (lenSectionName + lenTag))
                {
                    validKeyStr = TRUE;
                    result = VendorTagManager::GetInstance()->m_pLocationMap->Get(&hash, pTagLocation);
                }
            }
            else
            {
                validKeyStr = TRUE;
                result = VendorTagManager::GetInstance()->m_pLocationMap->Get(&hash, pTagLocation);
            }

            if (CamxResultSuccess == result)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCore, "Vendor tag section %s, tag %s exists in hash table, location is %x...",
                           pSectionName, pTagName, *pTagLocation);
                result = CamxResultSuccess;
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCore,
                    "Hash %s, Vendor tag section %s, tag %s does not exist in hash table, querying...",
                    hash, pSectionName, pTagName);
            }
        }
    }

    if (CamxResultSuccess != result)
    {
        for (UINT32 i = 0; i < pVendorTagInfo->numSections; i++)
        {
            if (0 == OsUtils::StrCmp(pVendorTagInfo->pVendorTagDataArray[i].pVendorTagSectionName, pSectionName))
            {
                for (UINT32 j = 0; j < pVendorTagInfo->pVendorTagDataArray[i].numTags; j++)
                {
                    if (0 == OsUtils::StrCmp(pVendorTagInfo->pVendorTagDataArray[i].pVendorTagaData[j].pVendorTagName,
                                             pTagName))
                    {
                        *pTagLocation = pVendorTagInfo->pVendorTagDataArray[i].firstVendorTag + j;
                        if (TRUE == validKeyStr)
                        {
                            VendorTagManager::GetInstance()->AddTagToHashMap(hash, pTagLocation);
                        }
                        result = CamxResultSuccess;
                        break;
                    }
                }

                break;
            }
        }
    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupCore, "Vendor tag %s in section %s does not exist!", pTagName, pSectionName);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VendorTagManager::GetVendorTagBlobSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T VendorTagManager::GetVendorTagBlobSize(
    TagSectionVisibility visibility)
{
    SIZE_T               size           = 0;
    const VendorTagInfo* pVendorTagInfo = VendorTagManager::GetInstance()->GetVendorTagInfo();

    if (NULL != pVendorTagInfo)
    {
        for (UINT32 i = 0; i < pVendorTagInfo->numSections; i++)
        {
            if ((pVendorTagInfo->pVendorTagDataArray[i].visbility == TagSectionVisibleToAll) ||
                    TagSectionVisibleToAll == visibility ||
                    Utils::IsBitMaskSet(pVendorTagInfo->pVendorTagDataArray[i].visbility, visibility))
            {
                for (UINT32 j = 0; j < pVendorTagInfo->pVendorTagDataArray[i].numTags; j++)
                {
                    size += pVendorTagInfo->pVendorTagDataArray[i].pVendorTagaData[j].numUnits *
                        HAL3MetadataUtil::GetSizeByType(static_cast<UINT8>(
                            pVendorTagInfo->pVendorTagDataArray[i].pVendorTagaData[j].vendorTagType));
                }
            }
        }
    }

    return size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VendorTagManager::GetMappedPropertyID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PropertyID VendorTagManager::GetMappedPropertyID(
    VendorTag vendorTag)
{
    PropertyID id = 0;
    if (vendorTag == g_controlVendorTag.AECControlVendorTag)
    {
        id = PropertyIDAECStatsControl;
    }
    else if (vendorTag == g_controlVendorTag.AWBControlVendorTag)
    {
        id = PropertyIDAWBStatsControl;
    }
    else if (vendorTag == g_controlVendorTag.AFDControlVendorTag)
    {
        id = PropertyIDAFDStatsControl;
    }
    else if (vendorTag == g_controlVendorTag.AFControlVendorTag)
    {
        id = PropertyIDAFStatsControl;
    }
    else if (vendorTag == g_controlVendorTag.IHistControlVendorTag)
    {
        id = PropertyIDIHistStatsControl;
    }
    else if (vendorTag == g_controlVendorTag.AECFrameVendorTag)
    {
        id = PropertyIDAECFrameControl;
    }
    else if (vendorTag == g_controlVendorTag.AWBFrameVendorTag)
    {
        id = PropertyIDAWBFrameControl;
    }
    else if (vendorTag == g_controlVendorTag.SensorGainVendorTag)
    {
        id = PropertyIDPostSensorGainId;
    }
    else if (vendorTag == g_controlVendorTag.IFEResidualCropVendorTag)
    {
        id = PropertyIDIFEDigitalZoom;
    }
    else if (vendorTag == g_controlVendorTag.IFEAppliedCropVendorTag)
    {
        id = PropertyIDIFEAppliedCrop;
    }
    else if (vendorTag == g_controlVendorTag.MFNRTotalNumFrames)
    {
        id = PropertyIDIPETotalFrameMFNR;
    }
    else if (vendorTag == g_controlVendorTag.MFSRTotalNumFrames)
    {
        id = PropertyIDIPETotalFrameMFSR;
    }
    return id;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VendorTagManager::GetInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VendorTagManager* VendorTagManager::GetInstance()
{
    static VendorTagManager s_VendorTagManagerSingleton;
    return &s_VendorTagManagerSingleton;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VendorTagManager::CopyVendorTagSectionData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult VendorTagManager::CopyVendorTagSectionData(
    VendorTagSectionData*       pDstSection,
    const VendorTagSectionData* pSrcSection)
{
    CamxResult result            = CamxResultSuccess;
    SIZE_T     srcSectionNameLen = OsUtils::StrLen(pSrcSection->pVendorTagSectionName);

    // dst section should not have any data
    CAMX_ASSERT(0    == pDstSection->numTags);
    CAMX_ASSERT(NULL == pDstSection->pVendorTagSectionName);
    CAMX_ASSERT(NULL == pDstSection->pVendorTagaData);

    pDstSection->numTags               = 0;
    pDstSection->pVendorTagSectionName = NULL;
    pDstSection->pVendorTagaData       = NULL;
    pDstSection->firstVendorTag        = pSrcSection->firstVendorTag;
    pDstSection->visbility             = pSrcSection->visbility;
    CHAR*        pVendorTagSectionName = static_cast<CHAR*>(CAMX_CALLOC(srcSectionNameLen + 1));
    if (NULL == pVendorTagSectionName)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory!");
        result = CamxResultENoMemory;
    }
    else
    {
        OsUtils::StrLCpy(pVendorTagSectionName, pSrcSection->pVendorTagSectionName, srcSectionNameLen + 1);
        pDstSection->pVendorTagSectionName = pVendorTagSectionName;
        pDstSection->pVendorTagaData       =
            static_cast<VendorTagData*>(CAMX_CALLOC(sizeof(VendorTagData) * pSrcSection->numTags));
        if (NULL != pDstSection->pVendorTagaData)
        {
            for (UINT32 i = 0; i < pSrcSection->numTags; i++)
            {
                SIZE_T srcTagNameLen = OsUtils::StrLen(pSrcSection->pVendorTagaData[i].pVendorTagName);
                CHAR* pVendorTagName = static_cast<CHAR*>(CAMX_CALLOC(srcTagNameLen + 1));
                if (NULL != pVendorTagName)
                {
                    OsUtils::StrLCpy(pVendorTagName,
                                     pSrcSection->pVendorTagaData[i].pVendorTagName,
                                     srcTagNameLen + 1);
                    pDstSection->pVendorTagaData[i].pVendorTagName = pVendorTagName;
                    pDstSection->pVendorTagaData[i].vendorTagType
                                        = pSrcSection->pVendorTagaData[i].vendorTagType;
                    pDstSection->pVendorTagaData[i].numUnits
                                        = pSrcSection->pVendorTagaData[i].numUnits;
                    pDstSection->numTags++;
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory!");
                    result = CamxResultENoMemory;
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory!");
            result = CamxResultENoMemory;
        }
    }

    if (CamxResultSuccess != result)
    {
        FreeVendorTagSectionData(pDstSection);
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VendorTagManager::FreeVendorTagSectionData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID VendorTagManager::FreeVendorTagSectionData(
    VendorTagSectionData* pSection)
{
    // NOWHINE CP036a: new section name is allocated from heap, need free.
    CAMX_FREE(const_cast<CHAR*>(pSection->pVendorTagSectionName));

    for (UINT32 i = 0; i < pSection->numTags; i++)
    {
        // NOWHINE CP036a: tag name is allocated from heap, need free.
        CAMX_FREE(const_cast<CHAR*>(pSection->pVendorTagaData[i].pVendorTagName));
        pSection->pVendorTagaData[i].pVendorTagName = NULL;
    }

    CAMX_FREE(pSection->pVendorTagaData);
    pSection->numTags               = 0;
    pSection->pVendorTagSectionName = NULL;
    pSection->pVendorTagaData       = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VendorTagManager::FreeVendorTagInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID VendorTagManager::FreeVendorTagInfo(
    VendorTagInfo* pVendorTagInfo)
{
    for (UINT32 i = 0; i < pVendorTagInfo->numSections; i++)
    {
        FreeVendorTagSectionData(&pVendorTagInfo->pVendorTagDataArray[i]);
    }

    CAMX_FREE(pVendorTagInfo->pVendorTagDataArray);
    pVendorTagInfo->pVendorTagDataArray = NULL;
    pVendorTagInfo->numSections         = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// VendorTagManager::PopulateControlVendorTagId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID VendorTagManager::PopulateControlVendorTagId()
{
    QueryVendorTagLocation("org.quic.camera2.statsconfigs", "AECStatsControl",    &g_controlVendorTag.AECControlVendorTag);
    QueryVendorTagLocation("org.quic.camera2.statsconfigs", "AWBStatsControl",    &g_controlVendorTag.AWBControlVendorTag);
    QueryVendorTagLocation("org.quic.camera2.statsconfigs", "AFStatsControl",     &g_controlVendorTag.AFControlVendorTag);
    QueryVendorTagLocation("org.quic.camera2.statsconfigs", "AFDStatsControl",    &g_controlVendorTag.AFDControlVendorTag);
    QueryVendorTagLocation("org.quic.camera2.statsconfigs", "IHistStatsControl",  &g_controlVendorTag.IHistControlVendorTag);
    QueryVendorTagLocation("org.quic.camera2.statsconfigs", "AECFrameControl",    &g_controlVendorTag.AECFrameVendorTag);
    QueryVendorTagLocation("org.quic.camera2.statsconfigs", "AWBFrameControl",    &g_controlVendorTag.AWBFrameVendorTag);
    QueryVendorTagLocation("org.quic.camera2.statsconfigs", "DigitalGainControl", &g_controlVendorTag.SensorGainVendorTag);
    QueryVendorTagLocation("org.quic.camera.ifecropinfo",
                           "ResidualCrop",
                           &g_controlVendorTag.IFEResidualCropVendorTag);
    QueryVendorTagLocation("org.quic.camera.ifecropinfo",
                           "AppliedCrop",
                           &g_controlVendorTag.IFEAppliedCropVendorTag);
    QueryVendorTagLocation("org.quic.camera.ifecropinfo",
                           "SensorIFEAppliedCrop",
                           &g_controlVendorTag.ModifiedCropWindow);
    QueryVendorTagLocation("org.quic.camera.gammainfo",
                           "GammaInfo",
                           &g_controlVendorTag.GammaOuputVendorTag);
    QueryVendorTagLocation("org.quic.camera2.mfnrconfigs",
                           "MFNRTotalNumFrames",
                           &g_controlVendorTag.MFNRTotalNumFrames);
    QueryVendorTagLocation("org.quic.camera2.mfsrconfigs",
                           "MFSRTotalNumFrames",
                           &g_controlVendorTag.MFSRTotalNumFrames);
    QueryVendorTagLocation("org.quic.camera.intermediateDimension",
                           "IntermediateDimension",
                           &g_controlVendorTag.IntermediateDimensionTag);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VendorTagManager::AppendVendorTagInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult VendorTagManager::AppendVendorTagInfo(
    const VendorTagInfo* pVendorTagInfoToAppend)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pVendorTagInfoToAppend);

    // This function merge the incoming vendor sections with existing ones. here we allocate a new array of sections and copy
    // the existing and incoming sections to the new array. After the successful merge, we will free the existing section
    // array and assign the new array to m_vendorTagInfo.

    if ((NULL != pVendorTagInfoToAppend) && (pVendorTagInfoToAppend->numSections > 0))
    {
        VendorTagSectionData* pSectionData =
            static_cast<VendorTagSectionData*>(CAMX_CALLOC(sizeof(VendorTagSectionData) *
            (m_vendorTagInfo.numSections + pVendorTagInfoToAppend->numSections)));
        if (NULL == pSectionData)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory!");
            result = CamxResultENoMemory;
        }
        else
        {
            UINT32 sectionCount = 0;

            for (UINT32 i = 0; i < m_vendorTagInfo.numSections; i++)
            {
                result = CopyVendorTagSectionData(&pSectionData[i], &m_vendorTagInfo.pVendorTagDataArray[i]);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to copy existing vendor tag section!");
                    break;
                }
                else
                {
                    sectionCount++;
                }
            }

            if (CamxResultSuccess == result)
            {
                CAMX_ASSERT(sectionCount == m_vendorTagInfo.numSections);
                for (UINT32 i = 0; i < pVendorTagInfoToAppend->numSections; i++)
                {
                    result = CopyVendorTagSectionData(&pSectionData[m_vendorTagInfo.numSections + i],
                                                      &pVendorTagInfoToAppend->pVendorTagDataArray[i]);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to copy pass in vendor tag section!");
                        break;
                    }
                    else
                    {
                        sectionCount++;
                    }
                }
            }

            if (CamxResultSuccess == result)
            {
                CAMX_ASSERT(sectionCount == (pVendorTagInfoToAppend->numSections + m_vendorTagInfo.numSections));

                FreeVendorTagInfo(&m_vendorTagInfo);

                m_vendorTagInfo.pVendorTagDataArray = pSectionData;
                m_vendorTagInfo.numSections         = sectionCount;
            }
            else
            {
                CAMX_ASSERT(sectionCount != (pVendorTagInfoToAppend->numSections + m_vendorTagInfo.numSections));
                CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to append vendor tag section!");

                for (UINT32 i = 0; i < sectionCount; i++)
                {
                    FreeVendorTagSectionData(&pSectionData[i]);
                }

                CAMX_FREE(pSectionData);
                pSectionData = NULL;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VendorTagManager::VendorTagManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VendorTagManager::VendorTagManager()
{
    m_vendorTagInfo   = { 0 };
    m_nextSectionBase = static_cast<UINT32>(VendorTagSectionPrivateEnd);
    m_initialized     = FALSE;
    m_pLocationMap    = NULL;

    CamxResult result = InitializeVendorTagInfo();

    CAMX_ASSERT((CamxResultSuccess == result) && (TRUE == m_initialized));

    if (TRUE == m_initialized)
    {
        UINT32 count = 0;

        for (UINT32 i = 0; i < m_vendorTagInfo.numSections; i++)
        {
            if (TRUE == Utils::IsBitMaskSet(m_vendorTagInfo.pVendorTagDataArray[i].visbility, TagSectionVisibleToAll))
            {
                count += m_vendorTagInfo.pVendorTagDataArray[i].numTags;
            }
        }

        HashmapParams   hashMapParams   = { 0 };

        hashMapParams.keySize       = MAX_SECTION_TAG_COMBINED_LEN; // max length of section plus tag name
        hashMapParams.valSize       = sizeof(UINT);
        hashMapParams.maxNumBuckets = count;
        hashMapParams.multiMap      = 0;
        m_pLocationMap              = Hashmap::Create(&hashMapParams);

        if (NULL == m_pLocationMap)
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("VendorTag manager initialization : Out of memory");
        }
        else
        {
            for (UINT32 i = 0; i < m_vendorTagInfo.numSections; i++)
            {
                for (UINT32 j = 0; j  < m_vendorTagInfo.pVendorTagDataArray[i].numTags; j++)
                {
                    CHAR    hash[MAX_SECTION_TAG_COMBINED_LEN] = {0};
                    UINT    tag;
                    SIZE_T  lenSection =
                        OsUtils::StrLen(m_vendorTagInfo.pVendorTagDataArray[i].pVendorTagSectionName);
                    SIZE_T  lenTag     =
                        OsUtils::StrLen(m_vendorTagInfo.pVendorTagDataArray[i].pVendorTagaData[j].pVendorTagName);

                    if ((lenSection + lenTag) < MAX_SECTION_TAG_COMBINED_LEN)
                    {
                        OsUtils::StrLCpy(hash, m_vendorTagInfo.pVendorTagDataArray[i].pVendorTagSectionName,
                                         MAX_SECTION_TAG_COMBINED_LEN);
                        OsUtils::StrLCpy(hash + lenSection,
                                         m_vendorTagInfo.pVendorTagDataArray[i].pVendorTagaData[j].pVendorTagName,
                                         MAX_SECTION_TAG_COMBINED_LEN - lenSection);

                        UINT location = m_vendorTagInfo.pVendorTagDataArray[i].firstVendorTag + j;

                        if (CamxResultSuccess == m_pLocationMap->Get(hash, &tag))
                        {
                            CAMX_LOG_ERROR(CamxLogGroupCore, "Locationmap has duplicated entry!");
                        }
                        else
                        {
                            m_pLocationMap->Put(&hash, &location);
                            CAMX_LOG_VERBOSE(CamxLogGroupCore, "put hash %s into Locationmap!", hash);
                        }
                    }
                }
            }

            CAMX_LOG_VERBOSE(CamxLogGroupCore, "m_pLocationMap created!");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "VendorTag manager initialization failed!");
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VendorTagManager::InitializeVendorTagInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult VendorTagManager::InitializeVendorTagInfo()
{
    CamxResult result = CamxResultSuccess;

    if (FALSE == m_initialized)
    {
        VendorTagInfo            hwVendorTagInfo = { 0 };
        ComponentVendorTagsInfo  componentVendorTagsInfo = { 0 };
        VendorTagInfo            coreVendorTagInfo = { g_CamXVendorTagSections, CAMX_ARRAY_SIZE(g_CamXVendorTagSections) };
        const HwEnvironment*     pHwEnvironment = HwEnvironment::GetInstance();

        // Query the vendor tag information from HW
        result = pHwEnvironment->GetHwStaticEntry()->QueryVendorTagsInfo(&hwVendorTagInfo);

        if (CamxResultEFailed != result)
        {
            result = AppendVendorTagInfo(&hwVendorTagInfo);
        }

        if (CamxResultEFailed != result)
        {
            result = AppendVendorTagInfo(&coreVendorTagInfo);
        }

        // Query the vendor tag information from External Component
        result = pHwEnvironment->GetHwStaticEntry()->QueryExternalComponentVendorTagsInfo(&componentVendorTagsInfo);

        if (CamxResultSuccess == result)
        {
            for (UINT32 i = 0; i < componentVendorTagsInfo.numVendorTagInfo ; i++)
            {
                result = AppendVendorTagInfo(&componentVendorTagsInfo.pVendorTagInfoArray[i]);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to append vendor tag Info!");
                    break;
                }
            }
        }

        if (componentVendorTagsInfo.pVendorTagInfoArray != NULL)
        {
            CAMX_FREE(componentVendorTagsInfo.pVendorTagInfoArray);
            componentVendorTagsInfo.pVendorTagInfoArray = NULL;
        }

        /// @todo(CAMX-1388) query vendor tag from internal SW and OEM node

        if (CamxResultSuccess == result)
        {
            // Move those preassigned section to the beginning of array for correct indexing
            UINT32 preAssignedCount = 0;
            for (UINT32 i = 0; i < m_vendorTagInfo.numSections; i++)
            {
                UINT32 preAssignedBase =
                    (m_vendorTagInfo.pVendorTagDataArray[i].firstVendorTag & VendorTagBaseMask);

                if (preAssignedBase != 0)
                {
                    UINT32 preAssignedIndex = (preAssignedBase >> NumOfVendorTagOffsetBit) - MetadataSectionVendorSection;

                    CAMX_ASSERT((preAssignedBase >> NumOfVendorTagOffsetBit) < VendorTagSectionPrivateEnd);

                    if (preAssignedIndex != i)
                    {
                        VendorTagSectionData pTempSection = { 0 };
                        Utils::Memcpy(&pTempSection, &m_vendorTagInfo.pVendorTagDataArray[i], sizeof(VendorTagSectionData));
                        Utils::Memcpy(&m_vendorTagInfo.pVendorTagDataArray[i],
                            &m_vendorTagInfo.pVendorTagDataArray[preAssignedIndex],
                            sizeof(VendorTagSectionData));
                        Utils::Memcpy(&m_vendorTagInfo.pVendorTagDataArray[preAssignedIndex],
                            &pTempSection,
                            sizeof(VendorTagSectionData));
                    }

                    preAssignedCount++;
                }
            }

            // Assign first vendor tag to those not pre-assigned sections
            m_nextSectionBase = static_cast<UINT32>(VendorTagSectionPrivateEnd);
            for (UINT32 i = preAssignedCount; i < m_vendorTagInfo.numSections; i++)
            {
                UINT32 preAssignedBase =
                    (m_vendorTagInfo.pVendorTagDataArray[i].firstVendorTag & VendorTagBaseMask);
                CAMX_ASSERT(preAssignedBase == 0);

                m_vendorTagInfo.pVendorTagDataArray[i].firstVendorTag
                    = (GetNextAvailableSectionBase() << NumOfVendorTagOffsetBit) |
                    (m_vendorTagInfo.pVendorTagDataArray[i].firstVendorTag & VendorTagOffsetMask);
            }

            m_pSectionOffsets = reinterpret_cast<UINT32*>(CAMX_CALLOC(sizeof(UINT32) * m_vendorTagInfo.numSections));

            if (NULL != m_pSectionOffsets)
            {
                m_pSectionOffsets[0] = 0;

                // Create LUT for offset in number of tags for each section, on top of the previous section
                for (UINT32 i = 0; i < m_vendorTagInfo.numSections - 1; i++)
                {
                    m_pSectionOffsets[i + 1] = m_pSectionOffsets[i] + m_vendorTagInfo.pVendorTagDataArray[i].numTags;
                }

                m_sizeAll = m_pSectionOffsets[m_vendorTagInfo.numSections - 1] +
                            m_vendorTagInfo.pVendorTagDataArray[m_vendorTagInfo.numSections - 1].numTags;

                m_pTagSizes       = reinterpret_cast<SIZE_T*>(CAMX_CALLOC(sizeof(SIZE_T) * m_sizeAll));

                UINT32 tag = 0;

                if (NULL != m_pTagSizes)
                {
                    // Create array of sizes, in order of the tags, to represent how much data is used by each tag
                    for (UINT32 i = 0; i < m_vendorTagInfo.numSections; i++)
                    {
                        for (UINT32 j = 0; j < m_vendorTagInfo.pVendorTagDataArray[i].numTags; j++)
                        {
                            m_pTagSizes[tag++] =
                                m_vendorTagInfo.pVendorTagDataArray[i].pVendorTagaData[j].numUnits *
                                HAL3MetadataUtil::GetSizeByType(static_cast<UINT8>(
                                    m_vendorTagInfo.pVendorTagDataArray[i].pVendorTagaData[j].vendorTagType));
                        }
                    }
                }
                else
                {
                    result = CamxResultENoMemory;
                }
            }
            else
            {
                result = CamxResultENoMemory;
            }

            if (CamxResultSuccess == result)
            {
                m_initialized = TRUE;
            }
        }
        else
        {
            FreeVendorTagInfo(&m_vendorTagInfo);
            CAMX_FREE(m_pTagSizes);
            CAMX_LOG_ERROR(CamxLogGroupCore, "Vendor tag information initialization failed!");
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VendorTagManager::~VendorTagManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VendorTagManager::~VendorTagManager()
{
    if (NULL != m_pSectionOffsets)
    {
        CAMX_FREE(m_pSectionOffsets);
        m_pSectionOffsets = NULL;
    }

    FreeVendorTagInfo(&m_vendorTagInfo);

    if (m_pTagSizes != NULL)
    {
        CAMX_FREE(m_pTagSizes);
    }

    m_pLocationMap->Destroy();
    m_pLocationMap = NULL;

    m_initialized = FALSE;
};

CAMX_NAMESPACE_END
