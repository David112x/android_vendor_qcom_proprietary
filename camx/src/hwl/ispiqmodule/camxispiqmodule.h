////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxispiqmodule.h
/// @brief TITAN IQ Module base class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXISPIQMODULE_H
#define CAMXISPIQMODULE_H

#include "chistatsproperty.h"
#include "chitintlessinterface.h"

#include "camxcslispdefs.h"
#include "camxcmdbuffermanager.h"
#include "camxdefs.h"
#include "camxformats.h"
#include "camxhal3types.h"
#include "camxhwcontext.h"
#include "camximageformatutils.h"
#include "camxifeproperty.h"
#include "camxisphwsetting.h"
#include "camxmem.h"
#include "camxmetadatapool.h"
#include "camxpacketbuilder.h"
#include "camxsensorproperty.h"
#include "camxtitan17xdefs.h"
#include "camxtuningdump.h"
#include "chiiqmodulesettings.h"
#include "chiisphvxdefs.h"
#include "chipdlibinterface.h"
#include "ipe_data.h"
#include "iqcommondefs.h"
#include "titan170_base.h"
#include "titan170_ife.h"
#include "camxifestripinginterface.h"
#include "lsc34setting.h"
#include "GeoLib.h"
#include "chituningmodeparam.h"

CAMX_NAMESPACE_BEGIN

class  ISPIQModule;

/// @brief ISP IQ Module Version Mask
static const UINT32 ISPVersionMask = 0xf0000000;

/// @brief Fixed point number with 12 fractional bits
static const UINT32 Q12 = 1 << 12;

/// @brief Fixed point number with 7 fractional bits
static const UINT32 Q7 = 1 << 7;

/// @brief Fixed point number with 4 fractional bits for ica30
static const UINT32 Q4 = 1 << 4;

/// @brief Fixed point number with 3 fractional bits
static const UINT32 Q3 = 1 << 3;

/// @brief DMI Entry size, in bytes
static const UINT DMIEntrySize = 16;

typedef UINT32 IFEPath;

static const IFEPath MaxMNDSPath       = 3;    ///< Maximum paths for MNDS Output

static const IFEPath FDOutput          = 0;    ///< MNDS/RoundClamp FD output path
static const IFEPath FullOutput        = 1;    ///< MNDS/RoundClamp Full output path
static const IFEPath DS4Output         = 2;    ///< RoundClamp DS4 output path
static const IFEPath DS16Output        = 3;    ///< RoundClamp DS16 output path
static const IFEPath PixelRawOutput    = 4;    ///< RoundClamp pixel raw dump
static const IFEPath PDAFRawOutput     = 5;    ///< PDAF RAW Output
static const IFEPath DisplayFullOutput = 6;    ///< MNDS/RoundClamp Display Full output path
static const IFEPath DisplayDS4Output  = 7;    ///< RoundClamp Display DS4 output path
static const IFEPath DisplayDS16Output = 8;    ///< RoundClamp Display DS16 output path
static const IFEPath LCROutput         = 9;    ///< LCR Output Path
static const IFEPath MaxRoundClampPath = 10;   ///< Maximum paths for Round and Clamp output

static const UINT32 MaxStatsPorts = CSLIFEPortIdStatIHIST - CSLIFEPortIdStatHDRBE + 1; ///< Maximum num of stats ports

static const UINT32 DS4Factor  = 4;    ///< Constant to calculate DS4 output with respect to Full out
static const UINT32 DS16Factor = 16;   ///< Constant to calculate DS16 output with respect to Full out
static const UINT32 DS64Factor = 64;   ///< Constant to calculate DS64 output with respect to Full out

static const UINT32 YUV420SubsampleFactor = 2;  ///< Subsample factor of chroma data with respect to Luma data

static const UINT32 ColorCorrectionRows     = 3;    ///< Color correction transform matrix rows
static const UINT32 ColorCorrectionColumns  = 3;    ///< Color correction transform matrix columns
static const UINT32 TotalChannels           = 4;    ///< Total IFE channels R, Gr, Gb and B

///< ICA Virtual domain dimensions
static const uint32_t IcaVirtualDomainWidth  = 8192;
static const uint32_t IcaVirtualDomainHeight = 6144;

enum ISPChannel
{
    ISPChannelRed = 0,      ///< ISP red channel
    ISPChannelGreenOdd,     ///< ISP green odd channel
    ISPChannelGreenEven,    ///< ISP green even channel
    ISPChannelBlue,         ///< ISP blue channel
    ISPChannelMax           ///< Max ISP channel
};

enum IFERDIPath
{
    RDI0 = 0,   ///< RDI0 path
    RDI1,       ///< RDI1 path
    RDI2,       ///< RDI2 path
    RDI3,       ///< RDI3 path
    MaxPath         ///< Max RDI paths
};

/// @ brief enumeration defining IPE path
enum IPEPath
{
    INPUT = 0,    ///< Input
    REFERENCE,    ///< Reference
    CVPICA,       ///< CVP ICA
};

/// @brief Image Warping Grid type
enum ImageWarpGrids
{
    In2OutGrid = 0,     ///< In2Out Grid type
    Out2InGrid,         ///< Out2In Grid type
    GridMaxType         ///< Max number of LDC Grid type
};

/// @brief ICAInputData
struct ICAConfigParameters
{
    IPEICAPerspectiveTransform  ICAInPerspectiveParams;       ///< Perspective parameters for ICA 1
    IPEICAGridTransform         ICAInGridParams;              ///< Grid parameters for ICA 1
    IPEICAInterpolationParams   ICAInInterpolationParams;     ///< Interpolation parameters for ICA 1
    IPEICAPerspectiveTransform  ICARefPerspectiveParams;      ///< Perspective parameters for ICA 2
    IPEICAGridTransform         ICARefGridParams;             ///< Grid parameters for ICA 2
    IPEICAInterpolationParams   ICARefInterpolationParams;    ///< Interpolation parameters for ICA 2
    IPEICAPerspectiveTransform  ICAReferenceParams;           ///< Reference (LRME) parameters for ICA 2
    VOID*                       pCurrICAInData;               ///< ICA input current frame Data used by NCLib
    VOID*                       pPrevICAInData;               ///< ICA reference current frame Data
    VOID*                       pCurWarpAssistData;           ///< Current Warp assist data output from NcLib
    VOID*                       pPrevWarpAssistData;          ///< Current Warp assist data output from NcLib
};

static const UINT ICAReferenceNumber = 2;

/// @brief ISP color correction gains
enum ISPColorCorrectionGain
{
    ISPColorCorrectionGainsRed,         ///< Red gain
    ISPColorCorrectionGainsGreenRed,    ///< Green-Red gain
    ISPColorCorrectionGainsGreenBlue,   ///< Green-Blue gain
    ISPColorCorrectionGainsBlue,        ///< Blue gain
    ISPColorCorrectionGainsMax,         ///< Total number of gains
};

/// @brief Command buffer managers in IPE node
enum IPECmdBufferId
{
    CmdBufferFrameProcess = 0,    ///< 0: Top level firmware payload (IpeFrameProcess)
    CmdBufferStriping,            ///< 1: Striping library output (IpeStripingLibOutMultiFrame)
    CmdBufferIQSettings,          ///< 2: IQsettings within top level payload (IpeIQSettings)
    CmdBufferPreLTM,              ///< 3: generic cdm payload of all pre LTM IQ modules
    CmdBufferPostLTM,             ///< 4: generic cdm payload of all post LTM IQ modules
    CmdBufferDMIHeader,           ///< 5: CDM headers for all DMI LUTs from IPE IQ modules
    CmdBufferIQPacket,            ///< 6: IQ Packet Manager
    CmdBufferNPS,                 ///< 7: CMD payload for NPS: ANR/TF modules
    CmdBufferBLMemory,            ///< 8: BL Memory Command buffer used by firmware
    CmdBufferGenericBlob,         ///< 9: KMD specific buffer
    IPECmdBufferMaxIds,           ///< 10: Max number of command buffers in IPQ node
};

/// @brief CDM Program Array Order : CDMProgramArrays are appended at the bottom of firmware payload (i.e. IpeFrameProcess)
/// For better coding and debugging of payload values, enforcing below order in which CDMProgramArrays shall be appended.
enum CDMProgramArrayOrder
{
    ProgramArrayANRFullPass = 0,             ///< 0: ANR Full Pass index
    ProgramArrayANRDS4,                      ///< 1: ANR DS4 Pass index
    ProgramArrayANRDS16,                     ///< 2: ANR DS16 Pass index
    ProgramArrayANRDS64,                     ///< 3: ANR DS64 Pass index
    ProgramArrayTFFullPass,                  ///< 4: TF Full Pass index
    ProgramArrayTFDS4,                       ///< 5: TF DS4 Pass index
    ProgramArrayTFDS16,                      ///< 6: TF DS16 Pass index
    ProgramArrayTFDS64,                      ///< 7: TF DS64 Pass index
    ProgramArrayPreLTM,                      ///< 8: Pre LTM data index
    ProgramArrayPostLTM,                     ///< 9: Post LTM data index
    ProgramArrayICA1,                        ///< 10; ICA1 data index
    ProgramArrayICA2 =
        ProgramArrayICA1 + MAX_HFR_GROUP,    ///< 18 ICA2 data index
    ProgramArrayMax,                         ///< 27: Max CDM Program Arrays mentioned in firmware payload
};

/// @brief CDM Program Order for Pre LTM
enum PreLTMCDMProgramOrder
{
    ProgramIndexPreLTMGeneric = 0,  ///< 0: Generic type of program
    ProgramIndexHNR,                ///< 1: HNR Module arry index
    ProgramIndexLTM,                ///< 2: PreLTM Module - LTM DMI index
    ProgramIndexMaxPreLTM,          ///< 3: Max number of PreLTM Cdm Programs
};

/// @brief CDM Program Order for Post LTM
enum PostLTMCDMProgramOrder
{
    ProgramIndexPostLTMGeneric = 0, ///< 0: Generic type of program
    ProgramIndexGLUT,               ///< 1: PostLTM Module - Gamma DMI program index
    ProgramIndex2DLUT,              ///< 2: PostLTM Module - 2D LUT DMI program index
    ProgramIndexASF,                ///< 3: PostLTM Module - ASF DMI program index
    ProgramIndexUpscale,            ///< 4: PostLTM Module - Upscale DMI program index
    ProgramIndexGRA,                ///< 5: PostLTM Module - GRA DMI program index
    ProgramIndexMaxPostLTM,         ///< 6: Max number of PostLTM Cdm Programs
};

enum ICAProgramOrder
{
    ProgramIndexICA1 = 0,   ///< ICA program index 1
    ProgramIndexICA2,       ///< ICA program index 2
    ProgramIndexICAMax      ///< ICA program index count
};
// FD and Full out paths are output from MNDS path, it should not exceed 2
CAMX_STATIC_ASSERT((FDOutput < 2) && (FullOutput < 2));

// DS4 and DS16 out paths are output from RoundandClamp path, it should not exceed 4
CAMX_STATIC_ASSERT((DS4Output < 4) && (DS16Output < 4));

/// @brief Aspect ratio tolerence beyond which aspect ratio adjustment required
static const FLOAT AspectRatioTolerance = 0.01f;

/// @brief Bits per pixel for different formats
static const UINT32 BitWidthEight    = 8;
static const UINT32 BitWidthTen      = 10;
static const UINT32 BitWidthFourteen = 14;
static const UINT32 Max8BitValue   = (1 << 8) - 1;   ///< 2^8 - 1
static const UINT32 Max10BitValue  = (1 << 10) - 1;  ///< 2^10 - 1
static const UINT32 Max14BitValue  = (1 << 14) - 1;  ///< 2^14 - 1

