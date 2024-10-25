// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VideoStreamInterface.h"
#include "PlatformFeatures.h"
#include "VideoRecordingSystem.h"
#include "WindowsRecordWarpper.generated.h"

/**
 * 
 */

UCLASS()
class VIDEOSTREAM_API UWindowsRecordWarpper :public UObject , public IVideoStreamInterface
{
public:

	GENERATED_BODY()

	~UWindowsRecordWarpper();

	virtual bool Encode_Video_Frame(void* VideoDataPtr, FIntPoint BufferSize, FIntPoint TargetSize) override;

	virtual bool Encode_Audio_Frame(const float* AudioData, int32 NumSamples, int32 NumChannels, int32 SampleRate) override;

	virtual bool Encode_Finish() override;

	bool SetCurrentAudioTime(uint8_t* time);

	virtual void UpdateLolStride(uint32 Stride);

	virtual bool Init(UWorld* World, FVideoStreamOptions Param, UVideoStreamSubsystem* InSubsystem) override;
};
