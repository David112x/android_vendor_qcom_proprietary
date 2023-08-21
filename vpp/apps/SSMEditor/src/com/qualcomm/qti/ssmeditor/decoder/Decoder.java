/**
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file Decoder.java
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

package com.qualcomm.qti.ssmeditor.decoder;

import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.util.Log;
import android.view.Surface;

import com.qualcomm.qti.mmca.decoder.MediaCodecParams;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Dictionary;
import java.util.Hashtable;
import java.util.List;

/**
 * Generic video decoder.
 */
public class Decoder implements Renderer.OnRenderCompleteListener {

    public enum ErrorCode {
        ERROR_INVALID_FILE,
        ERROR_DIMENSIONS,
        ERROR_NO_VALID_VIDEO_TRACK,
        ERROR_INVALID_LAYER_ID // TODO remove this. Not generic.
    }

    public enum State {
        UNINITIALIZED,
        READY,
        PLAYING,
        PAUSED,
        INIT_PAUSE,
        SEEKING,
        TRANSITION_FINISH_SEEKING
    }

    public static interface OnPlaybackChangedListener {
        public abstract void onPlaybackProgress(int pct, long curTime,
                boolean isSeeking);
        public abstract void onErrorFormat(Decoder.ErrorCode err, int maxHeight, int maxWidth);
        public abstract void onPlaybackEnd();
        public abstract void onIsPlaying(boolean isPlaying);
    }

    // Allowable state transition table
    private static Dictionary ALLOWED_STATE_TRANSITIONS;

    // Dimension constraint constants
    public static final int MIN_FRAME_RATE = 60;
    public static final int MAX_FRAME_RATE = 240;
    private static final int MAX_WIDTH_1080P = 1920;
    private static final int MAX_HEIGHT_1080P = 1080;
    private static final int MAX_WIDTH_720P = 1280;
    private static final int MAX_HEIGHT_720P = 720;
    private static final long SECOND_US = 1000000;

    // Logging
    private static final String TAG = "Decoder";

    // Playback state
    private State mCurState = State.UNINITIALIZED;
    private State mPrevState = State.UNINITIALIZED;
    private final Object mStateLock = new Object();

    // Format and Extractor related
    private Extractor mExtractor;
    private MediaFormat mVidFormat;
    private int mVidHeight = 0;
    private int mVidWidth = 0;
    private int mRotation = 0;
    private int mMaxSupportedWidth;
    private int mMaxSupportedHeight;

    // MediaCodec
    private MediaCodec mDecoder = null;
    private boolean mIsFlushing = false;
    private int mInputChunk = 0;
    private int mDecodedCount = 0;
    private boolean mSentEOS = false;
    private long mDuration;
    private int mSeekPct;
    private long mPresentationTimeUs = 0;
    private long mInterval;
    private boolean mFirstFrame;
    private long mLastRenderedFrame = -1;

    // Handler
    private HandlerThread mHandlerThread;
    private Looper mLooper;
    private Handler mHandler;

    // Listeners
    private List<OnPlaybackChangedListener> mPlaybackChangedListeners;

    private Renderer mRenderer;

    static {
        ALLOWED_STATE_TRANSITIONS = new Hashtable();

        // Key is target state, list is the set of allowed current states
        ALLOWED_STATE_TRANSITIONS.put(State.UNINITIALIZED, new State[] {
            State.READY,
            State.PLAYING,
            State.PAUSED,
            State.INIT_PAUSE,
            State.SEEKING,
            State.TRANSITION_FINISH_SEEKING
        });

        ALLOWED_STATE_TRANSITIONS.put(State.READY, new State[] {
            State.UNINITIALIZED
        });

        ALLOWED_STATE_TRANSITIONS.put(State.PLAYING, new State[] {
            State.READY,
            State.INIT_PAUSE,
            State.PAUSED,
            State.TRANSITION_FINISH_SEEKING
        });

        ALLOWED_STATE_TRANSITIONS.put(State.PAUSED, new State[] {
            State.INIT_PAUSE,
            State.TRANSITION_FINISH_SEEKING
        });

        ALLOWED_STATE_TRANSITIONS.put(State.INIT_PAUSE, new State[] {
            State.READY,
            State.PLAYING,
            State.TRANSITION_FINISH_SEEKING
        });

        ALLOWED_STATE_TRANSITIONS.put(State.SEEKING, new State[] {
            State.READY,
            State.PLAYING,
            State.PAUSED
        });

        ALLOWED_STATE_TRANSITIONS.put(State.TRANSITION_FINISH_SEEKING, new State[] {
            State.SEEKING
        });
    }

