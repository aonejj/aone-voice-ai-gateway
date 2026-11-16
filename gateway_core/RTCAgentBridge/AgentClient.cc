#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif

#define LOG_TAG "AgentClient"

#include <utils/Log.h>

#include <cassert>

#include "../RTCCodec/RTCCodecFactory.h"
#include "../RTCUtils/RTCTimeUtils.h"
#include "../RTCAudioUtils/RTCAudioChannelConverter.h"

#include "AgentClient.h"

//#define __ENABLE_VAD__

std::shared_ptr<AgentClientInterface> AgentClient::Create(const std::string& endpoint, const std::string& id) {
    return std::shared_ptr<AgentClientInterface>(new AgentClient(endpoint, id));
}

AgentClient::AgentClient(const std::string& endpoint, const std::string& id) 
: _endpoint(endpoint),
  _id(id) {
    
    _vad = RTCVADFactory::Create();
    if(_vad != nullptr) {
        _vad->SetConfig(16000, 1, RTCVADInterface::VADAggressiveness::kVadNormal);
    }

    _is_file_dump = false; // for debuging
    if(_is_file_dump) {
		std::string tmp = "_after_dec.dump";
		_after_dec_dump_file = fopen(tmp.c_str(), "wb");  
        tmp = "_before_enc.dump";
        _before_enc_dump_file = fopen(tmp.c_str(), "wb");  
        tmp = "_enc_resample.dump";
        _enc_resample_dump_file = fopen(tmp.c_str(), "wb");   
        tmp = "_enc_upmix.dump";
        _enc_upmix_dump_file = fopen(tmp.c_str(), "wb");   
        tmp = "_after_ring.dump";
        _after_ring_dump_file = fopen(tmp.c_str(), "wb");  
        tmp = "_after_enc_dec.dump";
        _after_enc_dec_dump_file = fopen(tmp.c_str(), "wb");
        tmp = "_0_dec.dump";
        _0_dec_dump_file = fopen(tmp.c_str(), "wb");        
    }

}

AgentClient::~AgentClient() {
    if(_is_file_dump) {
        fclose(_0_dec_dump_file);
        fclose(_after_dec_dump_file);
        fclose(_before_enc_dump_file);
        fclose(_enc_resample_dump_file);
        fclose(_enc_upmix_dump_file);
        fclose(_after_ring_dump_file); 
        fclose(_after_enc_dec_dump_file);       
    }
}

int32_t AgentClient::Open() {
    if(_session != nullptr) {
        return -1;
    }

    std::string error;
    _session = CreateGRPCAgentSession(_endpoint, _id, &error);
    if(_session == nullptr) {
        return -1;
    }

    int32_t ret = _session->Open();
    if(ret) {
        return ret;
    }


    _agent_in_format = _session->GetAgentSessionInAudioFormat();
    _agent_out_format = _session->GetAgentSessionOutAudioFormat();
    _session->SetSink(this);

    return 0;   
}

void AgentClient::Close() {
    if(_session != nullptr) {
        _session->SetSink(nullptr);
        _session->Release();
    }
}

RTCIngressAudioPacketSinkInterface* AgentClient::GetIngressPtr() {
    return this;
}

RTCEgressAudioSourceInterface* AgentClient::GetEgressPtr() {
    return this;
}

