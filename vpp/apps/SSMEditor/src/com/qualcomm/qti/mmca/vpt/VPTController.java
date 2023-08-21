/**
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file VPTController.java
 */
package com.qualcomm.qti.mmca.vpt;

import android.content.Context;
import android.content.res.Configuration;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraConstrainedHighSpeedCaptureSession;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CameraMetadata;
import android.hardware.camera2.CaptureFailure;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.TotalCaptureResult;
import android.hardware.camera2.params.OutputConfiguration;
import android.hardware.camera2.params.SessionConfiguration;
import android.media.MediaRecorder;
import android.os.ConditionVariable;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.util.Range;
import android.view.Surface;

import com.qualcomm.qti.mmca.camera.CameraCaptureKeys;
import com.qualcomm.qti.mmca.camera.HFRConfiguration;
import com.qualcomm.qti.mmca.util.HandlerExecutor;
import com.qualcomm.qti.mmca.util.LogHelper;
import com.qualcomm.qti.mmca.util.VideoFile;
import com.qualcomm.qti.mmca.vpt.classification.ClassificationBatch;
import com.qualcomm.qti.mmca.vpt.classification.ClassificationResult;
import com.qualcomm.qti.mmca.vpt.classification.Frame;
import com.qualcomm.qti.mmca.vpt.classification.Metadata;
import com.qualcomm.qti.mmca.vpt.classification.SessionInfo;
import com.qualcomm.qti.mmca.vpt.classification.SessionInfo.Stats;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.List;
import java.util.Vector;

public class VPTController {
    private static final String TAG = "VPTController";

    public interface VPTStateCallback {
        void onSessionReady();
        void onFinished();
        void onClassificationResultReceived();
        void onAllClassificationResultsReceived();
    }

    private static final int TIMEOUT_OPEN_CAMERA_DEVICE_MS      = 5000;
    private static final int TIMEOUT_CLOSE_CAMERA_DEVICE_MS     = 5000;

    private static final int TIMEOUT_OPEN_CAPTURE_SESSION_MS    = 5000;
    private static final int TIMEOUT_CLOSE_CAPTURE_SESSION_MS   = 5000;

    // Control
    List<VPTStateCallback> mCallbacks = new ArrayList<>();
    Object mLock = new Object();

    // Threads
    Handler mCameraHandler;
    HandlerThread mCameraThread;
    boolean mThreadStarted = false;
    final ConditionVariable mCameraCond = new ConditionVariable();
    final ConditionVariable mCameraSessionConfigCond = new ConditionVariable();
    final ConditionVariable mCameraSessionCloseCond = new ConditionVariable();

    // Camera
    List<HFRConfiguration> mHFRConfigs;
    private CameraManager mManager;
    private CameraDevice mCamera;
    private CaptureRequest.Builder mRequestBuilder;
    private CameraConstrainedHighSpeedCaptureSession mCaptureSession;

    // Display
    private Surface mPreviewSurface;

    // Encoder
    private MediaRecorder mRecorder;
    private Surface mVideoSurface;
    private VideoFile mVideoFile;

    private StateTracker<VPTState> mState = new StateTracker<>(VPTState.NULL);

    private long mMediaRecorderStartTime;
    private long mMediaRecorderPauseTime;

    // VPT Specific
    private SessionInfo mSessionInfo;
    private VPTSessionConfiguration mConfig = null;
    private long mBaseTimestamp;
    private long mLastResultTs;
    private long mFirstTimestampInPreviousBatch;
    private Metadata mClassificationResults;

    enum VPTState {
        NULL,
        INITIALIZING,
        PREVIEWING,
        RECORDING,
        WAIT_RESULTS,
        ALL_RESULTS_RECEIVED,
        FINISHING,
        DESTROYING,
    }

    enum FinishingState {
        CLOSING_CAMERA,
        CLOSING_CAMERA_ASYNC,
        STOPPING_MEDIA_RECORDER,
        CLOSING_MEDIA_RECORDER,
    }

