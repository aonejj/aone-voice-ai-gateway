//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#ifndef __RTC_OPUS_DECODER_H__
#define __RTC_OPUS_DECODER_H__

#include "../RTCCodecInterface.h"

#include <opus/opus.h>

class RTCOpusDecoder : public RTCAudioDecoderInterface {
public:
	RTCOpusDecoder(int32_t sampling_rate, int32_t channels);

public:
	~RTCOpusDecoder();

public:
	status_t Decode(uint8_t* frame, uint32_t flen, int16_t* pcm_out, int32_t capacity,
			int32_t* decode_samples) override;

private:
	int32_t _sampling_rate;
	int32_t _channels;
	OpusDecoder* _decoder = nullptr;

private:
    RTCOpusDecoder(const RTCOpusDecoder&) = delete;
    RTCOpusDecoder& operator=(const RTCOpusDecoder&) = delete; 
};

#endif  // __RTC_OPUS_DECODER_H__
