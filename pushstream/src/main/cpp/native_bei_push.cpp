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


    int ret = 0;
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

    int picture_size = av_image_get_buffer_size(pCodecCtx->pix_fmt, pCodecCtx->width,
                                                pCodecCtx->height, 1);
    uint8_t *buffers = (uint8_t *) av_malloc(picture_size);


    //将buffers的地址赋给AVFrame中的图像数据，根据像素格式判断有几个数据指针
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, buffers, pCodecCtx->pix_fmt,
                         pCodecCtx->width, pCodecCtx->height, 1);

    //安卓摄像头数据为NV21格式，此处将其转换为YUV420P格式
    ////N21   0~width * height是Y分量，  width*height~ width*height*3/2是VU交替存储
    //复制Y分量的数据
    memcpy(pFrameYUV->data[0], in, y_length); //Y
    pFrameYUV->pts = count;
    for (int i = 0; i < uv_length; i++) {
        //将v数据存到第三个平面
        *(pFrameYUV->data[2] + i) = *(in + y_length + i * 2);
        //将U数据存到第二个平面
        *(pFrameYUV->data[1] + i) = *(in + y_length + i * 2 + 1);
    }

    pFrameYUV->format = AV_PIX_FMT_YUV420P;
    pFrameYUV->width = yuv_width;
    pFrameYUV->height = yuv_height;

    //例如对于H.264来说。1个AVPacket的data通常对应一个NAL
    //初始化AVPacket
    av_init_packet(&enc_pkt);
//    __android_log_print(ANDROID_LOG_WARN, "eric", "编码前时间:%lld",
//                        (long long) ((av_gettime() - startTime) / 1000));
    //开始编码YUV数据
    ret = avcodec_send_frame(pCodecCtx, pFrameYUV);
    if (ret != 0) {
        logw("avcodec_send_frame error");
        return -1;
    }
    //获取编码后的数据
    ret = avcodec_receive_packet(pCodecCtx, &enc_pkt);
//    __android_log_print(ANDROID_LOG_WARN, "eric", "编码时间:%lld",
//                        (long long) ((av_gettime() - startTime) / 1000));
    //是否编码前的YUV数据
    av_frame_free(&pFrameYUV);
    if (ret != 0 || enc_pkt.size <= 0) {
        loge("avcodec_receive_packet error");
        avError(ret);
        return -2;
    }
    enc_pkt.stream_index = video_st->index;
    AVRational time_base = ofmt_ctx->streams[0]->time_base;//{ 1, 1000 };
    enc_pkt.pts = count * (video_st->time_base.den) / ((video_st->time_base.num) * fps);
    enc_pkt.dts = enc_pkt.pts;
    enc_pkt.duration = (video_st->time_base.den) / ((video_st->time_base.num) * fps);
    __android_log_print(ANDROID_LOG_WARN, "eric",
                        "index:%d,pts:%lld,dts:%lld,duration:%lld,time_base:%d,%d",
                        count,
                        (long long) enc_pkt.pts,
                        (long long) enc_pkt.dts,
                        (long long) enc_pkt.duration,
                        time_base.num, time_base.den);
    enc_pkt.pos = -1;
//    AVRational time_base_q = {1, AV_TIME_BASE};
//    //计算视频播放时间
//    int64_t pts_time = av_rescale_q(enc_pkt.dts, time_base, time_base_q);
//    //计算实际视频的播放时间
//    if (count == 0) {
//        startTime = av_gettime();
//    }
//    int64_t now_time = av_gettime() - startTime;
//    __android_log_print(ANDROID_LOG_WARN, "eric", "delt time :%lld", (pts_time - now_time));
//    if (pts_time > now_time) {
//        //睡眠一段时间（目的是让当前视频记录的播放时间与实际时间同步）
//        av_usleep((unsigned int) (pts_time - now_time));
//    }

    ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);
    if (ret != 0) {
        loge("av_interleaved_write_frame failed");
    }
    count++;
    env->ReleaseByteArrayElements(buffer_, in, 0);
//    videoLive->encodeData(data,src_length,width,height,needRotate,degree);
//    env->ReleaseByteArrayElements(nv21_, data, 0);

}




