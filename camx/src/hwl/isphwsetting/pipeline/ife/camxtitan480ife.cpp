
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxtitan480ife.cpp
/// @brief IFE Node class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifeabf40.h"
#include "camxifeawbbgstats14.h"
#include "camxifebfstats25.h"
#include "camxifebhiststats14.h"
#include "camxifebls12.h"
#include "camxifepdpc30.h"
#include "camxifecamif.h"
#include "camxifecamiflite.h"
#include "camxifecc13.h"
#include "camxifecrop11.h"
#include "camxifecsid.h"
#include "camxifecsstats14.h"
#include "camxifecsstats14titan480.h"
#include "camxifecst12.h"
#include "camxifedemosaic36.h"
#include "camxifedemux13.h"
#include "camxifeds411.h"
// #include "camxifedualpd20.h"
#include "camxifegamma16.h"
#include "camxifegtm10.h"
#include "camxifehdr30.h"
#include "camxifehdrbestats15.h"
#include "camxifehdrbhiststats13.h"
#include "camxifehvx.h"
#include "camxifeihiststats12.h"
#include "camxifelcr10.h"
#include "camxifelinearization34.h"
// #include "camxifelsc40.h"
#include "camxifemnds21.h"
#include "camxifepedestal13.h"
#include "camxifepdaf20.h"
#include "camxiferoundclamp11.h"
#include "camxifersstats14.h"
#include "camxispstatsmodule.h"
#include "camxifetintlessbgstats15.h"
#include "camxifewb13.h"
#include "camxisppipeline.h"
#include "camxtitan480ife.h"
#include "camxswtmc11.h"
#include "camxswtmc12.h"
#include "camxisppipeline.h"
#include "camxcslispdefs.h"
#include "camxcslifedefs.h"

CAMX_NAMESPACE_BEGIN

IFEIQModuleInfo IFEIQModuleItems4[] =
{
    // SW modules
    {ISPIQModuleType::SWTMC,                    IFEPipelinePath::CommonPath,        TRUE,   SWTMC12::CreateIFE},
    {ISPIQModuleType::IFEHVX,                   IFEPipelinePath::CommonPath,        TRUE,   IFEHVX::Create },

    // Image processing modules
    {ISPIQModuleType::IFECAMIF,                 IFEPipelinePath::CommonPath,        TRUE,   IFECAMIF::Create},
    {ISPIQModuleType::IFEPedestalCorrection,    IFEPipelinePath::CommonPath,        FALSE,  IFEPedestal13::Create},
    {ISPIQModuleType::IFEABF,                   IFEPipelinePath::CommonPath,        TRUE,   IFEABF40::Create},
    {ISPIQModuleType::IFELinearization,         IFEPipelinePath::CommonPath,        TRUE,   IFELinearization34::Create},
    {ISPIQModuleType::IFEDemux,                 IFEPipelinePath::CommonPath,        TRUE,   IFEDemux13::Create},
    {ISPIQModuleType::IFEPDPC,                  IFEPipelinePath::CommonPath,        TRUE,   IFEPDPC30::Create},
    {ISPIQModuleType::IFEHDR,                   IFEPipelinePath::CommonPath,        TRUE,   IFEHDR30::Create},
    {ISPIQModuleType::IFELSC,                   IFEPipelinePath::CommonPath,        TRUE,   IFELSC40::Create},
    {ISPIQModuleType::IFEWB,                    IFEPipelinePath::CommonPath,        TRUE,   IFEWB13::Create},
    {ISPIQModuleType::IFEDemosaic,              IFEPipelinePath::CommonPath,        TRUE,   IFEDemosaic36::Create},
    // {ISPIQModuleType::IFEDemux,                 IFEPipelinePath::CommonPath,        TRUE,   IFEDemux10::Create},
    // {ISPIQModuleType::IFEDemux,                 IFEPipelinePath::CommonPath,        TRUE,   IFEChromaUp22::Create},
    {ISPIQModuleType::IFECC,                    IFEPipelinePath::CommonPath,        TRUE,   IFECC13::Create},
    {ISPIQModuleType::IFEGTM,                   IFEPipelinePath::CommonPath,        TRUE,   IFEGTM10::Create},
    {ISPIQModuleType::IFEGamma,                 IFEPipelinePath::CommonPath,        TRUE,   IFEGamma16::Create},
    {ISPIQModuleType::IFECST,                   IFEPipelinePath::CommonPath,        TRUE,   IFECST12::Create},

    // Full path modules
    {ISPIQModuleType::IFEMNDS,                  IFEPipelinePath::VideoFullPath,     TRUE,   IFEMNDS21::Create},
    {ISPIQModuleType::IFECrop,                  IFEPipelinePath::VideoFullPath,     TRUE,   IFECrop11::Create},
    {ISPIQModuleType::IFERoundClamp,            IFEPipelinePath::VideoFullPath,     TRUE,   IFERoundClamp11::Create},
    {ISPIQModuleType::IFEDS4  ,                 IFEPipelinePath::VideoDS4Path,      TRUE,   IFEDSX10::Create},
    {ISPIQModuleType::IFECrop,                  IFEPipelinePath::VideoDS4Path,      TRUE,   IFECrop11::Create},
    {ISPIQModuleType::IFERoundClamp,            IFEPipelinePath::VideoDS4Path,      TRUE,   IFERoundClamp11::Create},
    {ISPIQModuleType::IFEDS4  ,                 IFEPipelinePath::VideoDS16Path,     TRUE,   IFEDS411::Create},
    {ISPIQModuleType::IFECrop,                  IFEPipelinePath::VideoDS16Path,     TRUE,   IFECrop11::Create},
    {ISPIQModuleType::IFERoundClamp,            IFEPipelinePath::VideoDS16Path,     TRUE,   IFERoundClamp11::Create},

    // Display path Modules
    {ISPIQModuleType::IFEMNDS,                  IFEPipelinePath::DisplayFullPath,   TRUE,   IFEMNDS21::Create},
    {ISPIQModuleType::IFECrop,                  IFEPipelinePath::DisplayFullPath,   TRUE,   IFECrop11::Create},
    {ISPIQModuleType::IFERoundClamp,            IFEPipelinePath::DisplayFullPath,   TRUE,   IFERoundClamp11::Create},
    {ISPIQModuleType::IFEDS4  ,                 IFEPipelinePath::DisplayDS4Path,    TRUE,   IFEDS411::Create},
    {ISPIQModuleType::IFECrop,                  IFEPipelinePath::DisplayDS4Path,    TRUE,   IFECrop11::Create},
    {ISPIQModuleType::IFERoundClamp,            IFEPipelinePath::DisplayDS4Path,    TRUE,   IFERoundClamp11::Create},
    {ISPIQModuleType::IFEDS4  ,                 IFEPipelinePath::DisplayDS16Path,   TRUE,   IFEDS411::Create},
    {ISPIQModuleType::IFECrop,                  IFEPipelinePath::DisplayDS16Path,   TRUE,   IFECrop11::Create},
    {ISPIQModuleType::IFERoundClamp,            IFEPipelinePath::DisplayDS16Path,   TRUE,   IFERoundClamp11::Create},

    // FD path modules
    {ISPIQModuleType::IFEMNDS,                  IFEPipelinePath::FDPath,            TRUE,   IFEMNDS21::Create},
    {ISPIQModuleType::IFECrop,                  IFEPipelinePath::FDPath,            TRUE,   IFECrop11::Create},
    {ISPIQModuleType::IFERoundClamp,            IFEPipelinePath::FDPath,            TRUE,   IFERoundClamp11::Create},

    // Pixel Raw crop module
    {ISPIQModuleType::IFECrop,                  IFEPipelinePath::PixelRawDumpPath,  TRUE,   IFECrop11::Create},
    {ISPIQModuleType::IFERoundClamp,            IFEPipelinePath::PixelRawDumpPath,  TRUE,   IFERoundClamp11::Create},

    // PD processing modules
    {ISPIQModuleType::IFECAMIFDualPD,             IFEPipelinePath::DualPDPath,      TRUE,   IFECAMIF::Create},
    {ISPIQModuleType::IFEDUALPD,                  IFEPipelinePath::DualPDPath,      TRUE,   IFEPDAF20::Create},

    // LCR processing modules
    {ISPIQModuleType::IFELCR,                   IFEPipelinePath::LCRPath,           TRUE,   IFELCR10::Create},
    {ISPIQModuleType::IFECAMIFLCR,              IFEPipelinePath::LCRPath,           TRUE,   IFECAMIF::Create},

    {ISPIQModuleType::IFECSID,                  IFEPipelinePath::RDI0Path,          TRUE,   IFECSID::Create},
    {ISPIQModuleType::IFECSID,                  IFEPipelinePath::RDI1Path,          TRUE,   IFECSID::Create},
    {ISPIQModuleType::IFECSID,                  IFEPipelinePath::RDI2Path,          TRUE,   IFECSID::Create},
    // RDI processing modules
    {ISPIQModuleType::IFECAMIFRDI0,               IFEPipelinePath::RDI0Path,          TRUE,    IFECAMIF::Create},
    {ISPIQModuleType::IFECAMIFRDI1,               IFEPipelinePath::RDI1Path,          TRUE,    IFECAMIF::Create},
    {ISPIQModuleType::IFECAMIFRDI2,               IFEPipelinePath::RDI2Path,          TRUE,    IFECAMIF::Create},
    {ISPIQModuleType::IFECAMIFRDI3,               IFEPipelinePath::RDI3Path,          TRUE,    IFECAMIF::Create},
};


