////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslresourcedefs.h
/// @brief The definition of CSL device resource structures for packets that will be used both by CSL and HWL
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCSLRESOURCEDEFS_H
#define CAMXCSLRESOURCEDEFS_H

// NOWHINE FILE GR016: Converting CSL header to C, we need typedef

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "camxdefs.h"
#include "camxtypes.h"

/// @brief This enumerates VFE device resources.
typedef enum
{
    CSLVFEResourceIDPort = 0,           ///< Indicates a port resource
    CSLVFEResourceIDMaxNumResources     ///< To use as the number of resources
} CSLVFEResourceID;

/// @todo (CAMX-544) Add more resource Ids
/// @brief This enumerates VFE device resources.
typedef enum
{
    CSLVFEPortIdViewer = 0,         ///< Indicates viewer path output resource
    CSLVFEPortIdEncoder,            ///< Indicates encoder path output resource
    CSLVFEPortIdFD,                 ///< Indicates face detect path output resource
    CSLVFEPortIdRDI0,               ///< Indicates RDI 0 path output resource
    CSLVFEPortIdRDI1,               ///< Indicates RDI 1 path output resource
    CSLVFEPortIdRDI2,               ///< Indicates RDI 2 path output resource
    CSLVFEPortIdOffline,            ///< Indicates offline processing path
    CSLVFEPortIdMaxNumPortResources ///< To use as the number of port resources
} CSLVFEPortId;

CAMX_BEGIN_PACKED

/// @todo (CAMX-544) Add stats resource Ids
/// @brief This is the info associated with a VFE port resource.
typedef struct
{
    UINT32  portId; ///< Port Id
    UINT32  format; ///< This indicates the format of the output port resource.
} CAMX_PACKED CSLVFEPortResourceInfo;

/// @brief This enumerates IFE device output port resources.
typedef enum
{
    CSLIFEPortIdFull          = 0x3000, ///< IFE full pixel path output resource
    CSLIFEPortIdDownscaled4,            ///< IFE 1/4 pixel path output resource
    CSLIFEPortIdDownscaled16,           ///< IFE 1/16 pixel path output resource
    CSLIFEPortIdRawDump,                ///< Raw output path
    CSLIFEPortIdFD,                     ///< FD path
    CSLIFEPortIdPDAF,                   ///< PDAF path
    CSLIFEPortIdRDI0,                   ///< RDI 0 path output resource
    CSLIFEPortIdRDI1,                   ///< RDI 1 path output resource
    CSLIFEPortIdRDI2,                   ///< RDI 2 path output resource
    CSLIFEPortIdRDI3,                   ///< RDI 3 path output resource
    CSLIFEPortIdStatHDRBE,              ///< Stats HDR BE output resource
    CSLIFEPortIdStatHDRBHIST,           ///< Stats HDR BHIST output resource
    CSLIFEPortIdStatTintlessBG,         ///< Stats Tintless BG output resource
    CSLIFEPortIdStatBF,                 ///< Stats BF output resource
    CSLIFEPortIdStatAWBBG,              ///< Stats AWB BG output resource
    CSLIFEPortIdStatBHIST,              ///< Stats BHIST output resource
    CSLIFEPortIdStatRS,                 ///< Stats RS output resource
    CSLIFEPortIdStatCS,                 ///< Stats CS output resource
    CSLIFEPortIdStatIHIST,              ///< Stats IHIST output resource
    // @note: Following IFE Display output ports (Full/DS4/DS16) are supported in SDM855 (SM8150) only.
    CSLIFEPortIdDisplayFull,            ///< IFE display full pixel path output resource
    CSLIFEPortIdDisplayDownscaled4,     ///< IFE display 1/4 pixel path output resource
    CSLIFEPortIdDisplayDownscaled16,    ///< IFE display 1/16 pixel path output resource
    CSLIFEPortIdDualPD,                 ///< Dual PD HW
    CSLIFEPortIdRDIRD,                  ///< RDI RD used as fetch engine port
    CSLIFEPortIdLCR,                    ///< LCR
    CSLIFEPortIdMax                     ///< Max port Id
} CSLIFEOutputPortId;