/// @brief Command buffer managers in BPS node
enum BPSCmdBufferId
{
    BPSCmdBufferFrameProcess = 0,    ///< 0: Top level firmware payload (BpsFrameProcess)
    BPSCmdBufferStriping,            ///< 1: Striping library output (BpsStripingLibOutMultiFrame)
    BPSCmdBufferIQSettings,          ///< 2: IQsettings within top level payload (BpsIQSettings)
    BPSCmdBufferDMIHeader,           ///< 3: CDM headers for all DMI LUTs from BPS IQ modules
    BPSCmdBufferCDMProgram,          ///< 4: CDM headers for all CDM programs in BPS
    BPSCmdBufferIQPacket,            ///< 5: IQ Packet
    BPSCmdBufferBLMemory,            ///< 6: BL Memory Command buffer used by firmware
    BPSCmdBufferGenericBlob,         ///< 7: KMD specific buffer
    BPSCmdBufferMaxIds,              ///< 8: Max number of command buffers in BPS node
};

/// @brief BPS CDM Program Order
enum BPSCDMProgramOrder
{
    BPSProgramIndexGeneric = 0,      ///< 0: Generic type of program for BPS
    BPSProgramIndexPEDESTAL,         ///< 1: Pedestal DMI program index
    BPSProgramIndexLINEARIZATION,    ///< 2: Linearization DMI program index
    BPSProgramIndexBPCPDPC,          ///< 3: BPC PDPC DMI program index
    BPSProgramIndexGIC,              ///< 4: GIC DMI program index
    BPSProgramIndexABF,              ///< 5: ABF DMI program index
    BPSProgramIndexRolloff,          ///< 6: Roll off DMI program index
    BPSProgramIndexGTM,              ///< 7: GTM program index
    BPSProgramIndexGLUT,             ///< 8: GLUT DMI program index
    BPSProgramIndexHNR,              ///< 9: HNR DMI program index
    BPSProgramIndexMax,              ///< 10: Max number of BPS Cdm Program
};

/// @brief TITAN IQ Module Type
enum class ISPIQModuleType
{
    IFECAMIF,              ///< Camera Interface Module of IFE
    IFECAMIFLite,          ///< Camera Interface Module of Dual PD
    IFECAMIFDualPD,        ///< Camera Interface Moudle of new 2PD
    IFECAMIFLCR,           ///< Camera Interface Module of LCR source
    IFECAMIFRDI0,          ///< Camera Interface Module of RDI 0 source
    IFECAMIFRDI1,          ///< Camera Interface Module of RDI 1 source
    IFECAMIFRDI2,          ///< Camera Interface Module of RDI 2 source
    IFECAMIFRDI3,          ///< Camera Interface Module of RDI 3 source
    IFECSID,               ///< CSID Module
    IFEPedestalCorrection, ///< Pedestal Correction Module of IFE
    IFEBLS,                ///< Black Level Substraction Module of IFE
    IFELinearization,      ///< Linearization Module of IFE
    IFEDemux,              ///< Demux Module of IFE
    IFEHDR,                ///< HDR Module of IFE
    IFEPDPC,               ///< PDAF pixel correction Module of IFE
    IFEBPCBCC,             ///< Bad Pixel Corection Module of IFE
    IFEABF,                ///< Adaptive Bayer Filter Module of IFE
    IFELSC,                ///< Lens Shading Correction of IFE
    IFEDemosaic,           ///< Demosaic Module of IFE
    IFECC,                 ///< Color Correction Module of IFE
    IFEGTM,                ///< Global Tone Mapping of IFE
    IFEGamma,              ///< Gamma Correction Module of IFE
    IFECST,                ///< Color Space Transform Module of IFE
    IFEMNDS,               ///< Scaler Module of IFE
    IFEPreCrop,            ///< DS4 Pre Crop Module of IFE
    IFEDS4,                ///< DS4 Module of IFE
    IFECrop,               ///< Fov and Crop Module of IFE
    IFERoundClamp,         ///< Round and Clamp Module of IFE
    IFER2PD,               ///< R2PD Module of IFE
    IFEWB,                 ///< White Balance Module of IFE
    IFEHVX,                ///< Custom IQ module implemented in HVX
    IFEDUALPD,             ///< Dual PD HW
    IFEPDAF,               ///< PDAF Module of IFE
    IFELCR,                ///< LCR Module of IFE

    IPEICA,                ///< Image Correction and Adjustment Module in NPS
    IPERaster22,           ///< Raster Module in NPS
    IPERasterPD,           ///< Raster Module in PD path in NPS
    IPEANR,                ///< Advanced Noise Reduction Module in NPS
    IPETF,                 ///< Temporal Filter Module in NPS
    IPECAC,                ///< Colour Aberration Correction Module in PPS
    IPECrop,               ///< Standalone or prescaler Crop Module in IPE
    IPEChromaUpsample,     ///< Chroma Upsampling Module in PPS
    IPECST,                ///< Color Space Transform Module in PPS
    IPELTM,                ///< Local Tone Map Module in PPS
    IPEColorCorrection,    ///< Color Correction Module in PPS
    IPEGamma,              ///< Gamma Correction Module
    IPE2DLUT,              ///< 2D LUT Module in PPS
    IPEChromaEnhancement,  ///< Chroma Enhancement Module in PPS
    IPEChromaSuppression,  ///< Chroma Suppression Module in PPS
    IPESCE,                ///< Skin Coloe Enhancement Module in PPS
    IPEASF,                ///< Adaptive Spatial Filter Module in PPS
    IPEUpscaler,           ///< Up Scaler Module in PPS
    IPEGrainAdder,         ///< Grain Adder Module in PPS
    IPEDownScaler,         ///< Down Scaler Module
    IPEFOVCrop,            ///< Fov and Crop Module after downscaler
    IPEClamp,              ///< Clamp Module
    IPEHNR,                ///< HNR module in IPE
    IPELENR,               ///< LENR module in IPE

    BPSPedestalCorrection, ///< Pedestal Correction Module in BPS
    BPSLinearization,      ///< Linearization Module in BPS
    BPSBPCPDPC,            ///< Bad Pixel Corection & PDPC Module in BPS
    BPSDemux,              ///< Demux Module in BPS
    BPSHDR,                ///< HDR Motion artifcat/Reconstruction module in BPS
    BPSABF,                ///< Adaptive Bayer Filter Module in BPS
    BPSLSC,                ///< Lens shading module in BPS
    BPSGIC,                ///< GB/GR Imbalance correction module in BPS
    BPSDemosaic,           ///< Demosaic Module in BPS
    BPSCC,                 ///< Color Correction Module in BPS
    BPSGTM,                ///< Global Tone Mapping in BPS
    BPSGamma,              ///< Gamma Correction Module in BPS
    BPSCST,                ///< Color Space Transform Module in BPS
    BPSChromaSubSample,    ///< Chroma Sub sampling module in BPS
    BPSHNR,                ///< HNR module in BPS
    BPSWB,                 ///< White Balance Module in BPS
    BPSAWBBG,              ///< BPS AWB BG stats
    BPSHDRBHist,           ///< HDR BHist stats
    SWTMC,                 ///< SW TMC module
};

enum class DynamicIFEMaskBit
{
    SWTMC,                  ///< SWTMC
    IFEHVX,                 ///< IFEHVX
    IFECAMIF,               ///< IFECAMIF
    IFEPedestalCorrection,  ///< IFEPedestalCorrection
    IFEABF,                 ///< IFEABF
    IFELinearization,       ///< IFELinearization
    IFEPDPC,                ///< IFEPDPC
    IFEDemux,               ///< IFEDemux
    IFEHDR,                 ///< IFEHDR
    IFELSC,                 ///< IFELSC
    IFEWB,                  ///< IFEWB
    IFEDemosaic,            ///< IFEDemosaic
    IFECC,                  ///< IFECC
    IFEGTM,                 ///< IFEGTM
    IFEGamma,               ///< IFEGamma
    IFECST,                 ///< IFECST
    ALSC,                   ///< ALSC
};

static const UINT32 OffsetOfIFEIQModuleIndex = static_cast<UINT32>(ISPIQModuleType::IFECAMIF);
static const UINT32 OffsetOfIPEIQModuleIndex = static_cast<UINT32>(ISPIQModuleType::IPEICA);
static const UINT32 OffsetOfBPSIQModuleIndex = static_cast<UINT32>(ISPIQModuleType::BPSPedestalCorrection);

/// @brief IFE pipeline path
enum class IFEPipelinePath: UINT
{
    VideoFullPath = 0,     ///< Full path of IFE pipeline
    FDPath,           ///< FD path of IFE pipeline
    VideoDS4Path,          ///< DS4 path of IFE pipeline
    VideoDS16Path,         ///< DS16 path of IFE pipeline
    DisplayFullPath,  ///< Display full path of IFE pipeline
    DisplayDS4Path,   ///< Display DS4 path of IFE pipeline
    DisplayDS16Path,  ///< Display DS16 path of IFE pipeline
    CommonPath,       ///< Common path of IFE pipeline
    PixelRawDumpPath, ///< pixel raw path of IFE pipeline
    DualPDPath,       ///< Dual PD path
    LCRPath,          ///< LCR path
    RDI0Path,         ///< RDI0 path
    RDI1Path,         ///< RDI1 path
    RDI2Path,         ///< RDI2 path
    RDI3Path,         ///< RDI3 path
};

enum ICAVersion
{
    ICAVersion10 = 10,    ///< ICA Version 10
    ICAVersion20 = 20,    ///< ICA Version 20
    ICAVersion30 = 30,    ///< ICA Version 30
};

static const UINT IFEMaxNonCommonPaths = 15; ///< Max number of non-Common pipeline paths

enum class IFEModuleMode
{
    SingleIFENormal,    ///< Single IFE Normal Mode
    DualIFENormal       ///< Dual IFE Normal Mode
};

/// @brief structure to pack all IFE module flags
union IFEModuleFlags
{
    struct
    {
        BIT     isBayer             : 1;   ///< Bayer Sensor type
        BIT     isFDPathEnable      : 1;   ///< FD output path of IFE
        BIT     isFullPathEnable    : 1;   ///< Full output path of IFE
        BIT     isDS4PathEnable     : 1;   ///< DS4 output path of IFE
        BIT     isDS16PathEnable    : 1;   ///< DS16 output path of IFE
    };
    UINT    allFlags;
};

/// @brief IFE output paths
struct IFEOutputPath
{
    UINT32  path;   ///< Currently selected path. Can only be TRUE or FALSE
};

/// @brief DSP Mode
enum IFEDSPMode
{
    DSPModeNone = 0,          ///< No DSP mode is selected
    DSPModeOneWay,            ///< DSP mode one Way
    DSPModeRoundTrip,         ///< DSP mode Round trip
};

/// @brief This structure encapsulates data for a CAMIF crop request from the sensor
struct CropInfo
{
    UINT32    firstPixel;    ///< starting pixel for CAMIF crop
    UINT32    firstLine;     ///< starting line for CAMIF crop
    UINT32    lastPixel;     ///< last pixel for CAMIF crop
    UINT32    lastLine;      ///< last line for CAMIF crop
};

/// @brief SensorAspectRatioMode
enum SensorAspectRatioMode
{
    FULLMODE,                ///< 4:3 sensor
    NON_FULLMODE,            ///< other than 4:3, it can be 16:9
};

