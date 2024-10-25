//#include <glog/logging.h>
#include "AVMuxer.hpp"
#include "cvutil/TimeManager.hpp"

#define TICK_START() \
    struct  timespec start,finish; \
    double elapsed; \
    clock_gettime(CLOCK_MONOTONIC,&start);

#define TICK_END() \
    clock_gettime(CLOCK_MONOTONIC,&finish); \
    elapsed = (finish.tv_sec -start.tv_sec); \
    elapsed += (finish.tv_nsec -start.tv_nsec) / 1000000000.0; \
    LOG(INFO) << "++++++++++++++elapsed(msec)" << elapsed * 1000;

AVMuxer::AVMuxer(const MediaParams& params)
    :MediaMuxer(params)
{
    reset();
}
    
AVMuxer::~AVMuxer()
{
    cleanup();
}

int AVMuxer::open(const std::string& out_url)
{
    int ret = 0;

    if(out_url.empty()){
        UE_LOG(LogUMuxer, Error, TEXT("url is null"));
        //LOG(ERROR) << "url is null";
        return -1;
    }
    MediaMuxer::open(out_url);

    if(!init_params()){
        UE_LOG(LogUMuxer, Error, TEXT("init params failed"));
        //LOG(ERROR) << "init params failed";
        return -1;
    }

    const char* format_name = parse_video_protocal(out_url);
    avformat_alloc_output_context2(&fmt_ctx_,NULL,format_name,out_url.c_str());

    //try again
    if(NULL == fmt_ctx_) {
        UE_LOG(LogUMuxer, Error, TEXT("could not deduce output format from url :%s ,using MPEG"), out_url.c_str());
        //LOG(ERROR) << "could not deduce output format from url :" << out_url  << ", using MPEG" ;
        avformat_alloc_output_context2(&fmt_ctx_,NULL,"mpeg",out_url.c_str());
    }

    if(NULL == fmt_ctx_){
        UE_LOG(LogUMuxer, Error, TEXT("failed to deduce output format from url :%s"), out_url.c_str());
        //LOG(ERROR) << "failed to deduce output format from url :" << out_url;
        return -1;
    }

    // set video/audio format in rtmp flow 
    AVDictionary* fmt_opts = NULL;
    if(VideoProtoType::RTMP == protocal_type_) {
        if(0 == FPlatformString::Stricmp(media_params_.video_encoder_format.c_str(), AVMUXER_CODEC_STR_H264)){
            fmt_ctx_->oformat->video_codec = AV_CODEC_ID_H264;
            fmt_ctx_->oformat->audio_codec = AV_CODEC_ID_AAC;
            UE_LOG(LogUMuxer, Log, TEXT("rtmp video codec h264 ,audio codec aac"));
            //LOG(INFO) << "rtmp video codec h264 ,audio codec aac";
        }else if(0 == FPlatformString::Stricmp(media_params_.video_encoder_format.c_str() ,AVMUXER_CODEC_STR_FLV)){
            fmt_ctx_->oformat->video_codec = AV_CODEC_ID_FLV1;
            fmt_ctx_->oformat->audio_codec = AV_CODEC_ID_ADPCM_SWF;
            UE_LOG(LogUMuxer, Log, TEXT("rtmp video codec flv ,audio codec pcm"));
            //LOG(INFO) << "rtmp video codec flv ,audio codec pcm";
        }else{
            UE_LOG(LogUMuxer, Error, TEXT("video encoder format only support 'h264' and 'flv' but input %s"), media_params_.video_encoder_format.c_str());
            //LOG(ERROR) << "video encoder format only support 'h264' and 'flv' but input " << media_params_.video_encoder_format;
            return -1;
        }
    }else if (VideoProtoType::RTSP == protocal_type_){
        fmt_ctx_->oformat->video_codec = AV_CODEC_ID_H264;
        fmt_ctx_->oformat->audio_codec = AV_CODEC_ID_PCM_ALAW;
        av_dict_set(&fmt_opts,"rtsp_transport","tcp",0);
        av_dict_set(&fmt_opts,"max_delay","500",0);
        av_dict_set(&fmt_opts,"fflags","nobuffer",0);
        av_dict_set(&fmt_opts,"stimeout","2000000",0);
    }
    else if (VideoProtoType::FILE == protocal_type_) {
		if (0 == FPlatformString::Stricmp(media_params_.video_encoder_format.c_str(), AVMUXER_CODEC_STR_H264)) {
			fmt_ctx_->oformat->video_codec = AV_CODEC_ID_H264;
			fmt_ctx_->oformat->audio_codec = AV_CODEC_ID_AAC;
			UE_LOG(LogUMuxer, Log, TEXT("mp4 video codec h264 ,audio codec aac"));
        }
        else
        {
			fmt_ctx_->oformat->video_codec = (AVCodecID)enc_params_.codec_type;
			fmt_ctx_->oformat->audio_codec = AV_CODEC_ID_AAC;
        }
    }


    const AVOutputFormat* oformat = fmt_ctx_->oformat;
    if(video_enable_ && oformat->video_codec != AV_CODEC_ID_NONE){
        if(add_stream(&video_st_,oformat->video_codec) < 0)
            return -1;
        UE_LOG(LogUMuxer, Log, TEXT("video stream added"));
        //LOG(INFO) << "audio stream added";
    }
	if (video_enable_ && oformat->video_codec != AV_CODEC_ID_NONE) {
		if (add_stream(&audio_st_, oformat->audio_codec) < 0)
			return -1;
		UE_LOG(LogUMuxer, Log, TEXT("audio stream added"));
		//LOG(INFO) << "audio stream added";
	}

    if(video_enable_ && (open_video_codec(&video_st_) < 0))
        return -1;
    if(audio_enable_ && (open_audio_codec(&audio_st_) < 0))
        return -1;

    if(!(oformat->flags & AVFMT_NOFILE)){
        ret = avio_open(&fmt_ctx_->pb,out_url.c_str(),AVIO_FLAG_WRITE);
        if(ret < 0){
            show_error("could not open video file",ret);
            return -1;
        }
    }

    ret = avformat_write_header(fmt_ctx_,&fmt_opts);
    if(ret < 0){
        show_error("error occurred when opening output file",ret);
        return -1;
    }

    is_header_writed_ = true;

    return 0;
}