static const UINT32 CSLIFEPortIdMaxNumStatsPortResources = CSLIFEPortIdStatIHIST - CSLIFEPortIdStatHDRBE + 1;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLIFEPortIdMaxNumPortResourcesOfPlatform
///
/// @brief  Get maximum number of CSL IFE output ports supported in given platform
///
/// @param  titanVersion    CSL camera Titan version
///
/// @return max resources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE UINT32 CSLIFEPortIdMaxNumPortResourcesOfPlatform(
    const UINT32 titanVersion)
{
    UINT32 maxNumPortResources = 0;

    switch (titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan175:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan480:
            maxNumPortResources = CSLIFEPortIdMax - CSLIFEPortIdFull;
            break;

        // Limit CSL IFE port ID up to CSLIFEPortIdStatIHIST for non CSLTitan175/CSLTitan160 families
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan150:
        default:
            maxNumPortResources = CSLIFEPortIdDisplayFull - CSLIFEPortIdFull;
            break;
    }

    return maxNumPortResources;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLIFEPortIndex
///
/// @brief  Map CSL IFE port Id to zero-based index
///
/// @param  portId CSL IFE port Id
///
/// @return index
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE UINT32 CSLIFEPortIndex(
    CSLIFEOutputPortId portId)
{
    return  portId - CSLIFEPortIdFull;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLIFEStatsPortIndex
///
/// @brief  Map CSL IFE stats port Id to zero-based stats port index
///
/// @param  portId CSL IFE stats port Id
///
/// @return index
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE UINT32 CSLIFEStatsPortIndex(
    CSLIFEOutputPortId portId)
{
    return  portId - CSLIFEPortIdStatHDRBE;
}

/// @brief This enumerates IFE device input port resources.
typedef enum
{
    CSLIFEInputPortIdTPG    = 2500, ///< Testgen input
    CSLIFEInputPortIdPHY0,          ///< PHY0
    CSLIFEInputPortIdPHY1,          ///< PHY1
    CSLIFEInputPortIdPHY2,          ///< PHY2
    CSLIFEInputPortIdPHY3,          ///< PHY3
    CSLIFEInputPortIdMax            ///< Max resource Id
} CSLIFEInputPortId;

/// @brief This enumerates ICP device types.
typedef enum
{
    CSLICPDeviceTypeA5 = 1, ///< Indicates A5 device
    CSLICPDeviceTypeIPE,    ///< Indicates IPE device
    CSLICPDeviceTypeBPS,    ///< Indicates BPS device
    CSLICPDeviceTypeMax     ///< Max number of devices
} CSLICPDeviceType;

/// @brief This enumerates ICP device resources.
typedef enum
{
    CSLICPResourceIDBPS = 1,        ///< Indicates BPS Non Realtime resource    = 1
    CSLICPResourceIDIPERealTime,    ///< Indicates IPE realtime resource        = 2
    CSLICPResourceIDIPENonRealTime, ///< Indicates IPE Non Realtime resource    = 3
    CSLICPResourceIDIPESemiRealTime,///< Indicates IPE Pseudo Realtime resource = 4
    CSLICPResourceIDBPSRealTime,    ///< Indicates BPS realTime resource        = 5
    CSLICPResourceIDBPSSemiRealTime,///< Indicates BPS Pseudo RealTime resource = 6
    CSLICPResourceIDMax             ///< Max resource Id
} CSLICPResourceID;

static const UINT32 CSLICPResourceIDMaxNumResources = CSLICPResourceIDMax - CSLICPResourceIDBPS;

/// @brief This enumerates IPE device input port resources.
typedef enum
{
    CSLIPEInputPortIdFull = 0,       ///< Full size input
    CSLIPEInputPortIdDS4,            ///< 1:4 size input
    CSLIPEInputPortIdDS16,           ///< 1:16 size input
    CSLIPEInputPortIdDS64,           ///< 1:64 size input
    CSLIPEInputPortIdFullRef,        ///< IPE Full path reference path input resource
    CSLIPEInputPortIdDS4Ref,         ///< IPE DS4 path input resource
    CSLIPEInputPortIdDS16Ref,        ///< IPE DS16 path input resource
    CSLIPEInputPortIdDS64Ref,        ///< IPE DS64 path input resource
    CSLIPEInput2PortIdFull = 14,     ///< IPE Input 2 Full (aligning to firmware interface)
    CSLIPEInput2PortIdDSX  = 15,     ///< IPE Input 2 DSX (aligning to firmware interface)
} CSLIPEInputPortId;

/// @brief This enumerates IPE device output port resources.
typedef enum
{
    CSLIPEOutputPortIdDisplay = CSLIPEInputPortIdDS64Ref + 1,               ///< IPE display pixel path output resource
    CSLIPEOutputPortIdVideo,                                                ///< IPE video pixel path output resource
    CSLIPEOutputPortIdFullRef,                                              ///< IPE Full path reference output resource
    CSLIPEOutputPortIdDS4Ref,                                               ///< IPE DS4 path reference output resource
    CSLIPEOutputPortIdDS16Ref,                                              ///< IPE DS16 path reference output resource
    CSLIPEOutputPortIdDS64Ref,                                              ///< IPE DS64 path reference output resource
    CSLIPEInputScratchBufer = 16,     ///< IPE Input Sratch buffer
    CSLIPEOutputPortIdMaxNumPortResources = CSLIPEInputScratchBufer + 1       ///< To use as the number of port resources
} CSLIPEOutputPortId;


/// @brief This enumerates BPS device input port resources.
typedef enum
{
    CSLBPSInputPortImage = 0,   ///< Full size input
} CSLBPSInputPortId;

/// @brief This enumerates BPS device output port resources.
typedef enum
{
    CSLBPSOutputPortIdPDIImageFull = CSLBPSInputPortImage + 1,  ///< BPS Image full size PDI packed output path resource
    CSLBPSOutputPortIdPDIImageDS4,                              ///< BPS down scale 4 PDI packed output path resource
    CSLBPSOutputPortIdPDIImageDS16,                             ///< BPS down scale 16 PDI packed output path resource
    CSLBPSOutputPortIdPDIImageDS64,                             ///< BPS down scale 64 PDI packed output path resource
    CSLBPSOutputPortIdStatsBG,                                  ///< BPS Bayer Grid stats output path resource
    CSLBPSOutputPortIdStatsHDRBHIST,                            ///< BPS HDR Bayer histogram stats output resource
    CSLBPSOutputPortIdRegistrationImage1,                       ///< BPS Image registration1 image format output resource
    CSLBPSOutputPortIdRegistrationImage2,                       ///< BPS Image registration2 image format output resource
    CSLBPSOutputPortIdMaxNumPortResources                       ///< To use as the number of port resources
}  CSLBPSOutputPortId;

/// @brief This is the info associated with a IFE port resource.
typedef struct
{
    UINT32  portId; ///< Port Id
    UINT32  format; ///< This indicates the format of the output port resource.
} CAMX_PACKED CSLIFEPortResourceInfo;

/// @brief This enumerates JPEG device input port resources.
typedef enum
{
    CSLJPEGInputPortIdImage = 0,    ///< jpeg input port id
    CSLJPEGInputPortIdMax,          ///< jpeg input port id Max
} CSLJPEGInputPortId;

/// @brief This enumerates JPEG device output port resources.
typedef enum
{
    CSLJPEGOutputPortIdImage = CSLJPEGInputPortIdMax,                   ///< jpeg output port id
    CSLJPEGOutputPortIdMax,                                             ///< jpeg input port id max
    CSLJPEGOutputPortIdMaxNumPortResources = CSLJPEGOutputPortIdMax,    ///< jpeg input/output port id
} CSLJPEGOutputPortId;

/// @brief This enumerates JPEG device types.
typedef enum
{
    CSLJPEGDeviceTypeEncoder,   ///< Indicates jpeg Encoder device
    CSLJPEGDeviceTypeDma,       ///< Indicates jpeg DMA device
    CSLJPEGDeviceTypeMax        ///< Max number of devices
} CSLJPEGDeviceType;

/// @brief This enumerates JPEG device resources.
typedef enum
{
    CSLJPEGResourceIDEncoder,           ///< Indicates jpeg Encoder device
    CSLJPEGResourceIDDma,               ///< Indicates jpeg DMA device
    CSLJPEGResourceIDMaxNumResources,   ///< Max number of Resources
} CSLJPEGResourceID;

typedef enum
{
    CSLLRMEOutputPortIdVector = 0,                              ///< LRME output vector Port
    CSLLRMEOutputPortIdDS2,                                     ///< LRME DS2 output port
    CSLLRMEOutputPortIdMax                                      ///< Maximum output ports
} CSLLRMEOutputPortId;

typedef enum
{
    CSLLRMEInputPortTARIFEFull = CSLLRMEOutputPortIdDS2 + 1,    ///< Input port to take input from IFE Full port
    CSLLRMEInputPortREFIFEFull,                                 ///< Input port to take input from IFE Full port
    CSLLRMEInputPortTARIFEDS4,                                  ///< Input port to take input from IFE DS4 port
    CSLLRMEInputPortREFIFEDS4,                                  ///< Input port to take input from IFE DS4 port
    CSLLRMEInputPortTARIFEDS16,                                 ///< Input port to take input from IFE DS16 port
    CSLLRMEInputPortREFIFEDS16,                                 ///< Input port to take input from IFE DS16 port
    CSLLRMEInputPortREFLRMEDS2,                                 ///< Input port to take input LRME Downscaled port
    CSLLRMEInputPortIdMax                                       ///< Maximum Input Ports
} CSLLRMEInputPortId;

CAMX_END_PACKED

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CAMXCSLRESOURCEDEFS_H
