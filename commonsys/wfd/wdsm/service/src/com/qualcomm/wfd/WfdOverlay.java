/* ============================================================================
 * WfdOverlay.java
 *
 * WFD Overlay implementation for Direct-Streaming
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ============================================================================*/

package com.qualcomm.wfd;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.PixelFormat;
import android.graphics.Point;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.Display;
import android.view.Gravity;
import android.view.WindowManager;
import android.widget.ImageView;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import static android.content.Context.WINDOW_SERVICE;

public class WfdOverlay {
    private static final String TAG = "JGenericOV-Event";
    private static final String BUFTAG = "JGenericOV-BUF";
    private static final boolean DEBUG = false;
    private static final int OVERLAY_PRIVATE_HEADER_SIZE = 14;
    // The time window to present image (100ms)
    private static final long GENERIC_RENDER_TOLERANCE = 100000;
    // The time window to drop the png image (200ms)
    private static final long GENERIC_AV_SYNC_DROP_WINDOW = -200000;

    private static final int MSG_OVERLAY_EVENT = 0;
    private static final int MSG_OVERLAY_DECODE = 1;
    private static final int MSG_OVERLAY_CACHE = 2;
    private static final int MSG_OVERLAY_RENDER = 3;
    private static final int MSG_OVERLAY_CLEAN = 4;

    private static final int DEFERRED_MODE = 0;
    private static final int ACTIVE_MODE = 1;
    private static final int DEACTIVE_MODE = 2;

    private Context mContext;
    private WindowManager mWindowManager = null;
    // Render Thread: showing the image and updating the internal members
    private HandlerThread mRenderThread = null;
    private RenderHandler mRenderHandler = null;
    // Decode Thread: decoding the png image
    private DecodeHandler mDecoderHandler = null;
    private HandlerThread mDecoderThread = null;
    private int mSurfaceWidth = 0;
    private int mSurfaceHeight = 0;
    private Map<Integer, OverlayInfo> mOvInfoMap = null;
    private ArrayList<Integer> mRenderQueue = null;
    private ArrayList<Integer> mClearQueue = null;

    private float mWidthScaleFactor = 0;
    private float mHeightScaleFactor = 0;
    private int mDeltaXValue = 0;
    private int mDeltaYValue = 0;

    private long mDecodeLatency = 0;
    private long mFlushTime = 0;
    private long mBaseTime = 0;
    private boolean mIsFlush = false;
    private boolean mIsPause = false;
    private boolean mIsAvSync = false;
    private boolean mIsRunning = false;
    private Object mLock = null;

    public WfdOverlay(Context context, int surfaceWidth, int surfaceHeight) {
        Log.d(TAG, "create");
        mContext = context;
        mWindowManager = (WindowManager)mContext.getSystemService(WINDOW_SERVICE);
        mSurfaceWidth = surfaceWidth;
        mSurfaceHeight = surfaceHeight;
        mRenderThread = new HandlerThread("OverlayRender");
        mRenderThread.start();
        mRenderHandler = new RenderHandler(mRenderThread.getLooper());
        mOvInfoMap = new HashMap<Integer, OverlayInfo>();
        mRenderQueue = new ArrayList<Integer>();
        mClearQueue = new ArrayList<Integer>();
        mLock = new Object();
        mIsRunning = true;
    }

