////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2jpegdescriptor.cpp
/// @brief CHI feature JPEG descriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2types.h"
#include "chifeature2graphselector.h"

static const ChiFeature2TargetDescriptor JPEGFeatureInputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_CUSTOM_YUV",
    },
    {
        "TARGET_BUFFER_CUSTOM_YUV2",
    },
};

static const ChiFeature2TargetDescriptor JPEGFeatureOutputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_SNAPSHOT",
    },
    {
        "TARGET_BUFFER_HEIC_YUV",
    },
    {
        "TARGET_BUFFER_HEIC_BLOB",
    },
    {
        "TARGET_BUFFER_SNAPSHOT2",
    },
};

extern const ChiFeature2PortDescriptor JPEGInputPortDescriptors[] =
{
    {
        { 0, 0, 0, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer },
        "YUV_In",
        &JPEGFeatureInputTargetDescriptors[0],
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData },
        "JPEG_Input_Metadata",
        NULL,
    }
};

extern const ChiFeature2PortDescriptor JPEGOutputPortDescriptors[] =
{
    {
        {0, 0, 0, ChiFeature2PortDirectionType::ExternalOutput,  ChiFeature2PortType::ImageBuffer},
        "JPEG_Out",
        &JPEGFeatureOutputTargetDescriptors[0],
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalOutput,  ChiFeature2PortType::MetaData },
        "JPEG_Metadata_Out",
        NULL,
    },
    {
        {0, 0, 2, ChiFeature2PortDirectionType::ExternalOutput,  ChiFeature2PortType::ImageBuffer},
        "YUV_Out",
        &JPEGFeatureOutputTargetDescriptors[1],
    },
    {
        {0, 0, 3, ChiFeature2PortDirectionType::ExternalOutput,  ChiFeature2PortType::ImageBuffer},
        "Thumbnail_Out",
        &JPEGFeatureOutputTargetDescriptors[2],
    }
};

static const ChiFeature2PipelineDescriptor JPEGPipelineDescriptors[] =
{
    {
        0,
        0,
        "InternalZSLYuv2Jpeg",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(JPEGInputPortDescriptors),
        &JPEGInputPortDescriptors[0],
        CHX_ARRAY_SIZE(JPEGOutputPortDescriptors),
        &JPEGOutputPortDescriptors[0],
    },
};

static const ChiFeature2SessionDescriptor JPEGSessionDescriptors[] =
{
    {
        0,
        "JPEG",
        CHX_ARRAY_SIZE(JPEGPipelineDescriptors),
        &JPEGPipelineDescriptors[0],
    },
};

static ChiFeature2InputDependency JPEGInputDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(JPEGInputPortDescriptors),
        &JPEGInputPortDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor JPEGDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(JPEGInputDependencyDescriptor),
        &JPEGInputDependencyDescriptor[0],
        CHX_ARRAY_SIZE(JPEGInputPortDescriptors),
        &JPEGInputPortDescriptors[0],
        CHX_ARRAY_SIZE(JPEGOutputPortDescriptors),
        &JPEGOutputPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo JPEGPipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(JPEGDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &JPEGDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo JPEGStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(JPEGPipelineDependencyConfig),
        &JPEGPipelineDependencyConfig[0],
    }
};

static const ChiFeature2StageDescriptor JPEGStageDescriptor[] =
{
    {
        0,
        "JPEG",
        CHX_ARRAY_SIZE(JPEGStageInfo),
        &JPEGStageInfo[0],
    },
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2Descriptor JPEGFeatureDescriptor =
{
    static_cast<UINT32>(ChiFeature2Type::JPEG),
    "JPEG",
    CHX_ARRAY_SIZE(JPEGStageDescriptor),
    &JPEGStageDescriptor[0],
    CHX_ARRAY_SIZE(JPEGSessionDescriptors),
    &JPEGSessionDescriptors[0],
};


extern const ChiFeature2PortDescriptor JPEGInputPortDescriptorsNoGPU[] =
{
    {
        {0, 0, 0, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer },
        "YUV_In",
        &JPEGFeatureInputTargetDescriptors[0],
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData },
        "JPEG_Input_Metadata",
        NULL,
    }
};


extern const ChiFeature2PortDescriptor JPEGInputPortDescriptorsAll[] =
{
    {
        { 0, 0, 0, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer },
        "YUV_In",
        &JPEGFeatureInputTargetDescriptors[0],
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData },
        "JPEG_Input_Metadata",
        NULL,
    },
    {
        { 0, 0, 2, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer },
        "YUV_In2",
        &JPEGFeatureInputTargetDescriptors[1],
    },
};

extern const ChiFeature2PortDescriptor JPEGInputPortDescriptorsGPU[] =
{
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData },
        "JPEG_Input_Metadata",
        NULL,
    },
    {
        { 0, 0, 2, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer },
        "YUV_In2",
        &JPEGFeatureInputTargetDescriptors[1],
    },
};

