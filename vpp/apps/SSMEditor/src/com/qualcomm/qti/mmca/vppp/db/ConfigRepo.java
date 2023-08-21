/* ConfigRepo.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.mmca.vppp.db;

import android.content.res.AssetManager;
import android.util.Log;

import com.qualcomm.qti.ssmeditor.util.JsonLoader;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

import static android.content.res.AssetManager.ACCESS_BUFFER;

/**
 * Database class for configurations. Handles loading, retrieval, and saving of the configurations.
 * This should be a singleton class accessible using the Application Context.
 */
public class ConfigRepo {

    private static final String TAG = "ConfigRepo";

    private static final String CONFIG_PATH = "vppp";

    private HashMap<String, ChipConfig> mAllChipConfigs;
    private ChipConfig.Config mThisConfig; // Holder for device matching config

    private HashMap<String, Preset> mPresets;
    private HashMap<String, ControlGroup> mControlGroups;
    private HashMap<String, ControlField> mControlFields;

    /**
     * Creates a ConfigRepo based on configurations stored in the provided directories.
     * @param am AssetManager which should be used to search for pre-bundled configurations.
     *           Should not be null.
     * @param userConfigDir Directory where the user pushed configurations are stored.
     *                      Should not be null.
     * @throws JSONException Parsing error due to invalid JSON.
     */
    public ConfigRepo(AssetManager am, File userConfigDir) throws JSONException {
        mAllChipConfigs = new HashMap<>();
        mPresets = new HashMap<>();
        mControlGroups = new HashMap<>();
        mControlFields = new HashMap<>();

        // Parse pre-defined configs
        try {
            String[] bundledConfigFiles = am.list(CONFIG_PATH);
            Log.d(TAG, "ConfigRepo: num configs: " + bundledConfigFiles.length);
            for (String config : bundledConfigFiles) {
                parseAssetFile(am, CONFIG_PATH + "/" + config);
            }
        } catch (IOException | JSONException e) {
            e.printStackTrace();
        }

        // Parse user-configs
        File[] userConfigFiles = userConfigDir.listFiles();
        if (userConfigFiles != null) {
            Log.d(TAG, "ConfigRepo: num user configs: " + userConfigFiles.length);
            parseFiles(userConfigFiles);
        } else {
            Log.e(TAG, "No user configs found.");
        }

        // Populate the tree and link all objects together
        populateTree();
    }

    /**
     * Parses configuration JSON files that are located in the AssetManager, given by the
     * specified relative path from the root of the AssetManager.
     * @param am The asset manager to search for files from
     * @param file
     * @throws IOException Errors occurring as a result of invalid files in the AssetManager
     * @throws JSONException Parsing error due to invalid JSON.
     */
    private void parseAssetFile(AssetManager am, String file) throws IOException, JSONException {
        InputStream is = am.open(file, ACCESS_BUFFER);
        String jsonStr = JsonLoader.getJsonFromInputStream(is);
        try {
            parseJsonString(jsonStr);
        } catch (JSONException e) {
            e.printStackTrace();
            // Not a valid json file
            throw new JSONException("Invalid configuration: " + file, e);
        } finally {
            is.close();
        }
    }

    /**
     * Parses configuration JSONs from the {@code files} into their corresponding POJOs and adds
     * them to the database.
     * @param files Configuration JSONs to parse. Should not be null.
     * @throws JSONException Parsing error due to invalid JSON.
     */
    private void parseFiles(File[] files) throws JSONException {
        for (int i = 0; i < files.length; ++i) {
            Log.d(TAG, "parseFiles: file #" + i + " = " + files[i].getName());

            try {
                String jsonStr = JsonLoader.getJsonFromFile(files[i]);
                parseJsonString(jsonStr);
            } catch (JSONException e) {
                // Not a valid json file
                throw new JSONException("Invalid configuration: " + files[i].getName(), e);
            }
        }
    }

    /**
     * Converts a String containing JSON into a JSONObject and parses that object.
     * @param jsonStr the JSON to parse
     */
    private void parseJsonString(String jsonStr) throws JSONException {
        if (jsonStr != null) {
            JSONObject jsonObject = new JSONObject(jsonStr);
            parseJsonObject(jsonObject);
        }
    }