static IFEStatsModuleInfo IFEStatsModuleItems4[] =
{
    {ISPStatsModuleType::IFERS,          TRUE, RSStats14::Create},
    {ISPStatsModuleType::IFECS,          TRUE, CSStats14::Create},
    {ISPStatsModuleType::IFEIHist,       TRUE, IHistStats12::Create},
    {ISPStatsModuleType::IFEBHist,       TRUE, BHistStats14::Create},
    {ISPStatsModuleType::IFEHDRBE,       TRUE, HDRBEStats15::Create},
    {ISPStatsModuleType::IFEHDRBHist,    TRUE, HDRBHistStats13::Create},
    {ISPStatsModuleType::IFETintlessBG,  TRUE, TintlessBGStats15::Create},
    {ISPStatsModuleType::IFEBF,          TRUE, BFStats25::Create},
    {ISPStatsModuleType::IFEAWBBG,       TRUE, AWBBGStats14::Create}
};

enum IFERegGroupTypes
{
    IFETop = 0,     ///< IFETop
    ChromaUp,       ///< ChromaUp
    Pedestal,       ///< Pedestal
    Linearization,  ///< Linearization
    BPC,            ///< BPC
    HDR,            ///< HDR
    ABF,            ///< ABF
    LSC,            ///< LSC
    ColorCorrect,   ///< ColorCorrect
    GTMDMI,         ///< GTMDMI
    Gamma,          ///< Gamma
    Stats,          ///< Stats
    VidPath,        ///< VidDS16, VidDS4, VidFull paths
    DispPath,       ///< DispDS16, DispDS4, DispFull paths
    FD,             ///< FD
    PixelRaw,       ///< PixelRaw
    PDAF,           ///< PDAF
    PDOUT,          ///< PDOUT
    LCR,            ///< LCR
    Common          ///< Common
};

struct RegDumpInfo
{
    IFERegGroupTypes regGroupType;  ///< Register Group Type
    IFERegReadInfo   regReadInfo;   ///< Register Read Info
};

