/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.usta.core;

import android.content.Context;

import com.qualcomm.qti.ustaservice.USTAServiceManager;
import com.qualcomm.qti.ustaservice.USTASensorInfo;
import com.qualcomm.qti.ustaservice.SMSensorInfo;
import com.qualcomm.qti.ustaservice.SMNativeErrorCodes;
import com.qualcomm.qti.ustaservice.SMModeType;
import com.qualcomm.qti.ustaservice.SMReqMsgPayload;
import com.qualcomm.qti.ustaservice.SMClientReqMsgFields;

public class USTAServiceUtil {
    /*USTA Tab */
    private static USTAServiceManager ustaService;
    private static USTASensorInfo USTASensorInfoVar;
    public static void ServiceInstanceInit(Context context) {
        if(null == ustaService) {
            ustaService = USTAServiceManager.getInstance(context);
            USTASensorInfoVar = ustaService.getSensorsInformation();
        }
    }

    public static SensorListInfo[] getSensorList() {
        SensorListInfo[] senorsInfoList = new SensorListInfo[USTASensorInfoVar.SensorsInfo.size()];
        for(int sensorIndex = 0 ; sensorIndex < USTASensorInfoVar.SensorsInfo.size() ; sensorIndex++) {
            SensorListInfo it = new SensorListInfo();
            senorsInfoList[sensorIndex] = it;
            senorsInfoList[sensorIndex].setDataType(USTASensorInfoVar.SensorsInfo.get(sensorIndex).dataType);
            senorsInfoList[sensorIndex].setVendor(USTASensorInfoVar.SensorsInfo.get(sensorIndex).vendor);
            senorsInfoList[sensorIndex].setSuidLow(USTASensorInfoVar.SensorsInfo.get(sensorIndex).suidLow);
            senorsInfoList[sensorIndex].setSuidHigh(USTASensorInfoVar.SensorsInfo.get(sensorIndex).suidHigh);
        }
        return senorsInfoList;
    }

    public static String getSensorAttributes(int sensorIndex) {
        return USTASensorInfoVar.SensorsInfo.get(sensorIndex).attributesInfo;
    }

    public static String[] getClientProcessors() {
        String[] clientProcessInfo = new String[USTASensorInfoVar.clientProcessorTypes.size()];
        for(int typeIndex = 0 ; typeIndex < USTASensorInfoVar.clientProcessorTypes.size(); typeIndex++) {
            clientProcessInfo[typeIndex] = USTASensorInfoVar.clientProcessorTypes.get(typeIndex);
        }
        return clientProcessInfo;
    }

    public static String[] getWakeupDeliveryType() {
        String[] wakeupDeliveryInfo = new String[USTASensorInfoVar.wakeupDeliveryTypes.size()];
        for(int typeIndex = 0 ; typeIndex < USTASensorInfoVar.wakeupDeliveryTypes.size(); typeIndex++) {
            wakeupDeliveryInfo[typeIndex] = USTASensorInfoVar.wakeupDeliveryTypes.get(typeIndex);
        }
        return wakeupDeliveryInfo;
    }

    public static ReqMsgPayload[] getRequestMessages(int inSensorHandle) {
        ReqMsgPayload[] reqMsgInfo = new ReqMsgPayload[USTASensorInfoVar.SensorsInfo.get(inSensorHandle).requestMsgs.size()];
        for(int reqMsgIndex = 0 ; reqMsgIndex < USTASensorInfoVar.SensorsInfo.get(inSensorHandle).requestMsgs.size(); reqMsgIndex++) {
            reqMsgInfo[reqMsgIndex] = typeCasteUtility.SMReqMsgPayloadToReqMsgPayload(USTASensorInfoVar.SensorsInfo.get(inSensorHandle).requestMsgs.get(reqMsgIndex));
        }
        return reqMsgInfo;
    }

