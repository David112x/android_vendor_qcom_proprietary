////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2oemgraphdescriptors.cpp
/// @brief Definitions of static data describing all the feature graph descriptors
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graph.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constants
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
            Bayer2YuvInputPortDescriptors[0].globalId
        }
    }
};

static ChiFeature2GraphExtSinkLinkDesc Bayer2YUVFeatureGraphSinkLinks[]=
{
    {
        {
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[0].globalId
        }
    }
};

extern const ChiFeature2GraphDesc Bayer2YUVOEMFeatureGraphDescriptor =
{
    "Bayer2YUV",                                    // Feature graph name
    1,                                              // The number of feature instances
    &Bayer2YUVFeatureGraphFeatureInstanceDescs[0],  // The array containing all feature instance descriptors for the graph
    1,                                              // The number of source links
    &Bayer2YUVFeatureGraphSrcLinks[0],              // The array containing links associated with all source ports
    0,                                              // The number of internal links
    NULL,                                           // The array containing links associated with all internal ports
    1,                                              // The number of links pSinkLinks
    &Bayer2YUVFeatureGraphSinkLinks[0]              // The array containing links associated with all sink ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the Bayer2YUVFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the RealTimeFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor RealTimeFeatureDescriptor;
extern const ChiFeature2PortDescriptor RealTimeInputPortDescriptors[];
extern const ChiFeature2PortDescriptor RealTimeOutputPortDescriptors[];

static ChiFeature2InstanceProps RealTimeInstanceProps =
{
    0,
    0
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
            RealTimeOutputPortDescriptors[0].globalId
        }
    },
    {
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[1].globalId
        }
    },
    {
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[2].globalId
        }
    },
    {
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[3].globalId
        }
    }
};

extern const ChiFeature2GraphDesc RealTimeOEMFeatureGraphDescriptor =
{
    "Realtime",                                     // Feature graph name
    1,                                              // The number of feature instances
    &RealTimeFeatureGraphFeatureInstanceDescs[0],   // The array containing all feature instance descriptors for the graph
    0,                                              // The number of source links
    NULL,                                           // The array containing links associated with all source ports
    0,                                              // The number of internal links
    NULL,                                           // The array containing links associated with all internal ports
    4,                                              // The number of links pSinkLinks
    &RealTimeFeatureGraphSinkLinks[0]               // The array containing links associated with all sink ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the RealTimeFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the RawHDRFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor     RawHDRFeatureDescriptor;
extern const ChiFeature2PortDescriptor RawHDRInputPortDescriptors[];
extern const ChiFeature2PortDescriptor RawHDROutputPortDescriptors[];

static ChiFeature2InstanceProps RawHDRInstanceProps =
{
    0,
    0
};

static ChiFeature2InstanceDesc RawHDRFeatureGraphFeatureInstanceDescs[] =
{
    // Raw HDR feature
    {
        &RawHDRFeatureDescriptor,
        &RawHDRInstanceProps
    }
};

static ChiFeature2GraphExtSrcLinkDesc RawHDRFeatureGraphSrcLinks[]=
{
    {
        {
            RawHDRFeatureDescriptor.featureId,
            RawHDRInstanceProps,
            RawHDRInputPortDescriptors[0].globalId
        }
    }
};

static ChiFeature2GraphExtSinkLinkDesc RawHDRFeatureGraphSinkLinks[]=
{
    {
        {
            RawHDRFeatureDescriptor.featureId,
            RawHDRInstanceProps,
            RawHDROutputPortDescriptors[0].globalId
        }
    }
};

extern const ChiFeature2GraphDesc RawHDRFeatureGraphDescriptor =
{
    "RawHDR",                                       // Feature graph name
    1,                                              // The number of feature instances
    &RawHDRFeatureGraphFeatureInstanceDescs[0],     // The array containing all feature instance descriptors for the graph
    1,                                              // The number of source links
    &RawHDRFeatureGraphSrcLinks[0],                 // The array containing links associated with all source ports
    0,                                              // The number of internal links
    NULL,                                           // The array containing links associated with all internal ports
    1,                                              // The number of links pSinkLinks
    &RawHDRFeatureGraphSinkLinks[0]                 // The array containing links associated with all sink ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the RawHDRFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the RTBayer2YUVJPEGFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor     RealTimeFeatureDescriptor;
extern const ChiFeature2Descriptor     RealTimeFeatureWithSWRemosaicDescriptor;
extern const ChiFeature2Descriptor     Bayer2YuvFeatureDescriptor;
extern const ChiFeature2Descriptor     JPEGFeatureDescriptor;
extern const ChiFeature2PortDescriptor RealTimeInputPortDescriptors[];
extern const ChiFeature2PortDescriptor RealTimeOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor Bayer2YuvInputPortDescriptors[];
extern const ChiFeature2PortDescriptor Bayer2YuvOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGInputPortDescriptors[];

static ChiFeature2InstanceProps JPEGInstanceProps =
{
    0,
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
        &JPEGFeatureDescriptor,
        &JPEGInstanceProps
    }
};

static ChiFeature2GraphInternalLinkDesc RTBayer2YUVJPEGFeatureGraphInternalLinks[] =
{
    {
        {
            // Raw_out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[1].globalId
        },
        {   // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[0].globalId
        }
    },
    {
        {   // B2Y out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[0].globalId
        },
        {   // JPEG in
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGInputPortDescriptors[0].globalId
        }
    }
};

static ChiFeature2GraphExtSinkLinkDesc RTBayer2YUVJPEGFeatureGraphSinkLinks[] =
{
    {
        // Display_out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[0].globalId
        }
    },
    {   // FD_out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[2].globalId
        }
    },
    {   // JPEG out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[0].globalId
        }
    }

};

