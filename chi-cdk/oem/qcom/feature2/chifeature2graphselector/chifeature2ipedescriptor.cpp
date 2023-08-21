////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2ipedescriptor.cpp
/// @brief IPE feature descriptor definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graphselector.h"
#include "chifeature2types.h"

static const ChiFeature2TargetDescriptor IPEFeatureInputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_P010_UBWCTP10",
    },
};

static const ChiFeature2TargetDescriptor IPEFeatureOutputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_YUV_HAL",
    },
};

extern const ChiFeature2PortDescriptor IPEInputPortDescriptors[] =
{
    {
        { 0, 0, 0, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer },
        "YUV_In",
        &IPEFeatureInputTargetDescriptors[0],
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData },
        "IPE_Input_Metadata",
        NULL,
    },
};

extern const ChiFeature2PortDescriptor IPEOutputPortDescriptors[] =
{
    {
        { 0, 0, 0, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer },
        "YUV_Out",
        &IPEFeatureOutputTargetDescriptors[0],
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData },
        "YUV_Metadata_Out",
        NULL,
    },
};

static const ChiFeature2PipelineDescriptor IPEPipelineDescriptors[] =
{
    {
        0,
        0,
        "ZSLSnapshotIPEYUVTOHAL",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(IPEInputPortDescriptors),
        &IPEInputPortDescriptors[0],
        CHX_ARRAY_SIZE(IPEOutputPortDescriptors),
        &IPEOutputPortDescriptors[0],
    },
};

static const ChiFeature2SessionDescriptor IPESessionDescriptors[] =
{
    {
        0,
        "IPEYUVTOHAL",
        CHX_ARRAY_SIZE(IPEPipelineDescriptors),
        &IPEPipelineDescriptors[0],
    },
};

static ChiFeature2InputDependency IPEInputDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(IPEInputPortDescriptors),
        &IPEInputPortDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor IPEDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(IPEInputDependencyDescriptor),
        &IPEInputDependencyDescriptor[0],
        CHX_ARRAY_SIZE(IPEInputPortDescriptors),
        &IPEInputPortDescriptors[0],
        CHX_ARRAY_SIZE(IPEOutputPortDescriptors),
        &IPEOutputPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo IPEPipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(IPEDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &IPEDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo IPEStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(IPEPipelineDependencyConfig),
        &IPEPipelineDependencyConfig[0],
    }
};

static const ChiFeature2StageDescriptor IPEStageDescriptor[] =
{
    {
        0,
        "IPE",
        CHX_ARRAY_SIZE(IPEStageInfo),
        &IPEStageInfo[0],
    },
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2Descriptor IPEFeatureDescriptor =
{
    static_cast<UINT32>(ChiFeature2Type::IPE),
    "IPE",
    CHX_ARRAY_SIZE(IPEStageDescriptor),
    &IPEStageDescriptor[0],
    CHX_ARRAY_SIZE(IPESessionDescriptors),
    &IPESessionDescriptors[0],
};
