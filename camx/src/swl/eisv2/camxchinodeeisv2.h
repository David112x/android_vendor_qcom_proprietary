////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodeeisv2.h
/// @brief Chi node for Image stabilization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCHINODEEISV2_H
#define CAMXCHINODEEISV2_H

#include "camxchinodeutil.h"
#include "chieisdefs.h"
#include "chiipedefs.h"
#include "chiifedefs.h"
#include "chinode.h"
#include "eis2_interface.h"
#include "is_interface_utils.h"
#include "tuningsetmanager.h"
#if !_WINDOWS
#include <cutils/properties.h>
#endif

/// @brief structure to contain all the vendor tag Ids that EISv2 uses
struct EISV2VendorTags
{
    UINT32 ICAInPerspectiveTransformTagId;  ///< ICA In Perspective Transformation Vendor Tag Id
    UINT32 residualCropTagId;               ///< IFE Residual Crop Info Vendor Tag Id
    UINT32 appliedCropTagId;                ///< IFE Applied Crop Info Vendor Tag Id
    UINT32 mountAngleTagId;                 ///< Camera Sensor Mount Angle Vendor Tag Id
    UINT32 cameraPositionTagId;             ///< Camera Sensor Position Vendor Tag Id
    UINT32 QTimerTimestampTagId;            ///< QTimer SOF Timestamp Vendor Tag Id
    UINT32 sensorInfoTagId;                 ///< Sensor Info Vendor Tag Id
    UINT32 previewStreamDimensionsTagId;    ///< Output preview stream dimensions Vendor Tag Id
    UINT32 videoStreamDimensionsTagId;      ///< Output video stream dimensions Vendor Tag Id
    UINT32 EISV2EnabledTagId;               ///< EISv2 enabled flag Vendor Tag Id
    UINT32 EISV2FrameDelayTagId;            ///< EISv2 frame delay Vendor Tag Id
    UINT32 EISV2RequestedMarginTagId;       ///< EISv2 Requested Margin Vendor Tag Id
    UINT32 EISV2StabilizationMarginsTagId;  ///< EISv2 actual stabilization margins Vendor Tag Id
    UINT32 EISV2AdditionalCropOffsetTagId;  ///< EISv2 additional crop offset Vendor Tag Id
    UINT32 currentSensorModeTagId;          ///< Sensor's current mode index Vendor Tag Id
    UINT32 EISV2StabilizedOutputDimsTagId;  ///< EISv2 Stabilized Output Dimensions Vendor Tag Id
    UINT32 EISV2OutputDimsLookAheadTagId;   ///< EISv2 Stabilized Output Dimensions of Lookahead peer Vendor Tag Id
    UINT32 ICAInGridOut2InTransformTagId;   ///< ICA In Grid out2in Transformation Vendor Tag Id
    UINT32 ICAInGridIn2OutTransformTagId;   ///< ICA In Grid in2out Transformation Vendor Tag Id
    UINT32 physicalCameraConfigsTagId;      ///< Physical camera configs Tag id
    UINT32 multiCameraIdTagId;              ///< Multi camera id tag
    UINT32 chiNodeResidualCrop;             ///< Chi node residual crop Tag id
    UINT32 targetFPSTagId;                  ///< Target FPS for the usecase Vendor Tag Id
    UINT32 ICAReferenceParamsTagId;         ///< ICA reference params Tag Id
    UINT32 FOVCFactorTagId;                 ///< FOVC factor Tag Id
    UINT32 EISV2MinimalTotalMarginTagId;    ///< EISv2 Minimal Total Margin Vendor Tag Id
};

