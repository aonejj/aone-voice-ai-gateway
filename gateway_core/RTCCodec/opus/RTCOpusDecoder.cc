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

#define LOG_TAG "RTCOpusDecoder"
#include <utils/Log.h>

#include "RTCOpusDecoder.h"


RTCOpusDecoder::RTCOpusDecoder(int32_t sampling_rate, int32_t channels)
  : _sampling_rate(sampling_rate),
	_channels(channels) {
	ALOGV("RTCOpusDecoder::RTCOpusDecoder sampling_rate %d channels %d", sampling_rate, channels);
	int32_t err = 0;
	_decoder = opus_decoder_create(sampling_rate, channels, &err);
	if (err != OPUS_OK) {
		_decoder = nullptr;
	}
}

RTCOpusDecoder::~RTCOpusDecoder() {
	if (_decoder != nullptr) {
		opus_decoder_destroy(_decoder);
		_decoder = nullptr;
	}
}

status_t RTCOpusDecoder::Decode(uint8_t* frame, uint32_t flen, int16_t* pcm_out, int32_t capacity, int32_t* decode_samples) {
	if (!_decoder) {
		return INVALID_OPERATION;
	}

	int32_t ret;

	ret = opus_decode(_decoder, frame, flen, pcm_out, capacity, 0);
	if (ret < 0) {
		*decode_samples = 0;
		return INVALID_OPERATION;
	}

	*decode_samples = ret;
	return OK;
}
