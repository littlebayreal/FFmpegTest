package com.example.ffmpegtest.widget;

import android.content.Context;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * beiplayer播放器控制器
 */
public class BeiPlayer implements SurfaceHolder.Callback {
    private static final String TAG = "BeiPlayer";
    private SurfaceHolder mSurfaceHolder = null;
    private String mDataSource = null;
    static {
        System.loadLibrary("native-lib");
    }
    private OnPrepareListener onPrepareListener;
    private OnProgressListener onProgressListener;
    private OnErrorListener onErrorListener;

    public void setDataSource(String url){
        mDataSource = url;
    }
    public void prepare(){
        if (TextUtils.isEmpty(mDataSource))
            throw new RuntimeException("please set data source first");
        beiPlayerPrepare(mDataSource);
    }
    public void start(){
        beiPlayerStart();
    }
    /**
     * mp4文件seek .
     * @param milliseconds
     */
    public void seek(long milliseconds){
        beiPlayerSeek(milliseconds);
    }
    public int getDuration(){
        return beiPlayerGetDuration();
    }
    //Ndk player need path and surfaceview .
    public void setSurfaceView(SurfaceView surfaceView){

        if(null != mSurfaceHolder){
            mSurfaceHolder.removeCallback(this);
        }

        this.mSurfaceHolder = surfaceView.getHolder();
        this.mSurfaceHolder.addCallback(this);
    }

    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {
        beiPlayerSetSurface(mSurfaceHolder.getSurface());
    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {
        mSurfaceHolder = surfaceHolder;
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {
        beiPlayerPause();
    }
    public void setOnPrepareListener(OnPrepareListener l){
        this.onPrepareListener = l;
    }
    public void setOnProgressListener(OnProgressListener l){
        this.onProgressListener = l;
    }
    public void setOnErrorListener(OnErrorListener l){
        this.onErrorListener = l;
    }
    //-------------------------------------提供给c的通知回调方法---------------------------------------//
    /**
     * c层准备完毕.
     */
    void onPrepare(){
        if(null != onPrepareListener) onPrepareListener.onPrepare();
    }

    /**
     * 回调播放进度.
     * @param progress
     */
    void onProgress(int progress){
        if(null != onProgressListener) onProgressListener.onProgress(progress);
    }

    /**
     * 播放出错.
     * @param errorCode
     */
    void onError(int errorCode){
        if(null != onErrorListener) onErrorListener.onError(errorCode);
    }



    public interface OnPrepareListener{
        void onPrepare();
    }

    public interface OnProgressListener{
        void onProgress(int progress);
    }

    public interface OnErrorListener{
        void onError(int errorCode);
    }

    public native int beiPlayerSetSurface(Surface surface);
    public native void beiPlayerStart();
    public native int beiPlayerPrepare(String url);
    public native void beiPlayerSeek(long time);
    public native void beiPlayerPause();
    public native void beiPlayerStop();
    public native void beiPlayerRelease();
    public native void beiPlayerResume();
    public native int beiPlayerGetDuration();
}
