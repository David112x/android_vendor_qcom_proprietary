/**
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file VPTFileSystem.kt
 */
package com.qualcomm.qti.mmca.vpt

import android.content.Context
import android.net.Uri
import android.provider.OpenableColumns
import com.qualcomm.qti.mmca.util.FileSystem
import java.io.File

class VPTFileSystem : FileSystem() {
    companion object {
        const val EXTN_JSON = ".vpt.json"
        const val EXTN_PP_JSON = ".vpt_pp.json"
        const val EXTN_MP4 = ".mp4"

        @JvmStatic
        fun getVideoFileNameFromUri(context: Context, uri: Uri): String {
            var filename = ""
            context.contentResolver.query(uri, null, null, null, null, null)?.apply {
                if (moveToFirst()) {
                    filename = getString(
                        getColumnIndex(OpenableColumns.DISPLAY_NAME))
                }
                close()
            }
            return filename
        }

        @JvmStatic
        fun getStorageDirectory(ctx: Context) =
            ctx.getExternalFilesDir(null)

        @JvmStatic
        fun getMetadataFileForVideo(ctx: Context, video: String): File? {
            val metaFileName = video.replace(EXTN_MP4, EXTN_JSON)
            val expectedFile = File(getStorageDirectory(ctx), metaFileName)
            return if (expectedFile.exists()) expectedFile else null
        }

        @JvmStatic
        fun getMetadataFileForVideo(ctx: Context, uri: Uri): File? =
            getMetadataFileForVideo(ctx, getVideoFileNameFromUri(ctx, uri))
    }
}