// AgentEgressAudioSinkInterface method
bool AgentClient::PushPcm(const int16_t* pcm, size_t samples) {
    if(!_is_set_egress_format.load() || pcm == nullptr || samples == 0) {
        return false;
    }

    if(_is_file_dump) {
        fwrite(pcm, sizeof(int16_t), samples, _before_enc_dump_file);
    }      

    std::lock_guard<std::mutex> lock(_mutex);

    const size_t frames = samples / static_cast<size_t>(_agent_out_format.channels);
    if(frames == 0) {
        return false;
    }

    const size_t estimate_resample_frames =
        (frames * static_cast<size_t>(_rtc_out_format.sample_rate) + 
         _agent_out_format.sample_rate -1) / static_cast<size_t>(_agent_out_format.sample_rate) + kEgressMarginFrames;
    
    if(_pcm_ring_buf->FreeSpace() < estimate_resample_frames * static_cast<size_t>(_agent_out_format.channels)) {
        ALOGE("ring buffer not enough space");
        assert(0);
        return false;
    }
    
    // 10ms unit
    const size_t slice = std::max<size_t>(1, static_cast<size_t>(_agent_out_format.sample_rate) / 100);
    
    for (size_t offset = 0; offset < frames; offset += slice) {
        const size_t n = std::min(slice, frames - offset);
        const int16_t* src = pcm + (offset * static_cast<size_t>(_agent_out_format.channels));
        size_t proc_frames = n;

        // resample
        if(_out_resampler != nullptr) {
            int16_t* in_ptr = const_cast<int16_t*>(src);
            int32_t resamples = _out_resampler->Resample(in_ptr, n, _out_resample_buf, sizeof(_out_resample_buf));
            src = (int16_t*)_out_resample_buf;
            proc_frames = static_cast<size_t>(resamples);

            if(_is_file_dump) {
                fwrite((uint8_t*)_out_resample_buf, sizeof(int16_t), proc_frames, _enc_resample_dump_file);
            }              
        }

        const int16_t* out = src;
          
        if(!_pcm_ring_buf->Write(out, proc_frames)) {
            ALOGE("pcm ring buffer write fail");
            return false;    
        }

        if(_is_file_dump) {
            fwrite((uint8_t*)out, sizeof(int16_t), proc_frames, _enc_upmix_dump_file);
        }         
    }

    return true;
}

void AgentClient::Clear() {
    std::lock_guard<std::mutex> lock(_mutex);
    if(_pcm_ring_buf != nullptr) {
        _pcm_ring_buf->Clear();
    }
}

uint32_t AgentClient::QueuedMs() const {
    const uint64_t total = _pcm_ring_buf->Size();
    const uint64_t frames = total / static_cast<uint64_t>(_rtc_out_format.channels);
    return static_cast<uint32_t>(frames * 1000 / static_cast<uint64_t>(_rtc_out_format.sample_rate));
}  

// RTCIngressAudioPacketSinkInterface method
void AgentClient::SetIngressFormat(const RTCAudioFormat& format) {
    _rtc_in_format = format;
    if(_rtc_in_format.codec == RTCAudioFormat::Codec::kOpus) {
        _audio_decoder = RTCCodecFactory::CreateAudioDecoder(RTCCodecFactory::RTC_AUDIO_CODEC_OPUS,
					                                        _rtc_in_format.sample_rate, 
                                                            _rtc_in_format.channels);    
    } 

    if(_rtc_in_format.sample_rate != _agent_in_format.sample_rate) {
        _in_resampler = std::make_unique<RTCAudioResampler>();
        _in_resampler->SetConfig(_rtc_in_format.sample_rate, 
                                _agent_in_format.sample_rate, _agent_in_format.channels);
    }
}

void AgentClient::OnAudioFrame(const RTCIngressAudioFrame& frame) {
#ifdef __PERFORMANCE_PROFILE_ENABLED__    
    const int64_t w0 = rtcprof::WallUs();
    const int64_t c0 = rtcprof::ThreadCpuUs();
#endif
    OnAudioFrameImpl(frame);

#ifdef __PERFORMANCE_PROFILE_ENABLED__    
     _prof_on_frame.Add(rtcprof::WallUs() - w0, rtcprof::ThreadCpuUs() - c0);

    if (_prof_on_frame.ReportDue()) {
        ALOGI("profile_trace %s", _prof_on_frame.TakeReport().c_str());
        ALOGI("profile_trace %s", _prof_dec.TakeReport().c_str());
    }     
#endif    
}

