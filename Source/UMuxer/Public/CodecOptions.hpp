#pragma warning(push)
#pragma warning (disable : 4800)

#ifndef __CODEC_OPTIONS_HPP__
#define __CODEC_OPTIONS_HPP__

#include "CoreMinimal.h"
#include <map>

#ifdef __cplusplus
extern "C" {

#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>

#ifdef __cplusplus
}
#endif

namespace{

#define AVMUXER_CODEC_STR_H264 "h264"
#define AVMUXER_CODEC_STR_FLV "flv"

enum MuxerOptionType {
    MUX_OPT_TYPE_FLAG = -1,
    MUX_OPT_TYPE_BOOL,
    MUX_OPT_TYPE_INT,
    MUX_OPT_TYPE_INT64,
    MUX_OPT_TYPE_UINT64,
    MUX_OPT_TYPE_FLOAT,
    MUX_OPT_TYPE_DOUBLE,
    MUX_OPT_TYPE_STRING,
    MUX_OPT_TYPE_CONST
};


enum EncodeMode{
    MUX_ENC_MODE_DEFAULT = -1,
    MUX_ENC_MODE_PINGAN = 0
};

#define AVMUXER_CODEC_PARAMS_COLORSPACE "colorspace"
#define AVMUXER_CODEC_PARAMS_PROFILE "profile"
#define AVMUXER_CODEC_PARAMS_PIXFMT "out_pixfmt"
#define AVMUXER_CODEC_PARAMS_GOPSIZE "gopsize"
#define AVMUXER_CODEC_PARAMS_ENCMODE "encode_mode"
#define AVMUXER_CODEC_PARAMS_ASMPRATE "out_audio_sample_rate"
#define AVMUXER_CODEC_PARAMS_NTHREADS "thread_count"


struct  H264EncodeParams{
    int colorspace;
    int profile;
    int out_pixfmt;
    int gopsize;
    int encode_mode;
    int out_audio_sample_rate;
    int thread_count;
    int codec_type;

    H264EncodeParams() {
        gopsize = 12;
        out_audio_sample_rate = 16000;
        thread_count = 1;
        codec_type = AV_CODEC_ID_H264;
    }
};

struct  MuxerOption {
    const char* name;
    const char* help;
    enum MuxerOptionType type;

