/**
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.ustaservice;

public class JavaNativeInterfaceAPIs {

    /*Final JNI APIs - No Change in these APIs when compared to previous version APIs */
    private static native SensorListInfo[] getSensorList();

    private static native ReqMsgPayload[] getRequestMessages(int inSensorHandle);

    private static native String getSensorAttributes(int inSensorHandle);

    private static native NativeErrorCodes sendRequest(ModeType inModeType, int inSensorHandle, ReqMsgPayload inReqMsg, SendReqStdFields sendReqStdFieldObj, String logFileName);

    private static native String[] getClientProcessors();

    private static native String[] getWakeupDeliveryType();

    private static native void stopRequest(ModeType inModeType, int inSensorHandle, boolean isClientDisableRequestEnabled);

    private static native void destroySensors(ModeType inModeType);

    private static native void removeSensors(int[] inRogueSensors);

    private static native void updateLoggingFlag(boolean isLoggingDisabled);

    private static native void enableStreamingStatus(int sensorHandle);

    private static native void disableStreamingStatus(int sensorHandle);

    private static native String getSamplesFromNative(boolean isRegistrySensor);

    /*Device Mode */
    private static native int sendDeviceModeIndication(int ModeType , int ModeState);

    /*Accel Calibration */
    private static native void startCalibration();

    private static native String getAccelCalFromNative();

    private static native void stopCalibration();

    /*DRM */
    private static native int getchannel(int bufferSize);
    private static native void start(long[] sensorSuid, int samplePeriodUs, int flags, int streamHandle , int sensorHandle);
    private static native void stop(int streamHandle);
    private static native void closechannel(int streamHandle);
    private static native int getmaxrate(long[] sensorSuid);

    public static void destroySensorsWrapper(ModeType inModeType) {
        destroySensors(inModeType);
    }

    public static int sendDeviceModeIndicationWrapper(int ModeType , int ModeState) {
        return sendDeviceModeIndication(ModeType, ModeState);
    }

    public static SensorListInfo[] getSensorListNativeWrapper() {
        return getSensorList();
    }

    public static void enableStreamingStatusNativeWrapper(int sensorHandle) {
        enableStreamingStatus(sensorHandle);
    }

    public static void disableStreamingStatusNativeWrapper(int sensorHandle) {
        disableStreamingStatus(sensorHandle);
    }

    public static String getSensorAttributesNativeWrapper(int sensorHandle) {
        return getSensorAttributes(sensorHandle);
    }

    public static ReqMsgPayload[] getRequestMessagesNativeWrapper(int sensorHandle) {
        return getRequestMessages(sensorHandle);
    }

    public static String[] getClientProcessorsNativeWrapper() {
        return getClientProcessors();
    }

    public static String[] getWakeupDeliveryNativeWrapper() {
        return getWakeupDeliveryType();
    }

    public static NativeErrorCodes sendRequestNativeWrapper(ModeType inModeType, int inSensorHandle, ReqMsgPayload inReqMsg, SendReqStdFields sendReqStdFieldObj, String logFileName) {
        return(sendRequest(inModeType, inSensorHandle, inReqMsg, sendReqStdFieldObj, logFileName));
    }


    public static void stopRequestNativeWrapper(ModeType inModeType , int inSensorHandle , boolean isClientDisableRequestEnabled) {
        stopRequest(inModeType, inSensorHandle , isClientDisableRequestEnabled);
    }

    public static void removeSensorsNativeWrapper(int[] inRogueSensors) {
        removeSensors(inRogueSensors);
    }

    public static void updateLoggingFlagWrapper(boolean isLoggingDisabled){
        updateLoggingFlag(isLoggingDisabled);
    }

    public static String getSamplesFromNativeWrapper(boolean isRegistrySensor){
        return getSamplesFromNative(isRegistrySensor);
    }

    public static void startCalibrationWrapper() {
        startCalibration();
    }

    public static void stopCalibrationWrapper(){
        stopCalibration();
    }

    public static String getAccelCalFromNativeWrapper() {
       return getAccelCalFromNative();
    }

    public static int getchannelWrapper(int bufferSize) {
        return getchannel(bufferSize);
    }

    public static void startWrapper(long[] sensorSuid, int samplePeriodUs, int flags, int streamHandle, int sensorHandle) {
        start(sensorSuid,samplePeriodUs,flags,streamHandle, sensorHandle);
    }

    public static void stopWrapper(int streamHandle){
        stop(streamHandle);
    }

    public static void closechannelWrapper(int streamHandle){
        closechannel(streamHandle);
    }
    public static int getmaxrateWrapper(long[] sensorSuid){
        return getmaxrate(sensorSuid);
    }
}
