package com.sziti.pushstream;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity implements View.OnClickListener{
    public final static int RC_CAMERA = 100;
    static {
        System.loadLibrary("push_stream");
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        requestPermission();
        TextView tv = findViewById(R.id.tv);
        tv.setText(stringFromJNI());
    }
    @Override
    public void onClick(View view) {
        switch (((TextView)view).getText().toString()){
            case "开始推流":
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        start_encode_push(Environment.getExternalStorageDirectory()+"/DCIM/三八线19.mp4","rtmp://192.168.2.218/zxb/mylive");
                    }
                }).start();
                break;
        }
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
    public native String stringFromJNI();
    public native int start_encode_push(String filePath,String url);
}