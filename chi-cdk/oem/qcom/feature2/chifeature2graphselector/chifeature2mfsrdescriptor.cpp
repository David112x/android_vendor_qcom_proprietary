////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2mfsrdescriptor.cpp
/// @brief MFSR feature descriptor definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graphselector.h"
#include "chifeature2types.h"

static const ChiFeature2TargetDescriptor MfsrPrefilterInputTargetDescriptors[] =
{
    // RAW
    {
        "TARGET_BUFFER_RAW",
    },
};

static const ChiFeature2TargetDescriptor MfsrPrefilterOutputTargetDescriptors[] =
{
    // FULL
    {
        "TARGET_BUFFER_FULLREF_UBWCTP10_OUT",
    },

    // DS4
    {
        "TARGET_BUFFER_DS4REF_PD10_OUT",
    },

    // DS16
    {
        "TARGET_BUFFER_DS16REF_PD10_OUT",
    },

    // DS64
    {
        "TARGET_BUFFER_DS64REF_PD10_OUT",
    },

    // REG
    {
        "TARGET_BUFFER_REG_UBWCNV124R_OUT",
    },

    // CVP DME CXT
    {
        "TARGET_BUFFER_CVP_DMECONTEXT_OUT",
    },
};

static const ChiFeature2TargetDescriptor MfsrBlendInputTargetDescriptors[] =
{
    // RAW
    {
        "TARGET_BUFFER_RAW",
    },

    // FULL
    {
        "TARGET_BUFFER_FULLREF_UBWCTP10_IN",
    },

    // DS4
    {
        "TARGET_BUFFER_DS4REF_PD10_IN",
    },

    // DS16
    {
        "TARGET_BUFFER_DS16REF_PD10_IN",
    },

    // DS64
    {
        "TARGET_BUFFER_DS64REF_PD10_IN",
    },

    // REG
    {
        "TARGET_BUFFER_REG_UBWCNV124R_IN",
    },

    // CVP DME CXT
    {
        "TARGET_BUFFER_CVP_DMECONTEXT_IN",
    },
};

static const ChiFeature2TargetDescriptor MfsrBlendOutputTargetDescriptors[] =
{
    // FULL
    {
        "TARGET_BUFFER_FULLREF_UBWCTP10_OUT",
    },

    // DS4
    {
        "TARGET_BUFFER_DS4REF_PD10_OUT",
    },

    // CVP
    {
        "TARGET_BUFFER_REG_UBWCNV124R_OUT",
    },

    // CVP DME CXT
    {
        "TARGET_BUFFER_CVP_DMECONTEXT_OUT",
    },
};

static const ChiFeature2TargetDescriptor MfsrPostFilterInputTargetDescriptors[] =
{
    // FULL
    {
        "TARGET_BUFFER_FULLREF_UBWCTP10_IN",
    },
    // DS4
    {
        "TARGET_BUFFER_DS4REF_PD10_IN",
    },
};

static const ChiFeature2TargetDescriptor MfsrPostFilterOutputTargetDescriptors[] =
{
    // Video
    {
        "TARGET_BUFFER_YUV",
    },
};

