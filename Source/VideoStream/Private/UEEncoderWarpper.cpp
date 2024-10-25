// Fill out your copyright notice in the Description page of Project Settings.


#include "UEEncoderWarpper.h"
#include "VideoStreamSubsystem.h"
#include "PixelCaptureBufferFormat.h"
#include "PixelCaptureOutputFrameRHI.h"
#include "VideoEncoderFactory.h"
#include "VideoEncoderInput.h"
#include "VideoEncoder.h"
#include "ShaderCore.h"
#include "GlobalShader.h"
#include "ScreenRendering.h"
#include "CommonRenderResources.h"
#include "WmfMp4Writer.h"


const uint32 HardcodedAudioSamplerate = 48000;

const uint32 HardcodedAudioNumChannels = 2;

const uint32 HardcodedAudioBitrate = 192000;

UUEEncoderWarpper::~UUEEncoderWarpper()
{
	if (System && System->bIsRecording)
	{
		Encode_Finish();
	}
	
	{
		FScopeLock Lock(&AudioProcessingCS);
		if (AudioEncoder)
		{
			AudioEncoder->Shutdown();
			AudioEncoder.Reset();
		}
	}
	{
		FScopeLock Lock(&VideoProcessingCS);
		if (VideoEncoder)
		{
			VideoEncoder->Shutdown();
			VideoEncoder.Reset();
			BackBuffers.Empty();
		}
	}

	if (Mp4Writer)
	{
		Mp4Writer->Finalize();
		Mp4Writer = nullptr;
	}
}

bool UUEEncoderWarpper::Encode_Audio_Frame(const float* AudioData, int32 NumSamples, int32 NumChannels, int32 SampleRate)
{
	if (!AudioEncoder.IsValid())
	{
		return false;
	}

	Audio::AlignedFloatBuffer InData;
	InData.Append(AudioData, NumSamples);
	Audio::TSampleBuffer<float> FloatBuffer(InData, NumChannels, SampleRate);

	// Mix to stereo if required, since PixelStreaming only accept stereo at the moment
	if (FloatBuffer.GetNumChannels() != HardcodedAudioNumChannels)
	{
		FloatBuffer.MixBufferToChannels(HardcodedAudioNumChannels);
	}

	// Adjust the AudioClock if for some reason it falls behind real time. This can happen if the game spikes, or if we break into the debugger.
	FTimespan Now = FTimespan::FromSeconds(FPlatformTime::Seconds()) - AudioStartTime;

	if (AudioClock < Now.GetTotalSeconds())
	{
		UE_LOG(LogVideoStream, Warning, TEXT("Audio clock falling behind real time clock by %.3f seconds. Ajusting audio clock"), Now.GetTotalSeconds() - AudioClock);
		// Put it slightly ahead of the real time clock
		AudioClock = Now.GetTotalSeconds() + (FloatBuffer.GetSampleDuration() / 2);
	}

	AVEncoder::FAudioFrame Frame;
	Frame.Timestamp = FTimespan::FromSeconds(AudioClock);
	Frame.Duration = FTimespan::FromSeconds(FloatBuffer.GetSampleDuration());
	FloatBuffer.Clamp();
	Frame.Data = FloatBuffer;
	AudioEncoder->Encode(Frame);

	AudioClock += FloatBuffer.GetSampleDuration();

	return true;
}

