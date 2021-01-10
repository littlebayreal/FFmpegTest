//
// Created by bei on 2021/1/6.
//
#include <stdio.h>
#include <time.h>
#include <android/native_window_jni.h>
#include <jni.h>
#include <android/log.h>
#include <unistd.h>
#include "SLES/OpenSLES.h"
#include "SLES/OpenSLES_Android.h"
#include "macro.h"
/**
 * 播放viedeo
 * @param env
 * @param instance
 * @param videoPath
 * @param surface
 */
#ifdef ANDROID  //如果是android编译器 使用android log 输出


//#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "(>_<)", format, ##__VA_ARGS__)
//#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "(^_^)", format, ##__VA_ARGS__)

void createPlayer(const char *path);

int getAudioInfo(int *pInt, int *pInt1, const char *path);

int getPcm(void **buffer,size_t *bsize);

void getQueueCallBack(SLAndroidSimpleBufferQueueItf slBufferQueueItf, void *context);
#else

#define LOGE(format, ...)  printf("(>_<) " format "\n", ##__VA_ARGS__)
#define LOGI(format, ...)  printf("(^_^) " format "\n", ##__VA_ARGS__)
#endif

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libavutil/log.h"
#include "libswresample/swresample.h"
JNIEXPORT jint JNICALL
Java_com_example_ffmpegtest_widget_VideoSurface_ffPlay(JNIEnv *env, jobject instance,
                                                       jstring
                                                       videoPath, jobject surface) {
    const char *input = env->GetStringUTFChars(videoPath, NULL);
    if (input == NULL) {
        LOGE ("字符串转换失败......");
        return -1;
    }

//注册FFmpeg所有编解码器，以及相关协议。
    av_register_all();

//分配结构体
    AVFormatContext *formatContext = avformat_alloc_context();
//打开视频数据源。由于Android 对SDK存储权限的原因，如果没有为当前项目赋予SDK存储权限，打开本地视频文件时会失败
    int open_state = avformat_open_input(&formatContext, input, NULL, NULL);
    if (open_state < 0) {
        char errbuf[128];
        if (av_strerror(open_state, errbuf, sizeof(errbuf)) == 0) {
            LOGE("打开视频输入流信息失败，失败原因： %s", errbuf);
        }
        return -2;
    }
//为分配的AVFormatContext 结构体中填充数据
    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        LOGE("读取输入的视频流信息失败。");
        return -3;
    }
    int video_stream_index = -1;//记录视频流所在数组下标
    LOGI("当前视频数据，包含的数据流数量：%d", formatContext->nb_streams);
//找到"视频流".AVFormatContext 结构体中的nb_streams字段存储的就是当前视频文件中所包含的总数据流数量——
//视频流，音频流，字幕流
    for (int i = 0; i < formatContext->nb_streams; i++) {
        //如果是数据流的编码格式为AVMEDIA_TYPE_VIDEO——视频流。
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;//记录视频流下标
            break;
        }
    }
    if (video_stream_index == -1) {
        LOGE("没有找到 视频流。");
        return -4;
    }
    //通过编解码器的id——codec_id 获取对应（视频）流解码器
    AVCodecParameters *codecParameters = formatContext->streams[video_stream_index]->codecpar;
    AVCodec *videoDecoder = avcodec_find_decoder(codecParameters->codec_id);

    //通过视频轨道中的codec设置解码器类型
//    pCodecCtx=pFormatCtx->streams[videoindex]->codec;
//    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if (videoDecoder == NULL) {
        LOGE("未找到对应的流解码器。");
        return -5;
    }
//通过解码器分配(并用  默认值   初始化)一个解码器context
    AVCodecContext *codecContext = avcodec_alloc_context3(videoDecoder);
    if (codecContext == NULL) {
        LOGE("分配 解码器上下文失败。");
        return -6;
    }
//更具指定的编码器值填充编码器上下文
    if (avcodec_parameters_to_context(codecContext, codecParameters) < 0) {
        LOGE("填充编解码器上下文失败。");
        return -7;
    }
