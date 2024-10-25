#pragma once

#include "CoreMinimal.h"
#include "AudioMixerDevice.h"
#include "CustomFrameGrabber.h"
#include "UFFmpeg.h"
#include "Async/Async.h"
#include "AVMuxer.hpp"
#include "avutil/Muxer.hpp"
#include "Containers/Ticker.h"
//#include "IVideoStreamInterface.h"
#include "VideoStreamInterface.h"
#include "AVMuxerWarpper.generated.h"


class UVideoStreamSubsystem;
struct FVideoStreamOptions;

UCLASS()
class UAVMuxerWarpper : public UObject,public IVideoStreamInterface
{
public:
	GENERATED_BODY()

	virtual ~UAVMuxerWarpper();

	virtual bool Encode_Video_Frame(void* VideoDataPtr, FIntPoint BufferSize, FIntPoint TargetSize) override;
	
	virtual bool Encode_Audio_Frame(const float* AudioData, int32 NumSamples, int32 NumChannels, int32 SampleRate) override;

	virtual bool Encode_Finish() override;

	bool SetCurrentAudioTime(uint8_t* time);

	virtual void UpdateLolStride(uint32 Stride);

	virtual bool Init(UWorld* World, FVideoStreamOptions Param , UVideoStreamSubsystem* InSubsystem) override;

	virtual int32 GetVideoFrameNum() override;

	bool bIsDestory = true;

private:
	// TWeakObjectPtr<UFrameGetter> FrameGetter;
	UWorld* CurrentWorld;

	AVMuxer* Muxer = nullptr;
	
	float Video_Tick_Time;

	double CurrentAuidoTime = 0.0;

	int Width;

	int Height;

	float ticktime = 0.0f;

	FTSTicker::FDelegateHandle TickDelegateHandle;
	FDelegateHandle EndPIEDelegateHandle;
	FDelegateHandle OnBackBufferReadyToPresent_;

	int32 LolStride;

	TEnumAsByte<EWorldType::Type> GameMode;

	int32 last_video_encode_time_ = 0;

	int AudioPerFrameSize = 0;

	bool bIsLocal = false;

	UVideoStreamSubsystem* Subsystem;

	//void AddTickFunction();

	//bool AddTickTime(float time);
};



