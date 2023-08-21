////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2bayer2yuvdescriptor.cpp
/// @brief Bayer-to-YUV feature descriptor definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graphselector.h"
#include "chifeature2types.h"

static const ChiFeature2TargetDescriptor Bayer2YuvFeatureInputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_RAW",
    },
};

static const ChiFeature2TargetDescriptor Bayer2YuvFeatureOutputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_YUV_HAL",
    },
    {
        "TARGET_BUFFER_YUV_HAL2",
    },
};

extern const ChiFeature2PortDescriptor Bayer2YuvInputPortDescriptors[] =
{
    {
        {0, 0, 0, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer},
        "RDI_In",
        &Bayer2YuvFeatureInputTargetDescriptors[0],
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData},
        "B2Y_Input_Metadata",
        NULL,
    },
};

extern const ChiFeature2PortDescriptor Bayer2YuvOutputPortDescriptors[] =
{
    {
        {0, 0, 0, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer},
        "YUV_Out",
        &Bayer2YuvFeatureOutputTargetDescriptors[0],
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData },
        "YUV_Metadata_Out",
        NULL,
    },
    {
        { 0, 0, 2, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer },
        "YUV_Out2",
        &Bayer2YuvFeatureOutputTargetDescriptors[1],
    },
};

static const ChiFeature2PipelineDescriptor Bayer2YuvPipelineDescriptors[] =
{
    {
        0,
        0,
        "ZSLSnapshotYUVHAL",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(Bayer2YuvInputPortDescriptors),
        &Bayer2YuvInputPortDescriptors[0],
        CHX_ARRAY_SIZE(Bayer2YuvOutputPortDescriptors),
        &Bayer2YuvOutputPortDescriptors[0],
    },
    {
        0,
        0,
        "Bayer2YUVWithSWRemosaic",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(Bayer2YuvInputPortDescriptors),
        &Bayer2YuvInputPortDescriptors[0],
        CHX_ARRAY_SIZE(Bayer2YuvOutputPortDescriptors),
        &Bayer2YuvOutputPortDescriptors[0],
    },
};

static const ChiFeature2SessionDescriptor Bayer2YuvSessionDescriptors[] =
{
    {
        0,
        "Bayer2Yuv",
        CHX_ARRAY_SIZE(Bayer2YuvPipelineDescriptors),
        &Bayer2YuvPipelineDescriptors[0],
    },
};

static ChiFeature2InputDependency Bayer2YuvInputDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(Bayer2YuvInputPortDescriptors),
        &Bayer2YuvInputPortDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor Bayer2YuvDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(Bayer2YuvInputDependencyDescriptor),
        &Bayer2YuvInputDependencyDescriptor[0],
        CHX_ARRAY_SIZE(Bayer2YuvInputPortDescriptors),
        &Bayer2YuvInputPortDescriptors[0],
        CHX_ARRAY_SIZE(Bayer2YuvOutputPortDescriptors),
        &Bayer2YuvOutputPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo Bayer2YuvPipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(Bayer2YuvDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &Bayer2YuvDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo Bayer2YuvStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(Bayer2YuvPipelineDependencyConfig),
        &Bayer2YuvPipelineDependencyConfig[0],
    }
};

static const ChiFeature2StageDescriptor Bayer2YuvStageDescriptor[] =
{
    {
        0,
        "Bayer2Yuv",
        CHX_ARRAY_SIZE(Bayer2YuvStageInfo),
        &Bayer2YuvStageInfo[0],
    },
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2Descriptor Bayer2YuvFeatureDescriptor =
{
    static_cast<UINT32>(ChiFeature2Type::B2Y),
    "Bayer2Yuv",
    CHX_ARRAY_SIZE(Bayer2YuvStageDescriptor),
    &Bayer2YuvStageDescriptor[0],
    CHX_ARRAY_SIZE(Bayer2YuvSessionDescriptors),
    &Bayer2YuvSessionDescriptors[0],
};
