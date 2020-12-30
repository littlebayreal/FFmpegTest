package com.example.ffmpegtest;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {
    private TextView tv;
    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("avcodec");
        System.loadLibrary("avdevice");
        System.loadLibrary("avfilter");
        System.loadLibrary("avformat");
        System.loadLibrary("avutil");
        System.loadLibrary("swresample");
        System.loadLibrary("swscale");
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        tv = findViewById(R.id.sample_text);
//        tv.setText(stringFromJNI());

//        TextView tv2 = findViewById(R.id.sample_text2);
//        tv2.setText(typeOfSystem());
    }
    public void clickBtn(View v){
        switch (((TextView)v).getText().toString()){
            case "configuration":
                tv.setText(configurationinfo());
                break;
            case "protocol":
                tv.setText(urlprotocolinfo());
                break;
            case "format":
                tv.setText(avformatinfo());
                break;
            case "codec":
                tv.setText(avcodecinfo());
                break;
            case "filter":
                tv.setText(avfilterinfo());
                break;
            case "SimpleDecodeActivity":
                Intent intent = new Intent(MainActivity.this,SimpleDecodeActivity.class);
                startActivity(intent);
                break;
            case "ffmpeg+surfacview播放视频":
                intent = new Intent(MainActivity.this,SimplePlayerActivity.class);
                startActivity(intent);
                break;
        }
    }
//    public native String typeOfSystem();
    //JNI
    public native String urlprotocolinfo();
    public native String avformatinfo();
    public native String avcodecinfo();
    public native String avfilterinfo();
    public native String configurationinfo();
}