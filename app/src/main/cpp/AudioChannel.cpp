//
// Created by bei on 2021/1/10.
//
#include "AudioChannel.h"

void *audioPlay(void *args);

void *audioDecode(void *args);

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);

AudioChannel::AudioChannel(int id, JavaCallHelper *javaCallHelper, AVCodecContext *avCodecContext,
                           AVRational time_base, AVFormatContext *formatContext,
                           pthread_mutex_t _seekMutex, pthread_mutex_t _mutex_pause,
                           pthread_cond_t _cond_pause)
        : BaseChannel(id, javaCallHelper, avCodecContext, time_base, _seekMutex, _mutex_pause,
                      _cond_pause) {
    LOGI("AudioChannel构造函数");
    this->javaCallHelper = javaCallHelper;
    this->avCodecContext = avCodecContext;
    this->avFormatContext = formatContext;
    //初始化音频相关参数
    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    out_samplesize = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    out_sample_rate = 44100;
    //CD音频标准
    //44100 双声道 2字节
    buffer = (uint8_t *) (malloc(out_sample_rate * out_samplesize * out_channels));

//    LOGI("队列为空指针:%d",pkt_queue.size());
    //设置清空回调函数释放对象的回调.
    pkt_queue.setReleaseCallback(releaseAvPacket);
    frame_queue.setReleaseCallback(releaseAvFrame);
    //设置同步处理函数的回调
//    pkt_queue.setSyncHandle(syncHandle);
//    frame_queue.setSyncHandle(syncFrameHandle);
}

AudioChannel::~AudioChannel() {
    if (buffer) {//释放内存
        free(buffer);
        buffer = 0;
    }
}

void AudioChannel::play() {
//    初始化转换器上下文,设置重采样 .
    swrContext = swr_alloc_set_opts(0, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, out_sample_rate,
                                    avCodecContext->channel_layout,
                                    avCodecContext->sample_fmt,
                                    avCodecContext->sample_rate, 0, 0);
    //初始化转换器的其他参数.
    swr_init(swrContext);
    pkt_queue.setWork(1);
    frame_queue.setWork(1);
    isPlaying = true;
    //创建初始化OPENSL_ES的线程
    LOGI("音频创建解码和播放线程");
    //创建音频解码线程
    pthread_create(&pid_audio_play, NULL, audioPlay, this);
    //播放线程 frame->yuv.
    pthread_create(&pid_audio_decode, NULL, audioDecode, this);
}

void AudioChannel::pause() {
    (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_PAUSED);
}

void AudioChannel::resume() {
    (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_PLAYING);
}

void AudioChannel::stop() {
    LOGE("AudioChannel::stop()");
    isPlaying = false;
    pkt_queue.setWork(0);
    frame_queue.setWork(0);
    pthread_join(pid_audio_decode, 0);
    pthread_join(pid_audio_play, 0);

    //设置停止状态
    if (bqPlayerInterface) {
        (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_STOPPED);
        bqPlayerInterface = 0;
    }

    //释放播放器
    if (bqPlayerObject) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = 0;
        bqPlayerBufferQueue = 0;
    }

    //释放混音器
    if (outputMixObject) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = 0;
    }

    //释放引擎
    if (engineObject) {
        (*engineObject)->Destroy(engineObject);
        engineObject = 0;
        engineInterface = 0;
    }

    if (swrContext) {
        swr_free(&swrContext);
        swrContext = 0;
    }
}

//线程中执行播放方法
void *audioPlay(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *> (args);
    audioChannel->initOpenSL();
    return 0;
}

void *audioDecode(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->decoder();
    return 0;
}

/**
 * 初始化音频解码.
 */