//通过所给的编解码器初始化编解码器上下文
    if (avcodec_open2(codecContext, videoDecoder, NULL) < 0) {
        LOGE("初始化 解码器上下文失败。");
        return -8;
    }
//输出视频信息
    LOGI("视频的文件格式：%s", formatContext->iformat->name);
    LOGI("视频时长：%lf", (formatContext->
            duration * av_q2d(formatContext->streams[video_stream_index]->time_base)
                     ) / 1000000);
    LOGI("视频的宽高：%d,%d", codecContext->width, codecContext->height);
    LOGI("解码器的名称：%s", videoDecoder->name);
    AVPixelFormat dstFormat = AV_PIX_FMT_RGBA;
//分配存储压缩数据的结构体对象AVPacket
//如果是视频流，AVPacket会包含一帧的压缩数据。
//但如果是音频则可能会包含多帧的压缩数据
    AVPacket *packet = av_packet_alloc();
//分配解码后的每一数据信息的结构体（指针）
    AVFrame *frame = av_frame_alloc();
//分配最终显示出来的目标帧信息的结构体（指针）
    AVFrame *outFrame = av_frame_alloc();
    uint8_t *out_buffer = (uint8_t *) av_malloc(
            (size_t) av_image_get_buffer_size(dstFormat, codecContext->width, codecContext->height,
                                              1));
//更具指定的数据初始化/填充缓冲区
    av_image_fill_arrays(outFrame->data, outFrame->linesize, out_buffer, dstFormat,
                         codecContext->width, codecContext->height, 1);
//初始化SwsContext
    SwsContext *swsContext = sws_getContext(
            codecContext->width   //原图片的宽
            , codecContext->height  //源图高
            , codecContext->pix_fmt //源图片format
            , codecContext->width  //目标图的宽
            , codecContext->height  //目标图的高
            , dstFormat, SWS_BICUBIC, NULL, NULL, NULL
    );
    if (swsContext == NULL) {
        LOGE("swsContext==NULL");
        return -9;
    }
//Android 原生绘制工具
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    if (nativeWindow == nullptr) {
        LOGE("nativeWindow==NULL");
        return -10;
    }
//定义绘图缓冲区
    ANativeWindow_Buffer outBuffer;
//通过设置宽高限制缓冲区中的像素数量，而非屏幕的物流显示尺寸。
//如果缓冲区与物理屏幕的显示尺寸不相符，则实际显示可能会是拉伸，或者被压缩的图像
    ANativeWindow_setBuffersGeometry(nativeWindow, codecContext
                                             ->width, codecContext->height,
                                     WINDOW_FORMAT_RGBA_8888);