bool UUEEncoderWarpper::Encode_Video_FrameRHI(const FTexture2DRHIRef& FrameBuffer)
{
	if(!VideoEncoder.IsValid())
		return false;

	FScopeLock Lock(&VideoProcessingCS);

	TSharedPtr<AVEncoder::FVideoEncoderInputFrame> InputFrame = ObtainInputFrame();

	const int32 FrameId = InputFrame->GetFrameID();

	UE_LOG(LogVideoStream, VeryVerbose, TEXT("Encode_Video_FrameRHI [%d]"), FrameId);

	if(FrameId == 0)
	{
		VideoStartTime = FTimespan::FromSeconds(FPlatformTime::Seconds()).GetTicks();
		UE_LOG(LogVideoStream, VeryVerbose, TEXT("Set VideoStartTime [%lld]"), VideoStartTime.GetTicks());
	}

	FTimespan Now = FTimespan::FromSeconds(FPlatformTime::Seconds()) - VideoStartTime;

	UE_LOG(LogVideoStream,VeryVerbose,TEXT("Now[%lld] VideoStartTime[%lld] FrameId[%d]"), Now.GetTicks(), VideoStartTime.GetTicks(), FrameId);
	InputFrame->SetTimestampUs(Now.GetTicks());

	ENQUEUE_RENDER_COMMAND(StreamViewportTextureCommand)
		([&, FrameBuffer, InputFrame](FRHICommandList& RHICmdList)
			{
				CopyTexture(FrameBuffer, BackBuffers[InputFrame]);

				AVEncoder::FVideoEncoder::FEncodeOptions EncodeOptions;
				if (InputFrame->GetFrameID() == 1)
				{
					EncodeOptions.bForceKeyFrame = true;
				}
				//EncodeOptions.bForceKeyFrame = true;
				{
					FScopeLock Lock(&VideoProcessingCS);
					if (VideoEncoder)
						VideoEncoder->Encode(InputFrame, EncodeOptions);
				}
			});
	video_index ++;
	return true;
}

bool UUEEncoderWarpper::Encode_Finish()
{
	check(IsInGameThread());

	bIsRecording = false;

	if (AudioStartTime != 0)
	{
		UE_LOG(LogVideoStream, Log, TEXT("Currently running, so also performing a Stop()"));
		if (AudioStartTime == 0)
		{
			UE_LOG(LogVideoStream, Log, TEXT("Not running"));
			return true;
		}

		AudioStartTime = 0;
		VideoStartTime = 0;
		AudioClock = 0;
	}

	{
		FScopeLock Lock(&AudioProcessingCS);
		if (AudioEncoder)
		{
			// AudioEncoder->Reset();
			AudioEncoder->Shutdown();
			AudioEncoder.Reset();
		}
	}
	{
		FScopeLock Lock(&VideoProcessingCS);
		if (VideoEncoder)
		{
			VideoEncoder->Shutdown();
			VideoEncoder.Reset();

			BackBuffers.Empty();
		}
	}

	if(Mp4Writer)
	{
		Mp4Writer->Finalize();
		Mp4Writer = nullptr;
	}

	//shut down


	return true;
}


void UUEEncoderWarpper::UpdateLolStride(uint32 Stride)
{

}

