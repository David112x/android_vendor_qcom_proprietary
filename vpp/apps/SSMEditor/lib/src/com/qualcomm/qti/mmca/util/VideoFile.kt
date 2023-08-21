/**
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file VideoFile.kt
 */
package com.qualcomm.qti.mmca.util

import android.content.ContentValues
import android.content.Context
import android.net.Uri
import android.os.ParcelFileDescriptor
import android.provider.MediaStore
import android.util.Log
import java.text.SimpleDateFormat
import java.util.*

open class VideoFile {

    open val cameraSaveRootDir = "DCIM"
    var uri: Uri? = null
    open var displayName: String = ""
    open val mimeType: String = "video/mp4"
    open val relativePath = cameraSaveRootDir

    var context: Context? = null
    private var fd: ParcelFileDescriptor? = null

    fun create(): Uri? {
        val values = ContentValues().apply {
            put(MediaStore.Video.Media.DISPLAY_NAME, displayName)
            put(MediaStore.Video.Media.MIME_TYPE, mimeType)
            put(MediaStore.Video.Media.RELATIVE_PATH, relativePath)
            put(MediaStore.Video.Media.IS_PENDING, 1)
        }

        val collection = MediaStore.Video.Media.getContentUri(
            MediaStore.VOLUME_EXTERNAL_PRIMARY)
        uri = context!!.contentResolver.insert(collection, values)
        return uri
    }

    fun delete() {
        closeFileDescriptor()
        uri?.let { u -> context?.contentResolver?.delete(u, null, null) }
        uri = null
    }

    fun publish() {
        if (uri == null) {
            Log.e(TAG, "can not publish, no uri: $this")
            return
        }

        val values = ContentValues().apply {
            put(MediaStore.Video.Media.IS_PENDING, 0)
        }
        context!!.contentResolver.update(uri!!, values, null, null)
    }

    fun openFileDescriptor(mode: String = "rw"): ParcelFileDescriptor? {
        Log.d(TAG, "$this: opening fd with mode $mode")
        fd = context!!.contentResolver.openFileDescriptor(uri!!, mode)
        return fd
    }

    fun closeFileDescriptor() {
        fd?.let {
            Log.d(TAG, "$this: closing fd")
            fd!!.close()
        } ?: let {
            Log.e(TAG, "$this: trying to close an unopened fd")
        }
        fd = null
    }

    fun getParcelFileDescriptor(): ParcelFileDescriptor? = fd

    override fun toString(): String = "VideoFile ($displayName)"
}
