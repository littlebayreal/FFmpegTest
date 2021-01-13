//
// Created by bei on 2021/1/10.
//

#ifndef FFMPEGTEST_MACRO_H
#define FFMPEGTEST_MACRO_H

#include <android/log.h>
#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "BeiPlayer", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "BeiPlayer", format, ##__VA_ARGS__)

//宏函数
#define DELETE(obj) if(obj){ delete obj; obj = 0; }

#define THREAD_MAIN 1
#define THREAD_CHILD 2

//错误代码
//打不开视频流
#define FFMPEG_CAN_NOT_OPEN_URL 1
//找不到流媒体
#define FFMPEG_CAN_NOT_FIND_STREAMS 2
//找不到解码器
#define FFMPEG_FIND_DECODER_FAIL 3
//无法根据解码器创建上下文
#define FFMPEG_ALLOC_CODEC_CONTEXT_FAIL 4
//根据留信息 配置上下文失败
#define FFMPEG_CODEC_PARAMETERS_FAIL 6
//打开解码器失败
#define FFMPEG_OPEN_DECODER_FIAL 7
//没有音视频
#define FFMPEG_NO_MEDIA 8

#endif //FFMPEGTEST_MACRO_H
