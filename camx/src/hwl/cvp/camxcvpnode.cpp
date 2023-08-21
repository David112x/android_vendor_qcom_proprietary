////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcvpnode.cpp
/// @brief CVP Node class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxnode.h"
#include "camxcvpnode.h"
#include "camxcdmdefs.h"
#include "camxcslresourcedefs.h"
#include "camximagebuffer.h"
#include "camximageformatutils.h"
#include "camxtrace.h"
#include "camxformats.h"
#include "camxpacketbuilder.h"
#include "camxvendortags.h"
#include "TransformEstimation.h"
#include "cvpSession.h"
#include "cvpUtils.h"
#include "Process_CVP.h"
#include "camxipeica20.h"
#include "camxiqinterface.h"
#include "camxtranslator.h"
#include "camxipeicatestdata.h"
#include "camxispiqmodule.h"
#include "camxtuningdatamanager.h"
#include "GeoLibUtils.h"

CAMX_NAMESPACE_BEGIN

CAMX_STATIC_ASSERT(sizeof(cvpDmeFrameConfigIca) ==
    sizeof(NcLibcvpDmeFrameConfigIca));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nGridDataSize) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nGridDataSize));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nGridData) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nGridData));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nPerspectiveDataSize) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nPerspectiveDataSize));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nPerspectiveData) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nPerspectiveData));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nCTCPerspTransformGeomM) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nCTCPerspTransformGeomM));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nCTCPerspTransformGeomN) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nCTCPerspTransformGeomN));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nOpgInterpLUT0) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nOpgInterpLUT0));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nOpgInterpLUT1) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nOpgInterpLUT1));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nOpgInterpLUT2) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nOpgInterpLUT2));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nCTCHalfOutputFrameWidth) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nCTCHalfOutputFrameWidth));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nCTCHalfOutputFrameHeight) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nCTCHalfOutputFrameHeight));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nCtcInputCoordPrecision) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nCtcInputCoordPrecision));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nCtcO2vScaleFactor_x) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nCtcO2vScaleFactor_x));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nCtcO2vScaleFactor_y) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nCtcO2vScaleFactor_y));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nCtcV2iInvScaleFactor_x) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nCtcV2iInvScaleFactor_x));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nCtcV2iInvScaleFactor_y) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nCtcV2iInvScaleFactor_y));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nControllerValidWidthMinus1) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nControllerValidWidthMinus1));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nControllerValidHeightMinus1) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nControllerValidHeightMinus1));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nCtcO2vOffset_x) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nCtcO2vOffset_x));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nCtcO2vOffset_y) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nCtcO2vOffset_y));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nCtcV2iOffset_x) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nCtcV2iOffset_x));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nCtcV2iOffset_y) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nCtcV2iOffset_y));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nCtcXTranslation) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nCtcXTranslation));
CAMX_STATIC_ASSERT(offsetof(cvpDmeFrameConfigIca, nCtcYTranslation) ==
    offsetof(NcLibcvpDmeFrameConfigIca, nCtcYTranslation));

static const UINT CVPMaxInPorts     = 3;
static const UINT CVPMaxOutPorts    = 3;

/// @brief CVP node states
enum cvpNodeStates
{
    CVP_NODE_CREATED,           ///< Initial state to begin with
    CVP_NODE_INITIALIZING,      ///< Move to this state after PRocessingNodeInitialize is called
    CVP_NODE_INITIALIZED,       ///< Move to this state after Finalize
    CVP_NODE_ACTIVE,            ///< Move to this state after device is acquired
};

// @brief list of vendor tags published by CVP
static const struct NodeVendorTag g_CVPOutputVendorTags[] =
{
    { "org.quic.camera2.ipeicaconfigs", "ICAReferenceParams" },
    { "org.quic.camera2.ipeicaconfigs", "ICAReferenceParamsLookAhead" },
    { "org.quic.camera2.ipeicaconfigs", "ICAInPerspectiveTransform" },
};

// @brief, index corresponding to above vendor tags
static const UINT32 ICAReferenceParamsIndex          = 0;
static const UINT32 ICAReferenceParamsLookAheadIndex = 1;
static const UINT32 ICAInPerspectiveTransformIndex   = 2;

// This list will follow order of modules in real hardware
static IPEIQModuleInfo IQModulesList[] =
{
    { ISPIQModuleType::IPEICA,               IPEPath::CVPICA,     TRUE,    IPEICA20::Create },
};

