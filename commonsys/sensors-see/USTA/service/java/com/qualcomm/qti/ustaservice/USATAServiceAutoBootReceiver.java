/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.ustaservice;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class USATAServiceAutoBootReceiver extends BroadcastReceiver {
    private static final String TAG = "USTA_AutoBoot_Receiver";
    @Override
    public void onReceive(Context context, Intent intent) {
        if(Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction()) ||
        Intent.ACTION_LOCKED_BOOT_COMPLETED.equals((intent.getAction()))) {
            Log.i(TAG, "Device is booted. Trying to start USTA Service");
            ComponentName USTAService = context.startService(
                    new Intent().setComponent(
                            new ComponentName(context.getPackageName(), USTAServiceImpl.class.getName())
                    )
            );
            if(null ==USTAService) {
                Log.e(TAG, "Not able to start USTAservice ");
            }
        }
    }
}
