/**
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file VPPPDecoder.java
 */

package com.qualcomm.qti.mmca.vppp;

import android.content.res.AssetFileDescriptor;
import android.media.MediaFormat;
import android.view.Surface;

import com.qualcomm.qti.mmca.decoder.MediaCodecParams;

import java.io.IOException;

import com.qualcomm.qti.ssmeditor.decoder.Decoder;
import com.qualcomm.qti.ssmeditor.decoder.Extractor;
import com.qualcomm.qti.ssmeditor.decoder.Renderer;

/**
 * VPP Player specific Decoder.
 */
public class VPPPDecoder {

    private static final String TAG = "VPPPDecoder";

    private Decoder mDecoder;
    private Renderer mRenderer;
    private Extractor mExtractor;

    public VPPPDecoder() {
        mRenderer = new Renderer();
        mDecoder = new Decoder();
    }

    public void configure(Surface surface, MediaCodecParams params) {
        mDecoder.configure(surface, params);
    }

    public void destroy() {
        mDecoder.destroy();
    }

    public boolean endSeek() {
        return mDecoder.endSeek();
    }

    public Decoder.State getCurrentState() {
        return mDecoder.getCurrentState();
    }

    public long getDuration() {
        return mDecoder.getDuration();
    }

    public MediaFormat getFormat() {
        return mDecoder.getFormat();
    }

    public long getLastRenderedFrameTimeUs() {
        return mDecoder.getLastRenderedFrameTimeUs();
    }

    public int getRotation() {
        return mDecoder.getRotation();
    }

    public int getVideoHeight() {
        return mDecoder.getVideoHeight();
    }

    public int getVideoWidth() {
        return mDecoder.getVideoWidth();
    }

    /**
     * @param savedSeekTime in microseconds
     */
    public void initialize(long savedSeekTime, AssetFileDescriptor dataSource)
            throws IOException, IllegalArgumentException {
        mExtractor = new Extractor();
        mExtractor.setDataSource(dataSource);
        mDecoder.initialize(savedSeekTime, mExtractor);
    }

    public boolean pause() {
        return mDecoder.pause();
    }

    public boolean play() {
        return mDecoder.play();
    }

    public void registerOnPlaybackChangedListener(Decoder.OnPlaybackChangedListener listener) {
        mDecoder.registerOnPlaybackChangedListener(listener);
    }

    public void unregisterOnPlaybackChangedListener(Decoder.OnPlaybackChangedListener listener) {
        mDecoder.unregisterOnPlaybackChangedListener(listener);
    }

    public boolean seek() {
        return mDecoder.seek();
    }

    public void setParams(MediaCodecParams params) {
        mDecoder.setParams(params);
    }

    public void start() {
        mDecoder.start(mRenderer);
    }

    public void updateProgress(int progressPercent) {
        mDecoder.updateProgress(progressPercent);
    }
}

