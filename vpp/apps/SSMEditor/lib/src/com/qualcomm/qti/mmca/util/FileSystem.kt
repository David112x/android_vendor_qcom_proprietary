/**
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file FileSystem.kt
 */
package com.qualcomm.qti.mmca.util

import android.content.Context
import java.io.File
import java.text.SimpleDateFormat
import java.util.*

const val TAG = "FileSystem"

open class FileSystem {

    companion object {
        @JvmStatic
        fun getUserConfigDir(ctx: Context) =
            File(ctx.getExternalFilesDir(null), "user_configs")

        @JvmStatic
        fun getDateTimeString(): String =
            SimpleDateFormat("yyyyMMdd_HHmmss").format(Date())
    }
}
