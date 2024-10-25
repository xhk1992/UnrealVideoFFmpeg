#ifndef __MUXER_H__
#define __MUXER_H__


#include "CoreMinimal.h"
#include "UMuxer.h"
#include <thread>
#include "cvutil/SyncQueue.hpp"
#include "cvutil/AutoBuffer.hpp"
#include "CodecOptions.hpp"

#include "OpenCVHelper.h"
#include "PreOpenCVHeaders.h"
#include "opencv2/core.hpp"
#include "PostOpenCVHeaders.h"


enum ImagePixelFormat{
    PIX_FMT_YUV420P,
    PIX_FMT_YUYV422,
    PIX_FMT_YUV444P,
    PIX_FMT_RGB24,
    PIX_FMT_BGR24,
    PIX_FMT_GRAY8,
    PIX_FMT_ARGB,
    PIX_FMT_RGBA,
    PIX_FMT_BGRA,
    PIX_FMT_YUVA420P,
    PIX_FMT_YUVA444P10LE,
};


enum AudioSampleFormat{
    SAMPLE_FMT_U8,
    SAMPLE_FMT_S16,
    SAMPLE_FMT_S32,
    SAMPLE_FMT_FLT,
};

struct MuxerParamInfo
{
    const char*    param_str;
    const char*    value_default;
};


struct MediaParams
{
    int device_type;
    int device_id;
    int frame_rate;
    int width;
    int height;
    ImagePixelFormat in_pix_fmt;
    int bitrate;
    std::string video_encoder_format;

    /* format like :"gopsize=12;profile=main;out_pixfmt=yuv420p;colorspace=bt709"
        gopsize:1~3000
        profile:
            GPU Encode:baseline,main,high,high444p;
            CPU Encode:baseline,main,extended,high,high422,high444,high444p,high444i,stereohigh 
    */

   std::string video_extra_params;

   AudioSampleFormat audio_sample_format;
   int audio_bits_per_sample;
   int audio_channels;
   int audio_sample_rate;

   MediaParams(){
       device_type = 1; //0 cpu 1 gpu
       device_id =0;
       frame_rate = 25;
       width = 1920;
       height = 1080;
       in_pix_fmt = PIX_FMT_BGR24;
       bitrate = 2000000;
       audio_sample_format = SAMPLE_FMT_S16;
       audio_bits_per_sample = 16;
       audio_channels = 1;
       audio_sample_rate = 16000;
       video_encoder_format = "h264" ;
   }
};


class UMUXER_API Muxer
{
public:
    Muxer(){};
    virtual ~Muxer() {};
    virtual int open(const std::string& url) = 0;
    virtual int start() = 0;
    virtual int feed_audio(char* data,size_t length) =0;
    virtual void feed_image(cv::Mat& image,bool voice_activity) =0;
    virtual int stop(bool wait_end = false) = 0;
    virtual int check_error(std::string& error_msg) = 0;
};

class UMUXER_API MediaMuxer : public Muxer
{
public:
    MediaMuxer(const MediaParams& params)
            : media_params_(params)
            , video_queue_(-1)
            , audio_queue_(-1){
            reset();
            }
    virtual ~MediaMuxer(){
        stop();
    }

    virtual int open(const std::string& url){
        const static int BUFFER_SIZE = 60;
        unsigned num_samples = nextpowerof2((unsigned)(media_params_.audio_sample_rate * BUFFER_SIZE * media_params_.audio_channels));
        unsigned num_bytes = num_samples * sizeof(short);
        //audio_buf_data_.allocate(num_samples);
        //PaUtil_InitializeRingBuffer(&audio_rbuf_,sizeof(short),num_samples,audio_buf_data_);

        if(!media_params_.video_extra_params.empty())
            str_to_attr_vals(media_params_.video_extra_params,extra_params_,";");
        CodecOptionsSelector::instance().parse_encode_params(extra_params_,enc_params_);
        return 0;
    }