    /**
     * Parses a jsonObject into its match Java class before adding it to its appropriate data store.
     * @param jsonObject json to parse. Should not be null.
     * @throws JSONException Parsing error. Illegal json.
     */
    private void parseJsonObject(JSONObject jsonObject) throws JSONException{
        Iterator<String> keys = jsonObject.keys();

        // Determine the type of config defined in the file based on the root key
        while (keys.hasNext()) {
            String key = keys.next();

            switch (key) {
                case ChipConfig.Keys.KEY:
                    List<ChipConfig> configs = JsonConfigParser.parseChipConfig(jsonObject);
                    configs.forEach(config -> addChipConfig(config));
                    break;

                case Preset.Keys.KEY:
                    List<Preset> presets = JsonConfigParser.parsePreset(jsonObject);
                    presets.forEach(preset -> addPreset(preset));
                    break;

                case ControlGroup.Keys.KEY:
                    List<ControlGroup> controlGroups =
                        JsonConfigParser.parseControlGroup(jsonObject);
                    controlGroups.forEach(controlGroup -> addControlGroup(controlGroup));
                    break;

                case ControlField.Keys.KEY:
                    List<ControlField> controlFields =
                        JsonConfigParser.parseControlField(jsonObject);
                    controlFields.forEach(controlField -> addControlField(controlField));
                    break;

                default:
                    // Not a valid config type
                    Log.e(TAG, "Invalid config type found: " + key);
                    break;
            }
        }
    }

    /**
     * Populates the database tree. Links each ChipConfig to its Configs, each Config to its
     * Presets, each Preset to its ControlGroups, and each ControlGroup to its ControlFields.
     * FieldOverrides in each Preset are also applied accordingly. This deep copies every single
     * element in the tree.
     */
    private void populateTree() {
        // Iterate through the Configs and populate each one
        int size = mAllChipConfigs.size();
        mAllChipConfigs.forEach((key, value) -> linkConfigs(value));
    }

    /**
     * Links a ChipConfig with its populated Configs.
     * @param chipConfig ChipConfig whose tree to populate. Should not be null.
     */
    private void linkConfigs(ChipConfig chipConfig) {
        List<ChipConfig.Config> configs = chipConfig.getConfigs();
        List<ChipConfig.Config> populatedConfigs = new ArrayList<>();

        for (ChipConfig.Config config : configs) {
            if (config != null) {
                ChipConfig.Config configToAdd = new ChipConfig.Config(config);
                linkPresets(configToAdd);
                populatedConfigs.add(configToAdd);
            }
        }

        chipConfig.setConfigs(populatedConfigs);
    }

    /**
     * Links a Config to all its Presets (and those to its ControlGroups and those ControlGroups to
     * their ControlFields)
     * @param config Config whose tree to populate. Should not be null.
     */
    private void linkPresets(ChipConfig.Config config) {
        // Find all the required Presets and copy them into the ChipConfig
        List<String> ids = config.getPresetIds();
        List<Preset> presets = new ArrayList<>();

        for (String id : ids) {
            Preset preset = getPreset(id);

            if (preset != null) {
                Preset presetToAdd = new Preset(preset);
                linkControlGroups(presetToAdd);
                presets.add(presetToAdd);
            }
        }

        config.setPresets(presets);
    }

    /**
     * Links a Preset to its ControlGroups (and those ControlGroups to their ControlFields).
     * Applies the Preset's FieldOverrides on its child ControlFields.
     * @param preset Preset whose tree to populate. Should not be null.
     */
    private void linkControlGroups(Preset preset) {
        // Find all the required ControlGroups and copy them into the Preset
        List<ControlGroup> controlGroups = new ArrayList<>();
        List<Preset.OverrideControl> controls = preset.getOverrideControls();

        for (Preset.OverrideControl control : controls) {
            String id = control.getControlGroupId();
            ControlGroup controlGroup = getControlGroup(id);

            // overrides will not be null
            List<Preset.OverrideControl.FieldOverride> overrides = control.getFieldsToOverride();

            if (controlGroup != null) {
                // Link the controlGroup to its controlFields
                ControlGroup controlGroupToAdd = new ControlGroup(controlGroup);
                linkControlFields(controlGroupToAdd, overrides);
                controlGroups.add(controlGroupToAdd);
            }
        }

        preset.setControlGroups(controlGroups);
    }


    /**
     * Populates the list of ControlFields in a ControlGroup based on the ids. Overrides the
     * default value of the ControlFields if described in {@code fieldOverrides}
     * @param controlGroup ControlGroup whose tree to populate. Should not be null.
     * @param fieldOverrides any overrides (non-null) for ControlFields in this ControlGroup.
     */
    private void linkControlFields(ControlGroup controlGroup,
                                   List<Preset.OverrideControl.FieldOverride> fieldOverrides) {
        // Find all the required ControlFields and copy them into the ControlGroup
        List<ControlField> controlFields = new ArrayList<>();
        List<String> ids = controlGroup.getControlFieldIds();
        for (String id : ids) {
            ControlField controlField = getControlField(id);

            if (controlField != null) {
                ControlField controlFieldToAdd = new ControlField(controlField);
                overrideField(controlFieldToAdd, fieldOverrides);
                controlFields.add(controlFieldToAdd);
            }
        }

        controlGroup.setControlFields(controlFields);
    }