void AudioChannel::initOpenSL() {
    //OpenSL ES播放音频套路
    //1：创建引擎并获取引擎接口
    SLresult result;
    // 1.1 创建引擎 SLObjectItf engineObject
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 1.2 初始化引擎  init
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 1.3 获取引擎接口SLEngineItf engineInterface
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE,
                                           &engineInterface);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    //2：设置混音器
    //2.1 创建混音器SLObjectItf outputMixObject
    result = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObject, 0,
                                                 0, 0);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 2.2 初始化混音器outputMixObject
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }

    //3：创建播放器:
    //3.1 配置输入声音信息
    //创建buffer缓冲类型的队列 2个队列
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};
    //设置播放器播放数据的信息
    //pcm+2(双声道)+44100(采样率)+ 16(采样位)+16(容器的大小)+LEFT|RIGHT(双声道)+小端数据
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1, SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};

    //数据源 将上述配置信息放到这个数据源中
    SLDataSource slDataSource = {&android_queue, &pcm};

    //3.2  配置音轨(输出)
    //设置混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSink = {&outputMix, NULL};
    //需要的接口  操作队列的接口
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    //3.3 创建播放器
    (*engineInterface)->CreateAudioPlayer(engineInterface, &bqPlayerObject, &slDataSource,
                                          &audioSink, 1,
                                          ids, req);
    //初始化播放器
    (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);

    //得到接口后调用  获取Player接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerInterface);

    /**
     * 4、设置播放回调函数
     */
    //获取播放器队列接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                    &bqPlayerBufferQueue);
    //设置回调
    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue,
                                                      bqPlayerCallback, this);
    /**
     * 5、设置播放状态
     */
    (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_PLAYING);
    /**
     * 6、手动激活一下这个回调
     */
    bqPlayerCallback(bqPlayerBufferQueue, this);
}

void AudioChannel::decoder() {
    LOGE("AudioChannel::decoder()  isPlaying: %d", isPlaying);
    AVPacket *packet = 0;
    while (isPlaying) {
        //音频的pakcket.
        LOGE("AudioChannel::decoder() #dequeue start !");
        int ret = pkt_queue.deQueue(packet);
        LOGE("AudioChannel::decoder() #dequeue success# !%s", packet);
        if (!isPlaying) {
            LOGE("AudioChannel::decoder() break");
            break;
        }
        if (!ret) {
            LOGE("AudioChannel::decoder() continue");
            continue;
        }
        //与seek逻辑清空codecContext解码器中缓存数据同步,否则多线程操作codecContext会有同步问题
        pthread_mutex_lock(&seekMutex);
        LOGE("AudioChannel::decoder() avcodec_send_packet start ! codecContext:%s", avCodecContext);
        //packet送去解码
        ret = avcodec_send_packet(avCodecContext, packet);

        releaseAvPacket(packet);
        if (ret == AVERROR(EAGAIN)) {
            LOGE("AudioChannel::decoder() avcodec_send_packet EAGAIN 等待数据包！");
            //需要更多数据
            LOGE("AudioChannel::decoder() 需要更多数据");
            continue;
        } else if (ret < 0) {
            LOGE("AudioChannel::decoder() avcodec_send_packet FAilure ret < 0 %d", ret);
            //失败
            break;
        }

        AVFrame *avFrame = av_frame_alloc();
        ret = avcodec_receive_frame(avCodecContext, avFrame);
        LOGE("AudioChannel::decoder() avcodec_receive_frame success ! avFrame:%s", avFrame);
        pthread_mutex_unlock(&seekMutex);
        if (ret == AVERROR(EAGAIN)) {
            //需要更多数据
            continue;
        } else if (ret < 0) {
            LOGE("AudioChannel::decoder() avcodec_receive_frame FAilure ret < 0 %d", ret);
            //失败
            break;
        }
        //packet -》frame.
        frame_queue.enQueue(avFrame);
        LOGE("AudioChannel::decoder() frame_queue enQueue success ! :%d", frame_queue.size());
        while (frame_queue.size() > 100 && isPlaying) {
            LOGE("AudioChannel::decoder() frame_queue %d is full, sleep 16 ms", frame_queue.size());
            av_usleep(16 * 1000);
            continue;
        }
    }
    releaseAvPacket(packet);
}

