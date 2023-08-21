/*============================================================================
  Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

  @file SensorRequestFragment.java
  @brief
   SensorRequestFragment class implementation
============================================================================*/
package com.qualcomm.qti.usta.ui;

import android.app.Fragment;
import android.app.FragmentManager;
import android.content.Context;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import com.qualcomm.qti.usta.R;
import com.qualcomm.qti.usta.core.AccessModifier;
import com.qualcomm.qti.usta.core.DataType;
import com.qualcomm.qti.usta.core.SensorContextJNI;
import com.qualcomm.qti.usta.core.SensorPayloadField;
import com.qualcomm.qti.usta.core.SensorScriptUtility;
import com.qualcomm.qti.usta.core.USTALog;
import com.qualcomm.qti.usta.core.ModeType;
import com.qualcomm.qti.usta.core.SensorContext;

import java.util.Arrays;
import java.util.Iterator;
import java.util.Map;
import java.util.Vector;


public class SensorRequestFragment extends Fragment {

  private SensorContext sensorContext;
  private View sensorRequestFragmentView;

  private int sensorHandle;
  private int sensorReqHandle;
  private CheckBox flushOnlyCB;
  private CheckBox maxBatchCB;
  private CheckBox isPassiveCB;
  private Spinner sensorListSpinner;
  private Spinner sensorReqMsgSpinner;
  private Spinner preDefiniedScriptSpinner;
  private ArrayAdapter<String> sensorReqMsgSpinnerAdapter;
  private Spinner clientProcessorSpinner;
  private Spinner wakeupDeliverySpinner;
  private EditText batchPeriodEditText;
  private EditText flushPeriodEditText;
  public static int previousSpinnerPosition = -1;

  @Override
  public void onCreate(Bundle savedInstanceData) {
    super.onCreate(savedInstanceData);
    sensorContext = SensorContext.getSensorContext(ModeType.USTA_MODE_TYPE_UI);
    setHasOptionsMenu(true);
  }

  @Override
  public void onPrepareOptionsMenu(Menu menu) {
    menu.findItem(R.id.disable_qmi_connection_id).setEnabled(false);
  }

