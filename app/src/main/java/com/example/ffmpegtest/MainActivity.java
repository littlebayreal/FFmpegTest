package com.example.ffmpegtest;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;

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
            case "BeiPlayer(基于ffmpeg封装)播放视频":
                intent = new Intent(MainActivity.this,BeiPlayerActivity.class);
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

    private byte[] readFile(String path) {
        // 1、创建一个文件对象
        File file = new File(path);
        byte[] bytes = new byte[256];
        // 2、使用字节流对象读入内存
        try {
            InputStream fileIn = new FileInputStream(file);
            //DataInputStream in = new DataInputStream(fileIn);

            // 使用缓存区读入对象效率更快
            BufferedInputStream in = new BufferedInputStream(fileIn);
            in.skip(88888);
            in.read(bytes,0,255);
//            FileOutputStream fileOut = new FileOutputStream("D:\\3.jpg");
//            DataOutputStream dataOut = new DataOutputStream(fileOut);

            // 使用缓存区写入对象效率更快
            //BufferedOutputStream dataOut=new BufferedOutputStream(fileOut);
//            int temp;
//            while ((temp = in.read()) != -1) {
//                dataOut.write(temp);
//            }

        } catch (FileNotFoundException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        return bytes;
    }
}