    public static NativeErrorCodes sendRequest(ModeType inModeType, int inSensorHandle, ReqMsgPayload inReqMsg, SendReqStdFields sendReqStdFieldObj, String logFileName) {
        try {
            SMNativeErrorCodes err = ustaService.sendRequest(typeCasteUtility.ModeTypeToSMModeType(inModeType),
                    inSensorHandle,
                    typeCasteUtility.ReqMsgPayloadToSMReqMsgPayload(inReqMsg),
                    typeCasteUtility.SendReqStdFieldsToSMClientReqMsgFields(sendReqStdFieldObj),
                    logFileName);
            return typeCasteUtility.SMNativeErrorCodesToNativeErrorCodes(err);
        }catch (Exception e) {
            USTALog.e(e.getMessage());
            return NativeErrorCodes.USTA_MEMORY_ERROR;
        }
    }

    public static void stopRequest(ModeType inModeType, int inSensorHandle, boolean isClientDisableRequestEnabled){
        try {
            ustaService.stopRequest(typeCasteUtility.ModeTypeToSMModeType(inModeType),
                    inSensorHandle,
                    isClientDisableRequestEnabled);
        }catch (Exception e) {
            USTALog.e(e.getMessage());
        }
    }

    public static void updateLoggingFlag(boolean status) {
        try {
            ustaService.updateLoggingFlag(status);
        } catch (Exception e){
            USTALog.e(e.getMessage());
        }
    }

    public static void enableStreamingStatus(int sensorHandle){
        try {
            ustaService.updateStreamingStatus(sensorHandle, true);
        } catch (Exception e){
            USTALog.e(e.getMessage());
        }
    }

    public static void disableStreamingStatus(int sensorHandle){
        try {
            ustaService.updateStreamingStatus(sensorHandle, false);
        } catch (Exception e){
            USTALog.e(e.getMessage());
        }
    }

    public static String getSamplesFromNative(boolean isRegistrySensor){
        String sampleDataTobeDisplaced = null;
        try {
            sampleDataTobeDisplaced = ustaService.getSamplesFromNative(isRegistrySensor);
        }catch (Exception e) {
            USTALog.e(e.getMessage());
        }
        return sampleDataTobeDisplaced;
    }

    public static void removeSensors(ModeType inModeType){
        try {
            USTALog.i("removeSensors - service Util");
            ustaService.removeSensors(typeCasteUtility.ModeTypeToSMModeType(inModeType));
        } catch (Exception e){
            USTALog.e(e.getMessage());
        }
    }

    /*Device Mode Tab */
    public static int sendDeviceModeIndication(int ModeType , int ModeState) {
        return ustaService.deviceModeState(ModeType, ModeState);
    }
    /*Accel Cal */
    public static void startCalibration() {
        try {
            ustaService.accelCalibration(true);
        } catch (Exception e) {
            USTALog.e(e.getMessage());
        }
    }

    public static void stopCalibration() {
        try {
            ustaService.accelCalibration(false);
        } catch (Exception e) {
            USTALog.e(e.getMessage());
        }
    }

    public static String getAccelCalFromNative() {
        String accelCalDataTobeDisplaced = null;
        try {
            accelCalDataTobeDisplaced =  ustaService.getCalSamples();
        } catch (Exception e) {
            USTALog.e(e.getMessage());
        }
        return accelCalDataTobeDisplaced;
    }

    /*DRM */
    public static int getchannel(int bufferSize) {
        int ret = -1;
        try {
            ret = ustaService.getchannelDRM(bufferSize);
        }catch (Exception e ) {
            USTALog.e(e.getMessage());
        }
        return ret;
    }

    public static void start(long[] sensorSuid, int samplePeriodUs, int flags, int streamHandle, int sensorHandle) {
        try {
            ustaService.startDRM(sensorSuid, samplePeriodUs, flags, streamHandle, sensorHandle);
        } catch (Exception e ) {
            USTALog.e(e.getMessage());
        }

    }

    public static void stop(int streamHandle) {
        try {
            ustaService.stopDRM(streamHandle);
        }catch (Exception e ) {
            USTALog.e(e.getMessage());
        }
    }

    public static void closechannel(int streamHandle){
        try {
            ustaService.closechannelDRM(streamHandle);
        }catch (Exception e ) {
            USTALog.e(e.getMessage());
        }
    }

    public static int getmaxrate(long[] sensorSuid) {
        int ret = -1 ;
        try {
            ret = ustaService.getmaxrateDRM(sensorSuid);
        }catch (Exception e) {
            USTALog.e(e.getMessage());
        }
        return ret;
    }


}