bool UUEEncoderWarpper::Init(UWorld* World, FVideoStreamOptions Param, UVideoStreamSubsystem* InSubsystem)
{
	if (GEngine->GameViewport->Viewport->GetSizeXY().X > 0 && GEngine->GameViewport->Viewport->GetSizeXY().Y > 0)
	{
		Param.Width = GEngine->GameViewport->Viewport->GetSizeXY().X;
		Param.Height = GEngine->GameViewport->Viewport->GetSizeXY().Y;
	}
	bool bIsOk = false;

	System = InSubsystem;

	video_index = 0;

	ON_SCOPE_EXIT
	{
		if (!bIsOk)
		{
			Encode_Finish();
		}
	};
    
	//初始化audio encoder

	AVEncoder::FAudioEncoderFactory* AudioEncoderFactory = AVEncoder::FAudioEncoderFactory::FindFactory("aac");
	if (!AudioEncoderFactory)
	{
		UE_LOG(LogVideoStream, Error, TEXT("No audio encoder for aac found"));
		return false;
	}

	AudioEncoder = AudioEncoderFactory->CreateEncoder("aac");
	if (!AudioEncoder)
	{
		UE_LOG(LogVideoStream, Error, TEXT("Could not create audio encoder"));
		return false;
	}

	AudioConfig.Samplerate = HardcodedAudioSamplerate;
	AudioConfig.NumChannels = HardcodedAudioNumChannels;
	AudioConfig.Bitrate = HardcodedAudioBitrate;
	AudioConfig.Codec = "aac";
	if (!AudioEncoder->Initialize(AudioConfig))
	{
		UE_LOG(LogVideoStream, Error, TEXT("Could not initialize audio encoder"));
		return false;
	}

	AudioEncoder->RegisterListener(*this);

	//初始化video encoder

	VideoConfig.Codec = "h264";
	VideoConfig.Width = Param.Width;
	VideoConfig.Height = Param.Height;
	// Specifying 0 will completely disable frame skipping (therefore encoding as many frames as possible)
	VideoConfig.Framerate = Param.FPS;
	VideoConfig.Bitrate = Param.VideoBitRate * 100;

	AVEncoder::FVideoEncoder::FLayerConfig videoInit;
	videoInit.Width = VideoConfig.Width;
	videoInit.Height = VideoConfig.Height;
	videoInit.MaxBitrate = Param.VideoBitRate;
	videoInit.TargetBitrate = Param.VideoBitRate;
	videoInit.MaxFramerate = Param.FPS;

	if (GDynamicRHI)
	{
		const ERHIInterfaceType RHIType = RHIGetInterfaceType();

#if PLATFORM_DESKTOP && !PLATFORM_APPLE
		if (RHIType == ERHIInterfaceType::D3D11)
		{
			VideoEncoderInput = AVEncoder::FVideoEncoderInput::CreateForD3D11(GDynamicRHI->RHIGetNativeDevice(), true, IsRHIDeviceAMD());
		}
		else if (RHIType == ERHIInterfaceType::D3D12)
		{
			VideoEncoderInput = AVEncoder::FVideoEncoderInput::CreateForD3D12(GDynamicRHI->RHIGetNativeDevice(), true, IsRHIDeviceNVIDIA());
		}
		else if (RHIType == ERHIInterfaceType::Vulkan)
		{
			VideoEncoderInput = AVEncoder::FVideoEncoderInput::CreateForVulkan(GDynamicRHI->RHIGetNativeDevice(), true);
		}
		else
#endif
		{
			UE_LOG(LogVideoStream, Error, TEXT("Video encoding is not supported with the current Platform/RHI combo."));
			return false;
		}
	}

	const TArray<AVEncoder::FVideoEncoderInfo>& AvailableEncodersInfo = AVEncoder::FVideoEncoderFactory::Get().GetAvailable();

	if (AvailableEncodersInfo.Num() == 0)
	{
		UE_LOG(LogVideoStream, Error, TEXT("No video encoders found. Check if relevent encoder plugins have been enabled for this project."));
		return false;
	}

	for (const auto& EncoderInfo : AvailableEncodersInfo)
	{
		if (EncoderInfo.CodecType == AVEncoder::ECodecType::H264)
		{
			VideoEncoder = AVEncoder::FVideoEncoderFactory::Get().Create(EncoderInfo.ID, VideoEncoderInput, videoInit);
		}
	}

	if (!VideoEncoder)
	{
		UE_LOG(LogVideoStream, Error, TEXT("No H264 video encoder found. Check if relevent encoder plugins have been enabled for this project."));
		return false;
	}

	VideoEncoder->SetOnEncodedPacket([this](uint32 LayerIndex, const TSharedPtr<AVEncoder::FVideoEncoderInputFrame> Frame, const AVEncoder::FCodecPacket& Packet)
		{ OnEncodedVideoFrame(LayerIndex, Frame, Packet); });

	if (!VideoEncoder)
	{
		UE_LOG(LogVideoStream, Fatal, TEXT("Creating video encoder failed."));
		return false;
	}

	bIsOk = true; // So Shutdown is not called due to the ON_SCOPE_EXIT

	//START

	if (AudioStartTime != 0)
	{
		UE_LOG(LogVideoStream, Log, TEXT("Already running"));
		return true;
	}

	if (!VideoEncoder)
	{
		UE_LOG(LogVideoStream, Log, TEXT("Not initialized yet , so also performing a Intialize()"));
	}

	AudioStartTime = FTimespan::FromSeconds(FPlatformTime::Seconds());
	AudioClock = 0;
	NumCapturedFrames = 0;

	//
	// subscribe to engine delegates for audio output and back buffer
	//

	Mp4Writer = new FVideoStreamWmfMp4Writer();

	if (!Mp4Writer->Initialize(*Param.OutFileName))
	{
		return false;
	}

	TOptional<DWORD> AudioRes = Mp4Writer->CreateAudioStream(AudioConfig.Codec,AudioConfig);

	if (AudioRes.IsSet())
	{
		AudioStreamIndex = AudioRes.GetValue();
	}
	else
	{
		return false;
	}

	TOptional<DWORD> VideoRes = Mp4Writer->CreateVideoStream(VideoConfig.Codec, VideoConfig);

	if (VideoRes.IsSet())
	{
		VideoStreamIndex = VideoRes.GetValue();
	}
	else
	{
		return false;
	}

	if (!Mp4Writer->Start())
	{
		return false;
	}
	VideoFrameNum = 0;
	return true;
}

