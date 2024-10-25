// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VideoStreamInterface.h"
#include "PlatformFeatures.h"
#include "VideoRecordingSystem.h"
#include "AudioEncoder.h"
#include "VideoEncoder.h"
#include "MediaPacket.h"
#include "RHI.h"
#include "RHIResources.h"
#include "Engine/GameEngine.h"
#include "AudioEncoderFactory.h"
#include "UEEncoderWarpper.generated.h"


/**
 * 
 */
class FVideoStreamWmfMp4Writer;

UCLASS()
class VIDEOSTREAM_API UUEEncoderWarpper :public UObject , public IVideoStreamInterface,public AVEncoder::IAudioEncoderListener
{
public:

	GENERATED_BODY()

	~UUEEncoderWarpper();

	virtual bool Encode_Audio_Frame(const float* AudioData, int32 NumSamples, int32 NumChannels, int32 SampleRate) override;

	virtual bool Encode_Video_FrameRHI(const FTexture2DRHIRef& FrameBuffer) override;

	virtual bool Encode_Finish();

	virtual void UpdateLolStride(uint32 Stride);

	virtual bool Init(UWorld* World, FVideoStreamOptions Param, UVideoStreamSubsystem* InSubsystem);

	void OnEncodedAudioFrame(const AVEncoder::FMediaPacket& Packet) override;

	virtual int32 GetVideoFrameNum() override;

private:

	void OnEncodedVideoFrame(uint32 LayerIndex, const TSharedPtr<AVEncoder::FVideoEncoderInputFrame> Frame, const AVEncoder::FCodecPacket& Packet);

	TSharedPtr<AVEncoder::FVideoEncoderInputFrame> ObtainInputFrame();

	FCriticalSection ListenersCS;

	AVEncoder::FVideoConfig VideoConfig;

	AVEncoder::FAudioConfig AudioConfig;

	TUniquePtr<AVEncoder::FVideoEncoder> VideoEncoder;

	TUniquePtr<AVEncoder::FAudioEncoder> AudioEncoder;

	TSharedPtr<AVEncoder::FVideoEncoderInput> VideoEncoderInput;

	FCriticalSection AudioProcessingCS;
	FCriticalSection VideoProcessingCS;
	FCriticalSection OnPacketsCS;

	uint64 NumCapturedFrames = 0;

	FTimespan AudioStartTime = 0;

	FTimespan VideoStartTime = 0;

	TMap<TSharedPtr<AVEncoder::FVideoEncoderInputFrame>, FTexture2DRHIRef> BackBuffers;

	void CopyTexture(const FTexture2DRHIRef& SourceTexture, FTexture2DRHIRef& DestinationTexture) const;

	double AudioClock = 0;

	FThreadSafeBool bIsRecording = false;
	FVideoStreamWmfMp4Writer* Mp4Writer = nullptr;
	DWORD AudioStreamIndex = 0;
	DWORD VideoStreamIndex = 0;

	class UVideoStreamSubsystem* System;

	int32 VideoFrameNum = 0;
};
