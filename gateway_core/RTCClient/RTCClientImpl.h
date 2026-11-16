//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#ifndef __RTC_CLIENT_IMPL_H__
#define __RTC_CLIENT_IMPL_H__

#include <string>
#include <vector>
#include <limits>
#include <memory>

#include <utils/Errors.h>

#include "../RTCIceInfo/RTCIceInfo.h"
#include "../RTCPeerConnection/rtc_pc_src/api/RTCPeer.h"
#include "../RTCPeerConnection/rtc_pc_src/api/RTCPeerObserver.h"

#include "RTCClientInterface.h"
#include "RTCClientListener.h"

class RTCClientRtpReceiverInterface;
class RTCClientAudioSender;
class RTCAudioSenderSchedulerInterface;

class RTCClientImpl : public RTCClientInterface,
				  	  public webrtc::RTCPeerObserver 
{
public:
	static std::shared_ptr<RTCClientInterface> Create(const std::string &id, 
										 			  std::vector<RTCIceServerInfo>& rtcIceInfos,
									     			  RTCClientListener* listener,
									     			  RTCAudioSenderSchedulerInterface* audio_sender_scheduler);
	~RTCClientImpl();

public:
	static bool InitializeSSL();
	static bool CleanupSSL(); 

public:	
	// RTCClientInterface method
	webrtc::RTCPeer* PeerPtr() override;
	status_t CreatePeerConnection() override;
	status_t CreateAnswer() override;
	std::string& GetId() override;
	status_t SetRemoteDescription(const std::string &sdp) override;
	status_t SetRemoteCandidate(int32_t mline_idx, 
						  const std::string &mid, const std::string &candidate) override;
	status_t Close() override;
	RTCClientInterface::RTCClientState GetState() override;
	RTCAudioIngressSinkSetInterface* GetAudioIngressSetInterface() override;
	RTCAudioEgressSourceSetInterface* GetAudioEgressSetInterface() override;

private:
	//////////////////////////////////////////////////////////////////////////
	// webrtc::RTCNodeObserver method
	void OnAddStream(
		rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
	void OnRemoveStream(
		rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
	void OnIceConnectionChange(
		webrtc::PeerConnectionInterfaceDefs::IceConnectionState new_state) override;
	void OnIceCandidate(
		const webrtc::IceCandidateInterface *candidate) override;
	// for planB sdp		
	void OnAddTrack(
		rtc::scoped_refptr<webrtc::RTCRtpReceiverInterface> receiver,
		const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams) override;
	// for planB sdp		
	void OnAddSimulcastTrack(std::vector<rtc::scoped_refptr<webrtc::RTCRtpReceiverInterface>>& receivers,
		const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams) override;
	void OnTrack(
		rtc::scoped_refptr<webrtc::RTCRtpTransceiverInterface> transceiver) override;
	void OnRemoveTrack(
		rtc::scoped_refptr<webrtc::RTCRtpReceiverInterface> receiver) override;
	void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;

	void OnSdpCreateSuccess(webrtc::SessionDescriptionInterface *desc) override;
	void OnSdpCreateFailure(webrtc::RTCError error) override;
	void OnSdpSetSuccess(bool is_local, webrtc::SessionDescriptionInterface* desc) override;
	void OnSdpSetFailure(webrtc::RTCError error, bool is_local) override;

private:
	RTCClientImpl(const std::string &id, std::vector<RTCIceServerInfo>& rtcIceInfos, 
		    RTCClientListener* listener, RTCAudioSenderSchedulerInterface* audio_sender_scheduler);

private:
	void _on_pcm_data(const int16_t* data, size_t samples, 	int sample_rate, int channels);
	void _dump_transceiver(rtc::scoped_refptr<webrtc::RTCRtpTransceiverInterface> transceiver);

private:
	std::string _id;
	RTCClientListener* _listener;

	webrtc::PeerConnectionInterfaceDefs::IceServers _ice_servers;
	rtc::scoped_refptr<webrtc::RTCPeer>	_rtc_peer;
	std::unique_ptr<webrtc::SessionDescriptionInterface> _local_desc;
	std::shared_ptr<RTCClientRtpReceiverInterface> _audio_receiver;
	bool _is_first_connected = false;
	bool _is_frist_on_audio_track = false;
	rtc::scoped_refptr<webrtc::RTCRtpTransceiverInterface> _audio_transceiver;
	std::vector<RTCCandidateInfo>	_remote_candidates;
	RTCClientListener::RTCIceState _ice_state = RTCClientListener::RTCIceState::RTC_ICE_STATE_INIT;

	RTCClientInterface::RTCClientState _state;

	std::shared_ptr<RTCClientRtpReceiverInterface> _audio_receiver_interface;
	std::shared_ptr<RTCClientAudioSender> _audio_sender;

	RTCAudioSenderSchedulerInterface* _audio_sender_scheduler;

private:
    RTCClientImpl(const RTCClientImpl&) = delete;
    RTCClientImpl& operator=(const RTCClientImpl&) = delete;  
};


#endif // __RTC_CLIENT_IMPL_H__