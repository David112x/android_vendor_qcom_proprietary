/*============================================================================
  Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

  @file SensorLowLatency.java
  @brief
  Sensor Low Latency Tab view creation
============================================================================*/
package com.qualcomm.qti.usta.ui;

import android.content.Context;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Environment;
import android.app.Fragment;
import android.app.FragmentManager;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.CheckBox;
import android.hardware.SensorDirectChannel;

import com.qualcomm.qti.usta.R;
import com.qualcomm.qti.usta.core.ModeType;
import com.qualcomm.qti.usta.core.SensorContext;
import com.qualcomm.qti.usta.core.SensorLowLatencyJNI;
import com.qualcomm.qti.usta.core.USTALog;
import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.math.BigInteger;
import java.util.StringTokenizer;
import java.util.Vector;

public class SensorLowLatency extends Fragment {
    private View sensorLowLatView;
    private TextView suidLowTextView;
    private TextView suidHighTextView;
    private TextView flagTextView;
    private Spinner sensorLowLatSpinnerList;
    private EditText periodEditText;
    private CheckBox channelTypeCB;
    private Vector<String> lowLatSensorsLookUpNames;
    private Vector<String> lowLatSensorsNames;
    private Vector<String> lowLatLowSuids;
    private Vector<String> lowLatHighSuids;
    private Vector<Integer> lowLatFlags;
    private Vector<Boolean> lowLatEnabled;
    private Vector<Integer> lowLatStreamHandle;
    private Vector<Integer> lowLatSensorHandle;
    private Vector<Integer> lowLatChannelType;
    private Button enableButton;
    private String samplePeriodUs;
    private static int currentSensorSpinnerPosition;
    private static int SRSpinnerPosition;
    long[] sensorSuid;
    private SensorContext sensorContext;
    private Spinner samplingRateSpinnerList;
    private Vector<String> samplingRateList;
    private static final int SNS_LOW_LAT_CHANNEL_TYPE_NOT_INIT = -1;
    private static final int SNS_LOW_LAT_CHANNEL_TYPE_ANDROID = 0x0;
    private static final int SNS_LOW_LAT_CHANNEL_TYPE_GENERIC = 0x1;
    private static final int SNS_LOW_LAT_FLAG_ANDROID_STYLE_CHANNEL = 0x2;
    private static final int SNS_LOW_LAT_CAL_SENSOR_HANDLE_START = 0x100;

    private void setChannelTypeCheckBox () {
        channelTypeCB = (CheckBox) sensorLowLatView.findViewById(R.id.channel_type_checkbox);
        channelTypeCB.setOnClickListener(new View.OnClickListener()
        {
            public void onClick(View v) {
            int lowLatFlag;
            if (((CheckBox)v).isChecked()) {
                lowLatChannelType.set(currentSensorSpinnerPosition, SNS_LOW_LAT_CHANNEL_TYPE_GENERIC);
                lowLatFlag = lowLatFlags.elementAt(currentSensorSpinnerPosition).intValue();
                lowLatFlag = lowLatFlag & (~SNS_LOW_LAT_FLAG_ANDROID_STYLE_CHANNEL);
                lowLatFlags.set(currentSensorSpinnerPosition, lowLatFlag);
                flagTextView.setText(lowLatFlags.elementAt(currentSensorSpinnerPosition).toString());
            } else {
                lowLatChannelType.set(currentSensorSpinnerPosition,  SNS_LOW_LAT_CHANNEL_TYPE_ANDROID);
                lowLatFlag = lowLatFlags.elementAt(currentSensorSpinnerPosition).intValue();
                lowLatFlag = lowLatFlag | (SNS_LOW_LAT_FLAG_ANDROID_STYLE_CHANNEL);
                lowLatFlags.set(currentSensorSpinnerPosition, lowLatFlag);
                flagTextView.setText(lowLatFlags.elementAt(currentSensorSpinnerPosition).toString());
            }
            }
        });
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        lowLatSensorsLookUpNames = new Vector<String>();
        lowLatSensorsNames = new Vector<String>();
        lowLatLowSuids = new Vector<String>();
        lowLatHighSuids = new Vector<String>();
        lowLatStreamHandle = new Vector<Integer>();
        lowLatSensorHandle = new Vector<Integer>();
        lowLatFlags = new Vector<Integer>();
        lowLatChannelType = new Vector<Integer>();
        lowLatEnabled = new Vector<Boolean>();
        sensorSuid = new long[2];
        samplingRateList = new Vector<String>();
        sensorLowLatView = inflater.inflate(R.layout.low_lat_tab, container, false);
        sensorContext = SensorContext.getSensorContext(ModeType.USTA_MODE_TYPE_UI);
        suidLowTextView = (TextView) sensorLowLatView.findViewById(R.id.low_lat_sensor_suid_low);
        suidHighTextView = (TextView) sensorLowLatView.findViewById(R.id.low_lat_sensor_suid_high);
        flagTextView = (TextView) sensorLowLatView.findViewById(R.id.low_lat_flag);
        updateLowLatSensorsInfo();
        setLowLatSensorsListSpinner();
        setEnableButton();
        setChannelTypeCheckBox();
        return sensorLowLatView;
    }

