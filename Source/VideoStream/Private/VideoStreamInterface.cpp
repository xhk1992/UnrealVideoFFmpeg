// Fill out your copyright notice in the Description page of Project Settings.


#include "VideoStreamInterface.h"
#include "VideoStreamSubsystem.h"

// Add default functionality here for any IVideoStreamInterface functions that are not pure virtual.

bool IVideoStreamInterface::Encode_Video_Frame(void* VideoDataPtr, FIntPoint BufferSize, FIntPoint TargetSize)
{
	return true;
}

bool IVideoStreamInterface::Encode_Video_FrameRHI(const FTexture2DRHIRef& FrameBuffer)
{
	return true;
}

bool IVideoStreamInterface::Encode_Audio_Frame(const float* AudioData, int32 NumSamples, int32 NumChannels, int32 SampleRate)
{
	return true;
}

bool IVideoStreamInterface::Encode_Finish()
{
	return true;
}

bool IVideoStreamInterface::Init(UWorld* World, FVideoStreamOptions Param, UVideoStreamSubsystem* InSubsystem)
{
	return true;
}
