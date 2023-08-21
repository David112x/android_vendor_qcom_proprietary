/**
 * Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file SegmentsView.java
 */

package com.qualcomm.qti.ssmeditor.ssmeditor;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

import com.qualcomm.qti.mmca.ssmeditor.Segment;

import java.util.ArrayList;
import java.util.List;

import com.qualcomm.qti.ssmeditor.R;

public class SegmentsView extends View {

    enum SegmentAction {
        NONE,
        ADD_CONTROL_LEFT,
        ADD_CONTROL_RIGHT,
        EDIT_CONTROL_LEFT,
        EDIT_CONTROL_RIGHT,
        EDIT_DRAG
    }

    public static class SegmentDescriptor {
        Segment segment;
        Paint paintSegment;
    }

    private class SegmentAssociation extends SegmentDescriptor {
        SegmentUi ui;
    }

    private static final String TAG = "SSMSegmentsView";

    private Context mContext;
    private List<SegmentAssociation> mSegments;
    private SegmentAssociation mActiveSegment;   // keeps track which segment is currently active
    private SegmentAssociation mSelectedSegment; // keeps track which segment is currently selected
    private SegmentAction mSegmentAction = SegmentAction.NONE;
    private int mPreviousSegmentDragPosition;

    private long mFinalTimestamp = Long.MAX_VALUE;
    private long mProgressTs;
    private float mDisplayScale;
    private int mMaxViewWidth = 0;
    private int mMaxViewHeight = 0;
    private Paint mBackgroundPaint;
    private Rect mBackgroundBox;
    private int mThumbOffset = 0;

    private List<OnChangeSegmentListener> mChangeSegmentListeners;

    public SegmentsView(Context context, AttributeSet attrs) {
        super(context, attrs);

        mContext = context;
        // ex. mDisplayScale * 50 = 50dp
        mDisplayScale = context.getResources().getDisplayMetrics().density;
        mSegments = new ArrayList<>();

        mChangeSegmentListeners = new ArrayList<OnChangeSegmentListener>();

        mBackgroundBox = new Rect(mThumbOffset, (int) (50 * mDisplayScale + 0.5f) / 5,
                mMaxViewWidth - mThumbOffset, mMaxViewHeight);
        mBackgroundPaint = new Paint();
        mBackgroundPaint.setStyle(Paint.Style.FILL);
        mBackgroundPaint.setColor(context.getResources().getColor(R.color.colorBackgroundSegment));
    }

    public interface OnChangeSegmentListener {
        void onAddSegment(long startTs, long endTs);
        void onEditSegment(Segment oldSegment, Segment newSegment);
    }

    public void registerOnChangeSegmentListener(OnChangeSegmentListener listener) {
        if (!mChangeSegmentListeners.contains(listener)) {
            mChangeSegmentListeners.add(listener);
        }
    }

    public void unregisterOnChangeSegmentListener(OnChangeSegmentListener listener) {
        if (mChangeSegmentListeners.contains(listener)) {
            mChangeSegmentListeners.remove(listener);
        }
    }

    public void addSegment(long startTs, long endTs) {
        Log.v(TAG, "[DEBUG] Firing add segment listeners: start=" + startTs + ", endTs=" + endTs);

        for (OnChangeSegmentListener l : mChangeSegmentListeners) {
            l.onAddSegment(startTs, endTs);
        }
    }

    public void editSegment(Segment oldSegment, Segment newSegment) {
        Log.v(TAG, "[DEBUG] Firing edit segment listeners:");

        for (OnChangeSegmentListener l : mChangeSegmentListeners) {
            l.onEditSegment(oldSegment, newSegment);
        }
    }

    public void setThumbOffset(int offset) {
        mThumbOffset = offset;
    }

    public void setSegments(List<SegmentDescriptor> segments) {
        mSegments.clear();

        segments.forEach( s -> {
            SegmentAssociation sa = new SegmentAssociation();

            sa.segment = new Segment(s.segment);
            sa.ui = null;
            sa.paintSegment = s.paintSegment;

            mSegments.add(sa);
        });

        Log.d(TAG, "setSegments");
        resetSegmentRects();
    }

    public void setFinalTimestamp(Long timestamp) {
        Log.d(TAG, "final timestamp set to: " + timestamp);
        mFinalTimestamp = timestamp;
        resetSegmentRects();
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        mMaxViewWidth = w;
        mMaxViewHeight = h;
        Log.d(TAG, "new size: width=" + mMaxViewWidth + ", height=" + mMaxViewHeight);
        mBackgroundBox.set(mThumbOffset, (int) (50 * mDisplayScale + 0.5f) / 5, w - mThumbOffset, h);
        resetSegmentRects();
    }

    public void setProgress(long timestamp) {
        SegmentAssociation targetSegment;

        Log.d(TAG, "setProgress() ts=" + timestamp);
        mProgressTs = timestamp;
        if (timestamp < 0) {
            targetSegment = null;
        } else {
            targetSegment = findSegmentAssociation(mProgressTs);
        }

        setActiveSegment(targetSegment);
        invalidate();
    }

