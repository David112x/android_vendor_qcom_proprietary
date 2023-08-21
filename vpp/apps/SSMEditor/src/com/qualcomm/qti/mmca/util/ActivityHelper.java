/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file ActivityHelper.java
 */
package com.qualcomm.qti.mmca.util;

import android.app.Activity;
import android.view.WindowManager;

public class ActivityHelper {

    public static void keepScreenOn(final Activity activity, final boolean on) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                int flagKeepScreenOn = WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON;
                if (on) {
                    activity.getWindow().addFlags(flagKeepScreenOn);
                } else {
                    activity.getWindow().clearFlags(flagKeepScreenOn);
                }
            }
        });
    }
}
