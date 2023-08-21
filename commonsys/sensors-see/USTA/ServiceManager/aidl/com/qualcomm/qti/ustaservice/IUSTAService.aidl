/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.ustaservice;

// Declare any non-default types here with import statements
import com.qualcomm.qti.ustaservice.USTASensorInfo;
import com.qualcomm.qti.ustaservice.SMModeType;
import com.qualcomm.qti.ustaservice.SMReqMsgPayload;
import com.qualcomm.qti.ustaservice.SMClientReqMsgFields;
import com.qualcomm.qti.ustaservice.SMNativeErrorCodes;

interface IUSTAService {

    /************************** USTA TAB FUNCTIONALITY ******************************/
    USTASensorInfo Initialize();

    SMNativeErrorCodes sendRequest(in SMModeType ustaMode,
                     int sensorsHandle,
                     in SMReqMsgPayload reqMsg,
                     in SMClientReqMsgFields stdFieldInfo,
                     String logFileName);

    void stopRequest(in SMModeType ustaMode,
                     int sensorsHandle,
                     boolean isQmiDisable);

    void updateLoggingFlag(boolean status);

    void updateStreamingStatus(int sensorsHandle,
                               boolean status);

    String getSamplesFromNative(boolean isRegistrySensor);

    void destroySensors(in SMModeType ustaMode);

    /************************** DEVICE MODE TAB FUNCTIONALITY ******************************/
    int deviceModeState(int number,
                         int state);

    /************************** ACEL CAL TAB FUNCTIONALITY ******************************/
    void accelCalibration(boolean enabled);

    String getCalSamples();

    /************************** DRM FUNCTIONALITY ******************************/
    int getchannelDRM(int bufferSize);

    void startDRM(in long[] sensorSuid, int samplePeriodUs, int flags, int streamHandle, int sensorHandle);

    void stopDRM(int streamHandle);

    void closechannelDRM(int streamHandle);

    int getmaxrateDRM(in long[] sensorSuid);

    /************************** ON DEVICE LOGGING FUNCTIONALITY ******************************/
    void onDeviceLogging();

    /************************* Error Handling Cases *********************************/
    void RegisterClientDeathStatus(in IBinder clientDeathListener);
}
