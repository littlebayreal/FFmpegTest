package com.example.ffmpegtest;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.widget.VideoView;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.nio.ByteBuffer;

import static android.os.Environment.getExternalStorageDirectory;

public class SimpleDecodeActivity extends AppCompatActivity {
    private static final String TAG = "SimpleDecodeActivity";
    private VideoView vv;
    public final static int RC_CAMERA = 100;
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_simple_decode);
        requestPermission();
        vv = findViewById(R.id.vv);

//        vv.setVideoPath(Environment.getExternalStorageDirectory() + "/DCIM/sintel.mp4");
//        vv.start();

        new Thread(new Runnable() {
            @Override
            public void run() {
                decode( Environment.getExternalStorageDirectory() + "/DCIM/sintel.mp4",
                        Environment.getExternalStorageDirectory() + "/DCIM/output.rgb");
            }
        }).start();

    }
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
//    public native void setDecodeListener(DecodeListener decodeListener);
    public native int decode(String inputurl, String outputurl);
    public interface DecodeListener {
        /**
         * 这里为了演示自定义 Callback 的用法，使用了自定义 Java-Callback 类作为回调参数，
         * 可直接使用基本类型或者其他引用类型做回调参数，根据自己的业务需求决定。
         */
        void onDecode(ByteBuffer yuvFrame,String type);
    }
    private void requestPermission() {
        //1. 检查是否已经有该权限
        if (Build.VERSION.SDK_INT >= 23 && (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
                != PackageManager.PERMISSION_GRANTED || ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO)
                != PackageManager.PERMISSION_GRANTED || ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED)) {
            //2. 权限没有开启，请求权限
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.CAMERA, Manifest.permission.RECORD_AUDIO, Manifest.permission.WRITE_EXTERNAL_STORAGE,Manifest.permission.READ_EXTERNAL_STORAGE}, RC_CAMERA);
        }else{
            //权限已经开启，做相应事情
//            isPermissionGranted = true;
//            init();
        }
    }

    //3. 接收申请成功或者失败回调
    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == RC_CAMERA) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                //权限被用户同意,做相应的事情
//                isPermissionGranted = true;
//                init();
            } else {
                //权限被用户拒绝，做相应的事情
//                finish();
            }
        }
    }
}