    @Override
    public void onCreate(Bundle savedInstanceData) {
        super.onCreate(savedInstanceData);
    }

    public void updateLowLatSensorsInfo(){
        updateLowLatSensorsNames();
        updateLowLatSensorFlags();
    }

    public void updateLowLatSensorFlags(){
        for(int sensorIndex = 0; sensorIndex < lowLatSensorsLookUpNames.size() ; sensorIndex++) {
            String lowLatSensorName = lowLatSensorsLookUpNames.elementAt(sensorIndex);
            boolean isCalSensor = false;
            boolean is2ndSensor = false;
            if(lowLatSensorName.contains("2nd")) {
                is2ndSensor = true;
            }
            if(lowLatSensorName.contains("Cal")) {
                isCalSensor = true;
            }
            if(isCalSensor || is2ndSensor) {
                String[] sensorNameSplit = lowLatSensorName.split("-");
                lowLatSensorName = sensorNameSplit[0];
            }

            USTALog.i("lowLatSensorsNames=" + lowLatSensorsLookUpNames.elementAt(sensorIndex) + " short name=" + lowLatSensorName + " is2ndSensor=" + is2ndSensor + " isCalSensor=" + isCalSensor);
            int currentSensorHandle = 0;
            boolean foundMatchSensor = false;
            boolean found1stSensor = false;
            for(int sensorHandle = 0 ; sensorHandle < sensorContext.getSensors().size() ; sensorHandle ++){
                String sensorName = sensorContext.getSensors().get(sensorHandle).getSensorName();
                String[] dataType = sensorName.split("-");
                if(dataType[0].equals(lowLatSensorName.toLowerCase())){
                    if(!found1stSensor && !is2ndSensor) {
                        found1stSensor = true;
                        foundMatchSensor = true;
                        currentSensorHandle = sensorHandle;
                        break;
                    }else if(found1stSensor && is2ndSensor){
                        foundMatchSensor = true;
                        currentSensorHandle = sensorHandle;
                        break;
                    } else {
                        found1stSensor = true;
                    }
                }
            }
            USTALog.i("foundMatchSensor=" + foundMatchSensor + " found1stSensor=" + found1stSensor + " currentSensorHandle=" + currentSensorHandle + "lowLatSensorsNames=" + lowLatSensorsLookUpNames.elementAt(sensorIndex));
            if(foundMatchSensor) {
                lowLatSensorsNames.add(lowLatSensorsLookUpNames.elementAt(sensorIndex));
                lowLatLowSuids.add(sensorContext.getSensors().elementAt(currentSensorHandle).getSensorSUIDLow());
                lowLatHighSuids.add(sensorContext.getSensors().elementAt(currentSensorHandle).getSensorSUIDHigh());

                if(isCalSensor == true ){
                    lowLatFlags.add(6);
                }else{
                    lowLatFlags.add(2);
                }
                lowLatEnabled.add(false);
                lowLatStreamHandle.add(0);
                if (isCalSensor == true ) {
                    lowLatSensorHandle.add(SNS_LOW_LAT_CAL_SENSOR_HANDLE_START + currentSensorHandle);
                } else {
                    lowLatSensorHandle.add(currentSensorHandle);
                }
                lowLatChannelType.add(SNS_LOW_LAT_CHANNEL_TYPE_ANDROID);
            }
        }
    }

