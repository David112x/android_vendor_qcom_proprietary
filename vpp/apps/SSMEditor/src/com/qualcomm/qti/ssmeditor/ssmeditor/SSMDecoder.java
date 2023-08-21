/**
 * Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file SSMDecoder.java
 */

package com.qualcomm.qti.ssmeditor.ssmeditor;

import android.content.res.AssetFileDescriptor;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.os.Build;
import android.os.Bundle;
import android.view.Surface;

import com.qualcomm.qti.mmca.ssmeditor.FRCSegment;
import com.qualcomm.qti.mmca.ssmeditor.FRCSegmentList;
import com.qualcomm.qti.ssmeditor.decoder.Decoder;
import com.qualcomm.qti.mmca.decoder.MediaCodecParams;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * SSME specific Decoder
 */
public class SSMDecoder implements FRCSegmentList.OnSegmentsChangedListener<FRCSegment> {

    private Decoder mDecoder;
    private SSMRenderer mRenderer;
    private SSMExtractor mExtractor;
    private FRCSegmentList mSegmentList;
    private boolean mCanUseVPP;

    public SSMDecoder(FRCSegmentList segmentList) {
        mSegmentList = segmentList;
        mDecoder = new Decoder();
        mRenderer = new SSMDisplayRenderer();
    }

    /**
     * start() must be called after this in order to properly encode.
     */
    public void attachEncoder(MediaCodec encoder) {
        mRenderer = new SSMEncodeRenderer(encoder);
        mRenderer.setSegments(mSegmentList.getSegments());
    }

    public void configure(Surface surface) {
        mDecoder.configure(surface, getFormatParams());
    }

    public void destroy() {
        mSegmentList.unregisterOnSegmentsChangedListener(this);
        mDecoder.destroy(); // releases Extractor as well
    }

    public boolean endSeek() {
        return mDecoder.endSeek();
    }