    private class StateTracker<T extends Enum> {
        private T mPreviousState;
        private T mCurrentState;
        private StateTracker<?> mSubState;

        public StateTracker(T state) {
            mCurrentState = state;
            mPreviousState = null;
        }

        public T get() {
            return mCurrentState;
        }

        public T getPreviousState() {
            return mPreviousState;
        }

        public void set(T state, StateTracker<?> subState) {
            mSubState = subState;
            mPreviousState = mCurrentState;
            mCurrentState = state;
            logEvent(mPreviousState + " -> " + mCurrentState);
        }

        public void set(T state) {
            set(state, null);
        }

        public StateTracker<?> getSubState() {
            return mSubState;
        }

        public boolean equals(T state) {
            return mCurrentState == state;
        }
    }

    public VPTController(Context context) {
        mState.set(VPTState.NULL);
        mManager = (CameraManager)context.getSystemService(Context.CAMERA_SERVICE);
    }

    public void addStateCallback(VPTStateCallback callback) {
        if (!mCallbacks.contains(callback)) {
            mCallbacks.add(callback);
        }
    }

    public void removeStateCallback(VPTStateCallback callback) {
        if (mCallbacks.contains(callback)) {
            mCallbacks.remove(callback);
        }
    }

    public void initialize() {
        logEvent("initialize()");
        startHandler();
    }

    public void destroy() {
        logEvent("destroy()");

        if (mCamera != null) {
            mState.set(VPTState.DESTROYING);
            closeCamera();
            waitUntilCamera(CameraEvent.DEVICE_CLOSED, TIMEOUT_CLOSE_CAMERA_DEVICE_MS, mCameraCond);
        }

        stopHandler();
        mState.set(VPTState.NULL);
    }

    public void configure(VPTSessionConfiguration config, Surface previewSurface)
            throws Exception {
        mClassificationResults = new Metadata();

        mLastResultTs = 0;
        mBaseTimestamp = 0;
        mMediaRecorderStartTime = 0;
        mMediaRecorderPauseTime = 0;
        mFirstTimestampInPreviousBatch = 0;

        mSessionInfo = mClassificationResults.getSessionInfo();

        mPreviewSurface = previewSurface;
        mConfig = config;

        mVideoFile = mConfig.getVideoFile();
        mVideoFile.openFileDescriptor("rw");

        mSessionInfo.set(SessionInfo.SESSION_NAME, mConfig.getFilenameNoExtension());
        mSessionInfo.set(SessionInfo.CAPTURE_RATE, Integer.toString(mConfig.captureRate));
        mClassificationResults.setFrameRate(mConfig.captureRate);

        if (mCamera == null) {
            openCamera();
            waitUntilCamera(CameraEvent.DEVICE_OPENED, TIMEOUT_OPEN_CAMERA_DEVICE_MS, mCameraCond);
            if (mCamera == null) {
                throw new Exception("failed to open camera device");
            }
        }

        mState.set(VPTState.INITIALIZING);
        setupMediaRecorder(mConfig);

        setupCameraCaptureSession(mConfig);
        waitUntilCamera(CameraEvent.CAPTURE_SESSION_CONFIGURED,
                TIMEOUT_OPEN_CAPTURE_SESSION_MS, mCameraSessionConfigCond);

        // TODO: should this be moved into an explicit start() command?
        startCaptureRequests();
    }

    public void finish(boolean waitForCompletion) {
        logEvent("finish(" + waitForCompletion + ")");

        if (mState.equals(VPTState.NULL)) {
            return;
        }

        if (waitForCompletion) {
            mState.set(VPTState.FINISHING, new StateTracker<>(FinishingState.CLOSING_CAMERA));
            closeCamera();
            waitUntilCamera(CameraEvent.DEVICE_CLOSED, TIMEOUT_CLOSE_CAMERA_DEVICE_MS, mCameraCond);

            stopAndCloseMediaRecorderIfNeeded();
        } else {
            mState.set(VPTState.FINISHING, new StateTracker<>(FinishingState.CLOSING_CAMERA_ASYNC));
            closeCamera();
        }
    }

