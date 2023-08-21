/**
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file SegmentUi.java
 */

package com.qualcomm.qti.ssmeditor.ssmeditor;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Rect;
import android.util.Log;

import com.qualcomm.qti.ssmeditor.R;

class SegmentUi {

    private static final String TAG = "SegmentUi";

    private Rect mRect;
    private Rect mEditableRect;
    private Rect mLeftTabHitbox;
    private Rect mRightTabHitbox;
    private Path mLeftTabUi;
    private Path mRightTabUi;

    private int mTabHitboxWidth;
    private int mTabHitboxHeight;
    private int mTabUiWidth;
    private int mTabUiHeight;

    private Paint mSegmentPaint;
    private Paint mActivePaint;
    private Paint mEditPaint;
    private Paint mTabPaint;

    private int mMaxHeight;
    private int mMaxWidth;
    private boolean mIsActive = false;
    private boolean mIsSelected = false;

    SegmentUi(Context context, int left, int right, int maxWidth, int maxHeight,
              float displayScale, Paint segmentPaint) {

        Log.d(TAG, "SegmentUi: left=" + left + ", right=" + right);

        this.mMaxWidth = maxWidth;
        this.mMaxHeight = maxHeight;

        this.mTabHitboxWidth = (int) (20 * displayScale + 0.5f);
        this.mTabHitboxHeight = (int) (50 * displayScale + 0.5f);

        // Make tab marker an equilateral triangle
        mTabUiHeight = this.mTabHitboxHeight / 5;
        mTabUiWidth = (int) (2 * mTabUiHeight / Math.sqrt(3));

        mRect = new Rect(left, mTabUiHeight, right, maxHeight);
        mEditableRect = new Rect(mRect);
        setupTabHitboxes();
        setupTabUi();

        mSegmentPaint = segmentPaint;
        if (mSegmentPaint == null) {
            // In the majority of cases, the caller should be setting
            // segmentPaint to inform us how to draw this. However in
            // the off chance they don't, we'll use a glaring yellow
            // to indicate that they didn't, and to avoid any NPEs.
            mSegmentPaint = new Paint();
            mSegmentPaint.setStyle(Paint.Style.FILL);
            mSegmentPaint.setColor(Color.YELLOW);
        }

        mActivePaint = new Paint();
        mActivePaint.setStyle(Paint.Style.STROKE);
        mActivePaint.setStrokeWidth(5);
        mActivePaint.setColor(Color.GREEN);

        mEditPaint = new Paint();
        mEditPaint.setStyle(Paint.Style.FILL);
        mEditPaint.setColor(Color.WHITE);
        mEditPaint.setAlpha(150);

        mTabPaint = new Paint();
        mTabPaint.setStyle(Paint.Style.FILL);
        mTabPaint.setColor(context.getResources().getColor(R.color.colorTab));
    }

    private void setupTabHitboxes() {
        mLeftTabHitbox = new Rect();
        mRightTabHitbox = new Rect();
        updateTabHitboxes();
    }

    private void updateTabHitboxes() {
        mLeftTabHitbox.set(mEditableRect.left - mTabHitboxWidth / 2,
                0,
                mEditableRect.left + mTabHitboxWidth + 2,
                mMaxHeight);
        mRightTabHitbox.set(mEditableRect.right - mTabHitboxWidth / 2,
                0,
                mEditableRect.right + mTabHitboxWidth / 2,
                mMaxHeight);
    }

    private void setupTabUi() {
        mLeftTabUi = new Path();
        mRightTabUi = new Path();
        mLeftTabUi.setFillType(Path.FillType.EVEN_ODD);
        mRightTabUi.setFillType(Path.FillType.EVEN_ODD);
        updateTabUi();
    }

    private void updateTabUi() {
        mLeftTabUi.reset();
        mLeftTabUi.moveTo(mEditableRect.left, mEditableRect.top);
        mLeftTabUi.lineTo(mEditableRect.left - mTabUiWidth / 2, 0);
        mLeftTabUi.lineTo(mEditableRect.left + mTabUiWidth / 2, 0);
        mLeftTabUi.lineTo(mEditableRect.left, mEditableRect.top);

        mRightTabUi.reset();
        mRightTabUi.moveTo(mEditableRect.right, mEditableRect.top);
        mRightTabUi.lineTo(mEditableRect.right - mTabUiWidth / 2, 0);
        mRightTabUi.lineTo(mEditableRect.right + mTabUiWidth / 2, 0);
        mRightTabUi.lineTo(mEditableRect.right, mEditableRect.top);
    }

    Rect getEditableRect() {
        return mEditableRect;
    }

    void activate() {
        mIsActive = true;
    }

    void deactivate() {
        mIsActive = false;
    }

    void select() {
        mIsSelected = true;
    }

    void deselect() {
        mIsSelected = false;
    }

    void updateSelectedSegment(int left, int right) {
        mEditableRect.left = left;
        mEditableRect.right = right;
        updateTabHitboxes();
        updateTabUi();
    }

    void onDraw(Canvas canvas) {
        canvas.drawRect(mRect, mSegmentPaint);
        canvas.drawPath(mLeftTabUi, mTabPaint);
        canvas.drawPath(mRightTabUi, mTabPaint);

        if (mIsSelected) {
            canvas.drawRect(mEditableRect, mEditPaint);
        }
        if (mIsActive) {
            canvas.drawRect(mRect, mActivePaint);
        }
    }

    boolean leftTabContains(int x, int y) {
        return mLeftTabHitbox.contains(x, y);
    }

    boolean rightTabContains(int x, int y) {
        return mRightTabHitbox.contains(x, y);
    }

    boolean rectContains(int x, int y) {
        return mEditableRect.contains(x, y);
    }
}