int AVMuxer::start()
{
    //DLOG(INFO) << "IN";

    if(audio_enable_){
        audio_enc_.reset(new AudioEncoderThread(this));
        audio_enc_->create_thread();
    }

    if(video_enable_){
        video_enc_.reset(new VideoEncoderThread(this));
        video_enc_->create_thread();
    }

    //DLOG(INFO) << "OUT";
    return 0;
}


int AVMuxer::stop(bool wait_end)
{
    MediaMuxer::stop(wait_end);
    audio_eof_ = true;

    if(audio_enc_){
        audio_enc_->stop();
        audio_enc_->join();
    }   

    if(video_enc_){
        video_enc_->stop();
        video_enc_->join();
    }

    if(fmt_ctx_
    && video_st_.codec_ctx
    && avcodec_is_open(video_st_.codec_ctx)
    && is_header_writed_){
    av_write_trailer(fmt_ctx_);
    }
    cleanup();
    reset();

    //DLOG(INFO) << "OUT";
    return 0;
}

const char* AVMuxer::parse_video_protocal(const std::string& video_url){
    if(!FPlatformString::Strncmp(video_url.c_str(),"rtmp:",5)){
        protocal_type_ = VideoProtoType::RTMP;
        return AVMUXER_CODEC_STR_FLV;
    }else if (!FPlatformString::Strncmp(video_url.c_str(),"rtsp:",5)){
        protocal_type_ = VideoProtoType::RTSP;
        return "rtsp";
    }else if (!FPlatformString::Strncmp(video_url.c_str(),"tcp:",4)){
        protocal_type_ = VideoProtoType::TCP;
        return "mpegts";
    }else if (!FPlatformString::Strncmp(video_url.c_str(),"udp:",4)){
        protocal_type_ = VideoProtoType::UDP;
        return "mpegts";
    }
    protocal_type_ = VideoProtoType::FILE;
    return NULL;
}

//void AVMuxer::AudioEncoderThread::thread_loop()
//{
//    UE_LOG(LogUMuxer, Log, TEXT("Enter into AudioEncoderThread"));
//    //LOG(INFO) << "Enter into AudioEncoderThread";
//    int ret = 0 ;
//
//    AVCodecContext * codec_ctx = pmuxer_->audio_st_.codec_ctx;
//    AVFrame* tmp_frame = pmuxer_->audio_st_.tmp_frame;
//    AVFrame* frame = pmuxer_->audio_st_.frame;
//    AVPacket pkt = {};
//
//    //uint8_t* data = (uint8_t*)tmp_frame->data[0];
//
//    uint8_t* data = new uint8_t[8192 + 256];
//    ring_buffer_size_t elements_in_buffer;
//    bool is_flush = true;
//    PaUtilRingBuffer swr_rbuf_;
//    char* swr_rbuf_data = NULL;
//
//    const int BUFFER_SIZE = 1;
//    unsigned num_samples = pmuxer_->nextpowerof2((unsigned)(codec_ctx->sample_rate * BUFFER_SIZE * codec_ctx-> channels));
//    int bytes_per_sample = av_get_bytes_per_sample(codec_ctx->sample_fmt);
//    unsigned num_bytes = num_samples * bytes_per_sample;
//    swr_rbuf_data = new char[num_bytes];
//    UE_LOG(LogUMuxer, Log, TEXT("swr_rbuf_data samples len %d"),num_samples);
//
//    if(PaUtil_InitializeRingBuffer(&swr_rbuf_,sizeof(char),num_bytes,swr_rbuf_data) < 0){
//        UE_LOG(LogUMuxer, Error, TEXT("Failed to initialize resample ring buffer.Size is not power of 2??"));
//        delete[] swr_rbuf_data;
//        return;
//    }
//    int swr_out_samples = swr_get_out_samples(pmuxer_->audio_st_.swr_ctx,tmp_frame->nb_samples) << 1;
//
//    char* out_buf = new char [swr_out_samples * bytes_per_sample + 256];
//
//    int out_count = 0;
//    bool is_sync = false;
//
//    while(!is_exit()){
//        if(pmuxer_->error_code_)
//            break;
//       
//        //从buffer里读数据 到tmpframe里
//        elements_in_buffer = PaUtil_GetRingBufferReadAvailable(&pmuxer_->audio_rbuf_);
//        if (tmp_frame->nb_samples * tmp_frame->channels <= elements_in_buffer) {
//            long  numRead  = PaUtil_ReadRingBuffer(&pmuxer_->audio_rbuf_, data, tmp_frame->nb_samples * tmp_frame->channels * 4);
//            UE_LOG(LogUMuxer, Error, TEXT("PaUtil_ReadRingBuffer %ld!"),numRead);
//        }
//        else {
//            std::this_thread::sleep_for(std::chrono::milliseconds(1));
//            continue;
//        }
//
//        tmp_frame->pts = pmuxer_->audio_st_.next_pts;
//        pmuxer_->audio_st_.next_pts += tmp_frame->nb_samples;
//
//		out_count = swr_convert(pmuxer_->audio_st_.swr_ctx,
//            (uint8_t**)&out_buf, 4096,
//            (const uint8_t**)&data, tmp_frame->nb_samples);
//
//        if(out_count < 0){
//            UE_LOG(LogUMuxer, Error, TEXT("error while audio converting!"));
//            //LOG(ERROR) << "error while audio converting!";
//            continue;
//        }
//
//        PaUtil_WriteRingBuffer(&swr_rbuf_,out_buf,out_count * bytes_per_sample);
//        while(frame && PaUtil_GetRingBufferReadAvailable(&swr_rbuf_) >= frame->nb_samples * bytes_per_sample){
//            
//            PaUtil_ReadRingBuffer(&swr_rbuf_,frame->data[0],frame->nb_samples * bytes_per_sample);
//
//            frame->pts = av_rescale_q(pmuxer_->audio_st_.samples, { 1,codec_ctx->sample_rate },codec_ctx->time_base);
//            
//            pmuxer_->audio_st_.samples += frame->nb_samples;
//audio_flush:
//            av_init_packet(&pkt);
//            ret = avcodec_send_frame(codec_ctx,frame);
//            if(ret != AVERROR_EOF && ret < 0){
//                pmuxer_->show_error("Error encoding audio frame :",ret);
//                pmuxer_->set_error(ret);
//                break;
//            }
//            while (true){
//                ret = avcodec_receive_packet(codec_ctx,&pkt);
//                if(ret == EAGAIN || ret < 0){
//                    break;
//                }
//
//                ret = pmuxer_->write_packet(pmuxer_->fmt_ctx_,&codec_ctx->time_base,
//                                            pmuxer_->audio_st_.st,&pkt);
//                if(ret < 0){
//                    pmuxer_->show_error("Error while writing audio frame: ",ret);
//                    pmuxer_->set_error(ret);
//                    break;
//                }
//            } 
//        }//end of
//        if(pmuxer_->error_code_ < 0) break;
//    }//end of while loop
//    if(is_flush){
//        frame = NULL;
//        is_flush = false;
//        goto audio_flush;
//    }
//    if (out_buf) delete[] out_buf;
//    if(swr_rbuf_data) delete[] swr_rbuf_data;
//    UE_LOG(LogUMuxer, Log, TEXT("Exit from AudioEncoderThread"));
//}


