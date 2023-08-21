/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file AlertDialogHelper.java
 */
package com.qualcomm.qti.mmca.util;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.provider.Settings;

public class AlertDialogHelper {

    public static void showPermissionsDeniedDialog(final Activity activity, String message) {
        AlertDialog.Builder builder = new AlertDialog.Builder(activity);

        DialogInterface.OnClickListener pListener = new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
                Intent settingsLaunch = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
                Uri settingsUri = Uri.fromParts("package", activity.getPackageName(), null);
                settingsLaunch.setData(settingsUri);
                activity.startActivityForResult(settingsLaunch, 1);
            }
        };

        DialogInterface.OnClickListener cListener = new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
                activity.finish();
            }
        };

        DialogInterface.OnDismissListener dListener = new DialogInterface.OnDismissListener() {
            public void onDismiss(DialogInterface dialog) {
                activity.finish();
            }
        };

        builder.setMessage(message)
                .setTitle("Permissions Denied")
                .setPositiveButton("Ok", pListener)
                .setNegativeButton("Close", cListener)
                .setOnDismissListener(dListener);

        AlertDialog dialog = builder.create();
        dialog.show();
    }
}
