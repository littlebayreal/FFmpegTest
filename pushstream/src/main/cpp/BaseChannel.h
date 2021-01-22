//
// Created by bei on 2021/1/10.
//

#ifndef FFMPEGTEST_BASECHANNEL_H
#define FFMPEGTEST_BASECHANNEL_H
#include <android/log.h>
#include "SafeQueue.h"
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
    BaseChannel(){}

    ~BaseChannel(){
        //销毁解码器上下文.


        LOGE("释放channel");
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


public:

};

#endif //FFMPEGTEST_BASECHANNEL_H
