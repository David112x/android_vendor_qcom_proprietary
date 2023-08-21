/**
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file ExtensionHolder.java
 */

package com.qualcomm.qti.mmca.vppp.ui;

import android.media.MediaFormat;
import android.os.Bundle;

import com.qualcomm.qti.mmca.decoder.MediaCodecParams;
import com.qualcomm.qti.mmca.vppp.db.ControlField;
import com.qualcomm.qti.mmca.vppp.db.VendorExtType;

/**
 * Data holder class for callbacks when an extension setting is changed.
 */
public class ExtensionHolder implements MediaCodecParams {

    private String mId;
    private String mVendorExt;
    private VendorExtType mVendorExtType;
    private Object mValue;

    public ExtensionHolder(ControlField controlField) {
        mId = controlField.getId();
        mVendorExt = controlField.getVendorExtension();
        mVendorExtType = controlField.getVendorExtensionType();
        mValue = controlField.getParams().getValue();
    }

    public String getId() {
        return mId;
    }

    public String getVendorExt() {
        return mVendorExt;
    }

    public VendorExtType getVendorExtType() {
        return mVendorExtType;
    }

    /**
     * This should be downcasted in the accessor class based on
     * {@link #mVendorExtType}.
     */
    public Object getValue() {
        return mValue;
    }

    @Override
    public MediaFormat pack(MediaFormat mediaFormat) {
        switch (mVendorExtType) {
            case INT:
                mediaFormat.setInteger(mVendorExt, Integer.valueOf(mValue.toString()));
                break;
            case DOUBLE:
                // There is no MediaFormat.setDouble()
                mediaFormat.setFloat(mVendorExt, Float.valueOf(mValue.toString()));
                break;
            case FLOAT:
                mediaFormat.setFloat(mVendorExt, Float.valueOf(mValue.toString()));
                break;
            case STRING:
                mediaFormat.setString(mVendorExt, mValue.toString());
                break;
            default:
                // TODO express error
                break;
        }

        return mediaFormat;
    }

    @Override
    public Bundle pack(Bundle bundle) {
        switch (mVendorExtType) {
            case INT:
                bundle.putInt(mVendorExt, Integer.valueOf(mValue.toString()));
                break;
            case DOUBLE:
                bundle.putDouble(mVendorExt, Double.valueOf(mValue.toString()));
                break;
            case FLOAT:
                bundle.putFloat(mVendorExt, Float.valueOf(mValue.toString()));
                break;
            case STRING:
                bundle.putString(mVendorExt, mValue.toString());
                break;
            default:
                // TODO express error
                break;
        }

        return bundle;
    }
}