void AVMuxer::AudioEncoderThread::thread_loop()
{
	UE_LOG(LogUMuxer, Log, TEXT("Enter into AudioEncoderThread"));
	//LOG(INFO) << "Enter into AudioEncoderThread";
	int ret = 0;

	AVCodecContext* codec_ctx = pmuxer_->audio_st_.codec_ctx;
	AVFrame* tmp_frame = pmuxer_->audio_st_.tmp_frame;
	AVFrame* frame = pmuxer_->audio_st_.frame;
	AVPacket pkt = {};

	//uint8_t* data = (uint8_t*)tmp_frame->data[0];

	uint8_t* data = new uint8_t[8192 + 256];
	//ring_buffer_size_t elements_in_buffer;
	bool is_flush = true;
	//PaUtilRingBuffer swr_rbuf_;
	char* swr_rbuf_data = NULL;

	const int BUFFER_SIZE = 1;
	unsigned num_samples = pmuxer_->nextpowerof2((unsigned)(codec_ctx->sample_rate * BUFFER_SIZE * codec_ctx->channels));
	int bytes_per_sample = av_get_bytes_per_sample(codec_ctx->sample_fmt);
	unsigned num_bytes = num_samples * bytes_per_sample;
	swr_rbuf_data = new char[num_bytes];
	UE_LOG(LogUMuxer, Log, TEXT("swr_rbuf_data samples len %d"), num_samples);

	/*if (PaUtil_InitializeRingBuffer(&swr_rbuf_, sizeof(uint8_t), num_bytes, swr_rbuf_data) < 0) {
		UE_LOG(LogUMuxer, Error, TEXT("Failed to initialize resample ring buffer.Size is not power of 2??"));
		delete[] swr_rbuf_data;
		return;
	}*/
	int swr_out_samples = swr_get_out_samples(pmuxer_->audio_st_.swr_ctx, tmp_frame->nb_samples) << 1;

	//char* out_buf = new char[swr_out_samples * bytes_per_sample + 256];

	int out_count = 0;
	bool is_sync = false;
    EncodeAudioData encode_data;

	while (!is_exit()) {
		if (pmuxer_->error_code_)
			break;

		//从buffer里读数据 到tmpframe里
		//elements_in_buffer = PaUtil_GetRingBufferReadAvailable(&pmuxer_->audio_rbuf_);

		if (pmuxer_->audio_queue_.read(encode_data) < 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;;
		}

		tmp_frame->pts = pmuxer_->audio_st_.next_pts;
		pmuxer_->audio_st_.next_pts += tmp_frame->nb_samples;

		out_count = swr_convert(pmuxer_->audio_st_.swr_ctx,
			pmuxer_->audio_st_.outputChannels, 4096,
			(const uint8_t**)&encode_data.Data, tmp_frame->nb_samples);

		if (out_count < 0) {
			UE_LOG(LogUMuxer, Error, TEXT("error while audio converting!"));
			//LOG(ERROR) << "error while audio converting!";
			continue;
		}

		frame->data[0] = pmuxer_->audio_st_.outputChannels[0];
		frame->data[1] = pmuxer_->audio_st_.outputChannels[1];

		frame->pts = av_rescale_q(pmuxer_->audio_st_.samples, { 1,codec_ctx->sample_rate }, codec_ctx->time_base);

		pmuxer_->audio_st_.samples += frame->nb_samples;
	audio_flush:
		av_init_packet(&pkt);
		ret = avcodec_send_frame(codec_ctx, frame);
		if (ret != AVERROR_EOF && ret < 0) {
			pmuxer_->show_error("Error encoding audio frame :", ret);
			pmuxer_->set_error(ret);
			break;
		}
		while (true) {
            ret = avcodec_receive_packet(codec_ctx, &pkt);
			if (ret == EAGAIN || ret < 0) {
				break;
			}
            UE_LOG(LogUMuxer, VeryVerbose, TEXT(">>>>>>>>> audio write_packet %lld <<<<<<<<<<"), pkt.pts);
            
            pkt.pts = pmuxer_->audio_st_.real_pts;
            pmuxer_->audio_st_.real_pts += 1024;
			ret = pmuxer_->write_packet(pmuxer_->fmt_ctx_, &codec_ctx->time_base,
				pmuxer_->audio_st_.st, &pkt);
			if (ret < 0) {
				pmuxer_->show_error("Error while writing audio frame: ", ret);
				pmuxer_->set_error(ret);
				break;
			}
		}
	}//end of
	
	if (is_flush) {
		frame = NULL;
		is_flush = false;
		goto audio_flush;
	}
	//if (out_buf) delete[] out_buf;
	if (swr_rbuf_data) delete[] swr_rbuf_data;
	UE_LOG(LogUMuxer, Log, TEXT("Exit from AudioEncoderThread"));
}