// this callback handler is called every time a buffer finishes playing
// 由opensl驱动不断发生的播放事件
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *_audioChannel) {

    LOGE("bqPlayerCallback()#invoked!");
    AudioChannel *audioChannel = static_cast<AudioChannel *>(_audioChannel);
    int datalen = audioChannel->getPcm();
    LOGE("bqPlayerCallback()# datalen:%d", datalen);
    if (datalen > 0) {
        (*bq)->Enqueue(bq, audioChannel->buffer, datalen);
    }
}

/**
 *  获取音频解码的pcm .
 * @return
 */
int AudioChannel::getPcm() {
//    LOGE("AudioChannel::getPcm()  %d",frame_queue.size());
    AVFrame *frame = 0;
    int data_size = 0;
//    while (isPlaying){
    int ret = frame_queue.deQueue(frame);
    //转换.
    if (!isPlaying) {
        if (ret) {
            releaseAvFrame(frame);
        }
        return data_size;
    }
    if (!ret) {
        return data_size;
    }
    //重采样
    //函数swr_get_delay得到输入sample和输出sample之间的延迟，
    // 并且其返回值的根据传入的第二个参数不同而不同。如果是输入的采样率，
    // 则返回值是输入sample个数；如果输入的是输出采样率，则返回值是输出sample个数。
    int64_t delay = swr_get_delay(swrContext, frame->sample_rate);
    //计算转换后的sample个数
    //转后后的sample个数的计算公式为：src_nb_samples * dst_sample_rate / src_sample_rate
    //delay+frame->nb_samples:这里写成这样是为了能实时处理。想想一下这个矛盾，这些音频数据，
    // 如果处理转换的时间大于产生的时间，那么就会造成生产的新数据堆积。
    // 写成这样，就计算了一个大的buffer size，用于转换。这样就不会产生数据堆积了。
    int64_t dstSamples = av_rescale_rnd(delay + frame->nb_samples, out_sample_rate,
                                        frame->sample_rate, AV_ROUND_UP);
    //返回每个声道采样的个数:dstSamples表示转换后缓冲区大小
    int samples = swr_convert(swrContext, &buffer, dstSamples,
                              (const uint8_t **) frame->data, frame->nb_samples);
    if (samples < 0) {
        return data_size;
    }

    //算成双声道字节数：每个采样16位表示（2字节）
    data_size = samples * out_channels * out_samplesize;
    //记录这一帧音频相对时间
    clock = frame->best_effort_timestamp * av_q2d(time_base);
    if (javaCallHelper) {
        javaCallHelper->onProgress(THREAD_CHILD, clock);
    }
    releaseAvFrame(frame);
    return data_size;
//        //frame -> 转化为pcm数据.
//        uint64_t  dst_nb_samples = av_rescale_rnd(
//                swr_get_delay(swrContext, frame->sample_rate) + frame->nb_samples,
//                out_sample_rate,
//                frame->sample_rate,
//                AV_ROUND_UP
//        );
//        //转换 返回值为转换后的sample个数.
//        int nb = swr_convert(swrContext, &buffer, dst_nb_samples,
//                             (const uint8_t**)frame->data, frame->nb_samples);
//
//        //计算转换后buffer的大小 44100*2（采样位数2个字节）*2（双通道）.  。
//        data_size = nb * out_channels*out_samplesize;
//
//        //计算当前音频的播放时钟clock. pts相对数量 time_base:时间单位（1,25）表示1/25分之一秒.
//        clock = frame->pts*av_q2d(time_base);

//        break;
//    }
//    releaseAvFrame(frame);
//    return data_size;
}

//seek frame ..
void AudioChannel::seek() {
    pkt_queue.setWork(0);
    frame_queue.setWork(0);
    pkt_queue.clear();
    frame_queue.clear();
    pkt_queue.setWork(1);
    frame_queue.setWork(1);
}


