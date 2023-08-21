/**
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file MainActivity.java
 */
package com.qualcomm.qti.ssmeditor;

import android.app.Activity;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
import android.view.View;
import android.content.Intent;

import com.qualcomm.qti.mmca.vppp.VPPPActivity;
import com.qualcomm.qti.mmca.vpt.VPTCaptureActivity;

public class MainActivity extends Activity {

    public static final String TAG = "MainActivity";

    private static final int GET_VIDEO_FILE = 01;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Log.d(TAG, "Startup");

        Button btnQptCapture = findViewById(R.id.btn_qpt_capture);
        btnQptCapture.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Log.d(TAG, "Launching QPT Capture");
                Intent intent = new Intent(MainActivity.this, VPTCaptureActivity.class);
                startActivity(intent);
            }
        });

        Button btnVppp = findViewById(R.id.btn_vpp_player);
        btnVppp.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Log.d(TAG, "Launching VPP Player");
                Intent videoChooserIntent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                videoChooserIntent.addCategory(Intent.CATEGORY_OPENABLE);
                videoChooserIntent.setType("video/*");
                startActivityForResult(videoChooserIntent, GET_VIDEO_FILE);
            }
        });
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent resultData) {
        if (requestCode == GET_VIDEO_FILE) {
            if (resultCode == Activity.RESULT_OK) {
                if (resultData != null) {
                    Log.d(TAG, "Video uri: " + resultData.getData().toString());
                    Uri selectedVideoUri = resultData.getData();

                    Log.d(TAG, "Launching Controls Demo");
                    Intent intent = new Intent(MainActivity.this, VPPPActivity.class);
                    intent.setData(selectedVideoUri);
                    startActivity(intent);
                }
            } else {
                Log.d(TAG, "onActivityResult: code = " + resultCode);
            }
        }
    }
}

