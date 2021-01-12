//
// Created by bei on 2021/1/10.
//
#include <jni.h>
#include <string>
#include "BeiPlayer.h"
#include "macro.h"

void *prepareFFmpeg_(void *args);

BeiPlayer::BeiPlayer(JavaCallHelper *_javaCallHelper, const char *dataSource) : javaCallHelper(
        _javaCallHelper) {
    url = new char[strlen(dataSource) + 1];
    strcpy(this->url, dataSource);
}

BeiPlayer::~BeiPlayer() {

}

/**
 * 准备操作，初始化比较耗时，所以我们引入线程来执行
 */
void BeiPlayer::prepare() {
    pthread_create(&pid_prepare, NULL, prepareFFmpeg_, this);
}

void *prepareFFmpeg_(void *args) {
    LOGI("prepareFFmpeg...");
    //this强制转换成PoeFFmeg对象.
    BeiPlayer *beiPlayerFFmpeg = static_cast<BeiPlayer *>(args);
    beiPlayerFFmpeg->prepareFFmpeg();
    //没有本行代码 程序异常退出
    return 0;
}
//子线程中的准备.
/***
 * 开启子线程准备.
 * 实例化VideoChannel。.
 */
void BeiPlayer::prepareFFmpeg() {
    //avformat 既可以解码本地文件 也可以解码直播文件，是一样的 .
    LOGI("prepareFFmpeg");
    avformat_network_init();
    LOGI("prepareFFmpeg network init success");
    //总上下文，用来解压视频为 视频流+音频流.
    formatContext = avformat_alloc_context();
    LOGI("prepareFFmpeg formatContext success");
    //参数配置
    AVDictionary *opts = NULL;
    //设置超时时间3s（3000ms->3000000mms)
    av_dict_set(&opts, "timeout", "3000000", 0);
    char buf[1024];
    //开始打开视频文件
    int ret = avformat_open_input(&formatContext, url, NULL, NULL);
    av_strerror(ret, buf, 1024);
    LOGI("prepareFFmpeg open_input success:%d", ret);
    if (ret != 0) {
        LOGE("* * * * * * video open failure! * * * * * * * * *n %d", ret);
        LOGE("Couldn’t open file %s: %d(%s)", url, ret, buf);
        //播放失败，通知java层播放失败了.
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
            return;
        }
    }
    //解析视频流 .放到frormatcontex里头.
    ret = avformat_find_stream_info(formatContext, NULL);
    LOGI("prepareFFmpeg find stream info success:%d", ret);
    if (ret != 0) {
        LOGE("* * * * * * video open failure! * * * * * * * * *n %d", ret);
        LOGE("Couldn’t open file %s: %d(%s)", url, ret, buf);
        //播放失败，通知java层播放失败了.
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }
    LOGI("prepareFFmpeg 遍历streams:%d", formatContext->nb_streams);
    for (int i = 0; i < formatContext->nb_streams; i++) {
        LOGI("streams size:%d", formatContext->nb_streams);
        AVStream *stream = formatContext->streams[i];
        //3.1 找解码参数
        AVCodecParameters *codecParameters = formatContext->streams[i]->codecpar;
        //3.2 找解码器
        AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
        if (!codec) {
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            }
            return;
        }

        //3.3 创建解码器上下文
        AVCodecContext *codecContext = avcodec_alloc_context3(codec);
        if (!codecContext) {
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            return;
        }
        //3.4 解码器参数附加到解码器上下文
        ret = avcodec_parameters_to_context(codecContext, codecParameters);
        if (ret < 0) {
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_CODEC_PARAMETERS_FAIL);
            }
            return;
        }

        //打开解码器.
        if (avcodec_open2(codecContext, codec, NULL) < 0) {
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FIAL);
            }
            return;
        }

        //初始化音频或视频的处理类
        if (AVMEDIA_TYPE_AUDIO == codecParameters->codec_type) {
            LOGI("读取到音频流:%d", i);
            //音频 交给audiochannel处理
            audioChannel = new AudioChannel(i, javaCallHelper, codecContext, stream->time_base,
                                            formatContext);
        } else if (AVMEDIA_TYPE_VIDEO == codecParameters->codec_type) {
            LOGI("读取到视频流:%d", i);
            //视频的帧率.
            AVRational av_frame_rate = stream->avg_frame_rate;

            int fps = av_q2d(av_frame_rate);

            //视频 交给videochannel处理
            videoChannel = new VideoChannel(i, javaCallHelper, codecContext, stream->time_base,
                                            formatContext);
            videoChannel->setRenderFrame(renderFrame);
            videoChannel->setFps(fps);

        }
    }

    //音视频都没有则抛出错误。没有满足规则的流.
    if (!audioChannel && !videoChannel) {
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_NO_MEDIA);
        }
        return;
    }

    //获取音频对象，音视频同步用. 将audiochannel交给videochannel
    videoChannel->audioChannel = audioChannel;
    //回调，准备工作完成
    if (javaCallHelper) {
        javaCallHelper->onPrepare(THREAD_CHILD);
    }
}
void* startThread(void *args) {
    LOGI("args address");
    BeiPlayer *beiFFmpeg = static_cast<BeiPlayer *>(args);
    beiFFmpeg->play();//开始从mp4中读取packet
    return 0;
}