/// @brief structure to contain all the per sensor data required for EISv2 algorithm
struct EISV2PerSensorData
{
    CameraConfiguration             cameraConfig;            ///< physical camera configuration
    CHINODEIMAGEFORMAT              inputSize;               ///< Image size
    CHINODEIMAGEFORMAT              outputSize;              ///< Output Image size
    CHINODEIMAGEFORMAT              sensorDimension;         ///< Output Sensor Dimension
    TuningSetManager*               pTuningManager;          ///< Pointer to tuning manager
    eis_1_2_0::chromatix_eis12Type* pEISChromatix;           ///< pointer to EIS chromatix
    VOID*                           pICAChromatix;           ///< pointer to ICA chromatix
    UINT32                          mountAngle;              ///< Sensor mount angle
    INT32                           cameraPosition;          ///< Camera sensor position
    UINT32                          targetFPS;               ///< Target Frame Rate
    CHINODEIMAGEFORMAT              activeArraySize;         ///< Active arrya size
};

/// @brief structure to contain eisv3 override settings
struct EISV2OverrideSettings
{
    MarginRequest           margins;                              ///< Margins
    cam_is_operation_mode_t algoOperationMode;                    ///< Calibration mode
    BOOL                    isLDCGridEnabled;                     ///< Flag for LDC grid enable
    BOOL                    isDefaultGridTransformEnabled;        ///< Flag for default grid/transform
    BOOL                    isEnabledDumpInputFile;               ///< Flag to indicate if Gyro Dump enabled
    BOOL                    isEnabledDumpOutputFile;              ///< Flag to indicate if algorithm output to file is enabled
    BOOL                    isEnabledDumpInputLogcat;             ///< Flag to indicate if algorithm output to file is enabled
    BOOL                    isEnabledDumpOutputLogcat;            ///< Flag to indicate if algorithm output to file is enabled
    BOOL                    isEnabledDumpForceFlush;              ///< Flag to indicate if node will be forced to call
                                                                  ///< algorithm log flush API each iteration
    cam_is_ois_mode         overrideOisMode;                      ///< Override algorithm's ois_mode output.
                                                                  ///< If 0 then doesn't override (uses algorithm's ois_mode).
};

/// @brief EIS2 path type
typedef enum EISV2PathType
{
    FullPath,           ///< Input path full port
    DS4Path,            ///< Input path DS4 port
    DS16Path,           ///< Input path DS16 port
    FDPath,             ///< Input path FD port
    DisplayFullPath,    ///< Input path Display Full port
    DisplayDS4Path,     ///< Input path Display DS4 port
    DisplayDS16Path     ///< Input path Display DS16 port
} EISV2PATHTYPE;

static const UINT32 MaxMulticamSensors = MaxLinkedCameras;
static const UINT64 QtimerFrequency    = 19200000;  ///< QTimer Freq = 19.2 MHz

