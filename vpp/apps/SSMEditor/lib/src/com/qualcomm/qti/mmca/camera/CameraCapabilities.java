/**
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file CameraCapabilities.java
 */
package com.qualcomm.qti.mmca.camera;

import android.content.Context;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CameraMetadata;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.util.Log;
import android.util.Range;
import android.util.Size;

import java.util.ArrayList;
import java.util.List;

public class CameraCapabilities {

    private static final String TAG = "CameraCapabilities";

    public static List<HFRConfiguration> getHFRConfigurations(Context context) {
        List<HFRConfiguration> configurations = new ArrayList<>();

        CameraManager manager = (CameraManager)context.getSystemService(Context.CAMERA_SERVICE);
        try {
            String[] cameraIds = manager.getCameraIdList();
            for (String id : cameraIds) {
                CameraCharacteristics characteristics = manager.getCameraCharacteristics(id);
                String info = characteristics.get(CameraCharacteristics.INFO_VERSION);
                Log.d(TAG, "Camera: " + info);

                Integer orientation = characteristics.get(CameraCharacteristics.SENSOR_ORIENTATION);
                Integer lensFacing = characteristics.get(CameraCharacteristics.LENS_FACING);
                Log.d(TAG, "Orientation=" + orientation + ", lensFacing=" + lensFacing);

                StreamConfigurationMap scm = characteristics.get(
                        CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
                Log.d(TAG, "StreamConfigurationMap: HFR Sizes:");
                for (Size sz : scm.getHighSpeedVideoSizes()) {
                    for (Range<Integer> fps : scm.getHighSpeedVideoFpsRangesFor(sz)) {
                        Log.d(TAG, "    " + sz.getWidth() + "x" + sz.getHeight() + ": " +
                                fps.getLower() + " to " + fps.getUpper());
                    }
                }

                Range<Integer>[] ranges = characteristics.get(
                        CameraCharacteristics.CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES);
                for (Range<Integer> range : ranges) {
                    Log.d(TAG, "CameraCharacteristics.CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES: " +
                            range.getLower() + " to " + range.getUpper());
                }

                int[] hfrConfigs = characteristics.get(
                        CameraCaptureKeys.HIGH_SPEED_VIDEO_CONFIGURATIONS);
                if (hfrConfigs == null) {
                    Log.d(TAG, "Camera " + id + " doesn't define any HFR configurations. Skipping.");
                    continue;
                }

                for (int i = 0; i < hfrConfigs.length; i += 5) {
                    int w = hfrConfigs[i];
                    int h = hfrConfigs[i + 1];
                    int fps1 = hfrConfigs[i + 2];
                    int fps2 = hfrConfigs[i + 3];
                    int batch = hfrConfigs[i + 4];

                    String valid = "invalid";
                    if (fps1 == fps2 && fps1 >= 120) {
                        HFRConfiguration cfg = new HFRConfiguration(id, w, h, fps1, batch);
                        cfg.sensorOrientation = orientation;
                        cfg.isRearCamera = lensFacing == CameraMetadata.LENS_FACING_BACK;
                        configurations.add(cfg);
                        valid = "valid";
                    }
                    Log.d(TAG, String.format("Camera [%s] -> %d x %d - %d/%d - %d (%s)",
                            id, w, h, fps1, fps2, batch, valid));
                }
            }
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }

        return configurations;
    }
}