/// @ brief Structure to encapsulate sensor dependent data
struct ISPSensorConfigureData
{
    FLOAT                          dGain;                       ///< Digital Gain Value
    PixelFormat                    format;                      ///< Image Format
    CropInfo                       CAMIFCrop;                   ///< CAMIF crop request from sensor
    StreamDimension                sensorOut;                   ///< Sensor output info
    BIT                            isBayer;                     ///< Flag to indicate Bayer sensor
    BIT                            isYUV;                       ///< Flag to indicate YUV sensor
    BIT                            isMono;                      ///< Flag to indicate Mono sensor
    BIT                            isIHDR;                      ///< Flag to indicate IHDR sensor
    FLOAT                          sensorScalingFactor;         ///< Downscaling factor of sensor
    FLOAT                          sensorBinningFactor;         ///< Sensor binning factor
    UINT                           ZZHDRColorPattern;           ///< ZZHDR Color Pattern Information
    ZZHDRFirstExposurePattern      ZZHDRFirstExposure;          ///< ZZHDR First Exposure Pattern
    SensorPDAFInfo                 sensorPDAFInfo;              ///< Sensor PDAF information
    UINT8                          bayerPattern;                ///< Sensor Bayer pattern
    UINT8                          CSIDBinningFactor;           ///< CSID binning factor
    UINT32                         fullResolutionWidth;         ///< Full resolution width
    UINT32                         fullResolutionHeight;        ///< Full resolution height
    UINT32                         sensorAspectRatioMode;       ///< for 4:3 senosr mode is FULLMODE(0)
                                                                ///  other mode NON_FULLMODE(1)
};

/// @ brief Structure to encapsulate HVX data
struct ISPHVXConfigureData
{
    StreamDimension    sensorInput;     ///< Sensor Input info
    PixelFormat        format;          ///< Image Format
    StreamDimension    HVXOut;          ///< HVX output info
    CropInfo           HVXCrop;         ///< HVX crop request
    CropInfo           origCAMIFCrop;   ///< Orig CAMIF
    CropWindow         origHALWindow;   ///< Orig Hal Window
    BOOL               DSEnabled;       ///< Down scale enabled
};

/// @ brief Structure to encapsulate CHI HVX interface
struct IFEHVXInfo
{
    CHIISPHVXALGORITHMCALLBACKS* pHVXAlgoCallbacks;  ///< Algo Calllback Interface for Chi to call into custom node.
    BOOL                         enableHVX;          ///< Enable HVX
    IFEDSPMode                   DSPMode;            ///< DSP mode
    ISPHVXConfigureData          HVXConfiguration;   ///< HVX configuration data
};

/// @ brief Structure to encapsulate Pixel subsample pattern
struct ISPPDDataSubsamplePattern
{
    UINT16 pixelSkipPattern; ///< Pixel Skip Pattern
    UINT16 lineSkipPattern;  ///< Line Skip pattern
};

/// @ brief Structure to encapsulate CAMIF Config data include Sabsample, will replace this structure
struct IFECAMIFInfo
{
    BOOL                      enableCAMIFSubsample;  ///< Enable CAMIF SubSample
    ISPPDDataSubsamplePattern CAMIFSubSamplePattern; ///< CAMIF subsample pattern
    CropInfo                  PDAFCAMIFCrop;         ///< PDAF Camif Crop Information
};

/// @brief IFE CSID Paths
enum IFECSIDPath
{
    IFECSIDIPP = 0, ///< IFE CSID Image Pixel Path
    IFECSIDPPP,     ///< IFE CSID PDAF Pixel Path
    IFECSIDRDI0,    ///< IFE CSID RDI0 Path
    IFECSIDRDI1,    ///< IFE CSID RDI1 Path
    IFECSIDRDI2,    ///< IFE CSID RDI2 Path
    IFECSIDRDI3,    ///< IFE CSID RDI3 Path
    IFECSIDMAX,     ///< IFE CSID Max  Path
};

/// @brief List of all IFE CAMIF Path Type
enum IFECAMIFPath
{
    IFECAMIFPXLPath = 0,   ///< Pixel Process pipeline path
    IFECAMIFDualPDPath,    ///< CAMIF Dual PD pipeline path
    IFECAMIFLCRPath,       ///< CAMIF LCR Pipeline path
    IFECAMIFRDI0Path,      ///< CAMIF RDI0 Pipeline path
    IFECAMIFRDI1Path,      ///< CAMIF RDI1 Pipeline path
    IFECAMIFRDI2Path,      ///< CAMIF RDI2 Pipeline path
    IFECAMIFRDI3Path,      ///< CAMIF RDI3 Pipeline path
    IFECAMIFPathMax,       ///< CAMIF Max path
};

/// @brief IFE Hardware Type List, include IFE Lite and IFE Normal
enum IFEHWTypeIds
{
    IFEHWTypeNormal = 0,   ///< Normal IFE ID
    IFEHWTypeLite,         ///< IFE Lite ID
    IFEHWTypeMax,          ///< IFE HW Type max
};

/// @ brief Structure to encapsulate All CAMIF configure data
struct IFECAMIFCfgInfo
{
    BOOL                      enableCAMIFPath[IFECAMIFPathMax]; ///< CAMIF Path enable
    IFEHWTypeIds              IFEHWType;                        ///< IFE Normal or IFE Lite
    IFECAMIFPath              CAMIFPathType;                    ///< Current Path Type
    BOOL                      enableCAMIFSubsample;             ///< Enable CAMIF Subsample
    ISPPDDataSubsamplePattern CAMIFSubsamplePattern;            ///< CAMIF subsample pattern
    CropInfo                  CAMIFCropInfo;                    ///< CAMIF Module crop information
};


/// @ brief Structure to encapsulate CSID Subsample data
struct IFECSIDExtractionInfo
{
    BOOL                      enableCSIDSubsample;  ///< Enable CSID SubSample
    ISPPDDataSubsamplePattern CSIDSubSamplePattern; ///< CSID subsample pattern
};

/// @ brief Structure to encapsulate PDAF data
struct IFEPDAFInfo
{
    BOOL                 enableSubsample;         ///< Enable PDAF SubSample
    UINT16               pixelSkipPattern;        ///< Pixel Skip Pattern
    UINT16               lineSkipPattern;         ///< Line Skip Pattern
    UINT32               bufferWidth;             ///< Width of the Buffer
    UINT32               bufferHeight;            ///< Height of the buffer
    UINT32               alignedBufferWidth;      ///< Aligned Buffer Width
    PDLibDataBufferInfo  pdafBufferInformation;   ///< Pdaf buffer information
};

/// @ brief Structure to encapsulate sensor dependent data
struct ISPHALConfigureData
{
    StreamDimension    stream[MaxRoundClampPath];    ///< HAL streams, corresponding to ISP output paths DS4, DS16, Full, FD
    Format             format[MaxRoundClampPath];    ///< output pixel format
};

/// @brief IPE has 2 main processing sections, noise processing and post processing. Within Post processing (PPS), they are
/// they are differentiated based on their position with respect to LT module as preLTM and postLTM.
enum class IPEProcessingSection
{
    IPEAll,             ///< All processing blocks
    IPENPS,             ///< Noise Processing blocks (ANR, TF etc.)
    IPEPPSPreLTM,       ///< Pre Processing section blocks before LTM (including LTM)
    IPEPPSPostLTM,      ///< Post Processing section blocks after LTM (Color correction to GRA)
    IPEPPS,             ///< Pre + Post processing blocks
    IPEScale,           ///< Scale Section
    IPENoZoomCrop,          ///< All processing blocks except for fill zoom window
    IPEInvalidSection,  ///< Invalid IPE IQ block
};

enum class BPSProcessingSection
{
    BPSAll,             ///< All processing blocks
    BPSHNR,             ///< BPS HNR processing block
    BPSLSCOut,          ///< BPS Preceding LSC processing block
    BPSPostLSC,         ///< BPS Post LSC processing block
    BPSInvalidSection,  ///< Invalid BPS IQ block
};

/// @brief Structure to hold Crop, Round and Clamp module enable registers
struct IFEModuleEnableConfigOld
{
    IFE_IFE_0_VFE_MODULE_LENS_EN                 lensProcessingModuleConfig;         ///< Lens processing Module enable
    IFE_IFE_0_VFE_MODULE_COLOR_EN                colorProcessingModuleConfig;        ///< Color processing Module enable
    IFE_IFE_0_VFE_MODULE_ZOOM_EN                 frameProcessingModuleConfig;        ///< Frame processing Module enable
    IFE_IFE_0_VFE_FD_OUT_Y_CROP_RND_CLAMP_CFG    FDLumaCropRoundClampConfig;         ///< FD Luma path Module enable
    IFE_IFE_0_VFE_FD_OUT_C_CROP_RND_CLAMP_CFG    FDChromaCropRoundClampConfig;       ///< FD Chroma path Module enable
    IFE_IFE_0_VFE_FULL_OUT_Y_CROP_RND_CLAMP_CFG  fullLumaCropRoundClampConfig;       ///< Full Luma path Module enable
    IFE_IFE_0_VFE_FULL_OUT_C_CROP_RND_CLAMP_CFG  fullChromaCropRoundClampConfig;     ///< Full Chroma path Module enable
    IFE_IFE_0_VFE_DS4_Y_CROP_RND_CLAMP_CFG       DS4LumaCropRoundClampConfig;        ///< DS4 Luma path Module enable
    IFE_IFE_0_VFE_DS4_C_CROP_RND_CLAMP_CFG       DS4ChromaCropRoundClampConfig;      ///< DS4 Chroma path Module enable
    IFE_IFE_0_VFE_DS16_Y_CROP_RND_CLAMP_CFG      DS16LumaCropRoundClampConfig;       ///< DS16 Luma path Module enable
    IFE_IFE_0_VFE_DS16_C_CROP_RND_CLAMP_CFG      DS16ChromaCropRoundClampConfig;     ///< DS16 Chroma path Module enable
    IFE_IFE_0_VFE_MODULE_STATS_EN                statsEnable;                        ///< Stats Module Enable
    IFE_IFE_0_VFE_STATS_CFG                      statsConfig;                        ///< Stats config
    IFE_IFE_0_VFE_DSP_TO_SEL                     dspConfig;                          ///< Dsp Config
    IFE_IFE_0_VFE_MODULE_DISP_EN                 frameProcessingDisplayModuleConfig;    ///< Frame processing Disp module enable
    IFE_IFE_0_VFE_DISP_OUT_Y_CROP_RND_CLAMP_CFG  displayFullLumaCropRoundClampConfig;   ///< Full Disp Luma path Module enable
    IFE_IFE_0_VFE_DISP_OUT_C_CROP_RND_CLAMP_CFG  displayFullChromaCropRoundClampConfig; ///< Full Disp Chroma path Module enable
    IFE_IFE_0_VFE_DISP_DS4_Y_CROP_RND_CLAMP_CFG  displayDS4LumaCropRoundClampConfig;    ///< DS4 Disp Luma path Module enable
    IFE_IFE_0_VFE_DISP_DS4_C_CROP_RND_CLAMP_CFG  displayDS4ChromaCropRoundClampConfig;  ///< DS4 Disp Chroma path Module enable
    IFE_IFE_0_VFE_DISP_DS16_Y_CROP_RND_CLAMP_CFG displayDS16LumaCropRoundClampConfig;   ///< DS16 Disp Luma path Module enable
    IFE_IFE_0_VFE_DISP_DS16_C_CROP_RND_CLAMP_CFG displayDS16ChromaCropRoundClampConfig; ///< DS16 Disp Chroma path Module enable
};


/// @brief Union to hold IFE pipeline core enable config
typedef union
{
    struct
    {
        BIT pedestalEnable          : 1;
        BIT liniearizationEnable    : 1;
        BIT demuxEnable             : 1;
        BIT chromaUpsampleEnable    : 1;
        BIT HDRReconEnable          : 1;
        BIT HDRMACEnable            : 1;
        BIT BPCEnable               : 1;
        BIT ABFEnable               : 1;
        BIT rolloffEnable           : 1;
        BIT demosaicEnable          : 1;
        BIT BLSEnable               : 1;
        BIT PDAFEnable              : 1;
        BIT binningCorrectionEnable : 1;
        BIT CCEnable                : 1;
        BIT GTMEnable               : 1;
        BIT gammaEnable             : 1;
        BIT CSTEnable               : 1;
        BIT PDAFPathEnable          : 1;
        BIT pixelRawPathEnable      : 1;
        BIT DSPHVXTapoutConfig      : 1;
        BIT HDREnable               : 1;
    };
    UINT    IQModulesConfig;
} IQModulesCoreEnableConfig;