static const UINT32 CVPMinimumWidthPixels = 480;
static const UINT32 CVPMinimumHeightLines = 270;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::CVPNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CVPNode::CVPNode()
{
    m_pNodeName                     = "CVP";
    m_numInputPorts                 = CVPMaxInPorts;
    m_numOutputPorts                = CVPMaxOutPorts;
    m_hDevice                       = -1;
    m_pSourceContextBufferManager   = NULL;
    m_pGridBufferManager            = NULL;
    m_hCVPSessionHandle             = NULL;
    m_hCVPDME_Handle                = NULL;
    m_disableCVPDriver              = GetStaticSettings()->disableCVPDriver;
    m_unityTransformEnabled         = GetStaticSettings()->OverrideCVPTransformToUnity;
    m_bICAEnabled                   = FALSE;
    m_camIdChanged                  = TRUE;
    m_fullInputWidth                = 0;
    m_fullInputHeight               = 0;
    m_inputWidth                    = 0;
    m_inputHeight                   = 0;
    m_previousRequestIdIndex        = 0;
    m_dumpCVPICAInputConfig         = FALSE;
    m_dumpGeolibResult              = FALSE;

    m_stabilizationMargin.widthPixels = 0;
    m_stabilizationMargin.heightLines = 0;

    m_ppGridBuffers             = NULL;
    m_ppSourceContextBuffers    = NULL;
    m_phSynxSrcImage            = NULL;
    m_phSynxDmeOutput           = NULL;
    m_phSynxIcaOutput           = NULL;
    m_pStillFrameConfig         = NULL;
    m_pForceIdentityTransform   = NULL;
    m_requestQueueDepth         = DefaultRequestQueueDepth;

    m_scaleFactorW  = 1.0f;
    m_scaleFactorH  = 1.0f;
    m_coarseCenterW = 0.0f;
    m_coarseCenterH = 0.0f;
    m_fTexture      = FALSE;

    CAMX_LOG_INFO(CamxLogGroupCVP, "CVP node driver disable: %u", m_disableCVPDriver);

    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera2.ipeicaconfigs",
                                                                              "ICAInPerspectiveTransform",
                                                                              &m_CVPICATAGLocation[0]));

    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera2.ipeicaconfigs",
                                                                              "ICAInGridOut2InTransform",
                                                                              &m_CVPICATAGLocation[1]));

    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera.geoLibStillFrameConfig",
                                                                              "GeoLibStillFrameConfigPrefilter",
                                                                              &m_stillFrameConfigurationTagIdPrefilter));

    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera.geoLibStillFrameConfig",
                                                                              "GeoLibStillFrameConfigBlending",
                                                                              &m_stillFrameConfigurationTagIdBlending));
    if (CamxResultSuccess != AllocateCommonLibraryData())
    {
        CAMX_LOG_ERROR(CamxLogGroupCVP, "Unable to initilize common library data, no memory");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::~CVPNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CVPNode::~CVPNode()
{
    Cleanup();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CVPNode* CVPNode::Create(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);
    CVPNode* pNodeObj = NULL;

    if ((NULL != pCreateInputData) && (NULL != pCreateInputData->pNodeInfo))
    {
        pNodeObj = CAMX_NEW CVPNode;
        if (NULL != pNodeObj)
        {
            INT32            stabType           = 0;
            UINT32           propertyCount      = pCreateInputData->pNodeInfo->nodePropertyCount;
            PerNodeProperty* pNodeProperties    = pCreateInputData->pNodeInfo->pNodeProperties;
            for (UINT32 count = 0; count < propertyCount; count++)
            {
                UINT32 nodePropertyId        = pNodeProperties[count].id;
                VOID*  pNodePropertyValue    = pNodeProperties[count].pValue;

                switch (nodePropertyId)
                {
                    case NodePropertyProfileId:
                        pNodeObj->m_instanceProperty.profileId = static_cast<CVPProfileId>(
                            atoi(static_cast<const CHAR*>(pNodePropertyValue)));
                        break;
                    case NodePropertyStabilizationType:
                        // If EIS is enabled, IPE instance needs to know if its in EIS2.0 path 3.0.
                        // Node property value is shifted to use multiple stabilization type together.
                        stabType |= (1 << (atoi(static_cast<const CHAR*>(pNodePropertyValue))));

                        // Check if EIS ICA dependency need to be bypassed
                        if ((TRUE == HwEnvironment::GetInstance()->GetStaticSettings()->bypassIPEICADependency) &&
                            ((IPEStabilizationTypeEIS2 & stabType) ||
                            (IPEStabilizationTypeEIS3 & stabType)))
                        {
                            stabType &= ~(IPEStabilizationTypeEIS2 | IPEStabilizationTypeEIS3);
                            CAMX_LOG_INFO(CamxLogGroupCVP, "EIS stabalization disabled stabType %d", stabType);
                        }

                        pNodeObj->m_instanceProperty.stabilizationType = static_cast<IPEStabilizationType>(stabType);
                        break;
                    case NodePropertyProcessingType:
                        pNodeObj->m_instanceProperty.processingType = static_cast<CVPProcessingType>(
                            atoi(static_cast<const CHAR*>(pNodePropertyValue)));
                        break;
                    default:
                        CAMX_LOG_ERROR(CamxLogGroupCVP, "Unhandled node property Id %d", nodePropertyId);
                        break;
                }
            }
            CAMX_LOG_INFO(CamxLogGroupCVP, "CVP Instance profileId: %d, processingType: %d, stab type %d",
                          pNodeObj->m_instanceProperty.profileId,
                          pNodeObj->m_instanceProperty.processingType,
                          pNodeObj->m_instanceProperty.stabilizationType);

            if (IPEStabilizationTypeEIS3 ==
                (IPEStabilizationTypeEIS3 & pNodeObj->m_instanceProperty.stabilizationType))
            {
                pCreateOutputData->createFlags.hasDelayedNotification = TRUE;
            }

            synx_result_t synxResult = synx_initialize(NULL);
            if (CamxResultSuccess != synxResult)
            {
                CAMX_LOG_ERROR(CamxLogGroupCVP, "synx_initialize fail");
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCVP, "synx_initialize success");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCVP, "Create CVP node failed!!");
        }
    }

    return pNodeObj;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::Cleanup
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CVPNode::Cleanup()
{
    synx_result_t           synxResult;
    UINT count            = 0;

    CAMX_LOG_VERBOSE(CamxLogGroupCVP, "%s, CVP Cleanup invoked", NodeIdentifierString());

    CleanupSession();

    synxResult = synx_uninitialize();
    if (CamxResultSuccess != synxResult)
    {
        CAMX_LOG_ERROR(CamxLogGroupCVP, "synx_uninitialize fail");
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "synx_uninitialize success");
    }

    if ((NULL != m_pSourceContextBufferManager) && (NULL != m_ppSourceContextBuffers))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "Deleting source context buffers");
        DeleteBufferManager(m_pSourceContextBufferManager, m_ppSourceContextBuffers);

        CAMX_FREE(m_ppSourceContextBuffers);
        m_ppSourceContextBuffers = NULL;
    }

    if ((NULL != m_pGridBufferManager) && (NULL != m_ppGridBuffers))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "Deleting grid buffers");
        DeleteBufferManager(m_pGridBufferManager, m_ppGridBuffers);

        CAMX_FREE(m_ppGridBuffers);
        m_ppGridBuffers = NULL;
    }

    if (NULL != m_phSynxSrcImage)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "Deleting source image synx handler");
        CAMX_FREE(m_phSynxSrcImage);
        m_phSynxSrcImage = NULL;
    }

    if (NULL != m_phSynxDmeOutput)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "Deleting DME output synx handler");
        CAMX_FREE(m_phSynxDmeOutput);
        m_phSynxDmeOutput = NULL;
    }

    if (NULL != m_phSynxIcaOutput)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "Deleting ICA outputsynx handler");
        CAMX_FREE(m_phSynxIcaOutput);
        m_phSynxIcaOutput = NULL;
    }

    if (NULL != m_pStillFrameConfig)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "Deleting Geolib still frame config");
        CAMX_FREE(m_pStillFrameConfig);
        m_pStillFrameConfig = NULL;
    }

    if (NULL != m_pForceIdentityTransform)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "Deleting  force identity transform pointer");
        CAMX_FREE(m_pForceIdentityTransform);
        m_pForceIdentityTransform = NULL;
    }

    // De-allocate all of the IQ modules
    for (count = 0; count < m_numCVPIQModulesEnabled; count++)
    {
        if (NULL != m_pEnabledCVPIQModule[count])
        {
            m_pEnabledCVPIQModule[count]->Destroy();
            m_pEnabledCVPIQModule[count] = NULL;
        }
    }

    DeallocateCommonLibraryData();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::ConfigureCVPCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::ConfigureCVPCapability()
{
    CamxResult result = CamxResultSuccess;

    m_capability.minInputWidth           = 36;
    m_capability.minInputHeight          = 24;
    m_capability.maxInputWidth           = 1920;
    m_capability.maxInputHeight          = 1440;
    m_capability.optimalInputWidth       = 1920;
    m_capability.optimalInputHeight      = 1440;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::ProcessingNodeInitialize(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateInputData);
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    CamxResult  result           = CamxResultSuccess;
    UINT32      groupID          = 1;
    UINT        numOutputPorts   = 0;
    UINT        outputPortId[MaxBufferComposite];

    pCreateOutputData->maxOutputPorts = CVPMaxOutPorts;
    pCreateOutputData->maxInputPorts  = CVPMaxInPorts;

    // Configure CVP Capability
    result = ConfigureCVPCapability();

    GetAllOutputPortIds(&numOutputPorts, &outputPortId[0]);

    for (UINT outputPortIndex = 0; outputPortIndex < numOutputPorts; outputPortIndex++)
    {
        pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] = groupID++;
    }

    pCreateOutputData->bufferComposite.hasCompositeMask = FALSE;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVPNode::CheckInputRequirement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::CheckInputRequirement(
    BufferRequirement inputPortRequirement,
    BufferRequirement maxSupportedResolution)
{
    CamxResult result = CamxResultSuccess;

    if ((inputPortRequirement.minWidth > inputPortRequirement.optimalWidth) ||
        (inputPortRequirement.minWidth > inputPortRequirement.maxWidth) ||
        (inputPortRequirement.optimalWidth > inputPortRequirement.maxWidth))
    {
        result = CamxResultEInvalidArg;
    }
    else if ((inputPortRequirement.minHeight > inputPortRequirement.optimalHeight) ||
             (inputPortRequirement.minHeight > inputPortRequirement.maxHeight) ||
             (inputPortRequirement.optimalHeight > inputPortRequirement.maxHeight))
    {
        result = CamxResultEInvalidArg;
    }
    else if ((inputPortRequirement.maxWidth < maxSupportedResolution.minWidth) ||
             (inputPortRequirement.maxHeight < maxSupportedResolution.minHeight) ||
             (inputPortRequirement.minWidth > maxSupportedResolution.maxWidth) ||
             (inputPortRequirement.minHeight > maxSupportedResolution.maxHeight))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCVP, "Invalid Input requirements");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVPNode::SetupVectorOutputRequirementOptions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::SetupVectorOutputRequirementOptions(
    OutputPortNegotiationData* pOutputPortNegotiationData)
{
    CamxResult result = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pOutputPortNegotiationData);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVPNode::SetupVectorOutputFinalResolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CVPNode::SetupVectorOutputFinalResolution(
    BufferProperties* pFinalOutputBufferProperties,
    UINT width,
    UINT height)
{
    CAMX_UNREFERENCED_PARAM(pFinalOutputBufferProperties);
    CAMX_UNREFERENCED_PARAM(width);
    CAMX_UNREFERENCED_PARAM(height);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVPNode::ProcessingNodeFinalizeInputRequirement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::ProcessingNodeFinalizeInputRequirement(
    BufferNegotiationData* pBufferNegotiationData)
{
    CamxResult result       = CamxResultSuccess;
    UINT       numInputPort = 0;
    UINT       inputPortId[CVPMaxInputPorts];

    GetAllInputPortIds(&numInputPort, &inputPortId[0]);

    if (CamxResultSuccess == result)
    {
        // Loop through the input ports and specify the requirement based on port type/ID
        for (UINT inputIndex = 0; inputIndex < numInputPort; inputIndex++)
        {
            pBufferNegotiationData->inputBufferOptions[inputIndex].nodeId     = Type();
            pBufferNegotiationData->inputBufferOptions[inputIndex].instanceId = InstanceID();
            pBufferNegotiationData->inputBufferOptions[inputIndex].portId     = inputPortId[inputIndex];

            BufferRequirement* pInputBufferRequirement =
                &pBufferNegotiationData->inputBufferOptions[inputIndex].bufferRequirement;

            if (CVPInputPortTARIFEFull == inputPortId[inputIndex])
            {

                pInputBufferRequirement->maxWidth       = 0xFFFF;
                pInputBufferRequirement->maxHeight      = 0xFFFF;
                pInputBufferRequirement->minWidth       = 0;
                pInputBufferRequirement->minHeight      = 0;
                pInputBufferRequirement->optimalWidth   = 0;
                pInputBufferRequirement->optimalHeight  = 0;
            }
            else
            {
                pInputBufferRequirement->maxWidth       = 0xFFFF;
                pInputBufferRequirement->maxHeight      = 0xFFFF;
                pInputBufferRequirement->minWidth       = m_capability.minInputWidth;
                pInputBufferRequirement->minHeight      = m_capability.minInputHeight;
                pInputBufferRequirement->optimalWidth   = m_capability.minInputWidth;
                pInputBufferRequirement->optimalHeight  = m_capability.minInputHeight;
            }

            switch (inputPortId[inputIndex])
            {
                case CVPInputPortTARIFEDS4:
                    //  CVPInputPortTARIFEDS4 has linear NV12 as input format for MFNR / MFSR registration case,
                    //  and the alignments are 128 / 32 for Y plane and 128 / 16 for UV plane.
                    //  Otherwise, it is format PD10 and alignments are 8 / 2.
                    if (TRUE == IsProfileIdRegistration())
                    {
                        pInputBufferRequirement->planeAlignment[0].strideAlignment   = CVPStrideAlignmentNV12PlaneY;
                        pInputBufferRequirement->planeAlignment[1].strideAlignment   = CVPStrideAlignmentNV12PlaneUV;
                        pInputBufferRequirement->planeAlignment[0].scanlineAlignment = CVPHeightAlignmentNV12PlaneY;
                        pInputBufferRequirement->planeAlignment[1].scanlineAlignment = CVPHeightAlignmentNV12PlaneUV;

                        CAMX_LOG_INFO(CamxLogGroupCVP, "%s, CVPInputPortTARIFEDS4 alignment=%u, %u, %u, %u",
                                      NodeIdentifierString(),
                                      pInputBufferRequirement->planeAlignment[0].strideAlignment,
                                      pInputBufferRequirement->planeAlignment[1].strideAlignment,
                                      pInputBufferRequirement->planeAlignment[0].scanlineAlignment,
                                      pInputBufferRequirement->planeAlignment[1].scanlineAlignment);
                    }
                    break;
                case CVPInputPortREFIFEDS4:
                    //  CVPInputPortREFIFEDS4 has UBWCNV12-4R as input format for MFNR / MFSR registration case,
                    //  and the alignments are 256 / 16 for both Y and UV plane.
                    //  Otherwise, it is format PD10 and alignments are 8 / 2.
                    if (TRUE == IsProfileIdRegistration())
                    {
                        pInputBufferRequirement->planeAlignment[0].strideAlignment   = CVPStrideAlignmentUBWCNV124RPlaneY;
                        pInputBufferRequirement->planeAlignment[1].strideAlignment   = CVPStrideAlignmentUBWCNV124RPlaneUV;
                        pInputBufferRequirement->planeAlignment[0].scanlineAlignment = CVPHeightAlignmentUBWCNV124RPlaneY;
                        pInputBufferRequirement->planeAlignment[1].scanlineAlignment = CVPHeightAlignmentUBWCNV124RPlaneUV;

                        CAMX_LOG_INFO(CamxLogGroupCVP, "%s, CVPInputPortREFIFEDS4 alignment=%u, %u, %u, %u",
                                      NodeIdentifierString(),
                                      pInputBufferRequirement->planeAlignment[0].strideAlignment,
                                      pInputBufferRequirement->planeAlignment[1].strideAlignment,
                                      pInputBufferRequirement->planeAlignment[0].scanlineAlignment,
                                      pInputBufferRequirement->planeAlignment[1].scanlineAlignment);
                    }
                    break;
                default:
                    break;
            }

            CAMX_LOG_INFO(CamxLogGroupCVP,
                "Backward buffer Negotiation port(%d) input requirement min(%d * %d) optimal(%d * %d) max(%d * %d)\n",
                inputPortId[inputIndex],
                pInputBufferRequirement->minWidth, pInputBufferRequirement->minHeight,
                pInputBufferRequirement->optimalWidth, pInputBufferRequirement->optimalHeight,
                pInputBufferRequirement->maxWidth, pInputBufferRequirement->maxHeight);
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupCVP, "Backward buffer negotiation result %d", result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVPNode::FinalizeBufferProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CVPNode::FinalizeBufferProperties(
    BufferNegotiationData* pBufferNegotiationData)
{
    if (NULL != pBufferNegotiationData)
    {
        UINT               maxRes               = m_capability.maxInputWidth * m_capability.maxInputHeight;
        UINT               minDiff              = UINT_MAX;
        UINT               tarPort              = 0;
        UINT               refPort              = 0;
        UINT               resDiff              = 0;
        UINT               numInputPort         = 0;
        FLOAT              fullInputScaleFactor = 1.0f;
        UINT               inputPortId[CVPMaxInputPorts];
        const ImageFormat* pFormat;

        GetAllInputPortIds(&numInputPort, &inputPortId[0]);

        for (UINT inputIndex = 0; inputIndex < numInputPort; inputIndex++)
        {
            switch (inputPortId[inputIndex])
            {
                case CVPInputPortTARIFEFull:
                    pFormat           = pBufferNegotiationData->pInputPortNegotiationData[inputIndex].pImageFormat;
                    CAMX_LOG_INFO(CamxLogGroupCVP, "%s: Full port %d res %dX%d",
                                  NodeIdentifierString(),
                                  inputPortId[inputIndex],
                                  pFormat->width, pFormat->height);

                    m_fullInputHeight = pFormat->height;
                    m_fullInputWidth  = pFormat->width;
                    break;
                case CVPInputPortTARIFEDS4:
                case CVPInputPortTARIFEDS16:
                    pFormat = pBufferNegotiationData->pInputPortNegotiationData[inputIndex].pImageFormat;

                    CAMX_LOG_VERBOSE(CamxLogGroupCVP, "%s: idx %d port %d res %dX%d",
                                     NodeIdentifierString(),
                                     inputIndex,
                                     inputPortId[inputIndex],
                                     pFormat->width,
                                     pFormat->height);

                    resDiff = pFormat->width * pFormat->height;
                    CAMX_LOG_VERBOSE(CamxLogGroupCVP, "rediff 1 %u", resDiff);
                    resDiff = (resDiff > maxRes)? (resDiff - maxRes): (maxRes - resDiff);
                    CAMX_LOG_VERBOSE(CamxLogGroupCVP, "rediff 2 %u mindiff %u", resDiff, minDiff);
                    if ((m_capability.maxInputWidth >= pFormat->width && m_capability.maxInputHeight >= pFormat->height) &&
                        (resDiff < minDiff))
                    {
                        minDiff       = resDiff;
                        tarPort       = inputPortId[inputIndex];
                        refPort       = inputPortId[inputIndex] + 1;
                        m_inputWidth  = pFormat->width;
                        m_inputHeight = pFormat->height;
                        Utils::Memcpy(&m_inputFormatInfo, pFormat, sizeof(ImageFormat));
                    }
                    m_inputTARDS4Format = CVP_COLORFORMAT_PD10;
                    m_inputREFDS4Format = CVP_COLORFORMAT_PD10;

                    break;
                default:
                    break;
            }
        }
        CAMX_LOG_INFO(CamxLogGroupCVP, "Selected tar and ref ports %d/%d\n", tarPort, refPort);
        CAMX_LOG_INFO(CamxLogGroupCVP, "Negotiated Input Width/Height %u/%u\n", m_inputWidth, m_inputHeight);

        switch (tarPort)
        {
            case CVPInputPortTARIFEFull:
            {
                fullInputScaleFactor = 1.0f;
                break;
            }
            case CVPInputPortTARIFEDS4:
            {
                fullInputScaleFactor = 4.0f;

                if (TRUE == IsProfileIdRegistration())
                {
                    if ((0 != m_fullInputWidth) && (0 != m_inputWidth))
                    {
                        fullInputScaleFactor = (static_cast<FLOAT>(m_fullInputWidth) / static_cast<FLOAT>(m_inputWidth));
                    }
                    else
                    {
                        // defaulting to factor 3 which is registration expectation. may not be true always.
                        // for accurate factor connect full port.
                        fullInputScaleFactor = 3.0f;
                    }
                }
                break;
            }
            case CVPInputPortTARIFEDS16:
            default:
            {
                fullInputScaleFactor = 16;
                break;
            }
        }

        if ((0 == m_fullInputWidth) || (0 == m_fullInputHeight))
        {
            CAMX_LOG_WARN(CamxLogGroupCVP, "Full Input Port not connected!! Connect full port for better results");
            m_fullInputWidth  = static_cast<UINT32>(static_cast<FLOAT>(m_inputWidth)  * fullInputScaleFactor);
            m_fullInputHeight = static_cast<UINT32>(static_cast<FLOAT>(m_inputHeight) * fullInputScaleFactor);
        }

        // Initialize scaled width as full input width
        m_scaledWidth  = m_fullInputWidth;
        m_scaledHeight = m_fullInputHeight;
        m_ICAOutWidth  = m_inputWidth;
        m_ICAOutHeight = m_inputHeight;
        CAMX_LOG_INFO(CamxLogGroupCVP, "%s, fullInputScaleFactor=%f, Full input: %u x %u, Target input: %u x %u",
                      NodeIdentifierString(),
                      fullInputScaleFactor,
                      m_fullInputWidth,
                      m_fullInputHeight,
                      m_inputWidth,
                      m_inputHeight);

        for (UINT inputIndex = 0; inputIndex < numInputPort; inputIndex++)
        {
            if ((inputPortId[inputIndex] == refPort) && (TRUE != IsProfileIdRegistration()))
            {
                // need request - 1 buffer on ref port
                SetInputPortBufferDelta(inputIndex, 1);
            }
            else if (inputPortId[inputIndex] == CVPInputPortTARIFEFull)
            {
                UINT inputPortIndex = InputPortIndex(inputPortId[inputIndex]);
                CAMX_LOG_VERBOSE(CamxLogGroupCVP, "Disabling Full for profile %d",
                                 m_instanceProperty.profileId);
                DisableInputOutputLink(inputPortIndex, TRUE);
            }
        }

        for (UINT index = 0; index < pBufferNegotiationData->numOutputPortsNotified; index++)
        {
            OutputPortNegotiationData* pOutputPortNegotiationData   =
                &pBufferNegotiationData->pOutputPortNegotiationData[index];
            InputPortNegotiationData*  pInputPortNegotiationData    =
                &pBufferNegotiationData->pInputPortNegotiationData[0];
            BufferProperties*          pFinalOutputBufferProperties =
                pOutputPortNegotiationData->pFinalOutputBufferProperties;

            UINT outputPortId = GetOutputPortId(pOutputPortNegotiationData->outputPortIndex);

            switch (outputPortId)
            {
                case CVPOutputPortData:
                    //  Due to the size of DME context buffer need to wait until cvpInitDme is done.
                    //  So here we try to use a big enough default size for the buffer first
                    pFinalOutputBufferProperties->imageFormat.width  = CVPOutputPortDataDefaultSize;
                    pFinalOutputBufferProperties->imageFormat.height = 1;
                    CAMX_LOG_INFO(CamxLogGroupCVP, "CVPOutputPortData: W x H = %u x %u",
                                  pFinalOutputBufferProperties->imageFormat.width,
                                  pFinalOutputBufferProperties->imageFormat.height);
                    break;
                case CVPOutputPortImage:
                    pFinalOutputBufferProperties->imageFormat.width   = Utils::MaxUINT32(CVPMinimumWidthPixels, m_inputWidth);
                    pFinalOutputBufferProperties->imageFormat.height  = Utils::MaxUINT32(CVPMinimumHeightLines, m_inputHeight);

                    // CVPOutputPortImage port for CVP ICA should always output format UBWCNV12-4R,
                    // and its alignments are 256 / 16 for stride / height.
                    pFinalOutputBufferProperties->imageFormat.width  = Utils::AlignGeneric32(m_inputWidth, 256);
                    pFinalOutputBufferProperties->imageFormat.height = Utils::AlignGeneric32(m_inputHeight, 16);

                    pFinalOutputBufferProperties->imageFormat.planeAlignment[0].strideAlignment   = 256;
                    pFinalOutputBufferProperties->imageFormat.planeAlignment[1].strideAlignment   = 256;
                    pFinalOutputBufferProperties->imageFormat.planeAlignment[0].scanlineAlignment = 16;
                    pFinalOutputBufferProperties->imageFormat.planeAlignment[1].scanlineAlignment = 16;
                    CAMX_LOG_INFO(CamxLogGroupCVP, "output image port Width/Height %u/%u\n",
                        pFinalOutputBufferProperties->imageFormat.width, pFinalOutputBufferProperties->imageFormat.height);
                    break;
                default:
                    break;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCVP, "Invalid buffer negotiation data ptr");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::SetAAAInputData()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CVPNode::SetAAAInputData(
    ISPInputData* pInputData)
{
    VOID*   pData[2]   = { 0 };
    UINT64  offsets[2] = { 0 };

    if (TRUE ==  GetPipeline()->HasStatsNode())
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "Get 3A properties for realtime pipeline");
        // Note, the property order should matched what defined in the enum IPEVendorTagId
        static const UINT Properties3A[] =
        {
            PropertyIDAECFrameControl,
            PropertyIDAWBFrameControl,
        };
        static const UINT PropertySize = CAMX_ARRAY_SIZE(Properties3A);
        GetDataList(Properties3A, pData, offsets, PropertySize);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "Get 3A properties for non realtime pipeline");
        static const UINT Properties3A[] =
        {
            PropertyIDAECFrameControl    | InputMetadataSectionMask,
            PropertyIDAWBFrameControl    | InputMetadataSectionMask,
        };
        static const UINT PropertySize = CAMX_ARRAY_SIZE(Properties3A);
        GetDataList(Properties3A, pData, offsets, PropertySize);
    }

    if (NULL != pData[0])
    {
        Utils::Memcpy(pInputData->pAECUpdateData, pData[0], sizeof(AECFrameControl));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCVP, "Invalid AEC Frame metadata");
    }

    if (NULL != pData[1])
    {
        Utils::Memcpy(pInputData->pAWBUpdateData, pData[1], sizeof(AWBFrameControl));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCVP, "Invalid AWB Frame metadata");
    }


}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVPNode::IsFDEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CVPNode::IsFDEnabled(
    VOID)
{
    BOOL bIsFDPostingResultsEnabled = FALSE;

    // Set offset to 1 to point to the previous request.
    GetFDPerFrameMetaDataSettings(1, &bIsFDPostingResultsEnabled, NULL);

    return bIsFDPostingResultsEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVPNode::GetFaceROI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::GetFaceROI(
    ISPInputData* pInputData,
    UINT          parentNodeId)
{
    CamxResult              result                      = CamxResultSuccess;
    FaceROIInformation      faceRoiData                 = {};
    RectangleCoordinate*    pRoiRect                    = NULL;
    CHIRectangle            roiRect                     = {};
    CHIRectangle            cropInfo                    = {};
    CHIDimension            currentFrameDimension       = {};
    CHIRectangle            currentFrameMapWrtReference = {};
    CHIRectangle            roiWrtReferenceFrame        = {};
    UINT32                  metaTagFDRoi                = 0;
    BOOL                    bIsFDPostingResultsEnabled  = FALSE;

    GetFDPerFrameMetaDataSettings((BPS == parentNodeId) ? 0 : 2, &bIsFDPostingResultsEnabled, NULL);
    if (TRUE == bIsFDPostingResultsEnabled)
    {
        result = VendorTagManager::QueryVendorTagLocation(VendorTagSectionOEMFDResults,
                                                          VendorTagNameOEMFDResults, &metaTagFDRoi);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCVP, "query FD vendor failed result %d", result);
        }
        else
        {
            UINT              GetProps[]              = { metaTagFDRoi };
            static const UINT GetPropsLength          = CAMX_ARRAY_SIZE(GetProps);
            VOID*             pData[GetPropsLength]   = { 0 };
            UINT64            offsets[GetPropsLength] = { 0 };

            if (FALSE == IsRealTime())
            {
                GetProps[0] |= InputMetadataSectionMask;
            }

            offsets[0] = (BPS == parentNodeId) ? 0 : 2;
            GetDataList(GetProps, pData, offsets, GetPropsLength);

            if (NULL != pData[0])
            {
                Utils::Memcpy(&faceRoiData, pData[0], sizeof(FaceROIInformation));

                // Translate face roi if BPS is not parent
                if (BPS != parentNodeId)
                {
                    PortCropInfo portCropInfo = { { 0 } };
                    if (CamxResultSuccess == Node::GetPortCrop(this, CVPInputPortTARIFEFull, &portCropInfo, NULL))
                    {
                        cropInfo                            = portCropInfo.appliedCrop;
                        currentFrameMapWrtReference.top     = cropInfo.top;
                        currentFrameMapWrtReference.left    = cropInfo.left;
                        currentFrameMapWrtReference.width   = cropInfo.width;
                        currentFrameMapWrtReference.height  = cropInfo.height;

                        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "currentFrameWrtReference T:%d L:%d W:%d H:%d",
                                         currentFrameMapWrtReference.top, currentFrameMapWrtReference.left,
                                         currentFrameMapWrtReference.width, currentFrameMapWrtReference.height);
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCVP, "no applied crop data");
                        result = CamxResultEFailed;
                    }

                    if (CamxResultSuccess == result)
                    {
                        // Input width/height
                        currentFrameDimension.width  = m_fullInputWidth;
                        currentFrameDimension.height = m_fullInputHeight;
                        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "current dim W:%d H:%d",
                                         currentFrameDimension.width, currentFrameDimension.height);
                    }
                }

                if (CamxResultSuccess == result)
                {
                    pInputData->fDData.numberOfFace = static_cast<UINT16>(
                        (faceRoiData.ROICount > MAX_FACE_NUM) ? MAX_FACE_NUM : faceRoiData.ROICount);

                    CAMX_LOG_VERBOSE(CamxLogGroupCVP, "Face ROI is published face num %d max %d",
                                     pInputData->fDData.numberOfFace, MAX_FACE_NUM);

                    for (UINT16 i = 0; i < pInputData->fDData.numberOfFace; i++)
                    {
                        pRoiRect = &faceRoiData.unstabilizedROI[i].faceRect;

                        roiWrtReferenceFrame.left   = pRoiRect->left;
                        roiWrtReferenceFrame.top    = pRoiRect->top;
                        roiWrtReferenceFrame.width  = pRoiRect->width;
                        roiWrtReferenceFrame.height = pRoiRect->height;

                        // If BPS is parent, no conversion would be performed
                        roiRect = Translator::ConvertROIFromReferenceToCurrent(
                            &currentFrameDimension, &currentFrameMapWrtReference, &roiWrtReferenceFrame);

                        pInputData->fDData.faceCenterX[i] = static_cast<INT16>(roiRect.left + (roiRect.width / 2));
                        pInputData->fDData.faceCenterY[i] = static_cast<INT16>(roiRect.top + (roiRect.height / 2));
                        pInputData->fDData.faceRadius[i]  =
                            static_cast<INT16>(Utils::MinUINT32(roiRect.width, roiRect.height));

                        CAMX_LOG_VERBOSE(CamxLogGroupCVP, " center x:%d y:%d r:%d",
                                         pInputData->fDData.faceCenterX[i],
                                         pInputData->fDData.faceCenterY[i],
                                         pInputData->fDData.faceRadius[i]);
                    }
                }
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupCVP, "Face ROI is not published");
            }
        }
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVPNode::UpdateTuningModeData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::UpdateTuningModeData(
    ChiTuningModeParameter* pTuningModeData,
    ISPInputData*           pModuleInput)
{
    CamxResult result = CamxResultSuccess;
    if ((NULL != pModuleInput) && (NULL != pTuningModeData))
    {
        if (m_pPreTuningDataManager == pModuleInput->pTuningDataManager)
        {
            pModuleInput->tuningModeChanged = ISPIQModule::IsTuningModeDataChanged(
                pTuningModeData,
                &m_tuningData);
        }
        else
        {
            pModuleInput->tuningModeChanged = TRUE;
            m_pPreTuningDataManager         = pModuleInput->pTuningDataManager;
            CAMX_LOG_INFO(CamxLogGroupCVP, "TuningDataManager pointer is updated");
        }

        // if camera ID changed, it should set tuningModeChanged TRUE to trigger all IQ
        // module to update tuning parameters
        if (m_camIdChanged == TRUE)
        {
            pModuleInput->tuningModeChanged = TRUE;
        }

        // Needed to have different tuning data for different instances of a node within same pipeline
        // Also, cache tuning mode selector data for comparison for next frame, to help
        // optimize tuning data (tree) search in the IQ modules.
        // And, force update the tuning mode if camera id is updated, IPE node in SAT preview offline pipeline,
        // the active camera will switch based on the zoom level, we also need to update tuning data even
        // tuning mode not changed.
        if (TRUE == pModuleInput->tuningModeChanged)
        {
            Utils::Memcpy(&m_tuningData, pTuningModeData, sizeof(ChiTuningModeParameter));
        }

        // Now refer to the updated tuning mode selector data
        pModuleInput->pTuningData = &m_tuningData;

        // DumpTuningModeData();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCVP, " Invalid tuning mode data %pK, or module input %d",
            pTuningModeData, pModuleInput);
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVPNode::SetDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CVPNode::SetDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData)
{
    NodeProcessRequestData* pNodeRequestData    = pExecuteProcessRequestData->pNodeProcessRequestData;
    UINT32&                 rCount              = pNodeRequestData->dependencyInfo[0].propertyDependency.count;

    UINT64 requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(pNodeRequestData->pCaptureRequest->requestId);
    if (FirstValidRequestId < requestIdOffsetFromLastFlush)
    {
        pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount] = GetNodeCompleteProperty();
        // Always point to the previous request. Should NOT be tied to the pipeline delay!
        pNodeRequestData->dependencyInfo[0].propertyDependency.offsets[rCount] = 1;
        rCount++;
    }

    if (TRUE == GetStaticSettings()->enableImageBufferLateBinding)
    {
        // If latebinding is enabled, we want to delay packet preparation as late as possible, in other terms, we want to
        // prepare and submit to hw when it can really start processing. This is once all the input fences (+ property)
        // dependencies are satisfied. So, lets set input fence dependencies

        UINT fenceCount = SetInputBuffersReadyDependency(pExecuteProcessRequestData, 0);

        if (0 < fenceCount)
        {
            pNodeRequestData->dependencyInfo[0].processSequenceId = 1;
            pNodeRequestData->numDependencyLists                  = 1;
        }
    }

    if (TRUE == GetPipeline()->HasStatsNode())
    {
        // 3A dependency
        if (TRUE == IsTagPresentInPublishList(PropertyIDAECFrameControl))
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount++] = PropertyIDAECFrameControl;
        }
        if (TRUE == IsTagPresentInPublishList(PropertyIDAWBFrameControl))
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount++] = PropertyIDAWBFrameControl;
        }
    }

    // Set dependency for GeoLib
    SetGeoLibStillFrameConfigurationDependency(pNodeRequestData);

    if (TRUE == m_bICAEnabled)
    {
        SetICADependencies(pNodeRequestData);
    }

    if (0 < pNodeRequestData->dependencyInfo[0].propertyDependency.count)
    {
        pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency = TRUE;
    }

    pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency   = TRUE;
    pNodeRequestData->dependencyInfo[0].processSequenceId                                   = 1;
    pNodeRequestData->numDependencyLists                                                    = 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Dme_CallBack
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Dme_CallBack(
    cvpStatus      eStatus,
    cvpImage*      pScaledRefImage,
    cvpMem*        pScaledRefFrameCtx,
    cvpDmeOutput*  pDmeOutput,
    cvpHandle      hDme,
    VOID*          pSessionUserData,
    VOID*          pTaskUserData)
{
    CAMX_LOG_VERBOSE(CamxLogGroupCVP, "Dme_CallBack E");
    (void) eStatus;
    (void) pScaledRefImage;
    (void) pScaledRefFrameCtx;
    (void) pDmeOutput;
    (void) hDme;
    (void) pSessionUserData;

    UINT64* pRequestId = static_cast<UINT64 *>(pTaskUserData);

    CAMX_LOG_VERBOSE(CamxLogGroupCVP, "CVP Disabled from settings X requestId %llu", *pRequestId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::SetCVPDimensions()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVPNode::SetCVPDimensions()
{

    if ((0 != m_stabilizationMargin.widthPixels) &&
        (0 != m_stabilizationMargin.heightLines))
    {
        m_scaledWidth  = Utils::AlignGeneric32((m_fullInputWidth - m_stabilizationMargin.widthPixels), 32);
        m_scaledWidth  = Utils::MinUINT32(m_fullInputWidth, m_scaledWidth);
        m_ICAOutWidth  = Utils::AlignGeneric32(m_scaledWidth, 8) / 4;

        m_scaledHeight = Utils::AlignGeneric32((m_fullInputHeight - m_stabilizationMargin.heightLines), 32);
        m_scaledHeight = Utils::MinUINT32(m_fullInputHeight, m_scaledHeight);
        m_ICAOutHeight = Utils::AlignGeneric32(m_scaledHeight, 8) / 4;
    }
    else
    {
        m_scaledWidth  = m_fullInputWidth;
        m_scaledHeight = m_fullInputHeight;
        m_ICAOutWidth  = m_inputWidth;
        m_ICAOutHeight = m_inputHeight;
    }
    CAMX_LOG_INFO(CamxLogGroupCVP, "%s: w %d, h %d,full w %d,full h %d,margin w %d, h %d,icaout w %d,h %d,scaled w %d,h %d",
                  NodeIdentifierString(), m_inputWidth, m_inputHeight,
                  m_fullInputWidth, m_fullInputHeight,
                  m_stabilizationMargin.widthPixels,
                  m_stabilizationMargin.heightLines,
                  m_ICAOutWidth, m_ICAOutHeight,
                  m_scaledWidth, m_scaledHeight);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::SetValuesForGeoLibIcaMapping
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVPNode::SetValuesForGeoLibIcaMapping(
    GeoLibIcaMapping* pIcaMapping,
    UINT32 widthInPixels,
    UINT32 heightInLines)
{

    CAMX_LOG_INFO(CamxLogGroupCVP, "%s: w %d, h %d , full w %d, full h %d,  margin w %d, h %d",
                  NodeIdentifierString(), widthInPixels, heightInLines,
                  m_fullInputWidth, m_fullInputHeight,
                  m_stabilizationMargin.widthPixels,
                  m_stabilizationMargin.heightLines);

    pIcaMapping->inputImageSize.widthPixels  = widthInPixels;
    pIcaMapping->inputImageSize.heightLines  = heightInLines;
    pIcaMapping->inputValidSize.widthPixels  = widthInPixels;
    pIcaMapping->inputValidSize.heightLines  = heightInLines;

    if ((0 != m_stabilizationMargin.widthPixels) &&
        (0 != m_stabilizationMargin.heightLines))
    {
        pIcaMapping->inputImageRoi.size.x        = (static_cast<FLOAT>(widthInPixels) /
                                                   (static_cast<FLOAT>(m_fullInputWidth) / 4.0f));
        pIcaMapping->inputImageRoi.size.y        = (static_cast<FLOAT>(heightInLines) /
                                                   (static_cast<FLOAT>(m_fullInputHeight) / 4.0f));
        pIcaMapping->inputImageRoi.offset.x      = 0.5f * (1 / pIcaMapping->inputImageRoi.size.x - 1);
        pIcaMapping->inputImageRoi.offset.y      = 0.5f * (1 / pIcaMapping->inputImageRoi.size.y - 1);

        pIcaMapping->outputImageSize.widthPixels = m_ICAOutWidth;
        pIcaMapping->outputImageSize.heightLines = m_ICAOutHeight;
        pIcaMapping->outputImageRoi.size.x       = static_cast<FLOAT>(m_ICAOutWidth) /
                                                   (static_cast<FLOAT>(m_fullInputWidth) / 4.0f);
        pIcaMapping->outputImageRoi.size.y       = static_cast<FLOAT>(m_ICAOutHeight) /
                                                   (static_cast<FLOAT>(m_fullInputHeight) / 4.0f);

        pIcaMapping->outputImageRoi.offset.x = 0.5f * (1 - pIcaMapping->outputImageRoi.size.x);
        pIcaMapping->outputImageRoi.offset.y = 0.5f * (1 - pIcaMapping->outputImageRoi.size.y);

        m_scaleFactorW                           = m_scaledWidth / m_ICAOutWidth;
        m_scaleFactorH                           = m_scaledHeight / m_ICAOutHeight;
        m_coarseCenterW                          = m_ICAOutWidth / 2;
        m_coarseCenterH                          = m_ICAOutHeight / 2;
    }
    else
    {
        pIcaMapping->inputImageRoi.size.x        = 1.0f;
        pIcaMapping->inputImageRoi.size.y        = 1.0f;
        pIcaMapping->inputImageRoi.offset.x      = 0.5f * (1 / pIcaMapping->inputImageRoi.size.x - 1);
        pIcaMapping->inputImageRoi.offset.y      = 0.5f * (1 / pIcaMapping->inputImageRoi.size.y - 1);
        pIcaMapping->outputImageSize.widthPixels = widthInPixels;
        pIcaMapping->outputImageSize.heightLines = heightInLines;
        pIcaMapping->outputImageRoi.size.x       = 1.0f;
        pIcaMapping->outputImageRoi.size.y       = 1.0f;
        pIcaMapping->outputImageRoi.offset.x     = 0.5f * (pIcaMapping->outputImageRoi.size.x - 1);
        pIcaMapping->outputImageRoi.offset.y     = 0.5f * (pIcaMapping->outputImageRoi.size.y - 1);
        m_scaleFactorW                           = m_fullInputWidth / widthInPixels;
        m_scaleFactorH                           = m_fullInputHeight / heightInLines;
        m_coarseCenterW                          = widthInPixels / 2;
        m_coarseCenterH                          = heightInLines / 2;
    }

    CAMX_LOG_INFO(CamxLogGroupCVP, " input  w %d, h %d,valid w %d h %d, size x %f, y %f, offset x %f, y %f",
                  pIcaMapping->inputImageSize.widthPixels,
                  pIcaMapping->inputImageSize.heightLines,
                  pIcaMapping->inputValidSize.widthPixels,
                  pIcaMapping->inputValidSize.heightLines,
                  pIcaMapping->inputImageRoi.size.x,
                  pIcaMapping->inputImageRoi.size.y,
                  pIcaMapping->inputImageRoi.offset.x,
                  pIcaMapping->inputImageRoi.offset.y);

    CAMX_LOG_INFO(CamxLogGroupCVP, " output  w %d, h %d,size x %f, y %f, offset x %f, y %f",
                  pIcaMapping->outputImageSize.widthPixels,
                  pIcaMapping->outputImageSize.heightLines,
                  pIcaMapping->outputImageRoi.size.x,
                  pIcaMapping->outputImageRoi.size.y,
                  pIcaMapping->outputImageRoi.offset.x,
                  pIcaMapping->outputImageRoi.offset.y);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::SetICADependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CVPNode::SetICADependencies(
    NodeProcessRequestData*  pNodeRequestData)
{
    if (NULL != pNodeRequestData)
    {
        SetPortLinkICATransformDependency(pNodeRequestData);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCVP, "%s:Invalid node request data", NodeIdentifierString());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::UpdateICADependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::UpdateICADependencies(
    ISPInputData* pInputData)
{
    CamxResult     result                                   = CamxResultSuccess;
    VOID*          pPSPropertyDataICA[1]                    = { 0 };
    UINT64         psPropertyDataICAOffset[1]               = { 0 };
    UINT           psPropertiesICATag[1]                    = { 0 };

    if ((TRUE == IsStabilizationTypeSAT()) || (TRUE == IsStabilizationTypeEIS()))
    {
        // In perspective transform tag
        psPropertiesICATag[0]      = m_CVPICATAGLocation[0];
        pPSPropertyDataICA[0]      = NULL;
        psPropertyDataICAOffset[0] = { 0 };

        // get port link based metadata
        GetPSDataList(CVPInputPortTARIFEDS4, psPropertiesICATag, pPSPropertyDataICA, psPropertyDataICAOffset, 1);

        if (NULL != pPSPropertyDataICA[0])
        {
            Utils::Memcpy(&pInputData->ICAConfigData.ICAInPerspectiveParams,
                reinterpret_cast<IPEICAPerspectiveTransform*>(pPSPropertyDataICA[0]),
                sizeof(pInputData->ICAConfigData.ICAInPerspectiveParams));
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupCVP, "IN perspective transform dependency met but data NULL");
            pInputData->ICAConfigData.ICAInPerspectiveParams.perspectiveTransformEnable         = TRUE;
            pInputData->ICAConfigData.ICAInPerspectiveParams.perspectiveConfidence              = 1;
            pInputData->ICAConfigData.ICAInPerspectiveParams.byPassAlignmentMatrixAdjustement   = TRUE;
            pInputData->ICAConfigData.ICAInPerspectiveParams.perspetiveGeometryNumColumns       = 1;
            pInputData->ICAConfigData.ICAInPerspectiveParams.perspectiveGeometryNumRows         = 1;
            pInputData->ICAConfigData.ICAInPerspectiveParams.transformDefinedOnWidth            = m_fullInputWidth;
            pInputData->ICAConfigData.ICAInPerspectiveParams.transformDefinedOnHeight           = m_fullInputHeight;
            pInputData->ICAConfigData.ICAInPerspectiveParams.ReusePerspectiveTransform          = 0;
            memcpy(&pInputData->ICAConfigData.ICAInPerspectiveParams.perspectiveTransformArray,
                   &perspArray, sizeof(FLOAT) * ICAParametersPerPerspectiveTransform);
        }

        CAMX_LOG_INFO(CamxLogGroupCVP, "CVP:%d perspective IN %d, w %d, h %d, c %d, r %d, frameNum %llu",
                      InstanceID(),
                      pInputData->ICAConfigData.ICAInPerspectiveParams.perspectiveTransformEnable,
                      pInputData->ICAConfigData.ICAInPerspectiveParams.transformDefinedOnWidth,
                      pInputData->ICAConfigData.ICAInPerspectiveParams.transformDefinedOnHeight,
                      pInputData->ICAConfigData.ICAInPerspectiveParams.perspetiveGeometryNumColumns,
                      pInputData->ICAConfigData.ICAInPerspectiveParams.perspectiveGeometryNumRows,
                      pInputData->frameNum);
    }

    if (TRUE == static_cast<Titan17xContext*>(
        GetHwContext())->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->enableICAInGrid)
    {
        pPSPropertyDataICA[0]      = NULL;
        psPropertyDataICAOffset[0] = { 0 };

        if ((TRUE == IsStabilizationTypeEIS()) || (TRUE == CheckForNonEISLDCGridDependency()))
        {
            // In grid transform tag
            psPropertiesICATag[0] = m_CVPICATAGLocation[1];
            // get port link based metadata
            GetPSDataList(CVPInputPortTARIFEDS4, psPropertiesICATag, pPSPropertyDataICA, psPropertyDataICAOffset, 1);
        }

        if (NULL != pPSPropertyDataICA[0])
        {
            Utils::Memcpy(&pInputData->ICAConfigData.ICAInGridParams,
                reinterpret_cast<IPEICAGridTransform*>(pPSPropertyDataICA[0]),
                sizeof(pInputData->ICAConfigData.ICAInGridParams));
        }
        else if ((TRUE == IsStabilizationTypeSAT()) ||
                 (TRUE == IsCVPInBlend()))
        {
            // Query from Input data for offline pipeline
            static const UINT PropertiesOfflineICATag[] =
            {
                m_CVPICATAGLocation[1] | InputMetadataSectionMask
            };
            VOID*        pICAData[1]              = { 0 };
            UINT64       propertyDataIPEOffset[1] = { 0 };

            GetDataList(PropertiesOfflineICATag, pICAData, propertyDataIPEOffset, 1);
            if (NULL != pICAData[0])
            {
                Utils::Memcpy(&pInputData->ICAConfigData.ICAInGridParams,
                    reinterpret_cast<IPEICAGridTransform*>(pICAData[0]),
                    sizeof(pInputData->ICAConfigData.ICAInGridParams));
            }
        }

        CAMX_LOG_INFO(CamxLogGroupCVP, "CVP:%d grid IN %d, w %d, h %d, corner %d, frameNum %llu",
                      InstanceID(),
                      pInputData->ICAConfigData.ICAInGridParams.gridTransformEnable,
                      pInputData->ICAConfigData.ICAInGridParams.transformDefinedOnWidth,
                      pInputData->ICAConfigData.ICAInGridParams.transformDefinedOnHeight,
                      pInputData->ICAConfigData.ICAInGridParams.gridTransformArrayExtrapolatedCorners,
                      pInputData->frameNum);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::ExecuteProcessRequest(
    ExecuteProcessRequestData* pExecuteProcessRequestData)
{
    BOOL                    hasDependency    = FALSE;
    CamxResult              result           = CamxResultSuccess;
    NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
    PerRequestActivePorts*  pPerRequestPorts = pExecuteProcessRequestData->pEnabledPortsInfo;
    UINT64                  requestId        = pNodeRequestData->pCaptureRequest->requestId;
    UINT32                  requestIdIndex   = static_cast<UINT32>(requestId) % m_requestQueueDepth;
    UINT32                  numBatchedFrames = pNodeRequestData->pCaptureRequest->numBatchedFrames;
    UINT                    parentNodeID     = 0;
    BOOL                    isMasterCamera   = TRUE;

    CAMX_LOG_VERBOSE(CamxLogGroupCVP,
                     "%s CVP Entry Req %llu, seq %d, numInputPorts %d, numOutputPorts %d, driver disabled %u, id %u, type %u"
                     "numBatchedFrames = %u",
                     NodeIdentifierString(),
                     requestId,
                     pNodeRequestData->processSequenceId,
                     pPerRequestPorts->numInputPorts,
                     pPerRequestPorts->numOutputPorts,
                     m_disableCVPDriver,
                     m_instanceProperty.profileId,
                     m_instanceProperty.processingType,
                     numBatchedFrames);

    for (UINT i = 0; i < pPerRequestPorts->numInputPorts; i++)
    {
        PerRequestInputPortInfo* pInputPort = &pPerRequestPorts->pInputPorts[i];

        if ((NULL != pInputPort) && (CVPInputPortTARIFEDS4 == pInputPort->portId))
        {
            parentNodeID = GetParentNodeType(pInputPort->portId);
        }
    }

    if (0 == pNodeRequestData->processSequenceId)
    {
        SetDependencies(pExecuteProcessRequestData);
    }

    if (pNodeRequestData->dependencyInfo[0].dependencyFlags.dependencyFlagsMask)
    {
        hasDependency = TRUE;
    }

    if (FALSE == hasDependency)
    {
        if (TRUE == m_disableCVPDriver)
        {
            for (UINT outputIndex = 0; outputIndex < pPerRequestPorts->numOutputPorts; outputIndex++)
            {
                PerRequestOutputPortInfo* pOutputPort = &pPerRequestPorts->pOutputPorts[outputIndex];
                if ((NULL != pOutputPort) &&
                    ((CVPOutputPortData == pOutputPort->portId) || (CVPOutputPortImage == pOutputPort->portId)))
                {
                    CSLFenceSignal(*pOutputPort->phFence, CSLFenceResultSuccess);
                }
            }
        }
        else
        {
            cvpDmeFrameConfigIca*   pCVPICAFrameConfig = NULL;
            ISPInputData            moduleInput        = {};
            AECFrameControl         AECUpdateData      = {};
            AWBFrameControl         AWBUpdateData      = {};
            IpeIQSettings           ipeIQsettings;
            cvpMem                  gridBuffer;

            // Initialize ICA parameters
            moduleInput.ICAConfigData.ICAInGridParams.gridTransformEnable                  = 0;
            moduleInput.ICAConfigData.ICAInInterpolationParams.customInterpolationEnabled  = 0;
            moduleInput.ICAConfigData.ICAInPerspectiveParams.perspectiveTransformEnable    = 0;
            moduleInput.ICAConfigData.ICARefGridParams.gridTransformEnable                 = 0;
            moduleInput.ICAConfigData.ICARefPerspectiveParams.perspectiveTransformEnable   = 0;
            moduleInput.ICAConfigData.ICARefInterpolationParams.customInterpolationEnabled = 0;
            moduleInput.ICAConfigData.ICAReferenceParams.perspectiveTransformEnable        = 0;
            moduleInput.registerBETEn                                                      = FALSE;
            moduleInput.frameNum                                                           = requestId;

            BOOL   isMultiCameraUsecase;
            UINT32 numberOfCamerasRunning;
            UINT32 currentCameraId;
            UINT32 cameraId = GetPipeline()->GetCameraId();

            GetMultiCameraInfo(&isMultiCameraUsecase, &numberOfCamerasRunning, &currentCameraId, &isMasterCamera);

            if (TRUE == isMultiCameraUsecase)
            {
                UINT32 cameraIDmetaTag = 0;
                result = VendorTagManager::QueryVendorTagLocation("com.qti.chi.metadataOwnerInfo",
                         "MetadataOwner",
                         &cameraIDmetaTag);

                if (CamxResultSuccess == result)
                {
                    UINT              propertiesIPE[]   = { cameraIDmetaTag | InputMetadataSectionMask };
                    static const UINT Length            = CAMX_ARRAY_SIZE(propertiesIPE);
                    VOID*             pData[Length]     = { 0 };
                    UINT64            offsets[Length]   = { 0 };

                    result = GetDataList(propertiesIPE, pData, offsets, Length);

                    if (CamxResultSuccess == result)
                    {
                        if (NULL != pData[0])
                        {
                            cameraId = *reinterpret_cast<UINT*>(pData[0]);
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupCVP,
                                "CVP:%u Can't get Current camera ID!!!, pData is NULL", InstanceID());
                        }
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCVP, "CVP:%u Can't query vendor tag: MetadataOwner", InstanceID());
                }

                CAMX_LOG_VERBOSE(CamxLogGroupCVP,
                    "node: %s,req ID %llu numberOfCamerasRunning = %d, cameraId = %d isMasterCamera = %d",
                    NodeIdentifierString(), requestId, numberOfCamerasRunning, cameraId, isMasterCamera);
            }

            if (cameraId == m_cameraId)
            {
                m_camIdChanged = FALSE;
            }
            else
            {
                m_cameraId = cameraId;
                m_camIdChanged = TRUE;
            }

            moduleInput.pipelineIPEData.pIPEIQSettings             = &ipeIQsettings;

            moduleInput.pTuningDataManager         = GetTuningDataManagerWithCameraId(cameraId);
            moduleInput.pHwContext                 = GetHwContext();
            moduleInput.pipelineIPEData.instanceID = InstanceID();
            moduleInput.titanVersion               = static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion();

            UpdateICADependencies(&moduleInput);

            moduleInput.pAECUpdateData                             = &AECUpdateData;
            moduleInput.pAWBUpdateData                             = &AWBUpdateData;

            SetAAAInputData(&moduleInput);

            moduleInput.pipelineIPEData.inputDimension.widthPixels = m_inputWidth;
            moduleInput.pipelineIPEData.inputDimension.heightLines = m_inputHeight;
            moduleInput.opticalCenterX                             = (m_inputWidth) / 2;
            moduleInput.opticalCenterY                             = (m_inputHeight) / 2;

            moduleInput.pipelineIPEData.marginDimension.widthPixels = 0;
            moduleInput.pipelineIPEData.marginDimension.heightLines = 0;

            moduleInput.pipelineIPEData.ICAMode    = ICA_CONFIG_NONE;

            if (TRUE == m_bICAEnabled)
            {
                if (TRUE == IsProfileIdRegistration())
                {
                    SetupICAPerspectiveForRegistration(&moduleInput.ICAConfigData.ICAInPerspectiveParams);
                    GetGeoLibICAMapping(&moduleInput.pipelineCVPData.icaInputData.icaMappingFull, requestId);
                }
                else
                {
                    // Need to access this from IFE posted vendor tag
                    SetValuesForGeoLibIcaMapping(&moduleInput.pipelineCVPData.icaInputData.icaMappingFull,
                                                 m_inputWidth,
                                                 m_inputHeight);
                }
            }

            if ((TRUE == IsFDEnabled())     &&
                (1    == pNodeRequestData->pCaptureRequest->numBatchedFrames))
            {
                result = GetFaceROI(&moduleInput, m_parentNodeID);
            }

            result = UpdateTuningModeData(pExecuteProcessRequestData->pTuningModeData, &moduleInput);

            if (CamxResultSuccess == result)
            {
                result = LoadCVPChromatix(&moduleInput);
                m_dependenceData.pChromatix = m_pChromatix;
            }

            if (CamxResultSuccess == result)
            {
                // program settings for IQ modules
                result = ProgramIQConfig(&moduleInput);
            }

            if (TRUE == m_bICAEnabled)
            {
                if (CamxResultSuccess == result)
                {
                    pCVPICAFrameConfig = static_cast<cvpDmeFrameConfigIca*>(moduleInput.pipelineCVPData.pCVPICAFrameCfgData);

                    if (TRUE == m_dumpCVPICAInputConfig)
                    {
                        DumpICAFrameConfig(pCVPICAFrameConfig);
                    }

                    Utils::Memcpy(m_ppGridBuffers[requestIdIndex]->GetCSLBufferInfo()->pVirtualAddr,
                                  pCVPICAFrameConfig,
                                  sizeof(cvpDmeFrameConfigIca));

                    gridBuffer.nFD   = m_ppGridBuffers[requestIdIndex]->GetCSLBufferInfo()->fd;
                    gridBuffer.nSize = m_ppGridBuffers[requestIdIndex]->GetCSLBufferInfo()->size;

                    CAMX_LOG_INFO(CamxLogGroupCVP, "Grid buffer req %lld, Grid size %d Pes size %d!!",
                                  requestId, pCVPICAFrameConfig->nGridDataSize, pCVPICAFrameConfig->nPerspectiveDataSize);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCVP, "program IQ modules settings failed");
                }
            }

            BOOL                    isReferenceEnabled = FALSE;
            cvpStatus               status             = CVP_SUCCESS;

            cvpImage                pInputImage;
            cvpImage                pReferenceImage;
            cvpImage                pICAOutputImage;

            cvpMem                  inputBuffer;
            cvpMem                  referenceBuffer;
            cvpMem                  dmeOutput;
            cvpMem                  icaOutput;
            cvpMem                  srcFrameCtx;
            cvpMem                  refFrameCtx;
            cvpFence                fenceInfo[8];

            synx_export_params_t    exportParams;
            synx_external_desc_t    externalDesc;
            synx_result_t           synxResult;
            UINT32                  synxSrcImageKey    = 0;
            UINT32                  synxDmeOutputKey   = 0;
            UINT32                  synxIcaOutputKey   = 0;
            synx_create_params_t    synxParams         = {0};
            externalDesc.type                          = SYNX_TYPE_CSL_FENCE;
            externalDesc.id[0]                         = 0;
            exportParams.size                          = sizeof(exportParams);

            for (UINT inputIndex = 0; inputIndex < pPerRequestPorts->numInputPorts; inputIndex++)
            {
                PerRequestInputPortInfo* pInputPort = &pPerRequestPorts->pInputPorts[inputIndex];
                const ImageFormat*       pFormat    = pInputPort->pImageBuffer->GetFormat();

                switch(pInputPort->portId)
                {
                    case CVPInputPortTARIFEDS4:
                        m_parentNodeID      = GetParentNodeType(pInputPort->portId);

                        inputBuffer.nFD     = pInputPort->pImageBuffer->GetCSLBufferInfo()->fd;

                        if (numBatchedFrames > 0)
                        {
                            //  Normally, the size of image buffer we get here is the batched size.
                            //  But what CVP expect is the size of one frame, so we need to / numBatchedFrames for it.
                            inputBuffer.nSize   = pInputPort->pImageBuffer->GetCSLBufferInfo()->size / numBatchedFrames;
                            inputBuffer.nOffset = pInputPort->pImageBuffer->GetPlaneSize(0) * (numBatchedFrames - 1);
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupCVP, "Input: TARIFEDS4, numBatchedFrames is %u, <= 0", numBatchedFrames);
                            result = CamxResultEFailed;
                            break;
                        }

                        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "Input: TARIFEDS4, fd %d size %d offset %d!",
                                         inputBuffer.nFD, inputBuffer.nSize, inputBuffer.nOffset);

                        pInputImage.pBuffer = &inputBuffer;
                        if (NULL != pFormat)
                        {
                            status = cvpQueryImageInfo_dme(m_inputTARDS4Format,
                                                           pFormat->width,
                                                           pFormat->height,
                                                           &pInputImage.sImageInfo,
                                                           m_bICAEnabled);

                            CAMX_LOG_VERBOSE(CamxLogGroupCVP, "fmt %d %dX%d nplane %d size %d str[0] %d size[0] %d,"
                                             "str[1] %d size[1] %d",
                                             pInputImage.sImageInfo.eFormat,
                                             pInputImage.sImageInfo.nWidth,
                                             pInputImage.sImageInfo.nHeight,
                                             pInputImage.sImageInfo.nPlane,
                                             pInputImage.sImageInfo.nTotalSize,
                                             pInputImage.sImageInfo.nWidthStride[0],
                                             pInputImage.sImageInfo.nAlignedSize[0],
                                             pInputImage.sImageInfo.nWidthStride[1],
                                             pInputImage.sImageInfo.nAlignedSize[1]);

                            result = CreateSynxObject(&m_phSynxSrcImage[requestIdIndex],
                                                      *(pInputPort->phFence),
                                                      &synxSrcImageKey);
                        }
                        break;

                    case CVPInputPortREFIFEDS4:
                        isReferenceEnabled      = TRUE;
                        referenceBuffer.nFD     = pInputPort->pImageBuffer->GetCSLBufferInfo()->fd;

                        if (numBatchedFrames > 0)
                        {
                            //  Normally, the size of image buffer we get here is the batched size.
                            //  But what CVP expect is the size of one frame, so we need to / numBatchedFrames for it.
                            referenceBuffer.nSize   = pInputPort->pImageBuffer->GetCSLBufferInfo()->size / numBatchedFrames;
                            referenceBuffer.nOffset = pInputPort->pImageBuffer->GetPlaneSize(0) * (numBatchedFrames - 1);
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupCVP, "Input: REFIFEDS4, numBatchedFrames is %u, <= 0", numBatchedFrames);
                            result = CamxResultEFailed;
                            break;
                        }

                        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "Input: REFIFEDS4, fd %d size %d offset %d!",
                                         referenceBuffer.nFD, referenceBuffer.nSize, referenceBuffer.nOffset);

                        pReferenceImage.pBuffer = &referenceBuffer;

                        if (NULL != pFormat)
                        {
                            status = cvpQueryImageInfo_dme(m_inputREFDS4Format,
                                                           pFormat->width,
                                                           pFormat->height,
                                                           &pReferenceImage.sImageInfo,
                                                           m_bICAEnabled);

                            CAMX_LOG_VERBOSE(CamxLogGroupCVP, "fmt %d %dX%d nplane %d size %d str[0] %d size[0] %d,"
                                             "str[1] %d size[1] %d",
                                             pReferenceImage.sImageInfo.eFormat,
                                             pReferenceImage.sImageInfo.nWidth,
                                             pReferenceImage.sImageInfo.nHeight,
                                             pReferenceImage.sImageInfo.nPlane,
                                             pReferenceImage.sImageInfo.nTotalSize,
                                             pReferenceImage.sImageInfo.nWidthStride[0],
                                             pReferenceImage.sImageInfo.nAlignedSize[0],
                                             pReferenceImage.sImageInfo.nWidthStride[1],
                                             pReferenceImage.sImageInfo.nAlignedSize[1]);
                        }
                        break;

                    case CVPInputPortData:
                        //  This port only used in blend stage now.
                        if (TRUE == IsCVPInBlend())
                        {
                            //  Here we try to get the marker which we appended in the end of DME src context buffer
                            //  in prefilter. Because the anchor is always the same one in blend stage, so with this
                            //  marker, we can only copy this context buffer to local once and reuse it for the later
                            //  CVP blend request to avoid redundant memcpy.
                            UINT32 prefilterRequestIdIndex =
                                        *(static_cast<UINT32*>(pInputPort->pImageBuffer->GetCSLBufferInfo()->pVirtualAddr) +
                                                            (Utils::ByteAlign32(m_outMemReq.nRefFrameContextBytes, 4) >> 2));

                            if (m_previousRequestIdIndex != prefilterRequestIdIndex)
                            {
                                m_previousRequestIdIndex = prefilterRequestIdIndex;
                                Utils::Memcpy(m_ppSourceContextBuffers[0]->GetCSLBufferInfo()->pVirtualAddr,
                                              pInputPort->pImageBuffer->GetCSLBufferInfo()->pVirtualAddr,
                                              m_outMemReq.nRefFrameContextBytes);
                            }
                        }
                        break;
                    default:
                        break;
                }

                if (NULL != pFormat)
                {
                    CAMX_LOG_INFO(CamxLogGroupCVP,
                                  "%s reporting Input config, portId=%d, imgBuf=%p, hFence=%d, dims %dx%d, request=%llu",
                                  NodeIdentifierString(),
                                  pInputPort->portId,
                                  pInputPort->pImageBuffer,
                                  *(pInputPort->phFence),
                                  pFormat->width, pFormat->height,
                                  requestId);
                }
            }

            if (CamxResultSuccess == result)
            {
                for (UINT outputIndex = 0; outputIndex < pPerRequestPorts->numOutputPorts; outputIndex++)
                {
                    PerRequestOutputPortInfo* pOutputPort = &pPerRequestPorts->pOutputPorts[outputIndex];

                    if (NULL != pOutputPort)
                    {
                        switch(pOutputPort->portId)
                        {
                            case CVPOutputPortData:
                                if (NULL != pOutputPort->ppImageBuffer[0])
                                {
                                    const ImageFormat* pFormat = pOutputPort->ppImageBuffer[0]->GetFormat();
                                    if (NULL != pFormat)
                                    {
                                        CAMX_LOG_INFO(CamxLogGroupCVP,
                                                      "CVP:%d reporting I/O config, portId=%d, imgBuf=%p, hFence=%d,"
                                                      " dims %dx%d request=%llu",
                                                      InstanceID(),
                                                      pOutputPort->portId,
                                                      pOutputPort->ppImageBuffer[0],
                                                      *(pOutputPort->phFence),
                                                      pFormat->width, pFormat->height,
                                                      requestId);
                                    }
                                    else
                                    {
                                        CAMX_LOG_ERROR(CamxLogGroupCVP, "pFormat is null");
                                        result = CamxResultEFailed;
                                        break;
                                    }

                                    dmeOutput.nFD   = pOutputPort->ppImageBuffer[0]->GetCSLBufferInfo()->fd;
                                    dmeOutput.nSize = m_outMemReq.nDMEOutputBytes;

                                    result = CreateSynxObject(&m_phSynxDmeOutput[requestIdIndex],
                                                              *(pOutputPort->phFence),
                                                              &synxDmeOutputKey);
                                }
                                break;
                            case CVPOutputPortImage:
                                if (NULL != pOutputPort->ppImageBuffer[0])
                                {
                                    const ImageFormat* pFormat = pOutputPort->ppImageBuffer[0]->GetFormat();
                                    if (NULL != pFormat)
                                    {
                                        CAMX_LOG_INFO(CamxLogGroupCVP,
                                                      "CVP:%d reporting I/O config, portId=%d, imgBuf=%p, hFence=%d,"
                                                      " dims %dx%d request=%llu, format=%u",
                                                      InstanceID(),
                                                      pOutputPort->portId,
                                                      pOutputPort->ppImageBuffer[0],
                                                      *(pOutputPort->phFence),
                                                      pFormat->width, pFormat->height,
                                                      pNodeRequestData->pCaptureRequest->requestId,
                                                      pFormat->format);
                                    }
                                    else
                                    {
                                        CAMX_LOG_ERROR(CamxLogGroupCVP, "pFormat is null");
                                        result = CamxResultEFailed;
                                        break;
                                    }

                                    icaOutput.nFD           = pOutputPort->ppImageBuffer[0]->GetCSLBufferInfo()->fd;
                                    icaOutput.nSize         = pOutputPort->ppImageBuffer[0]->GetCSLBufferInfo()->size;
                                    pICAOutputImage.pBuffer = &icaOutput;

                                    CAMX_LOG_VERBOSE(CamxLogGroupCVP, "ICA out fd %d size %d offset %d!",
                                                     icaOutput.nFD, icaOutput.nSize, icaOutput.nOffset);

                                    result = CreateSynxObject(&m_phSynxIcaOutput[requestIdIndex],
                                                              *(pOutputPort->phFence),
                                                              &synxIcaOutputKey);
                                }
                                break;
                            default:
                                break;
                        }
                    }
                }
            }

            if ((NULL == pInputImage.pBuffer) || ((TRUE == isReferenceEnabled) && (NULL == pReferenceImage.pBuffer)))
            {
                result = CamxResultEFailed;
            }

            if (CamxResultSuccess == result)
            {
                cvpFrameConfigDme  sFrameConfigDme;
                if (FALSE == isReferenceEnabled)
                {
                    sFrameConfigDme.bSkipMVCalc = TRUE;
                }
                else
                {
                    sFrameConfigDme.bSkipMVCalc = FALSE;
                }

                if (NULL != m_pOutputData)
                {
                    sFrameConfigDme.bEnableDescriptorLPF = m_pOutputData->descriptor_Lpf;
                    sFrameConfigDme.nMinFpxThreshold     = m_pOutputData->fpx_threshold;
                    sFrameConfigDme.nDescMatchThreshold  = m_pOutputData->desc_match_threshold;
                    CAMX_LOG_VERBOSE(CamxLogGroupCVP, "Chromatix:  desc_lpf %d fpx_thres %f descMatch %f",
                        m_pOutputData->descriptor_Lpf,
                        m_pOutputData->fpx_threshold,
                        m_pOutputData->desc_match_threshold);
                }

                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                ///                  |  non-registration case   |   Prefilter               |   Blend
                /// ------------------------------------------------------------------------------------------------------------
                /// srcFrameCtx.nFD  |  local buffer[index]     |   local buffer[index]     |  local buffer[end]
                ///                  |                          |                           |  (due to the result of source
                ///                  |                          |                           |   context buffer won't be used
                ///                  |                          |                           |   in blend stage, so we fix it
                ///                  |                          |                           |   to the end of the buffer array)
                /// ------------------------------------------------------------------------------------------------------------
                /// refFrameCtx.nFD  |  local buffer[index-1]   |   no need                 |  local buffer[0]
                ///                  |  (the srcFrameCtx.nFD    |                           |  (Copy from CVPInputPortData
                ///                  |   of previous frame)     |                           |   buffer for the prefilter's
                ///                  |                          |                           |   context output and fixed to the
                ///                  |                          |                           |   begin of local buffer array)
                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

                //  Set the FD of frame context for different case
                if (TRUE == IsProfileIdRegistration())
                {
                    if (TRUE == IsCVPInPrefilter())
                    {
                        srcFrameCtx.nFD            = m_ppSourceContextBuffers[requestIdIndex]->GetCSLBufferInfo()->fd;
                        m_prefilterSrcContextIndex = requestIdIndex;
                    }
                    else if (TRUE == IsCVPInBlend())
                    {
                        srcFrameCtx.nFD = m_ppSourceContextBuffers[m_requestQueueDepth-1]->GetCSLBufferInfo()->fd;
                        refFrameCtx.nFD = m_ppSourceContextBuffers[0]->GetCSLBufferInfo()->fd;
                    }
                }
                else
                {
                    // non-registration case
                    srcFrameCtx.nFD = m_ppSourceContextBuffers[requestIdIndex]->GetCSLBufferInfo()->fd;
                    refFrameCtx.nFD = m_ppSourceContextBuffers[(requestId - 1) % m_requestQueueDepth]->GetCSLBufferInfo()->fd;
                }

                srcFrameCtx.nSize = m_outMemReq.nRefFrameContextBytes;
                refFrameCtx.nSize = m_outMemReq.nRefFrameContextBytes;

                // Need to make enums
                if (TRUE == m_bICAEnabled)
                {
                    // pFullResImage
                    fenceInfo[FullResImage].nSyncParam   = m_phSynxSrcImage[requestIdIndex];
                    fenceInfo[FullResImage].nSecureKey   = synxSrcImageKey;

                    // pScaledSrcImage
                    fenceInfo[ScaledSrcImage].nSyncParam = m_phSynxIcaOutput[requestIdIndex];
                    fenceInfo[ScaledSrcImage].nSecureKey = synxIcaOutputKey;
                }
                else
                {
                    // pFullResImage
                    fenceInfo[FullResImage].nSyncParam   = 0;
                    fenceInfo[FullResImage].nSecureKey   = 0;

                    // pScaledSrcImage
                    fenceInfo[ScaledSrcImage].nSyncParam = m_phSynxSrcImage[requestIdIndex];
                    fenceInfo[ScaledSrcImage].nSecureKey = synxSrcImageKey;
                }

                fenceInfo[ScaledSrcFrameCtxt].nSyncParam = 0;
                fenceInfo[ScaledSrcFrameCtxt].nSecureKey = 0;
                fenceInfo[PespectiveBuffer].nSyncParam   = 0;
                fenceInfo[PespectiveBuffer].nSecureKey   = 0;
                fenceInfo[GridBuffer].nSyncParam         = 0;
                fenceInfo[GridBuffer].nSecureKey         = 0;
                 // Ref already signalled so not needed
                fenceInfo[ScaledRefImage].nSyncParam     = 0; // (TRUE == isReferenceEnabled) ? hSynxRefImage :0;
                fenceInfo[ScaledRefImage].nSecureKey     = 0; // (TRUE == isReferenceEnabled) ? synxRefImageKey :0;
                fenceInfo[ScaledRefFrameCtxt].nSyncParam = 0;
                fenceInfo[ScaledRefFrameCtxt].nSecureKey = 0;
                fenceInfo[DMEOutput].nSyncParam          = m_phSynxDmeOutput[requestIdIndex];
                fenceInfo[DMEOutput ].nSecureKey         = synxDmeOutputKey;

                CAMX_LOG_INFO(CamxLogGroupCVP, "%s Async call: req=%lld, input:fd %d, ctxt %d, ref: fd %d, ctxt %d, isRef=%u",
                              NodeIdentifierString(),
                              requestId,
                              pInputImage.pBuffer->nFD,
                              srcFrameCtx.nFD,
                              (TRUE == isReferenceEnabled)? pReferenceImage.pBuffer->nFD: NULL,
                              refFrameCtx.nFD,
                              isReferenceEnabled);

                if ((TRUE == IsProfileIdRegistration()))
                {
                    CAMX_TRACE_ASYNC_BEGIN_F(CamxLogGroupCore, requestId, "ChiFeature2HWmultiframe::ProcessCVP");
                }

                status = cvpDme_Async(m_hCVPDME_Handle,
                                      &sFrameConfigDme,
                                      (TRUE == m_bICAEnabled)? &pInputImage : NULL,
                                      (TRUE == m_bICAEnabled)? &pICAOutputImage: &pInputImage,
                                      &srcFrameCtx,
                                      (TRUE == m_bICAEnabled)? &gridBuffer : NULL,
                                      (TRUE == m_bICAEnabled)? &gridBuffer : NULL,
                                      (TRUE == isReferenceEnabled) ? &pReferenceImage :
                                                                    ((TRUE == m_bICAEnabled)? &pICAOutputImage : &pInputImage),
                                      (TRUE == isReferenceEnabled) ? &refFrameCtx : &srcFrameCtx,
                                      &dmeOutput,
                                      &fenceInfo[0],
                                      8,
                                      NULL);

                if (status != CVP_SUCCESS)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCVP, "Async call for req %lld failed!!, status=%u", requestId, status);
                    result = CamxResultEFailed;
                }
            }
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCVP, "Submit req %lld failed!!", requestId);
            }
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupCVP, "%s:Exit Req %llu, seq %d", NodeIdentifierString(), requestId,
                    pNodeRequestData->processSequenceId);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::IsNodeDisabledWithOverride
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CVPNode::IsNodeDisabledWithOverride()
{
    BOOL result = FALSE;

    if ((FALSE == GetHwContext()->GetStaticSettings()->enableMCTF) &&
        (TRUE == IsCVPInMCTF()))
    {
        CAMX_LOG_ERROR(CamxLogGroupCVP, "MCTF CVP Disabled from settings");
        result = TRUE;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CVPNode::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::CreateCVPIQModules
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::CreateCVPIQModules()
{
    CamxResult              result                      = CamxResultSuccess;
    IPEIQModuleInfo*        pIQModule                   = IQModulesList;
    UINT                    indicesLengthRequired       = 0;
    INT32                   deviceIndex                 = -1;

    // Add device indices
    result = HwEnvironment::GetInstance()->GetDeviceIndices(CSLDeviceTypeICP, &deviceIndex, 1, &indicesLengthRequired);
    if (CamxResultSuccess == result)
    {
        result = AddDeviceIndex(deviceIndex);
        m_deviceIndex = deviceIndex;
    }

    IPEModuleCreateData moduleInputData = { 0 };
    moduleInputData.initializationData.pipelineIPEData.pDeviceIndex = &m_deviceIndex;
    moduleInputData.initializationData.pHwContext                   = GetHwContext();
    moduleInputData.initializationData.requestQueueDepth            = m_requestQueueDepth;
    moduleInputData.pNodeIdentifier                                 = NodeIdentifierString();

    result = CreateICAModule(&moduleInputData, pIQModule);
    CAMX_LOG_VERBOSE(CamxLogGroupCVP, "CreateICAModule result %d", result);

    if ((CamxResultSuccess == result) && (0 < m_numCVPIQModulesEnabled))
    {
        result = CreateIQModulesCmdBufferManager(&moduleInputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::CreateIQModulesCmdBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::CreateIQModulesCmdBufferManager(
    IPEModuleCreateData*     pModuleInputData)
{
    CamxResult              result                   = CamxResultSuccess;
    CmdBufferManagerParam*  pBufferManagerParam      = NULL;
    IQModuleCmdBufferParam  bufferManagerCreateParam = { 0 };
    CAMX_LOG_VERBOSE(CamxLogGroupCVP, "m_numCVPIQModulesEnabled %d", m_numCVPIQModulesEnabled);

    pBufferManagerParam =
        static_cast<CmdBufferManagerParam*>(CAMX_CALLOC(sizeof(CmdBufferManagerParam) * (m_numCVPIQModulesEnabled + 2)));

    if (NULL != pBufferManagerParam)
    {
        bufferManagerCreateParam.pCmdBufManagerParam    = pBufferManagerParam;
        bufferManagerCreateParam.numberOfCmdBufManagers = 0;

        for (UINT count = 0; count < m_numCVPIQModulesEnabled; count++)
        {
            pModuleInputData->pModule = m_pEnabledCVPIQModule[count];
            pModuleInputData->pModule->FillCmdBufferManagerParams(&pModuleInputData->initializationData,
                &bufferManagerCreateParam);
        }
        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "bufferManagerCreateParam.numberOfCmdBufManagers %d",
            bufferManagerCreateParam.numberOfCmdBufManagers);

        if (0 != bufferManagerCreateParam.numberOfCmdBufManagers)
        {
            // Create Cmd Buffer Managers for IQ Modules
            result = CmdBufferManager::CreateMultiManager(FALSE,
                                                          pBufferManagerParam,
                                                          bufferManagerCreateParam.numberOfCmdBufManagers);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCVP, "IQ Modules Cmd Buffer Manager Creation failed");
            }
        }

        // Free up the memory allocated bt IQ Blocks
        for (UINT index = 0; index < bufferManagerCreateParam.numberOfCmdBufManagers; index++)
        {
            if (NULL != pBufferManagerParam[index].pBufferManagerName)
            {
                CAMX_DELETE pBufferManagerParam[index].pBufferManagerName;
                pBufferManagerParam[index].pBufferManagerName = NULL;
            }

            if (NULL != pBufferManagerParam[index].pParams)
            {
                CAMX_DELETE pBufferManagerParam[index].pParams;
                pBufferManagerParam[index].pParams = NULL;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCVP, "Out Of Memory");
        result = CamxResultENoMemory;
    }

    if (NULL != pBufferManagerParam)
    {
        CAMX_FREE(pBufferManagerParam);
        pBufferManagerParam = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::ProgramIQConfig()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::ProgramIQConfig(
    ISPInputData* pInputData)
{
    CamxResult      result        = CamxResultSuccess;
    UINT            count         = 0;
    UINT            path          = 0;
    UINT            index         = 0;

    IPEIQModuleData moduleData;

    // Call IQInterface to Set up the Trigger data
    Node* pBaseNode = this;
    IQInterface::IQSetupTriggerData(pInputData, pBaseNode, IsRealTime(), NULL);
    result = IQInterface::CVP10CalculateSetting(&m_dependenceData, pInputData, m_pOutputData);

    if (CamxResultSuccess == result)
    {
        if (TRUE == m_dumpCVPICAInputConfig)
        {
            DumpCVPICAInputConfig(pInputData);
        }

        for (count = 0; count < m_numCVPIQModulesEnabled; count++)
        {
            result = m_pEnabledCVPIQModule[count]->Execute(pInputData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCVP, "%s: Failed to Run IQ Config, count %d", NodeIdentifierString(), count);
                break;
            }

            m_pEnabledCVPIQModule[count]->GetModuleData(&moduleData);

            switch (m_pEnabledCVPIQModule[count]->GetIQType())
            {
                case ISPIQModuleType::IPEICA:
                    path = moduleData.IPEPath;
                    if (IPEPath::CVPICA == path)
                    {
                        index = ProgramIndexICA1;
                    }
                    else
                    {
                        result = CamxResultEInvalidArg;
                        CAMX_LOG_ERROR(CamxLogGroupCVP, "%s: unsupported ipe path %d", NodeIdentifierString(), path);
                    }
                    break;
                default:
                    break;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCVP, "%s: CVP10CalculateSetting failed", NodeIdentifierString());
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cvpDmesessionEvent_Callback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cvpDmesessionEvent_Callback(
    cvpSession hSession,
    cvpEvent   eEvent,
    VOID*      pSessionUserData)
{
    CAMX_UNREFERENCED_PARAM(hSession);

    CAMX_LOG_INFO(CamxLogGroupCVP, "Session event callback");
    if (CVP_EVFATAL == eEvent)
    {
        CAMX_LOG_ERROR(CamxLogGroupCVP, "FATAL Event! Trigger recovery!");
        CVPDmeCbPrivData* pCVPDmeCbPrivData = static_cast<CVPDmeCbPrivData*>(pSessionUserData);
        if (NULL != pCVPDmeCbPrivData)
        {
            if ((0x33333333 == pCVPDmeCbPrivData->dmeSessionUserData) &&
                (NULL != pCVPDmeCbPrivData->pNode))
            {
                (static_cast<CVPNode*>(pCVPDmeCbPrivData->pNode))->TriggerRecoveryOnError();
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCVP, "Invalid cvp dme cb private data, node %pK", pCVPDmeCbPrivData->pNode);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCVP, "Invalid cvp dme cb private data ptr");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVPNode::CreateCVPSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
cvpSession CVPNode::CreateCVPSession()
{
    cvpSession hCVPSessionHandle;
    cvpEventHandlerCb   cvpDme_Cb = cvpDmesessionEvent_Callback;

    m_cvpDmeCbPrivData.dmeSessionUserData = 0x33333333;
    m_cvpDmeCbPrivData.pNode              = static_cast<VOID*>(this);

    cvpConfigSession cvpSessionConfig;
    cvpSessionConfig.eType = static_cast<cvpSessionType>(CVPSessionTypeDME);

    // CVP session priority
    if (IPEStabilizationTypeEIS3 == (IPEStabilizationTypeEIS3 & m_instanceProperty.stabilizationType))
    {
        cvpSessionConfig.nPriority = static_cast<cvpSessionPriority>(CVP_SESSION_PRIORITY_DME_VIDEO_PATH);
    }
    else if (TRUE == IsProfileIdRegistration())
    {
        cvpSessionConfig.nPriority = static_cast<cvpSessionPriority>(CVP_SESSION_PRIORITY_MF_REG_PATH);
    }
    else
    {
        cvpSessionConfig.nPriority = static_cast<cvpSessionPriority>(CVP_SESSION_PRIORITY_DME_PREVIEW_PATH);
    }

    hCVPSessionHandle = cvpCreateSession(cvpDme_Cb, static_cast<VOID*>(&m_cvpDmeCbPrivData), &cvpSessionConfig);

    return hCVPSessionHandle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVPNode::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::PostPipelineCreate()
{
    CamxResult  result = CamxResultSuccess;
    cvpStatus   status = CVP_EFAIL;
    ImageFormat format;
    Utils::Memset(&format, 0, sizeof(ImageFormat));

    GetStabilizationMargins();

    m_phSynxSrcImage          = static_cast<synx_handle_t*>(CAMX_CALLOC(sizeof(synx_handle_t) * m_requestQueueDepth));
    m_phSynxDmeOutput         = static_cast<synx_handle_t*>(CAMX_CALLOC(sizeof(synx_handle_t) * m_requestQueueDepth));
    m_phSynxIcaOutput         = static_cast<synx_handle_t*>(CAMX_CALLOC(sizeof(synx_handle_t) * m_requestQueueDepth));
    m_pStillFrameConfig       = static_cast<GeoLibStillFrameConfig*>(CAMX_CALLOC(sizeof(GeoLibStillFrameConfig) *
                                    m_requestQueueDepth));
    m_pForceIdentityTransform = static_cast<UINT8*>(CAMX_CALLOC(sizeof(UINT8) * m_requestQueueDepth));

    if ((NULL == m_phSynxSrcImage)    ||
        (NULL == m_phSynxDmeOutput)   ||
        (NULL == m_phSynxIcaOutput)   ||
        (NULL == m_pStillFrameConfig) ||
        (NULL == m_pForceIdentityTransform))
    {
        result = CamxResultENoMemory;
    }

    if (CamxResultSuccess == result)
    {
        for (UINT i = 0; i < m_requestQueueDepth; i++)
        {
            m_pForceIdentityTransform[i] = (TRUE == m_disableCVPDriver) ? 1 : 0;
        }
    }

    if ((CamxResultSuccess == result) && (FALSE == m_disableCVPDriver))
    {
        IQInterface::IQSetHardwareVersion(static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion(),
                                          static_cast<Titan17xContext *>(GetHwContext())->GetHwVersion());

        m_tuningData.noOfSelectionParameter   = 1;
        m_tuningData.TuningMode[0].mode       = ChiModeType::Default;

        result = LoadCVPChromatix();

        if ((TRUE == IsCVPWithICAProfile()) ||
            (TRUE == IsProfileIdRegistration()))
        {
            m_bICAEnabled = TRUE;
        }

        if ((CamxResultSuccess == result) && (TRUE == m_bICAEnabled))
        {
            result = CreateCVPIQModules();
            CAMX_LOG_INFO(CamxLogGroupCVP, "CreateCVPIQModules result %d!", result);
        }

        if (CamxResultSuccess == result)
        {
            m_hCVPSessionHandle = CreateCVPSession();

            if (m_hCVPSessionHandle == NULL)
            {
                CAMX_LOG_ERROR(CamxLogGroupCVP, "CreateSession Failed : m_hCVPSessionHandle is NULL");
                result = CamxResultEFailed;
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCVP, "CreateSessionDone");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCVP, "CreateCVPIQModules Failed!");
        }

        if (CamxResultSuccess == result)
        {

            SetCVPDimensions();

            cvpConfigDme pConfig;
            pConfig.bDSEnable                           = FALSE;
            pConfig.bEnableInlierTracking               = m_pReserveData->inlier_track_enable;
            pConfig.nMinAllowedTarVar                   = m_pRgnDataType->robustness_min_allowed_tar_var;
            pConfig.nMeaningfulNCCDiff                  = m_pRgnDataType->robustness_meaningful_ncc_diff;
            pConfig.sHcdConfigParam.nNumHorizZones      = 15;
            pConfig.sHcdConfigParam.nNumVertZones       = 23;
            pConfig.sHcdConfigParam.nZoneWidth          = 4;
            pConfig.sHcdConfigParam.nZoneHeight         = 2;
            pConfig.sHcdConfigParam.nCmShift            = 12;
            pConfig.sHcdConfigParam.nMode               = 2;
            pConfig.sHcdConfigParam.nNMSTap             = 1;

            if (2 == m_pReserveData->transform_model)
            {
                pConfig.sRansacConfigParam.nRansacModel = 1;
            }
            else if (1 == m_pReserveData->transform_model)
            {
                pConfig.sRansacConfigParam.nRansacModel = 2;
            }
            else
            {
                pConfig.sRansacConfigParam.nRansacModel = m_pReserveData->transform_model;
            }

            pConfig.sRansacConfigParam.nRansacThreshold = 0.5F;

            pConfig.bFtextEnable                        = (TRUE == IsProfileIdRegistration())? FALSE: TRUE;
            m_fTexture                                  = pConfig.bFtextEnable;
            pConfig.nScaledSrcWidth                     = m_inputWidth;
            pConfig.nScaledSrcHeight                    = m_inputHeight;

            UINT8 robustnessMesureDistMapCount =
                CAMX_ARRAY_SIZE(m_pRgnDataType->robustness_measure_dist_map_tab.robustness_measure_dist_map);

            for (UINT8 i = 0; i < robustnessMesureDistMapCount; i++)
            {
                pConfig.nRobustnessDistMap[i] =
                    m_pRgnDataType->robustness_measure_dist_map_tab.robustness_measure_dist_map[i];
            }

            CAMX_LOG_INFO(CamxLogGroupCVP, "RModel %u inlierTrack %u tarVar %f ncc diff %f, ftext %d",
                          m_pReserveData->transform_model,
                          m_pReserveData->inlier_track_enable,
                          m_pRgnDataType->robustness_min_allowed_tar_var,
                          m_pRgnDataType->robustness_meaningful_ncc_diff,
                          pConfig.bFtextEnable);


            if (TRUE == m_bICAEnabled)
            {
                pConfig.bICAEnable          = TRUE;
                cvpDmeConfigIca* pICAConfig = &pConfig.sIcaConfigParam;
                // Fill out registers from Geolib call. Hardcode for now

                if (TRUE == IsProfileIdRegistration())
                {
                    // needs to be linear DS input format for cvp registration
                    m_inputTARDS4Format = CVP_COLORFORMAT_NV12;
                }
                else
                {
                    //  Default type: PD10
                    m_inputTARDS4Format = CVP_COLORFORMAT_PD10;
                }

                m_inputREFDS4Format = CVP_COLORFORMAT_NV12_UBWC_4R;

                status = cvpQueryImageInfo_dme(m_inputTARDS4Format,
                                               m_inputWidth,
                                               m_inputHeight,
                                               &pICAConfig->sSrcImageInfo,
                                               m_bICAEnabled);

                status = cvpQueryImageInfo_dme(m_inputREFDS4Format,
                                               m_ICAOutWidth,
                                               m_ICAOutHeight,
                                               &pICAConfig->sDstImageInfo,
                                               m_bICAEnabled);

                pICAConfig->bCtcPerspectiveEnable = IsCtcPerspectiveEnabled();
                pICAConfig->bCtcGridEnable        = IsCtcGridEnabled();
                pICAConfig->bCtcTranslationOnly   = 0;

                CAMX_LOG_INFO(CamxLogGroupCVP, "Stabilization type %d ", m_instanceProperty.stabilizationType);
                CAMX_LOG_INFO(CamxLogGroupCVP, "src fmt %d %dX%d nplane %d size %d str[0] %d size[0] %d, str[1] %d size[1] %d",
                              pICAConfig->sSrcImageInfo.eFormat,
                              pICAConfig->sSrcImageInfo.nWidth,
                              pICAConfig->sSrcImageInfo.nHeight,
                              pICAConfig->sSrcImageInfo.nPlane,
                              pICAConfig->sSrcImageInfo.nTotalSize,
                              pICAConfig->sSrcImageInfo.nWidthStride[0],
                              pICAConfig->sSrcImageInfo.nAlignedSize[0],
                              pICAConfig->sSrcImageInfo.nWidthStride[1],
                              pICAConfig->sSrcImageInfo.nAlignedSize[1]);

                CAMX_LOG_INFO(CamxLogGroupCVP, "Dest fmt %d %dX%d nplane %d size %d str[0] %d size[0] %d, str[1] %d size[1] %d",
                              pICAConfig->sDstImageInfo.eFormat,
                              pICAConfig->sDstImageInfo.nWidth,
                              pICAConfig->sDstImageInfo.nHeight,
                              pICAConfig->sDstImageInfo.nPlane,
                              pICAConfig->sDstImageInfo.nTotalSize,
                              pICAConfig->sDstImageInfo.nWidthStride[0],
                              pICAConfig->sDstImageInfo.nAlignedSize[0],
                              pICAConfig->sDstImageInfo.nWidthStride[1],
                              pICAConfig->sDstImageInfo.nAlignedSize[1]);

                CAMX_LOG_INFO(CamxLogGroupCVP, "%s, bCtcPerspectiveEnable %d bCtcGridEnable %d bCtcTranslationOnly %d",
                              NodeIdentifierString(),
                              pICAConfig->bCtcPerspectiveEnable,
                              pICAConfig->bCtcGridEnable,
                              pICAConfig->bCtcTranslationOnly );
            }


            status = cvpQueryImageInfo_dme(m_inputTARDS4Format,
                                           m_fullInputWidth,
                                           m_fullInputHeight,
                                           &pConfig.sFullResImageInfo,
                                           m_bICAEnabled);

            CAMX_LOG_VERBOSE(CamxLogGroupCVP, "fmt %d %dX%d nplane %d size %d str[0] %d size[0] %d, str[1] %d size[1] %d",
                             pConfig.sFullResImageInfo.eFormat,
                             pConfig.sFullResImageInfo.nWidth,
                             pConfig.sFullResImageInfo.nHeight,
                             pConfig.sFullResImageInfo.nPlane,
                             pConfig.sFullResImageInfo.nTotalSize,
                             pConfig.sFullResImageInfo.nWidthStride[0],
                             pConfig.sFullResImageInfo.nAlignedSize[0],
                             pConfig.sFullResImageInfo.nWidthStride[1],
                             pConfig.sFullResImageInfo.nAlignedSize[1]);

            // Init Call
            CvpDmeCb dmeCb   = Dme_CallBack;
            m_hCVPDME_Handle = cvpInitDme(m_hCVPSessionHandle, &pConfig, &m_outMemReq, dmeCb, static_cast<VOID*>(this));
            if (m_hCVPDME_Handle == NULL)
            {
                CAMX_LOG_ERROR(CamxLogGroupCVP, "cvpInitDme Failed ,returns NULL");
                result = CamxResultEFailed;
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCVP, "cvpInitDme Successful! Output size = %llu", m_outMemReq.nDMEOutputBytes);
            }

            if (CamxResultSuccess == result)
            {
                // CVP Start Session
                status = cvpStartSession(m_hCVPSessionHandle);
                if (status != CVP_SUCCESS)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCVP, "cvpStartSession Failed : Status: %d ....", status);
                    result = CamxResultEFailed;
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            // Create buffer manager to access source context buffers for CVP
            format.format = Format::Blob;
            // Update with information from CVP API
            format.width  = m_outMemReq.nRefFrameContextBytes;
            format.height = 1;

            CAMX_LOG_VERBOSE(CamxLogGroupCVP, "Source buffer manager wXh %dx%d", format.width, format.height);
            result = CreateBufferManager(&m_pSourceContextBufferManager, format);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCVP, "Source buffer manager and buffer allocation Failed!");
            }
            else
            {
                m_ppSourceContextBuffers = static_cast<ImageBuffer**>(CAMX_CALLOC(sizeof(ImageBuffer*) * m_requestQueueDepth));
                if (NULL != m_ppSourceContextBuffers)
                {
                    result = AllocateBuffers(m_pSourceContextBufferManager, m_ppSourceContextBuffers);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCVP, "Source Buffer allocateBuffers Failed!");
                        m_pSourceContextBufferManager->Deactivate(FALSE);
                        m_pSourceContextBufferManager->Destroy();
                        m_pSourceContextBufferManager = NULL;

                        CAMX_FREE(m_ppSourceContextBuffers);
                        m_ppSourceContextBuffers = NULL;

                    }
                }
                else
                {
                    m_pSourceContextBufferManager->Deactivate(FALSE);
                    m_pSourceContextBufferManager->Destroy();
                    m_pSourceContextBufferManager = NULL;

                    result = CamxResultENoMemory;
                    CAMX_LOG_ERROR(CamxLogGroupCVP, "Source context buffer calloc Failed!");
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            // Create buffer manager to access output buffers for CVP
            format.format = Format::Blob;
            format.width  = sizeof(cvpDmeFrameConfigIca);
            format.height = 1;

            CAMX_LOG_INFO(CamxLogGroupCVP, "Grid buffer manager wXh %dx%d", format.width, format.height);
            result = CreateBufferManager(&m_pGridBufferManager, format);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCVP, "Grid buffer manager and buffer allocation Failed!");
            }
            else
            {
                m_ppGridBuffers = static_cast<ImageBuffer**>(CAMX_CALLOC(sizeof(ImageBuffer*) * m_requestQueueDepth));
                if (NULL != m_ppGridBuffers)
                {
                    result = AllocateBuffers(m_pGridBufferManager, m_ppGridBuffers);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCVP, "Grid buffer allocateBuffers Failed!");
                        m_pGridBufferManager->Deactivate(FALSE);
                        m_pGridBufferManager->Destroy();
                        m_pGridBufferManager = NULL;

                        CAMX_FREE(m_ppGridBuffers);
                        m_ppGridBuffers = NULL;
                    }
                }
                else
                {
                    m_pGridBufferManager->Deactivate(FALSE);
                    m_pGridBufferManager->Destroy();
                    m_pGridBufferManager = NULL;

                    result = CamxResultENoMemory;
                    CAMX_LOG_ERROR(CamxLogGroupCVP, "Grid buffer calloc Failed!");
                }
            }
        }

        GetFPSAndBatchSize();
        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "PostPipelineCreate X result %d", result);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCVP, "PostPipelineCreate Failed! Cleaning up");
            Cleanup();
        }

        m_dumpCVPICAInputConfig = GetStaticSettings()->dumpCVPICAInputConfig;
        m_dumpGeolibResult      = GetStaticSettings()->dumpGeolibResult;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::CleanupSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::CleanupSession()
{
    CamxResult result = CamxResultSuccess;

    if (NULL != m_hCVPSessionHandle)
    {
        cvpStatus status = cvpStopSession(m_hCVPSessionHandle);
        if (status != CVP_SUCCESS)
        {
            CAMX_LOG_ERROR(CamxLogGroupCVP, "cvpStopSession Failed : Status: %d", status);
            result = CamxResultEFailed;
        }

        if (CamxResultSuccess == result)
        {
            // DME De-Initialization
            status = cvpDeInitDme(m_hCVPDME_Handle);

            if (status != CVP_SUCCESS)
            {
                CAMX_LOG_ERROR(CamxLogGroupCVP, "cvpDeInitDme Failed : Status: %d", status);
                result = CamxResultEFailed;
            }
        }

        if (CamxResultSuccess == result)
        {

            // Delete the CVP Session
            status = cvpDeleteSession(m_hCVPSessionHandle);

            if (status != CVP_SUCCESS)
            {
                CAMX_LOG_ERROR(CamxLogGroupCVP, "cvpDeleteSession Failed : Status: %d", status);
                result = CamxResultEFailed;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::QueryMetadataPublishList(
    NodeMetadataList* pPublistTagList)
{
    CamxResult result   = CamxResultSuccess;
    UINT32     tagIndex = ICAReferenceParamsIndex;
    UINT32     count    = 0;
    UINT32     tagID;

    if (IPEStabilizationTypeEIS3 ==
        (IPEStabilizationTypeEIS3 & m_instanceProperty.stabilizationType))
    {
        tagIndex = ICAReferenceParamsLookAheadIndex;
    }
    else if (TRUE == IsProfileIdRegistration())
    {
        tagIndex = ICAInPerspectiveTransformIndex;
    }

    result = VendorTagManager::QueryVendorTagLocation(
            g_CVPOutputVendorTags[tagIndex].pSectionName,
            g_CVPOutputVendorTags[tagIndex].pTagName,
            &tagID);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta, "Cannot query vendor tag %s %s",
                       g_CVPOutputVendorTags[tagIndex].pSectionName,
                       g_CVPOutputVendorTags[tagIndex].pTagName);
    }
    else
    {
        pPublistTagList->tagArray[count] = tagID;
        pPublistTagList->tagCount        = ++count;
        CAMX_LOG_VERBOSE(CamxLogGroupMeta, "%d tag will be published", count);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::PostICATransform
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::PostICATransform(
    VOID* pTrans,
    UINT32 confidence,
    UINT32 width,
    UINT32 height,
    UINT64 requestId)
{
    CamxResult result = CamxResultSuccess;
    IPEICAPerspectiveTransform  perspectiveTransform;
    CPerspectiveTransform* pTransform = reinterpret_cast<CPerspectiveTransform *>(pTrans);

    Utils::Memcpy(perspectiveTransform.perspectiveTransformArray, &pTransform->m, 3 * 3 * sizeof(FLOAT));
    perspectiveTransform.perspectiveGeometryNumRows       = 1;
    perspectiveTransform.perspetiveGeometryNumColumns     = 1;
    perspectiveTransform.byPassAlignmentMatrixAdjustement =
        (1 == m_pForceIdentityTransform[requestId % m_requestQueueDepth]) ? TRUE : FALSE;
    perspectiveTransform.perspectiveConfidence            = confidence;
    perspectiveTransform.transformDefinedOnHeight         = height;
    perspectiveTransform.transformDefinedOnWidth          = width;
    perspectiveTransform.ReusePerspectiveTransform        = 0;
    perspectiveTransform.perspectiveTransformEnable       = 1;

    const VOID*  pPData[1]         = { &perspectiveTransform };
    UINT         pDataSize[1]      = { sizeof(IPEICAPerspectiveTransform) };
    UINT         CVPICATAGLocation = 0;

    if (IPEStabilizationTypeEIS3 ==
        (IPEStabilizationTypeEIS3 & m_instanceProperty.stabilizationType))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "CVP:%d posted ICA : refrence params lookahead tag id req %llu",
                         InstanceID(), requestId);
        result = VendorTagManager::QueryVendorTagLocation(g_CVPOutputVendorTags[1].pSectionName,
                                                          g_CVPOutputVendorTags[1].pTagName,
                                                          &CVPICATAGLocation);
    }
    else if (TRUE == IsProfileIdRegistration())
    {
        //  Registration for MFNR / MFSR
        result = VendorTagManager::QueryVendorTagLocation(g_CVPOutputVendorTags[2].pSectionName,
                                                          g_CVPOutputVendorTags[2].pTagName,
                                                          &CVPICATAGLocation);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "CVP:%d posted ICA : refrence params realtime tag id req %llu",
                       InstanceID(), requestId);
        result = VendorTagManager::QueryVendorTagLocation(g_CVPOutputVendorTags[0].pSectionName,
                                                          g_CVPOutputVendorTags[0].pTagName,
                                                          &CVPICATAGLocation);
    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupCVP, "Failed to get ICA vendor tag location");
    }
    else
    {
        result = WriteDataList(&CVPICATAGLocation, pPData, pDataSize, 1);

        if (CamxResultSuccess == result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCVP, "%s: posted ICA : Confidence %d res %dX%d for req %llu",
                            NodeIdentifierString(), confidence, height, width, requestId);
            CAMX_LOG_VERBOSE(CamxLogGroupCVP, "%s: [%lf %lf %lf] [%lf %lf %lf] [%lf %lf %lf] persp enable %d",
                            NodeIdentifierString(),
                            perspectiveTransform.perspectiveTransformArray[0],
                            perspectiveTransform.perspectiveTransformArray[1],
                            perspectiveTransform.perspectiveTransformArray[2],
                            perspectiveTransform.perspectiveTransformArray[3],
                            perspectiveTransform.perspectiveTransformArray[4],
                            perspectiveTransform.perspectiveTransformArray[5],
                            perspectiveTransform.perspectiveTransformArray[6],
                            perspectiveTransform.perspectiveTransformArray[7],
                            perspectiveTransform.perspectiveTransformArray[8],
                            perspectiveTransform.perspectiveTransformEnable);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCVP, "Failed to publish ICA transform for %s", NodeIdentifierString());
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::CreateBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::CreateBufferManager(
    ImageBufferManager** ppBufferManager,
    ImageFormat          format)
{
    CamxResult      result          = CamxResultSuccess;
    FormatParamInfo formatParamInfo = { 0 };

    formatParamInfo.yuvPlaneAlign = GetStaticSettings()->yuvPlaneAlignment;

    ImageFormatUtils::InitializeFormatParams(&format, &formatParamInfo);
    BufferManagerCreateData createData      = {};
    createData.bNeedDedicatedBuffers        = TRUE;
    createData.bDisableSelfShrinking        = TRUE;
    createData.deviceIndices[0]             = m_deviceIndex;
    createData.deviceCount                  = 1;
    createData.allocateBufferMemory         = TRUE;
    createData.bEnableLateBinding           = 0;
    createData.maxBufferCount               = m_requestQueueDepth;
    createData.immediateAllocBufferCount    = m_requestQueueDepth;
    createData.bufferManagerType            = BufferManagerType::CamxBufferManager;
    createData.linkProperties.pNode         = this;
    createData.numBatchedFrames             = 1;

    createData.bufferProperties.immediateAllocImageBuffers = m_requestQueueDepth;
    createData.bufferProperties.maxImageBuffers            = m_requestQueueDepth;
    createData.bufferProperties.consumerFlags              = 0;
    createData.bufferProperties.producerFlags              = 0;
    createData.bufferProperties.grallocFormat              = 0;
    createData.bufferProperties.bufferHeap                 = CSLBufferHeapIon;
    createData.bufferProperties.memFlags                   = CSLMemFlagHw|CSLMemFlagUMDAccess;
    createData.bufferProperties.imageFormat                = format;

    result = Node::CreateImageBufferManager("CVP_OutputBufferManager", &createData,
                                          ppBufferManager);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupCVP, "Create buffer manager failed");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::AllocateBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::AllocateBuffers(
    ImageBufferManager* pBufferManager,
    ImageBuffer**       ppBuffers)
{
    CamxResult result   = CamxResultSuccess;
    cvpStatus status    = CVP_SUCCESS;

    result =pBufferManager->Activate();
    if (CamxResultSuccess == result)
    {
        for (UINT bufferIndex = 0; bufferIndex < m_requestQueueDepth; bufferIndex++)
        {
            ppBuffers[bufferIndex] =
                pBufferManager->GetImageBuffer();

            if (NULL == ppBuffers[bufferIndex])
            {
                CAMX_LOG_ERROR(CamxLogGroupCVP, "Failure: Buffer not available");
                result = CamxResultENoMemory;
            }
            CAMX_LOG_VERBOSE(CamxLogGroupCVP, "get buffer success bufferindex %d , buffer %pK, vaddr %pK",
                             bufferIndex,
                             ppBuffers[bufferIndex],
                             ppBuffers[bufferIndex]->GetPlaneVirtualAddr(0, 0));
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::DeleteBufferManagers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::DeleteBufferManager(
    ImageBufferManager* pBufferManager,
    ImageBuffer**       ppBuffers)
{
    CamxResult result         = CamxResultSuccess;
    cvpStatus status                 = CVP_SUCCESS;

    for (UINT bufferIndex = 0; bufferIndex < m_requestQueueDepth; bufferIndex++)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "ppBuffers[%d] %pK", bufferIndex, ppBuffers[bufferIndex]);

        CAMX_ASSERT(TRUE == ppBuffers[bufferIndex]->HasBackingBuffer());
        ppBuffers[bufferIndex]->Release(FALSE);
        CAMX_ASSERT(FALSE == ppBuffers[bufferIndex]->HasBackingBuffer());
        ppBuffers[bufferIndex] = NULL;
    }

    if (NULL != pBufferManager)
    {
        pBufferManager->Deactivate(FALSE);
        pBufferManager->Destroy();
        pBufferManager = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// InterpolateICATransform
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID InterpolateICATransform(
    CPerspectiveTransform  transform,
    CPerspectiveTransform* pInterpolatedTransform,
    UINT                   numInnerCVPFrames)
{
    CPerspectiveTransform identityTransform;

    for (UINT i = 0; i < 3 ; i++)
    {
        for (UINT j = 0; j < 3; j++)
        {
            pInterpolatedTransform->m[i][j] =
                ((transform.m[i][j] - identityTransform.m[i][j]) / numInnerCVPFrames) + identityTransform.m[i][j];
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::ProcessingNodeFenceCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CVPNode::ProcessingNodeFenceCallback(
    NodeFenceHandlerData* pFenceHandlerData,
    UINT64 requestId,
    UINT   portId)
{
    CAMX_ASSERT(NULL != pFenceHandlerData);

    CamxResult  result      = CamxResultSuccess;
    OutputPort* pOutputPort = pFenceHandlerData->pOutputPort;

    CAMX_LOG_VERBOSE(CamxLogGroupCVP, "%s, Fence %d signalled for output port %d req %llu, m_disableCVPDriver=%u",
                     NodeIdentifierString(), pFenceHandlerData->hFence, portId, requestId, m_disableCVPDriver);

    CPerspectiveTransform MCTFtransform;

    if (TRUE == m_disableCVPDriver)
    {
        if (pOutputPort->portId == CVPOutputPortData)
        {
            result =
                PostICATransform(&MCTFtransform, MaxTransformConfidence, m_fullInputWidth, m_fullInputHeight, requestId);
        }
    }
    else
    {
        // Could be extract to another function
        CPerspectiveTransform transformUnity;
        CPerspectiveTransform interpolatedTransform;
        UINT32 transformConfidence      = 0;
        cvpDmeOutput*   pDmeOutput      = NULL;
        CSLFenceResult  fenceResult     = pFenceHandlerData->fenceResult;
        FenceHandlerBufferInfo* const outputBufferInfo = &pFenceHandlerData->pOutputBufferInfo[0];
        ImageBuffer*    pBuffer         = outputBufferInfo->pImageBuffer;
        UINT32          requestIdIndex  = static_cast<UINT32>(requestId) % m_requestQueueDepth;

        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "Fence %d signalled for output port %d req %llu",
                         pFenceHandlerData->hFence, portId, requestId);

        if (CSLFenceResultSuccess == fenceResult)
        {
            if (pOutputPort->portId == CVPOutputPortData)
            {
                if (NULL != pBuffer)
                {
                    pDmeOutput = static_cast<cvpDmeOutput*>(pBuffer->GetCSLBufferInfo()->pVirtualAddr);

                    if (NULL != pDmeOutput)
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "%s Req %llu confidence %d [%lf %lf %lf] [%lf %lf %lf] [%lf %lf %lf]",
                                         NodeIdentifierString(), requestId, pDmeOutput->nTransformConfidence,
                                         pDmeOutput->CoarseTransform[0], pDmeOutput->CoarseTransform[1],
                                         pDmeOutput->CoarseTransform[2], pDmeOutput->CoarseTransform[3],
                                         pDmeOutput->CoarseTransform[4], pDmeOutput->CoarseTransform[5],
                                         pDmeOutput->CoarseTransform[6], pDmeOutput->CoarseTransform[7],
                                         pDmeOutput->CoarseTransform[8]);
                        CAMX_LOG_INFO(CamxLogGroupCVP, "%s cvp meta valid %d", NodeIdentifierString(), pDmeOutput->nIsValid);

                        // Need to address truncation issue here from double to float
                        MCTFtransform.m[0][0] = static_cast<FLOAT>(pDmeOutput->CoarseTransform[0]);
                        MCTFtransform.m[0][1] = static_cast<FLOAT>(pDmeOutput->CoarseTransform[1]);
                        MCTFtransform.m[0][2] = static_cast<FLOAT>(pDmeOutput->CoarseTransform[2]);
                        MCTFtransform.m[1][0] = static_cast<FLOAT>(pDmeOutput->CoarseTransform[3]);
                        MCTFtransform.m[1][1] = static_cast<FLOAT>(pDmeOutput->CoarseTransform[4]);
                        MCTFtransform.m[1][2] = static_cast<FLOAT>(pDmeOutput->CoarseTransform[5]);
                        MCTFtransform.m[2][0] = static_cast<FLOAT>(pDmeOutput->CoarseTransform[6]);
                        MCTFtransform.m[2][1] = static_cast<FLOAT>(pDmeOutput->CoarseTransform[7]);
                        MCTFtransform.m[2][2] = static_cast<FLOAT>(pDmeOutput->CoarseTransform[8]);

                        if (TRUE == m_fTexture)
                        {
                            // Motion stats Bit 2, ftexture bit 1, Motion transform Bit 0
                            if (pDmeOutput->nIsValid != 7)
                            {
                                CAMX_LOG_ERROR(CamxLogGroupCVP, "%s Invalid valid flag from FW mctf usecase",
                                               NodeIdentifierString());
                                result = CamxResultEInvalidArg;
                            }
                        }
                        else
                        {
                            // Motion stats Bit 2, Motion transform Bit 0
                            if (pDmeOutput->nIsValid != 5)
                            {
                                CAMX_LOG_ERROR(CamxLogGroupCVP, "%s Invalid valid flag, from FW reg usecase",
                                               NodeIdentifierString());
                                result = CamxResultEInvalidArg;
                            }
                        }

                        if (CamxResultSuccess == result)
                        {
                            if (TRUE == IsProfileIdRegistration())
                            {
                                GeoLibStillFrameConfig* pStillFrameConfig =
                                    &m_pStillFrameConfig[requestId % m_requestQueueDepth];
                                m_scaleFactorW = pStillFrameConfig->coarse2fullScale.x;
                                m_scaleFactorH = pStillFrameConfig->coarse2fullScale.y;


                                m_coarseCenterW = pStillFrameConfig->coarseCenter.x;
                                m_coarseCenterH = pStillFrameConfig->coarseCenter.y;
                                CAMX_LOG_INFO(CamxLogGroupCVP, "%s, bpsout width %d, height %d, cvp ica width %d, height %d",
                                              NodeIdentifierString(),
                                              pStillFrameConfig->bpsOutSizeFull.widthPixels,
                                              pStillFrameConfig->bpsOutSizeFull.heightLines,
                                              pStillFrameConfig->cvpIcaMapping.inputImageSize.widthPixels,
                                              pStillFrameConfig->cvpIcaMapping.inputImageSize.heightLines);
                            }

                            transformConfidence = pDmeOutput->nTransformConfidence;
                            result = ProcessCvpMeResult(m_coarseCenterW,
                                                        m_coarseCenterH,
                                                        m_scaleFactorW,
                                                        m_scaleFactorH,
                                                        MCTFtransform.m,
                                                        &MCTFtransform);
                            if (CamxResultSuccess != result)
                            {
                                CAMX_LOG_ERROR(CamxLogGroupCVP, "%s: ProcessCvpMeResult failed %d",
                                               NodeIdentifierString(), result);
                            }

                            CAMX_LOG_VERBOSE(CamxLogGroupCVP, "%s Req %llu confidence %d [%f %f %f] [%f %f %f] [%f %f %f]",
                                             NodeIdentifierString(), requestId, pDmeOutput->nTransformConfidence,
                                             MCTFtransform.m[0][0], MCTFtransform.m[0][1], MCTFtransform.m[0][2],
                                             MCTFtransform.m[1][0], MCTFtransform.m[1][1], MCTFtransform.m[1][2],
                                             MCTFtransform.m[2][0], MCTFtransform.m[2][1], MCTFtransform.m[2][2]);

                            CAMX_LOG_INFO(CamxLogGroupCVP, "%s: scaleFactrW %f,scaleFactrH %f,coarseCentrW %f,coarseCentrH %f",
                                          NodeIdentifierString(), m_scaleFactorW, m_scaleFactorH,
                                          m_coarseCenterW, m_coarseCenterH);
                        }

                        if (CamxResultSuccess == result)
                        {
                            result = ConfigureCVPConfidenceParameter(&transformConfidence, requestId);
                            if (CamxResultSuccess != result)
                            {
                                CAMX_LOG_ERROR(CamxLogGroupCVP, "%s: ConfigureCVPConfidenceParameter failed %d",
                                    NodeIdentifierString(), result);
                            }
                        }

                        if (CamxResultSuccess == result)
                        {
                            CAMX_LOG_INFO(CamxLogGroupCVP, "Posting for request %llu,CVP confidence %d,"
                                          "transform confidence %d,Identity forced %d",
                                          requestId, pDmeOutput->nTransformConfidence,
                                          transformConfidence, m_pForceIdentityTransform[requestId % m_requestQueueDepth]);
                            // post indentity transform as per m_forceIdentityTransform state for hysteresis implementation

                            // ICA transform is posted with full input width /height although matrix is computed on
                            // DME input where margin is removed on by CVP ICA. this is needed because IPE FW ICA2 mapping
                            // is based on full input. Ideally should be posted on scaled width.
                            if ((1 == m_pForceIdentityTransform[requestId % m_requestQueueDepth]) ||
                            (TRUE == m_unityTransformEnabled))
                            {
                                // post unity transform and confidence
                                transformConfidence = 256;
                                result = PostICATransform(&transformUnity, transformConfidence,
                                    m_fullInputWidth, m_fullInputHeight, requestId);
                            }
                            else
                            {
                                if (m_maxBatchSize > 1)
                                {
                                    InterpolateICATransform(MCTFtransform, &interpolatedTransform, m_maxBatchSize - 1);
                                    PrintMatrix(MCTFtransform, transformConfidence, requestId);
                                    PrintMatrix(interpolatedTransform, transformConfidence, requestId);
                                    // ICA transform is posted with full input width /height although matrix is computed on
                                    // DME input where margin is removed by CVP ICA.this is needed because IPE FW ICA2 mapping
                                    // is based on full input. Ideally should be posted on scaled width.
                                    result = PostICATransform(&interpolatedTransform, transformConfidence,
                                        m_fullInputWidth, m_fullInputHeight, requestId);
                                }
                                else
                                {
                                    // ICA transform is posted with full input width /height although matrix is computed on
                                    // DME input where margin is removed by CVP ICA. this is needed because IPE FW ICA2 mapping
                                    // is based on full input. Ideally should be posted on scaled width.
                                    result = PostICATransform(&MCTFtransform, transformConfidence,
                                        m_fullInputWidth, m_fullInputHeight, requestId);
                                }
                            }

                            if (CamxResultSuccess != result)
                            {
                                CAMX_LOG_ERROR(CamxLogGroupCVP, "%s: PostICATransform failed %d",
                                    NodeIdentifierString(), result);
                            }
                        }

                        if (CamxResultSuccess == result)
                        {
                            UINT32      metaTag = 0;
                            CVPMetaDataInternal cvpMeta = { {0} };
                            Utils::Memcpy(&cvpMeta,
                                pDmeOutput,
                                sizeof(CVPMetaDataInternal));

                            result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.cvpMetaData",
                                "CVPMetaData",
                                &metaTag);

                            if (CamxResultSuccess == result)
                            {
                                static const UINT cvpMetaTag[] = { metaTag };
                                const VOID*       pcvpMetaData[1] = { &cvpMeta };
                                UINT              length = CAMX_ARRAY_SIZE(cvpMetaTag);
                                UINT              pCVPMetaDataCount[] = { sizeof(cvpMeta) };

                                WriteDataList(cvpMetaTag, pcvpMetaData, pCVPMetaDataCount, length);
                            }

                            //  Here we try to copy the DME source context buffer to the CVPOutputPortDatat buffer
                            //  in prefilter stage, whose data is already parsed as ICA transform matrix.
                            if (TRUE == IsCVPInPrefilter())
                            {
                                Utils::Memcpy(pDmeOutput,
                                      m_ppSourceContextBuffers[m_prefilterSrcContextIndex]->GetCSLBufferInfo()->pVirtualAddr,
                                      m_outMemReq.nRefFrameContextBytes);

                                //  Append requestIdIndex as a marker after the DME source context in the output buffer.
                                //  The requestIdIndex will be parsed in blend stage & be used to prevent the redundant memcpy.
                                *(reinterpret_cast<UINT32*>(pDmeOutput) +
                                    (Utils::ByteAlign32(m_outMemReq.nRefFrameContextBytes, 4) >> 2)) = requestIdIndex;
                            }
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCVP, "pDmeOutput is NULL");
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCVP, "pBuffer is NULL");
                }

                ReleaseSynxObject(requestIdIndex);
            }
            else if (pOutputPort->portId == CVPOutputPortImage)
            {
                if ((TRUE == IsProfileIdRegistration()))
                {
                    CAMX_TRACE_ASYNC_END_F(CamxLogGroupCore, requestId, "ChiFeature2HWmultiframe::ProcessCVP");
                }
                CAMX_LOG_INFO(CamxLogGroupCVP, "Image output port signalled for %llu", requestId);
            }
        }
        else
        {
            if (CSLFenceResultCanceled == fenceResult)
            {
                CAMX_LOG_INFO(CamxLogGroupPProc, "%s Submit packets with requestId = %llu failed %d"
                    " due to Ongoing Flush",
                    NodeIdentifierString(),
                    requestId,
                    result);
                // No need for Synx clean up since this is flush for requests in DRQ which have not been
                // associated with Synx objects yet
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "%s Submit packets with requestId = %llu failed %d",
                    NodeIdentifierString(),
                    requestId,
                    result);
                // CVP driver signalled failure for fence. Clean up needed.
                //  Here we only release the synx object when the port is CVPOutputPortData.
                //  CVPOutputPortData will always be signaled after CVPOutputPortImage.
                //  Here we try to avoid releasing all the synx objects when we receive the fence error on port
                // CVPOutputPortImage.
                if (pOutputPort->portId == CVPOutputPortData)
                {
                    ReleaseSynxObject(requestIdIndex);
                }
            }
        }
    }
    if (CamxResultSuccess != result)
    {
        OsUtils::RaiseSignalAbort();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVPNode::ConfigureCVPConfidenceParameter
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::ConfigureCVPConfidenceParameter(
    UINT32* pTransformConfidence,
    UINT64  requestID)
{
    CamxResult result                    = CamxResultSuccess;
    UINT32     ret                       = 0;
    UINT32     enableTransformConfidence = 0;
    INT32      transformConfidenceVal    = 0;

    CVP_Chromatix cvpChromatixData;
    Utils::Memset(&cvpChromatixData, 0, sizeof(CVP_Chromatix));

    // forceIdentity of previous request
    UINT64 prevReqIndex = (requestID != 0) ? ((requestID - 1) % m_requestQueueDepth) : 0;
    UINT8 forceIdentityValue = m_pForceIdentityTransform[prevReqIndex];

    if (NULL != m_pOutputData)
    {
        // CVP chromatix tuning data
        cvpChromatixData.enable_transform_confidence       = m_pOutputData->enable_transform_confidence;
        cvpChromatixData.transform_confidence_mapping_base = m_pOutputData->transform_confidence_mapping_base;
        cvpChromatixData.transform_confidence_mapping_c1   = m_pOutputData->transform_confidence_mapping_c1;
        cvpChromatixData.transform_confidence_mapping_c2   = m_pOutputData->transform_confidence_mapping_c2;

        cvpChromatixData.transform_confidence_thr_to_force_identity_transform =
            m_pOutputData->transform_confidence_thr_to_force_identity_transform;

        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "Chromatix: e_tranconf %f mappingbase %f c1 %f c2 %f threshold %f",
                         m_pOutputData->enable_transform_confidence,
                         m_pOutputData->transform_confidence_mapping_base,
                         m_pOutputData->transform_confidence_mapping_c1,
                         m_pOutputData->transform_confidence_mapping_c2,
                         m_pOutputData->transform_confidence_thr_to_force_identity_transform);
    }
    else
    {
        cvpChromatixData.enable_transform_confidence                          = CVPEnable_transform_confidence;
        cvpChromatixData.transform_confidence_mapping_base                    = CVPTransform_confidence_mapping_base;
        cvpChromatixData.transform_confidence_mapping_c1                      = CVPTransform_confidence_mapping_c1;
        cvpChromatixData.transform_confidence_mapping_c2                      = CVPTransform_confidence_mapping_c2;
        cvpChromatixData.transform_confidence_thr_to_force_identity_transform =
            CVPTransform_confidence_thr_to_force_identity;
    }

    // Calculate TF confidence based on CVP post processing (e.g Ransac) transform confidence and
    // check if identity transform needs to be forced based on confidence hsyterisis implementation.
    ret = ConfigureCvpConfidenceParameter(&cvpChromatixData,
                                          *pTransformConfidence,
                                          &enableTransformConfidence,
                                          &transformConfidenceVal,
                                          &forceIdentityValue);
    result = (NC_LIB_SUCCESS == ret) ? CamxResultSuccess : CamxResultEFailed;
    CAMX_LOG_VERBOSE(CamxLogGroupCVP, "CVP_ConfigureConfidenceParameter : result %d", result);

    if (CamxResultSuccess == result)
    {
        *pTransformConfidence                                      = transformConfidenceVal;
        m_pForceIdentityTransform[requestID % m_requestQueueDepth] = forceIdentityValue;
        CAMX_LOG_INFO(CamxLogGroupCVP, "%s:, request %llu,prevframeforceIdentity %d,currentframeforceIdentity %d,confidence %d",
                      NodeIdentifierString(),
                      requestID,
                      m_pForceIdentityTransform[prevReqIndex],
                      forceIdentityValue,
                      transformConfidenceVal);
    }
    else
    {
        m_pForceIdentityTransform[requestID % m_requestQueueDepth] = 1;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVPNode::GetFPSAndBatchSize()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::GetFPSAndBatchSize()
{
    static const UINT UsecasePropertiesIPE[] = { PropertyIDUsecaseFPS, PropertyIDUsecaseBatch };
    const UINT        length = CAMX_ARRAY_SIZE(UsecasePropertiesIPE);
    VOID*             pData[length] = { 0 };
    UINT64            usecasePropertyDataIPEOffset[length] = { 0 };

    CamxResult result = CamxResultSuccess;

    GetDataList(UsecasePropertiesIPE, pData, usecasePropertyDataIPEOffset, length);

    // This is a soft dependency
    if ((NULL != pData[0]) && (NULL != pData[1]))
    {
        m_FPS = *reinterpret_cast<UINT*>(pData[0]);
        m_maxBatchSize = *reinterpret_cast<UINT*>(pData[1]);
    }
    else
    {
        m_FPS = 30;
        m_maxBatchSize = 1;
    }

    CAMX_LOG_INFO(CamxLogGroupCVP, "%s batch size %d for fps %d", NodeIdentifierString(), m_maxBatchSize, m_FPS);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVPNode::PrintMatrix()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CVPNode::PrintMatrix(
    CPerspectiveTransform   matrix,
    UINT32                  confidence,
    UINT64                  requestId)
{
    CAMX_LOG_VERBOSE(CamxLogGroupCVP, "%s: Req %llu confidence %d [%lf %lf %lf] [%lf %lf %lf] [%lf %lf %lf]",
                         NodeIdentifierString(), requestId, confidence,
                         matrix.m[0][0], matrix.m[0][1], matrix.m[0][2],
                         matrix.m[1][0], matrix.m[1][1], matrix.m[1][2],
                         matrix.m[2][0], matrix.m[2][1], matrix.m[2][2]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVPNode::DumpICAFrameConfig()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CVPNode::DumpICAFrameConfig(
    cvpDmeFrameConfigIca* pIcaFrameCfg)
{
    if (NULL != pIcaFrameCfg)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "Grid %llu %llu %llu %llu",
                         pIcaFrameCfg->nGridData[0],
                         pIcaFrameCfg->nGridData[1],
                         pIcaFrameCfg->nGridData[2],
                         pIcaFrameCfg->nGridData[3]);

        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "nCTCHalfOutputFrameWidth %d nCTCHalfOutputFrameHeight %d",
                         pIcaFrameCfg->nCTCHalfOutputFrameWidth, pIcaFrameCfg->nCTCHalfOutputFrameHeight);

        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "nCTCPerspTransGeomM %d nCTCPerspTransGeomN %d",
                         pIcaFrameCfg->nCTCPerspTransformGeomM, pIcaFrameCfg->nCTCPerspTransformGeomN);

        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "nCtcInputCoordPrecision %d", pIcaFrameCfg->nCtcInputCoordPrecision);

        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "nCtcO2vScaleFactor_x %d nCtcO2vScaleFactor_y %d "
                         "nCtcV2iInvScaleFactor_x %d nCtcV2iInvScaleFactor_y %d",
                         pIcaFrameCfg->nCtcO2vScaleFactor_x,
                         pIcaFrameCfg->nCtcO2vScaleFactor_y,
                         pIcaFrameCfg->nCtcV2iInvScaleFactor_x,
                         pIcaFrameCfg->nCtcV2iInvScaleFactor_y);

        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "nCtcO2vOffset_x %d nCtcO2vOffset_y %d nCtcV2iOffset_x %d nCtcV2iOffset_y %d",
                         pIcaFrameCfg->nCtcO2vOffset_x,
                         pIcaFrameCfg->nCtcO2vOffset_y,
                         pIcaFrameCfg->nCtcV2iOffset_x,
                         pIcaFrameCfg->nCtcV2iOffset_y);

        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "nControllerValidWidthMinus1 %d nControllerValidHeightMinus1 %d"
                         " nCtcXTranslation %d nCtcYTranslation %d",
                          pIcaFrameCfg->nControllerValidWidthMinus1,
                          pIcaFrameCfg->nControllerValidHeightMinus1,
                          pIcaFrameCfg->nCtcXTranslation,
                          pIcaFrameCfg->nCtcYTranslation);

        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "pConfig opg 0 %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
                         pIcaFrameCfg->nOpgInterpLUT0[0],  pIcaFrameCfg->nOpgInterpLUT0[1],
                         pIcaFrameCfg->nOpgInterpLUT0[2],  pIcaFrameCfg->nOpgInterpLUT0[3],
                         pIcaFrameCfg->nOpgInterpLUT0[4],  pIcaFrameCfg->nOpgInterpLUT0[5],
                         pIcaFrameCfg->nOpgInterpLUT0[6],  pIcaFrameCfg->nOpgInterpLUT0[7],
                         pIcaFrameCfg->nOpgInterpLUT0[8],  pIcaFrameCfg->nOpgInterpLUT0[9],
                         pIcaFrameCfg->nOpgInterpLUT0[10], pIcaFrameCfg->nOpgInterpLUT0[11],
                         pIcaFrameCfg->nOpgInterpLUT0[12], pIcaFrameCfg->nOpgInterpLUT0[13],
                         pIcaFrameCfg->nOpgInterpLUT0[14], pIcaFrameCfg->nOpgInterpLUT0[15]);

        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "pConfig opg 1 %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
                         pIcaFrameCfg->nOpgInterpLUT1[0],  pIcaFrameCfg->nOpgInterpLUT1[1],
                         pIcaFrameCfg->nOpgInterpLUT1[2],  pIcaFrameCfg->nOpgInterpLUT1[3],
                         pIcaFrameCfg->nOpgInterpLUT1[4],  pIcaFrameCfg->nOpgInterpLUT1[5],
                         pIcaFrameCfg->nOpgInterpLUT1[6],  pIcaFrameCfg->nOpgInterpLUT1[7],
                         pIcaFrameCfg->nOpgInterpLUT1[8],  pIcaFrameCfg->nOpgInterpLUT1[9],
                         pIcaFrameCfg->nOpgInterpLUT1[10], pIcaFrameCfg->nOpgInterpLUT1[11],
                         pIcaFrameCfg->nOpgInterpLUT1[12], pIcaFrameCfg->nOpgInterpLUT1[13],
                         pIcaFrameCfg->nOpgInterpLUT1[14], pIcaFrameCfg->nOpgInterpLUT1[15]);

        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "pConfig opg 2 %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
                         pIcaFrameCfg->nOpgInterpLUT2[0],  pIcaFrameCfg->nOpgInterpLUT2[1],
                         pIcaFrameCfg->nOpgInterpLUT2[2],  pIcaFrameCfg->nOpgInterpLUT2[3],
                         pIcaFrameCfg->nOpgInterpLUT2[4],  pIcaFrameCfg->nOpgInterpLUT2[5],
                         pIcaFrameCfg->nOpgInterpLUT2[6],  pIcaFrameCfg->nOpgInterpLUT2[7],
                         pIcaFrameCfg->nOpgInterpLUT2[8],  pIcaFrameCfg->nOpgInterpLUT2[9],
                         pIcaFrameCfg->nOpgInterpLUT2[10], pIcaFrameCfg->nOpgInterpLUT2[11],
                         pIcaFrameCfg->nOpgInterpLUT2[12], pIcaFrameCfg->nOpgInterpLUT2[13],
                         pIcaFrameCfg->nOpgInterpLUT2[14], pIcaFrameCfg->nOpgInterpLUT2[15]);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCVP, "Invalid pointers Ica frame config : %pK", pIcaFrameCfg);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVPNode::SetupICAPerspectiveForRegistration()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CVPNode::SetupICAPerspectiveForRegistration(
    IPEICAPerspectiveTransform* pICAInPerspectiveParams)
{
    pICAInPerspectiveParams->perspectiveTransformEnable   = IsCtcPerspectiveEnabled();
    pICAInPerspectiveParams->transformDefinedOnWidth      = m_fullInputWidth;
    pICAInPerspectiveParams->transformDefinedOnHeight     = m_fullInputHeight;
    pICAInPerspectiveParams->perspetiveGeometryNumColumns = 3;
    pICAInPerspectiveParams->perspectiveGeometryNumRows   = 3;

    //  Set the perspectiveTransformArray to default value
    FLOAT* pPerspectiveTransformArray = pICAInPerspectiveParams->perspectiveTransformArray;

    for (UINT i = 0; i < 9; i++)
    {
        UINT32 offset = i * 9;
        pPerspectiveTransformArray[offset + 0] = 1.0f;
        pPerspectiveTransformArray[offset + 1] = 0.0f;
        pPerspectiveTransformArray[offset + 2] = 0.0f;
        pPerspectiveTransformArray[offset + 3] = 0.0f;
        pPerspectiveTransformArray[offset + 4] = 1.0f;
        pPerspectiveTransformArray[offset + 5] = 0.0f;
        pPerspectiveTransformArray[offset + 6] = 0.0f;
        pPerspectiveTransformArray[offset + 7] = 0.0f;
        pPerspectiveTransformArray[offset + 8] = 1.0f;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::DumpCVPICAInputConfig()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CVPNode::DumpCVPICAInputConfig(
    const ISPInputData*   pInputData)
{
    CAMX_LOG_VERBOSE(CamxLogGroupCVP, "%s, Start to dump CVP ICA configuration for requset id = %llu",
                     NodeIdentifierString(), pInputData->frameNum);

    CamxDateTime    time;
    CHAR            curTime[256];
    OsUtils::GetDateTime(&time);
    OsUtils::SNPrintF(curTime, 256, "%02u%02u%02u-%03u",
                      time.hours,
                      time.minutes,
                      time.seconds,
                      time.microseconds/1000);

    CHAR            dumpFilename[256];
    OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename),
                      "%s/cvpIcaInputConfig_%s_%s_req[%llu].txt",
                      CamX::ConfigFileDirectory, curTime, NodeIdentifierString(), pInputData->frameNum);

    FILE*   pFile = OsUtils::FOpen(dumpFilename, "w");

    if (NULL != pFile)
    {
        //  This outputBufferSize value is calculated by the final output data size
        //  So once we change the dump content, we should also change this outputBufferSize to meet the new size.
        UINT32          dataCount        = 0;
        const UINT32    outputBufferSize = 54000;
        CHAR            outputString[outputBufferSize];

        //  Dump for CVP ICA input perspective config
        DumpCVPICAInputPerspectiveParams(outputString,
                                         dataCount,
                                         outputBufferSize,
                                         &pInputData->ICAConfigData.ICAInPerspectiveParams);

        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "%s, Data count for persp = %u",
                         NodeIdentifierString(), dataCount);

        //  Dump for CVP ICA input grid config
        DumpCVPICAInputGridParams(outputString,
                                  dataCount,
                                  outputBufferSize,
                                  &pInputData->ICAConfigData.ICAInGridParams);

        CAMX_LOG_VERBOSE(CamxLogGroupCVP, "%s, Data count for persp + grid = %u",
                         NodeIdentifierString(), dataCount);

        OsUtils::FWrite(outputString, dataCount, 1, pFile);
        OsUtils::FClose(pFile);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCVP, "Can not create CVP ICA input config dump file");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::DumpCVPICAInputPerspectiveParams()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CVPNode::DumpCVPICAInputPerspectiveParams(
    CHAR*                               pOutputString,
    UINT32&                             rDataCount,
    UINT32                              outputBufferSize,
    const IPEICAPerspectiveTransform*   pICAInPerspectiveParams)
{
    rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                    "----------------- CVP ICA Input Perspective Parameters (start) -------------\n");

    rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                    "perspectiveTransformEnable: %u\n",
                                    pICAInPerspectiveParams->perspectiveTransformEnable);

    rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                    "ReusePerspectiveTransform: %u\n",
                                    pICAInPerspectiveParams->ReusePerspectiveTransform);

    rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                    "perspetiveGeometryNumColumns: %u\n",
                                    pICAInPerspectiveParams->perspetiveGeometryNumColumns);

    rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                    "perspectiveGeometryNumRows: %u\n",
                                    static_cast<UINT32>(pICAInPerspectiveParams->perspectiveGeometryNumRows));

    const FLOAT* pPerspectiveTransformArray = pICAInPerspectiveParams->perspectiveTransformArray;
    for (UINT i = 0; i < 9; i++)
    {
        UINT32 offset = i * 9;
        rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                        "perspectiveTransformArray: index=%u,  %f, %f, %f, %f, %f, %f, %f, %f, %f\n",
                                        offset,
                                        pPerspectiveTransformArray[offset + 0],
                                        pPerspectiveTransformArray[offset + 1],
                                        pPerspectiveTransformArray[offset + 2],
                                        pPerspectiveTransformArray[offset + 3],
                                        pPerspectiveTransformArray[offset + 4],
                                        pPerspectiveTransformArray[offset + 5],
                                        pPerspectiveTransformArray[offset + 6],
                                        pPerspectiveTransformArray[offset + 7],
                                        pPerspectiveTransformArray[offset + 8]);
    }

    rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                    "perspectiveConfidence: %u\n",
                                    pICAInPerspectiveParams->perspectiveConfidence);

    rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                    "byPassAlignmentMatrixAdjustement: %u\n",
                                    static_cast<UINT32>(pICAInPerspectiveParams->byPassAlignmentMatrixAdjustement));

    rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                    "transformDefinedOnWidth: %u\n",
                                    pICAInPerspectiveParams->transformDefinedOnWidth);

    rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                    "transformDefinedOnHeight: %u\n",
                                    pICAInPerspectiveParams->transformDefinedOnHeight);

    rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                    "----------------- CVP ICA Input Perspective Parameters (end) -------------\n\n");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::DumpCVPICAInputGridParams()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CVPNode::DumpCVPICAInputGridParams(
    CHAR*                           pOutputString,
    UINT32&                         rDataCount,
    UINT32                          outputBufferSize,
    const IPEICAGridTransform*      pICAInGridParams)
{
    rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                    "----------------- CVP ICA Input Grid Parameters (start) -------------\n");

    rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                    "gridTransformEnable: %u\n",
                                    pICAInGridParams->gridTransformEnable);

    rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                    "reuseGridTransform: %u\n",
                                    pICAInGridParams->reuseGridTransform);

    const ICAGridArray* pGridTransformArray = pICAInGridParams->gridTransformArray;
    //  For CVP ICA 2.0, there are 35 * 27 entries
    UINT32 entriesNums = 35 * 27;
    for (UINT i = 0; i < entriesNums; i++)
    {
        rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                        "gridTransformArray: index=%i,  X:%f, Y:%f\n",
                                        i,
                                        pGridTransformArray[i].x,
                                        pGridTransformArray[i].y);
    }

    rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                    "gridTransformArrayExtrapolatedCorners: %u\n",
                                    pICAInGridParams->gridTransformArrayExtrapolatedCorners);

    for (UINT i = 0; i < 4; i++)
    {
        rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                        "gridTransformArrayCorners: index=%i,  X:%f, Y:%f\n",
                                        i,
                                        pICAInGridParams->gridTransformArrayCorners[i].x,
                                        pICAInGridParams->gridTransformArrayCorners[i].y);
    }

    rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                    "transformDefinedOnWidth: %u\n",
                                    pICAInGridParams->transformDefinedOnWidth);

    rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                    "transformDefinedOnHeight: %u\n",
                                    pICAInGridParams->transformDefinedOnHeight);

    rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                    "geometry: %u\n",
                                    static_cast<UINT32>(pICAInGridParams->geometry));

    rDataCount += OsUtils::SNPrintF(pOutputString + rDataCount, outputBufferSize - rDataCount,
                                    "----------------- CVP ICA Input Grid Parameters (end) -------------\n");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::GetGeoLibICAMapping
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::GetGeoLibICAMapping(
    GeoLibIcaMapping*   pCvpIcaMapping,
    UINT64              requestId)
{
    CamxResult result = CamxResultSuccess;
    UINT32     geoLibTag;

    if (TRUE == IsCVPInPrefilter())
    {
        geoLibTag = m_stillFrameConfigurationTagIdPrefilter;
        CAMX_LOG_INFO(CamxLogGroupCVP, "Retrieve Geolib tag for prefilter: tagId=0x%x", geoLibTag);
    }
    else if (TRUE == IsCVPInBlend())
    {
        geoLibTag = m_stillFrameConfigurationTagIdBlending | InputMetadataSectionMask;
        CAMX_LOG_INFO(CamxLogGroupCVP, "Retrieve Geolib tag for blend: tagId=0x%x", geoLibTag);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCVP, "%s, Can not get geolib tagId, profile id = %u, processing type = %u",
                       NodeIdentifierString(), m_instanceProperty.profileId, m_instanceProperty.processingType);
        result = CamxResultENoSuch;
    }

    if (CamxResultSuccess == result)
    {
        UINT       geoLibFrameConfigTag[]   = { geoLibTag };
        const UINT length                   = CAMX_ARRAY_SIZE(geoLibFrameConfigTag);
        VOID*      pData[length]            = { 0 };
        UINT64     pDataOffset[length]      = { 0 };

        result = GetDataList(geoLibFrameConfigTag, pData, pDataOffset, length);

        if (CamxResultSuccess == result && (NULL != pData[0]))
        {
            GeoLibStillFrameConfig* pStillFrameConfig = &m_pStillFrameConfig[requestId % m_requestQueueDepth];
            Utils::Memcpy(pStillFrameConfig, pData[0], sizeof(GeoLibStillFrameConfig));
            Utils::Memcpy(pCvpIcaMapping, &pStillFrameConfig->cvpIcaMapping, sizeof(GeoLibIcaMapping));

            if (TRUE == m_dumpGeolibResult)
            {
                DumpGeolibResult(pStillFrameConfig, requestId);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCVP, "%s, Can not get Geo lib frame configuration: result=%d, data=%p",
                           NodeIdentifierString(), result, pData[0]);
            result = CamxResultENoSuch;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::SetGeoLibStillFrameConfigurationDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::SetGeoLibStillFrameConfigurationDependency(
    NodeProcessRequestData*  pNodeRequestData)
{
    CamxResult result   = CamxResultSuccess;
    UINT32     count    = pNodeRequestData->dependencyInfo[0].propertyDependency.count;

    if (TRUE == IsCVPInPrefilter())
    {
        //  Temporarily hack this here. Will raise another gerrit to add geolib tags into publish list of BPS and
        //  reverse all the dependencies logic in BPS/IPE/CVP.
        if (TRUE == IsTagPresentInPublishList(m_stillFrameConfigurationTagIdPrefilter))
        {
            CAMX_LOG_INFO(CamxLogGroupCVP, "%s, No need to add Geolib frame configuration dependency",
                          NodeIdentifierString());
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupCVP, "%s, Add Geolib frame configuration dependency for tag=0x%x",
                          NodeIdentifierString(), m_stillFrameConfigurationTagIdPrefilter);
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count++] =
                                                            m_stillFrameConfigurationTagIdPrefilter;
            pNodeRequestData->dependencyInfo[0].propertyDependency.count               = count;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::DumpGeolibResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CVPNode::DumpGeolibResult(
    GeoLibStillFrameConfig* pStillFrameConfig,
    UINT64                  requestId)
{
    CHAR dumpFilename[256] = { 0 };

    OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename), "%s/%s_reequest_%llu_%s.log",
                      CamX::ConfigFileDirectory,
                      NodeIdentifierString(),
                      requestId,
                      "geolib_cvp_still_frame_config");

    FILE* pFile = OsUtils::FOpen(dumpFilename, "wb");
    if (NULL != pFile)
    {
        OsUtils::FPrintF(pFile, "==================== Still Output ====================\n");
        Dump_GeoLibStillFrameConfig(pStillFrameConfig, pFile);
        OsUtils::FClose(pFile);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCVP, "Can not create GeoLib output information dump file");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::CreateSynxObject
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::CreateSynxObject(
    synx_handle_t*  phSynxInputHandler,
    CSLFence        hInputFence,
    UINT32*         pSynxKey)
{
    CamxResult result   = CamxResultSuccess;

    synx_result_t           synxResult      = SYNX_SUCCESS;
    synx_create_params_t    synxParams      = {0};
    synx_export_params_t    exportParams;
    synx_external_desc_t    externalDesc;

    exportParams.size   = sizeof(exportParams);
    externalDesc.type   = SYNX_TYPE_CSL_FENCE;
    externalDesc.id[0]  = 0;

    synxResult = synx_create(&synxParams, phSynxInputHandler);

    if (synxResult == SYNX_SUCCESS)
    {
        externalDesc.id[0] = hInputFence;

        // Bind the synx object with the CSLFence handle
        synxResult = synx_bind(*phSynxInputHandler, &externalDesc);
        if (synxResult != SYNX_SUCCESS)
        {
            CAMX_LOG_ERROR(CamxLogGroupCVP, "Could not bind synx object with fence");
            result = CamxResultEFailed;
        }

        if (CamxResultSuccess == result)
        {
            exportParams.hSynx = *phSynxInputHandler;
            synxResult = synx_export(&exportParams, pSynxKey);
            if (SYNX_SUCCESS != synxResult)
            {
                CAMX_LOG_ERROR(CamxLogGroupCVP, "synx_export failed");
                result = CamxResultEFailed;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCVP, "Could not create synx object!");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::ReleaseSynxObject
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CVPNode::ReleaseSynxObject(
    UINT32  requestIdIndex)
{
    synx_result_t   synxResult = SYNX_SUCCESS;

    // Release synx object
    if (( 0 != m_phSynxDmeOutput[requestIdIndex]) &&
        (SYNX_STATUS_INVALID != synx_get_status(m_phSynxDmeOutput[requestIdIndex])))
    {
        synxResult = synx_release(m_phSynxDmeOutput[requestIdIndex]);
        if (SYNX_SUCCESS != synxResult)
        {
            CAMX_LOG_ERROR(CamxLogGroupCVP, "m_phSynxDmeOutput synx_release failed");
        }
        m_phSynxDmeOutput[requestIdIndex] = 0;
    }

    if ((TRUE == m_bICAEnabled) &&
        ( 0 != m_phSynxIcaOutput[requestIdIndex]) &&
        (SYNX_STATUS_INVALID != synx_get_status(m_phSynxIcaOutput[requestIdIndex])))
    {
        synxResult = synx_release(m_phSynxIcaOutput[requestIdIndex]);
        if (SYNX_SUCCESS != synxResult)
        {
            CAMX_LOG_ERROR(CamxLogGroupCVP, "m_phSynxIcaOutput synx_release failed");
        }
        m_phSynxIcaOutput[requestIdIndex] = 0;
    }

    if (( 0 != m_phSynxSrcImage[requestIdIndex]) &&
        (SYNX_STATUS_INVALID != synx_get_status(m_phSynxSrcImage[requestIdIndex])))
    {
        synxResult = synx_release(m_phSynxSrcImage[requestIdIndex]);
        if (SYNX_SUCCESS != synxResult)
        {
            CAMX_LOG_ERROR(CamxLogGroupCVP, "m_phSynxSrcImage synx_release failed");
        }
        m_phSynxSrcImage[requestIdIndex] = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVPNode::GetStabilizationMargins()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::GetStabilizationMargins()
{
    CamxResult          result                     = CamxResultSuccess;
    UINT32              receivedMarginEISTag       = 0;
    UINT32              additionalCropOffsetEISTag = 0;
    StabilizationMargin receivedMargin             = { 0 };
    ImageDimensions     additionalCropOffset       = { 0 };

    if (0 != (IPEStabilizationType::IPEStabilizationTypeEIS2 & m_instanceProperty.stabilizationType))
    {
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.eisrealtime",
                                                          "StabilizationMargins", &receivedMarginEISTag);
        CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Fail to query: StabilizationMargins");

    }
    else if (0 != (IPEStabilizationType::IPEStabilizationTypeEIS3 & m_instanceProperty.stabilizationType))
    {
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.eislookahead",
                                                          "StabilizationMargins", &receivedMarginEISTag);
        CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Fail to query: StabilizationMargins");

    }

    const UINT marginTags[] =
    {
        receivedMarginEISTag       | UsecaseMetadataSectionMask,
    };

    const SIZE_T length         = CAMX_ARRAY_SIZE(marginTags);
    VOID*        pData[length]  = { 0 };
    UINT64       offset[length] = { 0 };

    result = GetDataList(marginTags, pData, offset, length);

    if (CamxResultSuccess == result)
    {
        if (NULL != pData[0])
        {
            receivedMargin                    = *static_cast<StabilizationMargin*>(pData[0]);
            m_stabilizationMargin.widthPixels = Utils::EvenFloorUINT32(receivedMargin.widthPixels);
            m_stabilizationMargin.heightLines = Utils::EvenFloorUINT32(receivedMargin.heightLines);
        }

    }

    CAMX_LOG_VERBOSE(CamxLogGroupCore,
                     "IPE stabilization margins for stabilization type %d set to %ux%u",
                     m_instanceProperty.stabilizationType,
                     m_stabilizationMargin.widthPixels,
                     m_stabilizationMargin.heightLines);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVPNode::LoadCVPChromatix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::LoadCVPChromatix(
    const ISPInputData* pInputData)
{
    CamxResult                      result         = CamxResultSuccess;
    TuningDataManager*              pTuningManager = NULL;

    if (NULL != pInputData)
    {
        pTuningManager = pInputData->pTuningDataManager;
    }
    else
    {
        pTuningManager = GetTuningDataManagerWithCameraId(GetPipeline()->GetCameraId());
    }

    if (NULL != pTuningManager && TRUE == pTuningManager->IsValidChromatix())
    {
        m_pChromatix = pTuningManager->GetChromatix()->GetModule_cvp10_ipe(
            reinterpret_cast<TuningMode*>(&m_tuningData.TuningMode[0]),
            m_tuningData.noOfSelectionParameter);
        if (NULL != m_pChromatix)
        {
            m_pReserveData = &m_pChromatix->chromatix_cvp10_reserve;
            m_pRgnDataType = &m_pChromatix->chromatix_cvp10_core.mod_cvp10_pre_scale_ratio_data->
                pre_scale_ratio_data.mod_cvp10_hdr_aec_data->
                hdr_aec_data.mod_cvp10_aec_data->cvp10_rgn_data;
        }
    }

    if ((NULL == m_pChromatix) || (NULL == m_pReserveData) || (NULL == m_pRgnDataType))
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupCVP, "%s, Invalid chromatix %p, %p, %p",
            NodeIdentifierString(), m_pChromatix, m_pReserveData, m_pRgnDataType);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVPNode::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CVPNode::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;
    UINT               interpolationSize;

    interpolationSize = (sizeof(cvp_1_0_0::cvp10_rgn_dataType) * (CVP10MaxmiumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }

    if ((NULL == m_pOutputData) && (CamxResultSuccess == result))
    {
        m_pOutputData = CAMX_NEW CVP10OutputData;
        if (NULL == m_pOutputData)
        {
            result = CamxResultENoMemory;
            CAMX_FREE(m_dependenceData.pInterpolationData);
            m_dependenceData.pInterpolationData = NULL;
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVPNode::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CVPNode::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }

    if (NULL != m_pOutputData)
    {
        CAMX_DELETE m_pOutputData;
        m_pOutputData = NULL;
    }
}

CAMX_NAMESPACE_END