    public void startRecording() {
        logEvent("startRecording()");
        try {
            lockAutoFocus();
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
        startMediaRecorder();
        mState.set(VPTState.RECORDING);
    }

    public void stopRecording() {
        logEvent("stopRecording()");
        pauseMediaRecorder();
        mState.set(VPTState.WAIT_RESULTS);
    }

    public void updateDeviceOrientation() throws IllegalStateException {

        if (!mState.equals(VPTState.PREVIEWING)) {
            throw new IllegalStateException(
                    "updateDeviceOrientation called in invalid state" + mState);
        }

        mState.set(VPTState.INITIALIZING);

        mRecorder.reset();
        try {
            mCaptureSession.abortCaptures();
            configureMediaRecorder(mConfig);

            setupCameraCaptureSession(mConfig);
            waitUntilCamera(CameraEvent.CAPTURE_SESSION_CONFIGURED,
                    TIMEOUT_OPEN_CAPTURE_SESSION_MS, mCameraSessionConfigCond);

            // TODO: should this be moved into an explicit start() command?
            startCaptureRequests();
        } catch (IOException | CameraAccessException e) {
            e.printStackTrace();
        }
    }

    public Metadata getClassificationResults() {
        return mClassificationResults;
    }

    private void startHandler() {
        synchronized (mLock){
            if (mThreadStarted) {
                return;
            }
            mCameraThread = new HandlerThread("CameraThread");
            mCameraThread.start();
            mCameraHandler = new Handler(mCameraThread.getLooper());

            mThreadStarted = true;
        }
    }

    private void stopHandler() {
        synchronized (mLock) {
            if (!mThreadStarted) {
                return;
            }

            if (mCameraThread != null) {
                try {
                    mCameraThread.quitSafely();
                    mCameraThread.join();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }

            mCameraThread = null;
            mCameraHandler = null;

            mThreadStarted = false;
        }
    }

    private void openCamera() throws CameraAccessException {
        logEvent("openCamera()");
        try {
            mManager.openCamera(mConfig.cameraId, mCameraStateCallback, mCameraHandler);
            mSessionInfo.set(Stats.DEBUG_OPEN_CAMERA_DEVICE_START);

        } catch (CameraAccessException e) {
            Log.e(TAG, "unable to access the camera :(");
            e.printStackTrace();
            mState.set(VPTState.NULL);
            throw e;
        }
    }

    private void closeCamera() {
        if (mCamera == null) {
            return;
        }

        appendDebugText("closing camera device... ");
        mCamera.close();
    }

    private enum CameraEvent {
        DEVICE_OPENED,
        DEVICE_CLOSED,
        CAPTURE_SESSION_CONFIGURED,
        CAPTURE_SESSION_CLOSED,
    };

    private void waitUntilCamera(CameraEvent event, int timeoutMs, ConditionVariable cond) {

        if (timeoutMs < 0) {
            cond.block();
        } else {
            cond.block(timeoutMs);
        }
        cond.close();

        if (event == CameraEvent.DEVICE_OPENED) {
            if (mCamera == null) {
                Log.e(TAG, "failed to open camera");
            }
        } else if (event == CameraEvent.DEVICE_CLOSED) {
            if (mCamera != null) {
                Log.e(TAG, "failed to close camera");
            }
        } else if (event == CameraEvent.CAPTURE_SESSION_CONFIGURED) {
            if (mCaptureSession == null) {
                Log.e(TAG, "failed to configure capture session");
            }
        } else if (event == CameraEvent.CAPTURE_SESSION_CLOSED) {
            if (mCaptureSession != null) {
                Log.e(TAG, "failed to close capture session");
            }
        }
    }

    CameraDevice.StateCallback mCameraStateCallback = new CameraDevice.StateCallback() {
        @Override
        public void onOpened(CameraDevice camera) {
            logEvent("CameraDevice.StateCallback.onOpened()");
            mSessionInfo.set(Stats.DEBUG_OPEN_CAMERA_DEVICE_START);

            mCamera = camera;
            mCameraCond.open();
        }

        @Override
        public void onDisconnected(CameraDevice camera) {
            logEvent("CameraDevice.StateCallback.onDisconnected()");
            mCamera = null;
        }

        @Override
        public void onError(CameraDevice camera, int error) {
            logEvent("CameraDevice.StateCallback.onError()");
            mCamera = null;
            // mCameraStats.cameraErrors.add(error);
        }

        @Override
        public void onClosed(CameraDevice camera) {
            logEvent("CameraDevice.StateCallback.onClosed()");
            mSessionInfo.set(Stats.DEBUG_CLOSE_CAMERA_DEVICE_END);
            mCamera = null;
            mCameraCond.open();

            if (mState.equals(VPTState.FINISHING)) {
                StateTracker<FinishingState> subState =
                        (StateTracker<FinishingState>) mState.getSubState();
                if (subState.equals(FinishingState.CLOSING_CAMERA_ASYNC)) {
                    stopAndCloseMediaRecorderIfNeeded();
                    mCallbacks.forEach(cb -> cb.onFinished());
                }
            }
        }
    };

    private void setupCameraCaptureSession(VPTSessionConfiguration ssmConfig)
            throws CameraAccessException {
        logEvent("createCaptureSession()");

        List<OutputConfiguration> outputConfigurations = new ArrayList<>(2);
        outputConfigurations.add(new OutputConfiguration(mPreviewSurface));
        outputConfigurations.add(new OutputConfiguration(mVideoSurface));

        mRequestBuilder = mCamera.createCaptureRequest(CameraDevice.TEMPLATE_RECORD);
        mRequestBuilder.addTarget(mPreviewSurface);
        mRequestBuilder.addTarget(mVideoSurface);
        mRequestBuilder.set(CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE,
                new Range<>(ssmConfig.captureRate, ssmConfig.captureRate));
        mRequestBuilder.set(CameraCaptureKeys.VPT_HFR_ENABLE, 1);

        SessionConfiguration cameraSessionConfig =
                new SessionConfiguration(SessionConfiguration.SESSION_HIGH_SPEED,
                        outputConfigurations,
                        new HandlerExecutor(mCameraHandler), mCaptureSessionStateCallback);
        cameraSessionConfig.setSessionParameters(mRequestBuilder.build());
        mSessionInfo.set(Stats.DEBUG_CREATE_CAMERA_SESSION_START);
        mCamera.createCaptureSession(cameraSessionConfig);
    }

    private void closeCaptureSession() {
        // It isn't mandatory to close the capture session when closing the
        // camera or reconfiguring the capture session. This chunk of code is
        // not called, but left here intentionally for debugging and
        // performance profiling.
        if (mCaptureSession == null) {
            // TODO: FIXME: if async, need to trigger a callback here
            return;
        }

        mSessionInfo.set(Stats.DEBUG_CLOSE_CAPTURE_SESSION_START);
        appendDebugText("closing capture session...");
        mCaptureSession.close();
    }

    CameraCaptureSession.StateCallback mCaptureSessionStateCallback =
            new CameraCaptureSession.StateCallback() {

        @Override
        public void onConfigured(CameraCaptureSession session) {
            Log.d(TAG, "onConfigured, session=" + session);
            mCaptureSession = (CameraConstrainedHighSpeedCaptureSession)session;

            mSessionInfo.set(Stats.DEBUG_CREATE_CAMERA_SESSION_END);
            logEvent("capture session created");
            Log.d(TAG, "onConfigured (CameraConstrainedHighSpeedCaptureSession)");

            mCameraSessionConfigCond.open();
        }

        @Override
        public void onConfigureFailed(CameraCaptureSession session) {
            mCameraSessionConfigCond.open();
            Log.e(TAG, "failed to configure capture session");
        }

        @Override
        public void onClosed(CameraCaptureSession session) {
            Log.d(TAG, "onClosed, session=" + session + ", current session=" + mCaptureSession);

            // If we reconfigure (i.e. in the case of orientation change), onConfigured()
            // for the new session can come before onClosed() of the previous session.
            // If we don't have this check, we can end up setting the new session to null
            // which may then crash if we try to access it.
            if (mCaptureSession == session) {
                mCaptureSession = null;
                mSessionInfo.set(Stats.DEBUG_CLOSE_CAPTURE_SESSION_END);
            }

            appendDebugText("capture session closed");
            mCameraSessionCloseCond.open();
        }
    };

    protected void startCaptureRequests() throws CameraAccessException {
        List<CaptureRequest> requestList =
                mCaptureSession.createHighSpeedRequestList(mRequestBuilder.build());
        mCaptureSession.setRepeatingBurst(requestList, mCameraCaptureCallback, mCameraHandler);
        mSessionInfo.set(Stats.StartRepeatingCaptureRequests);
    }

    protected void lockAutoFocus() throws CameraAccessException {
        mRequestBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER,
                CameraMetadata.CONTROL_AF_TRIGGER_START);

        List<CaptureRequest> requestList =
                mCaptureSession.createHighSpeedRequestList(mRequestBuilder.build());
        mCaptureSession.setRepeatingBurst(requestList, mCameraCaptureCallback, mCameraHandler);
    }

    CameraCaptureSession.CaptureCallback mCameraCaptureCallback =
            new CameraCaptureSession.CaptureCallback() {
        @Override
        public void onCaptureStarted(CameraCaptureSession session, CaptureRequest request,
                                     long timestamp, long frameNumber) {
            super.onCaptureStarted(session, request, timestamp, frameNumber);

            if (mState.equals(VPTState.INITIALIZING)) {
                mState.set(VPTState.PREVIEWING);
                mCallbacks.forEach(cb -> cb.onSessionReady());
            }
        }

        @Override
        public void onCaptureProgressed(CameraCaptureSession session,
                                        CaptureRequest request,
                                        CaptureResult partialResult) {
            super.onCaptureProgressed(session, request, partialResult);
        }

        @Override
        public void onCaptureCompleted(CameraCaptureSession session,
                                       CaptureRequest request,
                                       TotalCaptureResult result) {
            super.onCaptureCompleted(session, request, result);

            long frameNumber = result.getFrameNumber();
            Long vptFrameNumber = result.get(CameraCaptureKeys.VPT_PROCESSED_FRAME_NUMBER);
            Long timestamp = result.get(CaptureResult.SENSOR_TIMESTAMP);

            if (timestamp == null) {
                timestamp = -1L;
            }
            long delta = timestamp - mLastResultTs;
            mLastResultTs = timestamp;

            if (vptFrameNumber != null) {
                Log.i(TAG, "onCaptureCompleted - ProcessedFrameNumber, frameNumber=" + frameNumber +
                        ", vptFrameNum=" + vptFrameNumber +
                        ", timestamp=" + timestamp +
                        ", delta=" + delta);
            } else {
                Log.i(TAG, "onCaptureCompleted, frameNumber=" + frameNumber);
            }

            // If the user has pressed record, and we haven't yet seen a frame
            // with capture timestamp after record was pressed, set it here.
            if (mMediaRecorderStartTime != 0 && mBaseTimestamp == 0
                    && timestamp >= mMediaRecorderStartTime) {
                mBaseTimestamp = timestamp;
                mSessionInfo.set(Stats.VPTBaseTimestamp, mBaseTimestamp);
            }

            byte[] classifications = result.get(CameraCaptureKeys.VPT_CLASSIFICATIONS);
            if (classifications != null) {
                mSessionInfo.set(Stats.VPTFirstClassificationResultTime);
                // TODO: Due to CHS batch mode, we may get this callback
                // multiple times within a batch, containing the same data each
                // time. We can probably remedy this by using a second metadata
                // which sends the frame for which this is valid for? The only
                // problem is that frame number in HAL does not match the frame
                // number that the application sees. It would probably be
                // better to look at the first timestamp in the classification
                // result and see if its been seen before or not. If it has not
                // been seen then we can treat the results as something valid,
                // otherwise we should throw them out.

                if (handleClassificationMetadata(classifications) > 0) {
                    mCallbacks.forEach(cb -> cb.onClassificationResultReceived());
                }

                if (mMediaRecorderPauseTime != 0 &&
                        mState.equals(VPTState.WAIT_RESULTS) &&
                        mFirstTimestampInPreviousBatch > mMediaRecorderPauseTime) {
                    // TODO: make all of this state transition and UI updating cleaner
                    mState.set(VPTState.ALL_RESULTS_RECEIVED);
                    mCallbacks.forEach(cb -> cb.onAllClassificationResultsReceived());
                }
            }
        }

        @Override
        public void onCaptureFailed(CameraCaptureSession session,
                                    CaptureRequest request,
                                    CaptureFailure failure) {
            super.onCaptureFailed(session, request, failure);
            Log.i(TAG, "onCaptureFailed(), frameNumber=" + failure.getFrameNumber());
        }

        @Override
        public void onCaptureSequenceCompleted(CameraCaptureSession session,
                                               int sequenceId,
                                               long frameNumber) {
            super.onCaptureSequenceCompleted(session, sequenceId, frameNumber);
            Log.i(TAG, "onCaptureSequenceCompleted, sequenceId=" +
                    sequenceId + ", frameNumber=" + frameNumber);
        }

        @Override
        public void onCaptureSequenceAborted(CameraCaptureSession session, int sequenceId) {
            super.onCaptureSequenceAborted(session, sequenceId);
            Log.i(TAG, "onCaptureSequenceAborted, sequenceId=" + sequenceId);
        }

        @Override
        public void onCaptureBufferLost(CameraCaptureSession session, CaptureRequest request,
                                        Surface target, long frameNumber) {
            super.onCaptureBufferLost(session, request, target, frameNumber);
        }
    };

    protected void setupMediaRecorder(VPTSessionConfiguration config) throws IOException {
        logEvent("setupMediaRecorder()");
        mRecorder = new MediaRecorder();
        configureMediaRecorder(config);
    }

    protected void configureMediaRecorder(VPTSessionConfiguration config) throws IOException {

        // Setup sources (must be done first)
        mRecorder.setVideoSource(MediaRecorder.VideoSource.SURFACE);
        // mRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);

        // Output settings
        mRecorder.setOutputFormat(MediaRecorder.OutputFormat.MPEG_4);
        mRecorder.setOutputFile(mVideoFile.getParcelFileDescriptor().getFileDescriptor());

        // Camera settings
        mRecorder.setCaptureRate(config.captureRate);

        // Video settings
        mRecorder.setVideoFrameRate(config.videoRate);
        mRecorder.setVideoSize(config.width, config.height);
        mRecorder.setVideoEncoder(config.videoEncoder);
        mRecorder.setVideoEncodingBitRate(getVideoBitRate(config));
        mRecorder.setOrientationHint(getMediaRecorderOrientation(config));

        mRecorder.prepare();
        mVideoSurface = mRecorder.getSurface();
    }

    protected void closeMediaRecorder() {
        Log.d(TAG, "closeMediaRecorder");
        if (mRecorder != null) {
            mRecorder.release();
            mRecorder = null;
        }
    }

    private void startMediaRecorder() {
        Log.d(TAG, "startMediaRecorder");
        mMediaRecorderStartTime = System.nanoTime();
        mSessionInfo.set(Stats.MediaRecorderStartTime, mMediaRecorderStartTime);
        appendDebugText("startMediaRecorder, startTime=" + mMediaRecorderStartTime);
        mRecorder.start();
    }

    private void pauseMediaRecorder() {
        Log.d(TAG, "pauseMediaRecorder");
        mRecorder.pause();
        mMediaRecorderPauseTime = System.nanoTime();
        mSessionInfo.set(Stats.MediaRecorderPauseTime, mMediaRecorderPauseTime);
        appendDebugText("pauseMediaRecorder, pauseTime=" + mMediaRecorderPauseTime);
    }

    private void stopAndCloseMediaRecorderIfNeeded() {
        if (mRecorder != null && mState.equals(VPTState.FINISHING)) {
            StateTracker<FinishingState> subState =
                    (StateTracker<FinishingState>) mState.getSubState();

            subState.set(FinishingState.STOPPING_MEDIA_RECORDER);
            if (mState.getPreviousState() == VPTState.RECORDING ||
                    mState.getPreviousState() == VPTState.WAIT_RESULTS ||
                    mState.getPreviousState() == VPTState.ALL_RESULTS_RECEIVED) {
                stopMediaRecorder();
            }

            subState.set(FinishingState.CLOSING_MEDIA_RECORDER);
            closeMediaRecorder();
        }
    }

    private void stopMediaRecorder() {
        Log.d(TAG, "stopMediaRecorder");
        mSessionInfo.set(Stats.MediaRecorderStopTime, System.nanoTime());

        mSessionInfo.set(Stats.DEBUG_STOP_MEDIA_RECORDER_START);
        mRecorder.stop();
        mSessionInfo.set(Stats.DEBUG_STOP_MEDIA_RECORDER_END);
        mMediaRecorderStartTime = 0;
        mMediaRecorderPauseTime = 0;

        mVideoFile.closeFileDescriptor();
        mVideoFile = null;
    }

    private int getVideoBitRate(VPTSessionConfiguration config) {
        return (int)80e6;
    }

    private int getMediaRecorderOrientation(VPTSessionConfiguration config) {
        // sensor orientation is relative to device's natural orientation
        int sensor = config.sensorOrientation;

        // current rotation of the display relative to the device's natural orientation
        int deviceRotation = 0;
        switch (config.deviceOrientation) {
            case Surface.ROTATION_0:    deviceRotation = 0;     break;
            case Surface.ROTATION_90:   deviceRotation = 90;    break;
            case Surface.ROTATION_180:  deviceRotation = 180;   break;
            case Surface.ROTATION_270:  deviceRotation = 270;   break;
        }

        // the current orientation of the display
        int currentOrientation = config.displayOrientation;

        // figure out the device's natural orientation
        int nativeOrientation = Configuration.ORIENTATION_LANDSCAPE;
        boolean isRotatedBy90 = deviceRotation == 90;
        boolean isRotatedBy270 = deviceRotation == 270;
        boolean isRotatedBy90Or270 =  isRotatedBy90 || isRotatedBy270;
        if (currentOrientation == Configuration.ORIENTATION_LANDSCAPE && isRotatedBy90Or270 ||
                currentOrientation == Configuration.ORIENTATION_PORTRAIT && !isRotatedBy90Or270) {
            nativeOrientation = Configuration.ORIENTATION_PORTRAIT;
        }

        int rotation = sensor;
        if (currentOrientation != nativeOrientation) {
            // need to change
            rotation = (360 + 180 + sensor - deviceRotation) % 360;
            if (config.isRearCamera) {
                rotation = (360 + sensor - deviceRotation) % 360;
            }
        }

        Log.d(TAG, "orientation, sensor=" + sensor +
                ", deviceRotation=" + deviceRotation +
                ", native=" + nativeOrientation +
                ", current=" + currentOrientation +
                ", rotation=" + rotation);

        return rotation;
    }

    private int handleClassificationMetadata(byte[] data) {

        int NUM_CLASSES_PER_FRAME = 2;

        // Order matters in the following calls to the byte buffer
        // since the buffer here must be read in the same manner that
        // it is backed by the HAL.
        ByteBuffer bb = ByteBuffer.wrap(data);
        bb.order(ByteOrder.LITTLE_ENDIAN);
        int numFrames = bb.getInt();

        ClassificationBatch b = new ClassificationBatch();
        List<ClassificationResult> results = new Vector<>();

        for (int i = 0; i < numFrames; i++) {
            boolean dspValid = bb.getInt() != 0;
            long ts = bb.getLong();
            int numValidClasses = bb.getInt();

            String logitStr = "";

            for (int j = 0; j < NUM_CLASSES_PER_FRAME; j++) {
                // The size of each frame payload is fixed, and we will always
                // receive NUM_CLASSES_PER_FRAME logits per frame. However the
                // HAL may choose to send us less actual valid logits.  The HAL
                // informs us of the number of valid frames in numValidClasses.
                // But since we have a fixed size payload, we need to move the
                // byte buffer pointer along regardless.
                int id = bb.getInt();
                float logit = bb.getFloat();

                if (j < numValidClasses) {
                    ClassificationResult currentResult =
                            new ClassificationResult(Integer.toString(id), logit);
                    if (!results.contains(currentResult)) {
                        results.add(currentResult);
                    }
                }

                logitStr = logitStr + "[" + id + "/" + logit + "]";
            }

            if (!dspValid) {
                Log.i(TAG, "occ: frame=[" + (i + 1) + "/" + numFrames + "] dspValid=" + dspValid +
                        ", ts=" + ts + ", numValidClasses=" + numValidClasses + " " + logitStr);
                continue;
            }

            if (i == 0) {
                long firstTimestampInPreviousBatch = mFirstTimestampInPreviousBatch;
                mFirstTimestampInPreviousBatch = ts;

                if (ts == firstTimestampInPreviousBatch) {
                    // Look at the comment when handleClassificationMetadata is
                    // called to see why we need to do this.
                    break;
                }
            }

            boolean appValid = true;
            if (mMediaRecorderStartTime == 0 || ts < mMediaRecorderStartTime) {
                // mMediaRecorderStartTime == 0 means that the client hasn't
                // started a recording session yet

                // ts < mMediaRecorderStartTime means that the client has
                // started recording, but this result is for frames before
                // the start of recording.
                appValid = false;
            } else if (mMediaRecorderPauseTime != 0 && ts > mMediaRecorderPauseTime) {
                // means that this classification result comes after the
                // end of the video stream
                appValid = false;
            }

            long newTs = (!appValid || mBaseTimestamp == 0) ? 0 : ts - mBaseTimestamp;

            Frame f = new Frame(i);
            f.setVideoTimestamp(newTs);
            f.setSystemTimestamp(ts);
            f.setDataValid(appValid ? 1 : 0);
            b.addFrame(f);

            Log.i(TAG, "occ: frame=[" + (i + 1) + "/" + numFrames + "] dspValid=" + dspValid +
                    ", aValid=" + appValid + ", ts=" + ts + ", newTs=" + newTs +
                    ", numValidClasses=" + numValidClasses + " " + logitStr);

        }

        b.setClassificationResults(results);
        if (!b.getFrames().isEmpty()) {
            if (mClassificationResults.getBatches().isEmpty()) {
                mSessionInfo.set(Stats.VPTFirstValidResult);
            }
            mClassificationResults.addBatch(b);
        }

        return b.getFrames().size();
    }

    private void logEvent(String log) {
        LogHelper.logEvent(TAG, log);
    }

    private void appendDebugText(String log) {
        LogHelper.logDebug(TAG, log);
    }
}