    /**
     * Overrides the defaultValue of the {@code controlField} if and only if there exists a
     * FieldOverride which matches it.
     * @param controlField field whose value to override. Modified if and only if there is a
     *                     matching definition. Should not be null.
     * @param fieldOverrides overrides defined for the {@link ControlGroup} in the {@link Preset}.
     *                       Should not be null.
     */
    private void overrideField(ControlField controlField,
                               List<Preset.OverrideControl.FieldOverride> fieldOverrides) {
        for (Preset.OverrideControl.FieldOverride fieldOverride : fieldOverrides) {
            // Override only if matches
            if (fieldOverride.getFieldId().equals(controlField.getId())) {

                VendorExtType type = controlField.getVendorExtensionType();
                switch (type) {
                    case INT:
                        controlField.getParams()
                                .setValue(Integer.valueOf((String) fieldOverride.getValue()));
                        break;
                    case DOUBLE:
                        controlField.getParams()
                                .setValue(Double.valueOf((String) fieldOverride.getValue()));
                        break;
                    case FLOAT:
                        controlField.getParams()
                                .setValue(Float.valueOf((String) fieldOverride.getValue()));
                        break;
                    case STRING:
                        controlField.getParams()
                                .setValue((String) fieldOverride.getValue());
                        break;
                    default:
                        // No other VendorExtType
                        Log.e(TAG, "Invalid VendorExtType: " + type);
                        break;
                }
            }
        }
    }

    /**
     * Add a ChipConfig to the store.
     * @param chipConfig
     */
    public void addChipConfig(ChipConfig chipConfig) {
        if (chipConfig != null) {
            mAllChipConfigs.put(chipConfig.getTarget(), chipConfig);
        }
    }

    /**
     * Add a Preset to the store.
     * @param preset Should not be null.
     */
    public void addPreset(Preset preset) {
        mPresets.put(preset.getId(), preset);
    }

    /**
     * Add a ControlGroup to the store.
     * @param controlGroup Should not be null.
     */
    public void addControlGroup(ControlGroup controlGroup) {
        mControlGroups.put(controlGroup.getId(), controlGroup);
    }

    /**
     * Add a ControlField to the store.
     * @param controlField Should not be null.
     */
    public void addControlField(ControlField controlField) {
        mControlFields.put(controlField.getId(), controlField);
    }

    private ChipConfig.Config findConfig(List<ChipConfig.Config> configs, int os) {
        ChipConfig.Config config = null;

        for (ChipConfig.Config c : configs) {
            if (c.getOSVersion() == os) {
                return c;
            } else if (c.getOSVersion() < os) {
                if (config == null || c.getOSVersion() > config.getOSVersion()) {
                    config = c;
                }
            }
        }

        return config;
    }

    /**
     * Get the list of presets available for the device's board model and OS version. If no
     * configuration matching the OS version is found, the latest is used. Will be
     * null if no matching configuration is found.
     *
     * @param target  Board target name
     * @param sdk_ver OS Version
     * @return valid configuration matching device specs or {@code null}.
     */
    public ChipConfig.Config getChipConfig(String target, int sdk_ver){
        Log.v(TAG, "getChipConfig: target = " + target + ", sdkVer = " + sdk_ver);
        if (mThisConfig != null) {
            return mThisConfig;
        }

        // Retrieve the matching chip config
        ChipConfig chipConfig = mAllChipConfigs.get(target);
        if (chipConfig != null && !chipConfig.getConfigs().isEmpty()) {
            Log.v(TAG, "getChipConfig: target config found");
            ChipConfig.Config config = findConfig(chipConfig.getConfigs(), sdk_ver);

            if (config != null) {
                Log.v(TAG, "getChipConfig: sdkVer config found");
                mThisConfig = config;
                return mThisConfig;
            }
        }

        return null;
    }

    /**
     * Retrieve a {@link Preset} by its id.
     *
     * @param id
     * @return May be null.
     */
    public Preset getPreset(String id) {
        return mPresets.get(id);
    }

    /**
     * Retrieve a {@link ControlGroup} by its id.
     *
     * @param id
     * @return May be null.
     */
    public ControlGroup getControlGroup(String id) {
        return mControlGroups.get(id);
    }

    /**
     * Retrieve a {@link ControlField} by its id.
     *
     * @param id
     * @return May be null.
     */
    public ControlField getControlField(String id) {
        return mControlFields.get(id);
    }

}
