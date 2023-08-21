/*============================================================================
  Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

  @file SensorLowLatencyJNI.java
  @brief
   SensorLowLatencyJNI class implementation
============================================================================*/
package com.qualcomm.qti.usta.core;

import android.os.Build;
import android.hardware.SensorDirectChannel;

public class SensorLowLatencyJNI {

  private static final int defaultCircBufferSize = 0x1000;
  public static final int defaultSamplePeriodUs = 2500;

  private static int supportedRates;
  private static final int SNS_DIRECT_RATE_NORMAL_SAMPLE_US    = 20000;
  private static final int SNS_DIRECT_RATE_FAST_SAMPLE_US      = 5000;
  private static final int SNS_DIRECT_RATE_VERY_FAST_SAMPLE_US = 1250;

  private static final float RATE_NORMAL_NOMINAL = 50;
  private static final float RATE_FAST_NOMINAL = 200;
  private static final float RATE_VERY_FAST_NOMINAL = 800;

  private static final int SNS_LOW_LAT_CHANNEL_TYPE_NOT_INIT = -1;
  private static final int SNS_LOW_LAT_CHANNEL_TYPE_ANDROID = 0x0;
  private static final int SNS_LOW_LAT_CHANNEL_TYPE_GENERIC = 0x1;
  private static final int SNS_LOW_LAT_FLAG_ANDROID_STYLE_CHANNEL = 0x2;

  public static int lowLatEnable(long[] sensorSuid, int samplePeriodUs, int flags, int channel_type, int sensor_handle) {
    int streamHandle = -1;
    if (!Build.MODEL.contains("Android SDK")) {
      streamHandle = lowLatGetChannel(defaultCircBufferSize);
      if (streamHandle == -1) {
        USTALog.e("lowLatGetChannel  streamHandle is not proper. ");
        return -1;
      }
      if (SNS_LOW_LAT_CHANNEL_TYPE_NOT_INIT != channel_type) {
        if (SNS_LOW_LAT_CHANNEL_TYPE_GENERIC == channel_type) {
          flags &= ~SNS_LOW_LAT_FLAG_ANDROID_STYLE_CHANNEL;
        } else if (SNS_LOW_LAT_CHANNEL_TYPE_ANDROID == channel_type){
          flags |= SNS_LOW_LAT_FLAG_ANDROID_STYLE_CHANNEL;
        }
      }
      if (false == lowLatStart(streamHandle, sensorSuid, samplePeriodUs, flags, sensor_handle)) {
        USTALog.e("lowLatGetChannel  lowLatStart Error. ");
        USTAServiceUtil.closechannel(streamHandle);
        return  -1;
      }
    } else {
      USTALog.i("SensorLowLatencyJNI - lowLatEnable");
    }
    return streamHandle;
  }

  public static boolean lowLatDisable(int streamHandle) {
    if (!Build.MODEL.contains("Android SDK")) {
      if(false == lowLatStop(streamHandle)){
        return false;
      }
      if(false == lowLatCloseChannel(streamHandle)){
        return false;
      }
      USTALog.i("SensorLowLatencyJNI - lowLatDisable");
    } else {
      USTALog.i("SensorLowLatencyJNI - lowLatDisable");
    }
    return true;
  }

  public static int lowLatGetChannel(int bufferSize) {
    int streamHandle = -1;
    if (!Build.MODEL.contains("Android SDK")) {
      try {
        streamHandle = USTAServiceUtil.getchannel(bufferSize);
      } catch (Exception e) {
        USTALog.e(e.getMessage());
        return -1;
      }
      USTALog.i("SensorLowLatencyJNI - getchannel is  " + streamHandle);
    } else {
      USTALog.i("SensorLowLatencyJNI - lowLatInit");
    }
    return streamHandle;
  }

  public static boolean lowLatStart(int streamHandle, long[] sensorSuid, int samplePeriodUs, int flags, int sensorHandle) {
    if (!Build.MODEL.contains("Android SDK")) {
      try {
        USTAServiceUtil.start(sensorSuid, samplePeriodUs, flags, streamHandle, sensorHandle);
      } catch (Exception e) {
        USTALog.e(e.getMessage());
        e.printStackTrace();
        return false;
      }
      USTALog.i("SensorLowLatencyJNI - lowLatStart with stream handle " + streamHandle);
      USTALog.i("SensorLowLatencyJNI - lowLatStart with sensor handle " + sensorHandle);
      USTALog.i("SensorLowLatencyJNI - lowLatStart with suid low "
              + sensorSuid[0]+ "suid high" + sensorSuid[1]);
      USTALog.i("SensorLowLatencyJNI - lowLatStart with samplePeriodUs " + samplePeriodUs);
      USTALog.i("SensorLowLatencyJNI - lowLatStart with flags " + flags);
      return true;
    } else {
      USTALog.i("SensorLowLatencyJNI - lowLatStart");
      return true;
    }
  }

  public static boolean lowLatStop(int streamHandle) {
    if (!Build.MODEL.contains("Android SDK")) {
      try {
        USTAServiceUtil.stop(streamHandle);
      }catch (Exception e){
        USTALog.e(e.getMessage());
        e.printStackTrace();
        return false;
      }
      USTALog.i("SensorLowLatencyJNI - lowLatStop");
    } else {
      USTALog.i("SensorLowLatencyJNI - lowLatStop");
    }
    return true;
  }

  public static boolean lowLatCloseChannel(int streamHandle) {
    if (!Build.MODEL.contains("Android SDK")) {
      try{
        USTAServiceUtil.closechannel(streamHandle);
      }catch (Exception e){
        USTALog.e(e.getMessage());
        e.printStackTrace();
        return false;
      }
      USTALog.i("SensorLowLatencyJNI - lowLatCloseChannel");
    } else {
      USTALog.i("SensorLowLatencyJNI - lowLatCloseChannel");
    }
    return true;
  }

  public static int lowLatChannelSampleRate(int rate_level) {
    int sample_period_us = -1;
    switch(rate_level) {
      case SensorDirectChannel.RATE_NORMAL:
        sample_period_us = SNS_DIRECT_RATE_NORMAL_SAMPLE_US;
        break;
      case SensorDirectChannel.RATE_FAST:
        sample_period_us = SNS_DIRECT_RATE_FAST_SAMPLE_US;
        break;
      case SensorDirectChannel.RATE_VERY_FAST:
        sample_period_us = SNS_DIRECT_RATE_VERY_FAST_SAMPLE_US;
        break;
      case SensorDirectChannel.RATE_STOP:
        sample_period_us = 0;
        break;
      default:
        USTALog.e("SensorLowLatencyJNI - unsupported sample rate");
        break;
    }
    USTALog.i("lowLatChannelSampleRate samplerate selected " + sample_period_us);
    return sample_period_us;
  }

  public static int lowLatGetMaxRate(long[] suid) {
    int low_lat_rates =  USTAServiceUtil.getmaxrate(suid);
    if ( RATE_VERY_FAST_NOMINAL < low_lat_rates )
      return SensorDirectChannel.RATE_VERY_FAST;
    else if ( RATE_FAST_NOMINAL < low_lat_rates )
      return SensorDirectChannel.RATE_FAST;
    else if ( RATE_NORMAL_NOMINAL < low_lat_rates )
      return SensorDirectChannel.RATE_NORMAL;
    else
      return SensorDirectChannel.RATE_STOP;
  }

}
