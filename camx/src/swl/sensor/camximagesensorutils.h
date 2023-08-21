////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   camximagesensorutils.h
///
/// @brief  Utility functions for camera image sensor and submodules.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXIMAGESENSORUTILS_H
#define CAMXIMAGESENSORUTILS_H

#include "awbglobalelements.h"
#include "camxcslsensordefs.h"
#include "camxsensordriver.h"
#include "camxsensorsdkcommon.h"
#include "camxstaticcaps.h"
#include "camxtuningdatamanager.h"
#include "camxutils.h"
#include "cc_1_3_0.h"
#include "modmwbv1.h"
#include "camxpacket.h"
#include "camximagebuffermanager.h"
#include "camximagebuffer.h"

CAMX_NAMESPACE_BEGIN

const FLOAT MinimumToMaximumFPSRatio = 0.25;            /// Maximum FPS is 4 times the minimum FPS
const FLOAT GainToISOMultiplicationFactor   = 100;      /// Gain of 1.0f correspond to ISO 100.
const UINT  MaximumI2CHardwareDelayinUSec   = 2000;     /// Maximum delay can be configured as hardware delay in Micro seconds

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Static utility class for image sensor.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ImageSensorUtils
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreatePowerSequenceCmd
    ///
    /// @brief  Create the power sequence commands for the image sensor.
    ///
    /// @param  pCmdBuffer        The command buffer for the power sequence.
    /// @param  isPowerUp         Power up / Down flag.
    /// @param  powerSequenceSize Size of the power sequence commands.
    /// @param  pSettings         The power up/down seq settings.
    ///
    /// @return CamxResultSuccess, if SUCCESS.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult CreatePowerSequenceCmd(
       VOID*         pCmdBuffer,
       BOOL          isPowerUp,
       UINT          powerSequenceSize,
       PowerSetting* pSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPowerSequenceCmdSize
    ///
    /// @brief  Computes the power sequence commands size.
    ///
    /// @param  powerSequenceSize Size of the power sequence commands.
    /// @param  pSettings         The power up/down seq settings.
    ///
    /// @return size of the power sequence commands.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT GetPowerSequenceCmdSize(
       UINT          powerSequenceSize,
       PowerSetting* pSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateWaitCommand
    ///
    /// @brief  Updates the wait command buffer parametrs based on hardware/software delay.
    ///
    /// @param  pWaitCmd    Pointer to the wait command buffer
    /// @param  delayUs     delay in micro seconds to be configured
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID UpdateWaitCommand(
        CSLSensorWaitCmd* pWaitCmd,
        UINT32            delayUs);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateI2CInfoCmd
    ///
    /// @brief  Creates the I2C Information command.
    ///
    /// @param  pI2CInfoCmd       Pointer to the command info structure.
    /// @param  slaveAddr         Slave address of the I2C slave.
    /// @param  I2CFrequencyMode  I2C Slave frequency mode.
    ///
    /// @return size of the power sequence commands.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult CreateI2CInfoCmd(
        CSLSensorI2CInfo*   pI2CInfoCmd,
        UINT16              slaveAddr,
        I2CFrequencyMode    I2CFrequencyMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRegisterSettingsCmdSize
    ///
    /// @brief  Get the command size for the Register Settings.
    ///
    /// @param  settingsCount         Register settings count.
    /// @param  pSettings             pointer to settings
    /// @param  pRegSettingIndex      start index for register setting for the current slave address(multi-slave addr support)
    ///
    /// @return size of the command for Register Settings.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT GetRegisterSettingsCmdSize(
        UINT             settingsCount,
        RegisterSetting* pSettings,
        UINT*            pRegSettingIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateRegisterSettingsCmd
    ///
    /// @brief  Create the command for the given Register Settings.
    ///
    /// @param  pCmdBuffer          Pointer to the Command Buffer.
    /// @param  settingsCount       Register settings count.
    /// @param  pSettings           Pointer to register settings
    /// @param  regSettingIdx       returns start index for the next slave addr(multi-slave addr support)
    ///                             or 0 in case of just one slave address
    /// @param  sensorFrequencyMode i2c frequency mode, default is FAST(Enum Value = 1)
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult CreateRegisterSettingsCmd(
        VOID*            pCmdBuffer,
        UINT             settingsCount,
        RegisterSetting* pSettings,
        UINT             regSettingIdx,
        I2CFrequencyMode sensorFrequencyMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTunedColorCorrectionData
    ///
    /// @brief  Obtains the color correction data for the specified tuned mode
    ///
    /// @param  pTuningManager          Pointer to the tuning manager.
    /// @param  pTuningMode             mode for which the CC data is needed.
    /// @param  numSelectors            number of the selectors
    /// @param  pColorCorrectionData    output data containing the CC values.
    /// @param  colorCorrectionDataSize size of the color correction data
    /// @param  colorTemperature        Specify the color correction data for this color temperature is needed
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetTunedColorCorrectionData(
        TuningDataManager* pTuningManager,
        TuningMode*        pTuningMode,
        UINT32             numSelectors,
        FLOAT*             pColorCorrectionData,
        SIZE_T             colorCorrectionDataSize,
        FLOAT              colorTemperature);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTunedMWBData
    ///
    /// @brief  Create the command for the given Register Settings.
    ///
    /// @param  pTuningManager          Pointer to the tuning manager.
    /// @param  pTuningMode             mode for which the CC data is needed.
    /// @param  numSelectors            number of the selectors
    /// @param  pMWBData                output data containing the MWB.
    /// @param  MWBIlluminantIndex      Index of the MWB data to be copied.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetTunedMWBData(
        TuningDataManager*                  pTuningManager,
        TuningMode*                         pTuningMode,
        UINT32                              numSelectors,
        awbglobalelements::awbRGBDataType*  pMWBData,
        UINT8                               MWBIlluminantIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetISO100GainData
    ///
    /// @brief  Get iso100gain from 3a chromatix.
    ///
    /// @param  pTuningManager        Pointer to the tuning manager.
    /// @param  pTuningMode           mode for which the CC data is needed.
    /// @param  pISO100Gain           the measured gain of sensor for ISO100
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetISO100GainData(
        TuningDataManager* pTuningManager,
        TuningMode*        pTuningMode,
        FLOAT *            pISO100Gain);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMaxExposureData
    ///
    /// @brief  Get max exposure value from 3a chromatix.
    ///
    /// @param  pTuningManager        Pointer to the tuning manager.
    /// @param  pTuningMode           mode for which the CC data is needed.
    /// @param  numSelectors          number of Tuning Mode selectors
    /// @param  pMaxExposure          the max exposure of the sensor
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetMaxExposureData(
        TuningDataManager* pTuningManager,
        TuningMode*        pTuningMode,
        UINT               numSelectors,
        UINT64*            pMaxExposure);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MatrixInverse3x3
    ///
    /// @brief  Inverses the 3x3 matrix
    ///
    /// @param  pInput      input 3x3 matrix.
    /// @param  pOutput     output 3x3 matrix.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID MatrixInverse3x3(
        const FLOAT*    pInput,
        FLOAT*          pOutput)
    {
        FLOAT determinant = 0.0f;

        /// @todo (CAMX-1215) - Make this function generic and move it to utils and add a Unit test case.
        determinant = (pInput[0] * ((pInput[4] * pInput[8]) - (pInput[5] * pInput[7]))) +
                      (pInput[1] * ((pInput[5] * pInput[6]) - (pInput[3] * pInput[8]))) +
                      (pInput[2] * ((pInput[3] * pInput[7]) - (pInput[4] * pInput[6])));

        if (determinant == 0)
        {
            return;
        }

        pOutput[0] = ((pInput[4] * pInput[8]) - (pInput[5] * pInput[7])) / determinant;
        pOutput[1] = ((pInput[2] * pInput[7]) - (pInput[1] * pInput[8])) / determinant;
        pOutput[2] = ((pInput[1] * pInput[5]) - (pInput[2] * pInput[4])) / determinant;
        pOutput[3] = ((pInput[5] * pInput[6]) - (pInput[3] * pInput[8])) / determinant;
        pOutput[4] = ((pInput[0] * pInput[8]) - (pInput[2] * pInput[6])) / determinant;
        pOutput[5] = ((pInput[2] * pInput[3]) - (pInput[0] * pInput[5])) / determinant;
        pOutput[6] = ((pInput[3] * pInput[7]) - (pInput[4] * pInput[6])) / determinant;
        pOutput[7] = ((pInput[1] * pInput[6]) - (pInput[0] * pInput[7])) / determinant;
        pOutput[8] = ((pInput[0] * pInput[4]) - (pInput[1] * pInput[3])) / determinant;

        return;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FloatToRational3x3
    ///
    /// @brief  Convert 3x3 float matrix into 3x3 rational matrix.
    ///
    /// @param  input   input 3x3 float matrix.
    /// @param  output  output 3x3 rational matrix.
    ///
    /// @return None.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE VOID FloatToRational3x3(
        FLOAT       input[3][3],
        Rational    output[3][3])
    {
        UINT8   row = 0;
        UINT8   coloumn = 0;

        for (row = 0; row < 3; row++)
        {
            for (coloumn = 0; coloumn < 3; coloumn++)
            {
                output[row][coloumn].numerator      = Utils::FLOATToSFixed32(input[row][coloumn], 7, TRUE);
                output[row][coloumn].denominator    = Utils::FLOATToSFixed32(1, 7, TRUE);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GenerateTransformMatrix
    ///
    /// @brief  generates the forward and color transform matrices.
    ///
    /// @param  forwardMatrix           input1 matrix of size 3x3.
    /// @param  colorMatrix             input2 matrix of size 3x3.
    /// @param  pColorCorrectionData    output matrix of size 3x3.
    /// @param  pMWBData                output matrix of size 3x3.
    /// @param  isD65                   output matrix of size 3x3.
    ///
    /// @return None.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GenerateTransformMatrix(
        Rational                            forwardMatrix[3][3],
        Rational                            colorMatrix[3][3],
        FLOAT*                              pColorCorrectionData,
        awbglobalelements::awbRGBDataType*  pMWBData,
        BOOL                                isD65);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GenerateCalibrationTransformMatrix
    ///
    /// @brief  generates calibration transform matrix.
    ///
    /// @param  matrix      output matrix of size 3x3 go get the calibration transform matrix
    /// @param  r_gain      wbc  R gain.
    /// @param  b_gain      WBC B gain.
    ///
    /// @return None.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GenerateCalibrationTransformMatrix(
        Rational    transformMatrix[3][3],
        FLOAT       rGain,
        FLOAT       bGain);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GenerateUnitMatrix
    ///
    /// @brief  generates the unit matrix
    ///
    /// @param  unitMatrix     output unit matrix of size 3x3.
    ///
    /// @return None.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GenerateUnitMatrix(
        Rational unitMatrix[3][3]);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsYUVCamera
    ///
    /// @brief  Finds whether its YUV camera or not based on the information available in resolution data.
    ///
    /// @param  pResolutionData     resolution data information.
    ///
    /// @return TRUE if its YUV camera or FALSE.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE BOOL IsYUVCamera(
        ResolutionData* pResolutionData)
    {
        return ((pResolutionData->colorFilterArrangement == ColorFilterArrangement::YUV_UYVY) ||
                (pResolutionData->colorFilterArrangement == ColorFilterArrangement::YUV_YUYV)) ?
               TRUE : FALSE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculatePhysicalSensorWidth
    ///
    /// @brief  Calculates the physical width of the sensor.
    ///
    /// @param  pSensorDriverData     pointer to sensor driver data.
    ///
    /// @return physical sensor width.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE FLOAT CalculatePhysicalSensorWidth(
        SensorDriverData* pSensorDriverData)
    {
        return static_cast<FLOAT>((pSensorDriverData->pixelArrayInfo.activeDimension.width) *
                                  ((pSensorDriverData->sensorProperty.pixelSize) /
                                   (MicroMetersPerMilliMeter)));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculatePhysicalSensorHeight
    ///
    /// @brief  Calculates the physical height of the sensor.
    ///
    /// @param  pSensorDriverData     pointer to sesnsor driver data.
    ///
    /// @return physical sensor height.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE FLOAT CalculatePhysicalSensorHeight(
        SensorDriverData* pSensorDriverData)
    {
        return static_cast<FLOAT>((pSensorDriverData->pixelArrayInfo.activeDimension.height) *
                                  ((pSensorDriverData->sensorProperty.pixelSize) /
                                   (MicroMetersPerMilliMeter)));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateTotalPixelArrayWidth
    ///
    /// @brief  Calculates the width of the pixel array.
    ///
    /// @param  pSensorDriverData     pointer to sensor driver data.
    ///
    /// @return pixel array width.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE INT32 CalculateTotalPixelArrayWidth(
        SensorDriverData* pSensorDriverData)
    {
        return ((pSensorDriverData->pixelArrayInfo.activeDimension.width) +
                (pSensorDriverData->pixelArrayInfo.dummyInfo.left) +
                (pSensorDriverData->pixelArrayInfo.dummyInfo.right));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateTotalPixelArrayHeight
    ///
    /// @brief  Calculates the height of the pixel array.
    ///
    /// @param  pSensorDriverData     pointer to sensor driver data.
    ///
    /// @return pixel array height.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE INT32 CalculateTotalPixelArrayHeight(
        SensorDriverData* pSensorDriverData)
    {
        return ((pSensorDriverData->pixelArrayInfo.activeDimension.height) +
                (pSensorDriverData->pixelArrayInfo.dummyInfo.top) +
                (pSensorDriverData->pixelArrayInfo.dummyInfo.bottom));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateMaximumISOSensitivity
    ///
    /// @brief  Calculates the maximum ISO sensitivity using exposure control information.
    ///
    /// @param  pSensorDriverData        pointer to sensor driver data.
    /// @param  ISOMultiplicationFactor  ISO Multiplication Factor
    ///
    /// @return Maximum value of ISO sensitivity.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE INT32 CalculateMaximumISOSensitivity(
        SensorDriverData* pSensorDriverData,
        FLOAT             ISOMultiplicationFactor)
    {
        return Utils::RoundFLOAT(static_cast<FLOAT>(pSensorDriverData->exposureControlInfo.maxAnalogGain) *
                                 static_cast<FLOAT>(pSensorDriverData->exposureControlInfo.maxDigitalGain) *
                                 ISOMultiplicationFactor);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateMinimumISOSensitivity
    ///
    /// @brief  Calculates the minimum ISO sensitivity using exposure control information.
    ///
    /// @param  pSensorDriverData        pointer to sensor driver data.
    /// @param  ISOMultiplicationFactor  ISO Multiplication Factor
    ///
    /// @return Minimum value of ISO sensitivity.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static INT32 CalculateMinimumISOSensitivity(
        SensorDriverData* pSensorDriverData,
        FLOAT             ISOMultiplicationFactor)
    {
        if (0 == pSensorDriverData->exposureControlInfo.minAnalogGain)
        {
            pSensorDriverData->exposureControlInfo.minAnalogGain = 1;
        }
        return Utils::RoundFLOAT(static_cast<FLOAT>(pSensorDriverData->exposureControlInfo.minAnalogGain) *
                                 ISOMultiplicationFactor);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadDeviceData
    ///
    /// @brief  Util function to read N bytes from a device and copy to a memory supplied by the calling function
    ///
    /// @param  deviceID          Device ID of the submodule to read from
    /// @param  numOfBytes        Number of bytes to read
    /// @param  pReadData         Pointer where the data needs to be copied to
    /// @param  opcode            Read opcode based on the device to be read
    /// @param  hCSLDeviceHandle  CSL Handle of the device to be read
    /// @param  device            Device type to be read
    /// @param  hCSLSession       CSL session handle
    /// @param  pSensorReadPacket Packet that will be submitted to the kernel to carry the request.
    ///                           This packet should have command buffer
    ///
    /// @return Result success or failure.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult ReadDeviceData(
        INT32         deviceID,
        UINT16        numOfBytes,
        UINT8*        pReadData,
        UINT32        opcode,
        INT32         hCSLDeviceHandle,
        CSLDeviceType device,
        INT32         hCSLSession,
        Packet*       pSensorReadPacket);

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetClosestColorCorrectionTransformIndex
    ///
    /// @brief  Get the closest tuned color correction transform index
    ///
    /// @param  pCC13AECData     pointer to bps color correction transform
    /// @patam  colorTemperature color temperature
    ///
    /// @return closest tuned color correction transform index.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 GetClosestColorCorrectionTransformIndex(
        cc_1_3_0::mod_cc13_aec_dataType* pCC13AECData,
        FLOAT                            colorTemperature);

    ImageSensorUtils()                                          = delete;
    ImageSensorUtils(const ImageSensorUtils& rOther)             = delete;
    ImageSensorUtils& operator=(const ImageSensorUtils& rOther)  = delete;
};

CAMX_NAMESPACE_END

#endif // CAMXIMAGESENSORUTILS_H
