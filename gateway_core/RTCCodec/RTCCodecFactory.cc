//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif

#define LOG_TAG "RTCCodecFactory"
#include <utils/Log.h>

#include "RTCCodecFactory.h"

#include "opus/RTCOpusDecoder.h"
#include "opus/RTCOpusEncoder.h"

std::unique_ptr<RTCAudioDecoderInterface> RTCCodecFactory::CreateAudioDecoder(
	RTCAudioCodecType type, int32_t sampling_rate, int32_t channels) {
	if (type == RTC_AUDIO_CODEC_OPUS) {
		return std::make_unique<RTCOpusDecoder>(sampling_rate, channels);
	}
	return nullptr;
}

std::unique_ptr<RTCAudioEncoderInterface> RTCCodecFactory::CreateAudioEncoder(
	RTCAudioCodecType type, int32_t sampling_rate, int32_t channels) {
	if (type == RTC_AUDIO_CODEC_OPUS) {
		return std::make_unique<RTCOpusEncoder>(sampling_rate, channels);
	}
	return nullptr;
}

