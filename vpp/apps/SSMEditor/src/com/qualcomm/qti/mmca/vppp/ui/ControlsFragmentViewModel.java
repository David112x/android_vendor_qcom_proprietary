/* ControlsFragmentViewModel.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.mmca.vppp.ui;

import android.os.Parcel;
import android.os.Parcelable;

import com.qualcomm.qti.mmca.vppp.db.Preset;

/**
 * ViewModel for the ControlsFragment
 */
public class ControlsFragmentViewModel implements Parcelable {

    private Preset mPreset; // Preset being displayed
    private int mPageIndex; // Page displayed on ViewPager

    /**
     * @param preset Should not be null.
     * @param pageIndex
     */
    public ControlsFragmentViewModel(Preset preset, int pageIndex) {
        this.mPreset = preset;
        this.mPageIndex = pageIndex;
    }

    /**
     * @param preset Should not be null.
     */
    public void setPreset(Preset preset) {
        this.mPreset = preset;
    }

    public void setPageIndex(int pageIndex) {
        this.mPageIndex = pageIndex;
    }

    public Preset getPreset() {
        return mPreset;
    }

    public int getPageIndex() {
        return mPageIndex;
    }

    public static final Parcelable.Creator<ControlsFragmentViewModel> CREATOR =
        new Parcelable.Creator<ControlsFragmentViewModel>() {
            @Override
            public ControlsFragmentViewModel createFromParcel(Parcel source) {
                return new ControlsFragmentViewModel(source);
            }

            @Override
            public ControlsFragmentViewModel[] newArray(int size) {
                return new ControlsFragmentViewModel[size];
            }
        };

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeParcelable(mPreset, flags);
        dest.writeInt(mPageIndex);
    }

    private ControlsFragmentViewModel(Parcel in) {
        this.mPreset = in.readParcelable(Preset.class.getClassLoader());
        this.mPageIndex = in.readInt();
    }
}
