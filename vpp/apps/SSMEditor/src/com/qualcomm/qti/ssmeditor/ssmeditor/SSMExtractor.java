/**
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file SSMExtractor.java
 */
/*
 * Not a Contribution.
 */
/*
 * Copyright (C) 2010 The Android Open Source Project
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

import android.content.res.AssetFileDescriptor;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.util.Log;

import com.qualcomm.qti.mmca.ssmeditor.FRCSegment;
import com.qualcomm.qti.ssmeditor.decoder.Extractor;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Collections;
import java.util.List;

/**
 *  MediaExtractor for SSME.
 */
public class SSMExtractor extends Extractor {

    private final String TAG = "SSMExtractor";

    /**
     * Class used for parsing layer ID.
     */
    class NALBuffer {
        byte[] data;
        int baseIndex;
        int size;
        int nalStartIndex;
        int nalSize;

        NALBuffer(int sz) {
            this.size = sz;
            this.data = new byte[sz];
            this.baseIndex = 0;
            this.nalStartIndex = 0;
            this.nalSize = 0;
        }
    };

    private static final int MIN_VALID_FRAME_RATE = 60;

    private List<FRCSegment> mSegmentList;

    public SSMExtractor() {
        super();
        mSegmentList = Collections.emptyList();
    }

    // Get next buffer timestamp
    // is slow zone
    // yes --> get frame (normal behaviour)
    // no --> keep advancing until the base layer
    @Override
    public int processFrame(ByteBuffer byteBuff) {
        int layerId;
        int framesProcessed = 0;
        int chunkSize = 0;

        chunkSize = readSampleData(byteBuff, 0);
        layerId = 0;
        if (getFrameRate() >= MIN_VALID_FRAME_RATE) {
            layerId = findAVCLayerId(byteBuff, chunkSize);
            if (layerId < 0 && chunkSize >= 0) {
                Log.e(TAG, "advance: unable to handle clip");
            }
        }

        if (!isInSlowZone(getSampleTime()) && chunkSize >= 0) {
            // Skip frames so that clip is at its base frame rate
            while (layerId > 0) {
                advance();
                framesProcessed++;
                byteBuff.clear();
                chunkSize = readSampleData(byteBuff, 0);

                if (chunkSize < 0) {
                    break;
                }

                layerId = findAVCLayerId(byteBuff, chunkSize);

                if (layerId < 0 && chunkSize >= 0) {
                    // Not hier-p encoded
                    break;
                }
            }
        }

        Log.d(TAG, "advance: frames processed: " + framesProcessed);
        return chunkSize;
    }

    @Override
    public void setDataSource(AssetFileDescriptor dataSource) throws IOException,
            IllegalArgumentException {
        super.setDataSource(dataSource);
        String mime = getVideoFormat().getString(MediaFormat.KEY_MIME);
        if (!mime.startsWith(MediaFormat.MIMETYPE_VIDEO_AVC)) {
            throw new IllegalArgumentException("Not contain avc track for SSME: " + mime);
        }

        try {
            mMaxLayerId = calculateMaxLayerId();
        } catch (IllegalArgumentException e) {
            throw new IllegalArgumentException("Not a valid clip for SSME: "
                    + dataSource.toString(), e);
        }

        mBaseFrameRate = calculateBaseFrameRate(mFrameRate, mMaxLayerId);

        seekToBeginning();
    }

    public void setSegments(List<FRCSegment> segmentList) {
        if (segmentList == null) {
            mSegmentList = Collections.emptyList();
        } else {
            mSegmentList = segmentList;
        }
    }

    // TODO see if this should be synchronized
    private boolean isInSlowZone(long timeUs) {
        for (FRCSegment segment : mSegmentList) {
            if (timeUs >= segment.getStartTime() && timeUs < segment.getEndTime()
                    && !segment.isBypass()) {
                return true;
            }
        }

        return false;
    }

    /**
     * Allocates a byte buffer based on the required size, or reuses the existing
     * buffer if possible.
     *
     * @param buffer The buffer to reuse if capacity >= size
     * @param size The required size of the byte buffer
     * @return The new or existing byte buffer
     */
    private ByteBuffer allocateByteBuffIfNecessary(ByteBuffer buffer, int size) {
        if (buffer == null || buffer.capacity() < size) {
            Log.d(TAG, "allocateByteBuffIfNecessary, size=" + size);
            return ByteBuffer.allocate(size);
        }

        return buffer;
    }

    /**
     * True if the clip is hier-p encoded and base frame rate <= 30.
     */
    public boolean canUseVpp() throws IllegalArgumentException {
        Log.v(TAG, "maxLayerId: " + mMaxLayerId);
        Log.v(TAG, "base frame rate: " + mBaseFrameRate);

        return mMaxLayerId != -1;
    }

