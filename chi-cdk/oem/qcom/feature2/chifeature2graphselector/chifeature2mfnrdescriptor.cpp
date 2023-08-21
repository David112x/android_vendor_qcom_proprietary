////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2mfnrdescriptor.cpp
/// @brief MFNR feature descriptor definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graphselector.h"
#include "chifeature2types.h"

static const ChiFeature2TargetDescriptor MFNRPrefilterInputTargetDescriptors[] =
{
    // RAW
    {
        "TARGET_BUFFER_RAW",
    },
};

static const ChiFeature2TargetDescriptor MFNRPrefilterOutputTargetDescriptors[] =
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
        "TARGET_BUFFER_REG_NV12_OUT",
    },
};

static const ChiFeature2TargetDescriptor MFNRBlendInputTargetDescriptors[] =
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
        "TARGET_BUFFER_REG_NV12_IN",
    },
};

static const ChiFeature2TargetDescriptor MFNRBlendOutputTargetDescriptors[] =
{
    // FULL
    {
        "TARGET_BUFFER_FULLREF_UBWCTP10_OUT",
    },

    // DS4
    {
        "TARGET_BUFFER_DS4REF_PD10_OUT",
    },
};

static const ChiFeature2TargetDescriptor MFNRPostFilterInputTargetDescriptors[] =
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
        "TARGET_BUFFER_REG_NV12_IN",
    },
};

static const ChiFeature2TargetDescriptor MFNRPostFilterOutputTargetDescriptors[] =
{
    // Video
    {
        "TARGET_BUFFER_YUV",
    },
};




