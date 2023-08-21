/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file HFRConfiguration.java
 */
package com.qualcomm.qti.mmca.camera;

public class HFRConfiguration {
    public int width;
    public int height;
    public int frameRate;
    public int batchSize;
    public String cameraId;
    public int sensorOrientation;
    public boolean isRearCamera;

    HFRConfiguration(String id, int w, int h, int fps, int batchSz) {
        cameraId = id;
        width = w;
        height = h;
        frameRate = fps;
        batchSize = batchSz;
    }
}
