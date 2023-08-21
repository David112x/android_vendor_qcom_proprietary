/* ====================================================================
 * Copyright (c) 2014, 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 * =====================================================================
 * @file IColorServiceImpl.java
 *
 */

package com.qti.service.colorservice;

import java.util.Hashtable;
import java.util.ArrayList;

import android.content.Context;
import android.os.RemoteException;
import android.util.Log;
import com.qti.snapdragon.sdk.display.IColorService;
import com.qti.snapdragon.sdk.display.ModeInfo;
import com.qti.snapdragon.sdk.display.ColorManager;
import com.qti.snapdragon.sdk.display.ColorManager.MODE_TYPE;
import com.qti.snapdragon.sdk.display.ColorManager.DCM_FEATURE;

import vendor.display.color.V1_0.IDisplayColor;
import vendor.display.color.V1_0.Result;
import vendor.display.color.V1_0.DispModeType;
import vendor.display.color.V1_0.RangeFloat;
import vendor.display.color.V1_0.Range;
import vendor.display.color.V1_0.PARange;
import vendor.display.color.V1_0.PAConfig;
import vendor.display.color.V1_0.PAConfigData;

/**
 *
 * This class implements the actual system service for the display SDK.
 * Internally, it mostly calls native display APIs and passes the return values
 * to the app. This class loads the required native libraries.
 *
 */
class IColorServiceImpl extends IColorService.Stub {

    private static final String TAG = "IColorServiceImpl";
    private final Context context;
    private final long[] hctx = {0};
    private float hueScaler, satScaler, valScaler, contScaler;
    private int[] desiredRange = { -180, 180, -50, 100, -100, 100, -100, 100, 0, 1};
    private ArrayList<vendor.display.color.V1_0.ModeInfo> modeListHidl =
                   new ArrayList<vendor.display.color.V1_0.ModeInfo>();
    public static boolean VERBOSE_ENABLED = false;

    //make sure below mapping is always maintained
    //feature mapping
    private static final Hashtable<Integer,Integer> DCM_FEATURE_MAP = new Hashtable<Integer, Integer>()
            {{ //SDK id, HIDL id
                put(0, 0); //COLOR_BALANCE
                put(1, 1); //MODE_SELECTION
                put(2, 2); //MODE_MANAGEMENT
                put(3, 3); //ADAPTIVE_BACKLIGHT
                put(4, 4); //GLOBAL_PA
                put(5, 5); //MEM_COLOR
                put(6, 6); //SVI
            }};
    //end feature mapping

    //display mapping
    private static final Hashtable<Integer, Integer> DCM_DISPLAY_MAP = new Hashtable<Integer, Integer>()
            {{ //SDK id, HIDL id
                put(0, 0); //primary
                put(1, 1); //external
                put(2, 2); //wifi
            }};
    //end display mapping


    /**
     * This constructor runs the service initialisation.
     *
     * @param context
     *            The current context
     * @throws InstantiationException
     *             if hidl initialisation fails
     */
    IColorServiceImpl(Context context) throws InstantiationException {
        this.context = context;
        final int[] result = {Result.PERMISSION_DENIED};
        int flags = 0;
        try {
            IDisplayColor mDisplayColor = IDisplayColor.getService(true);
            if (mDisplayColor != null) {
                mDisplayColor.init(flags, (int tmpReturn, long tmpHandle) -> {
                    result[0] = tmpReturn;
                    hctx[0] = tmpHandle;
                });

                if (result[0] != Result.OK) {
                    Log.e(TAG, "Faild to init display color service");
                    throw new InstantiationException("Init failed");
                }
            } else {
                Log.e(TAG, "Faild to get display color service");
                throw new InstantiationException("Faild to get service");
            }
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException:" + e);
        }
    }