extern const ChiFeature2GraphDesc RTBayer2YUVJPEGOEMFeatureGraphDescriptor =
{
    "RTBayer2YUVJPEG",                                    // Feature graph name
    3,                                                    // The number of feature instances
    &RTBayer2YUVJPEGFeatureGraphFeatureInstanceDescs[0],  // The array containing all feature instance descriptors for the graph
    0,                                                    // The number of source links
    NULL,                                                 // The array containing links associated with all source ports
    2,                                                    // The number of internal links
    &RTBayer2YUVJPEGFeatureGraphInternalLinks[0],         // The array containing links associated with all internal ports
    3,                                                    // The number of links pSinkLinks
    &RTBayer2YUVJPEGFeatureGraphSinkLinks[0]              // The array containing links associated with all sink ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the RTBayer2YUVJPEGFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the RTRawHDRBayer2YUVJPEGFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2Descriptor     RealTimeFeatureDescriptor;
extern const ChiFeature2Descriptor     Bayer2YuvFeatureDescriptor;
extern const ChiFeature2Descriptor     JPEGFeatureDescriptor;
extern const ChiFeature2Descriptor     RawHDRFeatureDescriptor;
extern const ChiFeature2PortDescriptor RealTimeInputPortDescriptors[];
extern const ChiFeature2PortDescriptor RealTimeOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor Bayer2YuvInputPortDescriptors[];
extern const ChiFeature2PortDescriptor Bayer2YuvOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor JPEGInputPortDescriptors[];
extern const ChiFeature2PortDescriptor RawHDRInputPortDescriptors[];
extern const ChiFeature2PortDescriptor RawHDROutputPortDescriptors[];

static ChiFeature2InstanceDesc RTRawHDRBayer2YUVJPEGFeatureGraphFeatureInstanceDescs[] =
{
    //  RealTime feature
    {
        &RealTimeFeatureDescriptor,
        &RealTimeInstanceProps
    },
    // RawHDR feature
    {
        &RawHDRFeatureDescriptor,
        &RawHDRInstanceProps
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
    }
};

static ChiFeature2GraphInternalLinkDesc RTRawHDRBayer2YUVJPEGFeatureGraphInternalLinks[] =
{
    {
        {
            // ZSL out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[1].globalId
        },
        {   // RawHDR in
            RawHDRFeatureDescriptor.featureId,
            RawHDRInstanceProps,
            RawHDRInputPortDescriptors[0].globalId
        }
    },
    {
        {
            // Realtime Metadata Out
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[5].globalId
        },
        {   // RawHDR Input Metadata
            RawHDRFeatureDescriptor.featureId,
            RawHDRInstanceProps,
            RawHDRInputPortDescriptors[4].globalId
        }
    },

    {
        {   // RawHDR out
            RawHDRFeatureDescriptor.featureId,
            RawHDRInstanceProps,
            RawHDROutputPortDescriptors[0].globalId
        },
        {   // B2Y in
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[0].globalId
        }
    },
    {
        {   // RawHDR metadata out
            RawHDRFeatureDescriptor.featureId,
            RawHDRInstanceProps,
            RawHDROutputPortDescriptors[1].globalId
        },
        {   // B2Y Input Metadata
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvInputPortDescriptors[1].globalId
        }
    },

    {
        {   // B2Y out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[0].globalId
        },
        {   // JPEG in
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGInputPortDescriptors[0].globalId
        }
    },

    {
        {   // B2Y metadata out
            Bayer2YuvFeatureDescriptor.featureId,
            Bayer2YUVInstanceProps,
            Bayer2YuvOutputPortDescriptors[1].globalId
        },
        {   // JPEG metadata in
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGInputPortDescriptors[1].globalId
        }
    }
};

static ChiFeature2GraphExtSinkLinkDesc RTRawHDRBayer2YUVJPEGFeatureGraphSinkLinks[] =
{
    {
        // Display_out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[0].globalId
        }
    },
    {   // FD_out
        {
            RealTimeFeatureDescriptor.featureId,
            RealTimeInstanceProps,
            RealTimeOutputPortDescriptors[2].globalId
        }
    },
    {   // JPEG out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[0].globalId
        }
    },
    {   // JPEG Metadata Out
        {
            JPEGFeatureDescriptor.featureId,
            JPEGInstanceProps,
            JPEGOutputPortDescriptors[1].globalId
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

extern const ChiFeature2GraphDesc RTRawHDRBayer2YUVJPEGFeatureGraphDescriptor =
{
    "RTRawHDRBayer2YUVJPEG",                                                // Feature graph name
     CHX_ARRAY_SIZE(RTRawHDRBayer2YUVJPEGFeatureGraphFeatureInstanceDescs), // The number of feature instances
    &RTRawHDRBayer2YUVJPEGFeatureGraphFeatureInstanceDescs[0],              // The array containing all
                                                                            //  feature instance descriptors
                                                                            //  for the graph
    0,                                                                      // The number of source links
    NULL,                                                                   // The array containing links
                                                                            // associated with all source ports
    CHX_ARRAY_SIZE(RTRawHDRBayer2YUVJPEGFeatureGraphInternalLinks),         // The number of internal links
    &RTRawHDRBayer2YUVJPEGFeatureGraphInternalLinks[0],                     // The array containing links
                                                                            // associated with
                                                                            //  all internal ports
    CHX_ARRAY_SIZE(RTRawHDRBayer2YUVJPEGFeatureGraphSinkLinks),             // The number of links pSinkLinks
    &RTRawHDRBayer2YUVJPEGFeatureGraphSinkLinks[0]                          // The array containing links
                                                                            // associated with all sink ports
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// END of Definitions of static data describing the RTRawHDRBayer2YUVJPEGFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