    public Decoder() {
        mHandlerThread = new HandlerThread("Decoder callback thread");
        mFirstFrame = true;

        mPlaybackChangedListeners = new ArrayList<OnPlaybackChangedListener>();
    }

    public void initialize(long savedSeekTimeUs, Extractor extractor)
            throws IllegalArgumentException {
        if (getCurrentState() != State.UNINITIALIZED) {
            Log.d(TAG, "Decoder initialized already; Should not do again");
            return;
        }

        try {
            mInputChunk = 0;

            mExtractor = extractor;
            Log.d(TAG, "Getting video format");
            mVidFormat = mExtractor.getVideoFormat();
            if (mVidFormat == null) {
                mDecoder = null;
                mCurState = State.UNINITIALIZED;
                notifyErrorFormat(ErrorCode.ERROR_NO_VALID_VIDEO_TRACK,
                        mMaxSupportedHeight, mMaxSupportedWidth);
                return;
            }

            mDuration = mVidFormat.getLong(MediaFormat.KEY_DURATION);

            mInterval = SECOND_US / mExtractor.getFrameRate() + 1;

            // Seek file to initial start point in video
            // If activity was destroyed, will start from saved time
            Log.d(TAG, "Setting playback at saved time: " + savedSeekTimeUs);
            realignPlayback(savedSeekTimeUs);

            int alignedSeekPct = getSeekPctFromTime(mExtractor.getSampleTime());
            notifyPlaybackProgress(alignedSeekPct, mExtractor.getSampleTime(),
                    false);

            mVidHeight = mVidFormat.getInteger(MediaFormat.KEY_HEIGHT);
            mVidWidth = mVidFormat.getInteger(MediaFormat.KEY_WIDTH);
            mRotation = mVidFormat.getInteger(MediaFormat.KEY_ROTATION);

            Log.d(TAG, "Video format input frame rate : " + mExtractor.getFrameRate());
            if (!checkValidParameters()) {
                notifyErrorFormat(ErrorCode.ERROR_DIMENSIONS,
                        mMaxSupportedHeight, mMaxSupportedWidth);
            }

            String mime = mVidFormat.getString(MediaFormat.KEY_MIME);
            mDecoder = MediaCodec.createDecoderByType(mime);

            changeState(State.READY);
        } catch (IOException e) {
            mCurState = State.UNINITIALIZED;
            Log.e(TAG, "IOException: " + e);
        } catch (IllegalArgumentException e) {
            mExtractor.release();
            mExtractor = null;
            throw new IllegalArgumentException(e);
        }
    }

    public void registerOnPlaybackChangedListener(OnPlaybackChangedListener listener) {
        if (!mPlaybackChangedListeners.contains(listener)) {
            mPlaybackChangedListeners.add(listener);
        }
    }

    public void unregisterOnPlaybackChangedListener(OnPlaybackChangedListener listener) {
        if (mPlaybackChangedListeners.contains(listener)) {
            mPlaybackChangedListeners.remove(listener);
        }
    }

    private void realignPlayback(long newTs) {
        mExtractor.seekTo(newTs, MediaExtractor.SEEK_TO_PREVIOUS_SYNC);
    }

    private int getSeekPctFromTime(long ts) {
        return (mDuration > 0) ? ((int) ((ts * 100L) / mDuration)) : 0;
    }

    private long getTimeFromSeekPct(int seekPct) {
        return seekPct * mDuration / 100L;
    }

    private void notifyPlaybackProgress(int seekPct, long ts, boolean isSeeking) {
        if (ts >= 0) {
            mLastRenderedFrame = ts;
        }

        Log.v(TAG, "Firing OnPlaybackChanged listeners");
        for (OnPlaybackChangedListener l : mPlaybackChangedListeners) {
            l.onPlaybackProgress(seekPct, ts, isSeeking);
        }
    }

    private void notifyErrorFormat(ErrorCode err, int maxHeight, int maxWidth) {
        Log.v(TAG, "Firing OnErrorFormat listeners");
        for (OnPlaybackChangedListener l : mPlaybackChangedListeners) {
            l.onErrorFormat(err, maxHeight, maxWidth);
        }
    }

