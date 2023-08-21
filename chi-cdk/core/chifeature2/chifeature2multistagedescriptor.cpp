////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2multistagedescriptor.cpp
/// @brief CHI feature multistage descriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2types.h"

// NOWHINE FILE NC009:  CHI files will start with CHI

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

//////////////////////////////////Bayer2YuvStage////////////////////////////////////////////////////////////////////////////////
static const ChiFeature2TargetDescriptor B2YFeatureInputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_RAW",
    },
};

static const ChiFeature2TargetDescriptor B2YFeatureOutputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_YUV_HAL",
    },
};

static const ChiFeature2PortDescriptor B2YInputPortDescriptors[] =
{
    {
        {0,0,0, ChiFeature2PortDirectionType::ExternalInput},
        "RDI_In",
        &B2YFeatureInputTargetDescriptors[0],
    },
};

static const ChiFeature2PortDescriptor B2YOutputPortDescriptors[] =
{
    {
        { 0,0,0, ChiFeature2PortDirectionType::InternalOutput},
        "YUV_Out",
        &B2YFeatureOutputTargetDescriptors[0],
    },
};

static ChiFeature2InputDependency B2YInputDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(B2YInputPortDescriptors),
        &B2YInputPortDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor B2YDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(B2YInputDependencyDescriptor),
        &B2YInputDependencyDescriptor[0],
        CHX_ARRAY_SIZE(B2YInputPortDescriptors),
        &B2YInputPortDescriptors[0],
        CHX_ARRAY_SIZE(B2YOutputPortDescriptors),
        &B2YOutputPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo B2YPipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(B2YDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &B2YDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo B2YStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(B2YPipelineDependencyConfig),
        &B2YPipelineDependencyConfig[0],
    }
};

static const ChiFeature2StageDescriptor B2YStageDescriptor[] =
{
    {
        0,
        "B2Y",
        CHX_ARRAY_SIZE(B2YStageInfo),
        &B2YStageInfo[0],
    },
};

//////////////////////////////////Yuv2YuvStage//////////////////////////////////////////////////////////////////////////////////
static const ChiFeature2TargetDescriptor YuvToYuvFeatureInputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_CUSTOM_YUV",
    },
};

static const ChiFeature2TargetDescriptor YuvToYuvOutputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_YUV",
    },
};

static const ChiFeature2PortDescriptor YuvToYuvInputPortDescriptors[] =
{
    {
        {0,1,0, ChiFeature2PortDirectionType::InternalInput},
        "Yuv_In",
        &YuvToYuvFeatureInputTargetDescriptors[0],
    },
};

static const ChiFeature2PortDescriptor YuvToYuvOutputPortDescriptors[] =
{
    {
        { 0,1,0, ChiFeature2PortDirectionType::ExternalOutput},
        "YUV_Out_External",
        &YuvToYuvOutputTargetDescriptors[0],
    },
};

static ChiFeature2InputDependency YuvToYuvInputDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(YuvToYuvInputPortDescriptors),
        &YuvToYuvInputPortDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor YuvToYuvDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(YuvToYuvInputDependencyDescriptor),
        &YuvToYuvInputDependencyDescriptor[0],
        CHX_ARRAY_SIZE(YuvToYuvInputPortDescriptors),
        &YuvToYuvInputPortDescriptors[0],
        CHX_ARRAY_SIZE(YuvToYuvOutputPortDescriptors),
        &YuvToYuvOutputPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo YuvToYuvPipelineDependencyConfig[] =
{
    {
        1,
        CHX_ARRAY_SIZE(YuvToYuvDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &YuvToYuvDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo YuvToYuvStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(YuvToYuvPipelineDependencyConfig),
        &YuvToYuvPipelineDependencyConfig[0],
    }
};

//////////////////////////////////Feature level descriptors/////////////////////////////////////////////////////////////////////////////////////////


// Information used to create pipelines
static const ChiFeature2PipelineDescriptor MultiStagePipelineDescriptors[] =
{
    {
        0,
        0,
        "ZSLSnapshotYUVHAL",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(B2YInputPortDescriptors),
        &B2YInputPortDescriptors[0],
        CHX_ARRAY_SIZE(B2YOutputPortDescriptors),
        &B2YOutputPortDescriptors[0],
    },
    {
        0,
        1,
        "ZSLYuv2Yuv",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(YuvToYuvInputPortDescriptors),
        &YuvToYuvInputPortDescriptors[0],
        CHX_ARRAY_SIZE(YuvToYuvOutputPortDescriptors),
        &YuvToYuvOutputPortDescriptors[0],
    },
};

// Information used to create sessions
static const ChiFeature2SessionDescriptor MultiStageSessionDescriptors[] =
{
    {
        0,
        "B2Y",
        CHX_ARRAY_SIZE(MultiStagePipelineDescriptors),
        &MultiStagePipelineDescriptors[0],
    },
};

// Information about all stages
static const ChiFeature2StageDescriptor MultiStageDescriptors[] =
{
    {
        0,
        "B2Y",
        CHX_ARRAY_SIZE(B2YStageInfo),
        &B2YStageInfo[0],
    },
    {
        1,
        "YuvToYuv",
        CHX_ARRAY_SIZE(YuvToYuvStageInfo),
        &YuvToYuvStageInfo[0],
    },
};

// Informations about all internal links
static const ChiFeature2InternalLinkDesc MultiStageInternalLinkDescriptors[] =
{
    // Bayer2Yuv Out ---> YuvToYuv In
    {
        &B2YOutputPortDescriptors[0],
        &YuvToYuvInputPortDescriptors[0],
    }
};

extern const ChiFeature2Descriptor MultiStageFeatureDescriptor =
{
    0,
    "B2Y",
    CHX_ARRAY_SIZE(MultiStageDescriptors),
    &MultiStageDescriptors[0],
    CHX_ARRAY_SIZE(MultiStageSessionDescriptors),
    &MultiStageSessionDescriptors[0],
    CHX_ARRAY_SIZE(MultiStageInternalLinkDescriptors),
    &MultiStageInternalLinkDescriptors[0],
};
