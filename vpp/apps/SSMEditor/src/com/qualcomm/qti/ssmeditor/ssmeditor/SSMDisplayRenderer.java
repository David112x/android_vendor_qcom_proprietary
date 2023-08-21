/* SSMDisplayRenderer.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.ssmeditor.ssmeditor;

import android.media.MediaCodec;
import android.util.Log;

import com.qualcomm.qti.ssmeditor.decoder.Decoder;

/**
 * Renderer for SSME to display to surface.
 */
public class SSMDisplayRenderer extends SSMRenderer {

    private static final String TAG = "SSMDisplayRenderer";

    private boolean mShouldResetClock; // whether or not to reset the clock

    public SSMDisplayRenderer() {
        super();

        mShouldResetClock = true;
    }

    @Override
    public void onOutputBufferAvailable(MediaCodec codec, int index,
            MediaCodec.BufferInfo bufferInfo, Decoder decoder) {
        Decoder.State state = decoder.getCurrentState();

        if (bufferInfo.flags == MediaCodec.BUFFER_FLAG_END_OF_STREAM) {
            codec.releaseOutputBuffer(index, false); // do not render to surface
        } else {
            if (mShouldResetClock) {
                resetClock();
                mShouldResetClock = false;
            }

            mTimestamp += calculateTimestampInterval(bufferInfo.presentationTimeUs,
                    decoder.getFrameRate(), decoder.getBaseFrameRate());

            Log.v(TAG, "Display frame = " + index
                    + " at " + mTimestamp
                    + " system time = " + System.nanoTime()
                    + " difference = " + (mTimestamp - System.nanoTime())
                    + " timestamp time = " + bufferInfo.presentationTimeUs * 1000);

            if (state == Decoder.State.SEEKING) {
                codec.releaseOutputBuffer(index, true);
            } else {
                Log.v(TAG, "releasing output buffer #" + index
                        + " at time: " + mTimestamp);
                codec.releaseOutputBuffer(index, mTimestamp);
            }
        }

        notifyListener(bufferInfo.presentationTimeUs);
    }

    @Override
    public void resumePlayback() {
        mShouldResetClock = true;
    }

    private void resetClock() {
        mTimestamp = System.nanoTime();
    }
}