    struct Default_val
    {
        bool b;
        int i;
        int64_t i64;
        uint64_t ui64;
        float fp;
        double dbl;
        const char* str;
    } default_val;
};

#ifdef WIN32

#define MUX_ENUM_INT(x,y) {x,"",MUX_OPT_TYPE_CONST,{true,y,0,0,0,0,nullptr}}

static const MuxerOption k_codec_context_params[] = {
    { AVMUXER_CODEC_PARAMS_COLORSPACE,"output codec colorspace",MUX_OPT_TYPE_INT,{true,AVCOL_SPC_UNSPECIFIED,0,0,0,0,nullptr}},
	MUX_ENUM_INT("bt709",       AVCOL_SPC_BT709),
	MUX_ENUM_INT("fcc",         AVCOL_SPC_FCC),
	MUX_ENUM_INT("bt470bg",     AVCOL_SPC_BT470BG),
	MUX_ENUM_INT("smpte170m",   AVCOL_SPC_SMPTE170M),
	MUX_ENUM_INT("smpte240m",   AVCOL_SPC_SMPTE240M),
	MUX_ENUM_INT("ycgco",       AVCOL_SPC_YCGCO),
	MUX_ENUM_INT("gbr",         AVCOL_SPC_RGB),
	MUX_ENUM_INT("bt2020nc",    AVCOL_SPC_BT2020_NCL),
	MUX_ENUM_INT("bt2020ncl",   AVCOL_SPC_BT2020_NCL),

	{ AVMUXER_CODEC_PARAMS_PROFILE,"output h264 encode profile",MUX_OPT_TYPE_INT,{true,FF_PROFILE_H264_MAIN,0,0,0,0,nullptr}},
	MUX_ENUM_INT("baseline",    FF_PROFILE_H264_BASELINE),
	MUX_ENUM_INT("main",        FF_PROFILE_H264_MAIN),
	MUX_ENUM_INT("extended",    FF_PROFILE_H264_EXTENDED),
	MUX_ENUM_INT("high",        FF_PROFILE_H264_HIGH),
	MUX_ENUM_INT("high422",     FF_PROFILE_H264_HIGH_422),
	MUX_ENUM_INT("high422i",    FF_PROFILE_H264_HIGH_422_INTRA),
	MUX_ENUM_INT("high444",     FF_PROFILE_H264_HIGH_444),
	MUX_ENUM_INT("high444p",    FF_PROFILE_H264_HIGH_444_PREDICTIVE),
	MUX_ENUM_INT("high444i",    FF_PROFILE_H264_HIGH_444_INTRA),
	MUX_ENUM_INT("stereohigh",  FF_PROFILE_H264_STEREO_HIGH),
	MUX_ENUM_INT("stereohigh",  FF_PROFILE_H264_STEREO_HIGH),

	{ AVMUXER_CODEC_PARAMS_PIXFMT ,"output pixel format",MUX_OPT_TYPE_INT ,{true,AV_PIX_FMT_YUV420P,0,0,0,0,nullptr}} ,
	MUX_ENUM_INT("yuv420p",         AV_PIX_FMT_YUV420P),
	MUX_ENUM_INT("yuyv422",         AV_PIX_FMT_YUYV422),
	MUX_ENUM_INT("yuv444p",         AV_PIX_FMT_YUV444P),
	MUX_ENUM_INT("yuv422p",         AV_PIX_FMT_YUV422P),
	MUX_ENUM_INT("yuv410p",         AV_PIX_FMT_YUV410P),
	MUX_ENUM_INT("yuv411p",         AV_PIX_FMT_YUV410P),
	MUX_ENUM_INT("yuva420p",        AV_PIX_FMT_YUVA420P),
	MUX_ENUM_INT("yuva444p10le",    AV_PIX_FMT_YUVA444P10LE),
	MUX_ENUM_INT("nv12",            AV_PIX_FMT_NV12),
	MUX_ENUM_INT("nv21",            AV_PIX_FMT_NV21),
	MUX_ENUM_INT("rgb24",           AV_PIX_FMT_RGB24),
	MUX_ENUM_INT("bgr24",           AV_PIX_FMT_BGR24),
	MUX_ENUM_INT("gray8",           AV_PIX_FMT_GRAY8),
	MUX_ENUM_INT("argb",            AV_PIX_FMT_ARGB),
};
#else
#define MUX_ENUM_INT(x,y) {x,"",MUX_OPT_TYPE_CONST,{.i = y}}
#define MUX_ENUM_FLOAT(x,y) {x,"",MUX_OPT_TYPE_CONST,{.fp = y}}
#define MUX_ENUM_STR(x,y) {x,"",MUX_OPT_TYPE_CONST,{.str = y}}

static const MuxerOption k_codec_context_params[] = {
    { AVMUXER_CODEC_PARAMS_COLORSPACE,"output codec colorspace",MUX_OPT_TYPE_INT,{.i = AVCOL_SPC_UNSPECIFIED}},
    MUX_ENUM_INT("bt709",       AVCOL_SPC_BT709 ),
    MUX_ENUM_INT("fcc",         AVCOL_SPC_FCC ),
    MUX_ENUM_INT("bt470bg",     AVCOL_SPC_BT470BG ),
    MUX_ENUM_INT("smpte170m",   AVCOL_SPC_SMPTE170M ),
    MUX_ENUM_INT("smpte240m",   AVCOL_SPC_SMPTE240M ),
    MUX_ENUM_INT("ycgco",       AVCOL_SPC_YCGCO ),
    MUX_ENUM_INT("gbr",         AVCOL_SPC_RGB ),
    MUX_ENUM_INT("bt2020nc",    AVCOL_SPC_BT2020_NCL ),
    MUX_ENUM_INT("bt2020ncl",   AVCOL_SPC_BT2020_NCL ),

    { AVMUXER_CODEC_PARAMS_PROFILE,"output h264 encode profile",MUX_OPT_TYPE_INT,{.i = FF_PROFILE_H264_MAIN}},
    MUX_ENUM_INT("baseline",    FF_PROFILE_H264_BASELINE),
    MUX_ENUM_INT("main",        FF_PROFILE_H264_MAIN),
    MUX_ENUM_INT("extended",    FF_PROFILE_H264_EXTENDED),
    MUX_ENUM_INT("high",        FF_PROFILE_H264_HIGH),
    MUX_ENUM_INT("high422",     FF_PROFILE_H264_HIGH_422),
    MUX_ENUM_INT("high422i",    FF_PROFILE_H264_HIGH_422_INTRA),
    MUX_ENUM_INT("high444",     FF_PROFILE_H264_HIGH_444),
    MUX_ENUM_INT("high444p",    FF_PROFILE_H264_HIGH_444_PREDICTIVE),
    MUX_ENUM_INT("high444i",    FF_PROFILE_H264_HIGH_444_INTRA),
    MUX_ENUM_INT("stereohigh",  FF_PROFILE_H264_STEREO_HIGH),
    MUX_ENUM_INT("stereohigh",  FF_PROFILE_H264_STEREO_HIGH),

    { AVMUXER_CODEC_PARAMS_PIXFMT ,"output pixel format",MUX_OPT_TYPE_INT ,{.i = AV_PIX_FMT_YUV420P}} ,
    MUX_ENUM_INT("yuv420p",         AV_PIX_FMT_YUV420P),
    MUX_ENUM_INT("yuyv422",         AV_PIX_FMT_YUYV422),
    MUX_ENUM_INT("yuv444p",         AV_PIX_FMT_YUV444P),
    MUX_ENUM_INT("yuv422p",         AV_PIX_FMT_YUV422P),
    MUX_ENUM_INT("yuv410p",         AV_PIX_FMT_YUV410P),
    MUX_ENUM_INT("yuv411p",         AV_PIX_FMT_YUV410P),
    MUX_ENUM_INT("yuva420p",        AV_PIX_FMT_YUVA420P),
    MUX_ENUM_INT("yuva444p10le",    AV_PIX_FMT_YUVA444P10LE),
    MUX_ENUM_INT("nv12",            AV_PIX_FMT_NV12),
    MUX_ENUM_INT("nv21",            AV_PIX_FMT_NV21),
    MUX_ENUM_INT("rgb24",           AV_PIX_FMT_RGB24),
    MUX_ENUM_INT("bgr24",           AV_PIX_FMT_BGR24),
    MUX_ENUM_INT("gray8",           AV_PIX_FMT_GRAY8),
    MUX_ENUM_INT("argb",            AV_PIX_FMT_ARGB),
};
#endif

class CodecOptionsSelector
{
public:
    static CodecOptionsSelector& instance() {
        static CodecOptionsSelector sel;
        return sel;
    }