    private SegmentAssociation findSegmentAssociation(long timestamp) {
        synchronized (this) {
            for (SegmentAssociation sa : mSegments) {
                Segment s = sa.segment;
                Log.d(TAG, "searching segment: start=" + s.getStartTime() +
                        ", end=" + s.getEndTime());
                if (s.getStartTime() <= timestamp && timestamp <= s.getEndTime()) {
                    return sa;
                }
            }
        }
        Log.d(TAG, "findSegmentAssociation returning null");
        return null;
    }

    private void setActiveSegment(SegmentAssociation segment) {
        Log.d(TAG, "New active segment: " + segment);

        if (mActiveSegment != null) {
            mActiveSegment.ui.deactivate();
        }
        mActiveSegment = segment;
        if (mActiveSegment != null) {
            mActiveSegment.ui.activate();
        }
    }

    private void setSelectedSegment(SegmentAssociation segment) {
        Log.d(TAG, "New selected segment: " + segment);

        if (mSelectedSegment != null) {
            mSelectedSegment.ui.deselect();
        }
        mSelectedSegment = segment;
        if (mSelectedSegment != null) {
            mSelectedSegment.ui.select();
        }
    }

    @Override
    protected void onDraw(Canvas canvas) {
        synchronized (this) {

            canvas.drawRect(mBackgroundBox, mBackgroundPaint);

            for (SegmentAssociation sa : mSegments) {
                SegmentUi s = sa.ui;
                // Draw active segment and selected segment last
                if (sa != mActiveSegment && sa != mSelectedSegment) {
                    s.onDraw(canvas);
                }
            }

            if (mActiveSegment != null) {
                mActiveSegment.ui.onDraw(canvas);
            }
            if (mSelectedSegment != null) {
                mSelectedSegment.ui.onDraw(canvas);
            }
        }
    }

    private void resetSegmentRects() {
        synchronized (this) {
            if (mSegments == null) {
                Log.e(TAG, "called to reset segments with null segments.");
            }

            for (SegmentAssociation sa : mSegments) {
                Segment s = sa.segment;

                int leftX = calculateXCoordinateFromTimestamp(s.getStartTime());
                int rightX = calculateXCoordinateFromTimestamp(s.getEndTime());

                sa.ui = new SegmentUi(mContext, leftX, rightX, mMaxViewWidth,
                        mMaxViewHeight, mDisplayScale, sa.paintSegment);
            }
        }

        setActiveSegment(findSegmentAssociation(mProgressTs));

        invalidate();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        switch (event.getActionMasked()) {

            case MotionEvent.ACTION_DOWN: {
                handleOnTouchActionDown((int) event.getX(), (int) event.getY());
                break;
            }

            case MotionEvent.ACTION_MOVE: {
                handleOnTouchActionMove((int) event.getX(), (int) event.getY());
                break;
            }

            case MotionEvent.ACTION_UP: {
                handleOnTouchActionUp((int) event.getX(), (int) event.getY());
                break;
            }
        }

        return true;
    }

    private void handleOnTouchActionDown(int onTouchX, int onTouchY) {
        Log.d(TAG, "onTouch down: x=" + onTouchX + ", y=" + onTouchY);

        for (SegmentAssociation sa : mSegments) {
            SegmentUi s = sa.ui;
            if (s.leftTabContains(onTouchX, onTouchY)) {
                Log.i(TAG, "selected segment action: left tab edit");
                setSelectedSegment(sa);
                mSegmentAction = SegmentAction.EDIT_CONTROL_LEFT;
                return;

            } else if (s.rightTabContains(onTouchX, onTouchY)) {
                Log.i(TAG, "selected segment action: right tab edit");
                setSelectedSegment(sa);
                mSegmentAction = SegmentAction.EDIT_CONTROL_RIGHT;
                return;

            } else if (s.rectContains(onTouchX, onTouchY)) {
                Log.i(TAG, "selected segment action: drag");
                setSelectedSegment(sa);
                mSegmentAction = SegmentAction.EDIT_DRAG;
                mPreviousSegmentDragPosition = onTouchX;
                return;
            }
        }

        // No segment was touched.
        Log.i(TAG, "selected segment action: add");
        mSegmentAction = SegmentAction.ADD_CONTROL_LEFT;
        SegmentAssociation sa = new SegmentAssociation();
        sa.ui = new SegmentUi(mContext, onTouchX, onTouchX, mMaxViewWidth,
                mMaxViewHeight, mDisplayScale, null);
        sa.segment = null;
        setSelectedSegment(sa);
    }

