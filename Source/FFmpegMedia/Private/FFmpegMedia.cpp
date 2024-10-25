// Copyright Epic Games, Inc. All Rights Reserved.

#include "FFmpegMedia.h"
//#include "Core.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Templates/SharedPointer.h"

#include "FFmpegMediaPlayer.h"

extern  "C" {
	#include "libavformat/avformat.h"
}

DEFINE_LOG_CATEGORY(LogFFmpegMedia);

#define LOCTEXT_NAMESPACE "FFFmpegMediaModule"

/**
 * FFmpegMedia模块
 */
class FFFmpegMediaModule : public IFFmpegMediaModule
{
public:
	/**
	 * 创建播放器
	 */
	virtual TSharedPtr<IMediaPlayer, ESPMode::ThreadSafe> CreatePlayer(IMediaEventSink& EventSink) {
		if (!Initialized)
		{
			UE_LOG(LogFFmpegMedia, Error, TEXT("FFmpegMediaModule not load, create FFmpegMediaPlayer failed"));
			return nullptr;
		}

		UE_LOG(LogFFmpegMedia, Log, TEXT("create FFmpegMediaPlayer success"));
		UE_LOG(LogFFmpegMedia, Log, TEXT("create FFmpegMediaPlayer success"));
		return MakeShareable(new FFmpegMediaPlayer(EventSink));
	}

	/**
	 * 获取平台支持的文件扩展名
	 */
	virtual TArray<FString> GetSupportedFileExtensions() {
		TMap<FString, FString> extensionMap;
		void* oformat_opaque = NULL;
		const AVOutputFormat* oformat = av_muxer_iterate(&oformat_opaque);
		while (oformat) {
			FString ext = oformat->extensions;
			if (!ext.IsEmpty()) {
				TArray<FString> supportedExts;
				//分割成数组
				ext.ParseIntoArray(supportedExts, TEXT(","));
				for (const FString& s : supportedExts) {
					if (extensionMap.Contains(s)) {
						extensionMap[s] += TEXT(",") + FString(oformat->name);
					}
					else {
						extensionMap.Add(s, oformat->name);
					}
				}
			}
			oformat = av_muxer_iterate(&oformat_opaque);
		}

		TArray<FString> extensions;
		extensionMap.GetKeys(extensions);
		return extensions;
	}

	/**
	 * 获取平台支持的URL
	 */
	virtual TArray<FString> GetSupportedUriSchemes() {
		void* opaque = NULL;
		//0: 输入协议, 1: 输出协议， 这里获取输入协议
		const char* name = avio_enum_protocols(&opaque, 0);
		TArray<FString> protocols;
		while (name) {
			protocols.Add(name);
			name = avio_enum_protocols(&opaque, 0);
		}
		return protocols;
	}

	/** IModuleInterface implementation */
	virtual void StartupModule() override {
		//开始d动态加载ffmpeg dll文件
		FUFFmpegModule& VideoStremFFmpeg = FModuleManager::Get().LoadModuleChecked<FUFFmpegModule>("UFFmpeg");
		av_register_all(); //ffmpeg注册组件，ffmpeg5中已经不存在
		avformat_network_init(); //初始化ffmpeg网络库
		UE_LOG(LogFFmpegMedia, Display, TEXT("FFmpeg Version: %s"), UTF8_TO_TCHAR(av_version_info()));
		UE_LOG(LogFFmpegMedia, Display, TEXT("FFmpeg AVCodec version: %d.%d.%d"), LIBAVFORMAT_VERSION_MAJOR, LIBAVFORMAT_VERSION_MINOR, LIBAVFORMAT_VERSION_MICRO);
		UE_LOG(LogFFmpegMedia, Display, TEXT("FFmpeg license: %s"), UTF8_TO_TCHAR(avformat_license()));
		av_log_set_level(AV_LOG_INFO);
		av_log_set_flags(AV_LOG_SKIP_REPEATED);
		av_log_set_callback(&log_callback);
		Initialized = true;
	}
	virtual void ShutdownModule() override {
		if (!Initialized)
		{
			return;
		}

		FModuleManager::Get().UnloadModule("UFFmpeg",true);
		Initialized = false;
	}
public:

	/** Virtual destructor. */
	virtual ~FFFmpegMediaModule() { }

	static void  log_callback(void*, int level, const char* format, va_list arglist) {

		///return;
		char buffer[2048];
#if PLATFORM_WINDOWS
		vsprintf_s(buffer, 2048, format, arglist);
#else
		vsnprintf(buffer, 2048, format, arglist);
#endif
		FString str = TEXT("FFMPEG - ");
		str += buffer;

		switch (level) {
		case AV_LOG_TRACE:
			UE_LOG(LogFFmpegMedia, VeryVerbose, TEXT("%s"), *str);
			break;
		case AV_LOG_DEBUG:
			UE_LOG(LogFFmpegMedia, VeryVerbose, TEXT("%s"), *str);
			break;
		case AV_LOG_VERBOSE:
			UE_LOG(LogFFmpegMedia, Verbose, TEXT("%s"), *str);
			break;
		case AV_LOG_INFO:
			UE_LOG(LogFFmpegMedia, Display, TEXT("%s"), *str);
			break;
		case AV_LOG_WARNING:
			UE_LOG(LogFFmpegMedia, Warning, TEXT("%s"), *str);
			break;
		case AV_LOG_ERROR:
			UE_LOG(LogFFmpegMedia, Error, TEXT("%s"), *str);
			break;
		case AV_LOG_FATAL:
			UE_LOG(LogFFmpegMedia, Fatal, TEXT("%s"), *str);
			break;
		default:
			UE_LOG(LogFFmpegMedia, Display, TEXT("%s"), *str);
		}
	}

private:
	/** 是否初始化 */
	bool Initialized;

	void* AVUtilLibrary;
	void* SWResampleLibrary;
	void* AVCodecLibrary;
	void* SWScaleLibrary;
	void* AVFormatLibrary;
	void* PostProcLibrary;
	void* AVFilterLibrary;
	void* AVDeviceLibrary;
};

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FFFmpegMediaModule, FFmpegMedia)
