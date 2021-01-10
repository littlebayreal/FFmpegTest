package com.example.ffmpegtest;

import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.example.ffmpegtest.widget.BeiPlayer;

public class BeiPlayerActivity extends AppCompatActivity {
    private BeiPlayer beiPlayer;
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_bei_player);

        beiPlayer = new BeiPlayer();
        beiPlayer.setSurfaceView(findViewById(R.id.sv));
        beiPlayer.setDataSource(Environment.getExternalStorageDirectory() + "/DCIM/sintel.mp4");
        beiPlayer.prepare();
//        beiPlayer.start();
    }

    public void clickBtn(View view) {
        switch (((Button)view).getText().toString()){
            case "播放":
                beiPlayer.start();
                break;
        }
    }
}
