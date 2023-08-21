////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2memcpydescriptor.cpp
/// @brief memcpy feature descriptor definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graphselector.h"
#include "chifeature2types.h"

static const ChiFeature2TargetDescriptor MemcpyFeatureInputFDTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_IN",
    },
};

static const ChiFeature2TargetDescriptor MemcpyFeatureOutputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_OUT",
    },
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2PortDescriptor MemcpyInputPortDescriptors[] =
{
    {
        {0, 0, 0, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer},
        "Memcpy_In0",
        &MemcpyFeatureInputFDTargetDescriptors[0],
    }
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2PortDescriptor MemcpyOutputPortDescriptors[] =
{
    {
        {0, 0, 0, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer},
        "Memcpy_fwk0",
        &MemcpyFeatureOutputTargetDescriptors[0],
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData },
        "Memcpy_Metadata_Out",
        NULL,
    },
    {
        { 0, 0, 2, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer },
        "Memcpy_out0",
        &MemcpyFeatureOutputTargetDescriptors[0],
    },

};

static const ChiFeature2PipelineDescriptor MemcpyPipelineDescriptors[] =
{
    {
        0,
        0,
        "MemcpyZSLYUV",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(MemcpyInputPortDescriptors),
        &MemcpyInputPortDescriptors[0],
        CHX_ARRAY_SIZE(MemcpyOutputPortDescriptors),
        &MemcpyOutputPortDescriptors[0],
    },
};

static const ChiFeature2SessionDescriptor MemcpySessionDescriptors[] =
{
    {
        0,
        "Memcpy",
        CHX_ARRAY_SIZE(MemcpyPipelineDescriptors),
        &MemcpyPipelineDescriptors[0],
    },
};

static ChiFeature2InputDependency MemcpyInputDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(MemcpyInputPortDescriptors),
        &MemcpyInputPortDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor MemcpyDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(MemcpyInputDependencyDescriptor),
        &MemcpyInputDependencyDescriptor[0],
        CHX_ARRAY_SIZE(MemcpyInputPortDescriptors),
        &MemcpyInputPortDescriptors[0],
        CHX_ARRAY_SIZE(MemcpyOutputPortDescriptors),
        &MemcpyOutputPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo MemcpyPipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(MemcpyDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &MemcpyDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo MemcpyStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(MemcpyPipelineDependencyConfig),
        &MemcpyPipelineDependencyConfig[0],
    }
};

static const ChiFeature2StageDescriptor MemcpyStageDescriptor[] =
{
    {
        0,
        "MemcpyInit",
        CHX_ARRAY_SIZE(MemcpyStageInfo),
        &MemcpyStageInfo[0],
    },
};

extern const ChiFeature2Descriptor MemcpyFeatureDescriptor =
{
    static_cast<UINT32>(ChiFeature2Type::MEMCPY),
    "Memcpy",
    CHX_ARRAY_SIZE(MemcpyStageDescriptor),
    &MemcpyStageDescriptor[0],
    CHX_ARRAY_SIZE(MemcpySessionDescriptors),
    &MemcpySessionDescriptors[0],
};
