/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.ustaservice;

import android.app.Service;
import android.os.ServiceManager;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class USTAServiceImpl extends Service {
    private static final String TAG = "USTA_ServiceImpl";
    private static final String USTA_SERVICE_NAME = "com.qualcomm.qti.ustaservice.USTAServiceImpl";
    private IBinder ustaClientBinder;
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        System.loadLibrary("USTANative");
        System.loadLibrary("SensorCalLibNative");
        if (getPackageManager().hasSystemFeature(PackageManager.FEATURE_WATCH) == false) {
            System.loadLibrary("sensor_low_lat");
        }
        ServiceManager.addService(USTA_SERVICE_NAME, new USTABinderService());

    }

    /*Class to implements all the Binder calls from App - Start*/
    public final class USTABinderService extends IUSTAService.Stub {

        @Override
        public USTASensorInfo Initialize() throws RemoteException {
            List<SMSensorInfo> sensorsInfo = new ArrayList<SMSensorInfo>();
            SensorListInfo[] sensorListFromNative = JavaNativeInterfaceAPIs.getSensorListNativeWrapper();

            boolean[] isSensorToBeRemoved = new boolean[sensorListFromNative.length];
            int removeSensorCount = 0;

            for (int sensorHandle = 0 ; sensorHandle < sensorListFromNative.length ; sensorHandle ++) {
                String attributeInfo = JavaNativeInterfaceAPIs.getSensorAttributesNativeWrapper(sensorHandle);
                List<SMReqMsgPayload> requestMsgsList = new ArrayList<SMReqMsgPayload>();
                ReqMsgPayload[] reqMsgPayLoad = JavaNativeInterfaceAPIs.getRequestMessagesNativeWrapper(sensorHandle);
                /*Findout the rouge sensor. If it is rougue, do not add it to the list at service level.
                 But Keep track of sensor Handle. So that we can remove all rougue sensors at a time from Native*/
                if(null == reqMsgPayLoad || 0 == reqMsgPayLoad.length) {
                    isSensorToBeRemoved[sensorHandle] = true;
                    removeSensorCount++;
                    continue;
                } else {
                    isSensorToBeRemoved[sensorHandle] = false;
                }

                for(int reqMsgIndex = 0 ; reqMsgIndex < reqMsgPayLoad.length ; reqMsgIndex ++) {
                    SMReqMsgPayload it = typeCasteUtility.ReqMsgPayloadToSMReqMsgPayload(reqMsgPayLoad[reqMsgIndex]);
                    requestMsgsList.add(it);
                }
                SMSensorInfo currentSensorInfo =  new SMSensorInfo(sensorListFromNative[sensorHandle].dataType,
                        sensorListFromNative[sensorHandle].vendor,
                        sensorListFromNative[sensorHandle].suidLow,
                        sensorListFromNative[sensorHandle].suidHigh,
                        attributeInfo, requestMsgsList);
                sensorsInfo.add(currentSensorInfo);
            }

            /*Consolidated rougue sensors */
            if( 0 != removeSensorCount) {
                int[] removeSensorHandleArray = new int[removeSensorCount];
                int removeSensorHandleArrayIndex = 0;
                for (int sensorHandle = 0 ; sensorHandle < sensorListFromNative.length ; sensorHandle ++) {
                    if(true == isSensorToBeRemoved[sensorHandle]) {
                        removeSensorHandleArray[removeSensorHandleArrayIndex] = sensorHandle;
                        removeSensorHandleArrayIndex++;
                    }
                }
                /*Delete rougue sensors From native now*/
                JavaNativeInterfaceAPIs.removeSensorsNativeWrapper(removeSensorHandleArray);
            }

            List<String> clientProcessors = Arrays.asList(JavaNativeInterfaceAPIs.getClientProcessorsNativeWrapper());
            List<String> wakeupDelivery = Arrays.asList(JavaNativeInterfaceAPIs.getWakeupDeliveryNativeWrapper());
            USTASensorInfo ustaSensorInfoObj = new USTASensorInfo(clientProcessors,
                    wakeupDelivery, sensorsInfo);
            return ustaSensorInfoObj;
        }

        @Override
        public SMNativeErrorCodes sendRequest(SMModeType ustaMode, int sensorsHandle, SMReqMsgPayload InReqMsg, SMClientReqMsgFields stdFieldInfo, String logFileName) throws RemoteException {
            ModeType mode = typeCasteUtility.SMModeTypeToModeType(ustaMode);
            SendReqStdFields sendReqstdFieldsInfo = typeCasteUtility.SMClientReqMsgFieldsToSendReqStdFields(stdFieldInfo);
            ReqMsgPayload reqMsg = typeCasteUtility.SMReqMsgPayloadToReqMsgPayload(InReqMsg);
            NativeErrorCodes err = JavaNativeInterfaceAPIs.sendRequestNativeWrapper(mode, sensorsHandle,reqMsg , sendReqstdFieldsInfo, logFileName);
            return typeCasteUtility.NativeErrorCodesToSMNativeErrorCodes(err);
        }

        @Override
        public void stopRequest(SMModeType ustaMode, int sensorsHandle, boolean isQmiDisable) throws RemoteException {
            ModeType mode = typeCasteUtility.SMModeTypeToModeType(ustaMode);
            JavaNativeInterfaceAPIs.stopRequestNativeWrapper(mode, sensorsHandle,isQmiDisable);
        }

        @Override
        public void updateLoggingFlag(boolean status) throws RemoteException {
            JavaNativeInterfaceAPIs.updateLoggingFlagWrapper(status);
        }

        @Override
        public void updateStreamingStatus(int sensorsHandle, boolean status) throws RemoteException {
            if(true == status) {
                JavaNativeInterfaceAPIs.enableStreamingStatusNativeWrapper(sensorsHandle);
            } else {
                JavaNativeInterfaceAPIs.disableStreamingStatusNativeWrapper(sensorsHandle);
            }
        }

        @Override
        public String getSamplesFromNative(boolean isRegistrySensor) throws RemoteException {
            return JavaNativeInterfaceAPIs.getSamplesFromNativeWrapper(isRegistrySensor);
        }

        @Override
        public void destroySensors(SMModeType ustaMode) throws RemoteException {
            ModeType mode = typeCasteUtility.SMModeTypeToModeType(ustaMode);
            JavaNativeInterfaceAPIs.destroySensorsWrapper(mode);
        }

        @Override
        public int deviceModeState(int number, int state) throws RemoteException {
            return JavaNativeInterfaceAPIs.sendDeviceModeIndicationWrapper(number,state);
        }

        @Override
        public void accelCalibration(boolean enabled) throws RemoteException {
            if(true == enabled) {
                JavaNativeInterfaceAPIs.startCalibrationWrapper();
            } else {
                JavaNativeInterfaceAPIs.stopCalibrationWrapper();
            }
        }

        @Override
        public String getCalSamples() throws RemoteException {
            return JavaNativeInterfaceAPIs.getAccelCalFromNativeWrapper();
        }

        @Override
        public int getchannelDRM(int bufferSize) throws RemoteException {
            return JavaNativeInterfaceAPIs.getchannelWrapper(bufferSize);
        }

        @Override
        public void startDRM(long[] sensorSuid, int samplePeriodUs, int flags, int streamHandle, int sensorHandle) throws RemoteException {
            JavaNativeInterfaceAPIs.startWrapper(sensorSuid,samplePeriodUs,flags,streamHandle, sensorHandle);
        }

        @Override
        public void stopDRM(int streamHandle) throws RemoteException {
            JavaNativeInterfaceAPIs.stopWrapper(streamHandle);
        }

        @Override
        public void closechannelDRM(int streamHandle) throws RemoteException {
            JavaNativeInterfaceAPIs.closechannelWrapper(streamHandle);
        }

        @Override
        public int getmaxrateDRM(long[] sensorSuid) throws RemoteException {
            return JavaNativeInterfaceAPIs.getmaxrateWrapper(sensorSuid);
        }

        @Override
        public void onDeviceLogging() throws RemoteException {

        }

        @Override
        public void RegisterClientDeathStatus(IBinder clientDeathListener) {

            ustaClientBinder = clientDeathListener;
            try {
                clientDeathListener.linkToDeath(new DeathRecipient() {
                    @Override
                    public void binderDied() {
                        JavaNativeInterfaceAPIs.destroySensorsWrapper(ModeType.USTA_MODE_TYPE_UI);
                        JavaNativeInterfaceAPIs.destroySensorsWrapper(ModeType.USTA_MODE_TYPE_COMMAND_LINE);
                        ustaClientBinder.unlinkToDeath(this, 0);
                    }
                }, 0);
            } catch (Exception e) {
                Log.i(TAG, "Exception while Link to Death call");
            }
        }
    }
    /*Class to implements all the Binder calls from App - End*/
}
