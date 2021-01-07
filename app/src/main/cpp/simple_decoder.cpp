//
// Created by bei on 2020/12/22.
//
#include <stdio.h>
#include <time.h>
#include <iostream>
using namespace std;

#ifdef ANDROID  //如果是android编译器 使用android log 输出

#include <jni.h>
#include <android/log.h>

#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "(>_<)", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "(^_^)", format, ##__VA_ARGS__)
#else
#define LOGE(format, ...)  printf("(>_<) " format "\n", ##__VA_ARGS__)
#define LOGI(format, ...)  printf("(^_^) " format "\n", ##__VA_ARGS__)
#endif

//Output FFmpeg's av_log()
void custom_log(void *ptr, int level, const char *fmt, va_list vl) {
    FILE *fp = fopen("/storage/emulated/0/DCIM/av_log.txt", "a+");
    if (fp) {
        vfprintf(fp, fmt, vl);
        fflush(fp);
        fclose(fp);
    }
}

jbyteArray as_byte_array(JNIEnv *env, unsigned char* buf, int size) {
    jbyteArray array = env->NewByteArray(size);
    //HERE I GET THE ERROR, I HAVE BEEN TRYING WITH len/2 and WORKS , PROBABLY SOME BYTS ARE GETTING LOST.
    env->SetByteArrayRegion(array, 0, size, (jbyte*)(buf));
    return array;
}

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libavutil/log.h"

JavaVM *g_VM;
jobject g_obj;
JNIEXPORT jint JNICALL
Java_com_example_ffmpegtest_SimpleDecodeActivity_decode(JNIEnv *env, jobject thiz, jstring inputurl,
                                                        jstring outputurl) {
    LOGE("simple decoder 开始解码");
    AVFormatContext *pFormatCtx;
    int i, videoindex;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVPacket *packet;
    AVFrame *pFrame, *pFrameYUV;
    uint8_t *out_buffer;
    int y_size;
    int ret, got_picture;
    struct SwsContext *img_convert_ctx;

    FILE *fp_yuv, *fp_txt;
    int frame_cnt;
    clock_t time_start, time_finish;//long型
    double time_duration = 0.0;

    char input_str[500] = {0};
    char output_str[500] = {0};
    char info[1000] = {0};
    sprintf(input_str, "%s", env->GetStringUTFChars(inputurl, NULL));
    sprintf(output_str, "%s", env->GetStringUTFChars(outputurl, NULL));
    //FFmpeg av_log() callback
    //avutil中设置日志的记录
    av_log_set_callback(custom_log);

    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();
    //打开文件流
    if (avformat_open_input(&pFormatCtx, input_str, NULL, NULL) != 0) {
        LOGE("simple decoder Couldn't open input stream.%s,%s", input_str, "\n");
        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("simple decoder Couldn't find stream information.\n");
        return -2;
    }
    videoindex = -1;
    //找到类型为视频的通道
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
            break;
        }
    }
    if (videoindex == -1) {
        LOGE("simple decoder Couldn't find a video stream.\n");
        return -1;
    }
    //通过视频轨道中的codec设置解码器类型
    pCodecCtx = pFormatCtx->streams[videoindex]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        LOGE("simple decoder Couldn't find Codec.\n");
        return -3;
    }
    //打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGE("simple decoder Couldn't open codec.\n");
        return -4;
    }
    pFrame = av_frame_alloc();
    pFrameYUV = av_frame_alloc();
    out_buffer = (unsigned char *) av_malloc(
            av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));

    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
                         AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
    //初始化packet
    packet = (AVPacket *) av_malloc(sizeof(AVPacket));

    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                     pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
                                     SWS_BICUBIC, NULL, NULL, NULL);

    sprintf(info, "[Input     ]%s\n", input_str);
    sprintf(info, "%s[Output    ]%s\n", info, output_str);
    sprintf(info, "%s[Format    ]%s\n", info, pFormatCtx->iformat->name);
    sprintf(info, "%s[Codec     ]%s\n", info, pCodecCtx->codec->name);
    sprintf(info, "%s[Resolution]%dx%d\n", info, pCodecCtx->width, pCodecCtx->height);

    //打开准备写入的文件
    fp_yuv = fopen(output_str, "wb+");
    if (fp_yuv == nullptr) {
        LOGE("simple decoder Cannot open output file.\n");
//        return -5;
    }