    public FRCSegment.Interp getCurrentInterpFactor() {
        return mSegmentList.getInterpFactorAtTime(mDecoder.getLastRenderedFrameTimeUs());
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

    public int alignFrameRate(int target, int threshold, int actual) {
        if (actual >= (target - threshold) && actual < (target + threshold)) {
            return target;
        }
        return actual;
    }

    public int getSlowdownMultiple() {
        int fr = mExtractor.getFrameRate();
        fr = alignFrameRate(30, 5, fr);
        fr = alignFrameRate(60, 5, fr);
        fr = alignFrameRate(120, 10, fr);
        fr = alignFrameRate(240, 10, fr);
        fr = alignFrameRate(480, 15, fr);
        fr = alignFrameRate(960, 15, fr);
        return fr / 30;
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

    public boolean has4xInterp() {
        return mCanUseVPP && (Build.DEVICE.equals("msmnile") ||
                Build.DEVICE.equals("kona") ||
                Build.DEVICE.equals("sm7150") ||
                Build.DEVICE.equals("sm6150"));
    }

    /**
     * @param savedSeekTime in microseconds
     */
    public void initialize(long savedSeekTime, AssetFileDescriptor dataSource)
        throws IOException, IllegalArgumentException {
        mExtractor = new SSMExtractor();
        mExtractor.setDataSource(dataSource);
        mCanUseVPP = mExtractor.canUseVpp();

        List<FRCSegment> segments = mSegmentList.getSegments();
        mExtractor.setSegments(segments);
        mRenderer.setSegments(segments);

        mSegmentList.registerOnSegmentsChangedListener(this);
        mDecoder.initialize(savedSeekTime, mExtractor);
    }

    public boolean isRenderingSlowMotion() {
        return mSegmentList.isInSlowZone(getLastRenderedFrameTimeUs());
    }

    // TODO create an isEncoding flag in the Activity to manage this state.
    public boolean isEncoding() {
        return mRenderer instanceof SSMEncodeRenderer;
    }

    public boolean isUsingVPP() {
        return mCanUseVPP;
    }


    @Override
    public void onSegmentsChanged(List<? extends FRCSegment> segments) {
        List<FRCSegment> sl = new ArrayList<>(segments);
        mExtractor.setSegments(sl);
        mRenderer.setSegments(sl);
        updateDecoderSegments();
    }

    public boolean pause() {
        return mDecoder.pause();
    }

    public boolean play() {
        return mDecoder.play();
    }

    public boolean seek() {
        return mDecoder.seek();
    }

    public void start() {
        mDecoder.start(mRenderer);
        updateDecoderSegments();
    }

    public void updateProgress(int progressPercent) {
        mDecoder.updateProgress(progressPercent);
    }

    public void registerOnPlaybackChangedListener(Decoder.OnPlaybackChangedListener listener) {
        mDecoder.registerOnPlaybackChangedListener(listener);
    }

    public void unregisterOnPlaybackChangedListener(Decoder.OnPlaybackChangedListener listener) {
        mDecoder.unregisterOnPlaybackChangedListener(listener);
    }

    /**
     * This class overrides the default VPP_FRC_FRM_CPY_IN parameters which are
     * set by the FRCSegment. This is necessary in order to workaround an issue
     * with VPP's output port being starved in the case where the sink is a
     * video encoder. The reason is that when VPP_FRM_CPY_IN is false, all of
     * the video buffers in circulation can be queued to the encoder during a
     * bypass segment.  When we encounter a FRC segment at some point in time
     * later, all of the buffers are held by the encoder and VPP's output port
     * becomes starved. This causes the decode pipeline to maintain separate
     * buffer pools between the decoder and vpp, and vpp and the sink. This is
     * implemented here as a workaround because organizationally, an FRCSegment
     * shouldn't care what it is being used for.
     */
    private class OverrideFrameCopyInputParams implements MediaCodecParams {

        private MediaCodecParams mParams;

        OverrideFrameCopyInputParams(MediaCodecParams params) {
            mParams = params;
        }

        @Override
        public MediaFormat pack(MediaFormat mediaFormat) {
            mediaFormat = mParams.pack(mediaFormat);
            if (isEncoding()) {
                mediaFormat.setInteger(MediaCodecParams.VPP_FRC_FRM_CPY_IN, 1);
            }
            return mediaFormat;
        }

        @Override
        public Bundle pack(Bundle bundle) {
            bundle = mParams.pack(bundle);
            if (isEncoding()) {
                bundle.putInt(MediaCodecParams.VPP_FRC_FRM_CPY_IN, 1);
            }
            return bundle;
        }
    }

    private void updateDecoderSegments() {
        // FRC extensions
        if (mDecoder.getCurrentState() != Decoder.State.UNINITIALIZED) {
            mDecoder.setParams(getResetParams());

            for (FRCSegment segment : mSegmentList.getSegments()) {
                mDecoder.setParams(new OverrideFrameCopyInputParams(segment));
            }
        }
    }

    private MediaCodecParams getFormatParams() {
        return new MediaCodecParams() {
            @Override
            public MediaFormat pack(MediaFormat mediaFormat) {
                if (mCanUseVPP) {
                    mediaFormat.setString(MediaCodecParams.VPP_MODE, "HQV_MODE_MANUAL");
                    mediaFormat.setInteger(MediaCodecParams.VPP_FRC_TS_START, 0);
                    mediaFormat.setInteger(MediaCodecParams.VPP_FRC_FRM_CPY_FB, 1);
                    mediaFormat.setInteger(MediaCodecParams.VPP_FRC_FRM_CPY_IN, 0);
                    mediaFormat.setString(MediaCodecParams.VPP_FRC_MODE, "FRC_MODE_SLOMO");
                    mediaFormat.setString(MediaCodecParams.VPP_FRC_LVL, "FRC_LEVEL_HIGH");
                    mediaFormat.setString(MediaCodecParams.VPP_FRC_INTERP, "FRC_INTERP_1X");
                    mediaFormat.setInteger(MediaCodecParams.VPP_NONREALTIME_ENABLE, 1);
                }
                return mediaFormat;
            }

            @Override
            public Bundle pack(Bundle bundle) {
                return bundle;
            }
        };
    }

    private MediaCodecParams getResetParams() {
        return  new MediaCodecParams() {
            @Override
            public MediaFormat pack(MediaFormat mediaFormat) {
                return mediaFormat;
            }

            @Override
            public Bundle pack(Bundle bundle) {
                if (mCanUseVPP) {
                    bundle.putString(MediaCodecParams.VPP_MODE, "HQV_MODE_MANUAL");
                    bundle.putInt(MediaCodecParams.VPP_FRC_TS_START, -1);
                    bundle.putInt(MediaCodecParams.VPP_FRC_FRM_CPY_FB, 1);
                    bundle.putInt(MediaCodecParams.VPP_FRC_FRM_CPY_IN, 0);
                    bundle.putString(MediaCodecParams.VPP_FRC_MODE, "FRC_MODE_SLOMO");
                    bundle.putString(MediaCodecParams.VPP_FRC_LVL, "FRC_LEVEL_HIGH");
                    bundle.putString(MediaCodecParams.VPP_FRC_INTERP, "FRC_INTERP_1X");
                }

                return bundle;
            }
        };
    }
}
