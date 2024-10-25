// Fill out your copyright notice in the Description page of Project Settings.


#include "FFmpeg/FFmpegPacketQueue.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/fifo.h"
}
typedef struct MyAVPacketList {//sub关联的内存释放
    AVPacket* pkt;
    int serial;
} MyAVPacketList;

FFmpegPacketQueue::FFmpegPacketQueue()
{
    pkt_list = av_fifo_alloc(sizeof(MyAVPacketList));
    nb_packets = 0;
    size = 0;
    duration = 0;
    abort_request = 0;
    serial = 0;
}

FFmpegPacketQueue::~FFmpegPacketQueue()
{
}

int FFmpegPacketQueue::PutPrivate(AVPacket* pkt)
{
    MyAVPacketList pkt1;

    //如果处于中止状态，则直接返回
    if (this->abort_request)
        return -1;

    if (av_fifo_space(this->pkt_list) < sizeof(pkt1)) {
        if (av_fifo_grow(this->pkt_list, sizeof(pkt1)) < 0)
            return -1;
    }

    pkt1.pkt = pkt;
    pkt1.serial = this->serial;

    av_fifo_generic_write(this->pkt_list, &pkt1, sizeof(pkt1), NULL);
    this->nb_packets++;
    this->size += pkt1.pkt->size + sizeof(pkt1);
    this->duration += pkt1.pkt->duration;
    /* XXX: should duplicate packet data in DV case */
    this->cond->signal();
    return 0;
}

int FFmpegPacketQueue::Put(AVPacket* pkt)
{
    AVPacket* pkt1;
    int ret;

    pkt1 = av_packet_alloc();
    if (!pkt1) {
        av_packet_unref(pkt);
        return -1;
    }
    av_packet_move_ref(pkt1, pkt);

    this->mutex->Lock();
    ret = this->PutPrivate(pkt1);
    this->mutex->Unlock();

    if (ret < 0)
        av_packet_free(&pkt1);

    return ret;
}

int FFmpegPacketQueue::PutNullpacket(AVPacket* pkt, int stream_index)
{
    pkt->stream_index = stream_index;
    return this->Put(pkt);
}

int FFmpegPacketQueue::Init()
{
    //memset(q, 0, sizeof(PacketQueue));
    this->pkt_list = av_fifo_alloc(sizeof(MyAVPacketList));
    if (!this->pkt_list)
        return AVERROR(ENOMEM);
    this->mutex = new FCriticalSection();
    if (!this->mutex) {
        av_log(NULL, AV_LOG_FATAL, "FFmpegPacketQueue CreateMutex fail\n");
        return AVERROR(ENOMEM);
    }
    this->cond = new FFmpegCond();
    if (!this->cond) {
        av_log(NULL, AV_LOG_FATAL, "FFmpegPacketQueue CreateCond() fail\n");
        return AVERROR(ENOMEM);
    }
    this->abort_request = 1;
    return 0;
}

void FFmpegPacketQueue::Flush()
{
    MyAVPacketList pkt1;

    this->mutex->Lock();
    while (this->pkt_list && av_fifo_size(this->pkt_list) >= sizeof(pkt1)) {
        av_fifo_generic_read(this->pkt_list, &pkt1, sizeof(pkt1), NULL);
        av_packet_free(&pkt1.pkt);
    }
    this->nb_packets = 0;
    this->size = 0;
    this->duration = 0;
    this->serial++;
    this->mutex->Unlock();
}

void FFmpegPacketQueue::Destroy()
{
    this->Flush();
    av_fifo_freep(&this->pkt_list);
    //SDL_DestroyMutex(q->mutex);
    //SDL_DestroyCond(q->cond);
}

void FFmpegPacketQueue::Abort()
{
    this->mutex->Lock();
    this->abort_request = 1; //将中止状态设置为1

    this->cond->signal();
    this->mutex->Unlock();
}

void FFmpegPacketQueue::Start()
{
    this->mutex->Lock();
    this->abort_request = 0; //将中止状态设置为0
    this->serial++;
    this->mutex->Unlock();
}

int FFmpegPacketQueue::Get(AVPacket* pkt, int block, int* serial_)
{
    MyAVPacketList pkt1;
    int ret;

    this->mutex->Lock();

    for (;;) {
        if (this->abort_request) {
            ret = -1;
            break;
        }

        if (av_fifo_size(this->pkt_list) >= sizeof(pkt1)) {
            av_fifo_generic_read(this->pkt_list, &pkt1, sizeof(pkt1), NULL);
            this->nb_packets--;
            this->size -= pkt1.pkt->size + sizeof(pkt1);
            this->duration -= pkt1.pkt->duration;
            av_packet_move_ref(pkt, pkt1.pkt);
            if (serial_)
                *serial_ = pkt1.serial;
            av_packet_free(&pkt1.pkt);
            ret = 1;
            break;
        }
        else if (!block) {
            ret = 0;
            break;
        }
        else {
            this->cond->wait(*this->mutex);
        }
    }
    this->mutex->Unlock();
    return ret;
}

int FFmpegPacketQueue::GetAbortRequest()
{
    return this->abort_request;
}

int FFmpegPacketQueue::GetSerial()
{
    return this->serial;
}

int FFmpegPacketQueue::GetNbPackets()
{
    return this->nb_packets;
}


