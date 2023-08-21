////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2fusiondescriptor.cpp
/// @brief Fusion feature descriptor definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graphselector.h"
#include "chifeature2types.h"

static const ChiFeature2TargetDescriptor FusionFeatureInputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_FUSION_INPUT0",
    },
    {
        "TARGET_BUFFER_FUSION_INPUT1",
    },
    {
        "TARGET_BUFFER_FUSION_INPUT2",
    },
    {
        "TARGET_BUFFER_FUSION_INPUT3",
    }
};

static const ChiFeature2TargetDescriptor FusionFeatureOutputTargetDescriptors[] =
{
    {
        "TARGET_FUSION_SNAPSHOT",
    },
};

extern const ChiFeature2PortDescriptor FusionInputPortDescriptors[] =
{
    {
        {0, 0, 0, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer},
        "YUV_In0_External",
        &FusionFeatureInputTargetDescriptors[0],
    },
    {
        {0, 0, 1, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData},
        "Meta_In0_External",
        NULL,
    },
    {
        {0, 0, 2, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer},
        "YUV_In1_External",
        &FusionFeatureInputTargetDescriptors[1],
    },
    {
        {0, 0, 3, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData},
        "Meta_In1_External",
        NULL,
    },
    {
        {0, 0, 4, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer},
        "YUV_In2_External",
        &FusionFeatureInputTargetDescriptors[2],
    },
    {
        {0, 0, 5, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData},
        "Meta_In2_External",
        NULL,
    },
    {
        {0, 0, 6, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer},
        "YUV_In3_External",
        &FusionFeatureInputTargetDescriptors[3],
    },
    {
        {0, 0, 7, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData},
        "Meta_In3_External",
        NULL,
    }
};

extern const ChiFeature2PortDescriptor FusionOutputPortDescriptors[] =
{
    {
        {0, 0, 0, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer},
        "FusionYUV_Out_External",
        &FusionFeatureOutputTargetDescriptors[0],
    },
    {
        {0, 0, 1, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData},
        "FusionMeta_Out_External",
        NULL,
    },
};

static const ChiFeature2PipelineDescriptor FusionPipelineDescriptors[] =
{
    {
        0,
        0,
        "FusionSnapshot",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(FusionInputPortDescriptors),
        &FusionInputPortDescriptors[0],
        CHX_ARRAY_SIZE(FusionOutputPortDescriptors),
        &FusionOutputPortDescriptors[0],
    }
};

static const ChiFeature2SessionDescriptor FusionSessionDescriptors[] =
{
    {
        0,
        "Fusion",
        CHX_ARRAY_SIZE(FusionPipelineDescriptors),
        &FusionPipelineDescriptors[0],
    }
};

static ChiFeature2InputDependency FusionInputDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(FusionInputPortDescriptors),
        &FusionInputPortDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor FusionDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(FusionInputDependencyDescriptor),
        &FusionInputDependencyDescriptor[0],
        CHX_ARRAY_SIZE(FusionInputPortDescriptors),
        &FusionInputPortDescriptors[0],
        CHX_ARRAY_SIZE(FusionOutputPortDescriptors),
        &FusionOutputPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo FusionPipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(FusionDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &FusionDependencyDescriptor[0],
    }
};

static const ChiFeature2SessionInfo FusionStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(FusionPipelineDependencyConfig),
        &FusionPipelineDependencyConfig[0],
    }
};

static const ChiFeature2StageDescriptor FusionStageDescriptor[] =
{
    {
        0,
        "Fusion",
        CHX_ARRAY_SIZE(FusionStageInfo),
        &FusionStageInfo[0],
    }
};

extern const ChiFeature2Descriptor FusionFeatureDescriptor =
{
    static_cast<UINT32>(ChiFeature2Type::FUSION),
    "FUSION",
    CHX_ARRAY_SIZE(FusionStageDescriptor),
    &FusionStageDescriptor[0],
    CHX_ARRAY_SIZE(FusionSessionDescriptors),
    &FusionSessionDescriptors[0],
    0,
    NULL
};