//循环读取数据流的下一帧
    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == video_stream_index) {
//讲原始数据发送到解码器
            int sendPacketState = avcodec_send_packet(codecContext, packet);
            if (sendPacketState == 0) {
                int receiveFrameState = avcodec_receive_frame(codecContext, frame);
                if (receiveFrameState == 0) {
                    //锁定窗口绘图界面
                    ANativeWindow_lock(nativeWindow, &outBuffer, NULL);
                    //对输出图像进行色彩，分辨率缩放，滤波处理
                    sws_scale(swsContext,
                              (const uint8_t *const *) frame->data, frame->linesize, 0,
                              frame->height, outFrame->data, outFrame->linesize);
                    //准备绘制的目标缓冲区
                    uint8_t *dst = (uint8_t *) outBuffer.bits;
                    //解码后的像素数据首地址
                    //这里由于使用的是RGBA格式，所以解码图像数据只保存在data[0]中。但如果是YUV就会有data[0]
                    //data[1],data[2]
                    uint8_t *src = outFrame->data[0];
                    //获取一行字节数
                    int oneLineByte = outBuffer.stride * 4;
                    //复制一行内存的实际数量
                    int srcStride = outFrame->linesize[0];
                    for (int i = 0; i < codecContext->height; i++) {
                        //将frame中已解码的数据根据真实数据长度复制给界面缓冲区
                        memcpy(dst + i * oneLineByte, src + i * srcStride, srcStride);
                    }
//解锁
                    ANativeWindow_unlockAndPost(nativeWindow);
//进行短暂休眠。如果休眠时间太长会导致播放的每帧画面有延迟感，如果短会有加速播放的感觉。
//一般一每秒60帧——16毫秒一帧的时间进行休眠
                    usleep(1000 * 40);//20毫秒

                } else if (receiveFrameState == AVERROR(EAGAIN)) {
                    LOGE("从解码器-接收-数据失败：AVERROR(EAGAIN)");
                } else if (receiveFrameState == AVERROR_EOF) {
                    LOGE("从解码器-接收-数据失败：AVERROR_EOF");
                } else if (receiveFrameState == AVERROR(EINVAL)) {
                    LOGE("从解码器-接收-数据失败：AVERROR(EINVAL)");
                } else {
                    LOGE("从解码器-接收-数据失败：未知");
                }
            } else if (sendPacketState == AVERROR(EAGAIN)) {//发送数据被拒绝，必须尝试先读取数据
                LOGE("向解码器-发送-数据包失败：AVERROR(EAGAIN)");//解码器已经刷新数据但是没有新的数据包能发送给解码器
            } else if (sendPacketState == AVERROR_EOF) {
                LOGE("向解码器-发送-数据失败：AVERROR_EOF");
            } else if (sendPacketState == AVERROR(EINVAL)) {//遍解码器没有打开，或者当前是编码器，也或者需要刷新数据
                LOGE("向解码器-发送-数据失败：AVERROR(EINVAL)");
            } else if (sendPacketState == AVERROR(ENOMEM)) {//数据包无法压如解码器队列，也可能是解码器解码错误
                LOGE("向解码器-发送-数据失败：AVERROR(ENOMEM)");
            } else {
                LOGE("向解码器-发送-数据失败：未知");
            }
        }
        av_packet_unref(packet);
    }
    //内存释放
    ANativeWindow_release(nativeWindow);
    av_frame_free(&outFrame);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);
    avformat_free_context(formatContext);
    env->ReleaseStringUTFChars(videoPath, input);
    //没有返回值会导致程序崩溃
    return 0;
}
//--------------------------------------openSL ES 播放声音--------------------------------------------------------------
SLObjectItf engineObject = NULL;//用SLObjectItf声明引擎接口对象
SLEngineItf engineEngine = NULL;//声明具体的引擎对象
SLObjectItf outputMixObject = NULL;//用SLObjectItf创建混音器接口对象
SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;////具体的混音器对象实例
SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;//默认情况


SLObjectItf audioplayer = NULL;//用SLObjectItf声明播放器接口对象
SLPlayItf slPlayItf = NULL;//播放器接口
SLAndroidSimpleBufferQueueItf slBufferQueueItf = NULL;//缓冲区队列接口

JNIEXPORT jint JNICALL
Java_com_example_ffmpegtest_SimplePlayerActivity_ffplayAudio(JNIEnv *env, jobject thiz,
                                                             jstring path) {
    //这是用OpenSL EL 进行播放
    //createEngine();
    SLEngineOption options[] = {
            {(SLuint32)SL_ENGINEOPTION_THREADSAFE, (SLuint32)SL_BOOLEAN_TRUE}
    };
    slCreateEngine(&engineObject, 0, options, 0, NULL, NULL);//创建引擎
    (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);//实现engineObject接口对象
    (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE,
                                  &engineEngine);//通过引擎调用接口初始化SLEngineItf

    //createMixVolume();
    (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, 0, 0);//用引擎对象创建混音器接口对象
    (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);//实现混音器接口对象
    SLresult sLresult = (*outputMixObject)->GetInterface(outputMixObject,
                                                         SL_IID_ENVIRONMENTALREVERB,
                                                         &outputMixEnvironmentalReverb);//利用混音器实例对象接口初始化具体的混音器对象
    //设置
    if (SL_RESULT_SUCCESS == sLresult) {
        (*outputMixEnvironmentalReverb)->
                SetEnvironmentalReverbProperties(outputMixEnvironmentalReverb, &settings);
    }
    const char *audioPath = env->GetStringUTFChars(path, NULL);
    createPlayer(audioPath);
//    int rate;
//    int channels;
//    getAudioInfo(&rate, &channels, audioPath);
//    void* buffer = {0};
//    size_t buffersize = 0;
//    getPcm(&buffer, &buffersize);
//    LOGI("buffersize:%d",buffersize);
    env->ReleaseStringUTFChars(path, audioPath);
    return 0;
 }
}

