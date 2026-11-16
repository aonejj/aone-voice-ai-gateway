#ifndef __RTC_AUDIO_CHANNEL_CONVERTER_H__
#define __RTC_AUDIO_CHANNEL_CONVERTER_H__

class RTCAudioChannelConverter {
public:
    static void DownmixInterleavedToMono(const int16_t *interleaved, size_t frames,
                                         int32_t channels, int16_t *deinterleaved);
    static void UpmixMonoToInterleaved(const int16_t* deinterleaved, size_t frames,
                                       int32_t channels, int16_t* interleaved);                                         
};

#endif // __RTC_AUDIO_CHANNEL_CONVERTER_H__