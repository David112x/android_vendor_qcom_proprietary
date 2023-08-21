/* JsonLoader.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.ssmeditor.util;

import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.io.File;

public class JsonLoader {
    private static final String TAG = "JsonLoader";

    public static String getJsonFromFile(File jsonFile) {
        String json = null;
        try {
            json = new String(Files.readAllBytes(jsonFile.toPath()),
                    StandardCharsets.UTF_8);
        } catch (Exception e) {
            Log.e(TAG, "getJSONFromFile exception: " + e);
        }
        return json;
    }

    public static String getJsonFromInputStream(InputStream is) {
        String json = null;
        try {
            byte[] buf = new byte[is.available()];
            is.read(buf);
            json = new String(buf);
        } catch (IOException e) {
            e.printStackTrace();
            Log.e(TAG, "failed to load json from stream");
        }
        return json;
    }
}
