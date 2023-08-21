/* NoScrollViewPager.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.mmca.vppp.ui;

import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;

import com.qualcomm.qti.mmca.view.viewpager.ViewPager;

/**
 *  A ViewPager that does not support swiping to scroll (all paging
 *  must be done programmatically). No touch events are handled on
 *  the ViewPager itself.
 */
public class NoScrollViewPager extends ViewPager {

    /**
     * context should not be null.
     */
    public NoScrollViewPager(Context context) {
        this(context, null);
    }

    /**
     * context should not be null.
     */
    public NoScrollViewPager(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        return false;
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        return false;
    }
}

