#ifndef  __AVMUXER_HPP__
#define  __AVMUXER_HPP__

#include "CoreMinimal.h"
#include "UMuxer.h"
#ifdef __cplusplus
extern "C"{
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavcodec/bsf.h>

#ifdef __cplusplus
}
#endif

#include <atomic>
#include <mutex>
#include "avutil/Muxer.hpp"
#include "cvutil/RefThread.hpp"
#include "CodecOptions.hpp"

#include "OpenCVHelper.h"
#include "PreOpenCVHeaders.h"
#include "opencv2/core.hpp"
#include "PostOpenCVHeaders.h"

class UMUXER_API AVMuxer :
    public MediaMuxer {
public:
    enum DevType {
        V_CPU,
        V_GPU
    };

    AVMuxer(const MediaParams& params);
    virtual ~AVMuxer();

    virtual int open(const std::string& url);

    virtual int start();

    virtual int stop(bool wait_end = false);

    virtual int check_error(std::string& error_msg);

    int get_video_frame();

protected:
    enum VideoProtoType {
        FILE = 0,
        RTMP,
        RTSP,
        UDP,
        TCP
    };

    struct  AudioParam
    {
        int channels;
        int sample_rate;
        enum AVSampleFormat sample_fmt;
        uint64_t channel_layout;
        int bits_per_sample;
    };

    struct VideoParam
    {
        int fps;
        int height;
        int width;
        enum AVPixelFormat pixel_fmt;
    };

    struct AudioStream
    {
        AVStream* st;
        AVCodecContext* codec_ctx;
        AVCodec* codec;
        int samples;
        int64 next_pts;
        int64 real_pts;

        AVFrame* frame;
        AVFrame* tmp_frame;
        struct SwrContext* swr_ctx;
        AudioParam audio_param;
        uint8_t* outputChannels[2] = {nullptr,nullptr};
    };

    struct VideoStream
    {
        AVStream* st;
        AVCodecContext* codec_ctx;
        AVCodec* codec;

        int64 next_pts;

        AVFrame* frame;
        AVFrame* tmp_frame;

        struct SwsContext* sws_ctx;
        VideoParam video_param;
        AVBSFContext* bsf_ctx;
    };

    class AudioEncoderThread : public RefThread{
        public:
            AudioEncoderThread(AVMuxer* muxer) : pmuxer_(muxer){}
            ~AudioEncoderThread(){}
        protected:
            virtual void thread_loop();
        private:
            AVMuxer* pmuxer_;
    };

    class VideoEncoderThread : public RefThread {
    public:
        VideoEncoderThread(AVMuxer* muxer) : pmuxer_(muxer) {}
        ~VideoEncoderThread() {}
    protected:
        virtual void thread_loop();
    private:
        AVMuxer* pmuxer_;
    };

private:
    const char* parse_video_protocal(const std::string& video_url);
    int add_stream(VideoStream* ost,enum AVCodecID codec_id);
    int add_stream(AudioStream* ost,enum AVCodecID codec_id);

    int open_video_codec(VideoStream* vost);
    int open_audio_codec(AudioStream* aost);

    AVFrame* alloc_picture(enum AVPixelFormat pix_fmt,int width,int height);

    AVFrame* alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                uint64_t channel_layout,
                                int sample_rate,int nb_samples);
    
    void close_audio();
    void close_video();

    void reset();

    bool init_params();

    unsigned nextpowerof2(unsigned val);

    int write_packet(AVFormatContext *fmt_ctx,const AVRational *time_base,AVStream *st,AVPacket *pkt);

    void cleanup();

    void show_error(const char* prefix,int err);

    void set_error(int err);
    AVPixelFormat get_ffpixel_format(ImagePixelFormat in_fmt);

private:
    VideoProtoType protocal_type_;
    AVFormatContext* fmt_ctx_;
    bool audio_enable_;
    bool video_enable_;
    int bitrate_;
    int frame_num;
    //input params
    AudioParam audio_in_param_;
    VideoParam video_in_param_;

    //output stream
    AudioStream audio_st_;
    VideoStream video_st_;

    std::unique_ptr<AudioEncoderThread> audio_enc_;
    std::unique_ptr<VideoEncoderThread> video_enc_;

    std::mutex write_mtx_;
    std::mutex read_mtx_;

    std::atomic<bool> audio_eof_;
    std::atomic<bool> video_eof_;

    bool is_header_writed_;
};
#if PLATFORM_WINDOWS
#ifdef WINDOWS_PLATFORM_TYPES_GUARD
    #include "Windows/HideWindowsPlatformTypes.h"
#endif
#endif
#endif
