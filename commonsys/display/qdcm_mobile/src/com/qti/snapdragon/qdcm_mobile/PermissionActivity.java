/*
* Copyright (c) 2018 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
package com.qti.snapdragon.qdcm_mobile;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.content.pm.PackageManager;
import android.support.v4.app.ActivityCompat;
import android.os.Build;
import android.util.Log;
import android.widget.Toast;

import java.util.List;
import java.util.ArrayList;

public class PermissionActivity extends Activity {

    private static final String TAG = "PermissionActivity.java";
    private static final int REQUEST_CODE = 100;
    private final static String[] REQUIRE_PERMISSIONS = {
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
    };

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        checkPermissions();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions,
                                           int[] grantResults) {
        if (requestCode == REQUEST_CODE){
            boolean isAllGranted = true;
            for (int grant : grantResults) {
                if (grant != PackageManager.PERMISSION_GRANTED) {
                    isAllGranted = false;
                    break;
                }
            }
            if (isAllGranted){
                Log.d(TAG," permissions granted success");
            }else {
                Log.e(TAG," permissions granted failed");
                finish();
            }
        }
    }

    private void checkPermissions(){
        String[] neededPermissions = checkRequestedPermission(
                this, REQUIRE_PERMISSIONS);
        if (neededPermissions.length > 0) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                ActivityCompat.requestPermissions(this, neededPermissions, REQUEST_CODE);
                Log.e(TAG, "checkPermissions RequestedPermission size = " +
                        neededPermissions.length);
            }
        }
    }

    private static String[] checkRequestedPermission(Activity activity, String[] permissionName) {
        boolean isPermissionGranted = true;
        List<String> needRequestPermission = new ArrayList<>();
        for (String tmp : permissionName) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                isPermissionGranted = (PackageManager.PERMISSION_GRANTED ==
                        activity.checkSelfPermission(tmp));
            }
            if (!isPermissionGranted) {
                needRequestPermission.add(tmp);
            }
        }
        String[] needRequestPermissionArray = new String[needRequestPermission.size()];
        needRequestPermission.toArray(needRequestPermissionArray);
        return needRequestPermissionArray;
    }
}