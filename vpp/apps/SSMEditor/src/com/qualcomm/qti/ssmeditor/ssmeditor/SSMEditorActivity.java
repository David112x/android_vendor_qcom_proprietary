/**
 * Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file SSMEditorActivity.java
 */
/**
 * Not a Contribution.
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

import android.Manifest;
import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.AssetFileDescriptor;
import android.content.res.Configuration;
import android.graphics.Paint;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.util.Log;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.FrameLayout;
import android.widget.FrameLayout.LayoutParams;
import android.widget.ImageButton;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import com.qualcomm.qti.mmca.ssmeditor.FRCSegment;
import com.qualcomm.qti.mmca.ssmeditor.FRCSegmentList;
import com.qualcomm.qti.mmca.ssmeditor.LabelledProbabilitySegment;
import com.qualcomm.qti.mmca.ssmeditor.SSMVideoFile;
import com.qualcomm.qti.mmca.ssmeditor.Segment;
import com.qualcomm.qti.mmca.ssmeditor.SegmentList;
import com.qualcomm.qti.mmca.util.ActivityHelper;
import com.qualcomm.qti.mmca.vpt.VPTFileSystem;
import com.qualcomm.qti.mmca.vpt.classification.ClassificationPostProcessor;
import com.qualcomm.qti.mmca.vpt.classification.Metadata;
import com.qualcomm.qti.mmca.vpt.postproc.ClassificationDefinition;
import com.qualcomm.qti.ssmeditor.R;
import com.qualcomm.qti.mmca.ssmeditor.LabelledProbabilitySegmentList;
import com.qualcomm.qti.ssmeditor.decoder.Decoder;
import com.qualcomm.qti.ssmeditor.util.JsonLoader;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class SSMEditorActivity extends Activity implements
        SurfaceHolder.Callback,
        SeekBar.OnSeekBarChangeListener {

    // Logging
    private static final String TAG = "SSMEditorActivity";

    // Labels
    private static final String LABEL_LP_SEGMENT_LIST = "LPSegmentList";
    private static final String LABEL_FRC_SEGMENT_LIST = "FRCSegmentList";

    private static final String LABEL_INTENT_LP_SEGMENT_LIST = "IntentLPSegmentList";
    private static final String LABEL_INTENT_FRC_SEGMENT_LIST = "IntentFRCSegmentList";

    private static final String LABEL_SEEK_TIME = "SeekTime";
    private static final String LABEL_INTERP_SWITCH_CHECKED = "InterpSwitchChecked";
    private static final String LABEL_WAS_PAUSED = "WasPaused";

    // Permissions
    private static final int READ_EXTERNAL_STORAGE_REQUEST_CODE = 10;
    private boolean mReadGranted = false;
    private boolean mPermissionDeniedPermanently = false;

    // Codec related
    private SSMDecoder mSSMDecoder;
    private long mSavedSeekTimeUs = 0;
    private boolean mWasPaused = true;
    private Surface mDisplaySurface = null;
    private Uri mInputUri = null;
    private AssetFileDescriptor mDataSource = null;
    private boolean mDonePlaying = false;
    private SSMVideoFile mVideoFile = null;

    // Manages slowdown segment logic
    private LabelledProbabilitySegmentList mLPSegmentList;
    private FRCSegmentList mFRCSegmentList;
    private final Object mStateUiLock = new Object();

    // SurfaceHolder callback
    private boolean mSurfaceDoneSetup = false;
    private int mSurfaceHeight = 0;
    private int mSurfaceWidth = 0;

    // View references
    private SurfaceView mSurfaceView;
    private SegmentsView mSegmentsView;
    private SeekBar mSeekBar;
    private ImageButton mMediaButton;
    private ProgressBar mLoadingWheel;
    private View mPreviewCover;
    private Switch mInterpSwitchButton;
    private int[] mOverlayUiIds = {R.id.mediaButton};
    private TextView mLogitPercent;
    private ProgressBar mLogitProgress;

    // Saved button state
    private Boolean mSavedInterpSwitchState = false;

    // Check if error dialog is already showing
    private boolean mDialogShowing = false;

    // Post Proc Configuration
    private int mInterestingThreshold =
            (int)(100 * ClassificationDefinition.DEFAULT_INTERESTING_THRESHOLD);
    private File mMetaFile;

    public static Intent createIntent(Context ctx, Uri videoUri) {
        Intent intent = new Intent(ctx, SSMEditorActivity.class);
        intent.setData(videoUri);
        return intent;
    }

    // Updates from SegmentList
    private SegmentList.OnSegmentsChangedListener<FRCSegment> mSegmentsChangedListener =
            new SegmentList.OnSegmentsChangedListener<FRCSegment>() {
                public void onSegmentsChanged(List<? extends FRCSegment> segments) {
                    Log.d(TAG, "onSegmentsChanged callback received: Updating outline");
                    configureSegmentView(mFRCSegmentList);
                    setSlowZoneDisplay();
                }
            };

    private SegmentsView.OnChangeSegmentListener mChangeSegmentListener =
            new SegmentsView.OnChangeSegmentListener() {
                // Updates from UI which responds to user request to add segment
                public void onAddSegment(long startTs, long endTs) {
                    Log.d(TAG, "onAddSegment callback received: Add segment to manager");

                    FRCSegment.Interp interp = FRCSegment.Interp.INTERP_2X;
                    if (mInterpSwitchButton.isChecked()) {
                        interp = FRCSegment.Interp.INTERP_4X;
                    }
                    mFRCSegmentList.addSegment(new FRCSegment(startTs, endTs, interp), true);
                }

                // Updates from UI which responds to user requests to edit existing segments
                public void onEditSegment(Segment oldSegment, Segment newSegment) {
                    Log.d(TAG, "onEditSegment callback received: Edit segment to manager");
                    mFRCSegmentList.editSegment(oldSegment.getStartTime(),
                            newSegment.getStartTime(), newSegment.getEndTime());
                }
            };

    private Decoder.OnPlaybackChangedListener mPlaybackChangedListener =
            new Decoder.OnPlaybackChangedListener() {
                // Updates UI based on changed playback state
                public void onPlaybackProgress(int pct, long curTime, boolean isSeeking) {
                    if (mSSMDecoder.getCurrentState() == Decoder.State.PLAYING) {
                        updatePlaybackUi(pct, curTime, isSeeking);
                    }
                }
                // When decoder detects issue with video, show error dialog
                public void onErrorFormat(Decoder.ErrorCode err,
                        int maxWidth, int maxHeight) {
                    showInputErrorDialog(err, maxWidth, maxHeight);
                }
                // Detect when video has finished, then update button to reflect state
                public void onPlaybackEnd() {
                    mDonePlaying = true;
                    updateButtonOverlay(false, true);
                }
               // Detect when playing and keep screen on for that duration
                public void onIsPlaying(boolean on) {
                    keepScreenOn(on);
                }
            };

    private String getFormattedTimestamp(long ts) {
        return String.format("%02d:%02d:%02d", ts / (60*60),
                ts / 60, ts%60);
    }

    public void setCurTimestamp(long curTimeUs) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                long curTime = curTimeUs;
                if (curTimeUs >= 0) {
                    curTime = curTimeUs / 1000000;
                    TextView curTimeText = findViewById(R.id.currentTimeStamp);
                    curTimeText.setText(getFormattedTimestamp(curTime));
                }
            }
        });
    }

    public long getCurTimeStamp() {
        TextView curTimeText = findViewById(R.id.currentTimeStamp);
        return Long.parseLong(curTimeText.getText().toString());
    }

    public void setDurationTimestamp(long durationUs) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                long duration = 0;
                if (durationUs >= 0) {
                    duration = durationUs / 1000000;
                }
                TextView durationText = findViewById(R.id.endTimeStamp);
                durationText.setText(getFormattedTimestamp(duration));
            }
        });
    }

    public void updatePlaybackUi(int pct, long curTime, boolean isSeeking) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (!mSSMDecoder.isEncoding()) {
                    Log.d(TAG, "Setting progress bar: " + pct);
                    if (!isSeeking) {
                        mSeekBar.setProgress(pct);
                    }
                    setSlowZoneDisplay();
                    setCurTimestamp(curTime);
                    updateProbabilityIndicator(curTime);
                } else {
                    Log.d(TAG, "Setting save progress");
                    setSaveProgress(pct);
                }
            }
        });
    }

    private void configureSegmentView(FRCSegmentList sl) {
        if (mSegmentsView == null) {
            Log.e(TAG, "mSegmentsView is null!");
            return;
        }

        List<FRCSegment> segments = sl.getSegments();
        List<SegmentsView.SegmentDescriptor> filteredList = new ArrayList<>();
        segments.forEach(s -> {
            if (!s.isBypass()) {

                Paint segmentPaint = new Paint();
                segmentPaint.setStyle(Paint.Style.FILL);
                switch (s.getInterpFactor()) {
                    case INTERP_2X:
                        segmentPaint.setColor(getResources().getColor(R.color.blue));
                        break;
                    case INTERP_4X:
                        segmentPaint.setColor(getResources().getColor(R.color.pink));
                        break;
                }

                SegmentsView.SegmentDescriptor sd = new SegmentsView.SegmentDescriptor();
                sd.segment = s;
                sd.paintSegment = segmentPaint;

                filteredList.add(sd);
            }
        });

        mSegmentsView.setFinalTimestamp(getClipDuration());
        mSegmentsView.setSegments(filteredList);
    }

    private void updateProbabilityIndicator(long ts) {
        int percent = mLPSegmentList.getProbabilityAtTime(ts);
        Log.d(TAG, "ts=" + ts + " is percent=" + percent);
        int scaledPercent = (int)(percent * 0.75);
        mLogitProgress.setProgress(scaledPercent, true);
        mLogitPercent.setText(percent + "%");
    }

    private String getSpeedText(FRCSegment.Interp factor) {
        String speedString = "1x";
        if (mSSMDecoder.isUsingVPP()) {
            if (factor == FRCSegment.Interp.INTERP_2X) {
                speedString = String.valueOf(2 * mSSMDecoder.getSlowdownMultiple()) + "x";
            } else if (factor == FRCSegment.Interp.INTERP_4X) {
                speedString = String.valueOf(4 * mSSMDecoder.getSlowdownMultiple()) + "x";
            }
        } else {
            if (factor != FRCSegment.Interp.BYPASS) {
                speedString = String.valueOf(mSSMDecoder.getSlowdownMultiple() + "x");
            }
        }
        return speedString;
    }

    public void setSlowZoneDisplay() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (!mSSMDecoder.isEncoding()) {
                    long curTime = mSSMDecoder.getLastRenderedFrameTimeUs();
                    TextView speedText = findViewById(R.id.playbackSpeedText);
                    speedText.setText(getSpeedText(mSSMDecoder.getCurrentInterpFactor()));

                    TextView classLabelText = findViewById(R.id.classLabelText);

                    if (mSSMDecoder.isRenderingSlowMotion()) {
                        mSegmentsView.setProgress(curTime);
                        classLabelText.setVisibility(View.VISIBLE);
                        classLabelText.setText(mLPSegmentList.getLabelAtTime(curTime));
                    } else {
                        mSegmentsView.setProgress(-1);
                        classLabelText.setVisibility(View.GONE);
                    }
                }
            }
        });
    }

    private boolean checkFilePermissions(boolean requestPermission) {
        mReadGranted = false;

        Log.d(TAG, "Requesting file permissions if not granted already");
        if (checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            if (requestPermission) {
                Log.d(TAG, "Getting read permission");
                requestPermissions(new String[]{Manifest.permission.READ_EXTERNAL_STORAGE},
                        READ_EXTERNAL_STORAGE_REQUEST_CODE);
                return false;
            }
        } else {
            Log.d(TAG, "Already granted read");
            mReadGranted = true;
        }

        return mReadGranted;
    }

    private void showPermissionsDenyDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);

        String message = getString(R.string.permissions_denied);

        DialogInterface.OnClickListener pListener = new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
                Intent settingsLaunch = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
                Uri settingsUri = Uri.fromParts("package", getPackageName(), null);
                settingsLaunch.setData(settingsUri);
                startActivityForResult(settingsLaunch, 1);
            }
        };

        DialogInterface.OnClickListener cListener = new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
                finish();
            }
        };

        DialogInterface.OnDismissListener dListener = new DialogInterface.OnDismissListener() {
            public void onDismiss(DialogInterface dialog) {
                finish();
            }
        };

        builder.setMessage(message)
                .setTitle("Permissions Denied")
                .setPositiveButton("OK", pListener)
                .setNegativeButton("Close", cListener)
                .setOnDismissListener(dListener);

        AlertDialog dialog = builder.create();
        dialog.show();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
            String[] permissions, int[] grantResults) {
        Log.d(TAG, "onRequestPermissionsResult with requestCode: " + requestCode);
        Log.v(TAG, "grantResults size: " + grantResults.length);

        boolean permissionDenied = false;

        if (grantResults.length > 0) {
            Log.d(TAG, "grantResults[0]: " + grantResults[0]);
            switch (requestCode) {
                case READ_EXTERNAL_STORAGE_REQUEST_CODE: {
                    if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                        Log.d(TAG, "read permission granted");
                        mReadGranted = true;
                    } else {
                        if (shouldShowRequestPermissionRationale(Manifest.permission.READ_EXTERNAL_STORAGE)) {
                            permissionDenied = true;
                            Log.d(TAG, "User has denied permission for now");
                        } else {
                            Log.d(TAG, "User has denied permission permanently");
                            mPermissionDeniedPermanently = true;
                            showPermissionsDenyDialog();
                        }
                    }
                    break;
                }
            }

            if (!mPermissionDeniedPermanently) {
                if (checkFilePermissions(false)) {
                    setupActivity();
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

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate() called");

        mFRCSegmentList = new FRCSegmentList();
        mLPSegmentList = new LabelledProbabilitySegmentList();

        // Get Intent that triggered this activity
        mInputUri = getIntent().getData();
        mMetaFile = VPTFileSystem.getMetadataFileForVideo(this, mInputUri);

        boolean intentHasFRCSegments = getIntent().hasExtra(LABEL_INTENT_FRC_SEGMENT_LIST);
        boolean intentHasLPSegments = getIntent().hasExtra(LABEL_INTENT_LP_SEGMENT_LIST);

        if (checkFilePermissions(false)) {
            super.onCreate(savedInstanceState);
            if (savedInstanceState != null) {
                loadSegmentsFromSavedState(savedInstanceState);
            } else if (intentHasFRCSegments || intentHasLPSegments) {
                loadSegmentsFromIntent(getIntent());
            } else if (mMetaFile != null) {
                loadSegmentsFromJson(mMetaFile);
            }
        } else {
            super.onCreate(null);
        }
    }

    private void loadSegmentsFromSavedState(Bundle savedInstanceState) {
        mFRCSegmentList = savedInstanceState.getParcelable(LABEL_FRC_SEGMENT_LIST);
        mLPSegmentList = savedInstanceState.getParcelable(LABEL_LP_SEGMENT_LIST);
        mSavedSeekTimeUs = savedInstanceState.getLong(LABEL_SEEK_TIME);
        mSavedInterpSwitchState = savedInstanceState.getBoolean(
                LABEL_INTERP_SWITCH_CHECKED);
        mWasPaused = savedInstanceState.getBoolean(LABEL_WAS_PAUSED);
    }

    private void loadSegmentsFromJson(File metadataFile) {
        ClassificationPostProcessor.PostProcResult ppr =
                executePostProc(metadataFile);

        loadLPSegments(ppr.lpSegments);
        loadFRCSegments(ppr.frcSegments);
    }

    private void loadSegmentsFromIntent(Intent intent) {
        if (intent == null) {
            return;
        }

        loadLPSegments(intent.getParcelableArrayListExtra(LABEL_INTENT_LP_SEGMENT_LIST));
        loadFRCSegments(intent.getParcelableArrayListExtra(LABEL_INTENT_FRC_SEGMENT_LIST));
    }

    private void loadLPSegments(ArrayList<LabelledProbabilitySegment> lpSegments) {
        mLPSegmentList.removeSegments();
        mLPSegmentList.addSegments(lpSegments);
    }

    private void loadFRCSegments(ArrayList<FRCSegment> frcSegments) {
        mFRCSegmentList.removeSegments();
        mFRCSegmentList.addSegments(frcSegments);
    }

    private ClassificationPostProcessor.PostProcResult executePostProc(File metadataFile) {

        String metadataJson = JsonLoader.getJsonFromFile(metadataFile);
        Metadata metadata = MetadataJson.getMetadataFromJson(metadataJson);
        return new ClassificationPostProcessor().execute(metadata);
    }

    private void createDecoder(boolean isEncoding) {
        try {
            mDataSource = getContentResolver().openAssetFileDescriptor(mInputUri, "r");

            // Create instance of ssm decoder to handle playback
            if (mDataSource != null) {
                mSSMDecoder = new SSMDecoder(mFRCSegmentList);

                if (!isEncoding) {
                    // Register owner of this class as a listener
                    mSSMDecoder.registerOnPlaybackChangedListener(mPlaybackChangedListener);
                }
                try {
                    mSSMDecoder.initialize(isEncoding ? 0 : mSavedSeekTimeUs, mDataSource);
                } catch (IllegalArgumentException | IOException e) {
                    e.printStackTrace();
                    showInputErrorDialog(Decoder.ErrorCode.ERROR_INVALID_LAYER_ID, 0, 0);
                }
            } else {
                showInputErrorDialog(Decoder.ErrorCode.ERROR_INVALID_FILE, 0, 0);
            }

            setDurationTimestamp(mSSMDecoder.getDuration());
        } catch (FileNotFoundException e) {
            Log.e(TAG, "assetfiledescript error: " + e);
        }
    }

    private void destroyDecoder() {
        Log.d(TAG, "Stopping decoder");
        mSSMDecoder.destroy();

        if (mDataSource != null) {
            try {
                mDataSource.close();
            } catch (IOException e) {
                Log.e(TAG, "mDataSource IOException: " + e);
            }
        }

        mSSMDecoder.unregisterOnPlaybackChangedListener(mPlaybackChangedListener);
    }

    private void animateOverlayVanish(long duration, long delay) {
        for (int i = 0; i < mOverlayUiIds.length; i++) {
            View v = findViewById(mOverlayUiIds[i]);
            if (v.getVisibility() == View.VISIBLE) {
                v.animate()
                    .alpha(0.0f)
                    .setDuration(duration)
                    .setStartDelay(delay)
                    .setListener(new AnimatorListenerAdapter() {
                        @Override
                        public void onAnimationEnd(Animator animation) {
                            super.onAnimationEnd(animation);
                            Log.d(TAG, "setting mediabutton visibility to gone");
                            v.setVisibility(View.GONE);
                        }
                    });
            }
        }
    }

    private void setupActivity() {
        setContentView(R.layout.activity_video_playback);

        // Seekbar for video player
        mSeekBar = findViewById(R.id.seekBar);
        mSeekBar.setOnSeekBarChangeListener(this);

        // Grab thumb size to use for positioning segment boundaries
        GradientDrawable thumb = (GradientDrawable) mSeekBar.getThumb();

        // SegmentsView gets attached to segment manager to display updates to
        // user added slow motion segments
        mSegmentsView = findViewById(R.id.segView);
        mSegmentsView.setThumbOffset(thumb.getIntrinsicWidth() / 2);

        // Register listener for add segment and edit segment from SegmentsView class
        mSegmentsView.registerOnChangeSegmentListener(mChangeSegmentListener);

        // Create and start decoder
        createDecoder(false);

        // Frame layout
        FrameLayout flayout = findViewById(R.id.flayout);

        // Touch on any part of layout brings up Ui for play/pause
        flayout.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (event.getAction() == MotionEvent.ACTION_DOWN) {
                    Log.d(TAG, "Layout down action detected");
                    updateButtonOverlay(true, true);
                    return true;
                }
                return false;
            }
        });

        // Switch button to select interp factor
        mInterpSwitchButton = findViewById(R.id.interpSwitchButton);
        mInterpSwitchButton.setChecked(mSavedInterpSwitchState);
        mInterpSwitchButton.setTextOff(getSpeedText(FRCSegment.Interp.INTERP_2X));
        mInterpSwitchButton.setTextOn(getSpeedText(FRCSegment.Interp.INTERP_4X));

        if (mSSMDecoder.has4xInterp()) {
            if (mInterpSwitchButton.isChecked()) {
                mInterpSwitchButton.setSwitchTextAppearance(this,
                        R.style.SwitchTextWhite);
            }

            mInterpSwitchButton.setOnCheckedChangeListener(
                    new Switch.OnCheckedChangeListener() {
                        public void onCheckedChanged(CompoundButton view,
                                boolean isChecked) {
                            if (isChecked) {
                                mInterpSwitchButton.setSwitchTextAppearance(
                                        SSMEditorActivity.this, R.style.SwitchTextWhite);
                            } else {
                                mInterpSwitchButton.setSwitchTextAppearance(
                                        SSMEditorActivity.this, R.style.SwitchTextBlack);
                            }
                        }
                    });
        } else {
            mInterpSwitchButton.setClickable(false);
        }

        // Button for play/pause
        mMediaButton = findViewById(R.id.mediaButton);

        mMediaButton.setOnClickListener(new ImageButton.OnClickListener() {
            public void onClick(View v) {
                Log.d(TAG, "Media Button clicked; State: " + mSSMDecoder.getCurrentState());
                if (mSSMDecoder.getCurrentState() == Decoder.State.PLAYING) {
                    synchronized (mStateUiLock) {
                        boolean startedPausing = mSSMDecoder.pause();
                        if (startedPausing) {
                            updateButtonOverlay(false, true);
                        }
                    }

                } else if (mSSMDecoder.getCurrentState() == Decoder.State.PAUSED) {
                    synchronized (mStateUiLock) {
                        boolean startedPlaying = mSSMDecoder.play();
                        if (startedPlaying) {
                            updateButtonOverlay(false, false);
                        }
                    }
                }
            }
        });

        // Button to remove all segments
        Button removeSegmentsButton = findViewById(R.id.removeSegmentsButton);
        removeSegmentsButton.setOnClickListener(new TextView.OnClickListener() {
            public void onClick(View v) {
                mFRCSegmentList.removeSegments();
            }
        });

        // Save Button saves a copy of the edited video with slow motion segments
        Button saveButton = findViewById(R.id.saveButton);
        saveButton.setOnClickListener(new TextView.OnClickListener() {
            public void onClick(View v) {
                Log.d(TAG, "Saving video");
                saveVideo();
            }
        });

        mLogitPercent = findViewById(R.id.logitPercentage);
        mLogitProgress = findViewById(R.id.progressBar);
        updateProbabilityIndicator(mSavedSeekTimeUs);

        // Views used during saving
        mPreviewCover = findViewById(R.id.previewCover);
        mLoadingWheel = findViewById(R.id.saveProgressBar);

        // Surfaceview used to show video
        mSurfaceView = findViewById(R.id.surfaceView);
        mSurfaceView.getHolder().addCallback(this);

        int screenWidth = getResources().getDisplayMetrics().widthPixels;
        int screenHeight = getResources().getDisplayMetrics().heightPixels;
        int decoderWidth = mSSMDecoder.getVideoWidth();
        int decoderHeight = mSSMDecoder.getVideoHeight();

        setSurfaceParameters(screenWidth, screenHeight, decoderWidth, decoderHeight);

        Log.d(TAG, "Surface width height " + mSurfaceWidth + " " + mSurfaceHeight);

        mFRCSegmentList.setFinalTimestamp(getClipDuration());
        configureSegmentView(mFRCSegmentList);

        // Set up initial state for video player
        if (mWasPaused) {
            mSSMDecoder.pause();
        } else {
            mSSMDecoder.play();
        }

        updateButtonOverlay(false, mWasPaused);

        // Register listener for segments for segment outline updates
        mFRCSegmentList.registerOnSegmentsChangedListener(mSegmentsChangedListener);
    }

    private int calculateDimFromRatio(int otherDim, int dimBase, int otherDimBase) {
        return (otherDim * dimBase) / otherDimBase;
    }

    private void setSurfaceParameters(int surfaceWidth, int surfaceHeight,
                int decoderWidth, int decoderHeight) {
        Log.d(TAG, "setSurfaceParameters: " + surfaceWidth + " " + surfaceHeight
                + " " + decoderWidth +" " + decoderHeight);

        int adjustedWidth = decoderWidth;
        int adjustedHeight = decoderHeight;

        // Switch height and width if encoded orientation is 90/270
        if (mSSMDecoder.getRotation() == 90 || mSSMDecoder.getRotation() == 270) {
            adjustedWidth = decoderHeight;
            adjustedHeight = decoderWidth;
        }

        mSurfaceWidth = surfaceWidth;
        if (adjustedWidth != 0) {
            mSurfaceHeight = calculateDimFromRatio(mSurfaceWidth, adjustedHeight,
                    adjustedWidth);
        }

        // Fit video the other way
        if (mSurfaceHeight > surfaceHeight) {
            mSurfaceHeight = surfaceHeight;
            if (adjustedHeight != 0) {
                mSurfaceWidth = calculateDimFromRatio(mSurfaceHeight, adjustedWidth,
                        adjustedHeight);
            }
        }
    }

    @Override
    protected void onResume() {
        Log.d(TAG, "onResume() called");
        super.onResume();

        if (!mPermissionDeniedPermanently) {
            if (checkFilePermissions(true)) {
                setupActivity();
            }
        }
    }

    private void restartDecoder(boolean encodeStart) {
        destroyDecoder();
        createDecoder(encodeStart);
    }

    private void savePlaybackState() {
        // Save time to go back to after encoding is done
        mWasPaused = (mSSMDecoder.getCurrentState() == Decoder.State.PAUSED);
        mSavedSeekTimeUs = mSSMDecoder.getLastRenderedFrameTimeUs();
    }

    private void saveVideo() {
        // Save a copy of the edited video with slow motion segments
        showProgressBar();
        setProgressDeterminate();

        savePlaybackState();
        restartDecoder(true);

        SSMEncoder encoder = new SSMEncoder();
        encoder.registerOnEncodeCompleteListener(
                new SSMEncoder.OnEncodeCompleteListener() {
                    @Override
                    public void onEncodeComplete() {
                        Log.d(TAG, "Received onEncodeComplete callback.");
                        onEncodeCompleteCallback();
                    }
                }
        );

        mVideoFile = new SSMVideoFile();
        mVideoFile.setContext(this);
        mVideoFile.create();
        encoder.saveVideo(mVideoFile.openFileDescriptor("rw"), mSSMDecoder);
    }

    private void onEncodeCompleteCallback() {
        restartDecoder(false);

        if (mDisplaySurface == null) {
            Log.e(TAG, "The display surface is null after encoding!");
        }
        updateButtonOverlay(false, mWasPaused);
        mSSMDecoder.configure(mDisplaySurface);
        mSSMDecoder.start();

        // Setting start state to pause
        if (mWasPaused) {
            mSSMDecoder.pause();
        } else {
            mSSMDecoder.play();
        }

        mVideoFile.closeFileDescriptor();
        mVideoFile.publish();
        mVideoFile = null;
        // Inform User
        Toast toast = Toast.makeText(this, "Save successful!", Toast.LENGTH_SHORT);
        toast.show();

        hideProgressBar();
    }

    @Override
    protected void onPause() {
        Log.d(TAG, "onPause() called");
        super.onPause();

        if (checkFilePermissions(false)) {
            mSavedInterpSwitchState = mInterpSwitchButton.isChecked();

            if (mSSMDecoder.isEncoding()) {
                mVideoFile.delete();
                mVideoFile = null;
                Toast.makeText(this, "Save unsuccessful", Toast.LENGTH_SHORT).show();
                hideProgressBar();
            } else {
                savePlaybackState();
            }

            destroyDecoder();

            mFRCSegmentList.unregisterOnSegmentsChangedListener(mSegmentsChangedListener);
            mSegmentsView.unregisterOnChangeSegmentListener(mChangeSegmentListener);

        } else {
            Log.e(TAG, "Permissions denied when onPause() called");
        }
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        if (checkFilePermissions(false)) {
            outState.putParcelable(LABEL_FRC_SEGMENT_LIST, mFRCSegmentList);
            outState.putParcelable(LABEL_LP_SEGMENT_LIST, mLPSegmentList);
            outState.putLong(LABEL_SEEK_TIME, mSSMDecoder.getLastRenderedFrameTimeUs());
            outState.putBoolean(LABEL_INTERP_SWITCH_CHECKED,
                    mInterpSwitchButton.isChecked());
            outState.putBoolean(LABEL_WAS_PAUSED, mWasPaused);
        }
        super.onSaveInstanceState(outState);
    }

    @Override
    public void onConfigurationChanged(Configuration config) {
        super.onConfigurationChanged(config);
        int screenWidth = getResources().getDisplayMetrics().widthPixels;
        int screenHeight = getResources().getDisplayMetrics().heightPixels;
        int decoderWidth = mSSMDecoder.getVideoWidth();
        int decoderHeight = mSSMDecoder.getVideoHeight();

        setSurfaceParameters(screenWidth, screenHeight, decoderWidth, decoderHeight);

        LayoutParams lp = new LayoutParams(mSurfaceWidth, mSurfaceHeight, Gravity.CENTER);
        mSurfaceView.setLayoutParams(lp);
        Log.d(TAG, "Setting surfaceview params width and height");
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        if (mSurfaceDoneSetup == true && (width == mSurfaceWidth) && (height == mSurfaceHeight)) {
            Log.d(TAG, "Start decoder with surface size w: " + width + " h: " + height);
            mDisplaySurface = holder.getSurface();
            mSSMDecoder.configure(mDisplaySurface);
            mSSMDecoder.start();
            mSegmentsView.invalidate();
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (mSurfaceView != null) {
            LayoutParams lp = new LayoutParams(mSurfaceWidth, mSurfaceHeight, Gravity.CENTER);
            mSurfaceView.setLayoutParams(lp);
            Log.d(TAG, "Setting surfaceview params width and height");
            mSurfaceDoneSetup = true;
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        // release everything we grabbed
        destroyDecoder();
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        mSSMDecoder.updateProgress(progress);
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
        Log.d(TAG, "Seekbar touch started");
        boolean startedSeeking = mSSMDecoder.seek();
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        Log.d(TAG, "Seekbar touch stopped");
        mSSMDecoder.endSeek();
    }

    private void showOverlay(boolean toPlay) {
        Log.d(TAG, "showOverlay");
        for (int i = 0; i < mOverlayUiIds.length; i++) {
            findViewById(mOverlayUiIds[i]).clearAnimation();
            findViewById(mOverlayUiIds[i]).animate().cancel();
            findViewById(mOverlayUiIds[i]).setVisibility(View.VISIBLE);
            findViewById(mOverlayUiIds[i]).setAlpha(1.0f);
        }
        animateOverlayVanish(500, 4000);
    }

    private void vanishOverlay() {
        Log.d(TAG, "vanishOverlay");
        for (int i = 0; i < mOverlayUiIds.length; i++) {
            findViewById(mOverlayUiIds[i]).clearAnimation();
            findViewById(mOverlayUiIds[i]).animate().cancel();
            findViewById(mOverlayUiIds[i]).setVisibility(View.GONE);
        }
    }

    public void updateButtonOverlay(boolean wasToggled, boolean toPause) {
        Log.d(TAG, "Update Media Button");
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (wasToggled) {
                    Log.v(TAG, "Media Button Toggled");
                    if (mMediaButton.getVisibility() == View.GONE) {
                        showOverlay(false);
                    } else {
                        vanishOverlay();
                    }

                } else {
                    if (toPause) {
                        mMediaButton.setImageResource(R.drawable.play);
                    } else {
                        mMediaButton.setImageResource(R.drawable.pause);
                    }
                    showOverlay(!toPause);
                }
            }
        });
    }

    public void showInputErrorDialog(Decoder.ErrorCode mode,
            int maxHeight, int maxWidth) {

        if (!mDialogShowing) {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    Log.d(TAG, "Showing Input Error Dialog");
                    // Show message and exit gracefully
                    AlertDialog.Builder builder = new AlertDialog.Builder(
                            SSMEditorActivity.this);

                    String message = getString(R.string.error_generic);

                    if (mode == Decoder.ErrorCode.ERROR_INVALID_FILE) {
                        message = getString(R.string.error_invalid_file);
                    } else if (mode == Decoder.ErrorCode.ERROR_DIMENSIONS) {
                        message = getString(R.string.error_dimensions) + " " + message;
                        message += "\n" + getString(R.string.error_max_res) +
                                " " + Build.DEVICE + ": " + maxHeight +
                                " " + maxWidth +
                                "\n" + getString(R.string.error_min_fps) +
                                " " + Decoder.MIN_FRAME_RATE +
                                "\n" + getString(R.string.error_max_fps) +
                                " " + Decoder.MAX_FRAME_RATE;
                    } else if (mode == Decoder.ErrorCode.ERROR_NO_VALID_VIDEO_TRACK) {
                        message = getString(R.string.error_no_valid_video_track) +
                                " " + message;
                    } else if (mode == Decoder.ErrorCode.ERROR_INVALID_LAYER_ID) {
                        message = getString(R.string.error_invalid_layer_id) +
                                " " + message;
                    }

                    builder.setMessage(message)
                            .setTitle(getString(R.string.error_input_title));

                    builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int id) {
                            finish();
                        }
                    });

                    builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
                        public void onDismiss(DialogInterface dialog) {
                            finish();
                        }
                    });

                    AlertDialog dialog = builder.create();
                    dialog.show();
                    mDialogShowing = true;
                }
            });
        }
    }

    private long getClipDuration() {
        return mSSMDecoder == null ? 0 : mSSMDecoder.getDuration();
    }

    private void setProgressDeterminate() {
        mLoadingWheel.setIndeterminate(false);
        mLoadingWheel.postInvalidate();
        mLoadingWheel.setProgress(0);
    }

    private void setSaveProgress(int percentage) {
        Log.d(TAG, "set save progress pct " + percentage);
        mLoadingWheel.setProgress(percentage);
    }

    private void showProgressBar() {
        showPreviewCover();
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE,
                WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE);
        mLoadingWheel.setVisibility(View.VISIBLE);
    }

    private void hideProgressBar() {
        hidePreviewCover();
        getWindow().clearFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE);
        mLoadingWheel.setVisibility(View.GONE);
    }

    private void showPreviewCover() {
        mPreviewCover.setVisibility(View.VISIBLE);
    }

    private void hidePreviewCover() {
        // Hide the preview cover if need.
        mPreviewCover.setVisibility(View.GONE);
    }

    public void keepScreenOn(boolean on) {
        ActivityHelper.keepScreenOn(this, on);
    }

    public boolean isDonePlaying() {
        return mDonePlaying;
    }
}
