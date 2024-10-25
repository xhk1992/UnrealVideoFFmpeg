// Fill out your copyright notice in the Description page of Project Settings.


#include "VideoStreamSubsystem.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "GameFramework/PlayerController.h"

#include "PixelCaptureBufferFormat.h"
#include "PixelCaptureOutputFrameRHI.h"

#include "PlatformFeatures.h"
#include "VideoRecordingSystem.h"



UVideoStreamSubsystem::UVideoStreamSubsystem()
	: UGameInstanceSubsystem()
{
}

void UVideoStreamSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	//初始化一个包装类

	ImplementsList.Add(NewObject<UAVMuxerWarpper>());

	ImplementsList.Add(NewObject<UNamedPipeWarpper>());

	ImplementsList.Add(NewObject<UOBSRecordWarpper>());

	ImplementsList.Add(NewObject<UWindowsRecordWarpper>());

	ImplementsList.Add(NewObject<UUEEncoderWarpper>());

	for(auto& i : ImplementsList)
	{
		i->_getUObject()->AddToRoot();
	}
	
	GameInstanceStartHandle = FWorldDelegates::OnStartGameInstance.AddUObject(this, &UVideoStreamSubsystem::HandleGameInstanceStart);

	TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UVideoStreamSubsystem::AddTickTime));
}

void UVideoStreamSubsystem::HandleGameInstanceStart(UGameInstance* GameInstance)
{
	FSceneViewport* VP = GameInstance->GetGameViewportClient()->GetGameViewport();

	if (VP)
	{
		CreatFrameGetter(VP->GetSizeXY().X, VP->GetSizeXY().Y);
		VP->SetOnSceneViewportResizeDel(FOnSceneViewportResize::CreateLambda([this](FVector2D newViewportSize) {
			if (newViewportSize.X > 0 && newViewportSize.Y > 0)
			{
				CreatFrameGetter((int)newViewportSize.X, (int)newViewportSize.Y);
			}
			}));
	}
	else
	{
		UE_LOG(LogVideoStream, Error, TEXT("VP Is NULL in HandleGameInstanceStart"));
	}


}

void UVideoStreamSubsystem::Deinitialize()
{
	//关闭 删除
	for (auto& i : ImplementsList)
	{
		i->_getUObject()->RemoveFromRoot();
	}
	ImplementsList.Empty();

	if (CustomFrameGrabber != nullptr && CustomFrameGrabber->IsCapturingFrames())
	{
		CustomFrameGrabber->StopCapturingFrames();
		CustomFrameGrabber->Shutdown();
	}

	FWorldDelegates::OnStartGameInstance.Remove(GameInstanceStartHandle);

	FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);

	Super::Deinitialize();
}

void UVideoStreamSubsystem::StartCapture(FVideoStreamOptions Options)
{
	UE_LOG(LogVideoStream, Log, TEXT("Start Video Capture Total Num [%d]"),Options.TotalFrameNum);
	if (bIsRecording)
		return;
	
	if(!bHasWarmUp)
		WarmUp(WarpperType);

	Ticktime = 0;

	if (WarpperType < ImplementsList.Num() && ImplementsList[WarpperType] != nullptr)
	{
		if (ImplementsList[WarpperType]->Init(GetGameInstance()->GetWorld(), Options, this) == false)
		{
			//初始化不成功
			UE_LOG(LogVideoStream, Log, TEXT("AVMuxerWarpper Init Failed"));
			OnMessage.Broadcast(false, TEXT("AVMuxerWarpper Init Failed"));
			return;
		}
		else
		{
			//初始化成功并保存当前的实现指针
			CurrentImplements = ImplementsList[WarpperType];
		}
		// 注册音频接收回调
		RegisterAudioListener();
		bIsRecording = true;
		Capture_Video_Index = 0;
		Video_Tick_Time = 1.0f / (float)Options.FPS;

		TotalFrameNum = Options.TotalFrameNum;

		Width = Options.Width;
		Height = Options.Height;
		//4k分辨率 使用fxaa 其他使用taa 仅录屏
		if (Width == 3840 && Height == 2160)
		{
			//GEngine->Exec(GetWorld(),TEXT("r.AntiAliasingMethod 1"));
		}
		else
		{
			//GEngine->Exec(GetWorld(), TEXT("r.AntiAliasingMethod 2"));
		}
	}
	else
	{
		UE_LOG(LogVideoStream, Log, TEXT("Start Video Capture"));
	}
}

