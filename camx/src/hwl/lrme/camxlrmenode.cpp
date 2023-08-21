
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxlrmenode.cpp
/// @brief LRME Node class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxnode.h"
#include "camxlrmenode.h"
#include "camxcdmdefs.h"
#include "camxcsllrmedefs.h"
#include "camxcslresourcedefs.h"
#include "camximagebuffer.h"
#include "camximageformatutils.h"
#include "camxhwcontext.h"
#include "camxtitan17xcontext.h"
#include "camxtitan17xdefs.h"
#include "camxtrace.h"
#include "camxformats.h"
#include "camxpacketbuilder.h"
#include "camxvendortags.h"
#include "titan170_lrme.h"
#include "camxlrmeproperty.h"

CAMX_NAMESPACE_BEGIN

// NOWHINE GR016: Defining the structure the way it's consistent with register header file
typedef struct {
    UINT32  TAR_HEIGHT : 14; /* 13:0 */
    UINT32  UNUSED0 : 18; /* 31:14 */
} _lrme_lrme_clc_tar_height;

typedef union {
    _lrme_lrme_clc_tar_height bitfields;
    _lrme_lrme_clc_tar_height bits;
    UINT32 u32All;
} LRME_LRME_CLC_TAR_HEIGHT;

// NOWHINE GR016: Defining the structure the way it's consistent with register header file
typedef struct {
    UINT32  REF_HEIGHT : 14; /* 13:0 */
    UINT32  UNUSED0 : 18; /* 31:14 */
} _lrme_lrme_clc_ref_height;

typedef union {
    _lrme_lrme_clc_ref_height bitfields;
    _lrme_lrme_clc_ref_height bits;
    UINT32 u32All;
} LRME_LRME_CLC_REF_HEIGHT;

static const UINT LRMEMaxInPorts     = 2;
static const UINT LRMEMaxOutPorts    = 2;
static const UINT LRMEMaxKMDDWORDS   = 1000;

