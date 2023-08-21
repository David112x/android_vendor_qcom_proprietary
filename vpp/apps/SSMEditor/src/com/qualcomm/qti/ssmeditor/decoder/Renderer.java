/* Renderer.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.ssmeditor.decoder;

import android.media.MediaCodec;
import android.util.Log;

/**
 * Class with base functionality of a Renderer component.
 */
public class Renderer {

    private static final String TAG = "Renderer";

    protected OnRenderCompleteListener mListener;

    private long mStartTime = 0; // nanoseconds
    private long mFirstBufferTime = 0; // microseconds
    private boolean mShouldResetClock = true;

    public void onOutputBufferAvailable(MediaCodec codec, int index,
            MediaCodec.BufferInfo bufferInfo, Decoder decoder) {

        if (bufferInfo.flags == MediaCodec.BUFFER_FLAG_END_OF_STREAM) {
            codec.releaseOutputBuffer(index, false); // do not render to surface
        } else {
            if (mShouldResetClock) {
                resetClock();
                mFirstBufferTime = bufferInfo.presentationTimeUs;
                mShouldResetClock = false;
            }

            long offset = bufferInfo.presentationTimeUs - mFirstBufferTime;
            long timestamp = mStartTime + (offset * 1000);

            long time = System.nanoTime(); // for logging only
            Log.v(TAG, "Display Frame: index = " + index
                    + " timestamp = " + timestamp
                    + " system time = " + time
                    + " queue difference = " + (time - timestamp)
                    + " buffer time = " + bufferInfo.presentationTimeUs
                    + " first buffer time = " + mFirstBufferTime
                    + " delta (seconds) = " + (offset / 1000000.0));

            if (decoder.getCurrentState() == Decoder.State.SEEKING) {
                codec.releaseOutputBuffer(index, true);
            } else {
                codec.releaseOutputBuffer(index, timestamp);
            }
        }

        notifyListener(bufferInfo.presentationTimeUs);
    }

    public void pausePlayback() {}

    public void resumePlayback() {
        mShouldResetClock = true;
    }

    private void resetClock() {
        mStartTime = System.nanoTime();
    }

    public void registerListener(OnRenderCompleteListener listener) {
        mListener = listener;
    }

    public void unregisterListener(OnRenderCompleteListener listener) {
        mListener = null;
    }

    protected void notifyListener(long timestamp) {
        if (mListener != null) {
            mListener.onRenderComplete(timestamp);
        }
    }

    public static interface OnRenderCompleteListener {
        public void onRenderComplete(long timestamp);
    }
}
