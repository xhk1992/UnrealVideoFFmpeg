// Copyright Epic Games, Inc. All Rights Reserved.

#include "VideoStream.h"
#include "UFFmpeg.h"
#include "UMuxer.h"
#include "IOpenCVHelperModule.h"

#define LOCTEXT_NAMESPACE "FVideoStreamModule"

bool FVideoStreamModule::Initialized = false;

void FVideoStreamModule::StartupModule()
{
	if (!Initialized)
	{
		FUFFmpegModule& VideoStremFFmpeg = FModuleManager::Get().LoadModuleChecked<FUFFmpegModule>("UFFmpeg");
		FUMuxerModule& VideoStreamMuxer = FModuleManager::Get().LoadModuleChecked<FUMuxerModule>("UMuxer");
		IOpenCVHelperModule& MyOpenCVHelper = FModuleManager::Get().LoadModuleChecked<IOpenCVHelperModule>("OpenCVHelper");
		Initialized = true;
	}
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FVideoStreamModule::ShutdownModule()
{
	if (!Initialized)
	{
		FModuleManager::Get().UnloadModule("UFFmpeg",true);
		FModuleManager::Get().UnloadModule("UMuxer", true);
		FModuleManager::Get().UnloadModule("MyOpenCVHelper", true);
		Initialized = false;
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVideoStreamModule, VideoStream)

DEFINE_LOG_CATEGORY(LogVideoStream);