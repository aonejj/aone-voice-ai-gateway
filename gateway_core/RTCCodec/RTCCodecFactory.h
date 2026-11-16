//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#ifndef __RTC_CODEC_FACTORY_H__
#define __RTC_CODEC_FACTORY_H__

#include <memory>

#include "RTCCodecInterface.h"

class RTCCodecFactory {
public:
	typedef enum {
		RTC_AUDIO_CODEC_OPUS,
	} RTCAudioCodecType;

public:
	static std::unique_ptr<RTCAudioDecoderInterface> CreateAudioDecoder(
		RTCAudioCodecType type, int32_t sampling_rate, int32_t channels);
	static std::unique_ptr<RTCAudioEncoderInterface> CreateAudioEncoder(
		RTCAudioCodecType type, int32_t sampling_rate, int32_t channels);

private:
	RTCCodecFactory() = default;
	~RTCCodecFactory() = default;

private:
    RTCCodecFactory(const RTCCodecFactory&) = delete;
    RTCCodecFactory& operator=(const RTCCodecFactory&) = delete;  
};

#endif // __RTC_CODEC_FACTORY_H__