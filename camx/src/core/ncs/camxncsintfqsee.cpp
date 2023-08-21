////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxncsintfqsee.cpp
/// @brief CamX NCS Interface QSEE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE CP006: QSEE Sensor interface requires us to use std string class
// NOWHINE FILE CP009: Needed by QMI interface
// NOWHINE FILE GR004: Need to get the const variables within this scope

#include "camxncsintfqsee.h"
#include "camxncssensor.h"
#include "camxncssensordata.h"
#include "sns_client_api_v01.h"


CAMX_NAMESPACE_BEGIN

using namespace std;

#define NCS_DEBUG_DUMP 0

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::GetInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
NCSIntfQSEE* NCSIntfQSEE::GetInstance(
    NCSInitializeInfo*  pInitializeInfo,
    NCSService*         pServiceObject)
{
    CamxResult   result     = CamxResultSuccess;
    NCSIntfQSEE* pNCSObject = NULL;

    static NCSIntfQSEE  intfObject;

    if (intfObject.m_intfState != NCSIntfInvalid)
    {
        result = intfObject.Initialize(pInitializeInfo, pServiceObject);
        if (result != CamxResultSuccess)
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Unable to init the QSEE object");
        }
        else
        {
            pNCSObject = &intfObject;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Fatal: No NCS interface object created");
    }

    return pNCSObject;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::Initialize(
    NCSInitializeInfo*  pInitializeInfo,
    NCSService*         pServiceObject)
{
    CamxResult     result      = CamxResultSuccess;
    NCSSensorData* pSensorData = NULL;
    LDLLNode*      pNode       = NULL;

    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Initializing QSEE");

    m_pQSEEIntfMutex->Lock();
    if (m_intfState == NCSIntfCreated)
    {
        for (UINT i = 0; i < QSEEAccessorPoolLen; i++)
        {
            pSensorData = CAMX_NEW NCSSensorData();
            if (NULL != pSensorData)
            {
                pNode = static_cast<LDLLNode*>(CAMX_CALLOC(sizeof(LDLLNode)));
                if (NULL != pNode)
                {
                    pNode->pData = pSensorData;
                    m_sensorDataObjectList.InsertToTail(pNode);
                }
                else
                {
                    result = CamxResultENoMemory;
                    break;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "Unable to create sensor objects");
                result = CamxResultENoMemory;
                break;
            }
        }

        if (CamxResultSuccess != result)
        {
            pNode = m_sensorDataObjectList.RemoveFromHead();
            while (NULL != pNode)
            {
                pSensorData = static_cast<NCSSensorData*>(pNode->pData);
                if (NULL != pSensorData)
                {
                    CAMX_DELETE pSensorData;
                    pSensorData = NULL;
                }
                CAMX_FREE(pNode);
                pNode = m_sensorDataObjectList.RemoveFromHead();
            }
        }

        // Create a connection link to ssc to query the capabilities of the probed sensors
        if (CamxResultSuccess == result && NULL == m_pProbeLink)
        {
            /// This is a lamda function needed to be passed on to the Sensors API.
            /// The 'this' param is passed on so that it is populated with the data.
            try {
                m_pProbeLink = CAMX_NEW SSCConnection(
                    [this](const uint8_t* pData, size_t size)
                    {
                        this->SensorCallback(pData, size);
                    });
            } catch (const runtime_error& e) {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "Failed to create SSC connection %s", e.what());
            }
            if (NULL == m_pProbeLink)
            {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "Fatal: Unable to create a sensor QSEE connection!");
                result = CamxResultENoMemory;
            }
        }

        if (CamxResultSuccess == result)
        {
            m_pChiContext     = pInitializeInfo->pCallbackData;
            m_attachChiFence  = pInitializeInfo->attachChiFence;
            m_releaseChiFence = pInitializeInfo->releaseChiFence;
            m_signalChiFence  = pInitializeInfo->signalChiFence;
            m_pServiceObject  = pServiceObject;
            m_intfState       = NCSIntfRunning;
        }
    }
    m_pQSEEIntfMutex->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::NCSIntfQSEE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
