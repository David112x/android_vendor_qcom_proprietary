////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2demuxdescriptor.cpp
/// @brief Bayer-to-YUV feature descriptor definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graphselector.h"
#include "chifeature2types.h"

static const ChiFeature2TargetDescriptor DemuxFeatureInputRDITargetDescriptors[] =
{
    {
        "TARGET_BUFFER_RAW",
    },
};

static const ChiFeature2TargetDescriptor DemuxFeatureOutputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_RAW",
    },
};

extern const ChiFeature2PortDescriptor DemuxInputPortDescriptors[] =
{
    // Make sure the input as the order RDI/FD/Meta/FDI/FD/Meta
    {
        {0, 0, 0, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer},
        "Demux_RDIIn0",
        &DemuxFeatureInputRDITargetDescriptors[0],
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData},
        "Demux_Input_Metadata0",
        NULL,
    }
};

extern const ChiFeature2PortDescriptor DemuxOutputPortDescriptors[] =
{
    // Make sure the output as the order RDI/Meta/RDI/Meta
    {
        {0, 0, 0, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer},
        "Demux_RDIOut0",
        NULL,
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData },
        "Demux_Metadata_Out0",
        NULL,
    },
    {
        {0, 0, 2, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer},
        "Demux_RDIOut1",
        NULL,
    },
    {
        { 0, 0, 3, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData },
        "Demux_Metadata_Out1",
        NULL,
    },
};

static const ChiFeature2PipelineDescriptor DemuxPipelineDescriptors[] =
{
    {
        0,
        0,
        "Demux",
        ChiFeature2PipelineType::Virtual,
        CHX_ARRAY_SIZE(DemuxInputPortDescriptors),
        &DemuxInputPortDescriptors[0],
        CHX_ARRAY_SIZE(DemuxOutputPortDescriptors),
        &DemuxOutputPortDescriptors[0],
    },
};

static const ChiFeature2SessionDescriptor DemuxSessionDescriptors[] =
{
    {
        0,
        "Demux",
        CHX_ARRAY_SIZE(DemuxPipelineDescriptors),
        &DemuxPipelineDescriptors[0],
    },
};

static ChiFeature2InputDependency DemuxInputDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(DemuxInputPortDescriptors),
        &DemuxInputPortDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor DemuxDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(DemuxInputDependencyDescriptor),
        &DemuxInputDependencyDescriptor[0],
        CHX_ARRAY_SIZE(DemuxInputPortDescriptors),
        &DemuxInputPortDescriptors[0],
        CHX_ARRAY_SIZE(DemuxOutputPortDescriptors),
        &DemuxOutputPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo DemuxPipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(DemuxDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &DemuxDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo DemuxStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(DemuxPipelineDependencyConfig),
        &DemuxPipelineDependencyConfig[0],
    }
};

static const ChiFeature2StageDescriptor DemuxStageDescriptor[] =
{
    {
        0,
        "DemuxInit",
        CHX_ARRAY_SIZE(DemuxStageInfo),
        &DemuxStageInfo[0],
    },
};

extern const ChiFeature2Descriptor DemuxFeatureDescriptor =
{
    static_cast<UINT32>(ChiFeature2Type::DEMUX),
    "Demux",
    CHX_ARRAY_SIZE(DemuxStageDescriptor),
    &DemuxStageDescriptor[0],
    CHX_ARRAY_SIZE(DemuxSessionDescriptors),
    &DemuxSessionDescriptors[0],
};
