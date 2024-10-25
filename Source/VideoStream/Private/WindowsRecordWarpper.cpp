// Fill out your copyright notice in the Description page of Project Settings.


#include "WindowsRecordWarpper.h"
#include "VideoStreamSubsystem.h"

UWindowsRecordWarpper::~UWindowsRecordWarpper()
{

}

bool UWindowsRecordWarpper::Encode_Video_Frame(void* VideoDataPtr, FIntPoint BufferSize, FIntPoint TargetSize)
{
	return true;
}

bool UWindowsRecordWarpper::Encode_Audio_Frame(const float* AudioData, int32 NumSamples, int32 NumChannels, int32 SampleRate)
{
	return true;
}

bool UWindowsRecordWarpper::Encode_Finish()
{
	return true;
}

bool UWindowsRecordWarpper::SetCurrentAudioTime(uint8_t* time)
{
	return true;
}

void UWindowsRecordWarpper::UpdateLolStride(uint32 Stride)
{

}

bool UWindowsRecordWarpper::Init(UWorld* World, FVideoStreamOptions Param, UVideoStreamSubsystem* InSubsystem)
{
	IVideoRecordingSystem* Video = IPlatformFeaturesModule::Get().GetVideoRecordingSystem();
	FVideoRecordingParameters WinVideoRecordParam;
	WinVideoRecordParam.bAutoStart = true;
	WinVideoRecordParam.RecordingLengthSeconds = Param.TotalFrameNum * Param.FPS;


	//Video->EnableRecording(true);
	Video->NewRecording(*Param.OutFileName, WinVideoRecordParam);
	Video->StartRecording();

	return true;
}
