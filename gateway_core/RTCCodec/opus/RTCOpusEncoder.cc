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

#define LOG_TAG "RTCOpusEncoder"
#include <utils/Log.h>

#include <cassert>

#include "RTCOpusEncoder.h"

RTCOpusEncoder::RTCOpusEncoder(int32_t sampling_rate, int32_t channels)  
 : _sampling_rate(sampling_rate),
   _channels(channels) {

	int err = 0;
	_encoder = opus_encoder_create(sampling_rate, channels, RTC_OPUS_APPLICATION_VOIP, &err);  // OPUS_APPLICATION_AUDIO  RTC_OPUS_APPLICATION_VOIP
	if (err != OPUS_OK) {
		_encoder = nullptr;
	}

	opus_encoder_ctl(_encoder, OPUS_SET_BITRATE(32000));

	int complexity = 5;
	opus_encoder_ctl(_encoder, OPUS_SET_COMPLEXITY(complexity));	
}

RTCOpusEncoder::~RTCOpusEncoder() {
	if (_encoder) {
		opus_encoder_destroy(_encoder);
		_encoder = nullptr;
	}

}

status_t RTCOpusEncoder::Encode(int16_t* pcm_data, int32_t pcm_len, uint8_t* encode_data, int32_t capacity, int32_t& encode_data_len) {
	if (!_encoder) {
		return INVALID_OPERATION;
	}

	encode_data_len = opus_encode(_encoder, pcm_data, pcm_len, encode_data, capacity);
	if (encode_data_len < 0) {
		ALOGV("error encoded_byte %d", encode_data_len);
		return INVALID_OPERATION;
	}

	return OK;
}

