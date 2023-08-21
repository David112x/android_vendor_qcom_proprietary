/* ControlsView.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.mmca.vppp.ui;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.LinearLayout;

import com.qualcomm.qti.mmca.vppp.db.Preset;
import com.qualcomm.qti.ssmeditor.R;
import com.qualcomm.qti.mmca.view.chip.ChipView;
import com.qualcomm.qti.mmca.view.chip.RadioChipGroup;

/**
 * Custom compound View for displaying controls. Houses the ViewPager and its title
 * ChipGroup used for pagination.
 */
public class ControlsView extends LinearLayout implements ControlFieldUIElement {

    private RadioChipGroup mChipGroup;
    private NoScrollViewPager mViewPager;
    private ControlsPagerAdapter mAdapter;

    private ControlChangedListener mControlChangedListener;

    private OnPageChangedListener mPageChangeListener;

    public ControlsView(Context context) {
        this(context, null);
    }

    public ControlsView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ControlsView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        inflate(getContext(), R.layout.controls_view, this);

        mChipGroup = findViewById(R.id.control_chip_radio_group);
        mViewPager = findViewById(R.id.controls_pager);
    }

    public void configure(Preset preset, ControlChangedListener listener) {
        mControlChangedListener = listener;

        // setup adapter
        mAdapter = new ControlsPagerAdapter(getContext(),
                preset.getControlGroups(), mControlChangedListener);
        mViewPager.setOffscreenPageLimit(preset.getControlGroups().size());
        mViewPager.setAdapter(mAdapter);

        mChipGroup.setSaveEnabled(false); // state should be managed by the user of this view
        mChipGroup.removeAllViews();

        // setup chip titles
        for (int i = 0; i < mAdapter.getCount(); i++) {
            mChipGroup.addChip(mAdapter.getPageTitle(i).toString());
        }

        mChipGroup.registerListener(new RadioChipGroup.OnCheckedChangedListener() {
            @Override
            public void onCheckedChanged(RadioChipGroup chipGroup, ChipView chip, int pos) {
                // Ensure the adapter scrolls in lockstep
                mViewPager.setCurrentItem(pos, true);

                if (mPageChangeListener != null) {
                    mPageChangeListener.onPageChanged(pos);
                }
            }
        });
    }

    /**
     * @param preset Should not be null.
     */
    public void setPreset(Preset preset) {
        configure(preset, mControlChangedListener);
    }

    public void setPage(int index) {
        if (0 <= index && index < mAdapter.getCount()) {
            mChipGroup.check(index);
        }
    }

    @Override
    public void setControlChangedListener(ControlChangedListener listener) {
        mControlChangedListener = listener;
        mAdapter.setControlChangedListener(listener);
    }

    public void setOnPageChangedListener(OnPageChangedListener listener) {
        mPageChangeListener = listener;
    }

    /**
     * Listener for when the page being displayed changes.
     */
    public interface OnPageChangedListener {
        /**
         * Called when the page being displayed on the pager is changed.
         *
         * @param newPage the index of the new page being displayed.
         */
        public void onPageChanged(int newPage);
    }
}
