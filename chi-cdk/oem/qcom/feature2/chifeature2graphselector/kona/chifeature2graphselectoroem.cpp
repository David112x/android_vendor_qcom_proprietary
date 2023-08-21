////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2graphselectoroem.cpp
/// @brief CHI feature2 derived graph selector implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graphselector.h"
#include "chifeature2graphselectoroem.h"

// NOWHINE FILE NC009:  CHI files will start with CHI
// NOWHINE FILE CP006:  Need whiner update: std::vector allowed in exceptional cases
// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files, required for table

extern std::set<UINT> CaptureIntentAll;
extern std::set<UINT> SceneModeAll;
extern std::set<UINT> noiseReductionmodeAll;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelectorOEM::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2GraphSelectorOEM* ChiFeature2GraphSelectorOEM::Create(
    FeatureGraphManagerConfig* pConfig,
    std::set<const CHAR*>&     rFeatureDescNameSet)
{
    ChiFeature2GraphSelectorOEM* pFeatureGraphSelector = CHX_NEW(ChiFeature2GraphSelectorOEM);

    pFeatureGraphSelector->PopulateAllTablesOEM();

    rFeatureDescNameSet = pFeatureGraphSelector->m_featureDescNameSet;

    pFeatureGraphSelector->Initialize(pConfig);

    return pFeatureGraphSelector;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::GetFeatureGraphMapforConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
keysToCloneDescMap ChiFeature2GraphSelectorOEM::GetFeatureGraphMapforConfig(
    FeatureGraphManagerConfig*  pConfig,
    FeatureGraphSelectorConfig& rSelectorOutput,
    GraphDescriptorTables*      pGraphDescriptorTables)
{
    CDK_UNUSED_PARAM(pGraphDescriptorTables);

    GraphDescriptorTables* pGraphDescriptorTablesOEM = GetGraphDescriptorTables();

    return ChiFeature2GraphSelector::GetFeatureGraphMapforConfig(pConfig,
                                                                 rSelectorOutput,
                                                                 pGraphDescriptorTablesOEM);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelectorOEM::PopulateAllTablesOEM
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphSelectorOEM::PopulateAllTablesOEM()
{
    BuildCameraIdSet();

    GraphDescriptorTables* pGraphDescriptorTables = GetGraphDescriptorTables();

    if (NULL != pGraphDescriptorTables)
    {
        /// @brief set of cameraIds and its corresponding list of Feature graph Descriptors
        *pGraphDescriptorTables->pCameraIdDescriptorNameSet =
        {
            // cameraId set    // Feature Graph Descriptor Name set
            // Single camera
            { SINGLE_CAMERA, { RTMFSRJPEGFeatureGraphDescriptor.pFeatureGraphName,
                               RTRawHDRBayer2YUVJPEGFeatureGraphDescriptor.pFeatureGraphName,
                               RTMFSRYUVFeatureGraphDescriptor.pFeatureGraphName,
                               RTMFSRHDRT1JPEGFeatureGraphDescriptor.pFeatureGraphName } },

           // Multi camera
           { BOKEH_CAMERA,  { MultiCameraBokehFeatureSuperGraphDescriptor.pFeatureGraphName } },
           { FUSION_CAMERA, { MultiCameraFusionFeatureSuperGraphDescriptor.pFeatureGraphName} },
           { BOKEH_CAMERA,  { MultiCameraHDRFeatureGraphDescriptor.pFeatureGraphName } },
        };

        /// @brief mapping feature graph descriptor name to feature graph descriptor
        *pGraphDescriptorTables->pFeatureGraphDescriptorsMap =
        {
            { RTMFSRHDRT1JPEGFeatureGraphDescriptor.pFeatureGraphName,        RTMFSRHDRT1JPEGFeatureGraphDescriptor },
            { RTMFSRJPEGFeatureGraphDescriptor.pFeatureGraphName,             RTMFSRJPEGFeatureGraphDescriptor },
            { MFSRFeatureGraphDescriptor.pFeatureGraphName,                   MFSRFeatureGraphDescriptor },
            { RTRawHDRBayer2YUVJPEGFeatureGraphDescriptor.pFeatureGraphName,  RTRawHDRBayer2YUVJPEGFeatureGraphDescriptor },
            { MultiCameraBokehFeatureSuperGraphDescriptor.pFeatureGraphName,  MultiCameraBokehFeatureSuperGraphDescriptor },
            { MultiCameraFusionFeatureSuperGraphDescriptor.pFeatureGraphName, MultiCameraFusionFeatureSuperGraphDescriptor },
            { RTMFSRYUVFeatureGraphDescriptor.pFeatureGraphName,              RTMFSRYUVFeatureGraphDescriptor },
            { MultiCameraHDRFeatureGraphDescriptor.pFeatureGraphName,         MultiCameraHDRFeatureGraphDescriptor },
        };

        /// @brief Keys to Feature Graph Descriptor map table
        *pGraphDescriptorTables->pFeatureGraphDescKeysMap =
        {
            /*******************************************************************************************************************************/
            /*************************************************Realtime+MFSR+JPEG FeatureGraph***********************************************/
            /*******************************************************************************************************************************/
            // FGD name    // CameraId          // CaptureIntent                         // SceneMode                                // NoiseReduction                       // vendortag  // OpsMode
            { RTMFSRJPEGFeatureGraphDescriptor.pFeatureGraphName,
                SINGLE_CAMERA, {  ControlCaptureIntentStillCapture,
                                  ControlCaptureIntentVideoSnapshot,
                                  ControlCaptureIntentZeroShutterLag,
                                  ControlCaptureIntentManual },         { ControlSceneModeFacePriority,
                                                                          ControlSceneModeAction,
                                                                          ControlSceneModePortrait,
                                                                          ControlSceneModeLandscape,
                                                                          ControlSceneModeNight,
                                                                          ControlSceneModeNightPortrait,
                                                                          ControlSceneModeTheatre,
                                                                          ControlSceneModeBeach,
                                                                          ControlSceneModeSnow,
                                                                          ControlSceneModeSunset,
                                                                          ControlSceneModeSteadyphoto,
                                                                          ControlSceneModeFireworks,
                                                                          ControlSceneModeSports,
                                                                          ControlSceneModeParty,
                                                                          ControlSceneModeCandlelight,
                                                                          ControlSceneModeBarcode },      { NoiseReductionModeHighQuality },  { CustomVendorTagMFNR },            0

            },

            /*******************************************************************************************************************************/
            /*************************************************Realtime+HDR+B2Y+JPEG FeatureGraph********************************************/
            /*******************************************************************************************************************************/
            // FGD name    // CameraId          // CaptureIntent                 // SceneMode                                // NoiseReduction                       // vendortag  // OpsMode
            { RTRawHDRBayer2YUVJPEGFeatureGraphDescriptor.pFeatureGraphName,
                 SINGLE_CAMERA, { ControlCaptureIntentStillCapture,
                                  ControlCaptureIntentVideoSnapshot,
                                  ControlCaptureIntentZeroShutterLag,
                                  ControlCaptureIntentManual },         { ControlSceneModeHDR }, { NoiseReductionModeOff,
                                                                                                   NoiseReductionModeFast,
                                                                                                   NoiseReductionModeMinimal,
                                                                                                   NoiseReductionModeZeroShutterLag, }, { CustomVendorTagRawReprocessing }, 0
            },


            /*******************************************************************************************************************************/
            /*************************************************Realtime+MFSR+HDR+JPEG FeatureGraph********************************************/
            /*******************************************************************************************************************************/
            // FGD name    // CameraId          // CaptureIntent                 // SceneMode                                // NoiseReduction                       // vendortag  // OpsMode
            { RTMFSRHDRT1JPEGFeatureGraphDescriptor.pFeatureGraphName,
                 SINGLE_CAMERA, { ControlCaptureIntentStillCapture,
                                  ControlCaptureIntentVideoSnapshot,
                                  ControlCaptureIntentZeroShutterLag,
                                  ControlCaptureIntentManual },             { ControlSceneModeHDR }, { NoiseReductionModeHighQuality }, { CustomVendorTagRawReprocessing }, 0
            },

            /*******************************************************************************************************************************/
            /*************************************************AnchorSync+MFSR+Fusion FeatureGraph*******************************************/
            /*******************************************************************************************************************************/
            // FGD name    // CameraId          // CaptureIntent                         // SceneMode                                // NoiseReduction                       // vendortag  // OpsMode
            { MultiCameraFusionFeatureSuperGraphDescriptor.pFeatureGraphName,
                FUSION_CAMERA,      { ControlCaptureIntentStillCapture,
                                     ControlCaptureIntentVideoSnapshot,
                                     ControlCaptureIntentZeroShutterLag,
                                     ControlCaptureIntentManual },         { ControlSceneModeFacePriority,
                                                                             ControlSceneModeAction,
                                                                             ControlSceneModePortrait,
                                                                             ControlSceneModeLandscape,
                                                                             ControlSceneModeNight,
                                                                             ControlSceneModeNightPortrait,
                                                                             ControlSceneModeTheatre,
                                                                             ControlSceneModeBeach,
                                                                             ControlSceneModeSnow,
                                                                             ControlSceneModeSunset,
                                                                             ControlSceneModeSteadyphoto,
                                                                             ControlSceneModeFireworks,
                                                                             ControlSceneModeSports,
                                                                             ControlSceneModeParty,
                                                                             ControlSceneModeCandlelight,
                                                                             ControlSceneModeBarcode },     { noiseReductionmodeAll },  { 0 },   0
            },

            /*******************************************************************************************************************************/
            /*************************************************AnchorSync+MFSR+B2Y+Bokeh FeatureGraph****************************************/
            /*******************************************************************************************************************************/
            // FGD name    // CameraId          // CaptureIntent                         // SceneMode                                // NoiseReduction                       // vendortag  // OpsMode
            { MultiCameraBokehFeatureSuperGraphDescriptor.pFeatureGraphName,
                BOKEH_CAMERA,      { ControlCaptureIntentStillCapture,
                                     ControlCaptureIntentVideoSnapshot,
                                     ControlCaptureIntentZeroShutterLag,
                                     ControlCaptureIntentManual },         { ControlSceneModeFacePriority,
                                                                             ControlSceneModeAction,
                                                                             ControlSceneModePortrait,
                                                                             ControlSceneModeLandscape,
                                                                             ControlSceneModeNight,
                                                                             ControlSceneModeNightPortrait,
                                                                             ControlSceneModeTheatre,
                                                                             ControlSceneModeBeach,
                                                                             ControlSceneModeSnow,
                                                                             ControlSceneModeSunset,
                                                                             ControlSceneModeSteadyphoto,
                                                                             ControlSceneModeFireworks,
                                                                             ControlSceneModeSports,
                                                                             ControlSceneModeParty,
                                                                             ControlSceneModeCandlelight,
                                                                             ControlSceneModeBarcode },     { noiseReductionmodeAll },  { 0 },   0
            },

            /*******************************************************************************************************************************/
            /*************************************************AnchorSync+HDR+B2Y+Bokeh FeatureGraph****************************************/
            /*******************************************************************************************************************************/
            // FGD name    // CameraId          // CaptureIntent                         // SceneMode                                // NoiseReduction                       // vendortag  // OpsMode
            { MultiCameraBokehFeatureSuperGraphDescriptor.pFeatureGraphName,
                BOKEH_CAMERA,      { ControlCaptureIntentStillCapture,
                                     ControlCaptureIntentVideoSnapshot,
                                     ControlCaptureIntentZeroShutterLag,
                                     ControlCaptureIntentManual },         { ControlSceneModeHDR},     { NoiseReductionModeFast, NoiseReductionModeOff }, { CustomVendorTagRawReprocessing }, 0
            },
            /*******************************************************************************************************************************/
            /*************************************************AnchorSync+HDR+B2Y+Bokeh FeatureGraph****************************************/
            /*******************************************************************************************************************************/
            // FGD name    // CameraId          // CaptureIntent                         // SceneMode                                // NoiseReduction                       // vendortag  // OpsMode
            { MultiCameraFusionFeatureSuperGraphDescriptor.pFeatureGraphName,
                FUSION_CAMERA,      { ControlCaptureIntentStillCapture,
                                     ControlCaptureIntentVideoSnapshot,
                                     ControlCaptureIntentZeroShutterLag,
                                     ControlCaptureIntentManual },         { ControlSceneModeHDR},     { NoiseReductionModeFast, NoiseReductionModeOff }, { CustomVendorTagRawReprocessing }, 0
            },

        };
    }

    m_featureDescNameSet =
    {
        HDRT1FeatureDescriptor.pFeatureName,
        Bayer2YuvFeatureDescriptor.pFeatureName,
        RealTimeFeatureDescriptor.pFeatureName,
        RealTimeFeatureWithSWRemosaicDescriptor.pFeatureName,
        // StubFeatureDescriptor.pFeatureName,
        JPEGFeatureDescriptor.pFeatureName,
        MFSRFeatureDescriptor.pFeatureName,
        MFNRFeatureDescriptor.pFeatureName,
        AnchorSyncFeatureDescriptor.pFeatureName,
        DemuxFeatureDescriptor.pFeatureName,
        SWMFFeatureDescriptor.pFeatureName,
        RawHDRFeatureDescriptor.pFeatureName,
        BokehFeatureDescriptor.pFeatureName,
        FusionFeatureDescriptor.pFeatureName,
        FrameSelectFeatureDescriptor.pFeatureName,
        MultiCameraHDRFeatureGraphDescriptor.pFeatureGraphName,
        MemcpyFeatureDescriptor.pFeatureName,
        JPEGFeatureDescriptorGPU.pFeatureName,
        SerializerFeatureDescriptor.pFeatureName,
        FormatConvertorFeatureDescriptor.pFeatureName
    };

    Dump(GetGraphDescriptorTables());

    PopulateAllTables();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelectorOEM::SelectFeatureGraphforRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2GraphDesc* ChiFeature2GraphSelectorOEM::SelectFeatureGraphforRequest(
    ChiFeature2UsecaseRequestObject* pChiUsecaseRequestObject,
    std::map<UINT32, ChiMetadata*>   pMetadataFrameNumberMap)
{
    GraphDescriptorTables* pGraphDescriptorTables = GetGraphDescriptorTables();
    return ChiFeature2GraphSelector::SelectFeatureGraphforRequestFromTable(pChiUsecaseRequestObject,
                                                                           pMetadataFrameNumberMap,
                                                                           pGraphDescriptorTables);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelectorOEM::~ChiFeature2GraphSelectorOEM
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2GraphSelectorOEM::~ChiFeature2GraphSelectorOEM()
{

}