void AVMuxer::VideoEncoderThread::thread_loop()
{
    UE_LOG(LogUMuxer, Log, TEXT("Enter into VideoEncoderThread"));
    //LOG(INGO) << "Enter into VideoEncoderThread";
    int ret = 0;
    VideoStream* ost = &pmuxer_->video_st_;
    AVCodecContext* codec_ctx = ost->codec_ctx;

    AVFrame* frame = ost->frame;
    AVPacket* pkt = av_packet_alloc();

    if(!ost->sws_ctx) {
        ost->sws_ctx = sws_getContext(pmuxer_->video_in_param_.width,
                pmuxer_->video_in_param_.height,
                pmuxer_->video_in_param_.pixel_fmt,
                codec_ctx->width,codec_ctx->height,
                codec_ctx->pix_fmt,
                SWS_BICUBIC,NULL,NULL,NULL);
    }
    EncodeImageData encode_img;
    bool need_scale = (pmuxer_->video_in_param_.pixel_fmt != codec_ctx->pix_fmt);
    bool is_flush = false;
    bool is_voice_activity = false;
    bool is_sync = false;
    //int frame_num = 0;

    while(!is_exit()) {
		if (pmuxer_->error_code_)
			break;
        //av sync 
        if(pmuxer_->audio_enable_ && (pmuxer_->protocal_type_ == VideoProtoType::RTSP || pmuxer_->protocal_type_ == VideoProtoType::FILE)){
            {
                std::lock_guard<std::mutex> lock(pmuxer_->read_mtx_);
                if(av_compare_ts(ost->next_pts,ost->codec_ctx->time_base,
                        pmuxer_->audio_st_.next_pts,pmuxer_->audio_st_.codec_ctx->time_base) > 0
                        && !pmuxer_->audio_eof_){
                    UE_LOG(LogUMuxer, VeryVerbose, TEXT("video is faster than audio"));
                    is_sync = true;        
                }   
            }
            if(is_sync){
                is_sync = false;
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
        }

        ret = pmuxer_->video_queue_.read(encode_img);
        if(ret < 0){
            UE_LOG(LogUMuxer, Log, TEXT("Input image queue is truely empty now"));
            //LOG(INFO) << "Input image queue is truely empty now";
            frame = NULL;
            is_flush = true;
            goto flush;
        }

        ret = av_frame_make_writable(frame);
        if(ret < 0){
            UE_LOG(LogUMuxer, Warning, TEXT("Current frame is not available"));
            //LOG(WARINIG) << "Current frame is not available";
            continue;
        }
        if(pmuxer_->protocal_type_ == VideoProtoType::RTSP){
            std::lock_guard<std::mutex> lock(pmuxer_->read_mtx_);
            ost->frame->pts = ost->next_pts++;
        }else{
            ost->frame->pts = ost->next_pts++;
        }

        if(need_scale){
            int step = encode_img.frame.step;
            sws_scale(ost->sws_ctx,&encode_img.frame.data,&step,0,encode_img.frame.rows,frame->data,frame->linesize);
        }else{
            //check(0) "Error mat to avframe,not implemented";
        }

flush:
      //TICK_START();
      //encode the image
      ret = avcodec_send_frame(codec_ctx,frame);
      if(ret != AVERROR_EOF && ret < 0){
        pmuxer_->show_error("Error encoding video frame: ",ret);
        pmuxer_->set_error(ret);
        break;
      }

      while(true){
        ret = avcodec_receive_packet(codec_ctx,pkt);
        if(ret == EAGAIN || ret < 0)
            break;

        if(pmuxer_->video_st_.bsf_ctx){
            ret = av_bsf_send_packet(pmuxer_->video_st_.bsf_ctx, pkt);
            if(ret != AVERROR_EOF && ret != AVERROR(EAGAIN) && ret < 0){
                pmuxer_->show_error("av_bsf_send_packet error : " ,ret);
                pmuxer_->set_error(ret);
                break;
            }

            av_packet_unref(pkt);
            ret = av_bsf_receive_packet(pmuxer_->video_st_.bsf_ctx,pkt);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            if(ret < 0){
                pmuxer_->show_error("av_bsf_receive_packet error: " ,ret);
                pmuxer_->set_error(ret);
                break;
            }
        }

        if(is_voice_activity != encode_img.voice_activity){
            if (encode_img.voice_activity)
            {
                UE_LOG(LogUMuxer, Log, TEXT(">>>>>>>>> Virtual anchor begin to speak <<<<<<<<<<"));
                //LOG(INFO) << ">>>>>>>>> Virtual anchor begin to speak <<<<<<<<<<";
            }
            else
            {
                UE_LOG(LogUMuxer, Log, TEXT(">>>>>>>>> Virtual anchor stop to speak <<<<<<<<<<"));
                //LOG(INFO) << ">>>>>>>>> Virtual anchor stop to speak <<<<<<<<<<";
            }
            is_voice_activity = encode_img.voice_activity;
        }

        UE_LOG(LogUMuxer, Verbose, TEXT(">>>>>>>>> write_packet %lld <<<<<<<<<<"), pkt->pts);
        ret = pmuxer_->write_packet(pmuxer_->fmt_ctx_,&codec_ctx->time_base,
                                        pmuxer_->video_st_.st,pkt);
        pmuxer_->frame_num ++;
        
        av_packet_unref(pkt);
        encode_img.frame.release();
        if(ret < 0){
            pmuxer_->show_error("Error while writing video frame: ",ret);
            pmuxer_->set_error(ret);
            break;
        }
      }

      //TICK_END();
        if(is_flush || pmuxer_->error_code_ < 0) break;
    }
    pmuxer_->video_eof_ = true;
    av_packet_free(&pkt);
    UE_LOG(LogUMuxer, Log, TEXT("Exit from VideoEncoderThread"));
    //LOG(INFO) << "Exit from VideoEncoderThread";
}

int AVMuxer::add_stream(AudioStream* ost ,enum AVCodecID codec_id)
{
    AVCodecContext* codec_ctx;
    AVCodec* codec;

    /*find the encoder*/
    codec = avcodec_find_encoder(codec_id);
    if(!codec){
        UE_LOG(LogUMuxer, Error, TEXT("Could not find encoder for %s"), avcodec_get_name(codec_id));
        //LOG(ERROR) << "Could not find encoder for " << avcodec_get_name(codec_id);
        return -1;
    }

    ost->st = avformat_new_stream(fmt_ctx_,NULL);
    if(!ost->st){
        UE_LOG(LogUMuxer, Error, TEXT("could not allocate stream"));
        //LOG(ERROR) << "could not allocate stream";
        return -1;
    }
    ost->st->id = fmt_ctx_->nb_streams - 1;
    codec_ctx = avcodec_alloc_context3(codec);
    if(!codec_ctx){
        UE_LOG(LogUMuxer, Error, TEXT("could not allocate codec context"));
        //LOG(ERROR) << "could not allocate codec context ";
        return -1;
    }

    ost->codec = codec;
    ost->codec_ctx = codec_ctx;

    switch (codec->type)
    {
    case AVMEDIA_TYPE_AUDIO:
        codec_ctx->sample_fmt = codec->sample_fmts ? codec->sample_fmts[0] : ost->audio_param.sample_fmt;
        //codec_ctx->bit_rate = 64000;
        if(VideoProtoType::RTMP == protocal_type_)
            codec_ctx->sample_rate = 22050;
        else if(VideoProtoType::RTSP == protocal_type_)
            codec_ctx->sample_rate = enc_params_.out_audio_sample_rate;
        else
            codec_ctx->sample_rate = ost->audio_param.sample_rate;

        if(codec->supported_samplerates){
            codec_ctx->sample_rate = codec->supported_samplerates[0];
            for(int i= 0;codec->supported_samplerates[i];i++){
                if(codec->supported_samplerates[i] == ost->audio_param.sample_rate){
                    codec_ctx->sample_rate = ost->audio_param.sample_rate;
                    break;
                }
            }
        }
        codec_ctx->channel_layout = ost->audio_param.channel_layout;
        if(codec->channel_layouts){
            codec_ctx->channel_layout = codec->channel_layouts[0];
            for(int i = 0; codec->channel_layouts[i];i++){
                if(codec->channel_layouts[i] == ost->audio_param.channel_layout){
                    codec_ctx->channel_layout = ost->audio_param.channel_layout;
                    break;
                }
            }
        }
        codec_ctx->channels = av_get_channel_layout_nb_channels(codec_ctx->channel_layout);
        ost->st->time_base  = {1,codec_ctx->sample_rate};
        codec_ctx->time_base = ost->st->time_base;
        break;
    
    default:
        break;
    }

    if(fmt_ctx_->oformat->flags & AVFMT_GLOBALHEADER)
        codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    //给ue双通道音频提前分配内存
    ost->outputChannels[0] = static_cast<uint8_t*>(FMemory::Realloc(ost->outputChannels[0], 4096));
    ost->outputChannels[1] = static_cast<uint8_t*>(FMemory::Realloc(ost->outputChannels[1], 4096));

    return 0;
}

int AVMuxer::add_stream(VideoStream* ost,enum AVCodecID codec_id)
{
    AVCodecContext* codec_ctx;
    AVCodec* codec;

    if(V_GPU == media_params_.device_type && AV_CODEC_ID_H264 == codec_id){
        codec = avcodec_find_encoder_by_name("h264_nvenc");
    }else{
        codec = avcodec_find_encoder(codec_id);
    }

    if(!codec){
        UE_LOG(LogUMuxer, Error, TEXT("could not find encoder for %s"),avcodec_get_name(codec_id));
        return -1;
    }
    ost->st = avformat_new_stream(fmt_ctx_, NULL);
    if (!ost->st) {
        UE_LOG(LogUMuxer, Error, TEXT("could not allocate stream"));
        return -1;
    }
    ost->st->id = fmt_ctx_->nb_streams - 1;
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        UE_LOG(LogUMuxer, Error, TEXT("could not allocate codec context"));
        return -1;
    }
    ost->codec = codec;
    ost->codec_ctx = codec_ctx;

    switch (codec->type)
    {
    case AVMEDIA_TYPE_VIDEO:
        codec_ctx->codec_id = codec_id;

        codec_ctx->width    = ost->video_param.width;
        codec_ctx->height   = ost->video_param.height;

        ost->st->time_base = {1,ost->video_param.fps};
        codec_ctx->time_base = ost->st->time_base;
        codec_ctx->gop_size  = enc_params_.gopsize;
        codec_ctx->profile   = enc_params_.profile;
        codec_ctx->colorspace = (AVColorSpace)enc_params_.colorspace;
		codec_ctx->bit_rate = bitrate_;
		codec_ctx->rc_min_rate = bitrate_;
		codec_ctx->rc_max_rate = bitrate_;
		codec_ctx->bit_rate_tolerance = bitrate_;
		codec_ctx->rc_buffer_size = bitrate_;
		codec_ctx->rc_initial_buffer_occupancy = codec_ctx->rc_buffer_size * 3 / 4;
        codec_ctx->pix_fmt = (AVPixelFormat)enc_params_.out_pixfmt;
        if(codec_ctx->codec_id == AV_CODEC_ID_MPEG2VIDEO){
            codec_ctx->max_b_frames = 2;
        }else if(codec_ctx->codec_id == AV_CODEC_ID_MPEG1VIDEO){
            codec_ctx->mb_decision = 2;
        }

        break;
    
    default:
        break;
    }

    if(fmt_ctx_->oformat->flags & AVFMT_GLOBALHEADER)
        codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    return 0;
}

