#ifndef __AGENT_CLIENT_H__
#define __AGENT_CLIENT_H__

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <cstring>

#include "../RTCClient/RTCIngressAudioPacketSinkInterface.h"
#include "../RTCClient/RTCEgressAudioSourceInterface.h"
#include "../RTCAudioUtils/RTCAudioResampler.h"
#include "AgentEgressAudioSinkInterface.h"
#include "AgentClientInterface.h"
#include "AgentPcmRingBuffer.h"
#include "GRPCAgentSessionAPI.h"
#include "../RTCUtils/RTCSectionProfiler.h"
#include "../RTCVad/RTCVADFactory.h"

class RTCAudioDecoderInterface;
class RTCAudioEncoderInterface;

class AgentClient : public AgentClientInterface,
                    public AgentEgressAudioSinkInterface, 
                    public RTCIngressAudioPacketSinkInterface,
                    public RTCEgressAudioSourceInterface {
public:
    static std::shared_ptr<AgentClientInterface> Create(const std::string& endpoint, const std::string& id);
    ~AgentClient();

public:
    int32_t Open() override;
    void Close() override;
    RTCIngressAudioPacketSinkInterface* GetIngressPtr() override;
    RTCEgressAudioSourceInterface* GetEgressPtr() override;

public:
    // AgentEgressAudioSinkInterface method
    bool PushPcm(const int16_t* pcm, size_t samples) override;
    void Clear() override;
    uint32_t QueuedMs() const override;  

    // RTCIngressAudioPacketSinkInterface method
    void SetIngressFormat(const RTCAudioFormat& format) override;
    void OnAudioFrame(const RTCIngressAudioFrame& frame) override;

    // RTCEgressAudioSourceInterface
    void SetEgressFormat(const RTCAudioFormat& format) override;
    bool PullEncodedFrame(uint8_t* buf, size_t capacity, size_t* size) override;    

private:
    static constexpr int32_t kMaxPcmBuffer = 4096;
    const uint32_t kEgressRingMs = 5000;
    const size_t kEgressMarginFrames = 64;    
    const uint32_t  kEndTicks = 3;
    const uint32_t kPrebufferMs = 40;
    const uint32_t kVadPassiveThresholdCount = 10;  // 20ms sample * 10

private:
    AgentClient(const std::string& endpoint, const std::string& id);
    bool PullEncodedFrameImpl(uint8_t* buf, size_t capacity, size_t* size);
    void OnAudioFrameImpl(const RTCIngressAudioFrame& frame);

private:
    std::string _endpoint;
    std::string _id;
    RTCAudioFormat _agent_in_format;
    RTCAudioFormat _agent_out_format;

    RTCAudioFormat _rtc_in_format;
    RTCAudioFormat _rtc_out_format;

    int16_t _pcm_buf[kMaxPcmBuffer];
    int16_t _down_mix_buf[kMaxPcmBuffer];
    int16_t _up_mix_buf[kMaxPcmBuffer];
    int16_t _in_resample_buf[kMaxPcmBuffer];
    int16_t _out_resample_buf[kMaxPcmBuffer];
    int16_t _out_egress_pcm_buf[kMaxPcmBuffer];

    std::unique_ptr<RTCAudioDecoderInterface> _audio_decoder;
    std::unique_ptr<RTCAudioEncoderInterface> _audio_encoder;

    std::shared_ptr<GRPCAgentSession> _session;

    std::mutex _mutex;
    std::unique_ptr<RTCAudioResampler> _in_resampler;
    std::unique_ptr<RTCAudioResampler> _out_resampler;
    std::unique_ptr<AgentPcmRingBuffer> _pcm_ring_buf;


    std::atomic<bool> _is_set_egress_format{false};
    bool _is_egressing = false;
    uint32_t _end_threshold_tick = 0;

    std::unique_ptr<RTCVADInterface> _vad;
    bool _is_vad_active = false;
    uint32_t _vad_passive_count = 0;

    ////////////////////////////////////////////////////////////////
    // profile
#ifdef __PERFORMANCE_PROFILE_ENABLED__
    RTCSectionStat _prof_total{"pull_total"};      
    RTCSectionStat _prof_enc{"opus_encode"};        
    RTCSectionStat _prof_interval{"tick_interval"}; 
    int64_t _prof_last_ok_us = 0;    

    RTCSectionStat _prof_on_frame{"on_frame_total"};
    RTCSectionStat _prof_dec{"opus_decode"};  
#endif    
    ////////////////////////////////////////////////////////////////  

    ////////////////////////////////////////////////////////////
    // for debug
    int64_t _prev_packet_ms = -1;
    int64_t _prev_send_ms = -1;
    std::unique_ptr<RTCAudioDecoderInterface> _debug_audio_decoder;

    // dump
    bool _is_file_dump = false;
    FILE* _0_dec_dump_file = nullptr;
    FILE* _after_dec_dump_file = nullptr;
    FILE* _before_enc_dump_file = nullptr;
    FILE* _enc_resample_dump_file = nullptr;
    FILE* _enc_upmix_dump_file = nullptr;
    FILE* _after_ring_dump_file = nullptr;
    FILE* _after_enc_dec_dump_file = nullptr;
    ///////////////////////////////////////////////////////////

private:
    AgentClient(const AgentClient&) = delete;
    AgentClient& operator=(const AgentClient&) = delete;    
};

#endif // __AGENT_CLIENT_H__