    /**
     * This method checks for the given feature to be supported on the given
     * display type
     *
     * @param displayId
     *            Id of the display to be checked
     * @param featureId
     *            Id of the feature to be checked
     * @return true if supported false if not supported
     */
    public boolean isFeatureSupported(int displayId, int featureId) {
        try {
            IDisplayColor mDisplayColor = IDisplayColor.getService(true);
            if (mDisplayColor == null) {
                Log.e(TAG, "mDisplayColor is null");
                return false;
            }
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException:" + e);
        }

      //feature mapping
        Integer hidlFeatureId = DCM_FEATURE_MAP.get(new Integer(featureId));
        if (hidlFeatureId == null) {
            Log.e(TAG, "Hidl feature "+featureId+ " doesnt exist");
            return false;
        }
        //end feature mapping
        //Display mapping
        Integer hidlDisplayId = DCM_DISPLAY_MAP.get(new Integer(displayId));
        if (hidlDisplayId == null) {
            Log.e(TAG, "Hidl display type "+ displayId + " is not supported");
            return false;
        }
        //end display mapping

        boolean feature_supported = false;
        DCM_FEATURE feature = DCM_FEATURE.values()[featureId];
        switch (feature) {
            case FEATURE_COLOR_BALANCE:
                if (displayId == 0)
                    feature_supported = true;
                break;
            case FEATURE_COLOR_MODE_SELECTION:
                feature_supported = true;
                break;
            case FEATURE_COLOR_MODE_MANAGEMENT:
                feature_supported = true;
                break;
            case FEATURE_ADAPTIVE_BACKLIGHT:
                if (displayId == 0)
                    feature_supported = false;
                break;
            case FEATURE_GLOBAL_PICTURE_ADJUSTMENT:
                feature_supported = true;
                break;
            case FEATURE_MEMORY_COLOR_ADJUSTMENT:
                feature_supported = false;
                break;
            case FEATURE_SUNLIGHT_VISBILITY_IMPROVEMENT:
                if (displayId == 0)
                    feature_supported = false;
                break;
            default:
                Log.e(TAG, "Invalid feature "+ feature);
                break;
        }

        if (!feature_supported)
            Log.e(TAG, "feature " + feature + " not supported");

        if (VERBOSE_ENABLED)
            Log.v(TAG, "displayId: " + displayId + " featureId: " + featureId +
                       " supported: " + feature_supported);
        return feature_supported;
    }

    //////color balance
    /**
     * This methed calls the native API to set the Color balance or warmth
     *
     * @param displayId
     *            display Id for which SVI is to be set
     * @param warmth
     *            value of warmth
     * @return return value from native api as is.
     */
    public int setColorBalance(int displayId, int warmth) {
        //Display mapping
        Integer hidlDisplayId = DCM_DISPLAY_MAP.get(new Integer(displayId));
        if (hidlDisplayId == null) {
            Log.e(TAG, "Hidl display type "+ displayId + " is not supported");
            return -1;
        }
        //end display mapping

        if (VERBOSE_ENABLED)
            Log.v(TAG, "setColorBalance: Entering: displayId " + displayId +
                       " warmth " + warmth);

        int result = Result.PERMISSION_DENIED;
        int flags = 0;
        try {
            IDisplayColor mDisplayColor = IDisplayColor.getService(true);
            if (mDisplayColor == null) {
                Log.e(TAG, "mDisplayColor is null");
                return -1;
            }

            result = mDisplayColor.setCB(hctx[0], displayId, warmth, flags);
            if (result != Result.OK)
                Log.e(TAG, "Faild to call setCB result " + result);
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException:" + e);
        }

        if (VERBOSE_ENABLED)
            Log.v(TAG, "setColorBalance: Exiting: flags " + flags + " result " + result);
        return result;
    }

