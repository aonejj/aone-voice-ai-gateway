#include "common_audio/include/audio_util.h"

#include "RTCAudioChannelConverter.h"

void RTCAudioChannelConverter::DownmixInterleavedToMono(const int16_t *interleaved, size_t frames,
                                            int32_t channels, int16_t *deinterleaved) {
    webrtc::DownmixInterleavedToMono(interleaved, frames, channels, deinterleaved);
}

void RTCAudioChannelConverter::UpmixMonoToInterleaved(const int16_t* deinterleaved, size_t frames,
                                                int32_t channels, int16_t* interleaved) {
    webrtc::UpmixMonoToInterleaved(deinterleaved, frames, channels, interleaved);
}