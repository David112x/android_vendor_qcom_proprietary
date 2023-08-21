/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file LogHelper.java
 */
package com.qualcomm.qti.mmca.util;

import java.util.ArrayList;
import java.util.List;

public class LogHelper {

    public interface LogListener {
        void onDebugLogAvailable(String tag, String message);
        void onEventLogAvailable(String tag, String message);
    }

    private static List<LogListener> mListeners;

    static {
        mListeners = new ArrayList<>();
    }

    public static void addListener(LogListener listener) {
        if (!mListeners.contains(listener)) {
            mListeners.add(listener);
        }
    }

    public static void logEvent(String tag, String log) {
        mListeners.forEach(l -> l.onEventLogAvailable(tag, log));
    }

    public static void logDebug(String tag, String log) {
        mListeners.forEach(l -> l.onDebugLogAvailable(tag, log));
    }
}
