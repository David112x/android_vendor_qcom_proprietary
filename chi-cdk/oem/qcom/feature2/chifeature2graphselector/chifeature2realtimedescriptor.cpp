////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2realtimedescriptor.cpp
/// @brief CHI feature realtime descriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "chifeature2types.h"

static const ChiFeature2TargetDescriptor RealTimeFeatureTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_DISPLAY",
    },
    {
        "TARGET_BUFFER_RAW_OUT",
    },
    {
        "TARGET_BUFFER_FD",
    },
    {
        "TARGET_BUFFER_VIDEO",
    },
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2PortDescriptor RealTimeOutputPortDescriptors[] =
{
    {
        {0, 0, 0, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer},
        "Display_Out",
        &RealTimeFeatureTargetDescriptors[0],
    },
    {
        {0, 0, 1, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer},
        "Raw_Out",
        &RealTimeFeatureTargetDescriptors[1],
    },
    {
        {0, 0, 2, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer},
        "Fd_Out",
        &RealTimeFeatureTargetDescriptors[2],
    },
    {
        {0, 0, 3, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer},
        "Video_Out",
        &RealTimeFeatureTargetDescriptors[3],
    },
    {
        {0, 0, 4, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::ImageBuffer},
        "Raw_Callback",
        &RealTimeFeatureTargetDescriptors[1],
    },
    {
        {0, 0, 5, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData},
        "rt_metadata_out",
        NULL,
    },
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2PortDescriptor ZSLInputPortDescriptors[] =
{
    {
        {0, 0, 0, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::ImageBuffer},
        "ZSL_In_Raw",
        &RealTimeFeatureTargetDescriptors[1],
    },
    {
        {0, 0, 1, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::MetaData},
        "ZSL_Meta_In",
        NULL,
    },
    {
        {0, 0, 2, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::ImageBuffer},
        "ZSL_In_FD",
        &RealTimeFeatureTargetDescriptors[1],
    },
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2PortDescriptor ZSLInputPortDescriptorsNoFd[] =
{
    {
        {0, 0, 0, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::ImageBuffer},
        "ZSL_In_Raw",
        &RealTimeFeatureTargetDescriptors[1],
    },
    {
        {0, 0, 1, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::MetaData},
        "ZSL_Meta_In",
        NULL,
    },
};

static const ChiFeature2PipelineDescriptor RealTimePipelineDescriptors[] =
{
    {
        0,
        0,
        "RealTimeFeatureZSLPreviewRaw",
        ChiFeature2PipelineType::CHI,
        0,
        NULL,
        CHX_ARRAY_SIZE(RealTimeOutputPortDescriptors),
        &RealTimeOutputPortDescriptors[0],
    },
    {
        0,
        0,
        "RealTimeFeatureZSLPreviewRawYUV",
        ChiFeature2PipelineType::CHI,
        0,      // InputPortDescriptors
        NULL,
        CHX_ARRAY_SIZE(RealTimeOutputPortDescriptors),
        &RealTimeOutputPortDescriptors[0],
    },
    {
        0,
        0,
        "RealTimeFeatureNZSLSnapshotRDI",
        ChiFeature2PipelineType::CHI,
        0,      // InputPortDescriptors
        NULL,
        CHX_ARRAY_SIZE(RealTimeOutputPortDescriptors),
        &RealTimeOutputPortDescriptors[0],
    },
    {
        0,
        0,
        "RealTimeFeatureThirdCamera",
        ChiFeature2PipelineType::CHI,
        0,      // InputPortDescriptors
        NULL,
        CHX_ARRAY_SIZE(RealTimeOutputPortDescriptors),
        &RealTimeOutputPortDescriptors[0],
    },
    {
        0,
        0,
        "RealTimeFeatureZSLPreviewRaw_PVLT1080p_PGTV",
        ChiFeature2PipelineType::CHI,
        0,      // InputPortDescriptors
        NULL,
        CHX_ARRAY_SIZE(RealTimeOutputPortDescriptors),
        &RealTimeOutputPortDescriptors[0],
    },
};

static const ChiFeature2SessionDescriptor RealTimeSessionDescriptors[] =
{
    {
        0,
        "RealTime",
        CHX_ARRAY_SIZE(RealTimePipelineDescriptors),
        &RealTimePipelineDescriptors[0],
    },
};

static ChiFeature2InputDependency ZSLInputDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(ZSLInputPortDescriptors),
        &ZSLInputPortDescriptors[0],
    },
    {
        CHX_ARRAY_SIZE(ZSLInputPortDescriptorsNoFd),
        &ZSLInputPortDescriptorsNoFd[0],
    }
};