void UVideoStreamSubsystem::WarmUp(EVideoStreamType Type /*= EVideoStreamType::FFmpeg*/)
{
	if (Type == UEAVEncoder)
	{
		//关闭framegrabber的抓帧
		if (CustomFrameGrabber != nullptr && CustomFrameGrabber->IsCapturingFrames())
		{
			CustomFrameGrabber->StopCapturingFrames();
			CustomFrameGrabber->Shutdown();
		}
		//注册视口回调
#if WITH_EDITOR
		//编辑器下没有UI 待优化
		UGameViewportClient::OnViewportRendered().AddUObject(this, &UVideoStreamSubsystem::OnViewportRendered);
#else
		FSlateApplication::Get().GetRenderer()->OnBackBufferReadyToPresent().AddUObject(this, &UVideoStreamSubsystem::OnFrameCaptured);
#endif
	}
	else if (Type == Obs)
	{
		//关闭framegrabber的抓帧
		if (CustomFrameGrabber != nullptr && CustomFrameGrabber->IsCapturingFrames())
		{
			CustomFrameGrabber->StopCapturingFrames();
			CustomFrameGrabber->Shutdown();
		}
	}
	else
	{
		if (CustomFrameGrabber && !CustomFrameGrabber->IsCapturingFrames())
		{
			CustomFrameGrabber->StartCapturingFrames();
			CustomFrameGrabber->CaptureThisFrame(FCustomFramePayloadPtr(new UVideoStreamSubsystemWarpper(this, GFrameNumber)));
		}
	}
	bHasWarmUp = true;
}	

void UVideoStreamSubsystem::StartCaptureWithType(FVideoStreamOptions Options, EVideoStreamType Type)
{
	if (Type >= ImplementsList.Num())
	{
		UE_LOG(LogVideoStream, Error, TEXT("Invalid type"));
		return;
	}
	//设置当前实现
	WarpperType = Type;

	Video_Tick_Time = 1.0f / (float)Options.FPS;

	StartCapture(Options);
}

void UVideoStreamSubsystem::EndCapture()
{
	UE_LOG(LogVideoStream, Log, TEXT("Call End Video Capture >>>>>>>.Wait End"));
	bShouldEnd = true;
}

bool UVideoStreamSubsystem::IsRecording()
{
	return CurrentImplements != nullptr && bIsRecording;
}

void UVideoStreamSubsystem::CreatFrameGetter(int w, int h)
{
	UE_LOG(LogVideoStream, Log, TEXT("UVideoStreamSubsystem::ReCreateFrameGrabber w [%d] h [%d]"),w,h);
	if (w <= 0 || h <= 0)
		return;
	if (CustomFrameGrabber != nullptr && CustomFrameGrabber->IsCapturingFrames())
	{
		CustomFrameGrabber->StopCapturingFrames();
		CustomFrameGrabber->Shutdown();
	}
	TSharedRef<FSceneViewport> SV = MakeShareable(new FSceneViewport(GEngine->GameViewport, GEngine->GameViewport->GetGameViewportWidget()));
	FIntPoint Size = GEngine->GameViewport->Viewport->GetSizeXY();
	CustomFrameGrabber = new FCustomFrameGrabber(SV, Size);

	if (CustomFrameGrabber)
	{
		CustomFrameGrabber->StartCapturingFrames();
		CustomFrameGrabber->CaptureThisFrame(FCustomFramePayloadPtr(new UVideoStreamSubsystemWarpper(this, GFrameNumber)));
	}
	else {
		OnMessage.Broadcast(false, TEXT("CustomFrameGrabber is NULL"));
	}
}

void UVideoStreamSubsystem::RecvWarpperMsg(bool bSuccess, FString Msg)
{
	AsyncTask(ENamedThreads::GameThread, [=]() {
			FString InMsg = Msg;
			OnMessage.Broadcast(bSuccess, InMsg);
		});
}