int AVMuxer::open_video_codec(VideoStream* vost)
{
    int ret;
    AVCodecContext* c = vost->codec_ctx;
    AVCodec* codec = vost->codec;

    /*set option*/
    if(V_GPU == media_params_.device_type && AV_CODEC_ID_H264 == c->codec_id){
        av_opt_set_int(c->priv_data,"gpu",media_params_.device_id, 0 );
        av_opt_set_int(c->priv_data,"zerolatency",1,0);
        av_opt_set_int(c->priv_data,"cbr",1,0);
        
        if(extra_params_.find(AVMUXER_CODEC_PARAMS_PROFILE) != extra_params_.end())
            av_opt_set(c->priv_data,"profile",extra_params_[AVMUXER_CODEC_PARAMS_PROFILE].c_str(),0);
    }
    c->thread_count = enc_params_.thread_count;

    if(protocal_type_ == VideoProtoType::RTSP){
        av_opt_set(c->priv_data,"tune","animation+zerolatency",0);
    }else{
        av_opt_set(c->priv_data,"tune","zerolatency",0);
    }

    ret = avcodec_open2(c,codec,NULL);
    if(ret < 0){
        show_error("could not open video codec ",ret);
        return -1;
    }    

    vost->frame = alloc_picture(c->pix_fmt, c-> width, c-> height);
    if(!vost->frame)
    {
        UE_LOG(LogUMuxer, Error, TEXT("could not allocate video frame!"));
        //LOG(ERROR) << "could not allocate video frame!";
        return -1;
    }

    vost->tmp_frame = NULL;
    if(c->pix_fmt != video_in_param_.pixel_fmt){
        vost->tmp_frame = alloc_picture(video_in_param_.pixel_fmt,video_in_param_.width,video_in_param_.height);
        if(!vost->tmp_frame){
            fprintf(stderr,"Could not allocate temporary picture \n");
            return -1;
        }
    }

    ret = avcodec_parameters_from_context(vost->st->codecpar,c);
    if(ret < 0){
        show_error("Could not copy the video stream parameters",ret);
        return -1;
    }
    return 0;
}