    /**
     * This method gets the current warmth for the display
     *
     * @param displayId
     *            id of the display for which SVI is required
     * @return the current warmth of the display or error.
     *          To get error value, deduct -100 from returned value
     */
    public int getColorBalance(int displayId) {
        //Display mapping
        Integer hidlDisplayId = DCM_DISPLAY_MAP.get(new Integer(displayId));
        if (hidlDisplayId == null) {
            Log.e(TAG, "Hidl display type "+ displayId + " is not supported");
            return (ColorManager.COLOR_BALANCE_WARMTH_LOWER_BOUND-1);
        }
        //end display mapping

        if (VERBOSE_ENABLED)
            Log.v(TAG, "getColorBalance: Entering: displayId " + displayId);

        final int[] warmness = {-101}; //lowest warmth is -100
        final int[] flags = {0};
        final int[] result = {Result.PERMISSION_DENIED};
        try {
            IDisplayColor mDisplayColor = IDisplayColor.getService(true);
            if (mDisplayColor == null) {
                Log.e(TAG, "mDisplayColor is null");
                return -1;
            }

            mDisplayColor.getCB(hctx[0], displayId,
                               (int tmpReturn, int tmpWarmess, int tmpFlags) -> {
                                    result[0] = tmpReturn;
                                    if (result[0] != Result.OK) {
                                        return;
                                    }
                                    warmness[0] = tmpWarmess;
                                    flags[0] = tmpFlags;
                                });
            if (result[0] != Result.OK)
                Log.e(TAG, "Faild to call getCB result " + result[0]);
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException:" + e);
        }

        if (VERBOSE_ENABLED)
            Log.v(TAG, "getColorBalance: Exiting: result " + result[0] + " warmness " +
                        warmness[0] + " flags " + flags[0]);
        return warmness[0];
    }
    //////end color balance

    ///mode
    public int getNumModes(int displayId, int modeType){
        //Display mapping
        Integer hidlDisplayId = DCM_DISPLAY_MAP.get(new Integer(displayId));
        if (hidlDisplayId == null) {
            Log.e(TAG, "Hidl display type "+ displayId + " is not supported");
            return -1;
        }
        //end display mapping

        if (VERBOSE_ENABLED)
            Log.v(TAG, "getNumModes: Entering: displayId " + displayId +
                       " modeType " + modeType);

        final int[] mode_cnt = {-1};
        final int[] flags = {0};
        final int[] result = {Result.PERMISSION_DENIED};
        int type = dispToHidlModeType(MODE_TYPE.values()[modeType]);
        try {
            IDisplayColor mDisplayColor = IDisplayColor.getService(true);
            if (mDisplayColor == null) {
                Log.e(TAG, "mDisplayColor is null");
                return -1;
            }

            mDisplayColor.getNumModes(hctx[0], displayId, type,
                                (int tmpReturn, int tmpNum, int tmpFlags) -> {
                                    result[0] = tmpReturn;
                                    if (result[0] != Result.OK) {
                                        return;
                                    }
                                    mode_cnt[0] = tmpNum;
                                    flags[0] = tmpFlags;
                                });
            if (result[0] != Result.OK)
                Log.e(TAG, "Faild to call getNumModes result " + result[0]);
       } catch(RemoteException e) {
           Log.e(TAG, "RemoteException:" + e);
       }

        if (VERBOSE_ENABLED)
            Log.v(TAG, "getNumModes: Exiting: result " + result[0] + " mode_cnt " +
                        mode_cnt[0] + " flags " + flags[0]);
       return mode_cnt[0];
    }

    public long[] getActiveMode(int displayId) {
        //Display mapping
        Integer hidlDisplayId = DCM_DISPLAY_MAP.get(new Integer(displayId));
        if (hidlDisplayId == null) {
            Log.e(TAG, "Hidl display type "+ displayId + " is not supported");
            return null;
        }
        //end display mapping

        if (VERBOSE_ENABLED)
            Log.v(TAG, "getActiveMode: Entering: displayId " + displayId);

        final int[] mask = {0};
        final int[] result = {Result.PERMISSION_DENIED};
        final long[] retVal = {-1, 0}; // {avtive_mode, flags}
        try {
            IDisplayColor mDisplayColor = IDisplayColor.getService(true);
            if (mDisplayColor == null) {
                Log.e(TAG, "mDisplayColor is null");
                return null;
            }
            mDisplayColor.getActiveModeId(hctx[0], displayId,
                    (int tmpReturn, int tmpModeId, int tmpMask, int tmpFlags) -> {
                        result[0] = tmpReturn;
                        if (result[0] != Result.OK) {
                            return;
                        }
                        retVal[0] = tmpModeId;
                        retVal[1] = tmpFlags;
                        mask[0] = tmpMask;
                    });
            if (result[0] != Result.OK)
                Log.e(TAG, "Failed to call getActiveModeId result " + result[0]);
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException:" + e);
        }

        if (VERBOSE_ENABLED)
            Log.v(TAG, "getActiveMode: Exiting: result " + result[0] + " avtive_mode " +
                       retVal[0] + " flags " + retVal[1] + " mask " + mask[0]);
        return retVal;
    }