static ChiFeature2DependencyConfigDescriptor RealTimeDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(ZSLInputDependencyDescriptor),
        &ZSLInputDependencyDescriptor[0],
        0,
        NULL,
        CHX_ARRAY_SIZE(RealTimeOutputPortDescriptors),
        &RealTimeOutputPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo RealTimePipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(RealTimeDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &RealTimeDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo RealTimeStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(RealTimePipelineDependencyConfig),
        &RealTimePipelineDependencyConfig[0],
    }
};

static const ChiFeature2StageDescriptor RealTimeStageDescriptor[] =
{
    {
        0,
        "RealTime",
        CHX_ARRAY_SIZE(RealTimeStageInfo),
        &RealTimeStageInfo[0],
    },
};

// Informations about all internal links
static const ChiFeature2InternalLinkDesc ZSLInternalLinkDescriptors[] =
{
    // RDI Out ---> RDI In
    {
        &RealTimeOutputPortDescriptors[1],
        &ZSLInputPortDescriptors[0],
    },
    // rt_metadata_out ---> ZSL_Meta_In
    {
        &RealTimeOutputPortDescriptors[5],
        &ZSLInputPortDescriptors[1],
    },
    // FD out ---> FD in
    {
        &RealTimeOutputPortDescriptors[2],
        &ZSLInputPortDescriptors[2],
    },
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2Descriptor RealTimeFeatureDescriptor =
{
    0,
    "RealTime",
    CHX_ARRAY_SIZE(RealTimeStageDescriptor),
    &RealTimeStageDescriptor[0],
    CHX_ARRAY_SIZE(RealTimeSessionDescriptors),  // session descriptor
    &RealTimeSessionDescriptors[0],
    CHX_ARRAY_SIZE(ZSLInternalLinkDescriptors),
    &ZSLInternalLinkDescriptors[0],
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// @brief Definitions of Realtime feature descriptor specific for sw remosaic sensor.
/////        For sw remosaic, there're two pipelines in realtime feature, 'SnapshotRDIOnly' and 'SWRemosaic' pipeline.
/////        Due to different stage descriptors and internal dependencies from the normal realtime feature descriptors,
/////        it is more clear to use a separate feature descriptor for sw remosaic.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const ChiFeature2TargetDescriptor RealTimeQCFARawOutputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_CFA_FULLSIZE_RAW",
    },
};

