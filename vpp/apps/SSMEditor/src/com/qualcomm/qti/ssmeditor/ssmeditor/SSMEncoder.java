/**
 * Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file SSMEncoder.java
 */
 /* Not a Contribution.
 * Apache license notifications and license are retained
 * for attribution purposes only.
 */
/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.qualcomm.qti.ssmeditor.ssmeditor;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecInfo.CodecCapabilities;
import android.media.MediaCodecInfo.VideoCapabilities;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.view.Surface;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

class SSMEncoder {
    private final static String TAG = "SSMEncoder";
    private final static String MIME_TYPE = "video/avc";

    private MediaMuxer mMuxer;
    private int mTrackIndex = -1;
    private List<OnEncodeCompleteListener> mOnEncodeCompleteListeners;
    private ParcelFileDescriptor mFd = null;

    public SSMEncoder() {
        mOnEncodeCompleteListeners = new ArrayList<OnEncodeCompleteListener>();
    }

    public void saveVideo(ParcelFileDescriptor fd, SSMDecoder decoder) {
        MediaCodec encoder;
        MediaFormat format;

        if (fd == null) {
            throw new IllegalStateException("Failed to get video output file");
        }

        mFd = fd;
        mMuxer = initiateMuxer(decoder.getRotation());
        try {
            encoder = MediaCodec.createEncoderByType(MIME_TYPE);
        } catch (IOException e) {
            Log.e(TAG, "Failed to create encoder type: " + e.getMessage());
            return;
        }
        encoder.setCallback(mEncoderCallback);
        format = setUpMediaFormat(decoder.getFormat());
        encoder.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
        Surface inputSurface = encoder.createInputSurface();

        decoder.configure(inputSurface);
        decoder.attachEncoder(encoder);

        Runnable encoderRunnable = new Runnable() {
            @Override
            public void run() {
                encoder.start();
                decoder.start();
                decoder.play();
            }
        };
        new Thread(encoderRunnable).start();
    }

    private MediaMuxer initiateMuxer(int rotation) {
        MediaMuxer muxer;

        try {
            muxer = new MediaMuxer(mFd.getFileDescriptor(),
                    MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);
            muxer.setOrientationHint(rotation);
        } catch (IOException ioe) {
            throw new IllegalStateException("MediaMuxer creation failed", ioe);
        }

        return muxer;
    }

    private MediaFormat setUpMediaFormat(MediaFormat decoderFormat) {
        MediaCodecInfo codecInfo = selectCodec(MIME_TYPE);
        String mime = decoderFormat.getString(MediaFormat.KEY_MIME);
        CodecCapabilities caps = codecInfo.getCapabilitiesForType(mime);
        VideoCapabilities vidCaps = caps.getVideoCapabilities();
        int width = decoderFormat.getInteger(MediaFormat.KEY_WIDTH);
        int height = decoderFormat.getInteger(MediaFormat.KEY_HEIGHT);
        int maxWidth = vidCaps.getSupportedWidths().getUpper();
        int maxHeight = vidCaps.getSupportedHeightsFor(width).getUpper();
        int maxRate = vidCaps.getSupportedFrameRatesFor(width, height).getUpper().intValue();
        int bitrate = vidCaps.getBitrateRange().clamp(
                (int) (vidCaps.getBitrateRange().getUpper()
                        / Math.sqrt((double) maxWidth * maxHeight / width / height)));
        Log.d(TAG, "reasonable bitrate for " + width + "x" + height + "@" + maxRate
                + " " + mime + " = " + bitrate);
        int format = MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface;
        int framerate = 30;
        float iframe_interval = 1;

        MediaFormat encoderFormat = MediaFormat.createVideoFormat(MIME_TYPE, width, height);
        encoderFormat.setInteger(MediaFormat.KEY_BIT_RATE, bitrate);
        encoderFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, format);
        encoderFormat.setInteger(MediaFormat.KEY_FRAME_RATE, framerate);
        encoderFormat.setInteger(MediaFormat.KEY_CAPTURE_RATE, framerate);
        encoderFormat.setFloat(MediaFormat.KEY_I_FRAME_INTERVAL, iframe_interval);