    public void updateLowLatSensorsNames(){
        lowLatSensorsLookUpNames.add("Accel");
        lowLatSensorsLookUpNames.add("Gyro");
        lowLatSensorsLookUpNames.add("Mag");
        lowLatSensorsLookUpNames.add("Gyro-Cal");
        lowLatSensorsLookUpNames.add("Mag-Cal");
        lowLatSensorsLookUpNames.add("Accel-2nd");
        lowLatSensorsLookUpNames.add("Gyro-2nd");
        lowLatSensorsLookUpNames.add("Mag-2nd");
        lowLatSensorsLookUpNames.add("Gyro-Cal-2nd");
        lowLatSensorsLookUpNames.add("Mag-Cal-2nd");
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        for (int enableCount = 0; enableCount < lowLatStreamHandle.size(); enableCount++) {
            if (lowLatEnabled.elementAt(enableCount) == true) {
                /*Since it is in destroy, We should not handle any error cases. As entire clean up has to be done at bottom*/
                SensorLowLatencyJNI.lowLatDisable(lowLatStreamHandle.elementAt(enableCount));
                USTALog.i("Disabled stream with handle 0x" + Long.toHexString(lowLatStreamHandle.elementAt(enableCount)));
            }
        }
        currentSensorSpinnerPosition = 0;
        samplePeriodUs = null;
        lowLatSensorsLookUpNames.clear();
        lowLatSensorsNames.clear();
        lowLatLowSuids.clear();
        lowLatHighSuids.clear();
        lowLatStreamHandle.clear();
        lowLatStreamHandle.clear();
        lowLatFlags.clear();
        lowLatChannelType.clear();
        lowLatEnabled.clear();
        samplingRateList.clear();
    }

    private void setLowLatSensorsListSpinner() {
        currentSensorSpinnerPosition = 0;
        sensorLowLatSpinnerList = (Spinner) sensorLowLatView.findViewById(R.id.low_lat_sensor_list_spinner);
        ArrayAdapter<String> sensorLowLatSpinnerListAdapter = new ArrayAdapter<>(getActivity().getApplicationContext(), android.R.layout.simple_list_item_1, lowLatSensorsNames);
        sensorLowLatSpinnerList.setAdapter(sensorLowLatSpinnerListAdapter);
        sensorLowLatSpinnerList.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                currentSensorSpinnerPosition = sensorLowLatSpinnerList.getSelectedItemPosition();
                updateLowLatSuids();
                updateSamplingRateList();
                updateChannelTypeBox();
                setSamplingRateSpinner();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
        sensorLowLatSpinnerList.setSelection(currentSensorSpinnerPosition);
    }

    private void updateSamplingRateList() {
        samplingRateList.clear();
        long[] sensorSuid;
        sensorSuid = new long[]{0,0};

        sensorSuid[0] = new BigInteger(lowLatLowSuids.elementAt(currentSensorSpinnerPosition),16).longValue();
        sensorSuid[1] = new BigInteger(lowLatHighSuids.elementAt(currentSensorSpinnerPosition),16).longValue();

        int max_rate = SensorLowLatencyJNI.lowLatGetMaxRate(sensorSuid);

        switch (max_rate) {
            case SensorDirectChannel.RATE_STOP:
                samplingRateList.add("");
                break;
            case SensorDirectChannel.RATE_NORMAL:
                samplingRateList.add("RATE_NORMAL");
                break;
            case SensorDirectChannel.RATE_FAST:
                samplingRateList.add("RATE_NORMAL");
                samplingRateList.add("RATE_FAST");
                break;
            case SensorDirectChannel.RATE_VERY_FAST:
                samplingRateList.add("RATE_NORMAL");
                samplingRateList.add("RATE_FAST");
                samplingRateList.add("RATE_VERY_FAST");
                break;
        }
    }

    private void updateChannelTypeBox(){
        channelTypeCB = (CheckBox) sensorLowLatView.findViewById(R.id.channel_type_checkbox);
        if (SNS_LOW_LAT_CHANNEL_TYPE_GENERIC == lowLatChannelType.elementAt(currentSensorSpinnerPosition)) {
            channelTypeCB.setChecked(true);
        } else {
            channelTypeCB.setChecked(false);
        }
    }

    private void setSamplingRateSpinner() {
        SRSpinnerPosition = 0;
        samplingRateSpinnerList = (Spinner) sensorLowLatView.findViewById(R.id.low_lat_period_spinner);

        ArrayAdapter<String> sensorLowLatSpinnerListAdapter = new ArrayAdapter<>(getActivity().getApplicationContext(), android.R.layout.simple_list_item_1, samplingRateList);
        samplingRateSpinnerList.setAdapter(sensorLowLatSpinnerListAdapter);
        samplingRateSpinnerList.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                SRSpinnerPosition = samplingRateSpinnerList.getSelectedItemPosition();
                samplePeriodUs = String.valueOf(SensorLowLatencyJNI.lowLatChannelSampleRate(SRSpinnerPosition + 1));
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
        samplingRateSpinnerList.setSelection(SRSpinnerPosition);
    }

