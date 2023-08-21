/* RadioChipGroup.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.mmca.view.chip;

import android.content.Context;
import android.os.Parcel;
import android.os.Parcelable;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;

import java.util.List;
import java.util.ArrayList;

import com.qualcomm.qti.ssmeditor.R;

/**
 * ChipGroup that functions as a RadioGroup.
 */
public class RadioChipGroup extends FlowLayout {

    private List<Integer> mIds;
    private int mCheckedIndex;
    private OnCheckedChangedListener mListener;

    public RadioChipGroup(Context context) {
        this(context, null);
    }

    public RadioChipGroup(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public RadioChipGroup(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        setSaveEnabled(true);
        setSingleLine(true);
        setItemSpacing(dpToPx(5));

        mCheckedIndex = 0;
        mIds = new ArrayList<>();
    }

    @Override
    public ViewGroup.LayoutParams generateLayoutParams(AttributeSet attrs) {
        return new RadioChipGroup.LayoutParams(getContext(), attrs);
    }

    @Override
    protected ViewGroup.LayoutParams generateLayoutParams(ViewGroup.LayoutParams lp) {
        return new RadioChipGroup.LayoutParams(lp);
    }

    @Override
    protected ViewGroup.LayoutParams generateDefaultLayoutParams() {
        return new RadioChipGroup.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
    }

    @Override
    protected boolean checkLayoutParams(ViewGroup.LayoutParams p) {
        return super.checkLayoutParams(p) && (p instanceof RadioChipGroup.LayoutParams);
    }

    @Override
    protected Parcelable onSaveInstanceState() {
        Parcelable superState = super.onSaveInstanceState();
        SavedState savedState = new SavedState(superState);
        savedState.mCheckedIndex = mCheckedIndex;
        return savedState;
    }

    @Override
    protected void onRestoreInstanceState(Parcelable state) {
        SavedState savedState = (SavedState) state;
        super.onRestoreInstanceState(savedState.getSuperState());
        mCheckedIndex = 0;
        check(savedState.mCheckedIndex);
    }

    private int dpToPx(int dp) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP,
                dp, getResources().getDisplayMetrics());
    }

    public ChipView addChip(String text) {
        ChipView chip = new ChipView(getContext());

        // NOTE for some reason, setting a chip style from styles.xml doesn't work
        // Chip chip = new Chip(getContext(), null, R.style.VPPPChipStyle);
        // Set all the chip styles
        ViewGroup.LayoutParams params = new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, dpToPx(35));
        chip.setLayoutParams(params);
        chip.setPadding(dpToPx(10), 0, dpToPx(10), 0);
        chip.setGravity(Gravity.CENTER);
        chip.setBackground(getContext().getDrawable(R.drawable.chip_background));
        chip.setEllipsize(TextUtils.TruncateAt.END);
        chip.setFocusable(true);
        chip.setClickable(true);
        chip.setMaxLines(1);
        chip.setMinLines(1);
        chip.setButtonDrawable(null);

        chip.setText(text);
        int id = View.generateViewId();
        chip.setId(id);
        mIds.add(id);
        chip.setSaveEnabled(true);
        chip.setOnCheckedChangeListener(mInternalChipListener);

        super.addView(chip);

        return chip;
    }

    public void check(int index) {
        if (0 <= index && index < mIds.size()) {
            ChipView chip = findViewById(mIds.get(index));
            if (chip != null) {
                chip.setChecked(true); // listener takes care of exact implementation
            }
        }
    }

    @Override
    public void removeAllViews() {
        super.removeAllViews();

        mCheckedIndex = 0;
        mIds.clear();
    }

    private void notifyListeners(ChipView newChecked, int pos) {
        if (mListener != null) {
            mListener.onCheckedChanged(this, newChecked, pos);
        }
    }

    public void registerListener(OnCheckedChangedListener listener) {
        mListener = listener;
    }

    public void unregisterListener(OnCheckedChangedListener listener) {
        mListener = null;
    }

    private ChipView.OnCheckedChangeListener mInternalChipListener
            = new CompoundButton.OnCheckedChangeListener() {
        @Override
        public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
            if (isChecked) {
                // Uncheck the last chip
                ChipView chip = findViewById(mIds.get(mCheckedIndex));
                chip.setChecked(false);
                chip.setClickable(true);

                // Check the new chip
                int newCheckedId = buttonView.getId();
                buttonView.setChecked(true);
                buttonView.setClickable(false);

                mCheckedIndex = mIds.indexOf(newCheckedId);

                notifyListeners((ChipView) buttonView, mCheckedIndex);
            }
        }
    };

    /**
     * Listener for when the selected chip in the group changes.
     */
    public interface OnCheckedChangedListener {
        /**
         * Called when a new chip is checked in the group.
         *
         * @param chipGroup the group in which the checked chip has changed.
         * @param chip      the newly checked Chip
         * @param pos       the position of the newly checked chip (index from 0).
         */
        public void onCheckedChanged(RadioChipGroup chipGroup, ChipView chip, int pos);
    }

    public static class LayoutParams extends MarginLayoutParams {
        public LayoutParams(Context context, AttributeSet attrs) {
            super(context, attrs);
        }

        public LayoutParams(ViewGroup.LayoutParams source) {
            super(source);
        }

        public LayoutParams(int width, int height) {
            super(width, height);
        }

        public LayoutParams(MarginLayoutParams source) {
            super(source);
        }
    }

    /**
     * Class to handle saving the state of the view on configuration changes.
     * Doesn't save the ids because those are regenerated after configuration
     * changes by the views.
     */
    private static class SavedState extends BaseSavedState {
        int mCheckedIndex;

        SavedState(Parcelable superState) {
            super(superState);
        }

        private SavedState(Parcel in) {
            super(in);

            this.mCheckedIndex = in.readInt();
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            super.writeToParcel(out, flags);
            out.writeInt(this.mCheckedIndex);
        }

        public static final Parcelable.Creator<SavedState> CREATOR =
                new Parcelable.Creator<SavedState>() {
            @Override
            public SavedState createFromParcel(Parcel in) {
                return new SavedState(in);
            }

            @Override
            public SavedState[] newArray(int size) {
                return new SavedState[size];
            }
        };
    }
}
