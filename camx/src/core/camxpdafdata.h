////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxpdafdata.h
/// @brief PDAF Driver Configuration Data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXPDAFDATA_H
#define CAMXPDAFDATA_H
#include "chistatsinterfacedefs.h"
#include "chipdlibcommon.h"
#include "chituningmodeparam.h"

// Forward declarations
struct CHIPDLib;
struct PDLibCreateParams;

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class containing PDAF APIs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PDAFData
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PDAFData
    ///
    /// @brief  Constructor
    ///
    /// @param  pPDAFConfigData    Pointer to PDAF Configuration data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit PDAFData(
        PDAFConfigurationData* pPDAFConfigData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~PDAFData
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~PDAFData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPDAFConfigData
    ///
    /// @brief  Retrieve PDAF Configuration Data
    ///
    /// @return PDAF Configuration data at index or NULL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const PDAFConfigurationData* GetPDAFConfigData()
    {
        return m_pPDAFConfigData;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPDAFEnabled
    ///
    /// @brief  Retrieve PDAF Configuration Data
    ///
    /// @return PDAF Configuration data at index or NULL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsPDAFEnabled()
    {
        return m_isPdafEnabled;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PDAFInit
    ///
    /// @brief  Populate PDAF Init data to creat the PD library
    ///
    /// @param  pPDLib                      Pointer to the pointer of the created PD library instance
    /// @param  resIdx                      Current Resolution Index
    /// @param  pActuatorData               Pointer to actuator data
    /// @param  pSensorData                 Pointer to sensor data
    /// @param  pOTPData                    Pointer to OTP data
    /// @param  pCameraInfo                 Pointer to camera information
    /// @param  pTuningManager              Pointer to tuning manager
    /// @param  pCAMIFT3DataPattern         Pointer to PDAF T3 Data Pattern
    /// @param  bufferFormat                RDI buffer format for LCR
    /// @param  pHwContext                  Pointer to Hardware Context
    /// @param  pPDHWEnableConditions       Pointer to PD HW Enable conditions
    /// @param  pSensorCrop                 Pointer to Sensor mode specific Crop Info
    /// @param  pChiTuningModeParameter     Pointer to Tuning Mode Parameter
    ///
    /// @return Success if data is present
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PDAFInit(
        CHIPDLib*                   pPDLib,
        UINT32                      resIdx,
        ActuatorData*               pActuatorData,
        ImageSensorData*            pSensorData,
        const EEPROMOTPData*        pOTPData,
        StatsCameraInfo*            pCameraInfo,
        VOID*                       pTuningManager,
        VOID*                       pCAMIFT3DataPattern,
        PDLibBufferFormat           bufferFormat,
        HwContext*                  pHwContext,
        PDHWEnableConditions*       pPDHWEnableConditions,
        SensorCropInfo*             pSensorCrop,
        ChiTuningModeParameter*     pChiTuningModeParameter);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PDAFDeInit
    ///
    /// @brief  Deinitialize PDAF
    ///
    /// @return Success if deinitialization is successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CamxResult PDAFDeInit()
    {
        return CamxResultSuccess;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPDAFSensorType
    ///
    /// @brief  Get PDAF Sensor type from PDAF Type
    ///
    /// @param  PDType Value from the sensor driver
    /// @param  pData  Pointer that will contain returned PDLibSensorType data
    ///
    /// @return Success if the translation is successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPDAFSensorType(
        PDAFType PDType,
        VOID*    pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPDAFPixelType
    ///
    /// @brief  Get PDAF Sensor type from PDAF Type
    ///
    /// @param  PDPixelType Value from the PDAF driver
    /// @param  pData       Pointer that will contain returned PDLibPixelType data
    ///
    /// @return Success if the translation is successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPDAFPixelType(
        PDAFPixelShieldInformation PDPixelType,
        VOID*                      pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillPDAFPixelCoordinates
    ///
    /// @brief  Fill Pixel coordinates
    ///
    /// @param  PDBlockPattern Block pattern containing the Pixel coordinates
    /// @param  pData          Pointer that will contain PD Pixel coordinates
    ///
    /// @return Success if the translation is successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillPDAFPixelCoordinates(
        PDAFBlockPattern PDBlockPattern,
        VOID*            pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AdjustNativePDAFOffsets
    ///
    /// @brief  Adjust Pixel coordinates based on PDAF offsets in native block pattern information
    ///
    /// @param  pData  Pointer that will contain PDAF native block pattern
    ///
    /// @return Success if the translation is successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AdjustNativePDAFOffsets(
        VOID*            pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPDAFBufferDataFormat
    ///
    /// @brief  Get PDAF Data Buffer Format
    ///
    /// @param  PDBufferDataFormat PDAF Data Buffer format configured in the PDAF driver
    /// @param  pData              Pointer that will contain PD buffer format of PD Library
    ///
    /// @return Success if the translation is successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPDAFBufferDataFormat(
        PDAFBufferDataFormat PDBufferDataFormat,
        VOID*                pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsHDREnabled
    ///
    /// @brief  Checks if the HDR mode is selected in the sensor
    ///
    /// @param  pResolutionData Resolution data for the selected sensor modefrom the sensor driver
    ///
    /// @return 1 If the mode supports HDR capability
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT IsHDREnabled(
        ResolutionData* pResolutionData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPDAFOTPData
    ///
    /// @brief  Fills in the OTP data into PD Initialization Parameters
    ///
    /// @param  pPDLibInitParams PD Library Initialization parameters to be passed while creating it
    /// @param  pOTPData         OTP data pointer
    /// @param  pPDAFModeInfo    Pointer to PDAF Mode information
    ///
    /// @return Success if OTP data is successfully populated
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPDAFOTPData(
        PDLibCreateParams*      pPDLibInitParams,
        const EEPROMOTPData*    pOTPData,
        PDAFModeInformation*    pPDAFModeInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPDAFActuatorData
    ///
    /// @brief  Fills in the actuator data into PD Initialization Parameters
    ///
    /// @param  pActuatorData         Actuator driver data
    /// @param  pPDLibInitParams      PD Library Initialization parameters to be passed while creating it
    /// @param  pActuatorDataBitwidth Actuator data bit width, needed for configuring PDAF Type1 sensor
    ///
    /// @return Success if Actuator data is successfully populated
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPDAFActuatorData(
        ActuatorData*      pActuatorData,
        PDLibCreateParams* pPDLibInitParams,
        UINT16*            pActuatorDataBitwidth);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPDAFSensorData
    ///
    /// @brief  Fills in the sensor data into PD Initialization Parameters
    ///
    /// @param  pSensorData      Sensor driver data
    /// @param  pPDLibInitParams PD Library Initialization parameters to be passed while creating it
    /// @param  resIdx           Current sensor resolution index
    ///
    /// @return Success if Sensor data is successfully populated
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPDAFSensorData(
        ImageSensorData*   pSensorData,
        PDLibCreateParams* pPDLibInitParams,
        UINT32             resIdx);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrintDebugPDAFData
    ///
    /// @brief  Prints the PDAF driver data for debugging
    ///
    /// @param  PDAFData PDAF driver data to print
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrintDebugPDAFData(
        PDLibCreateParams PDAFData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInitialHWPDConfig
    ///
    /// @brief  Get initial HW PD configuration during start session.
    ///
    /// @param  pPDLib      Pointer to the pointer of the created PD library instance
    /// @param  pHWConfig   Pointer to PDHW config
    ///
    /// @return Success if Sensor data is successfully populated
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetInitialHWPDConfig(
        CHIPDLib*   pPDLib,
        PDHwConfig* pHWConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCurrentPDAFModeIndex
    ///
    /// @brief  Get corresponding PDAF mode index for current sensor resolution
    ///
    /// @param  resIdx      Current sensor resolution
    /// @param  pPDAFMode   Corresponding PDAF Mode index for the current sensor resolution
    ///
    /// @return Success if corresponding PDAF mode exists
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetCurrentPDAFModeIndex(
        UINT32  resIdx,
        UINT32* pPDAFMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPDAFLibraryName
    ///
    /// @brief  Get the name of the PDAF library to load
    ///
    /// @param  resIdx        Current sensor resolution
    ///
    /// @return Corresponding PDAF library name to be loaded, NULL otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHAR* GetPDAFLibraryName(
        UINT32 resIdx);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPDAFModeInformation
    ///
    /// @brief  Get PDAF Mode information
    ///
    /// @param  PDAFModeIdx      Current PDAF Mode being initialized
    /// @param  ppPDAFModeInfo   Pointer to PDAF mode information
    ///
    /// @return Success if PDAF Mode information exists
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPDAFModeInformation(
        UINT32                PDAFModeIdx,
        PDAFModeInformation** ppPDAFModeInfo);

private:
    PDAFData(const PDAFData&) = delete;               ///< Disallow the copy constructor
    PDAFData& operator=(const PDAFData&) = delete;    ///< Disallow assignment operator

    BOOL                    m_isPdafEnabled;          ///< TRUE if PDAF config data is enabled
    PDAFConfigurationData*  m_pPDAFConfigData;        ///< PDAF driver data pointer
    PDAFDCCCalibrationData  m_calibrationDataType1;   ///< PDAF calibration data for type1;
    PDAF2DCalibrationData   m_calibrationDataType2;   ///< PDAF calibration data for type2;
};

CAMX_NAMESPACE_END

#endif // CAMXPDAFDATA_H