/// @brief Union to hold IFE pipeline Video and Display path enable config
typedef union
{
    struct
    {
        BIT MNDSEnable                         : 1;
        BIT CropEnable                         : 1;
        BIT DS4PostCropEnable                  : 1;
        BIT DS4PreCropEnable                   : 1;
        BIT DS16PostCropEnable                 : 1;
        BIT DS16PreCropEnable                  : 1;
        BIT DS4LumaEnable                      : 1;
        BIT DS16LumaEnable                     : 1;
        BIT DS4ChromaEnable                    : 1;
        BIT DS16ChromaEnable                   : 1;
        BIT DS4R2PDEnable                      : 1;
        BIT DS16R2PDEnable                     : 1;
        BIT RoundClampLumaCropEnable           : 1;
        BIT RoundClampLumaRoundEnable          : 1;
        BIT RoundClampLumaClampEnable          : 1;
        BIT RoundClampChromaCropEnable         : 1;
        BIT RoundClampChromaRoundEnable        : 1;
        BIT RoundClampChromaClampEnable        : 1;

        BIT DS4RoundClampLumaCropEnable        : 1;
        BIT DS4RoundClampLumaRoundEnable       : 1;
        BIT DS4RoundClampLumaClampEnable       : 1;
        BIT DS4RoundClampChromaCropEnable      : 1;
        BIT DS4RoundClampChromaRoundEnable     : 1;
        BIT DS4RoundClampChromaClampEnable     : 1;

        BIT DS16RoundClampLumaCropEnable       : 1;
        BIT DS16RoundClampLumaRoundEnable      : 1;
        BIT DS16RoundClampLumaClampEnable      : 1;
        BIT DS16RoundClampChromaCropEnable     : 1;
        BIT DS16RoundClampChromaRoundEnable    : 1;
        BIT DS16RoundClampChromaClampEnable    : 1;
    };
    UINT videoDisplayPathEnableConfig;
} ImagePathEnableConfig;

/// @brief Union to hold IFE pipeline FD path enable config
typedef union
{
    struct
    {
        BIT MNDSEnable                    : 1;
        BIT CropEnable                    : 1;
        BIT RoundClampLumaCropEnable      : 1;
        BIT RoundClampLumaRoundEnable     : 1;
        BIT RoundClampLumaClampEnable     : 1;
        BIT RoundClampChromaCropEnable    : 1;
        BIT RoundClampChromaRoundEnable   : 1;
        BIT RoundClampChromaClampEnable   : 1;
    };
    UINT frameProcessingModuleConfig;
} FDPathEnableConfig;

/// @brief Union to hold stats modules common config
typedef union
{
    struct
    {
        BIT HDRBEFieldSelect        :2;
        BIT HDRSatsSiteSelect       :1;
        BIT BhistBinUniformity      :1;
        BIT AWBBGQuadSyncEnable     :1;
        BIT colorConversionEnable   :1;
        BIT RSShiftBits             :4;
        BIT CSShiftBits             :4;
        BIT HDRBhistFieldSelect     :2;
        BIT HDRBhistChannelSelect   :1;
        BIT BhistChannelSelect      :3;
        BIT IhistChannelSelect      :2;
        BIT IHistSiteSelect         :2;
        BIT IHistShiftBits          :4;
    };
    UINT statsModulesConfig;
} StatsModulesControl;

/// @brief Union to hold IFE pipeline stats modules core enable config
typedef union
{
    struct
    {
        BIT HDRBEEnable     : 1;
        BIT HDRBhistEnable  : 1;
        BIT BAFEnable       : 1;
        BIT AWBBGEnable     : 1;
        BIT SkinBhistEnable : 1;
        BIT RSEnable        : 1;
        BIT CSEnable        : 1;
        BIT IHistEnable     : 1;
        BIT AECBGEnable     : 1;
    };
    UINT statsModulesEnableConfig;
} StatsModulesCoreEnableConfig;

/// @brief Structure to hold IFE pipeline DSP tapout selection
typedef union
{
    struct
    {
        BIT DSPSelect  : 3;
    };
    UINT DSPModuleConfig;

} DSPConfig;

/// @brief Structure to hold IFE common config
struct IFEModuleEnableConfig
{
    IQModulesCoreEnableConfig       IQModules;                  ///< IFE pipeline IQ modules enable config
    ImagePathEnableConfig           videoProcessingModules;     ///< Video frame processing modules enable config
    ImagePathEnableConfig           displayProcessingModules;   ///< Display frame processing modules enable config
    FDPathEnableConfig              FDprocessingModules;        ///< FD frame processing modules enable config
    StatsModulesControl             statsModules;               ///< Stats processing modules control config
    DSPConfig                       DSPConfig;                  ///< DSP tap out selection config
};

/// @brief MNDS state
struct MNDSState
{
    CropWindow         cropWindow;          ///< crop window for the client requested frame
    CropInfo           CAMIFCrop;           ///< Camif crop request from sensor
    UINT32             inputWidth;          ///< Camif output width in pixel
    UINT32             inputHeight;         ///< Camif output height in pixel
    UINT32             rndOptionH;          ///< Rnd Option H
    UINT32             rndOptionV;          ///< Rnd Option V
    UINT32             lumaImgSizeChange;   ///< Luma Img Size
    StreamDimension    streamDimension;     ///< FD path dimension
    IFEScalerOutput    MNDSOutput;          ///< MNDS output info fed to Crop module as input
    IFEModuleFlags     moduleFlags;         ///< MNDS module flags
};

/// @brief HVX state
struct HVXState
{
    UINT32             inputWidth;       ///< Input output width in pixel
    UINT32             inputHeight;      ///< Input output height in pixel
    CropInfo           cropWindow;       ///< Camif crop request
    StreamDimension    hvxOutDimension;  ///< HVX Out dimension
    FLOAT              scalingFactor;    ///< input to output scaling factor
    UINT32             overlap;          ///< OverLap
    UINT32             rightImageOffset; ///< Image Offset
};

/// @brief CROP state
struct CropState
{
    StreamDimension streamDimension;    ///< FD/Full/DS4/DS16 path dimension
    CropWindow      cropWindow;         ///< crop window for the requested frame
    CropWindow      modifiedCropWindow; ///< crop window for the requested frame
    IFEScalerOutput scalerOutput;       ///< Scaler output dimension and scale factor
    CHIRectangle    cropInfo;           ///< output path crop info to IPE
    CHIRectangle    appliedCropInfo;    ///< crop info IFE applied
};

/// @brief Round and Clamp state
struct RoundClampState
{
    StreamDimension FDDimension;            ///< RoundClamp FD path dimension
    StreamDimension fullDimension;          ///< RoundClamp (Video) Full path dimension
    StreamDimension DS4Dimension;           ///< RoundClamp (Video) DS4 path dimension
    StreamDimension DS16Dimension;          ///< RoundClamp (Video) DS16 path dimension
    StreamDimension displayFullDimension;   ///< RoundClamp Display Full path dimension
    StreamDimension displayDS4Dimension;    ///< RoundClamp Display DS4 path dimension
    StreamDimension displayDS16Dimension;   ///< RoundClamp Display DS16 path dimension
    StreamDimension pixelRawDimension;      ///< RoundClamp Pixel Raw path dimension
    IFEModuleFlags  moduleFlags;            ///< MNDS module flags
};

/// @brief Indicates how the exessive crop window should be adjusted to match output size
enum CropType
{
    CropTypeCentered = 0, ///< Crop equally from left and right
    CropTypeFromLeft,     ///< Crop from left
    CropTypeFromRight     ///< Crop from right
};

/// @brief DS4 path PreCrop Luma(Y) and Chroma(Cr) crop params
struct DS4PreCropInfo
{
    CropInfo YCrop;                 ///< Crop info for Y
    CropInfo CrCrop;                ///< Crop info for Cr
    UINT64   preDSXphaseInitLuma;   ///< preDSX_phase_init_Y;
    UINT64   preDSXphaseInitChroma; ///< preDSX_phase_init_C;
};

/// @brief DS4/16 state
struct DSState
{
    UINT32           sensorWidth;        ///< Sensow Width
    UINT32           sensorHeight;       ///< Sensor Height
    CropWindow       cropWindow;         ///< crop window for the client requested frame
    StreamDimension  DSXOutput;          ///< DSX out put width and height
    StreamDimension  DSXInput;           ///< DSX Input Width and Height
    FLOAT            DSXscaleFactor;     ///< DSX input to output scaling
    StreamDimension  MNDSOutput;         ///< MNDS module output dimension
    StreamDimension  DS4PathOutput;      ///< DS4 path output dimension
    StreamDimension  DS16PathOutput;     ///< DS16 path output dimension
    IFEModuleFlags   moduleFlags;        ///< DS4 1.0 module flags
    DS4PreCropInfo   preCropInfo;        ///< DS4 PreCrop Info
    BOOL             overWriteStripes;   ///< OverWrite Stripes
    CropType         cropType;           ///< cropType
    UINT32           bankSelect;         ///< Bank Select
};

/// @brief ABF state
struct ABFState
{
    union
    {
        ABF34InputData  dependenceData;     ///< Dependence Data for ABF34
        ABF40InputData  dependence40Data;   ///< Dependence Data for ABF40
    };
};

/// @brief LSC state
struct LSCState
{
    union
    {
        LSC34InputData      dependenceData;   ///< Dependence Data for this Module
        LSC40InputData      dependence40Data; ///< Dependence Data for this Module
    };
};

/// @brief BF state
struct BFStatsState
{
    UINT8 ROIIndexLUTBank;  ///< Index of ROI LUT bank currently selected
    UINT8 gammaLUTBank;     ///< Index of Gamma LUT bank currently selected
};

/// @brief Stats structutre to describe tap-out site selection
struct IFEStatsTapOut
{
    UINT32   HDRBEStatsSrcSelection;     ///< PRE_HDR: 0, POST_LSC: 1
    UINT32   HDRBHistStatsSrcSelection;  ///< PRE_HDR: 0, POST_LSC: 1
    UINT32   IHistStatsSrcSelection;     ///< PRE_GLUT: 0, POST_GLUT: 1
};

/// @ brief Structure to encapsulate IFE metadata
struct IFEMetadata
{
    IFECropInfo              cropInfo;                       ///< Remaining crop info for IPE to apply on IFE output
    IFECropInfo              appliedCropInfo;                ///< Crop info IFE applied
    ISPAWBBGStatsConfig      AWBBGStatsConfig;               ///< AWBBG Stats Config data for Parser Consumption
    ISPBFStatsConfig         BFStats;                        ///< BF configuration info
    ISPBHistStatsConfig      BHistStatsConfig;               ///< BHist Stats Config data for Parser Consumption
    ISPCSStatsConfig         CSStats;                        ///< CS stats configuration for stats parser
    ISPHDRBEStatsConfig      HDRBEStatsConfig;               ///< HDRBE Stats Config data for Parser Consumption
    ISPHDRBHistStatsConfig   HDRBHistStatsConfig;            ///< HDR BHist Stats Config data for Parser Consumption
    ISPIHistStatsConfig      IHistStatsConfig;               ///< IHist Stats Config data for Parser Consumption
    ISPRSStatsConfig         RSStats;                        ///< RS stats configuration for stats parser
    ISPTintlessBGStatsConfig tintlessBGStats;                ///< TintlessBG stats configuration for stats parser
    UINT8                    colorCorrectionAberrationMode;  ///< color correction aberration mode
    UINT32                   edgeMode;                       ///< edge mode value
    UINT8                    noiseReductionMode;             ///< noise reduction mode value
};

