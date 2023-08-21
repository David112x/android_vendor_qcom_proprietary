/**
 * Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file QtiApplication.java
 */

package com.qualcomm.qti.ssmeditor;

import android.app.Application;
import android.util.Log;

import com.qualcomm.qti.mmca.util.FileSystem;
import com.qualcomm.qti.mmca.vppp.db.ConfigRepo;

import org.json.JSONException;

public class QtiApplication extends Application {

    private static final String TAG = "QtiApplication";

    private ConfigRepo mConfigRepo;

    @Override
    public void onCreate() {
        Log.d(TAG, "OnCreate");
        super.onCreate();
    }

    @Override
    public void onTrimMemory(int level) {
        Log.d(TAG, "onTrimMemory");
        super.onTrimMemory(level);
    }

    @Override
    public void onTerminate() {
        Log.d(TAG, "onTerminate");
        super.onTerminate();
    }

    /**
     * Creates a ConfigRepo for the app
     * TODO parameterize this
     */
    private void createConfigRepo() throws JSONException  {
        mConfigRepo = new ConfigRepo(getAssets(), FileSystem.getUserConfigDir(this));
    }

    /**
     * Get the app instance of the ConfigRepo
     */
    public ConfigRepo getConfigRepo() throws JSONException {
        if (mConfigRepo == null) {
            createConfigRepo();
        }

        return mConfigRepo;
    }
}
