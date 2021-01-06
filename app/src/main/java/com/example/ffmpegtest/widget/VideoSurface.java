package com.example.ffmpegtest.widget;

import android.content.Context;
import android.os.Environment;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class VideoSurface extends SurfaceView implements SurfaceHolder.Callback {
    private static final String TAG = "VideoSurface";

    static {
        System.loadLibrary("native-lib");
    }

    public VideoSurface(Context context) {
        super(context);
        Log.v(TAG, "VideoSurface");
        getHolder().addCallback(this);
    }
    public VideoSurface(Context context, AttributeSet attributeSet){
        super(context,attributeSet);
        getHolder().addCallback(this);
    }
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width,
                               int height) {
        Log.v(TAG, "surfaceChanged, format is " + format + ", width is "
                + width + ", height is" + height);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.v(TAG, "surfaceCreated");
        new Thread(new Runnable() {
            @Override
            public void run() {
                ffPlay(Environment.getExternalStorageDirectory() + "/DCIM/sintel.mp4",holder.getSurface());
            }
        }).start();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.v(TAG, "surfaceDestroyed");
    }
    public native int ffPlay(String videoPath, Surface surfaceView);
}
