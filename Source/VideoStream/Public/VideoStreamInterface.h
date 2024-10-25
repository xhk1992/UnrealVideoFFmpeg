// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VideoStreamInterface.generated.h"

struct FVideoStreamOptions;
class UVideoStreamSubsystem;
// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UVideoStreamInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class VIDEOSTREAM_API IVideoStreamInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual bool Encode_Video_Frame(void* VideoDataPtr, FIntPoint BufferSize, FIntPoint TargetSize); //编码视频帧接口

	virtual bool Encode_Audio_Frame(const float* AudioData, int32 NumSamples, int32 NumChannels, int32 SampleRate);

	virtual bool Encode_Finish(); //编码结束

	virtual bool Encode_Video_FrameRHI(const FTexture2DRHIRef& FrameBuffer);

	virtual bool Init(UWorld* World, FVideoStreamOptions Param, UVideoStreamSubsystem* InSubsystem); //初始化接口

	virtual int32 GetVideoFrameNum() { return 0; } //获取编码的视频帧

	int32 video_index = 0;
};