        return encoderFormat;
    }

    private static MediaCodecInfo selectCodec(String mimeType) {
        int numCodecs = MediaCodecList.getCodecCount();
        for (int i = 0; i < numCodecs; i++) {
            MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(i);
            if (!codecInfo.isEncoder()) {
                continue;
            }
            String[] types = codecInfo.getSupportedTypes();
            for (int j = 0; j < types.length; j++) {
                if (types[j].equalsIgnoreCase(mimeType)) {
                    return codecInfo;
                }
            }
        }
        return null;
    }

    private final MediaCodec.Callback mEncoderCallback = new MediaCodec.Callback() {
        @Override
        public void onInputBufferAvailable(MediaCodec codec, int index) {
        }

        @Override
        public void onOutputBufferAvailable(MediaCodec codec, int index,
                                            MediaCodec.BufferInfo mBufferInfo) {
            // Normal flow: get output encoded buffer, send to muxer.
            ByteBuffer encodedData = codec.getOutputBuffer(index);

            Log.d(TAG, "Encoder Output Buffer: OBD, idx=" + index + ", sz=" +
                    mBufferInfo.size + ", offset=" + mBufferInfo.offset + ", ts=" +
                    mBufferInfo.presentationTimeUs + ", flags=" + mBufferInfo.flags);

            if (encodedData == null) {
                throw new RuntimeException("encoderOutputBuffer " + mBufferInfo.flags +
                        " was null");
            }
            if ((mBufferInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {

                Log.v(TAG, "ignoring BUFFER_FLAG_CODEC_CONFIG");
                mBufferInfo.size = 0;
            }

            if (mBufferInfo.size != 0) {
                if (mMuxer == null) {
                    throw new RuntimeException("muxer hasn't started");
                }

                encodedData.position(mBufferInfo.offset);
                encodedData.limit(mBufferInfo.offset + mBufferInfo.size);
                try {
                    mMuxer.writeSampleData(mTrackIndex, encodedData, mBufferInfo);
                    Log.v(TAG, "Sent " + mBufferInfo.size + " bytes to muxer and" +
                            "use mTrackIndex " + mTrackIndex);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
            codec.releaseOutputBuffer(index, false);

            if ((mBufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                Log.w(TAG, "reached end of stream");

                try {
                    mMuxer.stop();
                    mMuxer.release();
                    mMuxer = null;
                } catch (Exception e) {
                    Log.e(TAG, "I/O Error: " + e.getMessage());
                }

                codec.stop();
                codec.release();

                Log.d(TAG, "Done encoding");

                onEncodeComplete();
            }
        }

        @Override
        public void onOutputFormatChanged(MediaCodec codec, MediaFormat format) {
            // Subsequent data will conform to new format.
            // Can ignore if using getOutputFormat(outputBufferId)
            Log.d(TAG, "output format changed " + format);
            //mOutputFormat = format; // option B

            mTrackIndex = mMuxer.addTrack(format);
            Log.d(TAG, "track Index set " + mTrackIndex);
            mMuxer.start();
        }

        @Override
        public void onError(MediaCodec codec, MediaCodec.CodecException e) {
            Log.e(TAG, "encoder exception: " + e);
        }
    };

    private void onEncodeComplete() {
        notifyOnEncodeCompleteListeners();
    }

    public interface OnEncodeCompleteListener {
        void onEncodeComplete();
    }

    public void registerOnEncodeCompleteListener(OnEncodeCompleteListener listener) {
        if (!mOnEncodeCompleteListeners.contains(listener)) {
            mOnEncodeCompleteListeners.add(listener);
        }
    }

   public void unregisterOnEncodeCompleteListener(OnEncodeCompleteListener listener) {
       if (mOnEncodeCompleteListeners.contains(listener)) {
           mOnEncodeCompleteListeners.remove(listener);
       }
   }

    private void notifyOnEncodeCompleteListeners() {
        Log.d(TAG, "Notifying onEncodeComplete");
        for (OnEncodeCompleteListener listener : mOnEncodeCompleteListeners) {
            listener.onEncodeComplete();
        }
    }
}
