//
// Created by bei on 2021/1/10.
//

#ifndef FFMPEGTEST_BASECHANNEL_H
#define FFMPEGTEST_BASECHANNEL_H
#include <android/log.h>
#include "safe_queue.h"
#include "JavaCallHelper.h"
#include "macro.h"
extern "C"{
#include <libavutil/rational.h>
//封装格式，总上下文
#include "libavformat/avformat.h"
//解码器.
#include "libavcodec/avcodec.h"
//#缩放
#include "libswscale/swscale.h"
// 重采样
#include "libswresample/swresample.h"
//时间工具
#include "libavutil/time.h"
//编码转换工具yuv->rgb888
#include "libavutil/imgutils.h"
}

//#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "BeiPlayer", format, ##__VA_ARGS__)
//#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "BeiPlayer", format, ##__VA_ARGS__)

class BaseChannel {
public:
    BaseChannel(int id, JavaCallHelper* javaCallHelper,
                AVCodecContext* codecContext,AVRational time_base):channelId(id),
                                                                   javaCallHelper(javaCallHelper),
                                                                   avCodecContext(codecContext),
                                                                   time_base(time_base)
    {

    }

    ~BaseChannel(){
        //销毁解码器上下文.
        if(avCodecContext){
            avcodec_close(avCodecContext);
            avcodec_free_context(&avCodecContext);
            avCodecContext = 0;
        }
        //销毁队列 ,此处有问题safe_queue.clear()，SafeQueue结构体未明确.
        pkt_queue.clear();
        frame_queue.clear();

        LOGE("释放channel:%d %d" , pkt_queue.size(), frame_queue.size());
    }

    static void releaseAvPacket(AVPacket*& packet){
        if(packet){
            av_packet_free(&packet);
            packet = 0;
        }
    }

    static void releaseAvFrame(AVFrame*& frame){
        if(frame){
            av_frame_free(&frame);
            frame = 0;
        }
    }
    static void syncHandle(queue<AVPacket*>& queue){

    }
    static void syncFrameHandle(queue<AVFrame*>& queue){

    }
    /**
     * 播放音频或视频.
     */
    virtual void play()=0;
    /**
     * 停止播放音频或视频.
     */
    virtual void stop()=0;

    /**
     * seek video.
     * @param ms
     */
    virtual void seek(long ms)=0;

public:
    SafeQueue<AVPacket *> pkt_queue;
    SafeQueue<AVFrame *> frame_queue;
    volatile int channelId;
    volatile bool isPlaying;
    AVCodecContext* avCodecContext;
    AVFormatContext* avFormatContext;
    JavaCallHelper* javaCallHelper;
    AVRational time_base;
};

#endif //FFMPEGTEST_BASECHANNEL_H