    private void handleOnTouchActionMove(int onTouchX, int onTouchY) {
        Log.d(TAG, "onTouch move: x=" + onTouchX + ", y=" + onTouchY);

        if (mSelectedSegment != null) {
            switch (mSegmentAction) {
                // Intentional fall-through.
                case EDIT_CONTROL_LEFT:
                case EDIT_CONTROL_RIGHT:
                    handleTabEditActionMove(onTouchX);
                    break;
                case EDIT_DRAG:
                    handleDragActionMove(onTouchX);
                    break;
                // Intentional fall-through.
                case ADD_CONTROL_LEFT:
                case ADD_CONTROL_RIGHT:
                    handleAddActionMove(onTouchX);
                    break;
                default:
                    Log.e(TAG, "handleOnTouchActionMove(): invalid segment action: " +
                            mSegmentAction);
                    break;
            }

            invalidate();
        }
    }

    private void handleTabEditActionMove(int onTouchX) {
        int left;
        int right;

        if (mSegmentAction == SegmentAction.EDIT_CONTROL_LEFT) {
            left = onTouchX;
            right = mSelectedSegment.ui.getEditableRect().right;
            if (right < left) {
                left = right;
            }

        } else {
            left = mSelectedSegment.ui.getEditableRect().left;
            right = onTouchX;
            if (right < left) {
                right = left;
            }
        }

        mSelectedSegment.ui.updateSelectedSegment(left, right);
    }

    private void handleDragActionMove(int onTouchX) {
        int left;
        int right;
        int movement = onTouchX - mPreviousSegmentDragPosition;

        left = mSelectedSegment.ui.getEditableRect().left + movement;
        right = mSelectedSegment.ui.getEditableRect().right + movement;
        if (left < 0) {
            left = 0;
            right = mSelectedSegment.ui.getEditableRect().width();
        } else if (right > mMaxViewWidth) {
            right = mMaxViewWidth;
            left = right - mSelectedSegment.ui.getEditableRect().width();
        }
        mSelectedSegment.ui.updateSelectedSegment(left, right);

        mPreviousSegmentDragPosition = onTouchX;
    }

    private void handleAddActionMove(int onTouchX) {
        int left, right, temp;

        if (mSegmentAction == SegmentAction.ADD_CONTROL_LEFT) {
            left = onTouchX;
            right = mSelectedSegment.ui.getEditableRect().right;
            if (right < left) {
                temp = left;
                left = right;
                right = temp;
                mSegmentAction = SegmentAction.ADD_CONTROL_RIGHT;
            }

        } else {
            left = mSelectedSegment.ui.getEditableRect().left;
            right = onTouchX;
            if (right < left) {
                temp = left;
                left = right;
                right = temp;
                mSegmentAction = SegmentAction.ADD_CONTROL_LEFT;
            }
        }

        mSelectedSegment.ui.updateSelectedSegment(left, right);
    }

    private void handleOnTouchActionUp(int onTouchX, int onTouchY) {
        Log.d(TAG, "onTouch up: x=" + onTouchX + ", y=" + onTouchY);

        if (mSelectedSegment != null) {
            commitSelectedSegment();
        }
        setSelectedSegment(null);
        mSegmentAction = SegmentAction.NONE;
    }

    private void commitSelectedSegment() {
        long newStartTs = calculateTimestampFromXCoordinate(
                mSelectedSegment.ui.getEditableRect().left);
        long newEndTs = calculateTimestampFromXCoordinate(
                mSelectedSegment.ui.getEditableRect().right);

        if (mSegmentAction == SegmentAction.ADD_CONTROL_LEFT ||
                mSegmentAction == SegmentAction.ADD_CONTROL_RIGHT) {
            addSegment(newStartTs, newEndTs);
        } else {
            editSegment(new Segment(mSelectedSegment.segment),
                    new Segment(newStartTs, newEndTs));
        }
    }

    private long calculateTimestampFromXCoordinate(int xCoordinate) {
        if (mMaxViewWidth == 0) {
            Log.e(TAG, "failed to get view width, called too early...");
        }
        return scaleToNewRange(xCoordinate, mThumbOffset, mMaxViewWidth - mThumbOffset,
                0, mFinalTimestamp);
    }

    private int calculateXCoordinateFromTimestamp(long timestamp) {
        if (mMaxViewWidth == 0) {
            Log.e(TAG, "failed to get view width, called too early...");
        }
        return (int) scaleToNewRange(timestamp, 0, mFinalTimestamp,
                mThumbOffset, mMaxViewWidth - mThumbOffset);
    }

    private long scaleToNewRange(long value, long oldRangeMin, long oldRangeMax,
                                 long newRangeMin, long newRangeMax) {
        if (value <= oldRangeMin) {
            return newRangeMin;
        } else if (value >= oldRangeMax) {
            return newRangeMax;
        }
        double percent = (value - oldRangeMin) / (double) (oldRangeMax - oldRangeMin);
        return Math.round(percent * (newRangeMax - newRangeMin)) + newRangeMin;
    }
}