int AVMuxer::open_audio_codec(AudioStream* aost)
{
    int nb_samples;
    int ret;
    AVCodecContext* c = aost->codec_ctx;
    AVCodec* codec = aost->codec;

    ret = avcodec_open2(c,codec,NULL);
    if(ret < 0){
        show_error("could not open audio codec",ret);
        return -1;
    }

    if(c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = c->frame_size;

    aost->frame = alloc_audio_frame(c->sample_fmt,c->channel_layout,
                                    c->sample_rate,nb_samples);
	aost->tmp_frame = alloc_audio_frame(audio_in_param_.sample_fmt, audio_in_param_.channel_layout,
		audio_in_param_.sample_rate, nb_samples);

    if(!aost->frame || !aost->tmp_frame){
        UE_LOG(LogUMuxer, Error, TEXT("could not alloc audio frame!"));
        //LOG(ERROR) << "could not alloc audio frame!";
        return -1;
    }

    ret = avcodec_parameters_from_context(aost->st->codecpar,c);
    if(ret < 0){
        show_error("could not copy the audio stream parameters!",ret);
        return -1;
    }

    aost->swr_ctx = swr_alloc();
    if(!aost->swr_ctx){
        UE_LOG(LogUMuxer, Error, TEXT("could not allocate audio resampler context!"));
        //LOG(ERROR) << "could not allocate audio resampler context!";
        return -1;
    }

    /*set option*/
	av_opt_set_int(aost->swr_ctx,           "in_channel_layout",    audio_in_param_.channels == 2 ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO, 0);
	av_opt_set_int(aost->swr_ctx,           "out_channel_layout",   AV_CH_LAYOUT_STEREO, 0          );
    av_opt_set_int(aost->swr_ctx,           "in_channel_count"      ,audio_in_param_.channels,     0);
    av_opt_set_int(aost->swr_ctx,           "in_sample_rate"        ,audio_in_param_.sample_rate,  0);
    av_opt_set_sample_fmt(aost->swr_ctx,    "in_sample_fmt"         ,audio_in_param_.sample_fmt,   0);
    av_opt_set_int(aost->swr_ctx,           "out_channel_count"     ,c->channels,                   0);
    av_opt_set_int(aost->swr_ctx,           "out_sample_rate"       ,c->sample_rate,    0);
    av_opt_set_sample_fmt(aost->swr_ctx,    "out_sample_fmt"        ,c->sample_fmt,                 0);
    
    UE_LOG(LogUMuxer, Log, TEXT("audio in channels %d sample_rate %d sample_fmt %d"), 
        audio_in_param_.channels,
        audio_in_param_.sample_rate,
        audio_in_param_.sample_fmt);

	UE_LOG(LogUMuxer, Log, TEXT("audio out channels %d sample_rate %d sample_fmt %d"),
		c->channels,
		c->sample_rate,
		c->sample_fmt);

    if((ret = swr_init(aost->swr_ctx)) < 0){
        show_error("failed to initialize audio resampling context!",ret);
        return -1;
    }

    return 0;
}

void AVMuxer::reset()
{
    MediaMuxer::reset();
    audio_enable_ = false;
    video_enable_ = false;

    audio_eof_ = false;
    video_eof_ = false;
    is_header_writed_ = false;
    error_code_ = 0;
    protocal_type_ = VideoProtoType::FILE;

    fmt_ctx_ = NULL;

    memset(&audio_in_param_,0,sizeof(AudioParam));
    memset(&video_in_param_,0,sizeof(VideoParam));

    if (audio_st_.outputChannels)
    {
        FMemory::Free(audio_st_.outputChannels[0]);
        FMemory::Free(audio_st_.outputChannels[1]);
    }
    memset(&audio_st_,0,sizeof(AudioStream));
    memset(&video_st_,0,sizeof(VideoStream));

    video_queue_.force(false);

    frame_num = 0;
}

bool AVMuxer::init_params()
{
    audio_in_param_.channels = media_params_.audio_channels;
    audio_in_param_.sample_rate = media_params_.audio_sample_rate;
    switch (media_params_.audio_sample_format)
    {
    case SAMPLE_FMT_U8:
        audio_in_param_.sample_fmt = AV_SAMPLE_FMT_U8;
        break;
    case SAMPLE_FMT_S16:
        audio_in_param_.sample_fmt = AV_SAMPLE_FMT_S16;
        break;
    case SAMPLE_FMT_S32:
        audio_in_param_.sample_fmt = AV_SAMPLE_FMT_S32;
        break;   
    case SAMPLE_FMT_FLT:
        audio_in_param_.sample_fmt = AV_SAMPLE_FMT_FLT;
        break;
    
    default:
        audio_in_param_.sample_fmt = AV_SAMPLE_FMT_NONE;
        break;
    }

    audio_in_param_.bits_per_sample = av_get_bytes_per_sample(audio_in_param_.sample_fmt) * 8;
    audio_in_param_.channel_layout = av_get_default_channel_layout(media_params_.audio_channels);
    audio_enable_ = true;

    video_in_param_.fps = media_params_.frame_rate;
    video_in_param_.height = media_params_.height;
    video_in_param_.width = media_params_.width;
    video_in_param_.pixel_fmt = get_ffpixel_format(media_params_.in_pix_fmt);
    video_enable_ = true;
    bitrate_ = media_params_.bitrate;

    //config output stream params
    video_st_.video_param.width = (video_in_param_.width / 2) * 2;
    video_st_.video_param.height = (video_in_param_.height / 2) * 2;
    video_st_.video_param.fps = video_in_param_.fps ;
    video_st_.video_param.pixel_fmt = video_in_param_.pixel_fmt;

    UE_LOG(LogUMuxer, Log, TEXT("video width in %d , out %d"), video_in_param_.width, video_st_.video_param.width);
    UE_LOG(LogUMuxer, Log, TEXT("video height in %d , out %d"), video_in_param_.height, video_st_.video_param.height);

    audio_st_.audio_param.channels = audio_in_param_.channels;
    audio_st_.audio_param.sample_rate = audio_in_param_.sample_rate;
    audio_st_.audio_param.sample_fmt = audio_in_param_.sample_fmt;
    audio_st_.audio_param.channel_layout = audio_in_param_.channel_layout;
    return true;
}

AVPixelFormat AVMuxer::get_ffpixel_format(ImagePixelFormat in_fmt)
{
    AVPixelFormat out_fmt;
    switch (in_fmt)
    {
    case PIX_FMT_YUV420P:
        out_fmt = AV_PIX_FMT_YUV420P;
        break;
    case PIX_FMT_YUYV422:
        out_fmt = AV_PIX_FMT_YUYV422;
        break;
    case PIX_FMT_YUV444P:
        out_fmt = AV_PIX_FMT_YUV444P;
        break;
    case PIX_FMT_RGB24:
        out_fmt = AV_PIX_FMT_RGB24;
        break;
    case PIX_FMT_BGR24:
        out_fmt = AV_PIX_FMT_BGR24;
        break;
    case PIX_FMT_GRAY8:
        out_fmt = AV_PIX_FMT_GRAY8;
        break;
    case PIX_FMT_ARGB:
        out_fmt = AV_PIX_FMT_ARGB;
        break;
    case PIX_FMT_RGBA:
        out_fmt = AV_PIX_FMT_RGBA;
        break;
    case PIX_FMT_BGRA:
        out_fmt = AV_PIX_FMT_BGRA;
        break;
    case PIX_FMT_YUVA420P:
        out_fmt = AV_PIX_FMT_YUVA420P;
        break;
    case PIX_FMT_YUVA444P10LE:
        out_fmt = AV_PIX_FMT_YUVA444P10LE;
        break;
    default:
        UE_LOG(LogUMuxer, Warning, TEXT("input video pixel format %d not support , AV_PIX_FMT_BGR24 instead"), in_fmt);
        //LOG(WARNING) << "input video pixel format " << in_fmt << "not support," << AV_PIX_FMT_BGR24 << "instead";
        out_fmt = AV_PIX_FMT_BGR24;
        break;
    }
    return out_fmt;
}

unsigned AVMuxer::nextpowerof2(unsigned val)
{
    val --;
    val = (val >> 1) | val;
    val = (val >> 2) | val;
    val = (val >> 4) | val;
    val = (val >> 8) | val;
    val = (val >> 16) | val;
    return ++val;
}

int AVMuxer::write_packet(AVFormatContext *fmt_ctx,const AVRational *time_base,AVStream *st,AVPacket *pkt)
{
    std::lock_guard<std::mutex> lock(write_mtx_);
    av_packet_rescale_ts(pkt,*time_base,st->time_base);
    pkt->stream_index = st->index;
#if AV_VERBOSE
    LOG(WARNING) << "###########" <<
        (st->index == 0 ? "video " : "audio ") << ", pts " << pkt->pts * av_q2d(st->time_base);
#endif

    if(st->index == 0 && pkt->pts == 0)
        UE_LOG(LogUMuxer, Log, TEXT(">>>>>>>>>> start to send video frams <<<<<<<<<"));
        //LOG(INFO) << ">>>>>>>>>> start to send video frams <<<<<<<<<";
    
    return av_interleaved_write_frame(fmt_ctx,pkt);
}

AVFrame* AVMuxer::alloc_picture(enum AVPixelFormat pix_fmt,int width, int height)
{
    AVFrame *picture;
    int ret;

    picture = av_frame_alloc();
    if(!picture)
        return NULL;

    picture->format = pix_fmt;
    picture->width = width;
    picture->height = height;

    ret = av_frame_get_buffer(picture,32);
    if(ret < 0 ){
        fprintf(stderr,"Could not allocate frame data'\n");
        return NULL;
    }

    return picture;
}

AVFrame* AVMuxer::alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                    uint64_t channel_layout,
                                    int sample_rate, int nb_samples)
{
    AVFrame * frame = av_frame_alloc();
    int ret;

    if(!frame){
        UE_LOG(LogUMuxer, Error, TEXT("Error allocation an audio frame"));
        //LOG(ERROR) << "Error allocation an audio frame";
        return NULL;
    }

    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if(nb_samples){
        ret = av_frame_get_buffer(frame,0);
        if(ret < 0){
            UE_LOG(LogUMuxer, Error, TEXT("Error allocation an audio buffer"));
            //LOG(ERROR) << "Error allocation an audio buffer";
            return NULL;
        }
    }

    return frame;
}

