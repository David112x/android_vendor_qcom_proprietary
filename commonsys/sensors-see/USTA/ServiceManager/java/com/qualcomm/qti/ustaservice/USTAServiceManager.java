/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.ustaservice;

import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;
import java.util.ArrayList;
import java.util.List;

public class USTAServiceManager {
    private static final String TAG = "USTA_ServiceManager";
    private static final String REMOTE_USTA_SERVICE_NAME = "com.qualcomm.qti.ustaservice.USTAServiceImpl";
    private static USTAServiceManager mUSTAServiceManagerInstance = null;
    private IUSTAService mUSTAServiceInstance = null;
    private Context mContext;

    public static synchronized USTAServiceManager getInstance(Context context) {
        if(null == mUSTAServiceManagerInstance) {
            mUSTAServiceManagerInstance = new USTAServiceManager(context);
        }
        return mUSTAServiceManagerInstance;
    }

    /*Wrappers exposed to APPS to communicate with AIDL */
    public USTASensorInfo getSensorsInformation() {
        connetUSTAService();
        if(null != mUSTAServiceInstance) {
            try {
                return mUSTAServiceInstance.Initialize();
            } catch (RemoteException e) {
                Log.i(TAG, e.toString());
                List<String> clientProcessorTypes = new ArrayList<String>();
                List<String> wakeupDeliveryTypes = new ArrayList<String>();;
                List<SMSensorInfo> SensorsInfo = new ArrayList<SMSensorInfo>();
                return new USTASensorInfo(clientProcessorTypes,wakeupDeliveryTypes,SensorsInfo);
            }
        } else {
            /*service is not connected. So return NULL*/
            List<String> clientProcessorTypes = new ArrayList<String>();
            List<String> wakeupDeliveryTypes = new ArrayList<String>();;
            List<SMSensorInfo> SensorsInfo = new ArrayList<SMSensorInfo>();
            return new USTASensorInfo(clientProcessorTypes,wakeupDeliveryTypes,SensorsInfo);
        }
    }

    public SMNativeErrorCodes sendRequest(SMModeType ustaMode,
                            int sensorsHandle,
                            SMReqMsgPayload reqMsg,
                            SMClientReqMsgFields stdFieldInfo,
                            String logFileName) {
        if(null != mUSTAServiceInstance){
            try {
                return mUSTAServiceInstance.sendRequest(
                        ustaMode,sensorsHandle,reqMsg,stdFieldInfo,logFileName
                );
            }catch (Exception e) {
                Log.i(TAG, e.toString());
                return SMNativeErrorCodes.USTA_MEMORY_ERROR;
            }
        } else {
            return SMNativeErrorCodes.USTA_MEMORY_ERROR;
        }
    }

    public void stopRequest(SMModeType ustaMode,
                     int sensorsHandle,
                     boolean isQmiDisable) {
        if(null != mUSTAServiceInstance){
            try {
                mUSTAServiceInstance.stopRequest(ustaMode,sensorsHandle,isQmiDisable);
            }catch (Exception e) {
                Log.i(TAG, e.toString());
            }
        }
    }

    public void removeSensors(SMModeType ustaMode) {
        if(null != mUSTAServiceInstance){
            try {
                mUSTAServiceInstance.destroySensors(ustaMode);
            }catch (Exception e) {
                Log.i(TAG, e.toString());
            }
        }
    }

    public void updateLoggingFlag(boolean status){
        if(null != mUSTAServiceInstance){
            try {
                mUSTAServiceInstance.updateLoggingFlag(status);
            }catch (Exception e) {
                Log.i(TAG, e.toString());
            }
        }
    }

    public void updateStreamingStatus(int sensorsHandle, boolean status) {
        if(null != mUSTAServiceInstance){
            try {
                mUSTAServiceInstance.updateStreamingStatus(sensorsHandle,status);
            }catch (Exception e) {
                Log.i(TAG, e.toString());
            }
        }
    }

