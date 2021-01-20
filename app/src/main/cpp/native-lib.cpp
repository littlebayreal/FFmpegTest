#include <jni.h>
#include <string>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"

JNIEXPORT jstring
JNICALL
Java_com_example_ffmpegtest_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
//    std::string hello = "Hello from C++";
//    return env->NewStringUTF(hello.c_str());
    char info[10000] = {0};
    sprintf(info, "%s\n", avcodec_configuration());
    return env->NewStringUTF(info);
}
//JNIEXPORT jstring
//JNICALL
//Java_com_example_ffmpegtest_MainActivity_typeOfSystem(JNIEnv *env, jobject thiz) {
//    //判断常量是否被定义
//#ifdefined(__arm__)
//#ifdefined(__ARM_ARCH_7A__)
//#ifdefined(__ARM_NEON__)
//#define ABI "armeabi-v7a/NEON"
//#else
//#define ABI "armeabi-v7a"
//#endif
//#else
//#define ABI "armeabi"
//#endif
//#elif defined(__i386__)
//#define ABI "x86"
//#elif defined(__mips__)
//#define ABI "mips"
//#else
//#define ABI "unknown"
//#endif
//    return env->NewStringUTF("Hello from JNI !  Compiled with ABI " ABI ".");
//}
JNIEXPORT jstring
JNICALL
Java_com_example_ffmpegtest_MainActivity_urlprotocolinfo(JNIEnv *env, jobject thiz) {
    char info[4000] = {0};
    av_register_all();

    struct URLProtocol *pup = NULL;
    //Input
    struct URLProtocol **p_temp = &pup;
    avio_enum_protocols((void **) p_temp, 0);
    while ((*p_temp) != NULL) {
        sprintf(info, "%s[Out][%10s]\n", info, avio_enum_protocols((void **) p_temp, 1));
    }
    return env->NewStringUTF(info);
}

JNIEXPORT jstring
JNICALL
Java_com_example_ffmpegtest_MainActivity_avformatinfo(JNIEnv *env, jobject thiz) {
    char info[40000] = {0};
    av_register_all();
    AVInputFormat *if_temp = av_iformat_next(NULL);
    AVOutputFormat *of_temp = av_oformat_next(NULL);
    //Input
    while (if_temp != NULL) {
        sprintf(info, "%s[In ][%10s]\n", info, if_temp->name);
        if_temp = if_temp->next;
    }
    //Output
    while (of_temp != NULL) {
        sprintf(info, "%s[Out][%10s]\n", info, of_temp->name);
        of_temp = of_temp->next;
    }
    //LOGE("%s", info);
    return env->NewStringUTF(info);
}
JNIEXPORT jstring
JNICALL
Java_com_example_ffmpegtest_MainActivity_avcodecinfo(JNIEnv *env, jobject thiz) {
    char info[40000] = {0};
    av_register_all();
    AVCodec *c_temp = av_codec_next(NULL);
    while (c_temp != NULL) {
        if (c_temp->decode != NULL) {
            sprintf(info, "%s[Dec]", info);
        } else {
            sprintf(info, "%s[Enc]", info);
        }
        switch (c_temp->type) {
            case AVMEDIA_TYPE_VIDEO:
                sprintf(info, "%s[Video]", info);
                break;
            case AVMEDIA_TYPE_AUDIO:
                sprintf(info, "%s[Audio]", info);
                break;
            default:
                sprintf(info, "%s[Other]", info);
                break;
        }
        sprintf(info, "%s[%10s]\n", info, c_temp->name);
        c_temp = c_temp->next;
    }
    return env->NewStringUTF(info);
}
JNIEXPORT jstring
JNICALL
Java_com_example_ffmpegtest_MainActivity_avfilterinfo(JNIEnv *env, jobject thiz) {
    char info[40000] = {0};
    avfilter_register_all();
    AVFilter *f_temp = (AVFilter *) avfilter_next(NULL);
    while (f_temp != NULL) {
        sprintf(info, "%s[%10s]\n", info, f_temp->name);
        f_temp = f_temp->next;
    }
    return env->NewStringUTF(info);
}
JNIEXPORT jstring
JNICALL
Java_com_example_ffmpegtest_MainActivity_configurationinfo(JNIEnv *env, jobject thiz) {
    char info[10000] = {0};
    av_register_all();
    sprintf(info, "%s\n", avcodec_configuration());
    //LOGE("%s", info);
    return env->NewStringUTF(info);
}
};