    public int setActiveMode(int displayId, int modeId) {
        //Display mapping
        Integer hidlDisplayId = DCM_DISPLAY_MAP.get(new Integer(displayId));
        if (hidlDisplayId == null) {
            Log.e(TAG, "Hidl display type "+ displayId + " is not supported");
            return -1;
        }
        //end display mapping

        if (VERBOSE_ENABLED)
            Log.v(TAG, "setActiveMode: Entering: displayId " + displayId + " modeId " + modeId);

        int flags = 0;
        int result = Result.PERMISSION_DENIED;
        try {
            IDisplayColor mDisplayColor = IDisplayColor.getService(true);
            if (mDisplayColor == null) {
                Log.e(TAG, "mDisplayColor is null");
                return -1;
            }

            result = mDisplayColor.setActiveModesId(hctx[0], displayId, modeId, flags);
            if (result != Result.OK)
                Log.e(TAG, "Failed to call setActiveModesId result " + result);
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException:" + e);
        }

        if (VERBOSE_ENABLED)
            Log.v(TAG, "setActiveMode: Exiting");
        return result;
    }

    public int deleteMode(int displayId, int modeId) {
        //Display mapping
        Integer hidlDisplayId = DCM_DISPLAY_MAP.get(new Integer(displayId));
        if (hidlDisplayId == null) {
            Log.e(TAG, "Hidl display type "+ displayId + " is not supported");
            return -1;
        }
        //end display mapping

        if (VERBOSE_ENABLED)
            Log.v(TAG, "deleteMode: Entering: modeId " + modeId);

        int flags = 0;
        int result = Result.PERMISSION_DENIED;
        try {
            IDisplayColor mDisplayColor = IDisplayColor.getService(true);
            if (mDisplayColor == null) {
                Log.e(TAG, "mDisplayColor is null");
                return -1;
            }

            result = mDisplayColor.deleteMode(hctx[0], displayId, modeId, flags);
            if (result != Result.OK)
                Log.e(TAG, "Faild to call deleteMode result "+ result);
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException:" + e);
        }

        if (VERBOSE_ENABLED)
            Log.v(TAG, "deleteMode: Exiting: result " + result);
        return result;
    }

    public ModeInfo[] getModes(int displayId, int modeType) {
        //Display mapping
        Integer hidlDisplayId = DCM_DISPLAY_MAP.get(new Integer(displayId));
        if(hidlDisplayId == null){
            Log.e(TAG, "Hidl display type "+ displayId + " is not supported");
            return null;
        }
        //end display mapping

        if (VERBOSE_ENABLED)
            Log.v(TAG, "getModes: Entering: modeType " + modeType);

        final int[] mode_cnt = {-1};
        final int[] flags = {0};
        final int[] result = {Result.PERMISSION_DENIED};
        int type = dispToHidlModeType(MODE_TYPE.values()[modeType]);
        try {
            IDisplayColor mDisplayColor = IDisplayColor.getService(true);
            if (mDisplayColor == null) {
                Log.e(TAG, "mDisplayColor is null");
                return null;
            }

            mDisplayColor.getNumModes(hctx[0], displayId, type,
                                (int tmpReturn, int tmpNum, int tmpFlags) -> {
                                    result[0] = tmpReturn;
                                    if (result[0] != Result.OK) {
                                        return;
                                    }
                                    mode_cnt[0] = tmpNum;
                                    flags[0] = tmpFlags;
                                });
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException:" + e);
        }

        if ((result[0] != Result.OK) || (mode_cnt[0] < 0)) {
            Log.e(TAG, "Failed to call getNumModes result: " + result[0] +
                        " mode_cnt:" + mode_cnt[0]);
            return null;
        }

        modeListHidl.clear();
        try {
            IDisplayColor mDisplayColor = IDisplayColor.getService(true);
            if (mDisplayColor == null) {
                Log.e(TAG, "mDisplayColor is null");
                return null;
            }

            mDisplayColor.getModes(hctx[0], displayId, type, mode_cnt[0],
                (int tmpReturn, ArrayList<vendor.display.color.V1_0.ModeInfo> tmpInfoList,
                 int tmpFlags) -> {
                    result[0] = tmpReturn;
                    if (result[0] != Result.OK) {
                        return;
                    }
                    modeListHidl = tmpInfoList;
                    flags[0] = tmpFlags;
                });
            if (result[0] != Result.OK) {
                Log.e(TAG, "Failed to call getModes result " + result[0]);
                return null;
            }
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException:" + e);
        }

        ModeInfo[] ModeListDisp = new ModeInfo[modeListHidl.size()];
        MODE_TYPE mode_type = MODE_TYPE.MODE_SYSTEM;
        for (int index = 0; index < modeListHidl.size(); ++index) {
            vendor.display.color.V1_0.ModeInfo modeHidl = modeListHidl.get(index);
            switch (modeHidl.type) {
                case DispModeType.OEM:
                    mode_type = MODE_TYPE.MODE_SYSTEM;
                    break;
                case DispModeType.USER:
                    mode_type = MODE_TYPE.MODE_USER;
                    break;
                case DispModeType.ALL:
                    mode_type = MODE_TYPE.MODE_ALL;
                    break;
                default:
                    Log.e(TAG, "Invalid hidl mode type "+ modeHidl.type);
                    return null;
            }
            ModeInfo modeDisp = new ModeInfo(modeHidl.id, modeHidl.name, mode_type.ordinal());
            ModeListDisp[index] = modeDisp;

            if (VERBOSE_ENABLED)
                Log.v(TAG, "getModes: index " + index + " id " + ModeListDisp[index].getId() +
                           " name " + ModeListDisp[index].getName() + " type " +
                           ModeListDisp[index].getModeType());
        }

        if (VERBOSE_ENABLED)
            Log.v(TAG, "getModes: Exiting");
        return ModeListDisp;
    }

