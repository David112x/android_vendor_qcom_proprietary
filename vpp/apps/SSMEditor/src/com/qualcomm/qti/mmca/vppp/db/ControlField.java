/* ControlField.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.mmca.vppp.db;

import android.os.Parcel;
import android.os.Parcelable;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * POJO to match the JSON schema for configuration ControlField.
 */
public class ControlField implements Parcelable {

    public static class Keys {
        public static final String KEY = "control-fields";
        public static final String ID = "id";
        public static final String DISPLAY_NAME = "display-name";
        public static final String UI_CONTROL_TYPE = "ui-control-type";
        public static final String VENDOR_EXTENSION = "vendor-extension";
        public static final String VENDOR_EXTENSION_TYPE = "vendor-extension-type";
        public static final String PARAMS = "params";

        public static class ControlType {
            public static final String RADIO_GROUP = "radio-group";
            public static final String SLIDER_CONTINUOUS = "slider-continuous";
            public static final String SLIDER_DISCRETE = "slider-discrete";
        }

        public static class ExtensionType {
            public static final String INT = "int";
            public static final String DOUBLE = "double";
            public static final String FLOAT = "float";
            public static final String STRING = "string";
        }

        public static class Params {
            public static final String MIN = "min";
            public static final String MAX = "max";
            public static final String STEP = "step";
            public static final String DEFAULT = "default";
            public static final String VALUES = "values";
        }
    }

    public enum UIControlType {
        RADIO_GROUP(0), SLIDER_CONTINUOUS(1), SLIDER_DISCRETE(2);

        private final int mValue; // Needed for ListView.getItemViewType()
        private UIControlType(int value) {
            mValue = value;
        }

        public int getValue() {
            return mValue;
        }

        public static UIControlType valueOf(int value) {
            switch (value) {
                case 0:
                    return RADIO_GROUP;
                case 1:
                    return SLIDER_CONTINUOUS;
                case 2:
                    return SLIDER_DISCRETE;
            }

            return null;
        }
    }

    private String mId;
    private String mDisplayName;
    private String mVendorExtension;

    private VendorExtType mVendorExtensionType;

    private Params mParams;

    private UIControlType mUIControlType;

    public ControlField(String id, String displayName, String vendorExtension,
                        VendorExtType vendorExtensionType, Params params,
                        UIControlType uiControlType) {

        this.mId = id;
        this.mDisplayName = displayName;
        this.mVendorExtension = vendorExtension;
        this.mVendorExtensionType = vendorExtensionType;
        this.mParams = params;
        this.mUIControlType = uiControlType;
    }

    public ControlField(ControlField controlField) {
        this.mId = controlField.mId;
        this.mDisplayName = controlField.mDisplayName;
        this.mVendorExtension = controlField.mVendorExtension;
        this.mVendorExtensionType = controlField.mVendorExtensionType;
        if (controlField.mParams != null) {
            this.mParams = new Params(controlField.mParams);
        }
        this.mUIControlType = controlField.mUIControlType;
    }

    private ControlField(Parcel in) {
        this.mId = in.readString();
        this.mDisplayName = in.readString();
        this.mVendorExtension = in.readString();
        this.mVendorExtensionType = VendorExtType.valueOf(in.readString());
        this.mParams = (Params) in.readSerializable();
        this.mUIControlType = UIControlType.valueOf(in.readString());
    }

    public String getId() {
        return mId;
    }

    public String getDisplayName() {
        return mDisplayName;
    }

    public String getVendorExtension() {
        return mVendorExtension;
    }

    public VendorExtType getVendorExtensionType() {
        return mVendorExtensionType;
    }

    public Params getParams() {
        return mParams;
    }

    public UIControlType getUIControlType() {
        return mUIControlType;
    }

    public static final Parcelable.Creator<ControlField> CREATOR =
        new Parcelable.Creator<ControlField>() {
        @Override
        public ControlField createFromParcel(Parcel source) {
            return new ControlField(source);
        }

        @Override
        public ControlField[] newArray(int size) {
            return new ControlField[size];
        }
    };

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(this.mId);
        dest.writeString(this.mDisplayName);
        dest.writeString(this.mVendorExtension);
        dest.writeString(this.mVendorExtensionType.name());
        dest.writeSerializable(this.mParams);
        dest.writeString(this.mUIControlType.name());
    }

    public static class Params<ValueType extends Serializable> implements Serializable {

        // For radio groups + discrete sliders
        private List<Value<ValueType>> mValues;

        // For continuous sliders
        private ValueType mMin;
        private ValueType mMax;
        private ValueType mStep;

        private ValueType mValue;

        public Params(List<Value<ValueType>> values, ValueType defaultValue) {
            mValues = values;
            mValue = defaultValue;
        }

        public Params(ValueType min, ValueType max, ValueType step, ValueType defaultValue) {
            mMin = min;
            mMax = max;
            mStep = step;
            mValue = defaultValue;
        }

        public Params(Params params) {
            // Copy values
            if (params.mValues != null) {
                this.mValues = new ArrayList<>(params.mValues.size());
                for (Object value : params.mValues) {
                    if (value instanceof Value) {
                        this.mValues.add(new Value<ValueType>((Value) value));
                    }
                }
            }

            this.mMin = (ValueType) params.mMin;
            this.mMax = (ValueType) params.mMax;
            this.mStep = (ValueType) params.mStep;
            this.mValue = (ValueType) params.mValue;
        }

        public List<Value<ValueType>> getValues() {
            return Collections.unmodifiableList(mValues);
        }

        public ValueType getMin() {
            return mMin;
        }

        public ValueType getMax() {
            return mMax;
        }

        public ValueType getStep() {
            return mStep;
        }

        public ValueType getValue() {
            return mValue;
        }

        public void setValue(ValueType newValue) {
            mValue = newValue;
        }

        /**
         * Returns the index in the list of values which corresponds
         * to defaultValue.
         *
         * @return index in values or -1 if not found
         */
        public int getValueIndex() {
            if (mValues == null || mValues.size() == 0) {
                return -1;
            }

            for (int i = 0; i < mValues.size(); i++) {
                if (mValues.get(i).getValue().equals(mValue)) {
                    return i;
                }
            }

            return -1;
        }

        public static class Value<T> implements Serializable {

            private String mDisplay;
            private T mValue;

            public Value(String display, T value) {
                this.mDisplay = display;
                this.mValue = value;
            }

            public Value(Value v) {
                this.mDisplay = v.mDisplay;
                this.mValue = (T) v.mValue;
            }

            public String getDisplay() {
                return mDisplay;
            }

            public T getValue() {
                return mValue;
            }

            @Override
            public boolean equals(Object o) {
                if (this == o) return true;
                if (o == null || getClass() != o.getClass()) return false;
                Value<?> value1 = (Value<?>) o;
                return mDisplay.equals(value1.mDisplay) &&
                        mValue.equals(value1.mValue);
            }
        }
    }
}