extern const ChiFeature2PortDescriptor JPEGOutputPortDescriptorsGPU[] =
{
    {
        {0, 0, 0, ChiFeature2PortDirectionType::ExternalOutput,  ChiFeature2PortType::ImageBuffer},
        "JPEG_Out",
        &JPEGFeatureOutputTargetDescriptors[0],
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalOutput,  ChiFeature2PortType::MetaData },
        "JPEG_Metadata_Out",
        NULL,
    },
    {
        {0, 0, 2, ChiFeature2PortDirectionType::ExternalOutput,  ChiFeature2PortType::ImageBuffer},
        "YUV_Out",
        &JPEGFeatureOutputTargetDescriptors[1],
    },
    {
        {0, 0, 3, ChiFeature2PortDirectionType::ExternalOutput,  ChiFeature2PortType::ImageBuffer},
        "Thumbnail_Out",
        &JPEGFeatureOutputTargetDescriptors[2],
    },
    {
        { 0, 0, 4, ChiFeature2PortDirectionType::ExternalOutput,  ChiFeature2PortType::ImageBuffer },
        "JPEG_Out2",
        &JPEGFeatureOutputTargetDescriptors[3],
    },
    {
        { 0, 0, 5, ChiFeature2PortDirectionType::ExternalOutput,  ChiFeature2PortType::ImageBuffer },
        "YUV_Out2",
        &JPEGFeatureOutputTargetDescriptors[1],
    },
    {
        { 0, 0, 6, ChiFeature2PortDirectionType::ExternalOutput,  ChiFeature2PortType::ImageBuffer },
        "Thumbnail_Out2",
        &JPEGFeatureOutputTargetDescriptors[2],
    },
};

static const ChiFeature2PipelineDescriptor JPEGPipelineDescriptorsGPU[] =
{
    {
        0,
        0,
        "InternalZSLYuv2JpegGPU",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(JPEGInputPortDescriptorsAll),
        &JPEGInputPortDescriptorsAll[0],
        CHX_ARRAY_SIZE(JPEGOutputPortDescriptorsGPU),
        &JPEGOutputPortDescriptorsGPU[0],
    },
};

static const ChiFeature2SessionDescriptor JPEGSessionDescriptorsGPU[] =
{
    {
        0,
        "JPEG",
        CHX_ARRAY_SIZE(JPEGPipelineDescriptorsGPU),
        &JPEGPipelineDescriptorsGPU[0],
    },
};

static ChiFeature2InputDependency JPEGInputDependencyDescriptorGPU[] =
{
    {
        CHX_ARRAY_SIZE(JPEGInputPortDescriptorsNoGPU),
        &JPEGInputPortDescriptorsNoGPU[0],
    },
    {
        CHX_ARRAY_SIZE(JPEGInputPortDescriptorsGPU),
        &JPEGInputPortDescriptorsGPU[0],
    }

};

static ChiFeature2DependencyConfigDescriptor JPEGDependencyDescriptorGPU[] =
{
    {
        CHX_ARRAY_SIZE(JPEGInputDependencyDescriptorGPU),
        &JPEGInputDependencyDescriptorGPU[0],
        CHX_ARRAY_SIZE(JPEGInputPortDescriptorsAll),
        &JPEGInputPortDescriptorsAll[0],
        CHX_ARRAY_SIZE(JPEGOutputPortDescriptorsGPU),
        &JPEGOutputPortDescriptorsGPU[0],
    }
};


static ChiFeature2PipelineInfo JPEGPipelineDependencyConfigGPU[] =
{
    {
        0,
        CHX_ARRAY_SIZE(JPEGDependencyDescriptorGPU),
        ChiFeature2HandleType::DependencyConfigInfo,
        &JPEGDependencyDescriptorGPU[0],
    },
};

static const ChiFeature2SessionInfo JPEGStageInfoGPU[] =
{
    {
        0,
        CHX_ARRAY_SIZE(JPEGPipelineDependencyConfigGPU),
        &JPEGPipelineDependencyConfigGPU[0],
    }
};

static const ChiFeature2StageDescriptor JPEGStageDescriptorGPU[] =
{
    {
        0,
        "JPEG",
        CHX_ARRAY_SIZE(JPEGStageInfoGPU),
        &JPEGStageInfoGPU[0],
    },
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2Descriptor JPEGFeatureDescriptorGPU =
{
    static_cast<UINT32>(ChiFeature2Type::JPEG),
    "JPEGGPU",
    CHX_ARRAY_SIZE(JPEGStageDescriptorGPU),
    &JPEGStageDescriptorGPU[0],
    CHX_ARRAY_SIZE(JPEGSessionDescriptorsGPU),
    &JPEGSessionDescriptorsGPU[0],
};
