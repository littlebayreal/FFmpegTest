//
// Created by bei on 2021/1/10.
//
#include "VideoChannel.h"
/**
 * 丢AVPacket ，丢非I帧.
 * @param q
 */
void dropPacket(queue<AVPacket *> &q){
    LOGE("丢弃视频Packet.....");
    while (!q.empty()){
        AVPacket* pkt = q.front();
        if(pkt->flags != AV_PKT_FLAG_KEY){
            q.pop();
            BaseChannel::releaseAvPacket(pkt);
        }else{
            break;
        }
    }
}

/**
 * 丢掉frame帧. 清空frame队列.
 * @param q
 */
void dropFrame(queue<AVFrame *> &q){
    LOGE("丢弃视频Frame.....");
    while (!q.empty()){
        AVFrame* frame = q.front();
        q.pop();
        BaseChannel::releaseAvFrame(frame);
    }
}

VideoChannel::VideoChannel(int id, JavaCallHelper *javaCallHelper, AVCodecContext *avCodecContext,AVRational time_base,AVFormatContext* formatContext,
                           pthread_mutex_t _seekMutex,pthread_mutex_t _mutex_pause,
                           pthread_cond_t _cond_pause)
        : BaseChannel(id, javaCallHelper, avCodecContext,time_base,_seekMutex,_mutex_pause,_cond_pause)
{
    this->javaCallHelper = javaCallHelper;
    this->avCodecContext = avCodecContext;
    this->avFormatContext = formatContext;
    pkt_queue.setReleaseCallback(releaseAvPacket);
    pkt_queue.setSyncHandle(dropPacket);
    frame_queue.setReleaseCallback(releaseAvFrame);
    frame_queue.setSyncHandle(dropFrame);
}



/**
 * 解码线程.
 * @param args
 * @return
 */
void * decode(void* args){
    VideoChannel* videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->decodePacket();
    return 0;
}


/**
 * 播放线程.
 * @param args
 * @return
 */
void* render_task(void* args){
    VideoChannel* videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->render();
    LOGI("视频播放线程正常退出");
    return 0;
}

/**
 * 开启视频解码packet线程 + 视频frame解码渲染线程.
 */
void VideoChannel::play() {
    pkt_queue.setWork(1);
    frame_queue.setWork(1);
    isPlaying = true ;
    //创建一个线程 。
    //解码线程packet->frame.
    pthread_create(&pid_video_play, NULL ,decode , this);
    //播放线程 frame->yuv.
    pthread_create(&pid_synchronize, NULL, render_task,this);
}
void VideoChannel::pause() {
    return;
}
void VideoChannel::resume() {
    return;
}
void VideoChannel::stop() {
    //1. set the playing flag false.
    isPlaying = false;
    //2. release thread deque packet thread .
    pthread_join(pid_video_play,NULL);
    LOGI("pid_video_play destroy");
    //3. release the synchronize thread for frame transform and render .
    pthread_join(pid_synchronize,NULL);
    LOGI("pid_synchronize destroy");
    //4. clear the queue .
    pkt_queue.clear();
    frame_queue.clear();
}

/**
 * 解码出packet队列数据 .
 */
void VideoChannel::decodePacket() {
    AVPacket* packet = 0;
    while (isPlaying){
        //流 --packet --音频 可以 单一  。
        int ret  = pkt_queue.deQueue(packet);
        if(!isPlaying) break;
        if(!ret){
            continue;
        }
//        LOGE("pkt_queue get packet susuccess :%d",ret);
        //解压frame.
//        LOGE("avcodec_send_packet start !");

        if(!avCodecContext){
            LOGE("avCodecContext is NULL!");
        }
        ret = avcodec_send_packet(avCodecContext, packet);
//        LOGE("avcodec_send_packet finished :%d",ret);
//        releaseAvPacket(packet);//释放packet.
        if(ret == AVERROR(EAGAIN)){
            LOGE("avcodec_send_packet EAGAIN 等待数据包！");
            //需要更多数据
            continue;
        }else if(ret < 0 ){
            LOGE("avcodec_send_packet FAilure ret < 0 %d",ret);
            //失败
            break;
        }
        AVFrame* avFrame = av_frame_alloc();
        ret = avcodec_receive_frame(avCodecContext, avFrame);
        //延缓队列缓存的速度，大于100帧等待10ms。
        while(isPlaying && frame_queue.size() > 100){
            av_usleep(1000*10);
            LOGE("frame queue is full！frame_queue size: %d",frame_queue.size());
            continue;
        }
        //压缩数据要 解压 yuv->rgb888
        //放入缓存队列.
        frame_queue.enQueue(avFrame);
    }

    //保险起见
    releaseAvPacket(packet);
}

