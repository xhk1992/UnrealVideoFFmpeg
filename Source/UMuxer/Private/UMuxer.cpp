// Copyright Epic Games, Inc. All Rights Reserved.
#include "UMuxer.h"

#define LOCTEXT_NAMESPACE "FUMuxerModule"

void FUMuxerModule::StartupModule()
{
	UE_LOG(LogUMuxer, Warning, TEXT("UMuxer: Log Started"));
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FUMuxerModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUMuxerModule, UMuxer)

DEFINE_LOG_CATEGORY(LogUMuxer)