static const ChiFeature2PortDescriptor MfsrPostFilterInputPortDescriptors[] =
{
    // Blend Loop Full
    {
        { 0, 2, 0, ChiFeature2PortDirectionType::InternalInput },
        "Postfilter_UBWC_In",
        &MfsrPostFilterInputTargetDescriptors[0],
    },
    // Blend Loop DS4
    {
        { 0, 2, 1, ChiFeature2PortDirectionType::InternalInput },
        "Postfilter_PD10DS4_In",
        &MfsrPostFilterInputTargetDescriptors[1],
    },

    {
        { 0, 2, 4, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::MetaData },
        "MFSR_Postfilter_Input_Metadata",
        NULL,
    }
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2PortDescriptor MfsrPostFilterOutPutPortDescriptors[] =
{
    // Video
    {
        { 0, 2, 0, ChiFeature2PortDirectionType::ExternalOutput },
        "MFSR_Postfilter_YUV_OUT",
        &MfsrPostFilterOutputTargetDescriptors[0],
    },

    {
        { 0, 2, 1, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData },
        "MFSR_Postfilter_YUV_Metadata_Out",
        NULL,
    },
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2PortDescriptor MfsrBlendInputPortDescriptors[] =
{
    // RAW
    {
        { 0, 1, 0, ChiFeature2PortDirectionType::ExternalInput },
        "Blend_RDI_In",
        &MfsrBlendInputTargetDescriptors[0],
    },

    // Prefilter FULL
    {
        { 0, 1, 1, ChiFeature2PortDirectionType::InternalInput },
        "Blend_UBWC_IN",
        &MfsrBlendInputTargetDescriptors[1],
    },

    // Prefilter DS4
    {
        { 0, 1, 2, ChiFeature2PortDirectionType::InternalInput },
        "Blend_PD10DS4_In",
        &MfsrBlendInputTargetDescriptors[2],
    },

    // Prefilter DS16
    {
        { 0, 1, 3, ChiFeature2PortDirectionType::InternalInput },
        "Blend_PD10DS16_In",
        &MfsrBlendInputTargetDescriptors[3],
    },

    // Prefilter DS64
    {
        { 0, 1, 4, ChiFeature2PortDirectionType::InternalInput },
        "Blend_PD10DS64_In",
        &MfsrBlendInputTargetDescriptors[4],
    },

    // Prefilter REG
    {
        { 0, 1, 5, ChiFeature2PortDirectionType::InternalInput },
        "Blend_UBWCNV12-4R_IN",
        &MfsrBlendInputTargetDescriptors[5],
    },

    // BlendInit FULL
    {
        { 0, 1, 6, ChiFeature2PortDirectionType::InternalInput },
        "Blend_UBWC_IN",
        &MfsrBlendInputTargetDescriptors[1],
    },

    {
        { 0, 1, 7, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::MetaData },
        "MFSR_Blend_Input_Metadata",
        NULL,
    },

    // Prefilter CVP DME CXT
    {
        { 0, 1, 8, ChiFeature2PortDirectionType::InternalInput },
        "Blend_DMECONTEXT_IN",
        &MfsrBlendInputTargetDescriptors[6],
    },
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2PortDescriptor MfsrBlendOutPutPortDescriptors[] =
{
    // FULL
    {
        { 0, 1, 0, ChiFeature2PortDirectionType::InternalOutput },
        "Blend_UBWC_OUT",
        &MfsrBlendOutputTargetDescriptors[0],
    },

    // DS4
    {
        { 0, 1, 1, ChiFeature2PortDirectionType::InternalOutput },
        "Blend_PD10DS4_OUT",
        &MfsrBlendOutputTargetDescriptors[1],
    },

    {
        { 0, 1, 2, ChiFeature2PortDirectionType::InternalOutput, ChiFeature2PortType::MetaData },
        "MFSR_blend_output_Metadata",
        NULL,
    },

    // CVP Image out
    {
        { 0, 1, 3, ChiFeature2PortDirectionType::InternalOutput },
        "Blend_UBWCNV12-4R_OUT",
        &MfsrBlendOutputTargetDescriptors[2],
    },

    // CVP DME CXT
    {
        { 0, 1, 4, ChiFeature2PortDirectionType::InternalOutput },
        "Blend_DMECONTEXT_OUT",
        &MfsrBlendOutputTargetDescriptors[3],
    },
};

extern const ChiFeature2PortDescriptor MfsrPrefilterInputPortDescriptors[] =
{
    // RAW
    {
        { 0, 0, 0, ChiFeature2PortDirectionType::ExternalInput },
        "Prefilter_RDI_In",
        &MfsrPrefilterInputTargetDescriptors[0],
    },
};

extern const ChiFeature2PortDescriptor MfsrPrefilterInputPortDescriptorsConfig[] =
{
    {
        { 0, 0, 0, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer },
        "Prefilter_RDI_In",
        &MfsrPrefilterInputTargetDescriptors[0],
    },

    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData },
        "MFSR_Prefilter_Input_Metadata",
        NULL,
    }

};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2PortDescriptor MfsrPrefilterOutPutPortDescriptors[] =
{
    // Full
    {
        { 0, 0, 0, ChiFeature2PortDirectionType::InternalOutput },
        "Prefilter_UBWC_OUT",
        &MfsrPrefilterOutputTargetDescriptors[0],
    },

    // DS4
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::InternalOutput },
        "Prefilter_PD10DS4_OUT",
        &MfsrPrefilterOutputTargetDescriptors[1],
    },

    // DS16
    {
        { 0, 0, 2, ChiFeature2PortDirectionType::InternalOutput },
        "Prefilter_PD10DS16_OUT",
        &MfsrPrefilterOutputTargetDescriptors[2],
    },

    // DS64
    {
        { 0, 0, 3, ChiFeature2PortDirectionType::InternalOutput },
        "Prefilter_PD10DS64_OUT",
        &MfsrPrefilterOutputTargetDescriptors[3],
    },

    // REG
    {
        { 0, 0, 4, ChiFeature2PortDirectionType::InternalOutput },
        "Prefilter_UBWCNV12-4R_OUT",
        &MfsrPrefilterOutputTargetDescriptors[4],
    },

    {
        { 0, 0, 5, ChiFeature2PortDirectionType::InternalOutput, ChiFeature2PortType::MetaData },
        "MFSR_Prefilter_output_Metadata",
        NULL,
    },

    // CVP DME CXT
    {
        { 0, 0, 6, ChiFeature2PortDirectionType::InternalOutput },
        "Prefilter_DMECONTEXT_OUT",
        &MfsrPrefilterOutputTargetDescriptors[5],
    },
};