    virtual int start() = 0;

    virtual int feed_audio(char* data, size_t length) {
        if (data != nullptr && length != 0)
        {
            EncodeAudioData ea(data);
            audio_queue_.write(ea);
        }
        return 0;
    }

    virtual void feed_image(cv::Mat& image,bool voice_activity){
        if(!image.empty()){
            EncodeImageData ea;
            ea.frame = image;
            ea.voice_activity = voice_activity;
            video_queue_.write(ea);
        }
    }

    virtual int stop(bool wait_end = false){
        
        //audio_queue_.force(true);
        audio_queue_.force(true);
        if (wait_end)
        {
            while (!video_queue_.empty())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        video_queue_.force(true);
        

		/*if(wait_end){
			ring_buffer_size_t elements_in_buffer = 0;
			do{
				elements_in_buffer = PaUtil_GetRingBufferReadAvailable(&audio_rbuf_);
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}while (elements_in_buffer > 1280 || !video_queue_.empty());
		}*/
        return 0;
    }

    virtual int check_error(std::string& error_msg) = 0;

    void str_to_attr_vals(std::string str, std::map<std::string, std::string>& attrs, const std::string& outer_spliters = ";")
    {
		std::string::size_type pos1, pos2;
		pos2 = str.find(outer_spliters);
		pos1 = 0;
		while (std::string::npos != pos2)
		{
            std::pair<std::string, std::string> pair = parseKeyValue(str.substr(pos1, pos2 - pos1));
            if (attrs.find(pair.first) == attrs.end())
            {
                attrs.insert(pair);
            }
			pos1 = pos2 + outer_spliters.size();
			pos2 = str.find(outer_spliters, pos1);
		}
        if (pos1 != str.length())
        {
			std::pair<std::string, std::string> pair = parseKeyValue(str.substr(pos1, pos2 - pos1));
			if (attrs.find(pair.first) == attrs.end())
			{
				attrs.insert(pair);
			}
        }
    }

	std::pair<std::string, std::string> parseKeyValue(const std::string& v, const std::string delim = "=")
	{
		size_t idx = v.find(delim);
		if (idx == std::string::npos)
			return std::make_pair("", "");
		else
		{
			std::string key = v.substr(0, idx);
			std::string value = (v.length() >= idx + 1) ? v.substr(idx + 1) : "";
			if (!value.empty() && value.back() == ',') value.pop_back();
			return std::make_pair(key, value);
		}
	}

protected:
    virtual void reset(){
        error_code_ = 0;
    }

    unsigned nextpowerof2(unsigned val){
        val --;
        val = (val >> 1) | val;
        val = (val >> 2) | val;
        val = (val >> 4) | val;
        val = (val >> 8) | val;
        val = (val >> 16) | val;
        return ++val;
    }

protected:
    struct  EncodeImageData
    {
        cv::Mat frame;
        bool voice_activity;
    };
    struct EncodeAudioData
    {
        void* Data;

        EncodeAudioData(void* InData)
		{
			Data = InData;
		}
        EncodeAudioData()
		{
			Data = nullptr;
		};
    };
    
    std::atomic<int>                    error_code_;
    std::string                         error_msg_;
    MediaParams                         media_params_;
    std::map<std::string,std::string>   extra_params_;
    H264EncodeParams                    enc_params_;

    //PaUtilRingBuffer                    audio_rbuf_;
    //AutoBuffer<short>                   audio_buf_data_;
    SyncQueue<EncodeImageData>          video_queue_;
    SyncQueue<EncodeAudioData>          audio_queue_;

    std::mutex                          err_msg_mtx_;
};
#if PLATFORM_WINDOWS
#ifdef WINDOWS_PLATFORM_TYPES_GUARD
#include "Windows/HideWindowsPlatformTypes.h"
#endif
#endif
#endif // !__MUXER_H__
