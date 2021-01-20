//
// Created by bei on 2021/1/10.
//
#include <jni.h>
#include <string>

#include "android/native_window_jni.h"
#include "JavaCallHelper.h"

extern "C" {
//封装格式，总上下文
#include "libavformat/avformat.h"
//解码器.
#include "libavcodec/avcodec.h"
//缩放
#include "libswscale/swscale.h"
// 重采样
#include "libswresample/swresample.h"
}

#include "BeiPlayer.h"

//绘图窗口.
ANativeWindow *window = 0;
BeiPlayer *mBeiPlayer = NULL;
JavaCallHelper *javaCallHelper;
//初始化线程锁
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//子线程想要回调java层就必须要先绑定到jvm.
JavaVM *javaVm = NULL;

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
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
void renderFrame(uint8_t *data, int linesize, int w, int h) {
    pthread_mutex_lock(&mutex);
    if (!window) {
        pthread_mutex_unlock(&mutex);
        return;
    }
    //开始渲染.
    LOGE("renderFrame start()!~...");
    //对本地窗口设置缓冲区大小RGBA .
    int ret = ANativeWindow_setBuffersGeometry(window, w, h,
                                               WINDOW_FORMAT_RGBA_8888);
    if (ret != 0) {
        LOGE("ANativeWindow_setBuffersGeometry failed");
        pthread_mutex_unlock(&mutex);
        return;
    }
    ANativeWindow_Buffer windowBuffer;
    if (ANativeWindow_lock(window, &windowBuffer, 0)) {
        ANativeWindow_release(window);
        window = nullptr;
        pthread_mutex_unlock(&mutex);
        return;
    }
    //拿到window的缓冲区. window_data[0] = 255,就代表刷新了红色.
    uint8_t *window_data = static_cast<uint8_t *>(windowBuffer.bits);
//    window_data = data; r g b a 每个元素占用4bit.
    int window_linesize = windowBuffer.stride * 4;
    uint8_t *src_data = data;
//    按行拷贝rgba数据到window_buffer里面 .
    for (int i = 0; i < windowBuffer.height; ++i) {
        //以目的地为准. 逐行拷贝.
        memcpy(window_data + i * window_linesize, src_data + i * linesize, window_linesize);
    }
    ANativeWindow_unlockAndPost(window);
    LOGE("renderFrame finished()!~...");
    pthread_mutex_unlock(&mutex);
}
void onScreenShot(AVFrame* avFrame){
//    if(mBeiPlayer) {
//        mBeiPlayer->videoChannel->releaseAvFrame(avFrame);
//        LOGI("释放avframe");
//    }
//    JNIEnv* env;
//    if((javaVm->AttachCurrentThread(&env , 0)) != JNI_OK){
//        return;
//    }
//    if(avFrame) {
//        //rgb接收的容器
//        AVFrame *pFrameYUV;//rgb.
//        pFrameYUV = av_frame_alloc();
//
//        uint8_t *out_buffer;
//
//        AVFrame *origin_frame = avFrame;
//        if (origin_frame == nullptr) {
//            LOGE("获取图像帧失败");
//        }
//        AVCodecContext *codecContext = mBeiPlayer->videoChannel->avCodecContext;
//
//        out_buffer = (unsigned char *) av_malloc(
//                av_image_get_buffer_size(AV_PIX_FMT_RGB24, codecContext->width,
//                                         codecContext->height, 1));
//
//        av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
//                             AV_PIX_FMT_RGB24, codecContext->width, codecContext->height,
//                             1);
//
//        LOGI("ScreenShot Width:%d Height:%d", origin_frame->width, origin_frame->height);
//        LOGI("ScreenShot Width:%d Height:%d", codecContext->width, codecContext->height);
//        SwsContext *img_convert_ctx = sws_getContext(codecContext->width, codecContext->height,
//                                                     codecContext->pix_fmt,
//                                                     codecContext->width, codecContext->height,
//                                                     AV_PIX_FMT_RGB24,
//                                                     SWS_BICUBIC, NULL, NULL, NULL);
//
//        sws_scale(img_convert_ctx, (const uint8_t *const *) origin_frame->data,
//                  origin_frame->linesize,
//                  0,
//                  codecContext->height,
//                  pFrameYUV->data, pFrameYUV->linesize);
//
//        int size = codecContext->height * codecContext->width;
//        jbyteArray jbarray = env->NewByteArray(size * 3);
//        env->SetByteArrayRegion(jbarray, 0, size * 3, (jbyte *) pFrameYUV->data[0]);
//        LOGI("ScreenShot Jbarray:%d", size);
//        return jbarray;
//    }
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ffmpegtest_widget_BeiPlayer_beiPlayerPrepare(JNIEnv *env, jobject thiz,
                                                              jstring url) {
    LOGE("beiPlayerPrepare...s");
    //转化播放源地址.
    const char *input = env->GetStringUTFChars(url, 0);
    //实现一个控制类.
    javaCallHelper = new JavaCallHelper(javaVm, env, thiz);
    mBeiPlayer = new BeiPlayer(javaCallHelper, input);
    //设置回调监听
    mBeiPlayer->setRenderCallBack(renderFrame);
//    mBeiPlayer->setScreenShotCallBack(onScreenShot);
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
    if (mBeiPlayer) {
        mBeiPlayer->start();
    }
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ffmpegtest_widget_BeiPlayer_beiPlayerSetSurface(JNIEnv *env, jobject thiz,
                                                                 jobject surface) {
    LOGE("set native surface invocked!");
    pthread_mutex_lock(&mutex);
    //释放之前的window实例.
    if (window) {
        ANativeWindow_release(window);
        window = 0;
    }
    //创建AWindow.
    window = ANativeWindow_fromSurface(env, surface);
    pthread_mutex_unlock(&mutex);
    return window == nullptr ? -1 : 0;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegtest_widget_BeiPlayer_beiPlayerPause(JNIEnv *env, jobject thiz) {
    if (mBeiPlayer) {
        mBeiPlayer->pause();
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegtest_widget_BeiPlayer_beiPlayerResume(JNIEnv *env, jobject thiz) {
    if (mBeiPlayer) {
        mBeiPlayer->resume();
    }
}
//关闭解码线程，释放资源.
extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegtest_widget_BeiPlayer_beiPlayerStop(JNIEnv *env, jobject thiz) {
    //1. 停止video解码
    if (mBeiPlayer) {
        mBeiPlayer->stop();
    }
    DELETE(javaCallHelper);
    //2 .停止audio解码.
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegtest_widget_BeiPlayer_beiPlayerRelease(JNIEnv *env, jobject thiz) {
    LOGE("native_release");
    pthread_mutex_lock(&mutex);
    if (window) {
        ANativeWindow_release(window);
        window = nullptr;
    }
    pthread_mutex_unlock(&mutex);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegtest_widget_BeiPlayer_beiPlayerSeek(JNIEnv *env, jobject thiz, jlong ms) {
    if (mBeiPlayer) {
        mBeiPlayer->seek(ms);
    }
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ffmpegtest_widget_BeiPlayer_beiPlayerGetDuration(JNIEnv *env, jobject thiz) {
    if (mBeiPlayer) {
        return mBeiPlayer->getDuration();
        LOGI("JNI getDuration:%d", mBeiPlayer->getDuration());
    }
    return 0;
}
//截取当前这一帧图像
extern "C"
JNIEXPORT jbyteArray JNICALL
Java_com_example_ffmpegtest_widget_BeiPlayer_beiPlayerScreenShot(JNIEnv *env, jobject thiz) {
//    if (mBeiPlayer) {
//       mBeiPlayer->screenShot();
//    }
    if (mBeiPlayer) {
        if (mBeiPlayer->videoChannel) {
            //rgb接收的容器
            AVFrame *pFrameYUV;//rgb.
            pFrameYUV = av_frame_alloc();

            uint8_t *out_buffer;

            AVFrame *origin_frame = mBeiPlayer->screenShot();
            if (origin_frame == nullptr) {
                LOGE("获取图像帧失败");
                return env->NewByteArray(0);
            }
            AVCodecContext* codecContext = mBeiPlayer->videoChannel->avCodecContext;
//            if(origin_frame->pict_type == AV_PICTURE_TYPE_I) {
//                LOGE("获取图像帧为关键帧");
            out_buffer = (unsigned char *) av_malloc(
                    av_image_get_buffer_size(AV_PIX_FMT_RGB24, codecContext->width,
                                             codecContext->height, 1));

            av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
                                 AV_PIX_FMT_RGB24, codecContext->width, codecContext->height,
                                 1);

            LOGI("ScreenShot Width:%d Height:%d",origin_frame->width,origin_frame->height);
            LOGI("ScreenShot Width:%d Height:%d",codecContext->width,codecContext->height);
            SwsContext *img_convert_ctx = sws_getContext(codecContext->width, codecContext->height, codecContext->pix_fmt,
                                                         codecContext->width, codecContext->height, AV_PIX_FMT_RGB24,
                                                         SWS_BICUBIC, NULL, NULL, NULL);

            sws_scale(img_convert_ctx, (const uint8_t *const *) origin_frame->data,
                      origin_frame->linesize,
                      0,
                      codecContext->height,
                      pFrameYUV->data, pFrameYUV->linesize);

            int size = codecContext->height * codecContext->width;
            jbyteArray jbarray = env->NewByteArray(size * 3);
            env->SetByteArrayRegion(jbarray, 0, size * 3, (jbyte *) pFrameYUV->data[0]);
            LOGI("ScreenShot Jbarray:%d", size);
            return jbarray;
        }
    }
    return env->NewByteArray(0);
}