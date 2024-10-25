#include "AVMuxerWarpper.h"
#include "VideoStreamSubsystem.h"
#include "Misc/FileHelper.h"
#include "OpenCVHelper.h"
#include "PreOpenCVHeaders.h"
#include "opencv2/opencv.hpp"
#include "PostOpenCVHeaders.h"


UAVMuxerWarpper::~UAVMuxerWarpper()
{
	if (Muxer)
	{
		Muxer->stop(false);
		delete Muxer;
	}
}

bool UAVMuxerWarpper::Init(UWorld* World, FVideoStreamOptions VideoParam, UVideoStreamSubsystem* InSubsystem)
{
	CurrentWorld = World;
	MediaParams mp;
	mp.device_id = 0;
	mp.frame_rate = VideoParam.FPS;

	if (GEngine->GameViewport->Viewport->GetSizeXY().X > 0 && GEngine->GameViewport->Viewport->GetSizeXY().Y > 0)
	{
		mp.width = GEngine->GameViewport->Viewport->GetSizeXY().X;
		mp.height = GEngine->GameViewport->Viewport->GetSizeXY().Y;
	}
	else
	{
		mp.width = VideoParam.Width;
		mp.height = VideoParam.Height;
	}
	mp.in_pix_fmt = PIX_FMT_BGRA; //UE默认格式
	mp.bitrate = VideoParam.VideoBitRate;
	mp.video_encoder_format = "h264";
	mp.audio_sample_rate = 48000;
	mp.audio_channels = 2;
	mp.audio_sample_format = SAMPLE_FMT_FLT;
	mp.device_type = AVMuxer::DevType::V_GPU;// VideoParam.bUseGPU ? AVMuxer::DevType::V_GPU : AVMuxer::DevType::V_CPU;
	mp.video_extra_params = "gopsize=12;profile=high;out_pixfmt=rgba";
	Muxer = new AVMuxer(mp);
	Height = mp.height;
	Width = mp.width;
	AudioPerFrameSize = 16000 / VideoParam.FPS * 2;

	Video_Tick_Time = float(1) / float(VideoParam.FPS);

	std::string url(TCHAR_TO_UTF8(*VideoParam.OutFileName));

	if (VideoParam.OutFileName.StartsWith(TEXT("rtmp")))
		bIsLocal = false;
	else
		bIsLocal = true;

	if (Muxer->open(url)) {
		UE_LOG(LogVideoStream, Error, TEXT("muxer open failed"));
		return false;
	}
	if (Muxer->start()) {
		UE_LOG(LogVideoStream, Error, TEXT("muxer start failed"));
		return false;
	}
	Subsystem = InSubsystem;

	//AddTickFunction();

	//RegisterAudioListener();

	bIsDestory = false;

	video_index = 0;

	return true;
}


bool UAVMuxerWarpper::Encode_Video_Frame(void* VideoDataPtr, FIntPoint BufferSize, FIntPoint TargetSize)
{
	SCOPED_NAMED_EVENT(UAVMuxerWarpper_Encode_Video_Frame, FColor::Green);
	
	if (!Muxer || bIsDestory)
		return false;

	UE_LOG(LogVideoStream, Verbose, TEXT("Encode_Video_Frame %f"), ticktime);

	/*if (TargetSize.X != Width || TargetSize.Y != Height)
	{
		return false;
	}*/

	video_index ++;

	//FString Path = FString::Printf(TEXT("D:/111/Saved/mat0___%d.png"), video_index);
	cv::Mat mat = cv::Mat(BufferSize.Y, BufferSize.X, CV_8UC4, VideoDataPtr);
	//cv::imwrite(TCHAR_TO_UTF8(*Path), mat);

	mat = mat(cv::Rect(0, 0, Width, Height) & cv::Rect(0, 0, mat.cols, mat.rows)).clone();

	//Path = FString::Printf(TEXT("D:/111/Saved/mat1___%d.png"),video_index);

	//cv::imwrite(TCHAR_TO_UTF8(*Path),mat);

	UE_LOG(LogVideoStream,Verbose,TEXT("UAVMuxerWarpper::Encode_Video_Frame [%d]"),video_index);

	Muxer->feed_image(mat, true);

	std::string error;
	if (Muxer->check_error(error) != 0)
	{
		UE_LOG(LogVideoStream, Error, TEXT("Encode_Video_Frame Error %s"), UTF8_TO_TCHAR(error.c_str()));
		Subsystem->RecvWarpperMsg(false, FString::Printf(TEXT("Encode_Audio_Frame Error %s"), UTF8_TO_TCHAR(error.c_str())));
		return false;
	}
	return true;
}

bool UAVMuxerWarpper::Encode_Audio_Frame(const float* AudioData, int32 NumSamples, int32 NumChannels, int32 SampleRate)
{
	if (!Muxer)
		return false;
	Muxer->feed_audio((char*)AudioData, 2048 * sizeof(float));
	std::string error;
	if (!Muxer && Muxer->check_error(error) != 0)
	{
		UE_LOG(LogVideoStream, Error, TEXT("Encode_Audio_Frame Error %s"), UTF8_TO_TCHAR(error.c_str()));
		Subsystem->RecvWarpperMsg(false,FString::Printf(TEXT("Encode_Audio_Frame Error %s"), UTF8_TO_TCHAR(error.c_str())));
		return false;
	}
	return true;
}


bool UAVMuxerWarpper::Encode_Finish()
{
	if (bIsDestory == false)
	{
		Subsystem->RecvWarpperMsg(true, TEXT("Encode_Finish"));
		bIsDestory = true;
		//UnRegisterAudioListener();
		Muxer->stop(false);
		delete Muxer;
		Muxer = nullptr;
		FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
		ticktime = 0;
		UE_LOG(LogVideoStream, Warning, TEXT("UAVMuxerWarpper::Encode_Finish"));
		return true;
	}
	UE_LOG(LogVideoStream, Warning, TEXT("Capture Not Start"));
	return false;
}

bool UAVMuxerWarpper::SetCurrentAudioTime(uint8_t* time)

{
	CurrentAuidoTime = *(double*)time;
	return true;
}


void UAVMuxerWarpper::UpdateLolStride(uint32 Stride)
{
	LolStride = Stride;
}

//void UAVMuxerWarpper::AddTickFunction()
//{
//	TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UAVMuxerWarpper::AddTickTime));
//}
//
//bool UAVMuxerWarpper::AddTickTime(float time)
//{
//	ticktime += time;
//	return true;
//}


int32 UAVMuxerWarpper::GetVideoFrameNum()
{
	if(Muxer)
		return Muxer->get_video_frame();
	else
		return 0;
}