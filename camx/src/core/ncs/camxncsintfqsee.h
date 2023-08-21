////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxncsintfqsee.h
/// @brief CamX Sensor QSEE interface class definition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXNCSINTFQSEE_H
#define CAMXNCSINTFQSEE_H

#include "camxncsintf.h"
#include "camxncssensor.h"
#include "camxlist.h"
#include "camxcsl.h"

#include "camxncsservice.h"
#include "camxncssscconnection.h"
#include "camxncssscutils.h"
#include "sns_std_sensor.pb.h"
#include "sns_cal.pb.h"

CAMX_NAMESPACE_BEGIN

static const INT  NCSQtimerFrequency    = 19200000; ///< Qtimer frequency
static const UINT QSEEConnectionTimeout = 3000;     ///< QSEE connection timeout period in ms
static const UINT QSEEAsyncQSize        = 30;       ///< Async request Q size
static const UINT QSEEAccessorPoolLen   = 10;       ///< Async request Q size
static const UINT QSEEBiasCompN         = 3;        ///< N - degrees of freedom for the bias comp vector.
static const UINT QSEECallbacksTimeout  = 300;      ///< QSEE connection timeout period in ms

class NCSService;

struct NCSCalib
{
    BOOL  isValid;                                   ///< Flag to indicate is the values are valid
    FLOAT bias[QSEEBiasCompN];                       ///< The zero bias correction subtracted to get calibrated sample.
};

struct RingBuffer
{
    VOID*     phBufferHandle;  ///< Ring buffer pointer
    SIZE_T    bufferSize;      ///< Size in bytes of the total ring buffer
    INT       currentPos;      ///< Current position where the new data would be copied
    UINT      bufferStride;    ///< Stride in bytes (size of each sample)
    UINT      totalSamples;    ///< total number of samples
    INT       curPosLocked;    ///< Counting lock to indicate this region is locked
    BOOL      isBufferWrapped; ///< Indicates if the buffer have been wrapped around
    INT       tickPeriod;      ///< QTimer tick period for 19.2MHz frequency and a given sensor
                               ///  sample rate.
    FILE*     pFileBuffer;     ///< File buffer for debugging;
    NCSCalib  calibData;       ///< Calibration data,less frequently updated/static
};

struct NCSAsyncRequest
{
    UINT64          tStart;     ///< Start timestamp
    UINT64          tEnd;       ///< End timestamp
    CHIFENCEHANDLE  hChiFence;  ///< Fence handle
    UINT64          enqTime;    ///< Time when it was enqueued, in Qtimer ticks
};

enum QSEEConnState
{
    QSEEConnStopped = 0,  ///< QSEE connection stopped state
    QSEEConnFlushing,     ///< QSEE connection flushing state
    QSEEConnRunning,      ///< QSEE connection Running state
    QSEEConnTimedout,     ///< QSEE connection timedout state
};

struct QSEEJob
{
    UINT64      timestamp;       ///< timestamp
    CamxResult  resultStatus;    ///< Result status
    INT         connIndex;       ///< Connection index
};

