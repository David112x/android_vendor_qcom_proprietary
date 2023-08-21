////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxtitan17xfactory.cpp
/// @brief Titan17xFactory class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// CamX Includes
#include "camxautofocusnode.h"
#include "camxawbnode.h"
#include "camxbpsnode.h"
#include "camxfdhwnode.h"
#include "camxfdmanagernode.h"
#include "camxifenode.h"
#include "camxincs.h"
#include "camxipenode.h"
#include "camxjpegencnode.h"
#include "camxmem.h"
#include "camxnode.h"
#include "camxstatsprocessingnode.h"
#include "camxjpegaggrnode.h"
#include "camxchinodewrapper.h"
#include "camxofflinestatsnode.h"
#include "camxstatsparsenode.h"
#include "camxlrmenode.h"
#if CVPENABLED
#include "camxcvpnode.h"
#endif // CVPENABLED
#include "camxransacnode.h"
#include "camxhistogramprocessnode.h"
#include "camxtrackernode.h"

// HWL Includes
#include "camxtitan17xdefs.h"
#include "camxtitan17xfactory.h"
#include "camxtitan17xsettingsmanager.h"

// HWL Node Includes
#include "camximagesensordata.h"
#include "camxactuatordata.h"
#include "camxoisdata.h"
#include "camxpdafdata.h"
#include "camxpdafconfig.h"
#include "camxsensornode.h"
#include "camxtorchnode.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xFactory::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HwFactory* Titan17xFactory::Create()
{
    HwFactory* pTitan17xFactory = CAMX_NEW Titan17xFactory;
    if (NULL == pTitan17xFactory)
    {
        CAMX_LOG_ERROR(CamxLogGroupHWL, "Out of memory; cannot create Titan17xFactory");
    }

    return pTitan17xFactory;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xFactory::Titan17xFactory
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Titan17xFactory::Titan17xFactory()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xFactory::~Titan17xFactory
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Titan17xFactory::~Titan17xFactory()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xFactory::HwCreateNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Node* Titan17xFactory::HwCreateNode(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData
    ) const
{
    Node* pNode = NULL;

    switch (pCreateInputData->pNodeInfo->nodeId)
    {
        case AutoFocus:
            pNode = AutoFocusNode::Create(pCreateInputData, pCreateOutputData);
            break;
        case AutoWhiteBalance:
            pNode = AWBNode::Create(pCreateInputData, pCreateOutputData);
            break;
        case BPS:
            pNode = BPSNode::Create(pCreateInputData, pCreateOutputData);
            break;
        case IFE:
            pNode = IFENode::Create(pCreateInputData, pCreateOutputData);
            break;
        case IPE:
            pNode = IPENode::Create(pCreateInputData, pCreateOutputData);
            break;
        case Sensor:
            pNode = SensorNode::Create(pCreateInputData, pCreateOutputData);
            break;
        case StatsProcessing:
            pNode = StatsProcessingNode::Create(pCreateInputData, pCreateOutputData);
            break;
        case JPEG:
            pNode = JPEGEncNode::Create(pCreateInputData, pCreateOutputData);
            break;
        case JPEGAggregator:
            pNode = JPEGAggrNode::Create(pCreateInputData, pCreateOutputData);
            break;
        case StatsParse:
            pNode = StatsParseNode::Create(pCreateInputData, pCreateOutputData);
            break;
        case ChiExternalNode:
            pNode = ChiNodeWrapper::Create(pCreateInputData, pCreateOutputData);
            break;
#if (!defined(LE_CAMERA)) // FD disabled LE_CAMERA
        case FDHw:
            pNode = FDHwNode::Create(pCreateInputData, pCreateOutputData);
            break;
        case FDManager:
            pNode = FDManagerNode::Create(pCreateInputData, pCreateOutputData);
            break;
#endif // FD disabled LE_CAMERA
        case Tracker:
            pNode = TrackerNode::Create(pCreateInputData, pCreateOutputData);
            break;
        case OfflineStats:
            pNode = OfflineStatsNode::Create(pCreateInputData, pCreateOutputData);
            break;
        case Torch:
            pNode = TorchNode::Create(pCreateInputData, pCreateOutputData);
            break;
        case LRME:
            pNode = LRMENode::Create(pCreateInputData, pCreateOutputData);
            break;
        case RANSAC:
            pNode = RANSACNode::Create(pCreateInputData, pCreateOutputData);
            break;
        case HistogramProcess:
            pNode = HistogramProcessNode::Create(pCreateInputData, pCreateOutputData);
            break;
#if CVPENABLED
        case CVP:
            pNode = CVPNode::Create(pCreateInputData, pCreateOutputData);
            break;
#endif // CVPENABLED
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Unexpected node type");
            break;
    }

    return pNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xFactory::HwCreateSettingsManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SettingsManager* Titan17xFactory::HwCreateSettingsManager() const
{
    return Titan17xSettingsManager::Create();
}

CAMX_NAMESPACE_END
