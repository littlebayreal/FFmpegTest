//
// Created by bei on 2021/1/10.
//

#ifndef FFMPEGTEST_MACRO_H
#define FFMPEGTEST_MACRO_H

#include <android/log.h>

#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "BeiPlayer", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "BeiPlayer", format, ##__VA_ARGS__)

//标记线程 因为子线程需要attach
#define THREAD_MAIN 1
#define THREAD_CHILD 2
//宏函数
#define DELETE(obj) if(obj){ delete obj; obj = 0; }

#endif //FFMPEGTEST_MACRO_H
