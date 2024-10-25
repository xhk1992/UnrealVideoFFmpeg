// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VideoStreamInterface.h"
#include "HttpModule.h"
#include "OBSRecordWarpper.generated.h"

/**
 * 
 */

UCLASS()
class VIDEOSTREAM_API UOBSRecordWarpper :public UObject , public IVideoStreamInterface
{
public:

	GENERATED_BODY()

	~UOBSRecordWarpper();

	virtual bool Encode_Video_Frame(void* VideoDataPtr, FIntPoint BufferSize, FIntPoint TargetSize) override;

	virtual bool Encode_Audio_Frame(const float* AudioData, int32 NumSamples, int32 NumChannels, int32 SampleRate) override;

	virtual bool Encode_Finish();

	bool SetCurrentAudioTime(uint8_t* time);

	virtual void UpdateLolStride(uint32 Stride);

	virtual bool Init(UWorld* World, FVideoStreamOptions Param, UVideoStreamSubsystem* InSubsystem) override;

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
};