void VideoChannel::render() {

    //从视频流中读取数据包 .
    SwsContext* swsContext = sws_getContext(
            avCodecContext->width,avCodecContext->height,
            avCodecContext->pix_fmt ,
            avCodecContext->width,avCodecContext->height,
            AV_PIX_FMT_RGBA,SWS_BILINEAR,
            0,0,0
    );

    //rgba接收的容器
    uint8_t * dst_data[4];//argb .
    //每一行的首地址
    int dst_linesize[4];

    av_image_alloc(dst_data, dst_linesize , avCodecContext->width ,avCodecContext->height,
                   AV_PIX_FMT_RGBA,1);
    //绘制界面 .
    //转化：YUV->RGB.
    AVFrame* frame = 0;
    //每个画面显示的时间，也就是图片之间显示间隔，单位秒
    double frame_delay = 1.0/fps;
    while (isPlaying){
        int ret = frame_queue.deQueue(frame);
        if(!isPlaying){
            break;
        }
        if(!ret){
            continue;
        }

        //linesize:每一行存放的数据的字节数
        sws_scale(swsContext,frame->data,frame->linesize,
                  0,avCodecContext->height,
                  dst_data,dst_linesize);
        //记录这一帧视频画面播放相对时间
        clock = frame->best_effort_timestamp * av_q2d(time_base);
        /**
         * 计算额外需要延迟播放的时间
         * When decoding, this signals how much the picture must be delayed.
         * extra_delay = repeat_pict / (2*fps)
         * int repeat_pict;
        */
        double extra_delay = frame->repeat_pict / (2*fps);
        //真实需要的时间间隔
        double delay = frame_delay + extra_delay;
        if (!audioChannel){//没有音频
            av_usleep(delay * 1000000);
        } else{//有音频
            if (clock == 0){
                av_usleep(delay * 1000000);
            } else{
                //比较音频与视频相对时间差：慢就追快就歇一会
                double diff = clock - audioChannel->clock;//视频减音频
                if(diff > 0){//视频快
                    LOGE("视频快了：%lf",diff);
                    if (diff >1){
                        av_usleep((delay *2 ) * 1000000);//差的比较大，慢慢赶
                    } else{
                        av_usleep((delay+diff) * 1000000);//差不多，多睡一会
                    }
                } else{//音频快
                    LOGE("音频快了：%lf",diff);
                    if(fabs(diff) >= 0.05){//差距比较大，考虑视频丢包
                        releaseAvFrame(frame);
                        frame_queue.sync();
                        continue;
                    } else{//差距没那么大，视频不用丢包，播放不延时就可以了

                    }
                }
            }
        }
        //没有音频才通过视频进度回调，有则以音频为主
        if (javaCallHelper && !audioChannel){
            javaCallHelper->onProgress(THREAD_CHILD,clock);
        }
        renderFrame(dst_data[0],dst_linesize[0],avCodecContext->width,avCodecContext->height);
        releaseAvFrame(frame);
    }
    av_freep(&dst_data[0]);
    isPlaying = 0;
    releaseAvFrame(frame);
    sws_freeContext(swsContext);
    swsContext = 0;
}

void VideoChannel::setRenderFrame(RenderFrame renderFrame) {
    this->renderFrame = renderFrame;
}

void VideoChannel::setFps(int fps) {
    this->fps = fps;
}
void VideoChannel::seek(long ms) {
    LOGE("VideoChannel::seek has not implemeted!");
}
