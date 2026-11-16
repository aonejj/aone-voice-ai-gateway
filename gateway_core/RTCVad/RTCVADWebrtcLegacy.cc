
#include "RTCVADWebrtcLegacy.h"

status_t RTCVADWebrtcLegacy::SetConfig(int32_t sampleRate, int32_t channels, VADAggressiveness mode) {
    _sample_rate = sampleRate;
    _channels = channels;
    _mode = mode;
    _vad = webrtc::CreateVad((webrtc::Vad::Aggressiveness)_mode);
    if(_vad == nullptr) {
        return NO_INIT;
    }
    return OK;
}

RTCVADWebrtcLegacy::VADActivity RTCVADWebrtcLegacy::VoiceActivity(int16_t* pcm, int32_t samples) {
    webrtc::Vad::Activity ret = _vad->VoiceActivity(pcm, samples, _sample_rate);
    return (RTCVADWebrtcLegacy::VADActivity) ret;
} 