    public int createNewMode(int displayId, String name, long flag, int cbWarmth){
        //not yet supported
        return -1;
    }

    public int modifyMode(int displayId, int modeId, String name, long flag, int cbWarmth){
        //not yet supported
        return -1;
    }

    public int getDefaultMode(int displayId){
        //Display mapping
        Integer hidlDisplayId = DCM_DISPLAY_MAP.get(new Integer(displayId));
        if (hidlDisplayId == null) {
            Log.e(TAG, "Hidl display type "+ displayId + " is not supported");
            return -1;
        }
        //end display mapping

        if (VERBOSE_ENABLED)
            Log.v(TAG, "getDefaultMode: Entering: displayId " + displayId);

        final int[] default_mode_id = {-1};
        final int[] flags = {0};
        final int[] result = {Result.PERMISSION_DENIED};
        try {
            IDisplayColor mDisplayColor = IDisplayColor.getService(true);
            if (mDisplayColor == null) {
                Log.e(TAG, "mDisplayColor is null");
                return -1;
            }

            mDisplayColor.getDefaultModeId(hctx[0], displayId,
                            (int tmpReturn, int tmpModeId, int tmpFlags) -> {
                                result[0] = tmpReturn;
                                if (result[0] != Result.OK) {
                                    return;
                                }
                                default_mode_id[0] = tmpModeId;
                                flags[0] = tmpFlags;
                            });
            if (result[0] != Result.OK)
                Log.e(TAG, "Faild to call getDefaultModeId result " + result[0]);
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException:" + e);
        }

        if (VERBOSE_ENABLED)
            Log.v(TAG, "getDefaultMode: Exiting: result " + result[0] + " default_mode_id " +
                        default_mode_id[0] + " flags " + flags[0]);
        return default_mode_id[0];
    }

    public int setDefaultMode(int displayId, int modeId) {
        //Display mapping
        Integer hidlDisplayId = DCM_DISPLAY_MAP.get(new Integer(displayId));
        if (hidlDisplayId == null) {
            Log.e(TAG, "Hidl display type "+ displayId + " is not supported");
            return -1;
        }
        //end display mapping
        if (VERBOSE_ENABLED)
            Log.v(TAG, "setDefaultMode: Entering: displayId " + displayId + " modeId " +
                        modeId);

        int flags = 0;
        int result = Result.PERMISSION_DENIED;
        try {
            IDisplayColor mDisplayColor = IDisplayColor.getService(true);
            if (mDisplayColor == null) {
                Log.e(TAG, "mDisplayColor is null");
                return -1;
            }

            result = mDisplayColor.setDefaultModesId(hctx[0], displayId, modeId, flags);
            if (result != Result.OK)
                Log.e(TAG, "Faild to call setDefaultModesId result " + result);
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException:" + e);
        }

        if (VERBOSE_ENABLED)
            Log.v(TAG, "setDefaultMode: Exiting: result" + result);
        return result;
    }