struct ISPInternalData;

/// @brief Structure to encapsulate IFE frame-level inter module dependent data
struct ISPFrameInternalData
{
    ISPInternalData* pFrameData; ///< Full frame ISP data
};

/// @brief Structure to encapsulate Lens shading attributes
struct LensShadingInfo
{
    DimensionCap lensShadingMapSize;                                ///< Lens Shading map size
    FLOAT        lensShadingMap[TotalChannels * MESH_ROLLOFF_SIZE]; ///< Lens Shading Map per channel
    UINT8        lensShadingMapMode;                                ///< Lens Shading map mode
    UINT8        shadingMode;                                       ///< Shading mode
};

/// @brief Tonamap point
struct ISPTonemapPoint
{
    FLOAT point[2]; ///< Input & output coordinates (IN, OUT), normalized from 0.0 to 1.0
};

struct ISPTonemapCurves
{
    UINT8           tonemapMode;                        ///< Tonemap mode
    INT32           curvePoints;                        ///< Number of tonemap curve points passed by framework
    ISPTonemapPoint tonemapCurveBlue[MaxCurvePoints];   ///< Pointer to tonemap curve blue
    ISPTonemapPoint tonemapCurveGreen[MaxCurvePoints];  ///< Pointer to tonemap curve green
    ISPTonemapPoint tonemapCurveRed[MaxCurvePoints];    ///< Pointer to tonemap curve red
};

/// @brief IFEWMData, contains the WM Update date
struct IFEWMData
{
    UINT32 portID;         ///< Port ID of the WM
    UINT32 hInit;          ///< Startng Pixel Offset in Pixels
    UINT32 width;          ///< Updated width of WM
    UINT32 height;         ///< Updated width of WM
    UINT32 mode;           ///< Updated mode 0x0: Line 0x1: Frame 0x2: Index
    UINT32 virtualFrameEn; ///< Virtual Frame Enable
};

/// @brief IFEWMUpdate
struct IFEWMUpdate
{
    UINT32    numberOfWMUpdates;                ///< Number of WM updates
    IFEWMData WMData[ISPMaxOutputPorts];        ///< WM Update data
};

/// @brief ISPCalculatedMetadata
struct ISPCalculatedMetadata
{
    FLOAT                   stretchGainBlue;                                ///< Stretch Gain Blue
    FLOAT                   stretchGainRed;                                 ///< Stretch Gain Red
    FLOAT                   stretchGainGreenEven;                           ///< Stretch Gain Green Even
    FLOAT                   stretchGainGreenOdd;                            ///< Stretch Gain Green Odd
    UINT32                  BLSblackLevelOffset;                            ///< Black level offset
    UINT32                  linearizationAppliedBlackLevel[ISPChannelMax];  ///< Dynamic Black Level
    HotPixelModeValues      hotPixelMode;                                   ///< hot pixel mode
    BOOL                    demuxEnable;                                    ///< Demux Enable
};

/// @brief Structure to set IFE DMI banks update
struct IFEDMIBankUpdate
{
    BOOL   isValid;           ///< TRUE if the Update is Valid
    UINT32 CSIDBank;          ///< CSID DMI Bank
    UINT32 pedestalBank;      ///< Pedestal DMI Bank
    UINT32 linearizaionBank;  ///< Linearization DMI Bank
    UINT32 BPCPDPCBank;       ///< BPS PDPC DMI Bank
    UINT32 ABFBank;           ///< ABF DMI Bank
    UINT32 LSCBank;           ///< LSC DMI Bank
    UINT32 GTMBank;           ///< GTM DMI Bank
    UINT32 gammaBank;         ///< Gamma DMI Bank
    UINT32 BFStatsDMIBank;    ///< BF Stats DMI Bank
    UINT32 DSXYBank;          ///< DSX Y DMI Bank
    UINT32 DSXCBank;          ///< DSX C DMI Bank
    UINT32 HDRBHistBank;      ///< HDR Bhist DMI Bank
    UINT32 BHistBank;         ///< Bhist DMI Bank
    UINT32 IHistBank;         ///< IHist DMI Bank
    UINT32 PDAFBank;          ///< PDAFBank
};

/// @ brief Structure to encapsulate IFE inter module dependent data
struct ISPInternalData
{
    FLOAT                   clamping;                                       ///< Clamping Value from Linearization Mode
    IFEScalerOutput         scalerOutput[MaxRoundClampPath];                ///< MNDS/DS4/DS16 output
    CropInfo                fullOutCrop;                                    ///< Full Luma post crop
    CropInfo                dispOutCrop;                                    ///< Display Luma post crop
    CropInfo                fdOutCrop;                                      ///< FD Luma post crop
    BIT                     isVideoDS4Enable;                               ///< Bool to check if DS4 path is enabled
    BIT                     isDisplayDS4Enable;                             ///< Bool to check if Display DS4 path is enabled
    IFEModuleEnableConfig   moduleEnable;                                   ///< IFE module enable register configuration
    IFEMetadata             metadata;                                       ///< IFE metadata
    UINT32                  blackLevelOffset;                               ///< Black level offset
    GammaInfo               gammaOutput;                                    ///< IFE / BPS Gamma output table
    FLOAT                   dynamicBlackLevel[ISPChannelMax];               ///< Dynamic Black Level
    LensShadingInfo         lensShadingInfo;                                ///< Lens Shading attributes
    DS4PreCropInfo          preCropInfo;                                    ///< DS4 path PreCrop module
    DS4PreCropInfo          preCropInfoDS16;                                ///< DS16 path PreCrop module
    DS4PreCropInfo          dispPreCropInfo;                                ///< DS4 display path PreCrop module
    DS4PreCropInfo          dispPreCropInfoDS16;                            ///< DS16 display path PreCrop module
    BOOL                    scalerDependencyChanged[MaxRoundClampPath];     ///< TRUE if full frame scaler dependency changed
    BOOL                    cropDependencyChanged[MaxRoundClampPath];       ///< TRUE if full frame scaler dependency changed
    BOOL                    lscDependencyChanged;                           ///< TRUE if lens roll off dependency changed
    Rational                CCTransformMatrix[3][3];                        ///< Color Correction transformation matrix
    UINT8                   colorCorrectionMode;                            ///< color correction mode
    ColorCorrectionGain     colorCorrectionGains;                           ///< color correction gains applied by WB
    UINT8                   blackLevelLock;                                 ///< Calculated black level lock
    HotPixelModeValues      hotPixelMode;                                   ///< hot pixel mode
    INT32                   controlPostRawSensitivityBoost;                 ///< Applied isp gain
    UINT8                   noiseReductionMode;                             ///< Noise reduction mode
    ISPTonemapCurves        toneMapData;                                    ///< tone map mode, curve points and curve
    FLOAT                   percentageOfGTM;                                ///< gtmPercentage for adrc
    IPEGammaPreOutput       IPEGamma15PreCalculationOutput;                 ///< Pre-calculation output for IPE gamma15
    IFEWMUpdate             WMUpdate;                                       ///< Max IFE Output ports per IFE.
};

/// @brief CVP Instance Profile Id
enum CVPProfileId
{
    CVPProfileIdDME = 0,            ///< DME only
    CVPProfileIdDMEwithICA,         ///< DME with ICA enabled
    CVPProfileIdICA,                ///< ICA only
    CVPProfileIdRegistration,       ///< Registration
    CVPProfileIdMax                 ///< Invalid
};

/// @brief CVP Instance Processing Id
enum CVPProcessingType
{
    CVPProcessingTypeDefault = 0,   ///< No Processing Type
    CVPProcessingTypePrefilter,     ///< Prefilter for MFNR / MFSR
    CVPProcessingTypeBlend,         ///< Blend for both MFNR / MFSR
    CVPProcessingTypeMax            ///< Invalid
};


/// @brief Instance Profile Id
enum IPEProfileId
{
    IPEProfileIdDefault = 0,    ///< All IQ Modules
    IPEProfileIdNPS,            ///< Noise Profile (ANR, TF, ICA1, ICA2)
    IPEProfileIdPPS,            ///< Post Processing Profile (All IQ blocks expect ANR, TF, ICA1, ICA2)
    IPEProfileIdScale,          ///< ScaleProfile (None)
    IPEProfileIdNoZoomCrop,     ///< All IQ Modules except fill zoom window
    IPEProfileIdHDR10,          ///< All IQ Modules process in HDR10 mode. Not a valid profile. need to be removed.
    IPEProfileIdIndications,    ///< Profile Indications
    IPEProfileIdICAWarpOnly,    ///< Use ICA1 for grid warping
    IPEProfileIdUpscale,        ///< Zoom only profile
    IPEProfileIdMax             ///< Invalid
};

/// @brief Instance Stabilization Id
enum IPEStabilizationType : UINT32
{
    IPEStabilizationTypeNone     = 0,   ///< No Stabilization
    IPEStabilizationTypeEIS2     = 2,   ///< EIS2.0
    IPEStabilizationTypeEIS3     = 4,   ///< EIS3.0
    IPEStabilizationMCTF         = 8,   ///< Motion Controlled alignment
    IPEStabilizationTypeSWEIS2   = 16,  ///< SW EIS2.0
    IPEStabilizationTypeSWEIS3   = 32,  ///< SW EIS3.0
    IPEStabilizationTypeSAT      = 64,  ///< SAT Transform
    IPEStabilizationTypeMax      = 128, ///< Invalid/max
};

/// @brief Instance Processing Id
enum IPEProcessingType
{
    IPEProcessingTypeDefault = 0,   ///< No Processing Type
    IPEMFNRPrefilter,               ///< Prefilter
    IPEMFNRBlend,                   ///< Blend
    IPEMFNRScale,                   ///< Scale
    IPEMFNRPostfilter,              ///< Postfilter
    IPEProcessingPreview,           ///< Preview Processing type
    IPEMFSRPrefilter,               ///< Prefilter
    IPEMFSRBlend,                   ///< Blend
    IPEMFSRPostfilter,              ///< Postfilter
};

/// @brief CVP Instance Property Information
struct CVPInstanceProperty
{
    CVPProfileId            profileId;                            ///< CVP profile ID
    CVPProcessingType       processingType;                       ///< CVP processing type
    UINT32                  stabilizationType;                    ///< EIS2.0/ EIS3.0 / MCTF with EIS
};


/// @brief IPE IQ Instance Property Information
struct IPEInstanceProperty
{
    UINT                    ipeOnlyDownscalerMode;                ///< IPE only downscaler enabled
    UINT                    ipeDownscalerWidth;                   ///< IPE only downscaler enabled width
    UINT                    ipeDownscalerHeight;                  ///< IPE only downscaler enabled height
    IPEProfileId            profileId;                            ///< NPS/PPS/Scale
    UINT32                  stabilizationType;                    ///< EIS2.0/ EIS3.0 / MCTF with EIS
    IPEProcessingType       processingType;                       ///< prefilter, blend, scale, postfilter
    BOOL                    enableCHICropInfoPropertyDependency;  ///< ensable CHI crop property dependency
    UINT                    enableFOVC;                           ///< FOVC Enable
};


/// @brief Instance Profile Id
enum IFEProfileId
{
    IFEProfileIdDefault = 0,    ///< Real time pipeline
    IFEProfileIdOffline,        ///< Offline mode
    IFEProfileIdMax,            ///< Invalid
};

/// @brief IFE IQ Property Information
struct IFEInstanceProperty
{
    UINT            IFECSIDHeight;          ///< IFE Dynamic CSID Height
    UINT            IFECSIDWidth;           ///< IFE Dynamic CSID Width
    UINT            IFECSIDLeft;            ///< IFE Dynamic CSID Left
    UINT            IFECSIDTop;             ///< IFE Dynamic CSID Top
    UINT            IFEStatsSkipPattern;    ///< IFE Stats Skip Pattern
    UINT            IFESingleOn;            ///< IFE flag to force IFE to single IFE
    IFEProfileId    profileId;              ///< realtime/offline
};

