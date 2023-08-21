/**
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file VPTCaptureActivity.java
 */
package com.qualcomm.qti.mmca.vpt;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.qualcomm.qti.mmca.camera.CameraCapabilities;
import com.qualcomm.qti.mmca.camera.HFRConfiguration;
import com.qualcomm.qti.mmca.util.ActivityHelper;
import com.qualcomm.qti.mmca.util.AlertDialogHelper;
import com.qualcomm.qti.mmca.util.LogHelper;
import com.qualcomm.qti.mmca.util.VideoFile;
import com.qualcomm.qti.mmca.vpt.classification.Metadata;
import com.qualcomm.qti.mmca.vpt.classification.SessionInfo;
import com.qualcomm.qti.ssmeditor.R;
import com.qualcomm.qti.ssmeditor.ssmeditor.SSMEditorActivity;

import java.io.IOException;
import java.util.List;

public class VPTCaptureActivity extends Activity implements LogHelper.LogListener {

    private static int WIDTH = 1280;
    private static int HEIGHT = 720;
    private static int FPS = 120;
    private static boolean REAR_CAMERA = true;

    private static final String TAG = "VPTCaptureActivity";

    // UI
    private Button mRecordButton;
    private TextView mDebugText;
    private TextView mStaticDebugText;

    // Camera
    List<HFRConfiguration> mHFRConfigs;

    // Display
    private SurfaceView mPreviewSurfaceView;
    private Surface mPreviewSurface;

    // Encoder
    private VPTState mVPTState;

    // VPT Specific
    private SessionInfo mSessionInfo;
    private VPTSessionConfiguration mConfig = null;
    private VideoFile mVideoFile = null;
    protected Metadata mClassificationResults = new Metadata();
    private static final int NUM_LEADING_INVALID_BATCHES = 10;

    private VPTController mVPTController;