    public int createNewModeAllFeatures(int displayId, String name){
        //Display mapping
        Integer hidlDisplayId = DCM_DISPLAY_MAP.get(new Integer(displayId));
        if (hidlDisplayId == null) {
            Log.e(TAG, "Hidl display type "+ displayId + " is not supported");
            return -1;
        }
        //end display mapping

        if (name.length() == 0) {
            Log.e(TAG, "Invalid mode name");
            return -1;
        }

        if (VERBOSE_ENABLED)
            Log.v(TAG, "createNewModeAllFeatures: Entering: displayId " + displayId +
                       " name " + name);

        int flags = 0;
        final int[] mode_id = {-1};
        final int[] result = {Result.PERMISSION_DENIED};
        try {
            IDisplayColor mDisplayColor = IDisplayColor.getService(true);
            if (mDisplayColor == null) {
                Log.e(TAG, "mDisplayColor is null");
                return -1;
            }

            mDisplayColor.saveMode(hctx[0], displayId, name, mode_id[0], flags,
                            (int tmpReturn, int tmpModeId) -> {
                                result[0] = tmpReturn;
                                if (result[0] != Result.OK) {
                                    return;
                                }
                                mode_id[0] = tmpModeId;
                            });
            if (result[0] != Result.OK)
                Log.e(TAG, "Faild to call saveMode result " + result[0]);
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException:" + e);
        }

        if (VERBOSE_ENABLED)
            Log.v(TAG, "createNewModeAllFeatures: Exiting: result " + result[0] +
                       " mode_id " + mode_id[0]);
        return mode_id[0];
    }

    public int modifyModeAllFeatures(int displayId, int modeId, String name){
      //Display mapping
        Integer hidlDisplayId = DCM_DISPLAY_MAP.get(new Integer(displayId));
        if (hidlDisplayId == null) {
            Log.e(TAG, "Hidl display type "+ displayId + " is not supported");
            return -1;
        }
      //end display mapping

        if (name.length() == 0) {
            Log.e(TAG, "Invalid mode name");
            return -1;
        }

        if (VERBOSE_ENABLED)
            Log.v(TAG, "modifyModeAllFeatures: Entering displayId " + displayId +
                        " modeId " + modeId + " name "  + name);

        int flags = 0;
        final int[] mode_id = {modeId};
        final int[] result = {Result.PERMISSION_DENIED};
        name = name.replaceAll("\n", "");
        try {
            IDisplayColor mDisplayColor = IDisplayColor.getService(true);
            if (mDisplayColor == null) {
                Log.e(TAG, "mDisplayColor is null");
                return -1;
            }

            mDisplayColor.saveMode(hctx[0], displayId, name, mode_id[0], flags,
                            (int tmpReturn, int tmpModeId) -> {
                                result[0] = tmpReturn;
                                mode_id[0] = tmpModeId;
                            });
            if (result[0] != Result.OK)
                Log.e(TAG, "Faild to call saveMode result " + result[0]);
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException:" + e);
        }

        if (VERBOSE_ENABLED)
            Log.v(TAG, "modifyModeAllFeatures: result " + result[0] + " mode_id " +
                        mode_id[0]);

        return result[0];
    }
    ///end mode

    //svi
    public int getRangeSunlightVisibilityStrength(int displayId, int minMax){
        //not yet supported
        return -1;
    }

    public int setSunlightVisibilityStrength(int displayId, int strengthVal){
        //not yet supported
        return -1;
    }

    public int getSunlightVisibilityStrength(int displayId){
        //not yet supported
        return -1;
    }
    //end svi

    //backlight
    public int getBacklightQualityLevel(int displayId){
        //not yet supported
        return -1;
    }

