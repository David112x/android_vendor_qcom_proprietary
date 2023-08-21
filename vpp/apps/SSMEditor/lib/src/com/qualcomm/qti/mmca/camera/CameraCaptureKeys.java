/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file CameraCaptureKeys.java
 */
package com.qualcomm.qti.mmca.camera;

import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CaptureResult;

public class CameraCaptureKeys {
    // HFR
    public static CameraCharacteristics.Key<int[]> HIGH_SPEED_VIDEO_CONFIGURATIONS =
            new CameraCharacteristics.Key<>(
                    "android.control.availableHighSpeedVideoConfigurations", int[].class);

    // VPT
    public static CaptureRequest.Key<Integer> VPT_HFR_ENABLE =
            new CaptureRequest.Key<>("com.qti.chi.vpt.HighFrameRateEnable", Integer.class);

    public static CaptureResult.Key<Long> VPT_PROCESSED_FRAME_NUMBER =
            new CaptureResult.Key<>("com.qti.node.vpt.ProcessedFrameNumber", Long.class);

    public static CaptureResult.Key<byte[]> VPT_CLASSIFICATIONS =
            new CaptureResult.Key<>("com.qti.node.vpt.Classifications", byte[].class);
}
