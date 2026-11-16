#include "common_audio/resampler/include/push_resampler.h"

#include "RTCAudioResampler.h"

class RTCAudioResampler::Impl {
public:
	Impl() = default;
	~Impl() = default;

public:
	int32_t SetConfig(int32_t samplingRate, int32_t channels, int32_t resampleRate);
	size_t Resample(const int16_t* source, size_t source_lenght,
							 int16_t* destination, size_t destination_capacity);

private:
	webrtc::PushResampler<int16_t> _resampler;

private:
	Impl(const Impl&) = delete;
	Impl& operator=(const Impl&) = delete;
};

int32_t RTCAudioResampler::Impl::SetConfig(int32_t samplingRate, int32_t resampleRate, int32_t channels) {
	_resampler.InitializeIfNeeded(samplingRate, resampleRate, channels);
	return 0;
}

size_t  RTCAudioResampler::Impl::Resample(const int16_t* source, size_t source_lenght,
																   int16_t* destination, size_t destination_capacity) {
	return _resampler.Resample(source, source_lenght, destination, destination_capacity);
}

RTCAudioResampler::RTCAudioResampler() = default;

RTCAudioResampler::~RTCAudioResampler() = default;

int32_t RTCAudioResampler::SetConfig(int32_t samplingRate, int32_t resampleRate, int32_t channels) {
	impl_ = std::make_unique<RTCAudioResampler::Impl>();
	if (impl_ == nullptr) {
		return -1;
	}

	return impl_->SetConfig(samplingRate, resampleRate, channels);
}

size_t RTCAudioResampler::Resample(const int16_t* source, size_t source_lenght,
	int16_t* destination, size_t destination_capacity) {

	if (impl_) {
		return impl_->Resample(source, source_lenght, destination, destination_capacity);
	}

	return 0;
}