    /**
     * Returns the maximum layer id or -1 if there is an error.
     */
    private int calculateMaxLayerId() throws IllegalArgumentException {
        long timestamp = getSampleTime();
        seekToBeginning();

        ByteBuffer byteBuff = null;
        int layerId;
        int maxLayerId = -1;
        int minLayerId = -1;
        int minLayersSeen = 0;

        do {
            byteBuff = allocateByteBuffIfNecessary(byteBuff,
                    (int)mExtractor.getSampleSize());
            int chunkSize = readSampleData(byteBuff, 0);
            mExtractor.advance();

            if (chunkSize < 0) {
                return maxLayerId;
            }

            layerId = findAVCLayerId(byteBuff, chunkSize);
            byteBuff.clear();

            Log.v(TAG, "layerId=" + layerId +
                    ", maxLayerId=" + maxLayerId +
                    ", minLayerId=" + minLayerId);

            if (layerId < 0 && chunkSize >= 0) {
                throw new IllegalArgumentException("Invalid HSR Clip");
            }

            maxLayerId = Math.max(maxLayerId, layerId);

            if (minLayerId == -1) {
                minLayerId = layerId;
            }

            if (layerId < minLayerId) {
                minLayersSeen = 0;
                minLayerId = layerId;
            }

            if (minLayerId == layerId) {
                minLayersSeen++;
            }

            if (minLayersSeen == 5) {
                Log.v(TAG, "seen minimum layers 5 times");
                break;
            }
        } while (layerId >= 0);

        seekTo(timestamp, MediaExtractor.SEEK_TO_PREVIOUS_SYNC);
        return maxLayerId;
    }

    private int calculateBaseFrameRate(int inputFR, int maxLayer) {
        return inputFR / (int) Math.pow(2, maxLayer);
    }

    @Override
    protected int calculateFrameRate() throws IllegalArgumentException {
        // Frame rate is an optional key so need to check first
        int defaultRate = 30;
        int frameRate = defaultRate;

        MediaFormat videoFormat = getVideoFormat();
        Log.v(TAG, "Selected video track format: " + videoFormat);

        if (videoFormat.containsKey(MediaFormat.KEY_FRAME_RATE)) {
            frameRate = videoFormat.getInteger(MediaFormat.KEY_FRAME_RATE);
            // Key frame rate is sometimes off from standard rates like 30, 60, 120, 240
            // Interpret the frame rates consistently by rounding it to closest 30 to
            // decide whether or not clip has valid parameters
            // Should be at least 28 to be counted as 30
            if (frameRate >= (defaultRate - 2)) {
                frameRate = ((frameRate + (defaultRate/2)) / defaultRate) * defaultRate;
            }
        }

        Log.v(TAG, "frame rate: " + frameRate);

        return frameRate;
    }

    private int getNextNALUnit(NALBuffer dataBuf, boolean startCodeFollows) {
        int size = dataBuf.size;
        byte[] data = dataBuf.data;
        int baseIndex = dataBuf.baseIndex;
        int offset = 0;
        int startOffset;
        int endOffset;

        dataBuf.nalStartIndex = 0;
        dataBuf.nalSize = 0;

        if (size < 3) {
            return -1;
        }

        // A valid startcode consists of at least two 0x00 bytes followed by 0x01.
        for (; offset + 2 < size; ++offset) {
            if (data[baseIndex + offset + 2] == 0x01 && data[baseIndex + offset] == 0x00
                    && data[baseIndex + offset + 1] == 0x00) {
                break;
                    }
        }
        if (offset + 2 >= size) {
            dataBuf.baseIndex += offset;
            dataBuf.size = 2;
            return -1;
        }
        offset += 3;

        startOffset = offset;

        for (;;) {
            while (offset < size && data[baseIndex + offset] != 0x01) {
                ++offset;
            }

            if (offset == size) {
                if (startCodeFollows) {
                    offset = size + 2;
                    break;
                }
                return -1;
            }

            if (data[baseIndex + offset - 1] == 0x00 && data[baseIndex + offset - 2] == 0x00) {
                break;
            }

            ++offset;
        }

        endOffset = offset - 2;
        while (endOffset > startOffset + 1 && data[baseIndex + endOffset - 1] == 0x00) {
            --endOffset;
        }

        dataBuf.nalStartIndex = baseIndex + startOffset;
        dataBuf.nalSize = endOffset - startOffset;

        if (offset + 2 < size) {
            dataBuf.baseIndex += offset - 2;
            dataBuf.size = size - offset + 2;
        } else {
            dataBuf.size = 0;
        }
        return 0;
    }

    private NALBuffer findNAL(NALBuffer dataBuf, int size, int nalType) {
        while (getNextNALUnit(dataBuf, true) == 0) {
            if (dataBuf.nalSize > 0 && (dataBuf.data[dataBuf.nalStartIndex] & 0x1f) == nalType) {
                Log.v(TAG, "nalSize: " + dataBuf.nalSize + " nalStartIndex: " + dataBuf.nalStartIndex);
                return dataBuf;
            }
        }
        return null;
    }

    private int findAVCLayerId(ByteBuffer buf, int size) {
        if (buf == null)
            return -1;

        final int kSvcNalType = 0xE;
        final int kSvcNalSearchRange = 32;
        // SVC NAL
        // |---0 1110|1--- ----|---- ----|iii- ---|
        //       ^                        ^
        //   NAL-type = 0xE               layer-Id
        //
        // layer_id 0 is for base layer, while 1, 2, ... are enhancement layers.
        // Layer n uses reference frames from layer 0, 1, ..., n-1.

        int layerId = 0;

        if (size < 0)
            size = 0;
        NALBuffer dataBuf = new NALBuffer(size);
        buf.get(dataBuf.data);

        dataBuf = findNAL(dataBuf, size > kSvcNalSearchRange ? kSvcNalSearchRange : size, kSvcNalType);
        if (dataBuf != null && dataBuf.nalSize >= 4) {
            layerId = (dataBuf.data[dataBuf.nalStartIndex + 3] >> 5) & 0x7;
        } else if (dataBuf == null) {
            return -1;
        }
        return layerId;
    }

}


