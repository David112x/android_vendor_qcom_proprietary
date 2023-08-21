/*============================================================================
  Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

  @file SensorEventDisplayFragment.java
  @brief
   SensorEventDisplayFragment class implementation
============================================================================*/

package com.qualcomm.qti.usta.ui;

import android.app.Fragment;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.PowerManager;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import org.json.JSONObject;
import org.json.JSONException;
import org.json.JSONArray;
import com.qualcomm.qti.usta.R;
import com.qualcomm.qti.usta.core.Sensor;
import com.qualcomm.qti.usta.core.SensorContextJNI;
import com.qualcomm.qti.usta.core.USTALog;
import java.util.HashMap;
import java.util.Map;
import java.util.Arrays;
import android.widget.Button;
public class SensorEventDisplayFragment extends Fragment implements Runnable {


  private View SensorEventDisplayFrgmentView;
  private TextView eventMessageTextView;
  public static boolean isDisplaySamplesEnabled = true;
  public static boolean isWakeLockEnabled = false;
  boolean isThreadRunning = true;
  private static Thread samplesThread;
  private TextView samplesCountedField;
  private TextView timeElapsedField;
  private TextView expectedSamplesField;
  private TextView messageIDField;
  private TextView TimeStampField;
  private TextView dataField;
  private TextView statusField;
  private Button jsonToggle;
  private boolean jsonOn;
  private TextView sampRateField;
  private TextView waterMarkField;
  private TextView floatResolution;
  private TextView floatRange;
  private PowerManager.WakeLock wakeLockObj;
  private static boolean isWakeUpRequest = false;
  private static int currentSensorhandle;
  private static Map wakeupDatabase = new HashMap();

  public static void updateCurrentSensorHandle(int sensorHandle) {
    currentSensorhandle = sensorHandle ;
    Object keyValueObj = wakeupDatabase.get(sensorHandle);
    if(keyValueObj != null ) {
      if (Boolean.TRUE.equals(keyValueObj)) {
        isWakeUpRequest = true;
      } else {
        isWakeUpRequest = false;
      }
    }else {
      isWakeUpRequest = false;
    }
  }

  public static void updateCurrentWakeUpInfo(int sensorHandle, boolean isWakeup) {
    wakeupDatabase.put(sensorHandle, isWakeup);
    if (isWakeup) {
      isWakeUpRequest = true;
    } else {
      isWakeUpRequest = false;
    }
  }

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    PowerManager powerManagerObj = (PowerManager) getActivity().getSystemService(getContext().POWER_SERVICE);
    wakeLockObj = powerManagerObj.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "USTA");
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

    SensorEventDisplayFrgmentView = inflater.inflate(R.layout.sensor_event_payload_display, container, false);

    //Initialize Payload TextFields
    eventMessageTextView = (TextView) SensorEventDisplayFrgmentView.findViewById(R.id.sensor_event_payload_textview_id);
    samplesCountedField=(TextView) SensorEventDisplayFrgmentView.findViewById(R.id.counted_Update);
    timeElapsedField=(TextView) SensorEventDisplayFrgmentView.findViewById(R.id.elapsed_update);
    expectedSamplesField=(TextView) SensorEventDisplayFrgmentView.findViewById(R.id.expected_update);
    messageIDField=(TextView) SensorEventDisplayFrgmentView.findViewById(R.id.msgID);
    TimeStampField=(TextView) SensorEventDisplayFrgmentView.findViewById(R.id.timeSTMP);
    dataField=(TextView) SensorEventDisplayFrgmentView.findViewById(R.id.dat_info);
    statusField=(TextView) SensorEventDisplayFrgmentView.findViewById(R.id.status_value);
    jsonToggle=(Button) SensorEventDisplayFrgmentView.findViewById(R.id.jsonShowButton);
    sampRateField=(TextView) SensorEventDisplayFrgmentView.findViewById(R.id.sampleRate);
    waterMarkField=(TextView) SensorEventDisplayFrgmentView.findViewById(R.id.waterMark);
    floatResolution=(TextView) SensorEventDisplayFrgmentView.findViewById(R.id.floatResolution);
    floatRange=(TextView) SensorEventDisplayFrgmentView.findViewById(R.id.floatRange);
    jsonOn=false;
    jsonToggle.setOnClickListener(new View.OnClickListener(){
        public void onClick(View v){
        jsonOn=!jsonOn;
          }
       });
      return SensorEventDisplayFrgmentView;
  }


  //Initializes thread for sample streaming
  public static SensorEventDisplayFragment CreateDisplaySamplesInstance() {
    SensorEventDisplayFragment SensorEventDisplayFragmentInstance = new SensorEventDisplayFragment();
    samplesThread = new Thread(SensorEventDisplayFragmentInstance);
    samplesThread.start();
    return SensorEventDisplayFragmentInstance;
  }

  //Callback funtion that displays sensor event data to the USTA GUI
  public void displaySamplesCallback(final String inSensorEventData) {

    if (isDisplaySamplesEnabled == true) {
      if (getActivity() != null) {
        getActivity().runOnUiThread(new Runnable() {
          @Override
          public void run() {
            if (inSensorEventData != null && inSensorEventData.length() != 0) {	//check for null message
              if(isWakeUpRequest == true) {
                wakeLockObj.acquire();
                USTALog.i("Acquired wakelock using power manager service ");
              }

            if(determineStandardConfigEvent(inSensorEventData)){//determine if event is a standard sensor config event
                  parseConfig(inSensorEventData);
            }
            else{//write event data to UI
             parseDataIntoFields(inSensorEventData);
             if(jsonOn==true){
                 eventMessageTextView.setText(inSensorEventData);
                 eventMessageTextView.setTextColor(Color.BLUE);
                 eventMessageTextView.setTypeface(null, Typeface.BOLD_ITALIC);
              }
           else{
             eventMessageTextView.setText("");
             }
              if(isWakeUpRequest == true) {
                wakeLockObj.release();
                USTALog.i("Released wakelock using power manager service ");
              }
           }
        } else {
              eventMessageTextView.setText("");
           }
         }
        });
       }
     } else {
      if (getActivity() != null) {
        getActivity().runOnUiThread(new Runnable() {
          @Override
          public void run() {
           eventMessageTextView.setText("");
          }
        });
      }
    }
  }

  //thread run event stream
  public void run() {
    while (isThreadRunning) {
      String inDisplayString = SensorContextJNI.getSamplesFromNativeWrapper(false);
      if(inDisplayString != null && inDisplayString.length() != 0){
        String updatedJSON="{\n"+inDisplayString+"\n}";//finalize JSON creation
        displaySamplesCallback(updatedJSON);
     }
    }
   }