void UVideoStreamSubsystem::OnNewSubmixBuffer(const USoundSubmix* OwningSubmix, float* AudioData, int32 NumSamples, int32 NumChannels, const int32 SampleRate, double AudioClock)
{
	if (bIsRecording && CurrentImplements != nullptr /*&& Capture_Video_Index < TotalFrameNum*/)
	{
		CurrentImplements->Encode_Audio_Frame(AudioData, NumSamples,NumChannels,SampleRate);
	}
	//else
	//{
	//	//GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("OnNewSubmixBuffer")));
	//}
}

void UVideoStreamSubsystem::RegisterAudioListener()
{
	if (FAudioDeviceHandle AudioDeviceHandle = GetGameInstance()->GetWorld()->GetAudioDevice())
	{
		AudioDeviceHandle->RegisterSubmixBufferListener(this);
	}
}

void UVideoStreamSubsystem::UnRegisterAudioListener()
{
	if (FAudioDeviceHandle AudioDeviceHandle = GetGameInstance()->GetWorld()->GetAudioDevice())
	{
		AudioDeviceHandle->UnregisterSubmixBufferListener(this);
	}
}

bool UVideoStreamSubsystem::AddTickTime(float time)
{
	Ticktime += time;

	if(WarpperType == Obs && bShouldEnd && Ticktime > (float)TotalFrameNum * 0.04f)
	{
		bShouldEnd = false;
		EndCaptureInternal();
	}
	else if (CurrentImplements && CurrentImplements->GetVideoFrameNum() >= TotalFrameNum - 10 && bShouldEnd)
	{
		bShouldEnd = false;
		EndCaptureInternal();
	}

	//添加超时判断
	if (CurrentImplements && WarpperType != Obs)
	{
		//先判断上次的值和当前是否相等
		if (LastFrameNum == CurrentImplements->GetVideoFrameNum())
		{
			//如果相等
			LastTickTime += time;
			
			if(LastTickTime > 10.0f)
			{
				//如果超过10秒 结束录制
				UE_LOG(LogVideoStream, Warning, TEXT("UVideoStreamSubsystem::OverTime > 10.0f"));
				bShouldEnd = false;
				EndCaptureInternal();
			}
		}
		else
		{
			LastFrameNum = CurrentImplements->GetVideoFrameNum();
			LastTickTime = 0.0f;
		}
	}


	return true;
}

void UVideoStreamSubsystem::OnFrameReady(FColor* ColorBuffer, FIntPoint BufferSize, FIntPoint TargetSize)
{
	FScopeLock ScopeLock(&VideoBufferMutex);

	if (!CanEncode())
		return;

	UE_LOG(LogVideoStream, VeryVerbose, TEXT("UVideoStreamSubsystemWarpper::OnFrameReady [%d] w[%d] h[%d]"), Capture_Video_Index , BufferSize.X,BufferSize.Y);

	if (CurrentImplements)
	{
		CurrentImplements->Encode_Video_Frame(ColorBuffer, BufferSize, TargetSize);
	}
}

void UVideoStreamSubsystem::OnFrameCaptured(SWindow& SlateWindow, const FTexture2DRHIRef& FrameBuffer)
{
	check(IsInRenderingThread());

	if (!CanEncode())
		return;

	if (WarpperType != EVideoStreamType::UEAVEncoder)
	{
		return;
	}
	UE_LOG(LogVideoStream, VeryVerbose, TEXT("UVideoStreamSubsystemWarpper::OnFrameCaptured [%d]"), Capture_Video_Index);
	if (CurrentImplements)
	{
		CurrentImplements->Encode_Video_FrameRHI(FrameBuffer);
	}
}

void UVideoStreamSubsystem::OnViewportRendered(FViewport* Viewport)
{
	if(!CanEncode())
		return;

	TSharedPtr<SViewport> InScene = static_cast<FSceneViewport*>(Viewport)->GetViewportWidget().Pin();

	const FTextureRHIRef& FrameBuffer = Viewport->GetRenderTargetTexture();

	if (!FrameBuffer)
	{
		return;
	}
	if(WarpperType != EVideoStreamType::UEAVEncoder)
	{
		return;
	}
	UE_LOG(LogVideoStream, VeryVerbose, TEXT("UVideoStreamSubsystemWarpper::OnFrameCaptured [%d]"), Capture_Video_Index);
	if (CurrentImplements)
	{
		CurrentImplements->Encode_Video_FrameRHI(FrameBuffer);
	}
}

