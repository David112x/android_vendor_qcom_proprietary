/* ControlFieldLVAdapter.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.mmca.vppp.ui;

import android.database.DataSetObserver;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListAdapter;
import android.widget.TextView;

import com.qualcomm.qti.mmca.vppp.db.ControlField;
import com.qualcomm.qti.ssmeditor.R;
import com.qualcomm.qti.mmca.view.chip.ChipView;
import com.qualcomm.qti.mmca.view.chip.RadioChipGroup;

import java.util.List;

/**
 * ListView for showing a list of ControlFields.
 */
public class ControlFieldLVAdapter implements ListAdapter, ControlFieldUIElement {

    private List<ControlField> mControlFields;
    private ControlChangedListener mListener;

    /**
     * @param controlFields Should not be null.
     * @param listener
     */
    public ControlFieldLVAdapter(List<ControlField> controlFields,
                                 ControlChangedListener listener) {
        this.mControlFields = controlFields;
        this.mListener = listener;
    }

    @Override
    public boolean areAllItemsEnabled() {
        return true;
    }

    @Override
    public boolean isEnabled(int position) {
        return getValidPosition(position) == position;
    }

    @Override
    public void registerDataSetObserver(DataSetObserver observer) {
        // Do nothing
    }

    @Override
    public void unregisterDataSetObserver(DataSetObserver observer) {
        // Do nothing
    }

    @Override
    public int getCount() {
        return mControlFields.size();
    }

    @Override
    public Object getItem(int position) {
        return mControlFields.get(getValidPosition(position));
    }

    @Override
    public long getItemId(int position) {
        return 0; // no need for us to associate an id
    }

    @Override
    public int getItemViewType(int position) {
        // Check the ControlField for the viewtype
        return ((ControlField) getItem(getValidPosition(position)))
            .getUIControlType().getValue();
    }

    @Override
    public boolean hasStableIds() {
        return false;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ControlField.UIControlType type = ControlField.UIControlType
                .valueOf(getItemViewType(position));

        final ControlField controlField = (ControlField) getItem(position);

        switch (type) {
            case RADIO_GROUP:
            case SLIDER_DISCRETE:
                // Implementing discrete slider as a radio group for now
                // TODO make a discrete slider
                View view = LayoutInflater.from(parent.getContext())
                        .inflate(R.layout.item_radio_group, parent, false);
                 RadioGroupViewHolder radioHolder = new RadioGroupViewHolder(view);

                radioHolder.mChipGroup.registerListener(new RadioChipGroup.OnCheckedChangedListener() {
                    @Override
                    public void onCheckedChanged(RadioChipGroup chipGroup, ChipView chip, int pos) {
                        // Notify the listener that a control was changed
                        if (mListener != null) {
                            List<ControlField.Params.Value> values = controlField.getParams()
                                    .getValues();
                            Object value = values.get(pos).getValue();

                            // Update the value in the controlField
                            switch (controlField.getVendorExtensionType()) {
                                case INT:
                                    controlField.getParams().setValue((Integer) value);
                                    break;
                                case DOUBLE:
                                    controlField.getParams().setValue((Double) value);
                                    break;
                                case FLOAT:
                                    controlField.getParams().setValue((Float) value);
                                    break;
                                case STRING:
                                    controlField.getParams().setValue((String) value);
                                    break;
                                default:
                                    break;
                            }

                            ExtensionHolder data = new ExtensionHolder(controlField);
                            mListener.onControlChanged(data);
                        }
                    }
                });

                radioHolder.setDataOnView(controlField);
                return radioHolder.mView;

            case SLIDER_CONTINUOUS:
                view = LayoutInflater.from(parent.getContext())
                        .inflate(R.layout.item_slider, parent, false);
                ContinuousSliderViewHolder sliderHolder = new ContinuousSliderViewHolder(view);

                sliderHolder.mContinuousSlider.addOnValueChangeListener(
                        new ContinuousSliderView.OnValueChangeListener() {
                            @Override
                            public void onValueChange(int newValue) {
                                // Notify the listener that a control was changed
                                if (mListener != null) {
                                    // Update the value in the controlField so that
                                    // it can be set after configuration changes.
                                    controlField.getParams().setValue(newValue);

                                    ExtensionHolder data = new ExtensionHolder(controlField);
                                    mListener.onControlChanged(data);
                                }
                            }
                        });

                sliderHolder.setDataOnView(controlField);
                return sliderHolder.mView;
            default:
                break;
        }

        return null;
    }

    @Override
    public int getViewTypeCount() {
        return ControlField.UIControlType.values().length;
    }

    @Override
    public boolean isEmpty() {
        return false;
    }

    @Override
    public CharSequence[] getAutofillOptions() {
        return new CharSequence[0];
    }

    @Override
    public void setControlChangedListener(ControlChangedListener listener) {
        this.mListener = listener;
    }

    private int getValidPosition(int position) throws ArrayIndexOutOfBoundsException {
        if (0 <= position && position < mControlFields.size()) {
            return position;
        }
        throw new ArrayIndexOutOfBoundsException("Index " + position  + " is not valid");
    }

    protected abstract class ViewHolder {
        public View mView;

        public ViewHolder(View view){
            this.mView = view;
        }

        public void setDataOnView(ControlField controlField) {
            if (controlField == null) {
                throw new NullPointerException("No ControlField data for"
                        + " this view.");
            }
        }
    }

    private class RadioGroupViewHolder extends ViewHolder {
        RadioChipGroup mChipGroup;
        TextView mName;

        public RadioGroupViewHolder(View view) {
            super(view);

            mChipGroup = view.findViewById(R.id.chip_group);
            mName = view.findViewById(R.id.group_name);

            mChipGroup.setSaveEnabled(false);
        }

        @Override
        public void setDataOnView(ControlField controlField) {
            super.setDataOnView(controlField);

            mName.setText(controlField.getDisplayName());

            List<ControlField.Params.Value> values = controlField.getParams().getValues();
            for (ControlField.Params.Value value : values) {
                mChipGroup.addChip(value.getDisplay());
            }

            // Set the default
            int valueIndex = controlField.getParams().getValueIndex();
            if (valueIndex != -1) {
                mChipGroup.check(valueIndex);
            }
        }
    }

    private class ContinuousSliderViewHolder extends ViewHolder {
        ContinuousSliderView mContinuousSlider;

        public ContinuousSliderViewHolder(View view) {
            super(view);

            mContinuousSlider = view.findViewById(R.id.control_slider);
        }

        @Override
        public void setDataOnView(ControlField controlField) {
            super.setDataOnView(controlField);

            mContinuousSlider.configure(
                    (int) controlField.getParams().getMin(),
                    (int) controlField.getParams().getMax(),
                    (int) controlField.getParams().getValue(),
                    (int) controlField.getParams().getStep());

            mContinuousSlider.setTitle(controlField.getDisplayName());
        }
    }

    // Empty for now. Should be created when a discrete slider is added
    private class DiscreteSliderViewHolder extends ViewHolder {
        public DiscreteSliderViewHolder(View view) {
            super(view);
        }

        @Override
        public void setDataOnView(ControlField controlField) {
            super.setDataOnView(controlField);
        }
    }
}
