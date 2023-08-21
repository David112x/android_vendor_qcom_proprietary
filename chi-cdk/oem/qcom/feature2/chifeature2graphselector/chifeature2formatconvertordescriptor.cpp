
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2formatconvertordescriptor.cpp
/// @brief Format convertor feature descriptor definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graphselector.h"
#include "chifeature2types.h"

static const ChiFeature2TargetDescriptor FormatConvertorFeatureInputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_RAW",
    },
};

static const ChiFeature2TargetDescriptor FormatConvertorFeatureOutputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_IDEAL_RAW_OUT",
    },
};

extern const ChiFeature2PortDescriptor FormatConvertorInputPortDescriptors[] =
{
    {
        {0, 0, 0, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer},
        "Input",
        &FormatConvertorFeatureInputTargetDescriptors[0],
    },
    {
        {0, 0, 1, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData},
        "Input_Metadata",
        NULL,
    },
};

extern const ChiFeature2PortDescriptor FormatConvertorOutputPortDescriptors[] =
{
    {
        {0, 0, 0, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer},
        "Output",
        &FormatConvertorFeatureOutputTargetDescriptors[0],
    },
    {
        {0, 0, 1, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData},
        "Output_Metadata",
        NULL,
    },
};

static const ChiFeature2PipelineDescriptor FormatConvertorPipelineDescriptors[] =
{
    {
        0,
        0,
        "ZSLSnapshotFormatConvertor",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(FormatConvertorInputPortDescriptors),
        &FormatConvertorInputPortDescriptors[0],
        CHX_ARRAY_SIZE(FormatConvertorOutputPortDescriptors),
        &FormatConvertorOutputPortDescriptors[0],
    },
};

static const ChiFeature2SessionDescriptor FormatConvertorSessionDescriptors[] =
{
    {
        0,
        "FORMATCONVERTOR",
        CHX_ARRAY_SIZE(FormatConvertorPipelineDescriptors),
        &FormatConvertorPipelineDescriptors[0],
    },
};

static ChiFeature2InputDependency FormatConvertorInputDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(FormatConvertorInputPortDescriptors),
        &FormatConvertorInputPortDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor FormatConvertorDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(FormatConvertorInputDependencyDescriptor),
        &FormatConvertorInputDependencyDescriptor[0],
        CHX_ARRAY_SIZE(FormatConvertorInputPortDescriptors),
        &FormatConvertorInputPortDescriptors[0],
        CHX_ARRAY_SIZE(FormatConvertorOutputPortDescriptors),
        &FormatConvertorOutputPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo FormatConvertorPipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(FormatConvertorDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &FormatConvertorDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo FormatConvertorStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(FormatConvertorPipelineDependencyConfig),
        &FormatConvertorPipelineDependencyConfig[0],
    }
};

static const ChiFeature2StageDescriptor FormatConvertorStageDescriptor[] =
{
    {
        0,
        "FormatConvertor",
        CHX_ARRAY_SIZE(FormatConvertorStageInfo),
        &FormatConvertorStageInfo[0],
    },
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2Descriptor FormatConvertorFeatureDescriptor =
{
    static_cast<UINT32>(ChiFeature2Type::FORMATCONVERTOR),
    "FormatConvertor",
    CHX_ARRAY_SIZE(FormatConvertorStageDescriptor),
    &FormatConvertorStageDescriptor[0],
    CHX_ARRAY_SIZE(FormatConvertorSessionDescriptors),
    &FormatConvertorSessionDescriptors[0],
};