struct QSEESensorConn
{
    UINT                         curReportRate;                   ///< Current sampling rate
    FLOAT                        curSamplingRate;                 ///< Current report rate
    RingBuffer                   bufferHandles;                   ///< Ring buffer handle array
    SSCConnection*               phSensorConnHandle;              ///< SSC connection handle array
    SensorUid                    suid;                            ///< SUID of this sensor
    SensorUid                    calibSUID;                       ///< SUID of corresponding calib sensor
    NCSSensorType                sensorType;                      ///< Sensor type
    LightweightDoublyLinkedList  asyncRequestQ;                   ///< Async request queue
    QSEEConnState                connectionState;                 ///< Connection state
    INT                          mappedClients;                   ///< Clients mapped to this link
    CamxTime                     lastSeenTime;                    ///< Last seen timestamp (last reported callback)
    BOOL                         needMonitor;                     ///< whether requires monitoring on this connection
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the NCS QSEE interface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class NCSIntfQSEE : public INCSIntfBase
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetListOfSensors
    ///
    /// @brief  Get list of available sensors from QSEE
    ///         This funciton is only called by NCS service Initialize
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetListOfSensors();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInstance
    ///
    /// @brief  Get QSEE interface singleton object
    ///
    /// @param  pInitializeInfo   Initialization information
    /// @param  pNCSServiceObject pointer to the NCS Service object
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static NCSIntfQSEE* GetInstance(
        NCSInitializeInfo* pInitializeInfo,
        NCSService*        pNCSServiceObject);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~NCSIntfQSEE
    ///
    /// @brief  QSEE interface object destructor, using default destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~NCSIntfQSEE();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryCapabilites
    ///
    /// @brief  Get capabilities of available sensors from QSEE
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult QueryCapabilites();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDataSync
    ///
    /// @brief  Sync call to get data from the sensor service
    ///
    /// @param  tstart     start time of the data
    /// @param  tend       end time of the data
    /// @param  connIndex  Connextion index
    ///
    /// @return returns NCSSensorData type acessor object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    NCSSensorHandle GetDataSync(
        UINT64        tstart,
        UINT64        tend,
        INT           connIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLastNSamples
    ///
    /// @brief  sync call to get the last N samples from the sensor service
    ///
    /// @param  numOfSamples number of last samples needed
    /// @param  connIndex    Connection index
    ///
    /// @return returns NCSSensorData type acessor object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    NCSSensorHandle GetLastNSamples(
        UINT  numOfSamples,
        INT   connIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RegisterService
    ///
    /// @brief  Register to the sensor service
    ///
    /// @param  hSensorHandle    sensor handle
    ///
    /// @return returns CamxResultSuccess if succeeded
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RegisterService(
        NCSSensorHandle hSensorHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UnregisterService
    ///
    /// @brief  Unregister from the sensor service
    ///
    /// @param  hSensorHandle sensor handle provided during registration
    ///
    /// @return returns CamxResultSuccess if succeeded
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UnregisterService(
        NCSSensorHandle hSensorHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillCaps
    ///
    /// @brief  Fill the capabilities of the sensor
    ///
    /// @param  pCaps      Capabilities structure pointer to be filled
    /// @param  sensorType Sensor type
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillCaps(
        NCSSensorCaps* pCaps,
        NCSSensorType  sensorType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// EnqueueAccessor
    ///
    /// @brief  Enqueue back the accessor object given during get data calls
    ///
    /// @param  hSensorObj   pointer to the sensor object
    /// @param  hAccesorObj  pointer to the accessor object given during data request calls
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult EnqueueAccessor(
        NCSSensorHandle     hSensorObj,
        NCSSensorDataHandle hAccesorObj);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDataAsync
    ///
    /// @brief  Asynchrounous call to get the data between timestamps
    ///
    /// @param  tStart     start timestamp
    /// @param  tEnd       end timestamp
    /// @param  connIndex  Connection index
    /// @param  pFence     CSL fence pointer
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetDataAsync(
        UINT64        tStart,
        UINT64        tEnd,
        INT           connIndex,
        VOID*         pFence);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ResetConnection
    ///
    /// @brief  Interface function to reestablish the sensor connection
    ///
    /// @param  connectionIndex sensor connection index
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ResetConnection(
        UINT    connectionIndex);


private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize the NCS object
    ///
    /// @param  pInitializeInfo   Initialization information
    /// @param  pServiceObject    pointer to the NCS Service object
    ///
    /// @return CamxResultSuccess on successful execution
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize(
        NCSInitializeInfo* pInitializeInfo,
        NCSService*        pServiceObject);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSampleTimestamp
    ///
    /// @brief  Helper function to get the sample timestamp based on the position
    ///
    /// @param  connIndex         sensor connection index
    /// @param  pos               sample index
    /// @param  pTs               returned timestamp
    ///
    /// @return TRUE if found the timestamp
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetSampleTimestamp(
        INT      connIndex,
        INT      pos,
        UINT64*  pTs);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RateMatch
    ///
    /// @brief  Helper function to see if sensor config matches with any of the established connections
    ///
    /// @param  pSensorConfig Sensor config pointer
    /// @param  connIndex     Connection index to be verified against
    /// @param  pNeedReconfig Pointer to indicate if the request is for higher sample rate and reconfig needed
    ///
    /// @return TRUE if the rates match found
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL RateMatch(
        NCSSensorConfig* pSensorConfig,
        INT              connIndex,
        BOOL*            pNeedReconfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReconfigSession
    ///
    /// @brief  Reconfigure the sensor session.
    ///
    /// @param  pConfig         Pointer to the config
    /// @param  connIndex       Connection index
    /// @param  recreateSSCConn Recreate ssc connection
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReconfigSession(
        NCSSensorConfig* pConfig,
        INT              connIndex,
        BOOL             recreateSSCConn);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetBufferLock
    ///
    /// @brief  Lock the ring buffer and keep from writing into it
    ///
    /// @param  connIndex  Connection index
    /// @param  start      Start point of lock
    /// @param  end        End point of lock
    ///
    /// @return state of the lock set
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL SetBufferLock(
        INT connIndex,
        INT start,
        INT end);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ClearBufferLock
    ///
    /// @brief  Clear the buffer lock
    ///
    /// @param  connIndex connection index
    ///
    /// @return CamxResultSuccess on successful clearing of the lock
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ClearBufferLock(
        INT connIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Release
    ///
    /// @brief  Release the session and destroy all the links to qsee and internal buffers
    ///
    /// @param  hSensor Sensor object handle
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Release(
        NCSSensorHandle hSensor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateClientSession
    ///
    /// @brief  Establish the session with QSEE for the requested sensors by client
    ///
    /// @param  hSensorHandle Sensor handle
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateClientSession(
        NCSSensorHandle hSensorHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateQSEEConnHandle
    ///
    /// @brief  Create and initialize a QSEE connection handle at the input slot index
    ///
    /// @param  connIndex     Connection index
    /// @param  pSensorConfig Sensor config pointer
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateQSEEConnHandle(
        INT              connIndex,
        NCSSensorConfig* pSensorConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupSensorLinkOnly
    ///
    /// @brief  Callback fucntion to setup and ssc connection link with QSEE for the given sensor type
    ///
    /// @param  suids        Sensor SUID pointer
    /// @param  pNCSObject   NCS interface Object pointer
    /// @param  datatype     Data type of the sensor (a string viz: accel, gyro...etc)
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetupSensorLinkOnly(
        // NOWHINE CP006: This is platform sensors driver defined callback signature, cannot change
        const std::vector<SensorUid>&  suids,
        NCSIntfQSEE*                   pNCSObject,
        // NOWHINE CP006: This is platform sensors driver defined callback signature, cannot change
        const std::string&             datatype);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SensorCallback
    ///
    /// @brief  Callback function to process the sensor QSEE service callbacks
    ///
    /// @param  pData payload data
    /// @param  size  size of the payload data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SensorCallback(
        const uint8_t* pData,
        size_t         size);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SendConfigRequest
    ///
    /// @brief  Utility function to send the config request for a sensor uid
    ///
    /// @param  connIndex Connection index
    /// @param  pConfig   Pointer to the sensor config
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SendConfigRequest(
        INT              connIndex,
        NCSSensorConfig* pConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProbeSensor
    ///
    /// @brief  Function to probe a sensor and mark its presence
    ///
    /// @param  sensorType Sensor type
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProbeSensor(
        NCSSensorType sensorType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequestSensorCaps
    ///
    /// @brief  Function to request a sensor capability
    ///
    /// @param  sensorType Sensor type
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RequestSensorCaps(
        NCSSensorType sensorType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AllocSensorStreamBuf
    ///
    /// @brief  Allocate the buffer for the circular buffering of data from streaming sensor
    ///
    /// @param  pConfig        Sensor config
    /// @param  pBufferSize    Returned Buffer size
    /// @param  pBufferStride  Returned buffer stride
    ///
    /// @return Buffer pointer to the circular buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID* AllocSensorStreamBuf(
        NCSSensorConfig*  pConfig,
        size_t*           pBufferSize,
        UINT*             pBufferStride);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FindSensorIndex
    ///
    /// @brief  Utility function to get the sensor connection index based on SUID
    ///
    /// @param  rSUID SUID of the sensor
    ///
    /// @return Sensor conn index, -1 for error, unable to find
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE INT FindSensorIndex(
        const SensorUid &rSUID)
    {
        INT result = -1;

        for (UINT i = 0; i < NCSMaxSupportedConns; i++)
        {
            if (m_sensorConnList[i].suid == rSUID)
            {
                result = static_cast<INT>(i);
                break;
            }
        }
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FindSensorType
    ///
    /// @brief  Utility function to get the sensor type based on SUID
    ///
    /// @param  rSUID SUID of the sensor
    ///
    /// @return Sensor conn index, -1 for error, unable to find
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE NCSSensorType FindSensorType(
        const SensorUid &rSUID)
    {
        INT result = -1;

        for (INT i = 0; i < NCSMaxType; i++)
        {
            if (m_suids[i][0]== rSUID)
            {
                result = i;
                break;
            }
        }
        return static_cast<NCSSensorType>(result);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// isCalibSource
    ///
    /// @brief  Utility function to get if a sensor is calibration type sensor.
    ///
    /// @param  sensorType Sensor type
    ///
    /// @return TRUE if calibration source, else FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL isCalibSource(
        INT sensorType)
    {
        BOOL result = FALSE;

        switch (sensorType)
        {
            // Only gyro and magnetometer need bias compensation/calibration
            case NCSGyroCalType:
            case NCSMagCalType:
                result = TRUE;
                break;
            default:
                break;
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// isCalibNeeded
    ///
    /// @brief  Utility function to get if calibartion needed
    ///
    /// @param  sensorType Sensor type
    ///
    /// @return TRUE if needed, else FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL isCalibNeeded(
        INT sensorType)
    {
        BOOL result = FALSE;

        switch (sensorType)
        {
            // Only gyro and magnetometer need bias compensation/calibration
            case NCSGyroType:
            case NCSMagnetometerType:
                result = TRUE;
                break;
            default:
                break;
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// isValidSensorType
    ///
    /// @brief  Utility function to get if sensor type is valid
    ///
    /// @param  sensorType Sensor type
    ///
    /// @return TRUE if valid, else FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL isValidSensorType(
        NCSSensorType sensorType)
    {
        BOOL result = FALSE;

        if (sensorType < NCSMaxType)
        {
            result = TRUE;
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SendAttribRequest
    ///
    /// @brief  Send a request to QSEE service for the attributes of a sensor
    ///
    /// @param  sensorType Sensor type
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SendAttribRequest(
        INT sensorType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SendCalibRequest
    ///
    /// @brief  Send calibration request to QSEE service for calibration bias data
    ///
    /// @param  sensorType Sensor type
    /// @param  pSSCConn   Pointer to the SSC connection
    ///
    /// @return CamxResultSuccess if succeeded
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SendCalibRequest(
        INT sensorType, SSCConnection* pSSCConn);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillSensorAttributes
    ///
    /// @brief  Fill the sensor attributes into local structure
    ///
    /// @param  pAttrEvent sensor attributes payload
    /// @param  sensorType Sensor type
    ///
    /// @return CamxResultSuccess on successful execution
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillSensorAttributes(
        sns_std_attr_event* pAttrEvent,
        NCSSensorType sensorType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillSensorData
    ///
    /// @brief  Fill the sensor data into the ring buffer
    ///
    /// @param  rSUID    Sensor SUID
    /// @param  rPbEvent Sensor payload containing the sensor data
    ///
    /// @return CamxResultSuccess on successful execution
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillSensorData(
        const SensorUid                             &rSUID,
        const sns_client_event_msg_sns_client_event &rPbEvent);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillCalibData
    ///
    /// @brief  Fill the calibration data into the calibration structure
    ///
    /// @param  rSUID    Sensor SUID
    /// @param  rPbEvent Sensor payload containing the sensor calibration data
    ///
    /// @return CamxResultSuccess on successful execution
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillCalibData(
        const SensorUid                             &rSUID,
        const sns_client_event_msg_sns_client_event &rPbEvent);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckAndSetLastSeenTime
    ///
    /// @brief  This function set the last seen timestamp for the currect sensor connection
    ///         It also check if there is any timeout on the other sensor connection
    ///
    /// @param  connIndex  Current sensor connection index
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CheckAndSetLastSeenTime(
        UINT    connIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TriggerClientFence
    ///
    /// @brief  Trigger the fence provided by the client
    ///
    /// @param  pJob Current timestamp
    ///
    /// @return CamxResultSuccess on successful execution
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult TriggerClientFence(
        QSEEJob* pJob);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SendDisableRequest
    ///
    /// @brief  Function to disable the sensor streaming
    ///
    /// @param  connIndex Connection index
    ///
    /// @return CamxResultSucces if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SendDisableRequest(
        INT connIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAccessorObject
    ///
    /// @brief  Get a free accessor object
    ///
    /// @return NCSSensorDataHandle Sensor data accessor object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline NCSSensorDataHandle GetAccessorObject()
    {
        NCSSensorDataHandle hSensorData = NULL;

        LDLLNode* pNode = m_sensorDataObjectList.RemoveFromHead();
        if (NULL != pNode)
        {
            hSensorData = static_cast<NCSSensorData*>(pNode->pData);
            CAMX_FREE(pNode);
            pNode = NULL;
        }

        return hSensorData;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PutAccessorObject
    ///
    /// @brief  Put a accessor object back to queue
    ///
    /// @param  hSensorData Sensor data handle
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline CamxResult PutAccessorObject(
        NCSSensorDataHandle hSensorData)
    {
        CamxResult result = CamxResultSuccess;

        if (NULL != hSensorData)
        {
            LDLLNode* pNode = static_cast<LDLLNode*>(CAMX_CALLOC(sizeof(LDLLNode)));
            if (NULL != pNode)
            {
                pNode->pData = static_cast<VOID*>(hSensorData);
                m_sensorDataObjectList.InsertToHead(pNode);
            }
            else
            {
                result = CamxResultENoMemory;
            }
        }
        else
        {
            result = CamxResultEInvalidArg;
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NCSIntfQSEE
    ///
    /// @brief  QSEE interface object constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    NCSIntfQSEE();

    // Do not implement the copy constructor or assignment operator
    NCSIntfQSEE(const NCSIntfQSEE& rNCSIntfQSEE)             = delete;
    NCSIntfQSEE& operator= (const NCSIntfQSEE& rNCSIntfQSEE) = delete;

    UINT32                       m_sensorList;                                ///< BITMAP the list of queried sensors
    NCSSensorCaps                m_sensorCaps[NCSMaxType];                    ///< Sensor capabilities list
    SensorUid                    m_suids[NCSMaxType][NCSMaxSupportedConns];   ///< Sensor suid list
    UINT                         m_numSensors[NCSMaxType];                    ///< Number of active sensor

    QSEESensorConn               m_sensorConnList[NCSMaxSupportedConns];      ///< List of sensor connection handles

    LightweightDoublyLinkedList  m_sensorDataObjectList;                      ///< Pool of accessor objects

    NCSIntfState                 m_intfState;                                 ///< NCS Interface status
    Mutex*                       m_pQSEEIntfMutex;                            ///< Mutex for QSEE interface APIs
    Condition*                   m_pQSEELinkUpdateCond;                       ///< QSEE link updation cond variable

    VOID*                        m_pChiContext;                               ///< Pointer to the chi context
    NCSAttachChiFence            m_attachChiFence;                            ///< function pointer for attach fence
    NCSReleaseChiFence           m_releaseChiFence;                           ///< function pointer for release fence
    NCSSignalChiFence            m_signalChiFence;                            ///< function pointer for signal fence

    NCSService*                  m_pServiceObject;                            ///< Pointer to the service object

    SSCConnection*               m_pProbeLink;                                ///< Link to probe sensor and capbilites
};

CAMX_NAMESPACE_END

#endif // CAMXNCSINTFQSEE_H