    private void notifyPlaybackEnd() {
        Log.v(TAG, "Firing OnPlaybackEnd listeners");
        for (OnPlaybackChangedListener l : mPlaybackChangedListeners) {
            l.onPlaybackEnd();
        }
    }

    private void notifyPlaying(boolean isPlaying) {
        Log.v(TAG, "Firing OnIsPlaying listeners");
        for (OnPlaybackChangedListener l : mPlaybackChangedListeners) {
            l.onIsPlaying(isPlaying);
        }
    }

    private final MediaCodec.Callback mCallback = new MediaCodec.Callback() {
        @Override
        public void onInputBufferAvailable(MediaCodec mc, int inputBufferId) {
            State state = getCurrentState();

            Log.v(TAG, "in buffer available, state : " + state);

            if (state == State.INIT_PAUSE) {
                // During pause, call wait on the handler thread running the
                // mDecoder callbacks
                Log.d(TAG, "Entering pause state which will lock thread");
                changeState(State.PAUSED);
                state = getCurrentState();
                Log.d(TAG, "State after waiting thread " + state);
            }

            if (state == State.SEEKING) {
                long newSeekPointUs = getTimeFromSeekPct(mSeekPct);
                realignPlayback(newSeekPointUs);
                notifyPlaybackProgress(mSeekPct, newSeekPointUs, true);
            }

            if (!mSentEOS || (mFirstFrame)) {
                if (mIsFlushing) {
                    return;
                }
                int inputBufIndex = inputBufferId;
                Log.v(TAG, "inputBufferId " + inputBufIndex);

                if (inputBufIndex < 0) {
                    Log.d(TAG, "input buffer not available");
                    return;
                }

                ByteBuffer inputBuf = mc.getInputBuffer(inputBufIndex);

                // Extractor reads file data into input buffer
                int chunkSize = mExtractor.processFrame(inputBuf);

                if ((chunkSize < 0) || (state == State.TRANSITION_FINISH_SEEKING)) {
                    // End of stream -- send empty frame with EOS flag set.
                    mc.queueInputBuffer(inputBufIndex, 0, 0, 0L,
                            MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                    mSentEOS = true;
                    Log.d(TAG, "Sent input EOS");

                } else {
                    int trackIndex = 0;

                    if (mExtractor.getSampleTrackIndex() != trackIndex) {
                        Log.w(TAG, "WEIRD: got sample from track " +
                                mExtractor.getSampleTrackIndex() + ", expected " + trackIndex);
                    }

                    // During backward seek, do not want backward timestamps
                    if (state == State.SEEKING) {
                        mPresentationTimeUs += mInterval;
                    } else {
                        mPresentationTimeUs = mExtractor.getSampleTime();
                    }

                    Log.v(TAG, "Input submitted presentation time : " + mPresentationTimeUs);

                    mc.queueInputBuffer(inputBufIndex, 0, chunkSize,
                            mPresentationTimeUs, mExtractor.getSampleFlags());

                    Log.v(TAG, "Submitted frame " + mInputChunk +
                            " to dec, size=" + chunkSize);

                    mInputChunk++;

                    if (state == State.PLAYING) {
                        mExtractor.advance();
                    }
                }
            }
        }

        @Override
        public void onOutputBufferAvailable(MediaCodec codec, int index,
                MediaCodec.BufferInfo bufferInfo) {
            if (mIsFlushing) {
                codec.releaseOutputBuffer(index, false);
            }

            State state = getCurrentState();

            Log.v(TAG, "BUFFER INFO: OBD, idx=" + index + ", sz=" + bufferInfo.size + ", offset=" +
                    bufferInfo.offset + ", ts=" + bufferInfo.presentationTimeUs +
                    ", flags=" + bufferInfo.flags + ", state=" + state);

            // Render the frame
            renderOutputBuffer(codec, index, bufferInfo);

            if (bufferInfo.flags == MediaCodec.BUFFER_FLAG_END_OF_STREAM) {
                flush(codec);

                if (state == State.TRANSITION_FINISH_SEEKING) {
                    long newSeekPointUs = getTimeFromSeekPct(mSeekPct);
                    realignPlayback(newSeekPointUs);
                    int alignedSeekPct = getSeekPctFromTime(mExtractor.getSampleTime());
                    notifyPlaybackProgress(alignedSeekPct, mExtractor.getSampleTime(), false);

                    resetPreviousState();
                    mFirstFrame = true;

                } else {
                    // when it reaches end of clip just leave it paused
                    pause();
                    // Set play point back to start of clip
                    realignPlayback(0);
                    notifyPlaybackEnd();
                }

                codec.start();

                mSentEOS = false;
            } else if (state == State.TRANSITION_FINISH_SEEKING) {
                mFirstFrame = true;
            }
        }

        @Override
        public void onOutputFormatChanged(MediaCodec codec, MediaFormat format) {
            Log.d(TAG, "Decoder output format changed " + format);
            mFirstFrame = false;
        }

        @Override
        public void onError(MediaCodec codec, MediaCodec.CodecException e) {
            Log.e(TAG, "Decoder error: " + e);
        }
    };

    private void renderOutputBuffer(MediaCodec codec, int index, MediaCodec.BufferInfo info) {
        mRenderer.onOutputBufferAvailable(codec, index, info, this);
    }

    private boolean checkValidParameters() {
        int dimLarge = 0;
        int dimSmall = 0;
        String build = Build.DEVICE;
        Log.d(TAG, "Device: " + build);

        // Resolution must not exceed
        if (mVidWidth >= mVidHeight) {
            dimLarge = mVidWidth;
            dimSmall = mVidHeight;
        } else {
            dimLarge = mVidHeight;
            dimSmall = mVidWidth;
        }

        if (build.equals("msmnile") || build.equals("kona")) {
            mMaxSupportedWidth = MAX_WIDTH_1080P;
            mMaxSupportedHeight = MAX_HEIGHT_1080P;
        } else {
            mMaxSupportedWidth = MAX_WIDTH_720P;
            mMaxSupportedHeight = MAX_HEIGHT_720P;
        }

        Log.d(TAG, "Max allowed dimens: " + mMaxSupportedWidth + " x " + mMaxSupportedHeight);

        return dimLarge <= mMaxSupportedWidth && dimSmall <= mMaxSupportedHeight
            && mExtractor.getFrameRate() >= MIN_FRAME_RATE
            && mExtractor.getFrameRate() <= MAX_FRAME_RATE;
    }

    public void configure(Surface surface, MediaCodecParams params) {
        if (getCurrentState() != State.UNINITIALIZED) {
            mVidFormat = params.pack(mVidFormat);
            mDecoder.configure(mVidFormat, surface, null, 0);
            Log.d(TAG, "Configured decoder");
            mDecodedCount = 0;
            mInputChunk = 0;
        } else {
            Log.e(TAG, "Attempt to configure while decoder is uninitialized");
        }
    }

    public void start(Renderer renderer) {
        mRenderer = renderer;
        mRenderer.registerListener(this);

        if (getCurrentState() != State.UNINITIALIZED) {
            mHandlerThread.start();
            mLooper = mHandlerThread.getLooper();
            mHandler = new Handler(mLooper);
            mDecoder.setCallback(mCallback, mHandler);
            mDecoder.start();
        } else {
            Log.e(TAG, "Attempt to start while decoder is uninitialized");
        }
    }

    public void destroy() {
        // flush codec
        mIsFlushing = true;
        // if in paused state, need to wake up decoder to flush it
        synchronized (mStateLock) {
            Log.d(TAG, "Waking up decoder thread before flush");
            mStateLock.notifyAll();
        }

        if (getCurrentState() != State.UNINITIALIZED) {
            if (mHandlerThread != null) {
                mHandlerThread.quit();
            }

            if (mLooper != null) {
                mLooper.quit();
            }

            if (mHandler != null) {
                Log.d(TAG, "Quit handlerthread and looper and removed messages");
                mHandler.removeCallbacksAndMessages(null);
                while (mHandlerThread.isAlive()) {
                    Log.d(TAG, "Handlerthread status: " + mHandlerThread.isAlive());
                }
            }

            if (mRenderer != null) {
                mRenderer.unregisterListener(this);
                mRenderer = null;
            }

            if (mDecoder != null) {
                mDecoder.release();
                mDecoder = null;
            }

            if (mExtractor != null) {
                mExtractor.release();
                mExtractor = null;
            }

            mCurState = State.UNINITIALIZED;
        }

        mIsFlushing = false;
    }

    public void updateProgress(int pct) {
        if (getCurrentState() == State.SEEKING) {
            Log.d(TAG, "Seeking to " + pct);
            // Extractor will seek to this point when filling input buffer
            mSeekPct = pct;
        }
    }

    // Public functions to request state change
    public boolean play() {
        return changeState(State.PLAYING);
    }

    public boolean pause() {
        return changeState(State.INIT_PAUSE);
    }

    public boolean seek() {
        return changeState(State.SEEKING);
    }

    public boolean endSeek() {
        return changeState(State.TRANSITION_FINISH_SEEKING);
    }

    // private function to handle internal state access
    private boolean changeState(State newState) {
        synchronized (mStateLock) {
            Log.d(TAG, "Attempt state changed from " + getCurrentState() + " to " + newState);
            State[] allowedStates = (State[]) ALLOWED_STATE_TRANSITIONS.get(newState);
            List<State> allowedStatesList = Arrays.asList(allowedStates);
            if (allowedStatesList.contains(getCurrentState())) {
                switch (newState) {
                    case UNINITIALIZED:
                        uninitialize();
                        break;
                    case READY:
                        readyDecoder();
                        break;
                    case PLAYING:
                        startPlaying();
                        break;
                    case PAUSED:
                        startPausing();
                        break;
                    case INIT_PAUSE:
                        initPausing();
                        break;
                    case SEEKING:
                        startSeeking();
                        break;
                    case TRANSITION_FINISH_SEEKING:
                        stopSeeking();
                        break;
                }
                return true;
            } else {
                Log.d(TAG, "Invalid state transition");
                return false;
            }
        }
    }

    private void readyDecoder() {
        mCurState = State.READY;
    }

    private void uninitialize() {
        mDecoder = null;
        mCurState = State.UNINITIALIZED;
    }

    private void startPlaying() {
        mCurState = State.PLAYING;
        Log.d(TAG, "unpause notify all !!");
        mStateLock.notifyAll();
        if (mRenderer != null) {
            /*
             * This check is needed because based on the existing callflow
             * in SSMEditorActivity, Decoder.play() is called before
             * Decoder.start() when resuming the Activity from the background.
             * TODO fix the callflow in SSMEditorActivity.
             */
            mRenderer.resumePlayback();
        }
        notifyPlaying(true);
    }

    private void startSeeking() {
        mPrevState = mCurState;

        mCurState = State.SEEKING;
        Log.d(TAG, "Unpause notify all !!");
        mStateLock.notifyAll();
    }

    private void stopSeeking() {
        mCurState = State.TRANSITION_FINISH_SEEKING;
    }

    private void initPausing() {
        mCurState = State.INIT_PAUSE;
    }

    private void startPausing() {
        try {
            mCurState = State.PAUSED;
            Log.d(TAG, "put on wait");
            notifyPlaying(false);
            mRenderer.pausePlayback();
            mStateLock.wait();
        } catch (InterruptedException e) {
            Log.e(TAG, "interrupted exception : " + e);
        }
    }

    private void resetPreviousState() {
        // Add bundles back
        Log.d(TAG, "Resetting state from " + mCurState + " to " + mPrevState);
        changeState(mPrevState);
    }

    public State getCurrentState() {
        return mCurState;
    }

    public long getDuration() {
        return mDuration;
    }

    public MediaFormat getFormat() {
        return mVidFormat;
    }

    public int getVideoHeight() {
        return mVidHeight;
    }

    public int getVideoWidth() {
        return mVidWidth;
    }

    public int getRotation() {
        return mRotation;
    }

    public long getLastRenderedFrameTimeUs() {
        return mLastRenderedFrame;
    }

    @Override
    public void onRenderComplete(long timestamp) {
        int curProgressPct = getSeekPctFromTime(timestamp);
        notifyPlaybackProgress(curProgressPct, timestamp, getCurrentState() == State.SEEKING);
    }

    private void flush(MediaCodec mediaCodec) {
        Log.d(TAG, "Starting flush");
        mIsFlushing = true;
        mediaCodec.flush();

        mInputChunk = 0;
        mDecodedCount = 0;

        mIsFlushing = false;
    }

    public int getFrameRate() {
        return mExtractor.getFrameRate();
    }

    public int getBaseFrameRate() {
        return mExtractor.getBaseFrameRate();
    }

    public void setParams(MediaCodecParams params) {
        if (getCurrentState() != State.UNINITIALIZED) {
            mDecoder.setParameters(params.pack(new Bundle()));
        }
    }
}
