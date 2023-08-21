/**
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file SSMVideoFile.kt
 */
package com.qualcomm.qti.mmca.ssmeditor

import com.qualcomm.qti.mmca.util.FileSystem
import com.qualcomm.qti.mmca.util.VideoFile

class SSMVideoFile : VideoFile() {

    override val relativePath = cameraSaveRootDir + "/SSMEditor"

    private var _displayName: String? = null
    override var displayName: String
        get() {
            if (_displayName == null) {
                _displayName = "SSM_${FileSystem.getDateTimeString()}.mp4"
            }
            return _displayName!!
        }
        set(value) {
            _displayName = value
        }

}