    public String getSamplesFromNative(boolean isRegistrySensor){
        if(null != mUSTAServiceInstance){
            try {
                return mUSTAServiceInstance.getSamplesFromNative(isRegistrySensor);
            }catch (Exception e) {
                Log.i(TAG, e.toString());
                return null;
            }
        } else {
            return null;
        }
    }

    /************************** DEVICE MODE TAB FUNCTIONALITY ******************************/
    public int deviceModeState(int number,int state){
        if(null != mUSTAServiceInstance){
            try {
                return mUSTAServiceInstance.deviceModeState(number,state);
            }catch (Exception e) {
                Log.i(TAG, e.toString());
                return -1;
            }
        } else {
            return -1;
        }
    }

    /************************** ACEL CAL TAB FUNCTIONALITY ******************************/
    public void accelCalibration(boolean enabled){
        if(null != mUSTAServiceInstance){
            try {
                mUSTAServiceInstance.accelCalibration(enabled);
            }catch (Exception e) {
                Log.i(TAG, e.toString());
            }
        }
    };

    public String getCalSamples(){
        if(null != mUSTAServiceInstance){
            try {
                return mUSTAServiceInstance.getCalSamples();
            }catch (Exception e) {
                Log.i(TAG, e.toString());
                return null;
            }
        } else {
            return null;
        }
    };

    /************************** DRM TAB FUNCTIONALITY ******************************/
    public int getchannelDRM(int bufferSize) {

        if(null != mUSTAServiceInstance){
            try {
                return mUSTAServiceInstance.getchannelDRM(bufferSize);
            }catch (Exception e ) {
                Log.e(TAG, e.getMessage());
                return -1;
            }
        } else {
            return -1;
        }
    }

    public void startDRM(long[] sensorSuid, int samplePeriodUs, int flags, int streamHandle, int sensorHandle) {
        if(null != mUSTAServiceInstance){
            try {
                mUSTAServiceInstance.startDRM(sensorSuid, samplePeriodUs, flags, streamHandle, sensorHandle);
            } catch (Exception e ) {
                Log.e(TAG,e.getMessage());
            }
        }


    }

    public void stopDRM(int streamHandle) {
        if(null != mUSTAServiceInstance){
            try {
                mUSTAServiceInstance.stopDRM(streamHandle);
            }catch (Exception e ) {
                Log.e(TAG,e.getMessage());
            }
        }

    }

    public void closechannelDRM(int streamHandle){
        if(null != mUSTAServiceInstance){
            try {
                mUSTAServiceInstance.closechannelDRM(streamHandle);
            }catch (Exception e ) {
                Log.e(TAG,e.getMessage());
            }
        }
    }

    public int getmaxrateDRM(long[] sensorSuid) {
        if(null != mUSTAServiceInstance){
            try {
                return mUSTAServiceInstance.getmaxrateDRM(sensorSuid);
            }catch (Exception e) {
                Log.e(TAG,e.getMessage());
                return -1;
            }
        } else {
            return -1;
        }
    }


    private USTAServiceManager(Context context) {
        mContext = context;
    }

    private void connetUSTAService() {
        if(null == mUSTAServiceInstance) {
            Log.i(TAG, "Connecting to Service " + REMOTE_USTA_SERVICE_NAME + " ");
            if(mContext.getPackageManager().resolveService(new Intent(REMOTE_USTA_SERVICE_NAME), 0) == null) {
                Log.e(TAG, REMOTE_USTA_SERVICE_NAME + " is not installed ");
            }
            if(ServiceManager.getService(REMOTE_USTA_SERVICE_NAME) == null) {
                Log.e(TAG, REMOTE_USTA_SERVICE_NAME + " is not running");
            }
            mUSTAServiceInstance = IUSTAService.Stub.asInterface(ServiceManager.getService(REMOTE_USTA_SERVICE_NAME));
            if(null == mUSTAServiceInstance) {
                Log.e(TAG, "Not able to get service " + REMOTE_USTA_SERVICE_NAME);
            }
            try {
                mUSTAServiceInstance.RegisterClientDeathStatus(new Binder());
            } catch ( Exception e ) {
                Log.i(TAG, "Exception received " + e.toString());
            }
        }
    }

}