void AgentClient::OnAudioFrameImpl(const RTCIngressAudioFrame& frame) {
    if(_audio_decoder == nullptr) {
        return;
    }

    int32_t decode_samples = kMaxPcmBuffer/_rtc_in_format.channels;
    status_t sret;
    {
#ifdef __PERFORMANCE_PROFILE_ENABLED__
        RTCScopedSectionTimer dec_timer(_prof_dec);
#endif        
        sret = _audio_decoder->Decode((uint8_t*)frame.data, frame.size, _pcm_buf, sizeof(_pcm_buf), &decode_samples);
    }

    if(_is_file_dump) {
        fwrite(_pcm_buf, sizeof(int16_t), decode_samples*_rtc_in_format.channels, _0_dec_dump_file);
    }        
    
    if(sret == OK) {
        uint8_t *send_pcm = nullptr;
        int32_t samples = decode_samples;
        if(_rtc_in_format.channels > 1 && _agent_in_format.channels == 1) {
            RTCAudioChannelConverter::DownmixInterleavedToMono(_pcm_buf, decode_samples, 
                                        _rtc_in_format.channels, _down_mix_buf);
            send_pcm = (uint8_t*)_down_mix_buf;                                            
        } else {
            send_pcm = (uint8_t*)_pcm_buf;
        }

        if(_in_resampler != nullptr) {
            int16_t* tmp_pcm = (int16_t*)send_pcm;
            int32_t samples_10ms = _rtc_in_format.sample_rate/100;
            int32_t loops = decode_samples / samples_10ms;
            samples = 0;
            int32_t offset = 0;
            for(int32_t i = 0; i < loops; i++) {
                samples += _in_resampler->Resample(tmp_pcm + offset, samples_10ms,
                                                    _in_resample_buf + samples, kMaxPcmBuffer - samples);
                offset += samples_10ms;
            }

            send_pcm = (uint8_t*)_in_resample_buf;                                                    
        }
#ifdef __ENABLE_VAD__
        if(_vad != nullptr) {
            RTCVADInterface::VADActivity activity = _vad->VoiceActivity((int16_t*)send_pcm, samples);
            if(activity == RTCVADInterface::VADActivity::kActive) {
                _is_vad_active = true;
                _vad_passive_count = 0;
            } else {
                if(_is_vad_active == true) {
                    if(_vad_passive_count > kVadPassiveThresholdCount) {
                        _is_vad_active = false;
                    }
                    _vad_passive_count++;
                }
            }

            if(_is_vad_active == false) {
                return;
            }
        }
#endif        
        if(_is_file_dump) {
            fwrite(send_pcm, sizeof(int16_t), samples*_agent_in_format.channels, _after_dec_dump_file);
        }

        _session->SendAudioFrame(send_pcm, samples*_agent_in_format.channels*sizeof(int16_t));
    }
}

// RTCEgressAudioSourceInterface
void AgentClient::SetEgressFormat(const RTCAudioFormat& format) {
    _rtc_out_format = format;

    ALOGV("agent_out_formate sample_rate %d channels %d rtc_out_format sample_rate %d channels %d", 
        _agent_out_format.sample_rate, _agent_out_format.channels, _rtc_out_format.sample_rate, _rtc_out_format.channels);

    if(_rtc_out_format.sample_rate != _agent_out_format.sample_rate) {
        _out_resampler = std::make_unique<RTCAudioResampler>();
        _out_resampler->SetConfig(_agent_out_format.sample_rate, 
                                  _rtc_out_format.sample_rate, _agent_out_format.channels);                                        
    }

    const size_t ring_samples = static_cast<size_t>(_rtc_out_format.sample_rate) * kEgressRingMs / 1000;
    _pcm_ring_buf = std::make_unique<AgentPcmRingBuffer>(ring_samples * _rtc_out_format.channels);

    if(_rtc_out_format.codec == RTCAudioFormat::Codec::kOpus) {
        _audio_encoder = RTCCodecFactory::CreateAudioEncoder(RTCCodecFactory::RTC_AUDIO_CODEC_OPUS,
					                                        _rtc_out_format.sample_rate, 
                                                            1 /*_rtc_out_format.channels*/);        
    }

    if(_is_file_dump) {
        _debug_audio_decoder = RTCCodecFactory::CreateAudioDecoder(RTCCodecFactory::RTC_AUDIO_CODEC_OPUS,
					                                        _rtc_out_format.sample_rate, 
                                                            _rtc_out_format.channels);        
    }

    _is_set_egress_format.store(true);
}

