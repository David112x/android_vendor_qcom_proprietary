/* JsonConfigParser.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.mmca.vppp.db;

import android.util.Log;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

/**
 * Helper class to handle parsing from json configurations to matching POJOs.
 */
public class JsonConfigParser {

    private static final String TAG = "JsonConfigParser";

    public static List<ChipConfig> parseChipConfig(JSONObject jsonChipDefinitions)
            throws JSONException {
        JSONArray jsonChipConfig = jsonChipDefinitions.getJSONArray(ChipConfig.Keys.KEY);

        List<ChipConfig> chipConfigList = new ArrayList<>();

        // Parse chip definitions
        for (int i = 0; i < jsonChipConfig.length(); i++) {
            JSONObject jsonObject = jsonChipConfig.getJSONObject(i);

            String target = jsonObject.getString(ChipConfig.Keys.TARGET);

            JSONArray jsonConfigs = jsonObject.getJSONArray(ChipConfig.Keys.CONFIGS);
            List<ChipConfig.Config> configList = new ArrayList<>();

            // Parse config specs
            for (int j = 0; j < jsonConfigs.length(); j++) {
                JSONObject jsonConfig = jsonConfigs.getJSONObject(j);

                int os = jsonConfig.getInt(ChipConfig.Keys.OS_VERSION);

                // Parse preset ids
                JSONArray jsonPresetIds = jsonConfig.getJSONArray(ChipConfig.Keys.PRESET_IDS);
                List<String> presetIds = new ArrayList<>();
                for (int k = 0; k < jsonPresetIds.length(); k++) {
                    presetIds.add((String) jsonPresetIds.get(k));
                }

                configList.add(new ChipConfig.Config(os, presetIds));
            }

            chipConfigList.add(new ChipConfig(target, configList));
        }

        return chipConfigList;
    }

    public static List<Preset> parsePreset(JSONObject presetJson) throws JSONException {
        JSONArray jsonPresets = presetJson.getJSONArray(Preset.Keys.KEY);
        List<Preset> presetList = new ArrayList<>();

        // Parse Preset
        for (int i = 0; i < jsonPresets.length(); i++) {
            JSONObject jsonPreset = jsonPresets.getJSONObject(i);

            String id = jsonPreset.getString(Preset.Keys.ID);
            String displayName = jsonPreset.getString(Preset.Keys.DISPLAY_NAME);

            JSONArray jsonControls = jsonPreset.getJSONArray(Preset.Keys.CONTROLS);
            List<Preset.OverrideControl> overrideControlList = new ArrayList<>();

            // Parse Controls
            for (int j = 0; j < jsonControls.length(); j++) {
                JSONObject jsonControl = jsonControls.getJSONObject(j);

                String groupId = jsonControl.getString(Preset.Keys.CONTROL_GROUP_ID);

                if (jsonControl.has(Preset.Keys.FIELDS)) {
                    // Parse fields
                    JSONArray jsonFields = jsonControl.getJSONArray(Preset.Keys.FIELDS);
                    List<Preset.OverrideControl.FieldOverride> fieldOverrideList =
                        new ArrayList<>();

                    for (int k = 0; k + 1 < jsonFields.length(); k += 2) {
                        fieldOverrideList.add(new Preset.OverrideControl.FieldOverride(
                                (String) jsonFields.get(k), jsonFields.get(k + 1)));
                    }

                    overrideControlList.add(new Preset.OverrideControl(groupId, fieldOverrideList));

                } else {
                    overrideControlList.add(new Preset.OverrideControl(groupId));
                }
            }

            presetList.add(new Preset(id, displayName, overrideControlList));
        }

        return presetList;
    }

    public static List<ControlGroup> parseControlGroup(JSONObject controlGroupJson)
            throws JSONException {
        JSONArray jsonGroups = controlGroupJson.getJSONArray(ControlGroup.Keys.KEY);
        List<ControlGroup> controlGroupList = new ArrayList<>();

        // parse control group definitions
        for (int i = 0; i < jsonGroups.length(); i++) {
            JSONObject jsonGroup = jsonGroups.getJSONObject(i);

            String id = jsonGroup.getString(ControlGroup.Keys.ID);
            String displayName = jsonGroup.getString(ControlGroup.Keys.DISPLAY_NAME);

            JSONArray jsonFieldIds = jsonGroup.getJSONArray(ControlGroup.Keys.CONTROL_FIELD_IDS);
            List<String> fieldIds = new ArrayList<>();
            for (int j = 0; j < jsonFieldIds.length(); j++) {
                fieldIds.add((String) jsonFieldIds.get(j));
            }

            controlGroupList.add(new ControlGroup(id, displayName, fieldIds));
        }

        return controlGroupList;
    }

    public static List<ControlField> parseControlField(JSONObject controlFieldJson)
            throws JSONException {
        JSONArray jsonFields = controlFieldJson.getJSONArray(ControlField.Keys.KEY);
        List<ControlField> controlFieldList = new ArrayList<>();

        // parse control field definitions
        for (int i = 0; i < jsonFields.length(); i++) {
            JSONObject jsonField = jsonFields.getJSONObject(i);

            String id = jsonField.getString(ControlField.Keys.ID);
            String displayName = jsonField.getString(ControlField.Keys.DISPLAY_NAME);
            ControlField.UIControlType controlType =
                    parseControlType(jsonField.getString(ControlField.Keys.UI_CONTROL_TYPE));
            String vendorExt = jsonField.getString(ControlField.Keys.VENDOR_EXTENSION);
            VendorExtType extType =
                    parseExtType(jsonField.getString(ControlField.Keys.VENDOR_EXTENSION_TYPE));
            ControlField.Params params =
                    parseParams(jsonField.getJSONObject(ControlField.Keys.PARAMS),
                            controlType, extType);

            controlFieldList.add(
                    new ControlField(id, displayName, vendorExt, extType, params, controlType));
        }

        return controlFieldList;
    }