    private void updateLowLatSuids() {
        suidLowTextView.setText("0x" + lowLatLowSuids.elementAt(currentSensorSpinnerPosition).toString());
        suidHighTextView.setText("0x" + lowLatHighSuids.elementAt(currentSensorSpinnerPosition).toString());
        flagTextView.setText(lowLatFlags.elementAt(currentSensorSpinnerPosition).toString());
        if (lowLatEnabled.elementAt(currentSensorSpinnerPosition).booleanValue() == true) {
            enableButton.setText("Disable");
        } else {
            enableButton.setText("Enable");
        }
    }

    private void setEnableButton() {
        enableButton = (Button) sensorLowLatView.findViewById(R.id.low_lat_button);
        enableButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                int streamHandle;
                final ScrollView logScrollView = (ScrollView) sensorLowLatView.findViewById(R.id.scroll_view_log);
                TextView logTextView = (TextView) logScrollView.findViewById(R.id.text_view_log);
                logTextView.append("");
                if (lowLatEnabled.elementAt(currentSensorSpinnerPosition).booleanValue() == true) {
                    lowLatEnabled.set(currentSensorSpinnerPosition, false);
                    enableButton.setText("Enable");
                    SensorLowLatencyJNI.lowLatDisable(lowLatStreamHandle.elementAt(currentSensorSpinnerPosition));
                    logTextView.append("Sensor:" + lowLatSensorsNames.elementAt(currentSensorSpinnerPosition) + " with stream handle:0x" + Integer.toHexString(lowLatStreamHandle.elementAt(currentSensorSpinnerPosition)) + " disabled.\n");
                    USTALog.i("DISABLED:Sensor:" + lowLatSensorsNames.elementAt(currentSensorSpinnerPosition) + " handle: 0x" + Integer.toHexString(lowLatStreamHandle.elementAt(currentSensorSpinnerPosition)) + " suid_low:0x" + lowLatLowSuids.elementAt(currentSensorSpinnerPosition) + " suid_high:0x" + lowLatLowSuids.elementAt(currentSensorSpinnerPosition) + " Flag:" + lowLatFlags.elementAt(currentSensorSpinnerPosition) + " channelType:" + lowLatChannelType.elementAt(currentSensorSpinnerPosition)+ " SensorHandle:" + lowLatSensorHandle.elementAt(currentSensorSpinnerPosition) +"\n");
                    lowLatStreamHandle.set(currentSensorSpinnerPosition, 0);
                } else {
                    sensorSuid[0] = new BigInteger(lowLatLowSuids.elementAt(currentSensorSpinnerPosition), 16).longValue();
                    sensorSuid[1] = new BigInteger(lowLatHighSuids.elementAt(currentSensorSpinnerPosition), 16).longValue();

                    streamHandle = SensorLowLatencyJNI.lowLatEnable(sensorSuid, Integer.decode(samplePeriodUs), lowLatFlags.elementAt(currentSensorSpinnerPosition), lowLatChannelType.elementAt(currentSensorSpinnerPosition), lowLatSensorHandle.elementAt(currentSensorSpinnerPosition));
                    if (streamHandle == -1) {
                        logTextView.setTextColor(Color.RED);
                        logTextView.append("Sensor:" + lowLatSensorsNames.elementAt(currentSensorSpinnerPosition) + " with stream handle:0x" + Integer.toHexString(streamHandle) + " NOT enabled properly.\n");
                        return;
                    }
                    lowLatEnabled.set(currentSensorSpinnerPosition, true);
                    enableButton.setText("Disable");
                    lowLatStreamHandle.set(currentSensorSpinnerPosition, streamHandle);
                    logTextView.append("Sensor:" + lowLatSensorsNames.elementAt(currentSensorSpinnerPosition) + " with stream handle:0x" + Integer.toHexString(streamHandle) + " enabled.\n");
                    USTALog.i("ENABLED:Sensor:" + lowLatSensorsNames.elementAt(currentSensorSpinnerPosition) + " handle: 0x" + Integer.toHexString(streamHandle) + " suid_low:0x" + Long.toHexString(sensorSuid[0]) + " suid_high:0x" + Long.toHexString(sensorSuid[1]) + " Flag:" + lowLatFlags.elementAt(currentSensorSpinnerPosition)+ " channelType:" + lowLatChannelType.elementAt(currentSensorSpinnerPosition) + " SensorHandle:" + lowLatSensorHandle.elementAt(currentSensorSpinnerPosition) + " samplePeriodUs:" + Integer.decode(samplePeriodUs) + "\n");
                }
                logScrollView.postDelayed(new Runnable() {
                    public void run() {
                        logScrollView.fullScroll(ScrollView.FOCUS_DOWN);
                    }
                }, 0);
            }
        });
    }


}