//    fp_txt = fopen("/storage/emulated/0/DCIM/decode_log.txt", "w+");
//    if (fp_txt == nullptr) {
//        LOGE("simple decoder Cannot open txt file.\n");
//        return -6;
//    }
    frame_cnt = 0;
    time_start = clock();
    //开始解包
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == videoindex) {
            //解码
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if (ret < 0) {
                LOGE("simple decoder Decode Error.\n");
                return -6;
            }
            if (got_picture) {
                sws_scale(img_convert_ctx, (const uint8_t **) pFrame->data, pFrame->linesize, 0,
                          pCodecCtx->height,
                          pFrameYUV->data, pFrameYUV->linesize);

                y_size = pCodecCtx->width * pCodecCtx->height;
                fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y
                fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
                fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
                //Output info

                char pictype_str[10] = {0};
                switch (pFrame->pict_type) {
                    case AV_PICTURE_TYPE_I:
                        sprintf(pictype_str, "I");
                        break;
                    case AV_PICTURE_TYPE_P:
                        sprintf(pictype_str, "P");
                        break;
                    case AV_PICTURE_TYPE_B:
                        sprintf(pictype_str, "B");
                        break;
                    default:
                        sprintf(pictype_str, "Other");
                        break;
                }
                LOGI("simple decoder Frame Index: %5d. Type:%s", frame_cnt, pictype_str);
                frame_cnt++;
            }
        }
        av_packet_unref(packet);
    }
    //flush decoder
    //FIX: Flush Frames remained in Codec
    while (1) {
        ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
        LOGI("simple decoder ret:%d", ret);
        if (ret < 0)
            break;
        if (!got_picture)
            break;
        sws_scale(img_convert_ctx, (const uint8_t **) pFrame->data, pFrame->linesize, 0,
                  pCodecCtx->height,
                  pFrameYUV->data, pFrameYUV->linesize);
        int y_size = pCodecCtx->width * pCodecCtx->height;
        fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y
        fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
        fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
        //Output info
        char pictype_str[10] = {0};
        switch (pFrame->pict_type) {
            case AV_PICTURE_TYPE_I:
                sprintf(pictype_str, "I");
                break;
            case AV_PICTURE_TYPE_P:
                sprintf(pictype_str, "P");
                break;
            case AV_PICTURE_TYPE_B:
                sprintf(pictype_str, "B");
                break;
            default:
                sprintf(pictype_str, "Other");
                break;
        }
        LOGI("simple decoder Frame Index: %5d. Type:%s", frame_cnt, pictype_str);
        frame_cnt++;
    }
    return 0;
}
JNIEXPORT jint JNICALL
Java_com_example_ffmpegtest_SimpleDecodeActivity_returnDecode(JNIEnv *env, jobject thiz,
                                                              jstring inputurl, jstring outputurl) {
    // TODO: implement returnDecode()
    jclass clazz = env->GetObjectClass(thiz);
    jmethodID callbackMethodID = env->GetMethodID(clazz, "onDecode", "([BLjava/lang/String;)V");
    if (callbackMethodID == NULL) {
        LOGE("doCallback getMethodId is failed \n");
    }

    LOGE("simple decoder 开始解码");
    AVFormatContext *pFormatCtx;
    int i, videoindex;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVPacket *packet;
    AVFrame *pFrame, *pFrameYUV;
    uint8_t *out_buffer;
    int y_size;
    int ret, got_picture;
    struct SwsContext *img_convert_ctx;

    FILE *fp_yuv;
    int frame_cnt;
    clock_t time_start, time_finish;//long型
    double time_duration = 0.0;

    char input_str[500] = {0};
    char output_str[500] = {0};
    char info[1000] = {0};
    sprintf(input_str, "%s", env->GetStringUTFChars(inputurl, NULL));
    sprintf(output_str, "%s", env->GetStringUTFChars(outputurl, NULL));
    //FFmpeg av_log() callback
    //avutil中设置日志的记录
    av_log_set_callback(custom_log);

    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();
    //打开文件流
    if (avformat_open_input(&pFormatCtx, input_str, NULL, NULL) != 0) {
        LOGE("simple decoder Couldn't open input stream.%s,%s", input_str, "\n");
        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("simple decoder Couldn't find stream information.\n");
        return -1;
    }
    videoindex = -1;
    //找到类型为视频的通道
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
            break;
        }
    }
    if (videoindex == -1) {
        LOGE("simple decoder Couldn't find a video stream.\n");
        return -1;
    }
    //通过视频轨道中的codec设置解码器类型
    pCodecCtx = pFormatCtx->streams[videoindex]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        LOGE("simple decoder Couldn't find Codec.\n");
        return -1;
    }
    //打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGE("simple decoder Couldn't open codec.\n");
        return -1;
    }
    pFrame = av_frame_alloc();
    pFrameYUV = av_frame_alloc();
    out_buffer = (unsigned char *) av_malloc(
            av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1));

    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
                         AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);
    //初始化packet
    packet = (AVPacket *) av_malloc(sizeof(AVPacket));

    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                     pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24,
                                     SWS_BICUBIC, NULL, NULL, NULL);

    sprintf(info, "[Input     ]%s\n", input_str);
    sprintf(info, "%s[Output    ]%s\n", info, output_str);
    sprintf(info, "%s[Format    ]%s\n", info, pFormatCtx->iformat->name);
    sprintf(info, "%s[Codec     ]%s\n", info, pCodecCtx->codec->name);
    sprintf(info, "%s[Resolution]%dx%d\n", info, pCodecCtx->width, pCodecCtx->height);
    LOGI("simple decoder:%s", info);
    //打开准写入的文件
    fp_yuv = fopen(output_str, "wb+");
    if (fp_yuv == NULL) {
        printf("Cannot open output file.\n");
        return -1;
    }
    frame_cnt = 0;
    time_start = clock();
    //开始解包
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == videoindex) {
            //解码
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if (ret < 0) {
                LOGE("Decode Error.\n");
                return -1;
            }
            if (got_picture) {
                sws_scale(img_convert_ctx, (const uint8_t *const *) pFrame->data, pFrame->linesize,
                          0,
                          pCodecCtx->height,
                          pFrameYUV->data, pFrameYUV->linesize);

                LOGI("simple decoder 图像大小:%d %d\n", pCodecCtx->width, pCodecCtx->height);
                y_size = pCodecCtx->width * pCodecCtx->height;
                fwrite(pFrameYUV->data[0], y_size * 3, 1, fp_yuv);
//                fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y
//                fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
//                fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V
                //Output info
                //设置回调 每解析一帧就返回一帧
//                char info_frame[100] = {0};
//                LOGI("simple decoder 本帧有多大:%d", y_size * 3);
//                for (int i = 0; i < 100; ++i) {
//                    sprintf(info_frame, "%s[%10s]\n", info_frame, (int *)pFrameYUV->data[0]);
//                }
//                LOGI("simple decoder 本帧解析结果:%c", info_frame);
//                jbyteArray jbarray = env->NewByteArray(y_size*3);//建立jbarray数组
//                env->SetByteArrayRegion(jbarray, 0, y_size*3, (jbyte*) pFrameYUV->data[0]);
                char pictype_str[10] = {0};
                switch (pFrame->pict_type) {
                    case AV_PICTURE_TYPE_I:
                        sprintf(pictype_str, "I");
                        break;
                    case AV_PICTURE_TYPE_P:
                        sprintf(pictype_str, "P");
                        break;
                    case AV_PICTURE_TYPE_B:
                        sprintf(pictype_str, "B");
                        break;
                    default:
                        sprintf(pictype_str, "Other");
                        break;
                }
                if(frame_cnt == 600) {
                    unsigned char *c = pFrameYUV->data[0];
                    jbyteArray j = as_byte_array(env, c, y_size * 3);
                    env->CallVoidMethod(thiz, callbackMethodID, j,
                                        env->NewStringUTF(pictype_str));
                }
                LOGI("Frame Index: %5d. Type:%s", frame_cnt, pictype_str);
                frame_cnt++;
            } else
                LOGE("simple decoder 解析失败:%d", got_picture);
        }
        av_packet_unref(packet);
    }
    //flush decoder
    //FIX: Flush Frames remained in Codec
    while (1) {
        ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
        LOGI("simple decoder ret:%d", ret);
        if (ret < 0)
            break;
        if (!got_picture)
            break;
        sws_scale(img_convert_ctx, (const uint8_t **) pFrame->data, pFrame->linesize, 0,
                  pCodecCtx->height,
                  pFrameYUV->data, pFrameYUV->linesize);
        int y_size = pCodecCtx->width * pCodecCtx->height;
        fwrite(pFrameYUV->data[0], y_size * 3, 1, fp_yuv);
//        fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y
//        fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
//        fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
        //设置回调 每解析一帧就返回一帧
//        jbyteArray jbarray = env->NewByteArray(y_size);//建立jbarray数组
//        env->SetByteArrayRegion(jbarray, 0, y_size, (jbyte *) pFrameYUV->data[0]);
        //Output info
        char pictype_str[10] = {0};
        switch (pFrame->pict_type) {
            case AV_PICTURE_TYPE_I:
                sprintf(pictype_str, "I");
                break;
            case AV_PICTURE_TYPE_P:
                sprintf(pictype_str, "P");
                break;
            case AV_PICTURE_TYPE_B:
                sprintf(pictype_str, "B");
                break;
            default:
                sprintf(pictype_str, "Other");
                break;
        }
//        env->CallVoidMethod(thiz, callbackMethodID, jbarray,
//                            env->NewStringUTF(pictype_str));
        LOGI("Frame Index: %5d. Type:%s", frame_cnt, pictype_str);
        frame_cnt++;
    }
    return 0;
}
//JNIEXPORT void JNICALL
//Java_com_example_ffmpegtest_SimpleDecodeActivity_setDecodeListener(JNIEnv *env, jobject thiz,jobject decode_listener) {
//    // TODO: implement setDecodeListener()
//

}