    private static ControlField.UIControlType parseControlType(String type) throws JSONException {
        switch (type) {
            case ControlField.Keys.ControlType.RADIO_GROUP:
                return ControlField.UIControlType.RADIO_GROUP;
            case ControlField.Keys.ControlType.SLIDER_CONTINUOUS:
                return ControlField.UIControlType.SLIDER_CONTINUOUS;
            case ControlField.Keys.ControlType.SLIDER_DISCRETE:
                return ControlField.UIControlType.SLIDER_DISCRETE;
            default:
                throw new JSONException("parseControlType: Unknown control type: " + type);
        }
    }

    private static VendorExtType parseExtType(String type) throws JSONException {
        switch (type) {
            case ControlField.Keys.ExtensionType.INT:
                return VendorExtType.INT;
            case ControlField.Keys.ExtensionType.DOUBLE:
                return VendorExtType.DOUBLE;
            case ControlField.Keys.ExtensionType.FLOAT:
                return VendorExtType.FLOAT;
            case ControlField.Keys.ExtensionType.STRING:
                return VendorExtType.STRING;
            default:
                throw new JSONException("parseExtType: Unknown extension type: " + type);
        }
    }

    private static ControlField.Params parseParams(JSONObject jsonParams,
                                                   ControlField.UIControlType controlType,
                                                   VendorExtType extType) throws JSONException {
        if (controlType == ControlField.UIControlType.SLIDER_CONTINUOUS) {
            // slider continuous
            if (extType == VendorExtType.INT) {
                int min = jsonParams.getInt(ControlField.Keys.Params.MIN);
                int max = jsonParams.getInt(ControlField.Keys.Params.MAX);
                int step = jsonParams.getInt(ControlField.Keys.Params.STEP);
                int defaultVal = jsonParams.getInt(ControlField.Keys.Params.DEFAULT);

                return new ControlField.Params<>(min, max, step, defaultVal);
            } else if (extType == VendorExtType.DOUBLE) {
                double min = jsonParams.getDouble(ControlField.Keys.Params.MIN);
                double max = jsonParams.getDouble(ControlField.Keys.Params.MAX);
                double step = jsonParams.getDouble(ControlField.Keys.Params.STEP);
                double defaultVal = jsonParams.getDouble(ControlField.Keys.Params.DEFAULT);

                return new ControlField.Params<>(min, max, step, defaultVal);
            } else if (extType == VendorExtType.FLOAT) {
                float min = Float.valueOf(jsonParams.getString(ControlField.Keys.Params.MIN));
                float max = Float.valueOf(jsonParams.getString(ControlField.Keys.Params.MAX));
                float step = Float.valueOf(jsonParams.getString(ControlField.Keys.Params.STEP));
                float defaultVal = Float.valueOf(jsonParams.getString(ControlField.Keys.Params.DEFAULT));

                return new ControlField.Params<>(min, max, step, defaultVal);
            } else {
                Log.e(TAG, "parseParams: Illegal extension type for continuous slider: "
                        + extType);
            }

            // No other types to handle
        } else {
            // radio groups and discrete sliders
            JSONArray jsonValues = jsonParams.getJSONArray(ControlField.Keys.Params.VALUES);

            if (extType == VendorExtType.INT) {
                List<ControlField.Params.Value<Integer>> values = new ArrayList<>(jsonValues.length());

                // parse the values
                for (int i = 0; i + 1 < jsonValues.length(); i += 2) {
                    values.add(new ControlField.Params.Value<>(jsonValues.getString(i),
                            jsonValues.getInt(i + 1)));
                }

                int defaultVal = jsonParams.getInt(ControlField.Keys.Params.DEFAULT);

                return new ControlField.Params<>(values, defaultVal);
            } else if (extType == VendorExtType.DOUBLE) {
                List<ControlField.Params.Value<Double>> values = new ArrayList<>(jsonValues.length());

                // parse the values
                for (int i = 0; i + 1 < jsonValues.length(); i += 2) {
                    values.add(new ControlField.Params.Value<>(jsonValues.getString(i),
                            jsonValues.getDouble(i + 1)));
                }

                double defaultVal = jsonParams.getDouble(ControlField.Keys.Params.DEFAULT);

                return new ControlField.Params<>(values, defaultVal);
            } else if (extType == VendorExtType.FLOAT) {
                List<ControlField.Params.Value<Float>> values = new ArrayList<>(jsonValues.length());

                // parse the values
                for (int i = 0; i + 1 < jsonValues.length(); i += 2) {
                    values.add(new ControlField.Params.Value<>(jsonValues.getString(i),
                            Float.valueOf(jsonValues.getString(i + 1))));
                }

                float defaultVal = Float.valueOf(jsonParams.getString(ControlField.Keys.Params.DEFAULT));

                return new ControlField.Params<>(values, defaultVal);
            } else if (extType == VendorExtType.STRING) {
                List<ControlField.Params.Value<String>> values = new ArrayList<>(jsonValues.length());

                // parse the values
                for (int i = 0; i + 1 < jsonValues.length(); i += 2) {
                    values.add(new ControlField.Params.Value<>(jsonValues.getString(i),
                            jsonValues.getString(i + 1)));
                }

                String defaultVal = jsonParams.getString(ControlField.Keys.Params.DEFAULT);

                return new ControlField.Params<>(values, defaultVal);
            } else {
                Log.e(TAG, "parseParams: Illegal extension type for discrete element: "
                        + extType);
            }
        }

        return null;
    }

}
