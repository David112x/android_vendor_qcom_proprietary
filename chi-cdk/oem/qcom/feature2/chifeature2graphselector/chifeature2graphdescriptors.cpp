////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2graphdescriptors.cpp
/// @brief Definitions of static data describing all the feature graph descriptors
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graph.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the HDRT1FeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor HDRT1FeatureDescriptor;
extern const ChiFeature2PortDescriptor HDRT1FeatureInputPortDescriptors[];
extern const ChiFeature2PortDescriptor HDRT1FeatureOutputPortDescriptors[];

static const UINT8 RealTimeOutputPortDisplay  = 0;
static const UINT8 RealTimeOutputPortRDI      = 1;
static const UINT8 RealTimeOutputPortFD       = 2;
static const UINT8 RealTimeOutputPortVideo    = 3;
static const UINT8 RealTimeOutputPortRawCb    = 4;
static const UINT8 RealTimeOutputPortMetaData = 5;

static const UINT8 B2YInputPortRDI            = 0;
static const UINT8 B2YInputPortMetaData       = 1;

static const UINT8 B2YOutputPortYUVOut        = 0;
static const UINT8 B2YOutputPortYUVMetaData   = 1;
static const UINT8 B2YOutputPortYUVOut2       = 2;

static const UINT8 JPEGOutputPort             = 0;
static const UINT8 JPEGOutputPortMetaData     = 1;

static const UINT8 JPEGInputPortYUV           = 0;
static const UINT8 JPEGInputPortMetaData      = 1;
static const UINT8 JPEGInputPortYUV2          = 2;

static const UINT8 SWMFOutputPortYUVOut   = 0;
static const UINT8 SWMFOutputPortMetaData = 1;

static const UINT8 SWMFInputPortYUV_In0_External  = 0;
static const UINT8 SWMFInputPortYUV_In1_Internal  = 1;
static const UINT8 SWMFInputPortYUV_In2_Internal  = 2;
static const UINT8 SWMFInputPortYUV_In3_Internal  = 3;
static const UINT8 SWMFInputPortYUV_In4_Internal  = 4;
static const UINT8 SWMFInputPortYUV_In5_Internal  = 5;
static const UINT8 SWMFInputPortMeta_In2_Internal = 9;

static const UINT8 HDROutputPortYUVOutExternal  = 0;
static const UINT8 HDROutputPortMetaOutExternal = 1;

static const UINT8 HDRInputPortYUV_In0_External  = 0;
static const UINT8 HDRInputPortYUV_In1_Internal  = 1;
static const UINT8 HDRInputPortYUV_In2_Internal  = 2;
static const UINT8 HDRInputPortYUV_In3_Internal  = 3;
static const UINT8 HDRInputPortMeta_In2_Internal = 4;

static const UINT8 MemcpyOutputPortFwk          = 0;
static const UINT8 MemcpyOutputPortInternalMeta = 1;
static const UINT8 MemcpyOutputPortInternal     = 2;

static ChiFeature2InstanceProps HDRT1InstanceProps =
{
    0,
    0
};

static ChiFeature2InstanceDesc HDRT1FeatureGraphFeatureInstanceDescs[] =
{
    // HDRT1 feature
    {
        &HDRT1FeatureDescriptor,
        &HDRT1InstanceProps
    }
};

static ChiFeature2GraphExtSrcLinkDesc HDRT1FeatureGraphSrcLinks[]=
{
    {
        {
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[0].globalId
        }
    }
};

static ChiFeature2GraphExtSinkLinkDesc HDRT1FeatureGraphSinkLinks[]=
{
    {
        {
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[0].globalId
        }
    }
};

extern const ChiFeature2GraphDesc HDRT1FeatureGraphDescriptor =
{
    "HDRT1",                                    // Feature graph name
    1,                                              // The number of feature instances
    &HDRT1FeatureGraphFeatureInstanceDescs[0],  // The array containing all feature instance descriptors for the graph
    1,                                              // The number of source links
    &HDRT1FeatureGraphSrcLinks[0],              // The array containing links associated with all source ports
    0,                                              // The number of internal links
    NULL,                                           // The array containing links associated with all internal ports
    1,                                              // The number of links pSinkLinks
    &HDRT1FeatureGraphSinkLinks[0]              // The array containing links associated with all sink ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the HDRT1FeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the Bayer2YUVFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor Bayer2YuvFeatureDescriptor;
extern const ChiFeature2PortDescriptor Bayer2YuvInputPortDescriptors[];
extern const ChiFeature2PortDescriptor Bayer2YuvOutputPortDescriptors[];

static ChiFeature2InstanceProps Bayer2YUVInstanceProps =
{
    0,
    0
};

static ChiFeature2InstanceDesc Bayer2YUVFeatureGraphFeatureInstanceDescs[] =
{
    // Bayer to YUV feature
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps
    }
};

static ChiFeature2GraphExtSrcLinkDesc Bayer2YUVFeatureGraphSrcLinks[]=
{
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        }
    }
};

static ChiFeature2GraphExtSinkLinkDesc Bayer2YUVFeatureGraphSinkLinks[]=
{
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        }
    }
};

extern const ChiFeature2GraphDesc Bayer2YUVFeatureGraphDescriptor =
{
    "Bayer2YUV",                                    // Feature graph name
    1,                                              // The number of feature instances
    &Bayer2YUVFeatureGraphFeatureInstanceDescs[0],  // The array containing all feature instance descriptors for the graph
    CHX_ARRAY_SIZE(Bayer2YUVFeatureGraphSrcLinks),  // The number of source links
    &Bayer2YUVFeatureGraphSrcLinks[0],              // The array containing links associated with all source ports
    0,                                              // The number of internal links
    NULL,                                           // The array containing links associated with all internal ports
    CHX_ARRAY_SIZE(Bayer2YUVFeatureGraphSinkLinks), // The number of links pSinkLinks
    &Bayer2YUVFeatureGraphSinkLinks[0]              // The array containing links associated with all sink ports
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the Bayer2YUVFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the MultiCameraFusionFeatureGraphDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor Bayer2YuvFeatureDescriptor;
extern const ChiFeature2PortDescriptor Bayer2YuvInputPortDescriptors[];
extern const ChiFeature2PortDescriptor Bayer2YuvOutputPortDescriptors[];
extern const ChiFeature2Descriptor FusionFeatureDescriptor;
extern const ChiFeature2PortDescriptor FusionInputPortDescriptors[];
extern const ChiFeature2PortDescriptor FusionOutputPortDescriptors[];

static ChiFeature2InstanceProps FusionFeatureInstanceProps =
{
    0,
    0
};

static ChiFeature2InstanceProps Bayer2YUVInstanceProps0 =
{
    0,
    0
};

static ChiFeature2InstanceProps Bayer2YUVInstanceProps1 =
{
    1,
    1
};

static ChiFeature2InstanceProps Bayer2YUVInstanceProps2 =
{
    2,
    2
};

static ChiFeature2InstanceProps Bayer2YUVInstanceProps3 =
{
    3,
    3
};


static ChiFeature2InstanceDesc MultiCameraFusionFeatureGraphFeatureInstanceDescs[] =
{
    // Bayer to YUV feature
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps0
    },
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps1
    },
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps2
    },
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps3
    },
    {
        &FusionFeatureDescriptor,
        &FusionFeatureInstanceProps
    }
};

static ChiFeature2GraphExtSrcLinkDesc MultiCameraFusionFeatureGraphSrcLinks[]=
{
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps0,
            Bayer2YuvInputPortDescriptors[0].globalId
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps0,
            Bayer2YuvInputPortDescriptors[1].globalId
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[0].globalId
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[1].globalId
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[0].globalId
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[1].globalId
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps3,
            Bayer2YuvInputPortDescriptors[0].globalId
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps3,
            Bayer2YuvInputPortDescriptors[1].globalId
        }
    }
};

static ChiFeature2GraphInternalLinkDesc MultiCameraFusionFeatureGraphInternalLinks[] =
{
    {
        {
            // YUV_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps0,
            Bayer2YuvOutputPortDescriptors[0].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[0].globalId
        }
    },
    {
        {
            // Meta_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps0,
            Bayer2YuvOutputPortDescriptors[1].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[1].globalId
        }
    },
    {
        {
            // YUV_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[0].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[2].globalId
        }
    },
    {
        {
            // Meta_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[1].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[3].globalId
        }
    },
    {
        {
            // YUV_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[0].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[4].globalId
        }
    },
    {
        {
            // Meta_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[1].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[5].globalId
        }
    },
    {
        {
            // YUV_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps3,
            Bayer2YuvOutputPortDescriptors[0].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[6].globalId
        }
    },
    {
        {
            // Meta_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps3,
            Bayer2YuvOutputPortDescriptors[1].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[7].globalId
        }
    }
};

static ChiFeature2GraphExtSinkLinkDesc MultiCameraFusionFeatureGraphSinkLinks[]=
{
    {
        {
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionOutputPortDescriptors[0].globalId
        }
    },
    {
        {
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionOutputPortDescriptors[1].globalId
        }
    },
};

extern const ChiFeature2GraphDesc MultiCameraFusionFeatureGraphDescriptor =
{
    "MultiCameraFusionGraph",                                           // Feature graph name
    CHX_ARRAY_SIZE(MultiCameraFusionFeatureGraphFeatureInstanceDescs),  // The number of feature instances
    &MultiCameraFusionFeatureGraphFeatureInstanceDescs[0],              // The array containing all feature instance
    CHX_ARRAY_SIZE(MultiCameraFusionFeatureGraphSrcLinks),              // The number of source links
    &MultiCameraFusionFeatureGraphSrcLinks[0],                          // The array of all source links
    CHX_ARRAY_SIZE(MultiCameraFusionFeatureGraphInternalLinks),         // The number of internal links
    &MultiCameraFusionFeatureGraphInternalLinks[0],                     // The array of all internal links
    CHX_ARRAY_SIZE(MultiCameraFusionFeatureGraphSinkLinks),             // The number of links pSinkLinks
    &MultiCameraFusionFeatureGraphSinkLinks[0],                         // The array of all sink links
    TRUE
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the MultiCameraFusionFeatureGraphDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the MFNRFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor     MFNRFeatureDescriptor;
extern const ChiFeature2PortDescriptor MFNRPrefilterInputPortDescriptors[];
extern const ChiFeature2PortDescriptor MFNRPostFilterOutPutPortDescriptors[];
extern const ChiFeature2PortDescriptor MFNRPrefilterInputPortDescriptorsConfig[];

static ChiFeature2InstanceProps MFNRInstanceProps =
{
    0,
    0
};

static ChiFeature2InstanceProps MFNRInstanceProps1 =
{
    1,
    1
};

static ChiFeature2InstanceProps MFNRInstanceProps2 =
{
    2,
    2
};

static ChiFeature2InstanceDesc MFNRFeatureGraphFeatureInstanceDescs[] =
{
    // MFNR feature
    {
        &MFNRFeatureDescriptor,
        &MFNRInstanceProps
    }
};

static ChiFeature2GraphExtSrcLinkDesc MFNRFeatureGraphSrcLinks[] =
{
    {
        {
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPrefilterInputPortDescriptorsConfig[0].globalId
        }
    },
    {
        {
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPrefilterInputPortDescriptorsConfig[1].globalId
        }
    }
};

static ChiFeature2GraphExtSinkLinkDesc MFNRFeatureGraphSinkLinks[] =
{
    {
        {
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        }
    },
    {
        {
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        }
    }
};

extern const ChiFeature2GraphDesc MFNRFeatureGraphDescriptor =
{
    "MFNR",                                         // Feature graph name
    1,                                              // The number of feature instances
    &MFNRFeatureGraphFeatureInstanceDescs[0],       // The array containing all feature instance descriptors for the graph
    CHX_ARRAY_SIZE(MFNRFeatureGraphSrcLinks),       // The number of external source links
    &MFNRFeatureGraphSrcLinks[0],                   // The array containing external links associated with all source ports
    0,                                              // The number of internal links
    NULL,                                           // The array containing link between two FG Nodes in a feature graph
    CHX_ARRAY_SIZE(MFNRFeatureGraphSinkLinks),      // The number of external links pSinkLinks
    &MFNRFeatureGraphSinkLinks[0]                   // The array containing external links associated with all sink ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the MFNRFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the MFSRFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor     MFSRFeatureDescriptor;
extern const ChiFeature2PortDescriptor MfsrPrefilterInputPortDescriptors[];
extern const ChiFeature2PortDescriptor MfsrPostFilterOutPutPortDescriptors[];
extern const ChiFeature2PortDescriptor MfsrPrefilterInputPortDescriptorsConfig[];

static ChiFeature2InstanceProps MFSRInstanceProps =
{
    0,
    0
};

static ChiFeature2InstanceProps MFSRInstanceProps1 =
{
    1,
    1
};

static ChiFeature2InstanceProps MFSRInstanceProps2 =
{
    2,
    2
};

static ChiFeature2InstanceDesc MFSRFeatureGraphFeatureInstanceDescs[] =
{
    // MFSR feature
    {
        &MFSRFeatureDescriptor,
        &MFSRInstanceProps
    }
};

static ChiFeature2GraphExtSrcLinkDesc MFSRFeatureGraphSrcLinks[] =
{
    {
        {
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPrefilterInputPortDescriptorsConfig[0].globalId
        }
    },
    {
        {
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPrefilterInputPortDescriptorsConfig[1].globalId
        }
    }
};

static ChiFeature2GraphExtSinkLinkDesc MFSRFeatureGraphSinkLinks[] =
{
    {
        {
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        }
    },
    {
        {
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPostFilterOutPutPortDescriptors[1].globalId
        }
    }
};

extern const ChiFeature2GraphDesc MFSRFeatureGraphDescriptor =
{
    "MFSR",                                         // Feature graph name
    1,                                              // The number of feature instances
    &MFSRFeatureGraphFeatureInstanceDescs[0],       // The array containing all feature instance descriptors for the graph
    CHX_ARRAY_SIZE(MFSRFeatureGraphSrcLinks),       // The number of external source links
    &MFSRFeatureGraphSrcLinks[0],                   // The array containing external links associated with all source ports
    0,                                              // The number of internal links
    NULL,                                           // The array containing link between two FG Nodes in a feature graph
    CHX_ARRAY_SIZE(MFSRFeatureGraphSinkLinks),      // The number of external links pSinkLinks
    &MFSRFeatureGraphSinkLinks[0]                   // The array containing external links associated with all sink ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the MFSRFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the RealTimeFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor RealTimeFeatureDescriptor;
extern const ChiFeature2PortDescriptor RealTimeInputPortDescriptors[];
extern const ChiFeature2PortDescriptor RealTimeOutputPortDescriptors[];

static const CHAR RealtimeFeatureProperty1Value[] =
{
    "string",
};

static const CHAR RealtimeFeatureProperty2Value[] =
{
    "1",
};

static ChiFeature2Property RealtimeFeatureProperties[] =
{
    {
        1,
        "property1",
        sizeof(RealtimeFeatureProperty1Value),
        &RealtimeFeatureProperty1Value[0],
    },
    {
        2,
        "property2",
        sizeof(RealtimeFeatureProperty2Value),
        &RealtimeFeatureProperty2Value[0],
    },
};

static ChiFeature2InstanceProps RealTimeInstanceProps =
{
    0,
    0,
    CHX_ARRAY_SIZE(RealtimeFeatureProperties),
    &RealtimeFeatureProperties[0],
};

static ChiFeature2InstanceDesc RealTimeFeatureGraphFeatureInstanceDescs[] =
{
    //  RealTime feature
    {
        &RealTimeFeatureDescriptor,
        &RealTimeInstanceProps
    }
};

static ChiFeature2GraphExtSinkLinkDesc RealTimeFeatureGraphSinkLinks[] =
{
    {
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortDisplay].globalId
        }
    },
    {
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortRDI].globalId
        }
    },
    {
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortFD].globalId
        }
    },
    {
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortVideo].globalId
        }
    },
    {
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortMetaData].globalId
        }
    }
};

