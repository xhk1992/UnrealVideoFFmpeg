// Copyright Epic Games, Inc. All Rights Reserved.

#include "FFmpegMediaSample.h"

#include "FFmpegMedia.h"
#include "IMediaAudioSample.h"
#include "IMediaBinarySample.h"
#include "IMediaOverlaySample.h"
#include "IMediaTextureSample.h"
#include "MediaSamples.h"

/* Local helpers
*****************************************************************************/

template<typename SampleType, typename SinkType>
bool FetchSample(TMediaSampleQueue<SampleType, SinkType>& SampleQueue, TRange<FTimespan> TimeRange, TSharedPtr<SampleType, ESPMode::ThreadSafe>& OutSample)
{
	TSharedPtr<SampleType, ESPMode::ThreadSafe> Sample;

	if (!SampleQueue.Peek(Sample))
	{
		return false;
	}

	const FTimespan SampleTime = Sample->GetTime().Time;

	if (!TimeRange.Overlaps(TRange<FTimespan>(SampleTime, SampleTime + Sample->GetDuration())))
	{
		return false;
	}

	OutSample = Sample;
	SampleQueue.Pop();

	return true;
}


template<typename SampleType, typename SinkType>
bool FetchSample(TMediaSampleQueue<SampleType, SinkType>& SampleQueue, TRange<FMediaTimeStamp> TimeRange, TSharedPtr<SampleType, ESPMode::ThreadSafe>& OutSample)
{
	TSharedPtr<SampleType, ESPMode::ThreadSafe> Sample;

	if (!SampleQueue.Peek(Sample))
	{
		return false;
	}

	const FMediaTimeStamp SampleTime = Sample->GetTime();

	if (!TimeRange.Overlaps(TRange<FMediaTimeStamp>(SampleTime, SampleTime + Sample->GetDuration())))
	{
		return false;
	}

	OutSample = Sample;
	SampleQueue.Pop();

	return true;
}

/* IMediaSamples interface
*****************************************************************************/

FFFmpegMediaSamples::FFFmpegMediaSamples(uint32 InMaxNumberOfQueuedAudioSamples, uint32 InMaxNumberOfQueuedVideoSamples, uint32 InMaxNumberOfQueuedCaptionSamples, uint32 InMaxNumberOfQueuedSubtitlesSamples, uint32 InMaxNumberOfQueuedMetaDataSamples)
	: AudioSampleQueue(InMaxNumberOfQueuedAudioSamples)
	, CaptionSampleQueue(InMaxNumberOfQueuedCaptionSamples)
	, MetadataSampleQueue(InMaxNumberOfQueuedMetaDataSamples)
	, SubtitleSampleQueue(InMaxNumberOfQueuedSubtitlesSamples)
	, VideoSampleQueue(InMaxNumberOfQueuedVideoSamples)
{
}


bool FFFmpegMediaSamples::FetchAudio(TRange<FTimespan> TimeRange, TSharedPtr<IMediaAudioSample, ESPMode::ThreadSafe>& OutSample)
{
	return FetchSample(AudioSampleQueue, TimeRange, OutSample);
}


bool FFFmpegMediaSamples::FetchCaption(TRange<FTimespan> TimeRange, TSharedPtr<IMediaOverlaySample, ESPMode::ThreadSafe>& OutSample)
{
	return FetchSample(CaptionSampleQueue, TimeRange, OutSample);
}


bool FFFmpegMediaSamples::FetchMetadata(TRange<FTimespan> TimeRange, TSharedPtr<IMediaBinarySample, ESPMode::ThreadSafe>& OutSample)
{
	return FetchSample(MetadataSampleQueue, TimeRange, OutSample);
}


bool FFFmpegMediaSamples::FetchSubtitle(TRange<FTimespan> TimeRange, TSharedPtr<IMediaOverlaySample, ESPMode::ThreadSafe>& OutSample)
{
	return FetchSample(SubtitleSampleQueue, TimeRange, OutSample);
}


bool FFFmpegMediaSamples::FetchVideo(TRange<FTimespan> TimeRange, TSharedPtr<IMediaTextureSample, ESPMode::ThreadSafe>& OutSample)
{
	return FetchSample(VideoSampleQueue, TimeRange, OutSample);
}

bool FFFmpegMediaSamples::FetchAudio(TRange<FMediaTimeStamp> TimeRange, TSharedPtr<IMediaAudioSample, ESPMode::ThreadSafe>& OutSample)
{
	return FetchSample(AudioSampleQueue, TimeRange, OutSample);
}

bool FFFmpegMediaSamples::FetchCaption(TRange<FMediaTimeStamp> TimeRange, TSharedPtr<IMediaOverlaySample, ESPMode::ThreadSafe>& OutSample)
{
	return FetchSample(CaptionSampleQueue, TimeRange, OutSample);
}

bool FFFmpegMediaSamples::FetchSubtitle(TRange<FMediaTimeStamp> TimeRange, TSharedPtr<IMediaOverlaySample, ESPMode::ThreadSafe>& OutSample)
{
	return FetchSample(SubtitleSampleQueue, TimeRange, OutSample);
}

bool FFFmpegMediaSamples::FetchMetadata(TRange<FMediaTimeStamp> TimeRange, TSharedPtr<IMediaBinarySample, ESPMode::ThreadSafe>& OutSample)
{
	return FetchSample(MetadataSampleQueue, TimeRange, OutSample);
}