/// @brief IPEPipelineData, IPE pipeline configuration data
struct IPEPipelineData
{
    const INT32*         pDeviceIndex;              ///< Pointer to ICP device index
    VOID*                pFrameProcessData;         ///< Pointer to firmware payload of type  IpeFrameProcessData
    VOID*                pIPEIQSettings;            ///< Pointer to IpeIQSettings
    VOID*                pWarpGeometryData;         ///< Warp geometry Data
    CmdBuffer**          ppIPECmdBuffer;            ///< Pointer to pointers of IPE command buffer managers
    UINT                 batchFrameNum;             ///< Number of frames in a batch
    UINT                 numPasses;                 ///< number of passess
    UINT8                hasTFRefInput;             ///< 1: TF has reference input; 0: TF has no reference input
    UINT8                isDigitalZoomEnabled;      ///< 1: digital zoom enable, 0: disable
    FLOAT                upscalingFactorMFSR;       ///< MFSR upscaling factor
    UINT32               configIOTopology;          ///< IPE configIO topology type
    UINT32               digitalZoomStartX;         ///< Digital zoom horizontal start offset
    UINT32               digitalZoomStartY;         ///< Digital zoom vertical start offset
    UINT32               ds4InputWidth;             ///< LENR Input stream width
    UINT32               ds4InputHeight;            ///< LENR Input stream height
    UINT32               instanceID;                ///< instance ID;
    UINT32               numOfFrames;               ///< Number of frames used in MFNR/MFSR case
    UINT32               numOutputRefPorts;         ///< Number of Output Reference Ports enabled for IPE
    UINT32               realtimeFlag;              ///< Flag for Realtime stream
    Format               fullInputFormat;           ///< Imageformat
    IPEInstanceProperty  instanceProperty;          ///< IPE Node Instance Property
    ImageDimensions      inputDimension;            ///< Input Dimension
    ImageDimensions      marginDimension;           ///< Margin Dimension
    ImageDimensions      fullInputDimension;        ///< Input Dimension
    BOOL                 compressiononOutput;       ///< Compression enabled on Output
    ConfigICAMode        ICAMode;                   ///< ICA Mode
    BOOL                 isLowResolution;           ///< Check the reolution is low or Higher for R manula control
    INT16                LTM_DC_Pass;               ///< DC pass num
    BOOL                 LTM_DC_Pass_Enable;        ///< DC pass enable
    DebugDataWriter*     pDebugDataWriter;          ///< Debug data writer for dumping tuning metadata
    GeoLibParameters*    pGeoLibParameters;         ///< Geolib Parameters;
};

/// @brief CVPPipelineData, CVP module data
struct CVPPipelineData
{
    VOID*                          pCVPICAFrameCfgData;            ///< CVP ICA frame config Data
    GeoLibIcaPassMapping           icaInputData;                   ///< ICA input Data
};

static const UINT MFSRDownscaleRatioShift           = 1;           ///< Currently the MFSR crop ratio is 2x,
                                                                   ///  so we need to right shift 1 bit to achieve it
static const UINT MaxIndexRegInputResolutionMap     = 7;

/// @brief Structure for registration input resolution info
struct RegInputResolutionInfo
{
    UINT32  width;      ///<    Width
    UINT32  height;     ///<    Height
};

/// @brief The mapping table of registration input resolution from tuning XML
static const RegInputResolutionInfo RegInputResolutionMap[]
{
    { 1920, 1440 },     ///<    Registration input resolution: 1440p
    { 1920, 1280 },     ///<    Registration input resolution: 1280p
    { 1920, 1080 },     ///<    Registration input resolution: 1080p
    { 1280, 960 },      ///<    Registration input resolution: 960p
    { 1280, 720 },      ///<    Registration input resolution: 720p
    { 960,  540 },      ///<    Registration input resolution: 540p
    { 480,  270 },      ///<    Registration input resolution: 270p
};

/// @brief Dual IFE stripe Id
enum DualIFEStripeId
{
    DualIFEStripeIdLeft = 0,    ///< Left stripe
    DualIFEStripeIdRight        ///< Right stripe
};

/// @brief Dual IFE impact for an IQ module
struct IQModuleDualIFEData
{
    BOOL dualIFESensitive;      ///< Indicates if the IQ module is affected by dual IFE mode
    BOOL dualIFEDMI32Sensitive; ///< Indicates if the left and right stripes needs separate DMI32 tables
    BOOL dualIFEDMI64Sensitive; ///< Indicates if the left and right stripes needs separate DMI64 tables
};

/// @brief IFEPipelineData
struct IFEPipelineData
{
    BOOL             programCAMIF;       ///< Set to True, if need to program CAMIF
    IFEModuleMode    moduleMode;         ///< IFE IQ Module Operation Mode Single IFE/ Dual IFE
    UINT             reserved;           ///< IFE pipeline configuration data
    ISPIQModuleType  IFEModuleType;      ///< IFE pipeline moudule Type
    IFEPipelinePath  IFEPath;            ///< IFE pipeline path
    UINT             numBatchedFrames;   ///< Number of Frames in the batch mode, otherwise it is 1
};

/// @brief IFEPipelineData
struct IFEDualPDPipelineData
{
    BOOL             programCAMIFLite;   ///< Set to True, if need to program CAMIF
    UINT             numBatchedFrames;   ///< Number of Frames in the batch mode, otherwise it is 1
};

/// @brief BPSPipelineData, BPS pipeline configuration data
struct BPSPipelineData
{
    VOID*            pIQSettings;         ///< Pointer to FW BPS IQ settings structure
    CmdBuffer**      ppBPSCmdBuffer;      ///< Pointer to pointers of BPS command buffer managers
    const INT32*     pDeviceIndex;        ///< Pointer to ICP device index
    UINT32           width;               ///< Stream width in pixels
    UINT32           height;              ///< Stream Height in pixels
    UINT32*          pBPSPathEnabled;     ///< Active BPS input/output info.
    DebugDataWriter* pDebugDataWriter;    ///< Debug data writer for dumping tuning metadata
};

/// @brief ISPAlgoStatsData, Contains the stats data required by ISP algorithms
struct ISPAlgoStatsData
{
    ParsedBHistStatsOutput*      pParsedBHISTStats;          ///< Parsed BHIST stats
    ParsedTintlessBGStatsOutput* pParsedTintlessBGStats;     ///< Parsed tintless BG stats
    ParsedAWBBGStatsOutput*      pParsedAWBBGStats;          ///< Parsed AWB BGStats
    ISPTintlessBGStatsConfig     tintlessBGConfig;           ///< TintlessBG config
    ISPBHistStatsConfig          BHistConfig;                ///< BHIST config
};

/// @brief ISP input configuration per stripe
struct ISPStripeConfig
{
    CropInfo            CAMIFCrop;                              ///< CAMIF crop request from sensor
    CropWindow          HALCrop[MaxRoundClampPath];             ///< Crop region from the client per stripe
    StreamDimension     stream[MaxRoundClampPath];              ///< Stream dimensions per stripe
    AECFrameControl     AECUpdateData;                          ///< AEC_Update_Data
    AECStatsControl     AECStatsUpdateData;                     ///< AEC stats module config data
    AWBFrameControl     AWBUpdateData;                          ///< AWB_Update_Data
    AWBStatsControl     AWBStatsUpdateData;                     ///< AWB stats Update Data
    AFFrameControl      AFUpdateData;                           ///< AF_Update_Data
    AFStatsControl      AFStatsUpdateData;                      ///< AF stats Update Data
    AFDStatsControl     AFDStatsUpdateData;                     ///< AFD_Update_Data
    CSStatsControl      CSStatsUpdateData;                      ///< CS Stats update data
    IHistStatsControl   IHistStatsUpdateData;                   ///< IHist stats module config data
    PDHwConfig          pdHwConfig;                             ///< PD HW Config Data
    HVXState            stateHVX;                               ///< HVX state
    MNDSState           stateMNDS[IFEMaxNonCommonPaths];        ///< MNDS state
    CropState           stateCrop[IFEMaxNonCommonPaths];        ///< Crop state
    RoundClampState     stateRoundClamp[IFEMaxNonCommonPaths];  ///< Round and clamp state
    DSState             stateDS[IFEMaxNonCommonPaths];          ///< DS4/16 state
    ABFState            stateABF;                               ///< ABF state
    LSCState            stateLSC;                               ///< LSC state
    BFStatsState        stateBF;                                ///< BF stats persistent data over multiple frames
    UINT32              stripeId;                               ///< Zero-indexed stripe identifier
    CropType            cropType;                               ///< Crop type
    StreamDimension     stats[MaxStatsPorts];                   ///< Stats output dimensions per stripe
    ISPAlgoStatsData    statsDataForISP;                        ///< Stats data for ISP
    /// @note This will point to a common area where all IFE stripes see the same data. Only dual-insensitive IQ modules
    ///       may write into it, but all module may read from it.
    ISPFrameInternalData*      pFrameLevelData;                ///< Frame-level internal ISP data
    IFEStripeInterfaceOutput*  pStripeOutput;                  ///< Stripe config from striping lib
    BOOL                       overwriteStripes;               ///< Overwrite module calculation using striping lib
    IFECAMIFInfo               CAMIFSubsampleInfo;             ///< Camif subsample Info for Pdaf
    IFECSIDExtractionInfo*     pCSIDSubsampleInfo;             ///< Pointer to the CSID SubSample Info
    IFECAMIFCfgInfo*           pCAMIFConfigInfo;               ///< Pointer to Camif configure Info for all path
};

/// @brief List of framework tags
static UINT32 ISPMetadataTags[] =
{
    BlackLevelLock                  ,
    ColorCorrectionAberrationMode   ,
    ColorCorrectionGains            ,
    ColorCorrectionMode             ,
    ColorCorrectionTransform        ,
    ControlAEMode                   ,
    ControlAWBMode                  ,
    ControlMode                     ,
    ControlPostRawSensitivityBoost  ,
    NoiseReductionMode              ,
    ShadingMode                     ,
    StatisticsHotPixelMapMode       ,
    StatisticsLensShadingMapMode    ,
    TonemapMode                     ,
    TonemapCurveBlue                ,
    TonemapCurveGreen               ,
    TonemapCurveRed                 ,
    ScalerCropRegion                ,
};



static const UINT NumISPMetadataTags = sizeof(ISPMetadataTags)/sizeof(UINT32);   ///< Number of vendor tags

static const UINT8 ISPMetadataTagReqOffset[NumISPMetadataTags] = { 0 };

/// @brief Framework tags requiring fer frame update by ISP
static const UINT ISPMetadataOutputTags[] =
{
    BlackLevelLock,
    ColorCorrectionAberrationMode,
    ColorCorrectionGains,
    ColorCorrectionMode,
    ColorCorrectionTransform,
    ControlPostRawSensitivityBoost,
    NoiseReductionMode,
    ShadingMode,
    StatisticsHotPixelMapMode,
    StatisticsLensShadingMapMode,
    ScalerCropRegion,
    TonemapMode,
    TonemapCurveBlue,
    TonemapCurveGreen,
    TonemapCurveRed,
};

static const UINT NumISPMetadataOutputTags = sizeof(ISPMetadataOutputTags) / sizeof(UINT32);   ///< Number of output vendor tags


/// @brief Color correction transform matrix
struct ISPColorCorrectionTransform
{
    CamxRational transformMatrix[ColorCorrectionRows][ColorCorrectionColumns];  ///< Color correction transform matrix
};

/// @brief Color correction gains
struct ISPColorCorrectionGains
{
    FLOAT gains[ISPColorCorrectionGainsMax];    ///< RGbGrB gains
};