extern const ChiFeature2GraphDesc RealTimeFeatureGraphDescriptor =
{
    "RealtimeFG",                                             // Feature graph name
    CHX_ARRAY_SIZE(RealTimeFeatureGraphFeatureInstanceDescs), // The number of feature instances
    &RealTimeFeatureGraphFeatureInstanceDescs[0],             // The array containing all feature instance
                                                              // descriptors for the graph
    0,                                                        // The number of source links
    NULL,                                                     // The array containing links associated with all source ports
    0,                                                        // The number of internal links
    NULL,                                                     // The array containing links associated with all internal ports
    CHX_ARRAY_SIZE(RealTimeFeatureGraphSinkLinks),            // The number of links pSinkLinks
    &RealTimeFeatureGraphSinkLinks[0]                         // The array containing links associated with all sink ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the RealTimeFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the StubFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor     StubFeatureDescriptor;
extern const ChiFeature2PortDescriptor StubOutputPortDescriptors[];

extern const ChiFeature2InstanceProps StubInstanceProps =
{
    0,
    0
};

static ChiFeature2InstanceDesc StubFeatureGraphFeatureInstanceDescs[] =
{
    {
        &RealTimeFeatureDescriptor,
        &RealTimeInstanceProps
    },
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps
    }
};

static ChiFeature2GraphInternalLinkDesc StubFeatureGraphInternalLinks[] =
{
    {
        {
            // RT OUT
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[0].globalId
        },
        {
            // B2Y In
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[0].globalId
        }
    },
    {
        {
            // rt_metadata_out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[1].globalId
        },
        {
            // B2Y Input Metadata
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[1].globalId
        }
    },
};

static ChiFeature2GraphExtSinkLinkDesc StubFeatureGraphSinkLinks[] =
{
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[0].globalId
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[1].globalId
        }
    },
};

extern const ChiFeature2GraphDesc StubFeatureGraphDescriptor =
{
    "Stub",                                                 // Feature graph name
    CHX_ARRAY_SIZE(StubFeatureGraphFeatureInstanceDescs),   // The number of feature instances
    &StubFeatureGraphFeatureInstanceDescs[0],               // The array containing all feature instance
                                                            // descriptors for the graph
    0,                                                      // The number of source links
    NULL,                                                   // The array containing links associated with all source ports
    CHX_ARRAY_SIZE(StubFeatureGraphInternalLinks),          // The number of internal links
    &StubFeatureGraphInternalLinks[0],                      // The array containing links associated with all internal ports
    CHX_ARRAY_SIZE(StubFeatureGraphSinkLinks),              // The number of links pSinkLinks
    &StubFeatureGraphSinkLinks[0]                           // The array containing links associated with all sink ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the StubFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the RTBayer2YUVJPEGFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor     RealTimeFeatureDescriptor;
extern const ChiFeature2Descriptor     RealTimeFeatureWithSWRemosaicDescriptor;
extern const ChiFeature2Descriptor     Bayer2YuvFeatureDescriptor;
extern const ChiFeature2Descriptor     JPEGFeatureDescriptorGPU;
extern const ChiFeature2Descriptor     MemcpyFeatureDescriptor;
extern const ChiFeature2PortDescriptor RealTimeInputPortDescriptors[];
extern const ChiFeature2PortDescriptor RealTimeOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor Bayer2YuvInputPortDescriptors[];
extern const ChiFeature2PortDescriptor Bayer2YuvOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGOutputPortDescriptorsGPU[];
extern const ChiFeature2PortDescriptor JPEGInputPortDescriptorsAll[];
extern const ChiFeature2PortDescriptor MemcpyInputPortDescriptors[];
extern const ChiFeature2PortDescriptor MemcpyOutputPortDescriptors[];


static ChiFeature2InstanceProps MemcpyInstanceProps =
{
    0,
    0
};

static ChiFeature2InstanceProps JPEGInstanceProps =
{
    0,
    0
};

static ChiFeature2InstanceProps JPEGInstancePropsGPU =
{
    1,
    0
};

static ChiFeature2InstanceDesc RTBayer2YUVJPEGFeatureGraphFeatureInstanceDescs[] =
{
    //  RealTime feature
    {
        &RealTimeFeatureDescriptor,
        &RealTimeInstanceProps
    },
    // Bayer to YUV feature
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps
    },
    // JPEG feature
    {
        &JPEGFeatureDescriptorGPU,
        &JPEGInstancePropsGPU
    },
    // Memcpy feature
    {
        &MemcpyFeatureDescriptor,
        &MemcpyInstanceProps
    }
};

static ChiFeature2GraphInternalLinkDesc RTBayer2YUVJPEGFeatureGraphInternalLinks[] =
{
    {
        {
            // Raw_out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortRDI].globalId
        },
        {
            // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        }
    },
    {
        {
            // rt_metadata_out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortMetaData].globalId
        },
        {
            // B2Y Input Metadata
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        }
    },
    {
        {
            // B2Y out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // JPEG in
            JPEGFeatureDescriptorGPU.featureId,
            JPEGInstancePropsGPU,
            JPEGInputPortDescriptorsAll[JPEGInputPortYUV].globalId
        }
    },
    {
        {
            // B2Y out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut2].globalId
        },
        {
            // JPEG in
            JPEGFeatureDescriptorGPU.featureId,
            JPEGInstancePropsGPU,
            JPEGInputPortDescriptorsAll[JPEGInputPortYUV2].globalId
        }
    },
    {
        {
            // YUV_Metadata_Out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // JPEG_Input_Metadata
            JPEGFeatureDescriptorGPU.featureId,
            JPEGInstancePropsGPU,
            JPEGInputPortDescriptorsAll[JPEGInputPortMetaData].globalId
        }
    },
    {
        {
            // Realtime FD out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortFD].globalId
        },
        {
            // Memcpy In
            MemcpyFeatureDescriptor.featureId,
            MemcpyInstanceProps,
            MemcpyInputPortDescriptors[0].globalId
        }
    },
};

static ChiFeature2GraphExtSinkLinkDesc RTBayer2YUVJPEGFeatureGraphSinkLinks[] =
{
    {
        // Display_out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortDisplay].globalId
        }
    },
    {
        // JPEG out
        {
            JPEGFeatureDescriptorGPU.featureId,
            JPEGInstancePropsGPU,
            JPEGOutputPortDescriptorsGPU[JPEGOutputPort].globalId
        }
    },
    {
        // JPEG Metadata Out
        {
            JPEGFeatureDescriptorGPU.featureId,
            JPEGInstancePropsGPU,
            JPEGOutputPortDescriptorsGPU[JPEGOutputPortMetaData].globalId
        }
    },
    {
        // HEIC YUV Out
        {
            JPEGFeatureDescriptorGPU.featureId,
            JPEGInstancePropsGPU,
            JPEGOutputPortDescriptorsGPU[2].globalId
        }
    },
    {
        // HEIC Thumbnail Out
        {
            JPEGFeatureDescriptorGPU.featureId,
            JPEGInstancePropsGPU,
            JPEGOutputPortDescriptorsGPU[3].globalId
        }
    },
    {
        // Video out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortVideo].globalId
        }
    },
    {
        // RAW out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortRawCb].globalId
        }
    },
    {
        // Memcpy yuv framework out
        {
            MemcpyFeatureDescriptor.featureId,
            MemcpyInstanceProps,
            MemcpyOutputPortDescriptors[MemcpyOutputPortFwk].globalId
        }
    },
    {
        // JPEG out2
        {
            JPEGFeatureDescriptorGPU.featureId,
            JPEGInstancePropsGPU,
            JPEGOutputPortDescriptorsGPU[4].globalId
        }
    },
};

extern const ChiFeature2GraphDesc RTBayer2YUVJPEGFeatureGraphDescriptor =
{
    "RTBayer2YUVJPEG",                                               // Feature graph name
    CHX_ARRAY_SIZE(RTBayer2YUVJPEGFeatureGraphFeatureInstanceDescs), // The number of feature instances
    &RTBayer2YUVJPEGFeatureGraphFeatureInstanceDescs[0],             // The array containing all feature instance
                                                                     // descriptors for the graph
    0,                                                               // The number of source links
    NULL,                                                            // The array containing links associated
                                                                     // with all source ports
    CHX_ARRAY_SIZE(RTBayer2YUVJPEGFeatureGraphInternalLinks),        // The number of internal links
    &RTBayer2YUVJPEGFeatureGraphInternalLinks[0],                    // The array containing links associated
                                                                     // with all internal ports
    CHX_ARRAY_SIZE(RTBayer2YUVJPEGFeatureGraphSinkLinks),            // The number of links pSinkLinks
    &RTBayer2YUVJPEGFeatureGraphSinkLinks[0]                         // The array containing links associated
                                                                     // with all sink ports
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the RTBayer2YUVJPEGFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the RTBayer2YUVHDRT1JPEGFGNativeResolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor     RealTimeFeatureDescriptor;
extern const ChiFeature2Descriptor     Bayer2YuvFeatureDescriptor;
extern const ChiFeature2Descriptor     JPEGFeatureDescriptor;
extern const ChiFeature2PortDescriptor RealTimeInputPortDescriptors[];
extern const ChiFeature2PortDescriptor RealTimeOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor Bayer2YuvInputPortDescriptors[];
extern const ChiFeature2PortDescriptor Bayer2YuvOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGInputPortDescriptors[];

// Bayer2YUV Property
static const CHAR Bayer2YUVFeatureProperty1Value[] =
{
    "TRUE",
};

static ChiFeature2Property Bayer2YUVFeatureProperties[] =
{
    {
        1,
        "nativeinputresolution",
        sizeof(Bayer2YUVFeatureProperty1Value),
        &Bayer2YUVFeatureProperty1Value[0],
    },
};

static ChiFeature2InstanceProps Bayer2YUVInstancePropsNativeResolution =
{
    1,
    0,
    CHX_ARRAY_SIZE(Bayer2YUVFeatureProperties),
    &Bayer2YUVFeatureProperties[0],
};

// JPEG Property
static const CHAR JPEGFeatureProperty1Value[] =
{
    "TRUE",
};

static ChiFeature2Property JPEGFeatureProperties[] =
{
    {
        1,
        "nativeinputresolution",
        sizeof(JPEGFeatureProperty1Value),
        &JPEGFeatureProperty1Value[0],
    },
};

static ChiFeature2InstanceProps JPEGInstancePropsNativeResolution =
{
    1,
    0,
    CHX_ARRAY_SIZE(JPEGFeatureProperties),
    &JPEGFeatureProperties[0],
};

// HDR Property
static const CHAR HDRT1InstanceFeatureProperty1Value[] =
{
    "TRUE",
};

static ChiFeature2Property HDRT1InstanceFeatureProperties[] =
{
    {
        1,
        "nativeinputresolution",
        sizeof(HDRT1InstanceFeatureProperty1Value),
        &HDRT1InstanceFeatureProperty1Value[0],
    },
};

static ChiFeature2InstanceProps HDRT1InstancePropsNativeResolution =
{
    1,
    0,
    CHX_ARRAY_SIZE(HDRT1InstanceFeatureProperties),
    &HDRT1InstanceFeatureProperties[0],
};

static ChiFeature2InstanceDesc RTBayer2YUVHDRT1JPEGFGNativeResolutionFeatureInstanceDescs[] =
{
    //  RealTime feature
    {
        &RealTimeFeatureDescriptor,
        &RealTimeInstanceProps
    },
    // Bayer to YUV feature
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstancePropsNativeResolution
    },
    // HDR feature
    {
        &HDRT1FeatureDescriptor,
        &HDRT1InstancePropsNativeResolution
    },
    // JPEG feature
    {
        &JPEGFeatureDescriptor,
        &JPEGInstancePropsNativeResolution
    }
};

static ChiFeature2GraphInternalLinkDesc RTBayer2YUVHDRT1JPEGFGNativeResolutionInternalLinks[] =
{
    {
        {
            // Raw_out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortRDI].globalId
        },
        {
            // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstancePropsNativeResolution,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        }
    },
    {
        {
            // rt_metadata_out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortMetaData].globalId
        },
        {   // B2Y Input Metadata
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstancePropsNativeResolution,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        }
    },

    {
        {   // B2Y out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstancePropsNativeResolution,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {   // HDRT1 in
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstancePropsNativeResolution,
            HDRT1FeatureInputPortDescriptors[HDRInputPortYUV_In0_External].globalId
        }
    },
    {
        {   // B2Y metadata out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstancePropsNativeResolution,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {   // HDRT1 metadata in
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstancePropsNativeResolution,
            HDRT1FeatureInputPortDescriptors[HDRInputPortMeta_In2_Internal].globalId
        }
    },

    {
        {   // HDRT1 out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstancePropsNativeResolution,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {   // JPEG in
            JPEGFeatureDescriptor.featureId,
            JPEGInstancePropsNativeResolution,
            JPEGInputPortDescriptors[JPEGInputPortYUV].globalId
        }
    },

    {
        {   // HDRT1 metadata out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstancePropsNativeResolution,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {   // JPEG metadata in
            JPEGFeatureDescriptor.featureId,
            JPEGInstancePropsNativeResolution,
            JPEGInputPortDescriptors[JPEGInputPortMetaData].globalId
        }
    }
};

static ChiFeature2GraphExtSinkLinkDesc RTBayer2YUVHDRT1JPEGFGNativeResolutionSinkLinks[] =
{
    {
        // Display_out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortDisplay].globalId
        }
    },
    {   // FD_out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortFD].globalId
        }
    },
    {   // JPEG out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstancePropsNativeResolution,
            JPEGOutputPortDescriptors[JPEGOutputPort].globalId
        }
    },
    {   // JPEG Metadata Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstancePropsNativeResolution,
            JPEGOutputPortDescriptors[JPEGOutputPortMetaData].globalId
        }
    },
    {   // HEIC YUV Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstancePropsNativeResolution,
            JPEGOutputPortDescriptors[2].globalId
        }
    },
    {   // HEIC Thumbnail Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstancePropsNativeResolution,
            JPEGOutputPortDescriptors[3].globalId
        }
    },
};

extern const ChiFeature2GraphDesc RTBayer2YUVHDRT1JPEGNativeResolutionFGDescriptor =
{
    "RTBayer2YUVHDRT1JPEGNativeRes",                                       // Feature graph name
     CHX_ARRAY_SIZE(RTBayer2YUVHDRT1JPEGFGNativeResolutionFeatureInstanceDescs),
                                                                           // The number of feature instances
    &RTBayer2YUVHDRT1JPEGFGNativeResolutionFeatureInstanceDescs[0],
                                                                           // The array containing all
                                                                           //  feature instance descriptors
                                                                           //  for the graph
    0,                                                                     // The number of source links
    NULL,                                                                  // The array containing links
                                                                           // associated with all source ports
    CHX_ARRAY_SIZE(RTBayer2YUVHDRT1JPEGFGNativeResolutionInternalLinks),
                                                                           // The number of internal links
    &RTBayer2YUVHDRT1JPEGFGNativeResolutionInternalLinks[0],
                                                                           // The array containing links
                                                                           // associated with
                                                                           //  all internal ports
    CHX_ARRAY_SIZE(RTBayer2YUVHDRT1JPEGFGNativeResolutionSinkLinks),
                                                                           // The number of links pSinkLinks
    &RTBayer2YUVHDRT1JPEGFGNativeResolutionSinkLinks[0]
                                                                           // The array containing links
                                                                           // associated with all sink ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the RTBayer2YUVHDRT1JPEGFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the RTBayer2YUVHDRT1JPEGFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor     RealTimeFeatureDescriptor;
extern const ChiFeature2Descriptor     Bayer2YuvFeatureDescriptor;
extern const ChiFeature2Descriptor     JPEGFeatureDescriptor;
extern const ChiFeature2PortDescriptor RealTimeInputPortDescriptors[];
extern const ChiFeature2PortDescriptor RealTimeOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor Bayer2YuvInputPortDescriptors[];
extern const ChiFeature2PortDescriptor Bayer2YuvOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGInputPortDescriptors[];

static ChiFeature2InstanceDesc RTBayer2YUVHDRT1JPEGFeatureGraphFeatureInstanceDescs[] =
{
    //  RealTime feature
    {
        &RealTimeFeatureDescriptor,
        &RealTimeInstanceProps
    },
    // Bayer to YUV feature
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps
    },
    // HDR feature
    {
        &HDRT1FeatureDescriptor,
        &HDRT1InstanceProps
    },
    // JPEG feature
    {
        &JPEGFeatureDescriptor,
        &JPEGInstanceProps
    },
    // Memcpy feature
    {
        &MemcpyFeatureDescriptor,
        &MemcpyInstanceProps
    }
};