void AVMuxer::close_video()
{
    avcodec_free_context(&video_st_.codec_ctx);
    av_frame_free(&video_st_.frame);
    av_frame_free(&video_st_.tmp_frame);
    sws_freeContext(video_st_.sws_ctx);
    if(video_st_.bsf_ctx) av_bsf_free(&video_st_.bsf_ctx);

    return;
}

void AVMuxer::close_audio()
{
    avcodec_free_context(&audio_st_.codec_ctx);
    av_frame_free(&audio_st_.frame);
    av_frame_free(&audio_st_.tmp_frame);
    swr_free(&audio_st_.swr_ctx);

    return;
}

void AVMuxer::show_error(const char* prefix ,int err)
{
    char error_msg[128] = {0};
    av_strerror(err,error_msg,sizeof(error_msg));
    UE_LOG(LogUMuxer, Error, TEXT("%s %s"), ANSI_TO_TCHAR(prefix), ANSI_TO_TCHAR(error_msg));
    //LOG(ERROR) << prefix << " " << error_msg;
}

void AVMuxer::set_error(int err)
{
    if(error_code_)
        return;
    error_code_ = err;
}

int AVMuxer::check_error(std::string& error_msg)
{
    char msg[128] = {0};
    av_strerror(error_code_,msg,sizeof(msg));
    error_msg = msg;
    return error_code_;
}

void AVMuxer::cleanup()
{
    //UE_LOG(LogUMuxer, Error, TEXT("cleanup"));
    if(video_enable_){
        close_video();
        video_enable_ = false;
    }
    if(audio_enable_){
        close_audio();
        audio_enable_ = false;
    }
    if(fmt_ctx_){
        if(!(fmt_ctx_->oformat->flags & AVFMT_NOFILE))
            avio_closep(&fmt_ctx_->pb);
        avformat_free_context(fmt_ctx_);
        fmt_ctx_ = NULL;
    }
}


int AVMuxer::get_video_frame()
{
    return frame_num;
}