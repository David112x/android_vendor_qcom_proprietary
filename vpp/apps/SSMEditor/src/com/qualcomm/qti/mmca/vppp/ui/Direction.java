/* Direction.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.mmca.vppp.ui;

import android.view.MotionEvent;

/**
 * Helper class for gesture events which represents the direction of a swipe or fling.
 */
public enum Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT;

    /**
     * Returns the direction of end relative to start.
     */
    public static Direction fromMotionEvent(MotionEvent start, MotionEvent end) {
        return fromCoords(start.getX(), start.getY(), end.getX(), end.getY());
    }

    private static Direction fromCoords(float startX, float startY, float endX, float endY) {
        double rad = Math.atan2(startY - endY, endX - startX) + Math.PI;
        double angle =  (rad * 180 / Math.PI + 180) % 360;

        return Direction.fromAngle(angle);
    }

    private static Direction fromAngle(double angle) {
        if (inRange(angle, 45, 135)) {
            return Direction.UP;
        }
        else if (inRange(angle, 0,45) || inRange(angle, 315, 360)) {
            return Direction.RIGHT;
        }
        else if (inRange(angle, 225, 315)) {
            return Direction.DOWN;
        }
        else {
            return Direction.LEFT;
        }

    }

    private static boolean inRange(double angle, float init, float end) {
        return (angle >= init) && (angle < end);
    }
}
