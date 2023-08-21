////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2bpsdescriptor.cpp
/// @brief BPS feature descriptor definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graphselector.h"
#include "chifeature2types.h"

static const ChiFeature2TargetDescriptor BPSFeatureInputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_RAW",
    },
};

static const ChiFeature2TargetDescriptor BPSFeatureOutputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_P010",
    },
    {
        "TARGET_BUFFER_DS4REF_PD10_OUT",
    },
    {
        "TARGET_BUFFER_DS16REF_PD10_OUT",
    },
    {
        "TARGET_BUFFER_DS64REF_PD10_OUT",
    },
};

extern const ChiFeature2PortDescriptor BPSInputPortDescriptors[] =
{
    {
        { 0, 0, 0, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer },
        "RDI_In",
        &BPSFeatureInputTargetDescriptors[0],
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData },
        "BPS_Input_Metadata",
        NULL,
    },
};

extern const ChiFeature2PortDescriptor BPSOutputPortDescriptors[] =
{
    {
        { 0, 0, 0, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer },
        "YUV_Out",
        &BPSFeatureOutputTargetDescriptors[0],
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer },
        "DS4_Out",
        &BPSFeatureOutputTargetDescriptors[1],
    },
    {
        { 0, 0, 2, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer },
        "DS16_Out",
        &BPSFeatureOutputTargetDescriptors[2],
    },
    {
        { 0, 0, 3, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer },
        "DS64_Out",
        &BPSFeatureOutputTargetDescriptors[3],
    },
    {
        { 0, 0, 4, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData },
        "YUV_Metadata_Out",
        NULL,
    },
};

static const ChiFeature2PipelineDescriptor BPSPipelineDescriptors[] =
{
    {
        0,
        0,
        "ZSLSnapshotBPSYUVTOHAL",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(BPSInputPortDescriptors),
        &BPSInputPortDescriptors[0],
        CHX_ARRAY_SIZE(BPSOutputPortDescriptors),
        &BPSOutputPortDescriptors[0],
    },
};

static const ChiFeature2SessionDescriptor BPSSessionDescriptors[] =
{
    {
        0,
        "BPSYUVTOHAL",
        CHX_ARRAY_SIZE(BPSPipelineDescriptors),
        &BPSPipelineDescriptors[0],
    },
};

static ChiFeature2InputDependency BPSInputDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(BPSInputPortDescriptors),
        &BPSInputPortDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor BPSDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(BPSInputDependencyDescriptor),
        &BPSInputDependencyDescriptor[0],
        CHX_ARRAY_SIZE(BPSInputPortDescriptors),
        &BPSInputPortDescriptors[0],
        CHX_ARRAY_SIZE(BPSOutputPortDescriptors),
        &BPSOutputPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo BPSPipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(BPSDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &BPSDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo BPSStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(BPSPipelineDependencyConfig),
        &BPSPipelineDependencyConfig[0],
    }
};

static const ChiFeature2StageDescriptor BPSStageDescriptor[] =
{
    {
        0,
        "BPS",
        CHX_ARRAY_SIZE(BPSStageInfo),
        &BPSStageInfo[0],
    },
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2Descriptor BPSFeatureDescriptor =
{
    static_cast<UINT32>(ChiFeature2Type::BPS),
    "BPS",
    CHX_ARRAY_SIZE(BPSStageDescriptor),
    &BPSStageDescriptor[0],
    CHX_ARRAY_SIZE(BPSSessionDescriptors),
    &BPSSessionDescriptors[0],
};