/// @brief ISP tags
struct ISPHALTagsData
{
    UINT8                       blackLevelLock;                 ///< black level lock value
    UINT8                       colorCorrectionAberrationMode;  ///< color correction aberration mode
    ColorCorrectionGain         colorCorrectionGains;           ///< color correction gains
    UINT8                       colorCorrectionMode;            ///< color correction mode
    ISPColorCorrectionTransform colorCorrectionTransform;       ///< color correction transform matrix
    UINT8                       controlAEMode;                  ///< AE mode
    UINT8                       controlAWBMode;                 ///< AWB mode
    UINT8                       controlAWBLock;                 ///< AWB lock
    UINT8                       controlAECLock;                 ///< AEC lock
    UINT8                       controlMode;                    ///< main control mode switch
    INT32                       controlPostRawSensitivityBoost; ///< Raw sensitivity boost control
    HotPixelModeValues          hotPixelMode;                   ///< hot pixel mode
    UINT8                       noiseReductionMode;             ///< noise reduction mode
    UINT8                       shadingMode;                    ///< shading mode
    UINT8                       statisticsHotPixelMapMode;      ///< hot pixel map mode
    UINT8                       statisticsLensShadingMapMode;   ///< lens shading map mode
    ISPTonemapCurves            tonemapCurves;                  ///< tonemap curves points
    CropWindow                  HALCrop;                        ///< HAL crop window for zoom
    INT32                       saturation;                     ///< saturation value
    UINT32                      edgeMode;                       ///< edge mode value
    UINT8                       controlVideoStabilizationMode;  ///< EIS mode value
    FLOAT                       sharpness;                      ///< sharpness value
    UINT8                       contrastLevel;                  ///< contrast level value
    LtmConstrast                ltmContrastStrength;            ///< ltmContrastStrength value
};

/// @brief IFE IQ module enable
struct IFEIQModuleEnable
{
    BIT pedestal;   ///< Pedestal enable bit
    BIT rolloff;    ///< rolloff enable bit
    BIT BAF;        ///< BAF enable bit
    BIT BGTintless; ///< BGTintless enable bit
    BIT BGAWB;      ///< BGAWB enable bit
    BIT BE;         ///< BE enable bit
    BIT PDAF;       ///< PDAF enable bit
    BIT HDR;        ///< HDR enable bit
    BIT BPC;        ///< BPC enable bit
    BIT ABF;        ///< ABF enable bit
};

/// @brief Striping input
struct IFEStripingInput
{
    IFEIQModuleEnable       enableBits;     ///< Enable bits
    IFEStripeInterfaceInput  stripingInput;  ///< Striping library's input
};

/// @brief ISP Titan HW version
enum ISPHWTitanVersion
{
    ISPHwTitanInvalid = 0x0,       ///< Invalid Titan HW version
    ISPHwTitan170     = 0x1,       ///< Titan HW version for SDM845/SDM710/SDM670 only
    ISPHwTitan175     = 0x2,       ///< Titan HW version for SDM855 (SM8150) only
    ISPHwTitan150     = 0x4,       ///< Titan HW version for SDM640 (SM6150) only.
    ISPHwTitan480     = 0x8,       ///< Spectra HW version 480
};

/// @brief ISPInputData
struct ISPInputData
{
    AECFrameControl*        pAECUpdateData;               ///< Pointer to the AEC_Update_Data
    AECStatsControl*        pAECStatsUpdateData;          ///< Pointer to the AEC stats Update Data
    AWBFrameControl*        pAWBUpdateData;               ///< Pointer to the AWB_Update_Data
    AWBStatsControl*        pAWBStatsUpdateData;          ///< Pointer to the AWB stats Update_Data
    AFFrameControl*         pAFUpdateData;                ///< Pointer to the AF_Update_Data
    AFStatsControl*         pAFStatsUpdateData;           ///< Pointer to the AF stats Update_Data
    AFDStatsControl*        pAFDStatsUpdateData;          ///< Pointer to AFD stats update data
    CSStatsControl*         pCSStatsUpdateData;           ///< Pointer to CS stats update data
    IHistStatsControl*      pIHistStatsUpdateData;        ///< Pointer to IHist stat config update data
    PDHwConfig*             pPDHwConfig;                  ///< Pointer to PD HW Config Data
    CmdBuffer*              pCmdBuffer;                   ///< Pointer to the Command Buffer Object
    CmdBuffer*              p32bitDMIBuffer;              ///< Pointer to the 32 bit DMI Buffer Manager
    UINT32*                 p32bitDMIBufferAddr;          ///< Pointer to the 32 bit DMI Buffer
    CmdBuffer*              p64bitDMIBuffer;              ///< Pointer to the 64 bit DMI Buffer Manager
    UINT32*                 p64bitDMIBufferAddr;          ///< Pointer to the 64 bit DMI Buffer
    CmdBuffer*              pDMICmdBuffer;                ///< Pointer to the CDM packets of LUT DMI buffers
    UINT32                  sensorID;                     ///< Sensor ID Number
    ISPSensorConfigureData  sensorData;                   ///< Configuration Data from Sensor Module
    ISPHALConfigureData     HALData;                      ///< Configuration Data from HAL/App
    ISPHALTagsData*         pHALTagsData;                 ///< HAL framework tag
    ISPStripeConfig*        pStripeConfig;                ///< Points to ISP input configuration
    ISPInternalData*        pCalculatedData;              ///< Data Calculated by IQ Modules
    IFEPipelineData         pipelineIFEData;              ///< IFE pipeline settings
    IFEPipelinePath         modulePath;                   ///< IFE pipeline path for module
    IFEDualPDPipelineData   dualPDPipelineData;           ///< DualPD pipeline data value
    IPEPipelineData         pipelineIPEData;              ///< Data Needed by IPE IQ Modules
    BPSPipelineData         pipelineBPSData;              ///< Data Needed by BPS IQ Modules
    CVPPipelineData         pipelineCVPData;              ///< Data needed by CVP IQ modules
    TuningDataManager*      pTuningDataManager;           ///< Tuning data manager
    HwContext*              pHwContext;                   ///< Pointer to the Hardware Context
    UINT64                  frameNum;                     ///< the unique frame number for this capture request
    UINT                    mfFrameNum;                   ///< frame number for MFNR/MFSR blending
    VOID*                   pOEMIQSetting;                ///< Pointer to the OEM IQ Interpolation Data
    VOID*                   pLibInitialData;              ///< Customtized Library initial data block
    IFEStripingInput*       pStripingInput;               ///< Striping library's input
    IFETuningMetadata*      pIFETuningMetadata;           ///< Metadata for IFE tuning support
    BPSTuningMetadata*      pBPSTuningMetadata;           ///< Metadata for BPS tuning support
    IPETuningMetadata*      pIPETuningMetadata;           ///< Metadata for IPE tuning support
    ChiTuningModeParameter* pTuningData;                  ///< pointer to tuning data selectors
    BOOL                    tuningModeChanged;            ///< TRUE: tuning mode parameter(s) changed; FALSE: Otherwise
    const EEPROMOTPData*    pOTPData;                     ///< OTP Calibration Data
    UINT16                  numberOfLED;                  ///< Number of LED
    ISPIQTriggerData        triggerData;                  ///< Interpolation library Trigger Data
    BOOL                    disableManual3ACCM;           ///< Override to disable manual 3A CCM
    FLOAT                   LEDFirstEntryRatio;           ///< First Entry Ratio for Dual LED
    FLOAT                   lensPosition;                 ///< lens position value
    FLOAT                   lensZoom;                     ///< lens zoom value
    FLOAT                   postScaleRatio;               ///< post scale ratio
    FLOAT                   preScaleRatio;                ///< pre Scale Ratio
    ICAConfigParameters     ICAConfigData;                ///< ICA Inputdata
    UINT64                  parentNodeID;                 ///< whether it comes from IFE or BPS
    UINT                    opticalCenterX;               ///< Optical center X
    UINT                    opticalCenterY;               ///< Optical center Y
    UINT64                  minRequiredSingleIFEClock;    ///< Minimum IFE clock needed to process current mode
    UINT32                  sensorBitWidth;               ///< Sensor bit width
    UINT32                  resourcePolicy;               ///< Prefered resource vs power trade-off
    ISPHVXConfigureData     HVXData;                      ///< HVX configuration data
    struct FDData           fDData;                       ///< FD Data
    BOOL                    isDualcamera;                 ///< Dualcamera check
    BOOL                    skipTintlessProcessing;       ///< Skip Tintless Algo Processing
    BOOL                    useHardcodedRegisterValues;   ///< TRUE: Use hardcoded reg. values; FALSE: Use calculate settings
    BOOL                    enableIFEDualStripeLog;       ///< TRUE: Enable Dual IFE Stripe In/Out info
    UINT32                  dumpRegConfig;                ///< Dump IQ
    UINT32                  regOffsetIndex;               ///< index of register offset
    UINT32                  requestQueueDepth;            ///< Depth of the request queue
    VOID*                   pBetDMIAddr;                  ///< BET DNI Debug address
    BOOL                    registerBETEn;                ///< BET enabled
    BOOL                    MFNRBET;                      ///< BET enabled
    UINT32                  forceIFEflag;                 ///< Flag to force IFE to single IFE
    UINT32                  icaVersion;                   ///< ica version
    const IFEOutputPath*    pIFEOutputPathInfo;           ///< Pointer to IFEOutputPath data of array in IFE node
    UINT32                  maxOutputWidthFD;             ///< Max Supported FD Output Width
    BOOL                    isHDR10On;                    ///< Flag for HDR10
    UINT32                  titanVersion;                 ///< titan version
    UINT32                  bitWidth;                     ///< ISP output bit width based on ISP output format
    UINT                    maximumPipelineDelay;         ///< Maximum Pipeline delay
    BOOL                    forceTriggerUpdate;           ///< Force Trigger Update
    CmdBufferManager**      ppCmdBufferManager;           ///< Pointer to the pointer of Ommand Buffer Manager
    BOOL                    isPrepareStripeInputContext;  ///< Flag to indicate prepare striping input context call
    IFEStatsTapOut          statsTapOut;                  ///< Stats tap-out information
    CSIDBinningInfo         csidBinningInfo;              ///< CSID binning information
    BOOL                    RDIOnlyCase;                  ///< RDI only Use cases
    BOOL                    isInitPacket;                 ///< TRUE if Init Packet Settings
    BOOL                    isPipelineWarmedUpState;      ///< Flag to indicate if the pipeline is in warmed-up state or not
    SeamlessInSensorState   seamlessInSensorState;        ///< Seamless in-sensor control state
    IFEStripeInterface*     pIFEStripeInterface;          ///< IFE striping interface
    ISPCalculatedMetadata*  pCalculatedMetadata;          ///< Data Calculated by IQ Modules
    IFEDMIBankUpdate        bankUpdate;                   ///< IFE DMI Bank Update
    IFEInstanceProperty     IFENodeInstanceProperty;      ///< varaible to hold the node instance properties
    BOOL*                   pFrameBased;                  ///< Update the WM config type
    StreamDimension*        pRDIStreams;                  ///< RDI Stream dimensions
    CropWindow              originalHALCrop;              ///< Original HAL crop request
    UINT                    IFEDynamicEnableMask;         ///< IFE Dynamic Enable Mask
    UINT32                  IFEPixelRawPort;              ///< IFE raw pixel port that is enabled
};

/// @brief IQ Library specific data
struct IQLibInitialData
{
    BOOL                    isSucceed;      ///< Indicator if the initialization is succeed
    VOID*                   pLibData;       ///< Pointer to the library data
    ChiTuningModeParameter* pTuningData;    ///< pointer to tuning data selectors
};

