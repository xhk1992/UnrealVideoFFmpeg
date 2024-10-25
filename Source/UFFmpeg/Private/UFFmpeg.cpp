// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "UFFmpeg.h"
#include "Interfaces/IPluginManager.h"
#include "Core.h"

#include<stdio.h>
#include<stdlib.h>

extern  "C" {
#include "libavformat/avformat.h"
}

#define LOCTEXT_NAMESPACE "FUffmpegModule"

bool FUFFmpegModule::Initialized = false;
void* FUFFmpegModule::AVUtilLibrary = nullptr;
void* FUFFmpegModule::SWResampleLibrary = nullptr;
void* FUFFmpegModule::AVCodecLibrary = nullptr;
void* FUFFmpegModule::AVFormatLibrary = nullptr;
void* FUFFmpegModule::SWScaleLibrary = nullptr;
void* FUFFmpegModule::PostProcLibrary = nullptr;
void* FUFFmpegModule::AVFilterLibrary = nullptr;
void* FUFFmpegModule::AVDeviceLibrary = nullptr;


void FUFFmpegModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	if (Initialized)
	{
		return;
	}
	AVUtilLibrary = LoadLibrary(TEXT("avutil"), TEXT("56"));
	SWResampleLibrary = LoadLibrary(TEXT("swresample"), TEXT("3"));
	AVCodecLibrary = LoadLibrary(TEXT("avcodec"), TEXT("58"));
	AVFormatLibrary = LoadLibrary(TEXT("avformat"), TEXT("58"));
	SWScaleLibrary = LoadLibrary(TEXT("swscale"), TEXT("5"));
	PostProcLibrary = LoadLibrary(TEXT("postproc"), TEXT("55"));
	AVFilterLibrary = LoadLibrary(TEXT("avfilter"), TEXT("7"));
	AVDeviceLibrary = LoadLibrary(TEXT("avdevice"), TEXT("58"));
	Initialized = true;
}

void FUFFmpegModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	if (!Initialized)
	{
		return;
	}
	if (AVDeviceLibrary) FPlatformProcess::FreeDllHandle(AVDeviceLibrary);
	if (AVFilterLibrary) FPlatformProcess::FreeDllHandle(AVFilterLibrary);
	if (PostProcLibrary) FPlatformProcess::FreeDllHandle(PostProcLibrary);
	if (SWScaleLibrary) FPlatformProcess::FreeDllHandle(SWScaleLibrary);
	if (AVFormatLibrary) FPlatformProcess::FreeDllHandle(AVFormatLibrary);
	if (AVCodecLibrary) FPlatformProcess::FreeDllHandle(AVCodecLibrary);
	if (SWResampleLibrary) FPlatformProcess::FreeDllHandle(SWResampleLibrary);
	if (AVUtilLibrary) FPlatformProcess::FreeDllHandle(AVUtilLibrary);

	Initialized = false;
}

void* FUFFmpegModule::LoadLibrary(const FString& name, const FString& version)
{

	FString BaseDir = IPluginManager::Get().FindPlugin("VideoStream")->GetBaseDir();

	FString LibDir;
	FString extension;
	FString prefix;
	FString separator;
#if PLATFORM_MAC
	LibDir = FPaths::Combine(*BaseDir, TEXT("ThirdParty/ffmpeg/lib/osx"));
	extension = TEXT(".dylib");
	prefix = "lib";
	separator = ".";
#elif PLATFORM_WINDOWS
	extension = TEXT(".dll");
	prefix = "";
	separator = "-";

	FString ProjectDir = FPaths::ProjectDir();
#if WITH_EDITOR
	LibDir = FPaths::Combine(*BaseDir, TEXT("ThirdParty/ffmpeg/bin/vs/x64"));
#else
	LibDir = FPaths::Combine(*ProjectDir, TEXT("Binaries/Win64"));
#endif
#endif
	if (!LibDir.IsEmpty()) {
		FString LibraryPath = FPaths::Combine(*LibDir, prefix + name + separator + version + extension);
		return FPlatformProcess::GetDllHandle(*LibraryPath);
	}
	return nullptr;

}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUFFmpegModule, UFFmpeg)

DEFINE_LOG_CATEGORY(LogVideoStreamFFmpeg);