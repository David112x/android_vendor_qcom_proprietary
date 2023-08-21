////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2bokehdescriptor.cpp
/// @brief Bokeh feature descriptor definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graphselector.h"
#include "chifeature2types.h"

static const ChiFeature2TargetDescriptor BokehFeatureInputTargetDescriptors[] =
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

static const ChiFeature2TargetDescriptor BokehFeatureOutputTargetDescriptors[] =
{
    {
        "TARGET_FUSION_SNAPSHOT",
    }
};

extern const ChiFeature2PortDescriptor BokehInputPortDescriptors[] =
{
    {
        {0, 0, 0, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer},
        "YUV_In0_External",
        &BokehFeatureInputTargetDescriptors[0],
    },
    {
        {0, 0, 1, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData},
        "Meta_In_External",
        NULL,
    },
    {
        {0, 0, 2, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer},
        "YUV_In1_External",
        &BokehFeatureInputTargetDescriptors[1],
    },
    {
        {0, 0, 3, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData},
        "Meta_In_External",
        NULL,
    },
    {
        {0, 0, 4, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer},
        "YUV_In1_External",
        &BokehFeatureInputTargetDescriptors[2],
    },
    {
        {0, 0, 5, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData},
        "Meta_In_External",
        NULL,
    },
    {
        {0, 0, 6, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer},
        "YUV_In1_External",
        &BokehFeatureInputTargetDescriptors[3],
    },
    {
        {0, 0, 7, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData},
        "Meta_In_External",
        NULL,
    }
};

extern const ChiFeature2PortDescriptor BokehOutputPortDescriptors[] =
{
    {
        {0, 0, 0, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer},
        "YUV_Out_External",
        &BokehFeatureOutputTargetDescriptors[0],
    },
    {
        {0, 0, 1, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData},
        "Meta_Out_External",
        &BokehFeatureOutputTargetDescriptors[0],
    }
};

static const ChiFeature2PipelineDescriptor BokehPipelineDescriptors[] =
{
    {
        0,
        0,
        "BokehSnapshot",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(BokehInputPortDescriptors),
        &BokehInputPortDescriptors[0],
        CHX_ARRAY_SIZE(BokehOutputPortDescriptors),
        &BokehOutputPortDescriptors[0],
    }
};

static const ChiFeature2SessionDescriptor BokehSessionDescriptors[] =
{
    {
        0,
        "Bokeh",
        CHX_ARRAY_SIZE(BokehPipelineDescriptors),
        &BokehPipelineDescriptors[0],
    }
};

static ChiFeature2InputDependency BokehInputDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(BokehInputPortDescriptors),
        &BokehInputPortDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor BokehDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(BokehInputDependencyDescriptor),
        &BokehInputDependencyDescriptor[0],
        CHX_ARRAY_SIZE(BokehInputPortDescriptors),
        &BokehInputPortDescriptors[0],
        CHX_ARRAY_SIZE(BokehOutputPortDescriptors),
        &BokehOutputPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo BokehPipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(BokehDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &BokehDependencyDescriptor[0],
    }
};

static const ChiFeature2SessionInfo BokehStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(BokehPipelineDependencyConfig),
        &BokehPipelineDependencyConfig[0],
    }
};

static const ChiFeature2StageDescriptor BokehStageDescriptor[] =
{
    {
        0,
        "Bokeh",
        CHX_ARRAY_SIZE(BokehStageInfo),
        &BokehStageInfo[0],
    }
};

extern const ChiFeature2Descriptor BokehFeatureDescriptor =
{
    static_cast<UINT32>(ChiFeature2Type::BOKEH),
    "BOKEH",
    CHX_ARRAY_SIZE(BokehStageDescriptor),
    &BokehStageDescriptor[0],
    CHX_ARRAY_SIZE(BokehSessionDescriptors),
    &BokehSessionDescriptors[0],
    0,
    NULL
};