//创建播放器
void createPlayer(const char *path) {
    //初始化ffmpeg
    int rate;
    int channels;
    getAudioInfo(&rate, &channels, path);
    LOGE("RATE %d", rate);
    LOGE("channels %d", channels);

    SLDataLocator_AndroidBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
//    /**
//    typedef struct SLDataFormat_PCM_ {
//        SLuint32 		formatType;  pcm
//        SLuint32 		numChannels;  通道数
//        SLuint32 		samplesPerSec;  采样率
//        SLuint32 		bitsPerSample;  采样位数
//        SLuint32 		containerSize;  包含位数
//        SLuint32 		channelMask;     立体声
//        SLuint32		endianness;    end标志位
//    } SLDataFormat_PCM;
//     */
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, (SLuint32) channels, (SLuint32) rate * 1000,
                            SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};

//    /*
//     * typedef struct SLDataSource_ {
//	        void *pLocator;//缓冲区队列
//	        void *pFormat;//数据样式,配置信息
//        } SLDataSource;
//     * */
    SLDataSource dataSource = {&android_queue, &pcm};

    SLDataLocator_OutputMix slDataLocator_outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};

    SLDataSink slDataSink = {&slDataLocator_outputMix, NULL};

    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_FALSE, SL_BOOLEAN_FALSE, SL_BOOLEAN_FALSE};

    /*
     * SLresult (*CreateAudioPlayer) (
		SLEngineItf self,
		SLObjectItf * pPlayer,
		SLDataSource *pAudioSrc,//数据设置
		SLDataSink *pAudioSnk,//关联混音器
		SLuint32 numInterfaces,
		const SLInterfaceID * pInterfaceIds,
		const SLboolean * pInterfaceRequired
	);
     * */

    SLresult result =  (*engineEngine)->CreateAudioPlayer(engineEngine, &audioplayer, &dataSource, &slDataSink, 3, ids,
                                       req);
    LOGE("执行到此处 result:%d",result);
    (*audioplayer)->Realize(audioplayer, SL_BOOLEAN_FALSE);
    LOGE("执行到此处2");
    (*audioplayer)->GetInterface(audioplayer, SL_IID_PLAY, &slPlayItf);//初始化播放器
    //注册缓冲区,通过缓冲区里面 的数据进行播放
    (*audioplayer)->GetInterface(audioplayer, SL_IID_BUFFERQUEUE, &slBufferQueueItf);
    //设置回调接口
    (*slBufferQueueItf)->RegisterCallback(slBufferQueueItf, getQueueCallBack, NULL);
    //播放
    (*slPlayItf)->SetPlayState(slPlayItf, SL_PLAYSTATE_PLAYING);

    //开始播放
    getQueueCallBack(slBufferQueueItf, NULL);
}
size_t buffersize = 0;
void *buffer;

//将pcm数据添加到缓冲区中
void getQueueCallBack(SLAndroidSimpleBufferQueueItf slBufferQueueItf, void *context) {
    buffersize = 0;
    //解码文件 获取音频元数据
    getPcm(&buffer, &buffersize);
    if (buffer != NULL && buffersize != 0) {
        //将得到的数据加入到队列中
        (*slBufferQueueItf)->Enqueue(slBufferQueueItf, buffer, buffersize);
    }
}

