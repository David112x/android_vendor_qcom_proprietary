/* Extractor.java
 *
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.ssmeditor.decoder;

import android.content.res.AssetFileDescriptor;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.util.Log;

import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * Class with a base implementation of android.media.MediaExtractor.
 */
public class Extractor {

    private static final String TAG = "Extractor";

    private static final String VIDEO_MIME_TYPE = "video/";

    protected MediaExtractor mExtractor;
    protected int mFrameRate;
    protected int mBaseFrameRate;
    protected int mMaxLayerId;

    public Extractor() {
        create();
    }

    /**
     * Returns the size of the last frame processed. User must call advance.
     */
    public int processFrame(ByteBuffer byteBuff) {
        return readSampleData(byteBuff, 0);
    }

    public void advance() {
        mExtractor.advance();
    }

    public void create() {
        mExtractor = new MediaExtractor();
    }

    public int getBaseFrameRate() {
        return mBaseFrameRate;
    }

    public int getFrameRate() {
        return mFrameRate;
    }

    public int getSampleFlags() {
        return mExtractor.getSampleFlags();
    }

    public long getSampleSize() {
        return mExtractor.getSampleSize();
    }

    public long getSampleTime() {
        return mExtractor.getSampleTime();
    }

    public int getSampleTrackIndex() {
        return mExtractor.getSampleTrackIndex();
    }

    /**
     * Get the format of the video track in the input file.
     */
    public MediaFormat getVideoFormat() throws IllegalArgumentException {
        int trackIndex = getSampleTrackIndex();

        if (trackIndex == -1) {
            trackIndex = selectVideoTrack();
        }

        return mExtractor.getTrackFormat(trackIndex);
    }

    public int readSampleData(ByteBuffer byteBuff, int offset) {
        return mExtractor.readSampleData(byteBuff, offset);
    }

    public void release() {
        mExtractor.release();
        mExtractor = null;
    }

    public void seekTo(long timeUs, int mode) {
        mExtractor.seekTo(timeUs, mode);
    }

    /**
     * Throws IllegalArgumentException if the source is invalid.
     */
    public void setDataSource(AssetFileDescriptor dataSource)
        throws IOException, IllegalArgumentException {
        mExtractor.setDataSource(dataSource);
        selectVideoTrack();

        mFrameRate = calculateFrameRate();
        mMaxLayerId = 1;
        mBaseFrameRate = 1;

        seekToBeginning();
    }

    /**
     * Selects the first video track from the data source (extractor ignores rest) and returns
     * its index.
     */
    protected int selectVideoTrack() throws IllegalArgumentException {
        // Select the first found video track and ignore rest
        for (int i = 0; i < mExtractor.getTrackCount(); i++) {
            MediaFormat format = mExtractor.getTrackFormat(i);
            String mime = format.getString(MediaFormat.KEY_MIME);
            if (mime.startsWith(VIDEO_MIME_TYPE)) {
                // Select the track
                mExtractor.selectTrack(i);
                Log.v(TAG, "Selected video track format: " + format);
                return i;
            }
        }

        throw new IllegalArgumentException("Data source does not contain a valid video track"
                + " with format: " + VIDEO_MIME_TYPE);
    }

    protected void seekToBeginning() {
        seekTo(0, MediaExtractor.SEEK_TO_PREVIOUS_SYNC);
    }

    protected int calculateFrameRate() throws IllegalArgumentException {
        int frameRate = 1;

        MediaFormat videoFormat = getVideoFormat();
        Log.v(TAG, "Selected video track format: " + videoFormat);

        if (videoFormat.containsKey(MediaFormat.KEY_FRAME_RATE)) {
            frameRate = videoFormat.getInteger(MediaFormat.KEY_FRAME_RATE);
        }

        Log.v(TAG, "frame rate: " + frameRate);

        return frameRate;
    }
}