static ChiFeature2GraphInternalLinkDesc RTBayer2YUVHDRT1JPEGFeatureGraphInternalLinks[] =
{
    {
        {
            // Raw_out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortRDI].globalId
        },
        {
            // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        }
    },
    {
        {
            // B2Y out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // HDRT1 in
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[HDRInputPortYUV_In0_External].globalId
        }
    },
    {
        {
            // rt_metadata_out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortMetaData].globalId
        },
        {
            // B2Y Input Metadata
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        }
    },
    {
        {
            // B2Y metadata out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // HDRT1 metadata in
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[HDRInputPortMeta_In2_Internal].globalId
        }
    },

    {
        {
            // HDRT1 out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // JPEG in
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGInputPortDescriptors[JPEGInputPortYUV].globalId
        }
    },

    {
        {
            // HDRT1 metadata out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // JPEG metadata in
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGInputPortDescriptors[JPEGInputPortMetaData].globalId
        }
    },
    {
        {
            // Realtime FD out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortFD].globalId
        },
        {
            // Memcpy In
            MemcpyFeatureDescriptor.featureId,
            MemcpyInstanceProps,
            MemcpyInputPortDescriptors[0].globalId
        }
    }
};

static ChiFeature2GraphExtSinkLinkDesc RTBayer2YUVHDRT1JPEGFeatureGraphSinkLinks[] =
{
    {
        // Display_out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortDisplay].globalId
        }
    },
    {
        // JPEG out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[JPEGOutputPort].globalId
        }
    },
    {
        // JPEG Metadata Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[JPEGOutputPortMetaData].globalId
        }
    },
    {
        // HEIC YUV Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[2].globalId
        }
    },
    {
        // HEIC Thumbnail Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[3].globalId
        }
    },
    {
        // Memcpy yuv framework out
        {
            MemcpyFeatureDescriptor.featureId,
            MemcpyInstanceProps,
            MemcpyOutputPortDescriptors[MemcpyOutputPortFwk].globalId
        }
    }
};

extern const ChiFeature2GraphDesc RTBayer2YUVHDRT1JPEGFeatureGraphDescriptor =
{
    "RTBayer2YUVHDRT1JPEG",                                                // Feature graph name
     CHX_ARRAY_SIZE(RTBayer2YUVHDRT1JPEGFeatureGraphFeatureInstanceDescs), // The number of feature instances
    &RTBayer2YUVHDRT1JPEGFeatureGraphFeatureInstanceDescs[0],              // The array containing all
                                                                           //  feature instance descriptors
                                                                           //  for the graph
    0,                                                                     // The number of source links
    NULL,                                                                  // The array containing links
                                                                           // associated with all source ports
    CHX_ARRAY_SIZE(RTBayer2YUVHDRT1JPEGFeatureGraphInternalLinks),         // The number of internal links
    &RTBayer2YUVHDRT1JPEGFeatureGraphInternalLinks[0],                     // The array containing links
                                                                           // associated with
                                                                           //  all internal ports
    CHX_ARRAY_SIZE(RTBayer2YUVHDRT1JPEGFeatureGraphSinkLinks),             // The number of links pSinkLinks
    &RTBayer2YUVHDRT1JPEGFeatureGraphSinkLinks[0]                          // The array containing links
                                                                           // associated with all sink ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the RTBayer2YUVHDRT1JPEGFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the RTMFNRJPEGFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor     RealTimeFeatureDescriptor;
extern const ChiFeature2Descriptor     MFNRFeatureDescriptor;
extern const ChiFeature2Descriptor     JPEGFeatureDescriptor;
extern const ChiFeature2Descriptor     AnchorSyncFeatureDescriptor;
extern const ChiFeature2PortDescriptor RealTimeInputPortDescriptors[];
extern const ChiFeature2PortDescriptor RealTimeOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor MFNRPrefilterInputPortDescriptors[];
extern const ChiFeature2PortDescriptor MFNRPostFilterOutPutPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGInputPortDescriptors[];
extern const ChiFeature2PortDescriptor AnchorSyncInputPortDescriptors[];
extern const ChiFeature2PortDescriptor AnchorSyncOutputPortDescriptors[];

static ChiFeature2InstanceProps AnchorSyncInstanceProps =
{
    0,
    0
};

static ChiFeature2InstanceDesc RTMFNRJPEGFeatureGraphFeatureInstanceDescs[] =
{
    //  RealTime feature
    {
        &RealTimeFeatureDescriptor,
        &RealTimeInstanceProps
    },
    //  AnchorSync feature
    {
        &AnchorSyncFeatureDescriptor,
        &AnchorSyncInstanceProps
    },
    // MFNR feature
    {
        &MFNRFeatureDescriptor,
        &MFNRInstanceProps
    },
    // JPEG feature
    {
        &JPEGFeatureDescriptor,
        &JPEGInstanceProps
    }
};

static ChiFeature2GraphInternalLinkDesc RTMFNRJPEGFeatureGraphInternalLinks[] =
{
    {
        {
            // RAW out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortRDI].globalId
        },
        {
            // AnchorSync RDI In
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[0].globalId
        }
    },
    {
        {
            // Realtime FD out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortFD].globalId
        },
        {
            // AnchorSync FD In
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[1].globalId
        }
    },
    {
        {
            // Realtime metadata out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortMetaData].globalId
        },
        {
            // AnchorSync Metadata IN
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[2].globalId
        }
    },
    {
        {
            // AnchorSync RDI Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // MFNR in
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPrefilterInputPortDescriptorsConfig[0].globalId
        }
    },
    {
        {
            // AnchorSync Metadata Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // MFNR Input Metadata
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPrefilterInputPortDescriptorsConfig[1].globalId
        }
    },
    {
        {
            // MFNR out
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // JPEG in
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGInputPortDescriptors[0].globalId
        }
    },
    {
        {
            // YUV_Metadata_Out
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // JPEG_Input_Metadata
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGInputPortDescriptors[1].globalId
        }
    }
};

static ChiFeature2GraphExtSinkLinkDesc RTMFNRJPEGFeatureGraphSinkLinks[] =
{
    {
        // Display_out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[0].globalId
        }
    },
    {
        // JPEG out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[0].globalId
        }
    },
    {
        // JPEG Metadata Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[1].globalId
        }
    },
    {
        // HEIC YUV
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[2].globalId
        }
    },
    {
        // HEIC Thumbnail
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[3].globalId
        }
    }
};

extern const ChiFeature2GraphDesc RTMFNRJPEGFeatureGraphDescriptor =
{
    "RTMFNRJPEG",                                               // Feature graph name
    CHX_ARRAY_SIZE(RTMFNRJPEGFeatureGraphFeatureInstanceDescs), // The number of feature instances
    &RTMFNRJPEGFeatureGraphFeatureInstanceDescs[0],             // The array containing all feature instance
                                                                // descriptors for the graph
    0,                                                          // The number of source links
    NULL,                                                       // The array containing links associated with all source ports
    CHX_ARRAY_SIZE(RTMFNRJPEGFeatureGraphInternalLinks),        // The number of internal links
    &RTMFNRJPEGFeatureGraphInternalLinks[0],                    // The array containing links associated with all internal ports
    CHX_ARRAY_SIZE(RTMFNRJPEGFeatureGraphSinkLinks),            // The number of links pSinkLinks
    &RTMFNRJPEGFeatureGraphSinkLinks[0]                         // The array containing links associated with all sink ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the RTMFNRJPEGFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the RTMFSRJPEGFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor     RealTimeFeatureDescriptor;
extern const ChiFeature2Descriptor     MFSRFeatureDescriptor;
extern const ChiFeature2Descriptor     JPEGFeatureDescriptor;
extern const ChiFeature2Descriptor     AnchorSyncFeatureDescriptor;
extern const ChiFeature2PortDescriptor RealTimeInputPortDescriptors[];
extern const ChiFeature2PortDescriptor RealTimeOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor MfsrPrefilterInputPortDescriptors[];
extern const ChiFeature2PortDescriptor MfsrPostFilterOutPutPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGInputPortDescriptors[];
extern const ChiFeature2PortDescriptor AnchorSyncInputPortDescriptors[];
extern const ChiFeature2PortDescriptor AnchorSyncOutputPortDescriptors[];

static ChiFeature2InstanceDesc RTMFSRJPEGFeatureGraphFeatureInstanceDescs[] =
{
    //  RealTime feature
    {
        &RealTimeFeatureDescriptor,
        &RealTimeInstanceProps
    },
    //  AnchorSync feature
    {
        &AnchorSyncFeatureDescriptor,
        &AnchorSyncInstanceProps
    },
    // MFSR feature
    {
        &MFSRFeatureDescriptor,
        &MFSRInstanceProps
    },
    // JPEG feature
    {
        &JPEGFeatureDescriptor,
        &JPEGInstanceProps
    },
};

static ChiFeature2GraphInternalLinkDesc RTMFSRJPEGFeatureGraphInternalLinks[] =
{
    {
        {
            // RAW out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortRDI].globalId
        },
        {
            // AnchorSync RDI In
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[0].globalId
        }
    },
    {
        {
            // Realtime FD out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortFD].globalId
        },
        {
            // AnchorSync FD In
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[1].globalId
        }
    },
    {
        {
            // Realtime metadata out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortMetaData].globalId
        },

        {
            // AnchorSync Metadata IN
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[2].globalId
        }
    },
    {
        {
            // AnchorSync RDI Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // MFSR in
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPrefilterInputPortDescriptorsConfig[0].globalId
        }
    },
    {
        {
            // AnchorSync Metadata Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // MFSR Input Metadata
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPrefilterInputPortDescriptorsConfig[1].globalId
        }
    },
    {
        {
            // MFSR out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // JPEG in
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGInputPortDescriptors[JPEGInputPortYUV].globalId
        }
    },
    {
        {
            // YUV_Metadata_Out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // JPEG_Input_Metadata
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGInputPortDescriptors[JPEGInputPortMetaData].globalId
        }
    }
};

static ChiFeature2GraphExtSinkLinkDesc RTMFSRJPEGFeatureGraphSinkLinks[] =
{
    {
        // Display_out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortDisplay].globalId
        }
    },
    {
        // JPEG out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[JPEGOutputPort].globalId
        }
    },
    {
        // JPEG Metadata Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[JPEGOutputPortMetaData].globalId
        }
    },
    {
        // HEIC YUV Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[2].globalId
        }
    },
    {
        // HEIC Thumbnail Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[3].globalId
        }
    }
};

extern const ChiFeature2GraphDesc RTMFSRJPEGFeatureGraphDescriptor =
{
    "RTMFSRJPEG",                                               // Feature graph name
    CHX_ARRAY_SIZE(RTMFSRJPEGFeatureGraphFeatureInstanceDescs), // The number of feature instances
    &RTMFSRJPEGFeatureGraphFeatureInstanceDescs[0],             // The array containing all feature instance
                                                                // descriptors for the graph
    0,                                                          // The number of source links
    NULL,                                                       // The array containing links associated with all source ports
    CHX_ARRAY_SIZE(RTMFSRJPEGFeatureGraphInternalLinks),        // The number of internal links
    &RTMFSRJPEGFeatureGraphInternalLinks[0],                    // The array containing links associated with all internal ports
    CHX_ARRAY_SIZE(RTMFSRJPEGFeatureGraphSinkLinks),            // The number of links pSinkLinks
    &RTMFSRJPEGFeatureGraphSinkLinks[0]                         // The array containing links associated with all sink ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the RTMFSRJPEGFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the RTMFSRHDRT1JPEGFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor     RealTimeFeatureDescriptor;
extern const ChiFeature2Descriptor     AnchorSyncFeatureDescriptor;
extern const ChiFeature2Descriptor     DemuxFeatureDescriptor;
extern const ChiFeature2Descriptor     Bayer2YuvFeatureDescriptor;
extern const ChiFeature2Descriptor     MFSRFeatureDescriptor;
extern const ChiFeature2Descriptor     SerializerFeatureDescriptor;
extern const ChiFeature2Descriptor     HDRT1FeatureDescriptor;
extern const ChiFeature2Descriptor     JPEGFeatureDescriptor;

extern const ChiFeature2PortDescriptor RealTimeInputPortDescriptors[];
extern const ChiFeature2PortDescriptor RealTimeOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor AnchorSyncInputPortDescriptors[];
extern const ChiFeature2PortDescriptor AnchorSyncOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor DemuxInputPortDescriptors[];
extern const ChiFeature2PortDescriptor DemuxOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor Bayer2YuvInputPortDescriptors[];
extern const ChiFeature2PortDescriptor Bayer2YuvOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor MfsrPrefilterInputPortDescriptors[];
extern const ChiFeature2PortDescriptor MfsrPostFilterOutPutPortDescriptors[];
extern const ChiFeature2PortDescriptor SerializerInputPortDescriptors[];
extern const ChiFeature2PortDescriptor SerializerOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor HDRT1FeatureInputPortDescriptors[];
extern const ChiFeature2PortDescriptor HDRT1FeatureOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGInputPortDescriptors[];

static ChiFeature2InstanceProps DemuxInstanceProps =
{
    0,
    0
};

static ChiFeature2InstanceProps SerializerInstanceProps =
{
    0,
    0
};

static ChiFeature2InstanceDesc RTMFNRHDRT1JPEGFeatureGraphFeatureInstanceDescs[] =
{
    // 1.RealTime feature
    {
        &RealTimeFeatureDescriptor,
        &RealTimeInstanceProps
    },
    // 2.AnchorSync feature
    {
        &AnchorSyncFeatureDescriptor,
        &AnchorSyncInstanceProps
    },
    // 3.Demux feature
    {
        &DemuxFeatureDescriptor,
        &DemuxInstanceProps
    },
    // 4.MFSR feature
    {
        &MFNRFeatureDescriptor,
        &MFNRInstanceProps
    },
    // 5.Bayer2Yuv feature
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps
    },
    // 6.Serializer feature
    {
        &SerializerFeatureDescriptor,
        &SerializerInstanceProps
    },
    // 7.HDR feature
    {
        &HDRT1FeatureDescriptor,
        &HDRT1InstanceProps
    },
    // 8.JPEG feature
    {
        &JPEGFeatureDescriptor,
        &JPEGInstanceProps
    },
};

static ChiFeature2GraphInternalLinkDesc RTMFNRHDRT1JPEGFeatureGraphInternalLinks[] =
{
    // 1.Realtime-AnchorSync RDI link
    {
        {
            // RAW out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortRDI].globalId
        },
        {
            // AnchorSync RDI In
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[0].globalId
        }
    },
    // 2.Realtime-AnchorSync FD link
    {
        {
            // Realtime FD out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortFD].globalId
        },
        {
            // AnchorSync FD In
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[1].globalId
        }
    },
    // 3.Realtime-AnchorSync Metadata link
    {
        {
            // Realtime metadata out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortMetaData].globalId
        },

        {
            // AnchorSync Metadata IN
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[2].globalId
        }
    },
    // 4.AnchorSync-Demux RDI link
    {
        {
            // AnchorSync RDI Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // Demux in
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxInputPortDescriptors[0].globalId
        }
    },
    // 5.AnchorSync-Demux Metadata link
    {
        {
            // AnchorSync Metadata Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // Demux Input Metadata
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxInputPortDescriptors[1].globalId
        }
    },
    // 6.Demux-MFSR RDI link
    {
        {
            // Demux RDI-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[0].globalId
        },
        {
            // MFSR RDI Input
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPrefilterInputPortDescriptorsConfig[0].globalId
        }
    },
    // 7.Demux-MFSR Metadata link
    {
        {
            // Demux Metadata-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[1].globalId
        },
        {
            // MFSR Input Metadata
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPrefilterInputPortDescriptorsConfig[1].globalId
        }
    },
    // 8.Demux-Bayer2Yuv RDI link
    {
        {
            // Demux RDI-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[2].globalId
        },
        {
            // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        }
    },
    // 9.Demux-Bayer2Yuv Metadata link
    {
        {
            // Demux Metadata-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[3].globalId
        },
        {
            // B2Y Metadata in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        }
    },
    // 10.MFSR-Serializer YUV link
    {
        {
            // MFSR YUV Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // Serializer YUV-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[0].globalId
        }
    },
    // 11.MFSR-Serializer Metadata link
    {
        {
            // MFSR Metadata Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Serializer Metadata-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[1].globalId
        }
    },
    // 12.Bayer2Yuv-Serializer YUV link
    {
        {
            // Bayer2Yuv Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // Serializer YUV-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[2].globalId
        }
    },
    // 13.Bayer2Yuv-Serializer Metadata link
    {
        {
            // Bayer2Yuv Metadata Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Serializer Metadata-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[3].globalId
        }
    },
    // 14.Serializer-HDR YUV link
    {
        {
            // Serializer YUV Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerOutputPortDescriptors[0].globalId
        },
        {
            // HDRT1 YUV Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[HDRInputPortYUV_In0_External].globalId
        }
    },
    // 15.Serializer-HDR Metadata link
    {
        {
            // Serializer Metadata Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerOutputPortDescriptors[1].globalId
        },
        {
            // HDRT1 Metadata Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[HDRInputPortMeta_In2_Internal].globalId
        }
    },
    // 16.HDR-JPEG YUV link
    {
        {
            // HDRT1 YUV Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // JPEG YUV Input
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGInputPortDescriptors[JPEGInputPortYUV].globalId
        }
    },
    // 17.HDR-JPEG Metadata link
    {
        {
            // HDRT1 Metadata Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // JPEG Metadata Input
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGInputPortDescriptors[JPEGInputPortMetaData].globalId
        }
    }
};

static ChiFeature2GraphExtSinkLinkDesc RTMFNRHDRT1JPEGFeatureGraphSinkLinks[] =
{
    {
        // Display_out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortDisplay].globalId
        }
    },
    {
        // JPEG out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[JPEGOutputPort].globalId
        }
    },
    {
        // JPEG Metadata Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[JPEGOutputPortMetaData].globalId
        }
    },
    {
        // HEIC YUV Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[2].globalId
        }
    },
    {
        // HEIC Thumbnail Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[3].globalId
        }
    }
};

