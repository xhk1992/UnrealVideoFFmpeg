// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"


class FUFFmpegModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
private:
	static void* LoadLibrary(const  FString& name, const FString& version);
	static void* AVUtilLibrary;
	static void* SWResampleLibrary;
	static void* AVCodecLibrary;
	static void* SWScaleLibrary;
	static void* AVFormatLibrary;
	static void* PostProcLibrary;
	static void* AVFilterLibrary;
	static void* AVDeviceLibrary;

	static bool Initialized;
};

DECLARE_LOG_CATEGORY_EXTERN(LogVideoStreamFFmpeg, Log, Log);