// NOWHINE FILE NC004c: Things outside the Camx namespace should be prefixed with Camx/CSL
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Chi node Class for Chi interface.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiEISV2Node
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialization required to create a node
    ///
    /// @param  pCreateInfo Pointer to a structure that defines create session information for the node.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Initialize(
        CHINODECREATEINFO* pCreateInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessRequest
    ///
    /// @brief  Implementation of PFNNODEPROCREQUEST defined in chinode.h
    ///
    /// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessRequest(
        CHINODEPROCESSREQUESTINFO* pProcessRequestInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetBufferInfo
    ///
    /// @brief  Implementation of PFNNODESETBUFFERINFO defined in chinode.h
    ///
    /// @param  pSetBufferInfo  Pointer to a structure with information to set the output buffer resolution and type.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetBufferInfo(
        CHINODESETBUFFERPROPERTIESINFO* pSetBufferInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryBufferInfo
    ///
    /// @brief  Implementation of PFNNODEQUERYBUFFERINFO defined in chinode.h
    ///
    /// @param  pQueryBufferInfo  Pointer to a structure with information to query buffer resolution and type.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult QueryBufferInfo(
        CHINODEQUERYBUFFERINFO* pQueryBufferInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryMetadataPublishList
    ///
    /// @brief  Implementation of PFNNODEQUERYBUFFERINFO defined in chinode.h
    ///
    /// @param  pMetadataPublishlist    Pointer to a structure to query the publish list
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult QueryMetadataPublishList(
              CHINODEMETADATALIST* pMetadataPublishlist);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostPipelineCreate
    ///
    /// @brief  Implementation of PFNPOSTPIPELINECREATE defined in chinode.h
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult PostPipelineCreate();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDataSource
    ///
    /// @brief  Inline function to Get data source
    ///
    /// @return Pointer to data source
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline CHIDATASOURCE* GetDataSource()
    {
        return &m_hChiDataSource;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiEISV2Node
    ///
    /// @brief  Constructor
    ///
    /// @param  overrideSettings  EISV2 override settings
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiEISV2Node(
        EISV2OverrideSettings overrideSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiEISV2Node
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiEISV2Node();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMinTotalMargins
    ///
    /// @brief  Get the minimal total margins that are defined in the tuning manager
    ///
    /// @param  minTotalMarginX   The minimal total margin for width from Tuning manager
    /// @param  minTotalMarginY   The minimal total margin for hight from Tuning manager
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetMinTotalMargins(
        FLOAT* minTotalMarginX,
        FLOAT* minTotalMarginY);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTotalMargins
    ///
    /// @brief  Get total margins from chromatix
    ///
    /// @param  overrideSettings    override settings configuration struct
    /// @param  pTotalMarginX       The total margin for width from Tuning manager
    /// @param  pTotalMarginY       The total margin for hight from Tuning manager
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetTotalMargins(
        const EISV2OverrideSettings* overrideSettings,
        FLOAT*  pTotalMarginX,
        FLOAT*  pTotalMarginY);
private:
    ChiEISV2Node(const ChiEISV2Node&) = delete;               ///< Disallow the copy constructor
    ChiEISV2Node& operator=(const ChiEISV2Node&) = delete;    ///< Disallow assignment operator

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteAlgo
    ///
    /// @brief  Get input params from metadata in the pipeline
    ///
    /// @param  pProcessRequestInfo  Process request Info
    /// @param  pEIS2Output          EIS2 algorithm output
    /// @param  sensorIndex          Sensor index
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ExecuteAlgo(
        CHINODEPROCESSREQUESTINFO* pProcessRequestInfo,
        is_output_type*            pEIS2Output,
        UINT32                     sensorIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGyroInterval
    ///
    /// @brief  Request Gyro data
    ///
    /// @param  frameNum       Process request frame number
    /// @param  sensorIndex    sensor index
    /// @param  pGyroInterval  Pointer to gyro interval structure to fill
    /// @param  pFrameTimesIn  Pointer to frame time structure to fill
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetGyroInterval(
        UINT64          frameNum,
        UINT32          sensorIndex,
        gyro_times_t*   pGyroInterval,
        frame_times_t*  pFrameTimesIn);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetGyroDependency
    ///
    /// @brief  Set dependency to Request Gyro data from NCS
    ///
    /// @param  pProcessRequestInfo  Process request Info
    /// @param  sensorIndex          Sensor index
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetGyroDependency(
        CHINODEPROCESSREQUESTINFO* pProcessRequestInfo,
        UINT32                     sensorIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDependencies
    ///
    /// @brief  Set Dependencies
    ///
    /// @param  pProcessRequestInfo  Process request Info
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetDependencies(
        CHINODEPROCESSREQUESTINFO* pProcessRequestInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateMetaData
    ///
    /// @brief  Update the metadata in the pipeline
    ///
    /// @param  requestId   The request id for current request
    /// @param  sensorIndex sensor index
    /// @param  pAlgoResult Result from the algo process
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateMetaData(
        UINT64          requestId,
        UINT32          sensorIndex,
        is_output_type* pAlgoResult);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAdditionalCropOffset
    ///
    /// @brief  Get the additional crop offset from the total margin
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetAdditionalCropOffset();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LoadLib
    ///
    /// @brief  Load the algo lib and map function pointers
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult LoadLib();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UnLoadLib
    ///
    /// @brief  UnLoad the algo lib
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult UnLoadLib();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeLib
    ///
    /// @brief  Initialize the algo lib and get handle
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InitializeLib();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillVendorTagIds
    ///
    /// @brief  Query all the vendor tag locations that EISv2 uses and save them
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult FillVendorTagIds();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetChromatixTuningHandle
    ///
    /// @brief  Get Chromatix tuning handle
    ///
    /// @param  sensorIndex Sensor index
    /// @param  sensorMode  Sensor mode
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetChromatixTuningHandle(
        UINT32 sensorIndex,
        UINT32 sensorMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetChromatixData
    ///
    /// @brief  Get Chromatix data required for EIS Algo Input
    ///
    /// @param  pEISInitData EIS algo Init data
    /// @param  sensorIndex  sensor index
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetChromatixData(
        is_init_data_sensor* pEISInitData,
        UINT32               sensorIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLDCGridFromICA20Chromatix
    ///
    /// @brief Get LDC grid from ICA20 Chromatix
    ///
    /// @param pEIS2Input  EIS algo init sensor data
    /// @param sensorIndex Sensor index
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetLDCGridFromICA20Chromatix(
        is_init_data_sensor* pEISInitData,
        UINT32               sensorIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLDCGridFromICA30Chromatix
    ///
    /// @brief Get LDC grid from ICA30 Chromatix
    ///
    /// @param pEIS2Input  EIS algo init sensor data
    /// @param sensorIndex Sensor index
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetLDCGridFromICA30Chromatix(
        is_init_data_sensor* pEISInitData,
        UINT32               sensorIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLDCGridFromEISChromatix
    ///
    /// @brief Get LDC grid from ICA30 Chromatix
    ///
    /// @param pEISLDCInitData  EIS LDC init data to fill
    /// @param pEISLDCChromatix EIS LDC chromatix struct
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetLDCGridFromEISChromatix(
        eis_lens_distortion_correction*                                             pEISLDCInitData,
        eis_1_2_0::chromatix_eis12_reserveType::lens_distortion_correctionStruct*   pEISLDCChromatix);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGyroFrequency
    ///
    /// @brief  Get gyro frequency from chromatix for the sensor index
    ///
    /// @param  sensorIndex Sensor index
    ///
    /// @return Gyro frequency in FLOAT
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    FLOAT GetGyroFrequency(
        UINT32 sensorIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillGyroData
    ///
    /// @brief  Get Gyro data required for EIS Algo Input
    ///
    /// @param  pProcessRequestInfo Process request Info
    /// @param  pGyroData           Pointer to gyro data structure to fill
    /// @param  pFrameTimes         Pointer to frame time structure to fill
    /// @param  sensorIndex         sensor index
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult FillGyroData(
        CHINODEPROCESSREQUESTINFO* pProcessRequestInfo,
        gyro_data_t*               pGyroData,
        frame_times_t*             pFrameTimes,
        UINT32                     sensorIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QtimerTicksToQtimerNano
    ///
    /// @brief Convert timestamp from QTimer Ticks to QTimer Nano secs
    ///
    /// @param Timestamp in Qtimer Ticks
    ///
    /// @return Timestamp in QTimer Nano secs
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT64 QtimerTicksToQtimerNano(UINT64 ticks)
    {
        return (UINT64(DOUBLE(ticks) * DOUBLE(NSEC_PER_SEC) / DOUBLE(QtimerFrequency)));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QtimerNanoToQtimerTicks
    ///
    /// @brief Convert timestamp from Qtimer Nano to QTimer Ticks
    ///
    /// @param Timestamp in QTimer Nano secs
    ///
    /// @return Timestamp in Qtimer Ticks
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline UINT64 QtimerNanoToQtimerTicks(UINT64 qtimerNano)
    {
        return static_cast<UINT64>(qtimerNano * DOUBLE(QtimerFrequency / DOUBLE(NSEC_PER_SEC)));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsEISv2Disabled
    ///
    /// @brief Check if EIS is disabled
    ///
    /// @param requestId Request ID
    ///
    /// @return TRUE if Disabled. False Otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsEISv2Disabled(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateEISGyroInterval
    ///
    /// @brief Check if EIS gyro interval is right and fix it
    ///
    /// @param pGyroInterval: the gyro interval need to validate
    ///
    /// @return Success if the input is validate already, Failed Otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline CDKResult ValidateEISGyroInterval(
        gyro_times_t *pGyroInterval)
    {
        if (pGyroInterval->first_ts >= pGyroInterval->last_ts)
        {
            LOG_ERROR(CamxLogGroupChi, "Invalid gyro time window:%" PRIu64 " %" PRIu64,
                pGyroInterval->first_ts, pGyroInterval->last_ts);
            return CDKResultEFailed;
        }

        return CDKResultSuccess;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateZoomWindow
    ///
    /// @brief  Update Crop window to handle stabilize margins
    ///
    /// @param  pCropRect   Residual crop window
    /// @param  sensorIndex Sensor index
    /// @param  requestId Request id
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateZoomWindow(
        CHIRectangle* pCropRect,
        UINT32        sensorIndex,
        UINT64        requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConvertICA20GridToICA10Grid
    ///
    /// @brief  Convert ICA20 (size 35 x 27) Grid to ICA10 Grid (size 33 x 25) and also provide extarpolate corners for ICA10
    ///         todo: Move this function to a common utils for EIS nodes
    ///
    /// @param  pInICA20Grid Pointer to ICA20 grid
    /// @param  pInICA10Grid Pointer to ICA10 grid
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ConvertICA20GridToICA10Grid(
        NcLibWarpGrid *pInICA20Grid,
        NcLibWarpGrid *pIutICA10Grid);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCropRectfromCropInfo
    ///
    /// @brief  Get crop rectangle from crop info based on m_inputPortPathType
    ///
    /// @param  cropInfo Pointer to crop Info
    /// @param  cropRect Pointer to crop rectangle
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetCropRectfromCropInfo(
        IFECropInfo* pCropInfo,
        CHIRectangle* pCropRect);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsBypassableNode
    ///
    /// @brief  Get whether a node is bypassable or not
    ///
    /// @return True if the node is bypassable otherwise false
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsBypassableNode() const
    {
        return m_nodeFlags.isBypassable;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsRealTimeNode
    ///
    /// @brief  Get whether a node part of realtime pipeline or not
    ///
    /// @return True if the node is bypassable otherwise false
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline BOOL IsRealTimeNode() const
    {
        return m_nodeFlags.isRealTime;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// getICAGridGeometryVersion
    ///
    /// @brief  Set ICA grid geometry based on num row and columns
    ///
    /// @param  gridNumRow          Num grid rows
    /// @param  gridNumColumn       Num grid columns
    /// @param  pICAGridGeometry    Pointer to ICA grid geometry
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID getICAGridGeometryVersion(
        uint32_t         gridNumRow,
        uint32_t         gridNumColumn,
        ICAGridGeometry* pICAGridGeometry);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCameraIndexFromID
    ///
    /// @brief  Get Index from camera_info structure based on camera ID
    ///
    /// @param  camRole   Camera Id
    ///
    /// @return camera index  if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetCameraIndexFromID(
        UINT32 cameraId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPerCameraConfig
    ///
    /// @brief  Get per camera config
    ///
    /// @return camera index  if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetPerCameraConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDeploymentType
    ///
    /// @brief  Get EIS deployment type
    ///
    /// @return EIS deployment type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    cam_is_deployment_type_t GetDeploymentType();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSensorAppliedCrop
    ///
    /// @brief  Get sensor applied crop
    ///
    /// @param  sensorIdx sensor index
    ///
    /// @return crop window for sensor applied crop
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    WindowRegionF GetSensorAppliedCrop(UINT32 sensorIdx);

    ///< Typedefs for EIS2 interface functions
    typedef CDKResult (*EIS2_INITIALIZE)                      (VOID**                             eis2_handle,
                                                               const  is_init_data_common*        init_common,
                                                               const  is_init_data_sensor*        init_sensors,
                                                               uint32_t                           num_sensors);

    typedef CDKResult (*EIS2_PROCESS)                         (VOID*                              eis2_handle,
                                                               const is_input_t*                  is_input,
                                                               is_output_type*                    is_output);

    typedef CDKResult (*EIS2_DEINITIALIZE)                    (VOID**                             eis2_handle);

    typedef CDKResult (*EIS2_GET_GYRO_INTERVAL)               (VOID*                              eis2_handle,
                                                               const frame_times_t*               frame_time_input,
                                                               uint32_t                           active_sensor_idx,
                                                               gyro_times_t*                      gyro_req_interval);

    typedef CDKResult (*EIS2_GET_TOTAL_MARGIN)                (VOID*                              eis2_handle,
                                                               uint32_t                           active_sensor_idx,
                                                               ImageDimensions*                   stabilizationMargins);

    typedef CDKResult (*EIS2_GET_TOTAL_MARGIN_EX)             (const is_get_stabilization_margin* in,
                                                               ImageDimensions*                   stabilizationMargins);

    typedef CDKResult(*EIS2_GET_STABILIZATION_CROP_RATIO_EX)  (const is_get_stabilization_margin* in,
                                                               double*                            stabilization_crop_ratio_x,
                                                               double*                            stabilization_crop_ratio_y);

    ///< Typedefs for EIS utility functions
    typedef CDKResult(*EIS_UTILS_CONVERT_TO_WINDOW_REGIONS)   (const WindowRegion*                ife,
                                                               const WindowRegion*                ipe,
                                                               double                             stabilization_crop_ratio_x,
                                                               double                             stabilization_crop_ratio_y,
                                                               uint32_t                           stabilization_input_size_x,
                                                               uint32_t                           stabilization_input_size_y,
                                                               eis_roi_windows*                   eisWindowRegions);

    typedef CDKResult(*EIS_UTILS_LOG_INIT)                    (VOID**                             context,
                                                               const is_utils_log_init*           init_data);

    typedef CDKResult(*EIS_UTILS_LOG_WRITE)                   (VOID*                              context,
                                                               const is_utils_log_write_data*     data);

    typedef CDKResult(*EIS_UTILS_LOG_OPEN)                    (VOID*                               context,
                                                               const char*                         new_prefix);

    typedef CDKResult(*EIS_UTILS_LOG_IS_OPENED)               (VOID*                               context,
                                                               bool*                               result);

    typedef CDKResult(*EIS_UTILS_LOG_CLOSE)                   (VOID*                               context);

    typedef CDKResult(*EIS_UTILS_LOG_FLUSH)                   (VOID*                               context);

    typedef CDKResult(*EIS_UTILS_LOG_DESTROY)                 (VOID**                              context);

    ///< EIS algorithm function pointers
    EIS2_INITIALIZE                      m_eis2Initialize;                  ///< Function pointer for eis2_initialize
    EIS2_PROCESS                         m_eis2Process;                     ///< Function pointer for eis2_process
    EIS2_DEINITIALIZE                    m_eis2Deinitialize;                ///< Function pointer for eis2_deinitialize
    EIS2_GET_GYRO_INTERVAL               m_eis2GetGyroTimeInterval;         ///< Function pointer for
                                                                            ///< eis2_get_gyro_time_interval
    EIS2_GET_TOTAL_MARGIN                m_eis2GetTotalMargin;              ///< Function pointer for eis2_get_total_margin
    EIS2_GET_TOTAL_MARGIN_EX             m_eis2GetTotalMarginEx;            ///< Function pointer for eis2_get_total_margin_ex
    EIS2_GET_STABILIZATION_CROP_RATIO_EX m_eis2GetStabilizationCropRatioEx; ///< Function poiter  for
                                                                            ///< eis2_get_stabilization_crop_ratio_ex
    EIS_UTILS_CONVERT_TO_WINDOW_REGIONS  m_eisUtilsConvertToWindowRegions;  ///< Function pointer for
                                                                            ///< eis_utility_convert_to_window_regions
    EIS_UTILS_LOG_INIT                   m_eisUtilsLogInit;                 ///< Function pointer for eis_utils_log_init
    EIS_UTILS_LOG_WRITE                  m_eisUtilsLogWrite;                ///< Function pointer for eis_utils_log_write
    EIS_UTILS_LOG_OPEN                   m_eisUtilsLogOpen;                 ///< Function pointer for eis_utils_log_open
    EIS_UTILS_LOG_IS_OPENED              m_eisUtilsLogIsOpened;             ///< Function pointer for eis_utils_log_is_opened
    EIS_UTILS_LOG_CLOSE                  m_eisUtilsLogClose;                ///< Function pointer for eis_utils_log_close
    EIS_UTILS_LOG_FLUSH                  m_eisUtilsLogFlush;                ///< Function pointer for eis_utils_log_flush
    EIS_UTILS_LOG_DESTROY                m_eisUtilsLogDestroy;              ///< Function pointer for eis_utils_log_destroy

    ///< Member Variables
    CHILIBRARYHANDLE         m_hEISv2Lib;                                   ///< Handle for EISV2 library.
    CHIHANDLE                m_hChiSession;                                 ///< The Chi session handle
    UINT32                   m_nodeId;                                      ///< The node's Id
    UINT32                   m_nodeCaps;                                    ///< The selected node caps
    UINT32                   m_lookahead;                                   ///< Number of future frames
    CHIDATASOURCE            m_hChiDataSource;                              ///< CHI Data Source Handle
    UINT32                   m_primarySensorIdx;                            ///< Primary sensor index
    EISV2PerSensorData       m_perSensorData[MaxMulticamSensors];           ///< Per camera data
    StabilizationMargin      m_stabilizationMargins;                        ///< Total stabilization margins in pixels
    ImageDimensions          m_additionalCropOffset;                        ///< Additional crop offset for output image
    VOID*                    m_phEIS2Handle;                                ///< EIS2 Algo Handle
    ChiICAVersion            m_ICAVersion;                                  ///< ICA version
    cam_is_operation_mode_t  m_algoOperationMode;                           ///< Calibration mode
    EISV2PathType            m_inputPortPathType;                           ///< EIS2 input port path type
    BOOL                     m_bIsLDCGridEnabled;                           ///< Flag for LDC grid enable
    BOOL                     m_isEnabledDumpForceFlush;                     ///< Flag to indicate if node will be forced to
                                                                            ///< call algorithm log flush API each iteration
    NcLibWarpGrid            m_LDCIn2OutWarpGrid[MaxMulticamSensors];       ///< LDC in to out warp grid
    NcLibWarpGrid            m_LDCOut2InWarpGrid[MaxMulticamSensors];       ///< LDC out to in warp grid
    NcLibWarpGridCoord*      m_pLDCIn2OutGrid[MaxMulticamSensors];          ///< Pointer to LDC in to out grid
    NcLibWarpGridCoord*      m_pLDCOut2InGrid[MaxMulticamSensors];          ///< Pointer to LDC out to in grid
    CHINODEFLAGS             m_nodeFlags;                                   ///< Node flags
    BOOL                     m_gyroNCSServiceAvailable;                     ///< Flag to indicate if gyro NCS service is
                                                                            ///< available
    BOOL                     m_bIsDefaultGridTransformEnabled;              ///< Flag to check if default grid and
                                                                            ///< transform enable
    UINT32                   m_numOfLinkedCameras;                          ///< Number of linked cameras in the current
                                                                            ///< session
    UINT32                   m_primaryCameraId;                             ///< Primary Camera Id
    double                   m_stabilizationCropRatioX[MaxMulticamSensors]; ///< Stabilization crop ratio on x-axis array,
                                                                            ///< for eis_utility_convert_to_window_regions.
    double                   m_stabilizationCropRatioY[MaxMulticamSensors]; ///< Stabilization crop ratio on y-axis array,
                                                                            ///< for eis_utility_convert_to_window_regions.
    VOID*                    m_pEisUtilsLogContext;                         ///< EIS log utility context
    is_utils_log_flags       m_isUtilsLogFlags;                             ///< flags for logging
    UINT                     m_isFOVCEnabled;                               ///< Flag to indicate if FOVC is enabled
    FLOAT                    m_cropFactorFOVC;                              ///< FOVC crop factor
    cam_is_ois_mode          m_overrideOisMode;                             ///< OIS mode override
};

#endif // CAMXCHINODEEISV2_H
