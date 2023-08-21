/**
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file VPPPActivity.java
 */

package com.qualcomm.qti.mmca.vppp;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.res.AssetFileDescriptor;
import android.content.res.Configuration;
import android.graphics.Color;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.GestureDetector;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout.LayoutParams;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.SeekBar;

import com.qualcomm.qti.mmca.util.ActivityHelper;
import com.qualcomm.qti.ssmeditor.QtiApplication;
import com.qualcomm.qti.ssmeditor.decoder.Decoder;
import com.qualcomm.qti.mmca.vppp.ui.ControlChangedListener;
import com.qualcomm.qti.mmca.vppp.ui.ControlsFragment;
import com.qualcomm.qti.mmca.vppp.ui.Direction;
import com.qualcomm.qti.mmca.vppp.ui.ExtensionHolder;

import java.io.FileNotFoundException;
import java.io.IOException;

import org.json.JSONException;

import com.qualcomm.qti.ssmeditor.R;

public class VPPPActivity extends Activity
    implements ControlChangedListener, SurfaceHolder.Callback,
               Decoder.OnPlaybackChangedListener, SeekBar.OnSeekBarChangeListener {

    public static final String TAG = "VPPPActivity";

    private static final String KEY_VM = "controls_activity_vm";
    private final int SHEET_ANIMATION_TIME = 200;

    private LinearLayout mBottomSheet;
    private final int mStartColor = Color.parseColor("#33FFFFFF");
    private final int mEndColor = Color.parseColor("#FFFFFFFF");
    private ControlsFragment mControlsFragment;
    private Button mChangePresetButton;

    private RelativeLayout mVideoControls;
    private ImageButton mPlayButton;
    private SeekBar mSeekBar;
    private SurfaceView mSurfaceView;
    private final Object mStateUiLock = new Object();

    private VPPPActivityViewModel mViewModel;
    private VPPPDecoder mDecoder;
    private AssetFileDescriptor mDataSource;

    private GestureDetector mGestureDetector;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_vppp);

        // Hide the status bar.
        View decorView = getWindow().getDecorView();
        int uiOptions = View.SYSTEM_UI_FLAG_FULLSCREEN;
        decorView.setSystemUiVisibility(uiOptions);

        try {
            createViewModel(savedInstanceState);
        } catch (JSONException e) {
            // No config found
            Log.e(TAG, "No configuration found matching device specs.", e);
            showErrorDialog(R.string.error_no_config);
        } catch (IllegalArgumentException e) {
            // No presets found
            Log.e(TAG, "No presets found for the device config.", e);
            showErrorDialog(R.string.error_no_presets);
        }

        mGestureDetector = new GestureDetector(this, gestureListener);

        mChangePresetButton = findViewById(R.id.btn_preset_change);
        mChangePresetButton.setOnClickListener(presetButtonListener);

        mBottomSheet = findViewById(R.id.bottom_sheet);
        setupControlsSheet();

        mVideoControls = findViewById(R.id.video_controls);
        mPlayButton = findViewById(R.id.media_button);
        mPlayButton.setOnClickListener(mPlayButtonListener);
        mSeekBar = findViewById(R.id.seek_bar);
        mSeekBar.setOnSeekBarChangeListener(this);

        mSurfaceView = findViewById(R.id.video_surface);
        mSurfaceView.getHolder().addCallback(this);

        // Launch the ControlsFragment from its saved state or create a new one using the dialog.
        // NOTE: mControlsFragment may be null at the end of onCreate().
        if (savedInstanceState != null && mViewModel.isPresetSelected()) {
            mControlsFragment = (ControlsFragment) getFragmentManager()
                    .getFragment(savedInstanceState, ControlsFragment.TAG);
        } else {
            showPresetDialog();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        createDecoder();
        updateSurfaceParams();
        resumePlayback();
    }

    @Override
    protected void onPause() {
        super.onPause();
        destroyDecoder();
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        Log.v(TAG, "onSaveInstanceState");

        // Save the state of the VM
        outState.putParcelable(KEY_VM, mViewModel);

        // Save the fragment's state
        if (mControlsFragment != null) {
            getFragmentManager().putFragment(outState,
                    ControlsFragment.TAG, mControlsFragment);
        }
    }

    @Override
    public void onConfigurationChanged(Configuration configuration) {
        super.onConfigurationChanged(configuration);

        updateSurfaceParams();
    }

    private void createViewModel(Bundle savedState) throws JSONException,
            IllegalArgumentException {
        if (savedState != null) {
            mViewModel = (VPPPActivityViewModel) savedState.getParcelable(KEY_VM);
            mViewModel.reload((QtiApplication) getApplication());
        } else {
            Uri videoUri = getIntent().getData();
            Log.v(TAG, "video uri: " + videoUri.toString());
            mViewModel = new VPPPActivityViewModel((QtiApplication) getApplication(),
                    Build.BOARD, Build.VERSION.SDK_INT, videoUri);
        }
    }

    /**
     * Create a new ControlsFragment from scratch.
     */
    private void createControlsFragment() {
        mControlsFragment = ControlsFragment.newInstance(mViewModel.getSelectedPreset());
        mControlsFragment.setRetainInstance(true);
        getFragmentManager()
            .beginTransaction()
            .add(R.id.controls_holder, mControlsFragment, ControlsFragment.TAG)
            .commit();
        collapseSheet();
    }

    private void createDecoder() {
        try {
            mDataSource = getContentResolver()
                    .openAssetFileDescriptor(mViewModel.getVideoUri(), "r");

            if (mDataSource != null) {
                mDecoder = new VPPPDecoder();
                mDecoder.registerOnPlaybackChangedListener(this);
                mDecoder.initialize(mViewModel.getCurrentVideoTime(), mDataSource);
            } else {
                showErrorDialog(R.string.error_invalid_file);
            }
        } catch (FileNotFoundException e) {
            Log.e(TAG, "Invalid file path.", e);
            showErrorDialog(R.string.error_invalid_file);
        } catch (IllegalArgumentException | IOException e) {
            Log.e(TAG, "Invalid file for VPP Player.", e);
            showErrorDialog(R.string.error_invalid_file);
        }
    }

    private void showPresetDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(VPPPActivity.this);
        builder.setTitle("Select Controls Preset");

        builder.setSingleChoiceItems(mViewModel.getPresetDisplayNames(),
                mViewModel.getSelectedPresetIndex(), null);

        builder.setPositiveButton("Done", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                // which refers to the button clicked, not the selected index
                int selectedIndex = ((AlertDialog) dialog).getListView().getCheckedItemPosition();

                if (selectedIndex != mViewModel.getSelectedPresetIndex()) {
                    mViewModel.setSelectedPresetIndex(selectedIndex);

                    if (mControlsFragment != null) {
                        mControlsFragment.updateData(mViewModel.getSelectedPreset());

                        // Recreate the decoder to enable the algorithms needed in the Preset
                        destroyDecoder();
                        createDecoder();
                        mDecoder.configure(mSurfaceView.getHolder().getSurface(),
                                mViewModel.getSelectedPresetAsParams());
                        mDecoder.start();

                        resumePlayback();
                    } else {
                        createControlsFragment();
                        // Call configure here so that the Params are set for the Decoder session
                        mDecoder.configure(mSurfaceView.getHolder().getSurface(),
                                mViewModel.getSelectedPresetAsParams());
                        mDecoder.start();
                    }



                } else if (selectedIndex == -1) {
                    // No preset selected
                    finish();
                }

                dialog.dismiss();
            }
        });

        builder.setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                // finish the activity if no preset was selected
                if (mViewModel.getSelectedPresetIndex() == -1) {
                    finish();
                }
            }
        });

        AlertDialog dialog = builder.create();
        dialog.show();
    }

    private void showErrorDialog(int messageResourceId) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Error");

        builder.setMessage(messageResourceId);
        builder.setNegativeButton("Exit", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                finish();
            }
        });

        builder.setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                finish();
            }
        });

        AlertDialog dialog = builder.create();
        dialog.show();
    }

    private void setupControlsSheet() {
        BottomSheetState state = mViewModel.getBottomSheetState();
        if (state == BottomSheetState.TRANSITIONING) {
            state = BottomSheetState.COLLAPSED;
        }

        if (state == BottomSheetState.HIDDEN) {
            hideSheet();
        } else if (state == BottomSheetState.COLLAPSED) {
            collapseSheet();
        } else if (state == BottomSheetState.EXPANDED) {
            expandSheet();
        }
    }

    private void collapseSheet() {
        if (mViewModel.getBottomSheetState() != BottomSheetState.TRANSITIONING) {
            mViewModel.setBottomSheetState(BottomSheetState.TRANSITIONING);

            final int parentHeight = getResources().getDisplayMetrics().heightPixels;
            final int peekHeight = parentHeight / 3;

            // Move to bottom third of screen
            mBottomSheet.animate()
                    .y(parentHeight - peekHeight)
                    .setDuration(SHEET_ANIMATION_TIME)
                    .setListener(new AnimatorListenerAdapter() {
                        @Override
                        public void onAnimationEnd(Animator animation) {
                            super.onAnimationEnd(animation);

                            mBottomSheet.setBackgroundColor((mEndColor - mStartColor) / 3);

                            // Resize to 1/3 of screen height
                            ViewGroup.LayoutParams params = mBottomSheet.getLayoutParams();
                            params.width = LayoutParams.FILL_PARENT;
                            params.height = peekHeight;
                            mBottomSheet.setLayoutParams(params);

                            // Hide the preset change button
                            mChangePresetButton.setVisibility(View.GONE);

                            mBottomSheet.invalidate();
                            mViewModel.setBottomSheetState(BottomSheetState.COLLAPSED);
                            Log.d(TAG, "sheet: collapse");
                        }
                    })
                    .start();
        }
    }

    private void expandSheet() {
        if (mViewModel.getBottomSheetState() != BottomSheetState.TRANSITIONING) {
            mViewModel.setBottomSheetState(BottomSheetState.TRANSITIONING);

            // Resize the view so it will fill the screen
            ViewGroup.LayoutParams params = mBottomSheet.getLayoutParams();
            params.height = LayoutParams.MATCH_PARENT;
            params.width = LayoutParams.MATCH_PARENT;
            mBottomSheet.setLayoutParams(params);

            // Show the preset change button
            mChangePresetButton.setVisibility(View.VISIBLE);

            // Move to top of the screen
            mBottomSheet.animate()
                    .y(0)
                    .setDuration(SHEET_ANIMATION_TIME)
                    .setListener(new AnimatorListenerAdapter() {
                        @Override
                        public void onAnimationEnd(Animator animation) {
                            super.onAnimationEnd(animation);

                            mBottomSheet.setBackgroundColor(mEndColor);
                            mBottomSheet.invalidate();
                            mViewModel.setBottomSheetState(BottomSheetState.EXPANDED);
                            Log.d(TAG, "sheet: expand");
                        }
                    })
                    .start();
        }
    }

    private void hideSheet() {
        if (mViewModel.getBottomSheetState() != BottomSheetState.TRANSITIONING) {
            mViewModel.setBottomSheetState(BottomSheetState.TRANSITIONING);

            final int parentHeight = getResources().getDisplayMetrics().heightPixels;

            // Move off screen
            mBottomSheet.animate()
                    .y(parentHeight)
                    .setDuration(SHEET_ANIMATION_TIME)
                    .setListener(new AnimatorListenerAdapter() {
                        @Override
                        public void onAnimationEnd(Animator animation) {
                            super.onAnimationEnd(animation);

                            mBottomSheet.setBackgroundColor(mStartColor);
                            mBottomSheet.invalidate();
                            mViewModel.setBottomSheetState(BottomSheetState.HIDDEN);
                            Log.d(TAG, "sheet: hide");
                        }
                    })
                    .start();
        }
    }

    @Override
    public void onBackPressed() {
        // Collapse the controls as applicable
        BottomSheetState state = mViewModel.getBottomSheetState();

        if (state == BottomSheetState.EXPANDED) {
            collapseSheet();
        } else if (state == BottomSheetState.COLLAPSED) {
            hideSheet();
        } else {
            super.onBackPressed();
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        mGestureDetector.onTouchEvent(event);
        return super.onTouchEvent(event);
    }

    private GestureDetector.OnGestureListener gestureListener =
        new GestureDetector.OnGestureListener() {

        @Override
        public boolean onDown(MotionEvent e) {
            // return true to handle it in the listener
            return true;
        }

        @Override
        public void onShowPress(MotionEvent e) {

        }

        @Override
        public boolean onSingleTapUp(MotionEvent e) {
            toggleControls();
            return true;
        }

        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2,
                float distanceX, float distanceY) {
            return onSwipe(Direction.fromMotionEvent(e1, e2));
        }

        @Override
        public void onLongPress(MotionEvent e) {

        }

        @Override
        public boolean onFling(MotionEvent e1, MotionEvent e2,
                float velocityX, float velocityY) {
            return onSwipe(Direction.fromMotionEvent(e1, e2));
        }

        boolean onSwipe(Direction direction) {
            BottomSheetState state = mViewModel.getBottomSheetState();

            switch (direction) {
                case UP:
                    if (state == BottomSheetState.HIDDEN) {
                        collapseSheet();
                    } else if (state == BottomSheetState.COLLAPSED) {
                        expandSheet();
                    }

                    break;
                case DOWN:
                    if (state == BottomSheetState.COLLAPSED) {
                        hideSheet();
                    }
                    break;
                default:
                    break;
            }

            return true;
        }
    };

    private View.OnClickListener mPlayButtonListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            Decoder.State state = mDecoder.getCurrentState();
            Log.v(TAG, "play/pause clicked with current state: " + state);

            if (state == Decoder.State.PLAYING) {
                synchronized(mStateUiLock) {
                    pause();
                    showVideoControls();
                }
            } else if (state == Decoder.State.PAUSED) {
                synchronized(mStateUiLock) {
                    play();
                    showVideoControls();
                }
            }
        }
    };

    private View.OnClickListener presetButtonListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            Log.d(TAG, "onClick: change preset button clicked");
            showPresetDialog();
        }
    };

    @Override
    public void onControlChanged(ExtensionHolder extensionHolder) {
        Log.d(TAG, "Extension Changed!!" +
                "\n\tid = " + extensionHolder.getId() +
                "\n\tvendorExt = " + extensionHolder.getVendorExt() +
                "\n\tvalue = " + extensionHolder.getValue().toString());

        mDecoder.setParams(extensionHolder);
    }

    private void resumePlayback() {
        // resume to the playback state from before
        if (mViewModel.getIsPlaying()) {
            play();
            hideVideoControls();
        } else {
            pause();
            showVideoControls();
        }
    }

    /**
     * Updates the dimensions of the video view based on phone rotation.
     */
    private void updateSurfaceParams() {
        int surfaceWidth = getResources().getDisplayMetrics().widthPixels;
        int surfaceHeight = getResources().getDisplayMetrics().heightPixels;
        int decoderWidth = mDecoder.getVideoWidth();
        int decoderHeight = mDecoder.getVideoHeight();

        int adjustedWidth = decoderWidth;
        int adjustedHeight = decoderHeight;

        if (mDecoder.getRotation() == 90 || mDecoder.getRotation() == 270) {
            adjustedWidth = decoderHeight;
            adjustedHeight = decoderWidth;
        }

        int width = surfaceWidth;
        int height = surfaceHeight;
        if (adjustedWidth != 0) {
            height = (width * adjustedHeight) / adjustedWidth;
        }

        if (height > surfaceHeight) {
            height = surfaceHeight;

            if (adjustedHeight != 0) {
                width = (height * adjustedWidth) / adjustedHeight;
            }
        }

        mSurfaceView.setLayoutParams(new LayoutParams(width, height, Gravity.CENTER));
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        if (mViewModel.isPresetSelected()) {
            mDecoder.configure(mSurfaceView.getHolder().getSurface(),
                    mViewModel.getSelectedPresetAsParams());
            mDecoder.start();
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (mSurfaceView != null) {
            updateSurfaceParams();
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        destroyDecoder();
    }


    private void destroyDecoder() {
        if (mDecoder != null) {
            mDecoder.unregisterOnPlaybackChangedListener(this);
            mDecoder.destroy();
            ActivityHelper.keepScreenOn(this, false);
        }

        if (mDataSource != null) {
            try {
                mDataSource.close();
            } catch (IOException e) {
                Log.e(TAG, "error closing data source: " + e);
            }
        }
    }

    @Override
    public void onPlaybackProgress(int percent, long curTime, boolean isSeeking) {
        mViewModel.setCurrentVideoTime(curTime);
        if (!isSeeking && mDecoder.getCurrentState() == Decoder.State.PLAYING) {
            updateProgress(percent);
        }
    }

    @Override
    public void onErrorFormat(Decoder.ErrorCode error, int maxHeight, int maxWidth) {
        Log.e(TAG, "Decoder error with code: " + error.name());
    }

    @Override
    public void onPlaybackEnd() {
        // Loop the video
        play();
    }

    @Override
    public void onIsPlaying(boolean isPlaying) { }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if (fromUser) {
            Log.d(TAG, "seekProgress: " + progress);
            showVideoControls();
            mDecoder.updateProgress(progress);
        }
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
        mDecoder.seek();
        showVideoControls();
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        mDecoder.endSeek();
    }

    private void updateProgress(int percent) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mSeekBar.setProgress(percent);
            }
        });
    }

    private void pause() {
        mViewModel.setIsPlaying(false);
        mDecoder.pause();
        togglePlayPauseControl(true);
        ActivityHelper.keepScreenOn(this, false);
    }

    private void play() {
        mViewModel.setIsPlaying(true);
        mDecoder.play();
        togglePlayPauseControl(false);
        ActivityHelper.keepScreenOn(this, true);
    }

    private void showVideoControls() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mVideoControls.clearAnimation();
                mVideoControls.animate().cancel();
                mVideoControls.setVisibility(View.VISIBLE);
                mVideoControls.setAlpha(1.0f);

                // Animate
                mVideoControls.animate()
                        .alpha(0.0f)
                        .setDuration(500)
                        .setStartDelay(2000)
                        .setListener(new AnimatorListenerAdapter() {
                            @Override
                            public void onAnimationEnd(Animator animation) {
                                super.onAnimationEnd(animation);
                                hideVideoControls();
                            }
                        });
            }
        });
    }

    private void hideVideoControls() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mVideoControls.clearAnimation();
                mVideoControls.animate().cancel();
                mVideoControls.setVisibility(View.GONE);
            }
        });
    }

    private void toggleControls() {
        if (mVideoControls.getVisibility() == View.GONE) {
            showVideoControls();
        } else {
            hideVideoControls();
        }
    }

    private void togglePlayPauseControl(boolean toPause) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (toPause) {
                    mPlayButton.setImageResource(R.drawable.play);
                } else {
                    mPlayButton.setImageResource(R.drawable.pause);
                }
            }
        });
    }
}
