//
// Created by bei on 2021/1/10.
//

#ifndef FFMPEGTEST_AUDIOCHANNEL_H
#define FFMPEGTEST_AUDIOCHANNEL_H
#include "BaseChannel.h"
#include <android/native_window.h>
#include <pthread.h>
#include "JavaCallHelper.h"
#include <SLES/OpenSLES_Android.h>
#endif //FFMPEGTEST_AUDIOCHANNEL_H
class AudioChannel : public BaseChannel {

public:
    AudioChannel(int id, JavaCallHelper *javaCallHelper, AVCodecContext *codecContext,AVRational time_base,AVFormatContext* formatContext);
    /**
     * 播放音频或视频.
     */
    virtual void play();
    /**
     * 停止播放音频或视频.
     */
    virtual void stop();

    virtual void seek(long ms);

    void initOpenSL();

    void decoder();

    int getPcm();

private:
    //音频播放线程
    pthread_t pid_audio_play;
    //音频解码线程
    pthread_t pid_audio_decode;
    //ffmpeg音频处理
    SwrContext* swrContext = NULL;
    int out_channels; //通道数
    int out_samplesize;//采样率
    int out_sample_rate;//采样频率.
public:
    //音频原始数据
    uint8_t * buffer;
    double  clock;// 音视频同步使用.
};