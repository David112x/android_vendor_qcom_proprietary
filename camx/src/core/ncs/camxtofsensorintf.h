////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxtofsensorintf.h
/// @brief CamX TOF Sensor interface class definition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXTOFSENSORINTF_H
#define CAMXTOFSENSORINTF_H

// NOWHINE FILE PR007b: Whiner incorrectly concludes as non-library files
#if (defined(LE_CAMERA))
#include <sensors.h>
#else // ANDROID
#include <hardware/sensors.h>
#endif // LE_CAMERA

#include "camxutils.h"
#include "camxthreadmanager.h"
#include "camxosutils.h"

CAMX_NAMESPACE_BEGIN

static const INT TOFMaxSamples   = 5;    ///< Maximum samples to be stored for TOF
static const INT TOFSampleThresh = 2;    ///< Threshold for past TOF samples

/// @brief Status of running thread
enum TOFThreadStatus
{
    TOFThreadStatusOff = 0,   ///< Thread has not been created
    TOFThreadStatusOn,        ///< Thread has been created and running
    TOFThreadStatusSuspend    ///< Thread is currently in suspended state
};

/// @brief TOF polling thread related data
struct TOFThreadContext
{
    JobHandle        hJobHandle;       ///< Job family handle
    ThreadManager*   pThreadManager;   ///< Thread manager pointer
    TOFThreadStatus  threadStatus;     ///< Current status of the thread
    Mutex*           pTOFStateMutex;   ///< TOF thread state mutex variable
    Condition*       pTOFStateCondVar; ///< TOF state condition variable
    VOID*            pSensorIntf;      ///< TOF interface object
};

/// @brief TOF Thread data container
struct TOFThreadData
{
    TOFThreadContext* pThreadContext; ///< Pointer to the thread context.
};

/// @brief Time of flight sensor internal data
struct TOFSensorObj
{
    VOID*                         phLibHandle;    ///< TOF sensor handle to access the sensor APIs
    struct sensors_module_t*      pModule;        ///< TOF sensor library hardware module structure
    struct sensors_poll_device_t* pDevice;        ///< TOF sensor library APIs to control the sensor
    INT                           hSensorHandle;  ///< Handle for specific TOF sensor
    INT                           maxRange;       ///< Maximum range that can be measured by the TOF sensor
    UINT                          reportRate;     ///< Reporting rate in micorseconds.
};

/// @brief TOF data structure
struct DataTOF
{
    FLOAT  version;     ///< Sensor version.
    FLOAT  type;        ///< Sensor specific type.
    FLOAT  distance;    ///< Estimated distance.
    FLOAT  confidence;  ///< Data confidence.
    FLOAT  nearLimit;   ///< Near limit of the sensor.
    FLOAT  farLimit;    ///< Far limit of the sensor.
    UINT64 timestamp;   ///< Timestamp of the sample.
    INT32  maxRange;    ///< Max range of the sensor.
    FLOAT  ambientRate; ///< ambient rate from sensor.
};

/// @brief TOF Sensor config structure
struct TOFSensorConfig
{
    FLOAT  samplingRate;   ///< Sampling rate in hertz.
    UINT   reportRate;     ///< Reporting rate in micorseconds.
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the TOF Sensor interface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TOFSensorIntf
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInstance
    ///
    /// @brief  Creates a laser sensor instance for the first time or returns the already created instance
    ///
    /// @param  pThreadManager  ThreadManger
    /// @param  pSensorLibrary  Sensor library Name
    ///
    /// @return laser Sensor instance
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_VISIBILITY_PUBLIC static TOFSensorIntf* GetInstance(
        ThreadManager*   pThreadManager,
        const CHAR*      pSensorLibrary);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destroy the TOF interface object
    ///
    /// @return CamxResultSuccess on successful execution
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_VISIBILITY_PUBLIC CamxResult Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureTOFSensor
    ///
    /// @brief  Function to configure TOF sensor and activate it
    ///
    /// @param  pConfig   Sensor configuration
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_VISIBILITY_PUBLIC CamxResult ConfigureTOFSensor(
        TOFSensorConfig* pConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLastNSamples
    ///
    /// @brief  sync call to get the last N samples from TOF sensor
    ///
    /// @param  pData          pointer to get data samples
    /// @param  numOfSamples   number of samples needed
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_VISIBILITY_PUBLIC CamxResult GetLastNSamples(
        DataTOF* pData,
        INT      numOfSamples);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TearDownTOFIntfLink
    ///
    /// @brief  Tear down connection with TOF sensor
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_VISIBILITY_PUBLIC CamxResult TearDownTOFIntfLink();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~TOFSensorIntf
    ///
    /// @brief  TOF interface object destructor, using default destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_VISIBILITY_PUBLIC virtual ~TOFSensorIntf();

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TOFSensorIntf
    ///
    /// @brief  TOF interface object constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    TOFSensorIntf();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize the TOF interface object
    ///
    /// @param  pThreadManager  Thread Manager Object pointer
    /// @param  pSensorLibrary  Sensor Library Name
    ///
    /// @return CamxResultSuccess on successful execution
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize(
        ThreadManager*   pThreadManager,
        const CHAR*      pSensorLibrary);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupTOFLink
    ///
    /// @brief  Setup connection with TOF sensor library
    ///
    /// @param  pSensorLibrary  Sensor Library Name
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupTOFLink(
        const CHAR*      pSensorLibrary);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TearDownTOFLink
    ///
    /// @brief  Tear down connection with TOF sensor library
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult TearDownTOFLink();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ActivateTOFSensor
    ///
    /// @brief  Enable/disable TOF sensor
    ///
    /// @param  onOffFlag   Flag to indicate On/Off switch
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ActivateTOFSensor(
        BOOL  onOffFlag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StartPollThread
    ///
    /// @brief  Start polling thread.
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult StartPollThread();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StopPollThread
    ///
    /// @brief  Stop polling thread.
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult StopPollThread();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TOFPollThread
    ///
    /// @brief  Thread handler for TOF sensor to poll for data.
    ///
    /// @param  pArg   argument passed to thread handler
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* TOFPollThread(
        VOID* pArg);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSensorCapability
    ///
    /// @brief  Get TOF sensor capability
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetSensorCapability();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillTOFSensorData
    ///
    /// @brief  Fill the sensor data into the ring buffer
    ///
    /// @param  pSensorData  Sensor data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FillTOFSensorData(
        sensors_event_t* pSensorData);


    // Do not implement the copy constructor or assignment operator
    TOFSensorIntf(const TOFSensorIntf& rTOFSensorIntf)             = delete;
    TOFSensorIntf& operator= (const TOFSensorIntf& rTOFSensorIntf) = delete;

    static TOFSensorIntf    s_laserSensor;             ///< Singleton Laser Sensor Object
    DataTOF                 m_TOFData[TOFMaxSamples];  ///< TOF data container
    INT32                   m_latestSampleIndex;       ///< TOF data queue index for latest sample
    Mutex*                  m_pTOFDataMutex;           ///< Mutex for TOF data access
    TOFSensorObj            m_TOFSensorObj;            ///< TOF sensor object
    TOFThreadContext        m_hTOFPollThHandle;        ///< TOF polling thread related data
    UINT32                  m_refCount;                ///< Number of active laser threads depending on num of cameras
    BOOL                    m_singletonObjectCreated;  ///< Flag to indicate if object is already created or not
};

CAMX_NAMESPACE_END

#endif // CAMXTOFSENSORINTF_H