static const ChiFeature2TargetDescriptor RealTimeSWRemosaicInputTargetDescriptors[] =
{
    {
        "TARGET_BUFFER_SWREMOSAIC_IN",
    },
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2PortDescriptor RealTimeQCFARawOutputPortDescriptors[] =
{
    {
        {1, 0, 0, ChiFeature2PortDirectionType::InternalOutput, ChiFeature2PortType::ImageBuffer},
        "QCFA_RAW_Out",
        &RealTimeQCFARawOutputTargetDescriptors[0],
    },
    {
        {1, 0, 1, ChiFeature2PortDirectionType::InternalOutput, ChiFeature2PortType::MetaData},
        "qcafa_raw_metadata_out",
        NULL,
    },
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2PortDescriptor SWRemosaicInputPortDescriptors[] =
{
    {
        {0, 0, 0, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::ImageBuffer},
        "SWRemosaic_RAW_In",
        &RealTimeSWRemosaicInputTargetDescriptors[0],
    },
    {
        {0, 0, 1, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::MetaData},
        "SWRemosaic_metadata_in",
        NULL,
    },
};

static ChiFeature2DependencyConfigDescriptor RealTimeQCFARawDependencyDescriptor[] =
{
    {
        0,
        NULL,
        0,
        NULL,
        CHX_ARRAY_SIZE(RealTimeQCFARawOutputPortDescriptors),
        &RealTimeQCFARawOutputPortDescriptors[0],
    },
};

static ChiFeature2PipelineInfo RealTimeQCFARawPipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(RealTimeQCFARawDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &RealTimeQCFARawDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo RealTimeQCFARawStageInfo[] =
{
    {
        1,
        CHX_ARRAY_SIZE(RealTimeQCFARawPipelineDependencyConfig),
        &RealTimeQCFARawPipelineDependencyConfig[0],
    }
};

static ChiFeature2InputDependency SWRemosaicInputDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(SWRemosaicInputPortDescriptors),
        &SWRemosaicInputPortDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor SWRemosaicDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(SWRemosaicInputDependencyDescriptor),
        &SWRemosaicInputDependencyDescriptor[0],
        CHX_ARRAY_SIZE(SWRemosaicInputPortDescriptors),
        &SWRemosaicInputPortDescriptors[0],
        CHX_ARRAY_SIZE(RealTimeOutputPortDescriptors),
        &RealTimeOutputPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo SWRemosaicPipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(SWRemosaicDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &SWRemosaicDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo SWRemosaicStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(SWRemosaicPipelineDependencyConfig),
        &SWRemosaicPipelineDependencyConfig[0],
    }
};

static const ChiFeature2StageDescriptor RealTimeWithSWRemosaicStageDescriptor[] =
{
    {
        0,
        "QuadCFAFullsizeRaw",
        CHX_ARRAY_SIZE(RealTimeQCFARawStageInfo),
        &RealTimeQCFARawStageInfo[0],
    },
    {
        1,
        "SWRemosaic",
        CHX_ARRAY_SIZE(SWRemosaicStageInfo),
        &SWRemosaicStageInfo[0],
    },
};

static const ChiFeature2PipelineDescriptor RealTimeFeatureSWRemosaicPipelineDescriptors[] =
{
    {
        0,
        0,
        "RealtimeFeatureSWRemosaic",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(SWRemosaicInputPortDescriptors),
        &SWRemosaicInputPortDescriptors[0],
        CHX_ARRAY_SIZE(RealTimeOutputPortDescriptors),
        &RealTimeOutputPortDescriptors[0],
    },
};

static const ChiFeature2PipelineDescriptor RealTimeFeatureQCFARawPipelineDescriptors[] =
{
    {
        1,
        0,
        "QuadCFAFullSizeRaw",
        ChiFeature2PipelineType::CHI,
        0,      // InputPortDescriptors
        NULL,
        CHX_ARRAY_SIZE(RealTimeQCFARawOutputPortDescriptors),
        &RealTimeQCFARawOutputPortDescriptors[0],
    },
};

static const ChiFeature2SessionDescriptor RealTimeWithSWRemosaicSessionDescriptors[] =
{
    {
        0,
        "RealtimeSWRemosaic",
        CHX_ARRAY_SIZE(RealTimeFeatureSWRemosaicPipelineDescriptors),
        &RealTimeFeatureSWRemosaicPipelineDescriptors[0],
    },
    {
        1,
        "RealTimeQCFARaw",
        CHX_ARRAY_SIZE(RealTimeFeatureQCFARawPipelineDescriptors),
        &RealTimeFeatureQCFARawPipelineDescriptors[0],
    },
};

// Informations about all internal links
static const ChiFeature2InternalLinkDesc SWRemosaicInternalLinkDescriptors[] =
{
    // QCFA RAW Out ---> SW Remosaic In
    {
        &RealTimeQCFARawOutputPortDescriptors[0],
        &SWRemosaicInputPortDescriptors[0],
    },
    // qcfa_raw_metadata_out ---> SWRemosaic_Meta_In
    {
        &RealTimeQCFARawOutputPortDescriptors[1],
        &SWRemosaicInputPortDescriptors[1],
    },
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2Descriptor RealTimeFeatureWithSWRemosaicDescriptor =
{
    0,
    "RealTimeWithSWRemosaic",
    CHX_ARRAY_SIZE(RealTimeWithSWRemosaicStageDescriptor),
    &RealTimeWithSWRemosaicStageDescriptor[0],
    CHX_ARRAY_SIZE(RealTimeWithSWRemosaicSessionDescriptors),  // session descriptor
    &RealTimeWithSWRemosaicSessionDescriptors[0],
    CHX_ARRAY_SIZE(SWRemosaicInternalLinkDescriptors),
    &SWRemosaicInternalLinkDescriptors[0],
};