static RegDumpInfo   IFERegDump[] =
{
    // IFE TOP CFG
    {
        IFETop,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_TOP_HW_VERSION,
                    128,
                }
            },
        }
    },
    // CSID
    {
        IFETop,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_CSID_HW_VERSION,
                    23,
                }
            },
        }
    },
    // CSID RX
    {
        IFETop,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_CSID_CSI2_RX_CFG0,
                    440
                }
            },
        }
    },
    // TESTGEN
    {
        IFETop,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_TESTGEN_MODULE_CFG,
                    12
                }
            },
        }
    },
    // CAMIF
    {
        IFETop,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_CAMIF_MODULE_CFG,
                    9
                }
            },
        }
    },
    // CAMIF DEBUG
    {
        IFETop,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_CAMIF_DEBUG_0,
                    2
                }
            },
        }
    },
    // DEMUX
    {
        IFETop,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DEMUX_MODULE_CFG,
                    4
                }
            },
        }
    },
    // CHROMA UP SAMPLE
    {
        ChromaUp,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_CHROMA_UP_MODULE_CFG,
                    6
                }
            },
        }
    },
    // PEDESTAL
    {
        Pedestal,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_PEDESTAL_DMI_LUT_BANK_CFG,
                    9
                }
            },
        }
    },
    // LINEARIZATION
    {
        Linearization,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_LINEARIZATION_DMI_LUT_BANK_CFG,
                    20
                }
            },
        }
    },
    // BPC PDPC
    {
        BPC,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_BPC_PDPC_DMI_LUT_BANK_CFG,
                    32
                }
            },
        }
    },
    // HDR BIN CORRECT
    {
        HDR,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_HDR_BINCORRECT_MODULE_CFG,
                    21
                }
            },
        }
    },
    // ABF
    {
        ABF,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_ABF_DMI_LUT_BANK_CFG,
                    48
                }
            },
        }
    },
    // LSC
    {
        LSC,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_LSC_DMI_LUT_BANK_CFG,
                    15
                }
            },
        }
    },
    // DEMOSAIC
    {
        IFETop,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DEMOSAIC_MODULE_CFG,
                    8
                }
            },
        }
    },
    // COLOR CORRECT
    {
        ColorCorrect,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_COLOR_CORRECT_MODULE_CFG,
                    11
                }
            },
        }
    },
    // GTM
    {
        GTMDMI,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_GTM_DMI_LUT_BANK_CFG,
                    3
                }
            },
        }
    },
    // GAMMA
    {
        Gamma,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_GLUT_DMI_LUT_BANK_CFG,
                    3
                }
            },
        }
    },
    // COLOR XFROM
    {
        IFETop,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_COLOR_XFORM_MODULE_CFG,
                    14
                }
            },
        }
    },
    // COLOR XFROM
    {
        IFETop,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_COLOR_XFORM_MODULE_CFG,
                    14
                }
            },
        }
    },
    // CRC PIXEL RAW
    {
        PixelRaw,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_PIXEL_RAW_OUT_MODULE_CFG,
                    10
                }
            },
        }
    },
    // MN Y FD
    {
        FD,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_FD_OUT_MODULE_CFG,
                    9
                }
            },
        }
    },
    // MN C FD
    {
        FD,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_FD_OUT_MODULE_CFG,
                    9
                }
            },
        }
    },
    // CRC FD Y
    {
        FD,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_FD_OUT_MODULE_CFG,
                    10
                }
            },
        }
    },
    // CRC FD C
    {
        FD,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_FD_OUT_MODULE_CFG,
                    10
                }
            },
        }
    },
    // MN Y DISP
    {
        DispPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_DISP_OUT_MODULE_CFG,
                    9
                }
            },
        }
    },
    // MN C DISP
    {
        DispPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_DISP_OUT_MODULE_CFG,
                    9
                }
            },
        }
    },
    // CRC DISP Y
    {
        DispPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_DISP_OUT_MODULE_CFG,
                    10
                }
            },
        }
    },
    // CRC DISP C
    {
        DispPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_DISP_OUT_MODULE_CFG,
                    10
                }
            },
        }
    },
    // MN Y DS4 DISP
    {
        DispPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_DISP_DS4_OUT_MODULE_CFG,
                    2
                }
            },
        }
    },
    // MN Y DS4 DISP
    {
        DispPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_DISP_DS4_OUT_CROP_LINE_CFG,
                    2
                }
            },
        }
    },
    // MN C DS4 DISP
    {
        DispPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_DISP_DS4_OUT_MODULE_CFG,
                    2
                }
            },
        }
    },
    // MN C DS4 DISP
    {
        DispPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_DISP_DS4_OUT_CROP_LINE_CFG,
                    2
                }
            },
        }
    },
    // CRC DS4 DISP Y
    {
        DispPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS4_OUT_MODULE_CFG,
                    10
                }
            },
        }
    },
    // CRC DS4 DISP C
    {
        DispPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS4_OUT_MODULE_CFG,
                    10
                }
            },
        }
    },
    // MN Y DS16 DISP
    {
        DispPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_DISP_DS16_OUT_MODULE_CFG,
                    2
                }
            },
        }
    },
    // MN Y DS16 DISP
    {
        DispPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_DISP_DS16_OUT_CROP_LINE_CFG,
                    2
                }
            },
        }
    },
    // MN C DS16 DISP
    {
        DispPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_DISP_DS16_OUT_MODULE_CFG,
                    2
                }
            },
        }
    },
    // MN C DS16 DISP
    {
        DispPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_DISP_DS16_OUT_CROP_LINE_CFG,
                    2
                }
            },
        }
    },
    // CRC DS16 DISP Y
    {
        DispPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS16_OUT_MODULE_CFG,
                    10
                }
            },
        }
    },
    // CRC DS16 DISP C
    {
        DispPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS16_OUT_MODULE_CFG,
                    10
                }
            },
        }
    },
    // MN Y VID
    {
        VidPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_VID_OUT_MODULE_CFG,
                    9
                }
            },
        }
    },
    // MN C VID
    {
        VidPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_VID_OUT_MODULE_CFG,
                    9
                }
            },
        }
    },
    // CRC VID Y
    {
        VidPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_VID_OUT_MODULE_CFG,
                    10
                }
            },
        }
    },
    // CRC VID C
    {
        VidPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_VID_OUT_MODULE_CFG,
                    10
                }
            },
        }
    },
    // DSX Y VID
    {
        VidPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_DMI_LUT_BANK_CFG,
                    34
                }
            },
        }
    },
    // DSX Y VID
    {
        VidPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_LUMA_0,
                    24
                }
            },
        }
    },
    // DSX C VID
    {
        VidPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DSX_C_VID_OUT_DMI_LUT_BANK_CFG,
                    18
                }
            },
        }
    },
    // DSX C VID
    {
        VidPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DSX_C_VID_OUT_PADDING_WEIGHTS_LEFT_RGT_CHROMA_0,
                    8
                }
            },
        }
    },
    // CRC VID Y
    {
        VidPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_Y_VID_OUT_MODULE_CFG,
                    10
                }
            },
        }
    },
    // CRC VID C
    {
        VidPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_C_VID_OUT_MODULE_CFG,
                    10
                }
            },
        }
    },
    // MN Y DS16 VID
    {
        VidPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_VID_DS16_OUT_MODULE_CFG,
                    2
                }
            },
        }
    },
    // MN Y DS16 VID
    {
        VidPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_VID_DS16_OUT_CROP_LINE_CFG,
                    2
                }
            },
        }
    },
    // MN C DS16 VID
    {
        VidPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_VID_DS16_OUT_MODULE_CFG,
                    2
                }
            },
        }
    },
    // MN C DS16 VID
    {
        VidPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_VID_DS16_OUT_CROP_LINE_CFG,
                    2
                }
            },
        }
    },
    // CRC DS16 VID Y
    {
        VidPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_VID_DS16_OUT_MODULE_CFG,
                    10
                }
            },
        }
    },
    // CRC DS16 VID C
    {
        VidPath,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_VID_DS16_OUT_MODULE_CFG,
                    10
                }
            },
        }
    },
    // BLS
    {
        IFETop,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_BLS_MODULE_CFG,
                    6
                }
            },
        }
    },
    // TINTLESS
    {
        Stats,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_MODULE_CFG,
                    12
                }
            },
        }
    },
    // HDR BHIST
    {
        Stats,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_STATS_HDR_BHIST_DMI_LUT_BANK_CFG,
                    6
                }
            },
        }
    },
    // HDR BE
    {
        Stats,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_STATS_HDR_BE_MODULE_CFG,
                    11
                }
            },
        }
    },
    // AWB BG
    {
        Stats,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_STATS_AWB_BG_MODULE_CFG,
                    12
                }
            },
        }
    },
    // BHIST
    {
        Stats,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_STATS_BHIST_DMI_LUT_BANK_CFG,
                    6
                }
            },
        }
    },
    // BAF
    {
        Stats,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_STATS_BAF_DMI_LUT_BANK_CFG,
                    40
                }
            },
        }
    },
    // RS
    {
        Stats,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_STATS_RS_MODULE_CFG,
                    5
                }
            },
        }
    },
    // CS
    {
        Stats,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_STATS_CS_MODULE_CFG,
                    5
                }
            },
        }
    },
    // IHIST
    {
        Stats,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PP_CLC_STATS_IHIST_DMI_LUT_BANK_CFG,
                    6
                }
            },
        }
    },
    // RDI0 CAMIF
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_RDI0_CLC_CAMIF_MODULE_CFG,
                    5
                }
            },
        }
    },
    // RDI1 CAMIF
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_RDI1_CLC_CAMIF_MODULE_CFG,
                    5
                }
            },
        }
    },
    // RDI2 CAMIF
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_RDI2_CLC_CAMIF_MODULE_CFG,
                    5
                }
            },
        }
    },
    // LCR CAMIF
    {
        LCR,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_LCR_CLC_CAMIF_MODULE_CFG,
                    5
                }
            },
        }
    },
    // LCR
    {
        LCR,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_LCRMODULE_CFG,
                    10
                }
            },
        }
    },
    // PDLIB CAMIF
    {
        PDAF,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_PDLIB_CLC_CAMIF_MODULE_CFG,
                    5
                }
            },
        }
    },
    // PDLIB
    {
        PDAF,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_PDLIB_DMI_LUT_BANK_CFG,
                    48
                }
            },
        }
    },
    // BUS RD
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_RD_HW_VERSION,
                    40
                }
            },
        }
    },
    // BUS WR
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_HW_VERSION,
                    51
                }
            },
        }
    },
    // BUS WM 0
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_0_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 1
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_1_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 2
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_2_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 3
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_3_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 4
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_4_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 5
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_5_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 6
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_6_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 7
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_7_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 8
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_8_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 9
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_9_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 10
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_10_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 11
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_11_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 12
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_12_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 13
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_13_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 14
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_14_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 15
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_15_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 16
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_16_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 17
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_17_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 18
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_18_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 19
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_19_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 20
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_20_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 21
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_21_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 22
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_22_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 23
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_23_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 24
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_24_CFG,
                    33
                }
            },
        }
    },
    // BUS WM 25
    {
        Common,
        {
            IFERegDumpReadTypeReg,
            0,
            {
                {
                    regIFE_IFE_0_CLC_BUS_WR_CLIENT_25_CFG,
                    33
                }
            },
        }
    },
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan480IFE::FetchPipelineCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan480IFE::FetchPipelineCapability(
    VOID* pPipelineCapability)
{
    IFECapabilityInfo* pIFECapability = static_cast<IFECapabilityInfo*>(pPipelineCapability);

    pIFECapability->numIFEIQModule           = sizeof(IFEIQModuleItems4) / sizeof(IFEIQModuleInfo);
    pIFECapability->pIFEIQModuleList         = &IFEIQModuleItems4[0];
    pIFECapability->numIFEStatsModule        = sizeof(IFEStatsModuleItems4) / sizeof(IFEStatsModuleInfo);
    pIFECapability->pIFEStatsModuleList      = &IFEStatsModuleItems4[0];
    pIFECapability->UBWCSupportedVersionMask = UBWCVersion2Mask | UBWCVersion3Mask | UBWCVersion4Mask;
    pIFECapability->UBWCLossySupport         = UBWCLossy;
    pIFECapability->lossy10bitWidth          = 1280;
    pIFECapability->lossy10bitHeight         = 720;
    pIFECapability->lossy8bitWidth           = 3840;
    pIFECapability->lossy8bitHeight          = 2160;
    pIFECapability->ICAVersion               = ICAVersion30;
    pIFECapability->LDCSupport               = TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan480IFE::ProgramIQModuleEnableConfig()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan480IFE::ProgramIQModuleEnableConfig(
    ISPInputData*       pInputData,
    ISPInternalData*    pISPData,
    IFEOutputPath*      pIFEOutputPathInfo)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pInputData);
    CAMX_UNREFERENCED_PARAM(pISPData);
    CAMX_UNREFERENCED_PARAM(pIFEOutputPathInfo);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan480IFE::GetISPIQModulesOfType()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan480IFE::GetISPIQModulesOfType(
    ISPIQModuleType moduleType,
    VOID*           pModuleInfo)
{
    UINT            numOfIQModule = sizeof(IFEIQModuleItems4) / sizeof(IFEIQModuleInfo);
    IFEIQModuleInfo* pIFEmoduleInfo = static_cast<IFEIQModuleInfo*>(pModuleInfo);

    for (UINT i = 0; i < numOfIQModule; i++)
    {
        if (IFEIQModuleItems4[i].moduleType == moduleType)
        {
            Utils::Memcpy(pIFEmoduleInfo, &IFEIQModuleItems4[i], sizeof(IFEIQModuleInfo));
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan480IFE::FillConfigTuningMetadata()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan480IFE::FillConfigTuningMetadata(
    ISPInputData* pInputData)
{
    CAMX_UNREFERENCED_PARAM(pInputData);
    // Pending to add enable config data
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan480IFE::DumpTuningMetadata()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan480IFE::DumpTuningMetadata(
    ISPInputData*       pInputData,
    DebugDataWriter*    pDebugDataWriter)
{
    CamxResult              result          = CamxResultSuccess;
    IFETuningMetadata480*   pMetadata480    = &pInputData->pIFETuningMetadata->metadata480;

    FillConfigTuningMetadata(pInputData);

    if (TRUE == pInputData->pCalculatedData->moduleEnable.IQModules.rolloffEnable)
    {
        result = pDebugDataWriter->AddDataTag(DebugDataTagID::TuningIFELSC40Register,
                                              DebugDataTagType::TuningIFELSC40Config,
                                              1,
                                              &pMetadata480->IFELSCData,
                                              sizeof(pMetadata480->IFELSCData));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupISP, "AddDataTag failed with error: %d", result);
            result = CamxResultSuccess; // Non-fatal error
        }

        result = pDebugDataWriter->AddDataTag(DebugDataTagID::TuningIFELSC40PackedMesh,
                                              DebugDataTagType::TuningLSC40MeshLUT,
                                              1,
                                              &pMetadata480->IFEDMIPacked.LSCMesh,
                                              sizeof(pMetadata480->IFEDMIPacked.LSCMesh));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupISP, "AddDataTag failed with error: %d", result);
            result = CamxResultSuccess; // Non-fatal error
        }
    }

    if (TRUE == pInputData->pCalculatedData->moduleEnable.IQModules.gammaEnable)
    {
        result = pDebugDataWriter->AddDataTag(DebugDataTagID::TuningIFEGamma16Register480,
                                              DebugDataTagType::UInt32,
                                              CAMX_ARRAY_SIZE(pMetadata480->IFEGammaData.gammaConfig),
                                              &pMetadata480->IFEGammaData.gammaConfig,
                                              sizeof(pMetadata480->IFEGammaData.gammaConfig));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupISP, "AddDataTag failed with error: %d", result);
            result = CamxResultSuccess; // Non-fatal error
        }

        result = pDebugDataWriter->AddDataTag(DebugDataTagID::TuningIFEGamma16PackedLUT,
                                              DebugDataTagType::TuningGammaCurve,
                                              CAMX_ARRAY_SIZE(pMetadata480->IFEDMIPacked.gamma),
                                              &pMetadata480->IFEDMIPacked.gamma,
                                              sizeof(pMetadata480->IFEDMIPacked.gamma));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupISP, "AddDataTag failed with error: %d", result);
            result = CamxResultSuccess; // Non-fatal error
        }
    }

    if (TRUE == pInputData->pCalculatedData->moduleEnable.IQModules.GTMEnable)
    {
        result = pDebugDataWriter->AddDataTag(DebugDataTagID::TuningIFEGTM10Register480,
                                              DebugDataTagType::UInt32,
                                              CAMX_ARRAY_SIZE(pMetadata480->IFEGTMData.GTMConfig),
                                              &pMetadata480->IFEGTMData.GTMConfig,
                                              sizeof(pMetadata480->IFEGTMData.GTMConfig));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupISP, "AddDataTag failed with error: %d", result);
            result = CamxResultSuccess; // Non-fatal error
        }

        result = pDebugDataWriter->AddDataTag(DebugDataTagID::TuningIFEGTM10PackedLUT,
                                              DebugDataTagType::TuningGTMLUT,
                                              1,
                                              &pMetadata480->IFEDMIPacked.GTM,
                                              sizeof(pMetadata480->IFEDMIPacked.GTM));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupISP, "AddDataTag failed with error: %d", result);
            result = CamxResultSuccess; // Non-fatal error
        }
    }

    if ((TRUE == pInputData->pCalculatedData->moduleEnable.IQModules.HDREnable))
    {
        result = pDebugDataWriter->AddDataTag(DebugDataTagID::TuningIFEHDR30Register,
                                              DebugDataTagType::TuningIFE30HDR,
                                              1,
                                              &pMetadata480->IFEHDRData,
                                              sizeof(pMetadata480->IFEHDRData));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupISP, "AddDataTag failed with error: %d", result);
            result = CamxResultSuccess; // Non-fatal error
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan480IFE::GetIFEDefaultConfig()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan480IFE::GetIFEDefaultConfig(
    IFEDefaultModuleConfig* pDefaultData)
{
    pDefaultData->RSStatsHorizRegions   = RSStats14MaxHorizRegions;
    pDefaultData->RSStatsVertRegions    = RSStats14MaxVertRegions;
    pDefaultData->CSStatsHorizRegions   = CSStats14Titan480MaxHorizRegions;
    pDefaultData->CSStatsVertRegions    = CSStats14Titan480MaxVertRegions;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan480IFE::GetPDHWCapability()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan480IFE::GetPDHWCapability(
    PDHwAvailablity* pCapabilityInfo,
    IFEProfileId     profileID)
{
    if (NULL != pCapabilityInfo)
    {
        if (IFEProfileIdOffline == profileID)
        {
            pCapabilityInfo->isLCRHwAvailable       = TRUE;
        }
        else
        {
            pCapabilityInfo->isDualPDHwAvailable    = TRUE;
            pCapabilityInfo->isLCRHwAvailable       = TRUE;
            pCapabilityInfo->isSparsePDHwAvailable  = TRUE;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan480IFE::SetupCoreConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan480IFE::SetupCoreConfig(
    CmdBuffer*      pCmdBuffer,
    IFEStatsTapOut* pStatsTapOut,
    VOID*           pCfg)
{
    IFECoreConfig* pCoreCfg = reinterpret_cast<IFECoreConfig*>(pCfg);

    CamxResult result = CamxResultSuccess;

    if ((NULL != pCoreCfg) && (NULL != pCmdBuffer) && (NULL != pStatsTapOut))
    {
        pCoreCfg->IHistSrcSel       = pStatsTapOut->IHistStatsSrcSelection;
        pCoreCfg->HDRBESrcSel       = pStatsTapOut->HDRBEStatsSrcSelection;
        pCoreCfg->HDRBHistSrcSel    = pStatsTapOut->HDRBHistStatsSrcSelection;

        result = PacketBuilder::WriteGenericBlobData(pCmdBuffer,
            IFEGenericBlobTypeIFETopConfig,
            static_cast<UINT32>(sizeof(IFECoreConfig)),
            reinterpret_cast<BYTE*>(pCoreCfg));
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Argument");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan480IFE::UpdateWMConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan480IFE::UpdateWMConfig(
    CmdBuffer*       pCmdBuffer,
    ISPInternalData* pCalculatedData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCmdBuffer) && (NULL != pCalculatedData))
    {
        IFEOutConfig outConfig = { 0 };
        for (UINT32 wmIndex = 0; wmIndex < pCalculatedData->WMUpdate.numberOfWMUpdates; wmIndex++)
        {
            outConfig.outputPortConfig[outConfig.numOutputPorts].height         =
                pCalculatedData->WMUpdate.WMData[wmIndex].height;
            outConfig.outputPortConfig[outConfig.numOutputPorts].width          =
                pCalculatedData->WMUpdate.WMData[wmIndex].width;
            outConfig.outputPortConfig[outConfig.numOutputPorts].hInit          =
                pCalculatedData->WMUpdate.WMData[wmIndex].hInit;
            outConfig.outputPortConfig[outConfig.numOutputPorts].mode           =
                pCalculatedData->WMUpdate.WMData[wmIndex].mode;
            outConfig.outputPortConfig[outConfig.numOutputPorts].portID         =
                pCalculatedData->WMUpdate.WMData[wmIndex].portID;
            outConfig.outputPortConfig[outConfig.numOutputPorts].virtualFrameEn =
                pCalculatedData->WMUpdate.WMData[wmIndex].virtualFrameEn;
            outConfig.numOutputPorts++;
            CAMX_LOG_INFO(CamxLogGroupISP, "WM PorID 0x%X [w x h][%d x %d] hInit %d mode %d virtFrameEn %d",
                          pCalculatedData->WMUpdate.WMData[wmIndex].portID, pCalculatedData->WMUpdate.WMData[wmIndex].width,
                          pCalculatedData->WMUpdate.WMData[wmIndex].height, pCalculatedData->WMUpdate.WMData[wmIndex].hInit,
                          pCalculatedData->WMUpdate.WMData[wmIndex].mode,
                          pCalculatedData->WMUpdate.WMData[wmIndex].virtualFrameEn);
        }

        if (0 < pCalculatedData->WMUpdate.numberOfWMUpdates)
        {
            result = PacketBuilder::WriteGenericBlobData(pCmdBuffer,
                                                         IFEGenericBlobTypeIFEOutConfig,
                                                         static_cast<UINT32>(sizeof(IFEOutConfig)),
                                                         reinterpret_cast<BYTE*>(&outConfig));
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Arguments CmdBuffer %p pCalculatedData %p", pCmdBuffer, pCalculatedData);
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan480IFE::SetupFlushRegDump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan480IFE::SetupFlushRegDump(
    VOID* pFlushBuffer)
{
    UINT                  numberOfRegReadInfoRegions = 0;
    BYTE*                 pFlushDumpPtr;
    IFERegDumpDescriptor* pRegDumpDescriptor         = NULL;
    IFERegDumpInfo*       pRegDumpInfo               = NULL;
    UINT                  numRegReadRange            = 0;

    if (NULL != pFlushBuffer)
    {
        pFlushDumpPtr   = reinterpret_cast<BYTE*>(pFlushBuffer);
        pRegDumpInfo    = reinterpret_cast<IFERegDumpInfo*>(pFlushBuffer);
        pRegDumpDescriptor = reinterpret_cast<IFERegDumpDescriptor*>(pFlushDumpPtr + sizeof(IFERegDumpInfo));
        pRegDumpInfo->numberOfRegDumps  = 1;
        pRegDumpInfo->regDumpOffsets[0] = sizeof(IFERegDumpInfo);
        pRegDumpDescriptor->bufferOffset = sizeof(IFERegDumpInfo) +
                                           (sizeof(IFERegDumpDescriptor) * pRegDumpInfo->numberOfRegDumps);
        pRegDumpDescriptor->bufferSize   = sizeof(IFERegDumpOutput) +
                                           (sizeof(UINT32) * MaxIFEFlushDumpRegions);
        pRegDumpDescriptor->type         = IFERegDumpTypeIFELeft;
        FillIFERegReadInfo(&pRegDumpDescriptor->regReadInfo[numRegReadRange],
                           regIFE_IFE_0_PP_CLC_PEDESTAL_MODULE_LUT_BANK_CFG,
                           1);
        numRegReadRange++;
        FillIFERegReadInfo(&pRegDumpDescriptor->regReadInfo[numRegReadRange],
                           regIFE_IFE_0_PP_CLC_LINEARIZATION_MODULE_LUT_BANK_CFG,
                           1);
        numRegReadRange++;
        FillIFERegReadInfo(&pRegDumpDescriptor->regReadInfo[numRegReadRange],
                           regIFE_IFE_0_PP_CLC_BPC_PDPC_MODULE_LUT_BANK_CFG,
                           1);
        numRegReadRange++;
        FillIFERegReadInfo(&pRegDumpDescriptor->regReadInfo[numRegReadRange],
                           regIFE_IFE_0_PP_CLC_ABF_MODULE_LUT_BANK_CFG,
                           1);
        numRegReadRange++;
        FillIFERegReadInfo(&pRegDumpDescriptor->regReadInfo[numRegReadRange],
                           regIFE_IFE_0_PP_CLC_LSC_MODULE_LUT_BANK_CFG,
                           1);
        numRegReadRange++;
        FillIFERegReadInfo(&pRegDumpDescriptor->regReadInfo[numRegReadRange],
                           regIFE_IFE_0_PP_CLC_GTM_MODULE_LUT_BANK_CFG,
                           1);
        numRegReadRange++;
        FillIFERegReadInfo(&pRegDumpDescriptor->regReadInfo[numRegReadRange],
                           regIFE_IFE_0_PP_CLC_GLUT_MODULE_LUT_BANK_CFG,
                           1);
        numRegReadRange++;
        FillIFERegReadInfo(&pRegDumpDescriptor->regReadInfo[numRegReadRange],
                           regIFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_MODULE_LUT_BANK_CFG,
                           1);
        numRegReadRange++;
        FillIFERegReadInfo(&pRegDumpDescriptor->regReadInfo[numRegReadRange],
                           regIFE_IFE_0_PP_CLC_DSX_C_VID_OUT_MODULE_LUT_BANK_CFG,
                           1);
        numRegReadRange++;
        FillIFERegReadInfo(&pRegDumpDescriptor->regReadInfo[numRegReadRange],
                           regIFE_IFE_0_PP_CLC_STATS_HDR_BHIST_MODULE_LUT_BANK_CFG,
                           1);
        numRegReadRange++;
        FillIFERegReadInfo(&pRegDumpDescriptor->regReadInfo[numRegReadRange],
                           regIFE_IFE_0_PP_CLC_STATS_BHIST_MODULE_LUT_BANK_CFG,
                           1);
        numRegReadRange++;
        FillIFERegReadInfo(&pRegDumpDescriptor->regReadInfo[numRegReadRange],
                           regIFE_IFE_0_PP_CLC_STATS_BAF_MODULE_LUT_BANK_CFG,
                           1);
        numRegReadRange++;
        FillIFERegReadInfo(&pRegDumpDescriptor->regReadInfo[numRegReadRange],
                           regIFE_IFE_0_PP_CLC_STATS_IHIST_MODULE_LUT_BANK_CFG,
                           1);
        numRegReadRange++;
        FillIFERegReadInfo(&pRegDumpDescriptor->regReadInfo[numRegReadRange],
                           regIFE_IFE_0_CLC_PDLIB_MODULE_LUT_BANK_CFG,
                           1);
        numRegReadRange++;
        pRegDumpDescriptor->numberOfRegReadInfo = numRegReadRange;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan480IFE::UpdateDMIBankSelectValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan480IFE::UpdateDMIBankSelectValue(
    IFEDMIBankUpdate* pBankUpdate,
    BOOL              isBankValueZero)
{
    pBankUpdate->isValid = TRUE;

    // The actual bank value will be set in each IQ module after toggled first.
    const UINT32 bankValue = (TRUE == isBankValueZero) ? 0 : 1;

    pBankUpdate->pedestalBank       = bankValue;
    pBankUpdate->linearizaionBank   = bankValue;
    pBankUpdate->BPCPDPCBank        = bankValue;
    pBankUpdate->ABFBank            = bankValue;
    pBankUpdate->LSCBank            = bankValue;
    pBankUpdate->GTMBank            = bankValue;
    pBankUpdate->gammaBank          = bankValue;
    pBankUpdate->DSXYBank           = bankValue;
    pBankUpdate->DSXCBank           = bankValue;
    pBankUpdate->HDRBHistBank       = bankValue;
    pBankUpdate->BHistBank          = bankValue;
    pBankUpdate->BFStatsDMIBank     = bankValue;
    pBankUpdate->IHistBank          = bankValue;
    pBankUpdate->PDAFBank           = bankValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan480IFE::ParseFlushRegDump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan480IFE::ParseFlushRegDump(
    VOID*             pFlushBuffer,
    IFEDMIBankUpdate* pBankUpdate)
{
    UINT                  numberOfRegReadInfoRegions = 0;
    BYTE*                 pFlushDumpPtr;
    IFERegDumpDescriptor* pRegDumpDescriptor = NULL;
    IFERegDumpInfo*       pRegDumpInfo = NULL;
    UINT                  numRegReadRange = 0;

    if ((NULL != pFlushBuffer) && (NULL != pBankUpdate))
    {
        pFlushDumpPtr                    = reinterpret_cast<BYTE*>(pFlushBuffer);
        pRegDumpInfo                     = reinterpret_cast<IFERegDumpInfo*>(pFlushBuffer);
        pRegDumpDescriptor               = reinterpret_cast<IFERegDumpDescriptor*>(pFlushDumpPtr +
                                                                                   pRegDumpInfo->regDumpOffsets[0]);
        pRegDumpDescriptor->bufferOffset = sizeof(IFERegDumpInfo) +
                                           (sizeof(IFERegDumpDescriptor) * pRegDumpInfo->numberOfRegDumps);

        IFERegDumpOutput* pOutput = reinterpret_cast<IFERegDumpOutput*>(pFlushDumpPtr + pRegDumpDescriptor->bufferOffset);

        UINT32* pRegData = &pOutput->data[0];
        UINT32  offset   = 0;
        UINT32  value    = 0;

        if ((pOutput->numberOfBytes > 0) && (pOutput->requestID > 0))
        {
            pBankUpdate->isValid = TRUE;
        }

        for (UINT32 index = 0; index < (pOutput->numberOfBytes / RegisterWidthInBytes) / 2; index++)
        {
            offset = *pRegData;
            pRegData++;
            value  = *pRegData;
            // The bank value to be updated should be the toggled value of the parsed register value
            const UINT32 nextValue = value ^ 1;

            switch (offset)
            {
                case regIFE_IFE_0_PP_CLC_PEDESTAL_MODULE_LUT_BANK_CFG:
                    pBankUpdate->pedestalBank = nextValue;
                    break;
                case regIFE_IFE_0_PP_CLC_LINEARIZATION_MODULE_LUT_BANK_CFG:
                    pBankUpdate->linearizaionBank = nextValue;
                    break;
                case regIFE_IFE_0_PP_CLC_BPC_PDPC_MODULE_LUT_BANK_CFG:
                    pBankUpdate->BPCPDPCBank = nextValue;
                    break;
                case regIFE_IFE_0_PP_CLC_ABF_MODULE_LUT_BANK_CFG:
                    pBankUpdate->ABFBank = nextValue;
                    break;
                case regIFE_IFE_0_PP_CLC_LSC_MODULE_LUT_BANK_CFG:
                    pBankUpdate->LSCBank = nextValue;
                    break;
                case regIFE_IFE_0_PP_CLC_GTM_MODULE_LUT_BANK_CFG:
                    pBankUpdate->GTMBank = nextValue;
                    break;
                case regIFE_IFE_0_PP_CLC_GLUT_MODULE_LUT_BANK_CFG:
                    pBankUpdate->gammaBank = nextValue;
                    break;
                case regIFE_IFE_0_PP_CLC_DSX_Y_VID_OUT_MODULE_LUT_BANK_CFG:
                    pBankUpdate->DSXYBank = nextValue;
                    break;
                case regIFE_IFE_0_PP_CLC_DSX_C_VID_OUT_MODULE_LUT_BANK_CFG:
                    pBankUpdate->DSXCBank = nextValue;
                    break;
                case regIFE_IFE_0_PP_CLC_STATS_HDR_BHIST_MODULE_LUT_BANK_CFG:
                    pBankUpdate->HDRBHistBank = nextValue;
                    break;
                case regIFE_IFE_0_PP_CLC_STATS_BHIST_MODULE_LUT_BANK_CFG:
                    pBankUpdate->BHistBank = nextValue;
                    break;
                case regIFE_IFE_0_PP_CLC_STATS_BAF_MODULE_LUT_BANK_CFG:
                    pBankUpdate->BFStatsDMIBank = nextValue;
                    break;
                case regIFE_IFE_0_PP_CLC_STATS_IHIST_MODULE_LUT_BANK_CFG:
                    pBankUpdate->IHistBank = nextValue;
                    break;
                case regIFE_IFE_0_CLC_PDLIB_MODULE_LUT_BANK_CFG:
                    pBankUpdate->PDAFBank = nextValue;
                    break;
                default:
                    CAMX_LOG_WARN(CamxLogGroupISP, "Invalif Reg Offset %d Value %d",
                        offset, value);
            }
            CAMX_LOG_INFO(CamxLogGroupISP, "Offset 0x%X  Value %d", offset, value);

            pRegData++;
            offset = 0;
            value  = 0;

        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan480IFE::SetupHangRegDump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan480IFE::SetupHangRegDump(
    VOID*         pHangBuffer,
    IFEModuleMode mode,
    BOOL          isPerFrameDump)
{
    UINT                    numberOfRegReadInfoRegions  = 0;
    BYTE*                   pDumpPtr;
    IFERegDumpDescriptor*   pLeftRegDumpDescriptor      = NULL;
    IFERegDumpDescriptor*   pRightRegDumpDescriptor     = NULL;
    IFERegDumpInfo*         pRegDumpInfo                = NULL;
    UINT                    numRegReadRange             = 0;
    const StaticSettings*   pSettings                   = HwEnvironment::GetInstance()->GetStaticSettings();
    UINT32                  bufferSize;

    if (NULL != pHangBuffer)
    {
        pDumpPtr           = reinterpret_cast<BYTE*>(pHangBuffer);
        pRegDumpInfo       = reinterpret_cast<IFERegDumpInfo*>(pHangBuffer);

        if (FALSE == isPerFrameDump)
        {
            bufferSize = m_hangDumpOutputBufferSize;
        }
        else
        {
            bufferSize = m_perFrameDumpOutputBufferSize;
        }

        if (IFEModuleMode::DualIFENormal == mode)
        {
            pRegDumpInfo->numberOfRegDumps  = 2;
            pRegDumpInfo->regDumpOffsets[0] = sizeof(IFERegDumpInfo);
            pRegDumpInfo->regDumpOffsets[1] = sizeof(IFERegDumpInfo) + sizeof(IFERegDumpDescriptor);

            pLeftRegDumpDescriptor = reinterpret_cast<IFERegDumpDescriptor*>(pDumpPtr + pRegDumpInfo->regDumpOffsets[0]);

            pLeftRegDumpDescriptor->type         = IFERegDumpTypeIFELeft;

            pLeftRegDumpDescriptor->bufferOffset = sizeof(IFERegDumpInfo) +
                                                   (sizeof(IFERegDumpDescriptor) * pRegDumpInfo->numberOfRegDumps);
            pLeftRegDumpDescriptor->bufferSize   = bufferSize;


            pRightRegDumpDescriptor = reinterpret_cast<IFERegDumpDescriptor*>(pDumpPtr + pRegDumpInfo->regDumpOffsets[1]);

            pRightRegDumpDescriptor->type         = IFERegDumpTypeIFERight;

            pRightRegDumpDescriptor->bufferOffset = sizeof(IFERegDumpInfo) +
                                                    (sizeof(IFERegDumpDescriptor) * pRegDumpInfo->numberOfRegDumps) +
                                                    bufferSize;
            pRightRegDumpDescriptor->bufferSize   = bufferSize;

        }
        else
        {
            pRegDumpInfo->numberOfRegDumps       = 1;
            pRegDumpInfo->regDumpOffsets[0]      = sizeof(IFERegDumpInfo);
            pLeftRegDumpDescriptor               = reinterpret_cast<IFERegDumpDescriptor*>(pDumpPtr +
                                                                                           pRegDumpInfo->regDumpOffsets[0]);
            pLeftRegDumpDescriptor->bufferOffset = sizeof(IFERegDumpInfo) +
                                                   (sizeof(IFERegDumpDescriptor) * pRegDumpInfo->numberOfRegDumps);
            pLeftRegDumpDescriptor->bufferSize   = bufferSize;
            pLeftRegDumpDescriptor->type         = IFERegDumpTypeIFELeft;
        }

        BOOL includeReg = TRUE;

        for (UINT32 index = 0; index < sizeof(IFERegDump) / sizeof(RegDumpInfo); index++)
        {
            // include register if indicated by the ife reg dump mask for per frame dumps
            if (TRUE == isPerFrameDump)
            {
                includeReg = Utils::IsBitSet(pSettings->IFERegDumpMask, IFERegDump[index].regGroupType);
            }
            // TO -DO: Fill up the DMI Rgeisters Info also
            if ((TRUE == includeReg) && (IFERegDumpReadTypeReg == IFERegDump[index].regReadInfo.readType))
            {
                FillIFERegReadInfo(&pLeftRegDumpDescriptor->regReadInfo[numRegReadRange],
                                   IFERegDump[index].regReadInfo.regDescriptor.regReadCmd.offset,
                                   IFERegDump[index].regReadInfo.regDescriptor.regReadCmd.numberOfRegisters);
                if (IFEModuleMode::DualIFENormal == mode)
                {
                    FillIFERegReadInfo(&pRightRegDumpDescriptor->regReadInfo[numRegReadRange],
                                       IFERegDump[index].regReadInfo.regDescriptor.regReadCmd.offset,
                                       IFERegDump[index].regReadInfo.regDescriptor.regReadCmd.numberOfRegisters);
                }
                numRegReadRange++;
            }
        }

        pLeftRegDumpDescriptor->numberOfRegReadInfo = numRegReadRange;
        if (IFEModuleMode::DualIFENormal == mode)
        {
            pRightRegDumpDescriptor->numberOfRegReadInfo = numRegReadRange;
        }
    }

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan480IFE::ParseHangRegDump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan480IFE::ParseHangRegDump(
    VOID* pHangBuffer,
    INT   leftHangFd,
    INT   rightHangFd)
{
    UINT                  numberOfRegReadInfoRegions = 0;
    BYTE*                 pDumpPtr;
    IFERegDumpDescriptor* pLeftRegDumpDescriptor = NULL;
    IFERegDumpDescriptor* pRightRegDumpDescriptor = NULL;
    IFERegDumpInfo*       pRegDumpInfo = NULL;

    if (NULL != pHangBuffer)
    {
        pDumpPtr               = reinterpret_cast<BYTE*>(pHangBuffer);
        pRegDumpInfo           = reinterpret_cast<IFERegDumpInfo*>(pHangBuffer);
        pLeftRegDumpDescriptor = reinterpret_cast<IFERegDumpDescriptor*>(pDumpPtr + pRegDumpInfo->regDumpOffsets[0]);

        if (pRegDumpInfo->numberOfRegDumps > 1)
        {
            pRightRegDumpDescriptor = reinterpret_cast<IFERegDumpDescriptor*>(pDumpPtr + pRegDumpInfo->regDumpOffsets[1]);
        }

        IFERegDumpOutput* pOutput = reinterpret_cast<IFERegDumpOutput*>(pDumpPtr + pLeftRegDumpDescriptor->bufferOffset);

        UINT32* pRegData = &pOutput->data[0];
        UINT32  offset   = 0;
        UINT32  value    = 0;
        if (0 <= leftHangFd)
        {
            for (UINT32 index = 0; index < (pOutput->numberOfBytes / RegisterWidthInBytes) / 2; index ++)
            {
                offset = *pRegData;
                pRegData++;
                value  = *pRegData;
                CAMX_LOG_TO_FILE(leftHangFd, 0, "0x%X=0x%X", offset, value);
                pRegData++;
            }
        }

        if (NULL != pRightRegDumpDescriptor)
        {
            pOutput  = reinterpret_cast<IFERegDumpOutput*>(pDumpPtr + pRightRegDumpDescriptor->bufferOffset);
            pRegData = &pOutput->data[0];

            if (0 <= rightHangFd)
            {
                for (UINT32 index = 0; index < (pOutput->numberOfBytes / RegisterWidthInBytes) / 2; index ++)
                {
                    offset = *pRegData;
                    pRegData++;
                    value  = *pRegData;
                    CAMX_LOG_TO_FILE(rightHangFd, 0, "0x%X=0x%X", offset, value);
                    pRegData++;
                }
            }
        }
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan480IFE::SetThrottlePattern
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan480IFE::SetThrottlePattern(
    FLOAT      throughPut,
    CmdBuffer* pCmdBuffer)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCmdBuffer)
    {
        IFE_IFE_0_TOP_CORE_CFG_2  throttleCfg;

        if (Utils::AbsoluteFLOAT(throughPut) > 0.75)
        {
            throttleCfg.bitfields.THROTTLE_EN = 0;
        }
        else if (Utils::AbsoluteFLOAT(throughPut) > 0.50)
        {
            throttleCfg.bitfields.THROTTLE_EN          = 1;
            throttleCfg.bitfields.THROTTLE_PATTERN_VAL = IFEThrottlePattern75;
        }
        else if (Utils::AbsoluteFLOAT(throughPut) > 0.25)
        {
            throttleCfg.bitfields.THROTTLE_EN          = 1;
            throttleCfg.bitfields.THROTTLE_PATTERN_VAL = IFEThrottlePattern50;
        }
        else
        {
            throttleCfg.bitfields.THROTTLE_EN          = 1;
            throttleCfg.bitfields.THROTTLE_PATTERN_VAL = IFEThrottlePattern25;
        }

        CAMX_LOG_INFO(CamxLogGroupISP, "Throttle Cfg 0x%X", throttleCfg.u32All);
        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_TOP_CORE_CFG_2,
                                              (sizeof(IFE_IFE_0_TOP_CORE_CFG_2) / RegisterWidthInBytes),
                                              reinterpret_cast<UINT32*>(&throttleCfg));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Throttle Cfg Submit Failed");
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan480IFE::Titan480IFE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Titan480IFE::Titan480IFE()
{
    const StaticSettings* pSettings = HwEnvironment::GetInstance()->GetStaticSettings();

    SetRegCmdSize(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEThrottleCfgSize));

    SetFlushDumpBufferSize(sizeof(IFERegDumpInfo)                               +
                          (sizeof(IFERegDumpDescriptor) * MaxRegDumpOffsets)    +
                          sizeof(IFERegDumpOutput)                              +
                          (sizeof(UINT32) * MaxIFEFlushDumpRegions * 2));

    for (UINT32 index = 0; index < sizeof(IFERegDump) / sizeof(RegDumpInfo); index++)
    {
        if (IFERegDumpReadTypeReg == IFERegDump[index].regReadInfo.readType)
        {
            m_hangDumpOutputBufferSize+= IFERegDump[index].regReadInfo.regDescriptor.regReadCmd.numberOfRegisters;
        }
        else if (IFERegDumpReadTypeDMI == IFERegDump[index].regReadInfo.readType)
        {
            m_hangDumpOutputBufferSize+= IFERegDump[index].regReadInfo.regDescriptor.dmiReadCmd.regRangeCmd.numberOfRegisters +
                                         IFERegDump[index].regReadInfo.regDescriptor.dmiReadCmd.numberOfRegWrites;
        }
    }

    m_hangDumpOutputBufferSize = sizeof(IFERegDumpOutput) + (m_hangDumpOutputBufferSize * sizeof(UINT32) * 2);

    SetHangDumpBufferSize(sizeof(IFERegDumpInfo)                                +
                          (sizeof(IFERegDumpDescriptor) * MaxRegDumpOffsets)    +
                          (m_hangDumpOutputBufferSize   * MaxRegDumpOffsets));

    // TO -DO: follow up on per frame dump output buffer size
    m_perFrameDumpOutputBufferSize = m_hangDumpOutputBufferSize;

    UINT regDumpBufferSize = 0;
    if (TRUE == pSettings->enableIFERegDump)
    {
        regDumpBufferSize = sizeof(IFERegDumpInfo)                                  +
                            (sizeof(IFERegDumpDescriptor) * MaxRegDumpOffsets)      +
                            (m_perFrameDumpOutputBufferSize   * MaxRegDumpOffsets);
    }
    SetRegDumpBufferSize(regDumpBufferSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan480IFE::~Titan480IFE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Titan480IFE::~Titan480IFE()
{

}

CAMX_NAMESPACE_END
