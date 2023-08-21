/*============================================================================
  Copyright (c) 2017, 2019 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

  @file AccelCalibrationJNI.java
  @brief
   AccelCalibrationJNI class implementation
============================================================================*/
package com.qualcomm.qti.usta.core;

import android.os.Build;

public class AccelCalibrationJNI {

  public static void startCalibrationWrapper() {
    if (!Build.MODEL.contains("Android SDK")) {
      USTAServiceUtil.startCalibration();
    }
    else{
      USTALog.i("startCalibrationWrapper - Stub");
    }
  }
  public static void stopCalibrationWrapper(){
    if (!Build.MODEL.contains("Android SDK")) {
      USTAServiceUtil.stopCalibration();
    }
    else{
      USTALog.i("stopCalibrationWrapper - Stub");
    }
  }

  public static String getAccelCalFromNativeWrapper() {
    if (!Build.MODEL.contains("Android SDK")) {
      return USTAServiceUtil.getAccelCalFromNative();
    } else {
      return "getAccelCalFromNativeWrapper - TestStub ";
    }
  }
}
