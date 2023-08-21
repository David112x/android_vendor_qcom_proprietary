////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2hdrtype1descriptor.cpp
/// @brief CHI feature HDR Type1 descriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graphselector.h"
#include "chifeature2types.h"

static const ChiFeature2TargetDescriptor HDRT1FeatureInputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_YUV0",
    },
    {
        "TARGET_BUFFER_YUV1",
    },
    {
        "TARGET_BUFFER_YUV2",
    },
};

static const ChiFeature2TargetDescriptor HDRT1FeaturetOutputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_YUV_OUT",
    }
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2PortDescriptor HDRT1FeatureInputPortDescriptors[] =
{
    {
        { 0, 0, 0, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer },
        "YUV_In0_External",
        &HDRT1FeatureInputTargetDescriptors[0],
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::ImageBuffer },
        "YUV_In1_Internal",
        &HDRT1FeatureInputTargetDescriptors[0],
    },
    {
        { 0, 0, 2, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::ImageBuffer },
        "YUV_In2_Internal",
        &HDRT1FeatureInputTargetDescriptors[1],
    },
    {
        { 0, 0, 3, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::ImageBuffer },
        "YUV_In3_Internal",
        &HDRT1FeatureInputTargetDescriptors[2],
    },
    {
        { 0, 0, 4, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData },
        "Meta_In2_Internal",
        NULL,
    },
};

extern const ChiFeature2PortDescriptor HDRT1FeatureOutputPortDescriptors[] =
{
    {
        { 0, 0, 0, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer},
        "YUV_Out_External",
        &HDRT1FeaturetOutputTargetDescriptors[0],
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData},
        "Meta_Out_External",
        NULL,
    }
};

static const ChiFeature2PortDescriptor HDRT1InitInputPortDescriptors[] =
{
    HDRT1FeatureInputPortDescriptors[0],
    HDRT1FeatureInputPortDescriptors[4]
};

static const ChiFeature2PortDescriptor HDRT1FinalInputPortDescriptors[] =
{
    HDRT1FeatureInputPortDescriptors[1],
    HDRT1FeatureInputPortDescriptors[2],
    HDRT1FeatureInputPortDescriptors[3],
    HDRT1FeatureInputPortDescriptors[4]
};

static const ChiFeature2PortDescriptor HDRT1FinalOutputPortDescriptors[] =
{
    HDRT1FeatureOutputPortDescriptors[0],
    HDRT1FeatureOutputPortDescriptors[1]
};

static ChiFeature2InputDependency HDRT1InitInputDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(HDRT1InitInputPortDescriptors),
        &HDRT1InitInputPortDescriptors[0]
    },

    {
        CHX_ARRAY_SIZE(HDRT1InitInputPortDescriptors),
        &HDRT1InitInputPortDescriptors[0]
    },

    {
        CHX_ARRAY_SIZE(HDRT1InitInputPortDescriptors),
        &HDRT1InitInputPortDescriptors[0]
    }
};

static ChiFeature2DependencyConfigDescriptor HDRT1InitDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(HDRT1InitInputDependencyDescriptor),
        &HDRT1InitInputDependencyDescriptor[0],
        CHX_ARRAY_SIZE(HDRT1FinalInputPortDescriptors),
        &HDRT1FinalInputPortDescriptors[0],
        CHX_ARRAY_SIZE(HDRT1FinalOutputPortDescriptors),
        &HDRT1FinalOutputPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo HDRT1InitPipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(HDRT1InitDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &HDRT1InitDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo HDRT1InitStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(HDRT1InitPipelineDependencyConfig),
        &HDRT1InitPipelineDependencyConfig[0],
    }
};

/*
Feature level descriptors
*/

// Information used to create pipelines
static const ChiFeature2PipelineDescriptor HDRT1PipelineDescriptors[] =
{
    {
        0,
        0,
        "Merge3YuvCustomTo1Yuv",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(HDRT1FeatureInputPortDescriptors),
        &HDRT1FeatureInputPortDescriptors[0],
        CHX_ARRAY_SIZE(HDRT1FeatureOutputPortDescriptors),
        &HDRT1FeatureOutputPortDescriptors[0],
    },
};

// Information used to create sessions
static const ChiFeature2SessionDescriptor HDRT1SessionDescriptors[] =
{
    {
        0,
        "HDRT1",
        CHX_ARRAY_SIZE(HDRT1PipelineDescriptors),
        &HDRT1PipelineDescriptors[0],
    },
};

// Information about all stages
static const ChiFeature2StageDescriptor HDRT1StageDescriptors[] =
{
    {
        0,
        "HDR_init",
        CHX_ARRAY_SIZE(HDRT1InitStageInfo),
        &HDRT1InitStageInfo[0],
    },
};

extern const ChiFeature2Descriptor HDRT1FeatureDescriptor =
{
    static_cast<UINT32>(ChiFeature2Type::HDR),
    "HDR",
    CHX_ARRAY_SIZE(HDRT1StageDescriptors),
    &HDRT1StageDescriptors[0],
    CHX_ARRAY_SIZE(HDRT1SessionDescriptors),
    &HDRT1SessionDescriptors[0],
    0,
    NULL,
};