bool AgentClient::PullEncodedFrame(uint8_t* buf, size_t capacity, size_t* size) {
#ifdef __PERFORMANCE_PROFILE_ENABLED__    
    const int64_t w0 = rtcprof::WallUs();
    const int64_t c0 = rtcprof::ThreadCpuUs();
#endif    

    bool ret = PullEncodedFrameImpl(buf, capacity, size);

 #ifdef __PERFORMANCE_PROFILE_ENABLED__   
    if(ret) {
        _prof_total.Add(rtcprof::WallUs() - w0, rtcprof::ThreadCpuUs() - c0);
 
        if (_prof_last_ok_us != 0 && (w0 - _prof_last_ok_us) < 100000) {
            _prof_interval.Add(w0 - _prof_last_ok_us);
        }
        _prof_last_ok_us = w0;
 
        if (_prof_total.ReportDue()) {
            ALOGI("profile_trace %s", _prof_total.TakeReport().c_str());
            ALOGI("profile_trace %s", _prof_enc.TakeReport().c_str());
            ALOGI("profile_trace %s", _prof_interval.TakeReport().c_str());
        }
    }
#endif

    return ret;
}

bool AgentClient::PullEncodedFrameImpl(uint8_t* buf, size_t capacity, size_t* size) {
    if(!_is_set_egress_format.load()) {
        return false;
    }

    const size_t frame_samples = (_rtc_out_format.sample_rate / 50);

    if(!_is_egressing) {
        if(QueuedMs() < kPrebufferMs) {
            return false;
        }
        _is_egressing = true;
    }

    if(_pcm_ring_buf->ReadExact(_out_egress_pcm_buf, frame_samples)) {

        if(_is_file_dump) {
            fwrite(_out_egress_pcm_buf, sizeof(int16_t), frame_samples, _after_ring_dump_file);
        }        

        if(_audio_encoder != nullptr) {
            int32_t enclen = 0;
            status_t sret; 

            {
#ifdef __PERFORMANCE_PROFILE_ENABLED__                
                RTCScopedSectionTimer enc_timer(_prof_enc);
#endif                
                sret = _audio_encoder->Encode((int16_t*)_out_egress_pcm_buf, frame_samples, buf, capacity, enclen);
            }
            
            if(sret != OK) {
                ALOGV("enc error");
                return false;
            }
            *size = enclen;

            if(_is_file_dump) {
                int16_t tmp_pcm_buf[kMaxPcmBuffer];
                int32_t decode_samples = kMaxPcmBuffer/_rtc_in_format.channels;
                status_t sret = _debug_audio_decoder->Decode((uint8_t*)buf, *size, tmp_pcm_buf, sizeof(tmp_pcm_buf), &decode_samples);
                fwrite(tmp_pcm_buf, sizeof(int16_t), decode_samples * _rtc_out_format.channels, _after_enc_dec_dump_file);
            }
        } else {
            std::memcpy(buf, (uint8_t*)_out_egress_pcm_buf,frame_samples*sizeof(int16_t));
            *size = frame_samples*sizeof(int16_t);
        }

        return true;
    }

    if(_pcm_ring_buf->Size() == 0) {
        _is_egressing = false;
        return false;
    }

    if(++_end_threshold_tick >= kEndTicks) {
        size_t got = _pcm_ring_buf->Read(_out_egress_pcm_buf, frame_samples);
        std::fill(_out_egress_pcm_buf + got, _out_egress_pcm_buf + frame_samples, 0);
        _end_threshold_tick = 0;
        _is_egressing = false;

        if(_audio_encoder != nullptr) {
            int32_t enclen = 0;
            status_t sret;
            {
#ifdef __PERFORMANCE_PROFILE_ENABLED__                
                RTCScopedSectionTimer enc_timer(_prof_enc);
#endif                
                sret = _audio_encoder->Encode((int16_t*)_out_egress_pcm_buf, frame_samples, buf, capacity, enclen);
            }             
            
            if(sret != OK) {
                ALOGV("enc error");
                return false;
            }
            *size = enclen;
            if(_is_file_dump) {
                int16_t tmp_pcm_buf[kMaxPcmBuffer];
                int32_t decode_samples = kMaxPcmBuffer/_rtc_in_format.channels;
                status_t sret = _debug_audio_decoder->Decode((uint8_t*)buf, *size, tmp_pcm_buf, sizeof(tmp_pcm_buf), &decode_samples);
                fwrite(tmp_pcm_buf, sizeof(int16_t), decode_samples * _rtc_out_format.channels, _after_enc_dec_dump_file);
            }            
        } else {
            std::memcpy(buf, (uint8_t*)_out_egress_pcm_buf, frame_samples*sizeof(int16_t));
            *size = frame_samples*sizeof(int16_t);
        }
        return true;
    }

    return false;
}

