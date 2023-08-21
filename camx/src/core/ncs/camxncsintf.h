////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxncsintf.h
/// @brief CamX NCS interface class definition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXNCSINTF_H
#define CAMXNCSINTF_H

#include "camxdefs.h"
#include "camxmem.h"
#include "camxutils.h"
#include "camxtypes.h"
#include "chi.h"

CAMX_NAMESPACE_BEGIN

const UINT NCSMaxBufferedTime        = 3;    ///< 3 seconds of data would be buffered at most
const UINT NCSMaxLockedSegments      = 50;   ///< Maximum number of client locked seuqence of samples in the Ring buffer
const UINT NCSSessionLinkTimeout     = 100;  ///< Timeout for establishing the NCS link
const UINT NCSMaxRates               = 10;   ///< NCS max supported rates;
const UINT MaxSupportedSensorClients = 30;   ///< Max supported sensor clients
const UINT NCSMaxSupportedConns      = 10;   ///< NCS max supported sensor connection;


#if defined (CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // NOWHINE PR002 <- Win32 definition
const CHAR NCSLogPath[]              = "/data/vendor/camera/NCSSensorDump.log"; ///< NCS sensor file path
#else
const CHAR NCSLogPath[]              = "/data/misc/camera/NCSSensorDump.log"; ///< NCS sensor file path
#endif // Android-P or later

typedef VOID* NCSSensorDataHandle; ///< NCSSensor data accessor handle
typedef VOID* NCSSensorHandle;     ///< NCSSensor object handle
typedef VOID* NCSSessionHandle;    ///< NCS Session handle

/// @brief NCS interface states
enum NCSIntfState
{
    NCSIntfInvalid = 0, ///< Interface is invalid
    NCSIntfCreated,     ///< Interface is created
    NCSIntfRunning,     ///< Interface is currently running
    NCSIntfStopped,     ///< Intreface is stopped
    NCSIntfTimeout,     ///< Interface has timed out
    NCSIntfRelinking    ///< Interface has started relinking to sensors
};

/// @brief NCS Sensor types
enum NCSSensorType
{
    NCSInvalidType = 0,         ///< Invalid sensor type.
    NCSGyroType,                ///< Gyroscope sensor type.
    NCSAccelerometerType,       ///< Accelerometer sensor type.
    NCSMagnetometerType,        ///< Magnetometer sensor type.
    NCSGravityType,             ///< Gravity sensor type.
    NCSLinearAccelerationType,  ///< Linear acceleration sensor type.
    NCSTimeOfFlightType,        ///< Time of flight sensor type.
    NCSLightType,               ///< Light sensor type.
    NCSGyroCalType,             ///< Gyro calibration type
    NCSMagCalType,              ///< Magnetometer calibration type
    NCSMaxType,                 ///< Max sensor type.
};

/// @brief NCS Sensor interface type enum
enum NCSIntfType
{
    QSEE = 0,                   ///< QSEE interface.
    ASM,                        ///< Android native interface.
    MaxNCSIntfType,             ///< Max interface types
};

/// @brief NCS gyro data structure
struct NCSDataGyro
{
    FLOAT  x;          ///< Angular velocity about x axis.
    FLOAT  y;          ///< Angular velocity about y axis.
    FLOAT  z;          ///< Angular velocity about z axis.
    UINT64 timestamp;  ///< Timestamp of the sample.
};

/// @brief NCS accelerometer data structure
struct NCSDataAccel
{
    FLOAT  x;          ///< Acceleration about the x-axis.
    FLOAT  y;          ///< Acceleration about the y-axis.
    FLOAT  z;          ///< Acceleration about the z-axis.
    UINT64 timestamp;  ///< Timestamp when the sample was captured.
};

/// @brief NCS Ambient light data structure
struct NCSDataLight
{
    FLOAT lux;  ///< Lux value.
};

/// @brief NCS laser data structure
struct NCSDataLaser
{
    FLOAT  version;     ///< Sensor version.
    FLOAT  type;        ///< Sensor specific type.
    FLOAT  distance;    ///< Estimated distance.
    FLOAT  confidence;  ///< Data confidence.
    FLOAT  nearLimit;   ///< Near limit of the sensor.
    FLOAT  farLimit;    ///< Far limit of the sensor.
    UINT64 timestamp;   ///< Timestamp of the sample.
    INT32  maxRange;    ///< Max range of the sensor.
};

/// @brief NCS gravity data structure
struct NCSDataGravity
{
    FLOAT  x;          ///< Gravity value along x.
    FLOAT  y;          ///< Gravity value along y.
    FLOAT  z;          ///< Gravity value along z.
    FLOAT  lx;         ///< Linear acceleration along x.
    FLOAT  ly;         ///< Linear acceleration along y.
    FLOAT  lz;         ///< Linear acceleration along z.
    UINT64 timestamp;  ///< Timestamp of the sample.
};