/**
 * 打开播放标志，开始解码.
 */
void BeiPlayer::start() {
    LOGI("start");
    //播放成功.
    isPlaying = true;
    //开启解码.
    //音频解码
    if (audioChannel) {
        audioChannel->play();
    }
    LOGI("音频开始播放");
    // 视频解码.
    if (videoChannel) {
        //开启视频解码线程. 读取packet-》frame->synchronized->window_buffer.
        videoChannel->play();
    }
    LOGI("视频开始播放");
    //视频播放的时候开启一个解码线程.j解码packet.
    pthread_create(&pid_play, NULL, startThread, this);
}

/**
 * 在子线程中执行播放解码.
 */
void BeiPlayer::play() {
    int ret = 0;
    while (isPlaying) {
        LOGI("队列为空指针:%d", audioChannel->pkt_queue.size());
        //如果队列数据大于100则延缓解码速度.
        if (audioChannel && audioChannel->pkt_queue.size() > 100) {
            //思想，生产者的速度远远大于消费者.  10ms.
            av_usleep(1000 * 10);
            continue;
        }
        //如果队列数据大于100则延缓解码速度.
        if (videoChannel && videoChannel->pkt_queue.size() > 100) {
            //思想，生产者的速度远远大于消费者.  10ms.
            av_usleep(1000 * 10);
            continue;
        }

        //读取包
        AVPacket *packet = av_packet_alloc();
        //从媒体中读取音视频的packet包.
        ret = av_read_frame(formatContext, packet);
        if (ret == 0) {
            //将数据包加入队列.
            if (audioChannel && packet->stream_index == audioChannel->channelId) {
                LOGE("audioChannel->pkt_queue.enQueue(packet):%d", audioChannel->pkt_queue.size());
                audioChannel->pkt_queue.enQueue(packet);
            } else if (videoChannel && packet->stream_index == videoChannel->channelId) {
                videoChannel->pkt_queue.enQueue(packet);
                LOGE("videoChannel->pkt_queue.enQueue(packet):%d", videoChannel->pkt_queue.size());
            }
        } else if (ret == AVERROR_EOF) {
            //读取完毕，但是不一定播放完毕
            if (videoChannel->pkt_queue.empty() && videoChannel->frame_queue.empty()
                && audioChannel->pkt_queue.empty() && audioChannel->frame_queue.empty()) {
                LOGE("播放完毕");
                break;
            }
            //因为存在seek的原因，就算读取完毕，依然要循环 去执行av_read_frame(否则seek了没用...)
        } else {
            break;
        }
    }

    isPlaying = false;

    if (audioChannel) {
        audioChannel->stop();
    }
    if (videoChannel) {
        videoChannel->stop();
    }
}

void BeiPlayer::setRenderCallBack(RenderFrame renderFrame) {
    this->renderFrame = renderFrame;
}

void BeiPlayer::pause() {
    //先关闭播放状态.
    isPlaying = false;
}

void BeiPlayer::stop() {
    LOGI("stop beiplayer");
    //先关闭播放状态.
    isPlaying = false;
    //1. 停止prepare线程.

//    pthread_join(pid_prepare, NULL);
    LOGI("pid_prepare线程停止");

//    if(audioChannel){
//        audioChannel->stop();
//    }
//    if(videoChannel){
//        videoChannel->stop();
//    }
    //2. 停止play线程
//    pthread_join(pid_play, NULL);
    LOGI("pid_play线程停止");
}

//seek the frame to dest .
void BeiPlayer::seek(long ms) {
    //优先seek audio,如果没有audio则seek视频.
    if (audioChannel) {
        audioChannel->seek(ms);
    } else if (videoChannel) {
        videoChannel->seek(ms);
    }
}