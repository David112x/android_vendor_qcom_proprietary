/*============================================================================
  Copyright (c) 2018 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

  @file deviceModeTab.java
  @brief
   deviceModeTab class implementation for displaying the sensors attributes
============================================================================*/

package com.qualcomm.qti.usta.ui;
import android.content.Context;
import android.app.Fragment;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import com.qualcomm.qti.usta.R;
import com.qualcomm.qti.usta.core.SensorContextJNI;
import com.qualcomm.qti.usta.core.USTALog;

public class deviceModeTab extends Fragment {
    private View deviceModeInfoTabView;
    private String currentDeviceMode;
    private String currentModeState;
    private Button deviceModeButton;
    private EditText deviceModeNumberEditText;
    private EditText deviceModeStateEditText;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        super.onCreateView(inflater, container, savedInstanceState);
        deviceModeInfoTabView =  inflater.inflate(R.layout.device_mode_tab, container, false);
        setUpSendIndicationButton();
        setUpDeviceModeNumber();
        setUpDeviceModeAccuracy();
        return deviceModeInfoTabView;
    }

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
  }

    private void setUpSendIndicationButton() {

        deviceModeButton = (Button) deviceModeInfoTabView.findViewById(R.id.device_mode_send_button);

        deviceModeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if((currentDeviceMode != null && currentDeviceMode.length() != 0) && (currentModeState != null && currentModeState.length() != 0))
                    if(0 == SensorContextJNI.sendDeviceModeIndicationWrapper(Integer.parseInt(currentDeviceMode) , Integer.parseInt(currentModeState))) {
                        USTALog.i("Device mode notification was sent successfully ");
                        Toast.makeText(getContext(), "Request successfully sent to sensors core", Toast.LENGTH_LONG).show();
                    }
                    else {
                        USTALog.e("There is an error while sending your request to sensors core");
                        Toast.makeText(getContext(), "There is an error while sending your request to sensors core", Toast.LENGTH_LONG).show();
                    }
                else {
                    USTALog.e("Request not passed to native as there is no inputs from user");
                    Toast.makeText(getContext(), "Request is not processed. Please do enter both Mode and State", Toast.LENGTH_LONG).show();
                }
            }
        });
    }

    private void setUpDeviceModeNumber(){

        deviceModeNumberEditText = (EditText) deviceModeInfoTabView.findViewById(R.id.device_mode_number_handle);
        deviceModeNumberEditText.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {

                if (v.getId() == R.id.device_mode_number_handle && !hasFocus) {
                    InputMethodManager imm = (InputMethodManager) getActivity().getApplicationContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                    imm.hideSoftInputFromWindow(v.getWindowToken(), 0);
                }
            }
        });

        TextWatcher deviceModeNumberTextWatcher = new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {

            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {

                if (s.length() != 0) {
                    currentDeviceMode = s.toString();
                } else
                    currentDeviceMode = null;
            }

            @Override
            public void afterTextChanged(Editable s) {

            }
        };

        deviceModeNumberEditText.addTextChangedListener(deviceModeNumberTextWatcher);
    }

    private void setUpDeviceModeAccuracy(){

        deviceModeStateEditText = (EditText) deviceModeInfoTabView.findViewById(R.id.device_mode_state_handle);
        deviceModeStateEditText.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {

                if (v.getId() == R.id.device_mode_state_handle && !hasFocus) {
                    InputMethodManager imm = (InputMethodManager) getActivity().getApplicationContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                    imm.hideSoftInputFromWindow(v.getWindowToken(), 0);
                }
            }
        });

        TextWatcher deviceModeAccuracyTextWatcher = new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {

            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {

                if (s.length() != 0) {
                    currentModeState = s.toString();
                } else
                    currentModeState = null;
            }

            @Override
            public void afterTextChanged(Editable s) {

            }
        };

        deviceModeStateEditText.addTextChangedListener(deviceModeAccuracyTextWatcher);
    }

}
