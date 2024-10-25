// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "VideoStream.h"
#include "AVMuxerWarpper.h"
#include "CustomFrameGrabber.h"
#include "VideoStreamInterface.h"
#include "UEEncoderWarpper.h"
#include "VideoStreamSubsystem.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVideoStreamMessage,bool,bSuccess,FString, ErrorText);

USTRUCT(BlueprintType)
struct FVideoStreamOptions
{
	GENERATED_BODY()
	// Out file name will be auto generated if empty
	UPROPERTY(BlueprintReadWrite, Category = "VideoStream", EditAnywhere)
	FString OutFileName;
	UPROPERTY(BlueprintReadWrite, Category = "VideoStream", EditAnywhere)
	FString VideoFilter;
	UPROPERTY(BlueprintReadWrite, Category = "VideoStream", EditAnywhere)
	bool UseGPU = true;
	UPROPERTY(BlueprintReadWrite, Category = "VideoStream", EditAnywhere)
	int FPS = 30;
	UPROPERTY(BlueprintReadWrite, Category = "VideoStream", EditAnywhere)
	int32 VideoBitRate = 8000000;
	UPROPERTY(BlueprintReadWrite, Category = "VideoStream", EditAnywhere)
	float AudioDelay = 0.f;
	UPROPERTY(BlueprintReadWrite, Category = "VideoStream", EditAnywhere)
	float SoundVolume = 1.f;
	UPROPERTY(BlueprintReadWrite, Category = "VideoStream", EditAnywhere)
	int32 Width = 0;
	UPROPERTY(BlueprintReadWrite, Category = "VideoStream", EditAnywhere)
	int32 Height = 0;
	UPROPERTY(BlueprintReadWrite, Category = "VideoStream", EditAnywhere)
	int TotalFrameNum = 0;
};

UENUM()
enum EVideoStreamType
{
	FFmpeg = 0,
	NamePipe,
	Obs,
	XGP,
	UEAVEncoder,
};

class UVideoStreamSubsystemWarpper : public ICustomFramePayload
{
public:
	UVideoStreamSubsystemWarpper(const class UVideoStreamSubsystem* subsystem, uint32 num)
	{
		Subsystem = subsystem;
		InFrameNum = num;
	}

	~UVideoStreamSubsystemWarpper() {};

protected:
	virtual bool OnFrameReady_RenderThread(FColor* ColorBuffer, FIntPoint BufferSize, FIntPoint TargetSize) const;

public:
	const class UVideoStreamSubsystem* Subsystem;
	uint32 InFrameNum;
};

/**
 * 
 */
UCLASS()
class VIDEOSTREAM_API UVideoStreamSubsystem : public UGameInstanceSubsystem, public ISubmixBufferListener
{
	GENERATED_BODY()

public :
	UVideoStreamSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "VideoStream")
	void StartCapture(FVideoStreamOptions Options);

	UFUNCTION(BlueprintCallable, Category = "VideoStream")
	void WarmUp(EVideoStreamType Type = EVideoStreamType::FFmpeg);

	/// <summary>
	/// 
	/// </summary>
	/// <param name="Options"></param>
	/// <param name="Type">0:avmuxer 1:name pipe 2:obs 3:windowsrecord 4 ueencoder</param>
	UFUNCTION(BlueprintCallable, Category = "VideoStream")
	void StartCaptureWithType(FVideoStreamOptions Options, EVideoStreamType Type = EVideoStreamType::FFmpeg);
	
	UFUNCTION(BlueprintCallable, Category = "VideoStream")
	void EndCapture();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VideoStream")
	bool IsRecording();
	
	UFUNCTION()
	void CreatFrameGetter(int w,int h);

	UFUNCTION()
	void HandleGameInstanceStart(UGameInstance* GameInstance);

	UPROPERTY(BlueprintAssignable, Category = "VideoStream")
	FOnVideoStreamMessage OnMessage;

	UFUNCTION()
	void RecvWarpperMsg(bool bSuccess, FString Msg);

	virtual void OnNewSubmixBuffer(const USoundSubmix* OwningSubmix, float* AudioData, int32 NumSamples, int32 NumChannels, const int32 SampleRate, double AudioClock) override;


	
private:

	TArray<IVideoStreamInterface*> ImplementsList;

	EVideoStreamType WarpperType = EVideoStreamType::FFmpeg ; //默认为0 指向avmuxer 1 则指向namedpipe

	FCustomFrameGrabber* CustomFrameGrabber = nullptr;

	FDelegateHandle GameInstanceStartHandle;

	void RegisterAudioListener();

	void UnRegisterAudioListener();

	FTSTicker::FDelegateHandle TickDelegateHandle;

	bool AddTickTime(float time);

	float Ticktime = 0.0f; //自增时间

	float Video_Tick_Time = 0.0f; //每帧间隔

	int Capture_Video_Index = 0;

	int TotalFrameNum = 0;

	int Width = 0;

	int Height = 0;

public:
	FCustomFrameGrabber* GetGrabber() { return CustomFrameGrabber; }

	IVideoStreamInterface* CurrentImplements = nullptr;

	void OnFrameReady(FColor* ColorBuffer, FIntPoint BufferSize, FIntPoint TargetSize);

	FThreadSafeBool bIsRecording = false;

private:

	void OnFrameCaptured(SWindow& SlateWindow, const FTexture2DRHIRef& FrameBuffer);

	void OnViewportRendered(FViewport* Viewport);

	bool bShouldEnd = false;

	void EndCaptureInternal();

	void OnFrameCapturedInternal(FColor* ColorBuffer, FIntPoint BufferSize, FIntPoint TargetSize);

	bool CanEncode();

	FCriticalSection VideoBufferMutex;

	bool bHasWarmUp = false;

	//超时判断
	float LastTickTime = 0.0f;
	int	LastFrameNum = 0;
};
