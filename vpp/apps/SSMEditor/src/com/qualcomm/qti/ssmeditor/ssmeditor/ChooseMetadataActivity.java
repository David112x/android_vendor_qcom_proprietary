/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file ChooseMetadataActivity.java
 */
package com.qualcomm.qti.ssmeditor.ssmeditor;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

import java.io.File;
import java.util.ArrayList;

import com.qualcomm.qti.ssmeditor.R;

public class ChooseMetadataActivity extends Activity {

    public static final String METADATA_FILENAME_RESULT = "result";

    private static final String TAG = "ChooseMetadataActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_choose_metadata);

        ArrayList<String> jFiles = new ArrayList<String>();
        findJsonFiles(getExternalFilesDir(Environment.DIRECTORY_DCIM), jFiles);

        ArrayAdapter<String> filesAdapter = new ArrayAdapter<String>(this,
                android.R.layout.simple_list_item_1, jFiles);

        ListView listView = findViewById(R.id.metadataList);
        listView.setAdapter(filesAdapter);

        listView.setOnItemClickListener(new ListView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position,
                    long id) {
                TextView textView = (TextView) view;
                Intent launcherIntent = new Intent();
                launcherIntent.putExtra(METADATA_FILENAME_RESULT, textView.getText());
                setResult(Activity.RESULT_OK, launcherIntent);
                finish();
            }
        });
    }

    private ArrayList<String> findJsonFiles(File metaDir, ArrayList<String> jsonFiles) {
        File files[] = metaDir.listFiles();
        String json = "json";

        if (files != null) {
            for (File file: files) {
                Log.d(TAG, "Searching: " + file.getName());
                if (file.isDirectory()) {
                    findJsonFiles(file, jsonFiles);
                } else {
                    if (file.getName().endsWith(json)) {
                        Log.d(TAG, "Found " + file.getName());
                        jsonFiles.add(metaDir.toString() + File.separator
                                + file.getName());
                    }
                }
            }
        }
        return jsonFiles;
    }
}
