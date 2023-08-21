////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxactuator.cpp
/// @brief Implements Actuator methods.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxincs.h"
#include "camxmem.h"
#include "camxcmdbuffer.h"
#include "camxcmdbuffermanager.h"
#include "camxcslsensordefs.h"
#include "camxhal3module.h"
#include "camxhwcontext.h"
#include "camxhwdefs.h"
#include "camximagesensormoduledata.h"
#include "camxpacket.h"
#include "camxactuator.h"
#include "camxsensornode.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Actuator::Actuator
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Actuator::Actuator(
    HwContext*    pHwContext,
    ActuatorData* pData)
{
    m_pHwContext           = pHwContext;
    m_pActuatorData        = pData;
    m_hActuatorDevice      = CSLInvalidHandle;
    m_initialConfigPending = TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Actuator::~Actuator
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Actuator::~Actuator()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Actuator::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Actuator::Create(
    ActuatorCreateData* pCreateData,
    INT32               actuatorDeviceIndex)
{
    CamxResult    result        = CamxResultEFailed;
    ActuatorData* pActuatorData = NULL;

    if (NULL != pCreateData)
    {
        HwContext* pHwContext = pCreateData->pHwContext;
        if (NULL != pHwContext)
        {
            // NOWHINE CP036a: exception
            pActuatorData = const_cast<ImageSensorModuleData*>(
                pHwContext->GetImageSensorModuleData(pCreateData->cameraId))->GetActuatorDataObject();

            if (NULL == pActuatorData)
            {
                CAMX_LOG_WARN(CamxLogGroupSensor, "ActuatorData is NULL");
                result = CamxResultEInvalidPointer;
            }
            else
            {
                pCreateData->pActuator = CAMX_NEW Actuator(pHwContext, pActuatorData);
                if (NULL != pCreateData->pActuator)
                {
                    result = pCreateData->pActuator->Initialize(pCreateData, actuatorDeviceIndex);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "No memory for Actuator creation!");
                    result = CamxResultENoMemory;
                }
            }
        }

    }

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_INFO(CamxLogGroupSensor, "Actuator created successfully!");
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Actuator unavailable or initialization failed: %d!", result);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Actuator::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Actuator::Destroy()
{
    CamxResult    result        = CamxResultEFailed;
    MetadataPool* pMainPool     = NULL;

    if (NULL != m_pParentNode)
    {
        pMainPool = m_pParentNode->GetPerFramePool(PoolType::PerFrameResult);

        if (NULL != pMainPool)
        {
            pMainPool->UnsubscribeAll(static_cast<IPropertyPoolObserver*>(this));
        }

        if (CSLInvalidHandle != m_hActuatorDevice)
        {
            if ((TRUE == SensorSubDevicesCache::GetInstance()->MustRelease(m_cameraId, ActuatorHandle)) ||
                (TRUE == m_pParentNode->IsFullRecoveryFlagSet()))
            {
                ParkLens();
                SensorSubDevicesCache::GetInstance()->ReleaseOneSubDevice(m_cameraId, ActuatorHandle);
                m_deviceAcquired = FALSE;
            }
        }
    }

    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Actuator::CreateInitializePacket
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Actuator::CreateInitializePacket()
{
    CamxResult      result               = CamxResultSuccess;
    UINT            I2CInfoCmdSize       = sizeof(CSLSensorI2CInfo);
    UINT            powerUpCmdSize       = m_pActuatorData->GetPowerSequenceCmdSize(TRUE);
    UINT            powerDownCmdSize     = m_pActuatorData->GetPowerSequenceCmdSize(FALSE);
    UINT            initializeCmdSize    = m_pActuatorData->GetInitializeCmdSize();
    PacketResource* pPacketResource      = NULL;
    Packet*         pPacket              = NULL;
    CmdBuffer*      pI2CInfoCmdBuffer    = NULL;
    CmdBuffer*      pPowerCmdBuffer      = NULL;
    CmdBuffer*      pInitializeCmdBuffer = NULL;

    pPacket                = m_pParentNode->GetPacket(m_pPacketManager);
    pI2CInfoCmdBuffer      = m_pParentNode->GetCmdBuffer(m_pI2CInfoCmdManager);
    pPowerCmdBuffer        = m_pParentNode->GetCmdBuffer(m_pPowerCmdManager);
    pInitializeCmdBuffer   = m_pParentNode->GetCmdBuffer(m_pInitializeCmdManager);

    if ((NULL == pPacket) ||
        (NULL == pI2CInfoCmdBuffer) ||
        (NULL == pPowerCmdBuffer) ||
        (NULL == pInitializeCmdBuffer))
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor,
                       "Memory allocation failed Packet: %p, I2CCmdBuffer: %p, m_pPowerCmdBuffer: %p, InitializeCmdBuffer: %p",
                       pPacket,
                       pI2CInfoCmdBuffer,
                       pPowerCmdBuffer,
                       pInitializeCmdBuffer);
        result = CamxResultENoMemory;
    }
    else
    {
        VOID* pCmdBegin = pI2CInfoCmdBuffer->BeginCommands(I2CInfoCmdSize / sizeof(UINT32));
        if (NULL != pCmdBegin)
        {
            CSLSensorI2CInfo* pCmdI2CInfo = reinterpret_cast<CSLSensorI2CInfo*>(pCmdBegin);

            result = m_pActuatorData->CreateI2CInfoCmd(pCmdI2CInfo);
            if (CamxResultSuccess == result)
            {
                result = pI2CInfoCmdBuffer->CommitCommands();
            }
        }

        if ((CamxResultSuccess == result) && (0 != powerUpCmdSize))
        {
            VOID* pPowerUpCmdBegin = pPowerCmdBuffer->BeginCommands(powerUpCmdSize / sizeof(UINT32));
            if (NULL != pPowerUpCmdBegin)
            {
                if (CamxResultSuccess == m_pActuatorData->CreatePowerSequenceCmd(TRUE, pPowerUpCmdBegin))
                {
                    result = pPowerCmdBuffer->CommitCommands();
                }
            }
        }

        if ((CamxResultSuccess == result) && (0 != powerDownCmdSize))
        {
            VOID* pPowerDownCmdBegin = pPowerCmdBuffer->BeginCommands(powerDownCmdSize / sizeof(UINT32));
            if (NULL != pPowerDownCmdBegin)
            {
                if (CamxResultSuccess == m_pActuatorData->CreatePowerSequenceCmd(FALSE, pPowerDownCmdBegin))
                {
                    result = pPowerCmdBuffer->CommitCommands();
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            pCmdBegin = pInitializeCmdBuffer->BeginCommands(initializeCmdSize / sizeof(UINT32));
            if (NULL != pCmdBegin)
            {
                VOID* pCmdInit = reinterpret_cast<CSLSensorProbeCmd*>(pCmdBegin);

                result = m_pActuatorData->CreateInitializeCmd(pCmdInit);
                if (CamxResultSuccess == result)
                {
                    result = pInitializeCmdBuffer->CommitCommands();
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            pPacket->SetOpcode(CSLDeviceTypeLensActuator, CSLPacketOpcodesActuatorInitialConfig);

            result = pPacket->AddCmdBufferReference(pI2CInfoCmdBuffer, NULL);
            {
                if (CamxResultSuccess == result)
                {
                    result = pPacket->AddCmdBufferReference(pPowerCmdBuffer, NULL);
                    if (CamxResultSuccess == result)
                    {
                        result = pPacket->AddCmdBufferReference(pInitializeCmdBuffer, NULL);
                        if (CamxResultSuccess == result)
                        {
                            result = pPacket->CommitPacket();
                        }
                    }
                }
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        result = m_pHwContext->Submit(m_pParentNode->GetCSLSession(), m_hActuatorDevice, pPacket);
    }

    // Need to recycle manually as not assosciated with any request ID
    m_pI2CInfoCmdManager->Recycle(pI2CInfoCmdBuffer);
    m_pPowerCmdManager->Recycle(pPowerCmdBuffer);
    m_pInitializeCmdManager->Recycle(pInitializeCmdBuffer);
    m_pPacketManager->Recycle(pPacket);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Actuator::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Actuator::Initialize(
    ActuatorCreateData* pCreateData,
    INT32               actuatorDeviceIndex)
{
    CamxResult   result              = CamxResultSuccess;
    Subscription subscription        = {UnitType::Property, PropertyIDAFFrameInfo};
    SubDeviceProperty actuatorDevice = SensorSubDevicesCache::GetInstance()->GetSubDevice(pCreateData->cameraId,
                                                                                          ActuatorHandle);

    m_pParentNode              = pCreateData->pParentNode;
    m_pPacketManager           = pCreateData->pPacketManager;
    m_pInitializeCmdManager    = pCreateData->pInitCmdManager;
    m_pI2CInfoCmdManager       = pCreateData->pI2CInfoCmdManager;
    m_pPowerCmdManager         = pCreateData->pPowerCmdManager;
    m_pMoveFocusCmdManager     = pCreateData->pMoveFocusCmdManager;
    m_cameraId                 = pCreateData->cameraId;
    m_requestQueueDepth        = pCreateData->requestQueueDepth;

    CAMX_ASSERT(NULL != pCreateData->pPerUsecasePool);

    if (FALSE == actuatorDevice.isAcquired)
    {
        CSLDeviceAttribute deviceAttribute = { 0 };

        deviceAttribute.attributeID              = CSLDeviceAttributeRealtimeOperation;
        deviceAttribute.pDeviceAttributeParam    = NULL;
        deviceAttribute.deviceAttributeParamSize = 0;

        CAMX_LOG_INFO(CamxLogGroupSensor, "Initializing actuator for camerId: %d", pCreateData->cameraId);
        result = CSLAcquireDevice(m_pParentNode->GetCSLSession(),
                                  &m_hActuatorDevice,
                                  actuatorDeviceIndex,
                                  NULL,
                                  0,
                                  &deviceAttribute,
                                  1,
                                  m_pParentNode->NodeIdentifierString());

        if (CamxResultSuccess == result)
        {
            m_deviceAcquired = TRUE;

            SensorSubDevicesCache::GetInstance()->SetSubDeviceHandle(m_pParentNode->GetCSLSession(), pCreateData->cameraId,
                m_hActuatorDevice, ActuatorHandle);

            result = CreateInitializePacket();
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed for deviceIndex: %d", actuatorDeviceIndex);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor,
                           "AcquireDevice on Actuator[%d] failed deviceIndex: %d, result: %d",
                           m_cameraId,
                           actuatorDeviceIndex,
                           result);
        }
    }
    else
    {
        CAMX_ASSERT(actuatorDevice.hDevice != CSLInvalidHandle);
        CAMX_LOG_INFO(CamxLogGroupSensor,
                      "Reusing actuator device handle: %p for camerId: %d",
                      actuatorDevice.hDevice,
                      pCreateData->cameraId);
        m_hActuatorDevice = actuatorDevice.hDevice;
        m_deviceAcquired = TRUE;
    }

    if (CamxResultSuccess == result)
    {
        if (NULL != m_pParentNode)
        {
            m_pParentNode->AddCSLDeviceHandle(m_hActuatorDevice);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Null Parent Node");
            return CamxResultEInvalidPointer;
        }
    }

    if (CamxResultSuccess == result)
    {
        MetadataPool* pMainPool = m_pParentNode->GetPerFramePool(PoolType::PerFrameResult);

        if (NULL != pMainPool)
        {
            result = pMainPool->Subscribe(&subscription, 1, static_cast<IPropertyPoolObserver*>(this), "Actuator");
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Null Main Pool");
            result = CamxResultEResource;
        }
    }

    if (CamxResultSuccess == result)
    {
        if (GetCurrentPosition(PositionUnit::Step) != InvalidLensData)
        {
            // Intialize the actuator to previously converged/requested position
            MoveFocus(GetCurrentPosition(PositionUnit::Step), PositionUnit::Step, CamxInvalidRequestId, 0, 0);
        }
        else
        {
            // Intialize the actuator to infinity position which is at the index (MaxSteps -1)
            MoveFocus((MaxSteps -1), PositionUnit::Step, CamxInvalidRequestId, 0, 0);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Actuator::ParkLens
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Actuator::ParkLens()
{
    if (ActuatorType::VCM == m_pActuatorData->GetActuatorLibrary()->slaveInfo.actuatorType)
    {
        UINT currentPosition = static_cast<UINT>(GetCurrentPosition(PositionUnit::DAC));

        while (currentPosition > 0)
        {
            UINT nearInfinityPosition = static_cast<UINT>(m_pActuatorData->GetInfinityPosition()) + ParkLensPaceValue;

            // Move in different pace to reduce park lens time
            if (currentPosition > nearInfinityPosition)
            {
                currentPosition = nearInfinityPosition;
            }
            else if ((currentPosition > ParkLensPaceValue) &&
                     (nearInfinityPosition > PaceThroughDistance) &&
                     (currentPosition > nearInfinityPosition - PaceThroughDistance))
            {
                currentPosition -= ParkLensPaceValue;
            }
            else
            {
                currentPosition = 0;
            }

            MoveFocus(currentPosition, PositionUnit::DAC, CamxInvalidRequestId, ParkLensDelayMicroseconds, 0);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Actuator::MoveFocus
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Actuator::MoveFocus(
    INT          targetPosition,
    PositionUnit unit,
    UINT64       requestId,
    UINT16       additionalDelay,
    BOOL         isManulaMode)
{
    CamxResult      result              = CamxResultENoMemory;
    PacketResource* pPacketResource     = NULL;
    CmdBuffer*      pMoveFocusCmdBuffer = NULL;
    Packet*         pPacket             = NULL;

    pPacket             = m_pParentNode->GetPacketForRequest(requestId, m_pPacketManager);
    pMoveFocusCmdBuffer = m_pParentNode->GetCmdBufferForRequest(requestId, m_pMoveFocusCmdManager);

    if ((NULL != pPacket) && (NULL != pMoveFocusCmdBuffer))
    {
        INT focusCmdSize = m_pActuatorData->GetMoveFocusCmdSize(additionalDelay);

        VOID* pCmdBegin = pMoveFocusCmdBuffer->BeginCommands(focusCmdSize / sizeof(UINT32));
        if (NULL != pCmdBegin)
        {
            VOID* pCmd = reinterpret_cast<CSLSensorI2CInfo*>(pCmdBegin);

            result = m_pActuatorData->CreateMoveFocusCmd(targetPosition, unit, pCmd, additionalDelay);
            if (CamxResultSuccess == result)
            {
                result = pMoveFocusCmdBuffer->CommitCommands();
            }
        }

        if (CamxResultSuccess == result)
        {
            CSLPacketOpcodesActuator opcode    = CSLPacketOpcodesActuatorAutoMove;
            UINT64                   CSLSyncID = CamxInvalidRequestId;;

            if (TRUE == isManulaMode)
            {
                opcode = CSLPacketOpcodesActuatorManualMove;
            }

            if ((FALSE == m_pParentNode->IsPipelineStreamedOn()) &&
                (CamxInvalidRequestId != requestId))
            {
                if (TRUE == m_initialConfigPending)
                {
                    opcode                 = CSLPacketOpcodesActuatorInitialConfig;
                    m_initialConfigPending = FALSE;
                }
            }

            if (CamxInvalidRequestId == requestId)
            {
                opcode = CSLPacketOpcodesActuatorInitialConfig;
            }
            else
            {
                CSLSyncID = m_pParentNode->GetCSLSyncId(requestId);
            }

            pMoveFocusCmdBuffer->SetRequestId(CSLSyncID);
            pPacket->SetRequestId(CSLSyncID);

            pPacket->SetOpcode(CSLDeviceTypeLensActuator, opcode);

            result = pPacket->AddCmdBufferReference(pMoveFocusCmdBuffer, NULL);
        }

        if (CamxResultSuccess == result)
        {
            result = pPacket->CommitPacket();
        }

        if (CamxResultSuccess == result)
        {
            CAMX_LOG_INFO(CamxLogGroupSensor,
                          "Submit Actuator[%d]: requestId:%llu, targetPosition: %d, PositionUnit: %d, isManual: %d",
                          m_cameraId,
                          requestId,
                          targetPosition,
                          unit,
                          isManulaMode);
            result = m_pHwContext->Submit(m_pParentNode->GetCSLSession(), m_hActuatorDevice, pPacket);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Actuator[%d] Failed: %d", m_cameraId, result);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor,
                       "Actuator[%d] Invalid buffer Packet: %p, MoveFocusCmdBuffer: %p, requestId:%llu",
                       m_cameraId,
                       pPacket,
                       pMoveFocusCmdBuffer,
                       requestId);
    }

    m_pPacketManager->RecycleAll(m_pParentNode->GetCSLSyncId(requestId));
    m_pMoveFocusCmdManager->RecycleAll(m_pParentNode->GetCSLSyncId(requestId));

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Actuator::SendNOP
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Actuator::SendNOP(
    UINT64  requestId)
{
    CamxResult      result          = CamxResultENoMemory;
    PacketResource* pPacketResource = NULL;
    Packet*         pPacket         = NULL;
    CmdBuffer*      pNopCmdBuffer   = NULL;

    pPacket       = m_pParentNode->GetPacketForRequest(requestId, m_pPacketManager);
    pNopCmdBuffer = m_pParentNode->GetCmdBufferForRequest(requestId, m_pMoveFocusCmdManager);

    if ((NULL != pPacket) && (NULL != pNopCmdBuffer))
    {
        pNopCmdBuffer->SetRequestId(m_pParentNode->GetCSLSyncId(requestId));
        pPacket->SetRequestId(m_pParentNode->GetCSLSyncId(requestId));
        pPacket->SetOpcode(CSLDeviceTypeLensActuator, CSLPacketOpcodesNop);

        result = pPacket->AddCmdBufferReference(pNopCmdBuffer, NULL);

        if (CamxResultSuccess == result)
        {
            result = pPacket->CommitPacket();
        }

        if (CamxResultSuccess == result)
        {
            CAMX_LOG_INFO(CamxLogGroupSensor, "Submit Actuator[%d]: requestId=%llu", m_cameraId, requestId);
            result = m_pHwContext->Submit(m_pParentNode->GetCSLSession(), m_hActuatorDevice, pPacket);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor,
                       "Actuator[%d] Invalid buffer Packet: %p, pNopCmdBuffer: %p, requestId:%llu",
                       m_cameraId,
                       pPacket,
                       pNopCmdBuffer,
                       requestId);
    }

    m_pPacketManager->RecycleAll(m_pParentNode->GetCSLSyncId(requestId));
    m_pMoveFocusCmdManager->RecycleAll(m_pParentNode->GetCSLSyncId(requestId));

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Actuator::GetCurrentPosition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT Actuator::GetCurrentPosition(
    PositionUnit unit)
{
    return m_pActuatorData->GetCurrentPosition(unit);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Actuator::GetSensitivity
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FLOAT Actuator::GetSensitivity()
{
    return m_pActuatorData->GetSensitivity();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Actuator::OnStreamOff
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Actuator::OnStreamOff(
    CHIDEACTIVATEPIPELINEMODE modeBitmask)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(modeBitmask);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Prepare stream off ");

    m_initialConfigPending = TRUE;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Actuator::OnPropertyUpdate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Actuator::OnPropertyUpdate(
    PropertyID  id,
    UINT64      requestId,
    UINT        pipelineId)
{
    MetadataPool*   pPerFramePool     = m_pParentNode->GetPerFramePool(PoolType::PerFrameResult);
    MetadataSlot*   pSlot             = NULL;
    CamxResult      result            = CamxResultSuccess;
    LensStateValues currentLensState  = LensStateStationary;

    CAMX_ASSERT(PropertyIDAFFrameInfo == id);
    CAMX_ASSERT(NULL != pPerFramePool);
    CAMX_UNREFERENCED_PARAM(pipelineId);

    if (NULL != pPerFramePool)
    {
        pSlot = pPerFramePool->GetSlot(requestId);
    }

    if (NULL != pSlot)
    {
        AFFrameInformation* pAFFrameinfo = static_cast<AFFrameInformation*>(pSlot->GetMetadataByTag(PropertyIDAFFrameInfo));

        if (NULL != pAFFrameinfo)
        {
            BOOL                  isManualMode         = FALSE;
            INT32                 targetLensPosition   = 0;
            PositionUnit          unitType             = PositionUnit::NoOperation;
            MoveLensOutput*       pMoveLens            = &pAFFrameinfo->moveLensOutput;
            const StaticSettings* pStaticSettings      = HwEnvironment::GetInstance()->GetStaticSettings();
            BOOL                  overrideLensPosition =
                (AFManualLensControlLogical == pStaticSettings->manualAf) ? TRUE : FALSE;

            if (TRUE == overrideLensPosition)
            {
                targetLensPosition = pStaticSettings->lensPos;
            }
            else
            {
                targetLensPosition = static_cast<INT>(pMoveLens->targetLensPosition);

                static const UINT MetadataAF[]   = { InputControlAFMode };
                static const UINT Length         = CAMX_ARRAY_SIZE(MetadataAF);
                VOID*             pData[Length]  = { 0 };
                UINT64            offset[Length] = { 0 };

                m_pParentNode->GetDataList(MetadataAF, pData, offset, Length);

                if (NULL != pData[0])
                {
                    ControlAFModeValues AFMode = *reinterpret_cast<ControlAFModeValues*>(pData[0]);

                    if (ControlAFModeOff == AFMode)
                    {
                        isManualMode = TRUE;
                    }
                }
            }

            if (pMoveLens->useDACValue == TRUE)
            {
                if ((GetCurrentPosition(PositionUnit::DAC) != targetLensPosition))
                {
                    unitType = PositionUnit::DAC;
                }
            }
            else
            {
                if ((GetCurrentPosition(PositionUnit::Step) != targetLensPosition))
                {
                    unitType = PositionUnit::Step;
                }
            }

            if (unitType == PositionUnit::NoOperation)
            {
                if (FALSE == m_pParentNode->IsPipelineStreamedOn())
                {
                    if (TRUE == m_initialConfigPending)
                    {
                        m_initialConfigPending = FALSE;
                    }
                    else
                    {
                        result = SendNOP(requestId);
                    }
                }
                else
                {
                    m_initialConfigPending = TRUE;
                    result = SendNOP(requestId);
                }

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_WARN(CamxLogGroupSensor,
                                   "Failed to send NOP command: result: %d, requestId: %llu",
                                   result,
                                   requestId);
                }

                currentLensState = LensStateStationary;
            }
            else
            {
                result = MoveFocus(targetLensPosition, unitType, requestId, 0, isManualMode);

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_WARN(CamxLogGroupSensor, "Failed to move focus: result: %d, requestId: %llu", result, requestId);
                }

                currentLensState = LensStateMoving;

                CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                                 "targetPosition: %d, unitType: %d, requestId: %llu, isManualMode: %d, isOverridePos: %d",
                                 targetLensPosition,
                                 unitType,
                                 requestId,
                                 isManualMode,
                                 overrideLensPosition);
            }

            static const UINT WritePropsInner[]                                  = { LensState };
            const VOID*       pOutputDataInner[CAMX_ARRAY_SIZE(WritePropsInner)] = { &currentLensState };
            UINT              pDataCountInner[CAMX_ARRAY_SIZE(WritePropsInner)]  = { 1 };

            m_pParentNode->WriteDataList(WritePropsInner,
                                         pOutputDataInner,
                                         pDataCountInner,
                                         CAMX_ARRAY_SIZE(WritePropsInner));
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed: to get property blob");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed: per frame slot is NULL");
    }
}

CAMX_NAMESPACE_END