    public int setBacklightQualityLevel(int displayId, int level){
        //not yet supported
        return -1;
    }

    public int getAdaptiveBacklightScale(int displayId){
        //not yet supported
        return -1;
    }
    //end backlight

    //active feature
    public int isActiveFeatureOn(int displayId, int feature){
        //not yet supported
        return -1;
    }

    public int setActiveFeatureControl(int displayId, int feature, int request){
        //not yet supported
        return -1;
    }
    //end active feature

    //mem color
    public int[] getRangeMemoryColorParameter(int displayId, int type){
        //not yet supported
        return null;
    }

    public int setMemoryColorParameters(int displayId, int type, int hue, int saturation, int intensity){
        //not yet supported
        return -1;
    }

    public int[] getMemoryColorParameters(int displayId, int type){
        //not yet supported
        return null;
    }

    public int disableMemoryColorConfiguration(int displayId, int type){
        //not yet supported
        return -1;
    }
    //end mem color


    //pa
    public int[] getRangePAParameter(int displayId){
        //Display mapping
        Integer hidlDisplayId = DCM_DISPLAY_MAP.get(Integer.valueOf(displayId));
        if (hidlDisplayId == null) {
            Log.e(TAG, "Hidl display type "+ displayId + " is not supported");
            return null;
        }
        //end display mapping

        if(VERBOSE_ENABLED)
            Log.v(TAG, "getRangePAParameter: Entering: displayId " + displayId);

        int pa_arr[] = new int[10];
        final int[] result = {Result.PERMISSION_DENIED};
        PARange[] pa_range = new PARange[1];
        try {
            IDisplayColor mDisplayColor = IDisplayColor.getService(true);
            if (mDisplayColor == null) {
                Log.e(TAG, "mDisplayColor is null");
                return null;
            }

            mDisplayColor.getGlobalPARange(hctx[0], displayId,
                            (int tmpReturn,  PARange tmpRange) -> {
                                result[0] = tmpReturn;
                                pa_range[0] = tmpRange;
                            });
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException:" + e);
        }

        if (result[0] != Result.OK) {
            Log.e(TAG, "Faild to call getRangePAParameter result " + result[0]);
            return null;
        }

        int i = 0;
        pa_arr[i++] = 0;
        pa_arr[i++] = 360;
        pa_arr[i++] = 0;
        pa_arr[i++] = 150;
        pa_arr[i++] = 0;
        pa_arr[i++] = 200;
        pa_arr[i++] = 0;
        pa_arr[i++] = 200;
        pa_arr[i++] = 0;
        pa_arr[i++] = 1;

        hueScaler = 180/pa_range[0].hue.max;
        satScaler = 100/pa_range[0].saturation.max;
        valScaler = pa_range[0].value.max/100;
        contScaler = 100/pa_range[0].contrast.max;

        if (VERBOSE_ENABLED)
            Log.v(TAG, "getRangePAParameter: Exiting");
        return pa_arr;
    }

    public int setPAParameters(int displayId, int flag, int hue, int saturation, int intensity, int contrast, int satThreshold) {
        // Display mapping
        Integer hidlDisplayId = DCM_DISPLAY_MAP.get(new Integer(displayId));
        if (hidlDisplayId == null) {
            Log.e(TAG, "Hidl display type " + displayId + " is not supported");
            return -1;
        }
        // end display mapping
        if (VERBOSE_ENABLED)
            Log.v(TAG, "setPAParameters: Entering: flag " + flag + " hue " + hue +
                       " saturation " + saturation + " intensity " + intensity +
                       " contrast " + contrast + " satThreshold " + satThreshold);

        int result = Result.PERMISSION_DENIED;
        int enable = 1;
        PAConfig pa_config = new PAConfig();

        pa_config.valid = true;
        pa_config.flags = flag;
        pa_config.data.hue = (int)((hue + desiredRange[0])/hueScaler);
        pa_config.data.saturation = (saturation + desiredRange[2])/satScaler; //100.0
        pa_config.data.value = (intensity + desiredRange[4]) *valScaler; //2.55
        pa_config.data.contrast = (contrast + desiredRange[6])/contScaler; //100.0
        pa_config.data.sat_thresh = (float)satThreshold;
        try {
            IDisplayColor mDisplayColor = IDisplayColor.getService(true);
            if (mDisplayColor == null) {
                Log.e(TAG, "mDisplayColor is null");
                return -1;
            }

            result = mDisplayColor.setGlobalPA(hctx[0], displayId, enable, pa_config);
            if (result != Result.OK)
                Log.e(TAG, "Faild to call setGlobalPA result " + result);
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException:" + e);
        }

        if (VERBOSE_ENABLED)
            Log.v(TAG, "setPAParameters: Exiting");
        return result;
    }