extern const ChiFeature2GraphDesc RTMFNRHDRT1JPEGFeatureGraphDescriptor =
{
    "RTMFNRHDRT1JPEG",                                                  // Feature graph name
    CHX_ARRAY_SIZE(RTMFNRHDRT1JPEGFeatureGraphFeatureInstanceDescs),    // The number of feature instances
    &RTMFNRHDRT1JPEGFeatureGraphFeatureInstanceDescs[0],                // The array containing all feature instance
                                                                        // descriptors for the graph
    0,                                                                  // The number of source links
    NULL,                                                               // The array containing links for all source ports
    CHX_ARRAY_SIZE(RTMFNRHDRT1JPEGFeatureGraphInternalLinks),           // The number of internal links
    &RTMFNRHDRT1JPEGFeatureGraphInternalLinks[0],                       // The array containing links for all internal ports
    CHX_ARRAY_SIZE(RTMFNRHDRT1JPEGFeatureGraphSinkLinks),               // The number of links pSinkLinks
    &RTMFNRHDRT1JPEGFeatureGraphSinkLinks[0]                            // The array containing links for all sink ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the RTMFSRHDRT1JPEGFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


static ChiFeature2InstanceDesc RTMFSRHDRT1JPEGFeatureGraphFeatureInstanceDescs[] =
{
    // 1.RealTime feature
    {
        &RealTimeFeatureDescriptor,
        &RealTimeInstanceProps
    },
    // 2.AnchorSync feature
    {
        &AnchorSyncFeatureDescriptor,
        &AnchorSyncInstanceProps
    },
    // 3.Demux feature
    {
        &DemuxFeatureDescriptor,
        &DemuxInstanceProps
    },
    // 4.MFSR feature
    {
        &MFSRFeatureDescriptor,
        &MFSRInstanceProps
    },
    // 5.Bayer2Yuv feature
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps
    },
    // 6.Serializer feature
    {
        &SerializerFeatureDescriptor,
        &SerializerInstanceProps
    },
    // 7.HDR feature
    {
        &HDRT1FeatureDescriptor,
        &HDRT1InstanceProps
    },
    // 8.JPEG feature
    {
        &JPEGFeatureDescriptor,
        &JPEGInstanceProps
    },
};

static ChiFeature2GraphInternalLinkDesc RTMFSRHDRT1JPEGFeatureGraphInternalLinks[] =
{
    // 1.Realtime-AnchorSync RDI link
    {
        {
            // RAW out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortRDI].globalId
        },
        {
            // AnchorSync RDI In
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[0].globalId
        }
    },
    // 2.Realtime-AnchorSync FD link
    {
        {
            // Realtime FD out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortFD].globalId
        },
        {
            // AnchorSync FD In
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[1].globalId
        }
    },
    // 3.Realtime-AnchorSync Metadata link
    {
        {
            // Realtime metadata out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortMetaData].globalId
        },

        {
            // AnchorSync Metadata IN
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[2].globalId
        }
    },
    // 4.AnchorSync-Demux RDI link
    {
        {
            // AnchorSync RDI Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // Demux in
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxInputPortDescriptors[0].globalId
        }
    },
    // 5.AnchorSync-Demux Metadata link
    {
        {
            // AnchorSync Metadata Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // Demux Input Metadata
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxInputPortDescriptors[1].globalId
        }
    },
    // 6.Demux-MFSR RDI link
    {
        {
            // Demux RDI-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[0].globalId
        },
        {
            // MFSR RDI Input
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPrefilterInputPortDescriptorsConfig[0].globalId
        }
    },
    // 7.Demux-MFSR Metadata link
    {
        {
            // Demux Metadata-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[1].globalId
        },
        {
            // MFSR Input Metadata
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPrefilterInputPortDescriptorsConfig[1].globalId
        }
    },
    // 8.Demux-Bayer2Yuv RDI link
    {
        {
            // Demux RDI-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[2].globalId
        },
        {
            // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        }
    },
    // 9.Demux-Bayer2Yuv Metadata link
    {
        {
            // Demux Metadata-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[3].globalId
        },
        {
            // B2Y Metadata in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        }
    },
    // 10.MFSR-Serializer YUV link
    {
        {
            // MFSR YUV Output
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // Serializer YUV-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[0].globalId
        }
    },
    // 11.MFSR-Serializer Metadata link
    {
        {
            // MFSR Metadata Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Serializer Metadata-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[1].globalId
        }
    },
    // 12.Bayer2Yuv-Serializer YUV link
    {
        {
            // Bayer2Yuv Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // Serializer YUV-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[2].globalId
        }
    },
    // 13.Bayer2Yuv-Serializer Metadata link
    {
        {
            // Bayer2Yuv Metadata Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Serializer Metadata-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[3].globalId
        }
    },
    // 14.Serializer-HDR YUV link
    {
        {
            // Serializer YUV Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerOutputPortDescriptors[0].globalId
        },
        {
            // HDRT1 YUV Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[HDRInputPortYUV_In0_External].globalId
        }
    },
    // 15.Serializer-HDR Metadata link
    {
        {
            // Serializer Metadata Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerOutputPortDescriptors[1].globalId
        },
        {
            // HDRT1 Metadata Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[HDRInputPortMeta_In2_Internal].globalId
        }
    },
    // 16.HDR-JPEG YUV link
    {
        {
            // HDRT1 YUV Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // JPEG YUV Input
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGInputPortDescriptors[JPEGInputPortYUV].globalId
        }
    },
    // 17.HDR-JPEG Metadata link
    {
        {
            // HDRT1 Metadata Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // JPEG Metadata Input
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGInputPortDescriptors[JPEGInputPortMetaData].globalId
        }
    }
};

static ChiFeature2GraphExtSinkLinkDesc RTMFSRHDRT1JPEGFeatureGraphSinkLinks[] =
{
    {
        // Display_out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortDisplay].globalId
        }
    },
    {
        // JPEG out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[JPEGOutputPort].globalId
        }
    },
    {
        // JPEG Metadata Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[JPEGOutputPortMetaData].globalId
        }
    },
    {
        // HEIC YUV Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[2].globalId
        }
    },
    {
        // HEIC Thumbnail Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[3].globalId
        }
    }
};

extern const ChiFeature2GraphDesc RTMFSRHDRT1JPEGFeatureGraphDescriptor =
{
    "RTMFSRHDRT1JPEG",                                                  // Feature graph name
    CHX_ARRAY_SIZE(RTMFSRHDRT1JPEGFeatureGraphFeatureInstanceDescs),    // The number of feature instances
    &RTMFSRHDRT1JPEGFeatureGraphFeatureInstanceDescs[0],                // The array containing all feature instance
                                                                        // descriptors for the graph
    0,                                                                  // The number of source links
    NULL,                                                               // The array containing links for all source ports
    CHX_ARRAY_SIZE(RTMFSRHDRT1JPEGFeatureGraphInternalLinks),           // The number of internal links
    &RTMFSRHDRT1JPEGFeatureGraphInternalLinks[0],                       // The array containing links for all internal ports
    CHX_ARRAY_SIZE(RTMFSRHDRT1JPEGFeatureGraphSinkLinks),               // The number of links pSinkLinks
    &RTMFSRHDRT1JPEGFeatureGraphSinkLinks[0]                            // The array containing links for all sink ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the RTMFSRHDRT1JPEGFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the RTBayer2YUVSWMFJPEGFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor     RealTimeFeatureDescriptor;
extern const ChiFeature2Descriptor     Bayer2YuvFeatureDescriptor;
extern const ChiFeature2Descriptor     JPEGFeatureDescriptor;
extern const ChiFeature2Descriptor     SWMFFeatureDescriptor;
extern const ChiFeature2PortDescriptor RealTimeInputPortDescriptors[];
extern const ChiFeature2PortDescriptor RealTimeOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor Bayer2YuvInputPortDescriptors[];
extern const ChiFeature2PortDescriptor Bayer2YuvOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGInputPortDescriptors[];
extern const ChiFeature2PortDescriptor SWMFFeatureInputPortDescriptors[];
extern const ChiFeature2PortDescriptor SWMFFeatureOutputPortDescriptors[];

static ChiFeature2InstanceProps SWMFInstanceProps =
{
    0,
    0
};

static ChiFeature2InstanceDesc RTBayer2YUVSWMFJPEGFeatureGraphFeatureInstanceDescs[] =
{
    //  RealTime feature
    {
        &RealTimeFeatureDescriptor,
        &RealTimeInstanceProps
    },
    // Bayer to YUV feature
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps
    },
    // HDR feature
    {
        &SWMFFeatureDescriptor,
        &SWMFInstanceProps
    },
    // JPEG feature
    {
        &JPEGFeatureDescriptor,
        &JPEGInstanceProps
    },
    // Memcpy feature
    {
        &MemcpyFeatureDescriptor,
        &MemcpyInstanceProps
    }
};

static ChiFeature2GraphInternalLinkDesc RTBayer2YUVSWMFJPEGFeatureGraphInternalLinks[] =
{
    {
        {
            // RAW out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortRDI].globalId
        },
        {
            // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        }
    },
    {
        {
            // rt_metadata_out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortMetaData].globalId
        },
        {
            // B2Y Input Metadata
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        }
    },
    {
        {
            // B2Y out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // SWMF in
            SWMFFeatureDescriptor.featureId,
            SWMFInstanceProps,
            SWMFFeatureInputPortDescriptors[SWMFInputPortYUV_In0_External].globalId
        }
    },
    {
        {
            // B2Y metadata out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // SWMF metadata in
            SWMFFeatureDescriptor.featureId,
            SWMFInstanceProps,
            SWMFFeatureInputPortDescriptors[SWMFInputPortMeta_In2_Internal].globalId
        }
    },

    {
        {
            // SWMF out
            SWMFFeatureDescriptor.featureId,
            SWMFInstanceProps,
            SWMFFeatureOutputPortDescriptors[SWMFOutputPortYUVOut].globalId
        },
        {
            // JPEG in
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGInputPortDescriptors[JPEGInputPortYUV].globalId
        }
    },

    {
        {
            // SWMF metadata out
            SWMFFeatureDescriptor.featureId,
            SWMFInstanceProps,
            SWMFFeatureOutputPortDescriptors[SWMFOutputPortMetaData].globalId
        },
        {
            // JPEG metadata in
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGInputPortDescriptors[JPEGInputPortMetaData].globalId
        }
    },
    {
        {
            // Realtime FD out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortFD].globalId
        },
        {
            // Memcpy In
            MemcpyFeatureDescriptor.featureId,
            MemcpyInstanceProps,
            MemcpyInputPortDescriptors[0].globalId
        }
    }
};

static ChiFeature2GraphExtSinkLinkDesc RTBayer2YUVSWMFJPEGFeatureGraphSinkLinks[] =
{
    {
        // Display_out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortDisplay].globalId
        }
    },
    {
        // JPEG out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[JPEGOutputPort].globalId
        }
    },
    {   // JPEG Metadata Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[JPEGOutputPortMetaData].globalId
        }
    },
    {
        // HEIC YUV
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[2].globalId
        }
    },
    {
        // HEIC Thumbnail
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[3].globalId
        }
    },
    {
        // Memcpy yuv framework out
        {
            MemcpyFeatureDescriptor.featureId,
            MemcpyInstanceProps,
            MemcpyOutputPortDescriptors[MemcpyOutputPortFwk].globalId
        }
    }
};

extern const ChiFeature2GraphDesc RTBayer2YUVSWMFJPEGFeatureGraphDescriptor =
{
    "RTBayer2YUVSWMFJPEG",                                                // Feature graph name
     CHX_ARRAY_SIZE(RTBayer2YUVSWMFJPEGFeatureGraphFeatureInstanceDescs), // The number of feature instances
    &RTBayer2YUVSWMFJPEGFeatureGraphFeatureInstanceDescs[0],              // The array containing all
                                                                           //  feature instance descriptors
                                                                           //  for the graph
    0,                                                                     // The number of source links
    NULL,                                                                  // The array containing links
                                                                           // associated with all source ports
    CHX_ARRAY_SIZE(RTBayer2YUVSWMFJPEGFeatureGraphInternalLinks),         // The number of internal links
    &RTBayer2YUVSWMFJPEGFeatureGraphInternalLinks[0],                     // The array containing links
                                                                           // associated with
                                                                           //  all internal ports
    CHX_ARRAY_SIZE(RTBayer2YUVSWMFJPEGFeatureGraphSinkLinks),             // The number of links pSinkLinks
    &RTBayer2YUVSWMFJPEGFeatureGraphSinkLinks[0]                          // The array containing links
                                                                           // associated with all sink ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the RTBayer2YUVSWMFJPEGFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the MultiCameraMFNRFusionFeatureGraphDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static ChiFeature2InstanceDesc MultiCameraMFNRFusionFeatureGraphFeatureInstanceDescs[] =
{
    //  AnchorSync feature
    {
        &AnchorSyncFeatureDescriptor,
        &AnchorSyncInstanceProps
    },
    {
        &MFSRFeatureDescriptor,
        &MFSRInstanceProps
    },
    {
        &MFSRFeatureDescriptor,
        &MFSRInstanceProps1
    },
    {
        &MFSRFeatureDescriptor,
        &MFSRInstanceProps2
    },
    {
        &FusionFeatureDescriptor,
        &FusionFeatureInstanceProps
    }
};

static ChiFeature2GraphExtSrcLinkDesc MultiCameraMFNRFusionFeatureGraphSrcLinks[]=
{
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[0].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[1].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[2].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[3].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[4].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[5].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[6].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[7].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[8].globalId
        }
    }
};

static ChiFeature2GraphInternalLinkDesc MultiCameraMFNRFusionFeatureGraphInternalLinks[] =
{
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // RDI_IN
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPrefilterInputPortDescriptorsConfig[0].globalId
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPrefilterInputPortDescriptorsConfig[1].globalId
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[2].globalId
        },
        {
            // RDI_IN
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPrefilterInputPortDescriptorsConfig[0].globalId
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[3].globalId
        },
        {
            // Meta_IN
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPrefilterInputPortDescriptorsConfig[1].globalId
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[4].globalId
        },
        {
            // RDI_IN
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPrefilterInputPortDescriptorsConfig[0].globalId
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[5].globalId
        },
        {
            // Meta_IN
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPrefilterInputPortDescriptorsConfig[1].globalId
        }
    },
    {
        {
            // YUV_out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[0].globalId
        }
    },
    {
        {
            // Meta_out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[1].globalId
        }
    },
    {
        {
            // YUV_out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[2].globalId
        }
    },
    {
        {
            // Meta_out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[3].globalId
        }
    },
    {
        {
            // YUV_out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[4].globalId
        }
    },
    {
        {
            // Meta_out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[5].globalId
        }
    }
};

static ChiFeature2GraphExtSinkLinkDesc MultiCameraMFNRFusionFeatureGraphSinkLinks[]=
{
    {
        {
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionOutputPortDescriptors[0].globalId
        }
    },
    {
        {
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionOutputPortDescriptors[1].globalId
        }
    },
};

