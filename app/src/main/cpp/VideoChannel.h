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
    VideoChannel(int id, JavaCallHelper *javaCallHelper1,AVCodecContext *codecContext,AVRational time_base,AVFormatContext* formatContext);
    /**
     * 播放音频或视频.
     */
    virtual void play();
    /**
     * 停止播放音频或视频.
     */
    virtual void stop();

    virtual void seek(long ms);
    /**
     * 解码packet队列-》frame.
     */
    void decodePacket();

    /**
     * YUV --》RGB888 .
     */
    void synchronizeFrame();

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
};

#endif //FFMPEGTEST_VIDEOCHANNEL_H
