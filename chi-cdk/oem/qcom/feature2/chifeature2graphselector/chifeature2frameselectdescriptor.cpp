////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2frameselectdescriptor.cpp
/// @brief FrameSelect feature descriptor definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graphselector.h"
#include "chifeature2types.h"

static const ChiFeature2TargetDescriptor FrameSelectFeatureInputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_RAW",
    },
};

static const ChiFeature2TargetDescriptor FrameSelectFeatureOutputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_RAW",
    },
};

extern const ChiFeature2PortDescriptor FrameSelectInputPortDescriptors[] =
{
    {
        { 0, 0, 0, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer },
        "FrameSelect_RDIIn0",
        NULL,
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData },
        "FrameSelect_Input_Metadata",
        NULL,
    },

};

extern const ChiFeature2PortDescriptor FrameSelectOutputPortDescriptors[] =
{
    {
        { 0, 0, 0, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer },
        "FrameSelect_Out0",
        NULL,
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData },
        "FrameSelect_Metadata_Out",
        NULL,
    },
};

static const ChiFeature2PipelineDescriptor FrameSelectPipelineDescriptors[] =
{
    {
        0,
        0,
        "FrameSelect",
        ChiFeature2PipelineType::Virtual,
        CHX_ARRAY_SIZE(FrameSelectInputPortDescriptors),
        &FrameSelectInputPortDescriptors[0],
        CHX_ARRAY_SIZE(FrameSelectOutputPortDescriptors),
        &FrameSelectOutputPortDescriptors[0],
    },
};

static const ChiFeature2SessionDescriptor FrameSelectSessionDescriptors[] =
{
    {
        0,
        "FrameSelect",
        CHX_ARRAY_SIZE(FrameSelectPipelineDescriptors),
        &FrameSelectPipelineDescriptors[0],
    },
};

static ChiFeature2InputDependency FrameSelectInputDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(FrameSelectInputPortDescriptors),
        &FrameSelectInputPortDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor FrameSelectDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(FrameSelectInputDependencyDescriptor),
        &FrameSelectInputDependencyDescriptor[0],
        CHX_ARRAY_SIZE(FrameSelectInputPortDescriptors),
        &FrameSelectInputPortDescriptors[0],
        CHX_ARRAY_SIZE(FrameSelectOutputPortDescriptors),
        &FrameSelectOutputPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo FrameSelectPipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(FrameSelectDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &FrameSelectDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo FrameSelectStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(FrameSelectPipelineDependencyConfig),
        &FrameSelectPipelineDependencyConfig[0],
    }
};

static const ChiFeature2StageDescriptor FrameSelectStageDescriptor[] =
{
    {
        0,
        "FrameSelectInit",
        CHX_ARRAY_SIZE(FrameSelectStageInfo),
        &FrameSelectStageInfo[0],
    },
};

extern const ChiFeature2Descriptor FrameSelectFeatureDescriptor =
{
    static_cast<UINT32>(ChiFeature2Type::FRAME_SELECT),
    "FrameSelect",
    CHX_ARRAY_SIZE(FrameSelectStageDescriptor),
    &FrameSelectStageDescriptor[0],
    CHX_ARRAY_SIZE(FrameSelectSessionDescriptors),
    &FrameSelectSessionDescriptors[0],
};
