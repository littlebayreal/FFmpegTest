//
// Created by bei on 2021/1/10.
//

#ifndef FFMPEGTEST_VIDEOCHANNEL_H
#define FFMPEGTEST_VIDEOCHANNEL_H

#include "BaseChannel.h"
#include <android/native_window.h>
#include <pthread.h>
#include "AudioChannel.h"
#include "JavaCallHelper.h"
//定义一个给native层使用的回调接口.
typedef void (*RenderFrame) (uint8_t*,int,int,int);
class VideoChannel :public BaseChannel{
public:
    VideoChannel(int id, JavaCallHelper *javaCallHelper1,AVCodecContext *codecContext,AVRational time_base,
            AVFormatContext* formatContext,pthread_mutex_t seekMutex,pthread_mutex_t mutex_pause,pthread_cond_t cond_pause);
    ~VideoChannel();
    /**
     * 播放音频或视频.
     */
    virtual void play();
    /**
     * 停止播放音频或视频.
     */
    virtual void stop();
    /**
        * 暂停播放
        */
    virtual void pause();
    /**
     * 继续播放
     */
    virtual void resume();
    virtual void seek();
    /**
     * 解码packet队列-》frame.
     */
    void decodePacket();

    /**
     * YUV --》RGB888 .
     */
    void render();

    void setRenderFrame(RenderFrame renderFrame);
    void setFps(int fps);
private:
    pthread_t  pid_video_play;
    pthread_t  pid_synchronize;
    RenderFrame renderFrame;
    int fps;
public:
    AudioChannel* audioChannel; //音视频同步使用 .
    double  clock;// 音视频同步使用.
    AVFrame * screen_shot_frame;//截图视频帧的记录
};

#endif //FFMPEGTEST_VIDEOCHANNEL_H
