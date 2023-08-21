////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxtitan17xhwl.cpp
/// @brief Titan17x HWL Init implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxhwenvironment.h"
#include "camxmem.h"
#include "camxtitan17xcontext.h"
#include "camxtitan17xfactory.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xGetStaticEntryMethods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xGetStaticEntryMethods(
    HwContextStaticEntry* pStaticEntry)
{
    CamxResult result = CamxResultSuccess;

    pStaticEntry->Create                               = &Titan17xContext::Create;
    pStaticEntry->GetStaticMetadataKeysInfo            = &Titan17xContext::GetStaticMetadataKeysInfo;
    pStaticEntry->GetStaticCaps                        = &Titan17xContext::GetStaticCaps;
    pStaticEntry->CreateHwFactory                      = &Titan17xFactory::Create;
    pStaticEntry->QueryVendorTagsInfo                  = &Titan17xContext::QueryVendorTagsInfo;
    pStaticEntry->GetHWBugWorkarounds                  = &Titan17xContext::GetHWBugWorkarounds;
    pStaticEntry->QueryExternalComponentVendorTagsInfo = &Titan17xContext::QueryExternalComponentVendorTagsInfo;

    return result;
}

CAMX_NAMESPACE_END