extern const ChiFeature2GraphDesc MultiCameraMFNRFusionFeatureGraphDescriptor =
{
    "MultiCameraMFNRFusionGraph",                                           // Feature graph name
    CHX_ARRAY_SIZE(MultiCameraMFNRFusionFeatureGraphFeatureInstanceDescs),  // The number of feature instances
    &MultiCameraMFNRFusionFeatureGraphFeatureInstanceDescs[0],              // The array containing all feature instance
    CHX_ARRAY_SIZE(MultiCameraMFNRFusionFeatureGraphSrcLinks),              // The number of source links
    &MultiCameraMFNRFusionFeatureGraphSrcLinks[0],                          // The array of all source links
    CHX_ARRAY_SIZE(MultiCameraMFNRFusionFeatureGraphInternalLinks),         // The number of internal links
    &MultiCameraMFNRFusionFeatureGraphInternalLinks[0],                     // The array of all internal links
    CHX_ARRAY_SIZE(MultiCameraMFNRFusionFeatureGraphSinkLinks),             // The number of links pSinkLinks
    &MultiCameraMFNRFusionFeatureGraphSinkLinks[0],                         // The array of all sink links
    TRUE
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the MultiCameraMFNRFusionFeatureGraphDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the RTBayer2YUVFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor     RealTimeFeatureDescriptor;
extern const ChiFeature2Descriptor     Bayer2YuvFeatureDescriptor;
extern const ChiFeature2PortDescriptor RealTimeInputPortDescriptors[];
extern const ChiFeature2PortDescriptor RealTimeOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor Bayer2YuvInputPortDescriptors[];
extern const ChiFeature2PortDescriptor Bayer2YuvOutputPortDescriptors[];


static ChiFeature2InstanceDesc RTBayer2YUVFeatureGraphFeatureInstanceDescs[] =
{
    //  RealTime feature
    {
        &RealTimeFeatureDescriptor,
        &RealTimeInstanceProps
    },
    // Bayer to YUV feature
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps
    },
};

static ChiFeature2GraphInternalLinkDesc RTBayer2YUVFeatureGraphInternalLinks[] =
{
    {
        // Raw_out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortRDI].globalId
        },
        // B2Y in
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        }
    },
    {
        // rt_metadata_out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortMetaData].globalId
        },
        // B2Y Input Metadata
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        }
    },
};

static ChiFeature2GraphExtSinkLinkDesc RTBayer2YUVFeatureGraphSinkLinks[] =
{
    {
        // Display out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortDisplay].globalId
        }
    },
    {
        // FD out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortFD].globalId
        }
    },
    {
        // B2Y out
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
    },
    {
        // B2Y Metadata Out
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
    },
};

extern const ChiFeature2GraphDesc RTBayer2YUVFeatureGraphDescriptor =
{
    "RTBayer2YUV",                                               // Feature graph name
    CHX_ARRAY_SIZE(RTBayer2YUVFeatureGraphFeatureInstanceDescs), // The number of feature instances
    &RTBayer2YUVFeatureGraphFeatureInstanceDescs[0],             // The array containing all feature instance
                                                                     // descriptors for the graph
    0,                                                               // The number of source links
    NULL,                                                            // The array containing links associated
                                                                     // with all source ports
    CHX_ARRAY_SIZE(RTBayer2YUVFeatureGraphInternalLinks),        // The number of internal links
    &RTBayer2YUVFeatureGraphInternalLinks[0],                    // The array containing links associated
                                                                     // with all internal ports
    CHX_ARRAY_SIZE(RTBayer2YUVFeatureGraphSinkLinks),            // The number of links pSinkLinks
    &RTBayer2YUVFeatureGraphSinkLinks[0]                         // The array containing links associated
                                                                     // with all sink ports
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the MultiCameraBokehFeatureSuperGraphDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor Bayer2YuvFeatureDescriptor;
extern const ChiFeature2PortDescriptor Bayer2YuvInputPortDescriptors[];
extern const ChiFeature2PortDescriptor Bayer2YuvOutputPortDescriptors[];
extern const ChiFeature2Descriptor BokehFeatureDescriptor;
extern const ChiFeature2PortDescriptor BokehInputPortDescriptors[];
extern const ChiFeature2PortDescriptor BokehOutputPortDescriptors[];

static ChiFeature2InstanceProps BokehFeatureInstanceProps =
{
    0,
    0
};

static ChiFeature2InstanceProps HDRT1InstanceProps1 =
{
    1,
    1
};

static ChiFeature2InstanceProps HDRT1InstanceProps2 =
{
    2,
    2
};


static ChiFeature2InstanceProps DemuxInstanceProps1 =
{
    1,
    1
};

static ChiFeature2InstanceProps SerializerInstanceProps1 =
{
    1,
    1
};

static ChiFeature2InstanceProps DemuxInstanceProps2 =
{
    2,
    2
};

static ChiFeature2InstanceProps SerializerInstanceProps2 =
{
    2,
    2
};

static ChiFeature2InstanceDesc MultiCameraBokehFeatureSuperGraphFeatureInstanceDescs[] =
{
    //  AnchorSync feature
    {
        &AnchorSyncFeatureDescriptor,
        &AnchorSyncInstanceProps
    },
    {
        &MFSRFeatureDescriptor,
        &MFSRInstanceProps
    },
    {
        &MFSRFeatureDescriptor,
        &MFSRInstanceProps1
    },
    {
        &MFSRFeatureDescriptor,
        &MFSRInstanceProps2
    },
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps
    },
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps1
    },
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps2
    },
    {
        &HDRT1FeatureDescriptor,
        &HDRT1InstanceProps
    },
    {
        &HDRT1FeatureDescriptor,
        &HDRT1InstanceProps1
    },
    {
        &HDRT1FeatureDescriptor,
        &HDRT1InstanceProps2
    },
    {
        &BokehFeatureDescriptor,
        &BokehFeatureInstanceProps
    },
    {
        &DemuxFeatureDescriptor,
        &DemuxInstanceProps
    },
    {
        &SerializerFeatureDescriptor,
        &SerializerInstanceProps
    },
    {
        &DemuxFeatureDescriptor,
        &DemuxInstanceProps1
    },
    {
        &SerializerFeatureDescriptor,
        &SerializerInstanceProps1
    },
    {
        &DemuxFeatureDescriptor,
        &DemuxInstanceProps2
    },
    {
        &SerializerFeatureDescriptor,
        &SerializerInstanceProps2
    },
};

static ChiFeature2GraphExtSrcLinkDesc MultiCameraBokehFeatureSuperGraphSrcLinks[]=
{
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[0].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[1].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[2].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[3].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[4].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[5].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[6].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[7].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[8].globalId
        }
    }
};

