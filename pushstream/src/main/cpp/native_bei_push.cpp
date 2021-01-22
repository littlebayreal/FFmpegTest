//
// Created by GNNBEI on 2021/1/22.
//
#include <jni.h>
#include <string>
#include "SafeQueue.h"
//#include "VideoLive.h"
#include <pthread.h>
//#include "macro.h"
//#include "AudioLive.h"
#include "BaseChannel.h"
#include "JavaCallHelper.h"
//#include "WYuvUtils.h"
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

SafeQueue<AVPacket *> packets;//存储已经编码后的数据
//VideoLive *videoLive = 0;
//AudioLive *audioLive = 0;
int isStart = 0;
pthread_t pid_start;//从packets中取出stmp包发送

int readyPushing = 0;
uint32_t start_time;

JavaVM *javaVm = nullptr;
JavaCallHelper *javaCallHelper = 0;

int JNI_OnLoad(JavaVM *vm, void *r) {
    javaVm = vm;
    return JNI_VERSION_1_6;
}
void releasePacketCallBack(AVPacket * packet){
    if (packet){
        BaseChannel::releaseAvPacket(packet);
        packet = 0;
    }
}
void * start(void* url){
    char* path = static_cast<char *>(url);
     //所有代码执行之前要调用av_register_all和avformat_network_init
    //初始化所有的封装和解封装 flv mp4 mp3 mov。不包含编码和解码
    av_register_all();
    //初始化网络库
    avformat_network_init();
    //////////////////////////////////////////////////////////////////
    //                   输入流处理部分
    /////////////////////////////////////////////////////////////////
    //打开文件，解封装 avformat_open_input
    //AVFormatContext **ps  输入封装的上下文。包含所有的格式内容和所有的IO。如果是文件就是文件IO，网络就对应网络IO
    //const char *url  路径
    //AVInputFormt * fmt 封装器
    //AVDictionary ** options 参数设置
    AVFormatContext *ictx = NULL;
    AVFormatContext *octx = NULL;
    AVPacket pkt;

    int ret = 0;
    //打开文件，解封文件头
    ret = avformat_open_input(&ictx, path, 0, NULL);
    if (ret < 0) {
        LOGE("打开文件失败:%d", ret);
        return 0;
    }
//    if (!rtmp){
//        LOGE("rtmp创建失败");
//        //通知java层
//        if (javaCallHelper){
//            javaCallHelper->onPrepare(THREAD_CHILD,0);
//        }
//        release(rtmp,path);
//        isStart = 0;
//        return 0;
//    }
//    RTMP_Init(rtmp);
//    rtmp->Link.timeout = 5;//5秒超时时间
//    int ret = RTMP_SetupURL(rtmp,path);
//    if(!ret){
//        LOGE("rtmp设置地址失败:%s",path);
//        //通知java层
//        if (javaCallHelper){
//            javaCallHelper->onPrepare(THREAD_CHILD,0);
//        }
//        release(rtmp,path);
//        isStart = 0;
//        return 0;
//    }
//    RTMP_EnableWrite(rtmp);//开启写出数据模式，向服务器发送数据
//    ret = RTMP_Connect(rtmp,0);//连接服务器
//    if(!ret){
//        LOGE("rtmp链接地址失败:%s",path);
//        //通知java层
//        if (javaCallHelper){
//            javaCallHelper->onPrepare(THREAD_CHILD,0);
//        }
//        release(rtmp,path);
//        isStart = 0;
//        return 0;
//    }
//    ret = RTMP_ConnectStream(rtmp,0);//创建一个链接流
//    if(!ret){
//        LOGE("rtmp链接流失败:%s",path);
//        //通知java层
//        if (javaCallHelper){
//            javaCallHelper->onPrepare(THREAD_CHILD,0);
//        }
//        release(rtmp,path);
//        isStart = 0;
//        return 0;
//    }
//    //正常链接服务器，可以推流了
//    readyPushing = 1;
//    //通知java层
//    if (javaCallHelper){
//        javaCallHelper->onPrepare(THREAD_CHILD,1);
//    }
//    //
//    start_time = RTMP_GetTime();//记录开始推流时间
//    packets.setWork(1);
////    RTMPPacket *packet;
//    //第一个数据是发送aac解码数据包
////    callBack(audioLive->getAudioTag());
//    while (readyPushing){//不断从队列取出数据进行发送
//        packets.pop(packet);
//        if (!readyPushing){
//            break;
//        }
//        if(!packet){
//            continue;
//        }
//        //发送数据
//        ret = RTMP_SendPacket(rtmp,packet,1);//1表示放入队列按照顺序发送
//        releasePacketCallBack(&packet);//发送完及时释放内存
//        if (!ret) {
//            LOGE("发送数据失败");
//            break;
//        } else{
//            LOGE("发送成功");
//        }
//    }
//    isStart = 0;
//    readyPushing = 0;
//    packets.setWork(0);
//    packets.clear();
//    if(packet){
//        releasePacketCallBack(&packet);
//    }
//    release(rtmp,path);
    return 0;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_sziti_pushstream_BeiPush_init(JNIEnv *env, jobject thiz) {
    javaCallHelper = new JavaCallHelper(javaVm, env, thiz);
//    videoLive = new VideoLive;
//    videoLive->setVideoCallBack(callBack);
//    audioLive = new AudioLive;
//    audioLive->setAudioCallBack(callBack);
//    packets.setReleaseCallBack(releasePacketCallBack);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_sziti_pushstream_BeiPush_start(JNIEnv *env, jobject instance, jstring path_) {
    if (isStart){
        return;
    }
    isStart = 1;
    const char *path = env->GetStringUTFChars(path_, 0);
    char *url = new char[strlen(path)+1];
    strcpy(url,path);
//    pthread_create(&pid_start,0,start,url);
    env->ReleaseStringUTFChars(path_, path);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_sziti_pushstream_BeiPush_stop(JNIEnv *env, jobject instance) {
    readyPushing = 0;
    packets.setWork(0);
    pthread_join(pid_start,0);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_sziti_pushstream_BeiPush_release(JNIEnv *env, jobject instance) {

//    DELETE(videoLive);
//    DELETE(audioLive);
}

extern "C" //设置视频编码信息
JNIEXPORT void JNICALL
Java_com_sziti_pushstream_BeiPush_setVideoEncoderInfo(JNIEnv *env, jobject instance,
                                                              jint width, jint height, jint fps,
                                                              jint bitrate) {
//    if(videoLive){
//        videoLive->openVideoEncodec(width,height,fps,bitrate);
//    }
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_sziti_pushstream_BeiPush_getInputSamples(JNIEnv *env, jobject instance) {

//    if(audioLive){
//        return audioLive->getInputSamples();
//    }
    return -1;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_sziti_pushstream_BeiPush_setAudioEncoderInfo(JNIEnv *env, jobject instance,
                                                          jint sampleRateInHz, jint channels) {

//    if(audioLive){
//        audioLive->setAudioEncInfo(sampleRateInHz,channels);
//    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_sziti_pushstream_BeiPush_pushAudio(JNIEnv *env, jobject instance,
                                            jbyteArray data_) {
//    if(!videoLive || !readyPushing){
//        return;
//    }
//    jbyte *data = env->GetByteArrayElements(data_, NULL);
//    audioLive->encodeData(data);
//    env->ReleaseByteArrayElements(data_, data, 0);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_sziti_pushstream_BeiPush_pushVideo(JNIEnv *env, jobject instance, jbyteArray nv21_,
                                                    jint width, jint height, jboolean needRotate,
                                                    jint degree) {
//    if(!videoLive || !readyPushing){
//        return;
//    }
    jbyte *data = env->GetByteArrayElements(nv21_, NULL);
    jint src_length = env->GetArrayLength(nv21_);
//    videoLive->encodeData(data,src_length,width,height,needRotate,degree);
//    env->ReleaseByteArrayElements(nv21_, data, 0);

}




