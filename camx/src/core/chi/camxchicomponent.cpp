////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchicomponent.cpp
/// @brief Landing methods for CamX implementation of CHI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <system/camera_metadata.h>
#include "camera_metadata_hidden.h"

#include "camxchi.h"
#include "camxdebug.h"
#include "camxentry.h"
#include "camxhal3.h"
#include "camxosutils.h"
#include "camxpipeline.h"
#include "camxtrace.h"
#include "camxutils.h"
#include "camxhwenvironment.h"
#include "camxchicomponent.h"
#include "camxvendortags.h"
#include "chinode.h"
#include "chioverride.h"
#include "camxtypes.h"
#include "chistatsalgo.h"


CAMX_NAMESPACE_BEGIN

struct ComponentVendorTagsInfo  g_componentVendorTagsInfo;
#if defined (_LINUX)
#if defined(_LP64)
static const CHAR ExtCompPath[] = "/vendor/lib64/camera/components";
#else // _LP64
static const CHAR ExtCompPath[] = "/vendor/lib/camera/components";
#endif // _LP64
#elif defined (_WIN32)
static const CHAR ExtCompPath[] = ".";
#endif // defined(_LINUX) || defined(_WIN32)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AddComponentTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AddComponentTag(
    CHIQUERYVENDORTAG*   pQueryVendorTag)
{
    CamxResult           result = CamxResultSuccess;

    // Validate the queried vendor tag info
    if ((NULL == pQueryVendorTag->pVendorTagInfo) ||
        (NULL == pQueryVendorTag->pVendorTagInfo->pVendorTagDataArray) ||
        (0 == pQueryVendorTag->pVendorTagInfo->numSections))
    {
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        for (UINT32 i = 0; i < pQueryVendorTag->pVendorTagInfo->numSections; i++)
        {
            if ((0 == pQueryVendorTag->pVendorTagInfo->pVendorTagDataArray[i].numTags) ||
                (NULL == pQueryVendorTag->pVendorTagInfo->pVendorTagDataArray[i].pVendorTagaData) ||
                (NULL == pQueryVendorTag->pVendorTagInfo->pVendorTagDataArray[i].pVendorTagSectionName))
            {
                result = CamxResultEInvalidArg;
                break;
            }
        }
    }
    // Append to the global vendor tag info list
    if (CamxResultSuccess == result)
    {
        UINT32 index = g_componentVendorTagsInfo.numVendorTagInfo;
        g_componentVendorTagsInfo.pVendorTagInfoArray[index].numSections =
            pQueryVendorTag->pVendorTagInfo->numSections;
        g_componentVendorTagsInfo.pVendorTagInfoArray[index].pVendorTagDataArray =
            reinterpret_cast<VendorTagSectionData *>(pQueryVendorTag->pVendorTagInfo->pVendorTagDataArray);
        g_componentVendorTagsInfo.numVendorTagInfo = g_componentVendorTagsInfo.numVendorTagInfo + 1;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetComponentTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GetComponentTag(
    PFNCHIQUERYVENDORTAG pfnQueryVendorTag)
{
    CAMX_ASSERT(NULL != g_componentVendorTagsInfo.pVendorTagInfoArray);

    CHIQUERYVENDORTAG queryVendorTag = {0};

    queryVendorTag.size = sizeof(CHIQUERYVENDORTAG);
    pfnQueryVendorTag(&queryVendorTag);

    // static assert to check the definition in CHI followes CAMX's definition
    CAMX_STATIC_ASSERT(sizeof(VendorTagSectionData) == sizeof(CHIVENDORTAGSECTIONDATA));
    CAMX_STATIC_ASSERT(sizeof(VendorTagData) == sizeof(CHIVENDORTAGDATA));

    AddComponentTag(&queryVendorTag);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetHVXAlgorithmCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GetHVXAlgorithmCallback(
    CHAR*                  pLibFileName,
    ExternalComponentInfo* pExternalComponentInfo,
    UINT16                 index)
{
    PFCHIISPALGORITHMENTRY  pHVXAlgoEntry = NULL;
    CamX::OSLIBRARYHANDLE   handle        = CamX::OsUtils::LibMap(pLibFileName);
    pHVXAlgoEntry = reinterpret_cast<PFCHIISPALGORITHMENTRY>(CamX::OsUtils::LibGetAddr(handle, "ChiISPHVXAlgorithmEntry"));
    CAMX_ASSERT(NULL != pHVXAlgoEntry);

    if (NULL != pHVXAlgoEntry)
    {
        pHVXAlgoEntry(&pExternalComponentInfo[index].HVXAlgoCallbacks);
    }

    if (NULL != handle)
    {
        CamX::OsUtils::LibUnmap(handle);
        handle = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetHistAlgorithmCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID GetHistAlgorithmCallback(
    CHAR*                  pLibFileName,
    ExternalComponentInfo* pExternalComponentInfo,
    UINT16                 index)
{
    PFHDRCHIALGORITHMENTRY  pHDRAlgoEntry      = NULL;
    const CHAR              ChiHistEntry[]     = "ChiHistogramAlgorithmEntry";
    CamX::OSLIBRARYHANDLE   handle             = CamX::OsUtils::LibMap(pLibFileName);

    pHDRAlgoEntry = reinterpret_cast<PFHDRCHIALGORITHMENTRY>(CamX::OsUtils::LibGetAddr(handle, ChiHistEntry));
    CAMX_ASSERT_MESSAGE(NULL != pHDRAlgoEntry, "Not found symbol for Hist Algo entry function");

    if (NULL != pHDRAlgoEntry)
    {
        pExternalComponentInfo[index].histAlgoCallbacks.size = sizeof(ChiHistogramAlgorithmCallbacks);
        pHDRAlgoEntry(&pExternalComponentInfo[index].histAlgoCallbacks);

        if (NULL != pExternalComponentInfo[index].histAlgoCallbacks.pfnQueryVendorTag)
        {
            GetComponentTag(pExternalComponentInfo[index].histAlgoCallbacks.pfnQueryVendorTag);
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupChi, "Failed to retrieve ChiHistAlgorithmEntry function address. from lib:%s", pLibFileName);
    }

    if (NULL != handle)
    {
        CamX::OsUtils::LibUnmap(handle);
        handle = NULL;
    }


}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetAECAlgorithmCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID GetAECAlgorithmCallback(
    CHAR*                  pLibFileName,
    ExternalComponentInfo* pExternalComponentInfo,
    UINT16                 index)
{
    PFAECCHIALGORITHMENTRY      pAECAlgoEntry   = NULL;
    const CHAR                  ChiAECEntry[]   = "ChiAECAlgorithmEntry";

    CamX::OSLIBRARYHANDLE        handle = CamX::OsUtils::LibMap(pLibFileName);
    pAECAlgoEntry = reinterpret_cast<PFAECCHIALGORITHMENTRY>(CamX::OsUtils::LibGetAddr(handle, ChiAECEntry));
    CAMX_ASSERT_MESSAGE(NULL != pAECAlgoEntry, "Not found symbol for AEC entry function");

    if (NULL != pAECAlgoEntry)
    {
        pExternalComponentInfo[index].AECAlgoCallbacks.size = sizeof(ChiAECAlgorithmCallbacks);
        pAECAlgoEntry(&pExternalComponentInfo[index].AECAlgoCallbacks);

        if (NULL != pExternalComponentInfo[index].AECAlgoCallbacks.pfnQueryVendorTag)
        {
            GetComponentTag(pExternalComponentInfo[index].AECAlgoCallbacks.pfnQueryVendorTag);
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupChi, "Failed to retrieve ChiAECAlgorithmEntry function address.")
    }

    if (NULL != handle)
    {
        CamX::OsUtils::LibUnmap(handle);
        handle = NULL;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetAFAlgorithmCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID GetAFAlgorithmCallback(
    CHAR*                  pLibFileName,
    ExternalComponentInfo* pExternalComponentInfo,
    UINT16                 index)
{
    PFAFCHIALGORITHMENTRY       pAFAlgoEntry   = NULL;
    const CHAR                  ChiAFEntry[]   = "ChiAFAlgorithmEntry";
    CamX::OSLIBRARYHANDLE       handle         = CamX::OsUtils::LibMap(pLibFileName);

    pAFAlgoEntry = reinterpret_cast<PFAFCHIALGORITHMENTRY>(CamX::OsUtils::LibGetAddr(handle, ChiAFEntry));

    CAMX_ASSERT(NULL != pAFAlgoEntry);

    if (NULL != pAFAlgoEntry)
    {

        pExternalComponentInfo[index].AFAlgoCallbacks.size = sizeof(ChiAFAlgorithmCallbacks);
        pAFAlgoEntry(&pExternalComponentInfo[index].AFAlgoCallbacks);

        if (NULL != pExternalComponentInfo[index].AFAlgoCallbacks.pfnQueryVendorTag)
        {
            GetComponentTag(pExternalComponentInfo[index].AFAlgoCallbacks.pfnQueryVendorTag);
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupChi, "Failed to retrieve ChiAFAlgorithmEntry function address.")
    }

    if (NULL != handle)
    {
        CamX::OsUtils::LibUnmap(handle);
        handle = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetAWBAlgorithmCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID GetAWBAlgorithmCallback(
    CHAR*                  pLibFileName,
    ExternalComponentInfo* pExternalComponentInfo,
    UINT16                 index)
{
    PFAWBCHIALGORITHMENTRY      pAWBAlgoEntry   = NULL;
    const CHAR                  ChiAWBEntry[]   = "ChiAWBAlgorithmEntry";
    CamX::OSLIBRARYHANDLE       handle          = CamX::OsUtils::LibMap(pLibFileName);

    pAWBAlgoEntry = reinterpret_cast<PFAWBCHIALGORITHMENTRY>(CamX::OsUtils::LibGetAddr(handle, ChiAWBEntry));
    CAMX_ASSERT(NULL != pAWBAlgoEntry);


    if (NULL != pAWBAlgoEntry)
    {
        pExternalComponentInfo[index].AWBAlgoCallbacks.size = sizeof(ChiAWBAlgorithmCallbacks);
        pAWBAlgoEntry(&pExternalComponentInfo[index].AWBAlgoCallbacks);
        if (NULL != pExternalComponentInfo[index].AWBAlgoCallbacks.pfnQueryVendorTag)
        {
            GetComponentTag(pExternalComponentInfo[index].AWBAlgoCallbacks.pfnQueryVendorTag);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "Failed to retrieve ChiAWBAlgorithmEntry function address. from lib:%s", pLibFileName);
    }

    if (NULL != handle)
    {
        CamX::OsUtils::LibUnmap(handle);
        handle = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetAFDAlgorithmCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID GetAFDAlgorithmCallback(
    CHAR*                  pLibFileName,
    ExternalComponentInfo* pExternalComponentInfo,
    UINT16                 index)
{
    PFAFDCHIALGORITHMENTRY      pAFDAlgoEntry   = NULL;
    const CHAR                  ChiAFDEntry[]   = "ChiAFDAlgorithmEntry";
    CamX::OSLIBRARYHANDLE       handle          = CamX::OsUtils::LibMap(pLibFileName);

    pAFDAlgoEntry = reinterpret_cast<PFAFDCHIALGORITHMENTRY>(CamX::OsUtils::LibGetAddr(handle, ChiAFDEntry));
    CAMX_ASSERT(NULL != pAFDAlgoEntry);

    if (NULL != pAFDAlgoEntry)
    {
        pExternalComponentInfo[index].AFDAlgoCallbacks.size = sizeof(ChiAFDAlgorithmCallbacks);
        pAFDAlgoEntry(&pExternalComponentInfo[index].AFDAlgoCallbacks);

        if (NULL != pExternalComponentInfo[index].AFDAlgoCallbacks.pfnQueryVendorTag)
        {
            GetComponentTag(pExternalComponentInfo[index].AFDAlgoCallbacks.pfnQueryVendorTag);
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupChi, "Failed to retrieve ChiAFDAlgorithmEntry function address.")
    }

    if (NULL != handle)
    {
        CamX::OsUtils::LibUnmap(handle);
        handle = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetASDAlgorithmCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID GetASDAlgorithmCallback(
    CHAR*                  pLibFileName,
    ExternalComponentInfo* pExternalComponentInfo,
    UINT16                 index)
{
    PFASDCHIALGORITHMENTRY      pASDAlgoEntry   = NULL;
    const CHAR                  ChiASDEntry[]   = "ChiASDAlgorithmEntry";
    CamX::OSLIBRARYHANDLE       handle          = CamX::OsUtils::LibMap(pLibFileName);

    pASDAlgoEntry = reinterpret_cast<PFASDCHIALGORITHMENTRY>(CamX::OsUtils::LibGetAddr(handle, ChiASDEntry));
    CAMX_ASSERT_MESSAGE(NULL != pASDAlgoEntry, "Not found symbol for ASD entry function");

    if (NULL != pASDAlgoEntry)
    {
        pExternalComponentInfo[index].ASDAlgoCallbacks.size = sizeof(ChiASDAlgorithmCallbacks);
        pASDAlgoEntry(&pExternalComponentInfo[index].ASDAlgoCallbacks);

        if (NULL != pExternalComponentInfo[index].ASDAlgoCallbacks.pfnQueryVendorTag)
        {
            GetComponentTag(pExternalComponentInfo[index].ASDAlgoCallbacks.pfnQueryVendorTag);
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupChi, "Failed to retrieve ChiASDAlgorithmEntry function address.")
    }

    if (NULL != handle)
    {
        CamX::OsUtils::LibUnmap(handle);
        handle = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetPDLibraryCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID GetPDLibraryCallback(
    CHAR*                  pLibFileName,
    ExternalComponentInfo* pExternalComponentInfo,
    UINT16                 index)
{
    PFPDCHILIBRARYENTRY      pPDLibEntry = NULL;
    const CHAR               ChiPDEntry[] = "ChiPDLibraryEntry";
    CamX::OSLIBRARYHANDLE    handle = CamX::OsUtils::LibMap(pLibFileName);

    pPDLibEntry = reinterpret_cast<PFPDCHILIBRARYENTRY>(CamX::OsUtils::LibGetAddr(handle, ChiPDEntry));
    CAMX_ASSERT_MESSAGE(NULL != pPDLibEntry, "Not found symbol for PDLIB entry function");

    if (NULL != pPDLibEntry)
    {
        pExternalComponentInfo[index].PDLibCallbacks.size = sizeof(ChiPDLibraryCallbacks);
        pPDLibEntry(&pExternalComponentInfo[index].PDLibCallbacks);

        if (NULL != pExternalComponentInfo[index].PDLibCallbacks.pfnQueryVendorTag)
        {
            GetComponentTag(pExternalComponentInfo[index].PDLibCallbacks.pfnQueryVendorTag);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "Failed to retrieve ChiPDLibraryEntry function address.")
    }

    if (NULL != handle)
    {
        CamX::OsUtils::LibUnmap(handle);
        handle = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetTrackerAlgorithmCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID GetTrackerAlgorithmCallback(
    CHAR*                  pLibFileName,
    ExternalComponentInfo* pExternalComponentInfo,
    UINT16                 index)
{
    PFTRACKERCHIALGORITHMENTRY      pTrackerAlgoEntry   = NULL;
    const CHAR                      ChiTrackerEntry[]   = "ChiTrackerAlgorithmEntry";
    CamX::OSLIBRARYHANDLE           handle              = CamX::OsUtils::LibMap(pLibFileName);

    pTrackerAlgoEntry = reinterpret_cast<PFTRACKERCHIALGORITHMENTRY>(CamX::OsUtils::LibGetAddr(handle, ChiTrackerEntry));
    CAMX_ASSERT(NULL != pTrackerAlgoEntry);

    if (NULL != pTrackerAlgoEntry)
    {
        pExternalComponentInfo[index].trackerAlgoCallbacks.size = sizeof(ChiTrackerAlgorithmCallbacks);
        pTrackerAlgoEntry(&pExternalComponentInfo[index].trackerAlgoCallbacks);
        if (NULL != pExternalComponentInfo[index].trackerAlgoCallbacks.pfnQueryVendorTag)
        {
            GetComponentTag(pExternalComponentInfo[index].trackerAlgoCallbacks.pfnQueryVendorTag);
        }
    }
    else
    {
        CAMX_LOG_ERROR(
            CamxLogGroupChi, "Failed to retrieve ChiTrackerAlgorithmEntry function address. from lib:%s", pLibFileName);
    }

    if (NULL != handle)
    {
        CamX::OsUtils::LibUnmap(handle);
        handle = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ProbeChiComponents
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ProbeChiComponents(
    ExternalComponentInfo* pExternalComponentInfo,
    UINT*                  pNumExternalComponent)
{
    CamxResult           result             = CamxResultSuccess;
    UINT16               fileCountTypeNode  = 0;
    UINT16               fileCountTypeStats = 0;
    UINT16               fileCountTypeOverride = 0;
    UINT16               fileCountTypeHvx   = 0;
    UINT16               index              = 0;
    CHAR                 soFilesName[MaxExternalComponents][FILENAME_MAX];
    static const UINT    NumCHIOverrideModules = 2;
    CHAR                 chiOverrideSoFileName[NumCHIOverrideModules*FILENAME_MAX];
    CHAR                 outTokenString[FILENAME_MAX];
    PFCHINODEENTRY       pNodeEntry;
    static const CHAR    HistToken[]      = "LOCALHISTOGRAM";
    static const CHAR    AECToken[]       = "AEC";
    static const CHAR    AFToken[]        = "AF";
    static const CHAR    AWBToken[]       = "AWB";
    static const CHAR    AFDToken[]       = "AFD";
    static const CHAR    ASDToken[]       = "ASD";
    static const CHAR    TrackerToken[]   = "TRACKER";
    static const CHAR    PDToken[]        = "PDLIB";

    CSLCameraPlatform    CSLPlatform = {};

    result = CSLQueryCameraPlatform(&CSLPlatform);
    CAMX_ASSERT(CamxResultSuccess == result);

    switch (CSLPlatform.socId)
    {
        case CSLCameraTitanSocSM6350:
        case CSLCameraTitanSocSM7225:
#if defined(_LP64)
            fileCountTypeOverride = OsUtils::GetFilesFromPath("/vendor/lib64/camera/oem/bitra",
                                                              FILENAME_MAX,
                                                              &chiOverrideSoFileName[0],
                                                              "*",
                                                              "chi",
                                                              "*",
                                                              "*",
                                                              &SharedLibraryExtension[0]);
            if (0 == fileCountTypeOverride)
            {
                fileCountTypeOverride = OsUtils::GetFilesFromPath("/vendor/lib64/camera/qti/bitra",
                                                                  FILENAME_MAX,
                                                                  &chiOverrideSoFileName[0],
                                                                  "*",
                                                                  "chi",
                                                                  "*",
                                                                  "*",
                                                                  &SharedLibraryExtension[0]);
            }
#else // using LP32
            fileCountTypeOverride = OsUtils::GetFilesFromPath("/vendor/lib/camera/oem/bitra",
                                                              FILENAME_MAX,
                                                              &chiOverrideSoFileName[0],
                                                              "*",
                                                              "chi",
                                                              "*",
                                                              "*",
                                                              &SharedLibraryExtension[0]);
            if (0 == fileCountTypeOverride)
            {
                fileCountTypeOverride = OsUtils::GetFilesFromPath("/vendor/lib/camera/qti/bitra",
                                                                  FILENAME_MAX,
                                                                  &chiOverrideSoFileName[0],
                                                                  "*",
                                                                  "chi",
                                                                  "*",
                                                                  "*",
                                                                  &SharedLibraryExtension[0]);
            }
#endif // _LP64
            if (0 == fileCountTypeOverride)
            {
                fileCountTypeOverride = OsUtils::GetFilesFromPath(CHIOverrideModulePath,
                                                                  FILENAME_MAX,
                                                                  &chiOverrideSoFileName[0],
                                                                  "*",
                                                                  "chi",
                                                                  "*",
                                                                  "*",
                                                                  &SharedLibraryExtension[0]);
            }
            break;
        default:
            fileCountTypeOverride = OsUtils::GetFilesFromPath(CHIOverrideModulePath,
                                                              FILENAME_MAX,
                                                              &chiOverrideSoFileName[0],
                                                              "*",
                                                              "chi",
                                                              "*",
                                                              "*",
                                                              &SharedLibraryExtension[0]);
            break;
    }

    fileCountTypeNode  = OsUtils::GetFilesFromPath(ExtCompPath,
                                                   FILENAME_MAX,
                                                   &soFilesName[0][0],
                                                   "*",
                                                   "node",
                                                   "*",
                                                   "*",
                                                   &SharedLibraryExtension[0]);
    fileCountTypeStats = OsUtils::GetFilesFromPath(ExtCompPath,
                                                   FILENAME_MAX,
                                                   &soFilesName[fileCountTypeNode][0],
                                                   "*",
                                                   "stats",
                                                   "*",
                                                   "*",
                                                   &SharedLibraryExtension[0]);
    fileCountTypeHvx   = OsUtils::GetFilesFromPath(ExtCompPath,
                                                   FILENAME_MAX,
                                                   &soFilesName[fileCountTypeNode+fileCountTypeStats][0],
                                                   "*",
                                                   "hvx",
                                                   "*",
                                                   "*",
                                                   &SharedLibraryExtension[0]);

    g_componentVendorTagsInfo.numVendorTagInfo = 0;
    g_componentVendorTagsInfo.pVendorTagInfoArray =
        static_cast<VendorTagInfo*>(CAMX_CALLOC(sizeof(VendorTagInfo) *
            (fileCountTypeOverride + fileCountTypeNode + fileCountTypeStats)));
    CAMX_ASSERT(NULL != g_componentVendorTagsInfo.pVendorTagInfoArray);

    if (NULL != pNumExternalComponent)
    {
        *pNumExternalComponent = fileCountTypeNode + fileCountTypeStats + fileCountTypeHvx;
    }

    while (index < fileCountTypeNode + fileCountTypeStats + fileCountTypeHvx)
    {
        CamX::OSLIBRARYHANDLE handle = CamX::OsUtils::LibMap(&soFilesName[index][0]);

        if (index < fileCountTypeNode)
        {
            pNodeEntry = reinterpret_cast<PFCHINODEENTRY>(CamX::OsUtils::LibGetAddr(handle, "ChiNodeEntry"));
            CAMX_ASSERT(NULL != pNodeEntry);
            pExternalComponentInfo[index].nodeCallbacks.size = sizeof(ChiNodeCallbacks);
            if (NULL != pNodeEntry)
            {
                pNodeEntry(&pExternalComponentInfo[index].nodeCallbacks);
            }

            if (NULL != pExternalComponentInfo[index].nodeCallbacks.pQueryVendorTag)
            {
                GetComponentTag(pExternalComponentInfo[index].nodeCallbacks.pQueryVendorTag);
            }

            pExternalComponentInfo[index].nodeAlgoType = ExternalComponentNodeAlgo::COMPONENTNODE;
        }
        else if (index < (fileCountTypeNode+fileCountTypeStats))
        {
            pExternalComponentInfo[index].nodeAlgoType = ExternalComponentNodeAlgo::COMPONENTALGORITHM;


            if (OsUtils::GetFileNameToken(&soFilesName[index][0], 4, outTokenString, FILENAME_MAX) == TRUE)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupStats, "index %d, sofile: %s , outTokenString : %s ",
                    index, soFilesName[index], outTokenString);

                if (0 == OsUtils::StrNICmp(outTokenString, AECToken, OsUtils::StrLen(AECToken)))
                {
                    GetAECAlgorithmCallback(&soFilesName[index][0], pExternalComponentInfo, index);
                    pExternalComponentInfo[index].statsAlgo = ExternalComponentStatsAlgo::ALGOAEC;
                }
                else if (0 == OsUtils::StrNICmp(outTokenString, AFDToken, OsUtils::StrLen(AFDToken)))
                {
                    GetAFDAlgorithmCallback(&soFilesName[index][0], pExternalComponentInfo, index);
                    pExternalComponentInfo[index].statsAlgo = ExternalComponentStatsAlgo::ALGOAFD;
                }
                else if (0 == OsUtils::StrNICmp(outTokenString, AFToken, OsUtils::StrLen(AFToken)))
                {
                    GetAFAlgorithmCallback(&soFilesName[index][0], pExternalComponentInfo, index);
                    pExternalComponentInfo[index].statsAlgo = ExternalComponentStatsAlgo::ALGOAF;
                }
                else if (0 == OsUtils::StrNICmp(outTokenString, AWBToken, OsUtils::StrLen(AWBToken)))
                {
                    GetAWBAlgorithmCallback(&soFilesName[index][0], pExternalComponentInfo, index);
                    pExternalComponentInfo[index].statsAlgo = ExternalComponentStatsAlgo::ALGOAWB;
                }
                else if (0 == OsUtils::StrNICmp(outTokenString, ASDToken, OsUtils::StrLen(ASDToken)))
                {
                    GetASDAlgorithmCallback(&soFilesName[index][0], pExternalComponentInfo, index);
                    pExternalComponentInfo[index].statsAlgo = ExternalComponentStatsAlgo::ALGOASD;
                }
                else if (0 == OsUtils::StrNICmp(outTokenString, TrackerToken, OsUtils::StrLen(TrackerToken)))
                {
                    GetTrackerAlgorithmCallback(&soFilesName[index][0], pExternalComponentInfo, index);
                    pExternalComponentInfo[index].statsAlgo = ExternalComponentStatsAlgo::ALGOTRACK;
                }
                else if (0 == OsUtils::StrNICmp(outTokenString, PDToken, OsUtils::StrLen(PDToken)))
                {
                    GetPDLibraryCallback(&soFilesName[index][0], pExternalComponentInfo, index);
                    pExternalComponentInfo[index].statsAlgo = ExternalComponentStatsAlgo::ALGOPD;
                }
                else if (0 == OsUtils::StrNICmp(outTokenString, HistToken, OsUtils::StrLen(HistToken)))
                {
                    GetHistAlgorithmCallback(&soFilesName[index][0], pExternalComponentInfo, index);
                    pExternalComponentInfo[index].statsAlgo = ExternalComponentStatsAlgo::ALGOHIST;
                }

            }
        }
        else
        {
            pExternalComponentInfo[index].nodeAlgoType = ExternalComponentNodeAlgo::COMPONENTHVX;
            GetHVXAlgorithmCallback(&soFilesName[index][0], pExternalComponentInfo, index);
        }

        SIZE_T srcComponentNameLen = OsUtils::StrLen(&soFilesName[index][0]);
        pExternalComponentInfo[index].pComponentName = static_cast<CHAR*>(CAMX_CALLOC(srcComponentNameLen + 1));
        const CHAR* pFileName = OsUtils::GetFileName(&soFilesName[index][0]);
        CAMX_ASSERT(NULL != pFileName);
        CAMX_ASSERT(NULL != pExternalComponentInfo[index].pComponentName);
        if (NULL != pExternalComponentInfo[index].pComponentName)
        {
            OsUtils::StrLCpy(pExternalComponentInfo[index].pComponentName, pFileName, srcComponentNameLen + 1);
        }

        pExternalComponentInfo[index].inUse = 1;
        index = index + 1;
    }

    if (0 < fileCountTypeOverride)
    {
        const  CHAR*  pD        = NULL;
        INT    fileIndexBitra  = FILENAME_MAX;
        INT    fileIndex        = 0;
        PFNCHIQUERYVENDORTAG pQueryVendorTag;

        pD = OsUtils::StrStr(&chiOverrideSoFileName[0], "bitra");

        // pD is NULL if Bitra is not present in first file index
        if (pD != NULL)
        {
            fileIndexBitra = 0;
            fileIndex      = FILENAME_MAX;
        }

        if (CSLPlatform.socId == CSLCameraTitanSocSM6350 || CSLPlatform.socId == CSLCameraTitanSocSM7225)
        {
            pQueryVendorTag = reinterpret_cast<PFNCHIQUERYVENDORTAG>(CamX::OsUtils::LibGetAddr(
                              CamX::OsUtils::LibMap(&chiOverrideSoFileName[fileIndexBitra]), "chi_hal_query_vendertag"));
        }
        else
        {
            pQueryVendorTag = reinterpret_cast<PFNCHIQUERYVENDORTAG>(CamX::OsUtils::LibGetAddr(
                              CamX::OsUtils::LibMap(&chiOverrideSoFileName[fileIndex]), "chi_hal_query_vendertag"));
        }

        if (NULL != pQueryVendorTag)
        {
            GetComponentTag(pQueryVendorTag);
        }
    }

    return result;
}


CAMX_NAMESPACE_END