static const ChiFeature2PipelineDescriptor MFSRPipelineDescriptors[] =
{
    {
        0,
        0,
        "MfsrPrefilter",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(MfsrPrefilterInputPortDescriptorsConfig),
        &MfsrPrefilterInputPortDescriptorsConfig[0],
        CHX_ARRAY_SIZE(MfsrPrefilterOutPutPortDescriptors),
        &MfsrPrefilterOutPutPortDescriptors[0],
    },

    {
        0,
        1,
        "MfsrBlend",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(MfsrBlendInputPortDescriptors),
        &MfsrBlendInputPortDescriptors[0],
        CHX_ARRAY_SIZE(MfsrBlendOutPutPortDescriptors),
        &MfsrBlendOutPutPortDescriptors[0],
    },

    {
        0,
        2,
        "MfsrPostFilter",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(MfsrPostFilterInputPortDescriptors),
        &MfsrPostFilterInputPortDescriptors[0],
        CHX_ARRAY_SIZE(MfsrPostFilterOutPutPortDescriptors),
        &MfsrPostFilterOutPutPortDescriptors[0],
    },
};

static const ChiFeature2SessionDescriptor MFSRSessionDescriptors[] =
{
    {
        0,
        "MFSR",
        CHX_ARRAY_SIZE(MFSRPipelineDescriptors),
        &MFSRPipelineDescriptors[0],
    },
};

static const UINT32 MFSRSessionID[] =
{
    0,
};




/**************** Post Filter Stage ****************/

static const ChiFeature2PortDescriptor PostFilterInputPortDependencyDescriptors[] =
{
    // Blend Full
    {
        { 0, 2, 0, ChiFeature2PortDirectionType::InternalInput },
        "Postfilter_UBWC_In",
        &MfsrPostFilterInputTargetDescriptors[0],
    },
    // Blend DS4
    {
        { 0, 2, 1, ChiFeature2PortDirectionType::InternalInput },
        "Postfilter_PD10DS4_In",
        &MfsrPostFilterInputTargetDescriptors[1],
    },
};

static const ChiFeature2PortDescriptor PostFilterNoDS64InputPortDependencyDescriptors[] =
{
    // Blend Full
    {
        { 0, 2, 0, ChiFeature2PortDirectionType::InternalInput },
        "Postfilter_UBWC_In",
        &MfsrPostFilterInputTargetDescriptors[0],
    },
    // Blend DS4
    {
        { 0, 2, 1, ChiFeature2PortDirectionType::InternalInput },
        "Postfilter_PD10DS4_In",
        &MfsrPostFilterInputTargetDescriptors[1],
    },
};

