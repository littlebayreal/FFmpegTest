package com.sziti.pushstream;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity implements View.OnClickListener{
    static {
        System.loadLibrary("push_stream");
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        TextView tv = findViewById(R.id.tv);
        tv.setText(stringFromJNI());
    }
    @Override
    public void onClick(View view) {
        switch (((TextView)view).getText().toString()){
            case "开始推流":
                start_encode_push("","");
                break;
        }
    }
    public native String stringFromJNI();
    public native int start_encode_push(String filePath,String url);
}