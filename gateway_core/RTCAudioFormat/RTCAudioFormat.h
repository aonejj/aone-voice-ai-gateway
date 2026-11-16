#ifndef __RTC_AUDIO_IO_FORMAT_H__
#define __RTC_AUDIO_IO_FORMAT_H__

struct RTCAudioFormat {
    enum class Codec { kOpus, kPcmS16le } codec;
    int32_t sample_rate; 
    int32_t channels;
};

#endif // __RTC_AUDIO_IO_FORMAT_H__