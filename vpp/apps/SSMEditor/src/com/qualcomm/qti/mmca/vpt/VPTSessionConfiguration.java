/**
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file VPTSessionConfiguration.java
 */
package com.qualcomm.qti.mmca.vpt;

import android.content.Context;
import android.media.MediaRecorder;

import com.qualcomm.qti.mmca.camera.HFRConfiguration;
import com.qualcomm.qti.mmca.util.FileSystem;
import com.qualcomm.qti.mmca.util.VideoFile;

import java.io.File;

public class VPTSessionConfiguration {

    // Android specific
    Context context;

    // Device settings
    public int deviceOrientation;
    public int displayOrientation;

    // Common settings
    public int width;
    public int height;
    public String testName;
    private String dateTimeString;

    // Video settings
    public int videoRate;
    public int videoEncoder = MediaRecorder.VideoEncoder.H264;

    // Camera specific settings
    public String cameraId;
    public int batchSize;
    public int captureRate;
    public int sensorOrientation;
    public boolean isRearCamera;

    // VPT
    public String vptMetadataFilePath;
    public String vptPostProcFilePath;
    private VPTVideoFile mVideoFile = null;

    public VPTSessionConfiguration(Context ctx, HFRConfiguration hfrConfig) {
        this(ctx, "", hfrConfig);
    }

    public VPTSessionConfiguration(Context ctx, String name, HFRConfiguration hfrConfig) {
        width = hfrConfig.width;
        height = hfrConfig.height;
        batchSize = hfrConfig.batchSize;
        captureRate = hfrConfig.frameRate;
        videoRate = hfrConfig.frameRate;
        sensorOrientation = hfrConfig.sensorOrientation;
        isRearCamera = hfrConfig.isRearCamera;

        cameraId = hfrConfig.cameraId;
        context = ctx;
        testName = name;
        dateTimeString = FileSystem.getDateTimeString();
        vptMetadataFilePath = getMetadataFileName();
        vptPostProcFilePath = getPostProcFileName();
    }

    public String getFilenameNoExtension() {
        String filename = String.format("%s_%s_%dx%d_%dfps_%s",
                testName, dateTimeString, width, height, captureRate, cameraId);
        return filename;
    }

    private String getVideoFileName() {
        return getFilenameNoExtension() + VPTFileSystem.EXTN_MP4;
    }

    private String getMetadataFileName() {
        String filename = getFilenameNoExtension() + VPTFileSystem.EXTN_JSON;
        File f = new File(VPTFileSystem.getStorageDirectory(context), filename);
        return f.getAbsolutePath();
    }

    private String getPostProcFileName() {
        String filename = getFilenameNoExtension() + VPTFileSystem.EXTN_PP_JSON;
        File f = new File(VPTFileSystem.getStorageDirectory(context), filename);
        return f.getAbsolutePath();
    }

    private class VPTVideoFile extends VideoFile {
        @Override
        public String getRelativePath() {
            return getCameraSaveRootDir() + "/VPT";
        }
    }

    VideoFile getVideoFile() {
        if (mVideoFile == null) {
            mVideoFile = new VPTVideoFile();
            mVideoFile.setContext(context);
            mVideoFile.setDisplayName(getVideoFileName());
        }
        return mVideoFile;
    }

    @Override
    public String toString() {
        return String.format("cameraId=%s w=%d, h=%d, captureRate=%d, batchSize=%d",
                cameraId, width, height, captureRate, batchSize);
    }
}