NCSIntfQSEE::NCSIntfQSEE()
{
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Construct NCSIntfQSEE object......");

    m_sensorList = 0;
    for (UINT i = 0; i < NCSMaxType; i++)
    {
        m_sensorCaps[i].isValid = FALSE;
        m_numSensors[i]         = 0;
    }

    for (UINT i = 0; i < NCSMaxSupportedConns; i++)
    {
        m_sensorConnList[i].phSensorConnHandle = NULL;
    }

    m_pProbeLink     = NULL;
    m_intfState      = NCSIntfInvalid;

    m_pQSEEIntfMutex     = Mutex::Create("QSEEIntfMutex");
    m_pQSEELinkUpdateCond   = Condition::Create("QSEELinkUpdateCondition");
    if (NULL == m_pQSEEIntfMutex || NULL == m_pQSEELinkUpdateCond)
    {
        result = CamxResultENoMemory;

        if (NULL != m_pQSEEIntfMutex)
        {
            m_pQSEEIntfMutex->Destroy();
            m_pQSEEIntfMutex = NULL;
        }

        if (NULL != m_pQSEELinkUpdateCond)
        {
            m_pQSEELinkUpdateCond->Destroy();
            m_pQSEELinkUpdateCond = NULL;
        }

        CAMX_LOG_ERROR(CamxLogGroupNCS, "Unable to create mutex objects for NCS Interface");
    }

    if (CamxResultSuccess == result)
    {
        m_intfState = NCSIntfCreated;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::~NCSIntfQSEE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
NCSIntfQSEE::~NCSIntfQSEE()
{
    NCSSensorData* pSensorData = NULL;
    LDLLNode*      pNode       = NULL;

    if (NULL != m_pQSEEIntfMutex)
    {
        m_pQSEEIntfMutex->Destroy();
        m_pQSEEIntfMutex = NULL;
    }

    if (NULL != m_pQSEELinkUpdateCond)
    {
        m_pQSEELinkUpdateCond->Destroy();
        m_pQSEELinkUpdateCond = NULL;
    }

    // Destroy sensor data objects
    pNode = m_sensorDataObjectList.RemoveFromHead();
    while (NULL != pNode)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Destryoing NCS data objects!!");
        pSensorData = static_cast<NCSSensorData*>(pNode->pData);
        if (NULL != pSensorData)
        {
            CAMX_DELETE pSensorData;
            pSensorData = NULL;
        }
        CAMX_FREE(pNode);

        pNode = m_sensorDataObjectList.RemoveFromHead();
    }

    if (NULL != m_pProbeLink)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Delete ssc connection");
        CAMX_DELETE m_pProbeLink;
        m_pProbeLink = NULL;
    }

    m_intfState = NCSIntfInvalid;

    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "QSEE handle destroyed");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::GetListOfSensors
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::GetListOfSensors()
{
    CamxResult result        = CamxResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Getting list of sensors");

    m_pQSEEIntfMutex->Lock();
    if (m_intfState == NCSIntfRunning)
    {
        /// Probe for the sensors one at a time.
        for (INT sensorType = 0; sensorType < NCSMaxType; sensorType++)
        {
            result = ProbeSensor(static_cast<NCSSensorType>(sensorType));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupNCS, "Probe sensor %d with error %d", sensorType, result);
            }
        }
    }
    m_pQSEEIntfMutex->Unlock();

    // Always return success even probing failed on some sensor
    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::QueryCapabilites
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::QueryCapabilites()
{
    CamxResult      result      = CamxResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Querying for sensor capabilities");

    m_pQSEEIntfMutex->Lock();
    if (m_intfState == NCSIntfRunning)
    {
        for (INT sensorType = 0; sensorType < NCSMaxType; sensorType++)
        {
            // if sensor is probed then query caps, else don't.
            if (0 != (m_sensorList & (1 << sensorType)))
            {
                result = RequestSensorCaps(static_cast<NCSSensorType>(sensorType));
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_WARN(CamxLogGroupNCS, "Failed getting capabilities of sensor %d", sensorType);
                }
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Sensor %d has not been successfully probed", sensorType);
            }
        }
    }
    m_pQSEEIntfMutex->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::SendAttribRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID NCSIntfQSEE::SendAttribRequest(
    INT sensorType)
{
    SSCConnection*         phSSCHandle = NULL;
    SensorUid*             pSUID       = NULL;
    sns_client_request_msg pb_req_msg;
    sns_std_attr_req       pb_attr_req;
    string                 pb_attr_req_encoded;

    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Requesting for attributes for sensor %d", sensorType);

    m_sensorCaps[sensorType].isValid = FALSE;

    pSUID       = &m_suids[sensorType][0];

    phSSCHandle = m_pProbeLink;

    pb_attr_req.set_register_updates(false);
    pb_attr_req.SerializeToString(&pb_attr_req_encoded);

    pb_req_msg.set_msg_id(SNS_STD_MSGID_SNS_STD_ATTR_REQ);
    pb_req_msg.mutable_request()->set_payload(pb_attr_req_encoded);
    pb_req_msg.mutable_suid()->set_suid_high(pSUID->m_high);
    pb_req_msg.mutable_suid()->set_suid_low(pSUID->m_low);
    pb_req_msg.mutable_susp_config()->set_delivery_type(SNS_CLIENT_DELIVERY_WAKEUP);
    pb_req_msg.mutable_susp_config()->set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);

    static UINT8 reqMsg[SNS_CLIENT_REQ_LEN_MAX_V01];
    INT          reqMsgSize = pb_req_msg.ByteSize();

    if (reqMsgSize < SNS_CLIENT_REQ_LEN_MAX_V01)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Request message size=%d", reqMsgSize);
        pb_req_msg.SerializeToArray(reqMsg, reqMsgSize);
        m_pQSEEIntfMutex->Unlock();
        phSSCHandle->SendRequest(reqMsg, reqMsgSize);
        m_pQSEEIntfMutex->Lock();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Request message is too large: size=%d", reqMsgSize);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::SendCalibRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::SendCalibRequest(
    INT            sensorType,
    SSCConnection* phSSCHandle)
{
    SensorUid *            pSUID = NULL;
    sns_client_request_msg pb_req_msg;
    CamxResult             result = CamxResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Requesting for calibration data for sensor %d", sensorType);

    switch (sensorType)
    {
        case NCSGyroType:
            sensorType = NCSGyroCalType;
            break;
        case NCSMagnetometerType:
            sensorType = NCSMagCalType;
            break;
        default:
            sensorType = NCSInvalidType;
            break;
    }

    pSUID = &m_suids[sensorType][0];

    CAMX_ASSERT_MESSAGE((NULL != phSSCHandle), "invalid ssc connection handle");
    if (NULL != phSSCHandle)
    {
        pb_req_msg.set_msg_id(SNS_STD_SENSOR_MSGID_SNS_STD_ON_CHANGE_CONFIG);
        pb_req_msg.mutable_request()->clear_payload();

        pb_req_msg.mutable_suid()->set_suid_high(pSUID->m_high);
        pb_req_msg.mutable_suid()->set_suid_low(pSUID->m_low);

        pb_req_msg.mutable_request()->mutable_batching()->set_batch_period(0);
        pb_req_msg.mutable_request()->mutable_batching()->set_flush_period(UINT32_MAX);

        // @todo (CAMX-2393) check if need for delivery wakeup , refer sensor code.
        pb_req_msg.mutable_susp_config()->set_delivery_type(SNS_CLIENT_DELIVERY_WAKEUP);
        pb_req_msg.mutable_susp_config()->set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);

        static UINT8 reqMsg[SNS_CLIENT_REQ_LEN_MAX_V01];
        INT          reqMsgSize = pb_req_msg.ByteSize();

        if (reqMsgSize < SNS_CLIENT_REQ_LEN_MAX_V01)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Request message size=%d", reqMsgSize);
            pb_req_msg.SerializeToArray(reqMsg, reqMsgSize);
            m_pQSEEIntfMutex->Unlock();
            phSSCHandle->SendRequest(reqMsg, reqMsgSize);
            m_pQSEEIntfMutex->Lock();
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Request message is too large: size=%d", reqMsgSize);
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupNCS, "invalid ssc connection handle");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::SendConfigRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::SendConfigRequest(
    INT connIndex,
    NCSSensorConfig*  pSensorConfig)
{
    string                 config_encoded;
    sns_client_request_msg pb_req_msg;
    sns_std_sensor_config  config;
    SensorUid*             pSUID = NULL;
    SSCConnection*         pConn = NULL;
    NCSSensorType          sensorType;
    CamxResult             result = CamxResultSuccess;

    sensorType = m_sensorConnList[connIndex].sensorType;

    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Sending config request for connIndex %d sensor type %d",
                     connIndex,
                     sensorType);

    pSUID = &m_suids[sensorType][0];
    pConn = m_sensorConnList[connIndex].phSensorConnHandle;

    if ((pConn != NULL) && (pSUID != NULL) && (pSensorConfig != NULL))
    {
        m_sensorConnList[connIndex].suid = *pSUID;

        CAMX_LOG_INFO(CamxLogGroupNCS, "SampleRate %f ReportRate %d sensor type %d suidH 0x%x suidL 0x%x",
                         pSensorConfig->samplingRate,
                         pSensorConfig->reportRate,
                         pSensorConfig->sensorType,
                         pSUID->m_high,
                         pSUID->m_low);

        config.set_sample_rate(pSensorConfig->samplingRate);
        config.SerializeToString(&config_encoded);

        pb_req_msg.set_msg_id(SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_CONFIG);
        pb_req_msg.mutable_request()->set_payload(config_encoded);
        pb_req_msg.mutable_suid()->set_suid_high(pSUID->m_high);
        pb_req_msg.mutable_suid()->set_suid_low(pSUID->m_low);
        pb_req_msg.mutable_susp_config()->set_delivery_type(SNS_CLIENT_DELIVERY_WAKEUP);
        pb_req_msg.mutable_susp_config()->set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);

        static UINT8 reqMsg[SNS_CLIENT_REQ_LEN_MAX_V01];
        INT          reqMsgSize = pb_req_msg.ByteSize();

        if (reqMsgSize < SNS_CLIENT_REQ_LEN_MAX_V01)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Request message size=%d", reqMsgSize);
            pb_req_msg.SerializeToArray(reqMsg, reqMsgSize);
            m_pQSEEIntfMutex->Unlock();
            pConn->SendRequest(reqMsg, reqMsgSize);
            m_pQSEEIntfMutex->Lock();
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Request message is too large: size=%d", reqMsgSize);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Unable to Send config, invalid params conn %p suid %p sensorConfig %p",
                       pConn,
                       pSUID,
                       pSensorConfig);
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::SendDisableRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::SendDisableRequest(
    INT connIndex)
{
    sns_client_request_msg pb_req_msg;
    SensorUid*             pSUID       = NULL ;
    SSCConnection*         phSSCHandle = NULL;
    string                 pb_req_msg_encoded;
    NCSSensorType          sensorType;
    CamxResult             result      = CamxResultSuccess;

    if (connIndex >= 0)
    {
        sensorType  = m_sensorConnList[connIndex].sensorType;
        pSUID       = &m_sensorConnList[sensorType].suid;
        phSSCHandle = m_sensorConnList[connIndex].phSensorConnHandle;

        if ((NULL != pSUID) && (NULL != phSSCHandle))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Sending disable request for connIndex %d sensor type %d",
                connIndex,
                sensorType);

            pb_req_msg.set_msg_id(SNS_CLIENT_MSGID_SNS_CLIENT_DISABLE_REQ);
            pb_req_msg.mutable_suid()->set_suid_high(pSUID->m_high);
            pb_req_msg.mutable_suid()->set_suid_low(pSUID->m_low);
            pb_req_msg.mutable_susp_config()->set_delivery_type(SNS_CLIENT_DELIVERY_WAKEUP);
            pb_req_msg.mutable_susp_config()->set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);

            static UINT8 reqMsg[SNS_CLIENT_REQ_LEN_MAX_V01];
            INT          reqMsgSize = pb_req_msg.ByteSize();

            if (reqMsgSize < SNS_CLIENT_REQ_LEN_MAX_V01)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Request message size=%d", reqMsgSize);
                pb_req_msg.SerializeToArray(reqMsg, reqMsgSize);
                m_pQSEEIntfMutex->Unlock();
                phSSCHandle->SendRequest(reqMsg, reqMsgSize);
                m_pQSEEIntfMutex->Lock();
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "Request message is too large: size=%d", reqMsgSize);
            }
        }
        else
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Sending disable request failed");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Sending disable request failed");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::ProbeSensor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::ProbeSensor(
    NCSSensorType sensorType)
{
    BOOL        isSensorValid = FALSE;
    CamxResult  result        = CamxResultSuccess;
    SuidLookup* pLookup       = NULL;

    /// This is a lamda function needed to be passed on to the Sensors API.
    /// The 'this' param is passed on so that it is populated with the data.
    try {
        pLookup = CAMX_NEW SuidLookup(
            [this](const string& datatype, const vector<SensorUid>& suids)
        {
            SetupSensorLinkOnly(suids, this, datatype);
        });
    } catch (const runtime_error& e) {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Failed to Probe Sensor %d error %s", sensorType, e.what());
    }

    if (NULL != pLookup)
    {
        switch (sensorType)
        {
            case NCSGyroType:
                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Requesting GYRO probe, Waiting max for a 150 msec");
                pLookup->RequestSuid("gyro");
                isSensorValid = TRUE;
                break;
            case NCSAccelerometerType:
                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Requesting ACCEL probe, Waiting max for a 150 msec");
                pLookup->RequestSuid("accel");
                isSensorValid = TRUE;
                break;
            case NCSGravityType:
                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Request Gravity probe, Waiting max for a 150 msec");
                pLookup->RequestSuid("gravity");
                isSensorValid = TRUE;
                break;
            case NCSGyroCalType:
                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Requesting GYRO Calibration probe, Waiting max for a 150 msec");
                pLookup->RequestSuid("gyro_cal");
                isSensorValid = TRUE;
                break;
            case NCSMagCalType:
                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Requesting MAG Calibration probe, Waiting max for a 150 msec");
                pLookup->RequestSuid("mag_cal");
                isSensorValid = TRUE;
                break;
            default:
                break;
        }

        if (TRUE == isSensorValid)
        {
            /// Wait for async callback.
            result = m_pQSEELinkUpdateCond->TimedWait(
                m_pQSEEIntfMutex->GetNativeHandle(),
                QSEEConnectionTimeout);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupNCS, "Timed out for probing sensor %d!", sensorType);
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupNCS, "Probe success for sensor %d!", sensorType);
            }
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Sensor unsupported %d !!",
                             sensorType);
            result = CamxResultSuccess;
        }

        CAMX_DELETE pLookup;
        pLookup = NULL;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Unable to create lookup object, out of memory !!");
        result = CamxResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::RequestSensorCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::RequestSensorCaps(
    NCSSensorType sensorType)
{
    CamxResult      result      = CamxResultSuccess;

    // If the source is a non-calibration source, only then query attributes.
    // Calibration sources don't have attributes to probe for.
    if (FALSE == isCalibSource(sensorType))
    {
        SendAttribRequest(sensorType);
        result = m_pQSEELinkUpdateCond->TimedWait(
            m_pQSEEIntfMutex->GetNativeHandle(),
            QSEEConnectionTimeout);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupNCS, "Failed to query the attributes for sensor %d with result %d",
                           sensorType,
                           result);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Succeeded in query the attributes for sensor %d", sensorType);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::SetBufferLock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL NCSIntfQSEE::SetBufferLock(
    INT connIndex,
    INT start,
    INT end)
{
    INT  curPos;
    BOOL result = FALSE;
    // current position is the location where the sensor would write to in the next callback
    curPos = m_sensorConnList[connIndex].bufferHandles.currentPos;

    if (end >= start)
    {
        // Positions: 0...start...curPos...end...(N-1)
        if ((curPos <= end) && (curPos >= start))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupNCS, "bufLocked start %d end %d curPos %d", start, end, curPos);
            m_sensorConnList[connIndex].bufferHandles.curPosLocked += 1;
            result =  TRUE;
        }
    }
    // If the start and end positions warp around the ring buffer
    else
    {
        // Positions: 0...curPos..end ...start....(N - 1)
        // Positions: 0...end.....start..curPos...(N - 1)
        if ((curPos <= end) || (curPos >= start))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupNCS, "bufLocked start %d end %d curPos %d", start, end, curPos);
            m_sensorConnList[connIndex].bufferHandles.curPosLocked += 1;
            result = TRUE;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::ClearBufferLock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::ClearBufferLock(
    INT connIndex)
{
    CamxResult result = CamxResultSuccess;

    if (0 != m_sensorConnList[connIndex].bufferHandles.curPosLocked)
    {
        m_sensorConnList[connIndex].bufferHandles.curPosLocked -= 1;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::RateMatch
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL NCSIntfQSEE::RateMatch(
    NCSSensorConfig* pSensorConfig,
    INT              connIndex,
    BOOL*            pNeedReconfig)
{
    QSEESensorConn* pSensorConn = NULL;
    BOOL            isFound     = FALSE;

    CAMX_ASSERT((NULL != pSensorConfig) && (0 <= connIndex));

    pSensorConn = &m_sensorConnList[connIndex];
    if (pSensorConn->sensorType == pSensorConfig->sensorType)
    {
        isFound      = TRUE;
        if (pSensorConfig->samplingRate > pSensorConn->curSamplingRate)
        {
            *pNeedReconfig = TRUE;
        }
        else
        {
            *pNeedReconfig = FALSE;
        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Sensor type does not match!");
    }

    return isFound;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::CreateClientSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::CreateClientSession(
    NCSSensorHandle hSensor)
{
    CamxResult      result            = CamxResultSuccess;
    NCSSensorType   sensorType        = NCSMaxType;
    NCSSensor*      pSensorObject     = NULL;
    BOOL            isMatched         = FALSE;
    BOOL            isReconfigNeeded  = FALSE;
    UINT            freeConnIndex     = NCSMaxSupportedConns;

    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Creating SNS client session");

    if (NULL == hSensor)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Invalid input parameter!");
    }
    else
    {
        pSensorObject = static_cast<NCSSensor*>(hSensor);
    }

    if (NULL != pSensorObject)
    {
        sensorType = pSensorObject->GetSensorType();
        CAMX_LOG_INFO(CamxLogGroupNCS, "New register request for sensor %d: sample rate %f, report rate %d",
                    sensorType,
                    pSensorObject->GetSensorConfig()->samplingRate,
                    pSensorObject->GetSensorConfig()->reportRate);
        if (0 != (m_sensorList & (1 << sensorType)))
        {
            for (UINT connIndex = 0; connIndex < NCSMaxSupportedConns; connIndex++)
            {
                if (NULL != m_sensorConnList[connIndex].phSensorConnHandle)
                {
                    isMatched = RateMatch(pSensorObject->GetSensorConfig(), connIndex, &isReconfigNeeded);
                    if (FALSE == isMatched)
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "No match at connIndex %d sensor type %d continuing...",
                                         connIndex, sensorType);
                    }
                    else
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Rate match at connIndex %d sensor type %d, matched %d, reconfig %d",
                                         connIndex, sensorType, isMatched, isReconfigNeeded);

                        if (TRUE == isReconfigNeeded)
                        {
                            CAMX_LOG_INFO(CamxLogGroupNCS, "Need reconfig, doing it");
                            result = ReconfigSession(pSensorObject->GetSensorConfig(), connIndex, FALSE);
                            if (CamxResultSuccess != result)
                            {
                                CAMX_LOG_ERROR(CamxLogGroupNCS,
                                                 "Reconfig failed connIndex %d sensor type %d trying to reconfig %s",
                                                 connIndex, sensorType,
                                                 Utils::CamxResultToString(result));
                            }
                            else
                            {
                                CAMX_LOG_VERBOSE(CamxLogGroupNCS,
                                                "Reconfig success connIndex %d sensor type %d",
                                                 connIndex,
                                                 sensorType);
                                pSensorObject->SetConnIndex(connIndex);
                            }
                        }
                        else
                        {
                            // No need for reconfig, same conn can be reused
                            pSensorObject->SetConnIndex(connIndex);
                            m_sensorConnList[connIndex].mappedClients++;
                            CAMX_LOG_INFO(CamxLogGroupNCS,
                                            "Reused the connection at connIndex %d sensor type %d, num of client %d",
                                             connIndex,
                                             sensorType,
                                             m_sensorConnList[connIndex].mappedClients);
                            // update the freeConnIndex so verbose log will have the valid index
                            freeConnIndex = connIndex;
                        }
                        break;
                    }
                }
                else
                {
                    if (NCSMaxSupportedConns == freeConnIndex)
                    {
                        freeConnIndex = connIndex;
                    }
                }
            }

            if (FALSE == isMatched)
            {
                if (NCSMaxSupportedConns == freeConnIndex)
                {
                    CAMX_LOG_ERROR(CamxLogGroupNCS, "Fatal: Can not find free slot for a QSEE connection");
                }
                else
                {
                    // first time create the SSC connection
                    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Create a QSEE connection for sensor %d", freeConnIndex);
                    result = CreateQSEEConnHandle(freeConnIndex, pSensorObject->GetSensorConfig());
                    if (CamxResultSuccess == result)
                    {
                        pSensorObject->SetConnIndex(freeConnIndex);
                        m_sensorConnList[freeConnIndex].mappedClients = 1;
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupNCS, "Fatal: Unable to create a QSEE connection");
                    }
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Sensor has not been probed!");
            result = CamxResultEUnsupported;
        }
    }

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Mapped client at index %d sensorType %d mapped clients %d",
                         pSensorObject->GetConnIndex(),
                         pSensorObject->GetSensorType(),
                         m_sensorConnList[freeConnIndex].mappedClients);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::CreateQSEEConnHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::CreateQSEEConnHandle(
    INT              connIndex,
    NCSSensorConfig* pSensorConfig)
{
    CamxResult      result          = CamxResultSuccess;
    NCSSensorType   sensorType      = NCSMaxType;
    NCSSensorType   calibSensorType = NCSMaxType;
    size_t          bufferSize      = 0;
    UINT            bufferStride    = 0;
    SensorUid*      pSensorSUID     = NULL;
    SSCConnection*  phSSCHandle     = NULL;

    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "NCS: connIdx %d sampling rate %f report rate %d",
                     connIndex, pSensorConfig->samplingRate, pSensorConfig->reportRate);

    sensorType = pSensorConfig->sensorType;
    if (FALSE == isValidSensorType(sensorType))
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Invalid sensor type: %d", sensorType);
        return CamxResultEInvalidArg;
    }

    // Allocate buffer only if the sensor sampling rate is valid
    if (pSensorConfig->samplingRate > 0.0f)
    {
        m_sensorConnList[connIndex].bufferHandles.phBufferHandle =
            AllocSensorStreamBuf(pSensorConfig, &bufferSize, &bufferStride);
        if (NULL != m_sensorConnList[connIndex].bufferHandles.phBufferHandle)
        {
            m_sensorConnList[connIndex].bufferHandles.bufferSize      = bufferSize;
            m_sensorConnList[connIndex].bufferHandles.currentPos      = 0;
            m_sensorConnList[connIndex].bufferHandles.bufferStride    = bufferStride;
            m_sensorConnList[connIndex].bufferHandles.totalSamples    = bufferSize/bufferStride;
            m_sensorConnList[connIndex].bufferHandles.curPosLocked    = 0;
            m_sensorConnList[connIndex].bufferHandles.isBufferWrapped = FALSE;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Create ring buffer failed");
            result = CamxResultENoMemory;
        }

        if (CamxResultSuccess == result)
        {
#if NCS_DEBUG_DUMP
            m_bufferHandles[connIndex].pFileBuffer = OsUtils::FOpen(NCSLogPath, "w+");
            if (NULL == m_bufferHandles[connIndex].pFileBuffer)
            {
                CAMX_LOG_WARN(CamxLogGroupNCS, "Unable to open dump file !!");
            }
#endif // NCS_DEBUG_DUMP
        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Invalid sampling rate %f, No initialization done for this sensor %d",
                         pSensorConfig->samplingRate, connIndex);
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        /// Create ssc connection for a client.
        /// This is a lamda function needed to be passed on to the Sensors API.
        /// The 'this' param is passed on so that it is populated with the data.
        try {
            phSSCHandle = CAMX_NEW SSCConnection(
                [this](const uint8_t* pData, size_t size)
            {
                this->SensorCallback(pData, size);
            });
        } catch (const runtime_error& e) {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Failed to create SSC connection %s", e.what());
        }

        if (NULL == phSSCHandle)
        {
            result = CamxResultENoMemory;

            CAMX_FREE(m_sensorConnList[connIndex].bufferHandles.phBufferHandle);
            m_sensorConnList[connIndex].bufferHandles.phBufferHandle = NULL;
        }

        if (CamxResultSuccess == result)
        {
            m_sensorConnList[connIndex].phSensorConnHandle = phSSCHandle;
            m_sensorConnList[connIndex].sensorType = sensorType;

            /// Use the suid from the suid list of the sensors probed during bootup.
            /// Copy the same into the connection list.
            pSensorSUID = &m_suids[sensorType][0];
            if (NULL != pSensorSUID)
            {
                m_sensorConnList[connIndex].suid = *pSensorSUID;
            }

            SendConfigRequest(connIndex, pSensorConfig);
            m_sensorConnList[connIndex].connectionState = QSEEConnRunning;
            // Initialize last seen timestamp to the point when we send config request to sensor.
            OsUtils::GetTime(&m_sensorConnList[connIndex].lastSeenTime);

            m_sensorConnList[connIndex].curSamplingRate = pSensorConfig->samplingRate;
            m_sensorConnList[connIndex].curReportRate = pSensorConfig->reportRate;

            // Create corresponding calibration source links and configure the same over the connection created earlier
            if (TRUE == isCalibNeeded(sensorType))
            {
                SendCalibRequest(sensorType, phSSCHandle);
                switch (sensorType)
                {
                    case NCSGyroType:
                        calibSensorType = NCSGyroCalType;
                        break;
                    case NCSMagnetometerType:
                        calibSensorType = NCSMagCalType;
                        break;
                    default:
                        calibSensorType = NCSMaxType;
                        break;
                }
                if (TRUE == isValidSensorType(calibSensorType))
                {
                    m_sensorConnList[connIndex].calibSUID = m_suids[calibSensorType][0];
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupNCS, "Invalid sensor type");
                }
            }

            if (m_sensorConnList[connIndex].sensorType == NCSGyroType               ||
                m_sensorConnList[connIndex].sensorType == NCSAccelerometerType      ||
                m_sensorConnList[connIndex].sensorType == NCSMagnetometerType       ||
                m_sensorConnList[connIndex].sensorType == NCSGravityType            ||
                m_sensorConnList[connIndex].sensorType == NCSLinearAccelerationType ||
                m_sensorConnList[connIndex].sensorType == NCSTimeOfFlightType       ||
                m_sensorConnList[connIndex].sensorType == NCSLightType)
            {
                m_sensorConnList[connIndex].needMonitor = TRUE;
            }
            else
            {
                m_sensorConnList[connIndex].needMonitor = FALSE;
            }

            CAMX_LOG_INFO(CamxLogGroupNCS,
                             "Conn Idx %d sensorType %d buffer Q number %d suidH 0x%llx, suidL 0x%llx",
                             connIndex,
                             sensorType,
                             m_sensorConnList[connIndex].bufferHandles.totalSamples,
                             m_sensorConnList[connIndex].suid.m_high,
                             m_sensorConnList[connIndex].suid.m_low);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::GetDataAsync
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::GetDataAsync(
    UINT64        tStart,
    UINT64        tEnd,
    INT           connIndex,
    VOID*         pFence)
{
    NCSAsyncRequest* pAsyncRequest        = NULL;
    BOOL             isSensorValid        = TRUE;
    NCSSensorType    sensorType           = NCSMaxType;
    CamxResult       result               = CamxResultSuccess;

    if ((NULL == pFence) || (tStart > tEnd) || (connIndex < 0) || (connIndex >= static_cast<INT>(NCSMaxSupportedConns)))
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Invalid input parameters: tStart=%d, tEnd=%d, connIndex=%d, pFence=%p",
            tStart, tEnd, connIndex, pFence);
        return CamxResultEInvalidArg;
    }

    m_pQSEEIntfMutex->Lock();
    if (m_intfState == NCSIntfRunning)
    {
        sensorType = m_sensorConnList[connIndex].sensorType;
        switch (sensorType)
        {
            case NCSGyroType:
            case NCSAccelerometerType:
            case NCSGravityType:
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupNCS, "Unsupported sensor type");
                isSensorValid =  FALSE;
                break;
        }

        if ((TRUE == isSensorValid) &&
            (QSEEConnRunning == m_sensorConnList[connIndex].connectionState))
        {
            pAsyncRequest = static_cast<NCSAsyncRequest*>(CAMX_CALLOC(sizeof(NCSAsyncRequest)));
            if (NULL != pAsyncRequest)
            {
                pAsyncRequest->tEnd    = tEnd;
                pAsyncRequest->tStart  = tStart;
                pAsyncRequest->hChiFence = (static_cast<CHIFENCEHANDLE>(pFence));
                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "tStart %llu tEnd %llu phFence %p",
                               tStart, tEnd, pAsyncRequest->hChiFence);
                LDLLNode* pNode = static_cast<LDLLNode*>(CAMX_CALLOC(sizeof(LDLLNode)));
                if (NULL != pNode)
                {
                    pNode->pData = pAsyncRequest;

                    // Attach to fence before enqueuing async request
                    result = m_attachChiFence(m_pChiContext, pFence);

                    if (CamxResultSuccess == result)
                    {
                        m_sensorConnList[connIndex].asyncRequestQ.InsertToTail(pNode);
                    }
                }
                else
                {
                    result = CamxResultENoMemory;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "Unable to allocate async request object");
                result = CamxResultENoMore;
            }
            CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Pending Async requests %d connIndex %d",
                             m_sensorConnList[connIndex].asyncRequestQ.NumNodes(), connIndex);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Invalid sensor or connection in invalid state: %d",
                           m_sensorConnList[connIndex].connectionState);
            result = CamxResultEInvalidState;
        }
    }
    m_pQSEEIntfMutex->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::TriggerClientFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::TriggerClientFence(
    QSEEJob* pJob)
{
    UINT64                       tStart          = 0;
    UINT64                       tEnd            = 0;
    UINT64                       tCurrent        = 0;
    CHIFENCEHANDLE               hFence          = NULL;
    NCSAsyncRequest*             phRequestHandle = NULL;
    LightweightDoublyLinkedList* pClientRequestQ = NULL;
    CamxResult                   result          = CamxResultSuccess;
    INT                          connIndex       = NCSMaxSupportedConns;

    CAMX_ASSERT(NULL != pJob);

    connIndex = pJob->connIndex;
    CAMX_ASSERT_MESSAGE((connIndex >= 0) && (connIndex < static_cast<INT>(NCSMaxSupportedConns)),
                "connIndex %d", connIndex);

    pClientRequestQ = &m_sensorConnList[connIndex].asyncRequestQ;
    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Pending number of async requests %d on connIndex %d",
                        pClientRequestQ->NumNodes(), connIndex);
    if (pClientRequestQ->NumNodes() > 0)
    {
        LDLLNode* pNext = NULL;
        LDLLNode* pNode = pClientRequestQ->Head();
        for (; NULL != pNode; pNode = pNext)
        {
            pNext = LightweightDoublyLinkedList::NextNode(pNode);
            phRequestHandle = static_cast<NCSAsyncRequest*>(pNode->pData);
            if (NULL == phRequestHandle)
            {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "pRequestHandle NULL");
                pClientRequestQ->RemoveNode(pNode);
                CAMX_FREE(pNode);
                continue;
            }

            tStart   = phRequestHandle->tStart;
            tEnd     = phRequestHandle->tEnd;
            hFence   = phRequestHandle->hChiFence;
            tCurrent = pJob->timestamp;

            if (CamxResultSuccess == pJob->resultStatus)
            {
                if (tCurrent < tStart || tCurrent < tEnd)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Data is not ready for tStart %llu tEnd %llu tCurrent %llu",
                                    tStart, tEnd, tCurrent);
                    break;
                }
                // Trigger the fence on this and free the async data memory
                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "triggering fence %p for t1 %llu t2 %llu",
                                    hFence, tStart, tEnd);
                // Signal the fence on which the consumer is waiting.
                m_signalChiFence(m_pChiContext, hFence, pJob->resultStatus);
                m_releaseChiFence(m_pChiContext, hFence);
            }
            // Incase of error clear up all the fences pending to be signalled-with error.
            else
            {
                // Trigger the fence on this and free the async data memory
                CAMX_LOG_WARN(CamxLogGroupNCS, "Invalid state %s, triggering fence %d for t1 %llu t2 %llu",
                                Utils::CamxResultToString(pJob->resultStatus),
                                hFence, tStart, tEnd);
                m_signalChiFence(m_pChiContext, hFence, pJob->resultStatus);
                m_releaseChiFence(m_pChiContext, hFence);
            }

            CAMX_FREE(phRequestHandle);
            phRequestHandle = NULL;

            pClientRequestQ->RemoveNode(pNode);
            CAMX_FREE(pNode);
        }
    }
    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Exit!");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::GetSampleTimestamp
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::GetSampleTimestamp(
    INT connIndex,
    INT pos,
    UINT64* pTs)
{
    CamxResult   result                = CamxResultSuccess;
    UINT         bufferStride          = 0;
    UINT8*       phSensorBufferHandle  = NULL;

    phSensorBufferHandle = static_cast<UINT8*>(m_sensorConnList[connIndex].bufferHandles.phBufferHandle);
    bufferStride         = m_sensorConnList[connIndex].bufferHandles.bufferStride;
    switch (m_sensorConnList[connIndex].sensorType)
    {
        case NCSGyroType:
            *pTs  = reinterpret_cast<NCSDataGyro*>(phSensorBufferHandle + pos*bufferStride)->timestamp;
            break;
        case NCSAccelerometerType:
            *pTs  = reinterpret_cast<NCSDataAccel*>(phSensorBufferHandle + pos*bufferStride)->timestamp;
            break;
        case NCSGravityType:
            *pTs = reinterpret_cast<NCSDataGravity*>(phSensorBufferHandle + pos*bufferStride)->timestamp;
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Unsupported type");
            result = CamxResultENoSuch;
            break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::GetDataSync
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
NCSSensorDataHandle NCSIntfQSEE::GetDataSync(
    UINT64 tStart,
    UINT64 tEnd,
    INT    connIndex)
{
    INT                currentPos            = 0;
    INT                prevLocation          = 0;
    INT                endLocation           = 0;
    INT                startLocation         = 0;
    UINT               bufferSize            = 0;
    UINT               bufferStride          = 0;
    UINT8*             phSensorBufferHandle  = NULL;
    NCSSensorData*     phNCSDataHandle       = NULL;
    CamxResult         result                = CamxResultSuccess;
    INT                bufferQLength         = 0;
    UINT64             tCurrent              = 0;
    UINT64             tDiff                 = 0;
    INT                startOffset           = 0;
    INT                loop;

    CAMX_ASSERT((tStart <= tEnd) && (connIndex >= 0) && (connIndex < static_cast<INT>(NCSMaxSupportedConns)));

    if ((tStart > tEnd) || (connIndex < 0) || (connIndex >= static_cast<INT>(NCSMaxSupportedConns)))
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Invalid input parameters: tStart=%d, tEnd=%d, connIndex=%d",
            tStart, tEnd, connIndex);
        return NULL;
    }

    m_pQSEEIntfMutex->Lock();
    if (m_intfState == NCSIntfRunning)
    {
        // Intialize buffer parameters
        bufferSize           = static_cast<UINT>(m_sensorConnList[connIndex].bufferHandles.bufferSize);
        bufferStride         = m_sensorConnList[connIndex].bufferHandles.bufferStride;
        currentPos           = m_sensorConnList[connIndex].bufferHandles.currentPos;
        bufferQLength        = static_cast<INT>(m_sensorConnList[connIndex].bufferHandles.totalSamples);
        phSensorBufferHandle = static_cast<UINT8*>(m_sensorConnList[connIndex].bufferHandles.phBufferHandle);

        // Fetch a accessor object from the pool
        phNCSDataHandle = static_cast<NCSSensorData*>(GetAccessorObject());
        if ((NULL != phNCSDataHandle) &&
            (QSEEConnRunning == m_sensorConnList[connIndex].connectionState))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Current Queue size %d pNCSDataHandle %p",
                           m_sensorDataObjectList.NumNodes(), phNCSDataHandle);

            if (CamxResultSuccess == result)
            {
                prevLocation = currentPos;
                for (loop = 0; loop < bufferQLength; loop++)
                {
                    prevLocation--;
                    if (prevLocation < 0)
                    {
                        prevLocation = bufferQLength -1;
                    }
                    result = GetSampleTimestamp(connIndex, prevLocation, &tCurrent);
                    if (tCurrent > tEnd)
                    {
                        continue;
                    }
                    endLocation = prevLocation + 1;
                    break;
                }

                if ( loop < bufferQLength)
                {
                    tDiff         = tEnd - tStart;
                    startOffset     = static_cast<UINT>(tDiff / m_sensorConnList[connIndex].bufferHandles.tickPeriod);
                    startOffset++;
                    startLocation = ((endLocation - startOffset) + bufferQLength) % bufferQLength;
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupNCS, "No sample matches required timestamp %llu!", tEnd);
                    result = CamxResultENoMore;
                }
            }

            if (CamxResultSuccess == result)
            {
                result = phNCSDataHandle->SetDataLimits(startLocation, endLocation,
                                                        reinterpret_cast<VOID*>
                                                        (m_sensorConnList[connIndex].bufferHandles.phBufferHandle),
                                                        bufferQLength);
                if (CamxResultSuccess == result)
                {
                    phNCSDataHandle->SetBufferStride(
                        m_sensorConnList[connIndex].bufferHandles.bufferStride);
                    SetBufferLock(
                        connIndex, startLocation, endLocation);
                    CAMX_LOG_VERBOSE(CamxLogGroupNCS,
                        "Sensor %d: bufferQlength %d, tStart %llu, tEnd %llu, startLocation %d, endLocation %d",
                        m_sensorConnList[connIndex].sensorType,
                        bufferQLength, tStart, tEnd, startLocation, endLocation);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupNCS, "Unable to set data limits");
                }
            }
        }

        if (CamxResultSuccess != result)
        {
            if (CamxResultSuccess !=  PutAccessorObject(phNCSDataHandle))
            {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "Memory Leak, can not put back sesnor data object");
            }
            phNCSDataHandle = NULL;
        }
    }

    if (NULL == phNCSDataHandle)
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Can not get sensor data for sensor %d!", connIndex);
    }
    m_pQSEEIntfMutex->Unlock();

    return phNCSDataHandle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::GetLastNSamples
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
NCSSensorDataHandle NCSIntfQSEE::GetLastNSamples(
    UINT  numOfSamples,
    INT   connIndex)
{
    INT                currentPos           = 0;
    INT                startLocation        = 0;
    INT                endLocation          = 0;
    UINT               bufferSize           = 0;
    UINT               bufferStride         = 0;
    UINT8*             phSensorBufferHandle = NULL;
    NCSSensorData*     phNCSDataHandle      = NULL;
    CamxResult         result               = CamxResultEFailed;
    INT                bufferQLength        = 0;

    if (0 == numOfSamples || (connIndex < 0) || (connIndex >= static_cast<INT>(NCSMaxSupportedConns)))
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Invalid paramters: numofSample=%d, connectindex=%d",
            numOfSamples, connIndex);
        return NULL;
    }

    m_pQSEEIntfMutex->Lock();
    if (m_intfState == NCSIntfRunning)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Enter......");

        phSensorBufferHandle = static_cast<UINT8*>(m_sensorConnList[connIndex].bufferHandles.phBufferHandle);
        bufferSize           = static_cast<UINT>(m_sensorConnList[connIndex].bufferHandles.bufferSize);
        bufferStride         = m_sensorConnList[connIndex].bufferHandles.bufferStride;
        currentPos           = m_sensorConnList[connIndex].bufferHandles.currentPos;
        bufferQLength        = static_cast<INT>(m_sensorConnList[connIndex].bufferHandles.totalSamples);

        CAMX_ASSERT_MESSAGE((NULL != phSensorBufferHandle) && (0 != bufferSize) && (0 != bufferStride),
                            "Unable to save data, invalid state !!");

        phNCSDataHandle = static_cast<NCSSensorData*>(GetAccessorObject());
        if ((NULL != phNCSDataHandle) &&
            (QSEEConnRunning == m_sensorConnList[connIndex].connectionState))
        {
            if (TRUE == m_sensorConnList[connIndex].bufferHandles.isBufferWrapped)
            {
                if (numOfSamples <= static_cast<UINT>(currentPos))
                {
                    endLocation   = currentPos - 1;
                    startLocation = currentPos - numOfSamples;
                }
                else
                {
                    startLocation = bufferQLength - (numOfSamples - currentPos);
                    if (currentPos == 0)
                    {
                        endLocation = bufferQLength - 1;
                    }
                    else
                    {
                        endLocation = currentPos - 1;
                    }
                }
                result = CamxResultSuccess;
            }
            else
            {
                if (numOfSamples <= static_cast<UINT>(currentPos))
                {
                    endLocation   = currentPos - 1;
                    startLocation = currentPos - numOfSamples;
                    result        = CamxResultSuccess;
                }
                else
                {
                    result = CamxResultENoSuch;
                    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "sensor %d: Not enough sample data: numOfSamples %d, currentPos %d",
                        connIndex, numOfSamples, currentPos);
                }
            }

            if (CamxResultSuccess == result)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Sensor %d: startLocation %d, endLocation %d numOfSamples %d",
                    m_sensorConnList[connIndex].sensorType, startLocation, endLocation, numOfSamples)
                result  = phNCSDataHandle->SetDataLimits(startLocation,
                                                         endLocation,
                                                         reinterpret_cast<VOID*>(
                                                            m_sensorConnList[connIndex].bufferHandles.phBufferHandle),
                                                         bufferQLength);
                if (CamxResultSuccess == result)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Return access object %p", phNCSDataHandle);
                    phNCSDataHandle->SetBufferStride(m_sensorConnList[connIndex].bufferHandles.bufferStride);
                    // todo (CAMX-2393) readlock/write lock instead of counter to lock the access.
                    // Set the lock flag in case the current cursor position where data would be queued is in the acessed range
                    if (TRUE == SetBufferLock(connIndex, startLocation, endLocation))
                    {
                        phNCSDataHandle->SetHaveBufLocked();
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupNCS, "Unable to set data limits");
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "No more free data objects or connection in invalid state %d",
                           m_sensorConnList[connIndex].connectionState);
            result = CamxResultEFailed;
        }

        if (CamxResultSuccess != result && NULL != phNCSDataHandle)
        {
            result = PutAccessorObject(phNCSDataHandle);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "Fatal: Unable to push back the accessor !!");
            }
            phNCSDataHandle =  NULL;
        }
    }

    m_pQSEEIntfMutex->Unlock();

    return phNCSDataHandle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::Release
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::Release(
    NCSSensorHandle hSensor)
{
    UINT8*           pRingBuffer   = NULL;
    SSCConnection*   pSensorConn   = NULL;
    NCSAsyncRequest* pRequest      = NULL;
    NCSSensor*       pSensorObj    = NULL;
    INT              connIndex     = 0;

    CAMX_ASSERT(NULL != hSensor);

    pSensorObj = static_cast<NCSSensor*>(hSensor);
    connIndex = pSensorObj->GetConnIndex();

    CAMX_ASSERT_MESSAGE((connIndex >= 0) && (connIndex < static_cast<INT>(NCSMaxSupportedConns)),
                        "connIndex %d", connIndex);

    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Number of mapped clients at connIndex %d is %d",
                   connIndex,
                   m_sensorConnList[connIndex].mappedClients);

    if (m_sensorConnList[connIndex].mappedClients > 0)
    {
        m_sensorConnList[connIndex].mappedClients--;
        if (0 == m_sensorConnList[connIndex].mappedClients)
        {
            pSensorConn = m_sensorConnList[connIndex].phSensorConnHandle;
            // Destroy the connection to keep callbacks from coming
            m_sensorConnList[connIndex].connectionState = QSEEConnStopped;
            if (NULL != pSensorConn)
            {
                m_sensorConnList[connIndex].phSensorConnHandle = NULL;
                m_pQSEEIntfMutex->Unlock();
                CAMX_DELETE pSensorConn;
                m_pQSEEIntfMutex->Lock();

                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Disabling streaming for conn index %d", connIndex);
            }

            // Destroy the memory items being used in callback: Ring buffer
            pRingBuffer = reinterpret_cast<UINT8*>(m_sensorConnList[connIndex].bufferHandles.phBufferHandle);
            if (NULL != pRingBuffer)
            {
                CAMX_FREE(pRingBuffer);
                m_sensorConnList[connIndex].bufferHandles.phBufferHandle = NULL;
                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Releasing ring buffer for conn index %d", connIndex);
            }

            // Debug file handle close
            if (NULL != m_sensorConnList[connIndex].bufferHandles.pFileBuffer)
            {
                OsUtils::FClose(m_sensorConnList[connIndex].bufferHandles.pFileBuffer);
            }

            // Cleanup the async request Q, if any pending requests and destroy the queue
            LDLLNode* pNode = m_sensorConnList[connIndex].asyncRequestQ.RemoveFromHead();
            while (NULL != pNode)
            {
                pRequest = static_cast<NCSAsyncRequest*>(pNode->pData);
                if (NULL != pRequest)
                {
                    CAMX_FREE(pRequest);
                    pRequest = NULL;
                }
                CAMX_FREE(pNode);
                pNode = m_sensorConnList[connIndex].asyncRequestQ.RemoveFromHead();
            }
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Remaining number of clients mapped to connIndex %d is %d",
                     connIndex,
                     m_sensorConnList[connIndex].mappedClients);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::ReconfigSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::ReconfigSession(
    NCSSensorConfig* pConfig,
    INT              connIndex,
    BOOL             recreateSSCLink)
{
    SSCConnection*  pSSCConn      = NULL;
    RingBuffer*     pBuffer       = NULL;
    CamxResult      result        = CamxResultEFailed;

    CAMX_ASSERT(NULL != pConfig);

    if (QSEEConnRunning == m_sensorConnList[connIndex].connectionState)
    {
        CAMX_ASSERT_MESSAGE((connIndex >= 0) && (connIndex < static_cast<INT>(NCSMaxSupportedConns)),
                            "connIndex %d", connIndex);

        // Trigger pending async requests
        QSEEJob qseeJob      = { 0 };
        qseeJob.connIndex    = connIndex;
        qseeJob.resultStatus = CamxResultEDisabled;
        qseeJob.timestamp    = 0;

        result = TriggerClientFence(&qseeJob);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Triggering Client fence failed");
        }
        else
        {
            // if there is need to recreate the link
            if (TRUE == recreateSSCLink)
            {
                pSSCConn = m_sensorConnList[connIndex].phSensorConnHandle;
                if (NULL != pSSCConn)
                {
                    m_sensorConnList[connIndex].phSensorConnHandle = NULL;
                    m_pQSEEIntfMutex->Unlock();
                    CAMX_DELETE pSSCConn;
                    m_pQSEEIntfMutex->Lock();

                    if (QSEEConnRunning == m_sensorConnList[connIndex].connectionState &&
                        NULL == m_sensorConnList[connIndex].phSensorConnHandle)
                    {
                         // Create anew an ssc connection
                         try {
                             pSSCConn = CAMX_NEW SSCConnection(
                                 [this](const uint8_t* pData, size_t size)
                             {
                                 this->SensorCallback(pData, size);
                             });
                        } catch (const runtime_error& e) {
                            CAMX_LOG_ERROR(CamxLogGroupNCS, "Failed to create SSC connection %s", e.what());
                        }

                         if (NULL != pSSCConn)
                         {
                             m_sensorConnList[connIndex].phSensorConnHandle = pSSCConn;
                             result = CamxResultSuccess;
                         }
                         else
                         {
                             CAMX_LOG_ERROR(CamxLogGroupNCS, "Unable to create a sensor QSEE connection");
                             result = CamxResultENoMemory;
                         }
                    }
                }
            }

            if (CamxResultSuccess == result                                 &&
                NULL != m_sensorConnList[connIndex].phSensorConnHandle      &&
                QSEEConnRunning == m_sensorConnList[connIndex].connectionState)
            {
                // Send disable request to stop the callbacks
                SendDisableRequest(connIndex);

                pBuffer     = &m_sensorConnList[connIndex].bufferHandles;
                // Recheck if handle is NULL to avoid race condition
                // when camera close happens simultaneously
                if (NULL != m_sensorConnList[connIndex].phSensorConnHandle        &&
                   QSEEConnRunning == m_sensorConnList[connIndex].connectionState &&
                   NULL != pBuffer->phBufferHandle)
                {
                    m_sensorConnList[connIndex].connectionState = QSEEConnStopped;

                    // Reset state variables of the ring buffer
                    pBuffer->curPosLocked    = 0;
                    pBuffer->currentPos      = 0;
                    pBuffer->isBufferWrapped = FALSE;

                    // Reset ring buffer data to '0'
                    Utils::Memset(pBuffer->phBufferHandle, 0, pBuffer->bufferSize);

                    // Send reconfig request
                    SendConfigRequest(connIndex, pConfig);
                    // Recheck if handle is NULL to avoid race condition
                    if (NULL != m_sensorConnList[connIndex].phSensorConnHandle)
                    {
                        m_sensorConnList[connIndex].curSamplingRate = pConfig->samplingRate;
                        m_sensorConnList[connIndex].connectionState = QSEEConnRunning;
                    }
                    result = CamxResultSuccess;
                }
                else
                {
                    result = CamxResultEInvalidState;
                }
            }

        }
    }
    else
    {
        result = CamxResultEInvalidState;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::FillCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::FillCaps(
    NCSSensorCaps* pCaps,
    NCSSensorType  sensorType)
{
    CamxResult result = CamxResultEUnsupported;

    CAMX_ASSERT_MESSAGE((NULL != pCaps), "Pointer to be filled is NULL !");

    m_pQSEEIntfMutex->Lock();
    if (m_intfState == NCSIntfRunning)
    {
        // if the sensor is available then fill the caps structure
        if (0 != (m_sensorList & (1 << sensorType)))
        {
            *pCaps = m_sensorCaps[sensorType];
            result = CamxResultSuccess;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Sensor %d probe failed during boot", sensorType);
            result = CamxResultEFailed;
        }
    }
    m_pQSEEIntfMutex->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::SetupSensorLinkOnly
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID NCSIntfQSEE::SetupSensorLinkOnly(
    const std::vector<SensorUid>& suids,
    NCSIntfQSEE* pNCSObject,
    const string &datatype)
{
    CAMX_UNREFERENCED_PARAM(pNCSObject);

    if (suids.size() > 0)
    {
        if (m_intfState == NCSIntfRunning)
        {
            // Register a callback function
            NCSSensorType   sensorType   = NCSMaxType;
            CamxResult      result       = CamxResultSuccess;
            if (0 == datatype.compare("gyro"))
            {
                sensorType = NCSGyroType;
                CAMX_LOG_INFO(CamxLogGroupNCS, "GYRO sensor is available: numSensors %d", suids.size());
            }
            else if (0 == datatype.compare("accel"))
            {
                sensorType = NCSAccelerometerType;
                CAMX_LOG_INFO(CamxLogGroupNCS, "ACCEL sensor is available: numSensors %d", suids.size());
            }
            else if (0 == datatype.compare("gravity"))
            {
                sensorType = NCSGravityType;
                CAMX_LOG_INFO(CamxLogGroupNCS, "Gravity sensor is available: numSensors %d", suids.size());
             }
            else if (0 == datatype.compare("gyro_cal"))
            {
                sensorType = NCSGyroCalType;
                CAMX_LOG_INFO(CamxLogGroupNCS, "GyroCal Cal sensor is available: numSensors %d", suids.size());
            }
            else if (0 == datatype.compare("mag_cal"))
            {
                sensorType = NCSMagCalType;
                CAMX_LOG_INFO(CamxLogGroupNCS, "MagCal sensor is available: numSensors %d", suids.size());
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "Currently unsupported sensor type %s", datatype.c_str());
                result = CamxResultEUnsupported;
            }

            if (CamxResultSuccess == result)
            {
                m_sensorList |= (1 << sensorType);
                m_numSensors[sensorType] = suids.size();
                for (UINT i = 0; i < suids.size(); i++)
                {
                    if (i >= NCSMaxSupportedConns)
                    {
                        break;
                    }
                    m_suids[sensorType][i] = suids.at(i);
                    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Probed sensor: %s via probelink %p suid (idx:%d, H 0x%llx, L 0x%llx)",
                                     datatype.c_str(),
                                     m_pProbeLink,
                                     i,
                                     m_suids[sensorType][i].m_high,
                                     m_suids[sensorType][i].m_low);
                }
            }
        }
        m_pQSEELinkUpdateCond->Signal();
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::SensorCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID NCSIntfQSEE::SensorCallback(
    const uint8_t*  pData,
    size_t          size)
{
    sns_client_event_msg pb_event_msg;
    uint32_t                    msg_id;
    NCSSensorType               sensorType = NCSMaxType;

    // NOWHINE GR017: Code is from protobuf, which is not QC code
    pb_event_msg.ParseFromArray(pData, static_cast<int>(size));

    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Enter before lock");
    m_pQSEEIntfMutex->Lock();
    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Enter after lock");
    if (m_intfState == NCSIntfRunning)
    {
        SensorUid suid(pb_event_msg.suid().suid_low(), pb_event_msg.suid().suid_high());
        sensorType = FindSensorType(suid);
        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "=>Processing %d events for sensor %d, (H: 0x%llx, L: 0x%llx)",
            pb_event_msg.events_size(), sensorType, suid.m_high, suid.m_low);

        for (INT i = 0; i < pb_event_msg.events_size(); i++)
        {
            const sns_client_event_msg_sns_client_event &rPb_event = pb_event_msg.events(i);
            msg_id = rPb_event.msg_id();

            CAMX_LOG_VERBOSE(CamxLogGroupNCS, "==>Processing event %d", msg_id);
            switch (msg_id)
            {
                case SNS_STD_MSGID_SNS_STD_ERROR_EVENT:
                {
                    sns_std_error_event error;
                    error.ParseFromString(rPb_event.payload());
                    CAMX_LOG_ERROR(CamxLogGroupNCS, "Received error event %i for sensor %d", error.error(), sensorType);
                    break;
                }
                case SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_PHYSICAL_CONFIG_EVENT:
                {
                    sns_std_sensor_physical_config_event config;
                    INT                                  connIndex = -1;

                    connIndex  = FindSensorIndex(suid);
                    if (connIndex < 0)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupNCS, "Invalid sensor index %d", connIndex);
                    }
                    else
                    {
                        config.ParseFromString(rPb_event.payload());
                        m_sensorConnList[connIndex].bufferHandles.tickPeriod =
                            static_cast<INT>((1 / config.sample_rate()) * static_cast<FLOAT>(NCSQtimerFrequency));
                        CAMX_LOG_INFO(CamxLogGroupNCS,
                            "Received config event for Conn Idx %d sensor type %d, configured sample rate %f, tickPeriod %u",
                            connIndex,
                            sensorType,
                            config.sample_rate(),
                            m_sensorConnList[connIndex].bufferHandles.tickPeriod);
                    }
                    break;
                }
                case SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_EVENT:
                {
                    FillSensorData(suid, rPb_event);
                    break;
                }
                case SNS_STD_MSGID_SNS_STD_ATTR_EVENT:
                {
                    sns_std_attr_event   attr_event;
                    attr_event.ParseFromArray(rPb_event.payload().c_str(), rPb_event.payload().size());
                    // For attribute events the sensor type needs to be fetched from the suids list
                    if (CamxResultSuccess != FillSensorAttributes(&attr_event, sensorType))
                    {
                        CAMX_LOG_ERROR(CamxLogGroupNCS, "Unable to fill attributes for sensor %d", sensorType);
                    }
                    break;
                }
                case SNS_CAL_MSGID_SNS_CAL_EVENT:
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Fill calibration data");
                    if (CamxResultSuccess != FillCalibData(suid, rPb_event))
                    {
                        CAMX_LOG_ERROR(CamxLogGroupNCS, "Unable to handle Calibration event for sensor %d", sensorType);
                    }
                    break;
                }
                case SNS_STD_MSGID_SNS_STD_FLUSH_EVENT:
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Receive unhandled flush event for sensor %d", sensorType);
                    break;
                }
                default:
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Received unknown message ID %i", rPb_event.msg_id())
                    break;
                }
            }
        }
    }
    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Exit");
    m_pQSEEIntfMutex->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::FillSensorAttributes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::FillSensorAttributes(
    sns_std_attr_event* pAttrEvent,
    NCSSensorType sensorType)
{
    INT attrId;

    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Filling attributes for sensor %d", sensorType);
    for (INT i = 0; i < pAttrEvent->attributes_size(); i++)
    {
        attrId = pAttrEvent->attributes(i).attr_id();
        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Sensor %d, attribute ID :%d", sensorType, attrId);

        switch (attrId)
        {
            case SNS_STD_SENSOR_ATTRID_RATES:
            {
                const sns_std_attr_value &rAttrValue = pAttrEvent->attributes(i).value();
                for (UINT j = 0; j < Utils::MinUINT32(rAttrValue.values_size(), NCSMaxRates); j++)
                {
                    if (NCSMaxRates == j)
                    {
                        break;
                    }
                    if (rAttrValue.values(j).has_flt())
                    {
                        m_sensorCaps[sensorType].rates[j] = rAttrValue.values(j).flt();
                        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Float values of rates is %f", rAttrValue.values(j).flt());
                        m_sensorCaps[sensorType].numRates++;
                    }
                }
                break;
            }
            case SNS_STD_SENSOR_ATTRID_RESOLUTIONS:
            {
                const sns_std_attr_value &rAttrValue = pAttrEvent->attributes(i).value();
                for (UINT j = 0; j < Utils::MinUINT32(rAttrValue.values_size(), NCSMaxRates); j++)
                {
                    if (rAttrValue.values(j).has_flt())
                    {
                        m_sensorCaps[sensorType].resolutions[j] = rAttrValue.values(j).flt();
                        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Float values of resolutions is %f", rAttrValue.values(j).flt());
                        m_sensorCaps[sensorType].numResolutions++;
                    }
                }
                break;
            }
            case SNS_STD_SENSOR_ATTRID_RANGES:
            {
                const sns_std_attr_value &rAttrValue = pAttrEvent->attributes(i).value();
                for (UINT j = 0; j < Utils::MinUINT32(rAttrValue.values_size(), NCSMaxRates); j++)
                {
                    if (rAttrValue.values(j).has_flt())
                    {
                        m_sensorCaps[sensorType].ranges[j].start =
                            rAttrValue.values(j).subtype().values(0).flt();
                        m_sensorCaps[sensorType].ranges[j].end   =
                            rAttrValue.values(j).subtype().values(1).flt();
                        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Float values of ranges is %f and %f",
                                         rAttrValue.values(j).subtype().values(0).flt(),
                                         rAttrValue.values(j).subtype().values(1).flt());
                        m_sensorCaps[sensorType].numRanges++;
                    }
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
    m_sensorCaps[sensorType].isValid = TRUE;
    m_sensorCaps[sensorType].type    = sensorType;

    m_pQSEELinkUpdateCond->Signal();
    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Fill attribute done, signal....");

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::FillSensorData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::FillSensorData(
    const SensorUid                             &rSUID,
    const sns_client_event_msg_sns_client_event &rPb_event)
{
    NCSDataGyro*         pGyroData      = NULL;
    NCSDataAccel*        pAccelData     = NULL;
    NCSDataGravity*      pGravityData   = NULL;
    INT                  currentPos     = -1;
    UINT                 bufferSize     = 0;
    UINT                 bufferStride   = 0;
    NCSSensorType        sensorType     = NCSMaxType;
    VOID*                phBufferHandle = NULL;
    UINT                 bufferSamples  = 0;
    CamxResult           result         = CamxResultSuccess;
    NCSCalib             calibData      = { 0 };
    sns_std_sensor_event event;
    INT                  connIndex      = -1;

    connIndex  = FindSensorIndex(rSUID);
    if (connIndex < 0)
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Invalid sensor index %d", connIndex);
        return CamxResultEInvalidArg;
    }

    event.ParseFromString(rPb_event.payload());
    if (event.data_size() <= 0)
    {
        CAMX_LOG_WARN(CamxLogGroupNCS, "Drop empty Sensor data!");
        result = CamxResultENoSuch;
        return result;
    }

    CheckAndSetLastSeenTime(static_cast<UINT>(connIndex));

    m_pServiceObject->EnqueueJob(NULL);

    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "ConnIndex %d: Event data size is %d",
        connIndex,
        event.data_size());

    phBufferHandle = m_sensorConnList[connIndex].bufferHandles.phBufferHandle;
    bufferSize     = static_cast<UINT>(m_sensorConnList[connIndex].bufferHandles.bufferSize);
    bufferStride   = m_sensorConnList[connIndex].bufferHandles.bufferStride;
    currentPos     = m_sensorConnList[connIndex].bufferHandles.currentPos;
    bufferSamples  = m_sensorConnList[connIndex].bufferHandles.totalSamples;

    if ((NULL == phBufferHandle) || (0 == bufferSize) || (0 == bufferStride))
    {
        CAMX_LOG_WARN(CamxLogGroupNCS, "Buffer queue for sensor %d has been freed: bufh=%p, bufsize=%d, stride=%d",
            m_sensorConnList[connIndex].sensorType, phBufferHandle, bufferSize, bufferStride);
        result = CamxResultENoMemory;
        return result;
    }

    if (m_sensorConnList[connIndex].bufferHandles.curPosLocked <= 0)
    {
        sensorType = m_sensorConnList[connIndex].sensorType;
        switch (sensorType)
        {
            case NCSGyroType:
            {
                pGyroData =
                    reinterpret_cast<NCSDataGyro*>(reinterpret_cast<CHAR *>(phBufferHandle) + bufferStride*currentPos);
                CAMX_LOG_VERBOSE(CamxLogGroupNCS,
                                 "GYRO: Current Pos %d base addr %p computed addr %p, stride %d",
                                 currentPos,
                                 phBufferHandle,
                                 pGyroData,
                                 bufferStride);
                if (TRUE == m_sensorConnList[connIndex].bufferHandles.calibData.isValid)
                {
                    calibData = m_sensorConnList[connIndex].bufferHandles.calibData;
                }
                pGyroData->x         = event.data(0) - calibData.bias[0];
                pGyroData->y         = event.data(1) - calibData.bias[1];
                pGyroData->z         = event.data(2) - calibData.bias[2];
                pGyroData->timestamp = rPb_event.timestamp();
                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "POS %d: Received Gyro sample <%f, %f, %f> time %llu bias < %f %f %f>",
                            currentPos,
                            pGyroData->x, pGyroData->y, pGyroData->z, pGyroData->timestamp,
                            calibData.bias[0],
                            calibData.bias[1],
                            calibData.bias[2]);

                if (NULL != m_sensorConnList[connIndex].bufferHandles.pFileBuffer)
                {
                    OsUtils::FPrintF(m_sensorConnList[connIndex].bufferHandles.pFileBuffer,
                                     "x %f y %f z %f ts %llu\n",
                                     pGyroData->x, pGyroData->y, pGyroData->z, pGyroData->timestamp);
                }
                break;
            }
            case NCSAccelerometerType:
            {
                pAccelData =
                    reinterpret_cast<NCSDataAccel *>(reinterpret_cast<CHAR *>(phBufferHandle) + bufferStride*currentPos);
                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "ACCEL: Current Pos %d base addr %p computed addr %p bufferStride %d",
                                 currentPos, phBufferHandle, pAccelData, bufferStride);
                pAccelData->x         = event.data(0);
                pAccelData->y         = event.data(1);
                pAccelData->z         = event.data(2);
                pAccelData->timestamp = rPb_event.timestamp();

                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Received Accel sample <%f, %f, %f> time %llu, bias <%f %f %f>",
                                 event.data(0), event.data(1), event.data(2), pAccelData->timestamp,
                                 calibData.bias[0],
                                 calibData.bias[1],
                                 calibData.bias[2]);
                break;
            }
            case NCSGravityType:
            {
                pGravityData =
                    reinterpret_cast<NCSDataGravity *>(reinterpret_cast<CHAR *>(phBufferHandle) + bufferStride*currentPos);
                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "GRAVITY: Current Pos %d base addr %p computed addr %p bufferStride %d",
                    currentPos, phBufferHandle, pGravityData, bufferStride);
                pGravityData->x = event.data(0);
                pGravityData->y = event.data(1);
                pGravityData->z = event.data(2);
                pGravityData->lx = event.data(3);
                pGravityData->ly = event.data(4);
                pGravityData->lz = event.data(5);
                pGravityData->timestamp = rPb_event.timestamp();

                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Received GRAVITY sample <%f, %f, %f> time %llu, bias <%f %f %f>",
                    pGravityData->x, pGravityData->y, pGravityData->z, pGravityData->timestamp,
                    calibData.bias[0],
                    calibData.bias[1],
                    calibData.bias[2]);
                break;
            }
            default:
            {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "Unsupported sensor event, tossing away the data");
                result = CamxResultEUnsupported;
                break;
            }
        }

        if (CamxResultSuccess == result)
        {
            currentPos++;
            if (static_cast<UINT>(currentPos) >= bufferSamples)
            {
                currentPos %= bufferSamples;
                m_sensorConnList[connIndex].bufferHandles.isBufferWrapped = TRUE;
            }
            m_sensorConnList[connIndex].bufferHandles.currentPos = currentPos;

            // trigger fence
            QSEEJob newQSEEJob;
            newQSEEJob.connIndex = connIndex;
            newQSEEJob.resultStatus = result;
            newQSEEJob.timestamp = rPb_event.timestamp();
            TriggerClientFence(&newQSEEJob);
      }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupNCS, "Current position is locked for access by clients, dropping this data");
        result = CamxResultEReadOnly;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::FillCalibData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::FillCalibData(
    const SensorUid                             &rSUID,
    const sns_client_event_msg_sns_client_event &rPb_event)
{
    static sns_cal_event pb_cal_event;
    NCSSensorType        sensorType    = NCSMaxType;
    CamxResult           result        = CamxResultSuccess;

    sensorType = FindSensorType(rSUID);
    if (TRUE == isCalibSource(sensorType))
    {
        pb_cal_event.ParseFromString(rPb_event.payload());
        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "sensor type %d with calibration bias (%f %f %f) and status %d",
                         sensorType,
                         pb_cal_event.bias(0),
                         pb_cal_event.bias(1),
                         pb_cal_event.bias(2),
                         pb_cal_event.status());
    }
    for (UINT connIndex = 0; connIndex < NCSMaxSupportedConns; connIndex++)
    {
        if (m_sensorConnList[connIndex].calibSUID == rSUID)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupNCS, "connIndex %d, calib source suid: (H 0x%llx, L 0x%llx)",
                             connIndex,
                             m_sensorConnList[connIndex].calibSUID.m_high,
                             m_sensorConnList[connIndex].calibSUID.m_low);
            if (SNS_STD_SENSOR_SAMPLE_STATUS_UNRELIABLE != pb_cal_event.status())
            {
                m_sensorConnList[connIndex].bufferHandles.calibData.bias[0] = pb_cal_event.bias(0);
                m_sensorConnList[connIndex].bufferHandles.calibData.bias[1] = pb_cal_event.bias(1);
                m_sensorConnList[connIndex].bufferHandles.calibData.bias[2] = pb_cal_event.bias(2);
                m_sensorConnList[connIndex].bufferHandles.calibData.isValid = TRUE;
            }
            else
            {
                m_sensorConnList[connIndex].bufferHandles.calibData.isValid = FALSE;
                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Unreliable calibration data");
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::RegisterService
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::RegisterService(
    NCSSensorHandle hSensorHandle)
{
    NCSSensor*        pNCSSensorObject     = NULL;
    CamxResult        result               = CamxResultEFailed;
    NCSSensorType     sensorType           = NCSMaxType;

    if (NULL != hSensorHandle)
    {
        pNCSSensorObject = static_cast<NCSSensor*>(hSensorHandle);
        sensorType       = pNCSSensorObject->GetSensorType();

        m_pQSEEIntfMutex->Lock();
        if (m_intfState == NCSIntfRunning)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupNCS, "RegisterService for a client");
            BOOL probed = (0 != (m_sensorList & (1 << sensorType)));
            if (TRUE == probed)
            {
                result = CreateClientSession(pNCSSensorObject);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupNCS,
                                   "Unable to register to the NCS service, Unable to create client session %s!",
                                   Utils::CamxResultToString(result));
                }
                else
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupNCS,
                                     "client registerd for sensor type %d pNCSSensorObject %p",
                                     pNCSSensorObject->GetSensorType(),
                                     pNCSSensorObject);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "Failed: Sensor %d probed %d, obj %p", sensorType, probed, pNCSSensorObject);
            }
        }
        m_pQSEEIntfMutex->Unlock();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::UnregisterService
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::UnregisterService(
    NCSSensorHandle handle)
{
    NCSSensor* pSensorObject    = NULL;
    CamxResult result           = CamxResultENoSuch;

    m_pQSEEIntfMutex->Lock();
    if (m_intfState == NCSIntfRunning)
    {

        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "UnregisterService for sensor handle %p", handle);

        if (NULL != handle)
        {
            pSensorObject = static_cast<NCSSensor*>(handle);
            result = Release(pSensorObject);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "Error releasing session associated with sensor conn idx %d type %d",
                                pSensorObject->GetConnIndex(),
                                pSensorObject->GetSensorType());
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "NULL sensor handle");
            result = CamxResultEInvalidPointer;
        }
    }
    m_pQSEEIntfMutex->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::AllocSensorStreamBuf
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* NCSIntfQSEE::AllocSensorStreamBuf(
    NCSSensorConfig*  pConfig,
    size_t*           pBufferSize,
    UINT*             pBufferStride)
{
    UINT  lBufferSize   = 0;
    UINT  lBUfferStride = 0;
    VOID* pBuffer       = NULL;

    switch(pConfig->sensorType)
    {
        case NCSAccelerometerType:
            lBufferSize   = sizeof(NCSDataAccel) * static_cast<INT>(pConfig->samplingRate) * NCSMaxBufferedTime;
            lBUfferStride = sizeof(NCSDataAccel);
            break;
        case NCSGyroType:
            lBufferSize   = sizeof(NCSDataGyro)  * static_cast<INT>(pConfig->samplingRate) * NCSMaxBufferedTime;
            lBUfferStride = sizeof(NCSDataGyro);
            break;
        case NCSGravityType:
            lBufferSize   = sizeof(NCSDataGravity)  * static_cast<INT>(pConfig->samplingRate) * NCSMaxBufferedTime;
            lBUfferStride = sizeof(NCSDataGravity);
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Unhandled sensor type !!");
            *pBufferSize = 0;
            break;
    }

    if (lBufferSize > 0)
    {
        *pBufferSize    = lBufferSize;
        *pBufferStride  = lBUfferStride;

        pBuffer = CAMX_CALLOC(lBufferSize);
        if (NULL == pBuffer)
        {
            *pBufferSize = 0;
            CAMX_LOG_ERROR(CamxLogGroupNCS, "Unable to allocate sensor buffer");
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupNCS, "Allocated buffer for sensor of type %d handle %p size %d stride %d",
               pConfig->sensorType, pBuffer, lBufferSize, lBUfferStride);
        }
    }

    return pBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::EnqueueAccessor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::EnqueueAccessor(
    NCSSensorHandle     hSensorObj,
    NCSSensorDataHandle hAccesorObj)
{
    CamxResult     result = CamxResultSuccess;
    NCSSensorData* pNCSDataObject;
    NCSSensor*     pNCSSensorObj;
    INT            connIndex;

    m_pQSEEIntfMutex->Lock();

    // Unlock the buffer current position if it has locked it.
    pNCSSensorObj = static_cast<NCSSensor*>(hSensorObj);
    connIndex = pNCSSensorObj->GetConnIndex();
    ClearBufferLock(connIndex);

    // Clear the flag that this acessor has lockd the buffer
    pNCSDataObject = static_cast<NCSSensorData*>(hAccesorObj);
    pNCSDataObject->ClearHaveBufLocked();

    result = PutAccessorObject(hAccesorObj);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "AccessorObject is NULL");
    }

    m_pQSEEIntfMutex->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::CheckAndSetLastSeenTime
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID NCSIntfQSEE::CheckAndSetLastSeenTime(
    UINT connIndex)
{
    QSEESensorConn* pConn       = NULL;
    NCSJob*         pJob        = NULL;
    QSEEJob*        pQSEEJob    = NULL;
    UINT            timeLag     = 0;
    BOOL            isTimedOut  = FALSE;

    for (UINT index = 0; index < NCSMaxSupportedConns; index++)
    {
        pConn = &m_sensorConnList[index];

        if ((NULL == pConn->phSensorConnHandle)         ||
            (QSEEConnRunning != pConn->connectionState) ||
            (FALSE == pConn->needMonitor))
        {
            continue;
        }

        if (index == connIndex)
        {
            OsUtils::GetTime(&pConn->lastSeenTime);
        }
        else
        {
            UINT32 lastSeenTimeInMillis = OsUtils::CamxTimeToMillis(&pConn->lastSeenTime);
            CamxTime currentTime        = { 0 };

            OsUtils::GetTime(&currentTime);
            timeLag    = OsUtils::CamxTimeToMillis(&currentTime) - lastSeenTimeInMillis;
            isTimedOut = (QSEECallbacksTimeout < timeLag);

            // If timeout has occured.
            if (TRUE == isTimedOut)
            {
                CAMX_LOG_WARN(CamxLogGroupNCS,
                              "Timeout on sensor type %d, last seenTime %d ms currentTime %d timeLag %d ms",
                              pConn->sensorType,
                              OsUtils::CamxTimeToMillis(&pConn->lastSeenTime),
                              OsUtils::CamxTimeToMillis(&currentTime),
                              timeLag);

                pJob = static_cast<NCSJob*>(CAMX_CALLOC(sizeof(NCSJob)));
                if (NULL != pJob)
                {
                    pQSEEJob = static_cast<QSEEJob*>(CAMX_CALLOC(sizeof(QSEEJob)));
                    if (NULL != pQSEEJob)
                    {
                        pJob->jobType = NCSJobTypeResetConnection;
                        pJob->pPayload = pQSEEJob;
                        pQSEEJob->connIndex = index;
                        pQSEEJob->timestamp = 0;
                        pQSEEJob->resultStatus = CamxResultETimeout;
                        m_pServiceObject->EnqueueJob(pJob);
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupNCS, "Can not create QSEE job object");
                        CAMX_FREE(pJob);
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupNCS, "Can not create NCS job object");
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NCSIntfQSEE::ResetConnection
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NCSIntfQSEE::ResetConnection(
    UINT connectionIndex)
{
    QSEESensorConn* pConn      = NULL;
    CamxResult      result     = CamxResultEFailed;

    m_pQSEEIntfMutex->Lock();
    if (connectionIndex > NCSMaxSupportedConns)
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Invalid connection index %d", connectionIndex);
    }
    else if (NCSMaxSupportedConns == connectionIndex)
    {
        // complete reset
        for (UINT i = 0; i < NCSMaxSupportedConns; i++)
        {
            pConn = &m_sensorConnList[i];

            if ((NULL != pConn->phSensorConnHandle)         &&
                (QSEEConnRunning == pConn->connectionState) &&
                TRUE == pConn->needMonitor)
            {
                NCSSensorConfig sensorConfig = { 0 };
                sensorConfig.samplingRate    = pConn->curSamplingRate;
                sensorConfig.reportRate      = pConn->curReportRate;
                sensorConfig.sensorType      = pConn->sensorType;
                CAMX_LOG_WARN(CamxLogGroupNCS, "Reset connection for sensor type %d", sensorConfig.sensorType);
                result = ReconfigSession(&sensorConfig, i, TRUE);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupNCS, "Unable to reconfig on error %s",
                        Utils::CamxResultToString(result));
                }
            }
        }
    }
    else
    {
        // single connection reset
        pConn = &m_sensorConnList[connectionIndex];
        if ((NULL != pConn->phSensorConnHandle) && (QSEEConnRunning == pConn->connectionState))
        {
            NCSSensorConfig sensorConfig = { 0 };
            sensorConfig.samplingRate    = pConn->curSamplingRate;
            sensorConfig.reportRate      = pConn->curReportRate;
            sensorConfig.sensorType      = pConn->sensorType;

            CAMX_LOG_WARN(CamxLogGroupNCS, "Reset connection for sensor type %d", sensorConfig.sensorType);
            result = ReconfigSession(&sensorConfig, connectionIndex, TRUE);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupNCS, "Unable to reconfig on error %s",
                    Utils::CamxResultToString(result));
            }
        }
    }
    m_pQSEEIntfMutex->Unlock();

    return result;
}

CAMX_NAMESPACE_END
