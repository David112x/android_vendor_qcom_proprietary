/* SSMEncodeRenderer.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.ssmeditor.ssmeditor;

import android.media.MediaCodec;
import android.util.Log;

import com.qualcomm.qti.ssmeditor.decoder.Decoder;

public class SSMEncodeRenderer extends SSMRenderer {

    private static final String TAG = "SSMEncodeRenderer";

    private MediaCodec mEncoder;

    public SSMEncodeRenderer(MediaCodec encoder) {
        super();

        mEncoder = encoder;
    }

    @Override
    public void onOutputBufferAvailable(MediaCodec codec, int index,
            MediaCodec.BufferInfo bufferInfo, Decoder decoder) {
        if (bufferInfo.flags == MediaCodec.BUFFER_FLAG_END_OF_STREAM) {
            if (mEncoder != null) {
                Log.d(TAG, "signaling end of stream to encoder");
                mEncoder.signalEndOfInputStream();
            }

            codec.releaseOutputBuffer(index, false); // do not render to surface
        } else {

            mTimestamp += calculateTimestampInterval(bufferInfo.presentationTimeUs,
                    decoder.getFrameRate(), decoder.getBaseFrameRate());

            Log.v(TAG, "Display frame = " + index
                    + " at " + mTimestamp
                    + " system time = " + System.nanoTime()
                    + " difference = " + (mTimestamp - System.nanoTime())
                    + " timestamp time = " + bufferInfo.presentationTimeUs * 1000);

            Log.v(TAG, "releasing output buffer #" + index
                    + " at time: " + mTimestamp);
            codec.releaseOutputBuffer(index, mTimestamp);

            notifyListener(bufferInfo.presentationTimeUs);
        }
    }
}