/// @brief IFEModuleCreateData
struct IFEModuleCreateData
{
    ISPIQModule*    pModule;               ///< Pointer to the created IQ Module
    IFEPipelineData pipelineData;          ///< VFE pipeline setting
    ISPInputData    initializationData;    ///< default values to configure the modules
    IFEHVXInfo*     pHvxInitializeData;    ///< Values from CHI about algo information
    UINT32          titanVersion;          ///< Titan Version
};

/// @brief IPE IQ Module HW Version
struct IPEIQHWVersion
{
    UINT32 hwICAVersion;               ///< ICA Module Version
    UINT32 hwDLUTVersion;              ///< DLUT Module Version
    UINT32 hwANRVersion;               ///< ANR Module Version
    UINT32 hwTFVersion;                ///< TF Module Version
    UINT32 hwCACVersion;               ///< CAC Module Version
    UINT32 hwCSTVersion;               ///< CST Module Version
    UINT32 hwLTMVersion;               ///< LTM Module Version
    UINT32 hwColorCorrectionVersion;   ///< Color Correction Module Version
    UINT32 hwGammaVersion;             ///< Gamma Module Version
    UINT32 hwChromaEnhancementVersion; ///< ChromaEnhancement Module Version
    UINT32 hwChromaSuppressionVersion; ///< ChromasSuppression Module Version
    UINT32 hwSCEVersion;               ///< SCE Module Version
    UINT32 hwASFVersion;               ///< ASF Module Version
    UINT32 hwUpscalerVersion;          ///< Up Scaler Module Version
    UINT32 hwGrainAdderVersion;        ///< Grain Addr Version
    UINT32 hwHNRVersion;               ///< IPE HNR HW Version
    UINT32 hwLENRVersion;              ///< IPE LENR HW Version
};

/// @brief IPEModuleCreateData
struct IPEModuleCreateData
{
    ISPIQModule*    pModule;            ///< Pointer to the created IQ Module
    ISPInputData    initializationData; ///< Default values to configure the modules
    IPEPath         path;               ///< IPE path information
    const CHAR*     pNodeIdentifier;    ///< String identifier for the Node that creating this IQ Module object
    IPEIQHWVersion  moduleVersion;      ///< ISP IQ module version
    UINT32          titanVersion;       ///< Titan Version
};

/// @brief BPSModuleCreateData
struct BPSModuleCreateData
{
    ISPIQModule*    pModule;            ///< Pointer to the created IQ Module
    ISPInputData    initializationData; ///< Default values to configure the modules
    const CHAR*     pNodeIdentifier;    ///< String identifier for the Node that creating this IQ Module object
    UINT32          titanVersion;       ///< Titan Version
};

struct IPEIQModuleData
{
    INT  IPEPath;                    ///< IPE Path information
    UINT offsetPass[PASS_NAME_MAX];  ///< Offset where pass information starts for multipass used by ANR
    UINT singlePassCmdLength;        ///< The length of the Command List, in bytes used by ANR
};

struct IQModuleCmdBufferParam
{
    CmdBufferManagerParam*  pCmdBufManagerParam;    ///< Pointer to the Cmd Buffer Manager params to be filled by IQ Modules
    UINT32                  numberOfCmdBufManagers; ///< Number Of Command Buffers created to be filled by IQ modules
};

/// @brief IFE default module configuration
struct IFEDefaultModuleConfig
{
    UINT32 RSStatsHorizRegions; ///< RS stats Max horisontal region
    UINT32 RSStatsVertRegions;  ///< RS stats Max vertical region
    UINT32 CSStatsHorizRegions; ///< CS stats Max horizontal region
    UINT32 CSStatsVertRegions;  ///< CS stats Max horizontal region
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Base Class for all the ISP IQModule
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ISPIQModule
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Execute
    ///
    /// @brief  Execute process capture request to configure individual image quality module
    ///
    /// @param  pInputData Pointer to the Inputdata
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult Execute(
        ISPInputData* pInputData) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareStripingParameters
    ///
    /// @brief  Prepare striping parameters for striping lib
    ///
    /// @param  pInputData Pointer to the Inputdata
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PrepareStripingParameters(
        ISPInputData* pInputData)
    {
        CAMX_UNREFERENCED_PARAM(pInputData);
        return CamxResultSuccess;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillCmdBufferManagerParams
    ///
    /// @brief  Fills the command buffer manager params needed by IQ Module
    ///
    /// @param  pInputData Pointer to the IQ Module Input data structure
    /// @param  pParam     Pointer to the structure containing the command buffer manager param to be filled by IQ Module
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult FillCmdBufferManagerParams(
       const ISPInputData*     pInputData,
       IQModuleCmdBufferParam* pParam)
    {
        CAMX_UNREFERENCED_PARAM(pInputData);
        CAMX_UNREFERENCED_PARAM(pParam);
        return CamxResultSuccess;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRegCmd
    ///
    /// @brief  Retrieve the buffer of the register value
    ///
    /// @return Pointer of the register buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID* GetRegCmd()
    {
        return NULL;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDualIFEData
    ///
    /// @brief  Provides information on how dual IFE mode affects the IQ module
    ///
    /// @param  pDualIFEData Pointer to dual IFE data the module will fill in
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID GetDualIFEData(
        IQModuleDualIFEData* pDualIFEData)
    {
        CAMX_ASSERT(NULL != pDualIFEData);

        pDualIFEData->dualIFESensitive      = FALSE;
        pDualIFEData->dualIFEDMI32Sensitive = FALSE;
        pDualIFEData->dualIFEDMI64Sensitive = FALSE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIQCmdLength
    ///
    /// @brief  Get the Command Size
    ///
    /// @return Length of the command list
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetIQCmdLength()
    {
        return m_cmdLength;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Get32bitDMILength
    ///
    /// @brief  Get the 32 bit DMI Size
    ///
    /// @return Length of the DMI list
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 Get32bitDMILength()
    {
        return m_32bitDMILength;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Set32bitDMIBufferOffset
    ///
    /// @brief  Set Offset in 32bit DMI buffer
    ///
    /// @param  offset The 64-bit DMI buffer offset.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID Set32bitDMIBufferOffset(
        UINT offset)
    {
        m_32bitDMIBufferOffsetDword = offset;

        return;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Get64bitDMILength
    ///
    /// @brief  Get the 64 bit DMI Size
    ///
    /// @return Length of the DMI list
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 Get64bitDMILength()
    {
        return m_64bitDMILength;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Set64bitDMIBufferOffset
    ///
    /// @brief  Set Offset in 64bit DMI buffer
    ///
    /// @param  offset The 64-bit DMI buffer offset.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID Set64bitDMIBufferOffset(
        UINT offset)
    {
        m_64bitDMIBufferOffsetDword = offset;

        return;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetVersion
    ///
    /// @brief  Get the HW version
    ///
    /// @return HW version
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE virtual UINT32 GetVersion()
    {
        return 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumLUT
    ///
    /// @brief  Get the number of Look Up Tables in IQ block
    ///
    /// @return Number of tables
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetNumLUT()
    {
        return m_numLUT;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLUTOffset
    ///
    /// @brief  Get the offset where DMI CDM command for IQ module is present within DMI header cmd buffer
    ///
    /// @return Offset of LUT DMI header
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetLUTOffset()
    {
        return m_offsetLUT;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIQType
    ///
    /// @brief  Get the Type of this Module
    ///
    /// @return module type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE ISPIQModuleType GetIQType()
    {
        return m_type;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsDumpRegConfig
    ///
    /// @brief  Cheking if IQ dump register configure is enable
    ///
    /// @param  dumpRegConfigMask       dump mask
    /// @param  offsetOfIQModuleIndex   index begin of IQ modules
    ///
    /// @return BOOL    TRUE: Dump is enable, FALSE: Otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsDumpRegConfig(
        UINT32 dumpRegConfigMask,
        UINT32 offsetOfIQModuleIndex)
    {
        return (0 != (dumpRegConfigMask & (1 << (static_cast<UINT32>(m_type) - offsetOfIQModuleIndex))));
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetModuleData
    ///
    /// @brief  Get IQ module specific data
    ///
    /// @param  pModuleData    Pointer pointing to Module specific data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CAMX_INLINE VOID GetModuleData(
        VOID* pModuleData)
    {
        CAMX_UNREFERENCED_PARAM(pModuleData);
        return;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destroy the object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID Destroy()
    {
        CAMX_DELETE this;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsTuningModeDataChanged
    ///
    /// @brief  Determine if there is/are change(s) between the current and previously sets fo tuning mode/selector data
    ///
    /// @param  pCurrTuningModeParams   Pointer to current  tuning mode data
    /// @param  pPrevTuningModeParams   Pointer to previous tuning mode data
    ///
    /// @return BOOL    TRUE: Changed, FALSE: Otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE BOOL IsTuningModeDataChanged(
        const ChiTuningModeParameter* pCurrTuningModeParams,
        const ChiTuningModeParameter* pPrevTuningModeParams)
    {
        CAMX_STATIC_ASSERT_MESSAGE((static_cast<UINT32>(ChiModeType::Effect) == (MaxTuningMode - 1)),
            "Tuning Mode Structured Changed");

        BOOL tuningModeChanged = FALSE;

        if (NULL != pCurrTuningModeParams)
        {
            if (NULL != pPrevTuningModeParams)
            {
                tuningModeChanged = (0 != Utils::Memcmp(pCurrTuningModeParams,
                                                        pPrevTuningModeParams,
                                                        sizeof(ChiTuningModeParameter)));
            }
            // Always treat the initial mode as changed
            else
            {
                tuningModeChanged = TRUE;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid pointer to current tuning mode parameters (%p)", pCurrTuningModeParams);
        }

        return tuningModeChanged;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpTuningModeData
    ///
    /// @brief  Dump tuning data
    ///
    /// @param  pTuningData         Pointer to tuning mode data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE VOID DumpTuningModeData(
        const ChiTuningModeParameter* pTuningData)
    {
        if (NULL != pTuningData)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                             "Default %d, Sensor %d, Usecase %d, Feature1 %d, Feature2 %d, Scene %d, Effect %d",
                             pTuningData->TuningMode[0].subMode.value,
                             pTuningData->TuningMode[1].subMode.value,
                             pTuningData->TuningMode[2].subMode.usecase,
                             pTuningData->TuningMode[3].subMode.feature1,
                             pTuningData->TuningMode[4].subMode.feature2,
                             pTuningData->TuningMode[5].subMode.scene,
                             pTuningData->TuningMode[6].subMode.effect);
        }
    }
protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ISPIQModule
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ISPIQModule() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ISPIQModule
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ISPIQModule() = default;

    ISPHWSetting*   m_pHWSetting;                   ///< Pointer to the HW Setting Class
    ISPIQModuleType m_type;                         ///< IQ Module Type
    BOOL            m_dynamicEnable;                ///< Flag to indicate if this module is enabled dynamically
    BOOL            m_moduleEnable;                 ///< Flag to indicate if this module is enabled
    BOOL            m_dsBPCEnable;                  ///< Flag to indicate if DSBPC module is enabled
    UINT            m_cmdLength;                    ///< The length of the Command List, in dwords
    UINT            m_32bitDMILength;               ///< The length of the 32 bit DMI Table, in dwords
    UINT            m_32bitDMIBufferOffsetDword;    ///< Offset to the 32bit DMI buffer, in Dword
    UINT            m_64bitDMILength;               ///< The length of the 64 bit DMI Table, in Dword
    UINT            m_64bitDMIBufferOffsetDword;    ///< Offset to the 64bit DMI buffer, in Dword
    UINT            m_numLUT;                       ///< The number of look up tables
    UINT            m_offsetLUT;                    ///< Offset where DMI header starts for LUTs

private:
    ISPIQModule(const ISPIQModule&)            = delete;   ///< Disallow the copy constructor
    ISPIQModule& operator=(const ISPIQModule&) = delete;   ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXISPIQMODULE_H
