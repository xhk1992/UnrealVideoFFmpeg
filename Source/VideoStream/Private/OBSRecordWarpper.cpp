// Fill out your copyright notice in the Description page of Project Settings.


#include "OBSRecordWarpper.h"
#include "VideoStreamSubsystem.h"

UOBSRecordWarpper::~UOBSRecordWarpper()
{
	
}

bool UOBSRecordWarpper::Encode_Video_Frame(void* VideoDataPtr, FIntPoint BufferSize, FIntPoint TargetSize)
{
	return true;
}

bool UOBSRecordWarpper::Encode_Audio_Frame(const float* AudioData, int32 NumSamples, int32 NumChannels, int32 SampleRate)
{
	return true;
}

bool UOBSRecordWarpper::Encode_Finish()
{
	HttpRequest->SetURL(TEXT("http://127.0.0.1:4445/call/StopRecord"));
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/x-www-form-urlencoded"));
	HttpRequest->ProcessRequest();
	return true;
}

bool UOBSRecordWarpper::SetCurrentAudioTime(uint8_t* time)
{
	return true;
}

void UOBSRecordWarpper::UpdateLolStride(uint32 Stride)
{
	
}

bool UOBSRecordWarpper::Init(UWorld* World, FVideoStreamOptions Param, UVideoStreamSubsystem* InSubsystem)
{
	HttpRequest->SetURL(TEXT("http://127.0.0.1:4445/call/StartRecord"));
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/x-www-form-urlencoded"));
	HttpRequest->ProcessRequest();
	return true;
}