static const ChiFeature2PortDescriptor MFNRPostFilterInputPortDescriptors[] =
{
    // RAW
    {
        { 0, 2, 0, ChiFeature2PortDirectionType::ExternalInput },
        "RDI_In",
        &MFNRPostFilterInputTargetDescriptors[0],
    },

    // Prefilter FULL
    {
        { 0, 2, 1, ChiFeature2PortDirectionType::InternalInput },
        "UBWC_IN",
        &MFNRPostFilterInputTargetDescriptors[1],
    },

    // Prefilter DS4
    {
        { 0, 2, 2, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRPostFilterInputTargetDescriptors[2],
    },

    // Prefilter DS16
    {
        { 0, 2, 3, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRPostFilterInputTargetDescriptors[3],
    },

    // Prefilter DS64
    {
        { 0, 2, 4, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRPostFilterInputTargetDescriptors[4],
    },

    // Prefilter REG
    {
        { 0, 2, 5, ChiFeature2PortDirectionType::InternalInput },
        "YUV_In",
        &MFNRPostFilterInputTargetDescriptors[5],
    },

    {
        { 0, 2, 6, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData },
        "MFNR_Postfilter_Input_Metadata",
        NULL,
    }
};


extern const ChiFeature2PortDescriptor MFNRPostFilterOutPutPortDescriptors[] =
{
    // Video
    {
        { 0, 2, 0, ChiFeature2PortDirectionType::ExternalOutput },
        "MFNR_YUV_OUT",
        &MFNRPostFilterOutputTargetDescriptors[0],
    },

    {
        { 0, 2, 1, ChiFeature2PortDirectionType::ExternalOutput, ChiFeature2PortType::MetaData },
        "MFNR_YUV_Metadata_Out",
        NULL,
    },
};


static const ChiFeature2PortDescriptor MFNRBlendInputPortDescriptors[] =
{
    // RAW
    {
        { 0, 1, 0, ChiFeature2PortDirectionType::ExternalInput },
        "RDI_In",
        &MFNRBlendInputTargetDescriptors[0],
    },

    // Prefilter FULL
    {
        { 0, 1, 1, ChiFeature2PortDirectionType::InternalInput },
        "UBWC_IN",
        &MFNRBlendInputTargetDescriptors[1],
    },

    // Prefilter DS4
    {
        { 0, 1, 2, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRBlendInputTargetDescriptors[2],
    },

    // Prefilter DS16
    {
        { 0, 1, 3, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRBlendInputTargetDescriptors[3],
    },

    // Prefilter DS64
    {
        { 0, 1, 4, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRBlendInputTargetDescriptors[4],
    },

    // Prefilter REG
    {
        { 0, 1, 5, ChiFeature2PortDirectionType::InternalInput },
        "YUV_In",
        &MFNRBlendInputTargetDescriptors[5],
    },

    // BlendInit FULL
    {
        { 0, 1, 6, ChiFeature2PortDirectionType::InternalInput },
        "UBWC_IN",
        &MFNRBlendInputTargetDescriptors[1],
    },

    {
        { 0, 1, 7, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData },
        "MFNR_Blend_Input_Metadata",
        NULL,
    }

};

static const ChiFeature2PortDescriptor MFNRBlendOutPutPortDescriptors[] =
{
    // FULL
    {
        { 0, 1, 0, ChiFeature2PortDirectionType::InternalOutput },
        "UBWC_OUT",
        &MFNRBlendOutputTargetDescriptors[0],
    },

    // DS4
    {
        { 0, 1, 1, ChiFeature2PortDirectionType::InternalOutput },
        "PD10_OUT",
        &MFNRBlendOutputTargetDescriptors[1],
    },

    {
        { 0, 1, 2, ChiFeature2PortDirectionType::InternalOutput, ChiFeature2PortType::MetaData },
        "MFNR_blend_output_Metadata",
        NULL,
    }

};

extern const ChiFeature2PortDescriptor MFNRPrefilterInputPortDescriptors[] =
{
    // RAW
    {
        { 0, 0, 0, ChiFeature2PortDirectionType::ExternalInput },
        "RDI_In",
        &MFNRPrefilterInputTargetDescriptors[0],
    },
};

extern const ChiFeature2PortDescriptor MFNRPrefilterInputPortDescriptorsConfig[] =
{
    {
        { 0, 0, 0, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::ImageBuffer },
        "RDI_In",
        &MFNRPrefilterInputTargetDescriptors[0],
    },

    {
        { 0, 0, 1, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData },
        "MFNR_Input_Metadata",
        NULL,
    }

};

static const ChiFeature2PortDescriptor MFNRPrefilterOutPutPortDescriptors[] =
{
    // Full
    {
        { 0, 0, 0, ChiFeature2PortDirectionType::InternalOutput },
        "UBWC_OUT",
        &MFNRPrefilterOutputTargetDescriptors[0],
    },

    // DS4
    {
        { 0, 0, 1, ChiFeature2PortDirectionType::InternalOutput },
        "PD10_OUT",
        &MFNRPrefilterOutputTargetDescriptors[1],
    },

    // DS16
    {
        { 0, 0, 2, ChiFeature2PortDirectionType::InternalOutput },
        "PD10_OUT",
        &MFNRPrefilterOutputTargetDescriptors[2],
    },

    // DS64
    {
        { 0, 0, 3, ChiFeature2PortDirectionType::InternalOutput },
        "PD10_OUT",
        &MFNRPrefilterOutputTargetDescriptors[3],
    },

    // REG
    {
        { 0, 0, 4, ChiFeature2PortDirectionType::InternalOutput },
        "YUV_OUT",
        &MFNRPrefilterOutputTargetDescriptors[4],
    },

    {
        { 0, 0, 5, ChiFeature2PortDirectionType::InternalOutput, ChiFeature2PortType::MetaData },
        "MFNR_Prefilter_output_Metadata",
        NULL,
    }
};




static const ChiFeature2PipelineDescriptor MFNRPipelineDescriptors[] =
{
    {
        0,
        0,
        "MfnrPrefilter",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(MFNRPrefilterInputPortDescriptorsConfig),
        &MFNRPrefilterInputPortDescriptorsConfig[0],
        CHX_ARRAY_SIZE(MFNRPrefilterOutPutPortDescriptors),
        &MFNRPrefilterOutPutPortDescriptors[0],
    },

    {
        0,
        1,
        "MfnrBlend",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(MFNRBlendInputPortDescriptors),
        &MFNRBlendInputPortDescriptors[0],
        CHX_ARRAY_SIZE(MFNRBlendOutPutPortDescriptors),
        &MFNRBlendOutPutPortDescriptors[0],
    },

    {
        0,
        2,
        "MfnrPostFilter",
        ChiFeature2PipelineType::CHI,
        CHX_ARRAY_SIZE(MFNRPostFilterInputPortDescriptors),
        &MFNRPostFilterInputPortDescriptors[0],
        CHX_ARRAY_SIZE(MFNRPostFilterOutPutPortDescriptors),
        &MFNRPostFilterOutPutPortDescriptors[0],
    },
};

static const ChiFeature2SessionDescriptor MFNRSessionDescriptors[] =
{
    {
        0,
        "MFNR",
        CHX_ARRAY_SIZE(MFNRPipelineDescriptors),
        &MFNRPipelineDescriptors[0],
    },
};

static const UINT32 MFNRSessionID[] =
{
    0,
};




/**************** Post Filter Stage ****************/

static const ChiFeature2PortDescriptor MFNRPostFilterInputPortDependencyDescriptors[] =
{
    // Prefilter Full
    {
        { 0, 2, 1, ChiFeature2PortDirectionType::InternalInput },
        "UBWC_In",
        &MFNRPostFilterInputTargetDescriptors[1],
    },

    // Prefilter DS4
    {
        { 0, 2, 2, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRPostFilterInputTargetDescriptors[2],
    },

    // Prefilter DS16
    {
        { 0, 2, 3, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRPostFilterInputTargetDescriptors[3],
    },

    // Prefilter DS64
    {
        { 0, 2, 4, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRPostFilterInputTargetDescriptors[4],
    },

    // Prefilter REG
    {
        { 0, 2, 5, ChiFeature2PortDirectionType::InternalInput },
        "YUV_In",
        &MFNRPostFilterInputTargetDescriptors[5],
    },
};


/**************** Post Filter Stage with No Ds64 Input ****************/

static const ChiFeature2PortDescriptor MFNRPostFilterNoDS64InputPortDependencyDescriptors[] =
{
    // Prefilter Full
    {
        { 0, 2, 1, ChiFeature2PortDirectionType::InternalInput },
        "UBWC_In",
        &MFNRPostFilterInputTargetDescriptors[1],
    },

    // Prefilter DS4
    {
        { 0, 2, 2, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRPostFilterInputTargetDescriptors[2],
    },

    // Prefilter DS16
    {
        { 0, 2, 3, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRPostFilterInputTargetDescriptors[3],
    },

    // Prefilter REG
    {
        { 0, 2, 5, ChiFeature2PortDirectionType::InternalInput },
        "YUV_In",
        &MFNRPostFilterInputTargetDescriptors[5],
    },
};



static ChiFeature2InputDependency MFNRPostFilterInputPortDependency[] =
{
    {
        CHX_ARRAY_SIZE(MFNRPostFilterInputPortDependencyDescriptors),
        &MFNRPostFilterInputPortDependencyDescriptors[0],
    },
    {
        CHX_ARRAY_SIZE(MFNRPostFilterNoDS64InputPortDependencyDescriptors),
        &MFNRPostFilterNoDS64InputPortDependencyDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor MFNRPostFilterDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(MFNRPostFilterInputPortDependency),
        &MFNRPostFilterInputPortDependency[0],
        CHX_ARRAY_SIZE(MFNRPostFilterInputPortDescriptors),
        &MFNRPostFilterInputPortDescriptors[0],
        CHX_ARRAY_SIZE(MFNRPostFilterOutPutPortDescriptors),
        &MFNRPostFilterOutPutPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo MFNRPostFilterPipelineDependencyConfig[] =
{
    {
        2,
        CHX_ARRAY_SIZE(MFNRPostFilterDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &MFNRPostFilterDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo MFNRPostFilterStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(MFNRPostFilterPipelineDependencyConfig),
        &MFNRPostFilterPipelineDependencyConfig[0],
    }
};




/*********** Blend Loop Stage ***********************/

static const ChiFeature2PortDescriptor MFNRBlendLoopInputPortDescriptors[] =
{
    // Blendinit Full
    {
        { 0, 1, 6, ChiFeature2PortDirectionType::InternalInput },
        "UBWC_In",
        &MFNRBlendInputTargetDescriptors[1],
    },

    // Prefilter DS4
    {
        { 0, 1, 2, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRBlendInputTargetDescriptors[2],
    },

    // Prefilter DS16
    {
        { 0, 1, 3, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRBlendInputTargetDescriptors[3],
    },

    // Prefilter DS64
    {
        { 0, 1, 4, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRBlendInputTargetDescriptors[4],
    },

    // Prefilter REG
    {
        { 0, 1, 5, ChiFeature2PortDirectionType::InternalInput },
        "YUV_In",
        &MFNRBlendInputTargetDescriptors[5],
    },

    // RAW
    {
        { 0, 1, 0, ChiFeature2PortDirectionType::ExternalInput },
        "RDI_In",
        &MFNRBlendInputTargetDescriptors[0],
    },

    {
        { 0, 1, 7, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData },
        "MFNR_Blend_Input_Metadata",
        NULL,
    }
};

static const ChiFeature2PortDescriptor MFNRBlendLoopOutPutPortDescriptors[] =
{
    {
        { 0, 1, 0, ChiFeature2PortDirectionType::InternalOutput },
        "UBWC_OUT",
        &MFNRBlendOutputTargetDescriptors[0],
    },

    {
        { 0, 1, 1, ChiFeature2PortDirectionType::InternalOutput },
        "PD10_OUT",
        &MFNRBlendOutputTargetDescriptors[1],
    },

    {
        { 0, 1, 2, ChiFeature2PortDirectionType::InternalOutput, ChiFeature2PortType::MetaData },
        "MFNR_blend_output_Metadata",
        NULL,
    }
};

static const ChiFeature2PortDescriptor MFNRBlendLoopInputPortDependencyDescriptors[] =
{
    // BlendInit Full
    {
        { 0, 1, 6, ChiFeature2PortDirectionType::InternalInput },
        "UBWC_In",
        &MFNRBlendInputTargetDescriptors[1],
    },

    // Prefilter DS4
    {
        { 0, 1, 2, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRBlendInputTargetDescriptors[2],
    },

    // Prefilter DS16
    {
        { 0, 1, 3, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRBlendInputTargetDescriptors[3],
    },

    // Prefilter DS64
    {
        { 0, 1, 4, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRBlendInputTargetDescriptors[4],
    },

    // Prefilter REG
    {
        { 0, 1, 5, ChiFeature2PortDirectionType::InternalInput },
        "YUV_In",
        &MFNRBlendInputTargetDescriptors[5],
    },
};

static const ChiFeature2PortDescriptor MFNRBlendLoopNoDS64InputPortDependencyDescriptors[] =
{
    // BlendInit Full
    {
        { 0, 1, 6, ChiFeature2PortDirectionType::InternalInput },
        "UBWC_In",
        &MFNRBlendInputTargetDescriptors[1],
    },

    // Prefilter DS4
    {
        { 0, 1, 2, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRBlendInputTargetDescriptors[2],
    },

    // Prefilter DS16
    {
        { 0, 1, 3, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRBlendInputTargetDescriptors[3],
    },

    // Prefilter REG
    {
        { 0, 1, 5, ChiFeature2PortDirectionType::InternalInput },
        "YUV_In",
        &MFNRBlendInputTargetDescriptors[5],
    },
};

static ChiFeature2InputDependency MFNRBlendLoopInputPortDependency[] =
{
    {
        CHX_ARRAY_SIZE(MFNRBlendLoopInputPortDependencyDescriptors),
        &MFNRBlendLoopInputPortDependencyDescriptors[0],
    },
    {
        CHX_ARRAY_SIZE(MFNRBlendLoopNoDS64InputPortDependencyDescriptors),
        &MFNRBlendLoopNoDS64InputPortDependencyDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor MFNRBlendLoopDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(MFNRBlendLoopInputPortDependency),
        &MFNRBlendLoopInputPortDependency[0],
        CHX_ARRAY_SIZE(MFNRBlendLoopInputPortDescriptors),
        &MFNRBlendLoopInputPortDescriptors[0],
        CHX_ARRAY_SIZE(MFNRBlendLoopOutPutPortDescriptors),
        &MFNRBlendLoopOutPutPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo MFNRBlendLoopPipelineDependencyConfig[] =
{
    {
        1,
        CHX_ARRAY_SIZE(MFNRBlendLoopDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &MFNRBlendLoopDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo MFNRBlendLoopStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(MFNRBlendLoopPipelineDependencyConfig),
        &MFNRBlendLoopPipelineDependencyConfig[0],
    }
};





/************* Blend Init Stage *****************/

static const ChiFeature2PortDescriptor MFNRBlendInitInputPortDescriptors[] =
{
    // Perfilter Full
    {
        { 0, 1, 1, ChiFeature2PortDirectionType::InternalInput },
        "UBWC_In",
        &MFNRBlendInputTargetDescriptors[1],
    },

    // Prefilter DS4
    {
        { 0, 1, 2, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRBlendInputTargetDescriptors[2],
    },

    // Prefilter DS16
    {
        { 0, 1, 3, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRBlendInputTargetDescriptors[3],
    },

    // Prefilter DS64
    {
        { 0, 1, 4, ChiFeature2PortDirectionType::InternalInput },
        "YUV_In",
        &MFNRBlendInputTargetDescriptors[4],
    },

    // Prefilter REG
    {
        { 0, 1, 5, ChiFeature2PortDirectionType::InternalInput },
        "YUV_In",
        &MFNRBlendInputTargetDescriptors[5],
    },

    // RAW
    {
        { 0, 1, 0, ChiFeature2PortDirectionType::ExternalInput },
        "RDI_In",
        &MFNRBlendInputTargetDescriptors[0],
    },

    {
        { 0, 1, 7, ChiFeature2PortDirectionType::ExternalInput, ChiFeature2PortType::MetaData },
        "MFSR_Blend_Input_Metadata",
        NULL,
    }
};

static const ChiFeature2PortDescriptor BlendInitOutPutPortDescriptors[] =
{
    {
        { 0, 1, 0, ChiFeature2PortDirectionType::InternalOutput },
        "UBWC_OUT",
        &MFNRBlendOutputTargetDescriptors[0],
    },

    {
        { 0, 1, 1, ChiFeature2PortDirectionType::InternalOutput },
        "PD10_OUT",
        &MFNRBlendOutputTargetDescriptors[1],
    },
};

static const ChiFeature2PortDescriptor MFNRBlendInitInputPortDependencyDescriptors[] =
{
    // Prefilter Full
    {
        { 0, 1, 1, ChiFeature2PortDirectionType::InternalInput },
        "UBWC_In",
        &MFNRBlendInputTargetDescriptors[1],
    },

    // Prefilter DS4
    {
        { 0, 1, 2, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRBlendInputTargetDescriptors[2],
    },

    // Prefilter DS16
    {
        { 0, 1, 3, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRBlendInputTargetDescriptors[3],
    },

    // Prefilter DS64
    {
        { 0, 1, 4, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRBlendInputTargetDescriptors[4],
    },

    // Prefilter REG
    {
        { 0, 1, 5, ChiFeature2PortDirectionType::InternalInput },
        "YUV_In",
        &MFNRBlendInputTargetDescriptors[5],
    },
};

static const ChiFeature2PortDescriptor MFNRBlendInitNoDS64InputPortDependencyDescriptors[] =
{
    // Prefilter Full
    {
        { 0, 1, 1, ChiFeature2PortDirectionType::InternalInput },
        "UBWC_In",
        &MFNRBlendInputTargetDescriptors[1],
    },

    // Prefilter DS4
    {
        { 0, 1, 2, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRBlendInputTargetDescriptors[2],
    },

    // Prefilter DS16
    {
        { 0, 1, 3, ChiFeature2PortDirectionType::InternalInput },
        "PD10_In",
        &MFNRBlendInputTargetDescriptors[3],
    },

    // Prefilter REG
    {
        { 0, 1, 5, ChiFeature2PortDirectionType::InternalInput },
        "YUV_In",
        &MFNRBlendInputTargetDescriptors[5],
    },
};


static ChiFeature2InputDependency MFNRBlendInitInputPortDependency[] =
{
    {
        CHX_ARRAY_SIZE(MFNRBlendInitInputPortDependencyDescriptors),
        &MFNRBlendInitInputPortDependencyDescriptors[0],
    },
    {
        CHX_ARRAY_SIZE(MFNRBlendInitNoDS64InputPortDependencyDescriptors),
        &MFNRBlendInitNoDS64InputPortDependencyDescriptors[0],
    }
};

static ChiFeature2DependencyConfigDescriptor MFNRBlendInitDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(MFNRBlendInitInputPortDependency),
        &MFNRBlendInitInputPortDependency[0],
        CHX_ARRAY_SIZE(MFNRBlendInitInputPortDescriptors),
        &MFNRBlendInitInputPortDescriptors[0],
        CHX_ARRAY_SIZE(MFNRBlendLoopOutPutPortDescriptors),
        &MFNRBlendLoopOutPutPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo MFNRBlendInitPipelineDependencyConfig[] =
{
    {
        1,
        CHX_ARRAY_SIZE(MFNRBlendInitDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &MFNRBlendInitDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo MFNRBlendInitStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(MFNRBlendInitPipelineDependencyConfig),
        &MFNRBlendInitPipelineDependencyConfig[0],
    }
};




/**************** Prefilter Stage ****************/

static ChiFeature2InputDependency MFNRPrefilterInputPortDependency[] =
{
    {
        CHX_ARRAY_SIZE(MFNRPrefilterInputPortDescriptorsConfig),
        &MFNRPrefilterInputPortDescriptorsConfig[0],
    },

};

static ChiFeature2DependencyConfigDescriptor MFNRPrefilterDependencyDescriptor[] =
{
    {
        CHX_ARRAY_SIZE(MFNRPrefilterInputPortDependency),
        &MFNRPrefilterInputPortDependency[0],
        CHX_ARRAY_SIZE(MFNRPrefilterInputPortDescriptorsConfig),
        &MFNRPrefilterInputPortDescriptorsConfig[0],
        CHX_ARRAY_SIZE(MFNRPrefilterOutPutPortDescriptors),
        &MFNRPrefilterOutPutPortDescriptors[0],
    }
};

static ChiFeature2PipelineInfo MFNRPrefilterPipelineDependencyConfig[] =
{
    {
        0,
        CHX_ARRAY_SIZE(MFNRPrefilterDependencyDescriptor),
        ChiFeature2HandleType::DependencyConfigInfo,
        &MFNRPrefilterDependencyDescriptor[0],
    },
};

static const ChiFeature2SessionInfo MFNRPrefilterStageInfo[] =
{
    {
        0,
        CHX_ARRAY_SIZE(MFNRPrefilterPipelineDependencyConfig),
        &MFNRPrefilterPipelineDependencyConfig[0],
    }
};


static const ChiFeature2StageDescriptor MFNRStageDescriptor[] =
{
    {
        0,
        "Prefilter",
        CHX_ARRAY_SIZE(MFNRPrefilterStageInfo),
        &MFNRPrefilterStageInfo[0],
    },

    {
        1,
        "BlendInit",
        CHX_ARRAY_SIZE(MFNRBlendInitStageInfo),
        &MFNRBlendInitStageInfo[0],
    },

    {
        2,
        "BlendLoop",
        CHX_ARRAY_SIZE(MFNRBlendLoopStageInfo),
        &MFNRBlendLoopStageInfo[0],
    },

    {
        3,
        "Postfilter",
        CHX_ARRAY_SIZE(MFNRPostFilterStageInfo),
        &MFNRPostFilterStageInfo[0],
    },
};

// Informations about all internal links
static const ChiFeature2InternalLinkDesc MFNRStageInternalLinkDescriptors[] =
{

    // ====== Blend Init connections ======
    // prefilter stage FULL Out ---> BlendInit stage FULL In
    {
        &MFNRPrefilterOutPutPortDescriptors[0],
        &MFNRBlendInitInputPortDependencyDescriptors[0],
    },

    // prefilter stage DS4 Out ---> BlendInit stage DS4 In
    {
        &MFNRPrefilterOutPutPortDescriptors[1],
        &MFNRBlendInitInputPortDependencyDescriptors[1],
    },

    // prefilter stage DS16 Out ---> BlendInit stage DS16 In
    {
        &MFNRPrefilterOutPutPortDescriptors[2],
        &MFNRBlendInitInputPortDependencyDescriptors[2],
    },

    // prefilter stage DS64 Out ---> BlendInit stage DS64 In
    {
        &MFNRPrefilterOutPutPortDescriptors[3],
        &MFNRBlendInitInputPortDependencyDescriptors[3],
    },

    // prefilter stage REG Out ---> BlendInit stage REG In
    {
        &MFNRPrefilterOutPutPortDescriptors[4],
        &MFNRBlendInitInputPortDependencyDescriptors[4],
    },



    // ====== Blend Loop connections ======
    // Blend Init stage FULL ---> BlendLoop stage FULL In
    {
        &MFNRBlendLoopOutPutPortDescriptors[0],
        &MFNRBlendLoopInputPortDependencyDescriptors[0],
    },

    // Prefilter stage DS4 Out ---> BlendLoop stage DS4 In
    {
        &MFNRPrefilterOutPutPortDescriptors[1],
        &MFNRBlendLoopInputPortDependencyDescriptors[1],
    },

    // Prefilter stage DS16 Out ---> BlendLoop stage DS16 In
    {
        &MFNRPrefilterOutPutPortDescriptors[2],
        &MFNRBlendLoopInputPortDependencyDescriptors[2],
    },

    // Prefilter stage DS64 Out ---> BlendLoop stage DS64 In
    {
        &MFNRPrefilterOutPutPortDescriptors[3],
        &MFNRBlendLoopInputPortDependencyDescriptors[3],
    },

    // prefilter stage REG Out ---> BlendLoop stage REG In
    {
        &MFNRPrefilterOutPutPortDescriptors[4],
        &MFNRBlendLoopInputPortDependencyDescriptors[4],
    },


    // ====== Postfilter connections ======
    // Blendloop stage Full Out ---> postfilter stage Full In
    {
        &MFNRBlendLoopOutPutPortDescriptors[0],
        &MFNRPostFilterInputPortDependencyDescriptors[0],
    },

    // Prefilter stage DS4 Out ---> postfilter stage DS4 In
    {
        &MFNRPrefilterOutPutPortDescriptors[1],
        &MFNRPostFilterInputPortDependencyDescriptors[1],
    },

    // Prefilter DS16 Out ---> postfilter stage DS16 In
    {
        &MFNRPrefilterOutPutPortDescriptors[2],
        &MFNRPostFilterInputPortDependencyDescriptors[2],
    },

    // Prefilter DS64 Out ---> postfilter stage DS64 In
    {
        &MFNRPrefilterOutPutPortDescriptors[3],
        &MFNRPostFilterInputPortDependencyDescriptors[3],
    },

    // prefilter stage REG Out ---> postfilter stage REG In
    {
        &MFNRPrefilterOutPutPortDescriptors[4],
        &MFNRPostFilterInputPortDependencyDescriptors[4],
    },
};

CDK_VISIBILITY_PUBLIC extern const ChiFeature2Descriptor MFNRFeatureDescriptor =
{
    static_cast<UINT32>(ChiFeature2Type::MFSR),
    "MFSR",
    CHX_ARRAY_SIZE(MFNRStageDescriptor),
    &MFNRStageDescriptor[0],
    CHX_ARRAY_SIZE(MFNRSessionDescriptors),
    &MFNRSessionDescriptors[0],
    CHX_ARRAY_SIZE(MFNRStageInternalLinkDescriptors),
    &MFNRStageInternalLinkDescriptors[0],
};