    public int[] getPAParameters(int displayId){
        //Display mapping
        Integer hidlDisplayId = DCM_DISPLAY_MAP.get(Integer.valueOf(displayId));
        if (hidlDisplayId == null) {
            Log.e(TAG, "Hidl display type "+ displayId + " is not supported");
            return null;
        }
        //end display mapping

        if (VERBOSE_ENABLED)
            Log.v(TAG, "getPAParameters: Entering displayId " + displayId);

        final int[] result = {Result.PERMISSION_DENIED};
        final PAConfig[] pa_config = new PAConfig[1];
        final int[] enable = {0};
        try {
            IDisplayColor mDisplayColor = IDisplayColor.getService(true);
            if (mDisplayColor == null) {
                Log.e(TAG, "mDisplayColor is null");
                return null;
            }
            mDisplayColor.getGlobalPA(hctx[0], displayId,
                            (int tmpReturn, int tmpEnable, PAConfig tmpCfg) -> {
                                result[0] = tmpReturn;
                                enable[0] = tmpEnable;
                                pa_config[0] = tmpCfg;
                            });
        } catch(RemoteException e) {
            Log.e(TAG, "RemoteException:" + e);
        }

        if (result[0] != Result.OK) {
            Log.e(TAG, "Faild to getPAParameters result " + result[0]);
                return null;
        }

        int i = 0;
        int[] pa_arr = new int[6];
        pa_arr[i++] = pa_config[0].flags;
        pa_arr[i++] = (int)(pa_config[0].data.hue*hueScaler - desiredRange[0]);
        pa_arr[i++] = (int)(pa_config[0].data.saturation*satScaler - desiredRange[2]);
        pa_arr[i++] = (int)(pa_config[0].data.value/valScaler - (float)desiredRange[4]);
        pa_arr[i++] = (int)(pa_config[0].data.contrast*contScaler - (float)desiredRange[6]);
        pa_arr[i++] = (int)(pa_config[0].data.sat_thresh);

        if (VERBOSE_ENABLED)
            Log.v(TAG, "getPAParameters: Exiting: flags " + pa_arr[0] + " hue " + pa_arr[1] +
                       " saturation " + pa_arr[2] + " value " + pa_arr[3] + " contrast " +
                       pa_arr[4] + " sat_thresh " + pa_arr[5] + " enable " + enable[0]);
        return pa_arr;
    }
    //end pa

    /**
     * This method will call the hidl deInit.
     */
    public void release() {
        if (VERBOSE_ENABLED)
            Log.v(TAG, "release: Entering");
        int result = Result.PERMISSION_DENIED;
        int flags = 0;
        try {
             IDisplayColor mDisplayColor = IDisplayColor.getService(true);
             if (mDisplayColor == null) {
                 Log.e(TAG, "mDisplayColor is null");
                 return;
             }
             result = mDisplayColor.deInit(hctx[0], flags);
             if (result != Result.OK) {
                Log.e(TAG, "Faild to call deinit result " + result);
                return;
            }
         } catch (RemoteException e) {
             Log.e(TAG, "RemoteException:" + e);
         }

         if (VERBOSE_ENABLED)
             Log.v(TAG, "release: Exiting");
    }

    private int dispToHidlModeType(MODE_TYPE mode_type) {
        int type = DispModeType.ALL;

        switch (mode_type) {
            case MODE_SYSTEM:
                type = DispModeType.OEM;
                break;
            case MODE_USER:
                type = DispModeType.USER;
                break;
            case MODE_ALL:
                type = DispModeType.ALL;
                break;
            default:
                Log.e(TAG, "Invalid mode type "+ mode_type);
                break;
        }
        return type;
    }
}