void UUEEncoderWarpper::OnEncodedAudioFrame(const AVEncoder::FMediaPacket& Packet)
{
	if (!Mp4Writer)
	{
		return;
	}
	FScopeLock Lock(&AudioProcessingCS);
	if (!System->bIsRecording)
		return;
	Mp4Writer->Write(Packet,AudioStreamIndex);
}


int32 UUEEncoderWarpper::GetVideoFrameNum()
{
	return VideoFrameNum;
}

void UUEEncoderWarpper::OnEncodedVideoFrame(uint32 LayerIndex, const TSharedPtr<AVEncoder::FVideoEncoderInputFrame> Frame, const AVEncoder::FCodecPacket& Packet)
{
	//FScopeLock Lock(&OnPacketsCS);
	AVEncoder::FMediaPacket packet(AVEncoder::EPacketType::Video);

	UE_LOG(LogVideoStream, VeryVerbose, TEXT("OnEncodedVideoFrame [%d][%lld]"),Frame->GetFrameID(),Frame->GetTimestampUs());

	int32 FId = Frame->GetFrameID();
	if (FId == 0)
	{
		Frame->Release();
		return;
	}
	else
	{
		FId -= 1;
	}
	
	float d = 1000.f / (float)VideoConfig.Framerate;
	packet.Timestamp = FTimespan(FId * FTimespan::FromMilliseconds(d).GetTicks());
	packet.Duration = FTimespan::FromMilliseconds(d); // This should probably be 1.0f / fps in ms
	packet.Data = TArray<uint8>(Packet.Data.Get(), Packet.DataSize);
	packet.Video.bKeyFrame = Packet.IsKeyFrame;
	packet.Video.Width = Frame->GetWidth();
	packet.Video.Height = Frame->GetHeight();
	packet.Video.FrameAvgQP = Packet.VideoQP;
	packet.Video.Framerate = 25;

	Mp4Writer->Write(packet, VideoStreamIndex);
	VideoFrameNum ++;
	Frame->Release();
}

