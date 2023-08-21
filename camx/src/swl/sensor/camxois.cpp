////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxois.cpp
/// @brief Implements ois methods.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxincs.h"
#include "camxmem.h"
#include "camxpacket.h"
#include "camxcmdbuffer.h"
#include "camxcmdbuffermanager.h"
#include "camxcslsensordefs.h"
#include "camxhal3module.h"
#include "camxhwcontext.h"
#include "camxhwdefs.h"
#include "camximagesensormoduledata.h"
#include "camxois.h"
#include "camxsensornode.h"


CAMX_NAMESPACE_BEGIN

static const UINT InitialCalibCmdCount = 2; ///< Number of command buffers in config command

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OIS::OIS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OIS::OIS(
    HwContext* pHwContext,
    OISData* pData)
{
    m_pHwContext           = pHwContext;
    m_pOISData             = pData;
    m_hOisDevice           = CSLInvalidHandle;
    m_oisMode              = OISMode::DisableOIS;
    m_pCalibrateCmdManager = NULL;
    m_isCalibration        = FALSE;
    m_OISConfigStatus      = OISConfigurationStatus::OISConfigurationStateUninitialized;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OIS::~OIS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OIS::~OIS()
{
    if (NULL != m_pCalibrateCmdManager)
    {
        CAMX_DELETE m_pCalibrateCmdManager;
        m_pCalibrateCmdManager = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OIS::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OIS::Create(
    OisCreateData* pCreateData,
    INT32 oisDeviceIndex)
{
    CamxResult result = CamxResultEFailed;
    OISData* pOisData = NULL;

    if (NULL != pCreateData)
    {
        HwContext* pHwContext = pCreateData->pHwContext;
        if (NULL != pHwContext)
        {
            // NOWHINE CP036a: exception
            pOisData = const_cast<ImageSensorModuleData*>(
                pHwContext->GetImageSensorModuleData(pCreateData->cameraId))->GetOisDataObject();

            if (NULL == pOisData)
            {
                CAMX_LOG_WARN(CamxLogGroupSensor, "OISData is NULL for cameraID:%d", pCreateData->cameraId);
                result = CamxResultEInvalidPointer;
            }
            else
            {
                pCreateData->pOis= CAMX_NEW OIS(pHwContext, pOisData);
                if (NULL != pCreateData->pOis)
                {
                    result = pCreateData->pOis->Initialize(pCreateData, oisDeviceIndex);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "No memory for Ois creation!");
                    result = CamxResultENoMemory;
                }
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_INFO(CamxLogGroupSensor, "OIS created successfully!");
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "OIS unavailable or initialization failed: %d!", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OIS::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OIS::Destroy()
{

    if (CSLInvalidHandle != m_hOisDevice)
    {
        if ((TRUE == SensorSubDevicesCache::GetInstance()->MustRelease(m_cameraId, OISHandle)) ||
            ((NULL != m_pNode) && (TRUE == m_pNode->IsFullRecoveryFlagSet())))
        {
            SensorSubDevicesCache::GetInstance()->ReleaseOneSubDevice(m_cameraId, OISHandle);
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupSensor, "Caching OIS device handle: %p and OIS state: %d on cameraId: %d",
                m_hOisDevice, m_prevState, m_cameraId);
            SensorSubDevicesCache::GetInstance()->SetSubDeviceData(m_cameraId, m_prevState, OISHandle);
        }
    }

    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OIS::CreateInitializePacket
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OIS::CreateInitializePacket()
{
    CamxResult      result              = CamxResultSuccess;
    UINT            oisI2cInfoCmdSize   = sizeof(CSLOISI2CInfo);
    UINT            powerUpCmdSize      = m_pOISData->GetPowerSequenceCmdSize(TRUE);
    UINT            powerDownCmdSize    = m_pOISData->GetPowerSequenceCmdSize(FALSE);
    UINT            initializeCmdSize   = m_pOISData->GetInitializeCmdSize();
    PacketResource* pPacketResource     = NULL;
    UINT            calibCmdSize        = 0;
    CmdBuffer*      pCalibrateCmdBuffer = NULL;
    m_OISConfigStatus                   = OISConfigurationStatus::OISInitializationInProgress;

    if (CamxResultSuccess == m_pI2CInfoCmdManager->GetBuffer(&pPacketResource))
    {
        CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().cmdBuffer);

        // We know pResource actually points to a CmdBuffer so we may static_cast
        m_pI2CInfoCmdBuffer = static_cast<CmdBuffer*>(pPacketResource);
    }

    if (CamxResultSuccess == m_pPowerCmdManager->GetBuffer(&pPacketResource))
    {
        CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().cmdBuffer);

        // We know pResource actually points to a CmdBuffer so we may static_cast
        m_pPowerCmdBuffer = static_cast<CmdBuffer*>(pPacketResource);
    }

    if (CamxResultSuccess == m_pInitializeCmdManager->GetBuffer(&pPacketResource))
    {
        CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().cmdBuffer);

        // We know pResource actually points to a CmdBuffer so we may static_cast
        m_pInitializeCmdBuffer = static_cast<CmdBuffer*>(pPacketResource);
    }

    if ((TRUE == m_isCalibration) && (NULL != m_pCalibrateCmdManager))
    {
        if (CamxResultSuccess == m_pCalibrateCmdManager->GetBuffer(&pPacketResource))
        {
            CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().cmdBuffer);

            // We know pResource actually points to a CmdBuffer so we may static_cast
            pCalibrateCmdBuffer = static_cast<CmdBuffer*>(pPacketResource);

            if (NULL == pCalibrateCmdBuffer)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Memory allocation failed pCalibrateCmdBuffer: %p", pCalibrateCmdBuffer);
                result = CamxResultENoMemory;
            }
        }
    }

    if (CamxResultSuccess == m_pInitializePacketManager->GetBuffer(&pPacketResource))
    {
        CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().packet);

        // We know pResource actually points to a Packet so we may static_cast
        m_pPacket = static_cast<Packet*>(pPacketResource);
    }

    if ((NULL == m_pPacket) ||
        (NULL == m_pI2CInfoCmdBuffer) ||
        (NULL == m_pInitializeCmdBuffer) ||
        (NULL == m_pPowerCmdBuffer))
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor,
                       "Memory allocation failed Packet: %p, I2CCmdBuffer: %p, InitializeCmdBuffer: %p, m_pPowerCmdBuffer: %p",
                       m_pPacket,
                       m_pI2CInfoCmdBuffer,
                       m_pInitializeCmdBuffer,
                       m_pPowerCmdBuffer);
        result = CamxResultENoMemory;
    }
    else
    {
        VOID* pCmdBegin = m_pI2CInfoCmdBuffer->BeginCommands(oisI2cInfoCmdSize / sizeof(UINT32));
        if (NULL != pCmdBegin)
        {
            CSLOISI2CInfo* pCmdI2CInfo = reinterpret_cast<CSLOISI2CInfo*>(pCmdBegin);

            result = m_pOISData->CreateI2CInfoCmd(pCmdI2CInfo);

            if (CamxResultSuccess == result)
            {
                result = m_pI2CInfoCmdBuffer->CommitCommands();
            }
        }

        if ((CamxResultSuccess == result) && (0 != powerUpCmdSize))
        {
            VOID* pPowerUpCmdBegin = m_pPowerCmdBuffer->BeginCommands(powerUpCmdSize / sizeof(UINT32));
            if (NULL != pPowerUpCmdBegin)
            {
                if (CamxResultSuccess == m_pOISData->CreatePowerSequenceCmd(TRUE, pPowerUpCmdBegin))
                {
                    result = m_pPowerCmdBuffer->CommitCommands();
                }
            }
        }

        if ((CamxResultSuccess == result) && (0 != powerDownCmdSize))
        {
            VOID* pPowerDownCmdBegin = m_pPowerCmdBuffer->BeginCommands(powerDownCmdSize / sizeof(UINT32));
            if (NULL != pPowerDownCmdBegin)
            {
                if (CamxResultSuccess == m_pOISData->CreatePowerSequenceCmd(FALSE, pPowerDownCmdBegin))
                {
                    result = m_pPowerCmdBuffer->CommitCommands();
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            pCmdBegin = m_pInitializeCmdBuffer->BeginCommands(initializeCmdSize / sizeof(UINT32));
            if (NULL != pCmdBegin)
            {
                VOID* pCmdInit = reinterpret_cast<CSLSensorProbeCmd*>(pCmdBegin);

                result = m_pOISData->CreateInitializeCmd(pCmdInit);
                if (CamxResultSuccess == result)
                {
                    result = m_pInitializeCmdBuffer->CommitCommands();
                }
            }
        }

        if (TRUE == m_isCalibration && CamxResultSuccess == result && NULL != pCalibrateCmdBuffer)
        {
            calibCmdSize = m_pOISData->GetCalibrationCmdSize();
            pCmdBegin    = pCalibrateCmdBuffer->BeginCommands(calibCmdSize/ sizeof(UINT32));
            if (NULL != pCmdBegin)
            {
                VOID* pCmdInit = reinterpret_cast<CSLSensorProbeCmd*>(pCmdBegin);

                result = m_pOISData->CreateCalibrationCmd(pCmdInit);
                if (CamxResultSuccess == result)
                {
                    result = pCalibrateCmdBuffer->CommitCommands();
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            // Not associated with any request. Won't be recycled.
            m_pPacket->SetOpcode(CSLDeviceTypeOIS, CSLPacketOpcodesOisInitialConfig);

            result = m_pPacket->AddCmdBufferReference(m_pI2CInfoCmdBuffer, NULL);
        }

        if (CamxResultSuccess == result)
        {
            result = m_pPacket->AddCmdBufferReference(m_pPowerCmdBuffer, NULL);
        }

        if (CamxResultSuccess == result)
        {
            result = m_pPacket->AddCmdBufferReference(m_pInitializeCmdBuffer, NULL);
        }

        if (TRUE == m_isCalibration && CamxResultSuccess == result)
        {
            result = m_pPacket->AddCmdBufferReference(pCalibrateCmdBuffer, NULL);
        }

        if (CamxResultSuccess == result)
        {
            result = m_pPacket->CommitPacket();
        }
    }

    if (CamxResultSuccess == result)
    {
        result = m_pHwContext->Submit(m_pNode->GetCSLSession(), m_hOisDevice, m_pPacket);
    }

    m_pInitializePacketManager->Recycle(m_pPacket);
    m_pInitializeCmdManager->Recycle(m_pInitializeCmdBuffer);
    m_pPowerCmdManager->Recycle(m_pPowerCmdBuffer);
    m_pI2CInfoCmdManager->Recycle(m_pI2CInfoCmdBuffer);
    if (NULL != m_pCalibrateCmdManager)
    {
        m_pCalibrateCmdManager->Recycle(pCalibrateCmdBuffer);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OIS::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OIS::Initialize(
    OisCreateData* pCreateData,
    INT32          OISDeviceIndex)
{
    CamxResult result           = CamxResultSuccess;
    SubDeviceProperty OISDevice = SensorSubDevicesCache::GetInstance()->GetSubDevice(pCreateData->cameraId, OISHandle);

    m_cameraId                  = pCreateData->cameraId;
    m_pInitializePacketManager  = pCreateData->pInitPacketManager;
    m_pInitializeCmdManager     = pCreateData->pInitCmdManager;
    m_pI2CInfoCmdManager        = pCreateData->pI2CInfoCmdManager;
    m_pPowerCmdManager          = pCreateData->pPowerCmdManager;
    m_pOisModePacketManager     = pCreateData->pModePacketManager;
    m_pOisModeCmdManager        = pCreateData->pModeCmdManager;
    m_pNode                     = pCreateData->pParentNode;
    m_prevState                 = LensOpticalStabilizationModeOff;

    CalibrateOISData(pCreateData->pOTPData);
    if (TRUE == m_isCalibration)
    {
        result = CreateCalibrationCmdBufferManager();
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "OIS CreateCalibrationCmdBufferManager failed");
        }
    }

    if ((FALSE == OISDevice.isAcquired) && (CamxResultSuccess == result))
    {
        result = CSLAcquireDevice(m_pNode->GetCSLSession(),
                                  &m_hOisDevice,
                                  OISDeviceIndex,
                                  NULL,
                                  0,
                                  NULL,
                                  0,
                                  m_pNode->NodeIdentifierString());

        if (CamxResultSuccess == result)
        {
            CAMX_LOG_INFO(CamxLogGroupSensor,
                          "Initializing OIS for cameraId: %d, OIS device handle: %p",
                          pCreateData->cameraId,
                          m_hOisDevice);

            SensorSubDevicesCache::GetInstance()->SetSubDeviceHandle(
               m_pNode->GetCSLSession(), pCreateData->cameraId, m_hOisDevice, OISHandle);

            result = CreateInitializePacket();
            if (result == CamxResultSuccess)
            {
                m_OISConfigStatus = OISConfigurationStatus::OISInitializationComplete;
            }
            else
            {
                m_OISConfigStatus = OISConfigurationStatus::OISInitializationFailed;
            }
        }

        if ((CamxResultSuccess == result) && (0 != m_pOISData->GetOISLibrary()->enterDownloadModeSettingsExists))
        {
            result = ConfigureOISMode(OISMode::EnterDownLoadMode);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "mode: EnterDownLoadMode failed");
            }
        }
    }
    else if (CamxResultSuccess == result)
    {
        CAMX_ASSERT(OISDevice.hDevice != CSLInvalidHandle);
        m_hOisDevice = OISDevice.hDevice;
        m_prevState  = static_cast<LensOpticalStabilizationModeValues>(OISDevice.dataMask);
        CAMX_LOG_INFO(CamxLogGroupSensor,
                      "Reusing OIS device handle: %p for cameraId: %d, m_prevState: %d",
                      OISDevice.hDevice,
                      pCreateData->cameraId,
                      m_prevState);
        m_OISConfigStatus = OISConfigurationStatus::OISInitializationComplete;
    }

    if (CamxResultSuccess == result)
    {
        pCreateData->pParentNode->AddCSLDeviceHandle(m_hOisDevice);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor,
                       "AcquireDevice on OIS failed deviceIndex: %d, result: %d",
                       OISDeviceIndex,
                       result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OIS::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OIS::ExecuteProcessRequest(
    ExecuteProcessRequestData* pExecuteProcessRequestData)

{
    LensOpticalStabilizationModeValues oisEnable     = LensOpticalStabilizationModeOff;
    CamxResult                         result        = CamxResultSuccess;
    ControlCaptureIntentValues         captureIntent = ControlCaptureIntentEnd;
    UINT64                             requestId     = pExecuteProcessRequestData->pNodeProcessRequestData->
                                                       pCaptureRequest->requestId;
    OISMode                            setOISMode    = OISMode::ModeEnd;

    static const UINT MetadataOIS[] =
    {
        InputLensOpticalStabilizationMode,
        InputControlCaptureIntent
    };

    static const UINT Length = CAMX_ARRAY_SIZE(MetadataOIS);
    VOID* pData[Length] = { 0 };
    UINT64 propertyDataOisOffset[Length] = { 0 };

    m_pNode->GetDataList(MetadataOIS, pData, propertyDataOisOffset, Length);

    if (NULL != pData[0])
    {
        oisEnable = *(static_cast<LensOpticalStabilizationModeValues*>(pData[0]));
    }

    if (NULL != pData[1])
    {
        captureIntent = *(static_cast<ControlCaptureIntentValues*>(pData[1]));
    }

    if ((LensOpticalStabilizationModeOn == oisEnable) && (LensOpticalStabilizationModeOff == m_prevState))
    {
        if (0 !=  m_pOISData->GetOISLibrary()->centeringOnSettingsExists)
        {
            result = ConfigureOISMode(OISMode::CenteringOn);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "mode: CenteringOn failed");
            }
        }

        result = ConfigureOISMode(OISMode::EnableOIS);
        if (CamxResultSuccess == result)
        {
            m_oisMode   = OISMode::EnableOIS;
            m_prevState = LensOpticalStabilizationModeOn;
            if (m_pOISData->GetOISLibrary()->pantiltOnSettingsExists)
            {
                result    = ConfigureOISMode(OISMode::Pantilt);
                m_oisMode = OISMode::Pantilt;
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Mode: Pantilt failed");
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Mode: EnableOIS failed");
        }
    }
    else if (LensOpticalStabilizationModeOff == oisEnable && m_prevState == LensOpticalStabilizationModeOn)
    {
        result = ConfigureOISMode(OISMode::DisableOIS);
        if (CamxResultSuccess == result)
        {
            m_oisMode = OISMode::DisableOIS;
            m_prevState = LensOpticalStabilizationModeOff;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Mode: DisableOIS failed");
        }
    }
    else if (LensOpticalStabilizationModeOff == oisEnable && m_prevState == LensOpticalStabilizationModeOff)
    {
        if ((m_pOISData->GetOISLibrary()->centeringOnSettingsExists) && (m_oisMode != OISMode::CenteringOn))
        {
            result = ConfigureOISMode(OISMode::CenteringOn);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "mode: CenteringOn failed");
            }
            else
            {
                m_oisMode = OISMode::CenteringOn;
            }
        }
    }

    if ((CamxResultSuccess == result) && (LensOpticalStabilizationModeOn == m_prevState))
    {
        switch (captureIntent)
        {
            case ControlCaptureIntentVideoRecord:
            case ControlCaptureIntentVideoSnapshot:
                if (m_pOISData->GetOISLibrary()->movieModeSettingsExists)
                {
                    setOISMode = OISMode::Movie;
                }
                break;
            case ControlCaptureIntentPreview:
            case ControlCaptureIntentStillCapture:
            case ControlCaptureIntentZeroShutterLag:
                if (m_pOISData->GetOISLibrary()->stillModeSettingsExists)
                {
                    setOISMode = OISMode::Still;
                }
                break;
            default:
                break;
        }

        if ((m_oisMode != setOISMode) && (OISMode::ModeEnd != setOISMode))
        {
            result = ConfigureOISMode(setOISMode);
            if (CamxResultSuccess == result)
            {
                m_oisMode = setOISMode;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "OIS mode setting failed for mode: %d", setOISMode);
            }
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "OIS mode already set. mode: %d", setOISMode);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OIS::ConfigureOISMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OIS::ConfigureOISMode(
    OISMode mode)
{
    CamxResult      result              = CamxResultSuccess;
    UINT            oisCmdSize          = m_pOISData->GetOISModeCmdSize(mode);
    PacketResource* pPacketResource     = NULL;
    CmdBuffer*      pOisModeCmdBuffer   = NULL;
    Packet*         pOisModePacket      = NULL;

    CAMX_LOG_INFO(CamxLogGroupSensor, "set mode:%d ", mode);

    if (CamxResultSuccess == m_pOisModeCmdManager->GetBuffer(&pPacketResource))
    {
        CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().cmdBuffer);

        // We know pResource actually points to a CmdBuffer so we may static_cast
        pOisModeCmdBuffer = static_cast<CmdBuffer*>(pPacketResource);
    }

    if (CamxResultSuccess == m_pOisModePacketManager->GetBuffer(&pPacketResource))
    {
        CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().packet);

        // We know pResource actually points to a Packet so we may static_cast
        pOisModePacket = static_cast<Packet*>(pPacketResource);
    }

    if ((NULL == pOisModePacket)  || (NULL == pOisModeCmdBuffer))
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Memory allocation failed m_pOisModePacket: %p, m_pOisModeCmdBuffer: %p",
                       pOisModePacket, pOisModeCmdBuffer);
        result = CamxResultENoMemory;
    }
    else
    {
        VOID* pCmdBegin = pOisModeCmdBuffer->BeginCommands(oisCmdSize / sizeof(UINT32));
        if (NULL != pCmdBegin)
        {
            VOID* pCmd = reinterpret_cast<CSLSensorProbeCmd*>(pCmdBegin);

            result = m_pOISData->CreateOISModeCmd(pCmd, mode);

            if (CamxResultSuccess == result)
            {
                result = pOisModeCmdBuffer->CommitCommands();
            }
        }

        if (CamxResultSuccess == result)
        {
            pOisModePacket->SetOpcode(CSLDeviceTypeOIS, CSLPacketOpcodesOisMode);
            result = pOisModePacket->AddCmdBufferReference(pOisModeCmdBuffer, NULL);
        }

        if (CamxResultSuccess == result)
        {
            result = pOisModePacket->CommitPacket();
        }
    }

    if (CamxResultSuccess == result)
    {
        result = m_pHwContext->Submit(m_pNode->GetCSLSession(), m_hOisDevice, pOisModePacket);
    }

    m_pOisModeCmdManager->Recycle(pOisModeCmdBuffer);

    m_pOisModePacketManager->Recycle(pOisModePacket);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "result = %d", result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OIS::CreateCalibrationCmdBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OIS::CreateCalibrationCmdBufferManager()
{
    CamxResult result                       = CamxResultSuccess;

    ResourceParams cmdResourceParams        = { 0 };
    UINT           calibCmdSize             = m_pOISData->GetCalibrationCmdSize();

    cmdResourceParams.resourceSize          = calibCmdSize;
    cmdResourceParams.poolSize              = (InitialCalibCmdCount * calibCmdSize);
    cmdResourceParams.usageFlags.cmdBuffer  = 1;
    cmdResourceParams.cmdParams.type        = CmdType::I2C;
    cmdResourceParams.alignment             = CamxCommandBufferAlignmentInBytes;
    cmdResourceParams.pDeviceIndices        = NULL;
    cmdResourceParams.numDevices            = 0;
    cmdResourceParams.memFlags              = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

    m_pCalibrateCmdManager= CAMX_NEW CmdBufferManager(FALSE);
    if (NULL != m_pCalibrateCmdManager)
    {
        result = m_pCalibrateCmdManager->Initialize("OIS", &cmdResourceParams);
    }
    else
    {
        result = CamxResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OIS::CalibrateOISData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OIS::CalibrateOISData(
    const EEPROMOTPData* pOTPData)
{
    m_isCalibration = pOTPData->OISCalibration.isAvailable;

    if (TRUE == m_isCalibration)
    {
        m_pOISData->m_pCalibSettings = &(pOTPData->OISCalibration.settings);
    }

    CAMX_LOG_INFO(CamxLogGroupSensor, "isAvailable = %d", m_isCalibration);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OIS::ReleaseResources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OIS::ReleaseResources(
    CHIDEACTIVATEPIPELINEMODE modeBitmask)
{
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_INFO(CamxLogGroupSensor, "Disabling OIS for camera: %d, modeBitMask: %d", m_cameraId, modeBitmask);

    if (LensOpticalStabilizationModeOn == m_prevState)
    {
        result = ConfigureOISMode(OISMode::DisableOIS);
        if (CamxResultSuccess == result)
        {
            m_oisMode = OISMode::DisableOIS;
            m_prevState = LensOpticalStabilizationModeOff;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Mode: DisableOIS failed");
        }
    }

    return result;

}


CAMX_NAMESPACE_END
