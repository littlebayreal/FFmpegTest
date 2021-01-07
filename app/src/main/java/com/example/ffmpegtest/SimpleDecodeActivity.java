package com.example.ffmpegtest;

import android.Manifest;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.util.Log;
import android.view.SurfaceHolder;
import android.widget.VideoView;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.example.ffmpegtest.widget.FFmpegVideoView;

import java.nio.ByteBuffer;
import java.util.Arrays;

public class SimpleDecodeActivity extends AppCompatActivity {
    private static final String TAG = "SimpleDecodeActivity";
    private FFmpegVideoView vv;
    public final static int RC_CAMERA = 100;
    private HandlerThread mHandlerThread = null;
    private Handler mHandler;
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_simple_decode);
        requestPermission();

        mHandlerThread = new HandlerThread("decode thread");
        mHandlerThread.start();
        mHandler = new Handler(mHandlerThread.getLooper()){
            @Override
            public void handleMessage(Message msg) {
                super.handleMessage(msg);
                switch (msg.what){
                    case 0:
                        onDraw(msg.getData().getByteArray("data"));
                        break;
                }
            }
        };
        vv = findViewById(R.id.vv);

        new Thread(new Runnable() {
            @Override
            public void run() {
//                int ret = decode( Environment.getExternalStorageDirectory() + "/DCIM/sintel.mp4",Environment.getExternalStorageDirectory()
//                        + "/Download/output.yuv");
//                Log.i(TAG,"simple decode ret:"+ ret);
                int ret = returnDecode(Environment.getExternalStorageDirectory() + "/DCIM/sintel.mp4",Environment.getExternalStorageDirectory()
                        + "/DCIM/sintel_out.rgb");
                Log.i(TAG,"simple decode ret:"+ ret);
            }
        }).start();

    }
    static {
        System.loadLibrary("native-lib");
    }
//    public native void setDecodeListener(DecodeListener decodeListener);
    public native int returnDecode(String inputurl,String outputurl);
    public native int decode(String inputurl, String outputurl);
    public interface DecodeListener {
        /**
         * 这里为了演示自定义 Callback 的用法，使用了自定义 Java-Callback 类作为回调参数，
         * 可直接使用基本类型或者其他引用类型做回调参数，根据自己的业务需求决定。
         */
        void onDecode(ByteBuffer yuvFrame,String type);
    }
    public void onDecode(byte[] yuvFrame,String type){
        byte[] temp = new byte[128];
        System.arraycopy(yuvFrame,200000,temp,0,128);
        Log.i(TAG,"rgba:"+ Arrays.toString(temp));
        Log.i(TAG,"type:"+ type);

        Message message = new Message();
        message.what = 0;
        Bundle bundle = new Bundle();
        bundle.putByteArray("data",yuvFrame);
        message.setData(bundle);
        mHandler.sendMessage(message);
    }
    /**
     * @方法描述 将RGB字节数组转换成Bitmap，
     */
    static public Bitmap rgb2Bitmap(byte[] data, int width, int height) {
        int[] colors = convertByteToColor(data);    //取RGB值转换为int数组
        if (colors == null) {
            return null;
        }

        Bitmap bmp = Bitmap.createBitmap(colors, 0, width, width, height,
                Bitmap.Config.ARGB_8888);
        return bmp;
    }
    // 将一个byte数转成int
    // 实现这个函数的目的是为了将byte数当成无符号的变量去转化成int
    public static int convertByteToInt(byte data) {

        int heightBit = (int) ((data >> 4) & 0x0F);
        int lowBit = (int) (0x0F & data);
        return heightBit * 16 + lowBit;
    }
    // 将纯RGB数据数组转化成int像素数组
    public static int[] convertByteToColor(byte[] data) {
        int size = data.length;
        if (size == 0) {
            return null;
        }

        int arg = 0;
        if (size % 3 != 0) {
            arg = 1;
        }

        // 一般RGB字节数组的长度应该是3的倍数，
        // 不排除有特殊情况，多余的RGB数据用黑色0XFF000000填充
        int[] color = new int[size / 3 + arg];
        int red, green, blue;
        int colorLen = color.length;
        if (arg == 0) {
            for (int i = 0; i < colorLen; ++i) {
                red = convertByteToInt(data[i * 3]);
                green = convertByteToInt(data[i * 3 + 1]);
                blue = convertByteToInt(data[i * 3 + 2]);

                // 获取RGB分量值通过按位或生成int的像素值
                color[i] = (red << 16) | (green << 8) | blue | 0xFF000000;
            }
        } else {
            for (int i = 0; i < colorLen - 1; ++i) {
                red = convertByteToInt(data[i * 3]);
                green = convertByteToInt(data[i * 3 + 1]);
                blue = convertByteToInt(data[i * 3 + 2]);
                color[i] = (red << 16) | (green << 8) | blue | 0xFF000000;
            }

            color[colorLen - 1] = 0xFF000000;
        }

        return color;
    }
    public void onDraw(byte[] yuvFrame){
        SurfaceHolder surfaceHolder = vv.getHolder();
        Canvas canvas = surfaceHolder.lockCanvas();
//        ByteBuffer buffer = ByteBuffer.wrap(yuvFrame);
       int[] colors = convertByteToColor(yuvFrame);
       Bitmap videoBitmap = Bitmap.createBitmap(colors,640,360, Bitmap.Config.ARGB_8888);
//        videoBitmap.copyPixelsFromBuffer(buffer);
        canvas.drawBitmap(videoBitmap, 0, 0, null);
        surfaceHolder.unlockCanvasAndPost(canvas);
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