bool FFFmpegMediaSamples::FetchVideo(TRange<FMediaTimeStamp> TimeRange, TSharedPtr<IMediaTextureSample, ESPMode::ThreadSafe>& OutSample)
{
	return FetchSample(VideoSampleQueue, TimeRange, OutSample);
}

void FFFmpegMediaSamples::FlushSamples()
{
	// Flushing may have various side effects that better all happen on the gamethread
	// check(IsInGameThread() || IsInSlateThread());

	AudioSampleQueue.RequestFlush();
	MetadataSampleQueue.RequestFlush();
	CaptionSampleQueue.RequestFlush();
	SubtitleSampleQueue.RequestFlush();
	VideoSampleQueue.RequestFlush();
}

/**
 * Fetch video sample best suited for the given time range. Samples prior to the selected one will be removed from the queue.
 */
FFFmpegMediaSamples::EFetchBestSampleResult FFFmpegMediaSamples::FetchBestVideoSampleForTimeRange(const TRange<FMediaTimeStamp> & TimeRange, TSharedPtr<IMediaTextureSample, ESPMode::ThreadSafe>& OutSample, bool bReverse)
{
	FTimespan TimeRangeLow = TimeRange.GetLowerBoundValue().Time;
	FTimespan TimeRangeHigh = TimeRange.GetUpperBoundValue().Time;
	// Account for loop wraparound.
	if (TimeRangeHigh < TimeRangeLow)
	{
		TimeRangeHigh += CachedDuration;
	}

	// if (!VideoSampleQueue.FetchBestSampleForTimeRange(TimeRange, OutSample, bReverse))
	// {
	// 	return EFetchBestSampleResult::NoSample;
	// }
	
	///return EFetchBestSampleResult::Ok;
	
	while (true)
	{
		// Is there a sample available?
		TSharedPtr<IMediaTextureSample, ESPMode::ThreadSafe> Sample;
		if (VideoSampleQueue.Peek(Sample))
		{
			FTimespan SampleStartTime = Sample->GetTime().Time;
			FTimespan SampleEndTime = SampleStartTime + Sample->GetDuration();

			if(VideoSampleQueue.FetchBestSampleForTimeRange(TimeRange, OutSample, bReverse))
			{
				return EFetchBestSampleResult::Ok;
			}
			else if(SampleEndTime < TimeRangeLow)
			{
				VideoSampleQueue.Pop();
			}
			else
			{
				return EFetchBestSampleResult::NoSample;
			}
		}
		else
		{
			break;
		}
	}
	return EFetchBestSampleResult::NoSample;
}

/**
 * Remove any video samples from the queue that have no chance of being displayed anymore
 */
uint32 FFFmpegMediaSamples::PurgeOutdatedVideoSamples(const FMediaTimeStamp & ReferenceTime, bool bReversed, FTimespan MaxAge)
{
	return VideoSampleQueue.PurgeOutdatedSamples(ReferenceTime, bReversed, MaxAge);
}

/**
 * Remove any subtitle samples from the queue that have no chance of being displayed anymore
 */
uint32 FFFmpegMediaSamples::PurgeOutdatedSubtitleSamples(const FMediaTimeStamp & ReferenceTime, bool bReversed, FTimespan MaxAge)
{
	return SubtitleSampleQueue.PurgeOutdatedSamples(ReferenceTime, bReversed, MaxAge);
}

/**
 * Remove any caption samples from the queue that have no chance of being displayed anymore
 */
uint32 FFFmpegMediaSamples::PurgeOutdatedCaptionSamples(const FMediaTimeStamp& ReferenceTime, bool bReversed, FTimespan MaxAge)
{
	return CaptionSampleQueue.PurgeOutdatedSamples(ReferenceTime, bReversed, MaxAge);
}

/**
 * Remove any caption samples from the queue that have no chance of being displayed anymore
 */
uint32 FFFmpegMediaSamples::PurgeOutdatedMetadataSamples(const FMediaTimeStamp& ReferenceTime, bool bReversed, FTimespan MaxAge)
{
	return MetadataSampleQueue.PurgeOutdatedSamples(ReferenceTime, bReversed, MaxAge);
}

/**
 * Check if can receive more video samples
 */
bool FFFmpegMediaSamples::CanReceiveVideoSamples(uint32 Num) const
{
	return VideoSampleQueue.CanAcceptSamples(Num);
}

/**
 * Check if can receive more audio samples
 */
bool FFFmpegMediaSamples::CanReceiveAudioSamples(uint32 Num) const
{
	return AudioSampleQueue.CanAcceptSamples(Num);
}

/**
 * Check if can receive more subtitle samples
 */
bool  FFFmpegMediaSamples::CanReceiveSubtitleSamples(uint32 Num) const
{
	return SubtitleSampleQueue.CanAcceptSamples(Num);
}

/**
 * Check if can receive more caption samples
 */
bool  FFFmpegMediaSamples::CanReceiveCaptionSamples(uint32 Num) const
{
	return CaptionSampleQueue.CanAcceptSamples(Num);
}

/**
 * Check if can receive more metadata samples
 */
bool  FFFmpegMediaSamples::CanReceiveMetadataSamples(uint32 Num) const
{
	return MetadataSampleQueue.CanAcceptSamples(Num);
}
