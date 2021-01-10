//
// Created by bei on 2021/1/10.
//
#include <jni.h>
#include <string>

#include "android/native_window_jni.h"
#include "JavaCallHelper.h"

extern "C"{
//封装格式，总上下文
#include "libavformat/avformat.h"
//解码器.
#include "libavcodec/avcodec.h"
//#缩放
#include "libswscale/swscale.h"
// 重采样
#include "libswresample/swresample.h"
}
#include "BeiPlayer.h"

//绘图窗口.
ANativeWindow* window = 0;
BeiPlayer* mBeiPlayer = NULL;
JavaCallHelper* javaCallHelper;

//子线程想要回调java层就必须要先绑定到jvm.
JavaVM* javaVm = NULL;

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved){
    javaVm = vm;
    return JNI_VERSION_1_4;
}

/**
 * 渲染窗口的回调接口.
 * @param data
 * @param linesize
 * @param w
 * @param h
 */
void renderFrame(uint8_t* data, int linesize , int w, int h){
    //开始渲染 .
    LOGE("renderFrame start()!~...");
    //对本地窗口设置缓冲区大小RGBA .
    ANativeWindow_setBuffersGeometry(window , w , h,
                                     WINDOW_FORMAT_RGBA_8888);

    ANativeWindow_Buffer windowBuffer;
    if(ANativeWindow_lock(window, &windowBuffer,0)) {
        ANativeWindow_release(window);
        window = 0;
        return;
    }

    //拿到window的缓冲区. window_data[0] = 255,就代表刷新了红色.
    uint8_t * window_data = static_cast<uint8_t *>(windowBuffer.bits);
//    window_data = data; r g b a 每个元素占用4bit.
    int window_linesize = windowBuffer.stride*4;
    uint8_t * src_data = data;
//    按行拷贝rgba数据到window_buffer里面 .
    for (int i = 0; i < windowBuffer.height ; ++i) {
        //以目的地为准. 逐行拷贝.
        memcpy(window_data+i*window_linesize, src_data+i*linesize,window_linesize);
    }
    ANativeWindow_unlockAndPost(window);
    LOGE("renderFrame finished()!~...");
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ffmpegtest_widget_BeiPlayer_beiPlayerPrepare(JNIEnv *env, jobject thiz, jstring url) {
    LOGE("beiPlayerPrepare...s");
    //转化播放源地址.
    const char* input = env->GetStringUTFChars(url,0);
    //实现一个控制类.
    javaCallHelper = new JavaCallHelper(javaVm , env , thiz);
    mBeiPlayer = new BeiPlayer(javaCallHelper , input);
    //设置回调监听
    mBeiPlayer->setRenderCallBack(renderFrame);
    //进行准备
    mBeiPlayer->prepare();
    //释放资源.
    env->ReleaseStringUTFChars(url, input);
    LOGE("beiPlayerPrepare...e");
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegtest_widget_BeiPlayer_beiPlayerStart(JNIEnv *env, jobject thiz) {
    // 正式进入播放界面.
    if(mBeiPlayer){
        mBeiPlayer->start();
    }
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ffmpegtest_widget_BeiPlayer_beiPlayerSetSurface(JNIEnv *env, jobject thiz, jobject surface) {
    LOGE("set native surface invocked!");
    //释放之前的window实例.
    if(window){
        ANativeWindow_release(window);
        window = 0;
    }
    //创建AWindow.
    window = ANativeWindow_fromSurface(env, surface);
    return window == nullptr ? -1:0;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegtest_widget_BeiPlayer_beiPlayerPause(JNIEnv *env, jobject thiz) {
    // TODO: implement native_pause()
    if(mBeiPlayer){
        mBeiPlayer->pause();
    }
}

//关闭解码线程，释放资源.
extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegtest_widget_BeiPlayer_beiPlayerStop(JNIEnv *env, jobject thiz) {
    // TODO: implement native_close()
    //1. 停止video解码
    if(mBeiPlayer){
        mBeiPlayer->stop();
    }
    //2 .停止audio解码.
}extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegtest_widget_BeiPlayer_beiPlayerSeek(JNIEnv *env, jobject thiz, jlong ms) {
    // TODO: implement native_seek()
    if(mBeiPlayer){
        mBeiPlayer->seek(ms);
    }
}