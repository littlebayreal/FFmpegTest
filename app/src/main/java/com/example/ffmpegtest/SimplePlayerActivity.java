package com.example.ffmpegtest;

import android.graphics.PixelFormat;
import android.os.Bundle;
import android.os.Environment;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

public class SimplePlayerActivity extends AppCompatActivity {
    private SurfaceView sv;
    static {
        System.loadLibrary("native-lib");
    }
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_simple_player);
        sv = findViewById(R.id.surface_view);
        sv.getHolder().setFormat(PixelFormat.RGBA_8888);


    }
    public native int ffplayAudio(String path);
}
