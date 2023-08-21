////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2serializerdescriptor.cpp
/// @brief Serializer feature descriptor definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graphselector.h"
#include "chifeature2types.h"

CDK_VISIBILITY_PUBLIC extern const ChiFeature2PortDescriptor SerializerInputPortDescriptors[] =
{
    // Make sure the input as the order RDI/Meta/RDI/Meta
    {
        {0, 0, 0, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer},
        "Serializer_RDI_In0",
        NULL,
    },
    {
        {0, 0, 1, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData},
        "Serializer_Metadata_In0",
        NULL,
    },
    {
        {0, 0, 2, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer},
        "Serializer_RDI_In1",
        NULL,
    },
    {
        {0, 0, 3, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData},
        "Serializer_Metadata_In1",
        NULL,
    }
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2PortDescriptor SerializerOutputPortDescriptors[] =
{
    // Make sure the output as the order RDI/Meta
    {
        {0, 0, 0, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer},
        "Serializer_RDI_Out0",
        NULL,
    },
    {
        {0, 0, 1, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData},
        "Serializer_Output_Metadata0",
        NULL,
    }
};

static const ChiFeature2PipelineDescriptor SerializerPipelineDescriptors[] =
{
    {
        0,
        0,
        "Serializer",
        ChiFeature2PipelineType::Virtual,
        CHX_ARRAY_SIZE(SerializerInputPortDescriptors),
        &SerializerInputPortDescriptors[0],
        CHX_ARRAY_SIZE(SerializerOutputPortDescriptors),
        &SerializerOutputPortDescriptors[0],
    },
};

static const ChiFeature2SessionDescriptor SerializerSessionDescriptors[] =
{
    {
        0,
        "Serializer",
        CHX_ARRAY_SIZE(SerializerPipelineDescriptors),
        &SerializerPipelineDescriptors[0],
    },
};

static ChiFeature2InputDependency SerializerInputDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(SerializerInputPortDescriptors),
        &SerializerInputPortDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor SerializerDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(SerializerInputDependencyDescriptor),
        &SerializerInputDependencyDescriptor[0],
        CHX_ARRAY_SIZE(SerializerInputPortDescriptors),
        &SerializerInputPortDescriptors[0],
        CHX_ARRAY_SIZE(SerializerOutputPortDescriptors),
        &SerializerOutputPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo SerializerPipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(SerializerDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &SerializerDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo SerializerStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(SerializerPipelineDependencyConfig),
        &SerializerPipelineDependencyConfig[0],
    }
};

static const ChiFeature2StageDescriptor SerializerStageDescriptor[] =
{
    {
        0,
        "SerializerInit",
        CHX_ARRAY_SIZE(SerializerStageInfo),
        &SerializerStageInfo[0],
    },
};

extern const ChiFeature2Descriptor SerializerFeatureDescriptor =
{
    static_cast<UINT32>(ChiFeature2Type::SERIALIZER),
    "Serializer",
    CHX_ARRAY_SIZE(SerializerStageDescriptor),
    &SerializerStageDescriptor[0],
    CHX_ARRAY_SIZE(SerializerSessionDescriptors),
    &SerializerSessionDescriptors[0],
};
