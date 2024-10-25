// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


class UVideoStreamSubsystem;
struct FVideoStreamOptions;

/**
 * 
 */

//抽象类

class VIDEOSTREAM_API IVideoStreamInterface_Discarded
{
public:
	//virtual ~IVideoStreamInterface() {};

	//virtual bool Encode_Video_Frame(void* VideoDataPtr, FIntPoint BufferSize, FIntPoint TargetSize) = 0; //编码视频帧接口

	//virtual bool Encode_Audio_Frame(float* audio) = 0;//编码音频帧接口

	//virtual bool Encode_Finish() = 0; //编码结束

	//virtual bool Init(UWorld* World, FVideoStreamOptions Param, UVideoStreamSubsystem* InSubsystem) = 0; //初始化接口
};
