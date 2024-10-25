// Fill out your copyright notice in the Description page of Project Settings.


#include "NamedPipeWarpper.h"
#include "VideoStreamSubsystem.h"

UNamedPipeWarpper::~UNamedPipeWarpper()
{

}

const FString AudioPipeName = TEXT("\\\\.\\pipe\\audio_pipe");

const FString VideoPipeName = TEXT("\\\\.\\pipe\\video_pipe");

bool UNamedPipeWarpper::Encode_Video_Frame(void* VideoDataPtr, FIntPoint BufferSize, FIntPoint TargetSize)
{
	int64 t1 = FDateTime::Now().GetTicks() / 10000;

	char info[12];
	
	int w = BufferSize.X;
	int h = BufferSize.Y;
	int32 Size = w * h * 4 * 4;

	FMemory::Memcpy(&info[0], (char*)&w, 4);
	FMemory::Memcpy(&info[4], (char*)&h, 4);
	FMemory::Memcpy(&info[8], (char*)&Size, 4);

	UE_LOG(LogVideoStream, Log, TEXT("sss :%s %d"), info,sizeof(int));

	VideoPipe.WriteBytes(12, &info);

	int SendNum = BufferSize.X * BufferSize.Y * 4;
	UE_LOG(LogVideoStream, Log, TEXT("Video Fram SendNum :%d"),SendNum);
	VideoPipe.WriteBytes(SendNum, VideoDataPtr);

	int64 t2 = FDateTime::Now().GetTicks() / 10000;

	UE_LOG(LogVideoStream, Log, TEXT("Encode_Video_Frame Cost :%lld"), t2 - t1);
	
	return true;
}

bool UNamedPipeWarpper::Encode_Audio_Frame(const float* AudioData, int32 NumSamples, int32 NumChannels, int32 SampleRate)
{
	int64 t1 = FDateTime::Now().GetTicks() / 10000;

	int32 size = 2048 * sizeof(float);

	AudioPipe.WriteBytes(sizeof(int32), &size);

	AudioPipe.WriteBytes(size, AudioData);

	int64 t2 = FDateTime::Now().GetTicks() / 10000;

	UE_LOG(LogVideoStream, Log, TEXT("Encode_Audio_Frame Cost :%lld"), t2 - t1);

	return true;
}

bool UNamedPipeWarpper::Encode_Finish()
{
	VideoPipe.Destroy();
	AudioPipe.Destroy();
	return true;
}

bool UNamedPipeWarpper::SetCurrentAudioTime(uint8_t* time)
{
	return true;
}

void UNamedPipeWarpper::UpdateLolStride(uint32 Stride)
{

}

bool UNamedPipeWarpper::Init(UWorld* World, FVideoStreamOptions Param, UVideoStreamSubsystem* InSubsystem)
{
	bool bSuccess = true;

	bSuccess = VideoPipe.Create(VideoPipeName, false, false);

	if (!bSuccess)
	{
		UE_LOG(LogVideoStream, Error, TEXT("Creat Video Pipe Failed Name:%s"), *VideoPipeName);
		return false;
	}

	bSuccess = AudioPipe.Create(AudioPipeName, false, false);

	if (!bSuccess)
	{
		UE_LOG(LogVideoStream, Error, TEXT("Creat Audio Pipe Failed Name:%s"), *AudioPipeName);
		return false;
	}

	return true;
}
