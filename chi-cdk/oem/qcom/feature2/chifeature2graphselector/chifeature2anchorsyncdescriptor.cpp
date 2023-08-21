////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2anchorsyncdescriptor.cpp
/// @brief Bayer-to-YUV feature descriptor definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graphselector.h"
#include "chifeature2types.h"

static const ChiFeature2TargetDescriptor AnchorSyncFeatureInputRDITargetDescriptors[] =
{
    {
        "TARGET_BUFFER_RAW",
    },
};

static const ChiFeature2TargetDescriptor AnchorSyncFeatureInputFDTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_FD",
    },
};

static const ChiFeature2TargetDescriptor AnchorSyncFeatureOutputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_RAW",
    },
};

extern const ChiFeature2PortDescriptor AnchorSyncInputPortDescriptors[] =
{
    // Make sure the input as the order RDI/FD/Meta/FDI/FD/Meta
    {
        {0, 0, 0, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer},
        "AnchorSync_RDIIn0",
        &AnchorSyncFeatureInputRDITargetDescriptors[0],
    },
    {
        {0, 0, 1, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer},
        "AnchorSync_FDIn0",
        &AnchorSyncFeatureInputFDTargetDescriptors[0],
    },
    {
        { 0, 0, 2, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData},
        "AnchorSync_Input_Metadata0",
        NULL,
    },
    {
        {0, 0, 3, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer},
        "AnchorSync_RDIIn1",
        &AnchorSyncFeatureInputRDITargetDescriptors[0],
    },
    {
        {0, 0, 4, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer},
        "AnchorSync_FDIn1",
        &AnchorSyncFeatureInputFDTargetDescriptors[0],
    },
    {
        { 0, 0, 5, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData},
        "AnchorSync_Input_Metadata1",
        NULL,
    },
    {
        {0, 0, 6, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer},
        "AnchorSync_RDIIn2",
        &AnchorSyncFeatureInputRDITargetDescriptors[0],
    },
    {
        {0, 0, 7, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer},
        "AnchorSync_FDIn2",
        &AnchorSyncFeatureInputFDTargetDescriptors[0],
    },
    {
        { 0, 0, 8, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData},
        "AnchorSync_Input_Metadata2",
        NULL,
    },

};

extern const ChiFeature2PortDescriptor AnchorSyncOutputPortDescriptors[] =
{
    // Make sure the output as the order RDI/Meta/RDI/Meta
    {
        {0, 0, 0, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer},
        "AnchorSync_RDIOut0",
        NULL,
    },
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData },
        "AnchorSync_Metadata_Out0",
        NULL,
    },
    {
        {0, 0, 2, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer},
        "AnchorSync_RDIOut1",
        NULL,
    },
    {
        { 0, 0, 3, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData },
        "AnchorSync_Metadata_Out1",
        NULL,
    },
    {
        {0, 0, 4, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer},
        "AnchorSync_RDIOut0",
        NULL,
    },
    {
        { 0, 0, 5, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData },
        "AnchorSync_Metadata_Out2",
        NULL,
    },
    {
        {0, 0, 6, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer},
        "AnchorSync_DepthCalculationRDIOut",
        NULL,
    },
    {
        { 0, 0, 7, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData },
        "AnchorSync_DepthCalculationMetadataOut",
        NULL,
    },
};

static const ChiFeature2PipelineDescriptor AnchorSyncPipelineDescriptors[] =
{
    {
        0,
        0,
        "AnchorSync",
        ChiFeature2PipelineType::Virtual,
        CHX_ARRAY_SIZE(AnchorSyncInputPortDescriptors),
        &AnchorSyncInputPortDescriptors[0],
        CHX_ARRAY_SIZE(AnchorSyncOutputPortDescriptors),
        &AnchorSyncOutputPortDescriptors[0],
    },
};

static const ChiFeature2SessionDescriptor AnchorSyncSessionDescriptors[] =
{
    {
        0,
        "AnchorSync",
        CHX_ARRAY_SIZE(AnchorSyncPipelineDescriptors),
        &AnchorSyncPipelineDescriptors[0],
    },
};

static ChiFeature2InputDependency AnchorSyncInputDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(AnchorSyncInputPortDescriptors),
        &AnchorSyncInputPortDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor AnchorSyncDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(AnchorSyncInputDependencyDescriptor),
        &AnchorSyncInputDependencyDescriptor[0],
        CHX_ARRAY_SIZE(AnchorSyncInputPortDescriptors),
        &AnchorSyncInputPortDescriptors[0],
        CHX_ARRAY_SIZE(AnchorSyncOutputPortDescriptors),
        &AnchorSyncOutputPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo AnchorSyncPipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(AnchorSyncDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &AnchorSyncDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo AnchorSyncStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(AnchorSyncPipelineDependencyConfig),
        &AnchorSyncPipelineDependencyConfig[0],
    }
};

static const ChiFeature2StageDescriptor AnchorSyncStageDescriptor[] =
{
    {
        0,
        "AnchorSyncInit",
        CHX_ARRAY_SIZE(AnchorSyncStageInfo),
        &AnchorSyncStageInfo[0],
    },
};

extern const ChiFeature2Descriptor AnchorSyncFeatureDescriptor =
{
    static_cast<UINT32>(ChiFeature2Type::ANCHORSYNC),
    "AnchorSync",
    CHX_ARRAY_SIZE(AnchorSyncStageDescriptor),
    &AnchorSyncStageDescriptor[0],
    CHX_ARRAY_SIZE(AnchorSyncSessionDescriptors),
    &AnchorSyncSessionDescriptors[0],
};