//function that estimates the actual sampling rate of sensors based on time elapsed and events counter
//takes in the sns_client_event_msg json object
//returns -1 if the process is unsuccessful
public double  getActualSampRate(JSONObject obj){
    double reportingRate=0;
    try{
    double eventCount=Double.parseDouble(obj.getString("Event Counter").trim());
    double time=Double.parseDouble(obj.getString("Time Elapsed").trim());
    reportingRate=eventCount/time;
    return reportingRate;
    }catch(JSONException e){
    }
    return -1;
}

//Function that takes in json string created by the native layer
//Parses json data in UI TextViews
public void parseDataIntoFields(String JSONString){
    try{
        JSONObject obj=new JSONObject(JSONString);
        obj.getString("Event Counter");
        samplesCountedField.setText(obj.getString("Event Counter").trim());
        timeElapsedField.setText(obj.getString("Time Elapsed").trim());
        JSONObject eventMessage=obj.getJSONObject("sns_client_event_msg");
        JSONArray events=eventMessage.getJSONArray("events");
        TimeStampField.setText(events.getJSONObject(0).getString("timestamp"));
        messageIDField.setText(events.getJSONObject(0).getString("msg_id"));
        double timeElapsed=Double.parseDouble(obj.getString("Time Elapsed").trim());
       if(sampRateField.getText()!=null && !sampRateField.getText().toString().equals("")){
            int expectedSamplesReported=(int)(Double.parseDouble(sampRateField.getText().toString())*timeElapsed);
            expectedSamplesField.setText(Integer.toString(expectedSamplesReported));
           }
        JSONObject payload=events.getJSONObject(0).getJSONObject("payload");
        dataField.setText(payload.getJSONArray("data").toString()) ;
        statusField.setText(payload.getString("status"));
          }catch(JSONException e){
       }
}

//Function that takes in sns_client_event_msg and determines whether it is a standard sensor config event
public boolean determineStandardConfigEvent(String dat){
     try{
        JSONObject jsonParsed=new JSONObject(dat);
        String id=jsonParsed.getJSONObject("sns_client_event_msg").getJSONArray("events").getJSONObject(0).getString("msg_id");
        if(id.equals("768")){
        return true;
        }
        }catch(JSONException e){
     }
     return false;
}

//Takes in config event json string provided by USTA Native and parses data into USTA GUI
public void parseConfig(String dat){
     try{
        JSONObject jsonParsed=new JSONObject(dat);
        JSONObject events=jsonParsed.getJSONObject("sns_client_event_msg").getJSONArray("events").getJSONObject(0);
        if(events==null)
             return;
        JSONObject payload=events.getJSONObject("payload");
        if(payload==null)
             return;
        String sampleRate=payload.getString("sample_rate");
        String waterMark=payload.getString("water_mark");
        String resolution=payload.getString("resolution");
        String range=payload.getJSONArray("range").toString();
        if(sampleRate==null || waterMark==null||resolution==null||range==null)
              return;
        sampRateField.setText(sampleRate);
        waterMarkField.setText(waterMark);
        floatResolution.setText(resolution);
        floatRange.setText(range);
      }catch(JSONException e){}
  }
}
