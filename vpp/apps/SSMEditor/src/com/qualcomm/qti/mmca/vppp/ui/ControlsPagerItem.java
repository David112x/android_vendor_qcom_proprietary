/* ControlsPagerItem.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.mmca.vppp.ui;

import com.qualcomm.qti.mmca.vppp.db.ControlGroup;

/**
 * Data holder class for each page of the ViewPager.
 */
public class ControlsPagerItem implements ControlFieldUIElement {

    private ControlGroup mControlGroup;
    private ControlFieldLVAdapter mAdapter;

    ControlsPagerItem(ControlGroup controlGroup,
            ControlChangedListener listener) {
        this.mControlGroup = controlGroup;

        mAdapter = new ControlFieldLVAdapter(controlGroup.getControlFields(),
                listener);
    }

    public String getTitle() {
        return mControlGroup.getDisplayName();
    }

    public ControlFieldLVAdapter getAdapter() {
        return mAdapter;
    }

    @Override
    public void setControlChangedListener(ControlChangedListener listener) {
        mAdapter.setControlChangedListener(listener);
    }
}

