////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcustomization.h
/// @brief CamX Customization Header File
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXCUSTOMIZATION_H
#define CAMXCUSTOMIZATION_H

#include "camxhal3metadatatags.h"
#include "camxhal3metadatatagtypes.h"
#include "camxhal3types.h"
#include "camxhwenvironment.h"
#include "camxstaticcaps.h"
#include "camxtypes.h"

CAMX_NAMESPACE_BEGIN

/// @brief OEM to add its own properties to extend structure PlatformStaticCaps
///        PlatformStaticCapsExtended could be referenced via PlatformStaticCaps->pExtended
struct PlatformStaticCapsExtended
{
    // OEM to add additional platform static capabilities
};

/// @brief OEM to add its own properties to extend structure HwEnvironmentStaticCaps
///        HwEnvironmentStaticCapsExtended could be referenced via HwEnvironmentStaticCaps->pExtended
struct HwEnvironmentStaticCapsExtended
{
    // OEM to add additional hw environment static capabilities
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNINITIALIZEEXTENDEDPLATFORMSTATICCAPS
///
/// @brief  OEM to customize PlatformStaticCaps of each sensor. Implemented by OEM.
///
/// This function is called after CAMX initialized the default PlatformStaticCaps for each sensor. OEM could customize
/// PlatformStaticCaps via: 1.) update/override the values in PlatformStaticCaps. 2.) initialize new capacities defined in
/// PlatformStaticCapsExtended. OEM shall allocate memory for the PlatformStaticCapsExtended structure.
///
/// @param  pCaps           The array of PlatformStaticCaps
/// @param  numberSensors   The size of array pCaps
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*PFNINITIALIZEEXTENDEDPLATFORMSTATICCAPS)(
    PlatformStaticCaps* pCaps,
    UINT32 numberSensors);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNRELEASEEXTENDEDPLATFORMSTATICCAPS
///
/// @brief  OEM to release the pExtended pointer created via PFNINITIALIZEEXTENDEDPLATFORMSTATICCAPS. Implemented by OEM.
///
/// @param  pCaps           The array of PlatformStaticCaps
/// @param  numberSensors   The size of array pCaps
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*PFNRELEASEEXTENDEDPLATFORMSTATICCAPS)(
    PlatformStaticCaps* pCaps,
    UINT32 numberSensors);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNINITIALIZEEXTENDEDHWENVIRONMENTSTATICCAPS
///
/// @brief  OEM to customize HwEnvironmentStaticCaps of each sensor. Implemented by OEM.
///
/// This function is called after CAMX initialized the default HwEnvironmentStaticCaps for each sensor. OEM could customize
/// HwEnvironmentStaticCaps via: 1.) update/override the values in HwEnvironmentStaticCaps. 2.) initialize new capacities
/// defined in HwEnvironmentStaticCapsExtended. OEM shall allocate memory for the HwEnvironmentStaticCapsExtended structure.
///
/// @param  pCaps           The array of HwEnvironmentStaticCaps
/// @param  numberSensors   The size of array pCaps
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*PFNINITIALIZEEXTENDEDHWENVIRONMENTSTATICCAPS)(
    HwEnvironmentStaticCaps* pCaps, UINT32 numberSensors);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNRELEASEEXTENDEDHWENVIRONMENTSTATICCAPS
///
/// @brief  OEM to release the pExtended pointer created via PFNINITIALIZEEXTENDEDHWENVIRONMENTSTATICCAPS. Implemented by OEM.
///
/// @param  pCaps           The array of HwEnvironmentStaticCaps
/// @param  numberSensors   The size of array pCaps
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*PFNRELEASEEXTENDEDHWENVIRONMENTSTATICCAPS)(
    HwEnvironmentStaticCaps* pCaps, UINT32 numberSensors);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNGETHWENVIRONMENTINSTANCE
///
/// @brief  Allow OEM to get the HwEnvironment instance.  Implemented by CAMX.
///
/// @return The pointer to instance of HwEnvironment
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef HwEnvironment* (*PFNGETHWENVIRONMENTINSTANCE)();

/// @brief The interface implemented by OEM to customize the static caps
struct CAMXCustomizeOEMInterface
{
    UINT32                      size;                       ///< Size of this structure.
    UINT32                      majorVersion;               ///< Major version.
    UINT32                      minorVersion;               ///< Minor version.

    PFNINITIALIZEEXTENDEDPLATFORMSTATICCAPS         pInitializeExtendedPlatformStaticCaps;      ///< Initialize extended
                                                                                                ///  platform static capacities
    PFNRELEASEEXTENDEDPLATFORMSTATICCAPS            pReleaseExtendedPlatformStaticCaps;         ///< Release extended platform
                                                                                                ///  static capacities
    PFNINITIALIZEEXTENDEDHWENVIRONMENTSTATICCAPS    pInitializeExtendedHWEnvironmentStaticCaps; ///< Initialize extended
                                                                                                ///  HwEnvironment static
                                                                                                ///  capacities
    PFNRELEASEEXTENDEDHWENVIRONMENTSTATICCAPS       pReleaseExtendedHWEnvironmentStaticCaps;    ///< Release extended
                                                                                                ///  HwEnvironment static
                                                                                                ///  capacities
};

/// @brief Interface for OEM to query CAMX information
struct CAMXCustomizeCAMXInterface
{
    UINT32                      size;                       ///< Size of this structure.
    UINT32                      majorVersion;               ///< Major version.
    UINT32                      minorVersion;               ///< Minor version.
    PFNGETHWENVIRONMENTINSTANCE pGetHWEnvironment;          ///< Get the HwEnvironment instance
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAMXCustomizeEntry
///
/// @brief  Entry point called by CAMX to customize the sensor's capabilities
///
/// @param  ppOEMInterface  Pointer to the pointer of CAMXCustomizeOEMInterface. OEM to return the pointer to allow CAMX call
///                         the OEM customization functions.
/// @param  pCAMXInterface  Pointer to CAMXCustomizeCAMXInterface that defines callbacks that the OEM could use to get CAMX
///                         info.
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDK_VISIBILITY_PUBLIC VOID CAMXCustomizeEntry(
    CAMXCustomizeOEMInterface** ppOEMInterface,
    CAMXCustomizeCAMXInterface* pCAMXInterface);

CAMX_NAMESPACE_END

#endif // CAMXCUSTOMIZATION_H