/// @brief NCS Range structure
struct NCSRange
{
    FLOAT start;  ///< Start of the range.
    FLOAT end;    ///< End of the range.
};

/// @brief NCS Sensor capabilites structure
struct NCSSensorCaps
{
    NCSSensorType  type;                      ///< Sensor type.
    FLOAT          resolutions[NCSMaxRates];  ///< List of sensor resolutions.
    UINT           numResolutions;            ///< Number of resolutions.
    FLOAT          rates[NCSMaxRates];        ///< List of sensor rates.
    UINT           numRates;                  ///< Number of rates.
    NCSRange       ranges[NCSMaxRates];       ///< List of operational ranges.
    UINT           numRanges;                 ///< Number of ranges.
    BOOL           isValid;                   ///< Data valid flag.
};

/// @brief NCS Sensor config structure
struct NCSSensorConfig
{
    BOOL          operationMode;  ///< Batched mode(0) / streaming mode(1).
    FLOAT         samplingRate;   ///< Sampling rate in hertz.
    UINT          reportRate;     ///< Reporting rate in micorseconds.
    BOOL          needExact;      ///< Need the exact sample rate as configured
    NCSSensorType sensorType;     ///< Sensor type.
};


// @brief NCS chi fence callback functions
typedef CamxResult (*NCSAttachChiFence)(VOID* pContext, CHIFENCEHANDLE hChiFence);
typedef CamxResult (*NCSReleaseChiFence)(VOID* pContext, CHIFENCEHANDLE hChiFence);
typedef CamxResult (*NCSSignalChiFence)(VOID* pContext, CHIFENCEHANDLE hChiFence, CamxResult result);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that defined the interface of the NCS base class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class INCSIntfBase
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// INCSIntfBase
    ///
    /// @brief  the default constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INCSIntfBase() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~INCSIntfBase
    ///
    /// @brief  default virtual destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOWHINE NC004b The class methods have already been preprended with an I to indicate an interface
    virtual ~INCSIntfBase() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetListOfSensors
    ///
    /// @brief  Get the list of sensors
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult GetListOfSensors() = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryCapabilites
    ///
    /// @brief  Queries all the sensors' capabilities
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult QueryCapabilites() = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDataSync
    ///
    /// @brief  Get the data Synchronously
    ///
    /// @param  tStart     Start time stamp of data
    /// @param  tEnd       End timestamp of data
    /// @param  connIndex  Connection index
    ///
    /// @return Valid Accessor object handle if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual NCSSensorDataHandle GetDataSync(
        UINT64        tStart,
        UINT64        tEnd,
        INT           connIndex) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDataAsync
    ///
    /// @brief  Get the data Asynchronously
    ///
    /// @param  tStart      Start time stamp of data
    /// @param  tEnd        End timestamp of data
    /// @param  connIndex   Connection index
    /// @param  pFence      Pointer to a CSL fence
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult GetDataAsync(
        UINT64        tStart,
        UINT64        tEnd,
        INT           connIndex,
        VOID*         pFence) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLastNSamples
    ///
    /// @brief  Get the data Asynchronously
    ///
    /// @param  numOfSamples Number of samples
    /// @param  connIndex    Connection index
    ///
    /// @return Valid Accessor object handle if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual NCSSensorDataHandle GetLastNSamples(
        UINT          numOfSamples,
        INT           connIndex) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillCaps
    ///
    /// @brief  Fills the capabilities for a sensor
    ///
    /// @param  pCaps      Pointer to the capabilities, to be filled
    /// @param  sensorType Sensor type
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult FillCaps(
        NCSSensorCaps* pCaps,
        NCSSensorType  sensorType) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RegisterService
    ///
    /// @brief  Registers a sensor client to the ncs interface
    ///
    /// @param  hSensorHandle   Sensor handle
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult RegisterService(
        NCSSensorHandle hSensorHandle) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UnregisterService
    ///
    /// @brief  Unregister a client from the sensor
    ///
    /// @param  hSensorHandle Pointer to the sensor handle
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult UnregisterService(
        NCSSensorHandle hSensorHandle) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// EnqueueAccessor
    ///
    /// @brief  Enqueue accessor object from client
    ///
    /// @param  hSensorObject    Pointer related to the accessor object
    /// @param  hAccessorObject  Pointer to the accessor object
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult EnqueueAccessor(
        NCSSensorHandle      hSensorObject,
        NCSSensorDataHandle  hAccessorObject) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ResetConnection
    ///
    /// @brief  Interface function to check the NCS interface state.
    ///
    /// @param  connectionIndex    connection index.
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ResetConnection(
        UINT    connectionIndex) = 0;

private:
    INCSIntfBase(const INCSIntfBase&)                  = delete;     ///< Disallow the copy constructor.
    INCSIntfBase& operator= (const INCSIntfBase&)      = delete;     ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXNCSINTF_H