/// @brief LRME node states
enum lrmeNodeStates
{
    LRME_NODE_CREATED,           ///< Initial state to begin with
    LRME_NODE_INITIALIZING,      ///< Move to this state after PRocessingNodeInitialize is called
    LRME_NODE_INITIALIZED,       ///< Move to this state after Finalize
    LRME_NODE_ACTIVE,            ///< Move to this state after device is acquired
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static variables
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// @brief list of properties/tags and vendor tags published by LRME node
static const UINT32 sLRMEPublishList[] =
{
    PropertyIDLRMEFrameSettings,
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LRMENode::LRMENode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LRMENode::LRMENode()
{
    m_pNodeName           = "LRME";
    m_numInputPorts       = LRMEMaxInPorts;
    m_numOutputPorts      = LRMEMaxOutPorts;
    m_state               = LRME_NODE_CREATED;
    m_hDevice             = -1;
    m_resetReferenceInput = FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LRMENode::~LRMENode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LRMENode::~LRMENode()
{
    Cleanup();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LRMENode::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LRMENode* LRMENode::Create(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateInputData);
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    CAMX_LOG_VERBOSE(CamxLogGroupLRME, "Create LRME node");
    return CAMX_NEW LRMENode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LRMENode::Cleanup
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID LRMENode::Cleanup()
{
    m_state = LRME_NODE_CREATED;
    ReleaseDevice();
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LRMENode::InitializeRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID LRMENode::InitializeRegisters()
{
    UINT regIndex;

    for (regIndex = 0; regIndex < lrmeRegisterListMax; regIndex++)
    {
        switch (regIndex)
        {
            case lrmeCLCModuleConfig:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_MODULE_CFG;
                LRME_LRME_CLC_MODULE_CFG clcModuleConfig;
                clcModuleConfig.u32All = 0;
                clcModuleConfig.bits.EN = 1;
                clcModuleConfig.bits.SUBPELSEARCHENABLE = 1;
                clcModuleConfig.bits.ISREFVALID = 0;
                m_lrmeRegistersValue[regIndex] = clcModuleConfig.u32All;
                break;
            case lrmeCLCModuleFormat:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_MODULEFORMAT;
                break;
            case lrmeCLCModuleRangeStep:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_RANGESTEP;
                LRME_LRME_CLC_RANGESTEP clcRangeStep;
                clcRangeStep.u32All = 0;
                clcRangeStep.bits.STEPX = m_capability.stepx;
                clcRangeStep.bits.STEPY = m_capability.stepy;
                clcRangeStep.bits.SEARCHRANGEX = m_capability.searchAreaRangex;
                clcRangeStep.bits.SEARCHRANGEY = m_capability.searchAreaRangey;
                m_lrmeRegistersValue[regIndex] = clcRangeStep.u32All;
                break;
            case lrmeCLCModuleOffset:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_OFFSET;
                break;
            case lrmeCLCModuleMaxAllowedSad:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_MAXALLOWEDSAD;
                LRME_LRME_CLC_MAXALLOWEDSAD clcMaxAllowedSad;
                clcMaxAllowedSad.u32All = 0;
                clcMaxAllowedSad.bits.MAXALLOWEDSAD = 0x4B0;
                m_lrmeRegistersValue[regIndex] = clcMaxAllowedSad.u32All;
                break;
            case lrmeCLCModuleMinAllowedTarMad:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_MINALLOWEDTARMAD;
                LRME_LRME_CLC_MINALLOWEDTARMAD clcMinAllowedTarMad;
                clcMinAllowedTarMad.u32All = 0;
                clcMinAllowedTarMad.bits.MINALLOWEDTARMAD = 0x60;
                m_lrmeRegistersValue[regIndex] = clcMinAllowedTarMad.u32All;
                break;
            case lrmeCLCModuleMeaningfulSaDDiff:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_MEANINGFULSADDIFF;
                LRME_LRME_CLC_MEANINGFULSADDIFF clcMeaningfulSaDiff;
                clcMeaningfulSaDiff.u32All = 0;
                clcMeaningfulSaDiff.bits.MEANINGFULSADDIFF = 0x5;
                m_lrmeRegistersValue[regIndex] = clcMeaningfulSaDiff.u32All;
                break;
            case lrmeCLCModulMinSaDDiffDenom:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_MINSADDIFFDENOM;
                LRME_LRME_CLC_MINSADDIFFDENOM clcMinSaDiffDenom;
                clcMinSaDiffDenom.bits.MINSADDIFFDENOM = 0x180;
                m_lrmeRegistersValue[regIndex] = clcMinSaDiffDenom.u32All;
                break;
            case lrmeCLCModuleRobustnessMeasureDistMap0:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_ROBUSTNESSMEASUREDISTMAP_0;
                break;
            case lrmeCLCModuleRobustnessMeasureDistMap1:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_ROBUSTNESSMEASUREDISTMAP_1;
                LRME_LRME_CLC_ROBUSTNESSMEASUREDISTMAP_1 clcRobustnessMeasureDistMap1;
                clcRobustnessMeasureDistMap1.bits.ROBUSTNESSMEASUREDISTMAP_1 = 0x80;
                m_lrmeRegistersValue[regIndex] = clcRobustnessMeasureDistMap1.u32All;
                break;
            case lrmeCLCModuleRobustnessMeasureDistMap2:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_ROBUSTNESSMEASUREDISTMAP_2;
                LRME_LRME_CLC_ROBUSTNESSMEASUREDISTMAP_2 clcRobustnessMeasureDistMap2;
                clcRobustnessMeasureDistMap2.bits.ROBUSTNESSMEASUREDISTMAP_2 = 0x80;
                m_lrmeRegistersValue[regIndex] = clcRobustnessMeasureDistMap2.u32All;
                break;
            case lrmeCLCModuleRobustnessMeasureDistMap3:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_ROBUSTNESSMEASUREDISTMAP_3;
                LRME_LRME_CLC_ROBUSTNESSMEASUREDISTMAP_3 clcRobustnessMeasureDistMap3;
                clcRobustnessMeasureDistMap3.bits.ROBUSTNESSMEASUREDISTMAP_3 = 0x80;
                m_lrmeRegistersValue[regIndex] = clcRobustnessMeasureDistMap3.u32All;
                break;
            case lrmeCLCModuleRobustnessMeasureDistMap4:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_ROBUSTNESSMEASUREDISTMAP_4;
                LRME_LRME_CLC_ROBUSTNESSMEASUREDISTMAP_4 clcRobustnessMeasureDistMap4;
                clcRobustnessMeasureDistMap4.bits.ROBUSTNESSMEASUREDISTMAP_4 = 0x80;
                m_lrmeRegistersValue[regIndex] = clcRobustnessMeasureDistMap4.u32All;
                break;
            case lrmeCLCModuleRobustnessMeasureDistMap5:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_ROBUSTNESSMEASUREDISTMAP_5;
                LRME_LRME_CLC_ROBUSTNESSMEASUREDISTMAP_5 clcRobustnessMeasureDistMap5;
                clcRobustnessMeasureDistMap5.bits.ROBUSTNESSMEASUREDISTMAP_5 = 0x80;
                m_lrmeRegistersValue[regIndex] = clcRobustnessMeasureDistMap5.u32All;
                break;
            case lrmeCLCModuleRobustnessMeasureDistMap6:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_ROBUSTNESSMEASUREDISTMAP_6;
                LRME_LRME_CLC_ROBUSTNESSMEASUREDISTMAP_6 clcRobustnessMeasureDistMap6;
                clcRobustnessMeasureDistMap6.bits.ROBUSTNESSMEASUREDISTMAP_6 = 0x80;
                m_lrmeRegistersValue[regIndex] = clcRobustnessMeasureDistMap6.u32All;
                break;
            case lrmeCLCModuleRobustnessMeasureDistMap7:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_ROBUSTNESSMEASUREDISTMAP_7;
                LRME_LRME_CLC_ROBUSTNESSMEASUREDISTMAP_7 clcRobustnessMeasureDistMap7;
                clcRobustnessMeasureDistMap7.bits.ROBUSTNESSMEASUREDISTMAP_7 = 0x80;
                m_lrmeRegistersValue[regIndex] = clcRobustnessMeasureDistMap7.u32All;
                break;
            case lrmeCLCModuleDsCropHorizontal:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_DS_CROP_HORIZONTAL;
                break;
            case lrmeCLCModuleDsCropVertical:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_DS_CROP_VERTICAL;
                break;
            case lrmeCLCModuleTarPdUnpacker:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_TAR_PD_UNPACKER;
                break;
            case lrmeCLCModuleRefPdUnpacker:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_REF_PD_UNPACKER;
                break;
            case lrmeCLCModuleSwOverride:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_SW_OVERRIDE;
                break;
            case lrmeCLCModuleTarHeight:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_TAR_HEIGHT;
                LRME_LRME_CLC_TAR_HEIGHT clcTarHeight;
                clcTarHeight.u32All = 0;
                m_lrmeRegistersValue[regIndex] = clcTarHeight.u32All;
                break;
            case lrmeCLCModuleRefHeight:
                m_lrmeRegistersAddress[regIndex] = regLRME_LRME_CLC_REF_HEIGHT;
                LRME_LRME_CLC_REF_HEIGHT clcRefHeight;
                clcRefHeight.u32All = 0;
                m_lrmeRegistersValue[regIndex] = clcRefHeight.u32All;
                break;
            default:
                break;
        }
    }
    m_numRegisters = lrmeRegisterListMax;
    m_cmdBufferSize = PacketBuilder::RequiredWriteInterleavedRegsSizeInDwords(m_numRegisters) * sizeof(UINT32) +
                      LRMEMaxKMDDWORDS;
    CAMX_LOG_VERBOSE(CamxLogGroupLRME, "lrme num regs %u m_cmdBufferSize %u ", m_numRegisters, m_cmdBufferSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LRMENode::SetupLRMEFormats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID LRMENode::SetupLRMEFormats()
{
    switch (m_version.majorVersion)
    {
        case 0:
        case 1:
            m_capability.formats = CSLLRMEFormatY8 | CSLLRMEFormatPD8 | CSLLRMEFormatPD10 |
                                   CSLLRMEFormatNV12 | CSLLRMEFormatY10;
            break;
        default:
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LRMENode::ConfigureLRMECapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult LRMENode::ConfigureLRMECapability()
{
    CamxResult result = CamxResultSuccess;

    GetHwContext()->GetDeviceVersion(CSLDeviceTypeLRME, &m_version);

    switch (m_version.majorVersion)
    {
        case 0:
        // Fall through, version 0 and 1 treated as same
        case 1:
            m_capability.minInputWidth           = 36;
            m_capability.minInputHeight          = 24;
            m_capability.maxInputWidth           = 360;
            m_capability.maxInputHeight          = 540;
            m_capability.optimalInputWidth       = 240;
            m_capability.optimalInputHeight      = 136;
            m_capability.searchAreaRangex        = 12;
            m_capability.searchAreaRangey        = 8;
            m_capability.stepx                   = 12;
            m_capability.stepy                   = 8;
            m_capability.maxWidthDownscale       = 2;
            m_capability.maxHeightDownscale      = 1;
            m_capability.maxWidthCrop            = 4;
            m_capability.maxHeightCrop           = 1;
            SetupLRMEFormats();
            break;
        default:
            result = CamxResultEUnsupported;
            break;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LRMENode::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult LRMENode::ProcessingNodeInitialize(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateInputData);

    CamxResult        result = CamxResultSuccess;
    INT32             deviceIndex = -1;
    UINT              indicesLengthRequired = 0;
    HwEnvironment*    pHwEnvironment = HwEnvironment::GetInstance();

    CAMX_ASSERT(LRME == Type());
    CAMX_ASSERT(NULL != pCreateOutputData);

    m_lrmeDS2Connected = FALSE;
    if (LRME_NODE_CREATED != m_state)
    {
        CAMX_LOG_ERROR(CamxLogGroupLRME, "LRME in invalid state %d", m_state);
        result = CamxResultEFailed;
    }

    if (CamxResultSuccess == result)
    {
        m_state = LRME_NODE_INITIALIZING;
        pCreateOutputData->maxOutputPorts = m_numInputPorts;
        pCreateOutputData->maxInputPorts = m_numOutputPorts;

        // Add device indices
        result = pHwEnvironment->GetDeviceIndices(CSLDeviceTypeLRME, &deviceIndex, 1, &indicesLengthRequired);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupLRME, "Failed GetDeviceIndices %d", result);
            Cleanup();
        }
    }

    if (CamxResultSuccess == result)
    {
        CAMX_ASSERT(indicesLengthRequired == 1);
        result = AddDeviceIndex(deviceIndex);
        m_deviceIndex = deviceIndex;

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupLRME, "Failed to AddDeviceIndex %d", result);
            Cleanup();
        }
    }

    if (CamxResultSuccess == result)
    {
        // Configure LRME Capability
        result = ConfigureLRMECapability();

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupLRME, "Failed ConfigureLRMECapability %d", result);
            Cleanup();
        }
    }

    if (CamxResultSuccess == result)
    {
        InitializeRegisters();
        // 2 managers - 1 for command buffer and 1 for packet resource
        result = InitializeCmdBufferManagerList(lrmeBufferManagersMax);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupLRME, "Failed InitializeCmdBufferManagerList %d", result);
            Cleanup();
        }
    }

    UINT32  groupID         = 1;
    UINT    numOutputPorts  = 0;
    UINT    outputPortId[MaxBufferComposite];

    GetAllOutputPortIds(&numOutputPorts, &outputPortId[0]);

    for (UINT outputPortIndex = 0; outputPortIndex < numOutputPorts; outputPortIndex++)
    {
        pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] = groupID++;
    }

    pCreateOutputData->bufferComposite.hasCompositeMask = FALSE;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LRMENode::CheckInputRequirement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult LRMENode::CheckInputRequirement(
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
        CAMX_LOG_ERROR(CamxLogGroupLRME, "Invalid Input requirements");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LRMENode::SetupVectorOutputRequirementOptions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult LRMENode::SetupVectorOutputRequirementOptions(
    OutputPortNegotiationData* pOutputPortNegotiationData)
{
    CamxResult result = CamxResultSuccess;
    BufferRequirement* pBufferRequirementOptions = &pOutputPortNegotiationData->outputBufferRequirementOptions;
    UINT maxBlocks = static_cast<UINT>(ceil(static_cast<DOUBLE>(m_capability.maxInputWidth) / 12) *
        ceil(static_cast<DOUBLE>(m_capability.maxInputHeight) / 8));
    UINT minBlocks = static_cast<UINT>(ceil(static_cast<DOUBLE>(m_capability.minInputWidth) / 12) *
        ceil(static_cast<DOUBLE>(m_capability.minInputHeight) / 8));
    UINT32 maxOutputWidth = maxBlocks * 6;
    UINT32 minOutputWidth = minBlocks * 6;
    UINT32 maxOutputHeight = 1;
    UINT32 minOutputHeight = 1;
    BufferRequirement maxResolution;

    maxResolution.maxHeight = 1;
    maxResolution.minHeight = 1;
    maxResolution.maxWidth = maxOutputWidth;
    maxResolution.minWidth = minOutputWidth;

    for (UINT inputIndex = 0; inputIndex < pOutputPortNegotiationData->numInputPortsNotification; inputIndex++)
    {
        BufferRequirement  inputPortRequirement      = pOutputPortNegotiationData->inputPortRequirement[inputIndex];
        result = CheckInputRequirement(inputPortRequirement, maxResolution);
        if (CamxResultSuccess != result)
        {
            break;
        }
        // Setup effective resolution map
        BufferRequirement effectiveResolution = inputPortRequirement;
        CamX::Utils::ClampUINT32(effectiveResolution.maxWidth, minOutputWidth, maxOutputWidth);
        CamX::Utils::ClampUINT32(effectiveResolution.optimalWidth, minOutputWidth, maxOutputWidth);
        CamX::Utils::ClampUINT32(effectiveResolution.minWidth, minOutputWidth, maxOutputWidth);
        CamX::Utils::ClampUINT32(effectiveResolution.maxHeight, minOutputHeight, maxOutputHeight);
        CamX::Utils::ClampUINT32(effectiveResolution.optimalHeight, minOutputHeight, maxOutputHeight);
        CamX::Utils::ClampUINT32(effectiveResolution.minHeight, minOutputHeight, maxOutputHeight);

        if (0 == inputIndex)
        {
            *pBufferRequirementOptions = effectiveResolution;
        }
        else
        {
            // If ranges don't overlap then no possible selection available
            if (effectiveResolution.minWidth > pBufferRequirementOptions->maxWidth ||
                pBufferRequirementOptions->minWidth > effectiveResolution.maxWidth)
            {
                result = CamxResultEUnsupported;
                CAMX_LOG_ERROR(CamxLogGroupLRME, "Input requirements don't overlap");
            }
            else
            {
                pBufferRequirementOptions->maxWidth  = CamX::Utils::MinUINT32(effectiveResolution.maxWidth,
                                                          pBufferRequirementOptions->maxWidth);
                pBufferRequirementOptions->maxHeight = CamX::Utils::MinUINT32(effectiveResolution.maxHeight,
                                                          pBufferRequirementOptions->maxHeight);
                pBufferRequirementOptions->minWidth  = CamX::Utils::MaxUINT32(effectiveResolution.minWidth,
                                                          pBufferRequirementOptions->minWidth);
                pBufferRequirementOptions->minHeight = CamX::Utils::MaxUINT32(effectiveResolution.minHeight,
                                                          pBufferRequirementOptions->minHeight);
                pBufferRequirementOptions->optimalWidth = CamX::Utils::MaxUINT32(effectiveResolution.minWidth,
                                                            pBufferRequirementOptions->minWidth);
                pBufferRequirementOptions->optimalHeight = CamX::Utils::MaxUINT32(effectiveResolution.minHeight,
                                                            pBufferRequirementOptions->minHeight);
            }
        }
        if (CamxResultSuccess != result)
        {
            break;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LRMENode::SetupDS2OutputFinalResolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID LRMENode::SetupDS2OutputFinalResolution(
    BufferProperties* pFinalOutputBufferProperties,
    UINT width,
    UINT height)
{
    pFinalOutputBufferProperties->imageFormat.height = height;
    pFinalOutputBufferProperties->imageFormat.width  = width;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LRMENode::SetupVectorOutputFinalResolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID LRMENode::SetupVectorOutputFinalResolution(
    BufferProperties* pFinalOutputBufferProperties,
    UINT width,
    UINT height)
{
    m_lrmeVectorFormat = CSLLRMEVectorOutputShort;

    pFinalOutputBufferProperties->imageFormat.height = 1;
    UINT numBlocks = static_cast<UINT>(ceil(static_cast<DOUBLE>(width) / 12) * ceil(static_cast<DOUBLE>(height) / 8));
    pFinalOutputBufferProperties->imageFormat.width = numBlocks * (6 + m_lrmeVectorFormat * 8);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LRMENode::ProcessingNodeFinalizeInputRequirement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult LRMENode::ProcessingNodeFinalizeInputRequirement(
    BufferNegotiationData* pBufferNegotiationData)
{
    CamxResult result                            = CamxResultSuccess;
    UINT       factor                            = 0;
    UINT       numInputPort                      = 0;
    UINT       inputPortId[LRMEMaxIntputPorts];
    UINT       fullFactor                        = 0;
    UINT32     cslFormat                         = 0;

    // During renegotiation m_state would have been already set to LRME_NODE_INITIALIZED
    // Hence just checking for "LRME_NODE_INITIALIZING" would fail the negotiation
    if ((m_state != LRME_NODE_INITIALIZING) &&
       (m_state != LRME_NODE_INITIALIZED))
    {
        CAMX_LOG_ERROR(CamxLogGroupLRME, "LRME invalid state %d", m_state);
        result = CamxResultEFailed;
    }

    if (CamxResultSuccess == result)
    {
        GetAllInputPortIds(&numInputPort, &inputPortId[0]);

        // determine the full res port max width and height that can be accepted
        for (UINT inputIndex = 0; inputIndex < numInputPort; inputIndex++)
        {
            const ImageFormat* pImageFormat = GetInputPortImageFormat(inputIndex);
            if (NULL != pImageFormat)
            {
                cslFormat = GetCSLLRMEFormat(pImageFormat->format);
            }
            if (0 == (cslFormat & m_capability.formats))
            {
                continue;
            }
            switch (inputPortId[inputIndex])
            {
                case LRMEInputPortTARIFEFull:
                    if (0 == fullFactor)
                    {
                        fullFactor = 1;
                    }
                    break;
                case LRMEInputPortREFIFEFull:
                    continue;
                case LRMEInputPortTARIFEDS4:
                    if (4 > fullFactor)
                    {
                        fullFactor = 4;
                    }
                    break;
                case LRMEInputPortREFIFEDS4:
                    continue;
                case LRMEInputPortTARIFEDS16:
                    fullFactor = 16;
                    break;
                case LRMEInputPortREFIFEDS16:
                    continue;
                case LRMEInputPortREFLRMEDS2:
                    m_lrmeDS2Connected = TRUE;
                    continue;
                default:
                    result = CamxResultEFailed;
            }

            if (CamxResultSuccess != result)
            {
                break;
            }
        }
        if (0 == fullFactor)
        {
            CAMX_LOG_ERROR(CamxLogGroupLRME, "Failed to calculate negotiation factor, perhaps no TAR port connected");
            result = CamxResultEFailed;
        }
    }

    if (CamxResultSuccess == result)
    {
        // Setup the output port buffer requirement options
        for (UINT index = 0; index < pBufferNegotiationData->numOutputPortsNotified; index++)
        {
            OutputPortNegotiationData* pOutputPortNegotiationData = &pBufferNegotiationData->pOutputPortNegotiationData[index];
            UINT outputPortId = GetOutputPortId(pOutputPortNegotiationData->outputPortIndex);
            switch (outputPortId)
            {
                case LRMEOutputPortVector:
                    SetupVectorOutputRequirementOptions(pOutputPortNegotiationData);
                    break;
                case LRMEOutputPortDS2:
                    break;
                default:
                    result = CamxResultEUnsupported;
                    break;
            }
            if (CamxResultSuccess != result)
            {
                break;
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        // Loop through the input ports and specify the requirement based on port type/ID
        for (UINT inputIndex = 0; inputIndex < numInputPort; inputIndex++)
        {
            pBufferNegotiationData->inputBufferOptions[inputIndex].nodeId = Type();
            pBufferNegotiationData->inputBufferOptions[inputIndex].instanceId = InstanceID();
            pBufferNegotiationData->inputBufferOptions[inputIndex].portId = inputPortId[inputIndex];

            switch (inputPortId[inputIndex])
            {
                case LRMEInputPortTARIFEFull:
                case LRMEInputPortREFIFEFull:
                    factor = fullFactor;
                    break;
                case LRMEInputPortTARIFEDS4:
                case LRMEInputPortREFIFEDS4:
                    factor = fullFactor / 4;
                    break;
                case LRMEInputPortTARIFEDS16:
                case LRMEInputPortREFIFEDS16:
                    factor = fullFactor / 16;
                    break;
                case LRMEInputPortREFLRMEDS2:
                    if (4 == fullFactor)
                    {
                        factor = fullFactor / 2;
                    }
                    else if (16 == fullFactor)
                    {
                        factor = fullFactor / 8;
                    }
                    break;
                default:
                    result = CamxResultEFailed;
            }
            BufferRequirement* pInputBufferRequirement =
                &pBufferNegotiationData->inputBufferOptions[inputIndex].bufferRequirement;

            if (LRMEInputPortTARIFEFull == inputPortId[inputIndex])
            {
                pInputBufferRequirement->maxWidth      = 0xffff;
                pInputBufferRequirement->maxHeight     = 0xffff;
                pInputBufferRequirement->minWidth      = 0;
                pInputBufferRequirement->minHeight     = 0;
                pInputBufferRequirement->optimalWidth  = 0;
                pInputBufferRequirement->optimalHeight = 0;
            }
            else
            {
                pInputBufferRequirement->maxWidth      = m_capability.maxInputWidth;
                pInputBufferRequirement->maxHeight     = m_capability.maxInputHeight;
                pInputBufferRequirement->optimalWidth  = m_capability.optimalInputWidth;
                pInputBufferRequirement->optimalHeight = m_capability.optimalInputHeight;
                pInputBufferRequirement->minWidth      = m_capability.minInputWidth       * factor;
                pInputBufferRequirement->minHeight     = m_capability.minInputHeight      * factor;
            }

            CAMX_LOG_INFO(CamxLogGroupLRME,
                "Backward buffer Negotiation port(%d) input requirement min(%d * %d) optimal(%d * %d) max(%d * %d)\n",
                inputPortId[inputIndex],
                pInputBufferRequirement->minWidth, pInputBufferRequirement->minHeight,
                pInputBufferRequirement->optimalWidth, pInputBufferRequirement->optimalHeight,
                pInputBufferRequirement->maxWidth, pInputBufferRequirement->maxHeight);
        }
        m_state = LRME_NODE_INITIALIZED;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupLRME, "Backward buffer negotiation result %d", result);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LRMENode::FinalizeBufferProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID LRMENode::FinalizeBufferProperties(
    BufferNegotiationData* pBufferNegotiationData)
{
    CAMX_ASSERT(NULL != pBufferNegotiationData);

    INT                optimalRes                       = m_capability.maxInputWidth * m_capability.maxInputHeight;
    INT                minDiff                          = INT_MAX;
    const ImageFormat* pFormat;
    UINT               tarPort                          = 0;
    UINT               refPort                          = 0;
    INT                resDiff;
    INT                resDiffDS2                       = INT_MAX;
    UINT               numInputPort                     = 0;
    UINT               inputPortId[LRMEMaxIntputPorts];
    UINT               CLCWidth                         = 0;
    UINT               CLCHeight                        = 0;
    BOOL               enableDS2                        = FALSE;

    GetAllInputPortIds(&numInputPort, &inputPortId[0]);

    for (UINT inputIndex = 0; inputIndex < numInputPort; inputIndex++)
    {
        switch (inputPortId[inputIndex])
        {
            case LRMEInputPortTARIFEFull:
                pFormat           = pBufferNegotiationData->pInputPortNegotiationData[inputIndex].pImageFormat;
                m_fullInputHeight = pFormat->height;
                m_fullInputWidth  = pFormat->width;
                break;
            case LRMEInputPortTARIFEDS4:
            case LRMEInputPortTARIFEDS16:
                pFormat = pBufferNegotiationData->pInputPortNegotiationData[inputIndex].pImageFormat;
                resDiff = pFormat->width * pFormat->height;
                resDiff = CamX::Utils::AbsoluteINT32(resDiff - optimalRes);

                if ((TRUE == m_lrmeDS2Connected) && (TRUE == GetStaticSettings()->enableLRMEDS2))
                {
                    resDiffDS2 = ceil(static_cast<FLOAT>(pFormat->width / 2)) * ceil(static_cast<FLOAT>(pFormat->height / 2));
                    resDiffDS2 = CamX::Utils::AbsoluteINT32((resDiffDS2 - optimalRes));
                    if ((m_capability.maxInputWidth < pFormat->width || m_capability.maxInputHeight < pFormat->height) &&
                        (m_capability.maxInputWidth >= (pFormat->width/2) &&
                         m_capability.maxInputHeight >= (pFormat->height/2)))
                    {
                        CAMX_LOG_INFO(CamxLogGroupLRME, "Enabling DS2 for %dX%d", (pFormat->width/2), (pFormat->height/2));
                        enableDS2 = TRUE;
                    }
                }
                if (((m_capability.maxInputWidth >= pFormat->width && m_capability.maxInputHeight >= pFormat->height) ||
                      (TRUE == enableDS2))&&
                    (resDiff < minDiff || resDiffDS2 < minDiff))
                {
                    minDiff = CamX::Utils::MinUINT32(resDiff, resDiffDS2);
                    tarPort = inputPortId[inputIndex];
                    if ((minDiff == resDiff) && (FALSE == enableDS2))
                    {
                        refPort = inputPortId[inputIndex] + 1;
                        CLCHeight = pFormat->height;
                        CLCWidth = pFormat->width;
                    }
                    else
                    {
                        refPort = LRMEInputPortREFLRMEDS2;
                        CLCHeight = Utils::AlignGeneric32(ceil(static_cast<FLOAT>(pFormat->height / 2)), 2);
                        CLCWidth = Utils::AlignGeneric32(ceil(static_cast<FLOAT>(pFormat->width / 2)), 2);
                    }
                }

                m_ds2Enable = enableDS2;
                break;
            default:
                break;
        }
    }
    CAMX_LOG_INFO(CamxLogGroupLRME, "Selected tar and ref ports %d/%d\n", tarPort, refPort);
    CAMX_LOG_INFO(CamxLogGroupLRME, "Negotiated Input Width/Height %u/%u\n", CLCWidth, CLCHeight);

    // Disable unselected tar and ref ports
    if (INT_MAX != minDiff)
    {
        for (UINT inputIndex = 0; inputIndex < numInputPort; inputIndex++)
        {
            if (inputPortId[inputIndex] != tarPort && inputPortId[inputIndex] != refPort)
            {
                CAMX_LOG_INFO(CamxLogGroupLRME, "Disabling input port %d\n", inputPortId[inputIndex]);

                UINT inputPortIndex = InputPortIndex(inputPortId[inputIndex]);
                if (inputPortId[inputIndex] == (tarPort + 1) && (LRMEInputPortREFLRMEDS2 == refPort))
                {
                    // If the ref port is downscaled then do not remove the hw from device index list
                    // of corresponding input port
                    DisableInputOutputLink(inputPortIndex, FALSE);
                }
                else
                {
                    DisableInputOutputLink(inputPortIndex, TRUE);
                }
            }
            else if ((inputPortId[inputIndex] == refPort) && (LRMEInputPortREFLRMEDS2 != refPort))
            {
                // need request - 1 buffer on ref port
                SetInputPortBufferDelta(inputIndex, 1);
            }
        }
    }

    // Finalize the output requirement
    for (UINT index = 0; index < pBufferNegotiationData->numOutputPortsNotified; index++)
    {
        OutputPortNegotiationData* pOutputPortNegotiationData = &pBufferNegotiationData->pOutputPortNegotiationData[index];
        UINT outputPortId = GetOutputPortId(pOutputPortNegotiationData->outputPortIndex);
        switch (outputPortId)
        {
            case LRMEOutputPortVector:
                SetupVectorOutputFinalResolution(pOutputPortNegotiationData->pFinalOutputBufferProperties,
                                                 CLCWidth, CLCHeight);
                break;
            case LRMEOutputPortDS2:
                if (LRMEInputPortREFLRMEDS2 == refPort)
                {
                    SetupDS2OutputFinalResolution(pOutputPortNegotiationData->pFinalOutputBufferProperties,
                                                  CLCWidth, CLCHeight);
                }
                break;
            default:
                break;
        }
    }
    m_selectedTARPort = tarPort;
    m_selectedREFPort = refPort;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LRMENode::FillCmdBufferArray
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult LRMENode::FillCmdBufferArray(
    CmdBuffer* pCmdBuffer)
{
    CamxResult result               = CamxResultSuccess;
    UINT       rangeIndexStart      = 0;
    UINT       interleaveIndexStart = 0;
    UINT       numRange             = 0;
    UINT       numInterleaveRegs    = 0;
    UINT32*    pRegValuePair        = NULL;

    CAMX_LOG_VERBOSE(CamxLogGroupLRME, "Num regs %u", m_numRegisters);
    pRegValuePair = static_cast<UINT32*>(CAMX_CALLOC(m_numRegisters * 2 * sizeof(UINT32)));
    if (NULL == pRegValuePair)
    {
        CAMX_LOG_ERROR(CamxLogGroupLRME, "No Memory");
        return CamxResultENoMemory;
    }
    for (UINT regIndex = 0; regIndex < m_numRegisters; regIndex++)
    {
        if (0 == regIndex)
        {
            rangeIndexStart      = regIndex;
            interleaveIndexStart = regIndex;
            numRange = 1;
            numInterleaveRegs = 0;
            continue;
        }
        if (m_lrmeRegistersAddress[regIndex] == (m_lrmeRegistersAddress[regIndex - 1] + sizeof(UINT32)))
        {
            numRange++;
            if (0 < numInterleaveRegs)
            {
                if (CamxResultSuccess == result)
                {
                    for (UINT index = 0; index < numInterleaveRegs; index++)
                    {
                        pRegValuePair[index * 2] = m_lrmeRegistersAddress[interleaveIndexStart + index];
                        pRegValuePair[index * 2 + 1] = m_lrmeRegistersValue[interleaveIndexStart + index];
                    }
                    CAMX_LOG_VERBOSE(CamxLogGroupLRME, "Write interleave start %u num %u",
                        interleaveIndexStart, numInterleaveRegs);
                    result = PacketBuilder::WriteInterleavedRegs(pCmdBuffer, numInterleaveRegs, pRegValuePair);
                    numInterleaveRegs = 0;
                }
                if (CamxResultSuccess != result)
                {
                    break;
                }
            }
        }
        else
        {
            if (1 < numRange)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupLRME, "Write range start %u num %u", rangeIndexStart, numRange);
                result = PacketBuilder::WriteRegRange(pCmdBuffer, m_lrmeRegistersAddress[rangeIndexStart],
                    numRange, &m_lrmeRegistersValue[rangeIndexStart]);
                if (CamxResultSuccess != result)
                {
                    break;
                }
            }
            else
            {
                if (0 == numInterleaveRegs)
                {
                    interleaveIndexStart = regIndex - 1;
                }
                numInterleaveRegs++;
            }
            rangeIndexStart = regIndex;
            numRange = 1;
        }
    }
    if (CamxResultSuccess == result)
    {
        for (UINT index = 0; index < numInterleaveRegs; index++)
        {
            pRegValuePair[index * 2]     = m_lrmeRegistersAddress[interleaveIndexStart + index];
            pRegValuePair[index * 2 + 1] = m_lrmeRegistersValue[interleaveIndexStart + index];
        }
        if (0 < numInterleaveRegs)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupLRME, "Write interleave start %u num %u",
                interleaveIndexStart, numInterleaveRegs);
            result = PacketBuilder::WriteInterleavedRegs(pCmdBuffer, numInterleaveRegs, pRegValuePair);
        }
        if (CamxResultSuccess == result && 0 < numRange)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupLRME, "Write range start %u num %u", rangeIndexStart, numRange);
            result = PacketBuilder::WriteRegRange(pCmdBuffer, m_lrmeRegistersAddress[rangeIndexStart], numRange,
                                                  &m_lrmeRegistersValue[rangeIndexStart]);
        }
    }
    CAMX_FREE(pRegValuePair);
    pRegValuePair = NULL;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LRMENode::SetDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID LRMENode::SetDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData)
{
    NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;

    if (TRUE == GetStaticSettings()->enableImageBufferLateBinding)
    {
        // If latebinding is enabled, we want to delay packet preparation as late as possible, in other terms, we want to
        // prepare and submit to hw when it can really start processing. This is once all the input fences (+ property)
        // dependencies are satisfied. So, lets set input fence dependencies

        UINT fenceCount = SetInputBuffersReadyDependency(pExecuteProcessRequestData, 0);

        if (0 < fenceCount)
        {
            pNodeRequestData->dependencyInfo[0].processSequenceId = 1;
            pNodeRequestData->numDependencyLists = 1;
        }
    }

    pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency   = TRUE;
    pNodeRequestData->dependencyInfo[0].processSequenceId                                   = 1;
    pNodeRequestData->numDependencyLists                                                    = 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LRMENode::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult LRMENode::ExecuteProcessRequest(
    ExecuteProcessRequestData* pExecuteProcessRequestData)
{
    Packet*                    pPacket                            = NULL;
    CmdBuffer*                 pCmdBuffer                         = NULL;
    CamxResult                 result                             = CamxResultSuccess;
    NodeProcessRequestData*    pNodeRequestData                   = pExecuteProcessRequestData->pNodeProcessRequestData;
    PerRequestActivePorts*     pPerRequestPorts                   = pExecuteProcessRequestData->pEnabledPortsInfo;
    UINT64                     requestId                          = pNodeRequestData->pCaptureRequest->requestId;
    UINT32                     numBatchedFrames                   = pNodeRequestData->pCaptureRequest->numBatchedFrames;
    INT                        isRefValid                         = 0;
    BOOL                       hasDependency                      = FALSE;

    CAMX_LOG_INFO(CamxLogGroupLRME, "ExecuteProcessRequest enter: numBatchedFrames %d, request %lld",
                  numBatchedFrames, requestId);

    if (LRME_NODE_ACTIVE != m_state)
    {
        CAMX_LOG_ERROR(CamxLogGroupLRME, "LRME in invalid state %d", m_state);
        result = CamxResultEFailed;
    }

    if ((CamxResultSuccess  == result) &&
        (0                  == pNodeRequestData->processSequenceId))
    {
        SetDependencies(pExecuteProcessRequestData);
    }

    if (pNodeRequestData->dependencyInfo[0].dependencyFlags.dependencyFlagsMask)
    {
        hasDependency = TRUE;
    }

    if ((CamxResultSuccess == result) && (FALSE == hasDependency))
    {
        pNodeRequestData->numDependencyLists = 0;

        UINT64 requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(requestId);

        if (FirstValidRequestId == requestIdOffsetFromLastFlush)
        {
            // Disable reference input
            EnableRefPort(FALSE);
        }
        if (TRUE == IsSkipRequest())
        {
            result = SkipandSignalLRMEfences(pNodeRequestData, pPerRequestPorts, requestId, isRefValid);
            return result;
        }
        // If 1st request and only 1 o/p then there is no ref buffer, just signal the o/p fence in this case.
        // Also, the 1 o/p is vector buffer and in that case no downscaling is needed. Since, there is no
        // downscaling and no ref buffer the h/w does not need to do anything.
        if ((1 == pPerRequestPorts->numInputPorts)  &&
            (1 == pPerRequestPorts->numOutputPorts) &&
            (1 == numBatchedFrames))
        {
            // handle first request for non-batched mode
            if (FirstValidRequestId == requestIdOffsetFromLastFlush)
            {
                result = SkipandSignalLRMEfences(pNodeRequestData, pPerRequestPorts, requestId, isRefValid);
            }
            else
            {
                result = CamxResultEFailed;
            }
        }
        else
        {
            pPacket    = GetPacketForRequest(requestId, m_pLRMECmdBufferManager[lrmePacketManager]);
            pCmdBuffer = GetCmdBufferForRequest(requestId, m_pLRMECmdBufferManager[lrmeCmdBufferManager]);

            if ((NULL == pPacket) || (NULL == pCmdBuffer))
            {
                CAMX_LOG_ERROR(CamxLogGroupLRME, "Unable to get packet/cmd buffer %p/%p\n", pPacket, pCmdBuffer);
                result = CamxResultENoMemory;
            }

            if (CamxResultSuccess == result)
            {
                // Fill in IO information
                for (UINT inputIndex = 0; inputIndex < pPerRequestPorts->numInputPorts; inputIndex++)
                {
                    PerRequestInputPortInfo* pInputPort = &pPerRequestPorts->pInputPorts[inputIndex];
                    if ((NULL != pInputPort) && (NULL != pInputPort->pImageBuffer))
                    {
                        const ImageFormat* pFormat                      = pInputPort->pImageBuffer->GetFormat();
                        UINT32             numFences                    = (NULL == pInputPort->phFence) ? 0 : 1;
                        enum CSLLRMEIO     portId                       = (pInputPort->portId == m_selectedTARPort) ?
                                                                            CSLLRMETARInput : CSLLRMEREFInput;
                        UINT64             requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(requestId);
                        UINT               batchFrameIndex              = 0;

                        if ((m_resetReferenceInput == TRUE) && (portId == CSLLRMEREFInput))
                        {
                            CAMX_LOG_ERROR(CamxLogGroupLRME, "Skipping port %d for req %llu due to reset reference",
                                portId, pNodeRequestData->pCaptureRequest->requestId);
                            // Disable reference input
                            EnableRefPort(FALSE);
                            continue;
                        }

                        if ((1 < numBatchedFrames) &&
                            (FirstValidRequestId == requestIdOffsetFromLastFlush) &&
                            (1 == pPerRequestPorts->numInputPorts) &&
                            (FALSE == m_ds2Enable))
                        {
                            // First request batch mode, ds2 disabled: ref = curr[0], tar = curr[n-1]
                            for (UINT portIndex = 0; portIndex < LRMEMaxInPorts; portIndex++)
                            {
                                if (CSLLRMETARInput == portIndex)
                                {
                                    portId          = CSLLRMETARInput;
                                    batchFrameIndex = numBatchedFrames - 1;
                                }
                                else if (CSLLRMEREFInput == portIndex)
                                {
                                    portId          = CSLLRMEREFInput;
                                    batchFrameIndex = 0;
                                }
                                else
                                {
                                    result = CamxResultEUnsupported;
                                    break;
                                }

                                result = pPacket->AddIOConfig(pInputPort->pImageBuffer,
                                                              portId,
                                                              CSLIODirection::CSLIODirectionInput,
                                                              pInputPort->phFence,
                                                              numFences,
                                                              NULL,
                                                              NULL,
                                                              batchFrameIndex);

                                if (CamxResultSuccess != result)
                                {
                                    break;
                                }

                                if (NULL != pFormat)
                                {
                                    if (NULL != pInputPort->phFence)
                                    {
                                        CAMX_LOG_INFO(CamxLogGroupLRME, "LRME:%d reporting Input config, portId=%d, "
                                                      "dims %dx%d, imgBuf=0x%x, hFence=%d, request=%llu",
                                                      InstanceID(),
                                                      pInputPort->portId + portIndex,
                                                      pFormat->width,
                                                      pFormat->height,
                                                      pInputPort->pImageBuffer,
                                                      *(pInputPort->phFence),
                                                      requestId);
                                    }

                                    // Fill the hw format register
                                    FillHwFormat(portId, ConvertCamXToCSLLRMEFormat(pFormat->format));
                                    FillImageResolution(portId, pFormat);
                                }
                            }

                            if (CamxResultSuccess != result)
                            {
                                break;
                            }
                        }
                        else
                        {
                            // Non-First Request mode:
                            // Non-batched mode ds2 disabled: ref    = prev 0,    tar = curr 0
                            // Non-batched mode ds2 enabled : ds2ref = prev 0,    tar = curr 0
                            // Batched mode ds2 disabled    : ref    = prev[n-1], tar = curr[n-1]
                            // Batched mode ds2 enabled     : ds2ref = prev[0],   tar = curr[0]
                            if ((1 < numBatchedFrames) &&
                                (FALSE == m_ds2Enable))
                            {
                                // batched mode
                                batchFrameIndex = numBatchedFrames - 1;
                            }
                            else
                            {
                                // non-batched mode
                                batchFrameIndex = 0;
                            }

                            result = pPacket->AddIOConfig(pInputPort->pImageBuffer,
                                                          portId,
                                                          CSLIODirection::CSLIODirectionInput,
                                                          pInputPort->phFence,
                                                          numFences,
                                                          NULL,
                                                          NULL,
                                                          batchFrameIndex);

                            if (CamxResultSuccess != result)
                            {
                                break;
                            }

                            if (NULL != pFormat)
                            {
                                if (NULL != pInputPort->phFence)
                                {
                                    CAMX_LOG_INFO(CamxLogGroupLRME,
                                                  "LRME:%d reporting Input config, portId=%d, dims %dx%d, "
                                                  "imgBuf=0x%x, hFence=%d, request=%llu",
                                                  InstanceID(),
                                                  pInputPort->portId,
                                                  pFormat->width,
                                                  pFormat->height,
                                                  pInputPort->pImageBuffer,
                                                  *(pInputPort->phFence),
                                                  requestId);
                                }

                                // Fill the hw format register
                                FillHwFormat(portId, ConvertCamXToCSLLRMEFormat(pFormat->format));
                                FillImageResolution(portId, pFormat);
                            }
                        }

                        if (portId == CSLLRMEREFInput)
                        {
                            isRefValid = 1;
                            EnableRefPort(TRUE);
                        }
                    }
                }

                if (CamxResultSuccess == result)
                {
                    for (UINT outputIndex = 0; outputIndex < pPerRequestPorts->numOutputPorts; outputIndex++)
                    {
                        PerRequestOutputPortInfo* pOutputPort = &pPerRequestPorts->pOutputPorts[outputIndex];
                        if ((NULL != pOutputPort) && (NULL != pOutputPort->ppImageBuffer))
                        {
                            const ImageFormat* pFormat                      = pOutputPort->ppImageBuffer[0]->GetFormat();
                            UINT32             numFences                    = (NULL == pOutputPort->phFence) ? 0 : 1;
                            enum CSLLRMEIO     portId                       = (pOutputPort->portId == LRMEOutputPortVector) ?
                                                                                CSLLRMEVectorOutput : CSLLRMEDS2Output;
                            UINT64             requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(requestId);


                            if (((FirstValidRequestId != requestIdOffsetFromLastFlush) && (m_resetReferenceInput == FALSE)) ||
                                (CSLLRMEDS2Output == portId)       ||
                                ((1 < numBatchedFrames) && (FALSE == m_ds2Enable)))
                            {
                                result = pPacket->AddIOConfig(pOutputPort->ppImageBuffer[0],
                                                              portId,
                                                              CSLIODirection::CSLIODirectionOutput,
                                                              pOutputPort->phFence,
                                                              numFences,
                                                              NULL,
                                                              NULL,
                                                              0);

                                if (CamxResultSuccess != result)
                                {
                                    CAMX_LOG_ERROR(CamxLogGroupLRME, "LRME:%d : Failed in output AddIOConfig, result=%d",
                                                   InstanceID(), result);
                                    break;
                                }

                                if ((NULL != pOutputPort->phFence) && (NULL != pFormat))
                                {
                                    CAMX_LOG_INFO(CamxLogGroupLRME,
                                                  "LRME:%d reporting I/O config, portId=%d, imgBuf=0x%x, hFence=%d,"
                                                  " dims %dx%d request=%llu",
                                                  InstanceID(),
                                                  pOutputPort->portId,
                                                  pOutputPort->ppImageBuffer[0],
                                                  *(pOutputPort->phFence),
                                                  pFormat->width, pFormat->height,
                                                  pNodeRequestData->pCaptureRequest->requestId);
                                }

                                if (CSLLRMEVectorOutput == portId)
                                {
                                    FillHwFormat(portId, m_lrmeVectorFormat);
                                }
                                else
                                {
                                    // Enable DS2 output
                                    EnableDS2OutputPath(TRUE);
                                }
                            }
                            else
                            {
                                if (m_resetReferenceInput == TRUE)
                                {
                                    m_resetReferenceInput = FALSE;
                                }
                                if (NULL != pOutputPort->phFence)
                                {
                                    CSLFenceSignal(pOutputPort->phFence[0], CSLFenceResultSuccess);
                                }
                                else
                                {
                                    CAMX_LOG_ERROR(CamxLogGroupLRME, "LRME: %d Invalid fence signal", InstanceID());
                                }
                            }
                        }
                    }
                    if (CamxResultSuccess == result)
                    {
                        result = FillCmdBufferArray(pCmdBuffer);
                    }
                    UINT32 cmdIndex;
                    if (CamxResultSuccess == result)
                    {
                        result = pPacket->AddCmdBufferReference(pCmdBuffer, &cmdIndex);
                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupLRME, "AddCmdBufferReference fail %d", result);
                        }
                    }
                    if (CamxResultSuccess == result)
                    {
                        result = pPacket->SetKMDCmdBufferIndex(cmdIndex,
                                                               (pCmdBuffer->GetResourceUsedDwords() * sizeof(UINT32)));
                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupLRME, "SetKMDCmdBufferIndex fail %d", result);
                        }
                    }
                    if (CamxResultSuccess == result)
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupLRME, "lrme kmd buffer offset %u remain %u",
                                         pCmdBuffer->GetResourceUsedDwords(), pCmdBuffer->GetNumDwordsAvailable());
                        pPacket->SetOpcode(CSLDeviceType::CSLDeviceTypeLRME, CSLPacketOpcodesLRMEUpdate);
                        result = pPacket->CommitPacket();
                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupLRME, "CommitPacket fail %d", result);
                        }
                    }
                    if (CamxResultSuccess == result)
                    {
                        result = LRMEPostFrameSettings(isRefValid);
                    }
                    if (CamxResultSuccess == result)
                    {
                        // Packet ready submit to csl
                        result = GetHwContext()->Submit(GetCSLSession(), m_hDevice, pPacket);
                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupLRME, "Submit fail %d", result);
                        }
                    }
                }
            }
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupLRME, "ExecuteProcessRequest exit result %d", result);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LRMENode::IsNodeDisabledWithOverride
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LRMENode::IsNodeDisabledWithOverride()
{
    BOOL bMCTFDisabled = TRUE;

    if (TRUE == GetHwContext()->GetStaticSettings()->enableMCTF)
    {
        bMCTFDisabled = FALSE;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupLRME, "LRME Disabled from settings");
    }

    return bMCTFDisabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LRMENode::LRMEPostFrameSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult LRMENode::LRMEPostFrameSettings(
    INT refValid)
{
    CAMX_LOG_INFO(CamxLogGroupPProc, "Post frame settings with refValid  = %d", refValid);

    LRMEPropertyFrameSettings              frameSettings;
    LRME_LRME_CLC_RANGESTEP                clcRangeStep;
    clcRangeStep.u32All                  = m_lrmeRegistersValue[lrmeCLCModuleRangeStep];
    LRME_LRME_CLC_TAR_HEIGHT               clcTarHeight;
    clcTarHeight.u32All                  = m_lrmeRegistersValue[lrmeCLCModuleTarHeight];
    LRME_LRME_CLC_TAR_PD_UNPACKER          clcTarPdUnpacker;
    clcTarPdUnpacker.u32All              = m_lrmeRegistersValue[lrmeCLCModuleTarPdUnpacker];
    LRME_LRME_CLC_OFFSET                   clcOffset;
    clcOffset.u32All                     = m_lrmeRegistersValue[lrmeCLCModuleOffset];
    LRME_LRME_CLC_MODULE_CFG               clcModuleConfig;
    clcModuleConfig.u32All               = m_lrmeRegistersValue[lrmeCLCModuleConfig];

    frameSettings.LRMEStepX              = clcRangeStep.bitfields.STEPX;
    frameSettings.LRMEStepY              = clcRangeStep.bitfields.STEPY;
    frameSettings.LRMERefValid           = refValid;
    frameSettings.LRMEresultFormat       = 0;
    frameSettings.LRMETarH               = clcTarHeight.bitfields.TAR_HEIGHT;
    if (m_selectedREFPort == LRMEInputPortREFLRMEDS2)
    {
        // Ransac/Nclib expect DS8 dimension for TarW
        frameSettings.LRMETarW               = clcTarPdUnpacker.bitfields.TAR_PD_UNPACKER_LINEWIDTH/2;
    }
    else
    {
        frameSettings.LRMETarW               = clcTarPdUnpacker.bitfields.TAR_PD_UNPACKER_LINEWIDTH;
    }
    frameSettings.LRMETarOffsetX           = clcOffset.bitfields.OFFSETX;
    frameSettings.LRMETarOffsetY           = clcOffset.bitfields.OFFSETY;
    frameSettings.LRMERefOffsetX           = clcOffset.bitfields.OFFSETX;
    frameSettings.LRMERefOffsetY           = clcOffset.bitfields.OFFSETY;
    frameSettings.LRMEsubpelSearchEnable   = clcModuleConfig.bitfields.SUBPELSEARCHENABLE;
    frameSettings.fullHeight               = m_fullInputHeight;
    frameSettings.fullWidth                = m_fullInputWidth;
    frameSettings.alternateSkipProcessing  = m_alternateSkipProcessing;

    switch (m_selectedTARPort)
    {
        case LRMEInputPortTARIFEFull:
            frameSettings.LRMEUpscaleFactor = 1;
            break;
        case LRMEInputPortTARIFEDS4:
            if (m_selectedREFPort == LRMEInputPortREFLRMEDS2)
            {
                frameSettings.LRMEUpscaleFactor = 8;
            }
            else
            {
                frameSettings.LRMEUpscaleFactor = 4;
            }
            break;
        case LRMEInputPortTARIFEDS16:
        default:
            frameSettings.LRMEUpscaleFactor = 16;
            break;
    }

    if ((0 == m_fullInputHeight) || (0 == m_fullInputWidth))
    {
        CAMX_LOG_WARN(CamxLogGroupLRME, "Full Input Port not connected!! Connect full port for better results");
        frameSettings.fullHeight             = frameSettings.LRMETarH * frameSettings.LRMEUpscaleFactor;
        frameSettings.fullWidth              = frameSettings.LRMETarW * frameSettings.LRMEUpscaleFactor;
    }
    else
    {
        frameSettings.fullHeight             = m_fullInputHeight;
        frameSettings.fullWidth              = m_fullInputWidth;
    }

    // Post the property
    const VOID* ppData[1]     = { 0 };
    UINT        pDataCount[1] = { 0 };
    UINT        index = 0;

    pDataCount[index] = sizeof(frameSettings);
    ppData[index]     = &frameSettings;
    static const UINT PropertiesLRMEFrameSetting[] = { PropertyIDLRMEFrameSettings };
    return WriteDataList(PropertiesLRMEFrameSetting, ppData, pDataCount, 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LRMENode::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID LRMENode::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LRMENode::GetCSLLRMEFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 LRMENode::GetCSLLRMEFormat(
    const CamX::Format fmt)
{
    UINT32 result = 0;
    switch (fmt)
    {
        case CamX::Format::Y8:
            result = CSLLRMEFormatY8;
            break;
        case CamX::Format::PD10:
            result = CSLLRMEFormatPD10;
            break;
        case CamX::Format::YUV420NV12:
            result = CSLLRMEFormatNV12;
            break;
        case CamX::Format::P010:
            result = CSLLRMEFormatY10;
            break;
        default:
            break;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LRMENode::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult LRMENode::PostPipelineCreate()
{
    CamxResult                result                             = CamxResultSuccess;
    CSLLRMEAcquireDeviceInfo* pLRMEResource                      = NULL;
    UINT                      FPS                                = 30;
    MetadataSlot*             pSlot                              = GetPerFramePool(PoolType::PerUsecase)->GetSlot(0);
    const ImageFormat*        pImageFormat                       = NULL;
    UINT                      numInputPort                       = 0;
    UINT                      inputPortId[LRMEMaxIntputPorts];
    UINT                      numOutputPort                      = 0;
    UINT                      outputPortId[LRMEMaxOutputPorts];
    ResourceParams            params                             = { 0 };

    if (LRME_NODE_INITIALIZED != m_state)
    {
        CAMX_LOG_ERROR(CamxLogGroupLRME, "LRME in invalid state %d", m_state);
        result = CamxResultEFailed;
    }

    if (CamxResultSuccess == result)
    {
        m_numCmdBuffer               = GetPipeline()->GetRequestQueueDepth();
        m_numPacketBuffer            = GetPipeline()->GetRequestQueueDepth();
        m_alternateSkipProcessing    = IsSkipAlternateLRMEProcessing();

        if (CamxResultSuccess == result)
        {
            // Command buffer for request packet
            params.usageFlags.cmdBuffer        = 1;
            params.cmdParams.maxNumNestedAddrs = 0;
            params.cmdParams.type              = CmdType::CDMDirect;
            params.memFlags                    = (CSLMemFlagKMDAccess | CSLMemFlagUMDAccess);
            params.resourceSize                = m_cmdBufferSize;
            params.poolSize                    = params.resourceSize * m_numCmdBuffer;
            params.numDevices                  = 1;
            params.pDeviceIndices              = DeviceIndices();
            params.alignment                   = CamxCommandBufferAlignmentInBytes;
            result                             = CreateCmdBufferManager("CmdBufferManager", &params,
                                                                        &m_pLRMECmdBufferManager[lrmeCmdBufferManager]);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupLRME, "Failed CreateCmdBufferManager %d", result);
                Cleanup();
            }
        }

        if (CamxResultSuccess == result)
        {
            // Packet buffer request packet
            params                               = { 0 };
            params.usageFlags.packet             = 1;
            params.packetParams.maxNumCmdBuffers = 1;
            params.memFlags                      = (CSLMemFlagKMDAccess | CSLMemFlagUMDAccess);
            params.packetParams.maxNumIOConfigs  = 4;
            params.packetParams.maxNumCmdBuffers = 1;
            params.resourceSize                  = Packet::CalculatePacketSize(&params.packetParams);
            params.poolSize                      = params.resourceSize * m_numPacketBuffer;
            params.numDevices                    = 1;
            params.pDeviceIndices                = DeviceIndices();
            params.alignment                     = CamxPacketAlignmentInBytes;
            result                               = CreateCmdBufferManager("PacketManager", &params,
                                                                          &m_pLRMECmdBufferManager[lrmePacketManager]);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupLRME, "Failed CreateCmdBufferManager for packets %d", result);
                Cleanup();
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        static const UINT FPSTag[]      = { PropertyIDUsecaseFPS };
        VOID*             pData[1]      = { 0 };
        UINT64            dataOffset[1] = { 0 };

        GetDataList(FPSTag, pData, dataOffset, 1);

        if (NULL != pData[0])
        {
            FPS = *(static_cast<UINT*>(pData[0]));
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupLRME, "FPS: not published");
        }
    }

    pLRMEResource = static_cast<CSLLRMEAcquireDeviceInfo*>(CAMX_CALLOC(sizeof(CSLLRMEAcquireDeviceInfo)));
    if (NULL == pLRMEResource)
    {
        result = CamxResultENoMemory;
    }
    if (CamxResultSuccess == result)
    {
        pLRMEResource->ds2Enable = m_ds2Enable;
        pLRMEResource->fps = FPS;

        GetAllOutputPortIds(&numOutputPort, &outputPortId[0]);
        for (UINT outputIndex = 0; outputIndex < numOutputPort; outputIndex++)
        {
            switch (outputPortId[outputIndex])
            {
                case LRMEOutputPortVector:
                    pImageFormat = GetOutputPortImageFormat(outputIndex);
                    if (NULL != pImageFormat)
                    {
                        pLRMEResource->ioFormat[CSLLRMEVectorOutput].width  = pImageFormat->width;
                        pLRMEResource->ioFormat[CSLLRMEVectorOutput].height = pImageFormat->height;
                        pLRMEResource->ioFormat[CSLLRMEVectorOutput].format = CSLLRMEVectorOutputShort;
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupLRME, "pImageFormat is NULL");
                        result = CamxResultENoMemory;
                    }
                    break;
                default:
                    break;
            }
        }

        GetAllInputPortIds(&numInputPort, &inputPortId[0]);

        for (UINT inputIndex = 0; inputIndex < numInputPort; inputIndex++)
        {
            if (m_selectedTARPort == inputPortId[inputIndex])
            {
                pImageFormat = GetInputPortImageFormat(inputIndex);
                if (NULL != pImageFormat)
                {
                    pLRMEResource->ioFormat[CSLLRMETARInput].width = pImageFormat->width;
                    pLRMEResource->ioFormat[CSLLRMETARInput].height = pImageFormat->height;
                    pLRMEResource->ioFormat[CSLLRMETARInput].format = GetCSLLRMEFormat(pImageFormat->format);
                }
            }
            else if (m_selectedREFPort == inputPortId[inputIndex])
            {
                pImageFormat = GetInputPortImageFormat(inputIndex);
                if (NULL != pImageFormat)
                {
                    pLRMEResource->ioFormat[CSLLRMEREFInput].width = pImageFormat->width;
                    pLRMEResource->ioFormat[CSLLRMEREFInput].height = pImageFormat->height;
                    pLRMEResource->ioFormat[CSLLRMEREFInput].format = GetCSLLRMEFormat(pImageFormat->format);
                }
            }
        }

        CSLDeviceResource deviceResourceRequest;
        deviceResourceRequest.resourceID = 0;
        deviceResourceRequest.pDeviceResourceParam = pLRMEResource;
        deviceResourceRequest.deviceResourceParamSize = sizeof(CSLLRMEAcquireDeviceInfo);

        result = CSLAcquireDevice(GetCSLSession(),
                                  &m_hDevice,
                                  DeviceIndices()[0],
                                  &deviceResourceRequest,
                                  1,
                                  NULL,
                                  0,
                                  NodeIdentifierString());
        CAMX_FREE(deviceResourceRequest.pDeviceResourceParam);
    }
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupLRME, "Acquire LRME Device Failed");
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupLRME, "Acquire LRME Device success handle %d", m_hDevice);
        AddCSLDeviceHandle(m_hDevice);
        m_state = LRME_NODE_ACTIVE;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LRMENode::FillHwFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID LRMENode::FillHwFormat(
    enum CSLLRMEIO port,
    UINT32 format)
{
    LRME_LRME_CLC_MODULEFORMAT clcModuleFormat;
    clcModuleFormat.u32All = m_lrmeRegistersValue[lrmeCLCModuleFormat];

    switch (port)
    {
        case CSLLRMETARInput:
            clcModuleFormat.bits.TARDATAFORMAT = ConvertCSLLRMEToLRMEHwFormat(format);
            break;
        case CSLLRMEREFInput:
            clcModuleFormat.bits.REFDATAFORMAT = ConvertCSLLRMEToLRMEHwFormat(format);
            break;
        case CSLLRMEVectorOutput:
            clcModuleFormat.bits.RESULTSFORMAT = m_lrmeVectorFormat;
            break;
        default:
            break;
    }
    m_lrmeRegistersValue[lrmeCLCModuleFormat] = clcModuleFormat.u32All;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LRMENode::FillImageResolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID LRMENode::FillImageResolution(
    enum CSLLRMEIO port,
    const ImageFormat* pFormat)
{
    switch (port)
    {
        case CSLLRMETARInput:
            LRME_LRME_CLC_TAR_HEIGHT clcTarHeight;
            clcTarHeight.u32All = m_lrmeRegistersValue[lrmeCLCModuleTarHeight];
            LRME_LRME_CLC_TAR_PD_UNPACKER clcTarPdUnpacker;
            clcTarPdUnpacker.u32All = m_lrmeRegistersValue[lrmeCLCModuleTarPdUnpacker];
            if (LRMEInputPortREFLRMEDS2 == m_selectedREFPort)
            {
                clcTarPdUnpacker.bitfields.TAR_PD_UNPACKER_LINEWIDTH = pFormat->width;
                clcTarHeight.bitfields.TAR_HEIGHT = pFormat->height / 2;
            }
            else
            {
                clcTarPdUnpacker.bitfields.TAR_PD_UNPACKER_LINEWIDTH = pFormat->width;
                clcTarHeight.bitfields.TAR_HEIGHT = pFormat->height;
            }

            m_lrmeRegistersValue[lrmeCLCModuleTarHeight] = clcTarHeight.u32All;
            m_lrmeRegistersValue[lrmeCLCModuleTarPdUnpacker] = clcTarPdUnpacker.u32All;
            break;
        case CSLLRMEREFInput:
            LRME_LRME_CLC_REF_HEIGHT clcRefHeight;
            clcRefHeight.u32All = m_lrmeRegistersValue[lrmeCLCModuleRefHeight];
            LRME_LRME_CLC_REF_PD_UNPACKER clcRefPdUnpacker;
            clcRefPdUnpacker.u32All = m_lrmeRegistersValue[lrmeCLCModuleRefPdUnpacker];
            clcRefPdUnpacker.bitfields.REF_PD_UNPACKER_LINEWIDTH = pFormat->width;
            clcRefHeight.bitfields.REF_HEIGHT = pFormat->height;

            m_lrmeRegistersValue[lrmeCLCModuleRefHeight] = clcRefHeight.u32All;
            m_lrmeRegistersValue[lrmeCLCModuleRefPdUnpacker] = clcRefPdUnpacker.u32All;
            break;
        default:
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LRMENode::EnableDS2OutputPath
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID LRMENode::EnableDS2OutputPath(
    BOOL en)
{
    LRME_LRME_CLC_MODULE_CFG clcModConfig;
    clcModConfig.u32All = m_lrmeRegistersValue[lrmeCLCModuleConfig];
    if (TRUE == en)
    {
        clcModConfig.bitfields.DODOWNSCALING = 1;
    }
    else
    {
        clcModConfig.bitfields.DODOWNSCALING = 0;
    }
    m_lrmeRegistersValue[lrmeCLCModuleConfig] = clcModConfig.u32All;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LRMENode::EnableRefPort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID LRMENode::EnableRefPort(
    BOOL en)
{
    LRME_LRME_CLC_MODULE_CFG clcModConfig;
    clcModConfig.u32All = m_lrmeRegistersValue[lrmeCLCModuleConfig];
    if (TRUE == en)
    {
        clcModConfig.bitfields.ISREFVALID = 1;
    }
    else
    {
        clcModConfig.bitfields.ISREFVALID = 0;
    }
    m_lrmeRegistersValue[lrmeCLCModuleConfig] = clcModConfig.u32All;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LRMENode::ReleaseDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult LRMENode::ReleaseDevice()
{
    CamxResult result = CamxResultSuccess;

    if (NULL != GetHwContext() && -1 != m_hDevice)
    {
        result = CSLReleaseDevice(GetCSLSession(), m_hDevice);

        if (CamxResultSuccess != result)
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Failed to release device");
        }
    }
    m_hDevice = CSLInvalidHandle;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LRMENode::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult LRMENode::QueryMetadataPublishList(
    NodeMetadataList* pPublistTagList)
{
    pPublistTagList->tagCount = CAMX_ARRAY_SIZE(sLRMEPublishList);
    for (UINT32 count = 0; count < pPublistTagList->tagCount; ++count)
    {
        pPublistTagList->tagArray[count] = sLRMEPublishList[count];
    }
    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LRMENode::NotifyRequestProcessingError
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID LRMENode::NotifyRequestProcessingError(
    NodeFenceHandlerData* pFenceHandlerData,
    UINT                  unSignaledFenceCount)
{
    CAMX_UNREFERENCED_PARAM(unSignaledFenceCount);
    CAMX_ASSERT(NULL != pFenceHandlerData);
    OutputPort*     pOutputPort = pFenceHandlerData->pOutputPort;
    CSLFenceResult  fenceResult = pFenceHandlerData->fenceResult;

    if (CSLFenceResultSuccess != fenceResult)
    {
        if (LRMEOutputPortDS2 == pOutputPort->portId)
        {
            if (CSLFenceResultFailed == fenceResult)
            {
                CAMX_LOG_ERROR(CamxLogGroupLRME, "Fence failure for output port %d req %llu",
                    pOutputPort->portId, pFenceHandlerData->requestId);
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupLRME, "Fence failure during flush for output port %d req %llu",
                    pOutputPort->portId, pFenceHandlerData->requestId);
            }

            m_resetReferenceInput = TRUE;
        }
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LRMENode::IsSkipAlternateLRMEProcessing
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LRMENode::IsSkipAlternateLRMEProcessing()
{
    CamxResult             result                            = CamxResultSuccess;
    UINT32                 metaTagICACapabilities            = 0;
    UINT32                 transformType                     = 0;
    IPEICACapability       icaCapability                     = { 0 };
    UINT                   fps                               = 0;
    CHAR                   GPUSkipProcess[]                  = "512";
    BOOL                   isLRMEUsageLimited                = FALSE;

    result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.platformCapabilities",
        "IPEICACapabilities", &metaTagICACapabilities);

    if (CamxResultSuccess == result)
    {
        UINT GetProps[] =
        {
            metaTagICACapabilities | StaticMetadataSectionMask,
            PropertyIDUsecaseFPS,
        };

        static const UINT GetPropsLength           = CAMX_ARRAY_SIZE(GetProps);
        VOID*             pData[GetPropsLength]    = { 0 };
        UINT64            offsets[GetPropsLength]  = { 0 };

        GetDataList(GetProps, pData, offsets, GetPropsLength);
        if (NULL != pData[0])
        {
            Utils::Memcpy(&icaCapability, pData[0], sizeof(icaCapability));
            transformType = icaCapability.supportedIPEICATransformType;
        }
        if (NULL != pData[1])
        {
            fps = *reinterpret_cast<UINT*>(pData[1]);
        }
    }

    if ((TRUE == GetPipeline()->IsNodeExistByNodePropertyValue(GPUSkipProcess)) &&
       (static_cast<UINT32>(CamX::IPEICATransformType::DefaultTransform) !=  transformType) &&
       (60 == fps))
    {
        isLRMEUsageLimited = TRUE;
    }

    return isLRMEUsageLimited;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LRMENode::SkipandSignalLRMEfences
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LRMENode::SkipandSignalLRMEfences(
    NodeProcessRequestData*    pNodeRequestData,
    PerRequestActivePorts*     pPerRequestPorts,
    UINT64                     requestId,
    INT                        isRefValid)
{
    CamxResult                 result                             = CamxResultSuccess;
    pNodeRequestData->numDependencyLists = 0;
    for (UINT portIndex = 0; portIndex < pPerRequestPorts->numInputPorts; portIndex++)
    {
        PerRequestInputPortInfo* pPerRequestInputPort = &pPerRequestPorts->pInputPorts[portIndex];
        if (pPerRequestInputPort->portId != m_selectedTARPort)
        {
            // No need to wait for ref port as it is already signalled
            continue;
        }
        enum CSLLRMEIO portId      = CSLLRMETARInput;
        const ImageFormat* pFormat = pPerRequestInputPort->pImageBuffer->GetFormat();
        if (NULL != pFormat)
        {
            FillImageResolution(portId, pFormat);
        }
    }
    result = LRMEPostFrameSettings(isRefValid);
    if (CamxResultSuccess == result)
    {
        for (UINT portIndex = 0; portIndex < pPerRequestPorts->numOutputPorts; portIndex++)
        {
            PerRequestOutputPortInfo* pPerRequestOutputPort = &pPerRequestPorts->pOutputPorts[portIndex];
            CAMX_LOG_VERBOSE(CamxLogGroupLRME, "Request %lld, Signal ouput Fence %d",
                requestId, pPerRequestOutputPort->phFence[0]);
            CSLFenceSignal(pPerRequestOutputPort->phFence[0], CSLFenceResultSuccess);
        }
    }
    return result;
}

CAMX_NAMESPACE_END