AVFormatContext *pFormatCtx;
AVCodecContext *pCodecCtx;
AVCodec *pCodex;
AVPacket *packet;
AVFrame *frame;
SwrContext *swrContext;
uint8_t *out_buffer;
int out_channer_nb;
int audio_stream_idx = -1;

int getAudioInfo(int *rate, int *channel, const char *path) {
    av_register_all();
    const char *input = path;
    pFormatCtx = avformat_alloc_context();
    LOGE("Lujng %s", input);
    LOGE("xxx %p", pFormatCtx);
    int error;
    char buf[] = "";
    //打开视频地址并获取里面的内容(解封装)
    if (error = avformat_open_input(&pFormatCtx, input, NULL, NULL) < 0) {
        av_strerror(error, buf, 1024);
        // LOGE("%s" ,inputPath)
        LOGE("Couldn't open file %s: %d(%s)", input, error, buf);
        // LOGE("%d",error)
        LOGE("打开视频失败");
    }
    //3.获取视频信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("%s", "获取视频信息失败");
        return -1;
    }


    int i = 0;
    for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            LOGE("  找到音频id %d", pFormatCtx->streams[i]->codec->codec_type);
            audio_stream_idx = i;
            break;
        }
    }
//  mp3的解码器
//  获取音频编解码器
    pCodecCtx = pFormatCtx->streams[audio_stream_idx]->codec;
    LOGE("获取视频编码器上下文 %p  ", pCodecCtx);

    pCodex = avcodec_find_decoder(pCodecCtx->codec_id);
    LOGE("获取视频编码 %p", pCodex);

    if (avcodec_open2(pCodecCtx, pCodex, NULL) < 0) {
    }
    packet = (AVPacket *) av_malloc(sizeof(AVPacket));
//    av_init_packet(packet);
//    音频数据

    frame = av_frame_alloc();
//    mp3  里面所包含的编码格式   转换成  pcm   SwcContext
    swrContext = swr_alloc();
    int length = 0;
    int got_frame;
//    44100*2
    out_buffer = (uint8_t *) av_malloc(44100 * 2);
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
//    输出采样位数  16位
    enum AVSampleFormat out_formart = AV_SAMPLE_FMT_S16;
//输出的采样率必须与输入相同
    int out_sample_rate = pCodecCtx->sample_rate;
    //音频转换
    swr_alloc_set_opts(swrContext, out_ch_layout, out_formart, out_sample_rate,
                       pCodecCtx->channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate, 0,
                       NULL);

    swr_init(swrContext);
//  获取通道数  2
    out_channer_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    *rate = pCodecCtx->sample_rate;
    *channel = pCodecCtx->channels;
    return 0;
}

int getPcm(void **buffer, size_t *bsize) {
    int frameCount = 0;
    int got_frame;
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        LOGI("getPcm 音频流id:%d",audio_stream_idx);
        if (packet->stream_index == audio_stream_idx) {
            LOGI("getPcm packet:%d",packet->size);
//            解码  mp3   编码格式frame----pcm   frame
            avcodec_decode_audio4(pCodecCtx, frame, &got_frame, packet);
            if (got_frame) {
                LOGE("getPcm 音频解码");
                /**
                 * int swr_convert(struct SwrContext *s, uint8_t **out, int out_count,
                                const uint8_t **in , int in_count);
                 */
                swr_convert(swrContext, &out_buffer, 44100 * 2, (const uint8_t **) frame->data,
                            frame->nb_samples);
//                缓冲区的大小
                int size = av_samples_get_buffer_size(NULL, out_channer_nb, frame->nb_samples,
                                                      AV_SAMPLE_FMT_S16, 1);
                *buffer = out_buffer;
                *bsize = size;
                LOGI("getPcm size:",size);
                break;
            }
        }
    }
    return 0;
}
