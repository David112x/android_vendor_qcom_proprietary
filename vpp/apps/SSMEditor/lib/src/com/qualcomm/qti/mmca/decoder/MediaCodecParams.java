/**
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file MediaCodecParams.java
 */

package com.qualcomm.qti.mmca.decoder;

import android.media.MediaFormat;
import android.os.Bundle;

/**
 * Interface for setting media codec parameters onto a data format.
 */
public interface MediaCodecParams {

    // Vendor extension strings
    String VPP_MODE = "vendor.qti-ext-vpp.mode";
    String VPP_FRC_MODE = "vendor.qti-ext-vpp-frc.mode";
    String VPP_FRC_LVL = "vendor.qti-ext-vpp-frc.level";
    String VPP_FRC_INTERP = "vendor.qti-ext-vpp-frc.interp";
    String VPP_FRC_TS_START = "vendor.qti-ext-vpp-frc.ts_start";
    String VPP_FRC_FRM_CPY_FB = "vendor.qti-ext-vpp-frc.frame_copy_on_fallback";
    String VPP_FRC_FRM_CPY_IN = "vendor.qti-ext-vpp-frc.frame_copy_input";
    String VPP_NONREALTIME_ENABLE = "vendor.qti-ext-vpp-nonrealtime.enable";

    MediaFormat pack(MediaFormat mediaFormat);

    Bundle pack(Bundle bundle);

}
