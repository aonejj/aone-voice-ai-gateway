//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#ifndef __RTC_CODEC_INTERFACE_H__
#define __RTC_CODEC_INTERFACE_H__

#include <utils/Errors.h>

class RTCAudioDecoderInterface {
public:
	virtual ~RTCAudioDecoderInterface() = default;

public:
	virtual status_t Decode(uint8_t* frame, uint32_t flen, int16_t* pcm_out, int32_t capacity,
						int32_t* decode_samples) = 0;
};

class RTCAudioEncoderInterface {
public:
	virtual ~RTCAudioEncoderInterface() = default;

public:
	virtual status_t Encode(int16_t* pcm_data, int32_t pcm_len, uint8_t* encode_data, int32_t capacity, int32_t& encode_data_len) = 0;
};


#endif // __RTC_CODEC_INTERFACE_H__