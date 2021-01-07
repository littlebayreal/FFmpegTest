package com.example.ffmpegtest.widget;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class FFmpegVideoView extends SurfaceView {
    public FFmpegVideoView(Context context) {
        super(context);
        init();
    }

    public FFmpegVideoView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public FFmpegVideoView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }
    //    public FFmpegVideoView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
    //        super(context, attrs, defStyleAttr, defStyleRes);
    //    }
    private void init() {
        SurfaceHolder holder = getHolder();
        holder.setFormat(PixelFormat.RGBA_8888);
    }
}
