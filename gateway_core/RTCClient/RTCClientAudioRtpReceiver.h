//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#ifndef __RTC_CLIENT_AUDIO_RTP_RECEIVER_H__
#define __RTC_CLIENT_AUDIO_RTP_RECEIVER_H__

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "api/rtp_parameters.h"
#include "modules/rtp_rtcp/source/rtp_packet_received.h"
#include "../RTCPeerConnection/rtc_pc_src/api/RTCRtpTransceiverInterface.h"

#include "RTCClientRtpReceiverInterface.h"

class RTCClientInterface;
class RTCAudioDecoderInterface;

const int32_t kMaxPcmBuffer = 2000;

class RTCClientAudioRtpReceiver : public RTCClientRtpReceiverInterface {
public:
	explicit RTCClientAudioRtpReceiver(rtc::scoped_refptr<webrtc::RTCRtpTransceiverInterface> transceiver,
									 RTCClientInterface* peer);
	~RTCClientAudioRtpReceiver();

public:
	// RTCClientRtpReceiverInterface method
	std::vector<webrtc::RtpParameters> getRtpParameters() override;
	absl::optional<webrtc::RtpHeaderExtensionMap> getRtpHeaderExtensionMap() override;
	const RTCClientInterface* getRTCClientInterface() const override;
	void close() override;

	// webrtc::RTCMediaReceivedPacketSink method
	void OnRtpPacket(const webrtc::RtpPacketReceived& packet) override;
	void SetRtpHeaderExtensionMap(absl::optional<webrtc::RtpHeaderExtensionMap> extMap) override;

	// RTCAudioIngressSinkSetInterface method
	void setIngressSink(RTCIngressAudioPacketSinkInterface* ingressSink) override;	

private:
	void _construct_rtp_parameters(webrtc::RtpParameters& parameters);
	void _dump_transceiver(rtc::scoped_refptr<webrtc::RTCRtpTransceiverInterface> transceiver);

private:
	typedef struct {
		std::string mime_type;
		std::string name;
		int32_t sampling_rate;
		int32_t num_channels;
	} RTCRtpCodecParameter;	

private:
	int32_t _init_pt = -1;
	bool _is_set_parameter = false;
	rtc::scoped_refptr<webrtc::RTCRtpTransceiverInterface> _transceiver;
	webrtc::RtpParameters  _rtp_parameters;
	absl::optional<webrtc::RtpHeaderExtensionMap> _rtp_extensions;
	RTCClientInterface* _client;
	std::unordered_map<int32_t, RTCRtpCodecParameter> _codec_parameters;
	std::unique_ptr<RTCAudioDecoderInterface> _audio_decoder;
	int32_t _sampling_rate = -1;
	int32_t _num_channels = -1;
	int16_t _pcm_buf[kMaxPcmBuffer];
	
	std::mutex _sink_lock;
	RTCIngressAudioPacketSinkInterface* _ingress_sink = nullptr;

private:
    RTCClientAudioRtpReceiver(const RTCClientAudioRtpReceiver&) = delete;
    RTCClientAudioRtpReceiver& operator=(const RTCClientAudioRtpReceiver&) = delete;  
};

#endif //__RTC_CLIENT_AUDIO_RTP_RECEIVER_H__