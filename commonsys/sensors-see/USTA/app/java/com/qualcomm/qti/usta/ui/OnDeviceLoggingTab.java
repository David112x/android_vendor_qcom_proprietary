/*============================================================================
  Copyright (c) 2019 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

  @file OnDeviceLoggingTab.java
  @brief
   OnDeviceLoggingTab class implementation
============================================================================*/
package com.qualcomm.qti.usta.ui;

import android.app.Fragment;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Bundle;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import com.qualcomm.qti.usta.R;

import java.io.FileOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.FileReader;

import android.os.CountDownTimer;

public class OnDeviceLoggingTab extends Fragment {

  private Button startButton;
  private Button stopButton;
  private TextView odlStatus;
  private TextView timerStatus;
  private View odlTabView;
  private final String mnt_per_file = "/mnt/vendor/persist/sensors/file1";

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    super.onCreateView(inflater, container, savedInstanceState);

    if (null == odlTabView) {
      odlTabView = inflater.inflate(R.layout.on_device_logging, container, false);
      odlStatus = (TextView) odlTabView.findViewById(R.id.odl_status);
      timerStatus = (TextView) odlTabView.findViewById(R.id.timer_status);
      setUpStartButton();
      setUpStopButton();
    }
    try{
        String curLine;
        FileReader fr = new FileReader(mnt_per_file);
        BufferedReader br = new BufferedReader(fr);
        curLine=br.readLine();
        if(null != curLine){
            if(curLine.equalsIgnoreCase("start")){
                stopButton.setEnabled(true);
                odlStatus.setText("On Device Logging Already Started.. \nPlease Stop to Start Again");
                odlStatus.setTextColor(Color.RED);
            }
            if(curLine.equalsIgnoreCase("stop")){
                startButton.setEnabled(true);
            }
        }
        else{
            startButton.setEnabled(true);
        }
    }catch(IOException ioe){
        ioe.printStackTrace();
    }
    return odlTabView;
  }

  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
  }

  private void setUpStartButton() {

    startButton = (Button) odlTabView.findViewById(R.id.start_button);
    startButton.setEnabled(false);
    odlStatus.setText("On Device Logging !");
    timerStatus.setText("Timer Status");
    startButton.setOnClickListener(new View.OnClickListener() {

      @Override
      public void onClick(View v) {
        odlStatus.setText(" On Device Logging : Starting... \n Min 3 sec delay required!");
        odlStatus.setSingleLine(false);
        timerStatus.setTextColor(Color.RED);
        FileOutputStream outputStream;
        DataOutputStream dataOutStream;
        try {
          String fileContent = "star\n";
          outputStream = new FileOutputStream(mnt_per_file);
          dataOutStream = new DataOutputStream(new BufferedOutputStream(outputStream));
          dataOutStream.writeBytes(fileContent);
          dataOutStream.close();
          startButton.setEnabled(false);
          new CountDownTimer(3000, 1000) {
            public void onTick(long millisUntilFinished) {
              timerStatus.setText("Seconds remaining : " + millisUntilFinished / 1000);
            }
            public void onFinish() {
              timerStatus.setText(" On Device Logging - Started ! ");
              stopButton.setEnabled(true);
              odlStatus.setText("");
            }
          }.start();

        } catch (IOException ioe) {
          ioe.printStackTrace();
        }
      }
    });
  }

  private void setUpStopButton() {

    stopButton = (Button) odlTabView.findViewById(R.id.stop_button);
    stopButton.setEnabled(false);
    stopButton.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        odlStatus.setText(" On Device Logging : Stopping... \n Min 5 sec delay required! ");
        timerStatus.setTextColor(Color.RED);
        FileOutputStream outputStream;
        DataOutputStream dataOutStream;
        try {
          String fileContent = "stop\n";
          outputStream = new FileOutputStream(mnt_per_file);
          dataOutStream = new DataOutputStream(new BufferedOutputStream(outputStream));
          dataOutStream.writeBytes(fileContent);
          dataOutStream.close();
          stopButton.setEnabled(false);
          new CountDownTimer(5000, 1000) {
            public void onTick(long millisUntilFinished) {
              timerStatus.setText("Seconds remaining : " + millisUntilFinished / 1000);
            }
            public void onFinish() {
              timerStatus.setText("On Device Logging - Stopped! \n Logs Path : /data/vendor/sensors/");
              odlStatus.setText("");
              startButton.setEnabled(true);
            }
          }.start();
        } catch (IOException ioe) {
          ioe.printStackTrace();
        }
      }
    });
  }
}