    public void release() {
        Log.d(TAG, "release");
        if (mRenderThread != null && mRenderHandler != null) {
            String[] obj = {"STOP"};
            mRenderHandler.sendMessage(
                    mRenderHandler.obtainMessage(MSG_OVERLAY_EVENT, obj));
            synchronized (mLock) {
                while (mIsRunning) {
                    try {
                        Log.d(TAG, "wait to clean the bitmap");
                        mLock.wait();
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            }
            mRenderThread.quitSafely();
            try {
                mRenderThread.join();
                mRenderThread = null;
                Log.d(TAG, "render thread stopped");
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        mRenderThread = null;
        Log.d(TAG, "release done");
    }

    public void updateEvent(Object[] eventArrary) {
        if (eventArrary == null) {
            Log.e(TAG, "invalid event");
            return;
        }
        if ("BUFFER".equalsIgnoreCase((String)eventArrary[0])) {
            long timeUs = Long.decode((String)eventArrary[1]);
            int size = Integer.parseInt((String)eventArrary[2]);
            ByteBuffer buf = (ByteBuffer)eventArrary[3];
            Log.d(BUFTAG, "overlay buffer: " + size + "(" + timeUs + ")");
            if (mDecoderHandler != null && size >= OVERLAY_PRIVATE_HEADER_SIZE) {
                byte[] data = new byte[size];
                buf.get(data, 0, size);
                mDecoderHandler.sendMessage(
                        mDecoderHandler.obtainMessage(MSG_OVERLAY_DECODE,
                        new BufferInfo(data, timeUs, size)));
            }
        } else {
            if (mRenderHandler != null) {
                Object[] array = eventArrary.clone();
                mRenderHandler.sendMessage(mRenderHandler.obtainMessage(MSG_OVERLAY_EVENT, array));
            }
        }
    }

    private void doEvent(Object[] eventArrary) {
        String event = (String)eventArrary[0];
        Log.d(TAG, "doEvent: " + event);
        if ("CONFIGURE".equalsIgnoreCase(event)) {
            mDecodeLatency = Long.decode((String) eventArrary[1]);
            mIsAvSync = Long.decode((String) eventArrary[2]) == 0 ? true:false;
            String[] resolution = ((String) eventArrary[3]).split("-");
            int width = Integer.decode(resolution[0]);
            int height = Integer.decode(resolution[1]);
            doConfigure(width, height);
        } else if ("START".equalsIgnoreCase(event)) {
            doStart();
        } else if ("STOP".equalsIgnoreCase(event)) {
            doStop();
        } else if ("PAUSE".equalsIgnoreCase(event)) {
            mIsPause = true;
            mRenderHandler.removeMessages(MSG_OVERLAY_RENDER);
        } else if ("RESUME".equalsIgnoreCase(event)) {
            mIsPause = false;
            mIsFlush = false;
            mRenderHandler.sendEmptyMessage(MSG_OVERLAY_RENDER);
        } else if ("FLUSH".equalsIgnoreCase(event)) {
            mIsFlush = true;
            mFlushTime = Long.decode((String)eventArrary[1]);
            Log.d(TAG, "update flash time: " + mFlushTime);
        } else if ("BASETIME".equalsIgnoreCase(event)) {
            mBaseTime = Long.decode((String)eventArrary[1]);
            Log.d(TAG, "update base time: " + mBaseTime);
        }  else if ("DELAYTIME".equalsIgnoreCase(event)) {
            mDecodeLatency = Long.decode((String) eventArrary[1]);
            Log.d(TAG, "update decode latency: " + mDecodeLatency);
        }
    }

    private void doConfigure(int sessionWidth, int sessionHeight) {
        Display display = mWindowManager.getDefaultDisplay();
        Point outPoint = new Point();
        display.getRealSize(outPoint);
        int deviceW = outPoint.x;
        int deviceH = outPoint.y;
        mDeltaXValue = deviceW - mSurfaceWidth;
        mDeltaYValue = deviceH - mSurfaceHeight;
        mWidthScaleFactor = (float)mSurfaceWidth / (float)sessionWidth;
        mHeightScaleFactor = (float)mSurfaceHeight / (float)sessionHeight;
        Log.d(TAG, "device: " + deviceW + "x" + deviceH
                + " surface: " + mSurfaceWidth + "x" + mSurfaceHeight
                + " session: " + sessionWidth + "x" + mSurfaceHeight);
        Log.d(TAG, "delta x: " + mDeltaXValue + " delta y: " + mDeltaYValue
                + " scale w: " + mWidthScaleFactor + " scale h: " + mHeightScaleFactor);
    }

    private void doStart() {
        Log.d(TAG, "start");
        mIsPause = false;
        mIsFlush = false;
        if (mDecoderThread != null || mDecoderHandler != null) {
            Log.e(TAG, "previous decode session didn't stop");
        } else {
            mDecoderThread = new HandlerThread("OverlayDecode");
            mDecoderThread.start();
            mDecoderHandler = new DecodeHandler(mDecoderThread.getLooper());
        }
        mRenderHandler.sendEmptyMessage(MSG_OVERLAY_RENDER);
    }

    private void doStop() {
        Log.d(TAG, "stop");
        mIsPause = true;
        mRenderHandler.removeMessages(MSG_OVERLAY_EVENT);
        mRenderHandler.removeMessages(MSG_OVERLAY_CACHE);
        mRenderHandler.removeMessages(MSG_OVERLAY_RENDER);
        mRenderHandler.removeMessages(MSG_OVERLAY_CLEAN);
        if (mDecoderHandler != null) {
            mDecoderHandler.removeMessages(MSG_OVERLAY_DECODE);
        }
        if (mDecoderThread != null) {
            mDecoderThread.quitSafely();
            try {
                mDecoderThread.join();
                mDecoderThread = null;
                Log.d(TAG, "decoder thread stopped");
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        mDecoderThread = null;
        mDecoderHandler = null;
        mRenderQueue.clear();
        mClearQueue.clear();
        Iterator it;
        it = mOvInfoMap.entrySet().iterator();
        while (it.hasNext()) {
            Map.Entry entry = (Map.Entry) it.next();
            int id = (Integer) entry.getKey();
            OverlayInfo info = (OverlayInfo) entry.getValue();
            if (info.mImage != null) {
                // disappear the rendering image
                mWindowManager.removeViewImmediate(info.mImage);
                info.mImage = null;
                mRenderHandler.sendMessage(mRenderHandler.obtainMessage(MSG_OVERLAY_CLEAN, id, 1));
            } else {
                // release the bitmap which didn't render
                if (info.mBitmap != null) {
                    info.mBitmap.recycle();
                    info.mBitmap = null;
                }
                it.remove();
            }
        }
        Log.d(TAG, "doStop: mOvInfoMap size: " + mOvInfoMap.size());
        if (mOvInfoMap.size() == 0) {
            synchronized(mLock) {
                if (mIsRunning) {
                    mIsRunning = false;
                    mLock.notifyAll();
                    Log.d(TAG, "doStop: notify to stop");
                }
            }
        }
    }

    private void doDecode(BufferInfo buf) {
        if (buf == null) {
            Log.d(BUFTAG, "invalid buffer");
            return;
        }
        byte[] data = buf.mData;
        int id = getInt(data[0]);
        int pMode = getInt(data[1])>> 6 & 0x03;
        int dFlag = getInt(data[1]) >> 5 & 0x01;
        int x = getInt(data[2]) << 0x08 | getInt(data[3]);
        int y = getInt(data[4]) << 0x08 | getInt(data[5]);
        int w = getInt(data[6]) << 0x08 | getInt(data[7]);
        int h = getInt(data[8]) << 0x08 | getInt(data[9]);
        int z = 100000 + getInt(data[10]);
        Log.d(BUFTAG, "overlay info: id: " + id
                + " present mode: " + pMode + " image flag: " + dFlag
                + " x: " + x + " y: " + y + " z: " + z + " w: " + w + " h: " + h
                + " time: " + buf.mTimeUs);
        Bitmap bitmap = null;
        if (dFlag == 1 && (buf.mSize > OVERLAY_PRIVATE_HEADER_SIZE)) {
            long delay = buf.mTimeUs - getCurrentTimeUs() + mDecodeLatency;
            if (((delay <= GENERIC_AV_SYNC_DROP_WINDOW) && mIsAvSync)
                || (mIsFlush && (buf.mTimeUs <= mFlushTime))) {
                Log.d(BUFTAG, "drop this buffer");
                return;
            }
            if (DEBUG) Log.d(BUFTAG, "decode begin");
            int imageSize = buf.mSize - OVERLAY_PRIVATE_HEADER_SIZE;
            bitmap = BitmapFactory.decodeByteArray(data,
                    OVERLAY_PRIVATE_HEADER_SIZE, imageSize);
            if (bitmap == null) {
                Log.e(BUFTAG, "failed to decode image: " + id);
            } else {
                Log.d(BUFTAG, "decode image: " + id);
            }
            if (DEBUG) Log.d(BUFTAG, "decode end");
        }
        OverlayInfo info = new OverlayInfo(id, x, y, w, h, z, bitmap);
        if (pMode == ACTIVE_MODE) {
            info.mRenderTime = buf.mTimeUs;
        } else if (pMode == DEACTIVE_MODE || pMode == DEFERRED_MODE) {
            info.mClearTime = buf.mTimeUs;
        }
        mRenderHandler.sendMessage(mRenderHandler.obtainMessage(MSG_OVERLAY_CACHE, pMode, 0, info));
    }

    private void doCache(OverlayInfo info, int pMode) {
        if (pMode == ACTIVE_MODE && (info.mBitmap != null)
                && (!mIsFlush || info.mRenderTime > mFlushTime)) {
            Log.d(BUFTAG, "add info: " + info.mId + " in RenderQueue");
            mOvInfoMap.put(info.mId, info);
            mRenderQueue.add(info.mId);
        } else if (pMode == DEACTIVE_MODE || pMode == DEFERRED_MODE) {
            OverlayInfo oInfo = mOvInfoMap.get(info.mId);
            if (oInfo != null) {
                Log.d(BUFTAG, "add info: " + oInfo.mId + " in ClearQueue");
                oInfo.mClearTime = info.mClearTime;
                mClearQueue.add(oInfo.mId);
            }
        }
    }

    private void doRender() {
        if (mRenderQueue.size() == 0)
            return;
        int id = mRenderQueue.get(0);
        OverlayInfo info = mOvInfoMap.get(id);
        if (info == null)
            return;
        long delay = info.mRenderTime - getCurrentTimeUs() + mDecodeLatency;
        if (DEBUG) {
            Log.d(BUFTAG, "buffer_ts: " + info.mRenderTime + " base time: " + mBaseTime
                    + " lantency: " + mDecodeLatency);
            Log.d(BUFTAG, "show buffer decide time: " + delay);
        }
        if (delay < GENERIC_RENDER_TOLERANCE) {
            int w = (int) (info.mWidth * mWidthScaleFactor);
            int h = (int) (info.mHeight * mHeightScaleFactor);
            int x = (int) (info.mPosX * mWidthScaleFactor);
            int y = (int) (info.mPosY * mHeightScaleFactor) + mDeltaYValue;
            WindowManager.LayoutParams params = new WindowManager.LayoutParams(
                    w, h, x, y,
                    WindowManager.LayoutParams.TYPE_SYSTEM_OVERLAY,
                    WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                     | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL,
                    PixelFormat.TRANSLUCENT);
            params.gravity = Gravity.START | Gravity.TOP;
            info.mImage = new ImageView(mContext);
            info.mImage.setImageBitmap(info.mBitmap);
            Log.d(BUFTAG, "show image: " + info.mId + "(" + x + "," + y
                    + "," + w + "," + h + ")");
            mWindowManager.addView(info.mImage, params);
            if (DEBUG) Log.d(BUFTAG, "show image done");
            mRenderQueue.remove(0);

        }
    }

    private void doRemove() {
        while (mClearQueue.size() > 0) {
            int id = mClearQueue.get(0);
            OverlayInfo info = mOvInfoMap.get(id);
            if (info == null)
                break;
            long delay = info.mClearTime - getCurrentTimeUs() + mDecodeLatency;
            if (DEBUG) {
                Log.d(BUFTAG, "buffer_ts: " + info.mClearTime + " base time: " + mBaseTime
                        + " lantency: " + mDecodeLatency);
                Log.d(BUFTAG, "clear buffer decide time: " + delay);
            }
            if (delay < 0 && info.mImage != null) {
                mWindowManager.removeViewImmediate(info.mImage);
                info.mImage = null;
                mClearQueue.remove(0);
                Log.d(BUFTAG, "remove image: " + info.mId);
                mRenderHandler.sendMessage(mRenderHandler.obtainMessage(MSG_OVERLAY_CLEAN, id, 0));
            } else {
                break;
            }
        }
    }

    private void doClean(int id, int isStop) {
        OverlayInfo info = mOvInfoMap.get(id);
        if (info != null) {
            if (info.mBitmap != null) {
                info.mBitmap.recycle();
                info.mBitmap = null;
            }
            Log.d(BUFTAG, "clean bitmap: " + info.mId);
            mOvInfoMap.remove(id);
        }
        Log.d(BUFTAG, "doClean done: mOvInfoMap size: " + mOvInfoMap.size());
        if (isStop == 1 && mOvInfoMap.size() == 0) {
            synchronized(mLock) {
                if (mIsRunning) {
                    mIsRunning = false;
                    mLock.notifyAll();
                    Log.d(TAG, "doClean: notify to stop");
                }
            }
        }
    }

    private int getInt(byte b) {
        return b & 0xff;
    }

    private long getCurrentTimeUs() {
        long nowUs = System.nanoTime() / 1000;
        return nowUs - mBaseTime;
    }

    private final class RenderHandler extends Handler {
        RenderHandler(Looper looper) {
            super(looper);
        }
        @Override
        public void handleMessage(Message msg) {
            if(DEBUG){
                Log.d(TAG, "msg: " + msg.what);
            }
            switch (msg.what) {
                case MSG_OVERLAY_EVENT:
                    doEvent((Object[])msg.obj);
                    break;
                case MSG_OVERLAY_CACHE:
                    doCache((OverlayInfo)msg.obj, msg.arg1);
                    break;
                case MSG_OVERLAY_RENDER:
                    if (!mIsPause) {
                        doRender();
                        doRemove();
                        sendEmptyMessageDelayed(MSG_OVERLAY_RENDER, 5);
                    }
                    break;
                case MSG_OVERLAY_CLEAN:
                    doClean(msg.arg1, msg.arg2);
                    break;
                default:
                    break;
            }
        }
    }

    private final class DecodeHandler extends Handler {
        DecodeHandler(Looper looper) { super(looper); }
        @Override
        public void handleMessage(Message msg) {
            if(DEBUG){
                Log.d(TAG, "msg: " + msg.what);
            }
            switch (msg.what) {
                case MSG_OVERLAY_DECODE:
                    doDecode((BufferInfo) msg.obj);
                    break;
                default:
                    break;
            }
        }
    }

    private class BufferInfo {
        byte[] mData;
        long mTimeUs;
        int mSize;
        BufferInfo(byte[] data, long timeUs, int size) {
            mData = data;
            mTimeUs = timeUs;
            mSize = size;
        }
    }

    private class OverlayInfo {
        int mId = 0;
        int mPosX = 0;
        int mPosY = 0;
        int mWidth = 0;
        int mHeight = 0;
        int mZorder = 0;
        Bitmap mBitmap = null;
        long mRenderTime = 0;
        long mClearTime = 0;
        ImageView mImage = null;
        OverlayInfo(int id, int x, int y, int w, int h, int z, Bitmap bitmap) {
            mId = id;
            mPosX = x;
            mPosY = y;
            mWidth = w;
            mHeight = h;
            mZorder = z;
            mBitmap = bitmap;
        }
    }
}