TSharedPtr<AVEncoder::FVideoEncoderInputFrame> UUEEncoderWarpper::ObtainInputFrame()
{
	TSharedPtr<AVEncoder::FVideoEncoderInputFrame> InputFrame = VideoEncoderInput->ObtainInputFrame();
	InputFrame->SetWidth(VideoConfig.Width);
	InputFrame->SetHeight(VideoConfig.Height);
	if (!BackBuffers.Contains(InputFrame))
	{
#if PLATFORM_WINDOWS && PLATFORM_DESKTOP
		const ERHIInterfaceType RHIType = RHIGetInterfaceType();

		const FRHITextureCreateDesc Desc =
			FRHITextureCreateDesc::Create2D(TEXT("VideoCapturerBackBuffer"), VideoConfig.Width, VideoConfig.Height, PF_B8G8R8A8)
			.SetFlags(ETextureCreateFlags::Shared | ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::UAV)
			.SetInitialState(ERHIAccess::CopyDest);
			 
		if (RHIType == ERHIInterfaceType::D3D11)
		{
			FRHIResourceCreateInfo CreateInfo(TEXT("VideoCapturerBackBuffer"));
			FTextureRHIRef Texture = RHICreateTexture(Desc);

			InputFrame->SetTexture((ID3D11Texture2D*)Texture->GetNativeResource(), [&, InputFrame](ID3D11Texture2D* NativeTexture) { BackBuffers.Remove(InputFrame); });
			BackBuffers.Add(InputFrame, Texture);
		}
		else if (RHIType == ERHIInterfaceType::D3D12)
		{
			FRHIResourceCreateInfo CreateInfo(TEXT("VideoCapturerBackBuffer"));
			FTextureRHIRef Texture = RHICreateTexture(Desc);

			InputFrame->SetTexture((ID3D12Resource*)Texture->GetNativeResource(), [&, InputFrame](ID3D12Resource* NativeTexture) { BackBuffers.Remove(InputFrame); });
			BackBuffers.Add(InputFrame, Texture);
		}

		UE_LOG(LogTemp, Log, TEXT("%d backbuffers currently allocated"), BackBuffers.Num());
#else
		unimplemented();
#endif	// PLATFORM_DESKTOP && !PLATFORM_APPLE
	}

	return InputFrame;
}

void UUEEncoderWarpper::CopyTexture(const FTexture2DRHIRef & SourceTexture, FTexture2DRHIRef & DestinationTexture) const
{
	FRHICommandListImmediate& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();

	if (SourceTexture->GetFormat() == DestinationTexture->GetFormat() && SourceTexture->GetSizeXY() == DestinationTexture->GetSizeXY())
	{
		TransitionAndCopyTexture(RHICmdList, SourceTexture, DestinationTexture, {});
	}
	else // Texture format mismatch, use a shader to do the copy.
	{
		IRendererModule* RendererModule = &FModuleManager::GetModuleChecked<IRendererModule>("Renderer");

		// #todo-renderpasses there's no explicit resolve here? Do we need one?
		FRHIRenderPassInfo RPInfo(DestinationTexture, ERenderTargetActions::Load_Store);

		RHICmdList.Transition(FRHITransitionInfo(DestinationTexture, ERHIAccess::Unknown, ERHIAccess::RTV));
		RHICmdList.BeginRenderPass(RPInfo, TEXT("CopyBackbuffer"));

		{
			RHICmdList.SetViewport(0, 0, 0.0f, DestinationTexture->GetSizeX(), DestinationTexture->GetSizeY(), 1.0f);

			FGraphicsPipelineStateInitializer GraphicsPSOInit;
			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
			GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
			GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

			// New engine version...
			FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
			TShaderMapRef<FScreenVS> VertexShader(ShaderMap);
			TShaderMapRef<FScreenPS> PixelShader(ShaderMap);

			GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
			GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();

			GraphicsPSOInit.PrimitiveType = PT_TriangleList;

			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

			const bool bSameSize = (DestinationTexture->GetDesc().Extent == SourceTexture->GetDesc().Extent);
			FRHISamplerState* PixelSampler = bSameSize ? TStaticSamplerState<SF_Point>::GetRHI() : TStaticSamplerState<SF_Bilinear>::GetRHI();

			SetShaderParametersLegacyPS(RHICmdList, PixelShader, PixelSampler, SourceTexture);

			RendererModule->DrawRectangle(RHICmdList, 0, 0,                // Dest X, Y
				DestinationTexture->GetSizeX(),  // Dest Width
				DestinationTexture->GetSizeY(),  // Dest Height
				0, 0,                            // Source U, V
				1, 1,                            // Source USize, VSize
				DestinationTexture->GetSizeXY(), // Target buffer size
				FIntPoint(1, 1),                 // Source texture size
				VertexShader, EDRF_Default);
		}

		RHICmdList.EndRenderPass();
		RHICmdList.Transition(FRHITransitionInfo(DestinationTexture, ERHIAccess::RTV, ERHIAccess::SRVMask));
	}
}
