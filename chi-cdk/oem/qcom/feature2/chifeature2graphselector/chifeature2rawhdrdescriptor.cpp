////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2rawhdrdescriptor.cpp
/// @brief CHI feature RawHDR descriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graphselector.h"
#include "chifeature2types.h"

static const ChiFeature2TargetDescriptor RawHDRInputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_RAW_IN0",
    },
    {
        "TARGET_BUFFER_RAW_IN1",
    },
    {
        "TARGET_BUFFER_RAW_IN2",
    }
};

static const ChiFeature2TargetDescriptor RawHDROutputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_RAW_OUT",
    }
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2PortDescriptor RawHDRInputPortDescriptors[] =
{
    {
        { 0, 0, 0, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer },
        "RAW_In0_External",
        &RawHDRInputTargetDescriptors[0],
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::ImageBuffer },
        "RAW_In1_Internal",
        &RawHDRInputTargetDescriptors[0],
    },
    {
        { 0, 0, 2, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::ImageBuffer },
        "RAW_In2_Internal",
        &RawHDRInputTargetDescriptors[1],
    },
    {
        { 0, 0, 3, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::ImageBuffer },
        "RAW_In3_Internal",
        &RawHDRInputTargetDescriptors[2],
    },
    {
        { 0, 0, 4, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData },
        "Meta_In_Internal",
        NULL,
    }
};

extern const ChiFeature2PortDescriptor RawHDROutputPortDescriptors[] =
{
    {
        { 0, 0, 0, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer},
        "RAW_Out_External",
        &RawHDROutputTargetDescriptors[0],
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData},
        "Meta_Out_External",
        NULL,
    }
};

static const ChiFeature2PortDescriptor RawHDRInitInputPortDescriptors[] =
{
    RawHDRInputPortDescriptors[0],
    RawHDRInputPortDescriptors[4]
};

static const ChiFeature2PortDescriptor RawHDRFinalInputPortDescriptors[] =
{
    RawHDRInputPortDescriptors[1],
    RawHDRInputPortDescriptors[2],
    RawHDRInputPortDescriptors[3],
    RawHDRInputPortDescriptors[4]
};

static const ChiFeature2PortDescriptor RawHDRFinalOutputPortDescriptors[] =
{
    RawHDROutputPortDescriptors[0],
    RawHDROutputPortDescriptors[1]
};

static ChiFeature2InputDependency RawHDRInitInputDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(RawHDRInitInputPortDescriptors),
        &RawHDRInitInputPortDescriptors[0]
    },

    {
        CHX_ARRAY_SIZE(RawHDRInitInputPortDescriptors),
        &RawHDRInitInputPortDescriptors[0]
    },

    {
        CHX_ARRAY_SIZE(RawHDRInitInputPortDescriptors),
        &RawHDRInitInputPortDescriptors[0]
    },
};

static ChiFeature2DependencyConfigDescriptor RawHDRInitDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(RawHDRInitInputDependencyDescriptor),
        &RawHDRInitInputDependencyDescriptor[0],
        CHX_ARRAY_SIZE(RawHDRFinalInputPortDescriptors),
        &RawHDRFinalInputPortDescriptors[0],
        CHX_ARRAY_SIZE(RawHDRFinalOutputPortDescriptors),
        &RawHDRFinalOutputPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo RawHDRInitPipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(RawHDRInitDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &RawHDRInitDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo RawHDRInitStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(RawHDRInitPipelineDependencyConfig),
        &RawHDRInitPipelineDependencyConfig[0],
    }
};

/*
Feature level descriptors
*/

// Information used to create pipelines
static const ChiFeature2PipelineDescriptor RawHDRPipelineDescriptors[] =
{
    {
        0,
        0,
        "SWMFMergeRaw",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(RawHDRInputPortDescriptors),
        &RawHDRInputPortDescriptors[0],
        CHX_ARRAY_SIZE(RawHDROutputPortDescriptors),
        &RawHDROutputPortDescriptors[0],
    },
};

// Information used to create sessions
static const ChiFeature2SessionDescriptor RawHDRSessionDescriptors[] =
{
    {
        0,
        "RawHDR",
        CHX_ARRAY_SIZE(RawHDRPipelineDescriptors),
        &RawHDRPipelineDescriptors[0],
    },
};

// Information about all stages
static const ChiFeature2StageDescriptor RawHDRStageDescriptors[] =
{
    {
        0,
        "RawHDR_init",
        CHX_ARRAY_SIZE(RawHDRInitStageInfo),
        &RawHDRInitStageInfo[0],
    },
};

extern const ChiFeature2Descriptor RawHDRFeatureDescriptor =
{
    static_cast<UINT32>(ChiFeature2Type::RAWHDR),
    "RawHDR",
    CHX_ARRAY_SIZE(RawHDRStageDescriptors),
    &RawHDRStageDescriptors[0],
    CHX_ARRAY_SIZE(RawHDRSessionDescriptors),
    &RawHDRSessionDescriptors[0],
    0,
    NULL,
};