  private void setupSensorListSpinner() {

    sensorListSpinner = (Spinner) sensorRequestFragmentView.findViewById(R.id.sensor_list_spinner);
    ArrayAdapter<String> sensorListSpinnerAdapter = new ArrayAdapter<>(getActivity().getApplicationContext(), android.R.layout.simple_list_item_1, sensorContext.getSensorNames());

    sensorListSpinner.setAdapter(sensorListSpinnerAdapter);

    sensorListSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
      @Override
      public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {

        SensorContextJNI.disableStreamingStatusNativeWrapper(previousSpinnerPosition);
        sensorHandle = sensorListSpinner.getSelectedItemPosition();
        previousSpinnerPosition = sensorHandle;
        SensorEventDisplayFragment.updateCurrentSensorHandle(sensorHandle);
        SensorContextJNI.enableStreamingStatusNativeWrapper(sensorHandle);

        TextView suidLowTextView = (TextView) sensorRequestFragmentView.findViewById(R.id.suid_low);
        suidLowTextView.setText(sensorContext.getSensors().get(sensorHandle).getSensorSUIDLow());

        TextView suidHighTextView = (TextView) sensorRequestFragmentView.findViewById(R.id.suid_high);
        suidHighTextView.setText(sensorContext.getSensors().get(sensorHandle).getSensorSUIDHigh());

        TextView onChangeTextView = (TextView) sensorRequestFragmentView.findViewById(R.id.on_change_value);
        if(true == sensorContext.getSensors().get(sensorHandle).isOnChangeSensor()) {
          onChangeTextView.setText("True");
        } else {
          onChangeTextView.setText("False");
        }

        TextView hwIDTextView = (TextView) sensorRequestFragmentView.findViewById(R.id.hw_id_value);
        int hwID = sensorContext.getSensors().get(sensorHandle).getHwID();
        if(hwID == -1) {
          hwIDTextView.setText("NA");
        } else {
          hwIDTextView.setText(Integer.toString(hwID));
        }

        TextView rigitBodyTypeView = (TextView) sensorRequestFragmentView.findViewById(R.id.rigid_body_type);
        int rigidBodyType = sensorContext.getSensors().get(sensorHandle).getRigidBodyType();
        if(rigidBodyType == -1) {
          rigitBodyTypeView.setText("NA");
        } else {
          rigitBodyTypeView.setText(Integer.toString(rigidBodyType));
        }

        if(sensorContext.getSensorNames().get(sensorHandle).contains("registry")) {
          SensorContext.throwErrorDialog("Registry functionality is implemented in Registry TAB.  Please check in another Tab.", getContext());
        }
        else{
          resetRequestMsgListSpinner();
          resetPreDefinedScriptsSpinner();
          Toast.makeText(getActivity().getApplicationContext(), "You have selected: " + sensorContext.getSensorNames().get(sensorHandle), Toast.LENGTH_SHORT).show();
        }

      }

      @Override
      public void onNothingSelected(AdapterView<?> parent) {

      }
    });
  }

  private void assignStaticDataFromScript(Map scriptDataTable){
    /*Set Req Msg ID */
    sensorReqMsgSpinner = (Spinner) sensorRequestFragmentView.findViewById(R.id.req_msg_list_spinner);
    if(scriptDataTable != null && scriptDataTable.get("Req_Msgs_ID") != null) {
      Vector<String> req_MsgID = (Vector<String>)scriptDataTable.get("Req_Msgs_ID");
      Vector<String> reqMsgIDList = sensorContext.getSensors().get(sensorHandle).getSensorReqMsgsNamesWithMsgID();

      int defaultSpinnerPosition = 0;
      for(int reqMsgIndex = 0 ; reqMsgIndex < reqMsgIDList.size() ; reqMsgIndex++){
        if(reqMsgIDList.elementAt(reqMsgIndex).contains("513")) {
          defaultSpinnerPosition = reqMsgIndex;
          break;
        }
      }
      if (req_MsgID == null || req_MsgID.elementAt(0) == null || req_MsgID.elementAt(0).length() == 0 || req_MsgID.elementAt(0).equals("null")){
        sensorReqMsgSpinner.setSelection(defaultSpinnerPosition); // Default value for spinner
      }else {
        for(int loopIndex = 0 ; loopIndex < reqMsgIDList.size() ; loopIndex++){
          if(reqMsgIDList.elementAt(loopIndex).contains(req_MsgID.elementAt(0)) == true) {
            sensorReqMsgSpinner.setSelection(loopIndex);
            defaultSpinnerPosition = loopIndex;
            break;
          }
        }
      }
      sensorReqHandle = defaultSpinnerPosition;
    }
    /*Set Client Processor*/
    clientProcessorSpinner = (Spinner) sensorRequestFragmentView.findViewById(R.id.client_processor_spinner);
    if(scriptDataTable != null && scriptDataTable.get("Client_Processor") != null) {
      Vector<String> client_processor = (Vector<String>)scriptDataTable.get("Client_Processor");
      USTALog.d("client_processor from script is " + client_processor.elementAt(0).toLowerCase());
      if (client_processor == null || client_processor.elementAt(0) == null || client_processor.elementAt(0).length() == 0 || client_processor.elementAt(0).equals("null")){
        clientProcessorSpinner.setSelection(0); // Default value for spinner
        sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setClientProcessor(0);
      }else {
        Vector<String> clientProcessorList = sensorContext.getClientProcessors();
        for(int loopIndex = 0 ; loopIndex < clientProcessorList.size() ; loopIndex++){
          USTALog.d("client_processor from list is " + clientProcessorList.elementAt(loopIndex));
          if(clientProcessorList.elementAt(loopIndex).equals(client_processor.elementAt(0).toLowerCase()) == true) {
            USTALog.d("setting client_processor from list is " + clientProcessorList.elementAt(loopIndex) + " as spiiner position " + loopIndex);
            clientProcessorSpinner.setSelection(loopIndex);
            sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setClientProcessor(loopIndex);
            break;
          }
        }
      }
    }
    /*Set Wakeup Type */
    wakeupDeliverySpinner = (Spinner) sensorRequestFragmentView.findViewById(R.id.wakeup_spinner);
    if(scriptDataTable != null && scriptDataTable.get("wakeup_delivery") != null) {
      Vector<String> wakeup_delivery = (Vector<String>)scriptDataTable.get("wakeup_delivery");
      USTALog.d("wakeup_delivery from script is " + wakeup_delivery.elementAt(0).toLowerCase());
      if (wakeup_delivery == null || wakeup_delivery.elementAt(0) == null || wakeup_delivery.elementAt(0).length() == 0 || wakeup_delivery.elementAt(0).equals("null")){
        wakeupDeliverySpinner.setSelection(0); // Default value for spinner
        sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setWakeupDevilvery(0);
      }else {
        Vector<String> wakeDeliverList = sensorContext.getWakeupDelivery();
        for(int loopIndex = 0 ; loopIndex < wakeDeliverList.size() ; loopIndex++){
          USTALog.d("wakeDeliver from list is " + wakeDeliverList.elementAt(loopIndex));
          if(wakeDeliverList.elementAt(loopIndex).equals(wakeup_delivery.elementAt(0).toLowerCase()) == true) {
            USTALog.d("setting wakeDeliver from list is " + wakeDeliverList.elementAt(loopIndex) + " as spiiner position " + loopIndex);
            wakeupDeliverySpinner.setSelection(loopIndex);
            sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setWakeupDevilvery(loopIndex);
            break;
          }
        }
      }
    }

    /*Set Batch Period  */
    batchPeriodEditText = (EditText) sensorRequestFragmentView.findViewById(R.id.batch_period_input_text);
    if(scriptDataTable != null && scriptDataTable.get("batch_period") != null) {
      Vector<String> batch_period = (Vector<String>)scriptDataTable.get("batch_period");
      if (batch_period == null || batch_period.elementAt(0) == null || batch_period.elementAt(0).length() == 0 || batch_period.elementAt(0).equals("null")){
        batchPeriodEditText.setText(null);
        sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setBatchPeriod(null);
      }else {
        batchPeriodEditText.setText(batch_period.elementAt(0));
        sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setBatchPeriod(batch_period.elementAt(0));
      }
    }
    /*Set Flush period */
    flushPeriodEditText = (EditText) sensorRequestFragmentView.findViewById(R.id.flush_period_input_text);
    if(scriptDataTable != null && scriptDataTable.get("flush_period") != null) {
      Vector<String> flush_period = (Vector<String>)scriptDataTable.get("flush_period");
      if (flush_period == null || flush_period.elementAt(0) == null || flush_period.elementAt(0).length() == 0 || flush_period.elementAt(0).equals("null")){
        flushPeriodEditText.setText(null);
        sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setFlushPeriod(null);
      }else {
        flushPeriodEditText.setText(flush_period.elementAt(0));
        sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setFlushPeriod(flush_period.elementAt(0));
      }
    }
    /*Set Flush Only*/
    flushOnlyCB = (CheckBox) sensorRequestFragmentView.findViewById(R.id.flush_only_checkbox);
    if(scriptDataTable != null && scriptDataTable.get("flush_only") != null) {
      Vector<String> flush_only = (Vector<String>)scriptDataTable.get("flush_only");
      if (flush_only == null || flush_only.elementAt(0) == null || flush_only.elementAt(0).length() == 0 || flush_only.elementAt(0).equals("false") || flush_only.elementAt(0).equals("null")){
        flushOnlyCB.setChecked(false);
        sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setFlushOnly(false);
      }else {
        flushOnlyCB.setChecked(true);
        sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setFlushOnly(true);
      }
    }
    /*Set Max Batch */
    maxBatchCB = (CheckBox) sensorRequestFragmentView.findViewById(R.id.max_batch_checkbox);
    if(scriptDataTable != null && scriptDataTable.get("max_batch") != null) {
      Vector<String> bax_batch = (Vector<String>)scriptDataTable.get("max_batch");
      if (bax_batch == null || bax_batch.elementAt(0) == null || bax_batch.elementAt(0).length() == 0 || bax_batch.elementAt(0).equals("false") || bax_batch.elementAt(0).equals("null")){
        maxBatchCB.setChecked(false);
        sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setMaxBatch(false);
      }else {
        maxBatchCB.setChecked(true);
        sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setMaxBatch(true);
      }
    }

    isPassiveCB = (CheckBox) sensorRequestFragmentView.findViewById(R.id.is_passive_checkbox);
    if(scriptDataTable != null && scriptDataTable.get("is_passive") != null) {
      Vector<String> is_passive = (Vector<String>)scriptDataTable.get("is_passive");
      if (is_passive == null || is_passive.elementAt(0) == null || is_passive.elementAt(0).length() == 0 || is_passive.elementAt(0).equals("false") || is_passive.elementAt(0).equals("null")){
        isPassiveCB.setChecked(false);
        sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setPassive(false);
      }else {
        isPassiveCB.setChecked(true);
        sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setPassive(true);
      }
    }

  }

  private void updateFieldDataFromScript(SensorPayloadField payloadField, Map scriptDataTable) {
    /*Get the hashTable*/
    Map payLoadFragmentHashTable = SensorPayloadFragment.getMyInstance().getHashTable();

    if (payloadField.isUserDefined() == true) {
      //TODO
    } else {
      int currentWidgetID = -1;
      Vector<String> payLoadFieldValuesFromScript = (Vector<String>) scriptDataTable.get(payloadField.getFieldName());
      if (payLoadFieldValuesFromScript != null && payLoadFieldValuesFromScript.size() != 0) {

        /*this below case is same for both repeated with first data and non repeated . already we have editable box and widget ID . so simply search for it and update the field*/
        /* Iterate through it and check if it is matches with current payload */
        Iterator it = payLoadFragmentHashTable.entrySet().iterator();
        while (it.hasNext()) {
          Map.Entry currentMapEntry = (Map.Entry) it.next();
          if (currentMapEntry.getValue() != null) {
            SensorPayloadField currentPayLoadFieldFromHashTable = (SensorPayloadField) currentMapEntry.getValue();
            if (currentPayLoadFieldFromHashTable.getFieldName().equals(payloadField.getFieldName()) == true) {
              updateFieldValuefromScript(payloadField.getDataType(), payLoadFieldValuesFromScript.elementAt(0), (int) currentMapEntry.getKey(), currentPayLoadFieldFromHashTable);
              currentWidgetID = (int)currentMapEntry.getKey() ;
              break;
            }
          }
        }
        if (payloadField.getAccessModifier() == AccessModifier.REPEATED) {
          /*In this case , We don't have multiple fields. So create more editable boxes and update its content */
          createRepeateFields(payloadField ,payLoadFieldValuesFromScript.size()-1 , currentWidgetID);
        }
      }
    }
  }
  private void createRepeateFields(SensorPayloadField payloadField , int repeatCount , int currentWidgetID){
    if(currentWidgetID == -1 )
      return;
    int layoutWidgetId = currentWidgetID + 1 ;
    for(int loopIndex = 0 ; loopIndex < repeatCount ; loopIndex++ ){
      currentWidgetID ++;
      SensorPayloadFragment.getMyInstance().getHashTable().put(currentWidgetID, payloadField);
      SensorPayloadFragment.getMyInstance().incrementSensorPayloadWidgetID();

      if(SensorPayloadFragment.getMyInstance().isDataTypeNumerical(payloadField.getDataType())){
          SensorPayloadFragment.getMyInstance().addEditText((LinearLayout)SensorPayloadFragment.getMyInstance().getRepeatHashTableLayout().get(layoutWidgetId) ,payloadField.getDataType(), SensorPayloadFragment.getMyInstance().getSensorPayloadWidgetID());
      }
      if (payloadField.getDataType() == DataType.ENUM) {
        SensorPayloadFragment.getMyInstance().addSpinner( (LinearLayout) SensorPayloadFragment.getMyInstance().getRepeatHashTableLayout().get(layoutWidgetId), SensorPayloadFragment.getMyInstance().getSensorPayloadWidgetID());
      }
      if (payloadField.getDataType() == DataType.BOOLEAN) {
        SensorPayloadFragment.getMyInstance().addSpinner( (LinearLayout) SensorPayloadFragment.getMyInstance().getRepeatHashTableLayout().get(layoutWidgetId), SensorPayloadFragment.getMyInstance().getSensorPayloadWidgetID());
      }
    }
  }

  private void updateFieldValuefromScript(DataType fieldDataType, String fieldValue , int widgetID, SensorPayloadField currentPayLoadFieldFromHashTable){
    switch (fieldDataType){
      case ENUM:
        Spinner spinner = (Spinner) getActivity().findViewById(widgetID);

        if(fieldValue == null || fieldValue.length() == 0 || fieldValue.equals("null")){
          spinner.setSelection(0);
          currentPayLoadFieldFromHashTable.addValueToWidgetIDValueTable(String.valueOf(0) ,widgetID );
        }else{
          int spinnerPosition = 0;
          /*Get the spinner position*/
          Vector<String> enumList = new Vector<String>(Arrays.asList(currentPayLoadFieldFromHashTable.getEnumValues()));
          if(enumList != null && enumList.size() != 0){
            for(int enumListIndex = 0; enumListIndex < enumList.size() ; enumListIndex++){
              if(enumList.elementAt(enumListIndex).equals(fieldValue) == true) {
                spinnerPosition = enumListIndex;
                break;
              }
            }
          }
          spinner.setSelection(spinnerPosition);
          currentPayLoadFieldFromHashTable.addValueToWidgetIDValueTable(String.valueOf(spinnerPosition), widgetID);
        }
        break;
      case STRING:
      case FLOAT:
      case SIGNED_INTEGER32:
      case SIGNED_INTEGER64:
      case UNSIGNED_INTEGER32:
      case UNSIGNED_INTEGER64:
        EditText editText = (EditText) getActivity().findViewById(widgetID);
        if(fieldValue == null || fieldValue.length() == 0 || fieldValue.equals("null")){
          editText.setText(null);
          currentPayLoadFieldFromHashTable.addValueToWidgetIDValueTable(null ,widgetID );
        }else{
          editText.setText(fieldValue);
          currentPayLoadFieldFromHashTable.addValueToWidgetIDValueTable(fieldValue ,widgetID );
        }
        break;
      case BOOLEAN:
        CheckBox checkBox = (CheckBox) getActivity().findViewById(widgetID);
        if(fieldValue == null || fieldValue.length() == 0 || fieldValue.equals("null")){
          checkBox.setChecked(false);
          currentPayLoadFieldFromHashTable.addValueToWidgetIDValueTable(null ,widgetID );
        }else{
          checkBox.setChecked(true);
          currentPayLoadFieldFromHashTable.addValueToWidgetIDValueTable(String.valueOf(true) ,widgetID );
        }
        break;
    }
  }

  private void assignPayLoadDataFromScript(Map scriptDataTable) {

    Vector<SensorPayloadField> sensorPayloadFields = sensorContext.getSensors().get(sensorHandle).getSensorPayloadField(sensorReqHandle);
    if(sensorPayloadFields != null || sensorPayloadFields.size() != 0 ){
      for(int forEachField = 0 ; forEachField < sensorPayloadFields.size() ; forEachField++){
        SensorPayloadField payloadField = sensorPayloadFields.elementAt(forEachField);
        updateFieldDataFromScript(payloadField , scriptDataTable);
      }
    }
  }

  private void assignScriptData(Map scriptDataTable){
    assignStaticDataFromScript(scriptDataTable);
    assignPayLoadDataFromScript(scriptDataTable);
  }

  private void setupPreDefinedSCriptSpinner(){

    preDefiniedScriptSpinner = (Spinner) sensorRequestFragmentView.findViewById(R.id.pre_defined_script_spinner);
    ArrayAdapter<String> preDefiniedScriptSpinnerAdapter = new ArrayAdapter<>(getActivity().getApplicationContext(), android.R.layout.simple_list_item_1, sensorContext.getScriptUtil().getScriptList(sensorContext.getSensors().get(sensorHandle).getSensorSUIDLow()));
    preDefiniedScriptSpinner.setAdapter(preDefiniedScriptSpinnerAdapter);

    preDefiniedScriptSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
      @Override
      public void onItemSelected(AdapterView<?> parent, View view, int position, long id ) {
        Map scriptDataTable;
        if(preDefiniedScriptSpinner.getSelectedItemPosition() != 0 ){
          scriptDataTable = sensorContext.getScriptUtil().parseScriptFile(preDefiniedScriptSpinner.getSelectedItemPosition());
          assignScriptData(scriptDataTable);
        }
      }

      @Override
      public void onNothingSelected(AdapterView<?> parent) {

      }
    });

  }

  private void resetPreDefinedScriptsSpinner() {
      setupPreDefinedSCriptSpinner();
  }
  private void resetClientProcessorSpinner() {

    if (clientProcessorSpinner == null) {

      setupClientProcessorSpinner();
    } else {
      clientProcessorSpinner.setSelection(sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).getClientProcessor());
    }
  }

  private void resetWakeupDeliverySpinner() {

    if (wakeupDeliverySpinner == null) {

      setupWakeupDeliverySpinner();
    } else {

      wakeupDeliverySpinner.setSelection(sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).getWakeupDevilvery());
    }
  }

  private void resetRequestMsgListSpinner() {

    if (sensorReqMsgSpinner == null) {

      setupRequestMsgListSpinner();
    } else {
      sensorContext.getSensors().get(sensorHandle).clearSensorReqMsgNamesWithMsgID();
      sensorContext.getSensors().get(sensorHandle).setSensorReqMsgsNamesWithMsgID();
      @SuppressWarnings("unchecked")
      Vector<String> sensorReqMsgNamesToAdapter = (Vector<String>) sensorContext.getSensors().get(sensorHandle).getSensorReqMsgsNamesWithMsgID().clone();

      int defaultSpinnerPosition = 0;
      for(int reqMsgIndex = 0 ; reqMsgIndex < sensorReqMsgNamesToAdapter.size() ; reqMsgIndex++){
        if(sensorReqMsgNamesToAdapter.get(reqMsgIndex).contains("513")) {
          defaultSpinnerPosition = reqMsgIndex;
          break;
        }
      }

      sensorReqMsgSpinnerAdapter.clear();
      sensorReqMsgSpinnerAdapter.addAll(sensorReqMsgNamesToAdapter);
      sensorReqMsgSpinnerAdapter.notifyDataSetChanged();
      sensorReqMsgSpinner.setSelection(defaultSpinnerPosition);
      sensorReqHandle = defaultSpinnerPosition;

      sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setBatchPeriod(null);
      batchPeriodEditText.setText(null);

      sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setFlushPeriod(null);
      flushPeriodEditText.setText(null);

      sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setFlushOnly(false);
      flushOnlyCB.setChecked(false);

      sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setMaxBatch(false);
      maxBatchCB.setChecked(false);

      sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setPassive(false);
      isPassiveCB.setChecked(false);

      setupSensorPayloadFragment();
    }

    resetClientProcessorSpinner();
    resetWakeupDeliverySpinner();
  }

  private void setupRequestMsgListSpinner() {

    sensorReqMsgSpinner = (Spinner) sensorRequestFragmentView.findViewById(R.id.req_msg_list_spinner);

    sensorContext.getSensors().get(sensorHandle).clearSensorReqMsgNamesWithMsgID();
    sensorContext.getSensors().get(sensorHandle).setSensorReqMsgsNamesWithMsgID();
    Vector<String> sensorReqMsgNames = (Vector<String>) sensorContext.getSensors().get(sensorHandle).getSensorReqMsgsNamesWithMsgID();

    if ((null == sensorReqMsgNames) || sensorReqMsgNames.size() == 0) {
      return;
    }
    int defaultSpinnerPosition = 0;
    for(int reqMsgIndex = 0 ; reqMsgIndex < sensorReqMsgNames.size() ; reqMsgIndex++){
      if(sensorReqMsgNames.get(reqMsgIndex).contains("513")) {
        defaultSpinnerPosition = reqMsgIndex;
        break;
      }
    }

    Vector<String> sensorReqMsgNamesToAdapter = (Vector<String>) sensorReqMsgNames.clone();
    sensorReqMsgSpinnerAdapter = new ArrayAdapter<>(getActivity().getApplicationContext(), android.R.layout.simple_list_item_1, sensorReqMsgNamesToAdapter);
    sensorReqMsgSpinner.setAdapter(sensorReqMsgSpinnerAdapter);
    sensorReqMsgSpinner.setSelection(defaultSpinnerPosition);
    sensorReqHandle = defaultSpinnerPosition;

    sensorReqMsgSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
      @Override
      public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {

        sensorReqHandle = sensorReqMsgSpinner.getSelectedItemPosition();
        setupSensorPayloadFragment();

        resetClientProcessorSpinner();
        resetWakeupDeliverySpinner();
        batchPeriodEditText.setText(null /*sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).getBatchPeriod()*/);
        flushPeriodEditText.setText(null /*sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).getFlushPeriod()*/);
        flushOnlyCB.setChecked(false /*sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).isFlushOnly()*/);
        maxBatchCB.setChecked(false /*sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).isMaxBatch()*/);
        isPassiveCB.setChecked(false);
      }

      @Override
      public void onNothingSelected(AdapterView<?> parent) {

      }
    });
  }

  private void setupClientProcessorSpinner() {

    clientProcessorSpinner = (Spinner) sensorRequestFragmentView.findViewById(R.id.client_processor_spinner);
    ArrayAdapter<String> clientProcessorSpinnerAdapter = new ArrayAdapter<>(getActivity().getApplicationContext(), android.R.layout.simple_list_item_1, sensorContext.getClientProcessors());
    clientProcessorSpinner.setAdapter(clientProcessorSpinnerAdapter);

    clientProcessorSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
      @Override
      public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {

        sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setClientProcessor(clientProcessorSpinner.getSelectedItemPosition());
      }

      @Override
      public void onNothingSelected(AdapterView<?> parent) {

      }
    });
  }

  private void setupWakeupDeliverySpinner() {

    wakeupDeliverySpinner = (Spinner) sensorRequestFragmentView.findViewById(R.id.wakeup_spinner);
    ArrayAdapter<String> wakeupDeliverySpinnerAdapter = new ArrayAdapter<>(getActivity().getApplicationContext(), android.R.layout.simple_list_item_1, sensorContext.getWakeupDelivery());
    wakeupDeliverySpinner.setAdapter(wakeupDeliverySpinnerAdapter);

    wakeupDeliverySpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
      @Override
      public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {

        sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setWakeupDevilvery(wakeupDeliverySpinner.getSelectedItemPosition());
      }

      @Override
      public void onNothingSelected(AdapterView<?> parent) {

      }
    });
  }

  private void setupBatchPeriodEditText() {

    batchPeriodEditText = (EditText) sensorRequestFragmentView.findViewById(R.id.batch_period_input_text);
    batchPeriodEditText.setOnFocusChangeListener(new View.OnFocusChangeListener() {
      @Override
      public void onFocusChange(View v, boolean hasFocus) {

        if (v.getId() == R.id.batch_period_input_text && !hasFocus) {
          InputMethodManager imm = (InputMethodManager) getActivity().getApplicationContext().getSystemService(Context.INPUT_METHOD_SERVICE);
          imm.hideSoftInputFromWindow(v.getWindowToken(), 0);
        }
      }
    });

    TextWatcher batchPeriodTextWatcher = new TextWatcher() {
      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after) {

      }

      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count) {

        if (s.length() != 0) {
          sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setBatchPeriod(s.toString());
        } else
          sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setBatchPeriod(null);
      }

      @Override
      public void afterTextChanged(Editable s) {

      }
    };

    batchPeriodEditText.addTextChangedListener(batchPeriodTextWatcher);
  }

  private void setupFlushOnlyCheckBox () {
    flushOnlyCB    =   (CheckBox) sensorRequestFragmentView.findViewById(R.id.flush_only_checkbox);
    flushOnlyCB.setOnClickListener(new View.OnClickListener()
    {
      public void onClick(View v) {
        if (((CheckBox)v).isChecked()) {
          sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setFlushOnly(true);
        } else {
          sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setFlushOnly(false);
        }
      }
    });
  }

  private void setupMaxBatchCheckBox () {
    maxBatchCB    =   (CheckBox) sensorRequestFragmentView.findViewById(R.id.max_batch_checkbox);
    maxBatchCB.setOnClickListener(new View.OnClickListener()
    {
      public void onClick(View v) {
        if (((CheckBox)v).isChecked()) {
          sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setMaxBatch(true);
        } else {
          sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setMaxBatch(false);
        }
      }
    });
  }

  private void setupIsPassiveCheckBox () {
    isPassiveCB    =   (CheckBox) sensorRequestFragmentView.findViewById(R.id.is_passive_checkbox);
    isPassiveCB.setOnClickListener(new View.OnClickListener()
    {
      public void onClick(View v) {
        if (((CheckBox)v).isChecked()) {
          sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setPassive(true);
        } else {
          sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setPassive(false);
        }
      }
    });
  }

  private void setupFlushPeriodEditText() {

    flushPeriodEditText = (EditText) sensorRequestFragmentView.findViewById(R.id.flush_period_input_text);
    flushPeriodEditText.setOnFocusChangeListener(new View.OnFocusChangeListener() {
      @Override
      public void onFocusChange(View v, boolean hasFocus) {

        if (v.getId() == R.id.flush_period_input_text && !hasFocus) {
          InputMethodManager imm = (InputMethodManager) getActivity().getApplicationContext().getSystemService(Context.INPUT_METHOD_SERVICE);
          imm.hideSoftInputFromWindow(v.getWindowToken(), 0);
        }
      }
    });

    TextWatcher flushPeriodTextWatcher = new TextWatcher() {
      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after) {

      }

      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count) {

        if (s.length() != 0) {
          sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setFlushPeriod(s.toString());
        } else
          sensorContext.getSensors().get(sensorHandle).getSensorStdFields(sensorReqHandle).setFlushPeriod(null);
      }

      @Override
      public void afterTextChanged(Editable s) {

      }
    };

    flushPeriodEditText.addTextChangedListener(flushPeriodTextWatcher);
  }

  private void setupSensorPayloadFragment() {

    FragmentManager fm = getActivity().getFragmentManager();
    Fragment sensorPayloadFragment = SensorPayloadFragment.CreateSensorPayloadFragmentInstance(sensorHandle, sensorReqHandle, sensorContext);

    if (fm.findFragmentById(R.id.sensor_payload_fragment) != null) {

      fm.beginTransaction().replace(R.id.sensor_payload_fragment, sensorPayloadFragment).commit();
    } else {

      fm.beginTransaction().add(R.id.sensor_payload_fragment, sensorPayloadFragment).commit();
    }
  }

  private void setupSensorEvenPayloadFragment() {

    FragmentManager fm = getActivity().getFragmentManager();
    Fragment sensorEventPayloadFragment = SensorEventDisplayFragment.CreateDisplaySamplesInstance();

    if (fm.findFragmentById(R.id.sensor_event_payload_display_id) != null) {

      fm.beginTransaction().replace(R.id.sensor_event_payload_display_id, sensorEventPayloadFragment).commit();
    } else {

      fm.beginTransaction().add(R.id.sensor_event_payload_display_id, sensorEventPayloadFragment).commit();
    }

  }

  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceData) {

    if (sensorContext == null) {

      USTALog.e("sensorContext is null so Killing the activity ");

      SensorContext.throwErrorDialog("No Sensors Found! Please check if proto files are loaded.", getContext());

      return null;
    }

    sensorRequestFragmentView = inflater.inflate(R.layout.fragment_sensor_request, container, false);

    setupSensorListSpinner();

    setupBatchPeriodEditText();
    setupFlushPeriodEditText();
    setupFlushOnlyCheckBox();
    setupMaxBatchCheckBox();
    setupIsPassiveCheckBox();

    setupSensorEvenPayloadFragment();
    return sensorRequestFragmentView;
  }

}
