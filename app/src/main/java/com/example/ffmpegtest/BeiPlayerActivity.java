package com.example.ffmpegtest;

import android.Manifest;
import android.graphics.Bitmap;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.PermissionChecker;

import com.example.ffmpegtest.widget.BeiPlayer;

import java.io.File;

public class BeiPlayerActivity extends AppCompatActivity implements View.OnClickListener{
    private static final String TAG = "BeiPlayerActivity";
    // Used to load the 'native-lib' library on application startup.
    private int requestPermissionCode = 10086;
    private String[] requestPermission = new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.READ_EXTERNAL_STORAGE};
    private SurfaceView mSurfaceView;
    private SeekBar mSeekBar;
    private int mPorgress;
    private EditText mUrlEtv;

    private BeiPlayer mPlayer;
    private String mUrl = "rtmp://192.168.2.218/zxb/mylive";

    private boolean isTouch;//是否正在拖动seekBar
    private boolean isSeek;
    private TextView progress_time;
    private TextView total_time;
    private ImageView screen_shot_image;
    Button play,pause,screen_shot;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON );
        setContentView(R.layout.activity_bei_player);

        //初始化布局.
        mSurfaceView = findViewById(R.id.surface_view);
        play = findViewById(R.id.btn_play);
        pause = findViewById(R.id.btn_pause);
        screen_shot = findViewById(R.id.btn_screen_shot);
        play.setOnClickListener(this);
        pause.setOnClickListener(this);
        screen_shot.setOnClickListener(this);
        screen_shot_image = findViewById(R.id.screen_shot_image);
        mSeekBar = findViewById(R.id.seek_bar);
        mUrlEtv = findViewById(R.id.edt_url);
        progress_time = findViewById(R.id.progress_time);
        total_time = findViewById(R.id.total_time);
//        mUrl = "http://ovopark-record.oss-cn-shanghai.aliyuncs.com/039570f6-e4c3-4a1b-9886-5ad7e6d7181f.mp4";
//        mUrl = "http://118.31.174.18:5581/rtmp/8e5196c4-e7d9-41b0-9080-fa0da638d9e2/live.flv";
        mUrlEtv.setText(mUrl);
        mPlayer = new BeiPlayer();
        mPlayer.setSurfaceView(mSurfaceView);
        playLocal();
        mPlayer.setOnPrepareListener(new BeiPlayer.OnPrepareListener() {
            @Override
            public void onPrepare() {
                Log.i(TAG,"准备完成，准备播放");
                //获得视频时间
                final int duration = mPlayer.getDuration();//单位秒
                Log.i(TAG,"准备完成，准备播放 duration:"+ duration);
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
//                        Toast.makeText(BeiPlayerActivity.this, "准备好了", Toast.LENGTH_SHORT).show();
                        //直播获取的时间为0，直播的时候进度条隐藏，直播拖拽也没用，玩穿越？
                        if (duration != 0) {
                            int hh  = duration / 3600;
                            int mm  = (duration % 3600) / 60;
                            int ss  = (duration % 60);
                            total_time.setText(hh+":"+mm+":"+ss);
                        }
                    }
                });
                mPlayer.start();
            }
        });
        mPlayer.setOnErrorListener(new BeiPlayer.OnErrorListener() {
            @Override
            public void onError(int errorCode) {
                Log.i(TAG, "Java接到回调:" + errorCode);
            }
        });
        mPlayer.setOnProgressListener(new BeiPlayer.OnProgressListener() {
            @Override
            public void onProgress(final int progress) {
                if (!isTouch) {//拖动时不更新
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            int duration = mPlayer.getDuration();
                            if (duration != 0) {
                                if (isSeek) {//更新的时候刚刚拖动过，这里就先不更新了，拖动完已经更新过
                                    isSeek = false;
                                    return;
                                }
                                //更新进度 计算比例
                                mSeekBar.setProgress(progress * 100 / duration);
                                int hh  = progress / 3600;
                                int mm  = (progress % 3600) / 60;
                                int ss  = (progress % 60);
                                progress_time.setText(hh+":"+mm+":"+ss);
                            }
                        }
                    });
                }
            }
        });
        //监听进度变化.
        mSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {

            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                isTouch = true;
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                isSeek = true;
                isTouch = false;
                mPorgress = mPlayer.getDuration() * seekBar.getProgress() / 100;
                //进度调整
                Log.i(TAG,"seek progress:"+ mPorgress);
                mPlayer.seek(mPorgress);
            }
        });

        // Example of a call to a native method
        if(Build.VERSION.SDK_INT > Build.VERSION_CODES.M){
            if(PermissionChecker.checkSelfPermission(this,Manifest.permission.WRITE_EXTERNAL_STORAGE) != PermissionChecker.PERMISSION_GRANTED){
                requestPermissions(requestPermission,requestPermissionCode);
            }
        }
    }


    /**
     * 播放本地视频文件 /poe/input.mp4   3205837018613102
     */
    private void playLocal() {
//        File input = new File(Environment.getExternalStorageDirectory(),"/DCIM/三八线19.mp4");
//        File input = new File(getCacheDir(),"/input.mp4");
//        Log.i("BeiPlayer","input file: "+input.getAbsolutePath());
//        if(input.exists()){
//            Log.i("BeiPlayer","input 存在！");
//        }else{
//            Log.e("BeiPlayer","input 不存存在！");
//        }

//        mPlayer.setDataSource(input.getAbsolutePath());
        mPlayer.setDataSource(mUrlEtv.getText().toString());
//        mPlayer.setOnPrepareListener(new BeiPlayer.OnPrepareListener() {
//            @Override
//            public void onPrepare() {
//                Log.i("BeiPlayer","onPrepare()# mplayer.start()!");
//                mPlayer.start();
//            }
//        });
        mPlayer.prepare();
    }

    @Override
    protected void onPause() {
        super.onPause();
        if(null != mPlayer){
            Log.i(TAG,"暂停播放器");
            mPlayer.beiPlayerPause();
        }
    }

    @Override
    protected void onStop() {
        super.onStop();
        if(null != mPlayer){
            Log.i(TAG,"停止播放器");
            mPlayer.beiPlayerStop();
        }
    }

    @Override
    protected void onDestroy() {
        Log.i(TAG,"onDestroy");
        super.onDestroy();
        if(null != mPlayer){
            Log.i(TAG,"销毁播放器");
            mPlayer.beiPlayerRelease();
        }
    }

    @Override
    public void onClick(View view) {
        switch (view.getId()){
            case R.id.btn_play:
                if(null != mPlayer){
                    Log.i(TAG,"继续播放");
                    mPlayer.beiPlayerResume();
                }
                break;
            case R.id.btn_pause:
                if(null != mPlayer){
                    Log.i(TAG,"暂停播放");
                    mPlayer.beiPlayerPause();
                }
                break;
            case R.id.btn_screen_shot:
                if(null != mPlayer){
                    Log.i(TAG,"截图");
                    byte[] yuv = mPlayer.beiPlayerScreenShot();
                    Log.i(TAG,"截图 yuv"+ yuv.length);
                    DrawScreenShot(yuv);
                }
                break;
        }
    }
    public void DrawScreenShot(byte[] yuv){
        if(yuv != null && yuv.length > 0){
            int[] colors = SimpleDecodeActivity.convertByteToColor(yuv);
            //长宽如果不正确 原始数据将会无法显示图片
            Bitmap videoBitmap = Bitmap.createBitmap(colors,1280,720, Bitmap.Config.ARGB_8888);
            screen_shot_image.setImageBitmap(videoBitmap);
        }
    }
}