static ChiFeature2InputDependency PostFilterInputPortDependency[] =
{
    {
        CHX_ARRAY_SIZE(PostFilterInputPortDependencyDescriptors),
        &PostFilterInputPortDependencyDescriptors[0],
    },

    {
        CHX_ARRAY_SIZE(PostFilterNoDS64InputPortDependencyDescriptors),
        &PostFilterNoDS64InputPortDependencyDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor PostFilterDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(PostFilterInputPortDependency),
        &PostFilterInputPortDependency[0],
        CHX_ARRAY_SIZE(MfsrPostFilterInputPortDescriptors),
        &MfsrPostFilterInputPortDescriptors[0],
        CHX_ARRAY_SIZE(MfsrPostFilterOutPutPortDescriptors),
        &MfsrPostFilterOutPutPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo PostFilterPipelineDependencyConfig[] =
{
    {
        2,
        CHX_ARRAY_SIZE(PostFilterDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &PostFilterDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo PostFilterStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(PostFilterPipelineDependencyConfig),
        &PostFilterPipelineDependencyConfig[0],
    }
};




/*********** Blend Loop Stage ***********************/

static const ChiFeature2PortDescriptor BlendLoopInputPortDescriptors[] =
{
    // BlendInit Full
    {
        { 0, 1, 6, ChiFeature2PortDirectionType::InternalInput },
        "BlendLoop_UBWC_In",
        &MfsrBlendInputTargetDescriptors[1],
    },

    // Prefilter DS4
    {
        { 0, 1, 2, ChiFeature2PortDirectionType::InternalInput },
        "BlendLoop_PD10DS4_In",
        &MfsrBlendInputTargetDescriptors[2],
    },

    // Prefilter DS16
    {
        { 0, 1, 3, ChiFeature2PortDirectionType::InternalInput },
        "BlendLoop_PD10DS16_In",
        &MfsrBlendInputTargetDescriptors[3],
    },

    // Prefilter DS64
    {
        { 0, 1, 4, ChiFeature2PortDirectionType::InternalInput },
        "BlendLoop_PD10DS64_In",
        &MfsrBlendInputTargetDescriptors[4],
    },

    // Prefilter REG
    {
        { 0, 1, 5, ChiFeature2PortDirectionType::InternalInput },
        "BlendLoop_UBWCNV12-4R_IN",
        &MfsrBlendInputTargetDescriptors[5],
    },

    // RAW
    {
        { 0, 1, 0, ChiFeature2PortDirectionType::ExternalInput },
        "BlendLoop_RDI_In",
        &MfsrBlendInputTargetDescriptors[0],
    },

    {
        { 0, 1, 7, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::MetaData },
        "MFSR_BlendLoop_Input_Metadata",
        NULL,
    },

    // Prefilter CVP DME CXT
    {
        { 0, 1, 8, ChiFeature2PortDirectionType::InternalInput },
        "BlendLoop_DMECONTEXT_IN",
        &MfsrBlendInputTargetDescriptors[6],
    },
};

static const ChiFeature2PortDescriptor BlendLoopOutPutPortDescriptors[] =
{
    {
        { 0, 1, 0, ChiFeature2PortDirectionType::InternalOutput },
        "BlendLoop_UBWC_OUT",
        &MfsrBlendOutputTargetDescriptors[0],
    },

    {
        { 0, 1, 1, ChiFeature2PortDirectionType::InternalOutput },
        "BlendLoop_PD10DS4_OUT",
        &MfsrBlendOutputTargetDescriptors[1],
    },

    {
        { 0, 1, 2, ChiFeature2PortDirectionType::InternalOutput, ChiFeature2PortType::MetaData },
        "MFSRLoop_blend_output_Metadata",
        NULL,
    },

    {
        { 0, 1, 3, ChiFeature2PortDirectionType::InternalOutput },
        "BlendLoop_UBWCNV12-4R_OUT",
        &MfsrBlendOutputTargetDescriptors[2],
    },

    {
        { 0, 1, 4, ChiFeature2PortDirectionType::InternalOutput },
        "BlendLoop_DMECONTEXT_OUT",
        &MfsrBlendOutputTargetDescriptors[3],
    },
};

static const ChiFeature2PortDescriptor BlendLoopInputPortDependencyDescriptors[] =
{
    // BlendInit Full
    {
        { 0, 1, 6, ChiFeature2PortDirectionType::InternalInput },
        "BlendLoop_UBWC_In",
        &MfsrBlendInputTargetDescriptors[1],
    },

    // Prefilter DS4
    {
        { 0, 1, 2, ChiFeature2PortDirectionType::InternalInput },
        "BlendLoop_PD10DS4_In",
        &MfsrBlendInputTargetDescriptors[2],
    },

    // Prefilter DS16
    {
        { 0, 1, 3, ChiFeature2PortDirectionType::InternalInput },
        "BlendLoop_PD10DS16_In",
        &MfsrBlendInputTargetDescriptors[3],
    },

    // Prefilter DS64
    {
        { 0, 1, 4, ChiFeature2PortDirectionType::InternalInput },
        "BlendLoop_PD10DS64_In",
        &MfsrBlendInputTargetDescriptors[4],
    },

    // Prefilter REG
    {
        { 0, 1, 5, ChiFeature2PortDirectionType::InternalInput },
        "BlendLoop_UBWCNV12-4R_IN",
        &MfsrBlendInputTargetDescriptors[5],
    },

    // Prefilter CVP DME CXT
    {
        { 0, 1, 8, ChiFeature2PortDirectionType::InternalInput },
        "BlendLoop_DMECONTEXT_IN",
        &MfsrBlendInputTargetDescriptors[6],
    },
};


static const ChiFeature2PortDescriptor BlendLoopNoDS64InputPortDependencyDescriptors[] =
{
    // BlendInit Full
    {
        { 0, 1, 6, ChiFeature2PortDirectionType::InternalInput },
        "BlendLoop_UBWC_In",
        &MfsrBlendInputTargetDescriptors[1],
    },

    // Prefilter DS4
    {
        { 0, 1, 2, ChiFeature2PortDirectionType::InternalInput },
        "BlendLoop_PD10DS4_In",
        &MfsrBlendInputTargetDescriptors[2],
    },

    // Prefilter DS16
    {
        { 0, 1, 3, ChiFeature2PortDirectionType::InternalInput },
        "BlendLoop_PD10DS16_In",
        &MfsrBlendInputTargetDescriptors[3],
    },

    // Prefilter REG
    {
        { 0, 1, 5, ChiFeature2PortDirectionType::InternalInput },
        "BlendLoop_UBWCNV12-4R_IN",
        &MfsrBlendInputTargetDescriptors[5],
    },

    // Prefilter CVP DME CXT
    {
        { 0, 1, 8, ChiFeature2PortDirectionType::InternalInput },
        "BlendLoop_DMECONTEXT_IN",
        &MfsrBlendInputTargetDescriptors[6],
    },
};

static ChiFeature2InputDependency BlendLoopInputPortDependency[] =
{
    {
        CHX_ARRAY_SIZE(BlendLoopInputPortDependencyDescriptors),
        &BlendLoopInputPortDependencyDescriptors[0],
    },

    {
        CHX_ARRAY_SIZE(BlendLoopNoDS64InputPortDependencyDescriptors),
        &BlendLoopNoDS64InputPortDependencyDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor BlendLoopDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(BlendLoopInputPortDependency),
        &BlendLoopInputPortDependency[0],
        CHX_ARRAY_SIZE(BlendLoopInputPortDescriptors),
        &BlendLoopInputPortDescriptors[0],
        CHX_ARRAY_SIZE(BlendLoopOutPutPortDescriptors),
        &BlendLoopOutPutPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo BlendLoopPipelineDependencyConfig[] =
{
    {
        1,
        CHX_ARRAY_SIZE(BlendLoopDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &BlendLoopDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo BlendLoopStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(BlendLoopPipelineDependencyConfig),
        &BlendLoopPipelineDependencyConfig[0],
    }
};





/************* Blend Init Stage *****************/

static const ChiFeature2PortDescriptor BlendInitInputPortDescriptors[] =
{
    // Perfilter Full
    {
        { 0, 1, 1, ChiFeature2PortDirectionType::InternalInput },
        "BlendInit_UBWC_In",
        &MfsrBlendInputTargetDescriptors[1],
    },

    // Prefilter DS4
    {
        { 0, 1, 2, ChiFeature2PortDirectionType::InternalInput },
        "BlendInit_PD10DS4_In",
        &MfsrBlendInputTargetDescriptors[2],
    },

    // Prefilter DS16
    {
        { 0, 1, 3, ChiFeature2PortDirectionType::InternalInput },
        "BlendInit_PD10DS16_In",
        &MfsrBlendInputTargetDescriptors[3],
    },

    // Prefilter DS64
    {
        { 0, 1, 4, ChiFeature2PortDirectionType::InternalInput },
        "BlendInit_PD10DS64_In",
        &MfsrBlendInputTargetDescriptors[4],
    },

    // Prefilter REG
    {
        { 0, 1, 5, ChiFeature2PortDirectionType::InternalInput },
        "BlendInit_UBWCNV12-4R_IN",
        &MfsrBlendInputTargetDescriptors[5],
    },

    // RAW
    {
        { 0, 1, 0, ChiFeature2PortDirectionType::ExternalInput },
        "BlendInit_RDI_In",
        &MfsrBlendInputTargetDescriptors[0],
    },

    // prefilter Meta
    {
        { 0, 1, 7, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::MetaData },
        "MFSR_BlendInit_Input_Metadata",
        NULL,
    },

    // Prefilter CVP DME CXT
    {
        { 0, 1, 8, ChiFeature2PortDirectionType::InternalInput },
        "BlendInit_DMECONTEXT_IN",
        &MfsrBlendInputTargetDescriptors[6],
    },

};

static const ChiFeature2PortDescriptor BlendInitOutPutPortDescriptors[] =
{
    {
        { 0, 1, 0, ChiFeature2PortDirectionType::InternalOutput },
        "BlendInit_UBWC_OUT",
        &MfsrBlendOutputTargetDescriptors[0],
    },

    {
        { 0, 1, 1, ChiFeature2PortDirectionType::InternalOutput },
        "BlendInit_PD10DS4_OUT",
        &MfsrBlendOutputTargetDescriptors[1],
    },

    {
        { 0, 1, 2, ChiFeature2PortDirectionType::InternalOutput },
        "BlendInit_UBWCNV12-4R_OUT",
        &MfsrBlendOutputTargetDescriptors[2],
    },

    {
        { 0, 1, 3, ChiFeature2PortDirectionType::InternalOutput },
        "BlendInit_DMECONTEXT_OUT",
        &MfsrBlendOutputTargetDescriptors[3],
    },
};

static const ChiFeature2PortDescriptor BlendInitInputPortDependencyDescriptors[] =
{
    // Prefilter Full
    {
        { 0, 1, 1, ChiFeature2PortDirectionType::InternalInput },
        "BlendInit_UBWC_In",
        &MfsrBlendInputTargetDescriptors[1],
    },

    // Prefilter DS4
    {
        { 0, 1, 2, ChiFeature2PortDirectionType::InternalInput },
        "BlendInit_PD10DS4_In",
        &MfsrBlendInputTargetDescriptors[2],
    },

    // Prefilter DS16
    {
        { 0, 1, 3, ChiFeature2PortDirectionType::InternalInput },
        "BlendInit_PD10DS16_In",
        &MfsrBlendInputTargetDescriptors[3],
    },

    // Prefilter DS64
    {
        { 0, 1, 4, ChiFeature2PortDirectionType::InternalInput },
        "BlendInit_PD10DS64_In",
        &MfsrBlendInputTargetDescriptors[4],
    },

    // Prefilter REG
    {
        { 0, 1, 5, ChiFeature2PortDirectionType::InternalInput },
        "BlendInit_UBWCNV12-4R_IN",
        &MfsrBlendInputTargetDescriptors[5],
    },

    // Prefilter Meta
    {
        { 0, 1, 7, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::MetaData },
        "MFSR_BlendInit_Input_Metadata",
        NULL,
    },

    // Prefilter CVP DME CXT
    {
        { 0, 1, 8, ChiFeature2PortDirectionType::InternalInput },
        "BlendInit_DMECONTEXT_IN",
        &MfsrBlendInputTargetDescriptors[6],
    },
};


static const ChiFeature2PortDescriptor BlendInitNoDS64InputPortDependencyDescriptors[] =
{
    // Prefilter Full
    {
        { 0, 1, 1, ChiFeature2PortDirectionType::InternalInput },
        "BlendInit_UBWC_In",
        &MfsrBlendInputTargetDescriptors[1],
    },

    // Prefilter DS4
    {
        { 0, 1, 2, ChiFeature2PortDirectionType::InternalInput },
        "BlendInit_PD10DS4_In",
        &MfsrBlendInputTargetDescriptors[2],
    },

    // Prefilter DS16
    {
        { 0, 1, 3, ChiFeature2PortDirectionType::InternalInput },
        "BlendInit_PD10DS16_In",
        &MfsrBlendInputTargetDescriptors[3],
    },

    // Prefilter REG
    {
        { 0, 1, 5, ChiFeature2PortDirectionType::InternalInput },
        "BlendInit_UBWCNV12-4R_IN",
        &MfsrBlendInputTargetDescriptors[5],
    },

    // Prefilter Meta
    {
        { 0, 1, 7, ChiFeature2PortDirectionType::InternalInput, ChiFeature2PortType::MetaData },
        "MFSR_BlendInit_Input_Metadata",
        NULL,
    },

    // Prefilter CVP DME CXT
    {
        { 0, 1, 8, ChiFeature2PortDirectionType::InternalInput },
        "BlendInit_DMECONTEXT_IN",
        &MfsrBlendInputTargetDescriptors[6],
    },
};

static ChiFeature2InputDependency BlendInitInputPortDependency[] =
{
    {
        CHX_ARRAY_SIZE(BlendInitInputPortDependencyDescriptors),
        &BlendInitInputPortDependencyDescriptors[0],
    },

    {
        CHX_ARRAY_SIZE(BlendInitNoDS64InputPortDependencyDescriptors),
        &BlendInitNoDS64InputPortDependencyDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor BlendInitDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(BlendInitInputPortDependency),
        &BlendInitInputPortDependency[0],
        CHX_ARRAY_SIZE(BlendInitInputPortDescriptors),
        &BlendInitInputPortDescriptors[0],
        CHX_ARRAY_SIZE(BlendLoopOutPutPortDescriptors),
        &BlendLoopOutPutPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo BlendInitPipelineDependencyConfig[] =
{
    {
        1,
        CHX_ARRAY_SIZE(BlendInitDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &BlendInitDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo BlendInitStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(BlendInitPipelineDependencyConfig),
        &BlendInitPipelineDependencyConfig[0],
    }
};




/**************** Prefilter Stage ****************/

static ChiFeature2InputDependency PrefilterInputPortDependency[] =
{
    {
        CHX_ARRAY_SIZE(MfsrPrefilterInputPortDescriptorsConfig),
        &MfsrPrefilterInputPortDescriptorsConfig[0],
    },

};

static ChiFeature2DependencyConfigDescriptor PrefilterDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(PrefilterInputPortDependency),
        &PrefilterInputPortDependency[0],
        CHX_ARRAY_SIZE(MfsrPrefilterInputPortDescriptorsConfig),
        &MfsrPrefilterInputPortDescriptorsConfig[0],
        CHX_ARRAY_SIZE(MfsrPrefilterOutPutPortDescriptors),
        &MfsrPrefilterOutPutPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo PrefilterPipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(PrefilterDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &PrefilterDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo PrefilterStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(PrefilterPipelineDependencyConfig),
        &PrefilterPipelineDependencyConfig[0],
    }
};


static const ChiFeature2StageDescriptor MFSRStageDescriptor[] =
{
    {
        0,
        "Prefilter",
        CHX_ARRAY_SIZE(PrefilterStageInfo),
        &PrefilterStageInfo[0],
    },

    {
        1,
        "BlendInit",
        CHX_ARRAY_SIZE(BlendInitStageInfo),
        &BlendInitStageInfo[0],
    },

    {
        2,
        "BlendLoop",
        CHX_ARRAY_SIZE(BlendLoopStageInfo),
        &BlendLoopStageInfo[0],
    },

    {
        3,
        "Postfilter",
        CHX_ARRAY_SIZE(PostFilterStageInfo),
        &PostFilterStageInfo[0],
    },
};

// Informations about all internal links
static const ChiFeature2InternalLinkDesc MFSRStageInternalLinkDescriptors[] =
{

    // ====== Blend Init connections ======
    // prefilter stage FULL Out ---> BlendInit stage FULL In
    {
        &MfsrPrefilterOutPutPortDescriptors[0],
        &BlendInitInputPortDependencyDescriptors[0],
    },

    // prefilter stage DS4 Out ---> BlendInit stage DS4 In
    {
        &MfsrPrefilterOutPutPortDescriptors[1],
        &BlendInitInputPortDependencyDescriptors[1],
    },

    // prefilter stage DS16 Out ---> BlendInit stage DS16 In
    {
        &MfsrPrefilterOutPutPortDescriptors[2],
        &BlendInitInputPortDependencyDescriptors[2],
    },

    // prefilter stage DS64 Out ---> BlendInit stage DS64 In
    {
        &MfsrPrefilterOutPutPortDescriptors[3],
        &BlendInitInputPortDependencyDescriptors[3],
    },

    // prefilter stage REG Out ---> BlendInit stage REG In
    {
        &MfsrPrefilterOutPutPortDescriptors[4],
        &BlendInitInputPortDependencyDescriptors[4],
    },

    // prefilter stage Meta Out ---> BlendInit stage Meta In
    {
        &MfsrPrefilterOutPutPortDescriptors[5],
        &BlendInitInputPortDependencyDescriptors[5],
    },

    // prefilter stage CVP DME CXT Out ---> BlendInit stage CVP DME CXT In
    {
        &MfsrPrefilterOutPutPortDescriptors[6],
        &BlendInitInputPortDependencyDescriptors[6],
    },


    // ====== Blend Loop connections ======
    // Blend Init stage FULL ---> BlendLoop stage FULL In
    {
        &BlendLoopOutPutPortDescriptors[0],
        &BlendLoopInputPortDependencyDescriptors[0],
    },

    // Prefilter stage DS4 Out ---> BlendLoop stage DS4 In
    {
        &MfsrPrefilterOutPutPortDescriptors[1],
        &BlendLoopInputPortDependencyDescriptors[1],
    },

    // Prefilter stage DS16 Out ---> BlendLoop stage DS16 In
    {
        &MfsrPrefilterOutPutPortDescriptors[2],
        &BlendLoopInputPortDependencyDescriptors[2],
    },

    // Prefilter stage DS64 Out ---> BlendLoop stage DS64 In
    {
        &MfsrPrefilterOutPutPortDescriptors[3],
        &BlendLoopInputPortDependencyDescriptors[3],
    },

    // prefilter stage REG Out ---> BlendLoop stage REG In
    {
        &MfsrPrefilterOutPutPortDescriptors[4],
        &BlendLoopInputPortDependencyDescriptors[4],
    },

    // prefilter stage CVP DME CXT Out ---> BlendLoop stage CVP DME CXT In
    {
        &MfsrPrefilterOutPutPortDescriptors[6],
        &BlendLoopInputPortDependencyDescriptors[5],
    },


    // ====== Postfilter connections ======
    // Blendloop stage Full Out ---> postfilter stage Full In
    {
        &BlendLoopOutPutPortDescriptors[0],
        &PostFilterInputPortDependencyDescriptors[0],
    },

    // Blendloop stage DS4 Out ---> postfilter stage DS4 In
    {
        &BlendLoopOutPutPortDescriptors[1],
        &PostFilterInputPortDependencyDescriptors[1],
    },
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2Descriptor MFSRFeatureDescriptor =
{
    static_cast<UINT32>(ChiFeature2Type::MFSR),
    "MFSR",
    CHX_ARRAY_SIZE(MFSRStageDescriptor),
    &MFSRStageDescriptor[0],
    CHX_ARRAY_SIZE(MFSRSessionDescriptors),
    &MFSRSessionDescriptors[0],
    CHX_ARRAY_SIZE(MFSRStageInternalLinkDescriptors),
    &MFSRStageInternalLinkDescriptors[0],
};