    // Debug stats
    private long mResultsReceived = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate()");
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_vpt_capture);

        mPreviewSurfaceView = findViewById(R.id.svPreview);
        mPreviewSurfaceView.getHolder().addCallback(mSurfaceHolderCallback);

        mHFRConfigs = CameraCapabilities.getHFRConfigurations(this);

        mDebugText = findViewById(R.id.debugText);
        mDebugText.setMovementMethod(new ScrollingMovementMethod());

        mStaticDebugText = findViewById(R.id.staticDebugText);

        mRecordButton = findViewById(R.id.recordButton);
        mRecordButton.setOnClickListener(mRecordButtonClickListener);

        mSessionInfo = mClassificationResults.getSessionInfo();

        LogHelper.addListener(this);
        mVPTController = new VPTController(this);
        mVPTController.addStateCallback(mVPTCallback);
    }

    @Override
    protected void onStart() {
        Log.d(TAG, "onStart()");
        super.onStart();
        mVPTController.initialize();
    }

    @Override
    protected void onResume() {
        Log.d(TAG, "onResume()");

        super.onResume();

        if (!checkFilePermissions(true)) {
            return;
        }

        for (HFRConfiguration cfg : mHFRConfigs) {
            if (cfg.width == WIDTH && cfg.height == HEIGHT &&
                    cfg.frameRate == FPS && cfg.isRearCamera == REAR_CAMERA) {
                mConfig = new VPTSessionConfiguration(this, "vpt", cfg);
                mConfig.deviceOrientation = getWindowManager().getDefaultDisplay().getRotation();
                mConfig.displayOrientation = getResources().getConfiguration().orientation;
                mSessionInfo.set(SessionInfo.SESSION_NAME, mConfig.getFilenameNoExtension());
                mSessionInfo.set(SessionInfo.CAPTURE_RATE, Integer.toString(mConfig.captureRate));
                mClassificationResults.setFrameRate(mConfig.captureRate);
                mVideoFile = mConfig.getVideoFile();
                mVideoFile.create();
                break;
            }
        }

        // TODO: setup a list of possible HFR configs that we can run VPT with and let
        // the user decide what they want to do?
        mPreviewSurfaceView.getHolder().setFixedSize(WIDTH, HEIGHT);
        setState(VPTState.INITIALIZING);

        mResultsReceived = 0;
        updateStaticDebugText();
        ActivityHelper.keepScreenOn(this, true);
    }

    @Override
    protected void onPause() {
        Log.d(TAG, "onPause()");
        super.onPause();

        mVPTController.finish(true);

        // if didn't record, delete the file
        if (mConfig != null && // happens when permissions are requested in onResume()
                (getState() == VPTState.INITIALIZING || getState() == VPTState.READY)) {
            Log.d(TAG, "currently recording, deleting the existing file");
            mVideoFile.delete();
        }

        ActivityHelper.keepScreenOn(this, false);
    }

    @Override
    protected void onStop() {
        Log.d(TAG, "onStop()");
        super.onStop();
        mVPTController.destroy();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        int newDisplayRotation = getWindowManager().getDefaultDisplay().getRotation();
        int newDisplayOrientation = getResources().getConfiguration().orientation;

        Log.d(TAG, "new orientation=" + newConfig.orientation +
                ", displayRotation=" + newDisplayRotation +
                ", displayOrientation=" + newDisplayOrientation);

        if (mConfig.deviceOrientation != newDisplayRotation ||
                mConfig.displayOrientation != newDisplayOrientation) {

            try {
                logEvent("updated orientation to " +
                        (newDisplayOrientation == Configuration.ORIENTATION_LANDSCAPE ?
                                "landscape" : "portrait"));
                mConfig.deviceOrientation = newDisplayRotation;
                mConfig.displayOrientation = newDisplayOrientation;
                mVPTController.updateDeviceOrientation();

                mResultsReceived = 0;
                updateStaticDebugText();
            } catch (IllegalStateException e) {
                e.printStackTrace();
            }
        }
    }

    // TODO: write a permission manager to handle commonalities between this and SSM Editor
    private boolean mCameraPermissionGranted = false;
    private boolean mPermissionDeniedPermanently = false;
    private static final int CAMERA_PERMISSION_REQUEST_CODE = 30;
    private boolean checkFilePermissions(boolean requestPermission) {
        mCameraPermissionGranted = false;
        Log.d(TAG, "Requesting file permissions if not granted already");
        if (checkSelfPermission(Manifest.permission.CAMERA)
                != PackageManager.PERMISSION_GRANTED) {
            if (requestPermission) {
                Log.d(TAG, "Requesting camera permission");
                requestPermissions(new String[]{Manifest.permission.CAMERA},
                        CAMERA_PERMISSION_REQUEST_CODE);
                return false;
            }
        } else {
            Log.d(TAG, "Already granted camera permissions");
            mCameraPermissionGranted = true;
        }
        return mCameraPermissionGranted;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        Log.d(TAG, "onRequestPermissionsResult with requestCode=" + requestCode +
                ", grantResults.length=" + grantResults.length);

        boolean permissionDenied = false;
        if (grantResults.length > 0) {
            Log.d(TAG, "grantResults[0]: " + grantResults[0]);
            switch (requestCode) {
                case CAMERA_PERMISSION_REQUEST_CODE: {
                    if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                        Log.d(TAG, "read permission granted");
                        mCameraPermissionGranted = true;
                    } else {
                        if (shouldShowRequestPermissionRationale(Manifest.permission.CAMERA)) {
                            permissionDenied = true;
                            Log.d(TAG, "User has denied permission for now");
                        } else {
                            Log.d(TAG, "User has denied permission permanently");
                            mPermissionDeniedPermanently = true;
                            AlertDialogHelper.showPermissionsDeniedDialog(this,
                                    getString(R.string.permissions_denied));
                        }
                    }
                    break;
                }
            }

            if (!mPermissionDeniedPermanently) {
                if (checkFilePermissions(false)) {
                    // setupActivity();
                    return;
                } else if (permissionDenied) {
                    // If user set this app as the default one for videos,
                    // then it doesn't make sense to leave it as such if permission is denied
                    Log.d(TAG, "Removing app defaults");
                    getPackageManager().clearPackagePreferredActivities(getPackageName());
                    finish();
                }
            }
        }
    }

    VPTController.VPTStateCallback mVPTCallback = new VPTController.VPTStateCallback() {
        @Override
        public void onSessionReady() {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mRecordButton.setText(R.string.start_record);
                }
            });
        }

        @Override
        public void onFinished() {
            finishAndLaunchEditor();
        }

        @Override
        public void onClassificationResultReceived() {
            if (mResultsReceived == 0) {
                logEvent("first classification result received");
            }
            mResultsReceived += 1;
            updateStaticDebugText();
        }

        @Override
        public void onAllClassificationResultsReceived() {
            mVPTController.finish(false);
        }
    };

    private void updateStaticDebugText() {
        final String text = "Classification Batches: " + mResultsReceived;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mStaticDebugText.setText(text);
            }
        });
    }

    private void appendDebugText(final String text) {
        LogHelper.logDebug(TAG, text);
    }

    private void appendDebugTextInternal(final String text) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mDebugText.append("\n" + text);
            }
        });
    }

    SurfaceHolder.Callback mSurfaceHolderCallback = new SurfaceHolder.Callback() {
        @Override
        public void surfaceCreated(SurfaceHolder holder) {

        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            Log.d(TAG, "surfaceChanged, format=" + format + ", width=" +
                    width + ", height=" + height);
            mPreviewSurface = holder.getSurface();

            if (mCameraPermissionGranted) {
                try {
                    mVPTController.configure(mConfig, mPreviewSurface);
                    setState(VPTState.READY);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {

        }
    };

    private void launchEditor(Uri videoUri) {
        Log.d(TAG, "starting editor activity");
        Intent editorIntent = SSMEditorActivity.createIntent(this, videoUri);
        startActivity(editorIntent);
    }

    private void finishAndLaunchEditor() {
        if (getState() != VPTState.WAIT_RESULTS) {
            Log.e(TAG, "finishAndLaunchEditor called in invalid state: " + getState());
            return;
        }

        appendDebugText("saving results: " + mConfig.vptMetadataFilePath);
        Metadata classificationResults = mVPTController.getClassificationResults();
        classificationResults.pruneLeadingInvalidBatches(NUM_LEADING_INVALID_BATCHES);
        try {
            classificationResults.writeToFileAsJSON(mConfig.vptMetadataFilePath);
        } catch (IOException e) {
            e.printStackTrace();
        }

        mVideoFile.publish();
        launchEditor(mVideoFile.getUri());
    }

    @Override
    public void onDebugLogAvailable(String tag, String message) {
        Log.d(tag, message);
        appendDebugTextInternal(message);
    }

    @Override
    public void onEventLogAvailable(String tag, final String message) {
        final String msg = "VPT-EVENT: " + message;
        Log.d(tag, msg);
        appendDebugTextInternal(msg);
    }

    enum VPTState {
        INITIALIZING,
        READY,
        RECORDING,
        WAIT_RESULTS,
        FINISHING,
    }

    private void logEvent(String log) {
        LogHelper.logEvent(TAG, log);
    }

    private void setState(VPTState state) {
        mVPTState = state;
        logEvent("act_state=" + state);
    }

    private VPTState getState() {
        return mVPTState;
    }

    Button.OnClickListener mRecordButtonClickListener = new Button.OnClickListener() {
        @Override
        public void onClick(View v) {
            switch(getState()) {
                case READY:
                    mVPTController.startRecording();
                    mRecordButton.setText(R.string.stop_record);
                    setState(VPTState.RECORDING);
                    break;
                case RECORDING:
                    mRecordButton.setText(R.string.finishing);
                    setState(VPTState.WAIT_RESULTS);
                    mVPTController.stopRecording();
                    break;
            }
        }
    };
}
