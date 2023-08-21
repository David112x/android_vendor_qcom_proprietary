/*============================================================================
@file QSensorService.java

@brief
Service to receive broadcasted intents and handle them

Copyright (c) 2015, 2018 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
============================================================================*/
package com.qualcomm.qti.sensors.core.qsensortest;

import com.qualcomm.qti.sensors.qsensortest.R;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

public class QSensorService extends Service {
    static final private String TAG = "QSensorTest";
    static private Context context;
    static private Context getContext() { return QSensorService.context; }

    static final private int serviceId = 1;

    private CommandReceiver commandReceiver;

    @Override
    public void onCreate() {
        QSensorService.context = this;

        QSensorContext.init(this);

        if(null == this.commandReceiver)
            this.commandReceiver = new CommandReceiver();
        QSensorService.getContext().registerReceiver(this.commandReceiver, this.commandReceiver.getIntentFilter());
    }

    @Override
    public int onStartCommand (Intent intent, int flags, int startId) {
        super.onStartCommand(intent, flags, startId);
        String notificationChannelID = "com.qualcomm.qti.sensor";
        NotificationChannel nc = new NotificationChannel(notificationChannelID, "QSensorService",
                                                          NotificationManager.IMPORTANCE_DEFAULT);
        NotificationManager nm = (NotificationManager)getSystemService(Context.NOTIFICATION_SERVICE);
        nm.createNotificationChannel(nc);
        Notification notification = new Notification.Builder(this, notificationChannelID)
                .setContentTitle("QSensorService")
                .setContentText("is running in the foreground")
                .setSmallIcon(R.drawable.icon)
                .build();
        Log.i(TAG, "starting in the Foreground");
        startForeground(startId, notification);
        return START_STICKY;
    }


    @Override
    public void onDestroy() {
        stopForeground(true);
        super.onDestroy();
        if(null != this.commandReceiver)
            QSensorService.getContext().unregisterReceiver(this.commandReceiver);
    }

    @Override
    public IBinder onBind(Intent intent) {
        // TODO: Return the communication channel to the service.
        throw new UnsupportedOperationException("Not yet implemented");
    }
}