    void parse_encode_params(const std::map<std::string , std::string>& param_str_map,H264EncodeParams& h264_params ){
#define SET_H264_ENCODE_PARAM_INT(var) \
    const auto& iter_##var = param_str_map.find(#var); \
    h264_params.var = get_option_number<int>( \
        #var, iter_##var != param_str_map.end() ? iter_##var->second.c_str() : nullptr)
#define CONVERT_H264_ENCODE_PARAM_INT(var, v_default) \
    const auto& iter_##var = param_str_map.find(#var); \
    h264_params.var = iter_##var != param_str_map.end() ? atoi(iter_##var->second.c_str()) :v_default

        SET_H264_ENCODE_PARAM_INT(colorspace);
        SET_H264_ENCODE_PARAM_INT(profile);
        SET_H264_ENCODE_PARAM_INT(out_pixfmt);
        SET_H264_ENCODE_PARAM_INT(encode_mode);
        CONVERT_H264_ENCODE_PARAM_INT(gopsize,12);
        CONVERT_H264_ENCODE_PARAM_INT(out_audio_sample_rate,16000);
        CONVERT_H264_ENCODE_PARAM_INT(thread_count,1);
        CONVERT_H264_ENCODE_PARAM_INT(codec_type,AV_CODEC_ID_H264);
    }

    template <typename _Tp>
    _Tp get_option_number(const char* name,const char* val){
        int index =0;
        for(;index < option_len_;++index){
            if(k_codec_context_params[index].type != MUX_OPT_TYPE_CONST
            && 0 == FPlatformString::Stricmp(name,k_codec_context_params[index].name)){
                break;
            }
        }

        MuxerOption opt;
        _Tp out;
        opt.default_val = k_codec_context_params[index].default_val;
        opt.type = k_codec_context_params[index].type;
        if(val){
            for(int i = index +1;i < option_len_;i++){
				if (0 == FPlatformString::Stricmp(k_codec_context_params[i].name, val)) {
                    void* dst = &out;
                    get_val_number(k_codec_context_params[i], opt.type,&out);
                    return out;
                }
                if(k_codec_context_params[i].type != MUX_OPT_TYPE_CONST)
                    break;
            }
        }

        get_val_number(opt,opt.type,&out);
        return out;
    }

protected:
    void get_val_number(const MuxerOption& option, MuxerOptionType type,void* dst){
        switch (type)
        {
        case MUX_OPT_TYPE_BOOL:
            *(bool*)dst = option.default_val.b;
            break;
        case MUX_OPT_TYPE_INT:
            *(int*)dst = option.default_val.i;    
            break;    
        case MUX_OPT_TYPE_INT64:
            *(int64_t*)dst = option.default_val.i64;
            break;
        case MUX_OPT_TYPE_UINT64:
            *(uint64_t*)dst =option.default_val.ui64;
            break;
        case MUX_OPT_TYPE_FLOAT:
            *(float*)dst = option.default_val.fp;
            break;
        case MUX_OPT_TYPE_DOUBLE:
            *(double*)dst = option.default_val.dbl;
            break;
        case MUX_OPT_TYPE_STRING:
            *(const char**) dst = option.default_val.str;
        default:
            break;
        }
    }

private:
    CodecOptionsSelector(){
        option_len_ = sizeof(k_codec_context_params) / sizeof(MuxerOption);
    }    

    CodecOptionsSelector(CodecOptionsSelector &other){}
    CodecOptionsSelector& operator = (CodecOptionsSelector& other) { return other; }

private:
    int option_len_;

};

}

#endif // __CODEC_OPTIONS_HPP__

#pragma warning(pop)