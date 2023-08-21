////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxflash.cpp
/// @brief Implements Flash methods.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdebug.h"
#include "camxmem.h"
#include "camxcmdbuffer.h"
#include "camxcmdbuffermanager.h"
#include "camxcslsensordefs.h"
#include "camxhwcontext.h"
#include "camxhwdefs.h"
#include "camximagesensormoduledata.h"
#include "camxpacket.h"
#include "camxflash.h"
#include "camxsensornode.h"
#include "camxtuningdatamanager.h"
#include "chituningmodeparam.h"
#include "camxhal3module.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Flash::Flash
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Flash::Flash(
    HwContext* pHwContext,
    FlashData* pData)
    : m_pInitializeCmdManager(NULL)
    , m_pFireCmdManager(NULL)
    , m_pRERCmdManager(NULL)
    , m_hFlashDevice(CSLInvalidHandle)
    , m_operationMode(CSLNonRealtimeOperation)
    , m_initialConfigPending(TRUE)
    , m_torchStateOn(FALSE)
{
    m_pHwContext           = pHwContext;
    m_pFlashData           = pData;
    m_numberOfFlash        = pData->GetNumberOfFlashs();
    m_isMainflashRequired  = FALSE;
    m_flashOffDelayCounter = -1;
    m_lastFlashMode        = FlashModeOff;
    m_operationType        = FlashOperation::Off;

    // Setting RER Params to default values
    // Reading the data from tuning params in EPR.
    m_RERIteration           = RERCycles;
    m_REROnDelayMillisecond  = REROnOffTime;
    m_REROffDelayMillisecond = REROnOffTime;
    m_RERCompleted           = FALSE;

    /// @todo (CAMX-2525) Integrate Flash with 3A to do RER and preflash, hard code current for test purpose
    for (UINT i = 0; i < MaxLEDTriggers; i++)
    {
        m_lowCurrent[i]  = FlashDefaultCurrent;
        m_highCurrent[i] = FlashDefaultCurrent;
    }

    m_flashType           = pData->GetFlashType();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Flash::~Flash
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Flash::~Flash()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Flash::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Flash::Create(
    FlashCreateData* pCreateData)
{
    CamxResult result     = CamxResultSuccess;
    FlashData* pFlashData = NULL;

    if (NULL == pCreateData)
    {
        result = CamxResultEFailed;
    }
    else
    {
        HwContext* pHwContext = pCreateData->pHwContext;
        if (NULL != pHwContext)
        {
            // NOWHINE CP036a: exception
            pFlashData = const_cast<ImageSensorModuleData*>(
                pHwContext->GetImageSensorModuleData(pCreateData->cameraId))->GetFlashDataObject();

            if (NULL == pFlashData)
            {
                result = CamxResultEInvalidPointer;
                CAMX_LOG_WARN(CamxLogGroupSensor, "FlashData is NULL, flash is not support on cameraId %d",
                    pCreateData->cameraId);
            }
            else
            {
                pCreateData->pFlash = CAMX_NEW Flash(pHwContext, pFlashData);
                if (NULL != pCreateData->pFlash)
                {
                    result = pCreateData->pFlash->Initialize(pCreateData);
                }
                else
                {
                    result = CamxResultENoMemory;
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "No memory for Flash creation!");
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Flash::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Flash::Destroy()
{

    if (CSLInvalidHandle != m_hFlashDevice)
    {
        if ((TRUE == SensorSubDevicesCache::GetInstance()->MustRelease(m_cameraId, FlashHandle)) ||
            ((NULL != m_pParentNode) && (TRUE == m_pParentNode->IsFullRecoveryFlagSet())))
        {
            SensorSubDevicesCache::GetInstance()->ReleaseOneSubDevice(m_cameraId, FlashHandle);
        }
    }

    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Flash::OnStreamOff
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Flash::OnStreamOff(
    CHIDEACTIVATEPIPELINEMODE modeBitmask)
{
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Prepare stream off ");

    if ((FALSE == (CSLDeativateModeSensorStandBy & modeBitmask)) &&
        (FALSE == (CSLDeativateModeSensorRealtimeDevices & modeBitmask)))
    {
        m_initialConfigPending = TRUE;
        m_lastFlashMode        = FlashModeOff;
        m_lastFlashOperation   = FlashOperation::Off;
        m_flashOffDelayCounter = -1;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Flash::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Flash::ExecuteProcessRequest(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    AECFlashInfoType            flashType,
    UINT32                      LEDCurrents[LEDSettingCount],
    TuningDataManager*          pTuningManager)
{
    CamxResult                  result                = CamxResultSuccess;
    BOOL                        isPreflashInProgress  = FALSE;
    FlashModeValues             flashMode             = FlashModeOff;
    ControlCaptureIntentValues  captureIntent         = ControlCaptureIntentEnd;
    ControlAEModeValues         aeModeValue           = ControlAEModeEnd;
    BOOL                        bShouldSendNop        = TRUE;
    BOOL                        bNormalFlashControl   = TRUE;

    static const UINT PropertiesFlash[]               = { InputControlCaptureIntent, InputFlashMode, InputControlAEMode};
    static const UINT Length                          = CAMX_ARRAY_SIZE(PropertiesFlash);
    VOID*             pData[Length]                   = { 0 };
    UINT64            propertyDataFlashOffset[Length] = { 0 };

    UINT64 requestId = pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId;

    m_pParentNode->GetDataList(PropertiesFlash, pData, propertyDataFlashOffset, Length);

    if ((NULL != pData[0]) && (NULL != pData[1]) && (NULL != pData[2]))
    {
        captureIntent = *(static_cast<ControlCaptureIntentValues*>(pData[0]));
        flashMode     = *(static_cast<FlashModeValues*>(pData[1]));
        aeModeValue   = *(static_cast<ControlAEModeValues*>(pData[2]));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupSensor, "CaptureIntent not available, defaulting to FlashOff");
    }

    CAMX_LOG_INFO(CamxLogGroupSensor, "RequestId=%llu CaptureIntent=%d FlashMode=%d flashOffDelayCounter=%d MainflashReq=%d",
                  requestId,
                  captureIntent,
                  flashMode,
                  m_flashOffDelayCounter,
                  m_isMainflashRequired);
    CAMX_LOG_INFO(CamxLogGroupSensor, "FLASH[%d] PFState=%d flashType=%d aeModeValue=%d CalibFlashState=%d",
                  m_cameraId,
                  m_preFlashState,
                  flashType,
                  aeModeValue,
                  m_calibFlashState);

    if (pTuningManager != NULL && ControlAEModeOnAutoFlashRedeye == aeModeValue)
    {
        m_pRERTuningData         = pTuningManager->GetChromatix()->GetModule_ChromatixRERTuningData(
                                        (reinterpret_cast<TuningMode*>
                                        (&pExecuteProcessRequestData->pTuningModeData->TuningMode[0])),
                                        pExecuteProcessRequestData->pTuningModeData->noOfSelectionParameter);

        m_RERIteration           = static_cast<UINT16>(m_pRERTuningData->preflashCyclesCount);
        m_REROnDelayMillisecond  = m_pRERTuningData->msLEDPulsesDuration;
        m_REROffDelayMillisecond = m_pRERTuningData->msIntervalBetweenPulses;

        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "RERIteration = %d REROnDelayMS = %d REROffDelayMS =%d",
            m_RERIteration, m_REROnDelayMillisecond, m_REROffDelayMillisecond);
    }

    if (aeModeValue <= ControlAEModeOn)
    {
        if ((FlashModeOff == flashMode) || (FlashModeTorch == flashMode))
        {
            result = HandleTorch(flashMode, requestId, &bShouldSendNop);
            bNormalFlashControl = FALSE;

            if (FlashModeTorch == flashMode)
            {
                m_torchStateOn = TRUE;
            }
            else
            {
                m_torchStateOn = FALSE;
            }
        }
        else if (FlashModeSingle == flashMode)
        {
            Fire(FlashOperation::High, requestId, &bShouldSendNop);
            bNormalFlashControl = FALSE;
        }
    }
    else if (TRUE == m_torchStateOn)
    {
        // When Torch On State in Previous Process request and User Changes
        // AE mode with out OFF, Turn off the Flash in the EPR overriding
        // Normal Flash control
        result              = HandleTorch(FlashModeOff, requestId, &bShouldSendNop);
        m_torchStateOn      = FALSE;
        bNormalFlashControl = FALSE;
    }

    if (TRUE == bNormalFlashControl)
    {
        CAMX_ASSERT(NULL != LEDCurrents);

        if (NULL == LEDCurrents)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "LEDCurrents is Null");
            return CamxResultEInvalidPointer;
        }

        if (FlashInfoTypePre == flashType)
        {
            if (LEDCurrents[LEDSetting1] != 0 ||
                LEDCurrents[LEDSetting2] != 0)
            {
                m_lowCurrent[0] = LEDCurrents[LEDSetting1];
                m_lowCurrent[1] = LEDCurrents[LEDSetting2];
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "PreFlash Currents are Zero for RequestId = %llu", requestId);
            }
        }
        else if (FlashInfoTypeMain == flashType)
        {
            if (LEDCurrents[LEDSetting1] != 0 ||
                LEDCurrents[LEDSetting2] != 0)
            {
                m_highCurrent[0] = LEDCurrents[LEDSetting1];
                m_highCurrent[1] = LEDCurrents[LEDSetting2];
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "MainFlash Currents are Zero for RequestId = %llu", requestId);
            }
        }
        else if (FlashInfoTypeCalibration == flashType)
        {
            if (LEDCurrents[LEDSetting1] != 0 ||
                LEDCurrents[LEDSetting2] != 0)
            {
                m_highCurrent[0] = LEDCurrents[LEDSetting1];
                m_highCurrent[1] = LEDCurrents[LEDSetting2];
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "m_highCurrent[0] %d m_highCurrent[1] %d",
                    m_highCurrent[0],
                    m_highCurrent[1]);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "CalibrationFlash Currents are Zero for RequestId = %llu", requestId);
            }
        }

        if (FlashInfoTypeCalibration == flashType)
        {
            result = HandleCalibarationFlash(requestId, &bShouldSendNop);
        }
        else
        {
            result = HandlePreFlash(requestId, &bShouldSendNop, aeModeValue);

            /* If HAL sends Still Capture Request, regardless of preflash state, we want to move on
               and start main flash sequence, so setting flag to FALSE */
            if (ControlCaptureIntentStillCapture == captureIntent)
            {
                isPreflashInProgress = FALSE;

                /* For Non ZSL snapshot all nodes are destoryed hence all node data is destroyed
                 but the algorithm retains the FlashType. Setting the main flash to true based on flashtype */
                if ((LEDCurrents[LEDSetting1] == 0) && (LEDCurrents[LEDSetting2] == 0))
                {
                    m_isMainflashRequired = FALSE;
                }
                else if (FlashInfoTypeMain == flashType)
                {
                    m_isMainflashRequired = TRUE;
                    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "flashType = %d captureIntent = %d", flashType, captureIntent);
                }
            }
            else if (PreFlashState::PreFlashStateInactive != m_preFlashState)
            {
                isPreflashInProgress = TRUE;
            }

            /* If the preflash sequence is going on, there is no need to trigger main flash*/
            if ((FALSE == isPreflashInProgress) ||
                    (m_flashOffDelayCounter >= 0))
            {
                result = HandleMainFlash(requestId, flashMode, captureIntent, &bShouldSendNop);
            }
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Flash: Should send Nop packet = %d", bShouldSendNop);
    if (TRUE == bShouldSendNop)
    {
        if (FALSE == m_pParentNode->IsPipelineStreamedOn())
        {
            if (TRUE == m_initialConfigPending)
            {
                m_initialConfigPending = FALSE;
            }
            else
            {
                Nop(requestId);
            }
        }
        else
        {
            Nop(requestId);
        }
    }

    if (CamxResultSuccess == result)
    {
        UpdateFlashStateMetadata(m_operationType);
    }

    // Store last FlashMode
    m_lastFlashMode = flashMode;

    // What this is for?
    static const UINT FlashModeTag[]         = { FlashMode, PropertyIDRERCompleted};
    static const UINT FlashModeLength        = CAMX_ARRAY_SIZE(FlashModeTag);
    const VOID* pOutputData[FlashModeLength] = { &flashMode, &m_RERCompleted};
    UINT pDataCount[FlashModeLength]         = {1, 1};

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "requestId %lld PropertyID flashMode %d", requestId, flashMode);

    m_pParentNode->WriteDataList(FlashModeTag, pOutputData, pDataCount, FlashModeLength);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Flash::HandleMainFlash
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Flash::HandleMainFlash(
    UINT64                      requestId,
    FlashModeValues             flashMode,
    ControlCaptureIntentValues  captureIntent,
    BOOL* pShouldSendNop)
{
    const StaticSettings* pStaticSettings = m_pHwContext->GetStaticSettings();
    CAMX_ASSERT(pStaticSettings != NULL);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "requestID %lld flashMode %d CaptureIntent %d", requestId, flashMode, captureIntent);

    if (TRUE == pStaticSettings->enableFlashDebug)
    {
        m_isMainflashRequired = TRUE;
    }

    if (m_flashOffDelayCounter > 0)
    {
        m_flashOffDelayCounter--;
    }
    else if ((0 == m_flashOffDelayCounter) && (ControlCaptureIntentStillCapture != captureIntent))
    {
        Fire(FlashOperation::Off, requestId, pShouldSendNop);
        m_flashOffDelayCounter--;
    }
    else if (-1 == m_flashOffDelayCounter)
    {
        /* Capture Intent has to be Still capture AND (flash required required flag or Flash Mode should be on */
        if (ControlCaptureIntentStillCapture == captureIntent)
        {
            m_RERCompleted = false;
            if (TRUE == m_isMainflashRequired)
            {
                /* Main flash: Start here, and waits(sending nop) till delay off counter becomes -1 */
                Fire(FlashOperation::High, requestId, pShouldSendNop);
                m_isMainflashRequired  = FALSE;
                m_flashOffDelayCounter = FlashOffDelay;
            }
            else if (FlashModeOff == flashMode) /* If FlashMode is Off, turn off the flash */
            {
                /* Turn off flash, Only if earlier was not OFF */
                if (flashMode != m_lastFlashMode)
                {
                    Fire(FlashOperation::Off, requestId, pShouldSendNop);
                }
            }
        }
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Flash::HandleTorch
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Flash::HandleTorch(
    FlashModeValues flashMode,
    UINT64          requestId,
    BOOL*           pShouldSendNop)
{
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "flashMode %d m_lastFlashMode %d m_torchStateOn %d requestId %lld", flashMode,
                     m_lastFlashMode, m_torchStateOn, requestId);
    // If there is a state change.
    if (flashMode != m_lastFlashMode)
    {
        if (FlashModeOff == flashMode)
        {
            /* Turn off Torch light */
            Fire(FlashOperation::Off, requestId, pShouldSendNop);
        }
        else if (FlashModeTorch == flashMode)
        {
            /* Turn on torch light*/
            Fire(FlashOperation::Low, requestId, pShouldSendNop);
        }
    }
    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Flash::HandlePreFlash
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Flash::HandlePreFlash(
    UINT64 requestId,
    BOOL* pShouldSendNop,
    ControlAEModeValues aeMode)
{
    CamxResult        result                          = CamxResultSuccess;
    static const UINT PropertiesFlash[]               = { PropertyIDAECFrameControl };
    static const UINT Length                          = CAMX_ARRAY_SIZE(PropertiesFlash);
    VOID*             pData[Length]                   = { 0 };
    UINT64            propertyDataFlashOffset[Length] = { 0 };

    m_pParentNode->GetDataList(PropertiesFlash, pData, propertyDataFlashOffset, Length);

    if (NULL != pData[0])
    {
        AECFrameControl* pFrameControl = reinterpret_cast<AECFrameControl*>(pData[0]);
        PreFlashState    preFlashState = pFrameControl->preFlashState;

        // Only trigger Fire when state change.
        if (m_preFlashState != preFlashState)
        {
            CAMX_LOG_INFO(CamxLogGroupSensor, "SENSOR_FLASH_DEBUG: PreFlashState Change Old=%d New=%d",
                m_preFlashState, preFlashState);
            if (PreFlashState::PreFlashStateStart == preFlashState)
            {
                if (FlashDriverType::PMIC == m_flashType)
                {
                    QueryCurrent();
                }
                Fire(FlashOperation::Low, requestId, pShouldSendNop);
            }
            else if (PreFlashState::PreFlashStateCompleteLED == preFlashState)
            {
                Fire(FlashOperation::Off, requestId, pShouldSendNop);
                m_isMainflashRequired = TRUE;
            }
            else if (PreFlashState::PreFlashStateRER == preFlashState)
            {
                Fire(FlashOperation::Off, requestId, pShouldSendNop);
                m_isMainflashRequired = TRUE;

                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "RERIteration = %d REROnDelayMS = %d REROffDelayMS =%d",
                m_RERIteration, m_REROnDelayMillisecond, m_REROffDelayMillisecond);
                if (ControlAEModeOnAutoFlashRedeye == aeMode)
                {
                    result = FireRERSequence();

                    if (CamxResultSuccess == result)
                    {
                        m_RERCompleted = TRUE;
                    }
                    else
                    {
                        m_RERCompleted = FALSE;
                        CAMX_LOG_ERROR(CamxLogGroupSensor, "FireRERSequence Failed result = %d", result);
                    }
                }
            }
            else if (PreFlashState::PreFlashStateCompleteNoLED == preFlashState)
            {
                Fire(FlashOperation::Off, requestId, pShouldSendNop);
                m_isMainflashRequired = FALSE;
            }
            else
            {
                /* All other preflash states like Trigger AF, AWB, FD, RER? -> Still send nop
                   RER: May not need Nop -> Need to check that when we enable RER */
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "PreFlashState Other:%d -> Need to send NoOp", preFlashState);
                *pShouldSendNop = TRUE;
            }

            m_preFlashState = preFlashState;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Flash::HandleCalibarationFlash
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Flash::HandleCalibarationFlash(
    UINT64 requestId,
    BOOL* pShouldSendNop)
{
    CamxResult        result                          = CamxResultSuccess;
    static const UINT PropertiesCalibFlash[]          = { PropertyIDAECFrameControl };
    static const UINT Length                          = CAMX_ARRAY_SIZE(PropertiesCalibFlash);
    VOID*             pData[Length]                   = { 0 };
    UINT64            propertyDataFlashOffset[Length] = { 0 };

    m_pParentNode->GetDataList(PropertiesCalibFlash, pData, propertyDataFlashOffset, Length);

    if (NULL != pData[0])
    {
        AECFrameControl*      pFrameControl           = reinterpret_cast<AECFrameControl*>(pData[0]);
        CalibrationFlashState calibFlashState         = pFrameControl->calibFlashState;
        AECFlashInfoType      flashInfo               = pFrameControl->flashInfo;

        CAMX_LOG_INFO(CamxLogGroupSensor, "SENSOR_FLASH_DEBUG: flashInfo %d CalibFlashState New=%d, Old=%d",
            flashInfo,  calibFlashState, m_calibFlashState);
        if (CalibrationFlashState::CalibrationFlashCollecting == calibFlashState)
        {
            if (CalibrationFlashState::CalibrationFlashCollecting == m_calibFlashState)
            {
                // Do not run Fire() if current and previous flash state are both collecting
                *pShouldSendNop = TRUE;
            }
            else
            {
                if (FlashDriverType::PMIC == m_flashType)
                {
                    QueryCurrent();
                }
                Fire(FlashOperation::High, requestId, pShouldSendNop);
            }
        }
        else if (CalibrationFlashState::CalibrationFlashComplete == calibFlashState)
        {
            Fire(FlashOperation::Off, requestId, pShouldSendNop);
        }
        else if (CalibrationFlashState::CalibrationPartialComplete == calibFlashState)
        {
            Fire(FlashOperation::Off, requestId, pShouldSendNop);
        }
        else if (CalibrationFlashState::CalibrationFlashReady == calibFlashState)
        {
            if (FlashDriverType::PMIC == m_flashType)
            {
                QueryCurrent();
            }
            Fire(FlashOperation::High, requestId, pShouldSendNop);
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupSensor, "calibFlashState Other:%d -> Need to send NoOp");
            *pShouldSendNop = TRUE;
        }
        m_calibFlashState = calibFlashState;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Flash::CreateInitializePacket
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Flash::CreateInitializePacket()
{
    CamxResult      result               = CamxResultEFailed;
    PacketResource* pResource            = NULL;
    CmdBuffer*      pInitializeCmdBuffer = NULL;
    CmdBuffer*      pI2CCmdBuffer        = NULL;
    CmdBuffer*      pI2CInitCmdBuffer    = NULL;
    CmdBuffer*      pPowerCmdBuffer      = NULL;
    Packet*         pPacket              = NULL;
    UINT            powerUpCmdSize       = m_pFlashData->GetPowerSequenceCmdSize(TRUE);
    UINT            powerDownCmdSize     = m_pFlashData->GetPowerSequenceCmdSize(FALSE);

    result = m_pPacketManager->GetBuffer(&pResource);
    if (CamxResultSuccess == result)
    {
        pPacket = static_cast<Packet*>(pResource);
        if (CamxResultSuccess == m_pInitializeCmdManager->GetBuffer(&pResource))
        {
            pInitializeCmdBuffer = static_cast<CmdBuffer*>(pResource);
            VOID* pCmdBegin = pInitializeCmdBuffer->BeginCommands(sizeof(CSLFlashInfoCmd) / sizeof(UINT32));
            if (NULL != pCmdBegin)
            {
                CSLFlashInfoCmd* pFlashInfo = reinterpret_cast<CSLFlashInfoCmd*>(pCmdBegin);
                m_pFlashData->CreateInitializeCmd(pFlashInfo);
                result = pInitializeCmdBuffer->CommitCommands();

                // Make FlashI2CSlaveInfo Packet and commit only for I2C flash
                if (FlashDriverType::I2C == m_flashType)
                {
                    if (CamxResultSuccess == m_pI2CCmdManager->GetBuffer(&pResource))
                    {
                        pI2CCmdBuffer      = static_cast<CmdBuffer*>(pResource);
                        VOID* pI2CCmdBegin = pI2CCmdBuffer->BeginCommands(sizeof(CSLSensorI2CInfo)
                                                    / sizeof(UINT32));
                        if (NULL != pI2CCmdBegin)
                        {
                            CSLSensorI2CInfo * pFlashI2CInfo = reinterpret_cast<CSLSensorI2CInfo*>(pI2CCmdBegin);
                            m_pFlashData->CreateI2CInfoCmd(pFlashI2CInfo);
                            result = pI2CCmdBuffer->CommitCommands();
                        }
                    }

                    // Make FlashI2CSetting Packet and commit for I2C flash
                    if (CamxResultSuccess == result)
                    {
                        if (CamxResultSuccess == m_pI2CInitCmdManager->GetBuffer(&pResource))
                        {
                            pI2CInitCmdBuffer      = static_cast<CmdBuffer*>(pResource);
                            VOID* pI2CInitCmdBegin = pI2CInitCmdBuffer->BeginCommands(
                            m_pFlashData->GetI2CInitializeCmdSize() / sizeof(UINT32));
                            if (NULL != pI2CInitCmdBegin)
                            {
                                CSLSensorProbeCmd * pFlashI2CInitInfo =
                                            reinterpret_cast<CSLSensorProbeCmd*>(pI2CInitCmdBegin);
                                m_pFlashData->CreateI2CInitializeCmd(pFlashI2CInitInfo);
                                result = pI2CInitCmdBuffer->CommitCommands();
                            }
                        }
                    }

                    if ((CamxResultSuccess == result) && (0 != powerUpCmdSize))
                    {
                        if (CamxResultSuccess == m_pPowerCmdManager->GetBuffer(&pResource))
                        {
                            pPowerCmdBuffer = static_cast<CmdBuffer*>(pResource);
                            VOID* pPowerUpCmdBegin = pPowerCmdBuffer->BeginCommands(powerUpCmdSize / sizeof(UINT32));
                            if (NULL != pPowerUpCmdBegin)
                            {
                                if (CamxResultSuccess == m_pFlashData->CreatePowerSequenceCmd(TRUE, pPowerUpCmdBegin))
                                {
                                    result = pPowerCmdBuffer->CommitCommands();
                                }

                                if ((CamxResultSuccess == result) && (0 != powerDownCmdSize))
                                {
                                    VOID* pPowerDownCmdBegin = pPowerCmdBuffer->BeginCommands(powerDownCmdSize /
                                                                                                sizeof(UINT32));
                                    if (NULL != pPowerDownCmdBegin)
                                    {
                                        if (CamxResultSuccess ==
                                                            m_pFlashData->CreatePowerSequenceCmd(FALSE, pPowerDownCmdBegin))
                                        {
                                            result = pPowerCmdBuffer->CommitCommands();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                if (CamxResultSuccess == result)
                {
                    pPacket->SetOpcode(CSLDeviceTypeFlash, CSLPacketOpcodesFlashInitialConfig);
                    result = pPacket->AddCmdBufferReference(pInitializeCmdBuffer, NULL);

                    if (FlashDriverType::I2C == m_flashType)
                    {
                        result = pPacket->AddCmdBufferReference(pI2CCmdBuffer, NULL);

                        if (CamxResultSuccess == result)
                        {
                            result = pPacket->AddCmdBufferReference(pPowerCmdBuffer, NULL);
                            if (CamxResultSuccess == result)
                            {
                                result = pPacket->AddCmdBufferReference(pI2CInitCmdBuffer, NULL);
                            }
                        }

                    }

                    if (CamxResultSuccess == result)
                    {
                        result = pPacket->CommitPacket();
                        if (CamxResultSuccess == result)
                        {
                            result = m_pHwContext->Submit(m_pParentNode->GetCSLSession(), m_hFlashDevice, pPacket);
                            if (CamxResultSuccess == result)
                            {
                                CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                                                 "Flash init command success Flash device %d",
                                                 m_hFlashDevice);
                            }
                            else
                            {
                                CAMX_LOG_ERROR(CamxLogGroupSensor,
                                               "Flash init command faled Flash device %d",
                                               m_hFlashDevice);
                            }
                        }
                    }
                }
            }
            m_pInitializeCmdManager->Recycle(pInitializeCmdBuffer);

            if (FlashDriverType::I2C == m_flashType)
            {
                m_pI2CInitCmdManager->Recycle(pI2CInitCmdBuffer);
                m_pPowerCmdManager->Recycle(pPowerCmdBuffer);
                m_pI2CCmdManager->Recycle(pI2CCmdBuffer);
            }
        }
        m_pPacketManager->Recycle(pPacket);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Flash::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Flash::Initialize(
    FlashCreateData* pCreateData)
{
    CamxResult result = CamxResultEFailed;

    CSLDeviceAttribute deviceAttribute = { 0 };

    m_pParentNode = pCreateData->pParentNode;

    if (CSLRealtimeOperation == pCreateData->operationMode)
    {
        deviceAttribute.attributeID              = CSLDeviceAttributeRealtimeOperation;
        deviceAttribute.pDeviceAttributeParam    = NULL;
        deviceAttribute.deviceAttributeParamSize = 0;
    }
    else
    {
        deviceAttribute.attributeID              = CSLDeviceAttributeNonRealtimeOperation;
        deviceAttribute.pDeviceAttributeParam    = NULL;
        deviceAttribute.deviceAttributeParamSize = 0;
    }

    SubDeviceProperty flashDevice = SensorSubDevicesCache::GetInstance()->GetSubDevice(pCreateData->cameraId, FlashHandle);

    if (FALSE == flashDevice.isAcquired)
    {
        result = CSLAcquireDevice(m_pParentNode->GetCSLSession(),
                                  &m_hFlashDevice,
                                  m_pFlashData->GetDeviceIndex(),
                                  NULL,
                                  0,
                                  &deviceAttribute,
                                  1,
                                  m_pParentNode->NodeIdentifierString());

        if (CamxResultSuccess == result)
        {
            SensorSubDevicesCache::GetInstance()->SetSubDeviceHandle(m_pParentNode->GetCSLSession(), pCreateData->cameraId,
                m_hFlashDevice, FlashHandle);
        }
    }
    else
    {
        CAMX_ASSERT(flashDevice.hDevice != CSLInvalidHandle);
        CAMX_LOG_INFO(CamxLogGroupSensor, "Reusing flash device handle: %p for cameraId: %d",
            flashDevice.hDevice, pCreateData->cameraId);
        m_hFlashDevice = flashDevice.hDevice;
        result = CamxResultSuccess;
    }

    if (CamxResultSuccess == result)
    {
        m_pHwContext              = pCreateData->pHwContext;
        m_pPacketManager          = pCreateData->pPacketManager;
        m_pI2CCmdManager          = pCreateData->pI2CCmdManager;
        m_pInitializeCmdManager   = pCreateData->pInitializeCmdManager;
        m_pPowerCmdManager        = pCreateData->pFlashPowerCmdManager;
        m_pI2CInitCmdManager      = pCreateData->pI2CInitCmdManager;
        m_pFireCmdManager         = pCreateData->pFireCmdManager;
        m_pI2CFireCmdManager      = pCreateData->pI2CFireCmdManager;
        m_pRERCmdManager          = pCreateData->pRERCmdManager;
        m_pQueryCurrentCmdManager = pCreateData->pQueryCmdManager;
        m_operationMode           = pCreateData->operationMode;
        m_cameraId                = pCreateData->cameraId;

        if (NULL != m_pParentNode)
        {
            m_pParentNode->AddCSLDeviceHandle(m_hFlashDevice);
        }

        result = CreateInitializePacket();
        if (CamxResultSuccess == result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Flash initialized successfully!");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Flash CSLAquire failed %d", result)
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Flash::Fire
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Flash::Fire(
    FlashOperation   operationType,
    UINT64           requestId,
    BOOL*            pShouldSendNop)
{
    CamxResult result = CamxResultEFailed;

    switch (m_flashType)
    {
        case FlashDriverType::PMIC:
            CAMX_LOG_INFO(CamxLogGroupSensor, "FlashDeviceType::PMIC");
            result = FirePMIC(operationType, requestId, pShouldSendNop);
            break;

        case FlashDriverType::I2C:
            CAMX_LOG_INFO(CamxLogGroupSensor, "FlashDeviceType::I2C");
            result = FireI2C(operationType, requestId, pShouldSendNop);
            break;

        default:
            CAMX_LOG_INFO(CamxLogGroupSensor, "FlashDeviceType::Invalid Default to PMIC");
            result = FirePMIC(operationType, requestId, pShouldSendNop);
            break;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Flash::FireI2C
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Flash::FireI2C(
    FlashOperation   operationType,
    UINT64           requestId,
    BOOL*            pShouldSendNop)
{
    CamxResult      result             = CamxResultEFailed;
    PacketResource* pPacketResource    = NULL;
    UINT            fireI2CCmdSize     = m_pFlashData->GetI2CFireCmdSize(operationType);
    CmdBuffer*      pFireI2CCmdBuffer  = NULL;
    Packet*         pPacket            = NULL;

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "SENSOR_FLASH_DEBUG: Flash Fire ReqId=%llu Operation=%d LEDs=%d",
                     requestId, operationType, m_numberOfFlash);

    m_operationType = operationType;
    *pShouldSendNop = TRUE;

    if (m_lastFlashOperation != operationType)
    {
        if (CamxResultSuccess == m_pPacketManager->GetBufferForRequest(
            m_pParentNode->GetCSLSyncId(requestId), &pPacketResource))
        {
            pPacket = static_cast<Packet*>(pPacketResource);
            if (CamxResultSuccess == m_pI2CFireCmdManager->GetBufferForRequest(
                m_pParentNode->GetCSLSyncId(requestId), &pPacketResource))
            {
                pFireI2CCmdBuffer = static_cast<CmdBuffer*>(pPacketResource);
                VOID* pCmdBegin = pFireI2CCmdBuffer->BeginCommands(fireI2CCmdSize / sizeof(UINT32));
                if (NULL != pCmdBegin)
                {
                    CSLSensorProbeCmd* pCmd = reinterpret_cast<CSLSensorProbeCmd*>(pCmdBegin);
                    result                  = m_pFlashData->CreateI2CFireCmd(pCmd, operationType);

                    if (CamxResultSuccess == result)
                    {
                        result = pFireI2CCmdBuffer->CommitCommands();
                    }
                }

                if (CamxResultSuccess == result)
                {
                    CSLPacketOpcodesFlash opcode;

                    if (CSLRealtimeOperation == m_operationMode)
                    {
                        opcode = CSLPacketOpcodesFlashSet;
                        if (FALSE == m_pParentNode->IsPipelineStreamedOn())
                        {
                            if (TRUE == m_initialConfigPending)
                            {
                                opcode                 = CSLPacketOpcodesFlashInitialConfig;
                                m_initialConfigPending = FALSE;
                            }
                        }
                    }
                    else
                    {
                        opcode = CSLPacketOpcodesFlashSetNonRealTime;
                    }

                    pFireI2CCmdBuffer->SetRequestId(m_pParentNode->GetCSLSyncId(requestId));
                    pPacket->SetRequestId(m_pParentNode->GetCSLSyncId(requestId));
                    pPacket->SetOpcode(CSLDeviceTypeFlash, opcode);

                    result = pPacket->AddCmdBufferReference(pFireI2CCmdBuffer, NULL);
                }

                if (CamxResultSuccess == result)
                {
                    result = pPacket->CommitPacket();
                }

                if (CamxResultSuccess == result)
                {
                    result = m_pHwContext->Submit(m_pParentNode->GetCSLSession(), m_hFlashDevice, pPacket);
                    if (CamxResultSuccess == result)
                    {
                        UpdateFlashStateMetadata(m_operationType);
                        *pShouldSendNop = FALSE;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed: %d", result);
                }
            }
        }
        m_lastFlashOperation = operationType;
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Ignore Fire command as FlashOperation match lastFlastOperation %d",
                         m_lastFlashOperation);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Flash::FirePMIC
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Flash::FirePMIC(
    FlashOperation   operationType,
    UINT64           requestId,
    BOOL*            pShouldSendNop)

{
    CamxResult      result          = CamxResultEFailed;
    PacketResource* pPacketResource = NULL;
    UINT            fireCmdSize     = sizeof(CSLFlashFireCmd);
    CmdBuffer*      pFireCmdBuffer  = NULL;
    Packet*         pPacket         = NULL;

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "SENSOR_FLASH_DEBUG: Flash Fire ReqId=%llu Operation=%d LEDs=%d",
                     requestId, operationType, m_numberOfFlash);

    m_operationType = operationType;
    *pShouldSendNop = TRUE;

    if (m_lastFlashOperation != operationType)
    {
        if (CamxResultSuccess == m_pPacketManager->GetBufferForRequest(
            m_pParentNode->GetCSLSyncId(requestId), &pPacketResource))
        {
            pPacket = static_cast<Packet*>(pPacketResource);
            if (CamxResultSuccess == m_pFireCmdManager->GetBufferForRequest(
                m_pParentNode->GetCSLSyncId(requestId), &pPacketResource))
            {
                pFireCmdBuffer = static_cast<CmdBuffer*>(pPacketResource);
                VOID* pCmdBegin = pFireCmdBuffer->BeginCommands(fireCmdSize / sizeof(UINT32));
                if (NULL != pCmdBegin)
                {
                    CSLFlashFireCmd* pCmd = reinterpret_cast<CSLFlashFireCmd*>(pCmdBegin);

                    pCmd->count   = m_numberOfFlash;
                    if (CSLRealtimeOperation == m_operationMode)
                    {
                        if ((TRUE == m_initialConfigPending) && (FALSE == m_pParentNode->IsPipelineStreamedOn()))
                        {
                            pCmd->cmdType = CSLSensorCmdTypeFlashFireInit;
                        }
                        else
                        {
                            pCmd->cmdType = CSLSensorCmdTypeFlashFire;
                        }
                    }
                    else
                    {
                        pCmd->cmdType = CSLSensorCmdTypeFlashWidget;
                    }

                    switch (operationType)
                    {
                        case FlashOperation::Off:
                            CAMX_LOG_INFO(CamxLogGroupSensor, "SENSOR_FLASH_DEBUG: Flash Operation: OFF");
                            pCmd->opcode = CSLFlashOpcodeOff;
                            break;

                        case FlashOperation::Low:
                            CAMX_LOG_INFO(CamxLogGroupSensor,
                                          "SENSOR_FLASH_DEBUG: Flash Operation: Low: numberOfFlashs = %d 1:2 = %d : %d",
                                          m_numberOfFlash,
                                          m_lowCurrent[0],
                                          m_lowCurrent[1]);

                            pCmd->opcode = CSLFlashOpcodeFireLow;
                            for (UINT i = 0; i < pCmd->count; i++)
                            {
                                pCmd->currentMilliampere[i] = m_lowCurrent[i];
                            }
                            break;

                        case FlashOperation::High:
                            CAMX_LOG_INFO(CamxLogGroupSensor,
                                          "SENSOR_FLASH_DEBUG: Flash Operation: High: numberOfFlashs = %d 1:2 = %d : %d",
                                          m_numberOfFlash,
                                          m_highCurrent[0],
                                          m_highCurrent[1]);

                            pCmd->opcode = CSLFlashOpcodeFireHigh;
                            for (UINT i = 0; i < pCmd->count; i++)
                            {
                                pCmd->currentMilliampere[i] = m_highCurrent[i];
                            }
                            break;

                        default:
                            break;
                    }
                    result = pFireCmdBuffer->CommitCommands();
                }

                if (CamxResultSuccess == result)
                {
                    CSLPacketOpcodesFlash opcode;

                    if (CSLRealtimeOperation == m_operationMode)
                    {
                        opcode = CSLPacketOpcodesFlashSet;
                        if (FALSE == m_pParentNode->IsPipelineStreamedOn())
                        {
                            if (TRUE == m_initialConfigPending)
                            {
                                opcode                 = CSLPacketOpcodesFlashInitialConfig;
                                m_initialConfigPending = FALSE;
                            }
                        }
                    }
                    else
                    {
                        opcode = CSLPacketOpcodesFlashSetNonRealTime;
                    }

                    pFireCmdBuffer->SetRequestId(m_pParentNode->GetCSLSyncId(requestId));
                    pPacket->SetRequestId(m_pParentNode->GetCSLSyncId(requestId));
                    pPacket->SetOpcode(CSLDeviceTypeFlash, opcode);

                    result = pPacket->AddCmdBufferReference(pFireCmdBuffer, NULL);
                }

                if (CamxResultSuccess == result)
                {
                    result = pPacket->CommitPacket();
                }

                if (CamxResultSuccess == result)
                {
                    result = m_pHwContext->Submit(m_pParentNode->GetCSLSession(), m_hFlashDevice, pPacket);
                    if (CamxResultSuccess == result)
                    {
                        UpdateFlashStateMetadata(m_operationType);
                        *pShouldSendNop = FALSE;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed: %d", result);
                }
            }
        }
        m_lastFlashOperation = operationType;
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Ignore Fire command as FlashOperation match lastFlastOperation %d",
                         m_lastFlashOperation);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Flash::Nop
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Flash::Nop(
    UINT64         requestId)
{
    CamxResult      result          = CamxResultEFailed;
    PacketResource* pPacketResource = NULL;
    CmdBuffer*      pNopCmdBuffer  = NULL;
    Packet*         pPacket         = NULL;

    if (CamxResultSuccess == m_pPacketManager->GetBufferForRequest(
        m_pParentNode->GetCSLSyncId(requestId), &pPacketResource))
    {
        pPacket = static_cast<Packet*>(pPacketResource);
        if (CamxResultSuccess == m_pFireCmdManager->GetBufferForRequest(
            m_pParentNode->GetCSLSyncId(requestId), &pPacketResource))
        {
            pNopCmdBuffer = static_cast<CmdBuffer*>(pPacketResource);

            pNopCmdBuffer->SetRequestId(m_pParentNode->GetCSLSyncId(requestId));
            pPacket->SetRequestId(m_pParentNode->GetCSLSyncId(requestId));
            pPacket->SetOpcode(CSLDeviceTypeFlash, CSLPacketOpcodesNop);

            result = pPacket->AddCmdBufferReference(pNopCmdBuffer, NULL);

            if (CamxResultSuccess == result)
            {
                result = pPacket->CommitPacket();
            }

            if (CamxResultSuccess == result)
            {
                result = m_pHwContext->Submit(m_pParentNode->GetCSLSession(), m_hFlashDevice, pPacket);
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "NOP Packet: Submitted return = %d requestId=%llu", result, requestId);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Nop command Failed: result=%d requestId= %llu", result, requestId);
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Flash::UpdateFlashStateMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Flash::UpdateFlashStateMetadata(
    FlashOperation operationType)
{
    /// Flash state requested by ISP
    /// numberOfLED = 0 flash off
    /// numberOfLED = 1 single led or first led
    /// numberOfLED = 2 2nd led for dual led case

    UINT              LEDState;
    FlashStateValues  flashState;

    if (FlashOperation::Off == operationType)
    {
        LEDState   = 0;
        flashState = FlashStateReady;
    }
    else
    {
        LEDState   = m_numberOfFlash;
        flashState = FlashStateFired;
    }

    static const UINT WriteProps[]                       = { PropertyIDSensorNumberOfLEDs, FlashState  };
    const VOID*       pData[CAMX_ARRAY_SIZE(WriteProps)] = { &LEDState,                    &flashState };
    UINT              count[CAMX_ARRAY_SIZE(WriteProps)] = { sizeof(UINT),                 1           };

    return m_pParentNode->WriteDataList(WriteProps, pData, count, CAMX_ARRAY_SIZE(WriteProps));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Flash::FireRERSequence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Flash::FireRERSequence()
{
    CamxResult      result          = CamxResultEFailed;
    PacketResource* pPacketResource = NULL;
    UINT            RERCmdSize      = sizeof(CSLFlashRERCmd);
    CmdBuffer*      pRERCmdBuffer   = NULL;
    Packet*         pPacket         = NULL;

    if (CamxResultSuccess == m_pPacketManager->GetBuffer(&pPacketResource))
    {
        pPacket = static_cast<Packet*>(pPacketResource);

        if (CamxResultSuccess == m_pRERCmdManager->GetBuffer(&pPacketResource))
        {
            pRERCmdBuffer   = static_cast<CmdBuffer*>(pPacketResource);
            VOID* pCmdBegin = pRERCmdBuffer->BeginCommands(RERCmdSize / sizeof(UINT32));

            if (NULL != pCmdBegin)
            {
                CSLFlashRERCmd* pCmd           = reinterpret_cast<CSLFlashRERCmd*>(pCmdBegin);
                pCmd->count                    = m_numberOfFlash;
                pCmd->cmdType                  = CSLSensorCmdTypeFlashRER;
                pCmd->numberOfIteration        = m_RERIteration;
                pCmd->flashOnDelayMillisecond  = m_REROnDelayMillisecond;
                pCmd->flashOffDelayMillisecond = m_REROffDelayMillisecond;

                for (UINT i = 0; i < m_numberOfFlash; i++)
                {
                    pCmd->currentMilliampere[i] = m_highCurrent[i];
                }
            }
            result = pRERCmdBuffer->CommitCommands();
        }

        if (CamxResultSuccess == result)
        {
            pPacket->SetOpcode(CSLDeviceTypeFlash, CSLPacketOpcodesFlashSetNonRealTime);
            result = pPacket->AddCmdBufferReference(pRERCmdBuffer, NULL);
        }

        if (CamxResultSuccess == result)
        {
            result = pPacket->CommitPacket();
        }

        if (CamxResultSuccess == result)
        {
            result = m_pHwContext->Submit(m_pParentNode->GetCSLSession(), m_hFlashDevice, pPacket);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "RER Packet Submit Failed: %d", result);
        }

        m_pRERCmdManager->Recycle(pRERCmdBuffer);
        m_pPacketManager->Recycle(pPacket);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Flash::QueryCurrent
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Flash::QueryCurrent()
{
    CamxResult      result                 = CamxResultEFailed;
    PacketResource* pResource              = NULL;
    CmdBuffer*      pQueryCurrentCmdBuffer = NULL;
    Packet*         pPacket                = NULL;

    result = m_pPacketManager->GetBuffer(&pResource);
    if (CamxResultSuccess == result)
    {
        pPacket = static_cast<Packet*>(pResource);
        if (CamxResultSuccess == m_pQueryCurrentCmdManager->GetBuffer(&pResource))
        {
            pQueryCurrentCmdBuffer = static_cast<CmdBuffer*>(pResource);
            VOID* pCmdBegin = pQueryCurrentCmdBuffer->BeginCommands(sizeof(CSLFlashQueryCurrentCmd) / sizeof(UINT32));
            if (NULL != pCmdBegin)
            {
                CSLFlashQueryCurrentCmd* pFlashQueryCurrent = reinterpret_cast<CSLFlashQueryCurrentCmd*>(pCmdBegin);

                pFlashQueryCurrent->cmdType            = CSLSensorCmdTypeFlashQueryCurrent;
                /// @todo (CAMX-2589) Make Flash IOCTL structure to be 64bit aligned and not write directly to command buffer
                pFlashQueryCurrent->currentMilliampere = 0;

                result = pQueryCurrentCmdBuffer->CommitCommands();
                if (CamxResultSuccess == result)
                {
                    pPacket->SetOpcode(CSLDeviceTypeFlash, CSLPacketOpcodesFlashSetNonRealTime);
                    result = pPacket->AddCmdBufferReference(pQueryCurrentCmdBuffer, NULL);
                    if (CamxResultSuccess == result)
                    {
                        result = pPacket->CommitPacket();
                        if (CamxResultSuccess == result)
                        {
                            result = m_pHwContext->Submit(m_pParentNode->GetCSLSession(), m_hFlashDevice, pPacket);
                            if (CamxResultSuccess == result)
                            {
                                CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                                                 "Flash query current command Flash device %d current %d mA",
                                                 m_hFlashDevice,
                                                 pFlashQueryCurrent->currentMilliampere);

                                static const UINT FlashOutput[]       = { PropertyIDSensorFlashCurrent };
                                static const UINT Length              = CAMX_ARRAY_SIZE(FlashOutput);
                                const VOID*       pOutputData[Length] = { &pFlashQueryCurrent->currentMilliampere };
                                UINT              pDataCount[Length]  = { sizeof(pFlashQueryCurrent->currentMilliampere) };

                                result = m_pParentNode->WriteDataList(FlashOutput, pOutputData, pDataCount, Length);
                            }
                            else
                            {
                                CAMX_LOG_ERROR(CamxLogGroupSensor,
                                               "Flash query current command failed Flash device %d",
                                               m_hFlashDevice);
                            }
                        }
                    }
                }
            }
            m_pQueryCurrentCmdManager->Recycle(pQueryCurrentCmdBuffer);
        }
        m_pPacketManager->Recycle(pPacket);
    }

    return result;
}

CAMX_NAMESPACE_END
