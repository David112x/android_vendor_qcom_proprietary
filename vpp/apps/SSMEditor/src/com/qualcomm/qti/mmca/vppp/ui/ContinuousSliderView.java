/* ContinuousSliderView.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.mmca.vppp.ui;

import android.content.Context;
import android.text.Editable;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.View;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;

import com.qualcomm.qti.ssmeditor.R;

import java.util.ArrayList;
import java.util.List;

/**
 * Custom compound View containing a title, slider, and numerical entry field, where the number
 * field maps to the slider value to allow fine tuning.
 */
public class ContinuousSliderView extends LinearLayout
        implements View.OnKeyListener, SeekBar.OnSeekBarChangeListener {

    /**
     * Listener for when the view's value changes (either by user input or programmatically).
     */
    public interface OnValueChangeListener {
        void onValueChange(int newValue);
    }

    private TextView mTitleTV;
    private SeekBar mSlider;
    private EditText mNumberEntry;

    private int mMin = 0;
    private int mMax = 100;
    private int mStep = 1;

    private List<OnValueChangeListener> mOnValueChangeListeners;

    public ContinuousSliderView(Context context, int min, int max, int defaultValue, int step) {
        this(context);

        configure(min, max, defaultValue, step);
    }

    public ContinuousSliderView(Context context) {
        this(context, null);
    }

    public ContinuousSliderView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ContinuousSliderView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    private void init() {
        inflate(getContext(), R.layout.continuous_slider, this);

        mTitleTV = findViewById(R.id.slider_title);
        mSlider = findViewById(R.id.slider);
        mNumberEntry = findViewById(R.id.slider_num);

        mSlider.setOnSeekBarChangeListener(this);
        mNumberEntry.setOnKeyListener(this); // Listener to detect for enter key pressed

        mOnValueChangeListeners = new ArrayList<>();
    }

    public void configure(int min, int max, int defaultValue, int step) {
        mMin = min;
        mMax = max;
        mStep = step;

        setValue(defaultValue);
    }

    private void notifyListeners(int newValue) {
        for (OnValueChangeListener l : mOnValueChangeListeners) {
            l.onValueChange(newValue);
        }
    }

    /**
     * Applies the step factor to a value to generate the slider position for setting progress.
     */
    private int applyStep(int value) {
        return value / mStep * mStep;
    }

    public void addOnValueChangeListener(OnValueChangeListener listener) {
        if (!mOnValueChangeListeners.contains(listener)) {
            mOnValueChangeListeners.add(listener);
        }
    }

    public void removeOnValueChangeListener(OnValueChangeListener listener) {
        mOnValueChangeListeners.remove(listener);
    }

    public void setTitle(String title) {
        mTitleTV.setText(title);
    }

    public int getValue() {
        return getProgress();
    }

    private void setText(int value) {
        mNumberEntry.setText(String.valueOf(value));
    }

    /**
     * Sets the SeekBar's progress to a value. This should be called instead of
     * slider.setProgress(value).
     */
    private void setProgress(int value) {
        mSlider.setProgress(applyStep(value));
    }

    /**
     * Gets the SeekBar's progress factoring in the step. This should be called instead of
     * slider.getProgress().
     */
    private int getProgress() {
        return applyStep(mSlider.getProgress());
    }

    public void setValue(int value) {
        setProgress(value);
        setText(value);
        notifyListeners(value);
    }

    public int getMin() {
        return mMin;
    }

    public void setMin(int min) {
        mMin = min;
    }

    public int getMax() {
        return mMax;
    }

    public void setMax(int max) {
        mMax = max;
    }

    public int getStep() {
        return mStep;
    }

    public void setStep(int step) {
        mStep = step;
    }

    public void hideTitle() {
        mTitleTV.setVisibility(GONE);
    }

    public void showTitle() {
        mTitleTV.setVisibility(VISIBLE);
    }

    /**
     * Detect when user has entered a number in the text box.
     */
    @Override
    public boolean onKey(View v, int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_ENTER) {
            try {
                Editable entry = mNumberEntry.getText();
                if (entry != null) {
                    int newVal = Integer.valueOf(entry.toString());

                    if (newVal != getProgress() && newVal >= mMin && newVal <= mMax) {
                        setProgress(newVal);
                        notifyListeners(newVal);
                    } else {
                        // TODO show error msg as a Toast
                        // Revert to slider value
                        setText(getProgress());
                    }
                }

            } catch (NumberFormatException | NullPointerException e) {
                setText(getProgress());
            }
        }

        return false;
    }


    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if (fromUser) {
            progress = applyStep(progress);
            setText(progress);
            notifyListeners(progress);
        }
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
        // Do nothing
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        // Do nothing
    }
}
