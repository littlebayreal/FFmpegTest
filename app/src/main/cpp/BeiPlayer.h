//
// Created by GNNBEI on 2021/1/8.
//

#ifndef FFMPEGTEST_BEIPLAYER_H
#define FFMPEGTEST_BEIPLAYER_H
#include <pthread.h>
#include <android/log.h>
#include "android/native_window_jni.h"
#include <android/log.h>
#include "JavaCallHelper.h"
//#include "AudioChannel.h"
#include "VideoChannel.h"
#include "macro.h"
extern "C"{
//封装格式，总上下文
#include "libavformat/avformat.h"
//解码器.
#include "libavcodec/avcodec.h"
//#缩放
#include "libswscale/swscale.h"
//重采样
#include "libswresample/swresample.h"
//时间工具
#include "libavutil/time.h"
}

//#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "BeiPlayer", format, ##__VA_ARGS__)
//#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "BeiPlayer", format, ##__VA_ARGS__)

//播放控制.
class BeiPlayer {
private:
    bool isPause = false;
    int duration = 0;
    pthread_mutex_t seekMutex;
    pthread_mutex_t mutex_pause = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond_pause = PTHREAD_COND_INITIALIZER;
public:
    BeiPlayer(JavaCallHelper* _javaCallHelper, const char* dataSource);
    ~BeiPlayer();
    void prepare();
    void prepareFFmpeg();
    void start();
    void play();
    void pause();
    void resume();
    void stop();
    void setRenderCallBack(RenderFrame renderFrame);
    void seek(long ms);
    int getDuration();
    AVFrame *screenShot();
    void setScreenShotCallBack(OnScreenShot onScreenShot);
public:
    bool isPlaying;
    pthread_t pid_prepare;//准备完成后销毁.
    pthread_t pid_play;// 解码线程，一直存在直到播放完成.
    pthread_t pid_stop;//关闭播放器线程
    char* url;
    AVFormatContext* formatContext;
    AVCodecContext * codecContext;
    JavaCallHelper* javaCallHelper;
    VideoChannel* videoChannel;
    AudioChannel* audioChannel;
    RenderFrame renderFrame;
};
#endif //FFMPEGTEST_BEIPLAYER_H
