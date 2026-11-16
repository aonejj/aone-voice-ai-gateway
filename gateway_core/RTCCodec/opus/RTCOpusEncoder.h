//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#ifndef __RTC_OPUS_ENCODER_H__
#define __RTC_OPUS_ENCODER_H__

#include <vector>

#include "../RTCCodecInterface.h"

#include <opus/opus.h>

class RTCOpusEncoder : public RTCAudioEncoderInterface {
public:
	RTCOpusEncoder(int32_t sampling_rate, int32_t channels);

public:
	~RTCOpusEncoder();

public:
	status_t Encode(int16_t* pcm_data, int32_t pcm_len, uint8_t* encode_data, int32_t capacity, int32_t& encode_data_len) override;

private:
	enum {
		RTC_OPUS_APPLICATION_VOIP = 2048,
		RTC_OPUS_APPLICATION_AUDIO,
		RTC_OPUS_APPLICATION_RESTRICTED_LOWDELAY = 2051,
	};

private:
	int32_t _sampling_rate;
	int32_t _channels;
	OpusEncoder* _encoder = nullptr;

private:
    RTCOpusEncoder(const RTCOpusEncoder&) = delete;
    RTCOpusEncoder& operator=(const RTCOpusEncoder&) = delete; 	
};

#endif	// __RTC_OPUS_ENCODER_H__