void UVideoStreamSubsystem::EndCaptureInternal()
{
	bIsRecording = false;

	if (CurrentImplements != nullptr)
	{
		UnRegisterAudioListener();
		CurrentImplements->Encode_Finish();
		if (CustomFrameGrabber->IsCapturingFrames())
		{
			CustomFrameGrabber->StopCapturingFrames();
			CustomFrameGrabber->Shutdown();
		}
		CurrentImplements = nullptr;

#if WITH_EDITOR
		UGameViewportClient::OnViewportRendered().RemoveAll(this);
#else
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().GetRenderer()->OnBackBufferReadyToPresent().RemoveAll(this);
		}
#endif
	}

	UE_LOG(LogVideoStream, Log, TEXT("End Video Capture Finish"));


	auto EndGameLambda = [&]() {
		GetWorld()->GetFirstPlayerController()->ConsoleCommand("quit");
	};

	FTimerHandle EndGameHandle;
	FTimerDelegate TimerDelegate = FTimerDelegate::CreateLambda(EndGameLambda);
	GetWorld()->GetTimerManager().SetTimer(EndGameHandle, TimerDelegate, 1.0f, false,5.0);
}

void UVideoStreamSubsystem::OnFrameCapturedInternal(FColor* ColorBuffer, FIntPoint BufferSize, FIntPoint TargetSize)
{
	if (IsRecording())
	{
		OnFrameReady(ColorBuffer, BufferSize, TargetSize);
	}
}

bool UVideoStreamSubsystem::CanEncode()
{
	if (Ticktime >= Video_Tick_Time)
	{
		Ticktime -= Video_Tick_Time;
	}
	else {
		return false;
	}

	if(!bIsRecording)
		return false;

	if (Capture_Video_Index == 0)
	{
		Ticktime = 0;
	}

	//GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue, FString::Printf(TEXT("Encode_Video_Frame %d"), Video_Index));

	//av encoder 不跳 由内部编码完了再跳
	if(WarpperType != UEAVEncoder)
	{
		if (Capture_Video_Index < 4 && Width >= 3840 )
		{
			//跳过前三帧 4K 
			UE_LOG(LogVideoStream, Log, TEXT("Skip Frame in 4K [%d] [%d]"), Capture_Video_Index, TotalFrameNum);
			Capture_Video_Index++;
			return false;
		}
		else if (Capture_Video_Index < 3)
		{
			UE_LOG(LogVideoStream, Log, TEXT("Skip Frame in 1080P [%d] [%d]"), Capture_Video_Index, TotalFrameNum);
			Capture_Video_Index++;
			return false;
		}
	}

	if (Capture_Video_Index >= TotalFrameNum - 2 && CurrentImplements->video_index >= TotalFrameNum - 2 && bShouldEnd)
	{
		UE_LOG(LogVideoStream, Log, TEXT("Skip Last Frame [%d] [%d]"), Capture_Video_Index, TotalFrameNum);
		Capture_Video_Index++;
		//跳过最后2帧
		return false;
	}
	Capture_Video_Index++;
	return true;
}

bool UVideoStreamSubsystemWarpper::OnFrameReady_RenderThread(FColor* ColorBuffer, FIntPoint BufferSize, FIntPoint TargetSize) const
{
	//Todo::如果卡顿 需要额外的线程去处理数据 暂时不写
	UE_LOG(LogVideoStream, Verbose, TEXT("in OnFrameReady_RenderThread,BufferSize = [%d,%d],TargetSize size = [%d,%d]"), BufferSize.X, BufferSize.Y, TargetSize.X, TargetSize.Y);

	UVideoStreamSubsystem* system = (UVideoStreamSubsystem*)Subsystem;
	if (system->GetGrabber() && system->GetGrabber()->IsCapturingFrames())
	{
		system->GetGrabber()->CaptureThisFrame(FCustomFramePayloadPtr(new UVideoStreamSubsystemWarpper(Subsystem, GFrameNumber)));
	}
	
	if (system->IsRecording())
	{
		system->OnFrameReady(MoveTemp(ColorBuffer), BufferSize, TargetSize);
	}
	return false;
}