static ChiFeature2GraphInternalLinkDesc MultiCameraBokehFeatureSuperGraphInternalLinks[] =
{
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // RDI_IN
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[2].globalId
        },
        {
            // RDI_IN
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[3].globalId
        },
        {
            // Meta_IN
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[4].globalId
        },
        {
            // RDI_IN
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[5].globalId
        },
        {
            // Meta_IN
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // YUV_out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Meta_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // YUV_out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Meta_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // YUV_out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Meta_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[5].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[2].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[3].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[4].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[5].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // YUV_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Meta_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // YUV_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Meta_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // YUV_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Meta_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[5].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[2].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[3].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[4].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[5].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[0].globalId
        },
        {
            // YUV_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[0].globalId
        },
        {
            // YUV_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[0].globalId
        },
        {
            // YUV_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // Meta_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // Meta_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // Meta_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[5].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },

    // camera 0
    // 4.AnchorSync-Demux RDI link
    {
        {
            // AnchorSync RDI Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // Demux in
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }

    },
    // 5.AnchorSync-Demux Metadata link
    {
        {
            // AnchorSync Metadata Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // Demux Input Metadata
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 6.Demux-MFSR RDI link
    {
        {
            // Demux RDI-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[0].globalId
        },
        {
            // MFSR RDI Input
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 7.Demux-MFSR Metadata link
    {
        {
            // Demux Metadata-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[1].globalId
        },
        {
            // MFSR Input Metadata
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 8.Demux-Bayer2Yuv RDI link
    {
        {
            // Demux RDI-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[2].globalId
        },
        {
            // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 9.Demux-Bayer2Yuv Metadata link
    {
        {
            // Demux Metadata-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[3].globalId
        },
        {
            // B2Y Metadata in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 10.MFSR-Serializer YUV link
    {
        {
            // MFSR YUV Output
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // Serializer YUV-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 11.MFSR-Serializer Metadata link
    {
        {
            // MFSR Metadata Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Serializer Metadata-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 12.Bayer2Yuv-Serializer YUV link
    {
        {
            // Bayer2Yuv Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // Serializer YUV-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 13.Bayer2Yuv-Serializer Metadata link
    {
        {
            // Bayer2Yuv Metadata Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Serializer Metadata-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 14.Serializer-HDR YUV link
    {
        {
            // Serializer YUV Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerOutputPortDescriptors[0].globalId
        },
        {
            // HDRT1 YUV Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[HDRInputPortYUV_In0_External].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 15.Serializer-HDR Metadata link
    {
        {
            // Serializer Metadata Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerOutputPortDescriptors[1].globalId
        },
        {
            // HDRT1 Metadata Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[HDRInputPortMeta_In2_Internal].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 16.HDR-Bokeh YUV link
    {
        {
            // HDRT1 YUV Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFNR_HDR,
        }

    },
    // 17.HDR-Bokeh Metadata link
    {
        {
            // HDRT1 Metadata Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // camera 1
    // 4.AnchorSync-Demux RDI link
    {
        {
            // AnchorSync RDI Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[2].globalId
        },
        {
            // Demux in
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }

    },
    // 5.AnchorSync-Demux Metadata link
    {
        {
            // AnchorSync Metadata Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[3].globalId
        },
        {
            // Demux Input Metadata
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 6.Demux-MFSR RDI link
    {
        {
            // Demux RDI-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxOutputPortDescriptors[0].globalId
        },
        {
            // MFSR RDI Input
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 7.Demux-MFSR Metadata link
    {
        {
            // Demux Metadata-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxOutputPortDescriptors[1].globalId
        },
        {
            // MFSR Input Metadata
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 8.Demux-Bayer2Yuv RDI link
    {
        {
            // Demux RDI-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxOutputPortDescriptors[2].globalId
        },
        {
            // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 9.Demux-Bayer2Yuv Metadata link
    {
        {
            // Demux Metadata-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxOutputPortDescriptors[3].globalId
        },
        {
            // B2Y Metadata in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 10.MFSR-Serializer YUV link
    {
        {
            // MFSR YUV Output
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // Serializer YUV-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 11.MFSR-Serializer Metadata link
    {
        {
            // MFSR Metadata Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Serializer Metadata-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 12.Bayer2Yuv-Serializer YUV link
    {
        {
            // Bayer2Yuv Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // Serializer YUV-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 13.Bayer2Yuv-Serializer Metadata link
    {
        {
            // Bayer2Yuv Metadata Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Serializer Metadata-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 14.Serializer-HDR YUV link
    {
        {
            // Serializer YUV Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerOutputPortDescriptors[0].globalId
        },
        {
            // HDRT1 YUV Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureInputPortDescriptors[HDRInputPortYUV_In0_External].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 15.Serializer-HDR Metadata link
    {
        {
            // Serializer Metadata Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerOutputPortDescriptors[1].globalId
        },
        {
            // HDRT1 Metadata Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureInputPortDescriptors[HDRInputPortMeta_In2_Internal].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 16.HDR-Bokeh YUV link
    {
        {
            // HDRT1 YUV Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFNR_HDR,
        }

    },
    // 17.HDR-Bokeh Metadata link
    {
        {
            // HDRT1 Metadata Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },

    // camera 2
    // 4.AnchorSync-Demux RDI link
    {
        {
            // AnchorSync RDI Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[4].globalId
        },
        {
            // Demux in
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }

    },
    // 5.AnchorSync-Demux Metadata link
    {
        {
            // AnchorSync Metadata Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[5].globalId
        },
        {
            // Demux Input Metadata
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 6.Demux-MFSR RDI link
    {
        {
            // Demux RDI-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxOutputPortDescriptors[0].globalId
        },
        {
            // MFSR RDI Input
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 7.Demux-MFSR Metadata link
    {
        {
            // Demux Metadata-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxOutputPortDescriptors[1].globalId
        },
        {
            // MFSR Input Metadata
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 8.Demux-Bayer2Yuv RDI link
    {
        {
            // Demux RDI-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxOutputPortDescriptors[2].globalId
        },
        {
            // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 9.Demux-Bayer2Yuv Metadata link
    {
        {
            // Demux Metadata-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxOutputPortDescriptors[3].globalId
        },
        {
            // B2Y Metadata in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 10.MFSR-Serializer YUV link
    {
        {
            // MFSR YUV Output
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // Serializer YUV-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 11.MFSR-Serializer Metadata link
    {
        {
            // MFSR Metadata Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Serializer Metadata-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 12.Bayer2Yuv-Serializer YUV link
    {
        {
            // Bayer2Yuv Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // Serializer YUV-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 13.Bayer2Yuv-Serializer Metadata link
    {
        {
            // Bayer2Yuv Metadata Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Serializer Metadata-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 14.Serializer-HDR YUV link
    {
        {
            // Serializer YUV Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerOutputPortDescriptors[0].globalId
        },
        {
            // HDRT1 YUV Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureInputPortDescriptors[HDRInputPortYUV_In0_External].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 15.Serializer-HDR Metadata link
    {
        {
            // Serializer Metadata Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerOutputPortDescriptors[1].globalId
        },
        {
            // HDRT1 Metadata Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureInputPortDescriptors[HDRInputPortMeta_In2_Internal].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 16.HDR-Bokeh YUV link
    {
        {
            // HDRT1 YUV Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFNR_HDR,
        }

    },
    // 17.HDR-Bokeh Metadata link
    {
        {
            // HDRT1 Metadata Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[5].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFNR_HDR,
        }
    }
};

static ChiFeature2GraphExtSinkLinkDesc MultiCameraBokehFeatureSuperGraphSinkLinks[]=
{
    {
        {
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehOutputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::BOKEH,
        }
    },
    {
        {
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehOutputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::BOKEH,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    }
};

extern const ChiFeature2GraphDesc MultiCameraBokehFeatureSuperGraphDescriptor =
{
    "MultiCameraMFNRBokehSuperGraph",                                       // Feature graph name
    CHX_ARRAY_SIZE(MultiCameraBokehFeatureSuperGraphFeatureInstanceDescs),  // The number of feature instances
    &MultiCameraBokehFeatureSuperGraphFeatureInstanceDescs[0],              // The array containing all feature instance
    CHX_ARRAY_SIZE(MultiCameraBokehFeatureSuperGraphSrcLinks),              // The number of source links
    &MultiCameraBokehFeatureSuperGraphSrcLinks[0],                          // The array of all source links
    CHX_ARRAY_SIZE(MultiCameraBokehFeatureSuperGraphInternalLinks),         // The number of internal links
    &MultiCameraBokehFeatureSuperGraphInternalLinks[0],                     // The array of all internal links
    CHX_ARRAY_SIZE(MultiCameraBokehFeatureSuperGraphSinkLinks),             // The number of links pSinkLinks
    &MultiCameraBokehFeatureSuperGraphSinkLinks[0],                         // The array of all sink links
    TRUE
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the MultiCameraMFNRBokehFeatureGraphDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the MultiCameraBokehFeatureLitoSuperGraphDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static ChiFeature2InstanceDesc MultiCameraBokehFeatureLitoSuperGraphFeatureInstanceDescs[] =
{
    //  AnchorSync feature
    {
        &AnchorSyncFeatureDescriptor,
        &AnchorSyncInstanceProps
    },
    {
        &MFNRFeatureDescriptor,
        &MFNRInstanceProps
    },
    {
        &MFNRFeatureDescriptor,
        &MFNRInstanceProps1
    },
    {
        &MFNRFeatureDescriptor,
        &MFNRInstanceProps2
    },
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps
    },
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps1
    },
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps2
    },
    {
        &HDRT1FeatureDescriptor,
        &HDRT1InstanceProps
    },
    {
        &HDRT1FeatureDescriptor,
        &HDRT1InstanceProps1
    },
    {
        &HDRT1FeatureDescriptor,
        &HDRT1InstanceProps2
    },
    {
        &BokehFeatureDescriptor,
        &BokehFeatureInstanceProps
    },
    {
        &DemuxFeatureDescriptor,
        &DemuxInstanceProps
    },
    {
        &SerializerFeatureDescriptor,
        &SerializerInstanceProps
    },
    {
        &DemuxFeatureDescriptor,
        &DemuxInstanceProps1
    },
    {
        &SerializerFeatureDescriptor,
        &SerializerInstanceProps1
    },
    {
        &DemuxFeatureDescriptor,
        &DemuxInstanceProps2
    },
    {
        &SerializerFeatureDescriptor,
        &SerializerInstanceProps2
    },

};

static ChiFeature2GraphExtSrcLinkDesc MultiCameraBokehFeatureLitoSuperGraphSrcLinks[]=
{
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[0].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[1].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[2].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[3].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[4].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[5].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[6].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[7].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[8].globalId
        }
    }
};

static ChiFeature2GraphInternalLinkDesc MultiCameraBokehFeatureLitoSuperGraphInternalLinks[] =
{
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // RDI_IN
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[2].globalId
        },
        {
            // RDI_IN
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[3].globalId
        },
        {
            // Meta_IN
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[4].globalId
        },
        {
            // RDI_IN
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[5].globalId
        },
        {
            // Meta_IN
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // YUV_out
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_out
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Meta_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // YUV_out
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_out
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Meta_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // YUV_out
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_out
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Meta_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[5].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[2].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[3].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[4].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[5].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // YUV_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Meta_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // YUV_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Meta_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // YUV_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Meta_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[5].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[2].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[3].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[4].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[5].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[0].globalId
        },
        {
            // YUV_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[0].globalId
        },
        {
            // YUV_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[0].globalId
        },
        {
            // YUV_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // Meta_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // Meta_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // Meta_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[5].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },

    // camera 0
    // 4.AnchorSync-Demux RDI link
    {
        {
            // AnchorSync RDI Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // Demux in
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }

    },
    // 5.AnchorSync-Demux Metadata link
    {
        {
            // AnchorSync Metadata Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // Demux Input Metadata
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 6.Demux-MFSR RDI link
    {
        {
            // Demux RDI-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[0].globalId
        },
        {
            // MFSR RDI Input
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 7.Demux-MFSR Metadata link
    {
        {
            // Demux Metadata-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[1].globalId
        },
        {
            // MFSR Input Metadata
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 8.Demux-Bayer2Yuv RDI link
    {
        {
            // Demux RDI-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[2].globalId
        },
        {
            // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 9.Demux-Bayer2Yuv Metadata link
    {
        {
            // Demux Metadata-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[3].globalId
        },
        {
            // B2Y Metadata in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 10.MFSR-Serializer YUV link
    {
        {
            // MFSR YUV Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // Serializer YUV-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 11.MFSR-Serializer Metadata link
    {
        {
            // MFSR Metadata Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Serializer Metadata-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 12.Bayer2Yuv-Serializer YUV link
    {
        {
            // Bayer2Yuv Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // Serializer YUV-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 13.Bayer2Yuv-Serializer Metadata link
    {
        {
            // Bayer2Yuv Metadata Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Serializer Metadata-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 14.Serializer-HDR YUV link
    {
        {
            // Serializer YUV Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerOutputPortDescriptors[0].globalId
        },
        {
            // HDRT1 YUV Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[HDRInputPortYUV_In0_External].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 15.Serializer-HDR Metadata link
    {
        {
            // Serializer Metadata Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerOutputPortDescriptors[1].globalId
        },
        {
            // HDRT1 Metadata Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[HDRInputPortMeta_In2_Internal].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 16.HDR-Bokeh YUV link
    {
        {
            // HDRT1 YUV Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFNR_HDR,
        }

    },
    // 17.HDR-Bokeh Metadata link
    {
        {
            // HDRT1 Metadata Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // camera 1
    // 4.AnchorSync-Demux RDI link
    {
        {
            // AnchorSync RDI Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[2].globalId
        },
        {
            // Demux in
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }

    },
    // 5.AnchorSync-Demux Metadata link
    {
        {
            // AnchorSync Metadata Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[3].globalId
        },
        {
            // Demux Input Metadata
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 6.Demux-MFSR RDI link
    {
        {
            // Demux RDI-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxOutputPortDescriptors[0].globalId
        },
        {
            // MFSR RDI Input
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 7.Demux-MFSR Metadata link
    {
        {
            // Demux Metadata-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxOutputPortDescriptors[1].globalId
        },
        {
            // MFSR Input Metadata
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 8.Demux-Bayer2Yuv RDI link
    {
        {
            // Demux RDI-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxOutputPortDescriptors[2].globalId
        },
        {
            // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 9.Demux-Bayer2Yuv Metadata link
    {
        {
            // Demux Metadata-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxOutputPortDescriptors[3].globalId
        },
        {
            // B2Y Metadata in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 10.MFSR-Serializer YUV link
    {
        {
            // MFSR YUV Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // Serializer YUV-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 11.MFSR-Serializer Metadata link
    {
        {
            // MFSR Metadata Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Serializer Metadata-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 12.Bayer2Yuv-Serializer YUV link
    {
        {
            // Bayer2Yuv Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // Serializer YUV-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 13.Bayer2Yuv-Serializer Metadata link
    {
        {
            // Bayer2Yuv Metadata Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Serializer Metadata-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 14.Serializer-HDR YUV link
    {
        {
            // Serializer YUV Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerOutputPortDescriptors[0].globalId
        },
        {
            // HDRT1 YUV Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureInputPortDescriptors[HDRInputPortYUV_In0_External].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 15.Serializer-HDR Metadata link
    {
        {
            // Serializer Metadata Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerOutputPortDescriptors[1].globalId
        },
        {
            // HDRT1 Metadata Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureInputPortDescriptors[HDRInputPortMeta_In2_Internal].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 16.HDR-Bokeh YUV link
    {
        {
            // HDRT1 YUV Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFNR_HDR,
        }

    },
    // 17.HDR-Bokeh Metadata link
    {
        {
            // HDRT1 Metadata Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },

    // camera 2
    // 4.AnchorSync-Demux RDI link
    {
        {
            // AnchorSync RDI Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[4].globalId
        },
        {
            // Demux in
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }

    },
    // 5.AnchorSync-Demux Metadata link
    {
        {
            // AnchorSync Metadata Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[5].globalId
        },
        {
            // Demux Input Metadata
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 6.Demux-MFSR RDI link
    {
        {
            // Demux RDI-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxOutputPortDescriptors[0].globalId
        },
        {
            // MFSR RDI Input
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 7.Demux-MFSR Metadata link
    {
        {
            // Demux Metadata-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxOutputPortDescriptors[1].globalId
        },
        {
            // MFSR Input Metadata
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 8.Demux-Bayer2Yuv RDI link
    {
        {
            // Demux RDI-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxOutputPortDescriptors[2].globalId
        },
        {
            // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 9.Demux-Bayer2Yuv Metadata link
    {
        {
            // Demux Metadata-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxOutputPortDescriptors[3].globalId
        },
        {
            // B2Y Metadata in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 10.MFSR-Serializer YUV link
    {
        {
            // MFSR YUV Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // Serializer YUV-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 11.MFSR-Serializer Metadata link
    {
        {
            // MFSR Metadata Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Serializer Metadata-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 12.Bayer2Yuv-Serializer YUV link
    {
        {
            // Bayer2Yuv Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // Serializer YUV-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 13.Bayer2Yuv-Serializer Metadata link
    {
        {
            // Bayer2Yuv Metadata Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Serializer Metadata-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 14.Serializer-HDR YUV link
    {
        {
            // Serializer YUV Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerOutputPortDescriptors[0].globalId
        },
        {
            // HDRT1 YUV Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureInputPortDescriptors[HDRInputPortYUV_In0_External].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 15.Serializer-HDR Metadata link
    {
        {
            // Serializer Metadata Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerOutputPortDescriptors[1].globalId
        },
        {
            // HDRT1 Metadata Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureInputPortDescriptors[HDRInputPortMeta_In2_Internal].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone| ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 16.HDR-Bokeh YUV link
    {
        {
            // HDRT1 YUV Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFNR_HDR,
        }

    },
    // 17.HDR-Bokeh Metadata link
    {
        {
            // HDRT1 Metadata Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[5].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFNR_HDR,
        }
    }

};

static ChiFeature2GraphExtSinkLinkDesc MultiCameraBokehFeatureLitoSuperGraphSinkLinks[]=
{
    {
        {
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehOutputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::BOKEH,
        }
    },
    {
        {
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehOutputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::BOKEH,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },

};

extern const ChiFeature2GraphDesc MultiCameraBokehFeatureLitoSuperGraphDescriptor =
{
    "MultiCameraMFNRBokehSuperGraph",                                           // Feature graph name
    CHX_ARRAY_SIZE(MultiCameraBokehFeatureLitoSuperGraphFeatureInstanceDescs),  // The number of feature instances
    &MultiCameraBokehFeatureLitoSuperGraphFeatureInstanceDescs[0],              // The array containing all feature instance
    CHX_ARRAY_SIZE(MultiCameraBokehFeatureLitoSuperGraphSrcLinks),              // The number of source links
    &MultiCameraBokehFeatureLitoSuperGraphSrcLinks[0],                          // The array of all source links
    CHX_ARRAY_SIZE(MultiCameraBokehFeatureLitoSuperGraphInternalLinks),         // The number of internal links
    &MultiCameraBokehFeatureLitoSuperGraphInternalLinks[0],                     // The array of all internal links
    CHX_ARRAY_SIZE(MultiCameraBokehFeatureLitoSuperGraphSinkLinks),             // The number of links pSinkLinks
    &MultiCameraBokehFeatureLitoSuperGraphSinkLinks[0],                         // The array of all sink links
    TRUE
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the MultiCameraFusionFeatureLitoSuperGraphDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the MultiCameraFusionFeatureSuperGraphDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static ChiFeature2InstanceDesc MultiCameraFusionFeatureSuperGraphFeatureInstanceDescs[] =
{
    //  AnchorSync feature
    {
        &AnchorSyncFeatureDescriptor,
        &AnchorSyncInstanceProps
    },
    {
        &MFSRFeatureDescriptor,
        &MFSRInstanceProps
    },
    {
        &MFSRFeatureDescriptor,
        &MFSRInstanceProps1
    },
    {
        &MFSRFeatureDescriptor,
        &MFSRInstanceProps2
    },
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps
    },
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps1
    },
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps2
    },
    {
        &HDRT1FeatureDescriptor,
        &HDRT1InstanceProps
    },
    {
        &HDRT1FeatureDescriptor,
        &HDRT1InstanceProps1
    },
    {
        &HDRT1FeatureDescriptor,
        &HDRT1InstanceProps2
    },
    {
        &FusionFeatureDescriptor,
        &FusionFeatureInstanceProps
    },
    {
        &DemuxFeatureDescriptor,
        &DemuxInstanceProps
    },
    {
        &SerializerFeatureDescriptor,
        &SerializerInstanceProps
    },
    {
        &DemuxFeatureDescriptor,
        &DemuxInstanceProps1
    },
    {
        &SerializerFeatureDescriptor,
        &SerializerInstanceProps1
    },
    {
        &DemuxFeatureDescriptor,
        &DemuxInstanceProps2
    },
    {
        &SerializerFeatureDescriptor,
        &SerializerInstanceProps2
    }
};

static ChiFeature2GraphExtSrcLinkDesc MultiCameraFusionFeatureSuperGraphSrcLinks[]=
{
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[0].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[1].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[2].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[3].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[4].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[5].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[6].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[7].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[8].globalId
        }
    }
};

static ChiFeature2GraphInternalLinkDesc MultiCameraFusionFeatureSuperGraphInternalLinks[] =
{
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // RDI_IN
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[2].globalId
        },
        {
            // RDI_IN
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[3].globalId
        },
        {
            // Meta_IN
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[4].globalId
        },
        {
            // RDI_IN
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[5].globalId
        },
        {
            // Meta_IN
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // YUV_out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // YUV_out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // YUV_out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[5].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[2].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[3].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[4].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[5].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // YUV_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // YUV_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // YUV_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[5].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[2].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[3].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[4].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[5].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[0].globalId
        },
        {
            // YUV_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[0].globalId
        },
        {
            // YUV_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[0].globalId
        },
        {
            // YUV_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[5].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },


    // camera 0
    // 4.AnchorSync-Demux RDI link
    {
        {
            // AnchorSync RDI Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // Demux in
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }

    },
    // 5.AnchorSync-Demux Metadata link
    {
        {
            // AnchorSync Metadata Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // Demux Input Metadata
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 6.Demux-MFSR RDI link
    {
        {
            // Demux RDI-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[0].globalId
        },
        {
            // MFSR RDI Input
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 7.Demux-MFSR Metadata link
    {
        {
            // Demux Metadata-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[1].globalId
        },
        {
            // MFSR Input Metadata
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 8.Demux-Bayer2Yuv RDI link
    {
        {
            // Demux RDI-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[2].globalId
        },
        {
            // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 9.Demux-Bayer2Yuv Metadata link
    {
        {
            // Demux Metadata-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[3].globalId
        },
        {
            // B2Y Metadata in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 10.MFSR-Serializer YUV link
    {
        {
            // MFSR YUV Output
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // Serializer YUV-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 11.MFSR-Serializer Metadata link
    {
        {
            // MFSR Metadata Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Serializer Metadata-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 12.Bayer2Yuv-Serializer YUV link
    {
        {
            // Bayer2Yuv Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // Serializer YUV-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 13.Bayer2Yuv-Serializer Metadata link
    {
        {
            // Bayer2Yuv Metadata Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Serializer Metadata-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 14.Serializer-HDR YUV link
    {
        {
            // Serializer YUV Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerOutputPortDescriptors[0].globalId
        },
        {
            // HDRT1 YUV Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[HDRInputPortYUV_In0_External].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 15.Serializer-HDR Metadata link
    {
        {
            // Serializer Metadata Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerOutputPortDescriptors[1].globalId
        },
        {
            // HDRT1 Metadata Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[HDRInputPortMeta_In2_Internal].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // camera 1
    // 4.AnchorSync-Demux RDI link
    {
        {
            // AnchorSync RDI Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[2].globalId
        },
        {
            // Demux in
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }

    },
    // 5.AnchorSync-Demux Metadata link
    {
        {
            // AnchorSync Metadata Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[3].globalId
        },
        {
            // Demux Input Metadata
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 6.Demux-MFSR RDI link
    {
        {
            // Demux RDI-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxOutputPortDescriptors[0].globalId
        },
        {
            // MFSR RDI Input
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 7.Demux-MFSR Metadata link
    {
        {
            // Demux Metadata-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxOutputPortDescriptors[1].globalId
        },
        {
            // MFSR Input Metadata
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 8.Demux-Bayer2Yuv RDI link
    {
        {
            // Demux RDI-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxOutputPortDescriptors[2].globalId
        },
        {
            // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 9.Demux-Bayer2Yuv Metadata link
    {
        {
            // Demux Metadata-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxOutputPortDescriptors[3].globalId
        },
        {
            // B2Y Metadata in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 10.MFSR-Serializer YUV link
    {
        {
            // MFSR YUV Output
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // Serializer YUV-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 11.MFSR-Serializer Metadata link
    {
        {
            // MFSR Metadata Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Serializer Metadata-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 12.Bayer2Yuv-Serializer YUV link
    {
        {
            // Bayer2Yuv Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // Serializer YUV-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 13.Bayer2Yuv-Serializer Metadata link
    {
        {
            // Bayer2Yuv Metadata Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Serializer Metadata-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 14.Serializer-HDR YUV link
    {
        {
            // Serializer YUV Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerOutputPortDescriptors[0].globalId
        },
        {
            // HDRT1 YUV Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureInputPortDescriptors[HDRInputPortYUV_In0_External].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 15.Serializer-HDR Metadata link
    {
        {
            // Serializer Metadata Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerOutputPortDescriptors[1].globalId
        },
        {
            // HDRT1 Metadata Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureInputPortDescriptors[HDRInputPortMeta_In2_Internal].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },

    // camera 2
    // 4.AnchorSync-Demux RDI link
    {
        {
            // AnchorSync RDI Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[4].globalId
        },
        {
            // Demux in
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }

    },
    // 5.AnchorSync-Demux Metadata link
    {
        {
            // AnchorSync Metadata Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[5].globalId
        },
        {
            // Demux Input Metadata
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 6.Demux-MFSR RDI link
    {
        {
            // Demux RDI-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxOutputPortDescriptors[0].globalId
        },
        {
            // MFSR RDI Input
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 7.Demux-MFSR Metadata link
    {
        {
            // Demux Metadata-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxOutputPortDescriptors[1].globalId
        },
        {
            // MFSR Input Metadata
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 8.Demux-Bayer2Yuv RDI link
    {
        {
            // Demux RDI-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxOutputPortDescriptors[2].globalId
        },
        {
            // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 9.Demux-Bayer2Yuv Metadata link
    {
        {
            // Demux Metadata-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxOutputPortDescriptors[3].globalId
        },
        {
            // B2Y Metadata in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 10.MFSR-Serializer YUV link
    {
        {
            // MFSR YUV Output
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // Serializer YUV-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 11.MFSR-Serializer Metadata link
    {
        {
            // MFSR Metadata Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Serializer Metadata-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 12.Bayer2Yuv-Serializer YUV link
    {
        {
            // Bayer2Yuv Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // Serializer YUV-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 13.Bayer2Yuv-Serializer Metadata link
    {
        {
            // Bayer2Yuv Metadata Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Serializer Metadata-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 14.Serializer-HDR YUV link
    {
        {
            // Serializer YUV Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerOutputPortDescriptors[0].globalId
        },
        {
            // HDRT1 YUV Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureInputPortDescriptors[HDRInputPortYUV_In0_External].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 15.Serializer-HDR Metadata link
    {
        {
            // Serializer Metadata Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerOutputPortDescriptors[1].globalId
        },
        {
            // HDRT1 Metadata Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureInputPortDescriptors[HDRInputPortMeta_In2_Internal].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    }
};

static ChiFeature2GraphExtSinkLinkDesc MultiCameraFusionFeatureSuperGraphSinkLinks[]=
{
    {
        {
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionOutputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::FUSION,
        }
    },
    {
        {
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionOutputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::FUSION,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps1,
            MfsrPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps2,
            MfsrPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    }
};

extern const ChiFeature2GraphDesc MultiCameraFusionFeatureSuperGraphDescriptor =
{
    "MultiCameraMFNRFusionSuperGraph",                                       // Feature graph name
    CHX_ARRAY_SIZE(MultiCameraFusionFeatureSuperGraphFeatureInstanceDescs),  // The number of feature instances
    &MultiCameraFusionFeatureSuperGraphFeatureInstanceDescs[0],              // The array containing all feature instance
    CHX_ARRAY_SIZE(MultiCameraFusionFeatureSuperGraphSrcLinks),              // The number of source links
    &MultiCameraFusionFeatureSuperGraphSrcLinks[0],                          // The array of all source links
    CHX_ARRAY_SIZE(MultiCameraFusionFeatureSuperGraphInternalLinks),         // The number of internal links
    &MultiCameraFusionFeatureSuperGraphInternalLinks[0],                     // The array of all internal links
    CHX_ARRAY_SIZE(MultiCameraFusionFeatureSuperGraphSinkLinks),             // The number of links pSinkLinks
    &MultiCameraFusionFeatureSuperGraphSinkLinks[0],                         // The array of all sink links
    TRUE
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the MultiCameraMFNRFusionFeatureGraphDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the MultiCameraFusionFeatureLitoSuperGraphDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static ChiFeature2InstanceDesc MultiCameraFusionFeatureLitoSuperGraphFeatureInstanceDescs[] =
{
    //  AnchorSync feature
    {
        &AnchorSyncFeatureDescriptor,
        &AnchorSyncInstanceProps
    },
    {
        &MFNRFeatureDescriptor,
        &MFNRInstanceProps
    },
    {
        &MFNRFeatureDescriptor,
        &MFNRInstanceProps1
    },
    {
        &MFNRFeatureDescriptor,
        &MFNRInstanceProps2
    },
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps
    },
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps1
    },
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps2
    },
    {
        &HDRT1FeatureDescriptor,
        &HDRT1InstanceProps
    },
    {
        &HDRT1FeatureDescriptor,
        &HDRT1InstanceProps1
    },
    {
        &HDRT1FeatureDescriptor,
        &HDRT1InstanceProps2
    },
    {
        &FusionFeatureDescriptor,
        &FusionFeatureInstanceProps
    },
    {
        &DemuxFeatureDescriptor,
        &DemuxInstanceProps
    },
    {
        &SerializerFeatureDescriptor,
        &SerializerInstanceProps
    },
    {
        &DemuxFeatureDescriptor,
        &DemuxInstanceProps1
    },
    {
        &SerializerFeatureDescriptor,
        &SerializerInstanceProps1
    },
    {
        &DemuxFeatureDescriptor,
        &DemuxInstanceProps2
    },
    {
        &SerializerFeatureDescriptor,
        &SerializerInstanceProps2
    }
};

static ChiFeature2GraphExtSrcLinkDesc MultiCameraFusionFeatureLitoSuperGraphSrcLinks[]=
{
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[0].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[1].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[2].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[3].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[4].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[5].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[6].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[7].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[8].globalId
        }
    }
};

static ChiFeature2GraphInternalLinkDesc MultiCameraFusionFeatureLitoSuperGraphInternalLinks[] =
{
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // RDI_IN
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[2].globalId
        },
        {
            // RDI_IN
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[3].globalId
        },
        {
            // Meta_IN
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[4].globalId
        },
        {
            // RDI_IN
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[5].globalId
        },
        {
            // Meta_IN
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // YUV_out
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_out
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // YUV_out
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_out
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // YUV_out
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // Meta_out
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[5].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[2].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[3].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[4].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[5].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // YUV_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // YUV_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // YUV_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // Meta_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[5].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[2].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[3].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[4].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[5].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[0].globalId
        },
        {
            // YUV_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[0].globalId
        },
        {
            // YUV_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[0].globalId
        },
        {
            // YUV_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // Meta_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[5].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::HDR,
        }
    },
    // camera 0
    // 4.AnchorSync-Demux RDI link
    {
        {
            // AnchorSync RDI Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // Demux in
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }

    },
    // 5.AnchorSync-Demux Metadata link
    {
        {
            // AnchorSync Metadata Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // Demux Input Metadata
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 6.Demux-MFSR RDI link
    {
        {
            // Demux RDI-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[0].globalId
        },
        {
            // MFSR RDI Input
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 7.Demux-MFSR Metadata link
    {
        {
            // Demux Metadata-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[1].globalId
        },
        {
            // MFSR Input Metadata
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 8.Demux-Bayer2Yuv RDI link
    {
        {
            // Demux RDI-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[2].globalId
        },
        {
            // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 9.Demux-Bayer2Yuv Metadata link
    {
        {
            // Demux Metadata-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps,
            DemuxOutputPortDescriptors[3].globalId
        },
        {
            // B2Y Metadata in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 10.MFSR-Serializer YUV link
    {
        {
            // MFSR YUV Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // Serializer YUV-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 11.MFSR-Serializer Metadata link
    {
        {
            // MFSR Metadata Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Serializer Metadata-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 12.Bayer2Yuv-Serializer YUV link
    {
        {
            // Bayer2Yuv Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // Serializer YUV-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 13.Bayer2Yuv-Serializer Metadata link
    {
        {
            // Bayer2Yuv Metadata Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Serializer Metadata-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 14.Serializer-HDR YUV link
    {
        {
            // Serializer YUV Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerOutputPortDescriptors[0].globalId
        },
        {
            // HDRT1 YUV Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[HDRInputPortYUV_In0_External].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 15.Serializer-HDR Metadata link
    {
        {
            // Serializer Metadata Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps,
            SerializerOutputPortDescriptors[1].globalId
        },
        {
            // HDRT1 Metadata Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureInputPortDescriptors[HDRInputPortMeta_In2_Internal].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 16.HDR-Fusion YUV link
    {
        {
            // HDRT1 YUV Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 17.HDR-Fusion Metadata link
    {
        {
            // HDRT1 Metadata Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // camera 1
    // 4.AnchorSync-Demux RDI link
    {
        {
            // AnchorSync RDI Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[2].globalId
        },
        {
            // Demux in
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }

    },
    // 5.AnchorSync-Demux Metadata link
    {
        {
            // AnchorSync Metadata Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[3].globalId
        },
        {
            // Demux Input Metadata
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 6.Demux-MFSR RDI link
    {
        {
            // Demux RDI-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxOutputPortDescriptors[0].globalId
        },
        {
            // MFSR RDI Input
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 7.Demux-MFSR Metadata link
    {
        {
            // Demux Metadata-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxOutputPortDescriptors[1].globalId
        },
        {
            // MFSR Input Metadata
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 8.Demux-Bayer2Yuv RDI link
    {
        {
            // Demux RDI-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxOutputPortDescriptors[2].globalId
        },
        {
            // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 9.Demux-Bayer2Yuv Metadata link
    {
        {
            // Demux Metadata-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps1,
            DemuxOutputPortDescriptors[3].globalId
        },
        {
            // B2Y Metadata in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 10.MFSR-Serializer YUV link
    {
        {
            // MFSR YUV Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // Serializer YUV-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 11.MFSR-Serializer Metadata link
    {
        {
            // MFSR Metadata Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Serializer Metadata-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 12.Bayer2Yuv-Serializer YUV link
    {
        {
            // Bayer2Yuv Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // Serializer YUV-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 13.Bayer2Yuv-Serializer Metadata link
    {
        {
            // Bayer2Yuv Metadata Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Serializer Metadata-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 14.Serializer-HDR YUV link
    {
        {
            // Serializer YUV Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerOutputPortDescriptors[0].globalId
        },
        {
            // HDRT1 YUV Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureInputPortDescriptors[HDRInputPortYUV_In0_External].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 15.Serializer-HDR Metadata link
    {
        {
            // Serializer Metadata Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps1,
            SerializerOutputPortDescriptors[1].globalId
        },
        {
            // HDRT1 Metadata Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureInputPortDescriptors[HDRInputPortMeta_In2_Internal].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 16.HDR-Fusion YUV link
    {
        {
            // HDRT1 YUV Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 17.HDR-Fusion Metadata link
    {
        {
            // HDRT1 Metadata Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },

    // camera 2
    // 4.AnchorSync-Demux RDI link
    {
        {
            // AnchorSync RDI Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[4].globalId
        },
        {
            // Demux in
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }

    },
    // 5.AnchorSync-Demux Metadata link
    {
        {
            // AnchorSync Metadata Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[5].globalId
        },
        {
            // Demux Input Metadata
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 6.Demux-MFSR RDI link
    {
        {
            // Demux RDI-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxOutputPortDescriptors[0].globalId
        },
        {
            // MFSR RDI Input
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPrefilterInputPortDescriptorsConfig[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 7.Demux-MFSR Metadata link
    {
        {
            // Demux Metadata-1 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxOutputPortDescriptors[1].globalId
        },
        {
            // MFSR Input Metadata
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPrefilterInputPortDescriptorsConfig[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 8.Demux-Bayer2Yuv RDI link
    {
        {
            // Demux RDI-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxOutputPortDescriptors[2].globalId
        },
        {
            // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 9.Demux-Bayer2Yuv Metadata link
    {
        {
            // Demux Metadata-2 Out
            DemuxFeatureDescriptor.featureId,
            DemuxInstanceProps2,
            DemuxOutputPortDescriptors[3].globalId
        },
        {
            // B2Y Metadata in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 10.MFSR-Serializer YUV link
    {
        {
            // MFSR YUV Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            // Serializer YUV-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerInputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 11.MFSR-Serializer Metadata link
    {
        {
            // MFSR Metadata Output
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            // Serializer Metadata-1 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerInputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 12.Bayer2Yuv-Serializer YUV link
    {
        {
            // Bayer2Yuv Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // Serializer YUV-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerInputPortDescriptors[2].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 13.Bayer2Yuv-Serializer Metadata link
    {
        {
            // Bayer2Yuv Metadata Output
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Serializer Metadata-2 Input
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerInputPortDescriptors[3].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 14.Serializer-HDR YUV link
    {
        {
            // Serializer YUV Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerOutputPortDescriptors[0].globalId
        },
        {
            // HDRT1 YUV Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureInputPortDescriptors[HDRInputPortYUV_In0_External].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 15.Serializer-HDR Metadata link
    {
        {
            // Serializer Metadata Output
            SerializerFeatureDescriptor.featureId,
            SerializerInstanceProps2,
            SerializerOutputPortDescriptors[1].globalId
        },
        {
            // HDRT1 Metadata Input
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureInputPortDescriptors[HDRInputPortMeta_In2_Internal].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone | ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    // 16.HDR-Fusion YUV link
    {
        {
            // HDRT1 YUV Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[4].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFNR_HDR,
        }

    },
    // 17.HDR-Fusion Metadata link
    {
        {
            // HDRT1 Metadata Output
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            // YUV_In
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionInputPortDescriptors[5].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::MFNR_HDR,
        }
    }
};

static ChiFeature2GraphExtSinkLinkDesc MultiCameraFusionFeatureLitoSuperGraphSinkLinks[]=
{
    {
        {
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionOutputPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::FUSION,
        }
    },
    {
        {
            FusionFeatureDescriptor.featureId,
            FusionFeatureInstanceProps,
            FusionOutputPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::DualZone,
            ChiFeature2Type::FUSION,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }

    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::B2Y,
        }
    },
    {
        {
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps1,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps2,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFSR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortYUVOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps2,
            HDRT1FeatureOutputPortDescriptors[HDROutputPortMetaOutExternal].globalId
        },
        {
            ChiFeature2PruneGroup::SingleZone,
            ChiFeature2Type::MFNR_HDR,
        }
    }
};

extern const ChiFeature2GraphDesc MultiCameraFusionFeatureLitoSuperGraphDescriptor =
{
    "MultiCameraMFNRFusionSuperGraph",                                           // Feature graph name
    CHX_ARRAY_SIZE(MultiCameraFusionFeatureLitoSuperGraphFeatureInstanceDescs),  // The number of feature instances
    &MultiCameraFusionFeatureLitoSuperGraphFeatureInstanceDescs[0],              // The array containing all feature instance
    CHX_ARRAY_SIZE(MultiCameraFusionFeatureLitoSuperGraphSrcLinks),              // The number of source links
    &MultiCameraFusionFeatureLitoSuperGraphSrcLinks[0],                          // The array of all source links
    CHX_ARRAY_SIZE(MultiCameraFusionFeatureLitoSuperGraphInternalLinks),         // The number of internal links
    &MultiCameraFusionFeatureLitoSuperGraphInternalLinks[0],                     // The array of all internal links
    CHX_ARRAY_SIZE(MultiCameraFusionFeatureLitoSuperGraphSinkLinks),             // The number of links pSinkLinks
    &MultiCameraFusionFeatureLitoSuperGraphSinkLinks[0],                         // The array of all sink links
    TRUE
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the MultiCameraFusionFeatureLitoSuperGraphDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the MultiCameraHDRGraphDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static ChiFeature2InstanceProps AnchorSyncInstanceProps1 =
{
    1,
    1
};

static ChiFeature2InstanceProps BokehFeatureInstanceProps1 =
{
    1,
    1
};

static ChiFeature2InstanceDesc MultiCameraHDRFeatureGraphInstanceDescs[] =
{
    //  AnchorSync feature
    {
        &AnchorSyncFeatureDescriptor,
        &AnchorSyncInstanceProps1
    },
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps
    },
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps1
    },
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps2
    },
    {
        &HDRT1FeatureDescriptor,
        &HDRT1InstanceProps1
    },
    {
        &BokehFeatureDescriptor,
        &BokehFeatureInstanceProps
    }
};

static ChiFeature2GraphExtSrcLinkDesc MultiCameraHDRFeatureGraphSrcLinks[]=
{
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps1,
            AnchorSyncInputPortDescriptors[0].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps1,
            AnchorSyncInputPortDescriptors[1].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps1,
            AnchorSyncInputPortDescriptors[2].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps1,
            AnchorSyncInputPortDescriptors[3].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps1,
            AnchorSyncInputPortDescriptors[4].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps1,
            AnchorSyncInputPortDescriptors[5].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps1,
            AnchorSyncInputPortDescriptors[6].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps1,
            AnchorSyncInputPortDescriptors[7].globalId
        }
    },
    {
        {
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps1,
            AnchorSyncInputPortDescriptors[8].globalId
        }
    }
};

static ChiFeature2GraphInternalLinkDesc MultiCameraHDRFeatureGraphInternalLinks[] =
{
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps1,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps1,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        }
    },
    {
        {
            // RDI_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps1,
            AnchorSyncOutputPortDescriptors[2].globalId
        },
        {
            // RDI_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        }
    },
    {
        {
            // Meta_OUT
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps1,
            AnchorSyncOutputPortDescriptors[3].globalId
        },
        {
            // Meta_IN
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        }
    },
    {
        {
            // YUV_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[0].globalId
        },
        {
            // YUV_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureInputPortDescriptors[0].globalId
        }
    },
    {
        {
            // Meta_OUT
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps1,
            Bayer2YuvOutputPortDescriptors[1].globalId
        },
        {
            // Meta_IN
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureInputPortDescriptors[4].globalId
        }
    },
    {
        {
            // YUV_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[0].globalId
        }
    },
    {
        {
            // Meta_out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // Meta_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[1].globalId
        }
    },
    {
        {
            // YUV_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[0].globalId
        },
        {
            // YUV_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[2].globalId
        }
    },
    {
        {
            // Meta_out
            HDRT1FeatureDescriptor.featureId,
            HDRT1InstanceProps1,
            HDRT1FeatureOutputPortDescriptors[1].globalId
        },
        {
            // Meta_In
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehInputPortDescriptors[3].globalId
        }
    },
};

static ChiFeature2GraphExtSinkLinkDesc MultiCameraHDRFeatureGraphSinkLinks[]=
{
    {
        {
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehOutputPortDescriptors[0].globalId
        }
    },
    {
        {
            BokehFeatureDescriptor.featureId,
            BokehFeatureInstanceProps,
            BokehOutputPortDescriptors[1].globalId
        }
    },
};

extern const ChiFeature2GraphDesc MultiCameraHDRFeatureGraphDescriptor =
{
    "MultiCameraHDRGraph",                                    // Feature graph name
    CHX_ARRAY_SIZE(MultiCameraHDRFeatureGraphInstanceDescs),         // The number of feature instances
    &MultiCameraHDRFeatureGraphInstanceDescs[0],                     // The array containing all feature instance
    CHX_ARRAY_SIZE(MultiCameraHDRFeatureGraphSrcLinks),              // The number of source links
    &MultiCameraHDRFeatureGraphSrcLinks[0],                          // The array of all source links
    CHX_ARRAY_SIZE(MultiCameraHDRFeatureGraphInternalLinks),         // The number of internal links
    &MultiCameraHDRFeatureGraphInternalLinks[0],                     // The array of all internal links
    CHX_ARRAY_SIZE(MultiCameraHDRFeatureGraphSinkLinks),             // The number of links pSinkLinks
    &MultiCameraHDRFeatureGraphSinkLinks[0],                         // The array of all sink links
    TRUE
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the MultiCameraHDRFeatureGraphDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the RTMFNRYUVFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor     RealTimeFeatureDescriptor;
extern const ChiFeature2Descriptor     MFNRFeatureDescriptor;
extern const ChiFeature2Descriptor     AnchorSyncFeatureDescriptor;
extern const ChiFeature2PortDescriptor RealTimeInputPortDescriptors[];
extern const ChiFeature2PortDescriptor RealTimeOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor MFNRPrefilterInputPortDescriptors[];
extern const ChiFeature2PortDescriptor MFNRPostFilterOutPutPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGInputPortDescriptors[];
extern const ChiFeature2PortDescriptor AnchorSyncInputPortDescriptors[];
extern const ChiFeature2PortDescriptor AnchorSyncOutputPortDescriptors[];

static ChiFeature2InstanceDesc RTMFNRYUVFeatureGraphFeatureInstanceDescs[] =
{
    //  RealTime feature
    {
        &RealTimeFeatureDescriptor,
        &RealTimeInstanceProps
    },
    //  AnchorSync feature
    {
        &AnchorSyncFeatureDescriptor,
        &AnchorSyncInstanceProps
    },
    // MFNR feature
    {
        &MFNRFeatureDescriptor,
        &MFNRInstanceProps
    },
};

static ChiFeature2GraphInternalLinkDesc RTMFNRYUVFeatureGraphInternalLinks[] =
{
    {
        {
            // RAW out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortRDI].globalId
        },
        {
            // AnchorSync RDI In
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[0].globalId
        }
    },
    {
        {
            // FD out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortFD].globalId
        },
        {
            // AnchorSync FD In
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[1].globalId
        }
    },
    {
        {
            // rt_metadata_out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortMetaData].globalId
        },
        {
            // AnchorSync Metadata IN
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[2].globalId
        }
    },
    {
        {
            // AnchorSync RDI Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // MFNR in
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPrefilterInputPortDescriptorsConfig[0].globalId
        }
    },
    {
        {
            // AnchorSync Metadata Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // MFNR Input Metadata
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPrefilterInputPortDescriptorsConfig[1].globalId
        }
    },
};

static ChiFeature2GraphExtSinkLinkDesc RTMFNRYUVFeatureGraphSinkLinks[] =
{
    {
        // Display_out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[0].globalId
        }
    },
    {
        // MFNR out
        {
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[0].globalId
        }
    },
    {
        // YUV_Metadata_Out
        {
            MFNRFeatureDescriptor.featureId,
            MFNRInstanceProps,
            MFNRPostFilterOutPutPortDescriptors[1].globalId
        }
    }
};

extern const ChiFeature2GraphDesc RTMFNRYUVFeatureGraphDescriptor =
{
    "RTMFNRYUV",                                               // Feature graph name
    CHX_ARRAY_SIZE(RTMFNRYUVFeatureGraphFeatureInstanceDescs), // The number of feature instances
    &RTMFNRYUVFeatureGraphFeatureInstanceDescs[0],             // The array containing all feature instance
                                                               // descriptors for the graph
    0,                                                         // The number of source links
    NULL,                                                      // The array containing links associated with all source ports
    CHX_ARRAY_SIZE(RTMFNRYUVFeatureGraphInternalLinks),        // The number of internal links
    &RTMFNRYUVFeatureGraphInternalLinks[0],                    // The array containing links associated with all internal ports
    CHX_ARRAY_SIZE(RTMFNRYUVFeatureGraphSinkLinks),            // The number of links pSinkLinks
    &RTMFNRYUVFeatureGraphSinkLinks[0]                         // The array containing links associated with all sink ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the RTMFNRYUVFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the RTMFSRYUVFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor     RealTimeFeatureDescriptor;
extern const ChiFeature2Descriptor     MFSRFeatureDescriptor;
extern const ChiFeature2Descriptor     AnchorSyncFeatureDescriptor;
extern const ChiFeature2PortDescriptor RealTimeInputPortDescriptors[];
extern const ChiFeature2PortDescriptor RealTimeOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor MfsrPrefilterInputPortDescriptors[];
extern const ChiFeature2PortDescriptor MfsrPostFilterOutPutPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGInputPortDescriptors[];
extern const ChiFeature2PortDescriptor AnchorSyncInputPortDescriptors[];
extern const ChiFeature2PortDescriptor AnchorSyncOutputPortDescriptors[];

static ChiFeature2InstanceDesc RTMFSRYUVFeatureGraphFeatureInstanceDescs[] =
{
    //  RealTime feature
    {
        &RealTimeFeatureDescriptor,
        &RealTimeInstanceProps
    },
    //  AnchorSync feature
    {
        &AnchorSyncFeatureDescriptor,
        &AnchorSyncInstanceProps
    },
    // MFSR feature
    {
        &MFSRFeatureDescriptor,
        &MFSRInstanceProps
    }
};

static ChiFeature2GraphInternalLinkDesc RTMFSRYUVFeatureGraphInternalLinks[] =
{
    {
        {
            // RAW out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortRDI].globalId
        },
        {
            // AnchorSync RDI In
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[0].globalId
        }
    },
    {
        {
            // FD out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortFD].globalId
        },
        {
            // AnchorSync FD In
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[1].globalId
        }
    },
    {
        {
            // rt_metadata_out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortMetaData].globalId
        },
        {
            // AnchorSync Metadata IN
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncInputPortDescriptors[2].globalId
        }
    },
    {
        {
            // AnchorSync RDI Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[0].globalId
        },
        {
            // MFSR in
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPrefilterInputPortDescriptorsConfig[0].globalId
        }
    },
    {
        {
            // AnchorSync Metadata Out
            AnchorSyncFeatureDescriptor.featureId,
            AnchorSyncInstanceProps,
            AnchorSyncOutputPortDescriptors[1].globalId
        },
        {
            // MFSR Input Metadata
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPrefilterInputPortDescriptorsConfig[1].globalId
        }
    },
};

static ChiFeature2GraphExtSinkLinkDesc RTMFSRYUVFeatureGraphSinkLinks[] =
{
    {
        // Display_out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortDisplay].globalId
        }
    },
    {
        {
            // MFSR out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPostFilterOutPutPortDescriptors[0].globalId
        }
    },
    {
        {
            // YUV_Metadata_Out
            MFSRFeatureDescriptor.featureId,
            MFSRInstanceProps,
            MfsrPostFilterOutPutPortDescriptors[1].globalId
        }
    }
};

extern const ChiFeature2GraphDesc RTMFSRYUVFeatureGraphDescriptor =
{
    "RTMFSRYUV",                                               // Feature graph name
    CHX_ARRAY_SIZE(RTMFSRYUVFeatureGraphFeatureInstanceDescs), // The number of feature instances
    &RTMFSRYUVFeatureGraphFeatureInstanceDescs[0],             // The array containing all feature instance
                                                               // descriptors for the graph
    0,                                                         // The number of source links
    NULL,                                                      // The array containing links associated with all source ports
    CHX_ARRAY_SIZE(RTMFSRYUVFeatureGraphInternalLinks),        // The number of internal links
    &RTMFSRYUVFeatureGraphInternalLinks[0],                    // The array containing links associated with all internal ports
    CHX_ARRAY_SIZE(RTMFSRYUVFeatureGraphSinkLinks),            // The number of links pSinkLinks
    &RTMFSRYUVFeatureGraphSinkLinks[0]                         // The array containing links associated with all sink ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the RTMFSRYUVFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the RTMemcpyYUVFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This is for test purpose to use preview cb for testing

extern const ChiFeature2Descriptor     RealTimeFeatureDescriptor;
extern const ChiFeature2Descriptor     MemcpyFeatureDescriptor;
extern const ChiFeature2PortDescriptor RealTimeInputPortDescriptors[];
extern const ChiFeature2PortDescriptor RealTimeOutputPortDescriptors[];

static ChiFeature2InstanceDesc RTMemcpyYUVFeatureGraphFeatureInstanceDescs[] =
{
    //  RealTime feature
    {
        &RealTimeFeatureDescriptor,
        &RealTimeInstanceProps
    },
    // Memcpy feature
    {
        &MemcpyFeatureDescriptor,
        &MemcpyInstanceProps
    }

};

static ChiFeature2GraphInternalLinkDesc RTMemcpyYUVFeatureGraphInternalLinks[] =
{
    {
        {
            // Realtime FD out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortVideo].globalId
        },
        {
            // Memcpy In
            MemcpyFeatureDescriptor.featureId,
            MemcpyInstanceProps,
            MemcpyInputPortDescriptors[0].globalId
        }
    },
};

static ChiFeature2GraphExtSinkLinkDesc RTMemcpyYUVFeatureGraphSinkLinks[] =
{
    {
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortDisplay].globalId
        }
    },
    {
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortRDI].globalId
        }
    },
    {
        {
            MemcpyFeatureDescriptor.featureId,
            MemcpyInstanceProps,
            MemcpyOutputPortDescriptors[MemcpyOutputPortFwk].globalId
        }
    },
    {
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortFD].globalId
        }
    },
    {
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortMetaData].globalId
        }
    }
};

extern const ChiFeature2GraphDesc RTMemcpyYUVFeatureGraphDescriptor =
{
    "RTMemcpyYUV",                                               // Feature graph name
    CHX_ARRAY_SIZE(RTMemcpyYUVFeatureGraphFeatureInstanceDescs), // The number of feature instances
    &RTMemcpyYUVFeatureGraphFeatureInstanceDescs[0],             // The array containing all feature instance
                                                                 // descriptors for the graph
    0,                                                           // The number of source links
    NULL,                                                        // The array containing links associated with all source ports
                                                                 // with all source ports
    CHX_ARRAY_SIZE(RTMemcpyYUVFeatureGraphInternalLinks),        // The number of internal links
    &RTMemcpyYUVFeatureGraphInternalLinks[0],                    // The array containing links associated
                                                                 // with all internal ports
    CHX_ARRAY_SIZE(RTMemcpyYUVFeatureGraphSinkLinks),            // The number of links pSinkLinks
    &RTMemcpyYUVFeatureGraphSinkLinks[0]                         // The array containing links associated with all sink ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the RTMemcpyYUVFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the RTQCFAPostViewFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This is for test purpose to use preview cb for testing

extern const ChiFeature2Descriptor     RealTimeFeatureDescriptor;
extern const ChiFeature2Descriptor     MemcpyFeatureDescriptor;
extern const ChiFeature2PortDescriptor RealTimeInputPortDescriptors[];
extern const ChiFeature2PortDescriptor RealTimeOutputPortDescriptors[];

static ChiFeature2InstanceProps RealTimeInstanceProps2 =
{
    1,
    0,
    CHX_ARRAY_SIZE(RealtimeFeatureProperties),
    &RealtimeFeatureProperties[0],
};

static ChiFeature2InstanceDesc RTQCFAPostViewFeatureGraphFeatureInstanceDescs[] =
{
    //  RealTime feature
    {
        &RealTimeFeatureDescriptor,
        &RealTimeInstanceProps
    },
    //  RealTime2 feature
    {
        &RealTimeFeatureDescriptor,
        &RealTimeInstanceProps2
    },
    // Bayer to YUV feature
    {
        &Bayer2YuvFeatureDescriptor,
        &Bayer2YUVInstanceProps
    },
    // JPEG feature
    {
        &JPEGFeatureDescriptor,
        &JPEGInstanceProps
    },
    // Memcpy feature
    {
        &MemcpyFeatureDescriptor,
        &MemcpyInstanceProps
    }

};

static ChiFeature2GraphInternalLinkDesc RTQCFAPostViewFeatureGraphInternalLinks[] =
{
    {
        {
            // Realtime FD out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortFD].globalId
        },
        {
            // Memcpy In
            MemcpyFeatureDescriptor.featureId,
            MemcpyInstanceProps,
            MemcpyInputPortDescriptors[0].globalId
        }
    },
    {
        {
            // Raw_out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps2,
            RealTimeOutputPortDescriptors[RealTimeOutputPortRDI].globalId
        },
        {
            // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps2,
            Bayer2YuvInputPortDescriptors[B2YInputPortRDI].globalId
        }
    },
    {
        {
            // rt_metadata_out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps2,
            RealTimeOutputPortDescriptors[RealTimeOutputPortMetaData].globalId
        },
        {
            // B2Y Input Metadata
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[B2YInputPortMetaData].globalId
        }
    },
    {
        {
            // B2Y out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVOut].globalId
        },
        {
            // JPEG in
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGInputPortDescriptors[JPEGInputPortYUV].globalId
        }
    },
    {
        {
            // YUV_Metadata_Out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[B2YOutputPortYUVMetaData].globalId
        },
        {
            // JPEG_Input_Metadata
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGInputPortDescriptors[JPEGInputPortMetaData].globalId
        }
    }
};



static ChiFeature2GraphExtSinkLinkDesc RTQCFAPostViewFeatureGraphSinkLinks[] =
{
    {
        // Display_out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortDisplay].globalId
        }
    },
    {
        // RDI out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortRDI].globalId
        }
    },
    {
        // Memcpy YUV out
        {
            MemcpyFeatureDescriptor.featureId,
            MemcpyInstanceProps,
            MemcpyOutputPortDescriptors[MemcpyOutputPortFwk].globalId
        }
    },
    {
        // Video out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[RealTimeOutputPortVideo].globalId
        }
    },

    {
        // Display_out2
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps2,
            RealTimeOutputPortDescriptors[RealTimeOutputPortDisplay].globalId
        }
    },
    {
        // JPEG out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[JPEGOutputPort].globalId
        }
    },
    {
        // JPEG Metadata Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[JPEGOutputPortMetaData].globalId
        }
    },
    {
        // HEIC YUV Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[2].globalId
        }
    },
    {
        // HEIC Thumbnail Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[3].globalId
        }
    },
    {
        // Video out2
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps2,
            RealTimeOutputPortDescriptors[RealTimeOutputPortVideo].globalId
        }
    },
};

extern const ChiFeature2GraphDesc RTQCFAPostViewFeatureGraphDescriptor =
{
    "RTQCFAPostView",                                               // Feature graph name
    CHX_ARRAY_SIZE(RTQCFAPostViewFeatureGraphFeatureInstanceDescs), // The number of feature instances
    &RTQCFAPostViewFeatureGraphFeatureInstanceDescs[0],             // The array containing all feature instance
                                                                    // descriptors for the graph
    0,                                                              // The number of source links
    NULL,                                                           // The array containing links associated with all source
                                                                    // ports with all source ports
    CHX_ARRAY_SIZE(RTQCFAPostViewFeatureGraphInternalLinks),        // The number of internal links
    &RTQCFAPostViewFeatureGraphInternalLinks[0],                    // The array containing links associated
                                                                    // with all internal ports
    CHX_ARRAY_SIZE(RTQCFAPostViewFeatureGraphSinkLinks),            // The number of links pSinkLinks
    &RTQCFAPostViewFeatureGraphSinkLinks[0]                         // The array containing links associated with all sink
                                                                    // ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the RTQCFAPostViewFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
