////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxtorchnode.cpp
/// @brief TorchNode class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxflash.h"
#include "camxhwcontext.h"
#include "camxpipeline.h"
#include "camxtorchnode.h"
#include "camxtrace.h"
#include "camxvendortags.h"
#include "chi.h"
#include "chipdlibinterface.h"
#include "camxsensornode.h"

CAMX_NAMESPACE_BEGIN

static const UINT TorchMaxCmdBufferManagerCount = 9;                   ///< Number of max command buffer managers
static const UINT InitialConfigCmdCount         = 1;                   ///< Number of command buffers in config command
static const UINT I2CInfoCmdCount               = 1;                   ///< Number of command buffers in I2C command

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TorchNode::TorchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TorchNode::TorchNode()
{
    m_derivedNodeHandlesMetaDone = TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TorchNode::~TorchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TorchNode::~TorchNode()
{
    UINT cameraCloseStatus = GetStaticSettings()->overrideCameraClose;

    if (NULL != m_pFlash)
    {
        m_pFlash->Destroy();
        m_pFlash = NULL;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TorchNode::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TorchNode* TorchNode::Create(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateInputData);
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    return CAMX_NEW TorchNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TorchNode::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TorchNode::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TorchNode::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TorchNode::ProcessingNodeInitialize(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateInputData);
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TorchNode::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TorchNode::PostPipelineCreate()
{
    CamxResult result = CamxResultSuccess;

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TorchNode::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TorchNode::ExecuteProcessRequest(
    ExecuteProcessRequestData* pExecuteProcessRequestData)
{
    CAMX_UNREFERENCED_PARAM(pExecuteProcessRequestData);

    CamxResult result      = CamxResultSuccess;

    CAMX_ASSERT(NULL != m_pFlash);

    result = m_pFlash->ExecuteProcessRequest(pExecuteProcessRequestData, FlashInfoTypeMain, NULL, NULL);

    UINT64 requestId = pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId;

    ProcessPartialMetadataDone(requestId);
    ProcessMetadataDone(requestId);
    ProcessRequestIdDone(requestId);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TorchNode::ProcessingNodeFinalizeInitialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TorchNode::ProcessingNodeFinalizeInitialization(
    FinalizeInitializationData* pFinalizeInitializationData)
{
    CAMX_UNREFERENCED_PARAM(pFinalizeInitializationData);

    CamxResult        result                     = CamxResultSuccess;
    CmdBufferManager* pFlashPacketManager        = NULL;
    CmdBufferManager* pFlashInitializeCmdManager = NULL;
    CmdBufferManager* pFlashPowerCmdManager      = NULL;
    CmdBufferManager* pFlashI2CCmdManager        = NULL;
    CmdBufferManager* pFlashI2CInitCmdManager    = NULL;
    CmdBufferManager* pFlashFireCmdManager       = NULL;
    CmdBufferManager* pFlashI2CFireCmdManager    = NULL;
    CmdBufferManager* pFlashRERCmdManager        = NULL;
    CmdBufferManager* pFlashQueryCmdManager      = NULL;
    ResourceParams    packetResourceParams       = { 0 };
    UINT32            requestQueueDepth          = GetPipeline()->GetRequestQueueDepth();
    FlashDriverType   flashType                  = FlashDriverType::PMIC;

    m_pHwContext = GetHwContext();
    m_cameraId   = GetPipeline()->GetCameraId();

    // NOWHINE CP036a: Since the function is const, had to add the const_cast
    ImageSensorModuleData* pSensorModuleData     = const_cast<ImageSensorModuleData*>
                                                        (m_pHwContext->GetImageSensorModuleData(m_cameraId));

    result = InitializeCmdBufferManagerList(TorchMaxCmdBufferManagerCount);

    packetResourceParams.usageFlags.packet             = 1;
    packetResourceParams.packetParams.maxNumCmdBuffers = FlashCmdCount;
    packetResourceParams.packetParams.maxNumIOConfigs  = 0;
    packetResourceParams.packetParams.maxNumPatches    = 0;
    packetResourceParams.resourceSize                  = Packet::CalculatePacketSize(&packetResourceParams.packetParams);
    packetResourceParams.poolSize                      = (requestQueueDepth * packetResourceParams.resourceSize);
    packetResourceParams.alignment                     = CamxPacketAlignmentInBytes;
    packetResourceParams.pDeviceIndices                = NULL;
    packetResourceParams.numDevices                    = 0;
    packetResourceParams.memFlags                      = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

    result = CreateCmdBufferManager("FlashPacketManager", &packetResourceParams, &pFlashPacketManager);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Fail to create FlashPacketManager, result %d", result);
    }

    if (CamxResultSuccess == result)
    {
        ResourceParams cmdResourceParams ={ 0 };

        cmdResourceParams.resourceSize         = sizeof(CSLFlashInfoCmd);
        cmdResourceParams.poolSize             = sizeof(CSLFlashInfoCmd);
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::Generic;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("FlashInitializeCmdManager", &cmdResourceParams, &pFlashInitializeCmdManager);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Fail to create FlashInitializeCmdManager, result %d", result);
        }
    }

    if (NULL != pSensorModuleData->GetFlashDataObject())
    {
        flashType = pSensorModuleData->GetFlashDataObject()->GetFlashType();
    }

    if ((CamxResultSuccess == result) && (FlashDriverType::I2C == flashType))
    {
        UINT i2cInfoCmdSize                    = sizeof(CSLSensorI2CInfo);
        ResourceParams cmdResourceParams       = {0};

        cmdResourceParams.resourceSize         = i2cInfoCmdSize;
        cmdResourceParams.poolSize             = I2CInfoCmdCount * i2cInfoCmdSize;
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::Generic;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("FlashI2CInfoCmdManager", &cmdResourceParams, &pFlashI2CCmdManager);
    }

    if ((CamxResultSuccess == result) && (FlashDriverType::I2C == flashType))
    {
        UINT powerUpCmdSize                    = pSensorModuleData->GetFlashDataObject()->GetPowerSequenceCmdSize(TRUE);
        UINT powerDownCmdSize                  = pSensorModuleData->GetFlashDataObject()->GetPowerSequenceCmdSize(FALSE);
        UINT powerSequenceSize                 = (powerUpCmdSize + powerDownCmdSize);
        ResourceParams cmdResourceParams       = { 0 };

        cmdResourceParams.resourceSize         = powerSequenceSize;
        cmdResourceParams.poolSize             = cmdResourceParams.resourceSize;
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::Generic;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("FlashPowerCmdManager", &cmdResourceParams, &pFlashPowerCmdManager);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Fail to create FlashPowerCmdManager, result %d", result);
        }
    }

    if ((CamxResultSuccess == result) && (FlashDriverType::I2C == flashType))
    {
        UINT i2cInitializeCmdSize              = pSensorModuleData->GetFlashDataObject()->GetI2CInitializeCmdSize();
        ResourceParams cmdResourceParams       = {0};

        cmdResourceParams.resourceSize         = i2cInitializeCmdSize;
        cmdResourceParams.poolSize             = InitialConfigCmdCount * i2cInitializeCmdSize;
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::I2C;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("FlashI2CInitializeCmdManager", &cmdResourceParams, &pFlashI2CInitCmdManager);
    }

    if (CamxResultSuccess == result)
    {
        ResourceParams cmdResourceParams ={ 0 };

        cmdResourceParams.resourceSize         = sizeof(CSLFlashFireCmd);
        cmdResourceParams.poolSize             = (requestQueueDepth * cmdResourceParams.resourceSize);
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::Generic;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("FlashFireCmdManager", &cmdResourceParams, &pFlashFireCmdManager);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Fail to create FlashFireCmdManager, result %d", result);
        }
    }

    if (CamxResultSuccess == result)
    {
        ResourceParams cmdResourceParams ={ 0 };

        cmdResourceParams.resourceSize         = sizeof(CSLFlashQueryCurrentCmd);
        cmdResourceParams.poolSize             = (requestQueueDepth * cmdResourceParams.resourceSize);
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::Generic;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("FlashQueryCmdManager", &cmdResourceParams, &pFlashQueryCmdManager);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Fail to create FlashQueryCmdManager, result %d", result);
        }
    }

    if ((CamxResultSuccess == result) && (FlashDriverType::I2C == flashType))
    {
        UINT           i2cFireCmdSize          = pSensorModuleData->GetFlashDataObject()->GetI2CFireMaxCmdSize();
        ResourceParams cmdResourceParams       = {0};

        cmdResourceParams.resourceSize         = i2cFireCmdSize;
        cmdResourceParams.poolSize             = InitialConfigCmdCount * i2cFireCmdSize;
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::I2C;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("FlashI2CFireCmdManager", &cmdResourceParams, &pFlashI2CFireCmdManager);
    }

    if (CamxResultSuccess == result)
    {
        ResourceParams cmdResourceParams ={ 0 };

        cmdResourceParams.resourceSize         = sizeof(CSLFlashRERCmd);
        cmdResourceParams.poolSize             = sizeof(CSLFlashRERCmd);
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::Generic;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       =  NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("FlashRERCmdManager", &cmdResourceParams, &pFlashRERCmdManager);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Fail to create FlashRERCmdManager")
        }
    }

    if (CamxResultSuccess == result)
    {
        FlashCreateData createData ={ 0 };

        createData.pHwContext            = m_pHwContext;
        createData.pPacketManager        = pFlashPacketManager;
        createData.pI2CCmdManager        = pFlashI2CCmdManager;
        createData.pInitializeCmdManager = pFlashInitializeCmdManager;
        createData.pFlashPowerCmdManager = pFlashPowerCmdManager;
        createData.pI2CInitCmdManager    = pFlashI2CInitCmdManager;
        createData.pFireCmdManager       = pFlashFireCmdManager;
        createData.pI2CFireCmdManager    = pFlashI2CFireCmdManager;
        createData.pRERCmdManager        = pFlashRERCmdManager;
        createData.pQueryCmdManager      = pFlashQueryCmdManager;
        createData.pParentNode           = this;
        createData.cameraId              = m_cameraId;
        createData.operationMode         = CSLNonRealtimeOperation;

        result = Flash::Create(&createData);
        if (CamxResultSuccess == result)
        {
            m_pFlash = createData.pFlash;
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Flash resources initialized");
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Flash resources initialization failed");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TorchNode::FinalizeBufferProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TorchNode::FinalizeBufferProperties(
    BufferNegotiationData* pBufferNegotiationData)
{
    CAMX_UNREFERENCED_PARAM(pBufferNegotiationData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TorchNode::PrepareStreamOn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TorchNode::PrepareStreamOn()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

CAMX_NAMESPACE_END
