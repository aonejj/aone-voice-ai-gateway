#ifndef __RTC_AUDIO_RESAMPLER_H__
#define __RTC_AUDIO_RESAMPLER_H__

class RTCAudioResampler {
public:
    RTCAudioResampler();
    ~RTCAudioResampler();

public:
	int32_t SetConfig(int32_t samplingRate,  int32_t resampleRate, int32_t channels);
	size_t Resample(const int16_t* source, size_t source_lenght, 
						     int16_t* destination, size_t destination_capacity);

private:	
	class Impl;
	std::unique_ptr<Impl> impl_;

private:
	RTCAudioResampler(const RTCAudioResampler&) = delete;
	RTCAudioResampler& operator=(const RTCAudioResampler&) = delete;    
};

#endif // __RTC_AUDIO_RESAMPLER_H__