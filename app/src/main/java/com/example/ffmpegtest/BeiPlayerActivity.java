package com.example.ffmpegtest;

import android.Manifest;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.SeekBar;
import android.widget.TextView;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.PermissionChecker;

import com.example.ffmpegtest.widget.BeiPlayer;

import java.io.File;

public class BeiPlayerActivity extends AppCompatActivity {
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
    private String mUrl = "rtmp://192.168.1.3:1935/oflaDemo/BladeRunner2049.flv";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON );
        setContentView(R.layout.activity_bei_player);

        //初始化布局.
        mSurfaceView = findViewById(R.id.surface_view);
        Button play = findViewById(R.id.btn_play);
        mSeekBar = findViewById(R.id.seek_bar);
        mUrlEtv = findViewById(R.id.edt_url);

//        mUrl = "http://ovopark-record.oss-cn-shanghai.aliyuncs.com/039570f6-e4c3-4a1b-9886-5ad7e6d7181f.mp4";
        mUrl = "http://118.31.174.18:5581/rtmp/8e5196c4-e7d9-41b0-9080-fa0da638d9e2/live.flv";
        mUrlEtv.setText(mUrl);
        //监听进度变化.
        mSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {

            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                // TODO: 2020/4/22 结束seek后进行seek操作.
//                mPlayer.seek(10*60*1000);
            }
        });

        play.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                playLocal();
            }
        });

        mPlayer = new BeiPlayer();
        mPlayer.setSurfaceView(mSurfaceView);

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
        File input = new File(Environment.getExternalStorageDirectory(),"/DCIM/三八线18.mp4");
//        File input = new File(getCacheDir(),"/input.mp4");
//        Log.i("BeiPlayer","input file: "+input.getAbsolutePath());
//        if(input.exists()){
//            Log.i("BeiPlayer","input 存在！");
//        }else{
//            Log.e("BeiPlayer","input 不存存在！");
//        }

        mPlayer.setDataSource(input.getAbsolutePath());
//        mPlayer.setDataSource(mUrlEtv.getText().toString());
        mPlayer.setOnPrepareListener(new BeiPlayer.OnPrepareListener() {
            @Override
            public void onPrepare() {
                Log.i("BeiPlayer","onPrepare()# mplayer.start()!");
                mPlayer.start();
            }
        });
        mPlayer.prepare();
    }

    @Override
    protected void onPause() {
        super.onPause();
        if(null != mPlayer){
            Log.i(TAG,"关闭播放器");
            mPlayer.beiPlayerPause();
            mPlayer.beiPlayerStop();
        }
    }

    @Override
    protected void onStop() {
        super.onStop();
        if(null != mPlayer){
            Log.i(TAG,"关闭播放器");
            mPlayer.beiPlayerStop();
        }
    }

    @Override
    protected void onDestroy() {
        Log.i(TAG,"onDestroy");
        super.onDestroy();
        if(null != mPlayer){
            Log.i(TAG,"关闭播放器");
            mPlayer.beiPlayerStop();
        }
    }
}
