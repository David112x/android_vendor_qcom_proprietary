////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2swmfdescriptor.cpp
/// @brief CHI feature SWMF descriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graphselector.h"
#include "chifeature2types.h"

static const ChiFeature2TargetDescriptor SWMFFeatureInputTargetDescriptors[] =
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
    {
        "TARGET_BUFFER_YUV3",
    },
    {
        "TARGET_BUFFER_YUV4",
    },
    {
        "TARGET_BUFFER_YUV5",
    },
    {
        "TARGET_BUFFER_YUV6",
    },
    {
        "TARGET_BUFFER_YUV7",
    },
};

static const ChiFeature2TargetDescriptor SWMFFeatureOutputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_YUV_OUT",
    }
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2PortDescriptor SWMFFeatureInputPortDescriptors[] =
{
    {
        { 0, 0, 0, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer },
        "YUV_In0_External",
        &SWMFFeatureInputTargetDescriptors[0],
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::ImageBuffer },
        "YUV_In1_Internal",
        &SWMFFeatureInputTargetDescriptors[0],
    },
    {
        { 0, 0, 2, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::ImageBuffer },
        "YUV_In2_Internal",
        &SWMFFeatureInputTargetDescriptors[1],
    },
    {
        { 0, 0, 3, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::ImageBuffer },
        "YUV_In3_Internal",
        &SWMFFeatureInputTargetDescriptors[2],
    },
    {
        { 0, 0, 4, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::ImageBuffer },
        "YUV_In4_Internal",
        &SWMFFeatureInputTargetDescriptors[3],
    },
    {
        { 0, 0, 5, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::ImageBuffer },
        "YUV_In5_Internal",
        &SWMFFeatureInputTargetDescriptors[4],
    },
    {
        { 0, 0, 6, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::ImageBuffer },
        "YUV_In6_Internal",
        &SWMFFeatureInputTargetDescriptors[5],
    },
    {
        { 0, 0, 7, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::ImageBuffer },
        "YUV_In7_Internal",
        &SWMFFeatureInputTargetDescriptors[6],
    },
    {
        { 0, 0, 8, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::ImageBuffer },
        "YUV_In8_Internal",
        &SWMFFeatureInputTargetDescriptors[7],
    },
    {
        { 0, 0, 9, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData },
        "Meta_In2_Internal",
        NULL,
    },
};

extern const ChiFeature2PortDescriptor SWMFFeatureOutputPortDescriptors[] =
{
    {
        { 0, 0, 0, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer},
        "YUV_Out_External",
        &SWMFFeatureOutputTargetDescriptors[0],
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData},
        "Meta_Out_External",
        NULL,
    }
};

static const ChiFeature2PortDescriptor SWMFInitInputPortDescriptors[] =
{
    SWMFFeatureInputPortDescriptors[0],
    SWMFFeatureInputPortDescriptors[9]
};

static const ChiFeature2PortDescriptor SWMFFinalInputPortDescriptors[] =
{
    SWMFFeatureInputPortDescriptors[1],
    SWMFFeatureInputPortDescriptors[2],
    SWMFFeatureInputPortDescriptors[3],
    SWMFFeatureInputPortDescriptors[4],
    SWMFFeatureInputPortDescriptors[5],
    SWMFFeatureInputPortDescriptors[6],
    SWMFFeatureInputPortDescriptors[7],
    SWMFFeatureInputPortDescriptors[8],
    SWMFFeatureInputPortDescriptors[9]
};

static const ChiFeature2PortDescriptor SWMFFinalOutputPortDescriptors[] =
{
    SWMFFeatureOutputPortDescriptors[0],
    SWMFFeatureOutputPortDescriptors[1]
};

static ChiFeature2InputDependency SWMFInitInputDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(SWMFInitInputPortDescriptors),
        &SWMFInitInputPortDescriptors[0]
    },
};

static ChiFeature2DependencyConfigDescriptor SWMFInitDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(SWMFInitInputDependencyDescriptor),
        &SWMFInitInputDependencyDescriptor[0],
        CHX_ARRAY_SIZE(SWMFFinalInputPortDescriptors),
        &SWMFFinalInputPortDescriptors[0],
        CHX_ARRAY_SIZE(SWMFFinalOutputPortDescriptors),
        &SWMFFinalOutputPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo SWMFInitPipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(SWMFInitDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &SWMFInitDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo SWMFInitStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(SWMFInitPipelineDependencyConfig),
        &SWMFInitPipelineDependencyConfig[0],
    }
};

/*
Feature level descriptors
*/

// Information used to create pipelines
static const ChiFeature2PipelineDescriptor SWMFPipelineDescriptors[] =
{
    {
        0,
        0,
        "SWMFMergeYuv",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(SWMFFeatureInputPortDescriptors),
        &SWMFFeatureInputPortDescriptors[0],
        CHX_ARRAY_SIZE(SWMFFeatureOutputPortDescriptors),
        &SWMFFeatureOutputPortDescriptors[0],
    },
};

// Information used to create sessions
static const ChiFeature2SessionDescriptor SWMFSessionDescriptors[] =
{
    {
        0,
        "SWMF",
        CHX_ARRAY_SIZE(SWMFPipelineDescriptors),
        &SWMFPipelineDescriptors[0],
    },
};

// Information about all stages
static const ChiFeature2StageDescriptor SWMFStageDescriptors[] =
{
    {
        0,
        "SWMF_init",
        CHX_ARRAY_SIZE(SWMFInitStageInfo),
        &SWMFInitStageInfo[0],
    },
};

extern const ChiFeature2Descriptor SWMFFeatureDescriptor =
{
    static_cast<UINT32>(ChiFeature2Type::SWMF),
    "SWMF",
    CHX_ARRAY_SIZE(SWMFStageDescriptors),
    &SWMFStageDescriptors[0],
    CHX_ARRAY_SIZE(SWMFSessionDescriptors),
    &SWMFSessionDescriptors[0],
    0,
    NULL,
};
